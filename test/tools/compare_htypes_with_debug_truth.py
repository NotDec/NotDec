#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path


TYPE_LINE_RE = re.compile(r"^\[[+-]\]\s+(.+?)\s+=>\s+(.+)$")
FIELD_LINE_RE = re.compile(
    r"^\s*(.+?)\s+([A-Za-z0-9_]+);\s*(?:/\*\s*at offset:\s*([0-9]+)\s*\*/)?\s*$"
)
ADDR_KEY_RE = re.compile(r"^addr\(0x([0-9a-fA-F]+)\)$")


def split_top_level(text: str, delimiter: str) -> list[str]:
    parts: list[str] = []
    depth_paren = 0
    depth_angle = 0
    depth_bracket = 0
    current: list[str] = []
    for ch in text:
        if ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren -= 1
        elif ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle -= 1
        elif ch == "[":
            depth_bracket += 1
        elif ch == "]":
            depth_bracket -= 1
        elif (
            ch == delimiter
            and depth_paren == 0
            and depth_angle == 0
            and depth_bracket == 0
        ):
            parts.append("".join(current).strip())
            current = []
            continue
        current.append(ch)
    tail = "".join(current).strip()
    if tail:
        parts.append(tail)
    return parts


def canonical_type(raw: str) -> str:
    return "".join(raw.split())


def strip_outer_parens(raw: str) -> str:
    text = raw.strip()
    while text.startswith("(") and text.endswith(")"):
        depth = 0
        balanced = True
        for index, ch in enumerate(text):
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0 and index != len(text) - 1:
                    balanced = False
                    break
        if not balanced or depth != 0:
            break
        text = text[1:-1].strip()
    return text


def primitive_width(raw: str) -> int | None:
    match = re.fullmatch(r"(?:'[^:]+|top|bottom):(\d+)", raw)
    if match:
        return int(match.group(1))
    match = re.fullmatch(r"[iuf](\d+)", raw)
    if match:
        return int(match.group(1))
    return None


def parse_type_expr(raw: str) -> dict:
    text = strip_outer_parens(raw)

    parts = split_top_level(text, "|")
    if len(parts) > 1:
        return {"kind": "union", "items": [parse_type_expr(part) for part in parts], "raw": text}

    parts = split_top_level(text, "&")
    if len(parts) > 1:
        return {
            "kind": "intersection",
            "items": [parse_type_expr(part) for part in parts],
            "raw": text,
        }

    if text.startswith("ptr<") and text.endswith(">"):
        payload = text[4:-1]
        fields: dict[str, object] = {"kind": "pointer", "raw": text}
        for part in split_top_level(payload, ","):
            key, value = part.split("=", 1)
            key = key.strip()
            value = value.strip()
            if key == "psize":
                fields["psize"] = int(value)
            else:
                fields[key] = parse_type_expr(value)
                fields[f"{key}_raw"] = canonical_type(value)
        return fields

    array_match = re.fullmatch(r"(.+)\[(\d+)\]", text)
    if array_match:
        return {
            "kind": "array",
            "elem": parse_type_expr(array_match.group(1).strip()),
            "count": int(array_match.group(2)),
            "raw": text,
        }

    if text.endswith("*") and "(" not in text and " " not in text[:-1]:
        return {
            "kind": "pointer_ref",
            "to": parse_type_expr(text[:-1]),
            "raw": text,
        }

    if re.fullmatch(r"struct_[A-Za-z0-9_]+", text):
        return {"kind": "record_ref", "name": text, "raw": text}

    width = primitive_width(text)
    if width is not None:
        return {"kind": "primitive", "width_bits": width, "raw": text}

    if text == "void":
        return {"kind": "void", "raw": text}

    return {"kind": "unknown", "raw": text}


def expr_has_primitive(node: dict, expected_width: int | None) -> bool:
    kind = node["kind"]
    if kind == "primitive":
        width = node.get("width_bits")
        return expected_width is None or width == expected_width
    if kind in {"union", "intersection"}:
        return any(expr_has_primitive(item, expected_width) for item in node["items"])
    return False


def expr_has_pointer(node: dict) -> bool:
    kind = node["kind"]
    if kind in {"pointer", "pointer_ref"}:
        return True
    if kind in {"union", "intersection"}:
        return any(expr_has_pointer(item) for item in node["items"])
    return False


@dataclass
class FieldDecl:
    name: str
    offset_bytes: int | None
    expr_raw: str
    expr: dict


@dataclass
class StructDecl:
    name: str
    fields: list[FieldDecl]


@dataclass
class ParsedHTypes:
    decls: dict[str, StructDecl]
    types: dict[str, dict]
    types_raw: dict[str, str]
    memory_decl: str | None
    memory_type_raw: str | None
    memory_fields_by_offset: dict[int, FieldDecl]
    address_offsets_by_type: dict[str, list[int]]


def parse_htypes(path: Path) -> ParsedHTypes:
    text = path.read_text()
    section = None
    decl_lines: list[str] = []
    type_lines: list[str] = []
    memory_lines: list[str] = []

    for line in text.splitlines():
        stripped = line.strip()
        if stripped == "[decls]":
            section = "decls"
            continue
        if stripped == "[types]":
            section = "types"
            continue
        if stripped == "[memory]":
            section = "memory"
            continue
        if not stripped and section != "decls":
            continue
        if section == "decls":
            decl_lines.append(line)
        elif section == "types":
            type_lines.append(line)
        elif section == "memory":
            memory_lines.append(line)

    decls: dict[str, StructDecl] = {}
    index = 0
    while index < len(decl_lines):
        line = decl_lines[index].strip()
        if not line:
            index += 1
            continue
        if not line.startswith("struct "):
            index += 1
            continue
        name = line.split()[1]
        fields: list[FieldDecl] = []
        index += 1
        while index < len(decl_lines):
            body_line = decl_lines[index]
            if body_line.strip() == "};":
                break
            if body_line.strip():
                match = FIELD_LINE_RE.match(body_line)
                if match:
                    expr_raw = match.group(1).strip()
                    offset_bytes = int(match.group(3)) if match.group(3) else None
                    fields.append(
                        FieldDecl(
                            name=match.group(2),
                            offset_bytes=offset_bytes,
                            expr_raw=expr_raw,
                            expr=parse_type_expr(expr_raw),
                        )
                    )
            index += 1
        decls[name] = StructDecl(name=name, fields=fields)
        index += 1

    types: dict[str, dict] = {}
    types_raw: dict[str, str] = {}
    address_offsets_by_type: dict[str, list[int]] = {}
    for line in type_lines:
        match = TYPE_LINE_RE.match(line.strip())
        if not match:
            continue
        key = match.group(1).strip()
        raw = match.group(2).strip()
        types[key] = parse_type_expr(raw)
        types_raw[key] = raw
        address_match = ADDR_KEY_RE.match(key)
        if address_match:
            offset_bytes = int(address_match.group(1), 16)
            address_offsets_by_type.setdefault(canonical_type(raw), []).append(offset_bytes)

    memory_decl = None
    memory_type_raw = None
    for line in memory_lines:
        stripped = line.strip()
        if stripped.startswith("decl => "):
            memory_decl = stripped[len("decl => ") :].strip()
        if stripped.startswith("type => "):
            memory_type_raw = stripped[len("type => ") :].strip()

    memory_fields_by_offset: dict[int, FieldDecl] = {}
    if memory_decl and memory_decl in decls:
        for field in decls[memory_decl].fields:
            if field.offset_bytes is not None:
                memory_fields_by_offset[field.offset_bytes] = field

    return ParsedHTypes(
        decls=decls,
        types=types,
        types_raw=types_raw,
        memory_decl=memory_decl,
        memory_type_raw=memory_type_raw,
        memory_fields_by_offset=memory_fields_by_offset,
        address_offsets_by_type=address_offsets_by_type,
    )


class Comparator:
    def __init__(self, truth: dict, htypes: ParsedHTypes, config: dict):
        self.truth = truth
        self.htypes = htypes
        self.config = config

    def compare_root(self, root: dict) -> dict:
        truth_key = root["truth"]
        recovered_kind = root.get("recovered", "memory")
        base_offset_bytes = int(root.get("base_offset_bytes", 0))
        bindings = self.truth.get("bindings", {})
        truth_type = bindings.get(truth_key)
        if truth_type is None:
            return {
                "truth": truth_key,
                "passed": False,
                "mismatches": [f"missing truth binding: {truth_key}"],
            }

        if recovered_kind != "memory":
            return {
                "truth": truth_key,
                "passed": False,
                "mismatches": [f"unsupported recovered root kind: {recovered_kind}"],
            }

        if self.htypes.memory_decl is None:
            return {
                "truth": truth_key,
                "passed": False,
                "mismatches": ["missing [memory] decl in .htypes output"],
            }

        mismatches = self.compare_type(
            truth_type,
            {"kind": "memory_root", "base_offset_bytes": base_offset_bytes},
            path=truth_key,
            visited=set(),
            field_allowlist=self._field_allowlist(root),
        )
        return {
            "truth": truth_key,
            "recovered": recovered_kind,
            "base_offset_bytes": base_offset_bytes,
            "passed": not mismatches,
            "mismatches": mismatches,
        }

    def _field_allowlist(self, root: dict) -> set[str] | None:
        raw = root.get("field_allowlist")
        if raw is None:
            return None
        return {str(item) for item in raw}

    def compare_type(
        self,
        truth_type: dict,
        recovered: dict,
        *,
        path: str,
        visited: set[tuple[str, int]],
        field_allowlist: set[str] | None = None,
    ) -> list[str]:
        kind = truth_type["kind"]
        if kind == "pointer":
            if recovered["kind"] == "memory_root":
                return self.compare_type(
                    truth_type["to"],
                    recovered,
                    path=path + "->*",
                    visited=visited,
                    field_allowlist=field_allowlist,
                )
            if recovered["kind"] == "memory_slot":
                return self.compare_pointer_slot(truth_type, recovered, path=path, visited=visited)
            return [f"{path}: expected pointer, but recovered root kind is {recovered['kind']}"]

        if kind == "record_ref":
            if recovered["kind"] != "memory_root":
                return [f"{path}: record comparison only supports recovered memory roots for now"]
            return self.compare_record(
                truth_type["name"],
                recovered["base_offset_bytes"],
                path,
                visited,
                field_allowlist=field_allowlist,
            )

        if kind == "primitive":
            return self.compare_primitive(truth_type, recovered, path)

        return [f"{path}: unsupported truth kind {kind}"]

    def compare_record(
        self,
        record_name: str,
        base_offset_bytes: int,
        path: str,
        visited: set[tuple[str, int]],
        *,
        field_allowlist: set[str] | None = None,
    ) -> list[str]:
        marker = (record_name, base_offset_bytes)
        if marker in visited:
            return []
        visited.add(marker)

        record = self.truth["records"].get(record_name)
        if record is None:
            return [f"{path}: missing record definition {record_name}"]

        mismatches: list[str] = []
        for field in record["fields"]:
            if field_allowlist is not None and field["name"] not in field_allowlist:
                continue
            expected_offset = base_offset_bytes + field["offset_bits"] // 8
            recovered_field = self.htypes.memory_fields_by_offset.get(expected_offset)
            if recovered_field is None:
                mismatches.append(
                    f"{path}.{field['name']}: missing recovered field at absolute offset {expected_offset}"
                )
                continue
            mismatches.extend(
                self.compare_type(
                    field["type"],
                    {
                        "kind": "memory_slot",
                        "field": recovered_field,
                        "root_base_bytes": base_offset_bytes,
                    },
                    path=f"{path}.{field['name']}",
                    visited=visited,
                    field_allowlist=None,
                )
            )
        return mismatches

    def compare_primitive(self, truth_type: dict, recovered: dict, path: str) -> list[str]:
        expected_width = truth_type.get("size_bits")
        if recovered["kind"] == "memory_slot":
            expr = recovered["field"].expr
            raw = recovered["field"].expr_raw
            if expr["kind"] == "pointer" and "store" in expr:
                expr = expr["store"]
        elif recovered["kind"] == "expr":
            expr = recovered["expr"]
            raw = expr.get("raw", "<expr>")
        else:
            return [f"{path}: expected primitive, but recovered root kind is {recovered['kind']}"]

        if expr_has_primitive(expr, expected_width):
            return []
        return [f"{path}: expected primitive:{expected_width}, got {raw}"]

    def resolve_pointer_target(self, field: FieldDecl, root_base_bytes: int) -> dict | None:
        expr = field.expr
        if expr["kind"] != "pointer":
            if expr_has_pointer(expr):
                return {"kind": "opaque_pointer"}
            return None

        store_raw = expr.get("store_raw")
        if isinstance(store_raw, str):
            offsets = self.htypes.address_offsets_by_type.get(store_raw, [])
            if len(offsets) == 1:
                target_offset = offsets[0]
                if target_offset == root_base_bytes:
                    return {"kind": "memory_root", "base_offset_bytes": root_base_bytes}
                target_field = self.htypes.memory_fields_by_offset.get(target_offset)
                if target_field is not None:
                    return {
                        "kind": "memory_slot",
                        "field": target_field,
                        "root_base_bytes": root_base_bytes,
                    }
                return {"kind": "opaque_pointer"}
        return {"kind": "opaque_pointer"}

    def compare_pointer_slot(
        self,
        truth_type: dict,
        recovered: dict,
        *,
        path: str,
        visited: set[tuple[str, int]],
    ) -> list[str]:
        field = recovered["field"]
        target = self.resolve_pointer_target(field, recovered["root_base_bytes"])
        if target is None:
            return [f"{path}: expected pointer, got {field.expr_raw}"]

        target_truth = truth_type["to"]
        if target["kind"] == "opaque_pointer":
            return []
        return self.compare_type(target_truth, target, path=path + "->*", visited=visited)


def load_json(path: Path) -> dict:
    return json.loads(path.read_text())


def write_report_text(path: Path, report: dict) -> None:
    lines = []
    if report["passed"]:
        lines.append("PASS")
    else:
        lines.append("FAIL")
    for root in report["roots"]:
        status = "PASS" if root["passed"] else "FAIL"
        lines.append(f"{status} {root['truth']}")
        for mismatch in root["mismatches"]:
            lines.append(f"  {mismatch}")
    path.write_text("\n".join(lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--htypes", required=True)
    parser.add_argument("--truth", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--report-json", required=True)
    parser.add_argument("--report-text", required=True)
    args = parser.parse_args()

    truth = load_json(Path(args.truth).resolve())
    config = load_json(Path(args.config).resolve())
    htypes = parse_htypes(Path(args.htypes).resolve())

    comparator = Comparator(truth, htypes, config)
    roots = [comparator.compare_root(root) for root in config.get("roots", [])]
    passed = bool(roots) and all(root["passed"] for root in roots)
    report = {
        "passed": passed,
        "roots": roots,
    }

    report_json_path = Path(args.report_json).resolve()
    report_text_path = Path(args.report_text).resolve()
    report_json_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    write_report_text(report_text_path, report)
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
