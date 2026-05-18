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
