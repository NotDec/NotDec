#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path


DEFINE_RE = re.compile(r"^define\b.*@([A-Za-z0-9_.$-]+)\(", re.MULTILINE)
DECLARE_RE = re.compile(r"^declare\b.*@([A-Za-z0-9_.$-]+)\(", re.MULTILINE)
GLOBAL_RE = re.compile(r"^@([A-Za-z0-9_.$-]+)\s*=", re.MULTILINE)
TARGET_TRIPLE_RE = re.compile(r'^target triple = "([^"]+)"$', re.MULTILINE)


def format_command(cmd: list[str]) -> str:
    return " ".join(subprocess.list2cmdline([part]) for part in cmd)


def resolve_path(base: Path, path_str: str | None) -> Path | None:
    if path_str is None:
        return None
    path = Path(path_str)
    if path.is_absolute():
        return path
    return (base / path).resolve()


def load_manifest(path: Path) -> dict:
    return json.loads(path.read_text())


def format_log_section(
    title: str,
    command: list[str],
    process: subprocess.CompletedProcess[str],
) -> str:
    return (
        f"## {title}\n"
        f"$ {format_command(command)}\n"
        f"exit={process.returncode}\n\n"
        f"{process.stdout}"
    )


def write_log(log_path: Path, sections: list[str]) -> None:
    log_path.write_text("\n\n".join(sections) + "\n")


def run_command(
    *,
    title: str,
    command: list[str],
    cwd: Path,
    env: dict[str, str],
) -> tuple[subprocess.CompletedProcess[str], str]:
    process = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return process, format_log_section(title, command, process)


def notdec_command_succeeded(
    process: subprocess.CompletedProcess[str],
    ir_output_path: Path,
) -> bool:
    if process.returncode != 0:
        return False
    if "IR parsing failed:" in process.stdout:
        return False
    return ir_output_path.exists() and ir_output_path.stat().st_size > 0


def parse_ir_summary(path: Path) -> dict:
    text = path.read_text()
    triple_match = TARGET_TRIPLE_RE.search(text)
    return {
        "target_triple": triple_match.group(1) if triple_match else None,
        "defines": sorted(set(DEFINE_RE.findall(text))),
        "declares": sorted(set(DECLARE_RE.findall(text))),
        "globals": sorted(set(GLOBAL_RE.findall(text))),
    }


def compare_module_summary(
    *,
    output_path: Path,
    expected_path: Path,
    ground_truth_path: Path | None,
    report_json_path: Path,
    report_text_path: Path,
) -> bool:
    actual = parse_ir_summary(output_path)
    expected = json.loads(expected_path.read_text())

    ground_truth = None
    if ground_truth_path is not None:
        ground_truth = parse_ir_summary(ground_truth_path)

    mismatches: list[str] = []
    expected_triple = expected.get("target_triple")
    if expected_triple and actual["target_triple"] != expected_triple:
        mismatches.append(
            f"target triple mismatch: expected {expected_triple}, got {actual['target_triple']}"
        )

    for key in ("required_globals", "required_defines", "required_declares"):
        actual_key = key.removeprefix("required_")
        actual_values = set(actual[actual_key])
        for item in expected.get(key, []):
            if item not in actual_values:
                mismatches.append(f"missing {actual_key[:-1]}: {item}")

    for key in ("forbidden_globals", "forbidden_defines", "forbidden_declares"):
        actual_key = key.removeprefix("forbidden_")
        actual_values = set(actual[actual_key])
        for item in expected.get(key, []):
            if item in actual_values:
                mismatches.append(f"unexpected {actual_key[:-1]}: {item}")

    if ground_truth is not None:
        for key in (
            "ground_truth_required_globals",
            "ground_truth_required_defines",
            "ground_truth_required_declares",
        ):
            truth_key = key.removeprefix("ground_truth_required_")
            truth_values = set(ground_truth[truth_key])
            for item in expected.get(key, []):
                if item not in truth_values:
                    mismatches.append(f"ground truth missing {truth_key[:-1]}: {item}")

    report = {
        "passed": not mismatches,
        "mismatches": mismatches,
        "actual": actual,
        "ground_truth": str(ground_truth_path) if ground_truth_path is not None else None,
        "expected": str(expected_path),
    }
    report_json_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    lines = ["PASS" if report["passed"] else "FAIL"]
    for mismatch in mismatches:
        lines.append(f"  {mismatch}")
    report_text_path.write_text("\n".join(lines) + "\n")
    return report["passed"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--project-root", required=True)
    parser.add_argument("--workdir", required=True)
    args = parser.parse_args()

    manifest_path = Path(args.manifest).resolve()
    manifest_dir = manifest_path.parent
    project_root = Path(args.project_root).resolve()
    workdir = Path(args.workdir).resolve()
    workdir.mkdir(parents=True, exist_ok=True)

    manifest = load_manifest(manifest_path)
    env = os.environ.copy()
    env.update(manifest.get("env", {}))

    counters = {
        "pass": 0,
        "xfail": 0,
        "skip": 0,
        "fail": 0,
        "xpass": 0,
    }

    suite_name = manifest.get("suite", manifest_path.stem)
    default_args = manifest.get("default_args", [])

    print(f"Running suite: {suite_name}")

    for case in manifest["cases"]:
        name = case["name"]
        status = case.get("status", "pass")
        expected_path = resolve_path(manifest_dir, case.get("expected"))
        if expected_path is None:
            raise ValueError(f"case {name} is missing expected path")
        ground_truth_path = resolve_path(manifest_dir, case.get("ground_truth"))
        input_path = resolve_path(manifest_dir, case["input"])
        output_path = workdir / f"{name}.out.ll"
        report_json_path = workdir / f"{name}.compare.json"
        report_text_path = workdir / f"{name}.compare.txt"
        log_path = workdir / f"{name}.log"

        if status == "skip":
            counters["skip"] += 1
            print(f"[SKIP ] {name}")
            continue

        for path in (output_path, report_json_path, report_text_path):
            if path.exists():
                path.unlink()

        command = [
            args.binary,
            str(input_path),
            "-o",
            str(output_path),
            *default_args,
            *case.get("args", []),
        ]
        process, section = run_command(title="notdec", command=command, cwd=project_root, env=env)
        log_sections = [section]

        notdec_ok = notdec_command_succeeded(process, output_path)
        compare_ok = False
        if notdec_ok:
            compare_ok = compare_module_summary(
                output_path=output_path,
                expected_path=expected_path,
                ground_truth_path=ground_truth_path,
                report_json_path=report_json_path,
                report_text_path=report_text_path,
            )
            log_sections.append(report_text_path.read_text())

        write_log(log_path, log_sections)
        succeeded = notdec_ok and compare_ok

        if status == "pass":
            if succeeded:
                counters["pass"] += 1
                print(f"[PASS ] {name}")
            else:
                counters["fail"] += 1
                print(f"[FAIL ] {name}")
                print(f"        log: {log_path}")
                if report_text_path.exists():
                    print(f"        compare:  {report_text_path}")
            continue

        if status == "xfail":
            if succeeded:
                counters["xpass"] += 1
                print(f"[XPASS] {name}")
                print(f"        log: {log_path}")
            else:
                counters["xfail"] += 1
                reason = case.get("reason")
                suffix = f" ({reason})" if reason else ""
                print(f"[XFAIL] {name}{suffix}")
            continue

        counters["fail"] += 1
        print(f"[FAIL ] {name}")
        print(f"        unknown status: {status}")

    print(
        "Summary: "
        f"{counters['pass']} passed, "
        f"{counters['xfail']} xfailed, "
        f"{counters['skip']} skipped, "
        f"{counters['xpass']} xpassed, "
        f"{counters['fail']} failed"
    )

    return 0 if counters["fail"] == 0 and counters["xpass"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
