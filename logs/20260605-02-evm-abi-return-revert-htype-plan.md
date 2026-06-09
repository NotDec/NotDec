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

## 实现记录：HType buffer 读取 helper 抽取

本轮对应 `20260607-01-evm-abi-return-revert-htype-refactor-plan.md` 的阶段 2。
只整理重复的 HType record pointer 读取逻辑，不改 ABI return / revert / event 的语义分类。

修改：

- `include/notdec/Passes/evm/SolidityPatternUtils.h:20`：前置声明
  `notdec::ast::HType`、`RecordDecl` 和 `notdec::llvm2c::HTypeResult`。
- `include/notdec/Passes/evm/SolidityPatternUtils.h:114`：新增
  `HTypeBufferGap`，区分没有 pointer HType 和 pointer 不是 record pointee 两类缺口。
- `include/notdec/Passes/evm/SolidityPatternUtils.h:123`：新增
  `HTypeBufferView`。这个结构只带 `Record`、原始 `BaseType` 和缺口类型，不缓存
  `.htypes` 文本，也不做 Solidity 语义分类。
- `include/notdec/Passes/evm/SolidityPatternUtils.h:199`：声明
  `getHTypeBufferView`。
- `src/Passes/evm/solidity-patterns/HTypeBufferView.cpp:14`：新增
  `getRecordPointerPointee`，统一处理 pointer、set intersection 和 set union 里的
  record pointee。
- `src/Passes/evm/solidity-patterns/HTypeBufferView.cpp:39`：新增
  `containsPointerType`，用于把“完全不是 pointer”和“是 pointer 但不是 record pointer”
  分开报告。
- `src/Passes/evm/solidity-patterns/HTypeBufferView.cpp:57`：实现
  `getHTypeBufferView`，用 `getExtValuePtr(base, call, argIndex)` 查
  `HTypeResult::getDefaultValueType`。
- `src/CMakeLists.txt:15`：把 `HTypeBufferView.cpp` 加入 `notdec-core`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:36`：`getRecordPointeeHType`
  改用 `getHTypeBufferView`，保留 ABI return 自己的 debug 文案和统计。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:37`：
  `getRecordPointeeHType` 改用同一个 helper，revert payload 字段判断仍在
  `getRevertPayloadHType` / `classifyRevertFromHType` 内。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:26`：
  `getRecordPointeeHType` 改用同一个 helper，event pass 仍只标 `evm_logN`
  topic count。

效果：

- `AbiReturnPass.cpp`、`SolidityRevertPass.cpp`、`EventLogPass.cpp` 不再各自重复
  `getRecordPointerPointee` / `containsPointerType`。
- 新逻辑放在 `src/Passes/evm/solidity-patterns/HTypeBufferView.cpp`，没有继续增加
  `src/Passes/evm/SolidityPatterns.cpp` 的长度。
- helper 只读 `HTypeResult`，不扫描 IR，不解析 `.htypes`，也不把 HType 结果做成新的汇总层。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-error-string-htype-helper.ll --tr-level=2 --dump-htypes=/tmp/notdec-revert-error-string-htype-helper.htypes`
  通过；输出 IR 中 `evm_revert(..., %evm.alloc.addr, 100)` 仍带
  `!notdec.solidity.revert !{!"error_string"}`。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  仍失败，`7 passed, 92 failed`。这和当前重构计划里记录的迁移中状态一致；失败 suite
  仍不能作为阶段 2 helper 抽取的阻塞条件。

复杂度：2/10。只是把三份重复读取逻辑移到一个薄 helper。
维护成本：2/10。后续 ABI return / revert / event 能共用同一个 HType buffer 入口。
实现效果：7/10。减少重复，也避免 `SolidityPatterns.cpp` 继续变长；还没有进入阶段 3/4 的语义分类重构。

## 实现记录：SolidityRevertPass 基于 HType store evidence 读取 selector

问题：

- `SolidityRevertPass` 已经改成先看 revert base 的 HType record，但只知道
  offset 0 / 4 / 36 这些字段存在，还不知道字段里存入的 selector、panic code
  或 string length。
- 不能回到旧的“直接扫 IR store 推 payload”路线。store 只能作为 HType 已确认字段后的
  evidence 使用。

修改：

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:68`：新增
  `EVMStoreEvidence`，只保存 EVM store 的地址、原始 stored value、bit size 和来源指令。
  这个结构不暴露 BinarySub 临时类型节点。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:510`：`MLsubRecovery` 增加
  `EVMStores` 和 `getEVMStoreEvidence()`，供类型恢复后的 EVM semantic pass 读取。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:48`：前置声明 `isEVMModule`，让
  store 记录阶段能判断当前 module。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2178`：`recordStore` 在 EVM module
  下即使 pointer analysis 未启用，也保留 store 记录；非 EVM 仍按原来的
  pointer analysis 开关处理。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3163`：`MLsubRecovery::genASTTypes`
  合并各 SCC 结果时，把 EVM store 转成 `EVMStoreEvidence`，在 release BinarySub
  state 前只保留 LLVM value / instruction 指针。
- `include/notdec/Passes/evm/SolidityPatternUtils.h:199`：暴露
  `getOffsetFromBase` 和 `getSelectorWord`，供拆分后的 `SolidityRevertPass.cpp`
  复用旧工具函数，避免继续把逻辑写回 `SolidityPatterns.cpp`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:28`：
  `RevertPayloadHType` 增加 offset 0 / 4 / 36 对应的 store evidence 列表。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:78`：
  `getEvidenceOffsetFromBase` 支持 `store ptr %base` 和
  `store ptr inttoptr(base + const)` 两种 EVM lowering 形状。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:94`：
  `getFieldStoreValues` 先确认 HType record 有对应 field，再按 offset 过滤
  `EVMStoreEvidence`。这保证 store evidence 只作为 HType 字段的补充证据。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:159`：
  `classifyRevertFromHType` 现在从 offset 0 evidence 解 selector，区分
  `panic`、`error_string` 和 `custom_error_candidate`；offset 4 / 36 的唯一常量
  evidence 分别填 `PanicCode` 和 `ErrorStringLength`。同一字段出现冲突常量时放弃分类。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:247`：
  `SolidityRevertPass::run` 在 `TR.getResult()` 后读取 `TR.getEVMStoreEvidence()`，
  传给 HType 分类逻辑。

效果：

- `revert_error_string_01` 现在基于 HType record 字段和 store evidence 识别
  `Error(string)` selector `0x08c379a0`。
- offset 36 的 `store i256 5` 能写入
  `notdec.solidity_revert.error_string_length`，rewrite marker 参数也从 unknown
  变成 `5`。
- 新逻辑没有恢复旧 memory marker / raw store 全量扫描；所有 store evidence
  查询都在 HType field 已存在之后发生。
- 本次没有继续增加 `src/Passes/evm/SolidityPatterns.cpp` 的长度。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-evidence-final3.ll --tr-level=2 --dump-htypes=/tmp/notdec-revert-evidence-final3.htypes --gen-work-dir --work-dir=/tmp/notdec-revert-evidence-final3-work`
  通过。输出中 `evm_revert(..., %evm.alloc.addr, 100)` 带
  `!notdec.solidity_revert.selector` 和
  `!notdec.solidity_revert.error_string_length`，并插入
  `notdec_solidity_rewrite_revert_error_string(i256 147028384, i256 5)`。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.90s。
- `ctest --test-dir build -R 'notdec.type_recovery.(evm|llvm_ir).tr_level_2' --output-on-failure`
  中 EVM suite 通过；LLVM IR suite 的
  `21_PointerAnalysisBranchingFieldCycle` 仍失败，差异是 `rec_35` / `rec_36` /
  `rec_60` 这类 snapshot 命名漂移，和本轮 EVM-only store evidence 路线无关。
- 按用户要求，本轮不看 fortune 性能问题。

复杂度：4/10。新增了一个很薄的 EVM store evidence sidecar，pass 侧只做字段级读取。
维护成本：3/10。证据生命周期跟 `MLsubRecovery` 绑定，后续 ABI return 也可以复用，但需要继续保持“先 HType 字段、再 evidence”的边界。
实现效果：7/10。`Error(string)` selector 和 length 已走 HType evidence 路线；string literal 和更多 ABI payload 还没迁移。

## 实现记录：ABI return HType 字段分类和 store evidence helper 下沉

问题：

- `SolidityRevertPass` 已经有一套 HType field + store evidence 读取逻辑，后续
  ABI return 也会需要同样的 offset 判断。继续放在 `SolidityRevertPass.cpp`
  会让逻辑重复，也会让单个 pass 文件继续变长。
- `AbiReturnPass` 之前只按 `size == 32` / `returndatasize` 给 return 分类。
  这不符合阶段 4 的边界：本地非空 return 必须先从 HType record fields 读 payload。

修改：

- `include/notdec/Passes/evm/SolidityPatternUtils.h:20`：前置声明
  `mlsub::EVMStoreEvidence`，供 helper 声明使用。
- `include/notdec/Passes/evm/SolidityPatternUtils.h:210`：新增
  `hasHTypeFieldAt`、`getHTypeFieldStoreValues` 和
  `getUniqueUInt64FieldValue` 声明。
- `src/Passes/evm/solidity-patterns/HTypeStoreEvidence.cpp:12`：新增
  `getValueFromExtValue`、`isPtrToIntOf`、`getEvidenceOffsetFromBase`，统一处理
  `ptrtoint base` 和 `inttoptr(base + const)` 两类地址形状。
- `src/Passes/evm/solidity-patterns/HTypeStoreEvidence.cpp:42`：实现
  `hasHTypeFieldAt`。
- `src/Passes/evm/solidity-patterns/HTypeStoreEvidence.cpp:46`：实现
  `getHTypeFieldStoreValues`。仍然先确认 HType record 有字段，再读取 store evidence。
- `src/Passes/evm/solidity-patterns/HTypeStoreEvidence.cpp:68`：实现
  `getUniqueUInt64FieldValue`，多个不同常量视为冲突。
- `src/CMakeLists.txt:16`：把 `HTypeStoreEvidence.cpp` 加入 `notdec-core`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:71`：改用新 helper，
  删除 pass 内重复的 evidence offset / field store 读取函数。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:26`：新增
  `AbiReturnPayloadHType`，只记录当前 return buffer 的 HType field 形状，不做新的
  汇总层。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:58`：新增
  `getAbiReturnPayloadHType`，非 padding 字段计数，并识别 offset 0 / 32。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:80`：新增
  `classifyAbiReturnFromHType`。`empty` 和 `returndata_forward` 仍是直接入口；
  其它本地非空 return 必须先拿到 HType record，再分类为
  `static_1_word`、`dynamic_candidate`、`tuple_candidate` 或 `candidate`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:110`：
  `AbiReturnPass::run` 改用 HType 分类结果，拿不到 payload 时跳过并记录缺口。

效果：

- `AbiReturnPass` 不再只靠 size heuristic 标本地非空 return。
- `returndata_forward` 这类不是本地 buffer 构造的 return 仍保留直接识别。
- `SolidityRevertPass.cpp` 变短，store evidence 读取逻辑移到专门 cpp 文件。
- `0011_multi_public` 暴露出阶段 5 的缺口：`public__0xf39d8c65_0xe0::%evm.alloc.addr`
  仍是 `ptr<load=void, store=void, psize=256>`，不是 record pointer，所以本地
  32-byte return 暂时不会被 `AbiReturnPass` 标成 `static_1_word`。这需要继续修
  类型恢复，不能在 pass 里回退扫 store。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。
- `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-after-abi-helper.ll --tr-level=2 --dump-htypes=/tmp/notdec-revert-after-abi-helper.htypes --gen-work-dir --work-dir=/tmp/notdec-revert-after-abi-helper-work`
  通过，输出仍有
  `notdec_solidity_rewrite_revert_error_string(i256 147028384, i256 5)`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/0002_delegatecall_no_nonpayable.ll -o /tmp/notdec-abi-return-0002-after.ll --tr-level=2 --dump-htypes=/tmp/notdec-abi-return-0002-after.htypes --gen-work-dir --work-dir=/tmp/notdec-abi-return-0002-after-work`
  通过，returndata forward return 仍带
  `!notdec.solidity.abi_return !{!"returndata_forward"}`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/0011_multi_public.ll -o /tmp/notdec-abi-return-0011-after.ll --tr-level=2 --dump-htypes=/tmp/notdec-abi-return-0011-after.htypes --gen-work-dir --work-dir=/tmp/notdec-abi-return-0011-after-work`
  通过。HType 显示本地 32-byte return base 仍不是 record pointer。
- `ctest --test-dir build -R 'notdec.type_recovery.(evm|llvm_ir).tr_level_2' --output-on-failure`
  中 EVM suite 通过；LLVM IR suite 仍是
  `21_PointerAnalysisBranchingFieldCycle` snapshot 命名漂移失败。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  仍失败，`9 passed, 90 failed`。这和迁移中 oracle 状态一致；本轮没有按旧 oracle
  恢复 memory marker / raw store 扫描。
- 按用户要求，本轮不看 fortune 性能问题。

复杂度：4/10。新增一个薄 helper，并让 ABI return 分类走 HType record。
维护成本：3/10。helper 可被 ABI return / revert 共用；后续要继续守住“先 HType 字段、再 evidence”的边界。
实现效果：6/10。阶段 4 的 pass 边界更清楚，但单 word return 的 record 恢复还没解决，下一步应进入阶段 5。

## 阶段 5 决策点：single-word ABI return 的 offset 0 record 是否保留

现象：

- `0011_multi_public` 的 `public__0xf39d8c65_0xe0` 已经被 memory allocation rewrite
  改成：
  `%0 = call ptr @calloc_unbounded()`、
  `%evm.alloc.addr = ptrtoint ptr %0 to i256`、
  `store i256 %private.call, ptr %0`、
  `evm_return(mem, %evm.alloc.addr, 32)`。
- 但 HType 里 `public__0xf39d8c65_0xe0::%evm.alloc.addr` 仍是
  `ptr<load=void, store=void, psize=256>`，不是 record pointer。
- 我试过一个窄方向：在 EVM `load/store` 处理里，对直接访问 heap allocation base 的
  256-bit store 补 `{0: value}` record 约束。结果中间确实产生了单字段 record，
  但 `normalizeTransparentSingleFieldRecords` 又把它折成
  `'o17:256* & ptr<load=void, store='o17:256, psize=256>`。`AbiReturnPass`
  仍拿不到 record pointer。
- 这说明当前问题不是简单漏一条 store 约束，而是 HType normalize 的策略问题：
  单字段 offset 0 record 被认为是透明 wrapper，会被折掉。

不确定点：

- ABI return / revert / event 这类 EVM buffer 是否应该要求 single-word buffer 也保留成
  record pointer。
- 如果全局禁止 single-field record 透明化，可能影响普通 LLVM IR 和 C 输出质量。
- 如果只为 EVM allocation / ABI buffer 保留，需要给 HType normalize 提供来源信息，
  或者让 semantic pass 能识别“透明化后的 offset 0 buffer”。

备选路线：

- 路线 A：在 HType normalize 里保留来自 EVM ABI buffer 的 single-field record。
  需要给 record 或 value type 增加来源标记，范围较大，但语义最直接。
- 路线 B：`AbiReturnPass` 接受透明化后的
  `T* & ptr<load/store=T, psize=256>` 作为 single-word payload。这个改动小，
  但会让 pass 直接依赖 normalize 后的表示，需要谨慎确认不会变成旧式扫描兜底。
- 路线 C：在类型恢复侧为 ABI consumer base 单独附加不可透明化的 record view。
  这接近新的 HType view/sidecar，需要避免变成额外汇总层。

当前判断：

- 不应该在 `AbiReturnPass` 里回退扫 store。
- 也不应该贸然改全局 transparent single-field record normalize。
- 这里需要先决定 single-word ABI buffer 的表达方式，再继续阶段 5。
