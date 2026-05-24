# 2026-05-09 fortune frozen extra-constraints anchor refresh

## 背景

LLVM 22 版 `fortune.o3.wasm.ll` 更新后，按 `launch.json` 里的 `fortune frozen`
配置启动，最先卡在 `NOTDEC_EXTRA_CONSTRAINTS` 的 `ir_anchor.sha256` 校验。

## 目标

先只验证一件事：把约束文件锚点改到当前 frozen IR 后，主流程会继续卡在哪里。

## 实现

### 已完成：刷新约束文件锚点

- 修改文件：
  - `test/type-recovery/realworld/support/fortune.o3.wasm.extra.json:5`
- 修改内容：
  - `ir_anchor.sha256`
  - 从 `bc4c9272ae4b8811f0ed7ffa2009e4339c5bcaf1e360908606e3406018d7e920`
  - 改成 `82fd34f662bebeb64f64f633ad56bd153e91182f18d02163de68b167cfa7a2cf`

这次没有改源码函数，只改了 frozen 约束锚点。

## 验证

命令：

```bash
env NOTDEC_EXTRA_CONSTRAINTS=test/type-recovery/realworld/support/fortune.o3.wasm.extra.json \
    NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1 \
    NOTDEC_BINARYSUB_TRACE=1 \
    ./build/bin/notdec --frozen-tr-input-ir \
    test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
    -o /tmp/fortune.c --tr-level=2 -g --work-dir=/sn640/NotDec/work_dir
```

结果：

- 已经越过 `ir_anchor.sha256` mismatch。
- 新的第一处失败变成：

```text
Error: invalid MLsub extra constraints at functions.get_tbl.actions[0].target:
inst target requires non-void instruction
```

### 已完成：临时移除两个 `PNDiff` 约束

- 修改文件：
  - `test/type-recovery/realworld/support/fortune.o3.wasm.extra.json`
- 修改内容：
  - 删除 `functions.add_file.actions[0]`
  - 删除 `functions.get_tbl.actions[0]`
  - 当前保留为 `"functions": {}`

验证命令仍然使用同一条 `fortune frozen` 命令。

结果：

- 不再出现 `get_tbl` 的 selector 校验错误。
- 主流程继续运行约 `4.89s` 后，重新回到 MLsub 主体崩溃：

```text
terminate called after throwing an instance of 'std::out_of_range'
  what():  map::at
```

对应源码位置仍是：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2924`
- 触发对象是 LLVM 22 IR 里的 `llvm.smax.i32`

## 判断

当前可以先不碰这两个 `PNDiff` selector，后续先看主流程和 `PNDiff` warning：

- `get_tbl` 的 `inst` target 现在指向了 `void` 指令，说明旧 selector 已经漂了。
- 但把这两个约束先拿掉以后，当前真正挡主线的是
  `llvm.smax.i32` 进入 `unhandledCalls` 后在 SCC 索引处 `map::at` 崩溃。
- 等这个崩溃处理完，再根据新的 `PNDiff` warning 和 `SelectableValues.txt`
  把 selector 加回来更合适。

方案评分：

- 实现效果：7/10。已经把最前面的锚点问题排掉了。
- 复杂度：2/10。只是单行数据更新。
- 维护成本：3/10。后续真正要维护的是 selector，不是 hash 本身。

## 继续实现

### 已完成：修掉 LLVM 22 `llvm.smax/umin/...` 落进 `unhandledCalls` 的崩溃

- 修改文件：
  - `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3557-3560`
  - `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3650-3680`
- 涉及函数：
  - `notdec::mlsub::MLsubRecovery::bottomUpPhase`
    - 这里原来在 `unhandledCalls` 上崩
  - `notdec::mlsub::ConstraintsGenerator::MLsubVisitor::handleIntrinsicCall`
- 修改内容：
  - 新增 `isMinMaxIntrinsic()`
  - 把 `llvm.smax/smin/umax/umin` 当成“`icmp + select` 折叠后”的整数 intrinsic
  - 直接在 `handleIntrinsicCall()` 里生成结果与两个输入的 join 约束
  - 不再让它掉进 `unhandledCalls`，也就不会在
    `MLsubGenerator.cpp:2924` 的 `Func2SCCIndex.at(...)` 处崩溃

### 已完成：修掉 LLVM 22 opaque-pointer 下全局数组声明被建成 `void *`

- 修改文件：
  - `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1999-2003`
- 涉及函数：
  - `notdec::llvm2c::SAContext::createDecls`
- 修改内容：
  - 全局变量声明类型从 `GV.getType()` 改成 `GV.getValueType()`
  - 这样 `table_0` 这类数组/结构体全局在 LLVM 22 下不会因为 opaque pointer
    被错误地建成 `void *`
  - 直接修掉了之前 `visitConstant()` 里
    `assert(Ty->isArrayType())` 的断言

### 已完成：修掉 llvm2c `switch case` 常量错误走 `ExtValuePtr` operand 校验

- 修改文件：
  - `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1416-1428`
- 涉及函数：
  - `notdec::llvm2c::CFGBuilder::visitSwitchInst`
- 修改内容：
  - `switch` 的 case label 不再走 `EB.visitValue(..., User, OpInd)`
  - 直接按 `ConstantInt` 构造 `clang::IntegerLiteral`
  - 避开 `canonicalizeExtValue()` 对“常量必须真的是 user operand”的断言

## 验证补充

### 1. `fortune frozen` `.c` 路径

命令：

```bash
env NOTDEC_EXTRA_CONSTRAINTS=test/type-recovery/realworld/support/fortune.o3.wasm.extra.json \
    NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1 \
    NOTDEC_BINARYSUB_TRACE=1 \
    ./build/bin/notdec --frozen-tr-input-ir \
    test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
    -o /tmp/fortune.c --tr-level=2 -g --work-dir=/sn640/NotDec/work_dir
```

结果：

- 退出码：`0`
- 已成功产出 `/tmp/fortune.c`
- 运行时间：`real 24.58s`、`user 23.72s`、`sys 0.84s`
- 仍有大量 `llvm2c` warning，但不再崩溃

### 2. 当前关注口径 `.ll` 性能

命令：

```bash
env NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-structmerge-hlayout-final/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-structmerge-hlayout-final
```

结果：

- 退出码：`0`
- 时间：`real 18.59s`、`user 18.15s`、`sys 0.43s`
- 对比当前参考 `real 16.33s`，慢 `2.26s`

### 3. 额外回归

命令：

```bash
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
```

结果：

- 失败
- 当前失败日志显示 suite 里的 `NOTDEC_EXTRA_CONSTRAINTS` 仍按旧锚点启动，
  先报 `ir_anchor.sha256` mismatch，还没进入这次修好的主路径

## 当前结论

这轮已经把最关键的“LLVM 22 `fortune frozen` 跑不起来”从崩溃推进到可生成
`/tmp/fortune.c`：

- `MLsub` 的 `llvm.smax.i32 -> map::at` 已修
- `llvm2c` 的全局数组 initializer 断言已修
- `llvm2c` 的 `switch case` 常量 `ExtValuePtr` 断言已修

剩下两类问题还在，但都不是这轮的启动崩溃：

- `.c` 路径仍有不少 `failed to get global at offset` /
  `Cannot find type for load inst` warning
- 当前关注口径性能从 `16.33s` 升到 `18.59s`，需要后续再看

方案评分更新：

- 实现效果：8/10。主路径已经跑通，但 warning 和性能还有尾巴。
- 复杂度：5/10。三处修复都比较局部，基本都对着 LLVM 22 语义差异。
- 维护成本：5/10。`smax` 和全局/`switch` 修法都偏正路，不是临时绕过。

## PNDiff 约束回补

### 已完成：把 `add_file/get_tbl` 的两个 `number` 约束按新 selector 加回

- 修改文件：
  - `test/type-recovery/realworld/support/fortune.o3.wasm.extra.json:5-33`
- 涉及函数：
  - `add_file`
  - `get_tbl`
- 修改内容：
  - `ir_anchor.sha256` 再刷新到当前 `work_dir/02-mlsub-input.anchor.json`
    的 `7ccaa0ce7556eb72e00233fcb4ff00882ba5c863b9f529002533a6f24cc56b2c`
  - 加回：
    - `add_file::%bb.brif_next127.i1 -> number`
    - `get_tbl::%bb.loop_entry.i6 -> number`

判断依据：

- `work_dir/PNDiff.warn.txt` 原本只剩这两条 residual
- 两条都对应明确的整数加法，不是模糊的 pointer-like 值

### 验证

命令：

```bash
env NOTDEC_EXTRA_CONSTRAINTS=test/type-recovery/realworld/support/fortune.o3.wasm.extra.json \
    NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1 \
    NOTDEC_BINARYSUB_TRACE=1 \
    ./build/bin/notdec --frozen-tr-input-ir \
    test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
    -o /tmp/fortune.c --tr-level=2 -g --work-dir=/sn640/NotDec/work_dir
```

结果：

- 退出码：`0`
- `work_dir/` 已覆盖更新
- 时间：`real 24.22s`、`user 23.48s`、`sys 0.73s`
- 新的 `work_dir/PNDiff.warn.txt` 里：

```text
# Residual PNDiff constraints after solve

No residual Add/Sub constraints with unknown state after solve.
```

剩余 warning 只还有两条旧的 `lseek`：

- `lseek.pndiff[0]`
- `lseek.pndiff[2]`

它们是 `i64` 非 PNDiff LLVM type，被显式跳过，不是这次 `fortune` 残留点。
