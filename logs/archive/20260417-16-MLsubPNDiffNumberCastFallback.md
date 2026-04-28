# 20260417-16 MLsub PNDiff number cast fallback

## 背景

在 `resources/mlsub_builtin_summaries.json` 为 `time` 这类返回 `i64` 的函数配置
`pndiff: ret=number` 时，如果当前目标机器指针宽度是 `i32`，原来的
`applyPNDiffOverrides()` / `applyExtraConstraintPNDiffs()` 会因为目标值本身不是
pointer-sized int，直接记 warning 并忽略该条 `number` override。

当前关注 case `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll`
里就有典型路径：

```llvm
%78 = call i64 @time(i32 0)
%80 = trunc i64 %78 to i32
```

这里真正参与后续整数计算的是 `trunc` 结果 `%80`，因此需要把 `number`
override 往这一跳 cast 结果上补落。

## 本次修改

1. 在 `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:253-324`
   新增 helper：
   - `isPointerSizedNumberProjection()`
   - `collectNumberOverrideUserScanSeeds()`
   - `applyNumberOverrideToCompatibleCastUsers()`
2. 上述 helper 的规则是：
   - 当源值位宽 `>` pointer size 时，只接受 user 是 `trunc`
   - 当源值位宽 `<` pointer size 时，只接受 user 是 `zext` / `sext`
   - 只有 cast 结果位宽恰好等于 pointer size，且该结果在 PNDiff 图里是
     `PNRelated` 时，才把该结果节点设成 `Number`
3. `collectNumberOverrideUserScanSeeds()` 目前覆盖两类入口：
   - 直接的 `llvm::Value *`
   - `ReturnValue`
     对 `ReturnValue` 会枚举该函数的所有直接 `CallBase` user，把 call result
     作为后续 cast 扫描的 seed
4. 在 `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:841-900`
   的 `applyPNDiffOverrides()` 中接入该 fallback：
   - 若目标本身不是 `PNRelated`
   - 且请求的 state 是 `number`
   - 先尝试 `applyNumberOverrideToCompatibleCastUsers()`
   - 只有完全找不到可落点时才继续写 warning
5. 在 `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:903-981`
   的 `applyExtraConstraintPNDiffs()` 中同步接入同样逻辑，避免 summary override
   和 extra constraints 行为分叉。

## 涉及函数

- `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:221-250`
  `warnIgnoredPNDiffOverride()`
- `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:253-324`
  `isPointerSizedNumberProjection()`
- `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:274-298`
  `collectNumberOverrideUserScanSeeds()`
- `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:300-324`
  `applyNumberOverrideToCompatibleCastUsers()`
- `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:841-900`
  `applyPNDiffOverrides()`
- `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:903-981`
  `applyExtraConstraintPNDiffs()`

## 验证

1. 重新构建：

```bash
cmake --build ./build --target notdec-decompile
```

2. 运行聚焦 case：

```bash
env NOTDEC_DISABLE_INTERPROC=1 NOTDEC_SAT_DISABLE=1 \
  ./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/fortune.after.ll \
  --dump-htypes=/tmp/fortune.after.htypes \
  --tr-level=2 \
  --gen-work-dir \
  --work-dir=/tmp/notdec-fortune-after
```

3. 结果：
   - `/tmp/notdec-fortune-after/PNDiff.warn.txt` 中
     `time.pndiff[0]` 的 warning 已消失
   - `/tmp/notdec-fortune-after/03-pndiff-final.ll` 中保留：

```llvm
%78 = call i64 @time(i32 0); pndiff: op0=ptr
%80 = trunc i64 %78 to i32; pndiff: result=num
```

说明 `time` 的非 pointer-sized `ret=number` override 已经能落到其一跳
`trunc` 结果上。
