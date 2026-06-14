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
CPP_CHECKED_BOUNDS_MARKER_FUNCTION_RE = re.compile(
    r"StringRef getCheckedBoundsRewriteMarkerName\(StringRef Kind\) \{(.*?)\n\}",
    re.DOTALL,
)
CPP_CHECKED_BOUNDS_MARKER_PAIR_RE = re.compile(
    r'if \(Kind == "([^"]+)"\) \{\s*return "([^"]+)";\s*\}'
)
CHECKED_BOUNDS_KIND_TO_MARKER = {
    "checked_add": "notdec_solidity_rewrite_checked_add",
    "checked_sub": "notdec_solidity_rewrite_checked_sub",
    "checked_mul": "notdec_solidity_rewrite_checked_mul",
    "checked_add_bound": "notdec_solidity_rewrite_checked_add_bound",
    "checked_sub_bound": "notdec_solidity_rewrite_checked_sub_bound",
    "checked_mul_bound": "notdec_solidity_rewrite_checked_mul_bound",
    "checked_div": "notdec_solidity_rewrite_checked_div",
    "checked_mod": "notdec_solidity_rewrite_checked_mod",
    "checked_exp": "notdec_solidity_rewrite_checked_exp",
    "array_bounds_memory": "notdec_solidity_rewrite_array_bounds_memory",
    "array_bounds_calldata": "notdec_solidity_rewrite_array_bounds_calldata",
    "array_bounds_storage": "notdec_solidity_rewrite_array_bounds_storage",
    "memory_allocation_bounds": "notdec_solidity_rewrite_memory_allocation_bounds",
    "memory_allocation_pointer_bounds": (
        "notdec_solidity_rewrite_memory_allocation_pointer_bounds"
    ),
    "storage_bytes_encoding": "notdec_solidity_rewrite_storage_bytes_encoding",
    "storage_byte_array_length_bounds": (
        "notdec_solidity_rewrite_storage_byte_array_length_bounds"
    ),
    "storage_array_length_bounds": (
        "notdec_solidity_rewrite_storage_array_length_bounds"
    ),
    "enum_conversion": "notdec_solidity_rewrite_enum_conversion",
    "empty_array_pop_storage": "notdec_solidity_rewrite_empty_array_pop_storage",
}


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


def expected_checked_bounds_markers_from_kinds(case: dict) -> dict[str, int]:
    markers: dict[str, int] = {}
    for kind, count in case.get("expected_checked_bounds_kinds", {}).items():
        marker = CHECKED_BOUNDS_KIND_TO_MARKER.get(kind)
        if marker is None:
            continue
        markers[marker] = markers.get(marker, 0) + count
    return markers


def load_cpp_checked_bounds_marker_mapping(project_root: Path) -> dict[str, str]:
    source_path = (
        project_root / "src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp"
    )
    text = source_path.read_text()
    match = CPP_CHECKED_BOUNDS_MARKER_FUNCTION_RE.search(text)
    if not match:
        return {}
    return dict(CPP_CHECKED_BOUNDS_MARKER_PAIR_RE.findall(match.group(1)))


def format_checked_bounds_marker_mapping_errors(
    expected: dict[str, str], actual: dict[str, str]
) -> list[str]:
    errors: list[str] = []
    for kind in sorted(set(expected) | set(actual)):
        expected_marker = expected.get(kind)
        actual_marker = actual.get(kind)
        if expected_marker != actual_marker:
            errors.append(
                f"{kind}: runner={expected_marker or '<missing>'} "
                f"cpp={actual_marker or '<missing>'}"
            )
    return errors


def validate_checked_bounds_marker_oracles(
    manifest: dict, project_root: Path
) -> list[str]:
    errors: list[str] = []
    cpp_mapping = load_cpp_checked_bounds_marker_mapping(project_root)
    if cpp_mapping != CHECKED_BOUNDS_KIND_TO_MARKER:
        errors.append("runner checked-bounds kind-to-marker map differs from C++")
        errors.extend(
            format_checked_bounds_marker_mapping_errors(
                CHECKED_BOUNDS_KIND_TO_MARKER, cpp_mapping
            )
        )
    for case in manifest.get("cases", []):
        expected_kinds = case.get("expected_checked_bounds_kinds")
        if not expected_kinds:
            continue
        expected_markers = case.get("expected_checked_bounds_semantic_markers", {})
        derived_markers = expected_checked_bounds_markers_from_kinds(case)
        if derived_markers != expected_markers:
            errors.append(
                f"{case['name']}: expected_checked_bounds_semantic_markers "
                "does not match expected_checked_bounds_kinds"
            )
        expected_skip_reasons = case.get("expected_checked_bounds_skip_reasons", {})
        skip_total = sum(expected_skip_reasons.values())
        if skip_total:
            skipped_kinds = case.get("expected_checked_bounds_skipped_kinds", {})
            skipped_kind_total = sum(skipped_kinds.values())
            if skipped_kind_total != skip_total:
                errors.append(
                    f"{case['name']}: expected_checked_bounds_skipped_kinds "
                    "does not match expected_checked_bounds_skip_reasons total"
                )
    return errors


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


def count_checked_bounds_skipped_kinds(path: Path) -> dict[str, int]:
    text = path.read_text()
    values_by_node = {
        node_id: value
        for node_id, value in re.findall(
            r'^!(\d+) = !\{!"([^"]*)"\}', text, re.MULTILINE
        )
    }
    counts: dict[str, int] = {}
    for line in text.splitlines():
        if "!notdec.solidity_checked_bounds.skipped !" not in line:
            continue
        for node_id in re.findall(r"!notdec\.solidity\.checked_bounds !(\d+)", line):
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


def count_checked_bounds_cfg_rewrites(path: Path) -> int:
    text = path.read_text()
    return len(
        re.findall(
            r"br label %[^,\n]+,[^\n]*!notdec\.solidity\.checked_bounds",
            text,
        )
    )


def count_exact_marker(path: Path, marker_name: str) -> int:
    text = path.read_text()
    return len(
        re.findall(rf"\bcall\b[^\n@]*@{re.escape(marker_name)}\(", text)
    )


def count_function_definition(path: Path, function_name: str) -> int:
    text = path.read_text()
    return len(
        re.findall(rf"^define\b[^\n@]*@{re.escape(function_name)}\(", text, re.M)
    )


def count_memory_consumer_kinds(path: Path) -> dict[str, int]:
    text = path.read_text()
    names = {
        "1": "return",
        "2": "revert",
        "3": "event_log",
        "4": "external_call_input",
        "5": "external_call_output",
    }
    pattern = (
        r"call void @notdec_solidity_memory_consumer"
        r"\(i256 [^,]+, i256 [^,]+, i256 ([^)]+)\)"
    )
    counts: dict[str, int] = {}
    for value in re.findall(pattern, text):
        kind = names.get(value.strip())
        if kind is None:
            continue
        counts[kind] = counts.get(kind, 0) + 1
    return counts


def count_memory_copy_write_kinds(path: Path) -> dict[str, int]:
    text = path.read_text()
    names = {
        "2": "calldata",
        "3": "returndata",
        "4": "scratch_returndata",
        "5": "code",
        "6": "memory",
    }
    pattern = (
        r"call void @notdec_solidity_memory_copy_write"
        r"\(i256 [^,]+, i256 [^,]+, i256 [^,]+, i256 [^,]+, i256 ([^)]+)\)"
    )
    counts: dict[str, int] = {}
    for value in re.findall(pattern, text):
        kind = names.get(value.strip())
        if kind is None:
            continue
        counts[kind] = counts.get(kind, 0) + 1
    return counts


def count_marker_arg_pairs(path: Path, marker_name: str) -> dict[str, int]:
    text = path.read_text()
    pattern = (
        rf"call void @{re.escape(marker_name)}"
        rf"\(i256 ([^,]+), i256 ([^)]+)\)"
    )
    counts: dict[str, int] = {}
    for first, second in re.findall(pattern, text):
        key = f"{first.strip()}:{second.strip()}"
        counts[key] = counts.get(key, 0) + 1
    return counts


def count_hidden_markers(path: Path) -> int:
    text = path.read_text()
    return text.count("call void @notdec_solidity_rewrite_hidden(")


def count_hidden_metadata(path: Path) -> int:
    text = path.read_text()
    return text.count("!notdec.solidity.rewrite_hidden.")


def count_revert_kinds(path: Path) -> dict[str, int]:
    return count_metadata_string_values(path, "notdec.solidity.revert")


def count_panic_codes(path: Path) -> dict[str, int]:
    text = path.read_text()
    marker_codes = re.findall(
        r"call void @notdec_solidity_rewrite_revert_panic\(i256 ([^)]+)\)",
        text,
    )
    if marker_codes:
        counts: dict[str, int] = {}
        for code in marker_codes:
            key = code.strip()
            counts[key] = counts.get(key, 0) + 1
        return counts

    return count_metadata_string_values(path, "notdec.solidity_revert.panic_code")


def write_compare_report(
    *,
    report_path: Path,
    expected_counts: dict[str, int],
    actual_counts: dict[str, int],
) -> bool:
    passed = actual_counts == expected_counts
    lines = ["PASS" if passed else "FAIL"]
    for key in sorted(set(expected_counts) | set(actual_counts)):
        lines.append(f"  expected_{key}={expected_counts.get(key, 0)}")
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

    manifest_errors = validate_checked_bounds_marker_oracles(manifest, project_root)
    if manifest_errors:
        for error in manifest_errors:
            print(f"[FAIL ] checked_bounds_manifest_oracle: {error}")
        return 1

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
                    if metadata_name == "notdec.solidity.checked_bounds":
                        continue
                    expected_counts[
                        f"rewrite_marker:{rewrite_marker_name(metadata_name)}"
                    ] = count
            if expect_rewrite_hidden:
                hidden_count = sum(
                    count
                    for metadata_name, count in case.get(
                        "expected_metadata_counts", {}
                    ).items()
                    if metadata_name != "notdec.solidity.checked_bounds"
                )
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
            for kind, count in case.get("expected_checked_bounds_kinds", {}).items():
                expected_counts[f"checked_bounds_kind:{kind}"] = count
            for code, count in case.get(
                "expected_checked_bounds_panic_codes", {}
            ).items():
                expected_counts[f"checked_bounds_panic_code:{code}"] = count
            for payload, expected in case.get(
                "expected_checked_bounds_marker_payloads", {}
            ).items():
                expected_counts[f"checked_bounds_marker_payload:{payload}"] = expected
            for marker_name, expected in case.get(
                "expected_checked_bounds_semantic_markers", {}
            ).items():
                expected_counts[f"checked_bounds_semantic_marker:{marker_name}"] = (
                    expected
                )
            for reason, expected in case.get(
                "expected_checked_bounds_skip_reasons", {}
            ).items():
                expected_counts[f"checked_bounds_skip_reason:{reason}"] = expected
            for kind, expected in case.get(
                "expected_checked_bounds_skipped_kinds", {}
            ).items():
                expected_counts[f"checked_bounds_skipped_kind:{kind}"] = expected
            if "expected_checked_bounds_skip_reasons" in case:
                expected_counts["checked_bounds_skip_total"] = sum(
                    case.get("expected_checked_bounds_skip_reasons", {}).values()
                )
            if "expected_checked_bounds_cfg_rewrites" in case:
                expected_counts["checked_bounds_cfg_rewrites"] = case[
                    "expected_checked_bounds_cfg_rewrites"
                ]
            for marker_name, expected in case.get(
                "expected_memory_rewrite_markers", {}
            ).items():
                expected_counts[f"memory_rewrite_marker:{marker_name}"] = expected
            for marker_name, expected in case.get(
                "expected_exact_markers", {}
            ).items():
                expected_counts[f"exact_marker:{marker_name}"] = expected
            for function_name, expected in case.get(
                "expected_function_definitions", {}
            ).items():
                expected_counts[f"function_definition:{function_name}"] = expected
            for kind, expected in case.get(
                "expected_memory_consumer_kinds", {}
            ).items():
                expected_counts[f"memory_consumer_kind:{kind}"] = expected
            for kind, expected in case.get(
                "expected_memory_copy_write_kinds", {}
            ).items():
                expected_counts[f"memory_copy_write_kind:{kind}"] = expected
            for count, expected in case.get(
                "expected_custom_error_arg_counts", {}
            ).items():
                expected_counts[f"custom_error_arg_count:{count}"] = expected
            for length, expected in case.get(
                "expected_error_string_lengths", {}
            ).items():
                expected_counts[f"error_string_length:{length}"] = expected
            for payload, expected in case.get(
                "expected_error_string_marker_payloads", {}
            ).items():
                expected_counts[f"error_string_marker_payload:{payload}"] = expected
            for payload, expected in case.get(
                "expected_custom_error_marker_payloads", {}
            ).items():
                expected_counts[f"custom_error_marker_payload:{payload}"] = expected
            if expect_rewrite_markers:
                revert_kinds = case.get("expected_revert_kinds", {})
                if "panic" in revert_kinds:
                    expected_panic_markers = sum(
                        case.get("expected_panic_codes", {}).values()
                    )
                    expected_counts[
                        "rewrite_marker:notdec_solidity_rewrite_revert_panic"
                    ] = expected_panic_markers
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
                if (
                    expect_rewrite_markers
                    and metadata_name != "notdec.solidity.checked_bounds"
                ):
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
            actual_checked_bounds_kinds = count_metadata_string_values(
                output_ll, "notdec.solidity.checked_bounds"
            )
            for kind in case.get("expected_checked_bounds_kinds", {}):
                actual_counts[f"checked_bounds_kind:{kind}"] = (
                    actual_checked_bounds_kinds.get(str(kind), 0)
                )
            actual_checked_bounds_panic_codes = count_metadata_string_values(
                output_ll, "notdec.solidity_checked_bounds.panic_code"
            )
            for code in case.get("expected_checked_bounds_panic_codes", {}):
                actual_counts[f"checked_bounds_panic_code:{code}"] = (
                    actual_checked_bounds_panic_codes.get(str(code), 0)
                )
            actual_checked_bounds_marker_payloads = count_marker_arg_pairs(
                output_ll, "notdec_solidity_rewrite_checked_bounds_panic"
            )
            for payload in case.get("expected_checked_bounds_marker_payloads", {}):
                actual_counts[f"checked_bounds_marker_payload:{payload}"] = (
                    actual_checked_bounds_marker_payloads.get(str(payload), 0)
                )
            for marker_name in case.get(
                "expected_checked_bounds_semantic_markers", {}
            ):
                actual_counts[f"checked_bounds_semantic_marker:{marker_name}"] = (
                    count_exact_marker(output_ll, marker_name)
                )
            actual_checked_bounds_skip_reasons = count_metadata_string_values(
                output_ll, "notdec.solidity_checked_bounds.skipped"
            )
            for reason in case.get("expected_checked_bounds_skip_reasons", {}):
                actual_counts[f"checked_bounds_skip_reason:{reason}"] = (
                    actual_checked_bounds_skip_reasons.get(str(reason), 0)
                )
            actual_checked_bounds_skipped_kinds = count_checked_bounds_skipped_kinds(
                output_ll
            )
            for kind in case.get("expected_checked_bounds_skipped_kinds", {}):
                actual_counts[f"checked_bounds_skipped_kind:{kind}"] = (
                    actual_checked_bounds_skipped_kinds.get(str(kind), 0)
                )
            if "checked_bounds_skip_total" in expected_counts:
                actual_counts["checked_bounds_skip_total"] = sum(
                    actual_checked_bounds_skip_reasons.values()
                )
            if "checked_bounds_cfg_rewrites" in expected_counts:
                actual_counts["checked_bounds_cfg_rewrites"] = (
                    count_checked_bounds_cfg_rewrites(output_ll)
                )
            for marker_name in case.get("expected_memory_rewrite_markers", {}):
                actual_counts[f"memory_rewrite_marker:{marker_name}"] = (
                    count_exact_marker(output_ll, marker_name)
                )
            for marker_name in case.get("expected_exact_markers", {}):
                actual_counts[f"exact_marker:{marker_name}"] = count_exact_marker(
                    output_ll, marker_name
                )
            for function_name in case.get("expected_function_definitions", {}):
                actual_counts[f"function_definition:{function_name}"] = (
                    count_function_definition(output_ll, function_name)
                )
            actual_memory_consumer_kinds = count_memory_consumer_kinds(output_ll)
            for kind in case.get("expected_memory_consumer_kinds", {}):
                actual_counts[f"memory_consumer_kind:{kind}"] = (
                    actual_memory_consumer_kinds.get(str(kind), 0)
                )
            actual_memory_copy_write_kinds = count_memory_copy_write_kinds(output_ll)
            for kind in case.get("expected_memory_copy_write_kinds", {}):
                actual_counts[f"memory_copy_write_kind:{kind}"] = (
                    actual_memory_copy_write_kinds.get(str(kind), 0)
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
            actual_error_string_marker_payloads = count_marker_arg_pairs(
                output_ll, "notdec_solidity_rewrite_revert_error_string"
            )
            for payload in case.get("expected_error_string_marker_payloads", {}):
                actual_counts[f"error_string_marker_payload:{payload}"] = (
                    actual_error_string_marker_payloads.get(str(payload), 0)
                )
            actual_custom_error_marker_payloads = count_marker_arg_pairs(
                output_ll, "notdec_solidity_rewrite_revert_custom_error"
            )
            for payload in case.get("expected_custom_error_marker_payloads", {}):
                actual_counts[f"custom_error_marker_payload:{payload}"] = (
                    actual_custom_error_marker_payloads.get(str(payload), 0)
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

    audit_script = project_root / "scripts/audit-checked-bounds.py"
    if audit_script.exists():
        audit, audit_log = run_command(
            title="checked-bounds-audit",
            command=[
                sys.executable,
                str(audit_script),
                str(workdir),
                "--fail-on-mismatch",
            ],
            cwd=project_root,
            env=env,
        )
        (workdir / "checked-bounds-audit.log").write_text(audit_log)
        print(audit.stdout, end="")
        if audit.returncode == 0:
            print("[PASS ] checked_bounds_audit")
        else:
            failed += 1
            print("[FAIL ] checked_bounds_audit")
            print(f"        log: {workdir / 'checked-bounds-audit.log'}")

    print(f"Summary: {passed} passed, {failed} failed")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
