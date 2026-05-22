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


NONPAYABLE_DEFINE_RE = re.compile(
    r"^define\b.*!notdec\.solidity\.nonpayable\b", re.MULTILINE
)


def format_command(cmd: list[str]) -> str:
    return " ".join(subprocess.list2cmdline([part]) for part in cmd)


def resolve_path(base: Path, path_str: str) -> Path:
    path = Path(path_str)
    if path.is_absolute():
        return path
    return (base / path).resolve()


def expand_template(value: str, *, manifest_dir: Path, project_root: Path, workdir: Path) -> str:
    return value.format(
        manifest_dir=str(manifest_dir),
        project_root=str(project_root),
        workdir=str(workdir),
    )


def expand_args(
    raw_args: list[str],
    *,
    manifest_dir: Path,
    project_root: Path,
    workdir: Path,
) -> list[str]:
    return [
        expand_template(
            arg,
            manifest_dir=manifest_dir,
            project_root=project_root,
            workdir=workdir,
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


def write_log(log_path: Path, sections: list[str]) -> None:
    log_path.write_text("\n\n".join(sections) + "\n")


def count_nonpayable_functions(path: Path) -> int:
    return len(NONPAYABLE_DEFINE_RE.findall(path.read_text()))


def count_metadata_uses(path: Path, metadata_name: str) -> int:
    text = path.read_text()
    return text.count(f"!{metadata_name}")


def rewrite_marker_name(metadata_name: str) -> str:
    prefix = "notdec.solidity."
    if metadata_name.startswith(prefix):
        metadata_name = metadata_name[len(prefix) :]
    return "notdec_solidity_rewrite_" + metadata_name.replace(".", "_")


def count_rewrite_markers(path: Path, metadata_name: str) -> int:
    text = path.read_text()
    return text.count(f"call void @{rewrite_marker_name(metadata_name)}(")


def count_hidden_markers(path: Path) -> int:
    text = path.read_text()
    return text.count("call void @notdec_solidity_rewrite_hidden(")


def count_hidden_metadata(path: Path) -> int:
    text = path.read_text()
    return text.count("!notdec.solidity.rewrite_hidden.")


def write_compare_report(
    *,
    report_path: Path,
    expected_counts: dict[str, int],
    actual_counts: dict[str, int],
) -> bool:
    passed = actual_counts == expected_counts
    lines = ["PASS" if passed else "FAIL"]
    for key in sorted(expected_counts):
        lines.append(f"  expected_{key}={expected_counts[key]}")
        lines.append(f"  actual_{key}={actual_counts.get(key, 0)}")
    report_path.write_text("\n".join(lines) + "\n")
    return passed


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
    llvm_as = project_root / "llvm-22.1.0.obj/bin/llvm-as"

    if not llvm_as.exists():
        print(f"missing llvm-as: {llvm_as}", file=sys.stderr)
        return 1

    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True, exist_ok=True)

    manifest = json.loads(manifest_path.read_text())
    env = os.environ.copy()
    env.update(manifest.get("env", {}))
    default_args = manifest.get("default_args", [])

    passed = 0
    failed = 0
    print(f"Running suite: {manifest.get('suite', manifest_path.stem)}")

    for case in manifest["cases"]:
        name = case["name"]
        case_workdir = workdir / name
        case_workdir.mkdir(parents=True, exist_ok=True)
        input_path = resolve_path(manifest_dir, case["input"])
        output_ll = case_workdir / "out.ll"
        output_bc = case_workdir / "out.bc"
        compare_txt = case_workdir / "compare.txt"
        log_path = case_workdir / "run.log"

        notdec_cmd = [
            args.binary,
            str(input_path),
            "-o",
            str(output_ll),
            *expand_args(
                default_args,
                manifest_dir=manifest_dir,
                project_root=project_root,
                workdir=workdir,
            ),
            *expand_args(
                case.get("args", []),
                manifest_dir=manifest_dir,
                project_root=project_root,
                workdir=workdir,
            ),
        ]
        notdec_proc, notdec_section = run_command(
            title="notdec", command=notdec_cmd, cwd=project_root, env=env
        )
        log_sections = [notdec_section]

        case_ok = (
            notdec_proc.returncode == 0
            and output_ll.exists()
            and output_ll.stat().st_size > 0
        )

        if case_ok:
            llvm_as_cmd = [str(llvm_as), str(output_ll), "-o", str(output_bc)]
            llvm_as_proc, llvm_as_section = run_command(
                title="llvm-as", command=llvm_as_cmd, cwd=project_root, env=env
            )
            log_sections.append(llvm_as_section)
            case_ok = llvm_as_proc.returncode == 0 and output_bc.exists()

        if case_ok:
            expected_counts = {
                "nonpayable_functions": case["expected_nonpayable_functions"]
            }
            expected_counts.update(case.get("expected_metadata_counts", {}))
            expect_rewrite_markers = case.get(
                "expect_rewrite_markers",
                manifest.get("expect_rewrite_markers", False),
            )
            expect_rewrite_hidden = case.get(
                "expect_rewrite_hidden",
                manifest.get("expect_rewrite_hidden", False),
            )
            if expect_rewrite_markers:
                for metadata_name, count in case.get(
                    "expected_metadata_counts", {}
                ).items():
                    expected_counts[
                        f"rewrite_marker:{rewrite_marker_name(metadata_name)}"
                    ] = count
            if expect_rewrite_hidden:
                hidden_count = sum(case.get("expected_metadata_counts", {}).values())
                expected_counts["rewrite_hidden_markers"] = hidden_count
                expected_counts["rewrite_hidden_metadata"] = hidden_count
            actual_counts = {
                "nonpayable_functions": count_nonpayable_functions(output_ll)
            }
            for metadata_name in case.get("expected_metadata_counts", {}):
                actual_counts[metadata_name] = count_metadata_uses(
                    output_ll, metadata_name
                )
                if expect_rewrite_markers:
                    actual_counts[
                        f"rewrite_marker:{rewrite_marker_name(metadata_name)}"
                    ] = count_rewrite_markers(output_ll, metadata_name)
            if expect_rewrite_hidden:
                actual_counts["rewrite_hidden_markers"] = count_hidden_markers(
                    output_ll
                )
                actual_counts["rewrite_hidden_metadata"] = count_hidden_metadata(
                    output_ll
                )
            compare_ok = write_compare_report(
                report_path=compare_txt,
                expected_counts=expected_counts,
                actual_counts=actual_counts,
            )
            log_sections.append(compare_txt.read_text())
            case_ok = compare_ok

        write_log(log_path, log_sections)

        if case_ok:
            passed += 1
            print(f"[PASS ] {name}")
        else:
            failed += 1
            print(f"[FAIL ] {name}")
            print(f"        log: {log_path}")
            if compare_txt.exists():
                print(f"        compare: {compare_txt}")

    print(f"Summary: {passed} passed, {failed} failed")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
