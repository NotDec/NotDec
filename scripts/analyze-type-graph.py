#!/usr/bin/env python3
"""静态分析 binarysub dumpTypeGraph 输出的引用图，预测 canonicalize 折叠展开爆炸。

用法: analyze-type-graph.py graph1.txt [graph2.txt ...]

边表由 NOTDEC_DUMP_TYPE_GRAPH 生成，行格式:
  N <ptr> <kind>             节点 (V/R/F/P)
  R <ptr> <pol>              展开 root
  E <src> <spol> <dst> <dpol>  引用边

分析内容:
  - SCC（Tarjan）：环的数量/大小，即折叠 key 的组合空间
  - SCC 收缩 DAG 上的路径计数（ways_to / suffix）：重复展开次数上界
  - 环闭合路径估计：Sigma ways_to(环) x 2^环大小
  - 节点贡献 ways_to x suffix：合并候选排序
  - 高入度共享节点：多路径汇聚点
"""

import json
import sys
from collections import defaultdict, deque


def parse(path):
    # 节点 key 统一为 (ptr, pol)；N 行只提供 ptr -> kind 映射。
    ptr_kind = {}
    roots = set()
    edges = defaultdict(list)
    nodes = {}
    for line in open(path):
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "N":
            if len(parts) < 3:
                continue
            ptr_kind[parts[1]] = (parts[2], parts[3] if len(parts) > 3 else "")
        elif parts[0] == "R":
            roots.add((parts[1], parts[2]))
        elif parts[0] == "E":
            if len(parts) < 5:
                continue  # 截断行（探针被终止时）
            src = (parts[1], parts[2])
            dst = (parts[3], parts[4])
            edges[src].append(dst)
    for v in list(roots) + [w for ss in edges.values() for w in ss] +             list(edges.keys()):
        if v not in nodes:
            nodes[v] = ptr_kind.get(v[0], ("?", ""))
    return nodes, roots, edges


def analyze(path):
    nodes, roots, edges = parse(path)
    raw_nodes = len(nodes)
    raw_edges = sum(len(s) for s in edges.values())
    nodes, edges, roots = refine_graph(nodes, edges, roots)
    keys = list(nodes.keys())
    # iterative Tarjan（可处理 10^5 节点）
    index = {}
    lowlink = {}
    on_stack = set()
    stack = []
    scc_of = {}
    scc_size = defaultdict(int)
    scc_kind = defaultdict(lambda: defaultdict(int))
    counter = [0]
    next_sid = [0]
    for start in keys:
        if start in index:
            continue
        work = [(start, 0)]  # (node, next-edge-pos)
        while work:
            v, pi = work[-1]
            if pi == 0:
                index[v] = lowlink[v] = counter[0]
                counter[0] += 1
                stack.append(v)
                on_stack.add(v)
            succs = edges.get(v, [])
            if pi < len(succs):
                w = succs[pi]
                work[-1] = (v, pi + 1)
                if w not in index:
                    work.append((w, 0))
                elif w in on_stack:
                    lowlink[v] = min(lowlink[v], index[w])
            else:
                work.pop()
                if lowlink[v] == index[v]:
                    sid = next_sid[0]
                    next_sid[0] += 1
                    while True:
                        w = stack.pop()
                        on_stack.discard(w)
                        scc_of[w] = sid
                        scc_size[sid] += 1
                        scc_kind[sid][nodes[w][0]] += 1
                        if w == v:
                            break
                if work:
                    u = work[-1][0]
                    lowlink[u] = min(lowlink[u], lowlink[v])
    # 收缩 DAG
    dag_edges = defaultdict(set)
    for v, succs in edges.items():
        a = scc_of[v]
        for w in succs:
            b = scc_of[w]
            if a != b:
                dag_edges[a].add(b)
    dag_edges = {k: list(v) for k, v in dag_edges.items()}
    # Kahn 拓扑序
    indeg = defaultdict(int)
    for a, succs in dag_edges.items():
        for b in succs:
            indeg[b] += 1
    topo = []
    q = deque(s for s in range(next_sid[0]) if indeg[s] == 0)
    while q:
        a = q.popleft()
        topo.append(a)
        for b in dag_edges.get(a, []):
            indeg[b] -= 1
            if indeg[b] == 0:
                q.append(b)
    # 路径计数（大整数）
    ways_to = defaultdict(int)
    for r in roots:
        ways_to[scc_of[r]] += 1
    for a in topo:
        for b in dag_edges.get(a, []):
            ways_to[b] += ways_to[a]
    suffix = defaultdict(int)
    for a in reversed(topo):
        s = 1
        for b in dag_edges.get(a, []):
            s += suffix[b]
        suffix[a] = s
    # 环闭合路径估计
    ring_ways = 0
    ring_paths = 0
    nontriv = sorted((sid for sid in scc_size if scc_size[sid] > 1),
                     key=lambda s: -scc_size[s])
    for sid in nontriv:
        ring_ways += ways_to[sid]
        ring_paths += ways_to[sid] * (1 << min(scc_size[sid], 30))
    # 节点贡献（收缩 DAG 上近似；环内节点共享 SCC 的 ways）
    in_edges = defaultdict(list)
    for v, ss in edges.items():
        for w in ss:
            in_edges[w].append(v)
    contrib = []
    for v in keys:
        sid = scc_of[v]
        c = ways_to[sid] * suffix[sid]
        contrib.append((c, v, nodes[v], len(in_edges[v])))
    contrib.sort(reverse=True)
    # 互锁：非平凡 SCC 的入邻居 SCC 数
    scc_in = defaultdict(set)
    for v, ss in edges.items():
        a = scc_of[v]
        for w in ss:
            b = scc_of[w]
            if a != b:
                scc_in[b].add(a)
    interlock = [(sid, len(scc_in[sid])) for sid in nontriv]
    interlock.sort(key=lambda x: -x[1])
    high_indeg = sorted(
        ((v, k[0], len(in_edges[v])) for v, k in nodes.items()),
        key=lambda x: -x[2])[:15]
    return {
        "path": path,
        "_nodes": nodes,
        "_roots": roots,
        "_edges": dict(edges),
        "nodes": len(nodes),
        "edges": sum(len(s) for s in edges.values()),
        "roots": len(roots),
        "sccs": next_sid[0],
        "nontrivial_sccs": len(nontriv),
        "max_scc_size": scc_size[nontriv[0]] if nontriv else 0,
        "scc_sizes_top": [scc_size[s] for s in nontriv[:8]],
        "ring_ways_sum": ring_ways,
        "ring_paths_est_log10": (ring_paths.bit_length() - 1) * 0.30103,
        "max_ways_to_log10": (max(ways_to.values()).bit_length() - 1) * 0.30103
        if ways_to else 0,
        "total_paths_log10": (sum(ways_to.values()).bit_length() - 1) * 0.30103
        if ways_to else 0,
        "interlock_top": [(scc_size[s], n, len(scc_in[s])) for s, n in interlock[:8]],
        "contrib_top": [
            {"kind": k,
             "ways_to_log10": (ways_to[scc_of[v]].bit_length() - 1) * 0.30103,
             "suffix_log10": (suffix[scc_of[v]].bit_length() - 1) * 0.30103,
             "contrib_log10": (c.bit_length() - 1) * 0.30103,
             "in_deg": d, "node": v}
            for c, v, k, d in contrib[:15]],
        "high_indeg": high_indeg,
        "raw_nodes": raw_nodes,
        "raw_edges": raw_edges,
    }


def var_closures(nodes, edges):
    """按 canonicalize closeOver 语义收缩变量闭包。

    对每个 (变量, pol)，闭包 = 沿变量->变量同极性边可达的变量集合；
    超节点 id = frozenset(闭包)。结构节点 (R/F/P) 保持原样。
    """
    var_keys = [k for k in nodes if nodes[k] == "V"]
    var_succ = {}
    for k in var_keys:
        var_succ[k] = [w for w in edges.get(k, [])
                       if nodes[w] == "V" and w[1] == k[1]]
    memo = {}
    def closure(seed):
        if seed in memo:
            return memo[seed]
        seen = set()
        stack = [seed]
        while stack:
            v = stack.pop()
            if v in seen:
                continue
            seen.add(v)
            for w in var_succ.get(v, []):
                if w not in seen:
                    stack.append(w)
        memo[seed] = frozenset(seen)
        return memo[seed]
    # 超节点映射：原始 key -> 超节点 key
    sup = {}
    for k in var_keys:
        sup[k] = ("C", closure(k))
    for k in nodes:
        if nodes[k] != "V":
            sup[k] = k
    # 超图
    sup_nodes = {}
    sup_edges = defaultdict(list)
    sup_roots = set()
    def map_key(k):
        return sup.get(k, k)
    for k, kind in nodes.items():
        s = map_key(k)
        if s not in sup_nodes:
            sup_nodes[s] = kind
    for v, ss in edges.items():
        a = map_key(v)
        for w in ss:
            b = map_key(w)
            if a != b:
                sup_edges[a].append(b)
    return sup_nodes, sup_edges


def simulate_expansions(nodes, edges, roots, limit=5000000):
    """无缓存 DFS 折叠模拟：数 go1 展开次数（不含折叠调用）。

    规则对齐 canonicalizeType go1：
      - (node,pol) 在活动路径或全局折叠表 -> 折叠（不计数）
      - 否则展开 children（计数），并进入活动路径
      - 展开完成后从路径移除（兄弟之间不互为环）
    达到 limit 视为爆炸，抛 RuntimeError。
    """
    folded = set()
    count = [0]
    sys.setrecursionlimit(1000000)
    def expand(key, path):
        if key in path or key in folded:
            return
        count[0] += 1
        if count[0] > limit:
            raise RuntimeError("expansion limit exceeded")
        path.add(key)
        for w in edges.get(key, []):
            expand(w, path)
        path.remove(key)
    try:
        for r in roots:
            expand(r, set())
    except RuntimeError:
        return None
    return count[0]


def simulate_with_cache(nodes, edges, roots, limit=5000000):
    """带 go1FullCache 语义的展开模拟。

    规则对齐 canonicalizeType go1：
      - key 在活动路径或全局折叠表 -> 折叠（子帧折叠，不计展开）
      - key 在无折叠缓存 -> 命中（不计展开）
      - 否则展开 children（计数）；子树无折叠且自身未折叠 -> 入缓存
    返回展开次数，超过 limit 抛 RuntimeError。
    """
    folded = set()
    full_cache = set()
    count = [0]
    sys.setrecursionlimit(1000000)
    def expand(key, path):
        if key in path or key in folded:
            return True  # 折叠
        if key in full_cache:
            return False  # 缓存命中，无折叠
        count[0] += 1
        if count[0] > limit:
            raise RuntimeError("expansion limit exceeded")
        path.add(key)
        has_fold = False
        for w in edges.get(key, []):
            if expand(w, path):
                has_fold = True
        path.remove(key)
        if not has_fold and key not in folded:
            full_cache.add(key)
        return has_fold
    try:
        for r in roots:
            expand(r, set())
    except RuntimeError:
        return None
    return count[0]


def refine_graph(nodes, edges, roots):
    """结构分区细化：按 (kind, detail) 初始分区，迭代加后继分区签名。

    模拟 CompactType 的 hash-cons：同构结构节点合并成超节点。
    变量节点先按 closeOver 可达集收缩成 ('C', frozenset) 超节点（不合并）。
    返回 (sup_nodes, sup_edges, sup_roots)，key 为 ('C', fs) 或 ('S', class_id)。
    """
    var_keys = [k for k in nodes if nodes[k][0] == "V"]
    struct_keys = [k for k in nodes if nodes[k][0] != "V"]
    var_succ = {}
    for k in var_keys:
        var_succ[k] = [w for w in edges.get(k, [])
                       if nodes[w][0] == "V" and w[1] == k[1]]
    memo = {}
    def closure(seed):
        if seed in memo:
            return memo[seed]
        seen = set()
        stack = [seed]
        while stack:
            v = stack.pop()
            if v in seen:
                continue
            seen.add(v)
            for w in var_succ.get(v, []):
                if w not in seen:
                    stack.append(w)
        memo[seed] = frozenset(seen)
        return memo[seed]
    # 结构分区细化
    part = {k: (nodes[k][0], nodes[k][1]) for k in struct_keys}
    while True:
        sig = {}
        for k in struct_keys:
            succs = sorted(edges.get(k, []), key=str)
            succ_ids = []
            for w in succs:
                if nodes[w][0] == "V":
                    succ_ids.append(("C", closure(w)))
                else:
                    succ_ids.append(("S", part[w]))
            sig[k] = (part[k], tuple(succ_ids))
        # 稳定 = 每个当前类的成员签名一致（不再细分）
        sig_by_class = {}
        stable = True
        for k in struct_keys:
            if part[k] in sig_by_class:
                if sig_by_class[part[k]] != sig[k]:
                    stable = False
                    break
            else:
                sig_by_class[part[k]] = sig[k]
        if stable:
            break
        ids = {}
        new_part = {}
        for k in struct_keys:
            sk = sig[k]
            if sk not in ids:
                ids[sk] = len(ids)
            new_part[k] = ids[sk]
        part = new_part
    sup_nodes = {}
    for k in var_keys:
        sup_nodes[("C", closure(k))] = ("V", "closure")
    for k in struct_keys:
        sup_nodes[("S", part[k])] = nodes[k]
    sup_edges = defaultdict(list)
    seen_edges = set()
    def map_key(k):
        if nodes[k][0] == "V":
            return ("C", closure(k))
        return ("S", part[k])
    for v, ss in edges.items():
        a = map_key(v)
        for w in ss:
            b = map_key(w)
            if a != b and (a, b) not in seen_edges:
                seen_edges.add((a, b))
                sup_edges[a].append(b)
    sup_roots = set()
    for r in roots:
        sup_roots.add(map_key(r))
    return sup_nodes, sup_edges, sup_roots


def main():
    paths = sys.argv[1:]
    results = []
    for p in paths:
        r = analyze(p)
        exp2 = simulate_with_cache(r["_nodes"], r["_edges"], r["_roots"])
        r["sim_cached_expansions"] = exp2
        r["sim_cached_log10"] = (
            (exp2.bit_length() - 1) * 0.30103 if exp2 else None)
        results.append(r)
    fields = ["raw_nodes", "nodes", "raw_edges", "edges", "roots", "sccs",
              "nontrivial_sccs", "max_scc_size", "ring_ways_sum",
              "ring_paths_est_log10", "max_ways_to_log10",
              "total_paths_log10", "sim_cached_expansions"]
    print(f"{'metric':<22}" + "".join(f"{r['path'].split('/')[-1]:>18}"
                                      for r in results))
    for f in fields:
        print(f"{f:<22}" + "".join(f"{r[f]:>18}" for r in results))
    for r in results:
        print(f"\n=== {r['path']} ===")
        print("SCC sizes top:", r["scc_sizes_top"])
        print("Interlock (size, indegree) top:", r["interlock_top"])
        print("Contrib top:")
        for c in r["contrib_top"]:
            print(f"  {c['kind']} in_deg={c['in_deg']:>4} "
                  f"ways={c['ways_to_log10']:6.2f} "
                  f"suffix={c['suffix_log10']:6.2f} "
                  f"contrib={c['contrib_log10']:6.2f} {c['node']}")
        print("High in-degree nodes:")
        for v, k, d in r["high_indeg"][:10]:
            print(f"  {k} in_deg={d:>5} {v}")
    with open("/tmp/type-graph-analysis.json", "w") as f:
        json.dump(results, f, indent=1)


if __name__ == "__main__":
    main()
