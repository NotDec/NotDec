# evm2llvm apehex PHI facts 调研记录

## 背景

这次只调研，不改 PHI lowering。当前约束仍是：不能退回旧的 slot + mem2reg 方案。
如果 facts 语义不清楚，先修 Gigahorse 导出或补清楚语义，再考虑 evm2llvm 侧如何接受。

样本来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/manifest.csv`
- facts 在 `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/work/<sid>.cached/<sid>/out`

涉及样本：

- `05_medium_233cfe3212`
- `06_medium_fe9f436f05`
- `07_large_0994def38c`
- `08_large_9fc75dd266`

`07_large_0994def38c` 和 `08_large_9fc75dd266` 的输入 bytecode 完全相同，
`PHIIncoming.csv` 也完全相同，所以它们是同一个问题的重复样本。

## 结论

当前不是 evm2llvm 简单 validator 过严。`PHIIncoming.csv` 里确实出现了 LLVM native PHI
不能直接表达的情况：

1. `05/06`：`PHIIncoming` 的 predecessor 不是 PHI block 的直接 CFG predecessor。
   这些 predecessor 到 PHI block 中间插入了 inlined private call 的块。
2. `05/07/08`：同一个 `(phiStmt, predBlock)` 下有多个不同 incoming var。
   这不是重复行，不能简单去重。
3. 部分后继 PHI 已经有前面 PHI 的结果可用，但导出的 incoming 仍然用更早的 raw var，
   导致同一条直接边上出现两个值。

所以当前不适合在 evm2llvm 里放宽 validator。更合理的方向是修 Gigahorse 的
`PHIIncoming` 导出和 inliner 转换。

## 05：inlined path 让 incoming pred 失效

统计：

- `PHIIncoming.csv`：68 行
- `LocalBlockEdge.csv`：354 行
- exact edge 不存在的 `PHIIncoming`：6 行
- duplicate `(phiStmt, predBlock)`：3 组，都是两个不同 var

代表问题：

```text
PHI 0x38d_0x1S0x17e
block 0x38dB0x17e
pred  0x378B0x17e
vars  0x35dV0x17e, 0x3c3V0x17e
```

`LocalBlockEdge(0x378B0x17e, 0x38dB0x17e)` 不存在。真实路径是：

```text
0x378B0x17e
-> 0xbdfB0x378B0x17e
-> 0xbefB0x378B0x17e
-> 0xaefB0xbefB0x378B0x17e
-> 0xb05B0xbefB0x378B0x17e
-> 0xbf8B0x378B0x17e
-> 0x38dB0x17e
```

另外两组同构：

```text
0x3eaB0x1a2 -> ... -> 0x3ffB0x1a2
0x456B0x1b5 -> ... -> 0x46bB0x1b5
```

判断：

这些 PHI incoming 保留了 inline 前的 callsite predecessor。inliner 插入 callee 路径后，
PHI block 的直接 predecessor 已经变成 return exit block，例如 `0xbf8B...`。

这不是 evm2llvm 可以直接映射成 `addIncoming(value, predBB)` 的事实。LLVM PHI 要求 incoming
block 必须是当前 block 的直接 predecessor。

## 06：单条 forwarded PHI incoming 指向 inline 前 predecessor

统计：

- `PHIIncoming.csv`：17 行
- `LocalBlockEdge.csv`：302 行
- exact edge 不存在的 `PHIIncoming`：1 行
- duplicate `(phiStmt, predBlock)`：0 组

问题行：

```text
0x530_0x1S0x26c  0x530B0x26c  0x520B0x26c  0x520_0x1V0x26c
```

真实路径：

```text
0x520B0x26c
-> 0x69dB0x520B0x26c
-> 0x6a6B0x520B0x26c
-> 0xc06aB0x520B0x26c
-> 0x530B0x26c
```

判断：

这里 var 是 `0x520_0x1V0x26c`，看起来是 predecessor block 自己的 PHI 结果。
但 predecessor 仍然不是直接 CFG predecessor。需要由 Gigahorse inliner 把 incoming edge
重写到实际 return exit block，或者导出一层“edge value forwarding”关系给 evm2llvm。

## 07/08：同一直接 predecessor 上有两个不同 var

统计：

- `PHIIncoming.csv`：300 行
- `LocalBlockEdge.csv`：539 行
- exact edge 不存在的 `PHIIncoming`：0 行
- duplicate `(phiStmt, predBlock)`：23 组

代表问题：

```text
PHI 0xc09_0x2, block 0xc09, pred 0x711ae
  var 0xb53  CONST 0x0, def block 0xb53
  var 0xb6e  SUB,       def block 0xb59

PHI 0xc09_0x3, block 0xc09, pred 0x711ae
  var 0xb19  CONST 0x0, def block 0xb19
  var 0xb34  SUB,       def block 0xb1f
```

这不是重复行，两行 var 不同。`LocalBlockEdge(0x711ae, 0xc09)` 是真实直接边。

更早的 block 里其实已经有合法 PHI：

```text
Block 0xb6f:
  0xb6f_0x0 = PHI(0xb53 from 0xb53, 0xb6e from 0xb59)
  0xb6f_0x2 = PHI(0xb53_0x1 from 0xb53, 0xb59_0x1 from 0xb59)

Block 0xb80:
  0xb80_0x1 incoming from 0xb6f uses 0xb6f_0x0
  0xb80_0x2 incoming from 0xb6f uses 0xb6f_0x2

Block 0xbd5:
  0xbd5_0x0 incoming from 0xb80 uses 0xb80_0x1
  0xbd5_0x1 incoming from 0xb80 uses 0xb80_0x2
```

但后面 inlined call 返回点又退回 raw vars：

```text
Block 0x50f8f:
  0x50f8f_0x3 incoming from 0xbd5 uses 0xb53
  0x50f8f_0x3 incoming from 0xbd5 uses 0xb6e

Block 0x711ae:
  0x711ae_0x4 incoming from 0x50f8f uses 0xb53
  0x711ae_0x4 incoming from 0x50f8f uses 0xb6e

Block 0xc09:
  0xc09_0x2 incoming from 0x711ae uses 0xb53
  0xc09_0x2 incoming from 0x711ae uses 0xb6e
```

判断：

这类问题看起来来自 Gigahorse function inliner 新引入 PHI 的规则。相关位置：

- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:422`
  的 `NewPHIInfo`
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:536`
  的 `Out_PHIIncoming`

`NewPHIInfo` 用同一个 `stmtId` / `phiBlock` / `outExitBlock` 收集多个 `useVar`，所以会生成：

```text
same phiStmt, same phiBlock, same predBlock, different var
```

这对旧 edge-store 方案可以靠顺序覆盖勉强继续，但对 native LLVM PHI 是不合法语义。
native PHI 必须在每个 direct predecessor 上只有一个 incoming value。

## 03 的 CFG entry 问题

`03_small_9752b87bf2` 当前失败：

```text
Entry block to function must not have predecessors!
label %bb._0x5f0xf88
```

`IRFunctionEntry.csv` 里入口是：

```text
0x0
0xf88
0x31
```

这些入口本身没有同函数 predecessor。当前 verifier 报的 `%bb._0x5f0xf88` 不是
`IRFunctionEntry` 入口，更像是 evm2llvm 创建 LLVM function 时把 block 插入顺序弄成了
非入口块在前。这个问题和上面的 PHIIncoming 冲突不是同一个问题，后续应单独查
`LlvmLowerer` 创建 basic block 的顺序。

## 建议修复方向

1. 先修 Gigahorse inliner 的 `PHIIncoming` 转换：
   inline 后如果原 incoming pred 不再直连 PHI block，应改成实际 return exit block。
2. 对 `NewPHIInfo` 生成的新 PHI，必须保证每个 direct predecessor 只有一个 value。
   如果一个 pred 下有多个候选 raw var，应优先使用前面已经存在的 PHI def，而不是重新导出 raw var。
3. evm2llvm validator 当前应该继续拒绝：
   - incoming pred 不是 direct `LocalBlockEdge`
   - 同一 `(phiStmt, predBlock)` 下多个不同 var
4. 不建议在 evm2llvm 里做“任选一个 var”或按文件顺序覆盖。这会让 IR 通过 verifier，但语义不可信。

## 判断标准

修 Gigahorse 后，至少满足：

- 所有 `PHIIncoming(phi, block, pred, var)` 都有 `LocalBlockEdge(pred, block)`。
- 同一 `(phi, pred)` 只有一个 `var`。
- 每个 PHI 的 incoming predecessor 集合等于同函数内 CFG predecessor 集合。
- `05/06/07/08` 通过 evm2llvm emit，且 LLVM 22 `llvm-as` / `opt -passes=verify` 通过。

## 性能

本轮只读 facts 和源码，不改代码，不跑 fortune。

## 2026-05-15 Gigahorse 侧尝试修复记录

修改位置：

- `/sn640/gigahorse-toolchain/gigahorse.py:11`，引入 `deque`，用于同函数 CFG 可达性检查。
- `/sn640/gigahorse-toolchain/gigahorse.py:220`，扩展 `repair_phi_incoming`。

修复原则：

- 仍然只修可证明的 facts，不在 evm2llvm 里放宽 PHI validator。
- 如果 `PHIIncoming(pred, block)` 不是 direct `LocalBlockEdge`，但 `pred` 在同函数内唯一可达
  `block` 的某个 direct predecessor，则把 incoming pred 重写为这个 direct predecessor。
- 如果同一 `(phiStmt, block, predBlock)` 有多个 var，并且 `predBlock` 内有唯一一个 PHI 的
  use set 正好等于这些 var，则把 incoming var 改成该 PHI 的 def。
- 其他冲突保持原样，让 evm2llvm 继续报错，不猜。

验证命令：

```bash
python3 -m py_compile /sn640/gigahorse-toolchain/gigahorse.py
```

结果：通过。

对 05-08 的缓存 facts 拷贝到 `/tmp/gigahorse-phi-repair-source-test`，调用
`repair_phi_incoming` 后再跑 evm2llvm：

```bash
build-evm2llvm/bin/evm2llvm --facts <repaired-facts> --output <sid>.ll
```

结果：

- `05_medium_233cfe3212`：`PHIIncoming.csv` 有改动，但仍失败在
  `duplicate PHIIncoming predecessor for PHI 0x38d_0x1S0x17e from 0xbf8B0x378B0x17e`。
  原因是三组 duplicate 没有唯一可证明的 predecessor PHI def 可以替代。
- `06_medium_fe9f436f05`：PHI 校验通过，进入下一缺口：
  `unsupported opcode CODECOPY at 0x28c`。
- `07_large_0994def38c`：`PHIIncoming.csv` 从 300 行降到 274 行，PHI 校验通过，
  进入下一缺口：`unsupported opcode EXTCODESIZE at 0x99e`。
- `08_large_9fc75dd266`：同 07，因为输入 bytecode 和 `PHIIncoming.csv` 相同。

判断：

这次修复能解决 `06/07/08` 暴露的 PHI facts 问题；`05` 还需要继续修 Gigahorse
inliner 对 callsite raw vars 的导出语义。当前不应在 evm2llvm 里绕过 `05`。

性能：

只改 Gigahorse facts postprocess，不触碰 NotDec 主 pass pipeline、类型恢复、结构体合并或
pointer analysis，未跑 fortune。

## 2026-05-15 duplicate PHIIncoming 细查

先看数量：

- `05_medium_233cfe3212`：3 组 duplicate `(phiStmt, predBlock)`，没有完全重复行，都是不同 var。
- `06_medium_fe9f436f05`：无 duplicate。
- `07_large_0994def38c`：23 组 duplicate，没有完全重复行，都是不同 var。
- `08_large_9fc75dd266`：同 07。

所以这里的 duplicate 不是 CSV 重复写行，而是同一个 LLVM PHI incoming edge 上出现多个候选值。

### 07/08：pred block 已有等价 PHI

代表行：

```text
PHI 0xc09_0x2, block 0xc09, pred 0x711ae
  var 0xb53
  var 0xb6e
```

`pred` block `0x711ae` 内已有：

```text
0x711ae_0x4 = PHI(0xb53, 0xb6e)
```

同类还有：

```text
0xc09_0x3  pred 0x711ae  vars 0xb19, 0xb34  -> 0x711ae_0x5
0xc21_0x3  pred 0x711d9  vars 0xb53, 0xb6e  -> 0x711d9_0x5
0xc4d_0x5  pred 0xc21    vars 0xb53, 0xb6e  -> 0xc21_0x3
0xce4_0x0  pred 0xcd6    vars 0xb53, 0xb6e  -> 0xcd6_0x2
```

判断：

这是 `PHIOutputForwarded` 没有把 predecessor block 的 PHI result 当成 edge 上的唯一值，
而是把 raw value set 继续展开到了后继 PHI。当前 `gigahorse.py` 的 postprocess 修复
正是处理这种情况：如果 `predBlock` 内唯一 PHI 的 use set 等于 duplicate var set，
就用这个 PHI def 替换多行 raw vars。

验证：

- `07/08` 修复后 `PHIIncoming.csv` 从 300 行变成 274 行。
- evm2llvm 不再报 PHI duplicate，继续暴露 `unsupported opcode EXTCODESIZE at 0x99e`。

### 05：循环 PHI 的初始值和回边值被压到同一条边

代表行：

```text
PHI 0x38d_0x1S0x17e, block 0x38dB0x17e, pred 0x378B0x17e
  var 0x35dV0x17e  # CONST 0
  var 0x3c3V0x17e  # ADD 0x38d_0x1V0x17e + 1
```

这是标准循环变量：

```text
0x35e_0x0S0x17e:
  from 0x35cB0x17e uses 0x35dV0x17e
  from 0x38dB0x17e uses 0x3c3V0x17e

0x367_0x0S0x17e:
  from 0x35eB0x17e uses 0x35e_0x0V0x17e

0x378_0x0S0x17e:
  from 0x367B0x17e uses 0x367_0x0V0x17e
```

但 duplicate 行直接把 `0x35dV0x17e` 和 `0x3c3V0x17e` 都挂到了
`0x378B0x17e -> 0x38dB0x17e`。inline 后真实 CFG 是：

```text
0x378B0x17e
-> 0xbdfB0x378B0x17e
-> 0xbefB0x378B0x17e
-> 0xaefB0xbefB0x378B0x17e
-> 0xb05B0xbefB0x378B0x17e
-> 0xbf8B0x378B0x17e
-> 0x38dB0x17e
```

所以当前源码里的 postprocess 会先把 pred 重写成 `0xbf8B0x378B0x17e`，但仍有两个 var。
这里不能直接任选 `0x35d` 或 `0x3c3`。

进一步看 `0x378B0x17e` 内有两个同 use-set PHI：

```text
0x378_0x0S0x17e -> def 0x378_0x0V0x17e，被 MUL 使用
0x378_0x3S0x17e -> def 0x378_0x3V0x17e，未被普通语句使用
```

同构的另外两组：

```text
0x3ea_0x0V0x1a2 被使用，0x3ea_0x3V0x1a2 未使用
0x456_0x0V0x1b5 被使用，0x456_0x3V0x1b5 未使用
```

临时验证：

在 `/tmp/gigahorse-phi-05-originpred-usedphi` 手工尝试规则：

- duplicate var set 等于原始 pred block 内某些 PHI 的 use set；
- 多个候选 PHI 里只有一个 def 被非 PHI 语句使用；
- 用这个被使用的 PHI def 替换 duplicate raw vars。

替换结果：

```text
0x38d_0x1S0x17e <- 0x378_0x0V0x17e
0x3ff_0x1S0x1a2 <- 0x3ea_0x0V0x1a2
0x46b_0x1S0x1b5 <- 0x456_0x0V0x1b5
```

验证通过：

```text
evm2llvm rc=0
llvm-as rc=0
opt -passes=verify rc=0
```

判断：

这个规则比当前已经写进 `gigahorse.py` 的修复更激进一点。它依赖“同 use-set PHI 中只有一个
真正被普通语句使用”这个经验判断。当前 05 上效果正确，但还不应该马上当成通用语义规则。

更稳的修复方向：

1. 优先在 Datalog 侧让 `PHIOutputForwarded` 覆盖这种“原始 pred block PHI 已经合并 raw vars”
   的情况，而不是 Python 后处理猜。
2. 如果继续用 postprocess，需要把这条规则写得很窄：
   - duplicate var set 和 candidate PHI use set 完全相同；
   - candidate PHI 所在 block 必须是原始 pred，且能到达最终 direct pred；
   - candidate PHI def 必须唯一可选，例如只有一个 def 被非 PHI 语句使用；
   - 否则继续保持失败。

## 2026-05-15 evm2llvm entry block 顺序修复

问题：

`03_small_9752b87bf2` 在补完 `DELEGATECALL` 后生成 IR，但 LLVM verifier 报：

```text
Entry block to function must not have predecessors!
label %bb._0x5f0xf88
```

进一步确认：

- `IRFunctionEntry.csv` 里的入口是 `0x0`、`0xf88`、`0x31`。
- 这些入口在 `LocalBlockEdge.csv` 中没有同函数 predecessor。
- 报错的 `%bb._0x5f0xf88` 不是函数入口，而是因为 evm2llvm 创建 LLVM basic block 时按
  `function.Blocks` 的排序顺序插入。`function.Blocks` 由 `FactLoader` 排序后，不保证
  `EntryBlock` 是第一个。LLVM 以函数里第一个 basic block 作为 entry，因此物理顺序错了。

修改：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:354`：
  `lowerFunction` 创建 `llvm::BasicBlock` 时，从遍历 `function.Blocks` 改成遍历
  `functionBlockLoweringOrder(program, function)`。这个顺序从 `function.EntryBlock` 开始，
  所以 LLVM function 的第一个 block 一定是真入口。
- `external/NotDec-evm2llvm/test/CMakeLists.txt:41`：
  新增 `entry_order` fixture。
- `external/NotDec-evm2llvm/test/fixtures/entry_order/*`：
  构造一个最小回归。`InFunction.csv` 中 `0x10` 排在 `0x20` 前，但
  `IRFunctionEntry.csv` 指定入口为 `0x20`，且有 `LocalBlockEdge(0x20, 0x10)`。
  旧代码会把 `0x10` 插成 LLVM 物理 entry，verify 失败；新代码通过。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture.entry_order --output-on-failure
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
ctest --test-dir build-evm2llvm -R 'evm2llvm.gigahorse.simple-storage' --output-on-failure
```

结果：

- `entry_order`：3/3 通过。
- `evm2llvm.fixture`：18/18 通过。
- `evm2llvm.gigahorse.simple-storage`：3/3 通过。

apehex 复查：

```bash
exp=/sn640/NotDecChainExp/evm2llvm_apehex_pilot
sid=03_small_9752b87bf2
facts="$exp/work/${sid}.cached/$sid/out"
ll="$exp/outputs/${sid}.entryfix.ll"
bc="$exp/outputs/${sid}.entryfix.bc"
build-evm2llvm/bin/evm2llvm --facts "$facts" --output "$ll"
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as "$ll" -o "$bc"
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify "$bc" -o /dev/null
```

结果：通过。

性能：

只改 evm2llvm basic block 创建顺序和一个小 fixture，不触碰 NotDec 主 pass pipeline、
类型恢复、结构体合并或 pointer analysis，未跑 fortune。

## 2026-05-18 PHI 现状复查和对照信息输出

PHI 复查：

- 当前 `/sn640/gigahorse-toolchain/gigahorse.py` 的 `repair_phi_incoming` 仍是 5 月 15 日的窄修复：
  修非直连 predecessor，以及用 predecessor block 内唯一 PHI def 合并 duplicate raw vars。
- 对缓存 facts 原始状态复查：
  - `05_medium_233cfe3212`：68 行，6 条非直连 edge，3 组 duplicate。
  - `06_medium_fe9f436f05`：17 行，1 条非直连 edge，0 组 duplicate。
  - `07_large_0994def38c`：300 行，0 条非直连 edge，23 组 duplicate。
  - `08_large_9fc75dd266`：同 07。
- 拷贝到 `/tmp/evm2llvm-phi-recheck-442024` 并调用当前 `repair_phi_incoming` 后：
  - `05`：非直连 edge 修掉，但仍有 3 组 duplicate，evm2llvm 仍报
    `duplicate PHIIncoming predecessor for PHI 0x38d_0x1S0x17e from 0xbf8B0x378B0x17e`。
  - `06`：PHI 校验通过，继续暴露 `unsupported opcode CODECOPY at 0x28c`。
  - `07/08`：`PHIIncoming.csv` 降到 274 行，PHI 校验通过，继续暴露
    `unsupported opcode EXTCODESIZE at 0x99e`。

判断：

- 当前 PHI 转换对 `06/07/08` 的已知问题已经修好。
- `05` 还没修好；日志中“用原始 pred block 内唯一被非 PHI 使用的 PHI def”仍只是手工验证过的窄经验规则，
  没有进入源码。这里继续不在 evm2llvm 里绕过 validator。

新增对照信息：

- `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py:12`：
  新增 EVM opcode 表，覆盖常见固定 opcode、`PUSH0`、`PUSH1..32`、`DUP1..16`、
  `SWAP1..16`、`LOG0..4`。
- `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py:228`：
  新增 `read_bytecode_bytes`，读取 `.hex` 字节码。
- `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py:238`：
  新增 `write_bytecode_listing`，在 contract work 目录写
  `evm-bytecode.txt`，列为 `pc / bytes / opcode / operand`。
- `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py:350`：
  Gigahorse facts 生成后、调用 evm2llvm 前写出 `evm-bytecode.txt`。
- `external/NotDec-evm2llvm/include/notdec-evm2llvm/TacProgram.h:66`：
  `TacProgram` 增加 `OriginalStatementsByStmt` 和 `InlineInfoByStmt`，保存 TAC 到原始 EVM
  statement 的映射。
- `external/NotDec-evm2llvm/lib/FactLoader.cpp:80`：
  新增 `parseOriginalStatementList`，解析 Gigahorse 的 `[0x42, 0xbd, nil]` 形式列表。
- `external/NotDec-evm2llvm/lib/FactLoader.cpp:161`：
  读取 `TAC_Statement_OriginalStatement.csv`、
  `TAC_Statement_OriginalStatementList.csv` 和 `TAC_Statement_InlineInfo.csv`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:145`：
  新增 `metadataForStmt`，生成 `!notdec.evm` 元数据，字段包含 `tac=...`、`op=...`、
  `evm.pc=...`，有 inline 信息时还带 `inline=...`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:178`：
  新增 `InsertedInstructionAnnotator`，给一个 TAC statement lowering 期间新增的 LLVM 指令挂元数据。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:501`、`518`、`530`、`537`：
  PHI、private call、普通 TAC 指令和真实控制流 terminator 都会带 `!notdec.evm`。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
python3 -m py_compile external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
build-evm2llvm/bin/evm2llvm \
  --facts /sn640/NotDecChainExp/evm2llvm_apehex_pilot/work/03_small_9752b87bf2.cached/03_small_9752b87bf2/out \
  --output /tmp/evm2llvm-meta-check.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/evm2llvm-meta-check.ll \
  -o /tmp/evm2llvm-meta-check.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output \
  /tmp/evm2llvm-meta-check.bc
external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py \
  /sn640/NotDecChainExp/evm2llvm_apehex_pilot/inputs/01_tiny_c68dbd0999.hex \
  --output /tmp/evm2llvm-wrapper-check/out.ll \
  --evm2llvm build-evm2llvm/bin/evm2llvm \
  --gigahorse-dir /sn640/gigahorse-toolchain \
  --work-dir /tmp/evm2llvm-wrapper-check/work \
  --timeout-secs 180 \
  --gigahorse-extra-arg=-i \
  --gigahorse-extra-arg=--disable_inline \
  --gigahorse-extra-arg=--disable_scalable_fallback
```

结果：

- `evm2llvm.fixture`：18/18 通过。
- `03_small_9752b87bf2`：生成 IR、`llvm-as`、`opt -passes=verify` 通过。
- IR 中能看到形如 `!notdec.evm !0` 的 metadata，metadata 内容形如
  `!"tac=0xbdS0x3c", !"op=SLOAD", !"evm.pc=0x42,0xbd", !"inline=[0x97, nil]"`。
- wrapper 在 `/tmp/evm2llvm-wrapper-check/work/01_tiny_c68dbd0999/evm-bytecode.txt`
  生成了 bytecode 指令列表。

性能：

- 只改 evm2llvm wrapper 和 IR 注解，不触碰 NotDec 主 pass pipeline、类型恢复、结构体合并或
  pointer analysis，未跑 fortune。
- 复杂度评分：实现效果 8/10，能直接从 IR metadata 反查 TAC 和原始 EVM pc；复杂度 3/10，
  主要是一个插入期 annotator 和 wrapper 反汇编表；维护成本 3/10，后续如果要支持新 opcode，
  只需要补 Python 表。

## 2026-05-18：Gigahorse 侧 PHI inline 修复

实现位置：

- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:510`
  新增 `ExistingPHIForwardCandidate` 等关系。规则是：如果 predecessor block 里已经有一个 PHI，
  同时后继 `PHIIncoming` 在同一个 pred 上列出了多个 raw vars，并且这个 PHI 的 use set
  正好等于这些 raw vars，就把后继 incoming value 改成这个 pred block PHI def。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:578`
  新增 `ExistingPHIInlinedReturnPred`。private call inline 后，原来的 callsite block 不再是直接
  predecessor，`PHIIncoming` 改为指向 cloned callee exit block。
- `/sn640/gigahorse-toolchain/clientlib/tac-transformers/abstract_function_inliner.dl:587`
  拆开 `Out_PHIIncoming` 输出规则，分别处理“已 forward / 未 forward”和“inline return pred /
  普通 pred”四种情况。
- `/sn640/gigahorse-toolchain/gigahorse.py:11` 和
  `/sn640/gigahorse-toolchain/gigahorse.py:281` 附近：删除旧的 Python `repair_phi_incoming`
  后处理和调用。现在不再靠 Python 猜修 CSV。

修复后的 05 关键事实：

```text
0x38d_0x1S0x17e  0x38dB0x17e  0xbf8B0x378B0x17e  0x378_0x0V0x17e
0x3ff_0x1S0x1a2  0x3ffB0x1a2  0xbf8B0x3eaB0x1a2  0x3ea_0x0V0x1a2
0x46b_0x1S0x1b5  0x46bB0x1b5  0xbf8B0x456B0x1b5  0x456_0x0V0x1b5
```

也就是说：

- pred 已经是真实直接入边 `0xbf8B...`。
- value 已经是 pred block 内合并后的 PHI def，不再是两个 raw vars。

验证：

```bash
python3 -m py_compile /sn640/gigahorse-toolchain/gigahorse.py
python3 /sn640/gigahorse-toolchain/gigahorse.py \
  -w /tmp/gigahorse-dl-phi-05-norepair-tight -j 1 -T 180 \
  --results_file /tmp/gigahorse-dl-phi-05-norepair-tight/results.json \
  --restart /sn640/NotDecChainExp/evm2llvm_apehex_pilot/inputs/05_medium_233cfe3212.hex
build-evm2llvm/bin/evm2llvm \
  --facts /tmp/gigahorse-dl-phi-05-norepair-tight/05_medium_233cfe3212/out \
  --output /tmp/gigahorse-dl-phi-05-norepair-tight/05.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/gigahorse-dl-phi-05-norepair-tight/05.ll \
  -o /tmp/gigahorse-dl-phi-05-norepair-tight/05.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output \
  /tmp/gigahorse-dl-phi-05-norepair-tight/05.bc
```

结果：

- `05`：`PHIIncoming.csv` 65 行，`bad_edges=0`，重复 `(phi, block, pred)=0`；
  evm2llvm、`llvm-as`、`opt -passes=verify` 通过。
- `06`：`PHIIncoming.csv` 17 行，`bad_edges=0`，重复 `(phi, block, pred)=0`；
  evm2llvm 继续到已有问题 `unsupported opcode CODECOPY at 0x28c`。
- `07/08`：`PHIIncoming.csv` 274 行，`bad_edges=0`，重复 `(phi, block, pred)=0`；
  evm2llvm 继续到已有问题 `unsupported opcode EXTCODESIZE at 0x99e`。

判断：

- 当前这组 inline + PHI 问题已经在 Gigahorse 导出侧修复。
- evm2llvm 不需要放宽 PHI validator，也不需要 slot + mem2reg fallback。
- 复杂度评分：实现效果 8/10；复杂度 5/10，Datalog 规则比 Python repair 长，但语义位置更对；
  维护成本 4/10，后续主要风险是“多个候选 PHI 的消歧规则”仍依赖非 PHI use。
- 性能：只改 Gigahorse EVM facts 生成，不触碰 NotDec 主 pass pipeline、类型恢复、结构体合并或
  pointer analysis，未跑 fortune。
