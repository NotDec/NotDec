# 2026-05-15 bin2llvm 控制流和 PHI 小修计划

## 背景

`logs/20260515-01-Bin2llvmModulePipelinePlan.md` 阶段 5 里列了三个优先问题：

1. `CBRANCH target block is unknown`
2. PHI 没有放在 basic block 顶部
3. PHI incoming value 不支配 use

这三个问题都在 `external/NotDec-bin2llvm` 的 heritage lowering 里。模块级链路已经能把单函数失败隔离成
declaration，所以现在可以开始收敛这些函数级 lowering 问题。

## 当前观察

已有 `/tmp/notdec-heritage-sample.json` 和 `/tmp/notdec-heritage-max2.json` 都包含 `CBRANCH` 和 `MULTIEQUAL`。
当前它们能正常生成 IR，说明问题不是所有分支和 PHI 都触发，而是更复杂或更边界的 Ghidra 输出会触发。

代码现状：

1. `lowerBranch(...)` 只用 CBRANCH / BRANCH target varnode 的 `address` 去查 `BlockByStart`。
   如果 target 地址和 basic block start 字符串不一致，即使 `block.out` 已经给了 CFG 边，也会失败。
2. `lowerBlock(...)` 按 block 内 op 顺序逐条 lowering。
   如果 `MULTIEQUAL` 在 block 的 op 列表里不是最前面，就可能把 PHI 插在普通指令后面。
3. `lowerPhi(...)` 创建 PHI 时立刻读取 incoming value。
   如果 incoming 来自后面才 lower 的前驱、循环回边，或者读取未知值时创建了 `freeze poison` 指令，就可能产生 PHI 位置或 dominance 问题。

## 目标

先做小修，不改整体 lowering 模型：

1. CBRANCH / BRANCH target 地址解析失败时，使用 Ghidra 已导出的 `block.out` 边兜底。
2. 每个 block lowering 时先处理所有 `MULTIEQUAL`，保证 PHI 在 block 顶部。
3. PHI 先创建，incoming 等所有 block lowering 完成后统一补齐。
4. 对仍然无法解析的 PHI incoming，使用 typed poison constant，避免在 PHI block 顶部插入非 PHI 指令。

## 不做的事

1. 不重新设计 SSA / register state。
2. 不修所有 opcode 的真实语义。
3. 不在这一步做完整 DominatorTree 检查。
4. 不把 `LOAD/STORE` 内存模型一起改成 `inttoptr`。
5. 不跑 Bench2 大项目，因为当前环境没有 `analyzeHeadless`。

## 技术路线

1. 在 `HeritageLowerer` 里新增 pending PHI 记录。
2. `lowerPhi(...)` 只创建 `PHINode`，登记 incoming varnode id 和 predecessor block id，不立刻 `addIncoming`。
3. `lower()` 在所有 block lower 完成后调用 `finalizePendingPhis(...)`。
4. `lowerBlock(...)` 分两轮：
   - 先 lowering 当前 block 的全部 `MULTIEQUAL`
   - 再 lowering 其他普通 op 和 terminator
5. `lowerBranch(...)`：
   - target address 命中 `BlockByStart` 时保持原逻辑
   - CBRANCH target 未命中但 `block.Out.size() >= 2` 时，用 `block.Out[0]` 当 true successor，`block.Out[1]` 当 false successor
   - BRANCH target 未命中但 `block.Out.size() == 1` 时，用唯一 successor

## 风险

1. CBRANCH 兜底依赖 Ghidra `block.out` 顺序。已有样例里 `out[0]` 是 branch target，`out[1]` 是 fallthrough，但需要 Bench2 继续验证。
2. 延迟填 PHI incoming 能修 forward/backedge，但如果 Ghidra 给出的 incoming varnode 本身和 predecessor 不匹配，仍可能 verifier 失败。
3. 用 poison 兜底未知 incoming 会保住 IR 合法性，但语义仍是不完整的。

## 判断标准

1. 构造 target address 不命中但 `block.out` 正确的 CBRANCH fixture，能 lower 并通过 LLVM 22 `llvm-as`。
2. 构造 `MULTIEQUAL` 不在 block op 首位的 fixture，生成 IR 里 PHI 仍在 block 顶部，并通过 LLVM 22 `llvm-as`。
3. 构造 PHI block 在 incoming predecessor 之前 lower 的 fixture，最终 PHI incoming 使用后续前驱生成的值，而不是提前退化成 poison。
4. 旧样例 `/tmp/notdec-heritage-sample.json`、`/tmp/notdec-heritage-max2.json`、模块 smoke 继续通过。

## 性能

这次只改 `external/NotDec-bin2llvm`，不接 NotDec 主 pass pipeline，不跑 `fortune.o3.wasm.ll` 同口径性能。
改动是 per-function 线性扫描 block ops 和 PHI 列表，预期对 native lowering 时间影响很小。

## 2026-05-15 实施记录

已修复前三个问题里的可小修部分。

改动文件：

1. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 213 行 `HeritageLowerer::lower(...)`：所有 block lowering 完成后调用 `finalizePendingPhis(...)`，统一补 PHI incoming。
   - 第 233 行新增 `PendingPhi`：记录 `PHINode`、predecessor block id、incoming varnode id 和输出大小。
   - 第 1051 行 `lowerPhi(...)`：现在只创建 PHI 并登记 pending incoming，不再立刻读取 incoming value。
   - 第 1074 行 `resizeForPhiIncoming(...)`：PHI incoming 需要类型调整时，在 predecessor terminator 前插入 cast；常量直接转常量，其他未知 constant 用 poison。
   - 第 1103 行 `readPhiIncoming(...)`：pending PHI 最终补 incoming 时读取值；未知 varnode 用 typed poison，避免在 PHI block 顶部插入非 PHI 指令。
   - 第 1117 行 `finalizePendingPhis(...)`：遍历 pending PHI，并按 predecessor block 补齐 incoming。
   - 第 1223 行 `lowerBranch(...)`：CBRANCH target address 命中时保留原逻辑；未命中但 `block.Out.size() >= 2` 时使用 `out[0]` / `out[1]` 兜底。BRANCH target 未命中但只有一个 out edge 时，使用唯一 successor。
   - 第 1434 行 `lowerBlock(...)`：先处理当前 block 的全部 `MULTIEQUAL`，再处理普通 op 和 terminator，保证 PHI 在 block 顶部。

验证：

1. 编译：

```bash
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-llvm notdec-heritage-module-llvm -j4
```

2. CBRANCH 兜底 fixture：

把 `/tmp/notdec-heritage-sample.json` 里 CBRANCH target varnode `vn:34` 的 `address` 改成 `ram:deadbeef`，保留 block `out` 边。

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /tmp/notdec-heritage-cbranch-fallback.json \
  -o /tmp/notdec-heritage-cbranch-fallback.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-heritage-cbranch-fallback.ll \
  -o /tmp/notdec-heritage-cbranch-fallback.bc
```

通过，输出仍按 `out[0]` / `out[1]` 生成条件跳转。

3. PHI 顶部 fixture：

把 `bb:3` 的 op 顺序改成普通 op 在前、`MULTIEQUAL` 在后。

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /tmp/notdec-heritage-phi-top.json \
  -o /tmp/notdec-heritage-phi-top.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-heritage-phi-top.ll \
  -o /tmp/notdec-heritage-phi-top.bc
```

通过，生成 IR 中 `bb:3` 的 PHI 仍在第一条非 label 指令。

4. PHI deferred incoming fixture：

把 block 顺序改成 PHI block 先于部分 predecessor lowering。

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /tmp/notdec-heritage-phi-deferred.json \
  -o /tmp/notdec-heritage-phi-deferred.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-heritage-phi-deferred.ll \
  -o /tmp/notdec-heritage-phi-deferred.bc
```

通过，生成的 PHI incoming 使用后续 predecessor 里生成的 call 结果：

```llvm
%"vn:251" = phi i32 [ %5, %"bb:2" ], [ %4, %"bb:1" ]
```

5. 旧样例回归：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm /tmp/notdec-heritage-sample.json -o /tmp/notdec-heritage-sample.regress.ll
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm /tmp/notdec-heritage-max2.json -o /tmp/notdec-heritage-max2.regress.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-heritage-sample.regress.ll -o /tmp/notdec-heritage-sample.regress.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-heritage-max2.regress.ll -o /tmp/notdec-heritage-max2.regress.bc
```

均通过。

6. 模块工具回归：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm /tmp/notdec-heritage-module-smoke.json -o /tmp/notdec-heritage-module-smoke-body.regress.ll
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm /tmp/notdec-heritage-module-smoke.json -o /tmp/notdec-heritage-module-smoke-decls.regress.ll --declarations-only
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-heritage-module-smoke-body.regress.ll -o /tmp/notdec-heritage-module-smoke-body.regress.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-heritage-module-smoke-decls.regress.ll -o /tmp/notdec-heritage-module-smoke-decls.regress.bc
```

均通过。

未验证：

1. 没有真实 Ghidra headless / Bench2，因为当前环境没有 `analyzeHeadless`。
2. 没有跑 `fortune.o3.wasm.ll`，因为这次仍只影响 `external/NotDec-bin2llvm`。

当前方案评分：

实现效果：7/10。三个问题的常见触发形态已经能过 verifier，但还没用 Bench2 大样本确认。

复杂度：5/10。改动集中在函数级 lowering，新增 pending PHI，但没有引入支配树或全局数据流。

维护成本：5/10。`PendingPhi` 是局部机制，后续如果要做更严格 dominance 检查，可以继续接在 `finalizePendingPhis(...)`。

## 2026-05-15 补充：poison fallback warning

问题：

上一轮 unknown PHI incoming 会用 typed poison 兜底，但没有 warning。类似的 poison 兜底还有未初始化 varnode、无法在
predecessor edge 上 resize PHI incoming、无 successor 的非 void `BRANCHIND`、无 successor 的非 void fallthrough return。

改动：

1. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 35 行新增 `printPoisonFallbackWarning(...)`。
   - 第 251 行新增 `HeritageLowerer::warnPoisonFallback(...)`，同一个函数内相同原因只打印一次，避免重复读同一未知输入时刷屏。
   - 第 348 行 `read(...)`：未初始化 varnode 走 `freeze poison` 前打印 warning。
   - 第 1101 行 `resizeForPhiIncoming(...)`：PHI incoming 常量无法 resize 成目标类型时打印 warning。
   - 第 1107 行 `resizeForPhiIncoming(...)`：predecessor 没 terminator，无法在 edge 上插 cast 时打印 warning。
   - 第 1132 行 `readPhiIncoming(...)`：PHI incoming varnode 不可用，使用 typed poison 前打印 warning。
   - 第 1334 行 `lowerBranch(...)`：无 successor 的非 void `BRANCHIND` 返回 poison 前打印 warning。
   - 第 1482 行 `lowerBlock(...)`：无 successor 的非 void fallthrough 返回 poison 前打印 warning。

验证：

1. 编译：

```bash
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-llvm notdec-heritage-module-llvm -j4
```

2. PHI unknown incoming fixture：

把 `/tmp/notdec-heritage-sample.json` 的 `MULTIEQUAL` 第二个 incoming 改成不存在的 `vn:missing`。

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /tmp/notdec-heritage-phi-poison.json \
  -o /tmp/notdec-heritage-phi-poison.ll \
  2>/tmp/notdec-heritage-phi-poison.warn.log
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-heritage-phi-poison.ll \
  -o /tmp/notdec-heritage-phi-poison.bc
```

warning：

```text
Warning: heritage lowering uses poison fallback in branchy: PHI incoming varnode is unavailable: vn:missing
```

3. 模块 smoke：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-heritage-module-smoke.json \
  -o /tmp/notdec-heritage-module-smoke-warn.ll \
  2>/tmp/notdec-heritage-module-smoke.warn.log
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-heritage-module-smoke-warn.ll \
  -o /tmp/notdec-heritage-module-smoke-warn.bc
```

会对输入 varnode 的 `freeze poison` 打 warning。重复读同一个 varnode 只打印一次。

4. 正常 `branchy` 样例：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /tmp/notdec-heritage-sample.json \
  -o /tmp/notdec-heritage-sample.warn.ll \
  2>/tmp/notdec-heritage-sample.warn.log
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-heritage-sample.warn.ll \
  -o /tmp/notdec-heritage-sample.warn.bc
```

无 warning。
