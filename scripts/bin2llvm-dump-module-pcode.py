#!/usr/bin/env python3
"""Dump heritage-module JSON P-Code in the same order used by LLVM lowering."""

import argparse
import json
from pathlib import Path


TERMINATORS = {"BRANCH", "CBRANCH", "BRANCHIND", "RETURN"}


def default_output_path(input_path: Path) -> Path:
    if input_path.name.endswith(".json"):
        return input_path.with_name(input_path.name[:-5] + ".pcode.txt")
    return input_path.with_suffix(input_path.suffix + ".pcode.txt")


def print_op(out, op, indent="  "):
    text = op.get("text")
    if not text:
        text = f"{op.get('mnemonic')} <no text>"
    comments = [op.get("id"), f"block={op.get('parent')}"]
    call_target_name = op.get("callTargetName")
    call_target = op.get("callTarget")
    if call_target_name or call_target:
        comments.append(f"call={call_target_name or call_target}")
    out.write(f"{indent}{text}  ; {' '.join(str(item) for item in comments if item)}\n")


def iter_lowering_order(block, ops):
    block_ops = [ops[op_id] for op_id in block.get("ops", []) if op_id in ops]
    phis = [op for op in block_ops if op.get("mnemonic") == "MULTIEQUAL"]
    normal = [op for op in block_ops if op.get("mnemonic") != "MULTIEQUAL"]
    yield from phis
    for op in normal:
        yield op
        if op.get("mnemonic") in TERMINATORS:
            break


def dump_function(out, function):
    name = function.get("name", "<unnamed>")
    out.write(f"function {name} entry={function.get('entry')} status={function.get('status')}\n")
    out.write(
        f"  return={function.get('returnType')} callingConvention={function.get('callingConvention')}\n"
    )
    params = function.get("params", [])
    if params:
        out.write("  params:\n")
        for param in params:
            out.write(
                f"    {param.get('name')} type={param.get('type')} varnode={param.get('varnode')}\n"
            )

    ops = {op.get("id"): op for op in function.get("ops", [])}
    for block in function.get("blocks", []):
        out.write(
            f"\nblock {block.get('id')} index={block.get('index')} start={block.get('start')}\n"
        )
        out.write(f"  in: {', '.join(block.get('in', [])) or '-'}\n")
        out.write(f"  out: {', '.join(block.get('out', [])) or '-'}\n")
        for op in iter_lowering_order(block, ops):
            print_op(out, op, "  ")

    out.write("\n")


def dump_module(module, output_path: Path):
    with output_path.open("w", encoding="utf-8") as out:
        out.write(f"schema: {module.get('schema')}\n")
        program = module.get("program", {})
        if program:
            out.write(
                "program: "
                + " ".join(f"{key}={value}" for key, value in program.items())
                + "\n"
            )
        out.write("\n")

        for function in module.get("functions", []):
            dump_function(out, function)

        externals = module.get("externals", [])
        if externals:
            out.write("externals\n")
            for external in externals:
                out.write(
                    f"  {external.get('name')} address={external.get('address')} "
                    f"return={external.get('returnType')} source={external.get('source')}\n"
                )

        failures = module.get("failures", [])
        if failures:
            out.write("\nfailures\n")
            for failure in failures:
                out.write(
                    f"  {failure.get('name')} entry={failure.get('entry')} "
                    f"stage={failure.get('stage')} message={failure.get('message')}\n"
                )


def main():
    parser = argparse.ArgumentParser(
        description="Dump notdec.heritage-module.v0 JSON P-Code in LLVM lowering order."
    )
    parser.add_argument("input", type=Path, help="heritage module JSON")
    parser.add_argument("-o", "--output", type=Path, help="output pcode text path")
    args = parser.parse_args()

    output_path = args.output or default_output_path(args.input)
    with args.input.open("r", encoding="utf-8") as handle:
        module = json.load(handle)
    dump_module(module, output_path)


if __name__ == "__main__":
    main()
