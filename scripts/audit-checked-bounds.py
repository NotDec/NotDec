#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import re
from collections import Counter
from pathlib import Path


METADATA_RE = re.compile(r"!(\d+) = !\{!\"([^\"]*)\"\}", re.MULTILINE)
CPP_MARKER_FUNCTION_RE = re.compile(
    r"StringRef getCheckedBoundsRewriteMarkerName\(StringRef Kind\) \{(.*?)\n\}",
    re.DOTALL,
)
CPP_MARKER_PAIR_RE = re.compile(
    r'if \(Kind == "([^"]+)"\) \{\s*return "([^"]+)";\s*\}'
)
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
# Keep this in sync with getCheckedBoundsRewriteMarkerName in SolidityPatterns.cpp.
KIND_TO_MARKER = {
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


def metadata_values(text: str, metadata_name: str, metadata: dict[str, str]) -> Counter[str]:
    counts: Counter[str] = Counter()
    for metadata_id in re.findall(rf"!{re.escape(metadata_name)} !(\d+)", text):
        counts[metadata.get(metadata_id, "unknown")] += 1
    return counts


def expected_markers_by_kind(
    text: str, metadata: dict[str, str]
) -> tuple[Counter[str], Counter[str]]:
    expected: Counter[str] = Counter()
    unmapped: Counter[str] = Counter()
    for line in text.splitlines():
        kind_ids = re.findall(r"!notdec\.solidity\.checked_bounds !(\d+)", line)
        if not kind_ids or "!notdec.solidity_checked_bounds.skipped !" in line:
            continue
        for metadata_id in kind_ids:
            kind = metadata.get(metadata_id, "unknown")
            marker = KIND_TO_MARKER.get(kind)
            if marker:
                expected[marker] += 1
            else:
                unmapped[kind] += 1
    return expected, unmapped


def audit_file(path: Path) -> dict[str, Counter[str] | int]:
    text = path.read_text(errors="ignore")
    metadata = dict(METADATA_RE.findall(text))
    expected_markers, unmapped_rewrite_kinds = expected_markers_by_kind(text, metadata)
    return {
        "kinds": metadata_values(text, "notdec.solidity.checked_bounds", metadata),
        "panic_codes": metadata_values(
            text, "notdec.solidity_checked_bounds.panic_code", metadata
        ),
        "skip_reasons": metadata_values(
            text, "notdec.solidity_checked_bounds.skipped", metadata
        ),
        "semantic_markers": Counter(CHECKED_BOUNDS_MARKER_RE.findall(text)),
        "expected_markers": expected_markers,
        "unmapped_rewrite_kinds": unmapped_rewrite_kinds,
        "cfg_rewrites": len(CFG_REWRITE_RE.findall(text)),
    }


def print_counter(title: str, counts: Counter[str]) -> None:
    print(f"{title}: {sum(counts.values())}")
    for key, value in counts.most_common():
        print(f"  {key}: {value}")


def load_cpp_marker_mapping() -> dict[str, str]:
    source_path = (
        Path(__file__).resolve().parents[1]
        / "src/Passes/evm/SolidityPatterns.cpp"
    )
    text = source_path.read_text()
    match = CPP_MARKER_FUNCTION_RE.search(text)
    if not match:
        return {}
    return dict(CPP_MARKER_PAIR_RE.findall(match.group(1)))


def print_marker_mapping_mismatch(
    expected: dict[str, str], actual: dict[str, str]
) -> None:
    all_kinds = sorted(set(expected) | set(actual))
    for kind in all_kinds:
        expected_marker = expected.get(kind)
        actual_marker = actual.get(kind)
        if expected_marker != actual_marker:
            print(
                f"  {kind}: python={expected_marker or '<missing>'} "
                f"cpp={actual_marker or '<missing>'}"
            )


def write_csv_summary(
    path: Path,
    *,
    files: int,
    checked_bounds_total: int,
    skip_total: int,
    marker_total: int,
    cfg_rewrites: int,
    rewrite_expected: int,
    cpp_marker_mapping: str,
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
                "cpp_marker_mapping",
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
                "cpp_marker_mapping": cpp_marker_mapping,
            }
        )


def write_csv_rows(path: Path, rows: list[dict[str, str | int]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "input",
                "files",
                "checked_bounds_total",
                "skip_total",
                "semantic_marker_total",
                "cfg_rewrites",
                "rewrite_expected",
                "cpp_marker_mapping",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)


def collect_files(path: Path) -> list[Path]:
    if path.is_dir():
        return sorted(path.rglob("*.ll"))
    if path.suffix == ".ll":
        return [path]
    return []


def summarize_files(
    files: list[Path], cpp_marker_mapping_status: str
) -> tuple[dict[str, int | str | Counter[str]], list[tuple[Path, Counter[str]]]]:
    totals = {
        "kinds": Counter(),
        "panic_codes": Counter(),
        "skip_reasons": Counter(),
        "semantic_markers": Counter(),
        "expected_markers": Counter(),
        "unmapped_rewrite_kinds": Counter(),
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

    checked_bounds_total = sum(totals["kinds"].values())
    skip_total = sum(totals["skip_reasons"].values())
    rewrite_expected = checked_bounds_total - skip_total
    marker_total = sum(totals["semantic_markers"].values())
    summary = {
        **totals,
        "files": len(files),
        "cfg_rewrites": total_cfg_rewrites,
        "checked_bounds_total": checked_bounds_total,
        "skip_total": skip_total,
        "marker_total": marker_total,
        "rewrite_expected": rewrite_expected,
        "cpp_marker_mapping": cpp_marker_mapping_status,
    }
    return summary, skipped_files


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", help="Output directories or .ll files")
    parser.add_argument(
        "--csv",
        help="Write one-row CSV summary for batch trend tracking",
    )
    parser.add_argument(
        "--csv-by-path",
        help="Write one CSV row per input path for batch trend tracking",
    )
    parser.add_argument(
        "--csv-by-file",
        help="Write one CSV row per .ll file for locating checked-bounds cases",
    )
    parser.add_argument(
        "--csv-by-file-nonzero",
        action="store_true",
        help="Only include per-file CSV rows with checked-bounds metadata or skips",
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
            "matches semantic marker kinds and CFG rewrite counts"
        ),
    )
    args = parser.parse_args()

    cpp_marker_mapping = load_cpp_marker_mapping()
    cpp_marker_mapping_mismatch = cpp_marker_mapping != KIND_TO_MARKER
    cpp_marker_mapping_status = (
        "mismatch" if cpp_marker_mapping_mismatch else "matched"
    )

    input_files: list[tuple[str, list[Path]]] = []
    files: list[Path] = []
    for raw_path in args.paths:
        path = Path(raw_path)
        collected = collect_files(path)
        input_files.append((raw_path, collected))
        files.extend(collected)

    summary, skipped_files = summarize_files(files, cpp_marker_mapping_status)

    print(f"files: {len(files)}")
    print_counter("checked_bounds_kinds", summary["kinds"])  # type: ignore[arg-type]
    print_counter("panic_codes", summary["panic_codes"])  # type: ignore[arg-type]
    print_counter("semantic_markers", summary["semantic_markers"])  # type: ignore[arg-type]
    print_counter("expected_semantic_markers", summary["expected_markers"])  # type: ignore[arg-type]
    print_counter("unmapped_rewrite_kinds", summary["unmapped_rewrite_kinds"])  # type: ignore[arg-type]
    print_counter("skip_reasons", summary["skip_reasons"])  # type: ignore[arg-type]
    print(f"cfg_rewrites: {summary['cfg_rewrites']}")

    print(f"rewrite_expected: {summary['rewrite_expected']}")
    print(f"rewrite_markers: {summary['marker_total']}")
    print(f"cpp_marker_mapping: {cpp_marker_mapping_status}")

    if args.csv:
        write_csv_summary(
            Path(args.csv),
            files=int(summary["files"]),
            checked_bounds_total=int(summary["checked_bounds_total"]),
            skip_total=int(summary["skip_total"]),
            marker_total=int(summary["marker_total"]),
            cfg_rewrites=int(summary["cfg_rewrites"]),
            rewrite_expected=int(summary["rewrite_expected"]),
            cpp_marker_mapping=str(summary["cpp_marker_mapping"]),
        )

    if args.csv_by_path:
        rows: list[dict[str, str | int]] = []
        for raw_path, collected in input_files:
            path_summary, _ = summarize_files(collected, cpp_marker_mapping_status)
            rows.append(
                {
                    "input": raw_path,
                    "files": int(path_summary["files"]),
                    "checked_bounds_total": int(
                        path_summary["checked_bounds_total"]
                    ),
                    "skip_total": int(path_summary["skip_total"]),
                    "semantic_marker_total": int(path_summary["marker_total"]),
                    "cfg_rewrites": int(path_summary["cfg_rewrites"]),
                    "rewrite_expected": int(path_summary["rewrite_expected"]),
                    "cpp_marker_mapping": str(path_summary["cpp_marker_mapping"]),
                }
            )
        write_csv_rows(Path(args.csv_by_path), rows)

    if args.csv_by_file:
        rows: list[dict[str, str | int]] = []
        for file_path in files:
            file_summary, _ = summarize_files([file_path], cpp_marker_mapping_status)
            if (
                args.csv_by_file_nonzero
                and int(file_summary["checked_bounds_total"]) == 0
                and int(file_summary["skip_total"]) == 0
            ):
                continue
            rows.append(
                {
                    "input": str(file_path),
                    "files": int(file_summary["files"]),
                    "checked_bounds_total": int(
                        file_summary["checked_bounds_total"]
                    ),
                    "skip_total": int(file_summary["skip_total"]),
                    "semantic_marker_total": int(file_summary["marker_total"]),
                    "cfg_rewrites": int(file_summary["cfg_rewrites"]),
                    "rewrite_expected": int(file_summary["rewrite_expected"]),
                    "cpp_marker_mapping": str(file_summary["cpp_marker_mapping"]),
                }
            )
        write_csv_rows(Path(args.csv_by_file), rows)

    if args.list_skips and skipped_files:
        print("skip_files:")
        for path, reasons in skipped_files:
            reason_text = ", ".join(f"{k}:{v}" for k, v in reasons.most_common())
            print(f"  {path}: {reason_text}")

    marker_mismatch = summary["expected_markers"] != summary["semantic_markers"]
    if args.fail_on_mismatch and (
        summary["rewrite_expected"] != summary["marker_total"]
        or summary["rewrite_expected"] != summary["cfg_rewrites"]
        or marker_mismatch
        or summary["unmapped_rewrite_kinds"]
        or cpp_marker_mapping_mismatch
    ):
        print(
            "ERROR: checked-bounds rewrite mismatch: "
            f"expected={summary['rewrite_expected']} "
            f"markers={summary['marker_total']} "
            f"cfg_rewrites={summary['cfg_rewrites']}"
        )
        if marker_mismatch:
            all_markers = set(summary["expected_markers"]) | set(
                summary["semantic_markers"]
            )
            for marker in sorted(all_markers):
                expected = summary["expected_markers"].get(marker, 0)
                actual = summary["semantic_markers"].get(marker, 0)
                if expected != actual:
                    print(f"  {marker}: expected={expected} actual={actual}")
        if summary["unmapped_rewrite_kinds"]:
            for kind, count in summary["unmapped_rewrite_kinds"].most_common():
                print(f"  unmapped kind {kind}: {count}")
        if cpp_marker_mapping_mismatch:
            print("  C++ marker mapping differs from Python audit mapping:")
            print_marker_mapping_mismatch(KIND_TO_MARKER, cpp_marker_mapping)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
