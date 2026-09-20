import re, subprocess, sys, os
ROOT = "/sn640/NotDec"
BIN = os.path.join(ROOT, "build-notdec-nothreads/bin/notdec")

def run(path):
    out = "/tmp/" + os.path.basename(path) + ".out.ll"
    p = subprocess.run([BIN, path, "-o", out, "--tr-level=2"], cwd=ROOT,
                       capture_output=True, timeout=180)
    if p.returncode != 0 or not os.path.exists(out):
        print("  run failed rc=", p.returncode)
        return
    text = open(out, errors="replace").read()
    ids = dict(re.findall(r'^!([0-9]+) = !\{!"([^"]*)"\}', text, re.M))
    for m in re.finditer(r"^define[^\n]*?@([A-Za-z0-9_.$]+)\([^\n]*?\{", text, re.M):
        h = text[m.start():m.end()]
        rr = re.search(r"!notdec\.solidity\.abi_param\.record !([0-9]+)", h)
        aa = re.search(r"!notdec\.solidity\.abi_param\.annotated !([0-9]+)", h)
        if not rr and not aa:
            continue
        r = ids.get(rr.group(1), "?") if rr else "-"
        a = ids.get(aa.group(1), "?") if aa else "-"
        print("  %-30s record=%-3s annotated=%s" % (m.group(1), r, a))

for f in sys.argv[1:]:
    print("=== %s ===" % f)
    run(f)
