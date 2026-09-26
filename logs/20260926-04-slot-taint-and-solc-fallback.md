# 20260926-04 下一个值得修的问题：槽接受规则的不动点方向反了（+ solc pipeline 回退）

## 0. 一句话

上一轮之后看剩余 TODO，第一名**不是**缺哪个 EVM 指令，而是我自己那条槽接受规则的**不动点方向反了**：
它是"从空集出发，只接纳已经干净的槽"，于是**互相引用的槽永远进不来**——而循环计数器天然就是互相引用的
（`%_0x8f1_0x1` 的某个 store 读 `%_0x8fa_0x1` 的槽，后者又读前者的槽）。结果三个最大的 case
（0029/0114/0726，合计占全部 condition TODO 的 **50%**）在一个不变的数字上冻结了好几轮。
把不动点反过来（**先假设全部可用，再剔除真正被污染的**）之后：condition TODO **3159 → 2541（−618）**、
unresolved **139 → 134（−5，没有靠占位符换指标）**、goto/real 各 −4、**44 个 case 变好、0 个变差**。

## 1. 怎么定位到这里的

1. 先量分布：0029 792、0114 398、0726 398、0258 113…… 前三名 = 全部 3159 条的 **50%**，
   而它们上一轮一条没动。
2. 用上一轮加的 `NOTDEC_COND_DEBUG=1` 打出条件叶子：0029 的叶子是槽 reload
   （`_0x8f1_0x1.i.i.i.i.reload`）和 `evm_call`。
3. 用一小段 IR def-use 追踪脚本（`/tmp/chain_trace_926.py`）顺链：

```llvm
%evm.add61  = add i256 1, %_0x8fa_0x1      ; 计数器 +1
%_0x8fa_0x1 = phi [ %_0x8f1_0x1, bb._0x8f1 ]                 ; 循环体内复制
%_0x8f1_0x1 = phi [ 0, bb._0x8e4 ], [ %evm.add61, bb._0x90b ] ; 归纳变量
```

   即 `_0x8f1_0x1 ← _0x8fa_0x1 ← _0x8f1_0x1`：**互引用**。而"所有 store 的值都已解析"这个条件在
   只有部分槽可用时对两者都为假 → 两者都被拒 → 读它们的条件全部塌成 TODO。
   这不是"缺表示"，是**判定算法的方向问题**。

## 2. 正确的修法：最大不动点（乐观 + 污染传播）

原来（最小不动点，错的）：

```
accepted = {}
repeat: 把"所有 store 都已解析"的候选加入 accepted，直到不再变化
```

现在（最大不动点）：

```
accepted = 全部候选            // 先让所有槽可寻址：槽间引用渲染成标识符，不再算"未解析"
repeat: 删掉那些"仍有 store 渲染出未解析标记"的槽   // 只可能是外部真缺口
        删槽会污染读它的槽，所以迭代到不动点
```

关键点：**在"全部槽可寻址"的前提下**，store 值里若还有未解析标记，那一定不是槽造成的（槽已经渲染成标识符了），
所以它代表真实的缺失表示——剔除它、并沿污染链传播，是正确的。互引用的槽在这种语义下是干净的，
于是循环计数器变成局部变量。

## 3. 效果

0029 从 792 降到 664（−128），0114/0726 各 −64，0189 −53，0038/0450 各 −48，0679 −40，1991 −31；
全语料 44 个 case 变好、**没有一个变差**，而且 unresolved 还降了 5。0029 的输出形态变化很直观：

```solidity
temp_202 = 0;
temp_203 = 739426;
while (!(temp_203 == 0)) {          // 之前是 while (true) { if (false /* TODO */) break; }
  temp_202 = temp_202 + uint256(1);
  temp_203 = temp_203 / uint256(16);
}
while (temp_405 < temp_406) { ... }
```

| 指标 | before | after |
| --- | --- | --- |
| condition TODO | 3159 | **2541** |
| unresolved | 139 | **134** |
| goto / real / dangling | 1193 / 1111 / 6 | 1189 / 1107 / 6 |
| helper functions / call sites | 1612 / 3449 | **1738 / 3713** |
| 变差 case | — | **0** |

## 4. 顺带修的：solc pipeline 回退

`--via-ir` 默认之后，0146 出现 solc **未捕获异常**（`EVMObjectCompiler.cpp` 抛
`StackTooDeepError`：`Variable var_ret0_514 is 3 slot(s) too deep`）——即 IR pipeline 自己也有
栈溢出，而且是以内部异常的形式抛的。这个 case 在 **legacy 下能编过**。

所以 compile suite 改成：**先试 legacy（快 ~25×），失败再用 `--via-ir`，两者都失败才算失败**。
理由：两个 pipeline 的前端检查（类型、作用域、mutability——本 suite 至今抓到的所有真实错误类别）
完全相同，产物只需被其中一个接受；回退只在 codegen 限制上生效，检测能力不变。
`--solc-arg` 仍然可以钉死单一 pipeline。

## 5. 下一步（重新排序后的清单）

1. **未渲染的高频 runtime 调用**（表达式位置，很多会落进条件）：
   `evm_returndatasize`（1455 次调用！）/`evm_extcodesize`（263）/`evm_codesize`（165）/
   `evm_byte`（132）/`evm_extcodehash`（100）/`evm_create`（101）/`evm_tload`+`evm_tstore`（99）。
   这些在 BodyBuilder 里 grep 计数都是 **0**，即完全没有渲染路径；多数是"一行 escape"级别
   （`returndatasize()`/`extcodesize`/`codesize`/`extcodehash`），`evm_byte` 可以直接展开成
   `(x >> (248 - 8 * i)) & 0xff`（i 为常量时）。
2. **COPY 家族的语句 escape**：`evm_returndatacopy`（581）/`evm_calldatacopy`（252）/
   `evm_codecopy`（160）/`evm_mcopy`（102）至今**静默丢副作用**。注意它们的长度实参常常就是
   `evm_returndatasize`，所以第 1 项要先做，否则 escape 的实参仍然未解析而被拒。
3. 之后再跑一次 `NOTDEC_COND_DEBUG` 重排长尾（现在 `evm.lt.*`/`evm.iszero*` 系列约 200 条）。

## 6. 验证

`notdec.evm.solidity_pattern_compile`：103 case、`compile_failures=0`（含 legacy 回退），
unresolved 134/134、condition TODO 2541/2541、goto 1189/1189、real 1107/1107、dangling 6/6、
helper 1738/1738、call sites 3713/3713。
`notdec.evm.solidity_source` / `notdec.evm.solidity_rewrite` 通过；
`notdec.evm.solidity_patterns` 102/103（唯一失败 24259 仍是另一会话 mlsub WIP）。
