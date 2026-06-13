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

## Public Entry 边界

一个 public entry 内所有直接 calldata 访问都归到函数自己的 `%calldata` 指针。

如果 public entry 调用 private helper，而 helper 的形参表示 calldata offset，需要把 helper 内的访问映射回调用点的 `%calldata`。已有 `getUniqueCallsiteArgUInt64Constant()` 可以处理“所有 callsite 都传同一个常量”的 helper。对于被多个 public entry 用不同 offset 复用的 helper，不能把 helper 本身固定成某个 ABI 格式。

当前采用受控 clone 路线：只为直接参与 `evm_calldataload` source offset 或 `evm_calldatacopy` source offset 的整数形参做 clone，不按 return address 等无关常量实参 clone。clone 只服务类型恢复，统一打 `notdec.evm.calldata_access_clone` 元数据，后续 Solidity ABI return / revert / checked-bounds / event marker pass 跳过这些 clone，避免把 clone 内的 revert 和 guard 当作新的用户语义点重复统计。

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
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:121)：`collectArgumentUses()` / `getCalldataOffsetArgNos()` 只收集真正流入 calldata load/copy source offset 的 helper 形参，作为 clone 的依据。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:185)：`cloneHelperForConstants()` 为不同 constant offset 调用点生成 `.cd` clone，并在 clone 里把对应形参替成常量。
- [src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp:226)：`cloneCalldataHelpers()` 在模块级收集调用点并改写 callee，所以 `EvmCalldataAccessPass` 已从 function pass 改为 module pass。
- [include/notdec/Passes/evm/SolidityPatternUtils.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatternUtils.h:41)、[src/Passes/evm/SolidityPatterns.cpp](/sn640/NotDec/src/Passes/evm/SolidityPatterns.cpp:107)：新增 `notdec.evm.calldata_access_clone` 元数据和 `isCalldataAccessCloneFunction()`。
- [src/Passes/evm/solidity-patterns/AbiReturnPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:675)、[src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:677)、[src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp:280)、[src/Passes/evm/solidity-patterns/EventLogPass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EventLogPass.cpp:74)：后续 Solidity marker pass 跳过 calldata clone，避免重复 marker。
- [include/notdec/Passes/evm/SolidityPatterns.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatterns.h:46)、[src/Passes/PassManager.cpp](/sn640/NotDec/src/Passes/PassManager.cpp:311)：`EvmCalldataAccessPass` 改为 module pass，并仍放在 `MemoryBufferRewritePass` 后、类型恢复前。

验证：

- `cmake --build ./build --target notdec -j4`
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)|notdec.type_recovery.evm.tr_level_2' --output-on-failure`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll --tr-level=2 --emit-tr-input-ir=/tmp/0334-pretr-new.ll`
- `./llvm-22.1.0.obj/bin/llvm-as /tmp/0334-pretr-new.ll -o /tmp/0334-pretr-new.bc`
- `./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll --tr-level=2 --emit-tr-input-ir=/tmp/0334-pretr-clone-hidden.ll`
- `./llvm-22.1.0.obj/bin/llvm-as /tmp/0334-pretr-clone-hidden.ll -o /tmp/0334-pretr-clone-hidden.bc`

性能观察：引入隐藏 clone 后，最近一次验证中 `notdec.evm.solidity_patterns` 用时 452.98 秒，`notdec.evm.solidity_rewrite` 用时 82.30 秒，`notdec.type_recovery.evm.tr_level_2` 用时 0.96 秒。clone 让 patterns suite 比上次 435.74 秒增加约 17 秒，需要后续继续关注 clone 数量；没有继续跑 fortune，用户已要求先不要管 fortune 性能问题。

方案评分：

- 实现效果：9/10。直接访问已经改成普通 LLVM load/memcpy，能回推唯一 callsite 常量 offset、简单 `arg + const` offset，并能通过隐藏 clone 处理同一 helper 多个常量 offset 调用点。
- 复杂度：7/10。新增 module pass clone 逻辑，并要求后续 marker pass 识别 clone 可见性；但 clone 条件只绑定 calldata source offset 形参，没有按所有常量实参膨胀。
- 维护成本：7/10。当前路线比 summary/context 简单，但 clone 会增加 IR 体积和测试时间；后续如果出现大规模 clone，需要加 clone 数量限制或改 summary。
