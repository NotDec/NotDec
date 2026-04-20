#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


ENTRY_RE = re.compile(r"^(0x[0-9a-fA-F]+):(\s*)(DW_TAG_[A-Za-z0-9_]+|NULL)\s*$")
ATTR_RE = re.compile(r"^\s+DW_AT_([A-Za-z0-9_]+)\s+\((.*)\)\s*$")
ADDR_SIZE_RE = re.compile(r"addr_size = 0x([0-9a-fA-F]+)")
REF_RE = re.compile(r"^0x([0-9a-fA-F]+)(?:\s+\".*\")?$")
HEX_RE = re.compile(r"^0x([0-9a-fA-F]+)$")
DEC_RE = re.compile(r"^-?\d+$")
ADDR_LOC_RE = re.compile(r"DW_OP_addr 0x([0-9a-fA-F]+)")


@dataclass
class DwarfNode:
    offset: int
    tag: str
    attrs: dict[str, str] = field(default_factory=dict)
    children: list["DwarfNode"] = field(default_factory=list)


def run_dwarfdump(input_path: Path) -> str:
    return subprocess.run(
        ["llvm-dwarfdump", str(input_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=True,
    ).stdout


def parse_dwarfdump(text: str) -> tuple[list[DwarfNode], int]:
    roots: list[DwarfNode] = []
    stack: list[DwarfNode] = []
    current: DwarfNode | None = None
    addr_size_bits = 0

    for line in text.splitlines():
        if addr_size_bits == 0:
            match = ADDR_SIZE_RE.search(line)
            if match:
                addr_size_bits = int(match.group(1), 16) * 8

        entry_match = ENTRY_RE.match(line)
        if entry_match:
            tag = entry_match.group(3)
            current = None
            if tag == "NULL":
                continue

            offset = int(entry_match.group(1), 16)
            depth = len(entry_match.group(2)) // 2
            node = DwarfNode(offset=offset, tag=tag)
            while len(stack) > depth:
                stack.pop()
            if stack:
                stack[-1].children.append(node)
            else:
                roots.append(node)
            stack.append(node)
            current = node
            continue

        if current is None:
            continue

        attr_match = ATTR_RE.match(line)
        if not attr_match:
            continue
        current.attrs[attr_match.group(1)] = attr_match.group(2).strip()

    return roots, addr_size_bits


def flatten_nodes(roots: list[DwarfNode]) -> dict[int, DwarfNode]:
    nodes: dict[int, DwarfNode] = {}

    def visit(node: DwarfNode) -> None:
        nodes[node.offset] = node
        for child in node.children:
            visit(child)

    for root in roots:
        visit(root)
    return nodes


def parse_ref(raw: str | None) -> int | None:
    if raw is None:
        return None
    match = REF_RE.match(raw.strip())
    if match:
        return int(match.group(1), 16)
    return None


def parse_int(raw: str | None) -> int | None:
    if raw is None:
        return None
    text = raw.strip()
    hex_match = HEX_RE.match(text)
    if hex_match:
        return int(hex_match.group(1), 16)
    if DEC_RE.match(text):
        return int(text)
    return None


def parse_string(raw: str | None) -> str | None:
    if raw is None:
        return None
    text = raw.strip()
    if text.startswith('"') and text.endswith('"'):
        return text[1:-1]
    return None


def parse_bool(raw: str | None) -> bool:
    return raw == "true"


def parse_location(raw: str | None) -> dict | None:
    if raw is None:
        return None

    text = raw.strip()
    addrs = [int(value, 16) for value in ADDR_LOC_RE.findall(text)]
    if text.startswith("DW_OP_addr 0x") and "," not in text:
        return {"kind": "addr", "address": addrs[0]}
    if "DW_OP_piece" in text and addrs:
        return {
            "kind": "pieces",
            "addresses": addrs,
            "base_address": min(addrs),
        }
    return {"kind": "expr", "raw": text}


class TruthBuilder:
    def __init__(self, roots: list[DwarfNode], addr_size_bits: int):
        self.roots = roots
        self.nodes = flatten_nodes(roots)
        self.addr_size_bits = addr_size_bits
        self.record_aliases = self._build_record_aliases()

    def _build_record_aliases(self) -> dict[int, str]:
        aliases: dict[int, str] = {}
        for node in self.nodes.values():
            if node.tag != "DW_TAG_typedef":
                continue
            name = parse_string(node.attrs.get("name"))
            target_ref = parse_ref(node.attrs.get("type"))
            if not name or target_ref is None:
                continue
            target = self.nodes.get(target_ref)
            if target is None or target.tag != "DW_TAG_structure_type":
                continue
            aliases.setdefault(target_ref, f"struct.{name}")
        return aliases

    def record_name(self, offset: int) -> str:
        node = self.nodes[offset]
        name = parse_string(node.attrs.get("name"))
        if name:
            return f"struct.{name}"
        alias = self.record_aliases.get(offset)
        if alias is not None:
            return alias
        return f"anon.0x{offset:x}"

    def resolve_type(self, ref: int | None, visited: set[int] | None = None) -> dict:
        if ref is None:
            return {"kind": "unknown"}

        if visited is None:
            visited = set()
        if ref in visited:
            return {"kind": "cycle", "ref": f"0x{ref:x}"}
        visited = set(visited)
        visited.add(ref)

        node = self.nodes.get(ref)
        if node is None:
            return {"kind": "unknown", "ref": f"0x{ref:x}"}

        if node.tag == "DW_TAG_base_type":
            return {
                "kind": "primitive",
                "name": parse_string(node.attrs.get("name")) or "primitive",
                "encoding": node.attrs.get("encoding"),
                "size_bits": (parse_int(node.attrs.get("byte_size")) or 0) * 8,
            }

        if node.tag == "DW_TAG_pointer_type":
            return {
                "kind": "pointer",
                "size_bits": (parse_int(node.attrs.get("byte_size")) or (self.addr_size_bits // 8)) * 8,
                "to": self.resolve_type(parse_ref(node.attrs.get("type")), visited),
            }

        if node.tag == "DW_TAG_array_type":
            count = None
            for child in node.children:
                if child.tag != "DW_TAG_subrange_type":
                    continue
                count = parse_int(child.attrs.get("count"))
                if count is not None:
                    break
            return {
                "kind": "array",
                "count": count,
                "elem": self.resolve_type(parse_ref(node.attrs.get("type")), visited),
            }

        if node.tag == "DW_TAG_structure_type":
            return {
                "kind": "record_ref",
                "name": self.record_name(ref),
            }

        if node.tag in {
            "DW_TAG_typedef",
            "DW_TAG_const_type",
            "DW_TAG_volatile_type",
            "DW_TAG_restrict_type",
        }:
            return self.resolve_type(parse_ref(node.attrs.get("type")), visited)

        return {
            "kind": "unknown",
            "ref": f"0x{ref:x}",
            "tag": node.tag,
        }

    def extract_records(self) -> dict[str, dict]:
        records: dict[str, dict] = {}
        for node in self.nodes.values():
            if node.tag != "DW_TAG_structure_type":
                continue
            if parse_bool(node.attrs.get("declaration")):
                continue

            name = self.record_name(node.offset)
            fields: list[dict] = []
            for child in node.children:
                if child.tag != "DW_TAG_member":
                    continue
                field_name = parse_string(child.attrs.get("name")) or f"field_0x{child.offset:x}"
                field_offset_bytes = parse_int(child.attrs.get("data_member_location")) or 0
                field_size_bytes = parse_int(child.attrs.get("byte_size")) or 0
                fields.append(
                    {
                        "name": field_name,
                        "offset_bits": field_offset_bytes * 8,
                        "size_bits": field_size_bytes * 8,
                        "type": self.resolve_type(parse_ref(child.attrs.get("type"))),
                    }
                )

            records[name] = {
                "name": name,
                "size_bits": (parse_int(node.attrs.get("byte_size")) or 0) * 8,
                "fields": fields,
            }
        return records

    def extract_bindings(self) -> dict[str, dict]:
        bindings: dict[str, dict] = {}
        for root in self.roots:
            if root.tag != "DW_TAG_compile_unit":
                continue

            for child in root.children:
                if child.tag == "DW_TAG_variable":
                    name = parse_string(child.attrs.get("name"))
                    type_ref = parse_ref(child.attrs.get("type"))
                    if not name or type_ref is None:
                        continue
                    bindings[f"@{name}"] = {
                        "kind": "global",
                        "type": self.resolve_type(type_ref),
                        "location": parse_location(child.attrs.get("location")),
                    }
                    continue

                if child.tag != "DW_TAG_subprogram":
                    continue
                function_name = parse_string(child.attrs.get("name"))
                if not function_name:
                    continue

                arg_index = 0
                for grandchild in child.children:
                    if grandchild.tag != "DW_TAG_formal_parameter":
                        continue
                    type_ref = parse_ref(grandchild.attrs.get("type"))
                    if type_ref is None:
                        continue
                    bindings[f"{function_name}::arg{arg_index}"] = {
                        "kind": "function_param",
                        "type": self.resolve_type(type_ref),
                        "location": parse_location(grandchild.attrs.get("location")),
                    }
                    arg_index += 1
        return bindings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    output_path = Path(args.output).resolve()
    dwarf_text = run_dwarfdump(input_path)
    roots, addr_size_bits = parse_dwarfdump(dwarf_text)
    builder = TruthBuilder(roots, addr_size_bits=addr_size_bits or 32)

    result = {
        "source": str(input_path),
        "source_sha256": hashlib.sha256(input_path.read_bytes()).hexdigest(),
        "addr_size_bits": addr_size_bits or 32,
        "records": builder.extract_records(),
        "bindings": builder.extract_bindings(),
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
