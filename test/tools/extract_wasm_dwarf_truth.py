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
IDENT_RE = re.compile(r"[^A-Za-z0-9_]+")


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
    if not addrs:
        return {"kind": "expr", "raw": text}
    if text.startswith("DW_OP_addr 0x") and "," not in text:
        return {"kind": "addr", "address": addrs[0], "base_address": addrs[0]}
    if "DW_OP_piece" in text and addrs:
        return {
            "kind": "pieces",
            "addresses": addrs,
            "base_address": min(addrs),
        }
    return {"kind": "expr", "raw": text, "base_address": min(addrs)}


def sanitize_identifier(text: str) -> str:
    result = IDENT_RE.sub("_", text).strip("_")
    if not result:
        result = "unnamed"
    if result[0].isdigit():
        result = f"n_{result}"
    return result


class TruthBuilder:
    def __init__(self, roots: list[DwarfNode], addr_size_bits: int):
        self.roots = roots
        self.nodes = flatten_nodes(roots)
        self.addr_size_bits = addr_size_bits
        self.record_aliases = self._build_record_aliases()
        self.record_decl_names = self._build_record_decl_names()

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

    def _build_record_decl_names(self) -> dict[int, str]:
        names: dict[int, str] = {}
        used: set[str] = set()
        offsets = sorted(
            node.offset
            for node in self.nodes.values()
            if node.tag == "DW_TAG_structure_type" and not parse_bool(node.attrs.get("declaration"))
        )
        for offset in offsets:
            base = sanitize_identifier(self.record_name(offset).replace(".", "_"))
            candidate = base
            suffix = 1
            while candidate in used:
                suffix += 1
                candidate = f"{base}_{suffix}"
            used.add(candidate)
            names[offset] = candidate
        return names

    def record_decl_name(self, offset: int) -> str:
        return self.record_decl_names[offset]

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

        if node.tag == "DW_TAG_subroutine_type":
            params: list[dict] = []
            for child in node.children:
                if child.tag != "DW_TAG_formal_parameter":
                    continue
                params.append(self.resolve_type(parse_ref(child.attrs.get("type")), visited))
            ret_ref = parse_ref(node.attrs.get("type"))
            return {
                "kind": "function",
                "return": {"kind": "void"} if ret_ref is None else self.resolve_type(ret_ref, visited),
                "params": params,
            }

        if node.tag == "DW_TAG_unspecified_type":
            name = parse_string(node.attrs.get("name"))
            if name == "void":
                return {"kind": "void"}
            return {"kind": "unknown", "name": name or "unspecified"}

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
        for node in sorted(self.nodes.values(), key=lambda item: item.offset):
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
                        "decl_name": sanitize_identifier(field_name),
                        "offset_bits": field_offset_bytes * 8,
                        "size_bits": field_size_bytes * 8,
                        "type": self.resolve_type(parse_ref(child.attrs.get("type"))),
                    }
                )
            fields.sort(key=lambda item: (item["offset_bits"], item["name"], json.dumps(item["type"], sort_keys=True)))

            records[name] = {
                "name": name,
                "decl_name": self.record_decl_name(node.offset),
                "size_bits": (parse_int(node.attrs.get("byte_size")) or 0) * 8,
                "fields": fields,
            }
        return records

    def extract_globals(self) -> list[dict]:
        globals_out: list[dict] = []
        for root in self.roots:
            if root.tag != "DW_TAG_compile_unit":
                continue

            for child in root.children:
                if child.tag != "DW_TAG_variable":
                    continue
                name = parse_string(child.attrs.get("name"))
                type_ref = parse_ref(child.attrs.get("type"))
                if not name or type_ref is None:
                    continue
                location = parse_location(child.attrs.get("location"))
                globals_out.append(
                    {
                        "binding": f"@{name}",
                        "name": name,
                        "decl_name": sanitize_identifier(name),
                        "kind": "global",
                        "type": self.resolve_type(type_ref),
                        "location": location,
                        "offset_bytes": None
                        if location is None or location.get("base_address") is None
                        else int(location["base_address"]),
                    }
                )

        globals_out.sort(
            key=lambda item: (
                item["offset_bytes"] is None,
                -1 if item["offset_bytes"] is None else item["offset_bytes"],
                item["binding"],
            )
        )
        return globals_out

    def extract_functions(self) -> dict[str, dict]:
        functions: dict[str, dict] = {}
        for root in self.roots:
            if root.tag != "DW_TAG_compile_unit":
                continue

            for child in root.children:
                if child.tag == "DW_TAG_variable":
                    continue

                if child.tag != "DW_TAG_subprogram":
                    continue
                function_name = parse_string(child.attrs.get("name"))
                if not function_name:
                    continue

                ret_ref = parse_ref(child.attrs.get("type"))
                params: list[dict] = []
                for grandchild in child.children:
                    if grandchild.tag != "DW_TAG_formal_parameter":
                        continue
                    type_ref = parse_ref(grandchild.attrs.get("type"))
                    if type_ref is None:
                        continue
                    params.append(
                        {
                            "index": len(params),
                            "binding": f"{function_name}::arg{len(params)}",
                            "type": self.resolve_type(type_ref),
                            "location": parse_location(grandchild.attrs.get("location")),
                        }
                    )
                functions[function_name] = {
                    "name": function_name,
                    "return_type": {"kind": "void"}
                    if ret_ref is None
                    else self.resolve_type(ret_ref),
                    "params": params,
                }
        return dict(sorted(functions.items()))

    def extract_bindings(self, globals_out: list[dict], functions: dict[str, dict]) -> dict[str, dict]:
        bindings: dict[str, dict] = {}
        for item in globals_out:
            bindings[item["binding"]] = {
                "kind": item["kind"],
                "type": item["type"],
                "location": item["location"],
            }
        for function_name, function in functions.items():
            bindings[f"{function_name}::<ret>"] = {
                "kind": "function_return",
                "type": function["return_type"],
                "location": None,
            }
            for param in function["params"]:
                bindings[param["binding"]] = {
                    "kind": "function_param",
                    "type": param["type"],
                    "location": param["location"],
                }
        return bindings

    def type_size_bits(self, ty: dict, records: dict[str, dict]) -> int | None:
        kind = ty["kind"]
        if kind == "primitive":
            size_bits = int(ty.get("size_bits", 0))
            return size_bits if size_bits > 0 else None
        if kind == "pointer":
            size_bits = int(ty.get("size_bits", 0))
            return size_bits if size_bits > 0 else None
        if kind == "array":
            count = ty.get("count")
            if count is None:
                return None
            elem_size = self.type_size_bits(ty["elem"], records)
            return None if elem_size is None else elem_size * int(count)
        if kind == "record_ref":
            record = records.get(ty["name"])
            if record is None:
                return None
            size_bits = int(record.get("size_bits", 0))
            return size_bits if size_bits > 0 else None
        if kind == "void":
            return 0
        return None

    def render_type_text(self, ty: dict) -> str:
        kind = ty["kind"]
        if kind == "void":
            return "void"
        if kind == "primitive":
            name = ty.get("name")
            encoding = ty.get("encoding")
            size_bits = int(ty.get("size_bits", 0))
            if name == "_Bool":
                return "bool"
            if name in {"char", "signed char"}:
                return "i8"
            if name == "unsigned char":
                return "u8"
            if encoding == "DW_ATE_signed":
                return f"i{size_bits}"
            if encoding in {"DW_ATE_unsigned", "DW_ATE_unsigned_char", "DW_ATE_boolean"}:
                return f"u{size_bits}"
            if encoding in {"DW_ATE_float", "DW_ATE_complex_float"}:
                return f"f{size_bits}"
            return f"{sanitize_identifier(str(name or 'primitive'))}:{size_bits}"
        if kind == "pointer":
            return self.render_type_text(ty["to"]) + "*"
        if kind == "array":
            count = ty.get("count")
            suffix = "" if count is None else str(count)
            return f"{self.render_type_text(ty['elem'])}[{suffix}]"
        if kind == "record_ref":
            target_name = ty["name"]
            for offset, record_name in ((offset, self.record_name(offset)) for offset in self.record_decl_names):
                if record_name == target_name:
                    return self.record_decl_name(offset)
            return sanitize_identifier(target_name.replace(".", "_"))
        if kind == "function":
            params = ", ".join(self.render_type_text(param) for param in ty["params"])
            return f"{self.render_type_text(ty['return'])} (*)({params})"
        if kind == "cycle":
            return f"cycle({ty.get('ref', '?')})"
        return f"unknown({sanitize_identifier(str(ty.get('tag', ty.get('name', kind))))})"


def render_decl(name: str, fields: list[dict]) -> str:
    lines = [f"struct {name} {{"]
    cursor_bytes = 0
    padding_index = 0
    for field in fields:
        offset_bytes = int(field["offset_bits"]) // 8
        if offset_bytes > cursor_bytes:
            padding_bytes = offset_bytes - cursor_bytes
            lines.append(
                f"  u8[{padding_bytes}] padding_{padding_index}; /* at offset: {cursor_bytes} */"
            )
            padding_index += 1
        lines.append(
            f"  {field['type_text']} {field['decl_name']}; /* at offset: {offset_bytes} */"
        )
        size_bits = field.get("size_bits")
        if isinstance(size_bits, int) and size_bits > 0:
            cursor_bytes = max(cursor_bytes, offset_bytes + (size_bits // 8))
        else:
            cursor_bytes = max(cursor_bytes, offset_bytes)
    lines.append("};")
    return "\n".join(lines)


def write_text_output(output_path: Path, result: dict) -> None:
    builder: TruthBuilder = result["_builder"]
    records: dict[str, dict] = result["records"]
    globals_out: list[dict] = result["globals"]
    functions: dict[str, dict] = result["functions"]

    memory_decl_name = "dwarf_memory"
    memory_fields: list[dict] = []
    for item in globals_out:
        offset_bytes = item["offset_bytes"]
        if offset_bytes is None:
            continue
        memory_fields.append(
            {
                "decl_name": item["decl_name"],
                "type_text": builder.render_type_text(item["type"]),
                "offset_bits": offset_bytes * 8,
                "size_bits": builder.type_size_bits(item["type"], records) or 0,
            }
        )

    decl_blocks = [render_decl(memory_decl_name, memory_fields)]
    for record in sorted(records.values(), key=lambda item: item["decl_name"]):
        fields = [
            {
                "decl_name": field["decl_name"],
                "type_text": builder.render_type_text(field["type"]),
                "offset_bits": field["offset_bits"],
                "size_bits": field["size_bits"],
            }
            for field in record["fields"]
        ]
        decl_blocks.append(render_decl(str(record["decl_name"]), fields))

    lines = [
        "# WasmDwarfTruth",
        "",
        f"source => {result['source']}",
        f"source_sha256 => {result['source_sha256']}",
        "",
        "[memory]",
        f"decl => {memory_decl_name}",
        f"type => {memory_decl_name}",
    ]
    if globals_out:
        lines.append("globals:")
        for item in globals_out:
            offset = "<unknown>"
            if item["offset_bytes"] is not None:
                offset = f"0x{item['offset_bytes']:x} ({item['offset_bytes']})"
            lines.append(
                f"  {offset} {item['binding']} => {builder.render_type_text(item['type'])}"
            )
    lines.extend(["", "[functions]"])

    for function_name, function in functions.items():
        lines.append(f"@{function_name}")
        lines.append(f"  ret => {builder.render_type_text(function['return_type'])}")
        for param in function["params"]:
            lines.append(
                f"  arg{param['index']} => {builder.render_type_text(param['type'])}"
            )
        lines.append("")

    lines.append("[decls]")
    lines.append("")
    lines.append("\n\n".join(decl_blocks))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines).rstrip() + "\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--json-output")
    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    output_path = Path(args.output).resolve()
    dwarf_text = run_dwarfdump(input_path)
    roots, addr_size_bits = parse_dwarfdump(dwarf_text)
    builder = TruthBuilder(roots, addr_size_bits=addr_size_bits or 32)
    records = builder.extract_records()
    globals_out = builder.extract_globals()
    functions = builder.extract_functions()

    result = {
        "source": str(input_path),
        "source_sha256": hashlib.sha256(input_path.read_bytes()).hexdigest(),
        "addr_size_bits": addr_size_bits or 32,
        "records": records,
        "globals": globals_out,
        "functions": functions,
        "bindings": builder.extract_bindings(globals_out, functions),
        "_builder": builder,
    }

    write_text_output(output_path, result)

    if args.json_output:
        json_path = Path(args.json_output).resolve()
        json_path.parent.mkdir(parents=True, exist_ok=True)
        json_result = dict(result)
        del json_result["_builder"]
        json_path.write_text(json.dumps(json_result, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
