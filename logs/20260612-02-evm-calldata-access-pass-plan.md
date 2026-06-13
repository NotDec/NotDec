# EvmCalldataAccessPass 规划

## 原始 prompt

还是单独把EvmCalldataAccessPass的具体规划，EvmStorageSemanticsPass的具体规划单独拆到单独的logs/文件吧，logs/20260612-01-evm-high-level-semantics-pass-plan.md里面还是仅保留最早的那种简单的介绍。处理完这个之后commit一下

## 背景

当前 public entry 里的 calldata 访问主要表现为 `evm_calldataload`、`evm_calldatacopy`、calldata size guard 和若干 private helper 调用。这样类型推理看到的是一批分散 helper call，而不是一个有字段、有范围访问的输入对象。

Calldata 和普通 memory 的区别是：每个 public function 有自己的 ABI 格式。因此应该把每个 public entry 的 `%calldata` 参数当作这个函数自己的输入 buffer，把这个函数内所有 calldata 访问归到同一个对象上，再交给类型推理恢复结构。

这个 pass 不负责识别最终 ABI 参数类型，也不负责判断 address、bool、uintN、bytes/string、array。它只负责把 calldata 访问整理成类型推理能消费的形式。

## 目标

在类型恢复前执行：

```text
MemoryBufferRewritePass
EvmCalldataAccessPass
TypeRecovery
```

每个 public entry 直接使用已有 `%calldata` 指针作为 calldata buffer，不引入 `notdec_evm_get_calldata` 之类专门 helper。然后把该 public entry 内的 `evm_calldataload`、`evm_calldatacopy`、相关 bounds guard 和 private helper offset 关系都归到 `%calldata` 上。

## 重写形状

静态 word 访问从：

```llvm
%w0 = call i256 @evm_calldataload(ptr %calldata, i256 4)
%w1 = call i256 @evm_calldataload(ptr %calldata, i256 36)
```

整理成同一个 calldata buffer 的 offset 访问：

```llvm
%cd.base0 = ptrtoint ptr %calldata to i256
%cd.addr0.raw = add i256 %cd.base0, 4
%cd.addr0 = inttoptr i256 %cd.addr0.raw to ptr
%w0 = load i256, ptr %cd.addr0
%cd.base1 = ptrtoint ptr %calldata to i256
%cd.addr1.raw = add i256 %cd.base1, 36
%cd.addr1 = inttoptr i256 %cd.addr1.raw to ptr
%w1 = load i256, ptr %cd.addr1
```

这里直接使用 LLVM `load` 指令，不引入 `load_word` 或其他专门 helper。地址按当前 EVM memory 方案整理成 `ptrtoint` / `add` / `inttoptr`，避免在类型恢复前引入 MLsub 还没有正式处理的 GEP。关键是类型推理能看出 `%calldata` 是同一个对象，offset 4、36、68 等位置被读取。

range copy 从：

```llvm
call void @evm_calldatacopy(ptr %mem, ptr %calldata,
                            i256 %dst, i256 %src, i256 %len)
```

整理成：

```llvm
%src.base = ptrtoint ptr %calldata to i256
%src.raw = add i256 %src.base, %src
%src.ptr = inttoptr i256 %src.raw to ptr
%dst.ptr = inttoptr i256 %dst to ptr
call void @llvm.memcpy.p0.p0.i256(ptr %dst.ptr, ptr %src.ptr, i256 %len, i1 false)
```

也就是说，`evm_calldatacopy` 直接落成普通 `llvm.memcpy`，不要引入专门 calldata copy intrinsic。EVM memory 已经整体转向普通 LLVM memory，destination 也按现有规则转成普通 pointer。

这样 dynamic bytes/string、array tail 等结构不需要在 pass 里猜。类型推理可以根据 `%calldata` 上的 head load、tail load、range copy、bounds guard 恢复结构。

## 开头 calldata size guard

先只识别 public entry 开头的固定 calldata 最小长度检查，不处理动态 tail 的复杂 bounds。目标形状是 Solidity 常见的：

```yul
if lt(calldatasize(), 36) { revert(0, 0) }
```

对应 IR 可能是 `evm_calldatasize` 和常量比较后跳到 revert 块。识别范围保持保守：

- 只在 public entry 的入口附近找，要求 guard 支配后续正常 body。
- 只接受和常量比较的 `calldatasize`，例如 `calldatasize < 36`、`36 > calldatasize`、`calldatasize >= 36` 的等价形状。
- fail 分支必须是无 payload 的 revert，或者已经被 Solidity revert helper 识别成普通 reject 分支。
- 多个开头 guard 可以取最大最小长度；识别不到就不写证据。

识别到以后，不生成新的 calldata 获取 intrinsic。当前 `%calldata` 已经是函数参数，guard 的目标是把“这个路径上 `%calldata` 至少 N bytes”表达出来。可以引入一个只做语义标记的 checked intrinsic：

```llvm
%calldata.checked = call ptr @notdec_evm_calldata_min_size(ptr %calldata, i256 36)
```

这个 intrinsic 不分配对象，也不返回新的 calldata 来源；它返回的 pointer 是 `%calldata` 的 checked alias。后续被 guard 支配的 calldata load/copy 可以使用 `%calldata.checked` 作为 base，让类型恢复沿 IR 直接看到这个 buffer 的最小有效长度。MLsub 处理时把它当成 alias/refinement，并把 `min_size = 36` 作为该 calldata buffer 的对象大小下界。这样比函数级 metadata 更容易跟随 dominated use，也更适合作为 guard matcher 的转换目标。

这个信息只表达“入口要求 calldata 至少 N bytes”，不代表完整 ABI 长度，也不覆盖动态 bytes/string/array 的 tail bounds。动态 tail 的 offset、length、payload 越界检查仍然作为后续单独证据处理。

## Public Entry 边界

一个 public entry 内所有直接 calldata 访问都归到函数自己的 `%calldata` 指针。

如果 public entry 调用 private helper，而 helper 的形参表示 calldata offset，需要把 helper 内的访问映射回调用点的 `%calldata`。已有 `getUniqueCallsiteArgUInt64Constant()` 可以处理“所有 callsite 都传同一个常量”的 helper。对于被多个 public entry 用不同 offset 复用的 helper，不能把 helper 本身固定成某个 ABI 格式。

当前不复制 private helper。对于被多个调用点以不同 calldata offset 使用的 helper，只给原函数打 `notdec.mlsub.polymorphic_function` 元数据，说明这个函数需要按多态函数处理。这个元数据属于类型推理侧，EVM calldata pass 只是生产者；MLsub 在 SCC 分层时消费它，避免让 EVM 专用 metadata 渗进类型推理实现。

## 和类型推理 / HType 的关系

这个 pass 给类型推理提供：

- calldata buffer 对象，也就是 public entry 的 `%calldata` 指针。
- buffer 上的 word load。
- buffer range copy 到 memory object。
- bounds guard 和访问范围的关系。
- private helper 形参 offset 回推后的访问关系。

类型推理负责恢复：

- 哪些 offset 是 ABI head field。
- 哪些 head 指向 dynamic tail。
- dynamic tail 是 bytes/string/array/tuple。
- 元素类型和嵌套结构。
- address、bool、uintN/intN 等值类型。

后续语义 pass 只读取 calldata buffer 的 HType。例如 external call 或 signature pass 需要知道某个 bytes 参数时，应优先查 HType 是否说明 `%calldata` 的某个 field 是 bytes，再调用 dynamic bytes helper 处理 memory 侧 copy。不能回到各 pass 自己重新扫 `evm_calldataload/evm_calldatacopy` 的路线。

## 实现注意

- 不在这个 pass 里生成 `evm.abi.arg.address`、`evm.abi.arg.bytes` 之类最终参数 helper。
- 不在这个 pass 里处理 address 高 96 bit 清零、bool range、uintN mask、signextend 等 value normalization；这些是类型推理输入或单独 value normalization 逻辑。
- 不删除 bounds guard。guard 是否隐藏由后续 cleanup 或 guard pass 统一处理。
- PHI 合并出来的 calldata offset 不要跳过。能表达成 `%calldata` 上的动态 offset 访问就保留给类型推理。
- fallback 里手写协议也可以先作为 calldata buffer 访问表达，不要强行当 Solidity ABI 参数。

## 实现记录

已完成直接 public calldata 访问重写：

- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:27)：新增 `EvmCalldataAccessPass`。它只处理 `public_` selector/body 函数的当前 `%calldata` 参数；`evm_calldataload` 重写为 `ptrtoint %calldata + offset -> inttoptr -> load i256`；`evm_calldatacopy` 重写为 `llvm.memcpy.p0.p0.i256`。
- [include/notdec/Passes/evm/SolidityPatterns.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatterns.h:40)、[src/CMakeLists.txt](/sn640/NotDec/src/CMakeLists.txt:15)、[src/Passes/PassManager.cpp](/sn640/NotDec/src/Passes/PassManager.cpp:310)：声明 pass、加入构建，并放在 `MemoryBufferRewritePass` 后、类型恢复前。
- [src/Passes/evm/solidity-patterns/CheckedBoundsMatchers.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/CheckedBoundsMatchers.cpp:1741)：`checked_div` / `checked_mod` 的 rewrite marker 只要求 divisor 可用，避免非 guard operand 被优化下沉后导致降级。
- [src/Passes/evm/solidity-patterns/CheckedBoundsMatchers.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/CheckedBoundsMatchers.cpp:1807)：checked-bounds calldata array matcher 同时识别旧 `evm_calldataload` 和新 `inttoptr(add(ptrtoint(%calldata), offset))` 上的 `load i256`。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:70)：删掉 level0 rewrite suite 里残留的 checked-bounds 预期；checked-bounds 现在由 tr-level 2 的 patterns suite 覆盖。

后续增量：

- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:27)：放宽到 `private__` helper，只要函数第二个参数仍是 `%calldata`，就把 helper 内直接 `evm_calldataload` / `evm_calldatacopy` 也改成普通 LLVM 内存访问。这样 helper 形参里的动态 offset 会保留在地址表达式里，不在 helper 内固定成某个 public ABI。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:46)：对 calldata load/copy 的 source offset 使用 `getUniqueCallsiteArgUInt64Constant()`。如果 helper offset 形参在所有直接 callsite 都是同一个 64-bit 常量，就在重写时直接换成常量；否则保留动态 offset。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:52)：继续扩展 offset 回推，支持常量、唯一 callsite 参数常量，以及这些值之间的小范围 `add`。例如 helper 内 `%arg + 32`，且 `%arg` 在所有直接 callsite 都是 `4` 时，会写成 `%calldata + 36`；如果 callsite 不一致，仍保留动态 offset。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:118)：`collectArgumentUses()` / `getCalldataOffsetArgNos()` 只收集真正流入 calldata load/copy source offset 的 helper 形参，作为判断多态 calldata helper 的依据。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:168)：`hasPolymorphicCallsiteOffset()` 检查同一 offset 形参是否存在多个不同常量调用点，或者同时存在常量和动态调用点。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:197)：`markPolymorphicHelpers()` 给这类 helper 打 `notdec.mlsub.polymorphic_function` 元数据，metadata 值暂记为 `calldata_offset`，不复制函数。
- [include/notdec/TypeRecovery/mlsub/Metadata.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/Metadata.h:8)：新增类型推理侧通用 metadata 名 `notdec.mlsub.polymorphic_function`。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:3303)：`MLsubRecovery::prepareSCC()` 把带 `notdec.mlsub.polymorphic_function` 的函数也当作多态函数，和 `NOTDEC_POLY_FUNCS`、summary override 里的多态标记走同一条 SCC 分层路径。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:3192)：`MLsubRecovery::genASTTypes()` 合并最终 HType 时跳过多态 callee 为 caller interface 临时生成的重复记录，保留 value 所属 SCC 先写入的 HType。
- [include/notdec/Passes/evm/SolidityPatterns.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatterns.h:46)、[src/Passes/PassManager.cpp](/sn640/NotDec/src/Passes/PassManager.cpp:311)：`EvmCalldataAccessPass` 保持 module pass，用来查看 helper 的所有直接调用点；仍放在 `MemoryBufferRewritePass` 后、类型恢复前。
- 按用户后续判断，已删除隐藏 clone 路线：不再生成 `.cd` helper，也不再要求 ABI return / revert / checked-bounds / event marker pass 跳过 clone。

本次增量：

- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:40)：新增 `CalldataMinSizeGuard`，记录正常分支上的 checked alias 和最小 calldata 大小。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:74)：新增 `matchCalldataTooShortICmp()` / `matchCalldataTooShort()`，识别 `calldatasize < N`、`N > calldatasize` 以及 `zext`、`iszero`、`icmp ne 0` 包装后的等价形状。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:176)：新增 `isNoPayloadRevertBlock()`，只接受 fail successor 中的 `evm_revert(..., 0, 0)`。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:192)：新增 `notdec_evm_calldata_min_size(ptr, i256) -> ptr` 声明和插入逻辑。调用插在正常 successor 的第一条非 PHI 指令前，且要求这个 successor 只有 guard block 一个前驱，避免 marker 被失败路径或其他未检查路径执行。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:478)：`rewriteFunction()` 构建 `DominatorTree`，对被 checked 正常块支配的 `evm_calldataload` / `evm_calldatacopy` 选择最强的 checked alias 作为 base。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:4026)：`handleEVMMarkerCall()` 把 `notdec_evm_calldata_min_size` 返回值 remap 到原 `%calldata`，并把 size 参数标成非指针，避免它进入普通函数调用约束。

验证：

- `cmake --build ./build --target notdec -j4`
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)|notdec.type_recovery.evm.tr_level_2' --output-on-failure`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll --tr-level=2 --emit-tr-input-ir=/tmp/0334-pretr-new.ll`
- `./llvm-22.1.0.obj/bin/llvm-as /tmp/0334-pretr-new.ll -o /tmp/0334-pretr-new.bc`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll --tr-level=2 --emit-tr-input-ir=/tmp/0334-pretr-polymorphic.ll`
- `./llvm-22.1.0.obj/bin/llvm-as /tmp/0334-pretr-polymorphic.ll -o /tmp/0334-pretr-polymorphic.bc`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll --tr-level=2 --emit-tr-input-ir=/tmp/0334-pretr-mlsub-poly.ll`
- `./llvm-22.1.0.obj/bin/llvm-as /tmp/0334-pretr-mlsub-poly.ll -o /tmp/0334-pretr-mlsub-poly.bc`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll --tr-level=2 --gen-work-dir --work-dir=/tmp/notdec-0334-mlsub-poly -o /tmp/0334-mlsub-poly.ll`
- `/tmp/0334-pretr-mlsub-poly.ll` 里 `private__0x3597_0x3597` 带 `!notdec.mlsub.polymorphic_function`；没有 `.cd` helper，也没有残留 `evm_calldataload` / `evm_calldatacopy`。
- `/tmp/notdec-0334-mlsub-poly/SCCs.txt` 里 `private__0x3597_0x3597,evm_revert,evm_shl` 被拆成独立 `level = 1` SCC，说明 MLsub 已消费这个 metadata。
- `./build/bin/notdec test/evm/solidity-patterns/cases/0679_19497465_c2187cbc73_f22fac5262f8.ll -o /tmp/0679-mlsub-poly-fix.ll --tr-level=2`
- 修复后 0679 不再触发 `genASTTypes()` 的重复 HType 断言。
- `./build/bin/notdec test/evm/solidity-rewrite/cases/0188_19493600_ac3404db28_614bfb7197be.ll --tr-level=2 --emit-tr-input-ir=/tmp/0188-calldata-min-size.ll`
- `/tmp/0188-calldata-min-size.ll` 中 `calldatasize > 9` 的正常块开头生成 `%calldata.checked = call ptr @notdec_evm_calldata_min_size(ptr %calldata, i256 10)`，后续 calldata load/copy 使用 `%calldata.checked`。
- `./llvm-22.1.0.obj/bin/llvm-as /tmp/0188-calldata-min-size.ll -o /tmp/0188-calldata-min-size.bc`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0112_19493231_e681d3a0e3_c848651ad27d.ll --tr-level=2 --emit-tr-input-ir=/tmp/0112-calldata-min-size.ll`
- `/tmp/0112-calldata-min-size.ll` 没有生成 `notdec_evm_calldata_min_size`；该 selector 的 `size < 4` fail successor 不是直接空 revert，当前规则保守跳过。
- `./llvm-22.1.0.obj/bin/llvm-as /tmp/0112-calldata-min-size.ll -o /tmp/0112-calldata-min-size.bc`
- `./build/bin/notdec test/evm/solidity-rewrite/cases/0188_19493600_ac3404db28_614bfb7197be.ll -o /tmp/0188-calldata-min-size-final.ll --tr-level=2`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll -o /tmp/0334-calldata-min-size-final.ll --tr-level=2`
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)|notdec.type_recovery.evm.tr_level_2' --output-on-failure`

性能观察：移除 clone、改为多态 metadata 后，最近一次验证中 `notdec.evm.solidity_patterns` 用时 437.67 秒，`notdec.evm.solidity_rewrite` 用时 80.46 秒，`notdec.type_recovery.evm.tr_level_2` 用时 0.98 秒。相比隐藏 clone 的 452.98 秒，patterns suite 用时恢复到接近 clone 前水平；没有继续跑 fortune，用户已要求先不要管 fortune 性能问题。

本次 calldata min-size intrinsic 后，相关 CTest 用时：`notdec.evm.solidity_patterns` 442.24 秒，`notdec.evm.solidity_rewrite` 82.08 秒，`notdec.type_recovery.evm.tr_level_2` 0.98 秒。与前次同口径相比变化很小。

方案评分：

- 实现效果：8/10。直接访问已经改成普通 LLVM load/memcpy，能回推唯一 callsite 常量 offset、简单 `arg + const` offset，并能标出同一 helper 多个 calldata offset 调用点。标记本身不把不同 callsite 的 HType 证据拆开，后续仍需要类型推理或 summary/context 消费这个 metadata。
- 复杂度：6/10。保留 module pass 是为了看所有直接调用点，但不再复制函数，也不需要后续 marker pass 特判 clone。
- 维护成本：6/10。metadata 路线比 clone 轻，IR 体积稳定；后续维护重点是继续让 calldata 访问证据更完整地进入 HType，而不是让后续 pass 重新匹配 calldata 指令形状。
