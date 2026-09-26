# 20260926-06 alloc 基址 escape：把宿主指针换成符号化基址

## 0. 一句话

按 05 号 log 的分析落地了方案 B：`MemoryBufferAnalysis` 生成的 `ptrtoint(calloc(...))`
（free-memory-pointer load 的替代品）现在渲染成 `__evm_alloc_base(k)` —— **每个 allocation 一个互不相同的
符号基址**（`shl(128, k+1)`，远离真实内存偏移）。派生偏移、mload escape 的实参、以及承载它们的 SSA 槽
全部恢复可渲染：condition TODO **2541 → 2270（−271）**、unresolved **134 → 117（−17）**、
53 个 case 变好；代价是 goto/real 各 +9（新渲染出的局部变量/语句让结构器保留了一些以前会被折掉的分支）。

## 1. 实现

1. `BodyBuilder` 新增 `allocBaseExpr`：识别 `ptrtoint(<calloc|calloc_unbounded 调用>)`，用一张
   **指令 → 编号**的稳定映射（同一指令永远同一编号，保证 helper 在发射性不动点里重复渲染时输出一致）
   渲染 `__evm_alloc_base(k)`；接入 `valueExpr` 的入口（在 materialized/phi 之后、其它匹配之前）。
2. `Reader::makeIntrinsicFunction` 新增 prelude：
   `function __evm_alloc_base(uint256 index) internal pure returns (uint256 value)
   { assembly { value := shl(128, add(index, 1)) } }`。
3. compile suite 的 pipeline 回退加了第三档：`[]` → `["--via-ir"]` → `["--via-ir","--optimize"]`。
   0029/0114/0726（正是最大的三个 case）在 legacy、`--via-ir`、legacy+`--optimize` 下都是
   `Stack too deep`，只有 **`--via-ir --optimize`** 能编过——这也正是 solc 报错信息里推荐的组合。

## 2. 效果

| 指标 | before | after |
| --- | --- | --- |
| condition TODO | 2541 | **2270** |
| unresolved | 134 | **117** |
| goto / real / dangling | 1189 / 1107 / 6 | 1198 / 1116 / 6 |
| helper functions / call sites | 1738 / 3713 | **1835 / 4583** |
| 变好 / 变差 case | — | 53 变好 / 0 变差（7 个 case 仅 real jump +1~3） |

收益最大的：0258 −27、25928 −14（unresolved −6）、0029 −12、0009 −9、1490/1915/2002 各 −8。
0029/0114/0726 这次只掉了个位数——因为它们剩下的叶子已经换成了别的根因（见下）。

## 3. 剩余根因（下一步）

用 `NOTDEC_SLOT_DEBUG`/`NOTDEC_COND_DEBUG` 复测，top3 现在的叶子是
`evm.mload.*`（偏移仍非解析的那些）、`evm_call`、`and(1, evm.sload)`，且 0114 还有 16 条
`/* TODO: switch ... */`（switch 的 TODO 不在 condition TODO 计数里，是另一个口径）。
也就是说：alloc 基址解决后，剩下的阻塞回到"**某个第三方值**不可渲染"。下一步按调用量/叶子量排序：

1. `evm_returndatasize`（1455 次调用，后端 grep 计数 0）、`evm_extcodesize`(263)、`evm_codesize`(165)、
   `evm_extcodehash`(100)、`evm_byte`(132)、`evm_create`(101)、`evm_tload/tstore`(99)；
2. COPY 家族语句 escape（`returndatacopy` 581 / `calldatacopy` 252 / `codecopy` 160 / `mcopy` 102，
   至今静默丢副作用；注意它们的长度实参常是 `evm_returndatasize`，所以第 1 项要先做）；
3. 把 `switch` 的 TODO 也纳入统计口径，避免"修好了但指标看不见"。

## 4. 验证

`notdec.evm.solidity_pattern_compile`：103 case、`compile_failures=0`（三档 pipeline 回退），
unresolved 117/117、condition TODO 2270/2270、goto 1198/1198、real 1116/1116、dangling 6/6、
helper 1835/1835、call sites 4583/4583。
`solidity_source` / `solidity_rewrite` 通过；`solidity_patterns` 102/103（唯一失败 24259 仍是另一会话 WIP）。
