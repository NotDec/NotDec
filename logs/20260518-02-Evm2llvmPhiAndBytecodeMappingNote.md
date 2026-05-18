# evm2llvm PHI 现状和 bytecode/IR 对照说明

## 背景

这份日志单独说明两个问题：

1. Gigahorse 导出的 `PHIIncoming.csv` 当前修到什么程度，哪里还没修好。
2. evm2llvm 现在怎样输出 EVM bytecode 指令列表，并在 LLVM IR 里标出原始 EVM 指令地址，方便反查。

相关旧调查在：

- `logs/20260515-04-Evm2llvmPhiFactInvestigation.md`

样本来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot`

## PHI 问题本质

LLVM native PHI 有两个硬要求：

1. incoming predecessor 必须是 PHI 所在 block 的直接 CFG predecessor。
2. 同一个 PHI 在同一个 predecessor 上只能有一个 incoming value。

之前 Gigahorse facts 里有两类问题：

1. `PHIIncoming` 还指向 inline 前的 callsite block，但真实 CFG 已经插入了 callee block。
2. 同一个 `(phiStmt, block, predBlock)` 下有多个不同 `var`，这不能直接转成 LLVM PHI。

evm2llvm 当前继续拒绝这两类非法 facts，不做“任选一个值”这种绕过。

## 当前修复效果

当前 `/sn640/gigahorse-toolchain/gigahorse.py` 的 `repair_phi_incoming` 只做两条窄规则：

1. 如果 incoming pred 不是直接 predecessor，但能唯一追到 PHI block 的某个直接 predecessor，就把 pred 改成这个直接 predecessor。
2. 如果同一个 `(phiStmt, block, predBlock)` 有多个 raw var，并且 predecessor block 内有唯一一个 PHI 的 use set 正好等于这些 raw var，就用这个 PHI def 替换 raw vars。

复查结果：

| 样本 | 原始 PHI 问题 | repair 后结果 |
| --- | --- | --- |
| `05_medium_233cfe3212` | 6 条非直连 edge，3 组 duplicate | 非直连 edge 修掉，但 3 组 duplicate 仍在 |
| `06_medium_fe9f436f05` | 1 条非直连 edge | PHI 校验通过，继续失败在 `CODECOPY` 未支持 |
| `07_large_0994def38c` | 23 组 duplicate | PHI 校验通过，继续失败在 `EXTCODESIZE` 未支持 |
| `08_large_9fc75dd266` | 同 07 | 同 07 |

结论：

- `06/07/08` 的已知 PHI facts 问题已经修好。
- `05` 还没修好，不能说 PHI 转换整体完成。

## 05 还剩的问题

`05_medium_233cfe3212` 的代表原始 facts：

```text
0x38d_0x1S0x17e  0x38dB0x17e  0x378B0x17e  0x35dV0x17e
0x38d_0x1S0x17e  0x38dB0x17e  0x378B0x17e  0x3c3V0x17e
```

含义是：

- PHI statement：`0x38d_0x1S0x17e`
- PHI block：`0x38dB0x17e`
- incoming pred：`0x378B0x17e`
- incoming var 有两个：`0x35dV0x17e` 和 `0x3c3V0x17e`

但 `0x378B0x17e -> 0x38dB0x17e` 不是直接边。真实路径是：

```text
0x378B0x17e
-> 0xbdfB0x378B0x17e
-> 0xbefB0x378B0x17e
-> 0xaefB0xbefB0x378B0x17e
-> 0xb05B0xbefB0x378B0x17e
-> 0xbf8B0x378B0x17e
-> 0x38dB0x17e
```

当前 repair 会把 pred 改成 `0xbf8B0x378B0x17e`，但两个 incoming var 还在，所以 evm2llvm 仍报：

```text
duplicate PHIIncoming predecessor for PHI 0x38d_0x1S0x17e from 0xbf8B0x378B0x17e
```

之前手工验证过一种更激进规则：从原始 pred block 里找 use set 相同、且唯一被非 PHI 使用的 PHI def，用它替换 raw vars。这个能让 05 过 verifier，但还没有写进源码，因为它更像经验规则，不是 Gigahorse facts 的明确语义。

## 为什么真实路径会变成重复 PHI 入边

先看正常循环 PHI 应该是什么样。

`05` 里这组值本质上是一个循环变量：

```text
初始值：0x35dV0x17e  # CONST 0
回边值：0x3c3V0x17e  # ADD 0x38d_0x1V0x17e + 1
```

在 inline 前，这两个值应该来自两条不同边：

```text
loop header PHI:
  from loop preheader uses 0x35dV0x17e
  from loop backedge   uses 0x3c3V0x17e
```

LLVM 能表达这种 PHI，因为两个 incoming 的 predecessor 不同。

问题出在后面的 private call inline。inline 会把原来一条 call edge 展开成一段 callee 路径：

```text
0x378B0x17e
-> 0xbdfB0x378B0x17e
-> 0xbefB0x378B0x17e
-> 0xaefB0xbefB0x378B0x17e
-> 0xb05B0xbefB0x378B0x17e
-> 0xbf8B0x378B0x17e
-> 0x38dB0x17e
```

这时对 LLVM 来说，`0x38dB0x17e` 的直接 predecessor 已经不是原始 callsite
`0x378B0x17e`，而是 inline 后的返回出口 `0xbf8B0x378B0x17e`。

当前 repair 做了第一步：把 incoming pred 从 `0x378B0x17e` 改成
`0xbf8B0x378B0x17e`。这一步本身是对的。

但第二个问题还在：Gigahorse 导出的 incoming value 没有跟着变成“这条 inline 返回边上的唯一值”，
而是把原来参与合流的 raw vars 继续原样导出来：

```text
0x38d_0x1S0x17e  0x38dB0x17e  0xbf8B0x378B0x17e  0x35dV0x17e
0x38d_0x1S0x17e  0x38dB0x17e  0xbf8B0x378B0x17e  0x3c3V0x17e
```

这就是 duplicate incoming。两行的 PHI、block、pred 都一样，只有 var 不一样。

更直白地说：

- 控制流上，inline 后只剩一条直接入边：`0xbf8B... -> 0x38d...`。
- 数据流上，facts 还保留了两个候选 raw value：`0x35d...` 和 `0x3c3...`。
- LLVM PHI 要求“一条入边只能对应一个值”，所以 evm2llvm 不能直接 lower。

为什么不能简单选一个？

因为这两个值语义不同：

```text
0x35dV0x17e = 循环初始值 0
0x3c3V0x17e = 循环递增后的值
```

任选一个都会丢路径语义。正确结果应该是在 inline 前或 inline 过程中，先有一个 PHI 把这两个值合成一个 SSA 值，然后后继 PHI 的 incoming 使用这个 PHI result，而不是继续展开 raw vars。

`05` 里其实能看到这种候选 PHI：

```text
0x378_0x0V0x17e  # use set 正好是 {0x35dV0x17e, 0x3c3V0x17e}
```

手工验证时，用这个 PHI def 替换两个 raw vars，生成的 LLVM IR 可以通过 verifier：

```text
0x38d_0x1S0x17e <- 0x378_0x0V0x17e from 0xbf8B0x378B0x17e
```

但这个规则还没有进入源码，因为同一个 block 里可能有多个 use set 相同的 PHI。之前 `05` 里就有：

```text
0x378_0x0V0x17e  # 被普通语句使用
0x378_0x3V0x17e  # 未被普通语句使用
```

手工选择的是“被普通语句使用”的那个。这在当前样本上合理，但更像从结果倒推，不是 facts 明确告诉我们的语义。所以更稳的修法还是在 Gigahorse 的 inliner / `PHIOutputForwarded` 导出阶段，把后继 incoming 直接写成已经合流后的 PHI def。

## 新增 bytecode 指令 TXT

修改位置：

- `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py`

wrapper 现在在 Gigahorse facts 生成后，会在 contract work 目录写：

```text
<work-dir>/<contract-name>/evm-bytecode.txt
```

例如：

```text
/tmp/evm2llvm-wrapper-check/work/01_tiny_c68dbd0999/evm-bytecode.txt
```

格式：

```text
pc    bytes    opcode    operand
```

实际片段：

```text
pc    bytes       opcode        operand
0x0   0x6080      PUSH1         0x80
0x2   0x6040      PUSH1         0x40
0x4   0x52        MSTORE
0x5   0x73...ffff PUSH20        0xffffffffffffffffffffffffffffffffffffffff
0x1c  0x54        SLOAD
0x41  0x35        CALLDATALOAD
0x42  0x14        EQ
```

这里的 `pc` 是 EVM byte offset。`PUSHn` 会把立即数合并到同一行，所以后续对照要用 `pc`，不要用行号。

## LLVM IR 里的对照信息

修改位置：

- `external/NotDec-evm2llvm/include/notdec-evm2llvm/TacProgram.h`
- `external/NotDec-evm2llvm/lib/FactLoader.cpp`
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp`

evm2llvm 现在读取 Gigahorse 的：

```text
TAC_Statement_OriginalStatement.csv
TAC_Statement_OriginalStatementList.csv
TAC_Statement_InlineInfo.csv
```

然后给生成的 LLVM instruction 挂 `!notdec.evm` metadata。

IR 片段：

```llvm
%evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
%evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
%evm.eq = icmp eq i256 1549851163, %evm.shr, !notdec.evm !9
```

metadata 片段：

```llvm
!7 = !{!"tac=0xe", !"op=CALLDATALOAD", !"evm.pc=0xe"}
!8 = !{!"tac=0x10", !"op=SHR", !"evm.pc=0x10"}
!9 = !{!"tac=0x16", !"op=EQ", !"evm.pc=0x16"}
```

有 inline 信息时会看到多个原始 pc：

```llvm
!37 = !{!"tac=0xbdS0x3c", !"op=SLOAD", !"evm.pc=0x42,0xbd", !"inline=[0x97, nil]"}
```

这个例子里：

- `tac=0xbdS0x3c` 是 Gigahorse TAC statement。
- `op=SLOAD` 是 TAC/EVM 操作。
- `evm.pc=0x42,0xbd` 表示这个 TAC statement 的原始 EVM statement list。第一个通常是 inline/callsite 上下文，后面是实际被 inline 的原始指令。
- `inline=[0x97, nil]` 是 Gigahorse 给出的 inline 信息。

## 原始输入对照方法

以 `03_small_9752b87bf2` 的缓存 facts 为例：

原始 bytecode 在：

```text
/sn640/NotDecChainExp/evm2llvm_apehex_pilot/inputs/03_small_9752b87bf2.hex
```

Gigahorse work 目录里也有一份：

```text
/sn640/NotDecChainExp/evm2llvm_apehex_pilot/work/03_small_9752b87bf2.cached/03_small_9752b87bf2/bytecode.hex
```

原始 bytecode 片段：

```text
608060405260043610601f5760003560e01c80635c60da1b146031...
```

对应反汇编开头：

```text
0x0   0x6080  PUSH1  0x80
0x2   0x6040  PUSH1  0x40
0x4   0x52    MSTORE
```

对应 IR：

```llvm
call void @evm_mstore(ptr %mem, i256 64, i256 128), !notdec.evm !0
```

对应 metadata：

```llvm
!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
```

所以反查步骤是：

1. 在 IR 指令后找到 `!notdec.evm !N`。
2. 到文件末尾找 `!N = ...`。
3. 看 `evm.pc=...`。
4. 到 `evm-bytecode.txt` 里按 `pc` 找原始 EVM 指令。

## 验证命令

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
```

结果：

- `evm2llvm.fixture`：18/18 通过。
- `03_small_9752b87bf2`：IR 生成、`llvm-as`、`opt -passes=verify` 都通过。
- wrapper tiny 样本确认生成了 `evm-bytecode.txt`。

## 后续建议

1. 继续修 `05_medium_233cfe3212` 的 Gigahorse PHI 导出语义，不要在 evm2llvm 里放宽 validator。
2. 如果要让 `evm-bytecode.txt` 覆盖所有新 opcode，只需要补 `notdec-evm2llvm.py` 里的 opcode 表。
3. 后续查 IR 语义问题时，优先用 `!notdec.evm` 里的 `evm.pc` 回到 `evm-bytecode.txt`，再看 Gigahorse 的 `TAC_Statement_OriginalStatementList.csv`。

## 性能

这次只是写独立说明文档，没有改代码，没有跑 fortune。
