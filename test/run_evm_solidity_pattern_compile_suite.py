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
FUNCTION_RE = re.compile(r"^\s*(function|constructor|fallback|receive|modifier)\b")
# evm2llvm outlines shared code into private__* helpers.  The backend renders
# the recoverable ones as Solidity functions and their call sites; these two
# counts are coverage floors so the recovery cannot silently disappear.
HELPER_DEF_RE = re.compile(r"^    function (private__[A-Za-z0-9_]*)\(", re.M)
HELPER_NAME_RE = re.compile(r"\b(private__[A-Za-z0-9_]*)\(")


def split_functions(text: str) -> list[list[str]]:
    """Split the contract text into per-function line lists.

    Rendered block labels restart at 0 in every function, so goto targets must
    be resolved inside the same function: a file-global match would let a goto
    in one function resolve against an unrelated label in another and hide real
    jumps.
    """
    functions: list[list[str]] = []
    current: list[str] | None = None
    depth = 0
    for line in text.splitlines():
        if current is None:
            if not FUNCTION_RE.match(line):
                continue
            current = [line]
            depth = line.count("{") - line.count("}")
            if depth <= 0 and "{" in line:
                functions.append(current)
                current = None
            continue
        current.append(line)
        depth += line.count("{") - line.count("}")
        if depth <= 0:
            functions.append(current)
            current = None
    if current:
        functions.append(current)
    return functions


def count_goto_kinds(text: str) -> tuple[int, int, int, int]:
    """Split rendered gotos into total / fallthrough / dangling / real-jump.

    A fallthrough goto is immediately followed (ignoring closing braces) by its
    own target label, so dropping it would not change control flow.  A dangling
    goto targets a label that is never rendered.  The rest are real jumps the
    structured output cannot express.  Labels and gotos are matched inside one
    generated function, because block labels restart at 0 per function.
    """
    total = 0
    fallthrough = 0
    dangling = 0
    real = 0
    for lines in split_functions(text):
        labels = {int(m.group(1)) for line in lines for m in LABEL_RE.finditer(line)}
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
    # The IR pipeline is the default for this suite: the legacy codegen keeps
    # locals in EVM stack slots and rejects a body whose live locals exceed the
    # 16 slots DUP/SWAP can reach ("Stack too deep"), while the generated
    # decompiler output legitimately has many locals (SSA demotion slots, tuple
    # returns).  The front-end checks -- types, scopes, mutability: every error
    # class this suite has actually caught -- are identical under both
    # pipelines, so the validation net is unaffected.
    parser.add_argument(
        "--solc-arg",
        action="append",
        default=None,
        help="Extra solc argument; defaults to --via-ir",
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
    helper_functions = 0
    helper_call_sites = 0
    switch_todo = 0
    recover_body_todo = 0
    event_signature_todo = 0

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
        # Switch statements and whole-body fallbacks are control-flow gaps too;
        # they used to be invisible to the ratchet because it only counted the
        # "false /* TODO" condition form.
        switch_todo += text.count("/* TODO: switch")
        # Two different gaps: a body the structurer only partially recovered,
        # and an event whose ABI (parameter list) could not be recovered.
        recover_body_todo += text.count("TODO: recover remaining body")
        recover_body_todo += text.count("/* TODO: recover body */")
        event_signature_todo += text.count("TODO: recover event signature")
        total, fallthrough, dangling, real = count_goto_kinds(text)
        gotos += total
        goto_fallthrough += fallthrough
        goto_dangling += dangling
        goto_real += real
        definitions = len(HELPER_DEF_RE.findall(text))
        helper_functions += definitions
        helper_call_sites += len(HELPER_NAME_RE.findall(text)) - definitions

        if args.solc:
            # The front-end checks (types, scopes, mutability -- every error
            # class this suite has ever caught) are identical under both codegen
            # pipelines, so the artifact only has to be accepted by one of them:
            # try the legacy pipeline first (about 25x faster), and fall back to
            # the IR pipeline when a codegen limit such as "Stack too deep" (or
            # the uncaught StackTooDeepError the IR pipeline can throw) hits.
            if args.solc_arg is not None:
                pipelines = [list(args.solc_arg)]
            else:
                # The IR pipeline needs its optimizer enabled for the deepest
                # bodies (solc's own recommendation for Stack too deep), so that
                # is the last resort.
                pipelines = [[], ["--via-ir"], ["--via-ir", "--optimize"]]
            solc_process = None
            first_log = ""
            for extra in pipelines:
                solc_command = [args.solc, "--bin", *extra, str(output_sol)]
                solc_process = run_command(solc_command, project_root)
                if solc_process.returncode == 0:
                    break
                if not first_log:
                    first_log = solc_process.stdout
            if solc_process.returncode != 0:
                compile_failures += 1
                (workdir / (case.stem + ".solc.log")).write_text(
                    first_log or solc_process.stdout
                )
                print(
                    f"[FAIL ] {case.stem}: solc: "
                    f"{first_error_line(first_log or solc_process.stdout)}"
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
        (
            "switch_todo_occurrences",
            switch_todo,
            budget.get("max_switch_todo_occurrences", 0),
        ),
        (
            "recover_body_todo_occurrences",
            recover_body_todo,
            budget.get("max_recover_body_todo_occurrences", 0),
        ),
        (
            "event_signature_todo_occurrences",
            event_signature_todo,
            budget.get("max_event_signature_todo_occurrences", 0),
        ),
    ]
    print(
        "goto breakdown: total={} fallthrough={} dangling={} real={}".format(
            gotos, goto_fallthrough, goto_dangling, goto_real
        )
    )
    floors = [
        (
            "helper_functions",
            helper_functions,
            budget.get("min_helper_functions", 0),
        ),
        (
            "helper_call_sites",
            helper_call_sites,
            budget.get("min_helper_call_sites", 0),
        ),
    ]
    ok = gen_failures == 0
    for name, actual, limit in checks:
        status = "PASS" if actual <= limit else "FAIL"
        print(f"[{status}] {name}: actual={actual} budget={limit}")
        ok = ok and actual <= limit
    for name, actual, floor in floors:
        status = "PASS" if actual >= floor else "FAIL"
        print(f"[{status}] {name}: actual={actual} floor={floor}")
        ok = ok and actual >= floor
    print(f"Summary: cases={len(cases)} gen_failures={gen_failures}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
