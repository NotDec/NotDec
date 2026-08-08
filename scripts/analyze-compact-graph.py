#!/usr/bin/env python3
"""分析 CompactType 层图探针（NOTDEC_DUMP_COMPACT_GRAPH）输出。

行格式：
  N <ptr> <kind> <detail>    merged 结构节点（V/R/F/P）
  R <ptr> <pol>              canonicalize root
  E <src> <spol> <dst> <dpol>  结构引用边
  F <ptr> <pol>              折叠 key（go1 折叠事件，去重输出）

关键指标（对应折叠展开爆炸）：
  - fold_keys: F 行数（recursive 表规模）
  - fold_in_deg: 每个折叠 key 被多少结构引用（互锁汇聚）
  - fold_ref_edges: 折叠 key 之间的引用（环嵌套）
  - fold_paths: 从 roots 到折叠 key 的路径数（DAG DP，log10）
  - fold 事件增长：F 行位置分布（爆炸速率）
"""

import sys
from collections import defaultdict, deque


def parse(path):
    nodes = {}
    roots = set()
    folds = set()
    edges = defaultdict(list)
    for line in open(path):
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "N" and len(parts) >= 3:
            nodes[(parts[1], parts[2])] = parts[3] if len(parts) > 3 else ""
        elif parts[0] == "R" and len(parts) >= 3:
            roots.add((parts[1], parts[2]))
        elif parts[0] == "E" and len(parts) >= 5:
            edges[(parts[1], parts[2])].append((parts[3], parts[4]))
        elif parts[0] == "F" and len(parts) >= 3:
            folds.add((parts[1], parts[2]))
    return nodes, roots, folds, edges


def analyze(path):
    nodes, roots, folds, edges = parse(path)
    all_keys = (set(nodes) | set(folds) | set(roots) |
                set(edges) | {w for ss in edges.values() for w in ss})
    in_edges = defaultdict(list)
    for v, ss in edges.items():
        for w in ss:
            in_edges[w].append(v)
    # SCC（Tarjan，迭代）
    index = {}
    lowlink = {}
    on_stack = set()
    stack = []
    scc_of = {}
    scc_size = defaultdict(int)
    counter = [0]
    next_sid = [0]
    for start in all_keys:
        if start in index:
            continue
        work = [(start, 0)]
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
                        if w == v:
                            break
                if work:
                    u = work[-1][0]
                    lowlink[u] = min(lowlink[u], lowlink[v])
    nontriv = sorted((s for s in scc_size if scc_size[s] > 1),
                     key=lambda s: -scc_size[s])
    # DAG + 路径计数
    dag = defaultdict(set)
    for v, ss in edges.items():
        a = scc_of[v]
        for w in ss:
            b = scc_of[w]
            if a != b:
                dag[a].add(b)
    indeg = defaultdict(int)
    for a, ss in dag.items():
        for b in ss:
            indeg[b] += 1
    topo = []
    q = deque(s for s in range(next_sid[0]) if indeg[s] == 0)
    while q:
        a = q.popleft()
        topo.append(a)
        for b in dag.get(a, []):
            indeg[b] -= 1
            if indeg[b] == 0:
                q.append(b)
    ways_to = defaultdict(int)
    for r in roots:
        ways_to[scc_of[r]] += 1
    for a in topo:
        for b in dag.get(a, []):
            ways_to[b] += ways_to[a]
    # 折叠 key 指标
    fold_in = [(k, len(in_edges.get(k, []))) for k in folds]
    fold_in.sort(key=lambda x: -x[1])
    fold_scc = defaultdict(int)
    for k in folds:
        fold_scc[scc_of[k]] += 1
    fold_to_fold = sum(1 for k in folds for w in edges.get(k, []) if w in folds)
    fold_ways = [(k, ways_to[scc_of[k]]) for k in folds]
    fold_ways.sort(key=lambda x: -x[1])
    return {
        "path": path,
        "nodes": len(nodes),
        "edges": sum(len(s) for s in edges.values()),
        "roots": len(roots),
        "fold_keys": len(folds),
        "sccs": next_sid[0],
        "nontrivial_sccs": len(nontriv),
        "max_scc_size": scc_size[nontriv[0]] if nontriv else 0,
        "fold_scc_with_ring": sum(1 for s in fold_scc if scc_size[s] > 1),
        "fold_to_fold_edges": fold_to_fold,
        "fold_max_in_deg": fold_in[0][1] if fold_in else 0,
        "fold_in_deg_top": [(k, d) for k, d in fold_in[:10]],
        "fold_max_ways_log10": (
            (fold_ways[0][1].bit_length() - 1) * 0.30103 if fold_ways else 0),
        "fold_ways_top": [(k, (w.bit_length() - 1) * 0.30103)
                          for k, w in fold_ways[:10]],
        "max_ways_to_log10": (
            (max(ways_to.values()).bit_length() - 1) * 0.30103
            if ways_to else 0),
    }


def main():
    results = [analyze(p) for p in sys.argv[1:]]
    fields = ["nodes", "edges", "roots", "fold_keys", "sccs",
              "nontrivial_sccs", "max_scc_size", "fold_scc_with_ring",
              "fold_to_fold_edges", "fold_max_in_deg",
              "fold_max_ways_log10", "max_ways_to_log10"]
    print(f"{'metric':<24}" + "".join(f"{r['path'].split('/')[-1]:>18}"
                                      for r in results))
    for f in fields:
        print(f"{f:<24}" + "".join(f"{r[f]:>18}" for r in results))
    for r in results:
        print(f"\n=== {r['path']} ===")
        print("fold in-deg top:", r["fold_in_deg_top"])
        print("fold ways top:", r["fold_ways_top"])


if __name__ == "__main__":
    main()
