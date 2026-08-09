#!/usr/bin/env python3
"""分析折叠 key 的 bound 变体：等价性、差异根因、局部性、频率。

行格式（NOTDEC_DUMP_COMPACT_GRAPH 探针输出）：
  N <ptr> <pol> <kind> <detail>   结构节点（V/P/R/F/?；V 的 detail 含 var id）
  E <src> <spol> <dst> <dpol>     结构引用边
  B <freshVar> <pol> <boundPtr>   折叠 key 的一次 bound 设置

B 行含义：同一 freshVar 的 distinct boundPtr = 变体。hash-cons 下同结构
同 ptr，故变体按 boundPtr 去重即可。

输出：
  1. 等价性：迭代签名（互模拟细化）分组，统计等价对。
  2. 差异根因：两两对齐比较，第一个差异点分类（kind/detail/v-ref/arity）。
  3. 局部性：变体可达节点集 shared/diff 大小（同 ptr 即同结构）。
  4. 变体频率：每个多变体 key 的各变体出现次数。
"""
import sys
from collections import defaultdict, Counter


def load(path):
    nodes = {}
    edges = defaultdict(list)
    binds = []
    for line in open(path):
        p = line.split()
        if not p:
            continue
        if p[0] == 'N' and len(p) >= 4:
            nodes[(p[1], p[2])] = (p[3], p[4] if len(p) > 4 else "")
        elif p[0] == 'E' and len(p) >= 5:
            edges[(p[1], p[2])].append((p[3], p[4]))
        elif p[0] == 'B' and len(p) >= 4:
            binds.append((p[1], p[2], p[3]))
    return nodes, edges, binds


def reach(root, edges):
    r = set()
    st = [root]
    while st:
        k = st.pop()
        if k in r:
            continue
        r.add(k)
        st.extend(edges.get(k, []))
    return r


def variant_sig(root, nodes, edges):
    """bound 根可达子图的迭代签名（互模拟细化），可处理环。"""
    rch = reach(root, edges)
    part = {k: nodes.get(k, ("?", "")) for k in rch}
    while True:
        sig = {}
        for k in rch:
            kids = sorted((w, part[w]) for w in edges.get(k, []))
            sig[k] = (part[k], tuple(kids))
        ids = {}
        new_part = {}
        for k in rch:
            sk = sig[k]
            if sk not in ids:
                ids[sk] = len(ids)
            new_part[k] = ids[sk]
        if list(new_part.values()) == list(part.values()):
            break
        part = new_part
    return sig[root]


def v_refs(node, nodes, edges):
    """V 节点引用变量 id 集合（detail 里 vars=N 之后的 id 列表）。"""
    info = nodes.get(node, ("?", ""))
    if info[0] != 'V':
        return None
    return frozenset(info[1].split()[1:])


def first_diff(a, b, nodes, edges, seen=None):
    """对齐比较两棵 bound 树，返回第一个差异点类型；无差异返回 None。"""
    if seen is None:
        seen = set()
    key = (a, b)
    if key in seen:
        return None
    seen.add(key)
    ka = nodes.get(a, ("?", ""))
    kb = nodes.get(b, ("?", ""))
    if ka[0] != kb[0]:
        return ("kind", ka[0], kb[0])
    if ka != kb:
        va = v_refs(a, nodes, edges)
        vb = v_refs(b, nodes, edges)
        if va is not None and vb is not None and va != vb:
            return ("v-ref", ka[1], kb[1])
        return ("detail", ka[1], kb[1])
    ca = sorted(edges.get(a, []),
                key=lambda w: (w[1], nodes.get(w, ("?", ""))[0],
                               nodes.get(w, ("?", ""))[1]))
    cb = sorted(edges.get(b, []),
                key=lambda w: (w[1], nodes.get(w, ("?", ""))[0],
                               nodes.get(w, ("?", ""))[1]))
    if len(ca) != len(cb):
        return ("arity", len(ca), len(cb))
    for x, y in zip(ca, cb):
        d = first_diff(x, y, nodes, edges, seen)
        if d:
            return d
    return None


def analyze(path):
    nodes, edges, binds = load(path)
    per_key = defaultdict(list)
    for fv, pol, bptr in binds:
        per_key[(fv, pol)].append(bptr)
    print(f"== {path}")
    multi = 0
    equiv_pairs = 0
    total_pairs = 0
    diff_kinds = defaultdict(int)
    examples = defaultdict(list)
    shared_total = diff_total = 0
    for key, variants in per_key.items():
        uniq = list(dict.fromkeys(variants))
        if len(uniq) <= 1:
            continue
        multi += 1
        sigs = [(b, variant_sig((b, key[1]), nodes, edges)) for b in uniq]
        for i in range(len(sigs)):
            for j in range(i + 1, len(sigs)):
                a, sa = sigs[i]
                b, sb = sigs[j]
                total_pairs += 1
                if sa == sb:
                    equiv_pairs += 1
                    continue
                d = first_diff((a, key[1]), (b, key[1]), nodes, edges)
                if d is None:
                    d = ("pol-only", "", "")
                diff_kinds[d[0]] += 1
                if len(examples[d[0]]) < 2:
                    examples[d[0]].append((key, a, b, d))
                ra, rb = reach((a, key[1]), edges), reach((b, key[1]), edges)
                shared_total += len(ra & rb)
                diff_total += len(ra ^ rb)
        c = Counter(variants)
        if len(uniq) <= 40:
            print(f"  key {key}: variants={len(uniq)} total={sum(c.values())}"
                  f" freq={dict(c)}")
    print(f"  multi-variant keys: {multi}, equivalent pairs:"
          f" {equiv_pairs}/{total_pairs}")
    print(f"  diff roots: {dict(diff_kinds)}")
    if total_pairs:
        print(f"  locality: avg_shared={shared_total/total_pairs:.1f}"
              f" avg_diff={diff_total/total_pairs:.1f}")
    for k, exs in examples.items():
        for key, a, b, d in exs:
            print(f"    {k} example key={key} a={a} b={b} d={d}")


def main():
    for path in sys.argv[1:]:
        analyze(path)


if __name__ == "__main__":
    main()
