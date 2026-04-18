#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


DEFINE_RE = re.compile(r"^define\b.*@([A-Za-z0-9_.$-]+)\(", re.MULTILINE)
DECLARE_RE = re.compile(r"^declare\b.*@([A-Za-z0-9_.$-]+)\(", re.MULTILINE)
GLOBAL_RE = re.compile(r"^@([A-Za-z0-9_.$-]+)\s*=", re.MULTILINE)
TARGET_TRIPLE_RE = re.compile(r'^target triple = "([^"]+)"$', re.MULTILINE)
PNDIFF_KIND_RE = re.compile(r"^kind:\s+(Add|Sub)\s*$", re.MULTILINE)
PNDIFF_OVERRIDE_WARNING_RE = re.compile(
    r"^Warning: skip MLsub .* pndiff", re.MULTILINE
)
STACK_POINTER_SELECTED_RE = re.compile(r"^Selected stack pointer:\s*(.+)$", re.MULTILINE)
STACK_POINTER_ERROR_RE = re.compile(
    r"^(ERROR: .+? func: ([A-Za-z0-9_.$-]+))$",
    re.MULTILINE,
)
MEMSET_HEADER_RE = re.compile(
    r"^ ============== MemsetMatcher \[([^\]]+)\] ===============$",
    re.MULTILINE,
)
MEMSET_MERGE_RE = re.compile(r"^Merging\b", re.MULTILINE)
PNDIFF_CLEAR_MESSAGE = "No residual Add/Sub constraints with unknown state after solve."


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


def expand_template(
    value: str,
    *,
    manifest_dir: Path,
    project_root: Path,
    workdir: Path,
    case_workdir: Path,
) -> str:
    return value.format(
        manifest_dir=str(manifest_dir),
        project_root=str(project_root),
        workdir=str(workdir),
        case_workdir=str(case_workdir),
    )


def expand_args(
    raw_args: list[str],
    *,
    manifest_dir: Path,
    project_root: Path,
    workdir: Path,
    case_workdir: Path,
) -> list[str]:
    return [
        expand_template(
            arg,
            manifest_dir=manifest_dir,
            project_root=project_root,
            workdir=workdir,
            case_workdir=case_workdir,
        )
        for arg in raw_args
    ]


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


def parse_pndiff_summary(path: Path) -> dict:
    if not path.exists():
        return {
            "exists": False,
            "path": str(path),
            "residual_constraints": 0,
            "override_warnings": [],
            "clear_message": False,
        }

    text = path.read_text()
    override_warnings = PNDIFF_OVERRIDE_WARNING_RE.findall(text)
    clear_message = PNDIFF_CLEAR_MESSAGE in text
    residual_constraints = len(PNDIFF_KIND_RE.findall(text))
    if clear_message:
        residual_constraints = 0

    return {
        "exists": True,
        "path": str(path),
        "residual_constraints": residual_constraints,
        "override_warnings": override_warnings,
        "clear_message": clear_message,
    }


def parse_recovery_pass_summary(path: Path) -> dict:
    if not path.exists():
        return {
            "exists": False,
            "path": str(path),
            "selected_stack_pointer": None,
            "stack_pointer_errors": [],
            "stack_pointer_error_functions": [],
            "memset_functions": [],
            "memset_merge_count": 0,
        }

    text = path.read_text()
    selected_stack_pointer_match = STACK_POINTER_SELECTED_RE.search(text)
    stack_pointer_errors = STACK_POINTER_ERROR_RE.findall(text)
    memset_functions = MEMSET_HEADER_RE.findall(text)

    return {
        "exists": True,
        "path": str(path),
        "selected_stack_pointer": (
            selected_stack_pointer_match.group(1) if selected_stack_pointer_match else None
        ),
        "stack_pointer_errors": [line for line, _ in stack_pointer_errors],
        "stack_pointer_error_functions": [func for _, func in stack_pointer_errors],
        "memset_functions": sorted(set(memset_functions)),
        "memset_merge_count": len(MEMSET_MERGE_RE.findall(text)),
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


def compare_workdir_logs(
    *,
    case_workdir: Path,
    expected_path: Path,
    report_json_path: Path,
    report_text_path: Path,
) -> bool:
    expected = json.loads(expected_path.read_text())
    pndiff = parse_pndiff_summary(case_workdir / "PNDiff.warn.txt")
    recovery = parse_recovery_pass_summary(case_workdir / "01-recovery-passes.log")

    mismatches: list[str] = []

    if not pndiff["exists"]:
        mismatches.append(f"missing workdir log: {pndiff['path']}")
    if not recovery["exists"]:
        mismatches.append(f"missing workdir log: {recovery['path']}")

    max_residual_constraints = expected.get("max_residual_pndiff_constraints")
    if max_residual_constraints is not None:
        if pndiff["residual_constraints"] > max_residual_constraints:
            mismatches.append(
                "too many residual Add/Sub constraints: "
                f"expected <= {max_residual_constraints}, got {pndiff['residual_constraints']}"
            )

    required_stack_pointer_substring = expected.get("required_stack_pointer_substring")
    if required_stack_pointer_substring is not None:
        selected_stack_pointer = recovery["selected_stack_pointer"]
        if selected_stack_pointer is None:
            mismatches.append("missing selected stack pointer entry in 01-recovery-passes.log")
        elif required_stack_pointer_substring not in selected_stack_pointer:
            mismatches.append(
                "selected stack pointer mismatch: "
                f"expected substring {required_stack_pointer_substring!r}, "
                f"got {selected_stack_pointer!r}"
            )

    max_stack_pointer_errors = expected.get("max_stack_pointer_errors")
    if max_stack_pointer_errors is not None:
        actual_error_count = len(recovery["stack_pointer_errors"])
        if actual_error_count > max_stack_pointer_errors:
            mismatches.append(
                "too many stack pointer recovery errors: "
                f"expected <= {max_stack_pointer_errors}, got {actual_error_count}"
            )

    allowed_error_functions = expected.get("allowed_stack_pointer_error_functions")
    if allowed_error_functions is not None:
        allowed = set(allowed_error_functions)
        unexpected_functions = sorted(
            {func for func in recovery["stack_pointer_error_functions"] if func not in allowed}
        )
        if unexpected_functions:
            mismatches.append(
                "unexpected stack pointer error functions: "
                + ", ".join(unexpected_functions)
            )

    required_memset_functions = expected.get("required_memset_functions", [])
    actual_memset_functions = set(recovery["memset_functions"])
    for func in required_memset_functions:
        if func not in actual_memset_functions:
            mismatches.append(f"missing MemsetMatcher section for function: {func}")

    min_memset_merge_count = expected.get("min_memset_merge_count")
    if min_memset_merge_count is not None:
        if recovery["memset_merge_count"] < min_memset_merge_count:
            mismatches.append(
                "too few MemsetMatcher merges: "
                f"expected >= {min_memset_merge_count}, got {recovery['memset_merge_count']}"
            )

    report = {
        "passed": not mismatches,
        "mismatches": mismatches,
        "actual": {
            "pndiff": pndiff,
            "recovery": recovery,
        },
        "expected": str(expected_path),
    }
    report_json_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    lines = ["PASS" if report["passed"] else "FAIL"]
    lines.append(
        f"  residual_pndiff_constraints={pndiff['residual_constraints']}"
    )
    lines.append(
        f"  stack_pointer_errors={len(recovery['stack_pointer_errors'])}: "
        + ", ".join(recovery["stack_pointer_error_functions"])
        if recovery["stack_pointer_errors"]
        else "  stack_pointer_errors=0"
    )
    lines.append(
        f"  memset_merge_count={recovery['memset_merge_count']}"
    )
    lines.append(
        "  memset_functions="
        + (", ".join(recovery["memset_functions"]) if recovery["memset_functions"] else "<none>")
    )
    for mismatch in mismatches:
        lines.append(f"  {mismatch}")
    report_text_path.write_text("\n".join(lines) + "\n")
    return report["passed"]


def compare_case(
    *,
    output_path: Path,
    expected_path: Path,
    ground_truth_path: Path | None,
    case_workdir: Path,
    report_json_path: Path,
    report_text_path: Path,
) -> bool:
    expected = json.loads(expected_path.read_text())
    kind = expected.get("kind", "module-summary")

    if kind == "module-summary":
        return compare_module_summary(
            output_path=output_path,
            expected_path=expected_path,
            ground_truth_path=ground_truth_path,
            report_json_path=report_json_path,
            report_text_path=report_text_path,
        )

    if kind == "workdir-log":
        return compare_workdir_logs(
            case_workdir=case_workdir,
            expected_path=expected_path,
            report_json_path=report_json_path,
            report_text_path=report_text_path,
        )

    raise ValueError(f"unsupported lifting oracle kind: {kind}")


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
        case_workdir = workdir / name
        output_path = case_workdir / "out.ll"
        report_json_path = case_workdir / "compare.json"
        report_text_path = case_workdir / "compare.txt"
        log_path = case_workdir / "run.log"

        if status == "skip":
            counters["skip"] += 1
            print(f"[SKIP ] {name}")
            continue

        if case_workdir.exists():
            shutil.rmtree(case_workdir)
        case_workdir.mkdir(parents=True, exist_ok=True)

        command = [
            args.binary,
            str(input_path),
            "-o",
            str(output_path),
            *expand_args(
                default_args,
                manifest_dir=manifest_dir,
                project_root=project_root,
                workdir=workdir,
                case_workdir=case_workdir,
            ),
            *expand_args(
                case.get("args", []),
                manifest_dir=manifest_dir,
                project_root=project_root,
                workdir=workdir,
                case_workdir=case_workdir,
            ),
        ]
        process, section = run_command(title="notdec", command=command, cwd=project_root, env=env)
        log_sections = [section]

        notdec_ok = notdec_command_succeeded(process, output_path)
        compare_ok = False
        if notdec_ok:
            compare_ok = compare_case(
                output_path=output_path,
                expected_path=expected_path,
                ground_truth_path=ground_truth_path,
                case_workdir=case_workdir,
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
