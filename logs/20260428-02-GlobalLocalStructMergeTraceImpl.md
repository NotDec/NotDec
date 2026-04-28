# Global/Local Struct Merge Trace Implementation

## 背景

`logs/20260428-01-GlobalLocalCoOccurrenceStructMergePlan.md` 的第一、二阶段已经实现。
本次继续向后走，但根据讨论调整了顺序：

```text
global co-occur simplify
  -> struct merge analysis
  -> local co-occur simplify
  -> coalesce UType
```

不再提前计算 local plan 来标记 `structMergeOnlyVars`。结构体合并阶段直接使用
全局 simplify 后保留下来的变量；局部 simplify 放在后面，能消掉的变量就继续消掉。

## 修改文件

### `external/binarysub/src/binarysub.cpp`

修改范围：

- `1791-2141`
  - 新增结构体合并分析相关内部 helper。
- `2143-2197`
  - 修改 `TypeSimplifier::bulkSimplify`，接入 global/local 两阶段 simplify。

涉及函数和数据结构：

- `CompactConflictResult`
  - 记录两个 `CompactType` 是否冲突，以及冲突原因。
- `StructCandidateNode`
  - 表示一个结构体候选片段。
  - 字段包括候选 id、root、path、`CompactType` body、顶层变量 labels。
- `primitive_merge_conflict()`
  - 判断两个 primitive 在当前极性下是否可以无损合并。
  - 同名同 size 不冲突。
  - 同 primitive semantic family 时用 `joinOf` / `meetOf` 判断。
  - 否则视为 primitive 冲突。
- `compact_merge_conflict_impl()`
  - 递归判断两个 `CompactType` 是否冲突。
  - 变量本身不制造冲突。
  - primitive 和 aggregate 混用视为冲突。
  - record 只递归检查同名字段；不同字段不冲突。
  - function 检查参数个数，参数按反极性递归，返回值按当前极性递归。
  - pointer 检查 access size，load 按当前极性递归，store 按反极性递归。
- `collect_struct_candidates_impl()`
  - 从全局 simplify 后的 `CompactType` 树里抽取结构体候选。
  - 只在 `pol == false` 且当前节点有非空 record、非空 vars 时生成候选。
  - labels 只取当前节点顶层 `vars`，不深入字段内部找变量。
- `trace_struct_merge_analysis()`
  - 在 `binarysub_trace_enabled()` 时输出候选和贪心分组。
  - 目前只做 trace，不改变最终类型。
- `TypeSimplifier::bulkSimplify()`
  - 原流程：

    ```text
    canonicalize all roots
    -> global analyzeOccurrences
    -> global simplifyType
    -> coalesceCompactType
    ```

  - 新流程：

    ```text
    canonicalize all roots
    -> global analyzeOccurrences
    -> global simplifyType
    -> trace_struct_merge_analysis
    -> per-root local analyzeOccurrences
    -> per-root local simplifyType
    -> per-root coalesceCompactType
    ```

## 说明

本次还没有让结构体合并结果影响 `UType` / `HType`。原因是当前更需要先确认：

1. 全局 simplify 后能抽出哪些候选结构体片段。
2. 顶层变量 labels 是否符合预期。
3. 冲突函数是否过严或过松。
4. 贪心分组是否能在 fortune 上给出合理分组。

因此目前结构体合并只写入 binarysub trace。

## 验证

执行过：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.global-local-structmerge.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-global-local-structmerge
```

结果：

- `binarysub` 编译通过。
- `./build/binarysub` 全部测试通过。
- `notdec-decompile` 编译通过。
- `fortune.o3.wasm.ll` 在 `original` 指针分析模式下跑通，输出：

```text
/tmp/fortune.global-local-structmerge.out.ll
```

## 后续

下一步建议打开 `NOTDEC_BINARYSUB_TRACE=1` 跑目标用例，查看
`binarysub-trace.log` 里的：

```text
[struct-merge:candidate]
[struct-merge:group]
```

确认分组合理后，再决定是否把分组结果输出成 sidecar，并传给 `TypeBuilder`。

