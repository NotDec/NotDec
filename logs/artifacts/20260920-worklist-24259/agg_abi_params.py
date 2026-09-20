#!/usr/bin/env python3
"""Aggregate abi_param.record vs .annotated over the EVM pattern corpus."""
import json, os, re, subprocess, sys, time

ROOT = "/sn640/NotDec"
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")
MANI_DIR = os.path.join(ROOT, "test/evm/solidity-patterns")
MANI = json.load(open(os.path.join(MANI_DIR, "manifest.json")))
WORK = "/tmp/agg_abi"
os.makedirs(WORK, exist_ok=True)

# variants: default (production: every internal calldata helper marked),
#           strict  (NOTDEC_EVM_POLY_STRICT=1 positive matcher),
#           entries (NOTDEC_EVM_POLY_ENTRIES=1 experimental entry variant).
# The "no polymorphism at all" numbers in the log were measured with a temporary
# NOTDEC_DISABLE_POLY switch that has since been removed; to reproduce them,
# make shouldMarkPolymorphicFunction() return false and skip
# markBuiltinPolymorphicBufferFunctions().
variant = sys.argv[1] if len(sys.argv) > 1 else "default"
env = dict(os.environ)
if variant == "strict":
    env["NOTDEC_EVM_POLY_STRICT"] = "1"
elif variant == "entries":
    env["NOTDEC_EVM_POLY_ENTRIES"] = "1"
env.pop("NOTDEC_WORKLIST_ABORT_ADDS", None)

IDS = re.compile(r'^!(\d+) = !\{!"([^"]*)"\}', re.M)


def parse(path):
    text = open(path, errors="replace").read()
    ids = dict(IDS.findall(text))
    entries = []
    for m in re.finditer(r"^define[^\n]*?@([A-Za-z0-9_.$]+)\([^\n]*?\{", text, re.M):
        fn = m.group(1)
        seg = text[m.end(): text.find("\n}", m.end())]
        # metadata attaches to the define header (before '{'), search the header line
        header = text[m.start():m.end()]
        rec = re.search(r"!notdec\.solidity\.abi_param\.record !(\d+)", header)
        ann = re.search(r"!notdec\.solidity\.abi_param\.annotated !(\d+)", header)
        if not rec and not ann:
            continue
        def val(mm):
            return int(ids.get(mm.group(1), "-1")) if mm else None
        entries.append((fn, val(rec), val(ann)))
    return entries


stats = dict(cases=0, failed=0, entries=0, exact=0, too_many=0, too_few=0,
             record_only=0, single_value_cases=0, cases_with_entries=0)
t0 = time.time()
for case in MANI["cases"]:
    inp = os.path.join(MANI_DIR, case["input"])
    out = os.path.join(WORK, case["name"] + ".ll")
    args = list(MANI["default_args"]) + list(case.get("args", []))
    args = [a.replace("{manifest_dir}", MANI_DIR).replace("{project_root}", ROOT)
             .replace("{workdir}", WORK) for a in args]
    stats["cases"] += 1
    try:
        p = subprocess.run([BIN, inp, "-o", out] + args, env=env, cwd=ROOT,
                           capture_output=True, timeout=180)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        rc = 124
    if rc != 0 or not os.path.exists(out):
        stats["failed"] += 1
        print("FAIL %s rc=%s" % (case["name"], rc), flush=True)
        continue
    entries = parse(out)
    if not entries:
        continue
    stats["cases_with_entries"] += 1
    recs = []
    for fn, r, a in entries:
        stats["entries"] += 1
        recs.append(r)
        if r is not None and a is not None:
            if r == a: stats["exact"] += 1
            elif r > a: stats["too_many"] += 1
            else: stats["too_few"] += 1
        elif r is not None:
            stats["record_only"] += 1
    if recs and len(set(recs)) == 1:
        stats["single_value_cases"] += 1

stats["wall_s"] = round(time.time() - t0, 1)
stats["variant"] = variant
print(json.dumps(stats, indent=1))
