#!/usr/bin/env python3
"""IR-level shape analysis: block counts, switch/calldata reads, local duplication,
reducibility.  Used to decide whether the flattened shape's structurer failures come
from duplicated regions or from the merged caller/callee shape."""
import re, sys, glob, collections, hashlib, json

LABEL_RE = re.compile(r'^([-a-zA-Z$._0-9]+|"[^"]+"):')
DEF_RE = re.compile(r'^define\s+(?:\S+\s+)*?@("?[^"(]+"?)\(')
TERM_BR = re.compile(r'^\s*br\s+label\s+%([-a-zA-Z$._0-9]+)')
TERM_CBR = re.compile(r'^\s*br\s+i1\s+[^,]+,\s+label\s+%([-a-zA-Z$._0-9]+),\s+label\s+%([-a-zA-Z$._0-9]+)')
TERM_SWITCH = re.compile(r'^\s*switch\s+')

def split_functions(path):
    funcs = {}
    cur = None; curbb = None; buf = []
    for raw in open(path, errors='replace'):
        line = raw.rstrip('\n')
        m = DEF_RE.match(line)
        if m:
            cur = m.group(1).strip('"'); funcs[cur] = []; curbb = None; buf = []
            continue
        if cur is None:
            continue
        if line.startswith('}'):
            if curbb is not None:
                funcs[cur].append((curbb, buf))
            cur = None; curbb = None; buf = []
            continue
        lm = LABEL_RE.match(line)
        if lm and not line.lstrip().startswith(';'):
            if curbb is not None:
                funcs[cur].append((curbb, buf))
            curbb = lm.group(1).strip('"'); buf = []
            continue
        if curbb is not None:
            buf.append(line)
    return funcs

META_RE = re.compile(r',\s*![A-Za-z0-9._]+(?:\s+![0-9]+)?')

def successors(instrs):
    # terminator is the last instruction; join continuation lines for switch and
    # drop the trailing metadata attachments (", !notdec.evm !3") which would
    # otherwise defeat the end-anchored regexes.
    joined = ' '.join(x.strip() for x in instrs)
    joined = META_RE.sub('', joined)
    m = re.search(r'\bbr\s+i1\s+.*?label\s+%([-a-zA-Z$._0-9]+),\s*label\s+%([-a-zA-Z$._0-9]+)\s*$', joined)
    if m: return [m.group(1), m.group(2)]
    m = re.search(r'\bbr\s+label\s+%([-a-zA-Z$._0-9]+)\s*$', joined)
    if m: return [m.group(1)]
    m = re.search(r'\bswitch\s+.*?\[(.*)\]\s*$', joined)
    if m:
        return re.findall(r'label\s+%([-a-zA-Z$._0-9]+)', m.group(1))
    return []

def opcode(line):
    s = line.strip()
    if not s or s.startswith(';'): return None
    # strip result name
    m = re.match(r'^(?:%[-a-zA-Z$._0-9]+\s*=\s*)?([a-zA-Z][a-zA-Z0-9._]*)', s)
    if not m: return None
    return m.group(1)

def block_label(instrs):
    ops = []
    for line in instrs:
        o = opcode(line)
        if o: ops.append(o)
    return tuple(ops)

def analyze(path, topn=10):
    funcs = split_functions(path)
    out = {}
    for name, bbs in funcs.items():
        succ = {}; labels = {}
        for lbl, instrs in bbs:
            labels[lbl] = block_label(instrs)
            succ[lbl] = [s for s in successors(instrs) if s in dict(bbs)]
        n = len(bbs)
        # WL-style neighborhood hash, 4 rounds
        h = {lbl: hashlib.md5(repr(labels[lbl]).encode()).hexdigest()[:12] for lbl in labels}
        for _ in range(4):
            h = {lbl: hashlib.md5((h[lbl] + '|' + ','.join(sorted(h[s] for s in succ[lbl]))).encode()).hexdigest()[:12] for lbl in labels}
        cnt = collections.Counter(h.values())
        dup_blocks = sum(c for c in cnt.values() if c > 1)
        dup_groups = sum(1 for c in cnt.values() if c > 1)
        switches = sum(1 for lbl, ins in bbs if any(x.strip().startswith('switch ') for x in ins))
        calldata = sum(1 for lbl, ins in bbs for x in ins if 'evm_calldataload' in x or 'evm_calldatacopy' in x)
        calls = sum(1 for lbl, ins in bbs for x in ins if re.search(r'\bcall\b', x))
        out[name] = dict(blocks=n, dup_blocks=dup_blocks, dup_groups=dup_groups,
                         switches=switches, calldata_reads=calldata, calls=calls,
                         max_dup=max(cnt.values()) if cnt else 0)
    return out

if __name__ == '__main__':
    for path in sys.argv[1:]:
        res = analyze(path)
        total = sum(v['blocks'] for v in res.values())
        print('=====', path, 'funcs', len(res), 'blocks', total)
        for name, v in sorted(res.items(), key=lambda kv: -kv[1]['blocks'])[:12]:
            print('  %-42s blocks=%-5d dup=%-5d groups=%-4d maxcopy=%-4d switch=%-3d cdload=%-4d calls=%d' % (
                name[:42], v['blocks'], v['dup_blocks'], v['dup_groups'], v['max_dup'], v['switches'], v['calldata_reads'], v['calls']))
