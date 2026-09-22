#!/usr/bin/env python3
"""Join-block profile: blocks with >=2 predecessors, and the specific shape
'conditional branch whose THEN successor is also the post-dominator/join'."""
import sys, collections, importlib.util
spec = importlib.util.spec_from_file_location('ia', '/sn640/NotDec/logs/artifacts/20260922-structurer-duplication/ir_analyze.py')
ia = importlib.util.module_from_spec(spec); spec.loader.exec_module(ia)

def profile(path):
    funcs = ia.split_functions(path)
    out = []
    for name, bbs in funcs.items():
        order = [l for l, _ in bbs]
        succ = {}
        for lbl, instrs in bbs:
            succ[lbl] = [s for s in ia.successors(instrs) if s in set(order)]
        preds = collections.defaultdict(set)
        for lbl, ss in succ.items():
            for s in ss: preds[s].add(lbl)
        joins = [l for l in order if len(preds[l]) >= 2]
        # conditional branches whose then-successor is a join
        then_join = 0
        for lbl in order:
            ss = succ[lbl]
            if len(ss) == 2 and len(preds[ss[0]]) >= 2:
                then_join += 1
        out.append((name, len(order), len(joins), then_join, sum(1 for l in order if len(succ[l]) == 2)))
    return out

if __name__ == '__main__':
    for path in sys.argv[1:]:
        print('=====', path)
        for name, n, joins, tj, br2 in sorted(profile(path), key=lambda x: -x[1])[:10]:
            print('  %-44s blocks=%-5d joins=%-4d then_is_join=%-4d cond=%-4d' % (name[:44], n, joins, tj, br2))
