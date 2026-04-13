#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


METADATA_RE = re.compile(r"^!(\d+)\s*=\s*(.+)$")


def split_top_level(text: str, delimiter: str) -> list[str]:
    parts: list[str] = []
    depth_paren = 0
    depth_angle = 0
    depth_brace = 0
    in_string = False
    current: list[str] = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == '"' and (i == 0 or text[i - 1] != "\\"):
            in_string = not in_string
            current.append(ch)
            i += 1
            continue
        if not in_string:
            if ch == "(":
                depth_paren += 1
            elif ch == ")":
                depth_paren -= 1
            elif ch == "<":
                depth_angle += 1
            elif ch == ">":
                depth_angle -= 1
            elif ch == "{":
                depth_brace += 1
            elif ch == "}":
                depth_brace -= 1
            elif ch == delimiter and depth_paren == 0 and depth_angle == 0 and depth_brace == 0:
                parts.append("".join(current).strip())
                current = []
                i += 1
                continue
        current.append(ch)
        i += 1
    tail = "".join(current).strip()
    if tail:
        parts.append(tail)
    return parts


def parse_metadata_value(value: str):
    value = value.strip()
    if value.startswith('"') and value.endswith('"'):
        return value[1:-1]
    if value.startswith("!"):
        return value
    if value in {"true", "false"}:
        return value == "true"
    if re.fullmatch(r"-?\d+", value):
        return int(value)
    return value


class MetadataStore:
    def __init__(self, text: str):
        self.defs: dict[str, str] = {}
        for line in text.splitlines():
            match = METADATA_RE.match(line.strip())
            if match:
                self.defs[f"!{match.group(1)}"] = match.group(2).strip()
        self.node_cache: dict[str, dict] = {}

    def get_node(self, ref: str | None) -> dict | None:
        if ref is None or ref not in self.defs:
            return None
        if ref in self.node_cache:
            return self.node_cache[ref]

        raw = self.defs[ref]
        stripped = raw
        if stripped.startswith("distinct "):
            stripped = stripped[len("distinct ") :]

        if stripped.startswith("!{") and stripped.endswith("}"):
            entries = [part for part in split_top_level(stripped[2:-1], ",") if part]
            node = {"kind": "list", "entries": [parse_metadata_value(part) for part in entries]}
            self.node_cache[ref] = node
            return node

        match = re.match(r"!(\w+)\((.*)\)$", stripped)
        if not match:
            node = {"kind": "raw", "raw": stripped}
            self.node_cache[ref] = node
            return node

        kind = match.group(1)
        fields: dict[str, object] = {}
        positional: list[object] = []
        for part in split_top_level(match.group(2), ","):
            key_value = split_top_level(part, ":")
            if len(key_value) >= 2 and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key_value[0]):
                key = key_value[0]
                value = ":".join(key_value[1:])
                fields[key] = parse_metadata_value(value)
            else:
                positional.append(parse_metadata_value(part))
        node = {"kind": kind, "fields": fields, "positional": positional}
        self.node_cache[ref] = node
        return node


def record_name_for_ref(store: MetadataStore, ref: str) -> str:
    node = store.get_node(ref)
    if node is None:
        return f"anon.{ref[1:]}"
    fields = node["fields"]
    name = fields.get("name")
    if isinstance(name, str) and name:
        return f"struct.{name}"
    return f"anon.{ref[1:]}"


def resolve_type(store: MetadataStore, ref: str | None) -> dict:
    if ref is None:
        return {"kind": "unknown"}
    node = store.get_node(ref)
    if node is None:
        return {"kind": "unknown", "ref": ref}

    kind = node["kind"]
    fields = node.get("fields", {})

    if kind == "DIBasicType":
        return {
            "kind": "primitive",
            "name": fields.get("name", "primitive"),
            "size_bits": int(fields.get("size", 0) or 0),
            "encoding": fields.get("encoding"),
        }

    if kind == "DIDerivedType":
        tag = fields.get("tag")
        base_type = fields.get("baseType")
        if tag == "DW_TAG_pointer_type":
            return {
                "kind": "pointer",
                "size_bits": int(fields.get("size", 0) or 0),
                "to": resolve_type(store, base_type if isinstance(base_type, str) else None),
            }
        if tag in {
            "DW_TAG_const_type",
            "DW_TAG_volatile_type",
            "DW_TAG_restrict_type",
            "DW_TAG_typedef",
        }:
            return resolve_type(store, base_type if isinstance(base_type, str) else None)
        return {
            "kind": "derived",
            "tag": tag,
            "base": resolve_type(store, base_type if isinstance(base_type, str) else None),
        }

    if kind == "DICompositeType":
        tag = fields.get("tag")
        if tag == "DW_TAG_structure_type":
            return {
                "kind": "record_ref",
                "name": record_name_for_ref(store, ref),
            }
        return {
            "kind": "composite",
            "tag": tag,
        }

    return {
        "kind": "unknown",
        "ref": ref,
        "node_kind": kind,
    }


def extract_records(store: MetadataStore) -> dict:
    records: dict[str, dict] = {}
    for ref, raw in store.defs.items():
        if "DICompositeType" not in raw:
            continue
        node = store.get_node(ref)
        if node is None:
            continue
        fields = node["fields"]
        if fields.get("tag") != "DW_TAG_structure_type":
            continue

        record_name = record_name_for_ref(store, ref)
        members_ref = fields.get("elements")
        member_nodes = []
        if isinstance(members_ref, str):
            list_node = store.get_node(members_ref)
            if list_node and list_node["kind"] == "list":
                member_nodes = list_node["entries"]

        members = []
        for member_ref in member_nodes:
            if not isinstance(member_ref, str):
                continue
            member_node = store.get_node(member_ref)
            if member_node is None or member_node["kind"] != "DIDerivedType":
                continue
            member_fields = member_node["fields"]
            if member_fields.get("tag") != "DW_TAG_member":
                continue
            base_type = member_fields.get("baseType")
            members.append(
                {
                    "name": member_fields.get("name", ""),
                    "offset_bits": int(member_fields.get("offset", 0) or 0),
                    "size_bits": int(member_fields.get("size", 0) or 0),
                    "type": resolve_type(store, base_type if isinstance(base_type, str) else None),
                }
            )

        records[record_name] = {
            "name": record_name,
            "size_bits": int(fields.get("size", 0) or 0),
            "fields": members,
        }
    return records


def extract_bindings(store: MetadataStore) -> dict:
    bindings: dict[str, dict] = {}

    for ref, raw in store.defs.items():
        if "DIGlobalVariableExpression" not in raw:
            continue
        node = store.get_node(ref)
        if node is None:
            continue
        var_ref = node["fields"].get("var")
        if not isinstance(var_ref, str):
            continue
        var_node = store.get_node(var_ref)
        if var_node is None or var_node["kind"] != "DIGlobalVariable":
            continue
        var_fields = var_node["fields"]
        type_ref = var_fields.get("type")
        if not isinstance(type_ref, str):
            continue
        bindings[f"@{var_fields['name']}"] = resolve_type(store, type_ref)

    for ref, raw in store.defs.items():
        if "DILocalVariable" not in raw:
            continue
        node = store.get_node(ref)
        if node is None:
            continue
        fields = node["fields"]
        scope_ref = fields.get("scope")
        type_ref = fields.get("type")
        if not isinstance(scope_ref, str) or not isinstance(type_ref, str):
            continue
        scope_node = store.get_node(scope_ref)
        if scope_node is None or scope_node["kind"] != "DISubprogram":
            continue
        function_name = scope_node["fields"].get("name")
        variable_name = fields.get("name")
        if not isinstance(function_name, str) or not isinstance(variable_name, str):
            continue

        resolved = resolve_type(store, type_ref)
        arg_index = fields.get("arg")
        if isinstance(arg_index, int) and arg_index > 0:
            bindings[f"{function_name}::arg{arg_index - 1}"] = resolved
        bindings[f"{function_name}::{variable_name}"] = resolved

    return bindings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    output_path = Path(args.output).resolve()
    text = input_path.read_text()
    store = MetadataStore(text)

    result = {
        "source": str(input_path),
        "records": extract_records(store),
        "bindings": extract_bindings(store),
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
