# Struct Merge Sidecar Implementation

## 背景

上一阶段只把结构体候选和分组写到 binarysub trace。这样需要打开
`NOTDEC_BINARYSUB_TRACE=1` 才能看到结果，不方便常规 workdir 对比。

本次把结构体合并分析结果作为 sidecar 从 `binarysub` 返回，并在 NotDec workdir
里写成独立文件：

```text
type-struct-merge.md
```

目前仍然不改变最终 `UType` / `HType`。

## 修改文件

### `external/binarysub/include/binarysub/binarysub.h`

修改位置：

- `340-363`
  - 新增 `StructMergeCandidateInfo`
  - 新增 `StructMergeGroupInfo`
  - 新增 `StructMergeInfo`
  - 新增 `BulkSimplifyResult`
- `394-396`
  - 新增 `TypeSimplifier::bulkSimplifyDetailed`

说明：

- `StructMergeCandidateInfo` 保存候选节点 id、root、path、顶层 label id 集合和
  body 字符串。
- `StructMergeGroupInfo` 保存贪心分组 id、共享 label id 和候选节点 id 列表。
- `BulkSimplifyResult` 同时返回原来的 `types` 和新的 `structMerge`。
- 旧的 `bulkSimplify()` API 保留。

### `external/binarysub/src/binarysub.cpp`

修改位置：

- `2087-2170`
  - `trace_struct_merge_analysis()` 改成 `build_struct_merge_info()`。
  - 现在会同时构造 `StructMergeInfo`，并在 trace 开启时继续输出原 trace。
- `2175-2228`
  - 新增 `TypeSimplifier::bulkSimplifyDetailed()`。
  - `TypeSimplifier::bulkSimplify()` 改成 wrapper，只返回 `BulkSimplifyResult::types`。

涉及函数：

- `build_struct_merge_info`
  - 输入全局 simplify 后的 compact roots。
  - 输出候选节点和贪心分组。
- `TypeSimplifier::bulkSimplifyDetailed`
  - 保留现有 global/local simplify 流程。
  - 在 global simplify 后调用 `build_struct_merge_info()`。
- `TypeSimplifier::bulkSimplify`
  - 调用 `bulkSimplifyDetailed()` 并返回 `.types`，保持旧调用者兼容。

### `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

修改位置：

- `53-56`
  - 新增 `kTypeStructMergeFile = "type-struct-merge.md"`。
- `1507-1587`
  - 新增 `appendDebugStructMerge()`。
- `3068-3069`
  - `ConstraintsGenerator::genTypes()` 改用 `Ts.bulkSimplifyDetailed()`。
- `3105-3111`
  - workdir 存在时，额外调用 `appendDebugStructMerge()`。

涉及函数：

- `appendDebugStructMerge`
  - 为每个 candidate 输出：
    - node id
    - root
    - path
    - polarity
    - label ids
    - body
  - 为每个 group 输出：
    - group id
    - shared label id
    - candidate node ids
    - 每个 node 的 root/path 摘要
- `ConstraintsGenerator::genTypes`
  - 继续用 `BulkResult.types` 做原来的 `TypeBuilder` 转换。
  - 额外把 `BulkResult.structMerge` 写入 workdir。

## 验证

执行过：

```bash
cmake --build build --target binarysub -j4
cmake --build build --target notdec-decompile -j4
./build/binarysub
rm -rf /tmp/notdec-fortune-structmerge-sidecar
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.structmerge-sidecar.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-structmerge-sidecar
```

结果：

- `binarysub` 编译通过。
- `notdec-decompile` 编译通过。
- `./build/binarysub` 全部测试通过。
- fortune 用例跑通。
- 生成了：

```text
/tmp/notdec-fortune-structmerge-sidecar/type-struct-merge.md
```

该文件当前包含：

```text
Candidates: 226
Groups: 91
```

## 后续

下一步可以基于 `StructMergeInfo` 做真实落地：

1. 先把 group 合成稳定的结构体声明名。
2. 在 `TypeBuilder` 里遇到同 group 的 candidate 时复用同一个 record decl。
3. 再考虑冲突函数是否需要支持 offset range overlap，而不是只按字段字符串 exact match。

