# 原始 prompt

首先在划分 SCC 方面，我感觉 level 0 的不用管调用了，直接全部搞成一个 SCC。其他更高的level可以按照当前的方式处理。但是level0可以全部合并起来作为大的SCC。其次就是对于从全局变量出发的节点，全局变量相关的，可能得做模块级共享，固定 level=0 这一块再想想吧。先把前面的 level=0 的特殊处理做好。

# level0 SCC 合并实现记录

## 背景

fortune 的 source IR 里，`calc_equal_probs` 会通过全局 `File_list` 使用 `struct fd*`，但当前 call graph 里没有保留从主链路到它的普通调用边。原 SCC 分组只沿 same-level 调用边合并，导致 level0 里仍有多个 generator，进而让同一个全局 fd 链表相关节点落到不同 root。

本次只做前半段：level0 原始 SCC 全部合成一个 generator。全局变量模块级共享固定 level0 的方案还没有实现。

## 修改

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5086`：更新 `prepareSCC()` Phase 2 注释，说明 level0 是单态模块作用域，高 level 仍按 same-level 调用边合并。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5125`：在 same-level 调用边合并后，把所有 `RawSCCs[RawIndex].Level == 0` 的 raw SCC union 到同一个 root。
- `test/tools/compare_htypes_with_wasm_dwarf.py:17`：让字段解析同时识别 `recursive body at offset: N`，避免 `rec_*` 外壳字段丢 offset。
- `test/tools/compare_htypes_with_wasm_dwarf.py:137`：让 oracle 解析把 `rec_*` 当 record 引用。
- `test/tools/compare_htypes_with_wasm_dwarf.py:398`：新增 `Comparator.unwrap_recursive_decl()`，把 `rec_N { struct_M field_0; }` 或 `rec_N { struct_M* field_0; }` 展开到内部 record 再做布局比较。
- `test/tools/compare_htypes_with_wasm_dwarf.py:732`：record layout 比较前先展开 recursive wrapper。

## 验证

构建：

```bash
ninja -C build bin/notdec -v
```

结果：通过。

source IR fortune：

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-level0-scc-20260724-b/out.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-level0-scc-20260724-b/fortune.htypes.txt \
  --merge-eval-dir=/tmp/notdec-fortune-level0-scc-20260724-b/eval \
  --gen-work-dir \
  --work-dir=/tmp/notdec-fortune-level0-scc-20260724-b/work
```

结果：

- 退出码 0，`elapsed=28.34 user=34.64 sys=1.21 maxrss=1926096`。
- `/tmp/notdec-fortune-level0-scc-20260724-b/work/SCCs.txt` 中 `calc_equal_probs` 已进入 `SCC0`。
- `/tmp/notdec-fortune-level0-scc-20260724-b/eval/merge-eval-summary.json`：`bad_unions=0`，`fragmented_nodes=17`，`fragmented_types=2`，`wall_ms=26445`，`nodes_created=2424`。

realworld oracle：

```bash
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
```

结果：通过，`1/1 Test #27 ... Passed 32.32 sec`。

相关输出：

- `build/test/artifacts/notdec.type_recovery.realworld.tr_level_2/fortune.o3.wasm.out.htypes`
- `build/test/artifacts/notdec.type_recovery.realworld.tr_level_2/fortune.o3.wasm.compare.txt`
- `build/test/artifacts/notdec.type_recovery.realworld.tr_level_2/fortune.o3.wasm.compare.json`

## 结论

level0 现在不再依赖 call edge 做 generator 分组，fortune 里由全局 fd 链表连接但调用边缺失的函数可以进入同一个 SCC。高 level 的分组逻辑没改，仍保留 polymorphic summary 边界。

## 评分

- 实现效果：8/10。解决了 `calc_equal_probs` 被拆出 level0 主 SCC 的问题，oracle 和 merge eval 都通过。
- 复杂度：2/10。只在已有 DSU 分组上加一个 level0 union，测试脚本只补 recursive wrapper 解析。
- 维护成本：2/10。规则集中在 `prepareSCC()`，后续若实现全局变量模块级共享，不需要推翻这次改动。
