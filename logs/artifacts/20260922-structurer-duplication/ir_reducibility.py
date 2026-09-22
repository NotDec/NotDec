#!/usr/bin/env python3
"""Reducibility / loop-entry analysis over the parsed CFG.  A reducible CFG has
exactly one entry per loop (SCC); multiple entries = irreducible = goto-forced."""
import sys, collections, importlib.util
spec = importlib.util.spec_from_file_location('ia', '/sn640/NotDec/logs/artifacts/20260922-structurer-duplication/ir_analyze.py')
ia = importlib.util.module_from_spec(spec); spec.loader.exec_module(ia)

def sccs(nodes, succ):
    index = {}; low = {}; on = {}; stack = []; out = []; counter = [0]
    def strongconnect(v):
        work = [(v, iter(succ.get(v, [])))]
        index[v] = low[v] = counter[0]; counter[0] += 1; stack.append(v); on[v] = True
        while work:
            node, it = work[-1]
            advanced = False
            for w in it:
                if w not in index:
                    index[w] = low[w] = counter[0]; counter[0] += 1; stack.append(w); on[w] = True
                    work.append((w, iter(succ.get(w, []))))
                    advanced = True; break
                elif on.get(w):
                    low[node] = min(low[node], index[w])
            if advanced: continue
            work.pop()
            if work:
                p = work[-1][0]; low[p] = min(low[p], low[node])
            if low[node] == index[node]:
                comp = []
                while True:
                    w = stack.pop(); on[w] = False; comp.append(w)
                    if w == node: break
                out.append(comp)
    for v in nodes:
        if v not in index: strongconnect(v)
    return out

def analyze(path, funcfilter=None):
    funcs = ia.split_functions(path)
    res = []
    for name, bbs in funcs.items():
        if funcfilter and funcfilter not in name: continue
        preds = collections.defaultdict(list); succ = {}
        labels = [l for l, _ in bbs]
        for lbl, instrs in bbs:
            succ[lbl] = [s for s in ia.successors(instrs) if s in dict(bbs)]
            for s in succ[lbl]: preds[s].append(lbl)
        comps = sccs(labels, succ)
        multi = [c for c in comps if len(c) > 1 or (c[0] in succ.get(c[0], []))]
        irreducible = 0; details = []
        for c in multi:
            cs = set(c)
            entries = [n for n in c if any(p not in cs for p in preds[n])]
            if len(entries) > 1:
                irreducible += 1
                details.append((len(c), len(entries)))
        res.append((name, len(labels), len(multi), irreducible, details[:4]))
    return res

if __name__ == '__main__':
    for path in sys.argv[1:]:
        print('=====', path)
        for name, n, loops, irr, det in sorted(analyze(path), key=lambda x: -x[1])[:8]:
            print('  %-44s blocks=%-5d loops=%-3d irreducible=%-3d %s' % (name[:44], n, loops, irr, det))
