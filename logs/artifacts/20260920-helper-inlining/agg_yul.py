#!/usr/bin/env python3
"""Phase 0 measurement: Yul-function recognition + calldata summary coverage."""
import json, os, re, subprocess, time
ROOT = "/sn640/NotDec"
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")
MANI_DIR = os.path.join(ROOT, "test/evm/solidity-patterns")
MANI = json.load(open(os.path.join(MANI_DIR, "manifest.json")))
WORK = "/tmp/agg_yul"; os.makedirs(WORK, exist_ok=True)

def count_helper_loads(path):
    """evm_calldataload calls inside private_* functions of the input module."""
    text = open(path, errors="replace").read()
    n = 0; cur = None
    for line in text.splitlines():
        m = re.match(r'define\s+.*@([A-Za-z0-9_.$]+)\(', line)
        if m: cur = m.group(1); continue
        if cur and line.startswith('}'): cur = None; continue
        if cur and cur.startswith('private_') and '@evm_calldataload' in line:
            n += 1
    return n

stats = dict(cases=0, failed=0, summaries=0, with_loads=0, with_effects=0,
             effects=0, unresolved=0, label_found=0, label_none=0,
             labelconst=0, callsites=0, helper_loads_input=0,
             calldata_effects_relative=0, calldata_effects_const=0)
t0 = time.time()
for case in MANI["cases"]:
    inp = os.path.join(MANI_DIR, case["input"])
    stats["cases"] += 1
    stats["helper_loads_input"] += count_helper_loads(inp)
    out = os.path.join(WORK, case["name"] + ".ll")
    try:
        p = subprocess.run([BIN, inp, "-o", out] + list(MANI["default_args"]),
                           cwd=ROOT, capture_output=True, timeout=300)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        rc = 124
    if rc != 0 or not os.path.exists(out):
        stats["failed"] += 1; continue
    text = open(out, errors="replace").read()
    ids = dict(re.findall(r'^!([0-9]+) = !\{!"([^"]*)"\}', text, re.M))
    for m in re.finditer(r"^define[^\n]*?@([A-Za-z0-9_.$]+)\([^\n]*?\{", text, re.M):
        h = text[m.start():m.end()]
        mm = re.search(r"!notdec\.evm\.yul_function_summary !([0-9]+)", h)
        if not mm: continue
        s = ids.get(mm.group(1), "")
        stats["summaries"] += 1
        kv = dict(re.findall(r'(\w+)=([^;]*)', s))
        loads = int(kv.get("loads", 0)); unres = int(kv.get("unresolved", 0))
        effects = loads - unres
        if loads > 0: stats["with_loads"] += 1
        if effects > 0:
            stats["with_effects"] += 1
            stats["effects"] += effects
        stats["unresolved"] += unres
        if kv.get("label", "-1") != "-1": stats["label_found"] += 1
        else: stats["label_none"] += 1
        lc = kv.get("labelconst", "0/0").split("/")
        stats["labelconst"] += int(lc[0]); stats["callsites"] += int(lc[1])
        for eff in kv.get("calldata", "").split(","):
            if not eff: continue
            if eff.startswith("p"): stats["calldata_effects_relative"] += 1
            else: stats["calldata_effects_const"] += 1
stats["wall_s"] = round(time.time() - t0, 1)
print(json.dumps(stats, indent=1))
