# x64 source IR HType support

## 背景

fortune 的 source-built whole-module IR 是 `x86_64-unknown-linux-gnu`。原来
`PassManager` 把它归到 `TargetArch::Other`，`--dump-htypes` 会在构建 pipeline 时
直接退出，没进 NotDec 主项目类型推理。

## 实现

- `src/Passes/PassManager.cpp:67-80`
  - `TargetArch` 增加 `X64`。
  - `classifyTargetArch` 识别 `x86_64` / `amd64` triple。
- `src/Passes/PassManager.cpp:383-397`
  - `X64` 在 `tr-level >= 2` 时只跑 `MLsubRecoveryMain` / HType dump。
  - 不跑 Wasm pre-TR passes，避免套用 Wasm stack/memory 假设。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3253-3302`
  - HType root 收集只把变量送入 `bulkSimplifyDetailed`。
  - 非变量 root 直接 `coalesceType` 后交给 `TypeBuilder`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1720-1782`
  - debug 输出跳过非变量 root 的 origin 追踪，避免 `PolarVar` 比较断言。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3915-3930`
  - 普通 `ConstantExpr getelementptr` 按指针值保守建变量。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3950-3965`
  - vector 常量按整块数据保守建变量。
- `src/TypeRecovery/LowTy.cpp:105-117`
  - PNDiff 的 LLVM type 标记支持 vector，作为非指针值处理。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp:1-8,251-275`
  - `getSize` 支持 fixed vector，总大小为元素数量乘元素 bit-size。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
  - 移除已经被新版 binarysub 删除的 `StructMergeInfo` / `BulkSimplifyResult::structMerge`
    旧调试路径。

## 验证

```bash
cmake --build ./build --target notdec -j4
```

结果：通过。

```bash
./build/bin/notdec \
  /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.bc \
  -o /tmp/notdec-fortune-x64-htype-20260720-v6/out.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-x64-htype-20260720-v6/fortune.htypes.txt \
  -g --work-dir=/tmp/notdec-fortune-x64-htype-20260720-v6/work
```

结果：通过，输出：

- `/tmp/notdec-fortune-x64-htype-20260720-v6/fortune.htypes.txt`
- `/tmp/notdec-fortune-x64-htype-20260720-v6/work/ValueHTypes.txt`
- `/tmp/notdec-fortune-x64-htype-20260720-v6/work/ImportantHTypes.txt`

`fortune.htypes.txt` 共 2494 行。运行过程中仍有大量 GEP warning 和少量 intrinsic skip，
但没有阻塞 HType 推理。

