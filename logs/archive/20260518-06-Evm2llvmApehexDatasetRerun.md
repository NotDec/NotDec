# evm2llvm apehex 数据集重跑记录

## 背景

目标是用当前 Gigahorse PHI inline 修复后，重新跑
`/sn640/NotDecChainExp/evm2llvm_apehex_pilot` 里的 8 个合约。

这不是 repo 里的 ctest fixture，而是本地 apehex pilot 数据集：

```text
/sn640/NotDecChainExp/evm2llvm_apehex_pilot/inputs/*.hex
```

## 发现的问题和修改

1. 缺少几个 EVM opcode lowering。

   修改：

   - `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:25`
     新增 `evm_codecopy` helper 声明。
   - `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:46`
     新增 `evm_staticcall` helper 声明。
   - `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:50`
     新增 `evm_address` helper 声明。
   - `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:52`
     新增 `evm_extcodesize` helper 声明。
   - `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:217`
     `ADDRESS` 按无 operand 环境读取 lower。
   - `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:272`
     `EXTCODESIZE` 按一个 address operand lower。
   - `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:320`
     `STATICCALL` 按 6 个 operand、1 个 success def lower。
   - `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:386`
     `CODECOPY` 按 `dst/src/size` 三个 operand lower。

2. `RETURN` 后继续跟随 successor，导致 LLVM dominance 错误。

   06 暴露的问题：

   ```text
   Instruction does not dominate all uses!
     %evm.and13 = ...
     %evm.and40 = ...
   ```

   事实里有些 block 的最后一条 TAC 是 `RETURN`，但 `LocalBlockEdge.csv`
   仍然给了后继。之前 lowerer 会先生成 `evm_return`，再按 successor 生成 `br`，
   于是一个已经返回的路径被接到合流块，合流块又使用了另一条路径上的值。

   修改：

   - `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:270`
     在处理 successor 前先看 terminal TAC。
   - `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:272`
     `REVERT/THROW` 直接生成 `unreachable`。
   - `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:276`
     `RETURN/STOP` 直接生成 `ret void`，不再跟随 facts 里的 successor。

## 最终输出

最终 run 目录：

```text
/sn640/NotDecChainExp/evm2llvm_apehex_pilot/rerun-20260518-gigahorse-phi-final
```

结果文件：

```text
outputs/*.ll
outputs/*.bc
work/<id>/<id>/evm-bytecode.txt
summary.csv
logs/*.log
```

最终汇总：

```text
01_tiny_c68dbd0999      ok  ll=5684    bc=4344
02_tiny_6a9bc8606b      ok  ll=5684    bc=4344
03_small_9752b87bf2     ok  ll=13852   bc=7364
04_small_9bda9774f4     ok  ll=12851   bc=7160
05_medium_233cfe3212   ok  ll=214841  bc=81228
06_medium_fe9f436f05   ok  ll=194430  bc=76960
07_large_0994def38c    ok  ll=396415  bc=143492
08_large_9fc75dd266    ok  ll=396415  bc=143492
```

8 个合约都生成了 `.ll` 和 `.bc`，并且每个 work 目录里都有 `evm-bytecode.txt`。

## 验证

构建和 fixture：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
```

结果：

- `evm2llvm.fixture`：18/18 通过。

数据集每个合约都执行：

```bash
external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py \
  <input.hex> \
  --output <run>/outputs/<id>.ll \
  --evm2llvm build-evm2llvm/bin/evm2llvm \
  --gigahorse-dir /sn640/gigahorse-toolchain \
  --work-dir <run>/work/<id> \
  --timeout-secs 240

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  <run>/outputs/<id>.ll \
  -o <run>/outputs/<id>.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output \
  <run>/outputs/<id>.bc
```

结果：

- 8/8 生成 IR 成功。
- 8/8 `llvm-as` 成功。
- 8/8 `opt -passes=verify` 成功。

## 评估

- 实现效果：8/10。当前 apehex pilot 全部能出 verifier-clean IR。
- 复杂度：4/10。新增 opcode 都沿用已有 runtime helper 模式；terminator 修复是必要语义修复。
- 维护成本：3/10。后续遇到更多 opcode，继续按 helper 模式补即可。
- 性能：这次只改 evm2llvm，不触碰 NotDec 主 pass pipeline、类型恢复、结构体合并或 pointer analysis，
  未跑 fortune。

## 2026-05-19 继续跑 apehex train batch001

用户要求继续跑：

```text
/sn640/NotDecChainExp/apehex_evm_contracts
```

这次不是 pilot 8 个样本，而是 parquet 数据集里抽取的 train 前 40 个非空、
runtime-bytecode 去重样本。run 目录：

```text
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260518-evm2llvm-train-batch001
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch001-rerun1
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch001-rerun2
```

### 新增 opcode lowering

第一轮 40 个样本里，之前有 `SELFBALANCE`、`BALANCE`、`ORIGIN`、
`SELFDESTRUCT` 未支持。

修改：

- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:21`
  新增 `evm_balance(ptr env, i256 address) -> i256`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:53`
  新增 `evm_origin(ptr env) -> i256`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:57`
  新增 `evm_selfbalance(ptr env) -> i256`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:58`
  新增 `evm_selfdestruct(ptr env, i256 beneficiary) -> void`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:217`
  `lowerStateRead()` 把 `ORIGIN`、`SELFBALANCE` 加入无 operand 环境读取。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:237`
  `ORIGIN` lower 到 `evm_origin(env)`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:249`
  `SELFBALANCE` lower 到 `evm_selfbalance(env)`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:277`
  `BALANCE` lower 到 `evm_balance(env, address)`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:373`
  `lowerStateWrite()` 新增 `SELFDESTRUCT`，写成 runtime call。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:534`
  `SELFDESTRUCT` 接入 state-write opcode 分发。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:573`
  `BALANCE`、`ORIGIN`、`SELFBALANCE` 接入 state-read opcode 分发。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
```

结果：`evm2llvm.fixture` 18/18 通过。

### Gigahorse multi-return inline PHI 修复

`0038_19493119_aed19fef21_7d4f31efa437` 暴露了另一个 PHI 问题。
修复前：

```text
0x867S0x873_0  0x873  0x28e7cB0x867  0xadfV0x867
0x867S0x873_0  0x873  0x28e7cB0x867  0xa8fV0x867
0x867S0x873_0  0x873  0x28ea0B0x867  0xadfV0x867
0x867S0x873_0  0x873  0x28ea0B0x867  0xa8fV0x867
```

真实 CFG 是两条 return exit 边：

```text
0xa67B0x867 -> 0x28e7cB0x867 -> 0x873
0xab4B0x867 -> 0x28ea0B0x867 -> 0x873
```

两条边应该分别带一个值：

```text
0x28e7cB0x867 -> 0x873  uses 0xa8fV0x867
0x28ea0B0x867 -> 0x873  uses 0xadfV0x867
```

根因在 Gigahorse inliner：`NewPHIInfo` 生成时知道具体 `retStmt`，但关系里没有保存
`retStmt`。后面 `Out_PHIIncoming` 又按 `actualRet` 重新连接所有 return exit，
于是 return value 和 return exit 形成交叉积。

修改：

- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:422`
  给 `NewPHIInfo` 增加 `retStmt` 字段，并在注释里说明原因。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:429`
  formal return value 分支保留对应 `retStmt`。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:437`
  formal return alias 到实际参数的分支也保留对应 `retStmt`。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:445`
  更新 `InVar_OutVar(actualRet, "", defVar)` 的 `NewPHIInfo` arity。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:510`
  更新 cloned call ret use 改写处的 `NewPHIInfo` arity。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:642`
  更新 synthetic PHI statement 输出处的 `NewPHIInfo` arity。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:645`
  `Out_PHIIncoming` 按同一个 `retStmt` 连接 value 和 exit block，避免交叉积。

验证结果：

```text
/tmp/evm2llvm-0038-phi-fix/.../PHIIncoming.csv
```

修复后：

```text
PHIIncoming rows 249
duplicate groups 0
0x867S0x873_0  0x873  0x28e7cB0x867  0xa8fV0x867
0x867S0x873_0  0x873  0x28ea0B0x867  0xadfV0x867
```

并且：

```bash
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/0038_phi_fix.ll -o /tmp/0038_phi_fix.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output /tmp/0038_phi_fix.bc
```

通过。

### 旧失败样本重跑结果

对 `rerun1` 里剩余的 5 个 PHI 失败样本重跑：

```text
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch001-rerun2
```

结果：

```text
0010_19493030_17416f24c6_ea1ba3f21383  fail
0025_19493061_52bd89a2c3_3cd80c70634f  fail
0032_19493098_f0dab0bf78_d6f38c66dde5  fail
0034_19493108_6068bb046a_c17e3e727e03  ok
0038_19493119_aed19fef21_7d4f31efa437  ok
```

剩余问题：

- `0010`：6 组 duplicate，典型值是 `0x4652arg0x2` / `0x4652arg0x3`。
  这看起来是函数参数/栈位置合流问题，不是 multi-return inline 交叉积。
- `0025`：4 组 duplicate，典型值是 `0xcb5` / `0xda5_0x0`、
  `0xcaf` / `0xd94_0x0`。直接 pred 里有多个 use set 相同的 PHI，例如
  `0xd7f_0x0` 和 `0xd7f_0x6` 都合并 `0xcaf` / `0xd94_0x0`，仅靠 use set
  无法安全选择。
- `0032`：5 组 duplicate，其中既有普通合流值，也有 inline 后的嵌套 context 值。

判断：

- 这次 Gigahorse 修复覆盖的是明确的 multi-return inline value/exit 交叉积。
- 剩余 3 个失败不能用“任选一个”或“按 use set 猜一个 PHI”处理，否则会丢路径语义。
  需要继续补 Gigahorse 侧更精确的 stack slot / return value 对应关系。

### 性能

这次仍只改 evm2llvm lowering 和 Gigahorse facts 导出，不触碰 NotDec 主 pass pipeline、
类型恢复、结构体合并或 pointer analysis，未跑 fortune。

## 2026-05-19 继续扩批 batch002 / batch003

继续从 train 数据集抽去重 runtime bytecode：

```text
batch002: 第 41-80 个去重样本
batch003: 第 81-120 个去重样本
```

run 目录：

```text
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch002
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch003
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch003-rerun1
```

结果：

```text
batch002: 40/40 ok
batch003 初跑: 38/40 ok
batch003-rerun1: 2/2 ok
```

batch003 初跑失败原因：

```text
0106_19493210_5647e32328_fd78fdb37c87  unsupported opcode MCOPY
0118_19493248_593c688d00_799916048038  unsupported opcode NUMBER
```

### 新增 opcode lowering

修改：

- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:18`
  新增 `evm_mcopy(ptr mem, i256 dst, i256 src, i256 size) -> void`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:58`
  新增 `evm_number(ptr env) -> i256`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:217`
  `lowerStateRead()` 把 `NUMBER` 加入无 operand 环境读取。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:245`
  `NUMBER` lower 到 `evm_number(env)`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:395`
  `lowerStateWrite()` 新增 `MCOPY`，按 `dst/src/size` 三个 operand lower。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:558`
  `MCOPY` 接入 state-write opcode 分发。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:575`
  `NUMBER` 接入 state-read opcode 分发。
- `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py:79`
  `evm-bytecode.txt` opcode 表补 `0x5e -> MCOPY`。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
python3 -m py_compile external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
```

结果：

- `evm2llvm.fixture`：18/18 通过。
- `0106`、`0118` 重跑后均生成 `.ll` / `.bc`，并通过 `opt -passes=verify`。

### 当前数据集进展

按去重样本计算，前 120 个里：

```text
batch001: 40 个样本，已修掉 opcode 和一类 multi-return inline PHI，仍有 3 个 PHI 语义失败。
batch002: 40 个样本全部通过。
batch003: 40 个样本补 MCOPY / NUMBER 后全部通过。
```

当前剩余已知失败仍是 batch001 的：

```text
0010_19493030_17416f24c6_ea1ba3f21383
0025_19493061_52bd89a2c3_3cd80c70634f
0032_19493098_f0dab0bf78_d6f38c66dde5
```

这些仍是 PHIIncoming 语义问题，不应该在 evm2llvm 里放宽 validator。

## 2026-05-19 继续扩批 batch004

继续抽 train 去重样本第 121-160 个：

```text
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch004
```

初跑结果：

```text
40 total
35 ok
3 duplicate_phiincoming
1 unsupported_opcode_CODESIZE
1 PHIIncoming missing predecessor
```

失败样本：

```text
0124_19493266_358e0dec95_ac8a34628401  duplicate PHIIncoming predecessor
0132_19493312_4b69989842_dbb81b063fb4  unsupported opcode CODESIZE
0143_19493361_674da545a4_938fcd843b18  duplicate PHIIncoming predecessor
0146_19493376_503a732d50_138a4d77d617  PHIIncoming missing predecessor
0147_19493380_262c2cb2b2_c24931e22d66  duplicate PHIIncoming predecessor
```

`0146` 的具体报错：

```text
PHIIncoming missing predecessor for PHI 0x22f2_0x0S0x22ecS0x21faS0xb76
from 0x22ecB0x21faB0xb76
```

### 新增 CODESIZE lowering

修改：

- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:28`
  新增 `evm_codesize(ptr env) -> i256`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:208`
  `lowerStateRead()` 新增 `CODESIZE`，无 operand，lower 到 `evm_codesize(env)`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:615`
  `CODESIZE` 接入 state-read opcode 分发。

验证：

```text
/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch004-rerun1
```

`0132` 重跑后生成 `.ll` / `.bc`，并通过 `opt -passes=verify`。

回归：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
python3 -m py_compile external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
```

结果：`evm2llvm.fixture` 18/18 通过。

### 当前前 160 个样本状态

非 PHI opcode 问题都已补：

```text
SELFBALANCE / BALANCE / ORIGIN / SELFDESTRUCT
MCOPY / NUMBER
CODESIZE
```

剩余失败全是 PHI facts 语义问题：

```text
batch001: 0010, 0025, 0032
batch004: 0124, 0143, 0146, 0147
```

其中 `0146` 是缺 incoming，其他是 duplicate incoming。
