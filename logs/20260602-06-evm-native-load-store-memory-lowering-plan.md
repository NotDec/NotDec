# EVM memory helper 改成原生 load/store 计划

## 原始 prompt

按照这个规划，考虑一下怎么对接类型推理比较合适？比如，evm底层指令的mload和mstore是不是不需要额外指定什么内存是哪个对象？那么其实可以考虑转换为真正的LLVM的store和load指令？

---

好像模块顶层那个datalayout是不是可以描述大端序语义。在那边设置了应该就没问题。其次，llvm的memory也是byte addressed吧，非对齐访问应该也没问题。对于地址转换的问题，直接在每个mload或者store操作前，把操作数直接用inttoptr转换即可。对于alloc object，应该没什么关系，没太听懂什么问题。总的来说，似乎转换为这种load和store形式也没什么问题？

---

当前类型恢复输入的底层无类型IR，就是会主动处理这种inttoptr指令的，这样会把对应的addr或者数字在PNDiff层标注为指针类型，这个完全没问题，就应该裸的inttoptr。从base的角度，本质上就是以前那些base找到了顶层内存的值，对应到现在这种模式就是找不到base的话就是base是顶层内存。总的来说不需要考虑刻意去生成getelementptr指令。对于memory 初始为0的问题，wasm那边也有类似的问题，给所有函数都标 null_pointer_is_valid 属性就可以了。

---

没必要验证了，datalayout设置了就没什么事吧。当前目标是，重构为原生load和store指令，去掉所有mstore和mload。先把相关的内容，详细写到一个新的log/日志里面，把刚才聊到的所有的考量和我的解释都写进去，防止后面再出现这种改动的相关顾虑。

---

把阶段一改一下，改为修改那边evm2llvm项目，让那边支持两种模式，一种是不创建全局的内存对象，都通过inttoptr转为指针类型然后load/store。这种也是我们主项目链路用的模式。另外一种是创建全局内存对象，然后相关load和store前的转指针都变成对内存对象的getelementptr指令。这种模式可以通过evm2llvm那边的binary的命令行接口开启，但是主项目链路暂时不使用。总的来说直接从源头消除mload mstore的专门的external call，然后再继续往后推，迁移后面的其他pass之类的部分


## 背景

EVM 链路后续要接入类型恢复。当前 IR 里 memory 读写还是 helper：

```llvm
%v = call i256 @evm_mload(ptr %mem, i256 %addr)
call void @evm_mstore(ptr %mem, i256 %addr, i256 %value)
call void @evm_mstore8(ptr %mem, i256 %addr, i256 %value)
```

这对 Solidity pattern pass 做局部匹配还可以，但对类型恢复不够自然。NotDec 当前类型恢复本来就处理 LLVM 原生 `load/store/inttoptr`。特别是 PNDiff 层会根据 `inttoptr` 把对应的整数地址或常量标成 pointer 类型，所以 EVM memory 更适合降成普通 LLVM memory IR。

当前目标是重构 EVM memory lowering：

- 去掉 `evm_mload`。
- 去掉 `evm_mstore`。
- 去掉 `evm_mstore8`。
- 改成裸 `inttoptr` 加 LLVM 原生 `load/store`。
- 这个改动应优先在 evm2llvm 源头做，避免主项目再额外清理 `evm_mload/mstore` external call。

## 关键判断

### 1. 不需要额外指定“这是哪个 memory object”

之前有一个顾虑：如果直接把地址转成 pointer，类型恢复是否还能知道访问属于哪个 memory object。

结论：不需要额外指定。

原因：

- 类型恢复当前底层无类型 IR 就会主动处理 `inttoptr`。
- `inttoptr i256 %addr to ptr` 会让 `%addr` 在 PNDiff 层获得 pointer 语义。
- 如果 `%addr` 来自 `notdec_evm_alloc(size)`，那么分配结果自然是地址来源。
- 如果 `%addr` 不是某个明确 alloc 派生出来的地址，那么它就是顶层 EVM memory 里的地址。

也就是说：

- 找到 alloc object：地址值本身已经带着来源。
- 找不到 alloc object：base 就是顶层 memory。

不需要再人为构造 `base + offset` 的对象字段访问。

### 2. 不需要刻意生成 getelementptr

之前考虑过这种形式：

```llvm
%base.ptr = inttoptr i256 %base to ptr
%field.ptr = getelementptr i8, ptr %base.ptr, i256 %off
store i256 %v, ptr %field.ptr
```

现在判断不需要这样做。

原因：

- EVM memory 地址本来就是整数地址。
- 类型恢复会处理 `inttoptr`，裸地址转 pointer 就是当前低层 IR 应该表达的事实。
- 强行拆成 base + GEP 会多引入一层模式，反而让 EVM memory lowering 需要先证明 base/offset。
- 当前 Memory Object 的 alloc rewrite 只是提供更好的地址来源，不应该要求所有访问都先归属到某个 object。

推荐 lowering 是更直接的：

```llvm
%p = inttoptr i256 %addr to ptr
store i256 %value, ptr %p, align 1
%v = load i256, ptr %p, align 1
```

### 3. datalayout 已经表达大端序

EVM IR 的 datalayout 已经是：

```llvm
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
```

这里的 `E` 表示 big endian。EVM `mload/mstore` 的 32 字节 word 语义可以通过这个 datalayout 交给 LLVM 表达。

因此不需要再额外为 `mload/mstore` 做字节序 wrapper。

### 4. LLVM memory 本身就是 byte-addressed

EVM memory 是 byte-addressed，LLVM memory 也是 byte-addressed。

非对齐访问也可以直接用 `align 1` 表达：

```llvm
store i256 %value, ptr %p, align 1
%v = load i256, ptr %p, align 1
```

所以类似 ABI selector 写到 offset 0、参数从 offset 4 开始、动态数据不是 32 字节对齐这些情况，不是阻止改成原生 load/store 的理由。

### 5. memory 初始为 0 的顾虑用 null_pointer_is_valid 处理

之前还有一个顾虑：EVM memory 可以从 0 地址读写，而 LLVM 默认可能把 null pointer 访问当 UB。

当前判断：

- wasm memory 那边也有类似问题。
- 给相关函数标 `null_pointer_is_valid` 属性即可。

这能避免 `inttoptr 0 to ptr` 后的 load/store 被 LLVM 当成非法 null 访问优化掉。

这个属性应该作为 EVM lowering 的配套要求，而不是保留 `evm_mload/mstore` 的理由。

## 期望 IR 形态

### mload

改写前：

```llvm
%v = call i256 @evm_mload(ptr %mem, i256 %addr)
```

改写后：

```llvm
%p = inttoptr i256 %addr to ptr
%v = load i256, ptr %p, align 1
```

### mstore

改写前：

```llvm
call void @evm_mstore(ptr %mem, i256 %addr, i256 %value)
```

改写后：

```llvm
%p = inttoptr i256 %addr to ptr
store i256 %value, ptr %p, align 1
```

### mstore8

改写前：

```llvm
call void @evm_mstore8(ptr %mem, i256 %addr, i256 %value)
```

改写后：

```llvm
%byte = trunc i256 %value to i8
%p = inttoptr i256 %addr to ptr
store i8 %byte, ptr %p, align 1
```

`mstore8` 写的是 value 的低 8 位，所以这里用 `trunc i256 to i8`。

## 和 Memory Object alloc 的关系

MemoryBufferPass 仍然有价值，但职责更清楚：

- 识别 free memory pointer。
- 生成 `notdec_evm_alloc(size)`。
- 生成 `notdec_evm_alloc_unbounded()`。
- 生成 `notdec_evm_finalize_alloc(base, size)`。

这些 alloc call 提供地址来源，帮助类型恢复知道某些地址来自分配。

但后续 memory read/write 不需要额外标“属于哪个 object”。它们统一 lowering 成：

```llvm
inttoptr addr -> load/store
```

如果 `addr` 由 alloc 结果计算出来，类型恢复自然能沿 SSA 关系看到来源。

如果 `addr` 是常量 0、32、64 或其他无法归属 alloc 的值，那就是顶层 EVM memory 地址。

## 和类型恢复的关系

这个重构的主要目的就是让 EVM memory 进入现有类型恢复路径。

类型恢复可直接消费：

- `inttoptr`：把整数地址纳入 pointer 语义。
- `load`：形成从 pointer 读某个 bitwidth 的约束。
- `store`：形成向 pointer 写某个 bitwidth 的约束。
- `notdec_evm_alloc*`：作为地址来源和对象边界候选。

这样比给 `evm_mload/mstore` 单独写一套类型约束更直接，也减少 EVM 专用逻辑。

## 和后续 Solidity 语义 pass 的关系

这次改动之后，后续 pass 不应该再依赖 `evm_mload/mstore` helper。

需要迁移的方向：

- ABI return：从 load/store 和 `evm_return` 绑定 return buffer。
- revert encoding：从 load/store 和 `evm_revert` 绑定 revert buffer。
- event：从 load/store 和 `evm_logN` 绑定 event data buffer。
- external call：从 load/store/copy 和 `evm_call*` 绑定 input/output buffer。
- public entry ABI 参数：从 calldata load/copy 和普通 memory store 推断动态参数对象。

短期可以通过 helper 函数统一兼容旧 IR 和新 IR；最终应该删除旧 `evm_mload/mstore` matcher。

## 实现路线

### 阶段 1：在 evm2llvm 源头改 memory lowering

优先修改 evm2llvm 项目，不在主项目里先加一个后处理 lowering pass。

目标是从源头消除 `evm_mload` / `evm_mstore` / `evm_mstore8` 这类专门 external call，让 evm2llvm 直接生成 LLVM 原生 memory IR。

支持两种模式。

模式 1：裸 `inttoptr` 模式。

- `evm_mload` -> `inttoptr + load i256 align 1`
- `evm_mstore` -> `inttoptr + store i256 align 1`
- `evm_mstore8` -> `trunc i256 to i8 + inttoptr + store i8 align 1`

这是主项目链路使用的默认模式。它不创建全局 memory object，每次 memory 访问都直接把 EVM 整数地址用 `inttoptr` 转成 pointer。

这个模式必须由 evm2llvm 自动给相关函数加 `null_pointer_is_valid`，不要等主项目后处理再补。

模式 2：全局 memory object + GEP 模式。

- evm2llvm 创建一个全局 memory object。
- `mload/mstore/mstore8` 前的地址转换不再用裸 `inttoptr`。
- 地址先转成全局 memory object 上的 `getelementptr`，再生成 `load/store`。

这个模式只通过 evm2llvm 那边 binary 的命令行接口开启。主项目 NotDec 链路暂时不使用它。它的价值是方便单独调试或比较“全局 memory object”表达，但不是当前类型恢复主路线。

模式 2 也应该给 EVM 相关函数加 `null_pointer_is_valid`，保持两个输出模式的函数属性一致。

### 阶段 2：主项目接收新 IR，并迁移现有 Solidity pass matcher

evm2llvm 源头不再生成 `evm_mload/mstore` 后，主项目里的 Solidity pattern pass 需要继续往后推，逐步迁移到原生 load/store。

迁移原则：

- 不再新增 `notdec_solidity_memory_write/read/consumer` 这类 marker。
- 用统一 helper 识别：
  - pointer 是否来自 `inttoptr i256 addr`。
  - load/store 的地址值。
  - store 的 value。
  - i256 store 和 i8 store。
- copy 类 helper 可以暂时保留，后续再考虑降成 memcpy 或 range fact。

### 阶段 3：删除 mload/mstore helper 声明

当 evm2llvm 和主项目 pass 都迁完之后：

- 删除 `evm_mload` 声明。
- 删除 `evm_mstore` 声明。
- 删除 `evm_mstore8` 声明。
- manifest oracle 不再统计这些 helper 或基于它们的 marker。

## 不做什么

本轮不设计额外 memory object metadata 作为 pass 间主接口。

主项目链路不使用 GEP 模式。GEP 只作为 evm2llvm binary CLI 可选模式存在。

本轮不要求每个 memory access 归属到某个 alloc object。

本轮不把 calldata/returndata/storage 一起改成 LLVM load/store；先处理 EVM memory 的 `mload/mstore/mstore8`。

## 风险

- 现有 Solidity matcher 大量依赖 `evm_mload/mstore`，需要迁移，否则会直接掉 oracle。
- LLVM 优化可能更积极处理 load/store，所以 EVM pass 顺序要注意：需要 matcher 看旧 helper 的阶段必须在 lowering 前完成，或者 matcher 先完成迁移。
- `mcopy/calldatacopy/returndatacopy` 还保留 helper 时，range 写入和普通 store 的统一建模需要后续再补。

## 判断标准

- EVM 输出 IR 中不再出现 `evm_mload` / `evm_mstore`。
- `evm_mstore8` 也尽量消失；如果有暂留，必须明确原因。
- EVM 相关函数带 `null_pointer_is_valid`。
- 类型恢复能看到 `inttoptr`、`load`、`store`，并把地址值纳入 pointer 约束。
- Solidity patterns suite 里的 ABI return、revert、event、external call oracle 迁移到新 IR 形态后通过。

## 实现记录

本轮按阶段 1 和阶段 2 的最小闭环实现。阶段 3 没做全量删除，因为主项目测试样例仍有旧 helper IR，需要继续兼容。

### evm2llvm 源头 lowering

- `external/NotDec-evm2llvm/include/notdec-evm2llvm/LlvmLowerer.h:17` 新增 `EvmMemoryModel`，默认 `IntToPtr`。
- `external/NotDec-evm2llvm/tools/evm2llvm.cpp:20` 增加 `--memory-model inttoptr|global-array`，默认 `inttoptr`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:99` 增加 `memoryGlobal()`，给 `global-array` 模式创建 `@notdec_evm_memory`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:124` 增加 `memoryPointer()`，在默认模式生成 `inttoptr`，在调试模式生成 `getelementptr`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:438` 将 `MLOAD` 降成 `load i256, align 1`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:772` 将 `MSTORE` 降成 `store i256, align 1`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:777` 将 `MSTORE8` 降成 `trunc i256 to i8` 后 `store i8, align 1`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:157` 给生成函数加 `null_pointer_is_valid`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:15` 删除 `evm_mload` / `evm_mstore` / `evm_mstore8` 声明。

### 主项目 matcher 迁移

- `include/notdec/Passes/evm/SolidityPatternUtils.h:108` 增加 `EvmMemoryLoad` / `EvmMemoryStore`，统一表达旧 helper 和新原生 load/store。
- `src/Passes/evm/SolidityPatterns.cpp:81` 实现 `matchEvmMemoryLoad()`，支持 `evm_mload` 和 `load i256` from `inttoptr`。
- `src/Passes/evm/SolidityPatterns.cpp:98` 实现 `matchEvmMemoryStore()`，支持 `evm_mstore`、`evm_mstore8`、`store i256` 和 `store i8` to `inttoptr`。
- `src/Passes/evm/SolidityPatterns.cpp:1275` 让 `isFreeMemoryPointerLoad()` 使用统一 memory load helper。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:51` 将 memory read/write 原始节点从 `CallBase*` 放宽为 `Instruction*`，因为新 IR 的 load/store 不是 call。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:371` 到 `472` 改成从统一 helper 收集 allocation、word write、word read、byte write 和 array byte write，copy 类 helper 仍保留旧 call 识别。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:433` 到 `453` 让 ABI return 动态数组 copy loop 支持原生 load/store。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:543` 到 `584` 让 converted copy loop 里的 source cursor load 支持原生 load。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:589` 到 `620` 让 memory-to-memory helper copy 识别原生 load/store。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:305` 到 `365` 让 external call output word read 收集支持原生 memory load。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:590` 到 `625` 让 external call output word read marker 能从原生 load 插入。

### 验证

- 主项目构建：
  - `cmake --build ./build --target notdec -j4`
  - 通过。
- evm2llvm 源头测试：
  - `cmake -S external/NotDec-evm2llvm -B external/NotDec-evm2llvm/build-tests -G Ninja -DLLVM_DIR=/sn640/NotDec/llvm-22.1.0.obj/lib/cmake/llvm -DNOTDEC_EVM2LLVM_ENABLE_TESTS=ON -DNOTDEC_EVM2LLVM_ENABLE_GIGAHORSE_TESTS=OFF`
  - `cmake --build external/NotDec-evm2llvm/build-tests -j4`
  - `ctest --test-dir external/NotDec-evm2llvm/build-tests --output-on-failure`
  - 结果：33/33 通过。
- evm2llvm memory 模式检查：
  - `external/NotDec-evm2llvm/build/bin/evm2llvm --facts external/NotDec-evm2llvm/test/fixtures/state -o /tmp/notdec-evm-native-memory/state-inttoptr.ll`
  - `external/NotDec-evm2llvm/build/bin/evm2llvm --facts external/NotDec-evm2llvm/test/fixtures/state -o /tmp/notdec-evm-native-memory/state-global.ll --memory-model global-array`
  - 默认输出包含 `store i256 96, ptr inttoptr (i256 64 to ptr), align 1` 和 `load i256, ptr inttoptr (i256 64 to ptr), align 1`。
  - `global-array` 输出包含 `@notdec_evm_memory` 和 `getelementptr`。
  - 两种输出函数都带 `null_pointer_is_valid`。
- 主项目 EVM suite：
  - `ctest --test-dir build -R evm --output-on-failure`
  - 结果：`notdec.evm.solidity_patterns` 和 `notdec.evm.solidity_rewrite` 2/2 通过，总耗时 198.38 秒。
- apehex smoke：
  - 最近批次 `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260602-evm2llvm-train-batch686/outputs` 不足 100 个 `.ll`，实际跑 94 个。
  - 命令等价于对每个样本跑 `./build/bin/notdec <input.ll> -o /tmp/notdec-evm-native-memory-apehex-smoke/<name>.ll --tr-level=0`。
  - 结果：94/94 通过，0 失败，用时 67.85 秒。

### 评分

- 实现效果：8/10。evm2llvm 源头已经不再生成 `evm_mload/mstore/mstore8`，主项目关键 memory buffer、ABI return、external call 路径已兼容新 IR；旧 helper IR 也保持通过。
- 复杂度：6/10。新增一个统一 memory access view，成本可控；但 `SolidityPatterns.cpp` 里仍有部分旧 helper 专用匹配，后续删除 helper 时还要继续清。
- 维护成本：6/10。短期兼容旧/新两种 IR 会多一些分支，但集中在 helper 和少数 fallback，后续等测试样例全部迁移后可以删除旧 helper 分支。

更好的后续方案：继续把 `SolidityPatterns.cpp` 里 storage scratch keccak、revert raw write 等剩余 `evm_mload/mstore` 直接匹配迁到统一 helper；再更新测试输入和 oracle，最后删除旧 helper 兼容分支。

## 2026-06-03 追加实现记录：删除旧 memory helper 兼容

本轮把主项目中剩余的 `evm_mload` / `evm_mstore` / `evm_mstore8` 兼容路径去掉。
测试 IR 已经迁到原生 `load` / `store`，所以后续不再把旧 helper 当正常输入维护。

修改点：

- [include/notdec/Passes/evm/SolidityPatternUtils.h:105](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatternUtils.h:105)
  更新 `EvmMemoryLoad` / `EvmMemoryStore` 注释，明确 EVM memory 现在是
  `inttoptr` 后的原生 `load` / `store`。
- [src/Passes/evm/SolidityPatterns.cpp:85](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:85)
  删除 `matchEvmMemoryLoad()` 对 `evm_mload` 的兼容分支，只接受 `load i256`。
- [src/Passes/evm/SolidityPatterns.cpp:97](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:97)
  删除 `matchEvmMemoryStore()` 对 `evm_mstore` / `evm_mstore8` 的兼容分支，
  只接受 `store i256` / `store i8`。
- [src/Passes/evm/SolidityPatterns.cpp:1313](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:1313)
  删除 `isSameValue()` 里旧 `evm_mload` call 等价判断，统一走 native load 地址比较。
- [src/Passes/evm/SolidityPatterns.cpp:3726](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:3726)
  `matchArrayBounds()` 用 `matchEvmMemoryLoad()` 判断 memory array length。
- [src/Passes/evm/SolidityPatterns.cpp:3906](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:3906)
  `hasMemoryArrayAllocationComputation()` 改为通过 `EvmMemoryStore` 判断 header store。
- [src/Passes/evm/SolidityPatterns.cpp:4175](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:4175)
  `hasBytesAllocationStores()` 改为通过 `EvmMemoryStore` 判断 length store 和 free pointer store。
- [src/Passes/evm/SolidityPatterns.cpp:4219](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:4219)
  `hasAllocationHelperHeaderStore()` 改为通过 native store 判断 helper 返回指针的 header 写入。
- [src/Passes/evm/SolidityPatterns.cpp:4261](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:4261)
  `hasVoidMemoryAllocationHelperCall()` 从 native store 收集 header 指针。
- [src/Passes/evm/SolidityPatterns.cpp:4698](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:4698)
  `getMemoryPointerLoadSlot()` 改为从 native load 取地址。
- [src/Passes/evm/SolidityPatterns.cpp:4703](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:4703)
  `findFreeMemoryPointerStore()`、`findUniformFreeMemoryPointerStore()`、
  `findMemoryPointerStoreForLoad()`、`findMemoryPointerStoreSlot()`、
  `findMemoryPointerStoreToSlot()` 都改为匹配 native store。
- [src/Passes/evm/SolidityPatterns.cpp:5964](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:5964)
  `matchStorageScratchKeccak()` 改为从 native store 读取 scratch key / base slot。
- [src/Passes/evm/SolidityPatterns.cpp:6005](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:6005)
  `matchStorageArrayDataKeccak()` 改为从 native store 读取 base slot。
- [test/run_evm_solidity_patterns_suite.py:19](/sn640/NotDec/test/run_evm_solidity_patterns_suite.py:19)
  删除 runner 对旧 `evm_mload` / `evm_mstore` 文本形态的 fallback。
- [test/run_evm_solidity_patterns_suite.py:366](/sn640/NotDec/test/run_evm_solidity_patterns_suite.py:366)
  `count_revert_kinds()` 优先读取具体 revert rewrite marker，避免 native IR 形态下靠裸
  block 文本误分 `panic` / `error_string`。
- [test/evm/solidity-patterns/manifest.json](/sn640/NotDec/test/evm/solidity-patterns/manifest.json)
  按 native IR 当前输出更新 14 个单例 oracle；全局 `checked_bounds_audit` 仍保持
  actual 和 expected 一致。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json)
  更新 `0334_19494307_668d201319_1354ce2e324d` 的 checked-bounds 计数。

验证：

- `cmake --build ./build --target all -j4` 通过。
- `rg "evm_mload|evm_mstore|evm_mstore8|EVM_MLOAD|EVM_MSTORE" src include test/run_evm_solidity_patterns_suite.py -n`
  无输出。
- `ctest --test-dir build -R 'notdec.type_recovery.evm.tr_level_2|notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  3/3 通过，总耗时 `189.92s`。
- fortune 当前关注用例同口径：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M'
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll
  -o /tmp/notdec-fortune-native-memory-no-helper-compat.ll --tr-level=2
  --frozen-tr-input-ir
  --dump-htypes=/tmp/notdec-fortune-native-memory-no-helper-compat.htypes`
  通过，`elapsed=12.58 user=12.18 sys=0.39 maxrss=852300`。

评分：

- 实现效果：9/10。主项目代码和 runner 已不再识别旧 EVM memory helper，测试 IR 也已迁到
  native load/store。
- 复杂度：5/10。删除兼容分支后 matcher 更直接，但 Solidity pattern oracle 跟随 native IR
  重新计数，维护时要注意这些数字已经不是旧 helper 口径。
- 维护成本：5/10。后续重点可以转到类型恢复接 native load/store，不需要再维护旧 helper 双路径。

## 追加实现记录：alloc helper 改成 calloc

按新的判断，EVM free memory allocation 不再生成 `notdec_evm_alloc` /
`notdec_evm_alloc_unbounded`，而是在源头直接生成更通用的 `calloc` /
`calloc_unbounded`。旧名字只保留 matcher 兼容，不新增重命名 pass。

### 修改点

- `src/Passes/evm/MemoryBufferAnalysis.cpp:217` 的 `rewriteAllocation()` 改为生成
  `calloc(i256 1, i256 size)` 或 `calloc_unbounded()`，返回值是 `ptr`，再用
  `ptrtoint ptr ... to i256` 接回 EVM 地址表达；`calloc` 返回值标记 `noalias`。
- `src/Passes/evm/SolidityPatterns.cpp:1287` 的
  `isFreeMemoryAllocationBase()` 继续识别旧 `notdec_evm_alloc*`，并新增识别
  `ptrtoint(call @calloc(...))` 和 `ptrtoint(call @calloc_unbounded())`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:44` 新增
  `getSizedAllocationSize()`，event log 的 event data allocation 识别同时支持旧
  `notdec_evm_alloc(size)` 和新 `ptrtoint(call @calloc(1, size))`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3639` 的
  `isHeapAllocationCall()` 识别 `malloc`、`calloc`、`calloc_unbounded`，同时保留旧
  `notdec_evm_alloc*` 兼容。
- `notdec_evm_finalize_alloc(base, size)` 暂时保留，用于 allocation 时 size 还不可用、
  后面才确认大小的 unbounded 场景。

### 验证

- `cmake --build ./build --target all -j4`：通过。
- 单样例检查：
  `./build/bin/notdec test/evm/solidity-patterns/cases/0450_19495071_6b6c9447e0_6344565f4b31.ll -o /tmp/notdec-calloc-alloc-check.ll --tr-level=0`
  生成结果包含 `call ptr @calloc(i256 1, i256 ...)`、`ptrtoint ptr ... to i256`
  和 `call ptr @calloc_unbounded()`；没有再生成 `notdec_evm_alloc*`。
- apehex smoke：最近批次抽样 77 个 `.ll`，逐个执行
  `./build/bin/notdec <input.ll> -o /tmp/notdec-calloc-apehex-smoke/<name>.ll --tr-level=0`，
  77/77 通过，用时 50.02 秒。
- `ctest --test-dir build -R 'notdec.evm' --output-on-failure`：2/2 通过，总耗时
  195.16 秒。
- 试跑过
  `ctest --test-dir build -R 'notdec.evm|notdec.type_recovery.llvm_ir.tr_level_2' --output-on-failure`。
  EVM 测试通过；`notdec.type_recovery.llvm_ir.tr_level_2` 有 4 个非 `calloc`
  相关样例 diff，表现为既有的透明/递归结构输出变化，本轮没有改 oracle。
