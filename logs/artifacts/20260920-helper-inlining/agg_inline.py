#!/usr/bin/env python3
"""Pre-TR inline experiment: inline helpers with LLVM's inliner, then measure."""
import json, os, re, subprocess, time
ROOT = "/sn640/NotDec"
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")
OPT = "/sn640/NotDec/llvm-22.1.0.obj/bin/opt"
MANI_DIR = os.path.join(ROOT, "test/evm/solidity-patterns")
MANI = json.load(open(os.path.join(MANI_DIR, "manifest.json")))
INL = "/tmp/agg_inline_cases"; OUT = "/tmp/agg_inline_out"
os.makedirs(INL, exist_ok=True); os.makedirs(OUT, exist_ok=True)
HOST = 'target triple = "x86_64-unknown-linux-gnu"'
EVM = 'target triple = "evm-unknown-unknown"'

def patch(text, frm, to):
    return text.replace(frm, to)

def val(ids, h, kind):
    m = re.search(r"!notdec\.solidity\.abi_param\." + kind + r" !([0-9]+)", h)
    return ids.get(m.group(1)) if m else None

stats = dict(cases=0, failed=0, entries=0, exact=0, record_gt=0, record_lt=0,
             params_gt0=0, fully_typed=0, partially_typed=0, zero_typed=0,
             total_params=0, total_typed=0, missing_words=0,
             extra_entries=0, extra_words=0, params_eq_annotated=0,
             lines_before=0, lines_after=0, opt_failed=0)
t0 = time.time()
for case in MANI["cases"]:
    inp = os.path.join(MANI_DIR, case["input"])
    name = case["name"]
    stats["cases"] += 1
    text = open(inp, errors="replace").read()
    stats["lines_before"] += text.count("\n")
    open("/tmp/_host.ll", "w").write(patch(text, EVM, HOST))
    r = subprocess.run([OPT, "-passes=inline", "-S", "/tmp/_host.ll", "-o", "/tmp/_inl_host.ll"],
                       capture_output=True, timeout=300)
    if r.returncode != 0:
        stats["opt_failed"] += 1; stats["failed"] += 1; continue
    inl = patch(open("/tmp/_inl_host.ll", errors="replace").read(), HOST, EVM)
    ipath = os.path.join(INL, name + ".ll")
    open(ipath, "w").write(inl)
    stats["lines_after"] += inl.count("\n")
    out = os.path.join(OUT, name + ".ll")
    args = list(MANI["default_args"]) + list(case.get("args", []))
    try:
        p = subprocess.run([BIN, ipath, "-o", out] + args, cwd=ROOT, capture_output=True, timeout=300)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        rc = 124
    if rc != 0 or not os.path.exists(out):
        stats["failed"] += 1; continue
    text2 = open(out, errors="replace").read()
    ids = dict(re.findall(r'^!([0-9]+) = !\{!"([^"]*)"\}', text2, re.M))
    for m in re.finditer(r"^define[^\n]*?@([A-Za-z0-9_.$]+)\([^\n]*?\{", text2, re.M):
        h = text2[m.start():m.end()]
        if not re.search(r"!notdec\.solidity\.abi_param\.params", h):
            continue
        stats["entries"] += 1
        params = int(val(ids, h, "params") or 0)
        typed = int(val(ids, h, "typed") or 0)
        record = int(val(ids, h, "record") or 0)
        annotated = int(val(ids, h, "annotated") or 0)
        missing = val(ids, h, "missing"); extra = val(ids, h, "extra")
        if record == annotated: stats["exact"] += 1
        elif record > annotated: stats["record_gt"] += 1
        else: stats["record_lt"] += 1
        if params == annotated: stats["params_eq_annotated"] += 1
        if params > 0:
            stats["params_gt0"] += 1
            stats["total_params"] += params; stats["total_typed"] += typed
            if typed == params: stats["fully_typed"] += 1
            elif typed > 0: stats["partially_typed"] += 1
            else: stats["zero_typed"] += 1
        if missing: stats["missing_words"] += len(missing.split(","))
        if extra:
            stats["extra_entries"] += 1; stats["extra_words"] += len(extra.split(","))
stats["wall_s"] = round(time.time() - t0, 1)
print(json.dumps(stats, indent=1))
