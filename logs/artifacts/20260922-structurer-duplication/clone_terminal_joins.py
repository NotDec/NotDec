#!/usr/bin/env python3
"""Clone every TERMINAL join block (>=2 predecessors, terminator ret/unreachable,
no successors) for all but one predecessor.  Terminal blocks define no value any
successor reads, so the clone is always sound.  Diagnostic: does removing the
shared tail unblock the structurer?"""
import re, sys, importlib.util
spec = importlib.util.spec_from_file_location('ia', '/sn640/NotDec/logs/artifacts/20260922-structurer-duplication/ir_analyze.py')
ia = importlib.util.module_from_spec(spec); spec.loader.exec_module(ia)

src = sys.argv[1]
suffix = sys.argv[2]
text = open(src, errors='replace').read()
out = []
pos = 0
total_clones = 0
for m in re.finditer(r'^define [^\n]*@([^\s(]+)\(.*?^\}', text, re.S | re.M):
    func = m.group(0)
    lines = func.split('\n')
    head_end = next(i for i, l in enumerate(lines) if re.match(r'^[-a-zA-Z$._0-9]+:', l))
    head = '\n'.join(lines[:head_end])
    blocks = []; cur = None
    for l in lines[head_end:]:
        if re.match(r'^[-a-zA-Z$._0-9]+:', l):
            if cur: blocks.append(cur)
            cur = [l]
        elif cur is not None:
            cur.append(l)
    if cur: blocks.append(cur)
    closing = []
    if blocks and blocks[-1][-1].strip() == '}':
        closing = [blocks[-1].pop()]
        if len(blocks[-1]) == 1: blocks.pop()
    order = [b[0].split(':')[0] for b in blocks]
    idx = {l: i for i, l in enumerate(order)}
    succ = {}
    for b in blocks:
        lbl = b[0].split(':')[0]
        succ[lbl] = [s for s in ia.successors(b[1:]) if s in idx]
    preds = {l: [] for l in order}
    for l, ss in succ.items():
        for s in ss: preds[s].append(l)
    newblocks = list(blocks)
    counter = 0
    for lbl in order:
        b = blocks[idx[lbl]]
        ins = ' '.join(x.strip() for x in b[1:])
        terminal = (not succ[lbl]) and re.search(r'\b(ret|unreachable)\b[^\n]*$', ia.META_RE.sub('', ins).strip()) is not None
        if len(preds[lbl]) < 2 or not terminal:
            continue
        defs = set(re.findall(r'^\s*(%[-a-zA-Z$._0-9]+)\s*=', '\n'.join(b), re.M))
        keep = preds[lbl][0]
        for pred in preds[lbl][1:]:
            counter += 1; total_clones += 1
            newlbl = '%s.cl%d' % (lbl, counter)
            clone = []
            for l in b:
                l = l.replace(lbl + ':', newlbl + ':', 1)
                for d in defs:
                    pat = r'(?<![A-Za-z0-9_.])' + re.escape(d) + r'(?![A-Za-z0-9_.])'
                    repl = '%v' + d[1:] + 'c' if d[1:].isdigit() else d + 'c'
                    l = re.sub(pat, repl, l)
                clone.append(l)
            clone = [re.sub(r'; preds = .*$', '; preds = %' + pred, l) if l.startswith(newlbl + ':') else l for l in clone]
            pb = newblocks[idx[pred]]
            for i, l in enumerate(pb):
                stripped = ia.META_RE.sub('', l.strip())
                if re.search(r'%' + re.escape(lbl) + r'(\s|,|$)', stripped) and re.search(r'\b(br|switch)\b', stripped):
                    pb[i] = re.sub(r'%' + re.escape(lbl) + r'(?![A-Za-z0-9_.])', '%' + newlbl, l)
            newblocks.append(clone)
    body = head + '\n' + '\n'.join('\n'.join(b) for b in newblocks) + '\n' + '\n'.join(closing)
    out.append(body)
print('clones:', total_clones)
res = []
last = 0
for m in re.finditer(r'^define [^\n]*@([^\s(]+)\(.*?^\}', text, re.S | re.M):
    res.append(text[last:m.start()]); last = m.end()
res.append(text[last:])
# rebuild: interleave the untouched text with rewritten functions
result = ''
idx2 = 0
last = 0
for m in re.finditer(r'^define [^\n]*@([^\s(]+)\(.*?^\}', text, re.S | re.M):
    result += text[last:m.start()] + out[idx2]; idx2 += 1; last = m.end()
result += text[last:]
open(src.replace('.ll', '.' + suffix + '.ll'), 'w').write(result)
print('wrote', src.replace('.ll', '.' + suffix + '.ll'))
