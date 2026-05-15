# evm2llvm apehex 真实链上样本失败修复计划与记录

## 背景

`/sn640/NotDecChainExp/apehex_evm_contracts/hex/ethereum/cleaned` 已下载完成。
为了先看 evm2llvm 对真实链上 runtime bytecode 的鲁棒性，抽了 8 个样本：

- tiny: 2 个，171 bytes
- small: 2 个，298 / 323 bytes
- medium: 2 个，3313 / 2141 bytes
- large: 2 个，11293 / 11293 bytes

样本和结果在：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/manifest.csv`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/summary_cached.csv`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/logs/*.cached.log`

Gigahorse 首次运行会编译 Souffle 程序，耗时数分钟。缓存完成后，这 8 个样本都能在
1.6s 到 4.7s 左右生成 facts，且 `client_timeouts=0`、`errors=0`。
所以这轮失败主要在 evm2llvm，不是 Gigahorse 无法处理这些 bytecode。

另一个测试工具问题：不要用 Python `subprocess.run(..., timeout=...)` 直接包 wrapper。
这会让 wrapper 超时退出，但 Gigahorse / Souffle 子进程继续留在后台。批量跑时必须用
`timeout -k` 包住整条命令，或者 runner 自己创建进程组并杀整个进程组。

## 当前失败归类

### 1. `DELEGATECALL` 未支持

影响样本：

- `01_tiny_c68dbd0999`
- `02_tiny_6a9bc8606b`
- `03_small_9752b87bf2`

现象：

```text
unsupported opcode DELEGATECALL at 0x5e
unsupported opcode DELEGATECALL at 0xdf0x0
```

判断：

`InstructionLowerer` 目前只把 `CALL` 当成外部调用 lowering，`DELEGATECALL` 会落到
unsupported。真实链上 proxy / clone / forwarding 合约很常见，所以这是优先级最高的缺口。

修复方向：

新增 runtime helper，例如 `evm_delegatecall`。按 EVM 语义，`DELEGATECALL` 没有 `value`
参数，参数顺序是 `gas, to, in_offset, in_size, out_offset, out_size`，返回 success word。
lowering 时仍把 memory / returndata / env 作为显式状态参数传给 helper。

判断标准：

这 3 个样本不再停在 unsupported opcode。修完后如果继续暴露新的 opcode，再记录为下一类。

### 2. pilot 验证脚本误用了系统 LLVM 14

影响样本：

- `04_small_9bda9774f4`

现象：

```text
declare i256 @evm_mload(ptr, i256)
warning: ptr type is only supported in -opaque-pointers mode
error: expected type
```

判断：

这不是 evm2llvm bug。当前 `build-evm2llvm/CMakeCache.txt` 已指向
`/sn640/NotDec/llvm-22.1.0.obj`，`NOTDEC_LLVM_MAJOR_VERSION=22`。但 pilot 手写脚本用了
`/usr/bin/llvm-as` 和 `/usr/bin/opt`，系统版本是 LLVM 14，所以不认识 opaque pointer `ptr`。

复查命令：

```bash
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /sn640/NotDecChainExp/evm2llvm_apehex_pilot/outputs/04_small_9bda9774f4.cached.ll \
  -o /sn640/NotDecChainExp/evm2llvm_apehex_pilot/outputs/04_small_9bda9774f4.llvm22.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /sn640/NotDecChainExp/evm2llvm_apehex_pilot/outputs/04_small_9bda9774f4.llvm22.bc \
  -o /dev/null
```

结果：`verify_rc=0`。

修复方向：

不用改 evm2llvm 的 pointer 类型。需要改 pilot runner / 后续批量脚本，强制使用
`/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as` 和
`/sn640/NotDec/llvm-22.1.0.obj/bin/opt`，不要用 PATH 上的 `/usr/bin/llvm-as`。

判断标准：

`04_small_9bda9774f4` 在 LLVM 22 工具下作为通过样本计入，不再放进 evm2llvm bug 列表。

### 3. `PHIIncoming` 边不是 `LocalBlockEdge`

影响样本：

- `05_medium_233cfe3212`
- `06_medium_fe9f436f05`

现象：

```text
PHIIncoming edge is not a LocalBlockEdge: 0x378B0x17e -> 0x38dB0x17e
PHIIncoming edge is not a LocalBlockEdge: 0x520B0x26c -> 0x530B0x26c
```

判断：

当前 validator 假设 `PHIIncoming.csv` 里的每条边都必须出现在 `LocalBlockEdge.csv`。
真实 Gigahorse facts 里存在带上下文后缀的 PHI incoming 边，但 CFG edge 集没有完全同形记录。
这可能是 Gigahorse 上下文 clone / inline 后的 facts 表达差异，不一定是坏 facts。

修复方向：

先不要直接放宽所有校验。需要对比这两个样本的：

- `PHIIncoming.csv`
- `LocalBlockEdge.csv`
- `InFunction.csv`
- `TAC_Block.csv`

确认这些 incoming 边是否能映射到同函数内真实 predecessor。若只是上下文后缀或 edge 表达不一致，
可以在加载 facts 时建立一个规范化映射，或者在 validator 里接受“同函数、同 PHI block、可达”的
incoming 边。若确实不是 CFG predecessor，则应该保留报错。

判断标准：

这两个样本不再因为合法上下文化 PHI incoming 被拒绝；同时不能让明显跨函数或不存在 block 的
PHI incoming 静默通过。

### 4. 重复 `PHIIncoming` predecessor

影响样本：

- `07_large_0994def38c`
- `08_large_9fc75dd266`

现象：

```text
duplicate PHIIncoming predecessor for PHI 0x1330_0x8 from 0x1326
```

判断：

当前 validator 按 `(PhiStmt, PredBlock)` 要求唯一。真实 facts 里可能有同一 predecessor
对同一 PHI 提供多行 incoming。需要确认这些行是完全重复，还是同一 predecessor 下有不同 var。
两种情况处理不同：

- 完全重复：加载或验证时去重即可。
- 同一 predecessor 不同 var：这是语义冲突，不能随便选一个；需要看 Gigahorse 是否用多上下文
  合并表达了多条路径，或者当前 block id 粒度丢了上下文。

修复方向：

先对这两个样本打印重复行原文。如果是完全重复，`FactLoader` 去重最简单。如果不是完全重复，
需要把 PHI incoming 的 key 扩展到上下文粒度，或者在 lowering 前拆分/选择正确 edge。

判断标准：

完全重复不会导致 evm2llvm 失败；非完全重复仍要报出带 var 的清晰错误，不能生成不确定 IR。

## 建议修复顺序

1. 先修 pilot runner 的 LLVM 工具选择。
   所有验证命令都必须用 `/sn640/NotDec/llvm-22.1.0.obj/bin` 下的工具，避免把环境里的
   LLVM 14 报错误判成 evm2llvm bug。

2. 修 `DELEGATECALL`。
   这是最常见真实链上 opcode 缺口，修完能让前 3 个样本继续往下暴露后续问题。

3. 细分并修 PHI facts。
   先写一个小脚本或临时命令，把 `05/06/07/08` 的 PHIIncoming 冲突行和相关 CFG 行打印出来。
   看清楚后再改 validator / loader，避免把错误 facts 当正常 facts 放过。

4. 改 wrapper / pilot runner 的失败摘要。
   当前 summary 里很多 message 被 `CalledProcessError` 淹没。后续批量评估需要保留第一条
   evm2llvm 真实错误，例如 unsupported opcode、PHI 校验错误、LLVM verify 错误。

## 验证标准

最小验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
ctest --test-dir build-evm2llvm -R 'evm2llvm.gigahorse.simple-storage' --output-on-failure
```

apehex 回归：

```bash
# 使用 /sn640/NotDecChainExp/evm2llvm_apehex_pilot/manifest.csv
# 对 8 个样本跑：
# Gigahorse -> evm2llvm -> llvm-as -> opt -passes=verify
```

期望：

- 不再使用 `/usr/bin/llvm-as` / `/usr/bin/opt` 做 evm2llvm 验证。
- `DELEGATECALL` 样本至少能进入下一阶段。
- PHI 相关样本要么通过，要么给出更准确、可定位的冲突原因。
- 所有命令不能留下 Gigahorse / Souffle 后台进程。

## 性能关注

这轮修复只涉及 `external/NotDec-evm2llvm`，不触碰 NotDec 主 pass pipeline、类型恢复、
结构体合并或 pointer analysis。无需跑 fortune 当前关注用例。

但 evm2llvm 自身要记录两类时间：

- 首次 Gigahorse Souffle 编译时间，不计入单样本鲁棒性。
- 缓存后的单样本端到端时间，用来观察修复后是否引入明显回退。

## 当前方案评分

- 实现效果预期：8/10。能先解决真实链上样本暴露的基础缺口。
- 复杂度：5/10。工具选择和 `DELEGATECALL` 简单，PHI facts 需要谨慎。
- 维护成本：6/10。PHI 兼容逻辑如果写得太宽会掩盖坏 facts，需要配小样本回归。

有没有更好的方案：

可以先做一个独立 fact-normalizer，把 Gigahorse facts 统一成 evm2llvm 需要的 SSA/CFG 形式。
但现在样本还少，直接在 loader / validator 里小步修更合适。等 PHI 兼容规则稳定后，再考虑拆成
单独 normalizer。

## 实现记录

本轮只修简单问题，不改 PHI 语义，也不退回旧的 slot + mem2reg 思路。

### 已完成：LLVM 22 工具约束

修改文件：

- `AGENTS.md:59`，新增 evm2llvm PHI 修复约束：不能回退 slot + mem2reg，复杂 PHI 问题先记录。
- `AGENTS.md:186`，把当前 LLVM 版本说明改为 `llvm-22.1.0.obj`，并要求验证 IR 时直接使用
  `llvm-22.1.0.obj/bin/llvm-as` 和 `llvm-22.1.0.obj/bin/opt`。
- `logs/20260514-01-Evm2llvmRealBytecodeTests.md:156`，把 `04_small` 从 LLVM IR bug 改为
  LLVM 14 工具误用。

复查结果：

- `04_small_9bda9774f4.cached.ll` 使用 LLVM 22 的 `llvm-as` 和 `opt -passes=verify` 通过。
- 这说明原来的 opaque pointer 报错来自 `/usr/bin/llvm-as`，不是 evm2llvm 输出错误。

### 已完成：补齐简单 opcode lowering

修改文件：

- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:25`，新增
  `evm_returndatasize(ptr) -> i256`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:26`，新增
  `evm_returndatacopy(ptr, ptr, i256, i256, i256) -> void`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:41`，新增
  `evm_delegatecall(ptr, ptr, ptr, i256, i256, i256, i256, i256, i256) -> i256`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:208`，在
  `InstructionLowerer::lowerStateRead` 中支持 `RETURNDATASIZE`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:290`，在
  `InstructionLowerer::lowerStateWrite` 中支持 `DELEGATECALL`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:359`，在
  `InstructionLowerer::lowerStateWrite` 中支持 `RETURNDATACOPY`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:453` 和 `:495`，把这些 opcode 接入
  `InstructionLowerer::lower` 的分发。

效果：

- `01_tiny_c68dbd0999`：原先停在 `unsupported opcode DELEGATECALL`，现在 evm2llvm 生成 IR，
  LLVM 22 `llvm-as` 和 `opt -passes=verify` 通过。
- `02_tiny_6a9bc8606b`：同上，已通过。
- `03_small_9752b87bf2`：不再停在 `DELEGATECALL`，但 LLVM verifier 报
  `Entry block to function must not have predecessors! label %bb._0x5f0xf88`。
  这是 CFG / 函数入口切分问题，不是本轮简单 opcode 缺口。

### 暂不修：PHI 和 CFG 深层问题

保留问题：

- `03_small_9752b87bf2`：entry block 有 predecessor。需要检查 Gigahorse facts 中函数入口、
  `LocalBlockEdge`、`InFunction` 的关系。
- `05_medium_233cfe3212`、`06_medium_fe9f436f05`：
  `PHIIncoming edge is not a LocalBlockEdge`。
- `07_large_0994def38c`、`08_large_9fc75dd266`：
  duplicate `PHIIncoming` predecessor，且之前观察到可能是同一 predecessor 不同 var。

判断：

这些都可能涉及 Gigahorse 的上下文块、函数边界或 SSA incoming 语义。当前不做宽松绕过，
也不把 PHI 降回 slot + mem2reg。

### 验证命令与结果

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
ctest --test-dir build-evm2llvm -R 'evm2llvm.gigahorse.simple-storage' --output-on-failure
```

结果：

- `evm2llvm` 构建通过。
- `evm2llvm.fixture`：15/15 通过。
- `evm2llvm.gigahorse.simple-storage`：3/3 通过。

apehex 手动复查：

```bash
exp=/sn640/NotDecChainExp/evm2llvm_apehex_pilot
for sid in 01_tiny_c68dbd0999 02_tiny_6a9bc8606b 03_small_9752b87bf2; do
  facts="$exp/work/${sid}.cached/$sid/out"
  ll="$exp/outputs/${sid}.simplefix.ll"
  bc="$exp/outputs/${sid}.simplefix.bc"
  build-evm2llvm/bin/evm2llvm --facts "$facts" --output "$ll"
  /sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as "$ll" -o "$bc"
  /sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify "$bc" -o /dev/null
done
```

结果：

- `01_tiny_c68dbd0999` 通过。
- `02_tiny_6a9bc8606b` 通过。
- `03_small_9752b87bf2` 失败在 LLVM verifier：entry block 有 predecessor。

性能：

- 本轮只改 `external/NotDec-evm2llvm` opcode lowering 和文档，不触碰 NotDec 主 pass pipeline、
  类型恢复、结构体合并或 pointer analysis，所以未跑 fortune 时间对比。

### 实际方案评分

- 实现效果：6/10。解决了最简单、确定的 opcode 缺口，并纠正了 LLVM 14 误判；8 个样本仍有
  1 个 CFG 问题和 4 个 PHI 问题。
- 复杂度：3/10。新增 helper 和 lowering 分发，代码局部。
- 维护成本：3/10。runtime helper 语义直接对应 EVM opcode，后续主要风险是 helper 名称和
  状态参数约定需要保持一致。

更好的方案：

下一步应单独做 PHI / CFG facts 调研日志，先打印冲突边、函数归属和原始 incoming 行，再决定是
修 Gigahorse 导出、fact loader，还是 LLVM CFG 构造。不要先改 validator 放宽规则。
