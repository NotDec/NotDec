#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


def normalize_text(text: str) -> str:
    text = text.replace("\r\n", "\n").strip()
    if not text:
        return ""
    return "\n".join(line.rstrip() for line in text.split("\n")).strip()


def build_command(args: argparse.Namespace, output_path: Path) -> list[str]:
    command = [
        args.compiler,
        "--target=wasm32-unknown-wasi",
        "-opaque-pointers=0",
        "-S",
        "-emit-llvm",
        "-O0",
        "-g",
        "-Xclang",
        "-disable-O0-optnone",
        "-fno-builtin",
        "-fno-discard-value-names",
    ]
    for include_dir in args.include_dir:
        command.extend(["-I", str(Path(include_dir).resolve())])
    command.extend([str(Path(args.case).resolve()), "-o", str(output_path)])
    return command


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--compiler", default="clang-14")
    parser.add_argument("--include-dir", action="append", default=[])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    output_path = Path(args.output).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="notdec-debug-info-") as tmpdir:
        temp_output = Path(tmpdir) / output_path.name
        command = build_command(args, temp_output)
        process = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        if process.returncode != 0:
            sys.stdout.write(process.stdout)
            return process.returncode

        generated = temp_output.read_text()
        if args.check:
            if not output_path.exists():
                print(f"missing truth file: {output_path}")
                return 1
            existing = output_path.read_text()
            if normalize_text(existing) != normalize_text(generated):
                print(f"debug-info truth drifted: {output_path}")
                return 1
            return 0

        output_path.write_text(generated)
        print(output_path)
        return 0


if __name__ == "__main__":
    sys.exit(main())
