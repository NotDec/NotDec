# EVM 高层类型直接重写 Pass 规划

## 原始 prompt

MappingMatcher是什么，是storage吗，那一块单独处理，这里暂时不用管。把当前的规划写到一个logs/文件里面。特别开头要强调一下之前的考量：calldata、memory已经完全统一处理了，IR形状都是LLVM的load store了，所以统一处理是完全没问题的。而且也不要搞什么单独识别，单独rewrite在不同的pass，就按照类型处理，而不是按什么memory，calldata单独分

## 背景

当前 EVM 路线已经把 calldata 和 memory 访问基本整理到 LLVM IR 形状上：`evm_calldataload`、`evm_calldatacopy`、EVM memory access 等低层 helper 被改写成普通 `load`、`store`、`llvm.memcpy`，并把这些访问交给类型推理恢复 HType。

因此后续高层类型分析不应该再按 calldata、memory 分别写一套 pass，也不应该让不同 pass 各自扫描访问模式、各自 rewrite 同一种类型。现在更合适的方向是：先让低层访问规范化和类型推理完成，再基于现有 HType 结果直接重写更确定的高层对象。

也就是说，重写依据应该是“这是什么类型/布局”，不是“它来自 calldata 还是 memory”。不要因为对象落在不同 buffer 里，就把同一种类型拆成多套 pass。

Storage/mapping 不放在这份规划里。它们已经有单独的 storage 高层 rewrite 路线，并且现在也能从 HType 里拿到类型结果；这里不再重复规划 storage slot、mapping hash、storage array、storage bytes/string 这些内容。

storage 和非 storage 分成两个 pass，主要是因为访问入口和可替换的 IR 形状不一样：

- 本规划处理普通 LLVM `load` / `store` / `memcpy` 这一侧能确认的非 storage 高层对象访问。
- storage 规划处理 `sload` / `sstore` 及其 path rewrite，需要保留 mapping hash、slot、packed、bytes/string storage 编码这些 storage 专属规则。
- 两边都读取现有 HType，不额外造一层 HType 视图，也不在本规划里混合处理 storage 语义。

## 目标

新增一个高层类型重写 pass，暂名：

```text
EvmHighLevelTypePass
```

它的职责是读取类型恢复产出的现有 HType，匹配到能认出的高层对象后直接重写成更高层的 helper / intrinsic。它不负责重新把 calldata 或 memory helper rewrite 成 LLVM IR，不负责从访问模式反推类型，也不负责 storage/mapping。

这个 pass 应该从 HType 中读取并直接重写：

- struct / tuple-like object 的字段访问。
- HType 里边界明确的 record field load/store。
- 后续确认稳定后，再处理 dynamic bytes/string、dynamic array、static array。

第一版先做最确定的字段访问。array、bytes/string 先不因为 HType 里出现 `ArrayType` 就直接当 Solidity array 或 bytes/string rewrite。

## 基本设计

直接基于现有 HType 做重写，不再单独造一层视图。

这里不先把对象归一遍再处理。能直接确认的就直接重写，不能确认的就保留原样。

storage 相关的东西不放进这套 rewrite。storage 的 path、mapping、array、bytes/string 现在走另一条路线，不在这个 pass 里混进 storage 语义。

## 和现有 Pass 的关系

现有 pass 继续负责低层规范化和局部证据：

- `MemoryBufferRewritePass`：把 EVM memory 访问整理成 LLVM memory 访问。
- `EvmCalldataAccessPass`：把 calldata 访问整理成 LLVM load/memcpy，并提供 min-size checked alias。
- `CheckedBoundsPass`：识别 panic bounds 和 arithmetic guard，提供 bounds 证据。
- ABI return/revert/event/external call 不是第一版主线。ABI payload 往往要到返回、revert、log 或 external call 的使用点才知道它是不是 ABI 编码；这里不把 ABI 形状当成固定高层类型提前匹配。

`EvmHighLevelTypePass` 不应该再去匹配旧的 `evm_calldataload` / `evm_mload` 形状，也不应该用规范化后的 `load/store/memcpy` 访问模式反推类型。它基于现有 HType 和类型恢复记录下来的访问证据，匹配到了就直接 rewrite，不先走一层单独的标记或分类。这样可以避免每个语义 pass 都重新猜 calldata/memory 布局。

storage 侧也已经有自己的高层 rewrite pass。两边的关系可以这样定：

- `EvmHighLevelTypePass` 直接重写非 storage 高层对象。
- `EvmStorageHighLevelRewritePass` 继续处理 storage path rewrite。
- 两边都依赖同一套 HType 结果，但不共享 matcher。
- 后面如果有输出侧需要同时展示两边结果，读取 rewrite 后的 IR 即可，不在这里提前合并 storage 语义。

建议位置：

```text
TypeRecovery
EvmHighLevelTypePass
后续 cleanup
```

这个 pass 固定放在 TypeRecovery 后。它读取 HType，不是类型推理前的 evidence pass。如果发现 HType 信息不足，应回到低层证据生产或类型推理本身补输入，而不是在这个 pass 里补访问模式匹配。

## 大致做法

这一版不追求把所有高层语义一次性拉平，只按确定性从高到低处理：

- record / struct / tuple-like 对象：按 HType 字段边界和偏移重写字段 load/store，不再保留成通用指针算术。
- static array：只有当 HType 能稳定给出元素类型、元素宽度、固定边界，并且访问 index 能对应到元素访问时，再重写元素 load/store。
- dynamic array：只有当 length 位置、data 起点、元素 index 关系都能由 HType 或已有确定证据确认时，再重写 length 和 elem 访问。
- dynamic bytes/string：只在长度、短数据、长数据这些 Solidity 编码事实明确时处理；不能只因为看到 byte array 形状就提升。
- ABI return / revert / event / external call：按需在各自使用点处理 ABI 编码，不作为这里的固定高层类型。能迁进来的，也应该是已经退化成上面这些确定对象访问的 rewrite。

这些重写都放在同一个 pass 里做。每一类都只保留一套判断和 rewrite 路径，不单独拆 matcher pass。

## 不做什么

- 不按 calldata、memory 分别做 dynamic bytes / array 识别 pass。
- 不扫描 `load/store/memcpy` 访问模式来推导类型；类型结论必须来自 HType。
- 不新增只给后续 pass 读取的 metadata/result/marker。匹配到就直接 rewrite，匹配不到就保留原 IR。
- 不把 ABI payload 当成一种提前匹配的高层类型。
- 不把 address、bool、uintN/intN 的值域规范化塞进这个 pass。它们可以作为 HType 或 value normalization 的输入/输出，但不是这个 pass 的主任务。
- 不删除已有 guard。cleanup 仍放到后续统一处理。

## 风险

- 如果 HType 结果还不够完整，这个 pass 只能保留原样或做一部分重写。不要在这里用访问模式补猜类型，否则会回到多套 matcher 的路线。
- 如果 pass 直接 rewrite 太多 IR，容易和现有 ABI return/revert/event pass 抢同一段 buffer。第一版应先挑 HType 明确、rewrite 语义稳定的对象做。

## 判断标准

- 同一种类型只实现一套重写逻辑。例如 dynamic bytes 不因为 calldata/memory 写两套 matcher。
- 基于 HType 能把 dynamic bytes/string、dynamic array、static array、struct/tuple-like object 直接 rewrite 出来。
- ABI return、revert、event、external call 不作为第一版主线；ABI 编码按使用点处理，不提前当成 HType 高层对象。
- storage/mapping 不被这个 pass 混进来，避免把完全不同的 storage layout 规则揉到一起。
- patterns suite 和 type recovery suite 不能出现明显性能下降；第一版至少对当前关注用例做同口径耗时对比。

## 当前实现记录：memory record field load/store 第一版

本轮先落地一个窄闭环：只处理 HType 已经确认的 EVM memory record 字段读写。暂不碰 dynamic bytes/string、array，也不按 ABI payload 场景单独匹配。

实现内容：

- [include/notdec/Passes/evm/SolidityPatterns.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatterns.h:76)：新增 `EvmHighLevelTypePass` 声明，注释明确第一版只重写已恢复 HType memory record 字段访问。
- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:78)：新增 `EVMLoadEvidence`，和已有 `EVMStoreEvidence` 一样，只把原始 load、地址和 bit size 暴露给后续 HType rewrite，不暴露 BinarySub 临时类型节点。
- [src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp:23)：新增 `getMemoryRecord()`，从 `MemoryDecl` 或 `MemoryType` 的 pointee 取现有 memory record，不新建 HType 视图。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2181)：`ConstraintsGenerator::recordLoad()` 在 pointer analysis 关闭时也保留 EVM load evidence，行为和 store evidence 对齐。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:3243)：`MLsubRecovery::genASTTypes()` 清空并填充 `EVMLoads`，只收集真实 `LoadInst`。
- [src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp:79)：`rewriteMemoryFieldStore()` 读取 `EVMStoreEvidence`，只有 256-bit store、地址是常量 memory offset、且 HType memory record 在该 offset 有字段时，才把原 store 改成 `evm.htype.field.store(0, offset, value)`。
- [src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp:118)：新增 `rewriteMemoryFieldLoad()`，读取 `EVMLoadEvidence`，同样只处理 256-bit、常量 offset、HType 字段已确认的 load，并改成 `evm.htype.field.load(0, offset)`。
- [src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp:158)：`run()` 直接读取 `TR.getResult()`、`TR.getEVMStoreEvidence()` 和 `TR.getEVMLoadEvidence()`，匹配到就 rewrite，匹配不到保留原 IR。这里先处理 store，再处理 load，避免 store evidence 还引用即将被删除的 load。
- [src/CMakeLists.txt](/sn640/NotDec/src/CMakeLists.txt:17)：接入新实现文件。
- [src/Passes/PassManager.cpp](/sn640/NotDec/src/Passes/PassManager.cpp:330)：第一版把 pass 放在 `EvmStorageHighLevelRewritePass` 之后。原因是实现时发现如果放在 storage rewrite 前，会删掉 storage matcher 仍需要的 scratch memory store，导致 storage mapping oracle 失效。后续只有在确认前面的 pass 不再依赖这些 store 形状后，才考虑前移。
- [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:277)：rewrite suite 增加 `htype_field_store_calls` 和 `htype_field_load_calls` 计数。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:98)：`memory_record_field_high_level_rewrite` 复用 `04_evm_memory_helpers.ll`，固定两个字段 store 加一个 declaration、一个字段 load 加一个 declaration。

这次有一个计划未写清的点：pass 位置不能只按“TypeRecovery 后”理解。只要前面的 pass 还依赖原始 memory store 形状，新 pass 就不能提前删除这些 store。当前先作为最后的 HType-based cleanup。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/evm/cases/04_evm_memory_helpers.ll -o /tmp/notdec-htype-field-load.ll --tr-level=2 --frozen-tr-input-ir` 通过，输出 2 个 `evm.htype.field.store` call 和 1 个 `evm.htype.field.load` call。
- `llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-htype-field-load.ll -o /tmp/notdec-htype-field-load.bc` 通过。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-solidity-rewrite-htype-field-load-suite` 通过，`76 passed, 0 failed`。
- `ctest --test-dir build -R notdec.type_recovery.evm --output-on-failure` 通过。
- EVM 性能 smoke：`/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-high-level-type-load.ll --tr-level=2` 通过，本轮 `elapsed=17.98 rss_kb=952360`。同一用例近期记录为 `17.68s/958320KB`、`17.91s/958940KB` 和 `18.99s/955376KB`，未见明显退化。

评分：

- 实现效果：6/10。已经有一个直接 rewrite 闭环，覆盖 memory record field load/store；还没覆盖 dynamic bytes/string 和 array。
- 理解成本：5/10。新增一个 pass 和一个 helper intrinsic，规则很窄；但 pipeline 位置需要说明，避免误以为现在能提前替换所有 memory store。
- 维护成本：5/10。当前只依赖 HType memory record、EVMStoreEvidence 和 EVMLoadEvidence，后续扩到 array/bytes 前需要先确认 HType 里对应形状足够稳定。

下一步技术决策点：

- `25928_19774281_d048a8d52d_2758caa02f46.ll` 的 HType dump 里能看到 `top:256[]*`、`struct_13*[]*`、`typedef_1[]*` 这类非 storage array 形状，但它们目前更像类型恢复里的泛化 array / tail array，不直接等价于 Solidity dynamic array、static array 或 bytes/string。
- 如果下一步要 rewrite array，需要先决定：是先引入只表达 HType array 的泛用 helper，例如 `evm.htype.array.elem.*`，还是等更多 Solidity 语义证据把 dynamic/static array 和 bytes/string 区分清楚后再 rewrite。
- 当前不应直接把所有 `ArrayType` 当成 Solidity dynamic/static array。否则会把 tail-recursion normalization、ABI 临时 buffer、普通 memory array 混在一起。
