#!/usr/bin/env python3
"""Measure how work scales with the number of instantiated callee summaries."""
import json, os, re, subprocess, time

ROOT = "/sn640/NotDec"
SRC = os.path.join(ROOT, "test/evm/solidity-patterns/cases/24259_19755445_aefeec2314_4f43187f4106.ll")
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")
WORK = "/tmp/scaling24259"
os.makedirs(WORK, exist_ok=True)
BUDGET = int(os.environ.get("BUDGET_ADDS", "2000000"))
CAP = float(os.environ.get("CAP_S", "120"))

src = open('/tmp/reduce_24259.py').read().split('if __name__')[0]
ns = {}
exec(src, ns)
text = open(SRC).read()
preamble, blocks = ns['parse'](text)
names = [b[0] for b in blocks]
full = {n: blk for n, blk, _, _ in blocks}

# helpers directly called by public__0x4a00cc48_0x74f, ordered by call multiplicity
sweep = ["private__0xc2c_0xc2c", "private__0xcfb_0xcfb", "private__0xd2f_0xd2f",
         "private__0xe54_0xe54", "private__0xaf9_0xaf9", "private__0xdae_0xdae",
         "private__0xd1b_0xd1b", "private__0xb2f_0xb2f", "private__0xcc3_0xcc3"]
mult = {"private__0xc2c_0xc2c": 1, "private__0xcfb_0xcfb": 1, "private__0xd2f_0xd2f": 1,
        "private__0xe54_0xe54": 1, "private__0xaf9_0xaf9": 2, "private__0xdae_0xdae": 2,
        "private__0xd1b_0xd1b": 3, "private__0xb2f_0xb2f": 3, "private__0xcc3_0xcc3": 4}

def build(keep, path):
    parts = [preamble]
    for n in names:
        parts.append(full[n] if n in keep else ns['declare_version'](full[n]))
    open(path, 'w').write(''.join(parts))

env = dict(os.environ)
env['NOTDEC_EVM_POLY_ENTRIES'] = '1'
env['NOTDEC_WORKLIST_ABORT_ADDS'] = str(BUDGET)
env['NOTDEC_WORKLIST_SNAPSHOT'] = '1'
env.pop('NOTDEC_DEFERRED_DIAG', None)

rows = []
for k in range(0, len(sweep) + 1):
    keep = {"public___function_selector___0x0", "public__0x4a00cc48_0x74f"} | set(sweep[:k])
    cand = os.path.join(WORK, "k%d.ll" % k)
    build(keep, cand)
    t0 = time.time()
    try:
        p = subprocess.run([BIN, cand, "-o", os.path.join(WORK, "out.sol"), "--tr-level=2"],
                           env=env, cwd=ROOT, capture_output=True, timeout=CAP)
        out = p.stdout.decode(errors="replace") + p.stderr.decode(errors="replace")
        rc = p.returncode
    except subprocess.TimeoutExpired as ex:
        out = (ex.stdout or b'').decode(errors="replace") + (ex.stderr or b'').decode(errors="replace")
        rc = 124
    dt = time.time() - t0
    mob = re.search(r"\[worklist-abort\] add_calls=(\d+) nodes=(\d+) vars=(\d+) lb=(\d+) ub=(\d+) nested=(\d+)", out)
    mg = re.search(r"\[type-graph\] label=solve-and-lower-types-end nodes=(\d+) vars=(\d+) rep=\d+ merged=\d+ lb=(\d+) ub=(\d+) structural=\d+ nested=(\d+) add_calls=(\d+)", out)
    if mob:
        detail = dict(add_calls=int(mob.group(1)), nodes=int(mob.group(2)), vars=int(mob.group(3)),
                      lb=int(mob.group(4)), ub=int(mob.group(5)), nested=int(mob.group(6)))
    elif mg:
        detail = dict(add_calls=int(mg.group(6)), nodes=int(mg.group(1)), vars=int(mg.group(2)),
                      lb=int(mg.group(3)), ub=int(mg.group(4)), nested=int(mg.group(5)))
    else:
        detail = {}
    callsites = sum(mult[h] for h in sweep[:k])
    row = dict(k=k, helpers=list(sweep[:k]), callsites=callsites, rc=rc,
               seconds=round(dt, 2), **detail)
    rows.append(row)
    print(json.dumps(row), flush=True)

json.dump(rows, open(os.path.join(WORK, "scaling.json"), "w"), indent=1)
