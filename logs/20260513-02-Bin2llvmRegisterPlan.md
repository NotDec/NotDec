# 2026-05-13 bin2llvm 寄存器建模计划

## 背景

`external/NotDec-bin2llvm` 当前已经能把一小段 SLEIGH P-Code 降到 LLVM IR，并支持基本块、直接分支、
条件分支、load/store 和一批整数运算。

现在最大的问题是寄存器值跨基本块会丢失。当前 lowering 在每个 LLVM basic block 开始时清空本地
`Values`，这样可以绕开 SSA dominance 错误，但读不到前驱写入的寄存器时只能生成未知值。后续要支持真实
函数，就必须把寄存器从“临时 map”改成稳定的函数状态。

## 目标

第一阶段目标是让寄存器值能正确跨基本块、分支汇合和简单循环传播。

具体效果：

1. 同一个物理寄存器的多次写入能被 LLVM SSA 正确区分。
2. 分支汇合处由 LLVM pass 自动插入 PHI。
3. x86-64 的 `RAX/EAX/AX/AL/AH` 这类重叠寄存器不会被错误当成互不相关的变量。
4. `CALL` 至少能按选定 ABI 处理参数、返回值和 caller-saved/callee-saved 寄存器。
5. 先保证语义清楚，暂不追求完整 libdecomp 级别的 prototype recovery。

## 核心方案

用 LLVM stack slot 表示“逻辑寄存器”，再跑 `mem2reg`。

做法是：

1. 对每个 canonical register 建一个 entry-block `alloca`。
2. P-Code 读寄存器时，从对应 slot `load`，再按访问片段做 `trunc/lshr`。
3. P-Code 写寄存器时，把新值写回 slot；如果是部分写，先 `load` 原值，再 mask/insert。
4. 生成完函数后跑 LLVM `mem2reg`，让 LLVM 自己插 PHI。

这样不用手写支配树、live-in、live-out 和 PHI 插入。只要 slot 地址不逃逸，且只有普通 `load/store`，
`mem2reg` 就能处理。

## 重叠寄存器

寄存器不能简单按 `(space, offset, size)` 一个 varnode 一个 slot。以 x86-64 为例：

1. `RAX` 是 `(register, 0, 8)`。
2. `EAX` 逻辑上是 `RAX` 的低 32 位，但 x86-64 写 `EAX` 会清高 32 位。
3. `AX` 是低 16 位。
4. `AL` 是低 8 位。
5. `AH` 是第 8 到 15 位。

如果把它们拆成 5 个 slot，语义会错。正确做法是把会重叠的寄存器片段归到同一个最大 slot，比如都归到
`RAX.slot`。

边界信息可以从 Ghidra/SLEIGH 的 `Translate` 对象拿：

1. `getAllRegisters()` 拿所有命名寄存器。
2. `getRegister(name)` 拿寄存器的 `(space, offset, size)`。
3. `getRegisterName(space, off, size)` 找最小包含寄存器。
4. `getExactRegisterName(space, off, size)` 判断精确命名寄存器。

构建 canonical slot 时，按同一个 address space 里的区间重叠关系做 union。每个连通区间选最大命名寄存器
作为 slot。访问时要求 varnode 必须完全落在某个 canonical slot 内；如果找不到，就退回精确 varnode slot，
并记录告警，不静默合并。

## 部分读写规则

对 canonical slot 的部分访问按 bit offset 处理。

读片段：

1. 读完整 slot：直接 load。
2. 读低位片段：load 后 trunc。
3. 读中间片段，比如 `AH`：load 后 `lshr`，再 trunc。

写片段：

1. 写完整 slot：直接 store。
2. 写低位或中间片段：load 原 slot，清掉目标 bit range，再把新值 zext/shift/or 回去。
3. 对 SLEIGH 已经表达成完整写的情况，按 P-Code 原样处理。

x86-64 的 `mov eax, imm32` 经 SLEIGH 会生成对 8 字节 `RAX` 的写入，所以不需要在 LLVM lowering 里额外识别
“写 EAX 清高位”。`AL/AH/AX` 这种才需要走部分写。

## unique 和内存

不是所有 varnode 都走寄存器 slot。

1. `unique` 空间继续用当前 SSA 临时值 map。它是 P-Code 临时值，不需要跨机器指令长期保存。
2. `const` 直接生成 LLVM 常量。
3. `ram` 继续走当前内存模型，不交给 `mem2reg`。
4. `register` 空间才走 canonical register slot。

后续如果遇到 processor context、flag、segment base 等特殊 register，也先按 register slot 处理。是否隐藏或忽略，
由后续 ABI/架构元数据决定。

## 调用约定

寄存器 slot 只能解决寄存器状态传播，不能自己知道 ABI。调用约定需要单独输入。

Ghidra 的做法是从 `.cspec` 读取 compiler spec：

1. `<input>` 定义参数寄存器和栈参数位置。
2. `<output>` 定义返回值寄存器。
3. `<unaffected>` 定义 callee-saved 寄存器。
4. `<killedbycall>` 定义 caller-saved 或明显被调用破坏的寄存器。
5. 后续 `ActionActiveParam`、`ActionReturnRecovery` 再根据数据流判断实际参数和返回值。

`bin2llvm` 第一阶段不做完整 prototype recovery。先选择一个 ABI，比如 x86-64 SysV：

1. 参数候选：`RDI, RSI, RDX, RCX, R8, R9` 和栈参数。
2. 返回值：`RAX`，必要时再加 `RDX`。
3. caller-saved：按 ABI 把相关 slot 写成未知值，返回寄存器写成 call 返回值。
4. callee-saved：保持 slot 原值。

等寄存器 slot 和 `CALL` 最小语义稳定后，再考虑解析 `.cspec`，不要一开始复刻 Ghidra 的完整参数恢复。

## 和 Ghidra Heritage 的关系

Ghidra 在 libdecomp 里用 `Heritage` 做 SSA：

1. `Varnode(space, offset, size)` 是物理存储位置。
2. 每次写形成新的 SSA varnode。
3. 控制流汇合处插 `CPUI_MULTIEQUAL`。
4. `rename()` 沿支配树把读连接到当前写。

这个思路是对的，但直接复用 libdecomp 需要 `Architecture + Funcdata + FlowInfo + Action` 整套环境。对当前
`bin2llvm` 来说太重。

短期更合适的是在 LLVM lowering 层用 slot + `mem2reg` 达到同样目的。Ghidra 的 `Heritage` 主要作为语义参考，
尤其是重叠寄存器、partial access、call effect 这些边界。

## 实施顺序

建议分四步做。

第一步：寄存器元数据。

从 SLEIGH `Translate` 导出所有命名寄存器，构建 canonical register map。先支持 `register` 空间，其他空间保持现状。

第二步：slot lowering。

把 register varnode 的 read/write 改成 slot load/store。保留 `unique` 的本地 SSA map。生成函数后跑 `mem2reg`。

第三步：重叠访问。

实现 partial read/write，覆盖 `RAX/EAX/AX/AL/AH` 和 flags 这类用例。增加小样例验证每种访问不会互相丢状态。

第四步：CALL 最小 ABI。

先硬编码 x86-64 SysV，处理参数、返回值和 clobber。后续再解析 `.cspec`，逐步支持 Windows x64 和其他 ABI。

## 风险

1. 重叠寄存器分组如果过大，可能把不该关联的隐藏寄存器合到一起。
2. 分组如果过小，`AL` 写后读 `RAX` 会错。
3. `mem2reg` 只适合标量 slot；如果 slot 地址逃逸或用了复杂 GEP，就不会提升。
4. SIMD/vector 寄存器有大量 lane 访问，第一阶段只保证整数 GPR 和 flags。
5. CALL 的 ABI 处理如果太粗，会让跨调用寄存器状态过保守，但这比错误保留 caller-saved 更安全。

## 判断标准

第一阶段完成后，应满足：

1. 条件分支两侧写同一寄存器，汇合后 LLVM IR 里能看到 PHI。
2. 循环里寄存器自增，`mem2reg` 后 loop header 有 PHI。
3. `mov al, imm; read rax` 低 8 位能体现写入，高位保持原值。
4. `mov ah, imm; read rax` 只影响 bit 8..15。
5. `mov eax, imm` 按 SLEIGH 结果写完整 `RAX`，高 32 位清零。
6. `CALL` 后 caller-saved 寄存器不被错误沿用。

## 方案评分

实现效果：8/10。能解决当前跨基本块寄存器值丢失的问题，也能覆盖最关键的重叠寄存器。

复杂度：6/10。比手写 SSA 简单，但 canonical register map 和 partial write 仍然需要小心。

维护成本：7/10。slot + `mem2reg` 是 LLVM 常见做法，后续维护主要集中在架构元数据和 ABI 表。

更好的方案可能是直接接入 libdecomp 的 `Funcdata` 和 `Heritage`，拿已经 SSA 化的 P-Code 再降 LLVM。但这会显著增加
集成成本，不适合作为当前下一步。
