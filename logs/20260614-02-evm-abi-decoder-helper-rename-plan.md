# EVM ABI Decoder Helper 重命名规划

## 原始 prompt

那感觉可以对标solidity的这种方式，专门搞个pass，识别这种小的private helper函数，然后恢复成abi_decode_xxx这种名字。恢复名字就可以了，也不要搞什么decoder summary。把这个pass的规划单独写到一个logs/下面

## 背景

Solidity IR/Yul codegen 不是把一个固定 ABI decoder 二进制库原样链接进合约，而是按类型生成一批固定模板的 Yul helper。例如 `ABIFunctions::tupleDecoder()` 会生成 `abi_decode_tuple_...`，value type decoder 会生成 `abi_decode_t_uint256` 这类函数，calldata array / tail access 也有稳定模板。

这些名字编译到 EVM 后会丢失。Gigahorse/evm2llvm 侧再按跳转结构 outline 后，当前 IR 里通常只剩 `private__0x..._0x...`。这些 helper 很多仍保留 Solidity decoder 模板的局部形状，例如：

- 从 `%calldata` 的某个 offset 读一个 word。
- 对读出的 word 做 mask / validator。
- 读取动态 tail offset，再读取 length。
- 做 `offset + length * stride <= calldatasize` 这类 calldata bounds 检查。

当前 `EvmCalldataAccessPass` 已经把 helper 内的 `evm_calldataload` / `evm_calldatacopy` 改成普通 LLVM load / memcpy，也会标记多调用点 calldata offset helper 的多态性。下一步可以单独做一个轻量 pass，只负责把明显的 Solidity ABI decoder helper 改成接近 Solidity Yul helper 的名字，方便阅读和后续调试。

## 目标

新增一个 EVM Solidity pattern pass，识别小的 `private__` helper 函数，并根据函数体形状重命名为 `abi_decode_xxx` / `calldata_access_xxx` 风格名称。

这个 pass 只做重命名，不生成 decoder summary，不改 IR 语义，不引入新的 intrinsic，不改变类型恢复约束。

期望效果：

- 让 IR 里的 `private__0x...` 变成更可读的 helper 名，例如 `abi_decode_word_from_calldata`、`abi_decode_calldata_array_head`、`calldata_access_dynamic_tail`。
- 保留原始地址信息，避免丢失定位能力，例如追加 `__0x123` 后缀。
- 对匹配不稳的 helper 保持原名。

## Solidity 对标

主要对标这些 Solidity 生成点：

- `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp`
  - `ABIFunctions::tupleDecoder()`
  - `ABIFunctions::abiDecodingFunctionValueType()`
  - `ABIFunctions::abiDecodingFunctionCalldataArray()`
  - `ABIFunctions::calldataAccessFunction()`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`
  - `YulUtilFunctions::calldataArrayIndexAccessFunction()`
  - `YulUtilFunctions::calldataArrayIndexRangeAccess()`
  - `YulUtilFunctions::accessCalldataTailFunction()`
- `/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp`
  - `CompilerUtils::abiDecode()` 旧 codegen 路线，主要作为形状补充，不优先追求全覆盖。

这里不尝试恢复 Solidity 原始精确类型名。因为优化和 evm2llvm outline 后，函数名已经没了，类型信息也未必足够。第一阶段只恢复稳定的 helper 类别名。

## 第一阶段匹配范围

只处理 `private__` 开头、非 declaration、规模较小的函数。建议先限制指令数和基本块数，避免误把业务逻辑改名成 decoder。

第一批只做几类高置信度名字：

- `abi_decode_word_from_calldata`
  - 形状是从 `%calldata` 某个 offset 读一个 256-bit word。
  - 可带简单 mask / cleanup，例如 address、bool、uintN 的 validator。
  - 不在这里判断最终类型，只说明这是 calldata word decoder。
- `abi_decode_calldata_dynamic_head`
  - 形状是读取 head slot 里的 dynamic offset，并计算 `base + offset`。
  - 通常伴随 offset bounds 检查。
- `abi_decode_calldata_array_head`
  - 形状是读取 `length = calldataload(offset)`，计算 `data = offset + 32`，并检查 `data + length * stride <= end/calldatasize`。
- `calldata_array_index_access`
  - 形状是 `index < length`，然后 `base + index * stride`。
- `calldata_access_dynamic_tail`
  - 形状接近 Solidity `access_calldata_tail_*`：从 tail pointer 读 relative offset，转成 absolute address，再可选读取 length。

如果一个 helper 同时符合多个类别，按更具体的类别命名。例如 array head 优先于 generic dynamic head。

## 命名规则

重命名要稳定、可重复、低碰撞：

```text
private__0x123_0x123
  -> abi_decode_calldata_array_head__0x123
```

建议保留第一个地址后缀；如果 LLVM 自动处理重名，也可以接受 `.1` 后缀，但最好主动用原地址保持唯一性。

如果 helper 已经有非 `private__` 名字，或者已经被重命名过，不再处理。

如果函数带 `notdec.mlsub.polymorphic_function` metadata，也可以重命名，但不能删除 metadata。多态性和名字是两件事。

## Pass 位置

建议放在 `EvmCalldataAccessPass` 之后、类型恢复之前。

原因：

- `EvmCalldataAccessPass` 已经把 calldata load/copy 统一成普通 LLVM 内存访问，后续 matcher 可以同时支持旧 `evm_calldataload` 和新 load 形状，但优先匹配新形状。
- 只改函数名，不影响后续 pass 语义。
- 类型恢复、日志、调试输出可以看到更好的 helper 名。

如果发现重命名影响已有按 `private__` 前缀判断的逻辑，就要么把该逻辑改成 metadata 判断，要么把这个 pass 放到那些逻辑之后。第一阶段需要重点检查 `EvmCalldataAccessPass` 和 Solidity pattern utils 里对 `private__` 的依赖。

## 风险

- 误命名比漏命名更糟。名字会影响人读 IR，也可能影响少量基于函数名前缀的逻辑。因此第一版宁可保守。
- 优化后的 helper 可能被拆得很碎，单个 helper 只剩一小段 decoder 逻辑。不要为了凑 Solidity 完整 helper 而跨函数强行合并。
- 旧 codegen 和新 IR codegen 的形状不完全一样。第一版以新 IR/Yul 模板为主，旧 codegen 只覆盖非常明显的 word load / array head 形状。
- 如果 pass 太早运行，后续还依赖 `private__` 前缀的逻辑可能失效。需要明确 pass pipeline 位置。

## 判断标准

- 新增回归样例覆盖至少三类 helper：
  - calldata word decoder。
  - dynamic tail / array head decoder。
  - array index access。
- 对真实 patterns suite 抽样，重命名后的函数仍能通过 `llvm-as`。
- `notdec.evm.solidity_patterns` 和 `notdec.type_recovery.evm.tr_level_2` 通过。
- 对现有关注样例确认：
  - 不改变 `notdec.mlsub.polymorphic_function` metadata。
  - 不改变 calldata load/copy rewrite 结果。
  - 没有把明显业务 helper 改成 ABI decoder 名。
- 性能上只做小函数局部扫描，不应造成可见下降；跑同口径 CTest 记录用时。

## 不做什么

- 不恢复 decoder summary。
- 不恢复最终 ABI 参数类型。
- 不根据函数名猜 Solidity 源码签名。
- 不处理 selector dispatcher。
- 不把业务 helper 内偶然出现的 `calldataload` 改名成 decoder。

## 实现记录

已完成第一版轻量重命名 pass。

代码改动：

- `include/notdec/Passes/evm/SolidityPatterns.h:51` 新增 `AbiDecoderHelperRenamePass` 声明，明确只做 helper 重命名，不附加 decoder summary。
- `include/notdec/Passes/evm/SolidityPatternUtils.h:41` 新增 `notdec.evm.original_private_helper` metadata 名，用来保留原始 `private__` 名字。
- `src/Passes/PassManager.cpp:311` 将 pass 放在 `EvmCalldataAccessPass` 之后、`InstCombinePass` 之前。
- `src/Passes/evm/SolidityPatterns.cpp:99` 调整 `isPrivateHelperCall()`，让后续 pass 同时接受 `private__` 前缀和 `notdec.evm.original_private_helper` metadata。
- `src/Passes/evm/solidity-patterns/AbiDecoderHelperRenamePass.cpp:32` 到 `:395` 新增实现。当前只扫描小的 `private__` 函数，要求第二个参数是 `%calldata`，按函数体形状分类为：
  - `abi_decode_word_from_calldata`
  - `abi_decode_calldata_dynamic_head`
  - `abi_decode_calldata_array_head`
  - `calldata_array_index_access`
  - `calldata_access_dynamic_tail`
- `test/run_evm_solidity_patterns_suite.py:280`、`:556`、`:693` 增加 `expected_function_definitions` oracle，用函数定义名计数验证重命名。
- `test/evm/solidity-patterns/cases/abi_decoder_helper_rename_01.ll:11` 增加回归样例，覆盖 word decoder、array head、array index access。
- `test/evm/solidity-patterns/manifest.json:64` 接入该样例。

实现中做过一次收紧：`calldata_array_index_access` 不能只看函数内有 unsigned compare，否则真实样例里会误命名业务 helper。当前要求返回值里能追到非指针参数乘/左移常量的 index scale，并且函数内没有 calldata load，降低误匹配风险。

验证：

```bash
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-patterns/cases/abi_decoder_helper_rename_01.ll --tr-level=2 --emit-tr-input-ir=/tmp/abi-decoder-helper-rename-pretr.ll
rg -n "define .*@(abi_decode|calldata_array)|notdec.evm.original_private_helper|private__0x" /tmp/abi-decoder-helper-rename-pretr.ll
./llvm-22.1.0.obj/bin/llvm-as /tmp/abi-decoder-helper-rename-pretr.ll -o /tmp/abi-decoder-helper-rename-pretr.bc
./build/bin/notdec test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll --tr-level=2 --emit-tr-input-ir=/tmp/0334-abi-decoder-rename-pretr.ll
rg -n "define .*@(abi_decode|calldata_array|calldata_access).*notdec.evm.original_private_helper" /tmp/0334-abi-decoder-rename-pretr.ll | head -n 120
./llvm-22.1.0.obj/bin/llvm-as /tmp/0334-abi-decoder-rename-pretr.ll -o /tmp/0334-abi-decoder-rename-pretr.bc
/usr/bin/time -f 'elapsed %e' ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.type_recovery.evm.tr_level_2' --output-on-failure
git diff --check
```

结果：

- 专门 fixture 里生成了 `abi_decode_word_from_calldata__0x100`、`abi_decode_calldata_array_head__0x200`、`calldata_array_index_access__0x300`，并保留 `notdec.evm.original_private_helper` metadata。
- 真实样例 `0334_19494307_668d201319_1354ce2e324d.ll` 只重命名了 3 个 `abi_decode_word_from_calldata__...`，没有再大面积误命名 `calldata_array_index_access`。
- `notdec.evm.solidity_patterns` 和 `notdec.type_recovery.evm.tr_level_2` 均通过，总耗时 `447.64s`。
- `git diff --check` 通过。

复杂度评估：

- 实现效果：8/10。能恢复一批稳定 helper 名，且不改变 IR 语义。
- 理解成本：6/10。新增了一个独立 pass 和少量 matcher，但入口和作用都比较窄。
- 维护成本：6/10。后续如果要扩覆盖，应该继续优先加保守 fixture 和真实样例检查，不要把 matcher 扩成“看见 calldata 就改名”。
