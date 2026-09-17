#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path


def run_command(command: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def first_error_line(log: str) -> str:
    for line in log.splitlines():
        if line.startswith("Error"):
            return line
    return log.splitlines()[0] if log.splitlines() else "<no output>"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True)
    parser.add_argument("--cases", required=True)
    parser.add_argument("--budget", required=True)
    parser.add_argument("--project-root", required=True)
    parser.add_argument("--workdir", required=True)
    parser.add_argument(
        "--solc",
        default="",
        help="Optional solc binary; when set every generated .sol must compile",
    )
    args = parser.parse_args()

    cases_dir = Path(args.cases).resolve()
    budget_path = Path(args.budget).resolve()
    project_root = Path(args.project_root).resolve()
    workdir = Path(args.workdir).resolve()
    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True, exist_ok=True)

    budget = json.loads(budget_path.read_text())
    cases = sorted(cases_dir.glob("*.ll"))
    if not cases:
        print(f"no cases under {cases_dir}", file=sys.stderr)
        return 1

    unresolved = 0
    condition_todo = 0
    gotos = 0
    gen_failures = 0
    compile_failures = 0

    for case in cases:
        output_sol = workdir / (case.stem + ".sol")
        command = [
            args.binary,
            str(case),
            "-o",
            str(output_sol),
            "--tr-level=2",
        ]
        process = run_command(command, project_root)
        log = process.stdout
        if process.returncode != 0 or not output_sol.exists():
            gen_failures += 1
            (workdir / (case.stem + ".log")).write_text(
                f"$ {' '.join(command)}\nexit={process.returncode}\n\n{log}"
            )
            print(f"[FAIL ] {case.stem}: notdec exit={process.returncode}")
            continue

        text = output_sol.read_text()
        unresolved += text.count("TODO: unresolved value")
        condition_todo += text.count("false /* TODO")
        gotos += text.count("// goto block_")

        if args.solc:
            solc_command = [args.solc, "--bin", str(output_sol)]
            solc_process = run_command(solc_command, project_root)
            if solc_process.returncode != 0:
                compile_failures += 1
                (workdir / (case.stem + ".solc.log")).write_text(
                    solc_process.stdout
                )
                print(
                    f"[FAIL ] {case.stem}: solc: "
                    f"{first_error_line(solc_process.stdout)}"
                )

    checks = [
        ("compile_failures", compile_failures, budget.get("max_compile_failures", 0)),
        (
            "unresolved_value_occurrences",
            unresolved,
            budget["max_unresolved_value_occurrences"],
        ),
        (
            "condition_todo_occurrences",
            condition_todo,
            budget["max_condition_todo_occurrences"],
        ),
        ("goto_occurrences", gotos, budget["max_goto_occurrences"]),
    ]
    ok = gen_failures == 0
    for name, actual, limit in checks:
        status = "PASS" if actual <= limit else "FAIL"
        print(f"[{status}] {name}: actual={actual} budget={limit}")
        ok = ok and actual <= limit
    print(f"Summary: cases={len(cases)} gen_failures={gen_failures}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
