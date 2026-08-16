#!/usr/bin/env python3
"""Run a small Bench2 source-IR performance and type-result smoke suite."""

from __future__ import annotations

import argparse
import concurrent.futures
import copy
import datetime as dt
import hashlib
import json
import os
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
DEFAULT_BENCH_ROOT = Path("/sn640/NotDec-Exp/Bench2/source-ir/ir")
DEFAULT_NOTDEC = PROJECT_ROOT / "build-relwithdebinfo-20260731/bin/notdec"
DEFAULT_LLVM_AS = PROJECT_ROOT / "llvm-22.1.0.obj/bin/llvm-as"
DEFAULT_CASES = ("fortune", "ffplay", "vsftpd", "ngircd", "memcached")

# The quick set intentionally excludes wrk and libuv: their long-double and
# generic-buffer behavior make them poor default signals for the current
# pointer-type work. Larger projects remain available through --cases.
CASE_INPUTS = {
    "fortune": "fortune/fortune.bc",
    "wrk": "wrk/wrk.bc",
    "ffplay": "ffplay/ffplay.bc",
    "vsftpd": "vsftpd/vsftpd.bc",
    "ngircd": "ngircd/ngircd.bc",
    "libuv": "libuv/libuv.bc",
    "memcached": "memcached/memcached.bc",
    "lighttpd": "lighttpd/lighttpd.bc",
    "tmux": "tmux/tmux.bc",
    "redis-server": "redis-server/redis-server.bc",
}

# These files are the direct type-recovery products used for A/B comparison.
# Output IR is verified separately and is not treated as a type oracle.
TYPE_RESULT_FILES = {
    "value_types": "work/ValueTypes.txt",
    "value_htypes": "work/ValueHTypes.txt",
    "important_htypes": "work/ImportantHTypes.txt",
    "var_origins": "work/VarOrigins.txt",
    "debug_info_value_types": "eval/DebugInfoValueTypes.txt",
    "bad_unions": "eval/bad_unions.jsonl",
    "fragmented_types": "eval/fragmented_types.jsonl",
}

ACTIVE_PROCESSES: dict[str, subprocess.Popen[str]] = {}
ACTIVE_PROCESSES_LOCK = threading.Lock()
PRINT_LOCK = threading.Lock()


def eprint(message: str) -> None:
    with PRINT_LOCK:
        print(message, file=sys.stderr, flush=True)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_revision(path: Path) -> str | None:
    process = subprocess.run(
        ["git", "-C", str(path), "rev-parse", "HEAD"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        check=False,
    )
    if process.returncode != 0:
        return None
    return process.stdout.strip()


def parse_cases(value: str) -> list[str]:
    cases: list[str] = []
    for name in value.split(","):
        name = name.strip()
        if not name:
            continue
        if name not in CASE_INPUTS:
            raise argparse.ArgumentTypeError(
                f"unknown case {name!r}; use --list-cases to see valid names"
            )
        if name not in cases:
            cases.append(name)
    if not cases:
        raise argparse.ArgumentTypeError("--cases must select at least one case")
    return cases


def parse_extra_env(values: list[str]) -> dict[str, str]:
    result: dict[str, str] = {}
    for value in values:
        if "=" not in value:
            raise ValueError(f"--env expects NAME=VALUE, got {value!r}")
        name, env_value = value.split("=", 1)
        if not name.startswith("NOTDEC_"):
            raise ValueError(f"--env only accepts NOTDEC_* variables, got {name!r}")
        if name == "NOTDEC_BINARYSUB_THREADS":
            raise ValueError("use --threads instead of setting NOTDEC_BINARYSUB_THREADS")
        result[name] = env_value
    return result


def prepare_output_dir(value: str | None) -> Path:
    if value is None:
        return Path(tempfile.mkdtemp(prefix="notdec-source-ir-perf-"))

    output_dir = Path(value).resolve()
    if output_dir.exists() and not output_dir.is_dir():
        raise ValueError(f"output path is not a directory: {output_dir}")
    output_dir.mkdir(parents=True, exist_ok=True)
    if any(output_dir.iterdir()):
        raise ValueError(f"output directory is not empty: {output_dir}")
    return output_dir


def load_baseline(value: str | None) -> tuple[Path | None, dict[str, Any] | None]:
    if value is None:
        return None, None
    path = Path(value).resolve()
    if path.is_dir():
        path = path / "report.json"
    if not path.is_file():
        raise ValueError(f"baseline report does not exist: {path}")
    with path.open("r", encoding="utf-8") as stream:
        report = json.load(stream)
    if report.get("schema_version") != 1:
        raise ValueError(f"unsupported baseline schema in {path}")
    return path, report


def validate_baseline_config(
    baseline: dict[str, Any] | None,
    cases: list[str],
    jobs: int,
    threads: int,
    timeout_seconds: int,
    extra_env: dict[str, str],
) -> None:
    if baseline is None:
        return
    current = {
        "cases": cases,
        "jobs": jobs,
        "threads": threads,
        "timeout_seconds": timeout_seconds,
        "extra_env": extra_env,
        "canonicalize_parallel": extra_env.get(
            "NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL", "0"
        ),
        "strict_type_artifact_match": threads == 1,
    }
    baseline_config = baseline.get("config", {})
    mismatches = [
        key for key, value in current.items() if baseline_config.get(key) != value
    ]
    if mismatches:
        names = ", ".join(mismatches)
        raise ValueError(f"baseline uses a different run configuration: {names}")


def parse_time_stats(path: Path) -> dict[str, Any]:
    stats: dict[str, Any] = {}
    if not path.is_file():
        return stats
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        key, separator, value = line.partition("=")
        if not separator:
            continue
        if key == "wall_seconds":
            stats[key] = float(value)
        elif key in {"max_rss_kib", "exit_status"}:
            stats[key] = int(value)
    return stats


def terminate_process_group(process: subprocess.Popen[str]) -> None:
    if process.poll() is not None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
        process.wait(timeout=5)
    except ProcessLookupError:
        return
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait()


def stop_all_active_processes() -> None:
    with ACTIVE_PROCESSES_LOCK:
        processes = list(ACTIVE_PROCESSES.values())
    for process in processes:
        terminate_process_group(process)


def extract_eval_oracle(summary: dict[str, Any] | None) -> dict[str, Any] | None:
    if summary is None:
        return None
    oracle = copy.deepcopy(summary)
    performance = oracle.get("performance")
    if isinstance(performance, dict):
        # Runtime and peak memory are measurements. The remaining counters
        # describe the recovered graph and should stay equal in an A/B run.
        performance.pop("wall_ms", None)
        performance.pop("peak_rss_mb", None)
    return oracle


def percent_delta(current: float | int | None, baseline: float | int | None) -> float | None:
    if current is None or baseline in (None, 0):
        return None
    return (float(current) - float(baseline)) * 100.0 / float(baseline)


def run_case(
    name: str,
    input_path: Path,
    input_sha256: str,
    output_root: Path,
    notdec: Path,
    llvm_as: Path,
    threads: int,
    timeout_seconds: int,
    extra_env: dict[str, str],
) -> dict[str, Any]:
    case_dir = output_root / name
    work_dir = case_dir / "work"
    eval_dir = case_dir / "eval"
    case_dir.mkdir(parents=True)
    work_dir.mkdir()
    eval_dir.mkdir()

    stdout_path = case_dir / "notdec.stdout.log"
    stderr_path = case_dir / "notdec.stderr.log"
    time_path = case_dir / "time.txt"
    output_ir = case_dir / "out.ll"

    notdec_args = [
        str(notdec),
        str(input_path),
        "--tr-level=2",
        "--merge-struct-ptr-load-store",
        "--gen-work-dir",
        "--fast-work-dir",
        f"--work-dir={work_dir}",
        f"--merge-eval-dir={eval_dir}",
        "-o",
        str(output_ir),
    ]
    time_format = "wall_seconds=%e\nmax_rss_kib=%M\nexit_status=%x"
    command = [
        "setarch",
        os.uname().machine,
        "-R",
        "/usr/bin/time",
        "-f",
        time_format,
        "-o",
        str(time_path),
        *notdec_args,
    ]
    (case_dir / "command.txt").write_text(
        shlex.join(command) + "\n", encoding="utf-8"
    )

    # Remove inherited NOTDEC_* settings so an interactive debug shell cannot
    # silently change the benchmark. Explicit --env values are recorded below.
    environment = {
        key: value for key, value in os.environ.items() if not key.startswith("NOTDEC_")
    }
    environment["LC_ALL"] = "C"
    environment["NOTDEC_BINARYSUB_THREADS"] = str(threads)
    # Parallel canonicalization can assign different recursive groups under a
    # different worker schedule even with ASLR disabled. The smoke suite keeps
    # that stage serial so HType text is a useful A/B oracle; simplify still
    # uses --threads and remains representative for the current hotspot.
    environment["NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL"] = "0"
    environment.update(extra_env)

    eprint(f"[start] {name}")
    observed_start = time.monotonic()
    timed_out = False
    with stdout_path.open("w", encoding="utf-8") as stdout, stderr_path.open(
        "w", encoding="utf-8"
    ) as stderr:
        process = subprocess.Popen(
            command,
            stdout=stdout,
            stderr=stderr,
            text=True,
            env=environment,
            start_new_session=True,
        )
        with ACTIVE_PROCESSES_LOCK:
            ACTIVE_PROCESSES[name] = process
        try:
            return_code = process.wait(timeout=timeout_seconds)
        except subprocess.TimeoutExpired:
            timed_out = True
            terminate_process_group(process)
            return_code = process.returncode
        finally:
            with ACTIVE_PROCESSES_LOCK:
                ACTIVE_PROCESSES.pop(name, None)
    observed_wall_seconds = time.monotonic() - observed_start

    time_stats = parse_time_stats(time_path)
    verifier_log = case_dir / "llvm-as.log"
    verifier_return_code: int | None = None
    if return_code == 0 and output_ir.is_file():
        with verifier_log.open("w", encoding="utf-8") as log:
            verifier = subprocess.run(
                [str(llvm_as), str(output_ir), "-o", os.devnull],
                stdout=log,
                stderr=subprocess.STDOUT,
                text=True,
                check=False,
            )
        verifier_return_code = verifier.returncode

    eval_summary_path = eval_dir / "merge-eval-summary.json"
    eval_summary: dict[str, Any] | None = None
    if eval_summary_path.is_file():
        with eval_summary_path.open("r", encoding="utf-8") as stream:
            eval_summary = json.load(stream)

    artifact_hashes: dict[str, str] = {}
    missing_artifacts: list[str] = []
    for artifact_name, relative_path in TYPE_RESULT_FILES.items():
        artifact_path = case_dir / relative_path
        if artifact_path.is_file():
            artifact_hashes[artifact_name] = sha256_file(artifact_path)
        else:
            missing_artifacts.append(relative_path)

    errors: list[str] = []
    if timed_out:
        errors.append(f"timed out after {timeout_seconds} seconds")
    elif return_code != 0:
        errors.append(f"notdec exited with status {return_code}")
    if return_code == 0 and verifier_return_code != 0:
        errors.append(f"llvm-as exited with status {verifier_return_code}")
    if return_code == 0 and eval_summary is None:
        errors.append("merge-eval-summary.json is missing")
    if return_code == 0 and missing_artifacts:
        errors.append("missing type-result artifacts: " + ", ".join(missing_artifacts))
    if return_code == 0 and "wall_seconds" not in time_stats:
        errors.append("/usr/bin/time did not produce wall_seconds")

    measurement = {
        "wall_seconds": time_stats.get("wall_seconds", observed_wall_seconds),
        "observed_wall_seconds": round(observed_wall_seconds, 6),
        "max_rss_kib": time_stats.get("max_rss_kib"),
    }
    result = {
        "status": "pass" if not errors else ("timeout" if timed_out else "fail"),
        "input": {"path": str(input_path), "sha256": input_sha256},
        "command": command,
        "exit_code": return_code,
        "timed_out": timed_out,
        "verifier": {
            "ok": verifier_return_code == 0,
            "exit_code": verifier_return_code,
        },
        "measurement": measurement,
        "eval_summary": eval_summary,
        "eval_oracle": extract_eval_oracle(eval_summary),
        "artifact_sha256": artifact_hashes,
        "missing_artifacts": missing_artifacts,
        "errors": errors,
    }
    eprint(
        f"[done]  {name}: {result['status']} "
        f"wall={measurement['wall_seconds']:.2f}s "
        f"rss={measurement['max_rss_kib'] or 0}KiB"
    )
    return result


def add_baseline_comparison(
    report: dict[str, Any], baseline: dict[str, Any] | None
) -> bool:
    if baseline is None:
        return True

    comparison_clean = True
    strict_type_match = report["config"]["strict_type_artifact_match"]
    baseline_cases = baseline.get("results", {})
    for name, result in report["results"].items():
        old = baseline_cases.get(name)
        if old is None:
            result["comparison"] = {"comparable": False, "reason": "case missing"}
            comparison_clean = False
            continue

        input_match = result["input"]["sha256"] == old.get("input", {}).get("sha256")
        eval_match = result.get("eval_oracle") == old.get("eval_oracle")
        current_artifacts = result.get("artifact_sha256", {})
        old_artifacts = old.get("artifact_sha256", {})
        artifact_names = sorted(set(current_artifacts) | set(old_artifacts))
        artifact_matches = {
            artifact_name: current_artifacts.get(artifact_name)
            == old_artifacts.get(artifact_name)
            for artifact_name in artifact_names
        }
        artifacts_match = all(artifact_matches.values())
        comparison = {
            "comparable": input_match,
            "input_match": input_match,
            "wall_delta_percent": percent_delta(
                result["measurement"].get("wall_seconds"),
                old.get("measurement", {}).get("wall_seconds"),
            ),
            "max_rss_delta_percent": percent_delta(
                result["measurement"].get("max_rss_kib"),
                old.get("measurement", {}).get("max_rss_kib"),
            ),
            "eval_oracle_match": eval_match,
            "type_artifacts_match": artifacts_match,
            "type_artifact_matches": artifact_matches,
            "type_artifact_match_required": strict_type_match,
        }
        result["comparison"] = comparison
        if not input_match or not eval_match or (strict_type_match and not artifacts_match):
            comparison_clean = False
    return comparison_clean


def format_float(value: float | None, suffix: str = "") -> str:
    if value is None:
        return "-"
    return f"{value:+.1f}{suffix}"


def write_summary(report: dict[str, Any], output_dir: Path) -> None:
    columns = [
        "case",
        "status",
        "wall_s",
        "rss_mib",
        "bad_unions",
        "fragmented_nodes",
        "wall_delta",
        "rss_delta",
        "eval_match",
        "htype_match",
    ]
    rows: list[list[str]] = []
    for name in report["config"]["cases"]:
        result = report["results"][name]
        measurement = result["measurement"]
        summary = result.get("eval_summary") or {}
        comparison = result.get("comparison", {})
        rss_kib = measurement.get("max_rss_kib")
        rows.append(
            [
                name,
                result["status"],
                f"{measurement['wall_seconds']:.2f}",
                f"{rss_kib / 1024.0:.1f}" if rss_kib is not None else "-",
                str(summary.get("wrong_merge", {}).get("bad_unions", "-")),
                str(summary.get("fragmentation", {}).get("fragmented_nodes", "-")),
                format_float(comparison.get("wall_delta_percent"), "%"),
                format_float(comparison.get("max_rss_delta_percent"), "%"),
                str(comparison.get("eval_oracle_match", "-")).lower(),
                str(comparison.get("type_artifacts_match", "-")).lower(),
            ]
        )

    widths = [len(column) for column in columns]
    for row in rows:
        for index, value in enumerate(row):
            widths[index] = max(widths[index], len(value))
    lines = ["  ".join(value.ljust(widths[index]) for index, value in enumerate(columns))]
    lines.append("  ".join("-" * width for width in widths))
    lines.extend(
        "  ".join(value.ljust(widths[index]) for index, value in enumerate(row))
        for row in rows
    )
    summary_text = "\n".join(lines) + "\n"
    (output_dir / "summary.txt").write_text(summary_text, encoding="utf-8")
    print(summary_text, end="")


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Run Bench2 source-IR type recovery in parallel and record performance, "
            "merge-eval metrics, and HType hashes."
        )
    )
    parser.add_argument("--notdec", type=Path, default=DEFAULT_NOTDEC)
    parser.add_argument("--llvm-as", type=Path, default=DEFAULT_LLVM_AS)
    parser.add_argument("--bench-root", type=Path, default=DEFAULT_BENCH_ROOT)
    parser.add_argument("--output-dir")
    parser.add_argument(
        "--cases",
        type=parse_cases,
        default=list(DEFAULT_CASES),
        help="comma-separated case names (default: %(default)s)",
    )
    parser.add_argument("--jobs", type=int, default=2, help="parallel projects")
    parser.add_argument(
        "--threads", type=int, default=8, help="binarysub threads per project"
    )
    parser.add_argument(
        "--timeout-seconds", type=int, default=900, help="timeout per project"
    )
    parser.add_argument(
        "--baseline", help="old report.json or its containing run directory"
    )
    parser.add_argument(
        "--env",
        action="append",
        default=[],
        metavar="NOTDEC_NAME=VALUE",
        help="explicit extra NOTDEC_* setting; may be repeated",
    )
    parser.add_argument(
        "--list-cases", action="store_true", help="print available cases and exit"
    )
    return parser


def main() -> int:
    parser = build_argument_parser()
    args = parser.parse_args()
    if args.list_cases:
        for name, relative_path in CASE_INPUTS.items():
            marker = " (default)" if name in DEFAULT_CASES else ""
            print(f"{name:14} {relative_path}{marker}")
        return 0

    if args.jobs <= 0 or args.threads <= 0 or args.timeout_seconds <= 0:
        parser.error("--jobs, --threads, and --timeout-seconds must be positive")
    if not args.notdec.is_file() or not os.access(args.notdec, os.X_OK):
        parser.error(f"notdec is not executable: {args.notdec}")
    if not args.llvm_as.is_file() or not os.access(args.llvm_as, os.X_OK):
        parser.error(f"llvm-as is not executable: {args.llvm_as}")
    if shutil.which("setarch") is None:
        parser.error("setarch is required to disable ASLR for repeatable runs")
    if not Path("/usr/bin/time").is_file():
        parser.error("/usr/bin/time is required")

    try:
        extra_env = parse_extra_env(args.env)
        output_dir = prepare_output_dir(args.output_dir)
        baseline_path, baseline = load_baseline(args.baseline)
        validate_baseline_config(
            baseline,
            args.cases,
            args.jobs,
            args.threads,
            args.timeout_seconds,
            extra_env,
        )
    except (ValueError, OSError, json.JSONDecodeError) as error:
        parser.error(str(error))

    case_inputs: dict[str, tuple[Path, str]] = {}
    for name in args.cases:
        input_path = (args.bench_root / CASE_INPUTS[name]).resolve()
        if not input_path.is_file():
            parser.error(f"input does not exist: {input_path}")
        case_inputs[name] = (input_path, sha256_file(input_path))

    report: dict[str, Any] = {
        "schema_version": 1,
        "generated_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "output_dir": str(output_dir),
        "baseline_report": str(baseline_path) if baseline_path else None,
        "config": {
            "cases": args.cases,
            "jobs": args.jobs,
            "threads": args.threads,
            "timeout_seconds": args.timeout_seconds,
            "extra_env": extra_env,
            "canonicalize_parallel": extra_env.get(
                "NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL", "0"
            ),
            # Multi-threaded simplify can produce run-to-run HType text
            # variants even when merge-eval is identical. A single-thread run
            # is deterministic and therefore uses every type-file hash as a
            # hard comparison gate.
            "strict_type_artifact_match": args.threads == 1,
            "aslr_disabled": True,
            "bench_root": str(args.bench_root.resolve()),
            "notdec": str(args.notdec.resolve()),
            "notdec_sha256": sha256_file(args.notdec),
            "llvm_as": str(args.llvm_as.resolve()),
            "project_revision": git_revision(PROJECT_ROOT),
            "binarysub_revision": git_revision(PROJECT_ROOT / "external/binarysub"),
        },
        "results": {},
    }

    suite_start = time.monotonic()
    interrupted = False
    executor = concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs)
    futures: dict[concurrent.futures.Future[dict[str, Any]], str] = {}
    try:
        for name in args.cases:
            input_path, input_sha256 = case_inputs[name]
            future = executor.submit(
                run_case,
                name,
                input_path,
                input_sha256,
                output_dir,
                args.notdec.resolve(),
                args.llvm_as.resolve(),
                args.threads,
                args.timeout_seconds,
                extra_env,
            )
            futures[future] = name
        for future in concurrent.futures.as_completed(futures):
            name = futures[future]
            report["results"][name] = future.result()
    except KeyboardInterrupt:
        interrupted = True
        eprint("interrupted: stopping active case process groups")
        stop_all_active_processes()
    finally:
        executor.shutdown(wait=True, cancel_futures=True)

    report["suite_wall_seconds"] = round(time.monotonic() - suite_start, 6)
    if interrupted:
        return 130

    # Keep report ordering equal to the requested case order even though the
    # worker futures complete out of order.
    report["results"] = {
        name: report["results"][name] for name in args.cases
    }
    comparison_clean = add_baseline_comparison(report, baseline)
    runs_clean = all(result["status"] == "pass" for result in report["results"].values())
    report["all_runs_passed"] = runs_clean
    report["baseline_comparison_clean"] = comparison_clean if baseline else None

    (output_dir / "report.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    write_summary(report, output_dir)
    print(f"run directory: {output_dir}")
    print(f"suite wall: {report['suite_wall_seconds']:.2f}s")
    if baseline is not None and args.threads > 1:
        changed_types = [
            name
            for name, result in report["results"].items()
            if not result.get("comparison", {}).get("type_artifacts_match", True)
        ]
        if changed_types:
            print(
                "note: multi-thread HType hashes changed for "
                + ", ".join(changed_types)
                + "; merge-eval remains the hard gate. Use --threads 1 for "
                "strict type-file comparison."
            )

    if not runs_clean:
        return 1
    if baseline is not None and not comparison_clean:
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
