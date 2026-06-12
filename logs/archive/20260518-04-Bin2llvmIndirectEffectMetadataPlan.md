# 2026-05-18 bin2llvm INDIRECT effect 元数据计划

## 用户原始 prompt

```text
为当前这些修改单独写一个logs/下的plan文件，涵盖修复导出逻辑以及进一步挂上meta data等步骤。然后开始修改。
```

## 背景

`INDIRECT` 是 Ghidra heritage SSA 里的 marker，形态大致是：

```text
out = INDIRECT(input0, effect)
```

`input0` 是被影响前的值或存储位置，第二个输入不是普通数据，而是指向造成影响的 P-Code op。常见 effect 是 `CALL`、`STORE`、`CALLOTHER`。

当前导出有一个问题：`INDIRECT` 的第二个输入被当成普通 const varnode 导出，例如：

```json
"inputs": ["vn:168059", "vn:216342"]
```

其中 `vn:216342` 是：

```json
{ "space": "const", "offset": "61", "size": 4, "isConstant": true }
```

这会丢掉它原本指向的 effect op，native lowering 也就无法把 `INDIRECT` 和对应 LLVM `call` / `store` 关联起来。

只保留 P-Code 字符串意义不大。需要把 effect 映射到 LLVM IR 里的真实 instruction。

## 目标

1. Ghidra JSON 导出 `INDIRECT` 的 effect op id。
2. native loader 读取这个 effect op id。
3. lowering 时记录 `HeritageOp.Id -> LLVM Instruction`。
4. lowering `INDIRECT` 时，把 effect 对应的 LLVM instruction 挂到结果指令 metadata 上。
5. 不改变 `INDIRECT` 当前 copy-like 语义，只保留可追踪关系。

## 技术路线

### 1. 修 Ghidra 导出

在 `ExportHeritagePcode.java` 和 `ExportHeritageModule.java` 的 `writeOp(...)` 中：

1. 对 `op.getMnemonic().equals("INDIRECT")` 且输入数量大于 1 的 op，检查 `op.getInput(1)`。
2. 如果它是 Ghidra 导出的 const seqTime，用 `HighFunction.getOpRef(...)` 解析出对应 `PcodeOp`。
3. 输出字段：

```json
"effectOp": "op:ram:00106783:54834"
```

如果解析失败，字段为 `null`，不要伪造。

### 2. native schema

在 `HeritageOp` 增加：

```cpp
std::optional<std::string> EffectOp;
```

loader 读取 JSON 里的 `effectOp`。

### 3. lowering 映射

在 `HeritageLowerer` 增加：

```cpp
std::unordered_map<std::string, llvm::Instruction *> OpInstructionById;
```

当 op lower 出主要副作用指令时记录：

1. `STORE` -> `StoreInst`
2. `CALL` -> `CallInst`
3. helper call -> `CallInst`

如果普通表达式生成一个 instruction，也可以记录，但当前最重要的是 effect op。

### 4. 给 INDIRECT 挂 metadata

`INDIRECT` 仍走 copy-like，但单独分出 `lowerIndirect(...)`：

1. 读取 input0，写 output。
2. 尽量找到 output 对应的新 LLVM instruction。
3. 如果 `EffectOp` 能映射到 LLVM instruction，就挂：

```llvm
!notdec.indirect !N
```

metadata 里至少包含：

1. `effect`：`ValueAsMetadata` 指向对应 LLVM instruction。
2. `effectOp`：effect op id 字符串，作为兜底。
3. `input0`：原始 input0 varnode id。

实际实现时要注意：LLVM 不允许把 `store` 或部分局部 instruction 直接塞进全局 metadata operand。更稳的表示是：

1. effect 指令自身挂 `!notdec.effect !N`。
2. `INDIRECT` 结果指令挂 `!notdec.indirect !M`。
3. `!M` 里引用同一个 `!N`。

这样仍然能在 IR 中从 `INDIRECT` metadata 反查到具体 effect 指令，只是通过共享 metadata 节点关联，而不是把 instruction 当 metadata operand 直接保存。

如果 output 没有新 instruction，比如只是复用已有 SSA value，则先不强行插入新指令，避免为了 metadata 改语义。

## 风险

1. Ghidra Java API 里 iop varnode 的解析方式可能和 C++ 不完全一致，需要用现有环境编译/运行验证。
2. 有些 `INDIRECT` 的 effect op 可能在当前 lowering 中没有生成对应 LLVM instruction，例如被忽略或是 marker op，这时只能保留字符串 metadata 或不挂 effect value。
3. 对没有新 instruction 的 `INDIRECT`，metadata 可能暂时挂不上。不能为了 metadata 引入无意义指令。
4. 这一步只建立映射，不修完整 memory SSA；不要把它理解为 `INDIRECT` 语义已经完整恢复。

## 判断标准

1. 新导出的 `vsftpd module-limit5.json` 中，`INDIRECT` op 有 `effectOp` 字段。
2. native loader 能读取旧 JSON 和新 JSON；旧 JSON 的 `effectOp` 为空但不报错。
3. 重新 lowering 新 JSON 后，生成 IR 中能看到 `!notdec.indirect` metadata。
4. metadata 能通过共享 `!notdec.effect` 节点关联到对应 LLVM `call` / `store` instruction，而不是只保存 P-Code 字符串。
5. `vsftpd module-limit5` 和 `libuv module-limit5` 仍通过 LLVM 22 `llvm-as`。

## 性能关注

这次只改 `external/NotDec-bin2llvm`，不接 NotDec 主 pass pipeline，不需要跑 `fortune.o3.wasm.ll`。

验证时记录：

1. `vsftpd module-limit5` lowering 时间。
2. `llvm-as` 时间。
3. poison fallback 数量。

## 2026-05-18 实现记录

这次实现了 `INDIRECT` effect 导出和 IR metadata 关联。没有改变 `INDIRECT` 的 copy-like lowering 语义。

### 已改文件

1. `external/NotDec-bin2llvm/ghidra_scripts/ExportHeritageModule.java`
   - 第 373 行 `writeOps(...)`：把 `HighFunction` 传给 `writeOp(...)`，让 op 导出时能解析 iop 引用。
   - 第 379 行 `writeOp(...)`：新增 JSON 字段 `effectOp`。
   - 第 405 行 `indirectEffectOpId(...)`：对 `PcodeOp.INDIRECT` 的第二个输入取 constant offset，通过 `highFunction.getOpRef(...)` 还原 effect op，再写成 `op:<addr>:<seqTime>`。
2. `external/NotDec-bin2llvm/ghidra_scripts/ExportHeritagePcode.java`
   - 第 294 行 `writeOps(...)`：同样传入 `HighFunction`。
   - 第 300 行 `writeOp(...)`：新增 `effectOp` 字段。
   - 第 326 行 `indirectEffectOpId(...)`：单函数导出路径同口径解析 effect op。
3. `external/NotDec-bin2llvm/include/notdec-bin2llvm/HeritagePcode.h`
   - 第 38 行 `HeritageOp::EffectOp`：保存可选 effect op id。
4. `external/NotDec-bin2llvm/lib/HeritagePcode.cpp`
   - 第 196 行 `readOps(...)`：读取 `effectOp`，旧 JSON 没有该字段时保持空。
5. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 269 行 `PendingIndirectMetadata`：保存 effect op 尚未 lower 时挂起的 `INDIRECT` metadata。
   - 第 439 行 `rememberOpInstruction(...)`：记录 `HeritageOp.Id -> LLVM Instruction`，并给 effect 指令挂 `!notdec.effect`。
   - 第 456 行 `attachIndirectMetadata(...)`：给 `INDIRECT` 结果指令挂 `!notdec.indirect`；如果 effect 指令还没出现，先挂起。
   - 第 489 行 `attachPendingIndirectMetadata(...)`：effect 指令出现后补挂之前的 `INDIRECT` metadata。
   - 第 501 行 `attachIndirectMetadata(Instruction*, ...)`：实际创建 metadata，`!notdec.indirect` 引用同一个 `!notdec.effect` 节点。
   - 第 548 行 `lowerIndirect(...)`：从 `lowerCopyLike(...)` 拆出 `INDIRECT`，保留 copy-like 行为，同时尝试挂 metadata。
   - 第 1292 行 `lowerStore(...)`：记录 `STORE` 对应的 LLVM `store`。
   - 第 1438 行 `lowerCall(...)` 和第 1473 行 `lowerHelperCall(...)`：记录 `CALL` / helper call 对应的 LLVM `call`。

### 验证

构建：

```bash
cmake --build /tmp/notdec-bin2llvm-build \
  --target notdec-heritage-module-llvm notdec-heritage-llvm -j4
```

Ghidra 导出：

```bash
mkdir -p /tmp/notdec-ghidra-indirect-meta

/sn640/ghidra/build/dist/ghidra_11.3.2_DEV/support/analyzeHeadless \
  /tmp/notdec-ghidra-indirect-meta NotDecVsftpdIndirectMeta \
  -import /sn640/NotDec-Exp/Bench2/rootfs/usr/sbin/vsftpd \
  -scriptPath /sn640/NotDec/external/NotDec-bin2llvm/ghidra_scripts \
  -postScript ExportHeritageModule.java \
  /tmp/vsftpd-module-limit5-indirect-meta.json --limit=5 --timeout=60
```

结果：

1. Ghidra analysis 成功，总计约 9 秒。
2. module 导出成功，5 个函数全部成功，0 个失败。
3. `FUN_00106740` 里 `INDIRECT` 总数 25554，带 `effectOp` 的也是 25554。
4. 示例：`op:ram:00106783:54694` 的 `effectOp` 是 `op:ram:00106783:61`，后者是一个 `STORE`。

native lowering：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/vsftpd-module-limit5-indirect-meta.json \
  -o /tmp/vsftpd-module-limit5-indirect-meta.ll \
  2>/tmp/vsftpd-module-limit5-indirect-meta.lower.log

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/vsftpd-module-limit5-indirect-meta.ll \
  -o /tmp/vsftpd-module-limit5-indirect-meta.bc
```

结果：

1. `llvm-as` 通过。
2. lower log 中 5 个函数 body 全部生成，0 个失败。
3. IR 中 `!notdec.effect` 出现 269 处。
4. IR 中 `!notdec.indirect` 出现 10546 处。
5. 没有 poison fallback。

旧 JSON 兼容验证：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json \
  -o /tmp/vsftpd-indirect-oldjson.ll

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/vsftpd-indirect-oldjson.ll \
  -o /tmp/vsftpd-indirect-oldjson.bc

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/module-limit5.json \
  -o /tmp/libuv-indirect-oldjson.ll

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/libuv-indirect-oldjson.ll \
  -o /tmp/libuv-indirect-oldjson.bc
```

结果：两个旧 JSON 都能正常 lower 并通过 `llvm-as`。旧 JSON 没有 `effectOp`，所以不会产生 `!notdec.indirect`，但会给已 lowered 的 call/store 挂 `!notdec.effect`。

最小 smoke：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /tmp/notdec-indirect-effect-smoke.json \
  -o /tmp/notdec-indirect-effect-smoke.ll

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-indirect-effect-smoke.ll \
  -o /tmp/notdec-indirect-effect-smoke.bc
```

结果：IR 中能看到：

```llvm
store i32 42, ptr inttoptr (i64 4096 to ptr), align 1, !notdec.effect !0
%"vn:mem.mem" = load i32, ptr inttoptr (i64 4096 to ptr), align 1, !notdec.indirect !1
```

checker：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-check \
  /tmp/vsftpd-module-limit5-indirect-meta.json
```

结果：`status: ok`，direct call 240 个，全部解析到 external。

### 性能

这次只改 bin2llvm，不接 NotDec 主 pass pipeline，不需要跑 `fortune.o3.wasm.ll`。

同口径小样例计时：

1. `vsftpd module-limit5` lowering：`real 11.78s`，`user 11.53s`，`sys 0.24s`。
2. `vsftpd module-limit5` `llvm-as`：`real 1.34s`，`user 1.31s`，`sys 0.02s`。

相比上一轮 `vsftpd module-limit5` lowering `real 11.22s`，metadata 增加后变慢约 0.56 秒，可以接受。`llvm-as` 从约 0.65 秒到 1.34 秒，主要是 IR metadata 明显增多。

### 仍需注意

1. `!notdec.indirect` 数量少于导出的 `INDIRECT` 数量，因为只有实际 lower 到 LLVM instruction 的 `INDIRECT` 才能挂 metadata。
2. 当前只是把 effect 和 `INDIRECT` 结果在 IR 里关联起来，没有实现完整 memory SSA。
3. 对旧 JSON，不会有 `!notdec.indirect`，因为旧 JSON 没有 `effectOp`。
4. effect 指令和 `INDIRECT` 之间通过共享 metadata 节点关联，不直接把 `store/call` instruction 塞进 metadata operand；这是为了通过 LLVM verifier。

实现效果：8/10。导出层已经保留 effect op，IR 里能追到对应 effect 指令。

复杂度：5/10。新增字段和映射表较小，但 pending metadata 是必要复杂度，因为真实 `vsftpd` 里很多 effect op 出现在 `INDIRECT` 后面。

维护成本：5/10。metadata 名称集中在 `notdec.effect` / `notdec.indirect`，后续做 memory SSA 时可以直接消费。
