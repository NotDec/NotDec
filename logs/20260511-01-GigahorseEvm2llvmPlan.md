# 2026-05-11 Gigahorse EVM 到 LLVM IR 前端计划

## 原始需求

参考 `/sn640/gigahorse-toolchain/EVM_TO_LLVM_IR_ROADMAP.md`，在 `logs/` 写一个新的 plan。
后续代码放到 `external/NotDec-evm2llvm`，用 CMake 组织编译。

## 当前判断

实现路径：

```text
external/NotDec-evm2llvm/
```

这个任务不是新写一个 EVM 反汇编器。Gigahorse 已经能生成 TAC、CFG、函数和 selector 相关 facts。
NotDec 这边第一阶段只做一个 C++ 转换器：读取 Gigahorse 输出目录里的 CSV facts，用 LLVM C++ API 生成可验证的 LLVM IR。

## 目标

第一阶段目标很明确：做出一个能独立构建、独立运行的 `evm2llvm` CLI。

它应该做到：

1. 从 Gigahorse `out/` 目录读取必要 CSV facts。
2. 构造最小 TAC/CFG/function 数据模型。
3. 用 LLVM 22 C++ API 生成 `.ll`。
4. 对 module 跑 LLVM verifier。
5. 对简单 EVM 合约输出能被 `llvm-as` 和 `opt -verify` 接受的 IR。

先不接入 NotDec 顶层默认 `all` target。等 CLI 稳定后，再加可选开关接入主工程。

## 不做的事

第一阶段不做这些：

1. 不重新解析 EVM bytecode。
2. 不直接依赖 Gigahorse 内部 Datalog 执行流程。
3. 不恢复 Solidity 级别类型、struct、mapping、ABI 语义。
4. 不尝试把 EVM memory/storage 映射成普通 LLVM native pointer。
5. 不一次性覆盖全部 opcode。
6. 不先做 native LLVM `phi`。
7. 不先做完整可执行 EVM runtime。

这些都容易把范围拉大。第一版先保证 IR 结构合法，语义边界清楚。

## 总体方案

Gigahorse 继续负责前半段：

```text
EVM bytecode
  -> Gigahorse fact generation
  -> Souffle decompiler
  -> TAC / CFG / function facts
```

NotDec 新前端负责后半段：

```text
Gigahorse out/
  -> evm2llvm
  -> contract.ll
  -> llvm-as / opt -verify
```

转换器只把 CSV facts 当作输入接口，不解析 `contract.tac` 文本，也不调用 Souffle 内部逻辑。
这样 Gigahorse 和 NotDec 前端可以分开调试。

## 目录计划

建议在现有目录下新增这些内容：

```text
external/NotDec-evm2llvm/
  CMakeLists.txt
  include/notdec-evm2llvm/
    FactLoader.h
    TacProgram.h
    LlvmLowerer.h
    EvmRuntimeDecls.h
    InstructionLowerer.h
  lib/
    CMakeLists.txt
    FactLoader.cpp
    TacProgram.cpp
    LlvmLowerer.cpp
    EvmRuntimeDecls.cpp
    InstructionLowerer.cpp
  tools/
    CMakeLists.txt
    evm2llvm.cpp
  clients/
    llvmout.py
  test/
    CMakeLists.txt
    fixtures/
```

第一版可以少建文件，但职责不要混在 `main` 里太多。否则 opcode lowering 和 facts loading 很快会难维护。

`clients/llvmout.py` 是 Gigahorse shim，只负责定位 C++ binary、传入 facts 路径、处理错误。
真正 lowering 不写在 Python 里。

## CMake 计划

`external/NotDec-evm2llvm` 作为独立 CMake 子项目。

基本要求：

1. 支持单独 `cmake -S external/NotDec-evm2llvm -B ...` 构建。
2. 默认使用 `/sn640/NotDec/llvm-22.1.0.obj`，也允许通过 CMake cache 改 LLVM 路径。
3. 优先链接 LLVM shared library，和主工程当前策略一致。
4. 生成一个库 target 和一个 CLI target。
5. 测试先独立放在子项目下，不默认挂到 NotDec 顶层全量测试。

后续接入顶层时，在根 `CMakeLists.txt` 加一个开关即可：

```text
NOTDEC_ENABLE_EVM2LLVM
```

默认值建议先设为 `OFF`，避免影响现有 wasm/type-recovery/llvm2c 链路。

## 输入 facts 范围

第一阶段只读取最小闭环需要的 facts：

```text
TAC_Op.csv
TAC_Use.csv
TAC_Def.csv
TAC_Block.csv
TAC_Block_Head.csv
TAC_Variable_Value.csv
TAC_Variable_BlockValue.csv
LocalBlockEdge.csv
IRFallthroughEdge.csv
Function.csv
IRFunctionEntry.csv
InFunction.csv
FormalArgs.csv
PublicFunction.csv
HighLevelFunctionName.csv
```

缺失 facts 时不要猜。能给默认值的只限展示名这类非语义字段。
影响 CFG、函数入口、变量定义和常量值的 facts 缺失时，应直接报错。

## IR 语义模型

EVM word 统一 lowering 成 LLVM `i256`。

布尔值在 EVM 层仍用 `i256 0/1`。
真正生成 LLVM branch 时，再把 `i256` 转成 `i1`：

```text
icmp ne i256 %cond, 0
```

EVM 状态不要伪装成普通 C 内存：

1. memory 用 `ptr %mem` handle，通过 helper 访问。
2. calldata 用 `ptr %calldata` handle，通过 helper 访问。
3. returndata 用 `ptr %returndata` handle，通过 helper 访问。
4. env 用 `ptr %env` handle，通过 helper 访问。
5. storage 和 transient storage 不作为普通参数传递，只通过 `@evm_sload/@evm_sstore/@evm_tload/@evm_tstore` 访问。

这些 helper 不要标 `readnone`、`readonly` 这类激进属性。
否则 LLVM 优化可能错误重排 storage、external call、returndata 相关操作。

## Opcode lowering 策略

第一版直接生成 LLVM 指令的 opcode：

```text
CONST
MOV
ADD
SUB
MUL
AND
OR
XOR
NOT
EQ
LT
GT
SLT
SGT
ISZERO
```

第一版通过 runtime helper lowering 的 opcode：

```text
DIV
SDIV
MOD
SMOD
EXP
ADDMOD
MULMOD
SIGNEXTEND
BYTE
SHL
SHR
SAR
SHA3
MLOAD
MSTORE
MSTORE8
SLOAD
SSTORE
TLOAD
TSTORE
CALLDATALOAD
CALLDATASIZE
CALLDATACOPY
RETURNDATASIZE
RETURNDATACOPY
RETURN
REVERT
```

除法、取模、shift 先 helper 化是必要的。
LLVM 对除零、过宽 shift 有 poison 或 undefined 行为，EVM 对这些情况有定义。

未支持 opcode 必须显式报错，不能生成看起来合法但语义错误的 IR。

## CFG 和变量策略

CFG 以 Gigahorse facts 为准。
不要重新从 `JUMP/JUMPI` 解析目标。

每个 Gigahorse block 生成一个 LLVM basic block。

terminator 规则：

1. 无 successor：根据尾部 opcode 生成 `ret`、`unreachable` 或保守 `ret void`。
2. 一个 successor：生成 unconditional branch。
3. 两个 successor：生成 conditional branch，fallthrough edge 作为 false 分支。
4. 多于两个 successor：第一版报 unsupported。

变量第一版使用 slot-based lowering：

```text
alloca -> store -> load
```

这比直接生成 LLVM `phi` 简单，也更容易先拿到合法 IR。
后续如果 Gigahorse 增加 `PHI incoming` facts，再改成 native LLVM `phi`。

## 函数策略

Gigahorse 已经有函数发现结果，转换器直接复用。

每个函数生成一个 LLVM function。
函数参数包含 EVM 状态 handle 和 Gigahorse formal args：

```text
ptr %mem
ptr %calldata
ptr %returndata
ptr %env
i256 %arg0 ...
```

`CALLPRIVATE` 后续映射成 LLVM direct call。
第一阶段可以先支持目标唯一的 private call。
目标不唯一时直接 unsupported，不先做 dispatcher。

多返回可以用 LLVM literal struct：

```text
{ i256, i256 }
```

如果第一版函数返回处理来不及，可以先把 private call 延到第二阶段，但 public 函数、block 和普通 opcode skeleton 要先跑通。

## 测试计划

第一阶段测试只看三件事：

1. CLI 能从 fixture facts 生成 `.ll`。
2. 生成的 `.ll` 能通过 `llvm-as`。
3. 生成的 bitcode 能通过 `opt -verify`。

fixture 来源可以先选 Gigahorse 里最小的 storage/calldata 合约。
不要一开始直接拿复杂真实合约做主验收。

建议的验收样例：

1. 纯算术。
2. calldata 读取。
3. memory load/store。
4. storage load/store。
5. 简单条件分支。
6. 简单 public function dispatch。

第二阶段再加差分测试：固定 calldata/storage/env，比较 EVM 执行结果和 LLVM runtime stub 结果。

## 性能影响

本计划本身不改代码，不需要跑 fortune 当前关注用例。

后续实现时，如果只是新增独立 `external/NotDec-evm2llvm`，且默认不接入 NotDec 主 pipeline，理论上不会影响
`test/type-recovery/realworld/cases/fortune.o3.wasm.ll`。

如果后续改动根 `CMakeLists.txt`、pass pipeline、LLVM 版本配置、pointer analysis 或 type recovery，
需要按当前项目要求对比 fortune 用例运行时间。
当前参考值仍是：

```text
2026-04-29
NOTDEC_POINTER_ANALYSIS_MODE=original
--tr-level=2 --frozen-tr-input-ir -g
workdir /tmp/notdec-fortune-structmerge-hlayout-final
/usr/bin/time -p real 16.33s
```

## 风险

| 风险 | 影响 | 处理 |
| --- | --- | --- |
| CSV schema 和 roadmap 假设不完全一致 | loader 写完后发现字段不够 | 先用真实 Gigahorse `out/` 固定一组 fixture，再写 loader |
| EVM defined behavior 和 LLVM UB 不一致 | IR 语义错 | 有风险的 opcode 先 helper 化 |
| PHI incoming 信息不足 | native SSA 难生成 | 第一版 slot-based lowering |
| storage/external call side effect 被优化器误动 | 分析结果错 | helper 不加 `readnone/readonly` |
| 多返回/private call 复杂 | 函数 lowering 拖慢 | 第一版只支持唯一目标和简单返回 |
| 顶层 CMake 引入后影响现有构建 | 现有链路被拖慢或失败 | 先独立子项目，顶层开关默认 OFF |
| 目录大小写不统一 | 后续引用混乱 | 优先复用现有 `external/NotDec-evm2llvm` |

## 阶段计划

### 阶段 1：独立 CLI 和合法 IR

目标：`evm2llvm --facts <out_dir> --output contract.ll` 能生成可 verify 的 IR。

范围：

1. CMake skeleton。
2. facts loader。
3. TAC/CFG/function 数据结构。
4. runtime helper declarations。
5. function/block/branch skeleton。
6. 基础算术、比较、bitwise。
7. slot-based variable lowering。

判断标准：

1. 简单 fixture 可生成 `.ll`。
2. `llvm-as` 通过。
3. `opt -verify` 通过。
4. unsupported opcode 有明确错误信息。

### 阶段 2：EVM 状态 helper

目标：memory、storage、calldata、returndata 基本操作能 lowering。

范围：

1. `MLOAD/MSTORE/MSTORE8`。
2. `SLOAD/SSTORE`。
3. `CALLDATALOAD/CALLDATASIZE/CALLDATACOPY`。
4. `RETURNDATASIZE/RETURNDATACOPY`。
5. `RETURN/REVERT/STOP`。

判断标准：

1. 简单 storage 合约能生成合法 IR。
2. helper 声明集中在 `EvmRuntimeDecls`。
3. 没有把 EVM memory/storage 错误建成 native array。

### 阶段 3：函数和 Gigahorse client

目标：能作为 Gigahorse client 生成 `contract.ll`。

范围：

1. `clients/llvmout.py`。
2. public function 命名。
3. private call/return 的简单情形。
4. 多返回的 literal struct 方案。

判断标准：

1. Gigahorse `-C clients/llvmout.py` 能跑通简单合约。
2. CLI 独立模式和 Gigahorse client 模式输出一致。
3. 函数边界和 Gigahorse TAC 基本一致。

### 阶段 4：opcode 覆盖扩展

目标：覆盖主流合约常见 opcode。

范围：

1. 复杂算术 helper。
2. `SHA3`。
3. external call/create/log。
4. env 读取。
5. code copy 和 extcode 相关 helper。

判断标准：

1. 代表性 Gigahorse tests 能批量生成合法 IR。
2. 未支持 opcode 数量持续减少。
3. 新 helper 不破坏 verifier。

### 阶段 5：SSA 和语义验证

目标：IR 更接近自然 LLVM SSA，并开始验证语义。

范围：

1. 补 Gigahorse PHI incoming facts。
2. native LLVM `phi` lowering。
3. 小型 runtime stub。
4. opcode-level 单测。
5. 简单 public function 差分测试。

判断标准：

1. 不依赖 `mem2reg` 也能输出正确 SSA。
2. 固定 calldata/storage/env 的小样例和 EVM 执行结果一致。
3. 差分失败时能定位到 opcode、block 或 helper。

## 方案评分

实现效果：8/10。
先复用 Gigahorse facts，能最快拿到可验证 LLVM IR，但第一阶段还不是完整可执行 EVM。

复杂度：7/10。
C++ + LLVM API + CSV facts loader 的复杂度可控。真正复杂的是 opcode 语义和后续 runtime，不在第一阶段一次吃掉。

维护成本：7/10。
目录、loader、lowerer、runtime declarations 分开后，后续加 opcode 比较直接。缺点是 facts schema 变化会影响 loader。

更好方案：

如果目标是做安全分析 IR，而不是执行语义，当前方案合适。
如果目标是高保真执行，应该更早设计 runtime stub 和差分测试。
如果目标是恢复 Solidity 源码结构，应该另开 Solidity 语义恢复计划，不要塞进这个前端第一阶段。
