# 2026-04-09 PNDiff Warning 与 Final Annotated IR 落地

## 背景

继续承接：

- `logs/20260408-PNDiffWarnAndAnnoPlan.md`

本次没有再改 `PNDiff` 求解规则本身，而是把计划中的 work dir 输出真正接到当前主用的 `MLsub` 路径上。

## 本次改动

改动文件：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

新增了两类最终产物：

1. `workDir/PNDiff.warn.txt`
2. `workDir/03-pndiff-final.ll`

### 1. `PNDiff.warn.txt`

当前实现会在 `MLsubRecovery::run()` 的 `topDownPhase()` 之后统一输出。

输出来源：

- 遍历每个 SCC 对应 `ConstraintsGenerator::PG.Constraints`

文件内容按 SCC 分段，记录当前 solve 之后仍残留的 `Add/Sub` 约束，并输出：

- `kind`
- `inst`
- `result`
- `op0`
- `op1`
- `state`

其中 `state` 采用计划里收敛的短格式：

```text
result=ptr|num|unknown, op0=ptr|num|unknown, op1=ptr|num|unknown
```

如果 solve 后没有残留 `Add/Sub` 约束，也会生成文件，并写入：

```text
No residual Add/Sub constraints after solve.
```

### 2. `03-pndiff-final.ll`

新增了一个 MLsub 专用的 `AssemblyAnnotationWriter`。

当前注释策略：

- 函数头：输出 `ret/argN` 的 `ptr|num|unknown`
- 指令行：输出 `result/opN` 的 `ptr|num|unknown`

当前注释前缀固定为：

```text
; pndiff: ...
```

示例：

```llvm
%var1 = add i32 %stackPtr, -8; pndiff: result=unknown, op0=unknown, op1=num
```

## 输出时机

当前把两个文件都挂在：

- `MLsubRecovery::run()`
- `bottomUpPhase()`
- `topDownPhase()`
- 输出 warning / annotated IR

这样可以保证看到的是 solve 后、且经过当前 final type generation 阶段后的稳定 PNDiff 状态。

## 手工验证

### 1. 构建

执行：

```bash
cmake --build build --target notdec-decompile
```

结果：

- 通过

### 2. 真实样例执行

执行：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/12_Stack1.ll \
  -o /tmp/notdec-pndiff-out.ll \
  --tr-level=2 \
  --gen-work-dir \
  --work-dir=/tmp/notdec-pndiff-check
```

结果：

- 正常退出
- work dir 中出现：
  - `PNDiff.warn.txt`
  - `03-pndiff-final.ll`

其中：

- `PNDiff.warn.txt` 成功列出残留 `Add` 约束
- `03-pndiff-final.ll` 成功给 `add`、`ptrtoint`、`inttoptr`、`icmp` 等指令附加 `pndiff` 注释

## 额外验证记录

执行：

```bash
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

结果：

- 套件未全绿
- 当前看到：
  - `05_MultiOffset` 仍然 `exit=-11`
  - `11_SimpleRecursive1` 仍然 `exit=-11`
  - `09_OffsetLoop` 仍然存在 `.htypes` diff

这批失败日志里没有看到本次新增 writer 相关的断言或文件输出错误，当前更像是仓库已有的类型恢复回归现状，而不是这次 work dir 输出挂接直接引入的新崩溃。

