#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
from collections import Counter
from pathlib import Path


METADATA_RE = re.compile(r"!(\d+) = !\{!\"([^\"]*)\"\}", re.MULTILINE)
CFG_REWRITE_RE = re.compile(
    r"br label %[^,\n]+,[^\n]*!notdec\.solidity\.checked_bounds"
)
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


def metadata_values(text: str, metadata_name: str, metadata: dict[str, str]) -> Counter[str]:
    counts: Counter[str] = Counter()
    for metadata_id in re.findall(rf"!{re.escape(metadata_name)} !(\d+)", text):
        counts[metadata.get(metadata_id, "unknown")] += 1
    return counts


def audit_file(path: Path) -> dict[str, Counter[str] | int]:
    text = path.read_text(errors="ignore")
    metadata = dict(METADATA_RE.findall(text))
    return {
        "kinds": metadata_values(text, "notdec.solidity.checked_bounds", metadata),
        "panic_codes": metadata_values(
            text, "notdec.solidity_checked_bounds.panic_code", metadata
        ),
        "skip_reasons": metadata_values(
            text, "notdec.solidity_checked_bounds.skipped", metadata
        ),
        "semantic_markers": Counter(CHECKED_BOUNDS_MARKER_RE.findall(text)),
        "cfg_rewrites": len(CFG_REWRITE_RE.findall(text)),
    }


def print_counter(title: str, counts: Counter[str]) -> None:
    print(f"{title}: {sum(counts.values())}")
    for key, value in counts.most_common():
        print(f"  {key}: {value}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", help="Output directories or .ll files")
    parser.add_argument(
        "--list-skips",
        action="store_true",
        help="List files that still contain checked-bounds skip metadata",
    )
    args = parser.parse_args()

    files: list[Path] = []
    for raw_path in args.paths:
        path = Path(raw_path)
        if path.is_dir():
            files.extend(sorted(path.rglob("*.ll")))
        elif path.suffix == ".ll":
            files.append(path)

    totals = {
        "kinds": Counter(),
        "panic_codes": Counter(),
        "skip_reasons": Counter(),
        "semantic_markers": Counter(),
    }
    total_cfg_rewrites = 0
    skipped_files: list[tuple[Path, Counter[str]]] = []

    for path in files:
        result = audit_file(path)
        for key in totals:
            totals[key].update(result[key])  # type: ignore[arg-type]
        total_cfg_rewrites += int(result["cfg_rewrites"])
        skip_reasons = result["skip_reasons"]
        if isinstance(skip_reasons, Counter) and skip_reasons:
            skipped_files.append((path, skip_reasons))

    print(f"files: {len(files)}")
    print_counter("checked_bounds_kinds", totals["kinds"])
    print_counter("panic_codes", totals["panic_codes"])
    print_counter("semantic_markers", totals["semantic_markers"])
    print_counter("skip_reasons", totals["skip_reasons"])
    print(f"cfg_rewrites: {total_cfg_rewrites}")

    if args.list_skips and skipped_files:
        print("skip_files:")
        for path, reasons in skipped_files:
            reason_text = ", ".join(f"{k}:{v}" for k, v in reasons.most_common())
            print(f"  {path}: {reason_text}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
