#!/usr/bin/env python3
"""Aggregate the per-function parameter table (params/typed/missing/extra)."""
import json, os, re, subprocess, time
ROOT = "/sn640/NotDec"
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")
MANI_DIR = os.path.join(ROOT, "test/evm/solidity-patterns")
MANI = json.load(open(os.path.join(MANI_DIR, "manifest.json")))
WORK = "/tmp/agg_params"
os.makedirs(WORK, exist_ok=True)
env = dict(os.environ)
env.pop("NOTDEC_WORKLIST_ABORT_ADDS", None)

def val(ids, h, kind):
    m = re.search(r"!notdec\.solidity\.abi_param\." + kind + r" !([0-9]+)", h)
    return ids.get(m.group(1)) if m else None

stats = dict(cases=0, failed=0, entries=0,
             params_gt0=0, fully_typed=0, partially_typed=0, zero_typed=0,
             total_params=0, total_typed=0,
             record_extra_entries=0, extra_words=0, missing_words=0,
             params_eq_annotated=0, params_lt_annotated=0, params_gt_annotated=0)
t0 = time.time()
for case in MANI["cases"]:
    inp = os.path.join(MANI_DIR, case["input"])
    out = os.path.join(WORK, case["name"] + ".ll")
    args = list(MANI["default_args"]) + list(case.get("args", []))
    stats["cases"] += 1
    try:
        p = subprocess.run([BIN, inp, "-o", out] + args, env=env, cwd=ROOT,
                           capture_output=True, timeout=180)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        rc = 124
    if rc != 0 or not os.path.exists(out):
        stats["failed"] += 1
        continue
    text = open(out, errors="replace").read()
    ids = dict(re.findall(r'^!([0-9]+) = !\{!"([^"]*)"\}', text, re.M))
    for m in re.finditer(r"^define[^\n]*?@([A-Za-z0-9_.$]+)\([^\n]*?\{", text, re.M):
        h = text[m.start():m.end()]
        if not re.search(r"!notdec\.solidity\.abi_param\.params", h):
            continue
        stats["entries"] += 1
        params = int(val(ids, h, "params") or 0)
        typed = int(val(ids, h, "typed") or 0)
        annotated = int(val(ids, h, "annotated") or 0)
        missing = val(ids, h, "missing")
        extra = val(ids, h, "extra")
        if params > 0:
            stats["params_gt0"] += 1
            stats["total_params"] += params
            stats["total_typed"] += typed
            if typed == params: stats["fully_typed"] += 1
            elif typed > 0: stats["partially_typed"] += 1
            else: stats["zero_typed"] += 1
        if missing: stats["missing_words"] += len(missing.split(","))
        if extra:
            stats["record_extra_entries"] += 1
            stats["extra_words"] += len(extra.split(","))
        if params == annotated: stats["params_eq_annotated"] += 1
        elif params < annotated: stats["params_lt_annotated"] += 1
        else: stats["params_gt_annotated"] += 1
stats["wall_s"] = round(time.time() - t0, 1)
print(json.dumps(stats, indent=1))
