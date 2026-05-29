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
EVM_REVERT_CALL_RE = re.compile(
    r"\bcall void @evm_revert\(ptr [^,]+, i256 ([^,]+), i256 ([^)]+)\)"
)
EVM_MSTORE_CALL_RE = re.compile(
    r"\bcall void @evm_mstore\(ptr [^,]+, i256 ([^,]+), i256 ([^)]+)\)"
)
EVM_RETURNDATACOPY_CALL_RE = re.compile(
    r"\bcall void @evm_returndatacopy\(ptr [^,]+, ptr [^,]+, i256 ([^,]+), i256 ([^,]+), i256 ([^)]+)\)"
)
EVM_SHL_ASSIGN_RE = re.compile(
    r"^\s*(%[\w.\-]+) = call i256 @evm_shl\(i256 (\d+), i256 (\d+)\)"
)
EVM_MLOAD_ASSIGN_RE = re.compile(
    r"^\s*(%[\w.\-]+) = call i256 @evm_mload\(ptr [^,]+, i256 ([^)]+)\)"
)
EVM_ADD_ASSIGN_RE = re.compile(
    r"^\s*(%[\w.\-]+) = add i256 ([^,]+), ([^,!]+)"
)
PANIC_SELECTOR = 0x4E487B71
ERROR_SELECTOR = 0x08C379A0


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


def count_metadata_string_values(path: Path, metadata_name: str) -> dict[str, int]:
    text = path.read_text()
    node_ids = re.findall(rf"!{re.escape(metadata_name)} !(\d+)", text)
    if not node_ids:
        return {}

    values_by_node = {
        node_id: value
        for node_id, value in re.findall(r'^!(\d+) = !\{!"([^"]*)"\}', text, re.MULTILINE)
    }
    counts: dict[str, int] = {}
    for node_id in node_ids:
        value = values_by_node.get(node_id)
        if value is None:
            continue
        counts[value] = counts.get(value, 0) + 1
    return counts


def rewrite_marker_name(metadata_name: str) -> str:
    prefix = "notdec.solidity."
    if metadata_name.startswith(prefix):
        metadata_name = metadata_name[len(prefix) :]
    return "notdec_solidity_rewrite_" + metadata_name.replace(".", "_")


def count_rewrite_markers(path: Path, metadata_name: str) -> int:
    text = path.read_text()
    return text.count(f"call void @{rewrite_marker_name(metadata_name)}(")


def count_payability_cfg_rewrites(path: Path) -> int:
    text = path.read_text()
    return text.count("call void @notdec_solidity_cfg_rewrite_payability_guard(")


def count_exact_marker(path: Path, marker_name: str) -> int:
    text = path.read_text()
    return text.count(f"call void @{marker_name}(")


def count_hidden_markers(path: Path) -> int:
    text = path.read_text()
    return text.count("call void @notdec_solidity_rewrite_hidden(")


def count_hidden_metadata(path: Path) -> int:
    text = path.read_text()
    return text.count("!notdec.solidity.rewrite_hidden.")


def count_revert_kinds(path: Path) -> dict[str, int]:
    text = path.read_text()
    counts: dict[str, int] = {}
    for block in re.split(r"\n(?=[\w.$-]+:)", text):
        for kind in classify_reverts_from_block(block):
            counts[kind] = counts.get(kind, 0) + 1
    return counts


def is_constant_i256(value: str, expected: int) -> bool:
    return value.strip() == str(expected)


def is_returndatasize_value(value: str) -> bool:
    return value.strip().startswith("%evm.returndatasize")


def decode_selector_word(shift: int, payload: int) -> int | None:
    word = payload << shift
    if word >> 256:
        return None
    if word & ((1 << 224) - 1):
        return None
    selector = word >> 224
    if selector > 0xFFFFFFFF:
        return None
    return selector


def value_expr(value: str, exprs: dict[str, tuple]) -> tuple:
    value = value.strip()
    if value.isdigit():
        return ("const", int(value))
    return exprs.get(value, ("var", value))


def same_value(lhs: str, rhs: str, exprs: dict[str, tuple]) -> bool:
    return value_expr(lhs, exprs) == value_expr(rhs, exprs)


def const_offset_from_base(offset: str, base: str, exprs: dict[str, tuple]) -> int | None:
    offset_expr = value_expr(offset, exprs)
    base_expr = value_expr(base, exprs)
    if offset_expr == base_expr:
        return 0
    if (
        len(offset_expr) == 2
        and len(base_expr) == 2
        and offset_expr[0] == "const"
        and base_expr[0] == "const"
        and offset_expr[1] >= base_expr[1]
    ):
        return offset_expr[1] - base_expr[1]
    if (
        len(offset_expr) == 3
        and offset_expr[0] == "add"
        and offset_expr[1] == base_expr
        and isinstance(offset_expr[2], int)
    ):
        return offset_expr[2]
    return None


def selector_from_value(value: str, shl_values: dict[str, int]) -> int | None:
    value = value.strip()
    return shl_values.get(value)


def classify_reverts_from_block(block: str) -> list[str]:
    shl_values: dict[str, int] = {}
    exprs: dict[str, tuple] = {}
    memory_stores: list[tuple[str, str]] = []
    returndata_copy_offsets: list[str] = []
    kinds: list[str] = []

    for line in block.splitlines():
        shl_match = EVM_SHL_ASSIGN_RE.match(line)
        if shl_match:
            selector = decode_selector_word(
                int(shl_match.group(2)), int(shl_match.group(3))
            )
            if selector is not None:
                shl_values[shl_match.group(1)] = selector

        mload_match = EVM_MLOAD_ASSIGN_RE.match(line)
        if mload_match:
            exprs[mload_match.group(1)] = (
                "mload",
                value_expr(mload_match.group(2), exprs),
            )

        add_match = EVM_ADD_ASSIGN_RE.match(line)
        if add_match:
            lhs = add_match.group(1)
            left = add_match.group(2).strip()
            right = add_match.group(3).strip()
            if left.isdigit():
                exprs[lhs] = ("add", value_expr(right, exprs), int(left))
            elif right.isdigit():
                exprs[lhs] = ("add", value_expr(left, exprs), int(right))

        mstore_match = EVM_MSTORE_CALL_RE.search(line)
        if mstore_match:
            memory_stores.append((mstore_match.group(1), mstore_match.group(2)))

        returndata_copy_match = EVM_RETURNDATACOPY_CALL_RE.search(line)
        if returndata_copy_match:
            copy_offset = returndata_copy_match.group(1).strip()
            source_offset = returndata_copy_match.group(2).strip()
            length = returndata_copy_match.group(3).strip()
            if is_constant_i256(source_offset, 0) and is_returndatasize_value(length):
                returndata_copy_offsets.append(copy_offset)

        revert_match = EVM_REVERT_CALL_RE.search(line)
        if not revert_match:
            continue

        offset = revert_match.group(1).strip()
        length = revert_match.group(2).strip()
        selector = None
        for store_offset, store_value in memory_stores:
            if const_offset_from_base(store_offset, offset, exprs) == 0:
                selector = selector_from_value(store_value, shl_values)

        if is_constant_i256(offset, 0) and is_constant_i256(length, 0):
            kind = "empty"
        elif is_returndatasize_value(length) and any(
            same_value(offset, copy_offset, exprs)
            for copy_offset in returndata_copy_offsets
        ):
            kind = "returndata_bubble"
        elif is_constant_i256(length, 36) and selector == PANIC_SELECTOR:
            kind = "panic"
        elif selector == ERROR_SELECTOR:
            kind = "error_string"
        elif selector is not None and selector != PANIC_SELECTOR:
            kind = "custom_error_candidate"
        else:
            kind = "encoded_candidate"
        kinds.append(kind)
        memory_stores.clear()
        returndata_copy_offsets.clear()

    return kinds


def collect_panic_codes_from_block(block: str) -> list[int]:
    shl_values: dict[str, int] = {}
    saw_panic_selector = False
    pending_codes: list[int] = []
    codes: list[int] = []

    for line in block.splitlines():
        shl_match = EVM_SHL_ASSIGN_RE.match(line)
        if shl_match:
            selector = decode_selector_word(
                int(shl_match.group(2)), int(shl_match.group(3))
            )
            if selector is not None:
                shl_values[shl_match.group(1)] = selector

        mstore_match = EVM_MSTORE_CALL_RE.search(line)
        if mstore_match:
            offset = mstore_match.group(1).strip()
            value = mstore_match.group(2).strip()
            if is_constant_i256(offset, 0) and shl_values.get(value) == PANIC_SELECTOR:
                saw_panic_selector = True
            elif is_constant_i256(offset, 4) and value.isdigit():
                pending_codes.append(int(value))

        if EVM_REVERT_CALL_RE.search(line):
            if saw_panic_selector and "i256 36" in line:
                codes.append(pending_codes[-1] if pending_codes else -1)
            saw_panic_selector = False
            pending_codes.clear()

    return codes


def count_panic_codes(path: Path) -> dict[str, int]:
    text = path.read_text()
    counts: dict[str, int] = {}
    for block in re.split(r"\n(?=[\w.$-]+:)", text):
        for code in collect_panic_codes_from_block(block):
            key = "unknown" if code < 0 else str(code)
            counts[key] = counts.get(key, 0) + 1
    return counts


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
            if "payability" in case.get("patterns", []) or "nonpayable_guard" in case.get(
                "patterns", []
            ):
                expected_counts["payability_cfg_rewrites"] = case.get(
                    "expected_payability_cfg_rewrites",
                    case["expected_nonpayable_functions"],
                )
            for kind, count in case.get("expected_revert_kinds", {}).items():
                expected_counts[f"revert_kind:{kind}"] = count
            for code, count in case.get("expected_panic_codes", {}).items():
                expected_counts[f"panic_code:{code}"] = count
            if "expected_returndata_bubbles" in case:
                expected_counts["returndata_bubbles"] = case[
                    "expected_returndata_bubbles"
                ]
            for count, expected in case.get(
                "expected_custom_error_arg_counts", {}
            ).items():
                expected_counts[f"custom_error_arg_count:{count}"] = expected
            for length, expected in case.get(
                "expected_error_string_lengths", {}
            ).items():
                expected_counts[f"error_string_length:{length}"] = expected
            for literal, expected in case.get(
                "expected_error_string_literals", {}
            ).items():
                expected_counts[f"error_string_literal:{literal}"] = expected
            if expect_rewrite_markers:
                revert_kinds = case.get("expected_revert_kinds", {})
                if "panic" in revert_kinds:
                    expected_panic_markers = sum(
                        case.get("expected_panic_codes", {}).values()
                    )
                    expected_counts[
                        "rewrite_marker:notdec_solidity_rewrite_revert_panic"
                    ] = expected_panic_markers
                if "expected_returndata_bubbles" in case:
                    expected_counts[
                        "rewrite_marker:notdec_solidity_rewrite_revert_returndata_bubble"
                    ] = case["expected_returndata_bubbles"]
                if "error_string" in revert_kinds:
                    expected_counts[
                        "rewrite_marker:notdec_solidity_rewrite_revert_error_string"
                    ] = revert_kinds["error_string"]
                if "custom_error_candidate" in revert_kinds:
                    expected_counts[
                        "rewrite_marker:notdec_solidity_rewrite_revert_custom_error"
                    ] = revert_kinds["custom_error_candidate"]
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
            if "payability_cfg_rewrites" in expected_counts:
                actual_counts["payability_cfg_rewrites"] = count_payability_cfg_rewrites(
                    output_ll
                )
            actual_revert_kinds = count_revert_kinds(output_ll)
            for kind in case.get("expected_revert_kinds", {}):
                actual_counts[f"revert_kind:{kind}"] = actual_revert_kinds.get(
                    kind, 0
                )
            actual_panic_codes = count_panic_codes(output_ll)
            for code in case.get("expected_panic_codes", {}):
                actual_counts[f"panic_code:{code}"] = actual_panic_codes.get(
                    str(code), 0
                )
            if "returndata_bubbles" in expected_counts:
                actual_counts["returndata_bubbles"] = actual_revert_kinds.get(
                    "returndata_bubble", 0
                )
            actual_custom_error_arg_counts = count_metadata_string_values(
                output_ll, "notdec.solidity_revert.custom_error_arg_count"
            )
            for count in case.get("expected_custom_error_arg_counts", {}):
                actual_counts[f"custom_error_arg_count:{count}"] = (
                    actual_custom_error_arg_counts.get(str(count), 0)
                )
            actual_error_string_lengths = count_metadata_string_values(
                output_ll, "notdec.solidity_revert.error_string_length"
            )
            for length in case.get("expected_error_string_lengths", {}):
                actual_counts[f"error_string_length:{length}"] = (
                    actual_error_string_lengths.get(str(length), 0)
                )
            actual_error_string_literals = count_metadata_string_values(
                output_ll, "notdec.solidity_revert.error_string_literal"
            )
            for literal in case.get("expected_error_string_literals", {}):
                actual_counts[f"error_string_literal:{literal}"] = (
                    actual_error_string_literals.get(str(literal), 0)
                )
            if (
                "rewrite_marker:notdec_solidity_rewrite_revert_panic"
                in expected_counts
            ):
                actual_counts[
                    "rewrite_marker:notdec_solidity_rewrite_revert_panic"
                ] = count_exact_marker(
                    output_ll, "notdec_solidity_rewrite_revert_panic"
                )
            if (
                "rewrite_marker:notdec_solidity_rewrite_revert_returndata_bubble"
                in expected_counts
            ):
                actual_counts[
                    "rewrite_marker:notdec_solidity_rewrite_revert_returndata_bubble"
                ] = count_exact_marker(
                    output_ll,
                    "notdec_solidity_rewrite_revert_returndata_bubble",
                )
            if (
                "rewrite_marker:notdec_solidity_rewrite_revert_error_string"
                in expected_counts
            ):
                actual_counts[
                    "rewrite_marker:notdec_solidity_rewrite_revert_error_string"
                ] = count_exact_marker(
                    output_ll, "notdec_solidity_rewrite_revert_error_string"
                )
            if (
                "rewrite_marker:notdec_solidity_rewrite_revert_custom_error"
                in expected_counts
            ):
                actual_counts[
                    "rewrite_marker:notdec_solidity_rewrite_revert_custom_error"
                ] = count_exact_marker(
                    output_ll, "notdec_solidity_rewrite_revert_custom_error"
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
