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


def resolve_path(base: Path, path_str: str) -> Path:
    path = Path(path_str)
    if path.is_absolute():
        return path
    return (base / path).resolve()


def run_command(command: list[str], cwd: Path, env: dict[str, str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=cwd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def count_text(path: Path, needle: str) -> int:
    return path.read_text().count(needle)


def count_skip_reasons(text: str) -> dict[str, int]:
    metadata = dict(re.findall(r"!(\d+) = !\{!\"([^\"]+)\"\}", text))
    counts: dict[str, int] = {}
    for metadata_id in re.findall(r"!notdec\.solidity\.selector_outline_skipped !(\d+)", text):
        reason = metadata.get(metadata_id, "unknown")
        counts[f"skip_reason:{reason}"] = counts.get(f"skip_reason:{reason}", 0) + 1
    return counts


def count_outlined_functions(text: str) -> int:
    return len(
        re.findall(
            r"define internal void @(?:public__)?notdec_solidity_selector_inline\.",
            text,
        )
    )


def count_outline_calls(text: str) -> int:
    return len(
        re.findall(
            r"call void @(?:public__)?notdec_solidity_selector_inline\.",
            text,
        )
    )


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

    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True, exist_ok=True)

    manifest = json.loads(manifest_path.read_text())
    env = os.environ.copy()
    default_args = manifest.get("default_args", [])

    passed = 0
    failed = 0
    print(f"Running suite: {manifest.get('suite', manifest_path.stem)}")

    for case in manifest["cases"]:
        name = case["name"]
        case_workdir = workdir / name
        case_workdir.mkdir(parents=True, exist_ok=True)
        output_ll = case_workdir / "out.ll"
        output_bc = case_workdir / "out.bc"
        log_path = case_workdir / "run.log"

        cmd = [
            args.binary,
            str(resolve_path(manifest_dir, case["input"])),
            "-o",
            str(output_ll),
            *default_args,
            *case.get("args", []),
        ]
        notdec = run_command(cmd, project_root, env)
        logs = ["## notdec\n" + notdec.stdout]
        ok = notdec.returncode == 0 and output_ll.exists()

        if ok:
            llvm_as_proc = run_command(
                [str(llvm_as), str(output_ll), "-o", str(output_bc)],
                project_root,
                env,
            )
            logs.append("## llvm-as\n" + llvm_as_proc.stdout)
            ok = llvm_as_proc.returncode == 0 and output_bc.exists()

        if ok:
            output_text = output_ll.read_text()
            checks = {
                "outlined_functions": count_outlined_functions(output_text),
                "outline_calls": count_outline_calls(output_text),
                "outlined_metadata": output_text.count(
                    "!notdec.solidity.selector_outlined_body"
                ),
                "selector_inlined_body": output_text.count(
                    "!notdec.solidity.selector_inlined_body"
                ),
                "skipped_metadata": output_text.count(
                    "!notdec.solidity.selector_outline_skipped"
                ),
            }
            checks.update(count_skip_reasons(output_text))
            expected = case["expected_counts"]
            lines = ["PASS"]
            for key, value in expected.items():
                actual = checks.get(key, 0)
                if actual != value:
                    ok = False
                    lines[0] = "FAIL"
                lines.append(f"  expected_{key}={value}")
                lines.append(f"  actual_{key}={actual}")
            logs.append("## compare\n" + "\n".join(lines) + "\n")

        log_path.write_text("\n\n".join(logs))
        if ok:
            passed += 1
            print(f"[PASS ] {name}")
        else:
            failed += 1
            print(f"[FAIL ] {name}")
            print(f"        log: {log_path}")

    print(f"Summary: {passed} passed, {failed} failed")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
