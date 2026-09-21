#!/usr/bin/env python3
"""Targeted pre-TR inlining: only the calldata-reading helper chains."""
import json, os, re, subprocess, time
ROOT = "/sn640/NotDec"
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")
OPT = "/sn640/NotDec/llvm-22.1.0.obj/bin/opt"
MANI_DIR = os.path.join(ROOT, "test/evm/solidity-patterns")
MANI = json.load(open(os.path.join(MANI_DIR, "manifest.json")))
INL = "/tmp/agg_targeted_cases"; OUT = "/tmp/agg_targeted_out"
os.makedirs(INL, exist_ok=True); os.makedirs(OUT, exist_ok=True)
HOST = 'target triple = "x86_64-unknown-linux-gnu"'
EVM = 'target triple = "evm-unknown-unknown"'

def parse_funcs(text):
    """Return {name: body_text} for defined functions."""
    out = {}; cur = None; buf = []
    for line in text.splitlines(keepends=True):
        m = re.match(r'define\s+.*@([A-Za-z0-9_.$]+)\(', line)
        if m:
            cur = m.group(1); buf = [line]; continue
        if cur is not None:
            buf.append(line)
            if line.startswith('}'):
                out[cur] = ''.join(buf); cur = None
    return out

def select(text):
    funcs = parse_funcs(text)
    readers = set()
    for n, b in funcs.items():
        if not n.startswith('private_'):
            continue
        if '@evm_calldataload' in b or '@evm_calldatacopy' in b:
            readers.add(n)
    # transitive internal callees of readers
    selected = set(readers)
    changed = True
    while changed:
        changed = False
        for n in list(selected):
            for c in re.findall(r'@(private_[A-Za-z0-9_.$]+)\(', funcs.get(n, '')):
                if c in funcs and c not in selected:
                    selected.add(c); changed = True
    # rewrite define headers
    out = []
    for line in text.splitlines(keepends=True):
        m = re.match(r'(define\s+.*@([A-Za-z0-9_.$]+)\([^\n]*?)(\s*\{.*)$', line)
        if m:
            name = m.group(2)
            attr = ' alwaysinline' if name in selected else ' noinline'
            line = m.group(1) + attr + m.group(3)
        out.append(line)
    return ''.join(out), selected

stats = dict(cases=0, failed=0, entries=0, exact=0, record_gt=0, record_lt=0,
             params_gt0=0, fully_typed=0, partially_typed=0, zero_typed=0,
             total_params=0, total_typed=0, missing_words=0,
             extra_entries=0, extra_words=0, selected_total=0,
             lines_before=0, lines_after=0)
t0 = time.time()
for case in MANI["cases"]:
    inp = os.path.join(MANI_DIR, case["input"]); name = case["name"]
    stats["cases"] += 1
    text = open(inp, errors="replace").read()
    stats["lines_before"] += text.count("\n")
    marked, selected = select(text)
    stats["selected_total"] += len(selected)
    open("/tmp/_t_host.ll", "w").write(marked.replace(EVM, HOST))
    r = subprocess.run([OPT, "-passes=inline", "-S", "/tmp/_t_host.ll", "-o", "/tmp/_t_inl.ll"],
                       capture_output=True, timeout=300)
    if r.returncode != 0:
        stats["failed"] += 1; continue
    inl = open("/tmp/_t_inl.ll", errors="replace").read().replace(HOST, EVM)
    ipath = os.path.join(INL, name + ".ll"); open(ipath, "w").write(inl)
    stats["lines_after"] += inl.count("\n")
    out = os.path.join(OUT, name + ".ll")
    try:
        p = subprocess.run([BIN, ipath, "-o", out] + list(MANI["default_args"]),
                           cwd=ROOT, capture_output=True, timeout=300)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        rc = 124
    if rc != 0 or not os.path.exists(out):
        stats["failed"] += 1; continue
    t2 = open(out, errors="replace").read()
    ids = dict(re.findall(r'^!([0-9]+) = !\{!"([^"]*)"\}', t2, re.M))
    def val(h, kind):
        m = re.search(r"!notdec\.solidity\.abi_param\." + kind + r" !([0-9]+)", h)
        return ids.get(m.group(1)) if m else None
    for m in re.finditer(r"^define[^\n]*?@([A-Za-z0-9_.$]+)\([^\n]*?\{", t2, re.M):
        h = t2[m.start():m.end()]
        if not re.search(r"!notdec\.solidity\.abi_param\.params", h): continue
        stats["entries"] += 1
        params = int(val(h, "params") or 0); typed = int(val(h, "typed") or 0)
        record = int(val(h, "record") or 0); annotated = int(val(h, "annotated") or 0)
        if record == annotated: stats["exact"] += 1
        elif record > annotated: stats["record_gt"] += 1
        else: stats["record_lt"] += 1
        if params > 0:
            stats["params_gt0"] += 1
            stats["total_params"] += params; stats["total_typed"] += typed
            if typed == params: stats["fully_typed"] += 1
            elif typed > 0: stats["partially_typed"] += 1
            else: stats["zero_typed"] += 1
        mm = val(h, "missing")
        if mm: stats["missing_words"] += len(mm.split(","))
        ee = val(h, "extra")
        if ee: stats["extra_entries"] += 1; stats["extra_words"] += len(ee.split(","))
stats["wall_s"] = round(time.time() - t0, 1)
print(json.dumps(stats, indent=1))
