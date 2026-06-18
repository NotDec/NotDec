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

如果后续新增非 storage 高层类型重写 pass，它的职责应该是读取类型恢复产出的现有 HType，匹配到能认出的 Solidity/EVM 高层对象后直接重写成更高层的 helper / intrinsic。它不负责重新把 calldata 或 memory helper rewrite 成 LLVM IR，不负责从访问模式反推类型，也不负责 storage/mapping。

这个 pass 应该从 HType 中读取并直接重写：

- 后续确认稳定后的 dynamic bytes/string、dynamic array、static array。
- 使用点已经明确的 ABI return / revert / event / external call payload。

普通 HType record field load/store 不作为这个 pass 的目标。它只是类型恢复内部的字段布局，不是专门的 Solidity/EVM 高层语义；如果改写成 `evm.htype.field.*`，本质上只是换一种 marker，不值得放进 IR。

array、bytes/string 先不因为 HType 里出现 `ArrayType` 就直接当 Solidity array 或 bytes/string rewrite。

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

非 storage 高层类型 rewrite 不应该再去匹配旧的 `evm_calldataload` / `evm_mload` 形状，也不应该用规范化后的 `load/store/memcpy` 访问模式反推类型。它应该基于现有 HType 和明确语义证据，匹配到了就直接 rewrite，不先走一层单独的标记或分类。这样可以避免每个语义 pass 都重新猜 calldata/memory 布局。

storage 侧也已经有自己的高层 rewrite pass。两边的关系可以这样定：

- 非 storage 高层类型 rewrite 只处理有明确 Solidity/EVM 语义的对象。
- `EvmStorageHighLevelRewritePass` 继续处理 storage path rewrite。
- 两边都依赖同一套 HType 结果，但不共享 matcher。
- 后面如果有输出侧需要同时展示两边结果，读取 rewrite 后的 IR 即可，不在这里提前合并 storage 语义。

如果后续加这个 pass，建议位置：

```text
TypeRecovery
非 storage 高层类型 rewrite
后续 cleanup
```

这个 pass 固定放在 TypeRecovery 后。它读取 HType，不是类型推理前的 evidence pass。如果发现 HType 信息不足，应回到低层证据生产或类型推理本身补输入，而不是在这个 pass 里补访问模式匹配。

## 大致做法

这一版不追求把所有高层语义一次性拉平，只按确定性从高到低处理：

- 普通 record / struct / tuple-like 字段：只保留在 HType 结果里，不单独 rewrite 成 IR helper。
- static array：只有当 HType 能稳定给出元素类型、元素宽度、固定边界，并且访问 index 能对应到元素访问时，再重写元素 load/store。
- dynamic array：只有当 length 位置、data 起点、元素 index 关系都能由 HType 或已有确定证据确认时，再重写 length 和 elem 访问。
- dynamic bytes/string：只在长度、短数据、长数据这些 Solidity 编码事实明确时处理；不能只因为看到 byte array 形状就提升。
- ABI return / revert / event / external call：按需在各自使用点处理 ABI 编码，不作为这里的固定高层类型。能迁进来的，也应该是已经退化成上面这些确定对象访问的 rewrite。

这些重写都放在同一个 pass 里做。每一类都只保留一套判断和 rewrite 路径，不单独拆 matcher pass。

## 不做什么

- 不按 calldata、memory 分别做 dynamic bytes / array 识别 pass。
- 不扫描 `load/store/memcpy` 访问模式来推导类型；类型结论必须来自 HType。
- 不新增只给后续 pass 读取的 metadata/result/marker。匹配到就直接 rewrite，匹配不到就保留原 IR。
- 不把普通 HType record field load/store 改成 `evm.htype.field.*` 这种 generic helper。
- 不把 ABI payload 当成一种提前匹配的高层类型。
- 不把 address、bool、uintN/intN 的值域规范化塞进这个 pass。它们可以作为 HType 或 value normalization 的输入/输出，但不是这个 pass 的主任务。
- 不删除已有 guard。cleanup 仍放到后续统一处理。

## 风险

- 如果 HType 结果还不够完整，这个 pass 只能保留原样或做一部分重写。不要在这里用访问模式补猜类型，否则会回到多套 matcher 的路线。
- 如果 pass 直接 rewrite 太多 IR，容易和现有 ABI return/revert/event pass 抢同一段 buffer。第一版应先挑 HType 明确、rewrite 语义稳定的对象做。

## 判断标准

- 同一种类型只实现一套重写逻辑。例如 dynamic bytes 不因为 calldata/memory 写两套 matcher。
- 基于 HType 和明确语义证据，把 dynamic bytes/string、dynamic array、static array 这类真正高层对象直接 rewrite 出来。
- ABI return、revert、event、external call 不作为第一版主线；ABI 编码按使用点处理，不提前当成 HType 高层对象。
- storage/mapping 不被这个 pass 混进来，避免把完全不同的 storage layout 规则揉到一起。
- patterns suite 和 type recovery suite 不能出现明显性能下降；第一版至少对当前关注用例做同口径耗时对比。

## 已撤销：memory record field load/store rewrite

之前尝试过把 HType memory record 字段读写改成 `evm.htype.field.load/store`。这个方向已经撤销。

原因：

- record field 是 HType 内部布局，不是专门的 Solidity/EVM 高层语义。
- `evm.htype.field.*` 只是把普通 load/store 换成 generic helper，和“先打 marker 给后面消费”没有本质区别。
- 如果没有明确 consumer，它会增加 IR 噪音，反而模糊真正应该提升的 storage、bounds、revert、event、return 等语义。

处理结果：

- 删除 [src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmHighLevelTypePass.cpp) 整个实现，不再产出 `evm.htype.field.load/store`。
- 删除 [include/notdec/Passes/evm/SolidityPatterns.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatterns.h:76) 里的 `EvmHighLevelTypePass` 声明。
- 删除 [src/Passes/PassManager.cpp](/sn640/NotDec/src/Passes/PassManager.cpp:329) pipeline 里的 `EvmHighLevelTypePass`。
- 删除 [src/CMakeLists.txt](/sn640/NotDec/src/CMakeLists.txt:17) 对 `EvmHighLevelTypePass.cpp` 的编译接入。
- 删除 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:68) 里只为这条 rewrite 增加的 `EVMLoadEvidence` 和 `getEVMLoadEvidence()`。
- 恢复 [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2181) 的 `ConstraintsGenerator::recordLoad()`：pointer analysis 关闭时不再额外保留 EVM load evidence。
- 删除 [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:3258) 里 `MLsubRecovery::genASTTypes()` 对 `EVMLoads` 的收集。
- 删除 [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:277) 里的 `htype_field_*` 计数。
- 删除 [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:98) 里的 `memory_record_field_high_level_rewrite` 用例。
- 保留 `EVMStoreEvidence`、`HTypeBufferView` 等内部证据。它们不直接产出 generic HType IR，仍被 ABI return、revert、event 等已有语义 pass 使用。

撤销验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm --output-on-failure` 通过。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-solidity-rewrite-no-htype-field-suite` 通过，`75 passed, 0 failed`。
- EVM 性能 smoke：`/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-no-htype-field.ll --tr-level=2` 通过，本轮 `elapsed=18.32 rss_kb=950832`。同一用例近期记录为 `17.68s/958320KB`、`17.91s/958940KB`、`17.98s/952360KB` 和 `18.99s/955376KB`，未见明显退化。

下一步技术决策点：

- `25928_19774281_d048a8d52d_2758caa02f46.ll` 的 HType dump 里能看到 `top:256[]*`、`struct_13*[]*`、`typedef_1[]*` 这类非 storage array 形状，但它们目前更像类型恢复里的泛化 array / tail array，不直接等价于 Solidity dynamic array、static array 或 bytes/string。
- 如果下一步要 rewrite array，需要等更多 Solidity 语义证据把 dynamic/static array 和 bytes/string 区分清楚后再 rewrite。不要引入只表达 HType array 的泛用 helper。
- 当前不应直接把所有 `ArrayType` 当成 Solidity dynamic/static array。否则会把 tail-recursion normalization、ABI 临时 buffer、普通 memory array 混在一起。

## 当前观察：array / bytes 先不改

最近扫过的两个 ABI / EVM case 里，array / bytes 的 HType 还不够硬：

- `24562_19760107_e25027c623_f371f33b9104.ll` 里能看到 `top:256[]*`、`u256[]`、`struct_0*` 这类泛化数组/尾递归归一化结果，里面混着 ABI 临时 buffer 和普通 payload 形状。
- `0457_19495180_71d7525532_df21a257bd5c.ll` 里也有很多 `u256[]` 和 `top:256[]*`，但它们和 `public_*_bytes`、`public_*_string` 入口纠缠在一起，单靠 HType 还不能直接断成 Solidity array / bytes。

所以这一段先不做 rewrite，先按更明显的特征再收：

- static array：等固定边界、固定元素宽度、稳定 index->offset 关系都明确了再做。
- dynamic array：等 length、data 起点、index/stride 关系都稳定了再做。
- bytes/string：等长度编码、short/long 分支、copy 模式或 word 对齐这些事实更明确了再做。

现在的判断是：只看到 `ArrayType`、`top:256[]*`、`u256[]` 还不够，不能直接提升成 Solidity array/bytes。
