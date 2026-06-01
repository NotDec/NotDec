#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
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


def write_csv_summary(
    path: Path,
    *,
    files: int,
    checked_bounds_total: int,
    skip_total: int,
    marker_total: int,
    cfg_rewrites: int,
    rewrite_expected: int,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "files",
                "checked_bounds_total",
                "skip_total",
                "semantic_marker_total",
                "cfg_rewrites",
                "rewrite_expected",
            ],
        )
        writer.writeheader()
        writer.writerow(
            {
                "files": files,
                "checked_bounds_total": checked_bounds_total,
                "skip_total": skip_total,
                "semantic_marker_total": marker_total,
                "cfg_rewrites": cfg_rewrites,
                "rewrite_expected": rewrite_expected,
            }
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", help="Output directories or .ll files")
    parser.add_argument(
        "--csv",
        help="Write one-row CSV summary for batch trend tracking",
    )
    parser.add_argument(
        "--list-skips",
        action="store_true",
        help="List files that still contain checked-bounds skip metadata",
    )
    parser.add_argument(
        "--fail-on-mismatch",
        action="store_true",
        help=(
            "Return non-zero unless non-skipped checked-bounds metadata count "
            "matches semantic marker and CFG rewrite counts"
        ),
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

    checked_bounds_total = sum(totals["kinds"].values())
    skip_total = sum(totals["skip_reasons"].values())
    rewrite_expected = checked_bounds_total - skip_total
    marker_total = sum(totals["semantic_markers"].values())
    print(f"rewrite_expected: {rewrite_expected}")
    print(f"rewrite_markers: {marker_total}")

    if args.csv:
        write_csv_summary(
            Path(args.csv),
            files=len(files),
            checked_bounds_total=checked_bounds_total,
            skip_total=skip_total,
            marker_total=marker_total,
            cfg_rewrites=total_cfg_rewrites,
            rewrite_expected=rewrite_expected,
        )

    if args.list_skips and skipped_files:
        print("skip_files:")
        for path, reasons in skipped_files:
            reason_text = ", ".join(f"{k}:{v}" for k, v in reasons.most_common())
            print(f"  {path}: {reason_text}")

    if args.fail_on_mismatch and (
        rewrite_expected != marker_total or rewrite_expected != total_cfg_rewrites
    ):
        print(
            "ERROR: checked-bounds rewrite mismatch: "
            f"expected={rewrite_expected} markers={marker_total} "
            f"cfg_rewrites={total_cfg_rewrites}"
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
