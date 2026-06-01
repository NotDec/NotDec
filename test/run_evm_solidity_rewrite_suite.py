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


CHECKED_BOUNDS_MARKER_RE = re.compile(
    r"call void @(notdec_solidity_rewrite_(?:"
    r"checked_add|checked_sub|checked_mul|checked_add_bound|checked_sub_bound|"
    r"checked_mul_bound|checked_div|checked_mod|checked_exp|"
    r"array_bounds_memory|array_bounds_calldata|array_bounds_storage|"
    r"memory_allocation_bounds|memory_allocation_pointer_bounds|"
    r"storage_bytes_encoding|storage_byte_array_length_bounds|"
    r"storage_array_length_bounds|enum_conversion|empty_array_pop_storage"
    r"))\("
)


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


def count_metadata_string_values(text: str, metadata_name: str) -> dict[str, int]:
    metadata = dict(re.findall(r"!(\d+) = !\{!\"([^\"]+)\"\}", text))
    counts: dict[str, int] = {}
    for metadata_id in re.findall(rf"!{re.escape(metadata_name)} !(\d+)", text):
        value = metadata.get(metadata_id, "unknown")
        counts[value] = counts.get(value, 0) + 1
    return counts


def count_checked_bounds_skipped_kinds(text: str) -> dict[str, int]:
    metadata = dict(re.findall(r"!(\d+) = !\{!\"([^\"]+)\"\}", text))
    counts: dict[str, int] = {}
    for line in text.splitlines():
        if "!notdec.solidity_checked_bounds.skipped !" not in line:
            continue
        for metadata_id in re.findall(
            r"!notdec\.solidity\.checked_bounds !(\d+)", line
        ):
            value = metadata.get(metadata_id, "unknown")
            counts[value] = counts.get(value, 0) + 1
    return counts


def count_checked_bounds_cfg_rewrites(text: str) -> int:
    return len(
        re.findall(
            r"br label %[^,\n]+,[^\n]*!notdec\.solidity\.checked_bounds",
            text,
        )
    )


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


def validate_checked_bounds_oracles(manifest: dict) -> list[str]:
    errors: list[str] = []
    required_keys = {
        "checked_bounds_total",
        "checked_bounds_skip_total",
        "checked_bounds_semantic_marker_total",
        "checked_bounds_cfg_rewrites",
    }
    for case in manifest["cases"]:
        expected = case["expected_counts"]
        checked_keys = {key for key in expected if key.startswith("checked_bounds")}
        if not checked_keys:
            continue
        missing = sorted(required_keys - checked_keys)
        if missing:
            errors.append(
                f"{case['name']}: missing checked-bounds oracle keys: "
                + ", ".join(missing)
            )
            continue
        rewrite_expected = (
            expected["checked_bounds_total"] - expected["checked_bounds_skip_total"]
        )
        if expected["checked_bounds_semantic_marker_total"] != rewrite_expected:
            errors.append(
                f"{case['name']}: checked-bounds semantic marker total "
                "does not match rewrite total"
            )
        if expected["checked_bounds_cfg_rewrites"] != rewrite_expected:
            errors.append(
                f"{case['name']}: checked-bounds CFG rewrite total "
                "does not match rewrite total"
            )
        skip_total = expected["checked_bounds_skip_total"]
        if skip_total:
            reason_total = sum(
                value
                for key, value in expected.items()
                if key.startswith("checked_bounds_skip_reason:")
            )
            skipped_kind_total = sum(
                value
                for key, value in expected.items()
                if key.startswith("checked_bounds_skipped_kind:")
            )
            if reason_total != skip_total:
                errors.append(
                    f"{case['name']}: checked-bounds skip reasons "
                    "do not match skip total"
                )
            if skipped_kind_total != skip_total:
                errors.append(
                    f"{case['name']}: checked-bounds skipped kinds "
                    "do not match skip total"
                )
    return errors


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
    manifest_errors = validate_checked_bounds_oracles(manifest)
    if manifest_errors:
        print("Manifest checked-bounds oracle errors:")
        for error in manifest_errors:
            print(f"  {error}")
        return 1

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
                "skipped_metadata": output_text.count(
                    "!notdec.solidity.selector_outline_skipped"
                ),
            }
            checks.update(count_skip_reasons(output_text))
            checked_bounds_kinds = count_metadata_string_values(
                output_text, "notdec.solidity.checked_bounds"
            )
            checked_bounds_skip_reasons = count_metadata_string_values(
                output_text, "notdec.solidity_checked_bounds.skipped"
            )
            checked_bounds_skipped_kinds = count_checked_bounds_skipped_kinds(
                output_text
            )
            checks["checked_bounds_total"] = sum(checked_bounds_kinds.values())
            checks["checked_bounds_skip_total"] = sum(
                checked_bounds_skip_reasons.values()
            )
            checks["checked_bounds_semantic_marker_total"] = len(
                CHECKED_BOUNDS_MARKER_RE.findall(output_text)
            )
            checks["checked_bounds_cfg_rewrites"] = (
                count_checked_bounds_cfg_rewrites(output_text)
            )
            for reason, count in checked_bounds_skip_reasons.items():
                checks[f"checked_bounds_skip_reason:{reason}"] = count
            for kind, count in checked_bounds_skipped_kinds.items():
                checks[f"checked_bounds_skipped_kind:{kind}"] = count
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

    audit_script = project_root / "scripts/audit-checked-bounds.py"
    if audit_script.exists():
        audit = run_command(
            [
                sys.executable,
                str(audit_script),
                str(workdir),
                "--fail-on-mismatch",
            ],
            project_root,
            env,
        )
        audit_log = workdir / "checked-bounds-audit.log"
        audit_log.write_text(audit.stdout)
        print(audit.stdout, end="")
        if audit.returncode == 0:
            print("[PASS ] checked_bounds_audit")
        else:
            failed += 1
            print("[FAIL ] checked_bounds_audit")
            print(f"        log: {audit_log}")

    print(f"Summary: {passed} passed, {failed} failed")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
