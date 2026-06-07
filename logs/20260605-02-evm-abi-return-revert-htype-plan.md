# AbiReturnPass / SolidityRevertPass 对接 HType 计划

## 原始 prompt

把当前聊的关于改进AbiReturnPass 和SolidityRevertPass 的规划和各种注意事项都写到一个新的logs/下的文档里。特别注意目前准备删掉MemoryConsumer 和MemoryWrite 这种标注相关的东西，相关匹配逻辑挪到具体的Pass里面

## 背景

`AbiReturnPass` 和 `SolidityRevertPass` 都在处理 EVM memory 里的 ABI payload：

- `AbiReturnPass` 识别 `evm_return(mem, base, size)`。
- `SolidityRevertPass` 识别 `evm_revert(mem, base, size)`。

这里的 `base`、`size` 是底层 EVM return/revert 指令的参数，分别表示 payload 在 memory 里的起始
offset 和字节长度。

目前这些 pass 里仍有不少按访问形状找 payload 的逻辑，例如围绕
`notdec_solidity_memory_write`、`notdec_solidity_memory_consumer` 等 marker 回头扫写入。下一步不应该继续
扩大这套标注层。内存里的内容应该让类型推理负责，后置 pass 只读 HType 结果。

## 目标

1. `AbiReturnPass` 和 `SolidityRevertPass` 保留各自的出口识别逻辑。
2. payload 内容不再靠访问模式匹配恢复，而是通过 `HTypeResult` 查 `base` 的结构体指针类型和字段范围。
3. 删除或停止依赖 `MemoryConsumer`、`MemoryWrite` 这类语义/写入标注。
4. 如果 `base` 没被推成结构体指针，不做 mstore 扫描兜底，记录成类型恢复缺口。

## 总体路线

### 1. Pass 职责

`AbiReturnPass` 负责：

- 找 `evm_return(mem, base, size)`。
- 从 HType 里读取 return buffer 字段。
- 生成 return tuple / dynamic return 候选。

`SolidityRevertPass` 负责：

- 找 `evm_revert(mem, base, size)`。
- 从 HType 里读取 revert buffer 字段。
- 做 empty / Panic / Error(string) / custom error candidate / returndata bubble 分类。
- 保留 checked/bounds guard 和 Panic code 的关系。

两者暂不合并。revert 可以看作异常返回，但分类规则和 guard 关系比普通 return 多，单独保留 pass 更清楚。
可共用一个很薄的 buffer 字段读取 helper。

### 2. 类型结果入口

后置 pass 不解析 `.htypes` 文本，直接访问 C++ 数据结构：

- 通过 `MLsubRecovery::getResult(Mod, MAM)` 拿 `HTypeResult`。
- 对 `base` 调 `HTypeResult::getDefaultValueType(base)`。
- 如果结果是结构体指针，解引用到 record。
- 按 field range 读取 `[0, size)` 内的字段类型。

理论上，如果后续基于 `base` 做了 memory access，类型推理应该把 `base` 推成指针类型，并给出结构体字段。
这才是 return/revert payload 的主要信息来源。

### 3. 不再走访问模式匹配

`AbiReturnPass` / `SolidityRevertPass` 后续不应该再靠这些方式恢复 payload：

- 在 basic block 里向前扫 `notdec_solidity_memory_write`。
- 根据连续 `mstore` / `mstore8` / copy 的形状猜 ABI 字段。
- 用 `MemoryConsumer` 提前告诉 pass 这个 buffer 是 return/revert/event。
- 用 `MemoryWrite` 作为语义 pass 的主要输入。

如果类型恢复还需要低层 memory access facts，可以在类型恢复内部保留必要表示；但这些不应该作为
Solidity 语义 pass 的标注接口继续扩散。

### 4. 删除标注层

准备删除或停止使用：

- `MemoryConsumerKind`
- `MemoryConsumer`
- `MemoryBufferFacts::Consumers`
- 面向语义 pass 的 `MemoryWrite` 输入
- `notdec_solidity_memory_consumer` 这类 consumer marker
- return/revert pass 里围绕 `notdec_solidity_memory_write`、`notdec_solidity_memory_copy_write` 的 payload 推断逻辑

保留的边界是：

- pass 自己匹配 `evm_return` / `evm_revert` 出口。
- 类型推理负责把 memory access 汇总成 `base` 的结构体指针和字段类型。
- 后置 pass 只读 HType，不重建 memory layout。

## base 仍是整数时怎么办

如果 `evm_return` / `evm_revert` 的 `base` 查出来仍是普通整数：

1. 不回退到旧的访问模式匹配。
2. 记录函数名、call 指令、base、size、当前 HType。
3. 把样例列为类型恢复缺口，反馈给类型恢复阶段修。

这类情况通常说明 memory access 没有把 `base` 和对应 memory object 连起来，或者 HType lowering 没把对象字段暴露到
`base` 的结构体指针类型上。

## 风险

- 类型恢复现在可能还没有稳定地把所有 return/revert buffer base 推成结构体指针。
- 动态 bytes/string/array 的 tail range 可能还需要更好的 field range 表达。
- 删除 `MemoryWrite` 依赖前，要确认现有测试里哪些 oracle 只是 marker 数量，不是真正语义。
- `SolidityRevertPass` 的 Panic/Error 分类现在有一部分可能还依赖旧写入匹配，需要拆开迁移。

## 判断标准

- `AbiReturnPass` / `SolidityRevertPass` 不再通过 memory write marker 推 payload。
- return/revert buffer 的字段类型来自 `HTypeResult`。
- base 不是结构体指针的样例能被明确记录，而不是静默降级。
- 现有 EVM solidity patterns 测试仍通过，必要时更新只针对旧 marker 的 oracle。
- selected-apehex-80 中成功生成 HType 的样本，抽查 return/revert 结果不明显倒退。

## 实现记录：删除 MemoryConsumer 层

本轮先推进最明确的一步：删除 `MemoryConsumer` 汇总层，让具体 pass 自己看
`evm_return` / `evm_revert` / `evm_log*` 的参数。

改动位置：

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:19`：删除
  `MemoryConsumerKind`、`MemoryConsumer` 和 `MemoryBufferFacts::Consumers`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:320`：`analyzeMemoryBuffers` 不再收集
  return/revert/event/external-call consumer；这里只保留 allocation、write、read、array byte write facts。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:495`：`MemoryBufferRewritePass` 不再保留 consumer marker
  materialization 的死代码。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:101`：删除
  `findReturnConsumerMarker`，不再依赖 `notdec_solidity_memory_consumer`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1305`：动态 return 相关逻辑改成直接使用
  `evm_return` 的 `base` / `size` 参数。
- `src/Passes/evm/SolidityPatterns.cpp:1498`：删除依赖
  `notdec_solidity_memory_consumer` 和 `notdec_solidity_memory_copy_write` marker 的 returndata bubble
  识别路径；保留直接匹配 `evm_returndatacopy` 的路径。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:16`：删除已禁用的 revert consumer marker
  统计和插入代码。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:18`：删除已禁用的 event consumer marker
  统计和插入代码。

验证：

- `cmake --build ./build --target notdec-decompile -j4` 通过。
- `ctest --test-dir build -R '^notdec\.type_recovery\.evm\.tr_level_2$' --output-on-failure`
  通过，用时 0.93s。
- `ctest --test-dir build -R '^notdec\.evm\.solidity_patterns$' --output-on-failure`
  通过，用时 104.89s。

当前技术决策点：

- `AbiReturnPass`、`SolidityRevertPass`、`EventLogPass` 现在仍在 EVM pipeline 的类型恢复前运行。
  要真正读 `HTypeResult`，需要先决定把这些语义解释挪到类型恢复后，还是拆成 pre-TR 出口识别和
  post-TR HType 解释两段。
- `MemoryWrite` 结构和 `notdec_solidity_memory_write` 相关匹配还没有完全删除。
  `SolidityRevertPass` 的 Panic/Error/custom error 分类仍会扫 revert 前的 memory write。
  下一步要么先做 post-TR HType 查询入口，再替换这段逻辑；要么先把旧 marker 路径和 raw memory
  store 路径拆开，记录哪些样例还依赖访问模式。

## 实现记录：语义 pass 后移并接入 HType

本轮先把顺序和 HType 数据入口打通，还没有替换 payload 内部字段识别。

改动位置：

- `include/notdec/Passes/evm/SolidityPatterns.h:43`：`AbiReturnPass` 改成 module pass，
  构造时保存 `mlsub::MLsubRecovery &TR`。
- `include/notdec/Passes/evm/SolidityPatterns.h:54`：`SolidityRevertPass` 同样改成 module pass，
  后续可直接读 `HTypeResult`。
- `src/Passes/PassManager.cpp:167`：新增 `HTypeDumpPass`，把 `--dump-htypes` 放进 pass pipeline，
  不再用 cache。
- `src/Passes/PassManager.cpp:291`：`PassEnv::build_passes` 接收 `HTypeDumpPath`。
- `src/Passes/PassManager.cpp:319`：EVM pipeline 中，类型恢复后先运行 `HTypeDumpPass`，
  再运行 `AbiReturnPass`、`SolidityRevertPass`、`EventLogPass`。
- `src/Passes/PassManager.cpp:349`：Wasm/type-recovery pipeline 中，也在类型恢复后插入
  `HTypeDumpPass`。
- `src/Passes/PassManager.cpp:360`：`add_llvm2c` 删除 HType snapshot cache 参数；`.c` 输出继续直接
  `TR.getResult`。
- `include/notdec/Passes/PassManager.h:118`：`--dump-htypes` 仍要求 `tr-level >= 2`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:108`：新增 `hasRecordPointeeHType`，
  用 `getExtValuePtr(base, call, 1)` 查 `HTypeResult::getDefaultValueType`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1311`：`AbiReturnPass::run` 遍历 module 内函数，
  在非空 `evm_return` 上检查 base 是否是结构体指针；暂时只统计和 debug 输出，不改变匹配结果。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:57`：新增 revert 侧的
  `hasRecordPointeeHType`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:176`：`SolidityRevertPass::run` 遍历 module
  内函数，在非空 `evm_revert` 上检查 base HType。

当前判断：

- 不再解析 `.htypes` 文件；后置 pass 直接访问 C++ 的 `HTypeResult`。
- `--dump-htypes` 的输出时机现在由 pass 顺序控制，EVM 后续 pass 改 IR 不会再影响 EVM type recovery
  snapshot 的 value key。
- 旧的 memory write / copy write payload 匹配还没有删。下一步要拿一个具体样例，对照 HType record
  field，把 return/revert payload 分类从访问模式迁到字段读取。

验证：

- `cmake --build ./build --target notdec-decompile -j4` 通过。
- `ctest --test-dir build -R '^notdec\.type_recovery\.evm\.tr_level_2$' --output-on-failure`
  通过，用时 0.90s。
- `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-error-string-tr2.ll --tr-level=2 --dump-htypes=/tmp/notdec-revert-error-string-tr2.htypes`
  通过；输出 IR 里仍有 `notdec.solidity.revert` metadata。
- `ctest --test-dir build -R '^notdec\.type_recovery\.llvm_ir\.tr_level_2$' --output-on-failure`
  更新 4 个 snapshot oracle 后通过，用时 2.35s。更新的样例是 `17_StackArray`、`18_offset1`、
  `20_PointerAnalysisFieldCycle`、`21_PointerAnalysisBranchingFieldCycle`；diff 只删除未被
  value/memory 引用的 decl，符合 `HTypeSnapshotFormatter::getOrderedDecls` 的可达 decl 打印规则。
- fortune 同口径命令：
  `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-pipeline-check.ll --tr-level=2 --frozen-tr-input-ir --dump-htypes=/tmp/notdec-fortune-pipeline-check.htypes`
  通过，`elapsed=12.13`。

## 具体样例检查：revert_error_string_01

样例：

`test/evm/solidity-patterns/cases/revert_error_string_01.ll`

命令：

`./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-error-string-tr2.ll --tr-level=2 --dump-htypes=/tmp/notdec-revert-error-string-tr2.htypes`

关键 IR：

- `public_run___0x2a` 里先从 free memory pointer 读出 `%evm.mload`，然后向
  `inttoptr(%evm.mload + offset)` 连续 store ABI encoded `Error(string)` payload。
- `evm_revert` 使用的是后面重新从 `0x40` 读出的 `%evm.mload5`：
  `evm_revert(mem, %evm.mload5, %evm.sub)`。

当前 HType 结果：

- `%evm.mload` 的 upper type 是 pointer，因为它后面被 `inttoptr` 用来写 memory。
- `%evm.mload5` 的 type 仍是 `bottom/top`，不是 pointer。
- `--debug-only=evm-solidity-patterns` 会输出：
  `evm revert: base has no pointer HType: %evm.mload5 = load i256, ptr inttoptr (i256 64 to ptr)`。

判断：

- 这不是 `SolidityRevertPass` 拿不到 `HTypeResult`，而是类型恢复还没有把 free memory pointer reload
  和前面基于同一 base 的 memory writes 统一起来。
- 仅给 `evm_revert` 的 base 参数加 pointer 标记还不够；后置 pass 需要的是 base 指向的 payload record
  field。当前 HType 还没有把 `inttoptr(base + offset)` 的 store 汇总成 base 的 record 字段。
- 下一步应先补 EVM native memory object 的类型恢复：把 `inttoptr(base + constant offset)` 的 store/load
  转成 base record field 约束，至少先覆盖 free memory pointer load/reload 这种常见 Solidity ABI
  encoder 形状。补好后再回到 `SolidityRevertPass` 删除旧的 memory write 扫描。

## 实现记录：EVM inttoptr 源节点补齐

本轮先补 `inttoptr(base + offset)` store 约束断开的最小缺口。

修改：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3907`：在 `MLsubVisitor::visitCastInst` 中，
  对 EVM 模块里的 `IntToPtrInst` 先调用 `cg.getOrInsertNode(SrcVal)`。
  这样 `%notdec.evm.mem.ptr.N = inttoptr i256 %evm.addN to ptr` 会 remap 到已有的
  `%evm.addN` SimpleType，后续 store 约束和 ptradd field 约束落到同一个节点。
- `src/CMakeLists.txt:2`：把主库 target 从 `notdec` 改成 `notdec-core`。
- `src/CMakeLists.txt:85`：把可执行 target 改成 `notdec`，输出仍是 `build/bin/notdec`。
  之后 `cmake --build ./build --target notdec` 会重新链接可执行文件。
- `test/CMakeLists.txt:10`、`:27`、`:44`、`:61`：CTest 里的 `$<TARGET_FILE:...>` 改成
  `$<TARGET_FILE:notdec>`。
- `unittests/Retypd/CMakeLists.txt:10`、`:20`、`:30`：单测改为链接 `notdec-core`。

效果：

- `revert_error_string_01` 的 trace 中，`%notdec.evm.mem.ptr.3/4/5` 已从独立
  `mapping:create` 变成 `mapping:remap` 到 `%evm.add/add1/add3`。
- `ValueTypes.txt` 中 `%evm.mload` 的 upper 从
  `{@100: {}, @36: {}, @4: {}, @68: {}} & Ptr<...>` 变成
  `{@100: {}, @36: Ptr<...>, @4: Ptr<...>, @68: Ptr<...>} & Ptr<...>`。
  `type-struct-merge.md` 里 `@4/@36/@68` 字段也能看到 `PtrStore<...>`。
- `%evm.mload5` 仍是 `top`。这说明 free memory pointer reload 还没从 `0x40` slot 的
  load/store 关系拿到同一个 payload record，后续还要补这一段。

验证：

- `cmake --build ./build --target notdec -j4` 通过，并确认会链接 `build/bin/notdec`。
- `ctest --test-dir build -R 'notdec.type_recovery.(evm|llvm_ir).tr_level_2' --output-on-failure`
  通过，用时 3.18s。
- fortune 同口径：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-evmcast.ll --tr-level=2 --frozen-tr-input-ir --dump-htypes=/tmp/notdec-fortune-evmcast.htypes`
  通过，`elapsed=12.16 user=11.79 sys=0.36 maxrss=834508`。近期参考是 `elapsed=12.13`，
  没有明显性能退化。

方案备注：

- 曾尝试对所有 cast 都强制创建源 SimpleType，fortune 退化到 `elapsed=149.06`，已收窄为
  EVM `IntToPtrInst` 专用处理。
- 这个改动不合并 PNI 节点对应的 SimpleType，只补 cast 源值缺失时的 MLsub 映射入口。

## 实现记录：free memory pointer reload 合并

本轮补 `revert_error_string_01` 里没有 `mstore(0x40, new_ptr)` 的情况。

修改：

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:29`：更新 `MemoryAllocation`
  注释，说明没有写回 `0x40` 的 buffer 会生成 `calloc_unbounded()`。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:37`：给 `MemoryAllocation`
  增加 `Reloads`，记录同一 basic block 内未被 `mstore(0x40, ...)` 打断的
  free memory pointer reload。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:46`：增加
  `isFreeMemoryPointerClobber`，只把明确写 `0x40` 当作 reload 合并的边界。
  按当前判断，EVM call/copy 先假设不会修改 free memory pointer slot。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:51`、`:74`：增加
  `hasFreeMemoryPointerClobberBetween` 和 `collectFreeMemoryPointerReloads`，
  先只合并同一 basic block 内的 reload。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:226`：`rewriteAllocation` 允许
  `Alloc.Size == nullptr`，这种情况生成 `calloc_unbounded()`；同时把 `Reloads`
  里的 load 一起替换成同一个 `evm.alloc.addr`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:553`：在已有 `Writes` 基础上补无
  finalize allocation。条件是 base 来自 `mload(0x40)`，且后面确实有基于该 base
  的 memory write。

效果：

- `revert_error_string_01` 的 `public_run___0x2a` 现在在入口生成：
  `%0 = call ptr @calloc_unbounded()` 和 `%evm.alloc.addr = ptrtoint ptr %0 to i256`。
- 原来的 `%evm.mload5` 被替换掉，`evm_revert` 现在使用：
  `evm_revert(mem, %evm.alloc.addr, %evm.sub)`。
- `ValueTypes.txt` 中 `%evm.alloc.addr` 的 upper 是
  `{@100: {}, @36: Ptr<...>, @4: Ptr<...>, @68: Ptr<...>} & Ptr<...>`，
  也就是 revert base 已经能拿到 payload record 形状。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-error-string-unbounded.ll --tr-level=2 --dump-htypes=/tmp/notdec-revert-error-string-unbounded.htypes --gen-work-dir --work-dir=/tmp/notdec-revert-error-string-unbounded-work`
  通过。
- `ctest --test-dir build -R 'notdec.type_recovery.(evm|llvm_ir).tr_level_2' --output-on-failure`
  通过，用时 3.19s。
- fortune 同口径：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-unbounded-reload.ll --tr-level=2 --frozen-tr-input-ir --dump-htypes=/tmp/notdec-fortune-unbounded-reload.htypes`
  通过，`elapsed=12.11 user=11.71 sys=0.39 maxrss=834792`。近期参考是
  `elapsed=12.13/12.16`，没有明显性能退化。

## 实现记录：pointer-sized direct access 折到 offset 0

问题：

- binarysub 已有 `fold_direct_pointer_into_zero_field`，会在同一个 memory object
  既有普通字段又有 direct load/store 时，把 direct 访问折成 offset 0 字段。
- 原条件是 `psize >= pointer_size` 时不折。这样 `load32 p` 和 `load32 p.4`
  在 32-bit pointer 下仍打印成 `{4: ...} & Ptr<...>`，而不是更直接的
  `{0: ..., 4: ...}`。
- EVM 下 pointer size 和 word access 都是 256-bit，也会遇到同类问题。

修改：

- `external/binarysub/src/binarysub.cpp:529`：把保护条件从
  `psize >= pointer_size` 改成 `psize > pointer_size`，允许刚好等于 pointer
  size 的 direct access 折到 offset 0。超过 pointer size 的情况先保留保护。
- `external/binarysub/src/binarysub-test.cpp:399`：更新
  `test_pointer_record_wrap` 预期，`load32 p` 现在对应 field `0`。
- 更新 `test/type-recovery/llvm-ir/expected/tr-level-2/05_MultiOffset.htypes`
  和 `11_SimpleRecursive1.htypes`。变化都是 pointer-sized direct access 从
  `ptr<load/store ...>` 变成 offset 0 字段对应的指针类型。

效果：

- `revert_error_string_01` 中 `%evm.alloc.addr` 的 UType 现在包含
  `{0: Ptr<...>, @4: Ptr<...>, @36: Ptr<...>, @68: Ptr<...>}`。
- `SolidityRevertPass` 能仅根据 HType 把该样例标成 `error_string`。

验证：

- `cmake --build ./build --target binarysub notdec -j4` 通过。
- `./build/binarysub` 通过。
- `ctest --test-dir build -R 'notdec.type_recovery.(evm|llvm_ir).tr_level_2' --output-on-failure`
  通过，用时 3.21s。
- `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-fold-eq-psize.ll --tr-level=2 --dump-htypes=/tmp/notdec-revert-fold-eq-psize.htypes --gen-work-dir --work-dir=/tmp/notdec-revert-fold-eq-psize-work`
  通过，输出 metadata 为 `error_string`。
- fortune 同口径：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-fold-eq-psize.ll --tr-level=2 --frozen-tr-input-ir --dump-htypes=/tmp/notdec-fortune-fold-eq-psize.htypes`
  通过，`elapsed=11.99 user=11.63 sys=0.35 maxrss=839108`。近期参考是
  `11.89/11.96/12.19`，没有明显性能退化。

## 实现记录：EventLogPass 停止扫描 memory write marker

本轮把 `EventLogPass` 也接到类型恢复之后的 HType 数据入口。

修改：

- `include/notdec/Passes/evm/SolidityPatterns.h:76`：`EventLogPass` 改成
  module pass，构造时保存 `mlsub::MLsubRecovery &TR`。
- `src/Passes/PassManager.cpp:326`：EVM pipeline 中直接加入
  `evm::EventLogPass(*TR)`，不再通过 function adaptor。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:16`：删除
  event data allocation / word write / copy write marker 统计，改成 HType
  缺口统计。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:67`：新增
  `getRecordPointeeHType`，用 log data base 查 `HTypeResult`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:98`：
  `EventLogPass::run` 遍历 module。`evm_logN` 本身仍直接按 call 名字标
  `notdec.solidity.event`；非空 data payload 只查 HType，查不到只记录缺口，
  不再回退扫描 `notdec_solidity_memory_write` 或
  `notdec_solidity_memory_copy_write`。

效果：

- `EventLogPass.cpp` 里不再有 `notdec_solidity_memory_*` 引用。
- `test/type-recovery/evm/cases/02_evm_runtime_helpers.ll` 仍能在 `evm_log1`
  上输出 `notdec.solidity.event` metadata。
- 该样例的 `%buf.addr` 目前只是 `Ptr<...>`，还不是 record pointer；这会作为
  event data HType 缺口记录，不做访问模式兜底。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R 'notdec.type_recovery.(evm|llvm_ir).tr_level_2|notdec.evm.solidity_patterns' --output-on-failure`
  中 type recovery 两个 suite 通过；`notdec.evm.solidity_patterns` 仍失败，
  失败主要是现有 oracle 仍按旧的 ABI return / revert HType-only 迁移前结果统计，
  如 `notdec.solidity.abi_return` 和 `notdec.solidity.revert` 期望非 0、实际为 0。
- `./build/bin/notdec test/type-recovery/evm/cases/02_evm_runtime_helpers.ll -o /tmp/notdec-event-htype.ll --tr-level=2 --dump-htypes=/tmp/notdec-event-htype.htypes --gen-work-dir --work-dir=/tmp/notdec-event-htype-work`
  通过，输出 IR 中 `evm_log1` 带 `notdec.solidity.event`。
- fortune 同口径：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-event-htype.ll --tr-level=2 --frozen-tr-input-ir --dump-htypes=/tmp/notdec-fortune-event-htype.htypes`
  通过，`elapsed=12.11 user=11.74 sys=0.36 maxrss=839080`。近期参考是
  `11.99/12.19`，没有明显性能退化。
