# 20260926-05 下一个值得修的问题：alloc 基址（`evm.alloc.addr`）毒化了整片内存偏移网

## 0. 一句话

上一轮把槽的不动点方向修正后，再看剩余 2541 条 condition TODO 的**根因**（不是最外层包装，而是最底层的
不可渲染值）：top3（0029 664 / 0114 334 / 0726 334 = 52%）的阻塞链最终都落到
**`evm.alloc.addr`** —— `MemoryBufferAnalysis` 把动态 ABI buffer 的基址从"EVM 内存里的一个地址"换成了
**宿主指针的字面形式** `ptrtoint(calloc(...))`。Solidity/Yul 里没有 ptrtoint，这个值无法渲染；而它又是
free-memory-pointer 记账的起点，于是**所有**由它派生的内存偏移、由这些偏移喂的 mload/store、以及存储这些
值的 SSA 槽全部被污染 → 条件塌成 TODO。这是目前唯一一个**跨层**（pass ↔ 后端）的根因，价值也最大。

## 1. 怎么定位的

1. 分布：2541 条，top3 = 1332（52%）。
2. `NOTDEC_COND_DEBUG=1` 打条件叶子：0114 的叶子是 `evm.mload.*`（内联后缀）和 `evm.and8`；
   0029 是 `evm_call` / `evm.and8` / `evm.mload.*`。
3. 顺着 0114 的一条链追（`/tmp/chain_trace_926.py`）：

```llvm
%_0x29a_0x0 = phi [ %evm.add6, ... ], [ %evm.add23, ... ]   ; 循环里"下一个空闲地址"
%evm.add6   = add i256 32, %evm.mload                        ; %evm.mload = load ptr inttoptr(64) 即 mload(0x40)
%evm.add23  = add i256 32, %_0x1fc_0x0
```

   看起来 mload(0x40) 的偏移是常量 64，应该能 escape 成 `__evm_mload(64)`——但 0114 的输出里
   `__evm_mload` **只出现 1 次**。
4. 于是给"槽被剔除"加了同样的开关（`NOTDEC_SLOT_DEBUG=1`，本轮新增）打印**剔除原因和未解析叶子**，
   结果直接指出根：

```
[slot-taint] _0x1fc_0x0 <- evm.add6 : evm.alloc.addr
[slot-taint] _0x29a_0x0 <- evm.add6 : evm.alloc.addr
[slot-taint] private.call1.i.i.i <- evm.alloc.addr1 : evm.alloc.addr1
...（0114 里 8 个槽都是这个原因）
```

5. `evm.alloc.addr` 的来源：`src/Passes/evm/MemoryBufferAnalysis.cpp:319-327`：

```cpp
CallInst *NewBasePtr = AllocBuilder.CreateCall(AllocFn /* calloc */, {...});
Value *NewBase = AllocBuilder.CreatePtrToInt(NewBasePtr, I256, "evm.alloc.addr");
BaseLoad->replaceAllUsesWith(NewBase);     // BaseLoad 就是 mload(0x40)
```

   即：pass 把 **free-memory-pointer 的那个 load 的所有使用者**换成了 `ptrtoint(calloc(...))`。
   注意 `evm.alloc.addr` 在输入语料里出现 **0 次**（全 103 case），它是反编译期才产生的。

## 2. 为什么这是"值得修"且"必须跨层"的问题

- 后端**没有**合法的渲染方式：Solidity 没有指针→整数的转换，Yul 也没有。所以这不是"再补一个 escape"
  就能解决的（`assembly { value := ptrtoint(...) }` 不存在）。
- 但它在 EVM 语义里有明确对应物：动态 buffer 就是**平坦 EVM 内存里的一段**，它的"地址"本来就是程序自己
  算出来的字（通常是 free-memory-pointer 的值）。pass 用一个宿主指针替换掉了这个**具体的 EVM 地址**，
  丢掉的正是后端唯一能渲染的东西。
- 影响面：0114 里 8 个槽、0029/0726 同样（`evm.mload.*`/`evm_call` 的叶子都是它的下游）；
  它是"内层不可渲染值"里唯一一个出现在**三个最大 case 共同链**上的。

## 3. 建议的修法（两个方案，推荐 B）

**A. pass 侧：保留符号化的 EVM 基址，把"宿主指针"降级成元数据。**
`MemoryBufferAnalysis` 不再 `replaceAllUsesWith(ptrtoint(calloc))`，而是：
   - 给每个 allocation 分配一个**规范化的符号基址**（i256 常量，取值与程序里其它内存地址不冲突，
     例如 `1<<128 | k<<64`），用它替换 `mload(0x40)` 的用途；
   - 把"这个 buffer 对应哪个 calloc/大小/来源"放进 **metadata**（后续 pass 与后端都能读），
     而不是放进值本身。
   优点：从根上让值世界保持可渲染；后端完全不用改。风险：动了 pass 的核心表示，需要重新验证
   `AbiReturnPass`/`AbiDecodeResult` 等依赖 metadata 的地方，回归面大。

**B. 后端侧：把 `ptrtoint(calloc)` 渲染成"按 allocation 编号的符号基址"（推荐先做，改动局部）。**
   - 在 `valueExpr` 的 `PtrToIntInst`/`ConstantExpr` 分支（以及按名字 `evm.alloc.addr`）里识别
     `ptrtoint(<calloc/calloc_unbounded 调用>)`，渲染成 `__evm_alloc_base(k)`（k 按出现顺序编号，
     同一指令稳定映射到同一编号）；
   - prelude：`function __evm_alloc_base(uint256 index) internal pure returns (uint256 value)
     { assembly { value := shl(128, add(index, 1)) } }` —— 每个 allocation 得到互不相同的、
     远离真实内存偏移的符号基址；
   - 这样 `freePtr = base + 32` → `__evm_alloc_base(0) + 32`（可渲染），派生偏移全部可渲染，
     mload 的 escape 能点火，槽不再被污染。
   优点：改动局限在 `BodyBuilder` + prelude，回归面小；语义上诚实（"这是一个 allocation 的符号基址"），
   而且给后续折叠 matcher 留了抓手（同一 buffer 的访问共享 `__evm_alloc_base(k)`）。
   风险：如果某个 load/store 直接用 calloc 指针（而不是 `inttoptr(word)`）访问 buffer，那部分仍然不可渲染
   ——需要实测确认这种形状有多少（下一步第一件事就是量它）。

**共同点**：无论 A 还是 B，"allocation 基址"都必须变成**符号常量**而不是宿主指针，否则后端只能输出占位符。

## 4. 下一步的具体动作（按顺序）

1. 实现方案 B 的识别 + prelude（小改动），跑 `NOTDEC_SLOT_DEBUG`/`NOTDEC_COND_DEBUG` 看
   `evm.alloc.addr` 根是否消失、top3 的 TODO 掉多少；同时看 compile suite（via-ir/legacy 回退）是否仍 0 失败。
2. 量"calloc 指针直接访问"的形状（`load/store` 的指针是 calloc 调用）还有多少；如果占比高，
   再考虑把这些访问也标注成 `offset` 元数据，让后端按 `inttoptr` 同样处理。
3. 之后再回头处理第 5 节列出的高频未渲染调用（`evm_returndatasize` 1455 次等）——它们多半也是这些
   case 的下游，等基址问题解决后重排优先级更准。

## 5. 本轮新增的诊断（已保留，默认零输出）

`demotedSlotStoresRecovered` 在 `NOTDEC_SLOT_DEBUG=1` 时向 stderr 打印**每个被剔除的槽 + 触发剔除的
store 值 + 未解析叶子**：

```
[slot-taint] _0x29a_0x0 <- evm.add6 : evm.alloc.addr
```

和 `NOTDEC_COND_DEBUG` 一样：不设环境变量时输出逐字节不变。

## 6. 验证（本轮）

- compile suite：103 case、`compile_failures=0`、全部预算贴棘轮（2541/134/1189/1107/1738/3713）。
- source / rewrite / patterns 同上一轮（patterns 唯一失败 24259 仍是另一会话 mlsub WIP）。
