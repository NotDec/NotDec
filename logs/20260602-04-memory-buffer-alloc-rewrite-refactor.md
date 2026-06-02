# MemoryBuffer allocation rewrite refactor

## 原始 prompt

总的来说，要朝着下面的方向重构：

1. MemoryBufferPass的主要目的是，重写IR对那个0x40的栈指针的操作，类似传统二进制的方式，根据栈指针操作，转换为比如evm_alloc，evm_alloc_unbound这种调用。后续的mstore都直接基于它们的返回值。这里其实不需要太纠结语义完全不变，有一点越界访问都可以容忍，毕竟可以假设语义evm_alloc确实就分配贴在一起的内存。

2. 不需要单独有这种marker，就用普通的地址运算和mload mstore即可。
```
notdec_solidity_memory_write
notdec_solidity_memory_byte_write
notdec_solidity_memory_copy_write
notdec_solidity_memory_read
notdec_solidity_memory_consumer
notdec_solidity_memory_array_byte_write
```

尝试规划一下重构该怎么做，怎么较好地保留MemoryConsumer那边已有的识别能力，比如相关逻辑挪到别的Pass里面，而不是由MemoryBufferPass识别标记后后面Pass再Rewrite。这样那些专门的标记的marker也可以去掉了。

---

写一个logs/下的文档，用于本次重构。分两部分，期望架构和具体做法。前面一部分介绍希望的实现架构，先把刚才这里聊的写进去，包括简化为两个形态。考虑要不要设计两种标注大小的，如果大小非常确定的就用notdec_evm_alloc(size) 。如果不完全确定，但是有一个大小的数值的，就用metadata标一下，这种有没有必要？其他的部分不着急写进去，待会沟通清楚再写。

## 期望架构

### MemoryBufferPass 的目标

`MemoryBufferPass` 的主目标不再是往 IR 里插一批 memory fact marker。

它应该专注处理 Solidity / Yul free memory pointer，也就是 `mload(0x40)` 和 `mstore(0x40, newFreePtr)` 这一条线。目标类似传统二进制里的栈指针恢复：把基于 free memory pointer 的地址，改写成更明确的 allocation base。

也就是说，`MemoryBufferPass` 应该把这类形状：

```llvm
%base = call i256 @evm_mload(ptr %mem, i256 64)
%ptr = add i256 %base, 32
call void @evm_mstore(ptr %mem, i256 %ptr, i256 %value)
```

改成后续 pass 更容易理解的形状：

```llvm
%base.alloc = call i256 @notdec_evm_alloc_unbounded()
%ptr = add i256 %base.alloc, 32
call void @evm_mstore(ptr %mem, i256 %ptr, i256 %value)
```

如果 size 很确定，则用：

```llvm
%base.alloc = call i256 @notdec_evm_alloc(i256 %size)
```

后续 ABI return、revert、event、external call 等 pass 仍然直接看普通 `evm_mstore`、`evm_mload`、`evm_return`、`evm_revert`、`evm_logN`、`evm_call`。它们不应该依赖一批中间 memory marker。

### allocation 和 free pointer 写回形态

先保留三个 IR 形态：

```llvm
declare i256 @notdec_evm_alloc(i256 %size)
declare i256 @notdec_evm_alloc_unbounded()
declare void @notdec_evm_finalize_alloc(i256 %base, i256 %size)
```

含义：

- `notdec_evm_alloc(size)`
  - size 在 allocation 点可用，并且和 free memory pointer 写回关系明确。
  - 例如 `mstore(0x40, add(base, size))` 里的 size 能在 `%base` 定义点安全使用。
- `notdec_evm_alloc_unbounded()`
  - 只确定这是从 free memory pointer 取出的 allocation base。
  - 暂时不能在 allocation 点提供可靠 size，例如 size 不支配 allocation 点，或者只是后续 use site 上的候选值。
  - 常见于 `allocate_unbounded()`、ABI encode、external call returndata buffer、helper 返回的 open-ended buffer。
- `notdec_evm_finalize_alloc(base, size)`
  - 对应 Solidity 里更新 free memory pointer 的动作，例如 `mstore(0x40, add(base, size))`。
  - 它记录底层写回语义，比 metadata 更贴近真实 IR 形状。
  - 它不表示后续所有 use 都只能用这个 size；return / revert / log / call 仍然看自己的 base、size 参数。

这样不需要额外设计 size metadata。确定 size 时直接用 `notdec_evm_alloc(size)`；不能确定时用 `notdec_evm_alloc_unbounded()`，再把后续 free pointer 写回改成 `notdec_evm_finalize_alloc(base, size)`。

### 不再需要的通用 memory marker

下面这些 marker 不再作为目标架构的一部分：

```text
notdec_solidity_memory_write
notdec_solidity_memory_byte_write
notdec_solidity_memory_copy_write
notdec_solidity_memory_read
notdec_solidity_memory_consumer
notdec_solidity_memory_array_byte_write
```

原因：

- 它们基本是对原始 `evm_mstore` / `evm_mstore8` / copy / `evm_mload` / return / revert / log / call 参数的重复标注。
- 地址拆解和 base-relative offset 可以放在共享 C++ helper 里，不需要写回 IR。
- 后续 pass 本来就应该扫描自己的入口指令，例如 `evm_return`、`evm_revert`、`evm_logN`、`evm_call`。
- 真正应该写回 IR 的，是各语义 pass 的最终 rewrite，而不是中间 proof marker。

### MemoryConsumer 识别能力怎么保留

当前 `MemoryConsumer` 的价值是识别这些 EVM memory operand：

- `evm_return(mem, base, size)`
- `evm_revert(mem, base, size)`
- `evm_logN(mem, base, size, topics...)`
- `evm_call(..., inBase, inSize, outBase, outSize)`

这个能力要保留，但不应该由 `MemoryBufferPass` 插 `notdec_solidity_memory_consumer` marker。

更好的做法是把它变成共享 helper：

```cpp
enum class EvmMemoryUseKind {
  ReturnData,
  RevertData,
  LogData,
  CallInput,
  CallOutput,
};

struct EvmMemoryUse {
  llvm::CallBase *Call = nullptr;
  EvmMemoryUseKind Kind;
  llvm::Value *Base = nullptr;
  llvm::Value *Size = nullptr;
};
```

然后提供类似 helper：

```cpp
std::optional<EvmMemoryUse> getReturnMemoryUse(llvm::CallBase &Return);
std::optional<EvmMemoryUse> getRevertMemoryUse(llvm::CallBase &Revert);
std::optional<EvmMemoryUse> getLogMemoryUse(llvm::CallBase &Log);
llvm::SmallVector<EvmMemoryUse, 2> getExternalCallMemoryUses(llvm::CallBase &Call);
```

后续 pass 自己扫描对应 EVM 指令，并调用 helper：

- `AbiReturnPass` 扫 `evm_return`。
- `SolidityRevertPass` 扫 `evm_revert`。
- `EventLogPass` 扫 `evm_logN`。
- `ExternalCallPass` 扫 `evm_call` / `evm_staticcall` / `evm_delegatecall` / `evm_callcode`。

这样保留了之前的识别能力，但不需要通用 consumer marker。

## 具体做法

### 第一阶段：新增 allocation rewrite surface

新增两个 helper 函数声明：

```llvm
declare i256 @notdec_evm_alloc(i256)
declare i256 @notdec_evm_alloc_unbounded()
```

先只做最保守的 rewrite：

1. 找 `base = evm_mload(mem, 64)`。
2. 如果能在同一函数里确认 `mstore(mem, 64, base + size)`，并且 size 在 allocation 点可用：
   - 在 `base` 定义点插 `notdec_evm_alloc(size)`。
3. 否则：
   - 在 `base` 定义点插 `notdec_evm_alloc_unbounded()`。
4. 把安全范围内对原 `base` 的地址使用替换成 alloc 返回值。
5. 删除已改写的原始 `evm_mload(0x40)` 和 `evm_mstore(0x40, ...)`。

安全范围第一版限制：

- 不跨 PHI。
- 不合并多次 `mload(0x40)` reload。
- 不追 helper 参数。
- 只替换明确由当前 `base` 派生的地址表达式。
- 不要求 size 精确覆盖所有后续 offset，允许一定越界访问。

### 第二阶段：把 MemoryConsumer marker 迁成 helper

新增 `EvmMemoryUse` helper。

迁移顺序：

1. `AbiReturnPass`
   - 停止读取 `notdec_solidity_memory_consumer`。
   - 直接扫描 `evm_return` 并调用 `getReturnMemoryUse()`。
2. `SolidityRevertPass`
   - 停止读取 `notdec_solidity_memory_consumer`。
   - 直接扫描 `evm_revert` 并调用 `getRevertMemoryUse()`。
3. `EventLogPass`
   - 停止读取 `notdec_solidity_memory_consumer`。
   - 直接扫描 `evm_logN` 并调用 `getLogMemoryUse()`。
4. `ExternalCallPass`
   - 停止读取 `notdec_solidity_memory_consumer`。
   - 直接扫描 call opcode 并调用 `getExternalCallMemoryUses()`。

迁移过程中先保留旧 marker 插入和 oracle，确认专用 pass 的最终 semantic marker 数量不退。

### 第三阶段：把 write / read / copy marker 迁成 helper

新增共享 helper，用于从原始 EVM memory 指令中提取 base-relative 信息：

```cpp
std::optional<RelativeMemoryWrite>
matchMStoreRelativeToBase(llvm::CallBase &MStore, llvm::Value *Base);

std::optional<RelativeMemoryWrite>
matchMStore8RelativeToBase(llvm::CallBase &MStore8, llvm::Value *Base);

std::optional<RelativeMemoryCopy>
matchCopyRelativeToBase(llvm::CallBase &Copy, llvm::Value *Base);

std::optional<RelativeMemoryRead>
matchMLoadRelativeToBase(llvm::CallBase &Load, llvm::Value *Base);
```

然后逐步让这些 pass 不再读取通用 marker：

- `AbiReturnPass`
- `SolidityRevertPass`
- `EventLogPass`
- `ExternalCallPass`
- `StorageAddressingPass`

### 第四阶段：删除通用 memory marker

当所有消费者都迁完后，删除这些 marker 的插入逻辑和 oracle：

```text
notdec_solidity_memory_write
notdec_solidity_memory_byte_write
notdec_solidity_memory_copy_write
notdec_solidity_memory_read
notdec_solidity_memory_consumer
notdec_solidity_memory_array_byte_write
```

同时清理只用于证明“消费了 memory marker”的专用 proof marker。

最终只保留真正的高层 semantic rewrite marker。

## 实现记录 2026-06-02

### 本轮完成内容

本轮先落地 allocation rewrite surface，同时保留旧通用 memory marker 作为兼容层，避免 ABI return / external call / event 等识别一次性断掉。

具体改动：

- `src/Passes/evm/MemoryBufferAnalysis.cpp:17`
  - 增加 `NumMemoryFinalizeAllocations` 统计。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:183`
  - 增加 `getOrDeclareFunction()`，用于声明非 void 的 `notdec_evm_alloc*`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:219`
  - 增加 `rewriteAllocation()`。
  - size 在 allocation 点可用时插入 `notdec_evm_alloc(size)`。
  - size 不支配 allocation 点时插入 `notdec_evm_alloc_unbounded()`，并在原 free pointer 写回点插入 `notdec_evm_finalize_alloc(base, size)`。
  - 替换原 `evm_mload(0x40)` 的 uses，并删除已改写的 `evm_mload(0x40)` / `evm_mstore(0x40, ...)`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:633`
  - `MemoryBufferRewritePass::run()` 仍先插旧 marker，再做 allocation rewrite。这样后续 pass 暂时还能消费旧 marker。
- `src/Passes/evm/SolidityPatterns.cpp:1276`
  - `isFreeMemoryPointerStore()` 同时把 `notdec_evm_finalize_alloc` 当作 free pointer 写回边界。
- `src/Passes/evm/SolidityPatterns.cpp:1284`
  - 增加 `isFreeMemoryAllocationBase()`。
  - `isSameOrReloadedFreeMemoryBase()` 支持 `evm_mload(0x40)` 和 `notdec_evm_alloc*` 之间的等价，修复 rewrite 后 ABI / external call 匹配不到同一个 base 的问题。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:48`
  - revert memory consumer 不再读取 `notdec_solidity_memory_consumer`，直接使用 `evm_revert(mem, base, size)` 参数插专用 marker。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:112`
  - event allocation 查找支持 `notdec_evm_alloc(size)` 和 `notdec_evm_finalize_alloc(base, size)`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:243`
  - event memory consumer 不再读取 `notdec_solidity_memory_consumer`，直接使用 `evm_logN(mem, base, size, ...)` 参数插专用 marker。
- `src/Passes/PassManager.cpp:289`
  - 调整 EVM pass 顺序为 checked-bounds / storage / packed-storage 先跑，之后再跑 `MemoryBufferRewritePass`。
  - 原因是 checked-bounds 和 storage pass 仍需要原始 IR 形状，MemoryBuffer 现在会删除 free pointer 写回。
- `test/evm/solidity-patterns/manifest.json:5014`
  - 更新 `24574_19760246_e537c886f5_6e80990d311f` 的 external call output read oracle。
  - 新 rewrite 后同一个 output buffer base 能和后续 reads 对齐，真实多识别 2 个 output word read 和 1 个 decode buffer。

### 验证

构建：

```bash
cmake --build ./build --target all -j4
```

Solidity patterns：

```bash
python3 test/run_evm_solidity_patterns_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/evm/solidity-patterns/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-solidity-patterns-memory-refactor
```

结果：

- `99 passed, 0 failed`
- checked-bounds 汇总保持原口径：
  - `memory_allocation_bounds`: 69
  - `memory_allocation_pointer_bounds`: 63
  - `cfg_rewrites`: 831
- 新 allocation rewrite 在本轮 suite 输出中命中：
  - `notdec_evm_alloc`: 564
  - `notdec_evm_alloc_unbounded`: 439
  - `notdec_evm_finalize_alloc`: 439

### 复杂度和维护成本

- 实现效果：7/10。free memory pointer 已经有真实 IR rewrite，不再只是 marker；但 write/read/copy marker 还保留，仍是过渡态。
- 复杂度：6/10。新增的 alloc/finalize 形态比较直接，但 pass 顺序需要更明确：compiler guard 和 storage 要先看原始 IR。
- 后期维护成本：6/10。`isSameOrReloadedFreeMemoryBase()` 现在兼容 raw free pointer 和 alloc call，能降低后续 pass 迁移成本；但旧 marker 和新 alloc 形态并存，后续要尽快收敛。

### 下一轮候选

按价值排序：

1. 把 `AbiReturnPass` 的 return consumer 迁成直接读 `evm_return`，并把动态数组 helper 里的 `Consumer` 参数改成 base / size，去掉对 `notdec_solidity_memory_consumer` 的依赖。
2. 把 `ExternalCallPass` 的 input / output consumer 迁成直接读 `evm_call*` 参数，保留现有 copy/read marker 兼容。
3. 把 memory allocation 专用消费者从旧 `notdec_solidity_memory_allocation` 迁到 `notdec_evm_alloc*` / `notdec_evm_finalize_alloc`，然后删除旧 allocation marker。
4. 把 write / copy / read marker 拆成共享 C++ helper，先从 external call output read 和 ABI return data write 两个高频路径开始。
