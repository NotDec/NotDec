#!/usr/bin/env python3

from __future__ import annotations

import argparse
import difflib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


def resolve_path(base: Path, path_str: str) -> Path:
    path = Path(path_str)
    if path.is_absolute():
        return path
    return (base / path).resolve()


def format_command(command: list[str]) -> str:
    return " ".join(subprocess.list2cmdline([part]) for part in command)


def run_command(
    command: list[str], cwd: Path, env: dict[str, str]
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=cwd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def normalized_lines(path: Path) -> list[str]:
    text = path.read_text()
    lines = [line.rstrip() for line in text.splitlines()]
    while lines and lines[-1] == "":
        lines.pop()
    return [line + "\n" for line in lines]


def write_log(
    log_path: Path,
    command: list[str],
    process: subprocess.CompletedProcess[str],
    diff: list[str],
) -> None:
    sections = [
        "## notdec",
        f"$ {format_command(command)}",
        f"exit={process.returncode}",
        "",
        process.stdout,
    ]
    if diff:
        sections.extend(["", "## diff", *diff])
    log_path.write_text("\n".join(sections))


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

    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True, exist_ok=True)

    manifest = json.loads(manifest_path.read_text())
    default_args = manifest.get("default_args", [])
    env = os.environ.copy()
    env.update(manifest.get("env", {}))

    passed = 0
    failed = 0
    print(f"Running suite: {manifest.get('suite', manifest_path.stem)}")

    for case in manifest["cases"]:
        name = case["name"]
        case_workdir = workdir / name
        case_workdir.mkdir(parents=True, exist_ok=True)
        output_sol = case_workdir / "out.sol"
        log_path = case_workdir / "run.log"

        command = [
            args.binary,
            str(resolve_path(manifest_dir, case["input"])),
            "-o",
            str(output_sol),
            *default_args,
            *case.get("args", []),
        ]
        notdec = run_command(command, project_root, env)

        diff: list[str] = []
        ok = notdec.returncode == 0 and output_sol.exists()
        if ok:
            expected_path = resolve_path(manifest_dir, case["expected"])
            expected = normalized_lines(expected_path)
            actual = normalized_lines(output_sol)
            diff = list(
                difflib.unified_diff(
                    expected,
                    actual,
                    fromfile=str(expected_path),
                    tofile=str(output_sol),
                )
            )
            ok = not diff

        write_log(log_path, command, notdec, diff)
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
