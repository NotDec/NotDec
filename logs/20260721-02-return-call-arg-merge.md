原始 prompt：

> 继续之前的分析从debug info里面选一个类型，比如那个fd吧。好像说最需要补的是返回值的合并是吧，直接 return operand 和 function return root；调用实参/形参在两边都是结构体指针字段切片、且字段不冲突时合并。这个补上去再看看效果

# return / call arg merge 策略

## 背景

DebugInfo eval 里 `FILEDESC*` 仍有很多 root。当前已有 load/store struct pointer merge，但函数边界上还缺两类直接合并：

- `ret` 操作数和函数返回 root。
- call actual arg 和 formal arg；但只在两边都是一层结构体字段切片，且字段区间不冲突时合并。

## 实现

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:131` 新增 late merge 候选结构，保存 return operand / function return root。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:156` 新增 `StructFieldSlice`，只记录一层字段 offset / size，不递归扫描字段类型。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:168` 新增 call actual/formal 候选结构。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:269` 在 `ConstraintsGenerator::run()` 的 late phase 里运行 return merge 和 call arg/formal merge，再运行原来的 load/store merge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2741` 新增 `tryMergeVariablesForPolicy()`，统一检查 level/size、调用 `binarysub::merge_variable_into()`、同步 V2N 和 merge eval。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2801` 新增 `applyReturnValueMergePolicy()`，方向是 return operand -> function return root。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2831` 新增一层字段切片收集，只看 variable bound 上的直接 record field。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2874` 新增字段兼容判断：字段区间不重叠，或者完全相同的 offset/size，才允许 actual/formal merge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2931` 新增 `applyCallArgStructPtrMergePolicy()`，方向是 actual arg -> formal arg。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4155` 对跨 SCC summary instantiate 后的 actual/formal 也记录候选，并在 instantiate 循环后补跑一次 call arg/formal merge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6681` 对同 SCC 直接 call 记录 actual/formal 候选。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6711` 在 `visitReturnInst()` 里记录 return merge 候选。

## 验证

构建：

```bash
cmake --build ./build --target notdec -j4
```

结果：通过。只剩本文件原有的未用变量 warning，没有新增 unused helper warning。

fortune：

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-20260721-return-call-merge-125648.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-fortune-work-20260721-return-call-merge-125648 \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648
```

结果：

- 新增策略合计命中：return value merge 15 对；call arg/formal struct pointer merge 2 对。
- 原 load/store struct pointer merge 仍为 15 对。
- `Constraint generation done! SCC count:11`。
- `bad_unions = 0`，没有新增错误合并。
- `merged_nodes` 从上一轮 28 增加到 45。
- `fragmented_nodes = 18`、`fragmented_types = 2` 仍未下降；`FILEDESC*` 仍有 17 个 root，说明 fd 这组主要还缺更强的数据流/全局链路合并，不是这两条局部策略能直接解决。

输出文件：

- `/tmp/notdec-fortune-20260721-return-call-merge-125648.log`
- `/tmp/notdec-fortune-work-20260721-return-call-merge-125648/ValueTypes.txt`
- `/tmp/notdec-fortune-work-20260721-return-call-merge-125648/ValueHTypes.txt`
- `/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648/fragmented_types.jsonl`
- `/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648/bad_unions.jsonl`

## 评分

- 实现效果：6/10。策略命中且安全，但 fortune 的 fd fragmentation 没降。
- 复杂度：6/10。新增了统一 merge helper 和一层字段兼容判断，逻辑集中在 MLsubGenerator。
- 维护成本：6/10。后续如果要扩大 call arg/formal 合并，可以继续放宽字段证据条件；当前规则比较保守。

## 后续判断

fd 剩余碎片大多是函数参数和全局变量 root，例如 `File_list`、`File_tail`、`Fortfile`、`new_fp::<ret>`、`pick_child::<ret>` 以及多个 `FILEDESC*` 参数。下一步更可能需要围绕全局读写、链表字段流、以及 pointer-to-pointer 参数（`FILEDESC **head/tail`）补合并，而不是继续放宽本次的 actual/formal 条件。

## 2026-07-22 实现记录：声明类 buffer API 标记为多态

本轮把 `read`、`write`、`fgets`、`memcpy`、`memmove`、`memset`、`strcpy`、`strncpy`、`strcat`、`snprintf` 以及对应 fortified 变体这类外部声明标成多态。原因是这些函数的声明参数会被很多无关 callsite 共享；如果还按一个 monotype declaration 节点求解，`read::arg1` 这种 formal arg 会把不同 caller buffer 错合到一起。

改动：

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:572` 增加 `DetectedPolymorphicBufferFunctions`，记录本轮自动标成多态的声明。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:629` 增加 `markBuiltinPolymorphicBufferFunctions()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:82` 增加 `PolymorphicBufferFunctions.txt` workdir 文件名。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:85` 增加内置 buffer API 名单。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3649` 在 `detectMallocWrappers()` 后、`CallGraphAnalysis` 前调用标记逻辑。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3812` 实现 `markBuiltinPolymorphicBufferFunctions()`，只处理 `isDeclaration()` 的函数，写 metadata、运行日志和 workdir 列表。

验证：

```bash
cmake --build ./build --target notdec -j4
timeout 900s ./build/bin/notdec /tmp/notdec-fortune-subsets-20260721/fd_main_chain.ll \
  -o /tmp/notdec-fortune-buffer-poly-out.ll --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-work-20260722-buffer-poly-1 \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-20260722-buffer-poly-1
```

结果：

- 构建通过，只剩已有 unused variable warning。
- `PolymorphicBufferFunctions.txt` 识别到 `__sprintf_chk`、`strcpy`、`strcat`、`strncpy`、`__snprintf_chk`、`read`、`fgets`、`fputs`。
- `SCCs.txt` 中这些声明被分到 level 1，多态边界生效。
- `read::arg1` 和 `fgets::arg0` 在 `ValueTypes.txt` / `ValueHTypes.txt` 中回到 `top:64`，不再携带 caller 的小 record。
- 之前被 `read::arg1` 污染的 `get_tbl::%tbl` 现在是普通指针，不再是 `{0: ...} & Ptr` 交叉。
- eval 结果：`bad_unions = 0`，`polluted_components = 0`，`fragmented_nodes = 15`，`wall_ms = 570887`，`peak_rss_mb = 3575`。

输出文件：

- `/tmp/notdec-fortune-work-20260722-buffer-poly-1/PolymorphicBufferFunctions.txt`
- `/tmp/notdec-fortune-work-20260722-buffer-poly-1/SCCs.txt`
- `/tmp/notdec-fortune-work-20260722-buffer-poly-1/ValueTypes.txt`
- `/tmp/notdec-fortune-work-20260722-buffer-poly-1/ValueHTypes.txt`
- `/tmp/notdec-merge-eval-fortune-20260722-buffer-poly-1/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-20260722-buffer-poly-1/fragmented_types.jsonl`
- `/tmp/notdec-merge-eval-fortune-20260722-buffer-poly-1/bad_unions.jsonl`

`{record} & Ptr` folding 观察：

- `external/binarysub/src/binarysub.cpp:647` 已经有 folding：同一个 `CompactType` 同时有非 0 record field 和 direct pointer 时，会把 direct pointer 折到 offset 0 字段。
- `external/binarysub/src/binarysub.cpp:1633` 的 `go0(TMemObject)` 会传 `pol`，这条路径能触发 folding。
- `external/binarysub/src/binarysub.cpp:1737` 的 `go1` 递归处理完字段后重新构造 `CompactType`，这里没有传 `pol`，所以可能把已经同时存在的 record + direct pointer 原样留下。
- `external/binarysub/src/binarysub.cpp:2374` 的 `coalesceCompactType()` 会把 record 和 pointer 分别转成 UType component，多 component 时负 polarity 输出为 `{record} & Ptr`。

所以当前不是完全缺 folding，而是 folding 覆盖不全。`new_fp::<ret>` 里残留的 `{...} & Ptr<...>` 更像是简化后重建 compact type 时没有传 polarity，导致已有 folding 没触发。下一步如果要修，优先讨论是否只在 `go1` 重建处补传 `pol`；这会动 `external/binarysub`，需要单独验证。

评分：

- 实现效果：7/10。修掉了 `read::arg1` 这类外部 buffer declaration 污染，错误合并仍为 0；但性能仍然偏高。
- 复杂度：2/10。只是声明级自动 metadata 和一个 workdir 列表。
- 维护成本：3/10。后续如果要扩展 API 名单，只改一个内置列表；风险是过度标记真实项目里的同名外部声明，但限定了 declaration，影响可控。

## 2026-07-22 实现记录：CompactType 构造强制传 polarity

本轮把 binarysub 的 `CompactType` 构造入口从 `std::optional<bool> foldPolarity` 改成必传 `bool pol`。这样以后类似 `go1` 重建 compact type 时漏传 polarity，会直接变成编译错误，而不是静默跳过 `{record} + Ptr` folding。

改动：

- `external/binarysub/include/binarysub/binarysub.h:426` 修改 `CompactTypeArena::make()`，第一个参数改为必传 `bool pol`。
- `external/binarysub/include/binarysub/binarysub.h:547` 修改 `make_compact_type()`，第一个参数改为必传 `bool pol`。
- `external/binarysub/include/binarysub/binarysub.h:674` 修改 `TypeSimplifier::makeCompactType()`，第一个参数改为必传 `bool pol`。
- `external/binarysub/include/binarysub/binarysub.h:733` 修改 `applySimplificationPlan()` 和 `simplifyType()`，把 root polarity 显式传入。
- `external/binarysub/src/binarysub.cpp:639` 修改 `CompactTypeArena::make()` 实现，直接用 `pol` 做 offset 0 direct pointer folding。
- `external/binarysub/src/binarysub.cpp:1492` 修改 `TypeSimplifier::makeCompactType()` 实现，把 `pol` 传到 arena。
- `external/binarysub/src/binarysub.cpp:1524` 修改 `canonicalizeType()` 里的局部 `make_compact` helper，要求传入当前 polarity。
- `external/binarysub/src/binarysub.cpp:1735` 修正 `go1` 递归重建 `CompactType` 时传入当前 `pol`。
- `external/binarysub/src/binarysub.cpp:2104` 修改 `applySimplificationPlan()`，递归重建时 record/load 继续用当前 polarity，function arg/store 用反 polarity。
- `external/binarysub/src/binarysub.cpp:2717` 和 `external/binarysub/src/binarysub.cpp:2828` 分别在 bulk/single simplify 中把 root polarity 传给 `simplifyType()`。
- `external/binarysub/src/binarysub-test.cpp:598`、`:1419`、`:1427`、`:1435` 更新测试，显式传正/负 polarity。

验证：

```bash
cmake --build ./build --target binarysub TypeBuilderTest -j4
./build/binarysub
./build/bin/TypeBuilderTest
cmake --build ./build --target notdec -j4
timeout 900s ./build/bin/notdec /tmp/notdec-fortune-subsets-20260721/fd_main_chain.ll \
  -o /tmp/notdec-fortune-pol-required-out-2.ll --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-work-20260722-pol-required-2 \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-20260722-pol-required-2
```

结果：

- `binarysub` 自测通过。
- `TypeBuilderTest` 6 个测试通过。
- `notdec` 构建通过，只剩已有 MLsub visitor unused variable warning。
- fortune fd 子集跑完，`bad_unions = 0`，`polluted_components = 0`，`fragmented_nodes = 15`。
- `read::arg1` 仍为 `top:64`，buffer declaration 污染没有回退。
- `new_fp::<ret>` 仍有 `{...} & Ptr<...>` 残留。原因不是漏传 `pol`，而是 `external/binarysub/src/binarysub.cpp:647` 的 folding 还有 `*psize <= pointer_size` 条件；该值这里是 `psize=128`，所以仍不会折进 offset 0 字段。本轮没有放宽这个语义。

输出文件：

- `/tmp/notdec-fortune-work-20260722-pol-required-2/ValueTypes.txt`
- `/tmp/notdec-fortune-work-20260722-pol-required-2/ValueHTypes.txt`
- `/tmp/notdec-merge-eval-fortune-20260722-pol-required-2/merge-eval-summary.json`

评分：

- 实现效果：6/10。接口层已经防止后续漏传 polarity，但没有消除 `new_fp` 的 128-bit direct pointer 残留。
- 复杂度：4/10。改动集中在 binarysub compact type 构造和 simplify polarity 传递。
- 维护成本：3/10。以后新增 compact type 构造点必须显式选择 polarity，调用点会更啰嗦，但更不容易静默漏 folding。
