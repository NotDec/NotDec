#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path


LABEL_RE = re.compile(r"// block_(\d+):")
GOTO_RE = re.compile(r"// goto block_(\d+)")


def count_goto_kinds(text: str) -> tuple[int, int, int, int]:
    """Split rendered gotos into total / fallthrough / dangling / real-jump.

    A fallthrough goto is immediately followed (ignoring closing braces) by its
    own target label, so dropping it would not change control flow.  A dangling
    goto targets a label that is never rendered.  The rest are real jumps the
    structured output cannot express.
    """
    lines = text.splitlines()
    labels = {int(m.group(1)) for m in LABEL_RE.finditer(text)}
    total = 0
    fallthrough = 0
    dangling = 0
    real = 0
    for index, line in enumerate(lines):
        match = GOTO_RE.search(line)
        if match is None:
            continue
        total += 1
        target = int(match.group(1))
        if target not in labels:
            dangling += 1
            continue
        lookahead = index + 1
        while lookahead < len(lines) and lines[lookahead].strip() in ("", "}", "};"):
            lookahead += 1
        if lookahead < len(lines) and re.search(
            rf"// block_{target}:", lines[lookahead]
        ):
            fallthrough += 1
        else:
            real += 1
    return total, fallthrough, dangling, real


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
    goto_fallthrough = 0
    goto_dangling = 0
    goto_real = 0
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
        total, fallthrough, dangling, real = count_goto_kinds(text)
        gotos += total
        goto_fallthrough += fallthrough
        goto_dangling += dangling
        goto_real += real

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
        (
            "real_jump_occurrences",
            goto_real,
            budget["max_real_jump_occurrences"],
        ),
        (
            "dangling_goto_occurrences",
            goto_dangling,
            budget["max_dangling_goto_occurrences"],
        ),
    ]
    print(
        "goto breakdown: total={} fallthrough={} dangling={} real={}".format(
            gotos, goto_fallthrough, goto_dangling, goto_real
        )
    )
    ok = gen_failures == 0
    for name, actual, limit in checks:
        status = "PASS" if actual <= limit else "FAIL"
        print(f"[{status}] {name}: actual={actual} budget={limit}")
        ok = ok and actual <= limit
    print(f"Summary: cases={len(cases)} gen_failures={gen_failures}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
