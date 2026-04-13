# 2026-04-13 Binarysub And PNDiff Trace Log

## 背景

此前 `binarysub-trace.log` 在 `external/binarysub/src/binarysub-core.cpp` 内部直接读取
`NOTDEC_TYPE_RECOVERY_DEBUG_DIR` 并自行创建文件。

这有两个问题：

1. 旧环境变量路径已经被 workdir 机制取代
2. `binarysub` 与 `PNDiff` 没有共享同一条 trace 输出流，无法保证日志严格按真实执行顺序记录

## 本次修改

1. 新增显式开关 `NOTDEC_BINARYSUB_TRACE=1`
   - 只有设置为 `1` 时才会在 workdir 下创建 `binarysub-trace.log`
   - 未设置时不再默认生成该文件

2. 将 trace 文件生命周期上移到 `MLsubRecovery::run()`
   - 由外层根据 workdir 创建并持有 `binarysub-trace.log`
   - `external/binarysub` 不再读取 `NOTDEC_TYPE_RECOVERY_DEBUG_DIR`
   - `external/binarysub` 只接受外部注入的 `std::ostream`

3. 让 `PNDiff` 与 `binarysub` 共用同一条 trace stream
   - `ConstraintsGenerator` / `PNIGraph` 接收外层传入的 trace stream
   - 两边写入同一个文件，因此日志顺序与实际执行顺序一致

4. 为 `PNDiff` 增加关键事件 trace
   - 节点创建/删除
   - Add/Sub 约束加入
   - worklist pop
   - 规则命中、alias 推理、merge/unify
   - `setPtrOrNum` 状态变化
   - policy 导致的 number 标注
   - 约束完全求解与 ptradd reify
   - closure unify

## 验证

编译验证：

```bash
cmake --build build --target notdec-decompile -j2
```

行为验证：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/05_MultiOffset.ll \
  -o /tmp/notdec-trace-off.ll \
  --dump-htypes /tmp/notdec-trace-off.htypes \
  --tr-level=2 --gen-work-dir --work-dir /tmp/notdec-trace-off

NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/05_MultiOffset.ll \
  -o /tmp/notdec-trace-on.ll \
  --dump-htypes /tmp/notdec-trace-on.htypes \
  --tr-level=2 --gen-work-dir --work-dir /tmp/notdec-trace-on
```

结果：

- 未设置 `NOTDEC_BINARYSUB_TRACE` 时，`/tmp/notdec-trace-off/` 下没有
  `binarysub-trace.log`
- 设置 `NOTDEC_BINARYSUB_TRACE=1` 时，`/tmp/notdec-trace-on/binarysub-trace.log`
  被创建
- 日志中同时包含 `binarysub` 事件和 `PNDiff` 事件，并按执行顺序交错出现
