#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import sys
from dataclasses import dataclass
from pathlib import Path

import pyarrow.parquet as pq


DEFAULT_DATASET_ROOT = Path("/sn640/NotDecChainExp/apehex_evm_contracts")
DEFAULT_SPLIT = Path("hex/ethereum/cleaned")


@dataclass(frozen=True)
class Candidate:
    # This keeps the exact apehex provenance next to the sortable facts.  When a
    # candidate is promoted into the source suite, the row can be found again.
    parquet: str
    row_index: int
    block_number: int
    transaction_hash: str
    contract_address: str
    creation_bytecode: str
    runtime_bytecode: str
    creation_sourcecode: str

    @property
    def name(self) -> str:
        return (
            f"{self.block_number}_"
            f"{self.contract_address[:10]}_"
            f"{self.transaction_hash[:12]}"
        )

    @property
    def creation_bytes(self) -> int:
        return len(self.creation_bytecode) // 2

    @property
    def runtime_bytes(self) -> int:
        return len(self.runtime_bytecode) // 2

    @property
    def source_bytes(self) -> int:
        return len(self.creation_sourcecode.encode("utf-8"))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="List apehex source-backed contracts by runtime bytecode size."
    )
    parser.add_argument(
        "--dataset-root",
        type=Path,
        default=DEFAULT_DATASET_ROOT,
        help="apehex_evm_contracts root.",
    )
    parser.add_argument(
        "--split",
        type=Path,
        default=DEFAULT_SPLIT,
        help="Parquet split under the dataset root.",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=50,
        help="Maximum number of sorted candidates to print.",
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=2048,
        help="Rows read from parquet at a time.",
    )
    parser.add_argument(
        "--language",
        choices=["solidity", "any"],
        default="solidity",
        help="Default keeps rows that look like Solidity source.",
    )
    parser.add_argument(
        "--source-contains",
        action="append",
        default=[],
        help="Extra case-insensitive source substring filter. Can be repeated.",
    )
    parser.add_argument(
        "--no-dedupe-runtime",
        action="store_true",
        help="Do not drop duplicate runtime bytecode.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="CSV output path. Defaults to stdout.",
    )
    parser.add_argument(
        "--export-dir",
        type=Path,
        default=None,
        help="Optional staging directory for runtime.hex, source.txt and metadata.json.",
    )
    return parser.parse_args()


def clean_hex(value: str | None) -> str:
    if not value:
        return ""
    text = "".join(value.strip().split())
    if text.startswith("0x"):
        text = text[2:]
    return text.lower()


def clean_source(value: str | None) -> str:
    return (value or "").strip()


def looks_like_solidity(source: str) -> bool:
    lowered = source.lower()
    return "pragma solidity" in lowered or (
        '"language"' in lowered and "solidity" in lowered
    )


def source_matches_filters(
    source: str, language: str, contains_filters: list[str]
) -> bool:
    if not source:
        return False
    if language == "solidity" and not looks_like_solidity(source):
        return False
    lowered = source.lower()
    return all(text.lower() in lowered for text in contains_filters)


def iter_candidates(
    dataset_root: Path,
    split: Path,
    batch_size: int,
    language: str,
    contains_filters: list[str],
) -> list[Candidate]:
    base = dataset_root / split
    if not base.exists():
        raise FileNotFoundError(f"missing apehex split: {base}")

    columns = [
        "block_number",
        "transaction_hash",
        "contract_address",
        "creation_bytecode",
        "runtime_bytecode",
        "creation_sourcecode",
    ]
    candidates: list[Candidate] = []
    for parquet_file in sorted(base.glob("*.parquet")):
        row_index = 0
        parquet = pq.ParquetFile(parquet_file)
        for batch in parquet.iter_batches(batch_size=batch_size, columns=columns):
            for row in batch.to_pylist():
                source = clean_source(row.get("creation_sourcecode"))
                runtime = clean_hex(row.get("runtime_bytecode"))
                if not runtime or not source_matches_filters(
                    source, language, contains_filters
                ):
                    row_index += 1
                    continue

                candidates.append(
                    Candidate(
                        parquet=parquet_file.name,
                        row_index=row_index,
                        block_number=int(row["block_number"]),
                        transaction_hash=row["transaction_hash"],
                        contract_address=row["contract_address"],
                        creation_bytecode=clean_hex(row.get("creation_bytecode")),
                        runtime_bytecode=runtime,
                        creation_sourcecode=source,
                    )
                )
                row_index += 1
    return candidates


def sort_and_limit(
    candidates: list[Candidate], limit: int, dedupe_runtime: bool
) -> list[Candidate]:
    unique: list[Candidate] = []
    seen_runtime: set[str] = set()
    for candidate in candidates:
        if dedupe_runtime and candidate.runtime_bytecode in seen_runtime:
            continue
        seen_runtime.add(candidate.runtime_bytecode)
        unique.append(candidate)

    unique.sort(
        key=lambda item: (
            item.runtime_bytes,
            item.creation_bytes,
            item.source_bytes,
            item.block_number,
            item.contract_address,
        )
    )
    return unique[:limit]


def candidate_row(rank: int, candidate: Candidate) -> dict[str, object]:
    return {
        "rank": rank,
        "name": candidate.name,
        "parquet": candidate.parquet,
        "row_index": candidate.row_index,
        "block_number": candidate.block_number,
        "contract_address": candidate.contract_address,
        "transaction_hash": candidate.transaction_hash,
        "runtime_bytes": candidate.runtime_bytes,
        "creation_bytes": candidate.creation_bytes,
        "source_bytes": candidate.source_bytes,
        "runtime_prefix": candidate.runtime_bytecode[:64],
        "source_prefix": " ".join(candidate.creation_sourcecode.split())[:120],
    }


def write_csv(candidates: list[Candidate], output: Path | None) -> None:
    fieldnames = [
        "rank",
        "name",
        "parquet",
        "row_index",
        "block_number",
        "contract_address",
        "transaction_hash",
        "runtime_bytes",
        "creation_bytes",
        "source_bytes",
        "runtime_prefix",
        "source_prefix",
    ]
    if output is None:
        file = sys.stdout
        close = False
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        file = output.open("w", newline="")
        close = True

    try:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()
        for rank, candidate in enumerate(candidates, start=1):
            writer.writerow(candidate_row(rank, candidate))
    finally:
        if close:
            file.close()


def export_candidates(candidates: list[Candidate], export_dir: Path) -> None:
    export_dir.mkdir(parents=True, exist_ok=True)
    for rank, candidate in enumerate(candidates, start=1):
        case_dir = export_dir / f"{rank:04d}_{candidate.name}"
        case_dir.mkdir(parents=True, exist_ok=True)
        (case_dir / "runtime.hex").write_text(candidate.runtime_bytecode + "\n")
        (case_dir / "source.txt").write_text(candidate.creation_sourcecode + "\n")
        metadata = candidate_row(rank, candidate)
        (case_dir / "metadata.json").write_text(
            json.dumps(metadata, indent=2, sort_keys=True) + "\n"
        )


def main() -> int:
    args = parse_args()
    candidates = iter_candidates(
        args.dataset_root,
        args.split,
        args.batch_size,
        args.language,
        args.source_contains,
    )
    selected = sort_and_limit(
        candidates,
        args.limit,
        dedupe_runtime=not args.no_dedupe_runtime,
    )
    write_csv(selected, args.output)
    if args.export_dir is not None:
        export_candidates(selected, args.export_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
