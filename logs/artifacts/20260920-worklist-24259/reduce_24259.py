#!/usr/bin/env python3
"""Greedy IR reducer for the 24259 entry-polymorphism worklist blowup."""
import json
import os
import re
import subprocess
import sys
import time

ROOT = "/sn640/NotDec"
SRC = os.path.join(ROOT, "test/evm/solidity-patterns/cases/24259_19755445_aefeec2314_4f43187f4106.ll")
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")
WORK = "/tmp/reduce24259"
BUDGET_ADDS = os.environ.get("BUDGET_ADDS", "500000")
RUN_TIMEOUT = float(os.environ.get("RUN_TIMEOUT", "25"))

ENV = dict(os.environ)
ENV["NOTDEC_EVM_POLY_ENTRIES"] = "1"
ENV["NOTDEC_WORKLIST_ABORT_ADDS"] = BUDGET_ADDS
ENV.pop("NOTDEC_WORKLIST_SNAPSHOT", None)
ENV.pop("NOTDEC_DEFERRED_DIAG", None)


def parse(text):
    lines = text.splitlines(keepends=True)
    blocks = []
    i = 0
    start = None
    depth = 0
    header = ""
    while i < len(lines):
        line = lines[i]
        if start is None and line.startswith("define "):
            start = i
            header = ""
            depth = 0
        if start is not None:
            header += line
            depth += line.count("{") - line.count("}")
            if depth == 0 and "{" in header:
                m = re.search(r"@([A-Za-z0-9_.$]+)\s*\(", header)
                blocks.append((m.group(1), "".join(lines[start:i + 1]), start, i + 1))
                start = None
        i += 1
    used = set()
    for _, _, a, b in blocks:
        used.update(range(a, b))
    rest = "".join(l for idx, l in enumerate(lines) if idx not in used)
    return rest, blocks


LINKAGE_RE = re.compile(
    r"^declare (?:internal|private|available_externally|linkonce(?:_odr)?|"
    r"weak(?:_odr)?|common|appending|extern_weak|dllexport|dllimport)\s+")


def declare_version(header):
    text = header.replace("define ", "declare ", 1)
    # Linkage keywords are illegal on declarations.
    text = LINKAGE_RE.sub("declare ", text)
    idx = text.index("{")
    return text[:idx].rstrip() + "\n"


def run_case(path):
    cmd = [BIN, path, "-o", os.path.join(WORK, "out.sol"), "--tr-level=2"]
    try:
        p = subprocess.run(cmd, env=ENV, cwd=ROOT, capture_output=True,
                           timeout=RUN_TIMEOUT)
        return p.returncode
    except subprocess.TimeoutExpired:
        return 124


def main():
    os.makedirs(WORK, exist_ok=True)
    text = open(SRC).read()
    preamble, blocks = parse(text)
    names = [b[0] for b in blocks]
    full = {n: blk for n, blk, _, _ in blocks}
    print("functions (%d):" % len(names), names, flush=True)
    keep_always = {"public___function_selector___0x0"}
    order = [n for n in names if n not in keep_always]
    order.sort(key=lambda n: (not n.startswith("private__"), n))

    def build(keep, path):
        parts = [preamble]
        for n in names:
            parts.append(full[n] if n in keep else declare_version(full[n]))
        open(path, "w").write("".join(parts))

    cand = os.path.join(WORK, "cand.ll")
    build(set(names), cand)
    t0 = time.time()
    rc = run_case(cand)
    print("baseline rc=%d (%.1fs)" % (rc, time.time() - t0), flush=True)
    results = {"baseline": rc, "removed": []}
    if rc != 42:
        print("baseline does not trigger with this budget; abort", flush=True)
        json.dump(results, open(os.path.join(WORK, "result.json"), "w"), indent=1)
        return

    current = set(names)
    changed = True
    while changed:
        changed = False
        for n in order:
            if n not in current:
                continue
            trial = set(current) - {n}
            build(trial, cand)
            t0 = time.time()
            rc = run_case(cand)
            dt = time.time() - t0
            if rc == 42:
                current = trial
                changed = True
                results["removed"].append(n)
                print("REMOVED %-40s still blows up rc=42 (%.1fs) remaining=%d"
                      % (n, dt, len(current)), flush=True)
            else:
                print("keep   %-40s rc=%-3d (%.1fs)" % (n, rc, dt), flush=True)
    build(current, os.path.join(WORK, "reduced.ll"))
    results["remaining"] = sorted(current)
    json.dump(results, open(os.path.join(WORK, "result.json"), "w"), indent=1)
    print("done. removed:", results["removed"], flush=True)
    print("remaining:", sorted(current), flush=True)


if __name__ == "__main__":
    main()
