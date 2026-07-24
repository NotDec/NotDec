#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path


TYPE_LINE_RE = re.compile(r"^\[[+-]\]\s+(.+?)\s+=>\s+(.+)$")
TYPE_BOUNDS_LINE_RE = re.compile(
    r"^\[([+-])\]\s+(.+?)\s+=>\s+lower=(.+?)\s+;\s+upper=(.+)$"
)
FIELD_LINE_RE = re.compile(
    r"^\s*(.+?)\s+([A-Za-z0-9_]+);\s*(?:/\*\s*(?:recursive body\s+)?at offset:\s*([0-9-]+)\s*\*/)?\s*$"
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

    array_match = re.fullmatch(r"(.+)\[(\d*)\]", text)
    if array_match:
        count = array_match.group(2)
        return {
            "kind": "array",
            "elem": parse_type_expr(array_match.group(1).strip()),
            "count": int(count) if count else None,
            "raw": text,
        }

    if text.endswith("*") and "(" not in text and " " not in text[:-1]:
        return {
            "kind": "pointer_ref",
            "to": parse_type_expr(text[:-1]),
            "raw": text,
        }

    if re.fullmatch(r"(?:struct|rec)_[A-Za-z0-9_]+", text):
        return {"kind": "record_ref", "name": text, "raw": text}

    width = primitive_width(text)
    if width is not None:
        return {"kind": "primitive", "width_bits": width, "raw": text}

    if text == "void":
        return {"kind": "void", "raw": text}

    return {"kind": "unknown", "raw": text}


def parse_function_pointer_type(raw: str) -> dict | None:
    text = strip_outer_parens(raw)
    marker = " (*)("
    start = text.find(marker)
    if start < 0 or not text.endswith(")*"):
        return None

    ret_raw = text[:start].strip()
    params_end = -3 if text.endswith("))*") else -2
    params_raw = text[start + len(marker) : params_end].strip()
    if not ret_raw:
        return None

    params: list[dict] = []
    if params_raw and params_raw != "void":
        for item in split_top_level(params_raw, ","):
            params.append(parse_type_expr(item))

    return {
        "kind": "function_pointer",
        "raw": text,
        "ret": parse_type_expr(ret_raw),
        "params": params,
    }


def expr_has_primitive(node: dict, expected_width: int | None) -> bool:
    kind = node["kind"]
    if kind == "primitive":
        width = node.get("width_bits")
        return expected_width is None or width == expected_width
    if kind in {"union", "intersection"}:
        return any(expr_has_primitive(item, expected_width) for item in node["items"])
    return False


def find_record_pointer_targets(node: dict) -> set[str]:
    kind = node["kind"]
    if kind == "pointer_ref" and node["to"]["kind"] == "record_ref":
        return {node["to"]["name"]}
    if kind in {"union", "intersection"}:
        found: set[str] = set()
        for item in node["items"]:
            found.update(find_record_pointer_targets(item))
        return found
    if kind == "pointer":
        found: set[str] = set()
        for value in node.values():
            if isinstance(value, dict):
                found.update(find_record_value_targets(value))
                found.update(find_record_pointer_targets(value))
        return found
    return set()


def find_record_value_targets(node: dict) -> set[str]:
    kind = node["kind"]
    if kind == "record_ref":
        return {node["name"]}
    if kind == "pointer_ref" and node["to"]["kind"] == "record_ref":
        return {node["to"]["name"]}
    if kind in {"union", "intersection"}:
        found: set[str] = set()
        for item in node["items"]:
            found.update(find_record_value_targets(item))
        return found
    return set()


def memory_field_matches_primitive(field: FieldDecl, expected_width: int | None) -> bool:
    expr = field.expr
    if expr_has_primitive(expr, expected_width):
        return True
    if expr["kind"] == "pointer":
        for key in ("load", "store"):
            value = expr.get(key)
            if isinstance(value, dict) and expr_has_primitive(value, expected_width):
                return True
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
        if not line.startswith(("struct ", "union ")):
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
        stripped = line.strip()
        bounds_match = TYPE_BOUNDS_LINE_RE.match(stripped)
        if bounds_match:
            sign = bounds_match.group(1)
            key = bounds_match.group(2).strip()
            lower_raw = bounds_match.group(3).strip()
            upper_raw = bounds_match.group(4).strip()
            raw = lower_raw if sign == "+" else upper_raw
        else:
            match = TYPE_LINE_RE.match(stripped)
            if not match:
                continue
            key = match.group(1).strip()
            raw = match.group(2).strip()
        if not raw:
            continue
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
    if memory_decl is None and memory_type_raw:
        memory_expr = parse_type_expr(memory_type_raw)
        targets = sorted(find_record_pointer_targets(memory_expr))
        if not targets:
            targets = sorted(find_record_value_targets(memory_expr))
        if len(targets) == 1:
            memory_decl = targets[0]
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


def type_label(truth_type: dict) -> str:
    kind = truth_type["kind"]
    if kind == "primitive":
        return f"{truth_type.get('name', 'primitive')}:{truth_type.get('size_bits', 0)}"
    if kind == "pointer":
        return type_label(truth_type["to"]) + " *"
    if kind == "record_ref":
        return truth_type["name"]
    if kind == "array":
        count = truth_type.get("count")
        suffix = "" if count is None else str(count)
        return f"{type_label(truth_type['elem'])}[{suffix}]"
    return kind


class Comparator:
    def __init__(self, truth: dict, htypes: ParsedHTypes, config: dict):
        self.truth = truth
        self.htypes = htypes
        self.config = config

    def unwrap_recursive_decl(self, decl_name: str) -> str:
        # HType may print a recursive binder as rec_N { struct_M field_0; }.
        # For layout checks, compare against the body record at offset 0.
        seen: set[str] = set()
        while decl_name.startswith("rec_") and decl_name not in seen:
            seen.add(decl_name)
            decl = self.htypes.decls.get(decl_name)
            if decl is None or len(decl.fields) != 1:
                break
            field = decl.fields[0]
            if field.offset_bytes != 0:
                break
            expr = field.expr
            if expr["kind"] == "record_ref":
                decl_name = expr["name"]
                continue
            if expr["kind"] == "pointer_ref" and expr["to"]["kind"] == "record_ref":
                decl_name = expr["to"]["name"]
                continue
            break
        return decl_name

    def compare_root(self, root: dict) -> dict:
        binding_key = str(root.get("binding", root.get("truth", "")))
        if not binding_key:
            return {
                "binding": "",
                "passed": False,
                "mismatches": ["missing root binding"],
                "checked_fields": [],
            }

        binding = self.truth.get("bindings", {}).get(binding_key)
        if binding is None:
            return {
                "binding": binding_key,
                "passed": False,
                "mismatches": [f"missing truth binding: {binding_key}"],
                "checked_fields": [],
            }

        required_fields = self._required_fields(root)
        truth_type = binding["type"]
        location = binding.get("location")
        result = {
            "binding": binding_key,
            "truth_type": type_label(truth_type),
            "location": location,
            "checked_fields": [],
        }

        if binding["kind"] == "function_param":
            mismatches, recovered_decl, checked_fields = self.compare_function_param(
                binding_key,
                truth_type,
                required_fields=required_fields,
            )
            result.update(
                {
                    "mode": "function_param_layout",
                    "recovered_decl": recovered_decl,
                    "checked_fields": checked_fields,
                    "mismatches": mismatches,
                    "passed": not mismatches,
                }
            )
            return result

        if binding["kind"] != "global":
            result.update(
                {
                    "mode": "unsupported",
                    "mismatches": [f"unsupported truth binding kind: {binding['kind']}"],
                    "passed": False,
                }
            )
            return result

        if truth_type["kind"] == "pointer" and truth_type["to"]["kind"] == "record_ref":
            mismatches, recovered_decl, checked_fields = self.compare_global_pointer(
                binding_key,
                truth_type["to"],
                location=location,
                required_fields=required_fields,
            )
            result.update(
                {
                    "mode": "global_pointer_layout",
                    "recovered_decl": recovered_decl,
                    "checked_fields": checked_fields,
                    "mismatches": mismatches,
                    "passed": not mismatches,
                }
            )
            return result

        if truth_type["kind"] == "record_ref":
            mismatches, checked_fields = self.compare_record_at_address(
                binding_key,
                truth_type["name"],
                location=location,
                required_fields=required_fields,
            )
            result.update(
                {
                    "mode": "global_record_layout",
                    "checked_fields": checked_fields,
                    "mismatches": mismatches,
                    "passed": not mismatches,
                }
            )
            return result

        if truth_type["kind"] == "primitive":
            mismatches, checked_fields = self.compare_primitive_at_address(
                binding_key,
                truth_type,
                location=location,
            )
            result.update(
                {
                    "mode": "global_primitive",
                    "checked_fields": checked_fields,
                    "mismatches": mismatches,
                    "passed": not mismatches,
                }
            )
            return result

        result.update(
            {
                "mode": "unsupported",
                "mismatches": [f"{binding_key}: unsupported truth kind {truth_type['kind']}"],
                "passed": False,
            }
        )
        return result

    def _required_fields(self, root: dict) -> set[str] | None:
        raw = root.get("required_fields", root.get("field_allowlist"))
        if raw is None:
            return None
        return {str(item) for item in raw}

    def _location_base(self, location: dict | None) -> int | None:
        if location is None:
            return None
        if location["kind"] == "addr":
            return int(location["address"])
        if location["kind"] == "pieces":
            return int(location["base_address"])
        return None

    def compare_function_param(
        self,
        binding_key: str,
        truth_type: dict,
        *,
        required_fields: set[str] | None,
    ) -> tuple[list[str], str | None, list[dict]]:
        function_name, suffix = binding_key.rsplit("::arg", 1)
        param_index = int(suffix)
        function_raw = self.htypes.types_raw.get(f"@{function_name}")
        if function_raw is None:
            return [f"{binding_key}: missing recovered function type entry @{function_name}"], None, []

        signature = parse_function_pointer_type(function_raw)
        if signature is None:
            return [f"{binding_key}: unsupported recovered function type syntax: {function_raw}"], None, []

        if param_index >= len(signature["params"]):
            return [
                f"{binding_key}: recovered function @{function_name} has {len(signature['params'])} params"
            ], None, []

        param_expr = signature["params"][param_index]
        record_targets = sorted(find_record_pointer_targets(param_expr))
        if not record_targets:
            return [f"{binding_key}: expected recovered record pointer, got {param_expr['raw']}"], None, []
        if len(record_targets) != 1:
            return [f"{binding_key}: ambiguous recovered record pointer targets: {', '.join(record_targets)}"], None, []

        recovered_decl = record_targets[0]
        mismatches, checked_fields = self.compare_record_decl_layout(
            truth_type,
            recovered_decl_name=recovered_decl,
            path=binding_key,
            required_fields=required_fields,
        )
        return mismatches, recovered_decl, checked_fields

    def compare_global_pointer(
        self,
        binding_key: str,
        truth_record_ref: dict,
        *,
        location: dict | None,
        required_fields: set[str] | None,
    ) -> tuple[list[str], str | None, list[dict]]:
        address = self._location_base(location)
        if address is None:
            return [f"{binding_key}: missing supported global location"], None, []

        recovered_field = self.htypes.memory_fields_by_offset.get(address)
        if recovered_field is None:
            return [f"{binding_key}: missing recovered global slot at absolute offset {address}"], None, []

        record_targets = sorted(find_record_pointer_targets(recovered_field.expr))
        if not record_targets:
            record_targets = sorted(find_record_value_targets(recovered_field.expr))
        if not record_targets:
            return [f"{binding_key}: recovered slot has no record target: {recovered_field.expr_raw}"], None, []
        if len(record_targets) != 1:
            return [f"{binding_key}: ambiguous record pointer targets: {', '.join(record_targets)}"], None, []

        recovered_decl = record_targets[0]
        mismatches, checked_fields = self.compare_record_decl_layout(
            {"kind": "pointer", "to": truth_record_ref},
            recovered_decl_name=recovered_decl,
            path=binding_key,
            required_fields=required_fields,
        )
        return mismatches, recovered_decl, checked_fields

    def compare_record_at_address(
        self,
        binding_key: str,
        record_name: str,
        *,
        location: dict | None,
        required_fields: set[str] | None,
    ) -> tuple[list[str], list[dict]]:
        address = self._location_base(location)
        if address is None:
            return [f"{binding_key}: missing supported global location"], []
        return self.compare_record_memory_root(
            record_name=record_name,
            base_offset_bytes=address,
            path=binding_key,
            required_fields=required_fields,
        )

    def compare_primitive_at_address(
        self,
        binding_key: str,
        truth_type: dict,
        *,
        location: dict | None,
    ) -> tuple[list[str], list[dict]]:
        address = self._location_base(location)
        if address is None:
            return [f"{binding_key}: missing supported global location"], []

        recovered_field = self.htypes.memory_fields_by_offset.get(address)
        if recovered_field is None:
            return [f"{binding_key}: missing recovered field at absolute offset {address}"], []

        expected_width = truth_type.get("size_bits")
        matches = memory_field_matches_primitive(recovered_field, expected_width)
        checked = [
            {
                "name": binding_key,
                "status": "pass" if matches else "fail",
                "expected_offset_bytes": address,
                "recovered_expr": recovered_field.expr_raw,
            }
        ]
        if matches:
            return [], checked
        return [f"{binding_key}: expected primitive:{expected_width}, got {recovered_field.expr_raw}"], checked

    def compare_record_memory_root(
        self,
        *,
        record_name: str,
        base_offset_bytes: int,
        path: str,
        required_fields: set[str] | None,
    ) -> tuple[list[str], list[dict]]:
        record = self.truth["records"].get(record_name)
        if record is None:
            return [f"{path}: missing truth record definition {record_name}"], []

        mismatches: list[str] = []
        checked_fields: list[dict] = []
        for field in record["fields"]:
            if required_fields is not None and field["name"] not in required_fields:
                continue
            expected_offset = base_offset_bytes + field["offset_bits"] // 8
            recovered_field = self.htypes.memory_fields_by_offset.get(expected_offset)
            checked = {
                "name": field["name"],
                "expected_offset_bytes": expected_offset,
                "truth_type": type_label(field["type"]),
            }
            if recovered_field is None:
                checked["status"] = "missing"
                mismatches.append(
                    f"{path}.{field['name']}: missing recovered field at absolute offset {expected_offset}"
                )
                checked_fields.append(checked)
                continue

            checked["recovered_expr"] = recovered_field.expr_raw
            field_mismatches = self.compare_memory_field_type(
                field["type"],
                recovered_field,
                path=f"{path}.{field['name']}",
            )
            checked["status"] = "pass" if not field_mismatches else "fail"
            checked_fields.append(checked)
            mismatches.extend(field_mismatches)
        return mismatches, checked_fields

    def compare_record_decl_layout(
        self,
        truth_type: dict,
        *,
        recovered_decl_name: str,
        path: str,
        required_fields: set[str] | None,
    ) -> tuple[list[str], list[dict]]:
        if truth_type["kind"] == "pointer":
            truth_record_ref = truth_type["to"]
        else:
            truth_record_ref = truth_type

        if truth_record_ref["kind"] != "record_ref":
            return [f"{path}: expected truth record, got {truth_record_ref['kind']}"], []

        truth_record = self.truth["records"].get(truth_record_ref["name"])
        if truth_record is None:
            return [f"{path}: missing truth record definition {truth_record_ref['name']}"], []

        recovered_decl_name = self.unwrap_recursive_decl(recovered_decl_name)
        recovered_decl = self.htypes.decls.get(recovered_decl_name)
        if recovered_decl is None:
            return [f"{path}: missing recovered decl {recovered_decl_name}"], []

        recovered_fields_by_offset = {
            field.offset_bytes: field for field in recovered_decl.fields if field.offset_bytes is not None
        }

        mismatches: list[str] = []
        checked_fields: list[dict] = []
        for field in truth_record["fields"]:
            if required_fields is not None and field["name"] not in required_fields:
                continue
            expected_offset = field["offset_bits"] // 8
            recovered_field = recovered_fields_by_offset.get(expected_offset)
            checked = {
                "name": field["name"],
                "expected_offset_bytes": expected_offset,
                "truth_type": type_label(field["type"]),
            }
            if recovered_field is None:
                checked["status"] = "missing"
                mismatches.append(
                    f"{path}.{field['name']}: missing recovered field in {recovered_decl_name} at offset {expected_offset}"
                )
                checked_fields.append(checked)
                continue
            checked["status"] = "pass"
            checked["recovered_expr"] = recovered_field.expr_raw
            checked_fields.append(checked)
        return mismatches, checked_fields

    def compare_memory_field_type(self, truth_type: dict, recovered_field: FieldDecl, *, path: str) -> list[str]:
        if truth_type["kind"] == "primitive":
            expected_width = truth_type.get("size_bits")
            if memory_field_matches_primitive(recovered_field, expected_width):
                return []
            return [f"{path}: expected primitive:{expected_width}, got {recovered_field.expr_raw}"]

        if truth_type["kind"] == "pointer":
            if find_record_pointer_targets(recovered_field.expr):
                return []
            if truth_type["to"]["kind"] == "primitive" and recovered_field.expr["kind"] == "pointer":
                return []
            return [f"{path}: expected pointer-shaped field, got {recovered_field.expr_raw}"]

        if truth_type["kind"] == "record_ref":
            return []

        return [f"{path}: unsupported truth kind {truth_type['kind']}"]


def load_json(path: Path) -> dict:
    return json.loads(path.read_text())


def write_report_text(path: Path, report: dict) -> None:
    lines = ["PASS" if report["passed"] else "FAIL"]
    for root in report["roots"]:
        status = "PASS" if root["passed"] else "FAIL"
        lines.append(f"{status} {root['binding']} [{root.get('mode', 'unknown')}]")
        lines.append(f"  truth: {root.get('truth_type', 'unknown')}")
        if root.get("location") is not None:
            lines.append(f"  location: {json.dumps(root['location'], sort_keys=True)}")
        if root.get("recovered_decl") is not None:
            lines.append(f"  recovered_decl: {root['recovered_decl']}")
        for field in root.get("checked_fields", []):
            suffix = f" -> {field.get('recovered_expr')}" if field.get("recovered_expr") else ""
            lines.append(
                f"  {field['status'].upper():7s} {field['name']} @ {field.get('expected_offset_bytes', '?')}{suffix}"
            )
        for mismatch in root.get("mismatches", []):
            lines.append(f"  mismatch: {mismatch}")
    path.write_text("\n".join(lines) + "\n")


def write_report_markdown(path: Path, report: dict) -> None:
    lines = ["# PASS" if report["passed"] else "# FAIL", ""]
    for root in report["roots"]:
        status = "PASS" if root["passed"] else "FAIL"
        lines.append(f"## {status} `{root['binding']}`")
        lines.append("")
        lines.append(f"- Mode: `{root.get('mode', 'unknown')}`")
        lines.append(f"- Truth: `{root.get('truth_type', 'unknown')}`")
        if root.get("location") is not None:
            lines.append(f"- Location: `{json.dumps(root['location'], sort_keys=True)}`")
        if root.get("recovered_decl") is not None:
            lines.append(f"- Recovered decl: `{root['recovered_decl']}`")
        lines.append("")
        lines.append("| Status | Field | Expected Offset | Recovered |")
        lines.append("| --- | --- | ---: | --- |")
        for field in root.get("checked_fields", []):
            recovered = field.get("recovered_expr", "")
            lines.append(
                f"| {field['status']} | `{field['name']}` | {field.get('expected_offset_bytes', '')} | `{recovered}` |"
            )
        if root.get("mismatches"):
            lines.append("")
            lines.append("Mismatches:")
            for mismatch in root["mismatches"]:
                lines.append(f"- {mismatch}")
        lines.append("")
    path.write_text("\n".join(lines).rstrip() + "\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--htypes", required=True)
    parser.add_argument("--truth", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--report-json", required=True)
    parser.add_argument("--report-text", required=True)
    parser.add_argument("--report-markdown", required=True)
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
    report_markdown_path = Path(args.report_markdown).resolve()
    report_json_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    write_report_text(report_text_path, report)
    write_report_markdown(report_markdown_path, report)
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
