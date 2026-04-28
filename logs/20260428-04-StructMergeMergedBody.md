# Struct Merge Merged Body

## 背景

`type-struct-merge.md` 已经能输出候选节点和分组。为了判断每个分组真正会合成什么结构，
本次给 group sidecar 增加 `merged-body` 字段。

## 修改文件

### `external/binarysub/include/binarysub/binarysub.h`

- `349-353`
  - `StructMergeGroupInfo` 新增 `std::string mergedBody`。

### `external/binarysub/src/binarysub.cpp`

- `2156-2170`
  - `build_struct_merge_info()` 在生成每个 group 时，用 `merge_compact_types()`
    合并组内所有候选节点的 `Body`。
  - 合并后的 `CompactType` 用 `toString()` 存入 `StructMergeGroupInfo::mergedBody`。

### `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

- `1558-1587`
  - `appendDebugStructMerge()` 输出每个 group 的 `merged-body`。

## 验证

执行过：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
rm -rf /tmp/notdec-fortune-structmerge-mergedbody
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.structmerge-mergedbody.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-structmerge-mergedbody
```

结果：

- `binarysub` 测试通过。
- `notdec-decompile` 编译通过。
- fortune 用例跑通。
- `/tmp/notdec-fortune-structmerge-mergedbody/type-struct-merge.md` 中已经出现
  `merged-body:` 行。

