# Solidity 小例子迭代记录

## 原始需求

新建一个logs下的文件负责介绍当前不断增加小例子，改进迭代Solidity结果的过程。后续的改动日志也记录到里面

## 目标

这条线按小 Solidity 源码推进反编译结果。每次只选一个很小的源码或一个很小的语义点，先看当前输出，再修后端或前面 EVM rewrite，输出足够稳定后放进自动测试。

目标不是一次性恢复完整 Solidity，而是让每个已经修好的点都有固定样例和 expected，后面改代码时能自动发现退化。

## 测试位置

- 源码证据：`test/evm/solidity-source/cases/*.sol`
- 可选 bytecode：`test/evm/solidity-source/bytecode/*.hex`
- 冻结 IR：`test/evm/solidity-source/ir/*.ll`
- 固化输出：`test/evm/solidity-source/expected/*.sol`
- manifest：`test/evm/solidity-source/manifest.json`
- runner：`test/run_evm_solidity_source_suite.py`
- CTest：`notdec.evm.solidity_source`

已有 suite 搭建和前两条 case 的历史记录在 `logs/20260630-01-evm-solidity-source-suite.md`。从这个文件开始，后续同类改动都追加到这里。

## 迭代流程

1. 选一个小源码，最好只有一个主要语义点，比如 `return`、`require`、简单 storage getter。
2. 固定输入，优先用已经验证过的冻结 `.ll`。不要让测试依赖临时 solc、Gigahorse 或外部数据目录。
3. 跑当前 `notdec`，看 `.sol` 输出里最明显的问题。
4. 小范围修代码。优先修 Solidity backend；只有输入语义确实缺失时，才改 EVM rewrite 或前面 pass。
5. 输出达到当前可接受程度后，把源码、IR、expected 和 manifest 一起固化。
6. 跑 `notdec.evm.solidity_source`。影响 Solidity/storage rewrite 时，再跑 `notdec.evm.solidity_rewrite` 和 EVM 性能 smoke。
7. 在本文件追加实现记录，写清楚改了哪里、验证命令和结果。

## 接受标准

- expected 不能只是“当前烂输出截图”。必须比修复前更接近源码语义。
- 不确定的语义保留注释或低层 fallback，不硬猜成源码。
- 一个 case 只承担一个主要检查点，避免 expected 变成难维护的大文件。
- 后端打印层只消费已有 metadata、helper、HType 和 structuring 输出，不在打印层重做复杂分析。
- 每次改动后关注性能。只影响 EVM Solidity/storage 链路时，默认用：

```bash
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-smoke.sol --tr-level=2
```

## 记录格式

后续每次追加一个小节，格式保持简单：

````markdown
## YYYY-MM-DD：一句话说明

问题：

改动：

- `path:line` 说明改了哪个函数或文件。

验证：

```bash
命令
```

结果：

性能：
````

如果只是新增 expected 或文档，不需要写性能。涉及子模块时，先提交子模块，再提交顶层指针和本日志。

## 2026-07-01：建立本记录

问题：

前面已经建立了 Solidity source suite，并开始按 `empty_runtime`、`revert_error_string_01` 这种小例子修输出，但后续实现记录还散在原始 suite 日志里。

改动：

- `logs/20260701-02-solidity-small-cases-iteration.md:1` 新建本文件，作为后续 Solidity 小例子迭代的主记录。

验证：

文档改动，未跑测试。

结果：

后续同类改动日志追加到本文件。已有历史不搬迁，保留在 `logs/20260630-01-evm-solidity-source-suite.md`。

## 2026-07-01：return uint literal

问题：

`return_uint_literal_01` 的源码语义是 `return 7;`。修复前 `Reader` 能根据 `evm_return(..., 32)` 打印 `returns (uint256 ret0)`，但 `BodyBuilder` 不收集 `evm_return`，函数体只剩 `// block_0:`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:280) 新增常量整数、`inttoptr` 常量和返回值格式化 helper。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:312) 新增 `formatSingleWordReturn()`，只处理同一 basic block 内 `store i256 X` 到常量 offset，随后 `evm_return(..., offset, 32)` 的单 word 返回。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:367) 在 `collectStatements()` 中把匹配到的 `evm_return` 收集成 `return X;`。
- [test/evm/solidity-source/cases/return_uint_literal_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_literal_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_literal_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_literal_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_literal_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_literal_01.sol:1) 固化输出，函数体现在包含 `return 7;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:20) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_literal_01.ll -o /tmp/return_uint_literal_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_literal_01.ll -o /tmp/return_uint_literal_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-return-smoke.sol --tr-level=2
```

结果：

`return_uint_literal_01` 当前输出：

```solidity
contract Decompiled {
    function answer() public returns (uint256 ret0) {
        // block_0:
        return 7;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 0.54 秒；`notdec.evm.solidity_rewrite` 通过，用时 101.10 秒。

性能：

EVM smoke 用时 `elapsed=30.50 user=34.66 sys=0.73 maxrss=981444`。

## 2026-07-01：return uint add args

问题：

`return_uint_add_args_01` 的源码语义是 `return a + b;`。修复前单 word return 规则已经能识别 `evm_return(..., 32)`，但返回值是 `%sum = add i256 %arg0, %arg1` 时只打印 `return sum;`，没有展开成 Solidity 表达式。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:305) 新增 `binaryOperatorText()`，先支持 `add`、`sub`、`mul`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:318) 扩展 `formatReturnValue()`，返回值是简单 `BinaryOperator` 时打印 `lhs op rhs`。
- [test/evm/solidity-source/cases/return_uint_add_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_add_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_add_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_add_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_add_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_add_args_01.sol:1) 固化输出，函数体现在包含 `return arg0 + arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:26) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_add_args_01.ll -o /tmp/return_uint_add_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_add_args_01.ll -o /tmp/return_uint_add_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-add-smoke.sol --tr-level=2
```

结果：

`return_uint_add_args_01` 当前输出：

```solidity
contract Decompiled {
    function add(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 0.66 秒；`notdec.evm.solidity_rewrite` 通过，用时 101.41 秒。

性能：

EVM smoke 用时 `elapsed=31.36 user=35.41 sys=0.75 maxrss=984496`。

## 2026-07-01：return nested expression parentheses

问题：

`return_uint_nested_expr_01` 的源码语义是 `return (a + b) * c;`。修复前 `formatReturnValue()` 会递归打印二元表达式，但没有按 Solidity 运算符优先级加括号，输出变成 `return arg0 + arg1 * arg2;`，语义错误。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:318) 新增 `binaryOperatorPrecedence()`，为 `+`、`-`、`*` 提供最小优先级。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:330) 新增 `needsParentheses()`，子表达式优先级更低时加括号，同时保守处理右侧减法结合性。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:339) 扩展 `formatReturnValue()`，递归格式化返回表达式时传递父运算符信息。
- [test/evm/solidity-source/cases/return_uint_nested_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_nested_expr_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_nested_expr_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_nested_expr_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_nested_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_nested_expr_01.sol:1) 固化输出，函数体现在包含 `return (arg0 + arg1) * arg2;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:32) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_nested_expr_01.ll -o /tmp/return_uint_nested_expr_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_nested_expr_01.ll -o /tmp/return_uint_nested_expr_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-nested-expr-smoke.sol --tr-level=2
```

结果：

`return_uint_nested_expr_01` 当前输出：

```solidity
contract Decompiled {
    function mix(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return (arg0 + arg1) * arg2;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 0.83 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.76 秒。

性能：

EVM smoke 用时 `elapsed=30.81 user=34.92 sys=0.70 maxrss=985400`。

## 2026-07-01：return sub rhs expression coverage

问题：

上一轮已经修了嵌套表达式括号，但还缺少 `a - (b + c)` 这种右侧同优先级表达式的回归覆盖。当前后端已经能正确打印 `arg0 - (arg1 + arg2)`，这次只把行为固化下来。

改动：

- [test/evm/solidity-source/cases/return_uint_sub_rhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_sub_rhs_expr_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_sub_rhs_expr_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_sub_rhs_expr_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_sub_rhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_sub_rhs_expr_01.sol:1) 固化输出，函数体包含 `return arg0 - (arg1 + arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:38) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_sub_rhs_expr_01.ll -o /tmp/return_uint_sub_rhs_expr_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_sub_rhs_expr_01.ll -o /tmp/return_uint_sub_rhs_expr_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
```

结果：

`return_uint_sub_rhs_expr_01` 当前输出：

```solidity
contract Decompiled {
    function subExpr(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 - (arg1 + arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 0.90 秒。

性能：

只新增 source case 和 expected，没有改代码，未跑性能 smoke。

## 2026-07-01：return uint division

问题：

`return_uint_div_args_01` 的源码语义是 `return a / b;`。EVM IR 里 unsigned division 是 `evm_div(arg0, arg1)` helper。修复前单 word return 规则能找到返回值，但 `formatReturnValue()` 不认识 `evm_div`，只能打印 `return quotient;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:330) 新增 `evmBinaryOperatorText()`，先把 `evm_div` 映射成 Solidity `/`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:370) 扩展 `formatReturnValue()`，返回值是两参数 EVM helper call 时按二元表达式打印。
- [test/evm/solidity-source/cases/return_uint_div_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_div_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_div_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_div_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_div_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_div_args_01.sol:1) 固化输出，函数体包含 `return arg0 / arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:44) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_div_args_01.ll -o /tmp/return_uint_div_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_div_args_01.ll -o /tmp/return_uint_div_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-div-smoke.sol --tr-level=2
```

结果：

`return_uint_div_args_01` 当前输出：

```solidity
contract Decompiled {
    function div(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.02 秒；`notdec.evm.solidity_rewrite` 通过，用时 101.14 秒。

性能：

EVM smoke 用时 `elapsed=30.80 user=34.64 sys=0.77 maxrss=986840`。

## 2026-07-01：return uint modulo

问题：

`return_uint_mod_args_01` 的源码语义是 `return a % b;`。EVM IR 里 unsigned modulo 是 `evm_mod(arg0, arg1)` helper。修复前 `formatReturnValue()` 不认识 `evm_mod`，只能打印 `return remainder;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:330) 扩展 `evmBinaryOperatorText()`，把 `evm_mod` 映射成 Solidity `%`。
- [test/evm/solidity-source/cases/return_uint_mod_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_mod_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_mod_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_mod_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_mod_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_mod_args_01.sol:1) 固化输出，函数体包含 `return arg0 % arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:50) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_mod_args_01.ll -o /tmp/return_uint_mod_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_mod_args_01.ll -o /tmp/return_uint_mod_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-mod-smoke.sol --tr-level=2
```

结果：

`return_uint_mod_args_01` 当前输出：

```solidity
contract Decompiled {
    function mod(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 % arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.13 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.92 秒。

性能：

EVM smoke 用时 `elapsed=30.90 user=34.96 sys=0.72 maxrss=984356`。

## 2026-07-01：return uint left shift

问题：

`return_uint_shl_args_01` 的源码语义是 `return value << shift;`。EVM helper 是 `evm_shl(shift, value)`，参数顺序和 Solidity 表达式相反。修复前 `formatReturnValue()` 不认识 `evm_shl`，只能打印 `return shifted;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:340) 新增 `evmShiftOperatorText()`，先把 `evm_shl` 映射成 Solidity `<<`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:383) 扩展 `formatReturnValue()`，打印 shift helper 时把 helper 参数反过来，输出 `value << shift`。
- [test/evm/solidity-source/cases/return_uint_shl_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_shl_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_shl_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_shl_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_shl_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_shl_args_01.sol:1) 固化输出，函数体包含 `return arg0 << arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:56) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_shl_args_01.ll -o /tmp/return_uint_shl_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_shl_args_01.ll -o /tmp/return_uint_shl_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-shl-smoke.sol --tr-level=2
```

结果：

`return_uint_shl_args_01` 当前输出：

```solidity
contract Decompiled {
    function shl(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 << arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.25 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.52 秒。

性能：

EVM smoke 用时 `elapsed=30.70 user=34.83 sys=0.81 maxrss=956376`。

## 2026-07-01：return uint right shift

问题：

`return_uint_shr_args_01` 的源码语义是 `return value >> shift;`。EVM helper 是 `evm_shr(shift, value)`，参数顺序和 Solidity 表达式相反。修复前 `formatReturnValue()` 不认识 `evm_shr`，只能打印 `return shifted;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:340) 扩展 `evmShiftOperatorText()`，把 `evm_shr` 映射成 Solidity `>>`。
- [test/evm/solidity-source/cases/return_uint_shr_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_shr_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_shr_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_shr_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_shr_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_shr_args_01.sol:1) 固化输出，函数体包含 `return arg0 >> arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:62) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_shr_args_01.ll -o /tmp/return_uint_shr_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_shr_args_01.ll -o /tmp/return_uint_shr_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-shr-smoke.sol --tr-level=2
```

结果：

`return_uint_shr_args_01` 当前输出：

```solidity
contract Decompiled {
    function shr(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 >> arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.41 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.80 秒。

性能：

EVM smoke 用时 `elapsed=31.03 user=35.51 sys=0.79 maxrss=956816`。

## 2026-07-01：return uint bitwise and

问题：

`return_uint_and_args_01` 的源码语义是 `return left & right;`。冻结 IR 里 EVM `AND` 已经是 LLVM 原生 `and i256`。修复前 `formatReturnValue()` 只认识 `add/sub/mul`，所以只能打印 `return masked;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:305) 扩展 `binaryOperatorText()`，把 `llvm::Instruction::And` 打印成 Solidity `&`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:320) 扩展 `binaryOperatorPrecedence()`，给 `&` 设置比加减更低的优先级。
- [test/evm/solidity-source/cases/return_uint_and_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_and_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_and_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_and_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_and_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_and_args_01.sol:1) 固化输出，函数体包含 `return arg0 & arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:68) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_and_args_01.ll -o /tmp/return_uint_and_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_and_args_01.ll -o /tmp/return_uint_and_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-and-smoke.sol --tr-level=2
```

结果：

`return_uint_and_args_01` 当前输出：

```solidity
contract Decompiled {
    function bitand(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 & arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.47 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.94 秒。

性能：

EVM smoke 用时 `elapsed=29.87 user=34.00 sys=0.70 maxrss=983332`。

## 2026-07-01：return uint bitwise or

问题：

`return_uint_or_args_01` 的源码语义是 `return left | right;`。冻结 IR 里 EVM `OR` 已经是 LLVM 原生 `or i256`。修复前 `formatReturnValue()` 不认识 `llvm::Instruction::Or`，所以只能打印 `return combined;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:305) 扩展 `binaryOperatorText()`，把 `llvm::Instruction::Or` 打印成 Solidity `|`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:322) 扩展 `binaryOperatorPrecedence()`，给 `|` 设置比 `&` 更低的优先级。
- [test/evm/solidity-source/cases/return_uint_or_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_or_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_or_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_or_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_or_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_or_args_01.sol:1) 固化输出，函数体包含 `return arg0 | arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:74) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_or_args_01.ll -o /tmp/return_uint_or_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_or_args_01.ll -o /tmp/return_uint_or_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-or-smoke.sol --tr-level=2
```

结果：

`return_uint_or_args_01` 当前输出：

```solidity
contract Decompiled {
    function bitor(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 | arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.62 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.48 秒。

性能：

EVM smoke 用时 `elapsed=30.15 user=34.22 sys=0.78 maxrss=983196`。

## 2026-07-01：return uint bitwise xor

问题：

`return_uint_xor_args_01` 的源码语义是 `return left ^ right;`。冻结 IR 里 EVM `XOR` 已经是 LLVM 原生 `xor i256`。修复前 `formatReturnValue()` 不认识 `llvm::Instruction::Xor`，所以只能打印 `return mixed;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:305) 扩展 `binaryOperatorText()`，把 `llvm::Instruction::Xor` 打印成 Solidity `^`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:324) 扩展 `binaryOperatorPrecedence()`，给 `^` 设置在 `&` 和 `|` 之间的优先级。
- [test/evm/solidity-source/cases/return_uint_xor_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_xor_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_xor_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_xor_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_xor_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_xor_args_01.sol:1) 固化输出，函数体包含 `return arg0 ^ arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:80) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_xor_args_01.ll -o /tmp/return_uint_xor_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_xor_args_01.ll -o /tmp/return_uint_xor_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-xor-smoke.sol --tr-level=2
```

结果：

`return_uint_xor_args_01` 当前输出：

```solidity
contract Decompiled {
    function bitxor(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 ^ arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.69 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.03 秒。

性能：

EVM smoke 用时 `elapsed=30.02 user=34.21 sys=0.68 maxrss=982272`。

## 2026-07-01：return uint bitwise not

问题：

`return_uint_not_arg_01` 的源码语义是 `return ~value;`。冻结 IR 里 EVM `NOT` 是 `xor i256 %arg0, -1`。修复前 `formatReturnValue()` 会按普通 `^` 打印成 `return arg0 ^ 115792089237316195423570985008687907853269984665640564039457584007913129639935;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:287) 新增 `isAllOnesConstant()`，识别 LLVM 常量里的全 1 位形。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:367) 新增 `bitwiseNotOperand()`，识别 `xor X, -1` 和 `xor -1, X`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:389) 扩展 `formatReturnValue()`，把该模式打印成 Solidity `~X`。
- [test/evm/solidity-source/cases/return_uint_not_arg_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_not_arg_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_not_arg_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_not_arg_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_not_arg_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_not_arg_01.sol:1) 固化输出，函数体包含 `return ~arg0;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:86) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_not_arg_01.ll -o /tmp/return_uint_not_arg_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_not_arg_01.ll -o /tmp/return_uint_not_arg_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-not-smoke.sol --tr-level=2
```

结果：

`return_uint_not_arg_01` 当前输出：

```solidity
contract Decompiled {
    function bitnot(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return ~arg0;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.83 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.53 秒。

性能：

EVM smoke 用时 `elapsed=29.83 user=33.97 sys=0.65 maxrss=981888`。

## 2026-07-01：return uint exponent

问题：

`return_uint_exp_args_01` 的源码语义是 `return base ** exponent;`。冻结 IR 里 EVM `EXP` 是 `evm_exp(base, exponent)`。修复前 `formatReturnValue()` 不认识 `evm_exp`，只能打印 `return powered;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:347) 扩展 `evmBinaryOperatorText()`，把 `evm_exp` 打印成 Solidity `**`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:360) 新增 `evmBinaryOperatorPrecedence()`，让 `**` 的优先级高于乘除模。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:431) 在 `formatReturnValue()` 中使用 EVM helper 自己的优先级。
- [test/evm/solidity-source/cases/return_uint_exp_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_exp_args_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_exp_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_exp_args_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_exp_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_exp_args_01.sol:1) 固化输出，函数体包含 `return arg0 ** arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:92) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_exp_args_01.ll -o /tmp/return_uint_exp_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_exp_args_01.ll -o /tmp/return_uint_exp_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-exp-smoke.sol --tr-level=2
```

结果：

`return_uint_exp_args_01` 当前输出：

```solidity
contract Decompiled {
    function exp(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return arg0 ** arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 1.97 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.00 秒。

性能：

EVM smoke 用时 `elapsed=30.18 user=34.32 sys=0.72 maxrss=983616`。

## 2026-07-01：return uint division rhs expression

问题：

`return_uint_div_rhs_expr_01` 的源码语义是 `return numerator / (denominator / scale);`。冻结 IR 里是两层 `evm_div`。修复前 `formatReturnValue()` 只按优先级决定括号，同优先级右操作数没有区分非结合运算，错误打印成 `return arg0 / arg1 / arg2;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:390) 新增 `rightOperandNeedsSamePrecedenceParentheses()`，标记 `-`、`/`、`%`、`<<`、`>>` 这类右操作数同优先级时需要括号的运算符。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:395) 调整 `needsParentheses()`，把“右操作数同优先级要不要加括号”变成调用方传入的规则。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:405) 扩展 `formatReturnValue()`，格式化二元表达式和 EVM helper 时把该规则传给子表达式。
- [test/evm/solidity-source/cases/return_uint_div_rhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_div_rhs_expr_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_div_rhs_expr_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_div_rhs_expr_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_div_rhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_div_rhs_expr_01.sol:1) 固化输出，函数体包含 `return arg0 / (arg1 / arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:98) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_div_rhs_expr_01.ll -o /tmp/return_uint_div_rhs_expr_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_div_rhs_expr_01.ll -o /tmp/return_uint_div_rhs_expr_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-div-rhs-smoke.sol --tr-level=2
```

结果：

`return_uint_div_rhs_expr_01` 当前输出：

```solidity
contract Decompiled {
    function divrhs(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 / (arg1 / arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.06 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.14 秒。

性能：

EVM smoke 用时 `elapsed=30.19 user=34.27 sys=0.75 maxrss=985852`。

## 2026-07-01：return uint multiply rhs division

问题：

`return_uint_mul_rhs_div_01` 的源码语义是 `return left * (right / scale);`。冻结 IR 里右操作数是 `evm_div(right, scale)`。修复前 `formatReturnValue()` 没有把 `*` 的右侧同优先级表达式加括号，错误打印成 `return arg0 * arg1 / arg2;`，整数除法语义会变。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:390) 扩展 `rightOperandNeedsSamePrecedenceParentheses()`，把 `*` 也纳入右操作数同优先级需要括号的运算符。
- [test/evm/solidity-source/cases/return_uint_mul_rhs_div_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_mul_rhs_div_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_mul_rhs_div_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_mul_rhs_div_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_mul_rhs_div_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_mul_rhs_div_01.sol:1) 固化输出，函数体包含 `return arg0 * (arg1 / arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:104) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_mul_rhs_div_01.ll -o /tmp/return_uint_mul_rhs_div_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_mul_rhs_div_01.ll -o /tmp/return_uint_mul_rhs_div_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-mul-rhs-div-smoke.sol --tr-level=2
```

结果：

`return_uint_mul_rhs_div_01` 当前输出：

```solidity
contract Decompiled {
    function mulrhsdiv(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 * (arg1 / arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.19 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.58 秒。

性能：

EVM smoke 用时 `elapsed=30.13 user=35.53 sys=0.76 maxrss=985772`。

## 2026-07-01：return uint exponent lhs expression

问题：

`return_uint_exp_lhs_expr_01` 的源码语义是 `return (base ** exponent) ** outer;`。Solidity 的 `**` 是右结合，修复前 `formatReturnValue()` 没有给左侧同优先级 `**` 子表达式加括号，错误打印成 `return arg0 ** arg1 ** arg2;`，语义会变成 `arg0 ** (arg1 ** arg2)`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:396) 新增 `leftOperandNeedsSamePrecedenceParentheses()`，把 `**` 的左操作数标记为同优先级需要括号。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:400) 调整 `needsParentheses()`，支持左、右操作数分别传入同优先级括号规则。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:409) 扩展 `formatReturnValue()`，格式化二元表达式和 EVM helper 时也把左操作数规则传给子表达式。
- [test/evm/solidity-source/cases/return_uint_exp_lhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_exp_lhs_expr_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_exp_lhs_expr_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_exp_lhs_expr_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_exp_lhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_exp_lhs_expr_01.sol:1) 固化输出，函数体包含 `return (arg0 ** arg1) ** arg2;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:110) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_exp_lhs_expr_01.ll -o /tmp/return_uint_exp_lhs_expr_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_exp_lhs_expr_01.ll -o /tmp/return_uint_exp_lhs_expr_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-exp-lhs-smoke.sol --tr-level=2
```

结果：

`return_uint_exp_lhs_expr_01` 当前输出：

```solidity
contract Decompiled {
    function explhs(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return (arg0 ** arg1) ** arg2;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.30 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.79 秒。

性能：

EVM smoke 用时 `elapsed=30.08 user=34.39 sys=0.70 maxrss=983276`。

## 2026-07-01：return uint exponent rhs expression

问题：

`return_uint_exp_rhs_expr_01` 的源码语义是 `return base ** (exponent ** outer);`。Solidity 的 `**` 是右结合，当前输出 `return arg0 ** arg1 ** arg2;` 能保持语义。这个 case 用来防止后续括号规则把右侧 `**` 误加括号或误改结合性。

改动：

- [test/evm/solidity-source/cases/return_uint_exp_rhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_exp_rhs_expr_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_exp_rhs_expr_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_exp_rhs_expr_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_exp_rhs_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_exp_rhs_expr_01.sol:1) 固化输出，函数体包含 `return arg0 ** arg1 ** arg2;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:116) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_exp_rhs_expr_01.ll -o /tmp/return_uint_exp_rhs_expr_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_exp_rhs_expr_01.ll -o /tmp/return_uint_exp_rhs_expr_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-exp-rhs-smoke.sol --tr-level=2
```

结果：

`return_uint_exp_rhs_expr_01` 当前输出：

```solidity
contract Decompiled {
    function exprhs(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 ** arg1 ** arg2;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.45 秒。

性能：

EVM smoke 用时 `elapsed=30.40 user=34.60 sys=0.81 maxrss=982404`。

## 2026-07-01：return uint bitwise and rhs or

问题：

`return_uint_and_rhs_or_01` 的源码语义是 `return left & (middle | right);`。`|` 的优先级低于 `&`，当前输出会保留右侧括号，打印成 `return arg0 & (arg1 | arg2);`。这个 case 用来固化不同位运算优先级下的括号行为。

改动：

- [test/evm/solidity-source/cases/return_uint_and_rhs_or_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_and_rhs_or_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_and_rhs_or_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_and_rhs_or_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_and_rhs_or_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_and_rhs_or_01.sol:1) 固化输出，函数体包含 `return arg0 & (arg1 | arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:122) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_and_rhs_or_01.ll -o /tmp/return_uint_and_rhs_or_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_and_rhs_or_01.ll -o /tmp/return_uint_and_rhs_or_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-and-rhs-or-smoke.sol --tr-level=2
```

结果：

`return_uint_and_rhs_or_01` 当前输出：

```solidity
contract Decompiled {
    function andrhsor(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 & (arg1 | arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.55 秒。

性能：

EVM smoke 用时 `elapsed=30.18 user=34.36 sys=0.66 maxrss=984168`。

## 2026-07-01：return uint bitwise or rhs and

问题：

`return_uint_or_rhs_and_01` 的源码语义是 `return left | (middle & right);`。`&` 的优先级高于 `|`，当前输出可以省略右侧括号，打印成 `return arg0 | arg1 & arg2;`。这个 case 用来固化不同位运算优先级下的省括号行为。

改动：

- [test/evm/solidity-source/cases/return_uint_or_rhs_and_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_or_rhs_and_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_or_rhs_and_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_or_rhs_and_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_or_rhs_and_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_or_rhs_and_01.sol:1) 固化输出，函数体包含 `return arg0 | arg1 & arg2;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:128) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_or_rhs_and_01.ll -o /tmp/return_uint_or_rhs_and_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_or_rhs_and_01.ll -o /tmp/return_uint_or_rhs_and_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-or-rhs-and-smoke.sol --tr-level=2
```

结果：

`return_uint_or_rhs_and_01` 当前输出：

```solidity
contract Decompiled {
    function orrhsand(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 | arg1 & arg2;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.68 秒。

性能：

EVM smoke 用时 `elapsed=30.08 user=34.41 sys=0.60 maxrss=983648`。

## 2026-07-01：return uint add rhs shift

问题：

`return_uint_add_rhs_shl_01` 的源码语义是 `return value + (base << shift);`。Solidity 中加减优先级高于 shift。修复前 Solidity backend 把 shift 优先级放在加法之上，错误打印成 `return arg0 + arg1 << arg2;`，语义会变成 `(arg0 + arg1) << arg2`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:448) 调整 `formatReturnValue()` 中 `evm_shl` / `evm_shr` 的优先级，从高于加减改成低于加减、高于按位与。
- [test/evm/solidity-source/cases/return_uint_add_rhs_shl_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_add_rhs_shl_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_add_rhs_shl_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_add_rhs_shl_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_add_rhs_shl_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_add_rhs_shl_01.sol:1) 固化输出，函数体包含 `return arg0 + (arg1 << arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:134) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_add_rhs_shl_01.ll -o /tmp/return_uint_add_rhs_shl_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_add_rhs_shl_01.ll -o /tmp/return_uint_add_rhs_shl_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-add-rhs-shl-smoke.sol --tr-level=2
```

结果：

`return_uint_add_rhs_shl_01` 当前输出：

```solidity
contract Decompiled {
    function addrhsshl(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 + (arg1 << arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.79 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.33 秒。

性能：

EVM smoke 用时 `elapsed=30.04 user=34.18 sys=0.69 maxrss=983784`。

## 2026-07-01：return uint shift rhs add

问题：

`return_uint_shl_rhs_add_01` 的源码语义是 `return value << (shift + extra);`。Solidity 中加法优先级高于 shift，当前输出可以省略右侧括号，打印成 `return arg0 << arg1 + arg2;`。这个 case 用来固化 shift 和加法优先级下的省括号行为。

改动：

- [test/evm/solidity-source/cases/return_uint_shl_rhs_add_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_shl_rhs_add_01.sol:1) 新增源码证据。
- [test/evm/solidity-source/ir/return_uint_shl_rhs_add_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_shl_rhs_add_01.ll:1) 新增冻结 IR。
- [test/evm/solidity-source/expected/return_uint_shl_rhs_add_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_shl_rhs_add_01.sol:1) 固化输出，函数体包含 `return arg0 << arg1 + arg2;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:140) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_shl_rhs_add_01.ll -o /tmp/return_uint_shl_rhs_add_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_shl_rhs_add_01.ll -o /tmp/return_uint_shl_rhs_add_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-shl-rhs-add-smoke.sol --tr-level=2
```

结果：

`return_uint_shl_rhs_add_01` 当前输出：

```solidity
contract Decompiled {
    function shlrhsadd(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 << arg1 + arg2;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 2.89 秒。

性能：

EVM smoke 用时 `elapsed=30.22 user=34.43 sys=0.77 maxrss=984068`。

## 2026-07-01：return uint shift rhs and

问题：

`return_uint_shl_rhs_and_01` 的源码语义是 `return value << (shift & mask);`。Solidity 中 shift 优先级高于按位与，如果输出成 `arg0 << arg1 & arg2`，语义会变成 `(arg0 << arg1) & arg2`。当前 backend 已能打印 RHS 括号，本次只固化这个行为。

改动：

- [test/evm/solidity-source/cases/return_uint_shl_rhs_and_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_shl_rhs_and_01.sol:1) 新增源码证据，源码第 6 行包含 `return value << (shift & mask);`。
- [test/evm/solidity-source/ir/return_uint_shl_rhs_and_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_shl_rhs_and_01.ll:9) 新增冻结 IR，`public_shlrhsand_uint256_uint256_uint256__0x2a()` 先生成 `and`，再调用 `evm_shl`。
- [test/evm/solidity-source/expected/return_uint_shl_rhs_and_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_shl_rhs_and_01.sol:1) 固化输出，函数体包含 `return arg0 << (arg1 & arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:146) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_shl_rhs_and_01.ll -o /tmp/return_uint_shl_rhs_and_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_shl_rhs_and_01.ll -o /tmp/return_uint_shl_rhs_and_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-shl-rhs-and-smoke.sol --tr-level=2
```

结果：

`return_uint_shl_rhs_and_01` 当前输出：

```solidity
contract Decompiled {
    function shlrhsand(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 << (arg1 & arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.12 秒。

性能：

EVM smoke 用时 `elapsed=30.43 user=34.44 sys=0.79 maxrss=983096`。

## 2026-07-01：return uint shift lhs and

问题：

`return_uint_shl_lhs_and_01` 的源码语义是 `return (value & mask) << shift;`。Solidity 中 shift 优先级高于按位与，左侧按位与必须保留括号，否则 `arg0 & arg1 << arg2` 会变成 `arg0 & (arg1 << arg2)`。当前 backend 已能打印正确括号，本次只固化这个行为。

改动：

- [test/evm/solidity-source/cases/return_uint_shl_lhs_and_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_shl_lhs_and_01.sol:1) 新增源码证据，源码第 6 行包含 `return (value & mask) << shift;`。
- [test/evm/solidity-source/ir/return_uint_shl_lhs_and_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_shl_lhs_and_01.ll:9) 新增冻结 IR，`public_shllhsand_uint256_uint256_uint256__0x2a()` 先生成 `and`，再调用 `evm_shl`。
- [test/evm/solidity-source/expected/return_uint_shl_lhs_and_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_shl_lhs_and_01.sol:1) 固化输出，函数体包含 `return (arg0 & arg1) << arg2;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:152) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_shl_lhs_and_01.ll -o /tmp/return_uint_shl_lhs_and_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_shl_lhs_and_01.ll -o /tmp/return_uint_shl_lhs_and_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-shl-lhs-and-smoke.sol --tr-level=2
```

结果：

`return_uint_shl_lhs_and_01` 当前输出：

```solidity
contract Decompiled {
    function shllhsand(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return (arg0 & arg1) << arg2;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.10 秒。

性能：

EVM smoke 用时 `elapsed=30.34 user=34.61 sys=0.74 maxrss=985156`。

## 2026-07-01：return uint bitwise not rhs add

问题：

`return_uint_not_rhs_add_01` 的源码语义是 `return ~(value + extra);`。EVM `NOT` 仍然是 `xor X, -1`，这里重点检查 `~` 的子表达式是加法时是否保留括号。当前 backend 已能打印 `~(arg0 + arg1)`，本次只固化这个行为。

改动：

- [test/evm/solidity-source/cases/return_uint_not_rhs_add_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_not_rhs_add_01.sol:1) 新增源码证据，源码第 6 行包含 `return ~(value + extra);`。
- [test/evm/solidity-source/ir/return_uint_not_rhs_add_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_not_rhs_add_01.ll:8) 新增冻结 IR，`public_notrhsadd_uint256_uint256__0x2a()` 先生成 `add`，再用 `xor -1` 表达 `NOT`。
- [test/evm/solidity-source/expected/return_uint_not_rhs_add_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_not_rhs_add_01.sol:1) 固化输出，函数体包含 `return ~(arg0 + arg1);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:158) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_not_rhs_add_01.ll -o /tmp/return_uint_not_rhs_add_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_not_rhs_add_01.ll -o /tmp/return_uint_not_rhs_add_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-not-rhs-add-smoke.sol --tr-level=2
```

结果：

`return_uint_not_rhs_add_01` 当前输出：

```solidity
contract Decompiled {
    function notrhsadd(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return ~(arg0 + arg1);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.22 秒。

性能：

EVM smoke 用时 `elapsed=30.19 user=34.44 sys=0.56 maxrss=981916`。

## 2026-07-01：return uint multiply rhs modulo

问题：

`return_uint_mul_rhs_mod_01` 的源码语义是 `return left * (right % scale);`。`*` 和 `%` 在 Solidity 中同优先级且左结合，如果输出成 `arg0 * arg1 % arg2`，语义会变成 `(arg0 * arg1) % arg2`。当前 backend 已能给乘法右侧 `%` 表达式加括号，本次只固化这个行为。

改动：

- [test/evm/solidity-source/cases/return_uint_mul_rhs_mod_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_mul_rhs_mod_01.sol:1) 新增源码证据，源码第 6 行包含 `return left * (right % scale);`。
- [test/evm/solidity-source/ir/return_uint_mul_rhs_mod_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_mul_rhs_mod_01.ll:9) 新增冻结 IR，`public_mulrhsmod_uint256_uint256_uint256__0x2a()` 先调用 `evm_mod`，再执行 `mul`。
- [test/evm/solidity-source/expected/return_uint_mul_rhs_mod_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_mul_rhs_mod_01.sol:1) 固化输出，函数体包含 `return arg0 * (arg1 % arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:164) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_mul_rhs_mod_01.ll -o /tmp/return_uint_mul_rhs_mod_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_mul_rhs_mod_01.ll -o /tmp/return_uint_mul_rhs_mod_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-mul-rhs-mod-smoke.sol --tr-level=2
```

结果：

`return_uint_mul_rhs_mod_01` 当前输出：

```solidity
contract Decompiled {
    function mulrhsmod(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 * (arg1 % arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.38 秒。

性能：

EVM smoke 用时 `elapsed=30.42 user=34.71 sys=0.67 maxrss=985068`。

## 2026-07-01：return uint modulo rhs multiply

问题：

`return_uint_mod_rhs_mul_01` 的源码语义是 `return numerator % (denominator * scale);`。`%` 和 `*` 在 Solidity 中同优先级且左结合，如果输出成 `arg0 % arg1 * arg2`，语义会变成 `(arg0 % arg1) * arg2`。当前 backend 已能给 `%` 右侧乘法表达式加括号，本次只固化这个行为。

改动：

- [test/evm/solidity-source/cases/return_uint_mod_rhs_mul_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_mod_rhs_mul_01.sol:1) 新增源码证据，源码第 6 行包含 `return numerator % (denominator * scale);`。
- [test/evm/solidity-source/ir/return_uint_mod_rhs_mul_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_mod_rhs_mul_01.ll:9) 新增冻结 IR，`public_modrhsmul_uint256_uint256_uint256__0x2a()` 先执行 `mul`，再调用 `evm_mod`。
- [test/evm/solidity-source/expected/return_uint_mod_rhs_mul_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_mod_rhs_mul_01.sol:1) 固化输出，函数体包含 `return arg0 % (arg1 * arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:170) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_mod_rhs_mul_01.ll -o /tmp/return_uint_mod_rhs_mul_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_mod_rhs_mul_01.ll -o /tmp/return_uint_mod_rhs_mul_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-mod-rhs-mul-smoke.sol --tr-level=2
```

结果：

`return_uint_mod_rhs_mul_01` 当前输出：

```solidity
contract Decompiled {
    function modrhsmul(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return arg0 % (arg1 * arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.52 秒。

性能：

EVM smoke 用时 `elapsed=30.00 user=34.15 sys=0.65 maxrss=982424`。

## 2026-07-01：return uint addmod

问题：

`return_uint_addmod_args_01` 的源码语义是 `return addmod(left, right, modulus);`。冻结 IR 里 EVM `ADDMOD` 是三参数 helper `evm_addmod(arg0, arg1, arg2)`。修复前 `formatReturnValue()` 只处理二元 helper，当前输出退化成 `return result;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:377) 新增 `evmTernaryBuiltinName()`，先只把 `evm_addmod` 映射成 Solidity 内建函数 `addmod`。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:452) 扩展 `formatReturnValue()`，三参数 call 命中 `evmTernaryBuiltinName()` 时打印成 `addmod(a, b, m)`。
- [test/evm/solidity-source/cases/return_uint_addmod_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_addmod_args_01.sol:1) 新增源码证据，源码第 6 行包含 `return addmod(left, right, modulus);`。
- [test/evm/solidity-source/ir/return_uint_addmod_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_addmod_args_01.ll:9) 新增冻结 IR，`public_addmodargs_uint256_uint256_uint256__0x2a()` 调用 `evm_addmod` 后返回。
- [test/evm/solidity-source/expected/return_uint_addmod_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_addmod_args_01.sol:1) 固化输出，函数体包含 `return addmod(arg0, arg1, arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:176) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_addmod_args_01.ll -o /tmp/return_uint_addmod_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_addmod_args_01.ll -o /tmp/return_uint_addmod_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-addmod-smoke.sol --tr-level=2
```

结果：

修复前 `return_uint_addmod_args_01` 输出 `return result;`。修复后输出：

```solidity
contract Decompiled {
    function addmodargs(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return addmod(arg0, arg1, arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.72 秒；`notdec.evm.solidity_rewrite` 通过，用时 101.15 秒。

性能：

EVM smoke 用时 `elapsed=31.15 user=35.22 sys=0.74 maxrss=985148`。

## 2026-07-01：return uint mulmod

问题：

`return_uint_mulmod_args_01` 的源码语义是 `return mulmod(left, right, modulus);`。冻结 IR 里 EVM `MULMOD` 是三参数 helper `evm_mulmod(arg0, arg1, arg2)`。上一步已经让 `formatReturnValue()` 能打印三参数内建函数，但只接了 `evm_addmod`；修复前这个 case 仍退化成 `return result;`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:377) 扩展 `evmTernaryBuiltinName()`，把 `evm_mulmod` 映射成 Solidity 内建函数 `mulmod`。
- [test/evm/solidity-source/cases/return_uint_mulmod_args_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_mulmod_args_01.sol:1) 新增源码证据，源码第 6 行包含 `return mulmod(left, right, modulus);`。
- [test/evm/solidity-source/ir/return_uint_mulmod_args_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_mulmod_args_01.ll:9) 新增冻结 IR，`public_mulmodargs_uint256_uint256_uint256__0x2a()` 调用 `evm_mulmod` 后返回。
- [test/evm/solidity-source/expected/return_uint_mulmod_args_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_mulmod_args_01.sol:1) 固化输出，函数体包含 `return mulmod(arg0, arg1, arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:182) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_mulmod_args_01.ll -o /tmp/return_uint_mulmod_args_01.bc
cmake --build ./build --target notdec -j4
./build/bin/notdec test/evm/solidity-source/ir/return_uint_mulmod_args_01.ll -o /tmp/return_uint_mulmod_args_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-mulmod-smoke.sol --tr-level=2
```

结果：

修复前 `return_uint_mulmod_args_01` 输出 `return result;`。修复后输出：

```solidity
contract Decompiled {
    function mulmodargs(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return mulmod(arg0, arg1, arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.83 秒；`notdec.evm.solidity_rewrite` 通过，用时 100.90 秒。

性能：

EVM smoke 用时 `elapsed=31.07 user=35.28 sys=0.68 maxrss=983532`。

## 2026-07-01：return uint addmod arg expression

问题：

`return_uint_addmod_arg_expr_01` 的源码语义是 `return addmod(left + right, extra, modulus);`。上一步已经支持三参数内建函数 `addmod`，这里检查它的参数本身是表达式时能否继续递归打印，而不是退回临时变量。

改动：

- [test/evm/solidity-source/cases/return_uint_addmod_arg_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_addmod_arg_expr_01.sol:1) 新增源码证据，源码第 6 行包含 `return addmod(left + right, extra, modulus);`。
- [test/evm/solidity-source/ir/return_uint_addmod_arg_expr_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_addmod_arg_expr_01.ll:9) 新增冻结 IR，`public_addmodargexpr_uint256_uint256_uint256_uint256__0x2a()` 先执行 `add`，再把结果传给 `evm_addmod`。
- [test/evm/solidity-source/expected/return_uint_addmod_arg_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_addmod_arg_expr_01.sol:1) 固化输出，函数体包含 `return addmod(arg0 + arg1, arg2, arg3);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:188) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_addmod_arg_expr_01.ll -o /tmp/return_uint_addmod_arg_expr_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_addmod_arg_expr_01.ll -o /tmp/return_uint_addmod_arg_expr_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-addmod-arg-expr-smoke.sol --tr-level=2
```

结果：

`return_uint_addmod_arg_expr_01` 当前输出：

```solidity
contract Decompiled {
    function addmodargexpr(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        return addmod(arg0 + arg1, arg2, arg3);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 3.83 秒。

性能：

EVM smoke 用时 `elapsed=30.00 user=34.10 sys=0.79 maxrss=981976`。

## 2026-07-01：return uint mulmod arg expression

问题：

`return_uint_mulmod_arg_expr_01` 的源码语义是 `return mulmod(left * right, extra, modulus);`。`mulmod` 基础返回已经支持，这里检查它的参数本身是乘法表达式时能否继续递归打印。

改动：

- [test/evm/solidity-source/cases/return_uint_mulmod_arg_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_mulmod_arg_expr_01.sol:1) 新增源码证据，源码第 6 行包含 `return mulmod(left * right, extra, modulus);`。
- [test/evm/solidity-source/ir/return_uint_mulmod_arg_expr_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_mulmod_arg_expr_01.ll:9) 新增冻结 IR，`public_mulmodargexpr_uint256_uint256_uint256_uint256__0x2a()` 先执行 `mul`，再把结果传给 `evm_mulmod`。
- [test/evm/solidity-source/expected/return_uint_mulmod_arg_expr_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_mulmod_arg_expr_01.sol:1) 固化输出，函数体包含 `return mulmod(arg0 * arg1, arg2, arg3);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:194) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_mulmod_arg_expr_01.ll -o /tmp/return_uint_mulmod_arg_expr_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_mulmod_arg_expr_01.ll -o /tmp/return_uint_mulmod_arg_expr_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-mulmod-arg-expr-smoke.sol --tr-level=2
```

结果：

`return_uint_mulmod_arg_expr_01` 当前输出：

```solidity
contract Decompiled {
    function mulmodargexpr(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        return mulmod(arg0 * arg1, arg2, arg3);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.00 秒。

性能：

EVM smoke 用时 `elapsed=30.19 user=34.20 sys=0.79 maxrss=985084`。

## 2026-07-01：return uint add rhs addmod

问题：

`return_uint_add_rhs_addmod_01` 的源码语义是 `return base + addmod(left, right, modulus);`。`addmod` 基础返回和参数表达式已经支持，这里检查三参数内建函数作为二元表达式右操作数时能否继续打印成函数调用。

改动：

- [test/evm/solidity-source/cases/return_uint_add_rhs_addmod_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_add_rhs_addmod_01.sol:1) 新增源码证据，源码第 6 行包含 `return base + addmod(left, right, modulus);`。
- [test/evm/solidity-source/ir/return_uint_add_rhs_addmod_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_add_rhs_addmod_01.ll:9) 新增冻结 IR，`public_addrhsaddmod_uint256_uint256_uint256_uint256__0x2a()` 先调用 `evm_addmod`，再执行 `add`。
- [test/evm/solidity-source/expected/return_uint_add_rhs_addmod_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_add_rhs_addmod_01.sol:1) 固化输出，函数体包含 `return arg0 + addmod(arg1, arg2, arg3);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:200) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_add_rhs_addmod_01.ll -o /tmp/return_uint_add_rhs_addmod_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_add_rhs_addmod_01.ll -o /tmp/return_uint_add_rhs_addmod_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-add-rhs-addmod-smoke.sol --tr-level=2
```

结果：

`return_uint_add_rhs_addmod_01` 当前输出：

```solidity
contract Decompiled {
    function addrhsaddmod(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        return arg0 + addmod(arg1, arg2, arg3);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.06 秒。

性能：

EVM smoke 用时 `elapsed=30.44 user=34.76 sys=0.70 maxrss=984364`。

## 2026-07-01：return uint add rhs mulmod

问题：

`return_uint_add_rhs_mulmod_01` 的源码语义是 `return base + mulmod(left, right, modulus);`。`mulmod` 基础返回和参数表达式已经支持，这里检查三参数内建函数作为二元表达式右操作数时能否继续打印成函数调用。

改动：

- [test/evm/solidity-source/cases/return_uint_add_rhs_mulmod_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_add_rhs_mulmod_01.sol:1) 新增源码证据，源码第 6 行包含 `return base + mulmod(left, right, modulus);`。
- [test/evm/solidity-source/ir/return_uint_add_rhs_mulmod_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_add_rhs_mulmod_01.ll:9) 新增冻结 IR，`public_addrhsmulmod_uint256_uint256_uint256_uint256__0x2a()` 先调用 `evm_mulmod`，再执行 `add`。
- [test/evm/solidity-source/expected/return_uint_add_rhs_mulmod_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_add_rhs_mulmod_01.sol:1) 固化输出，函数体包含 `return arg0 + mulmod(arg1, arg2, arg3);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:206) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_add_rhs_mulmod_01.ll -o /tmp/return_uint_add_rhs_mulmod_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_add_rhs_mulmod_01.ll -o /tmp/return_uint_add_rhs_mulmod_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-add-rhs-mulmod-smoke.sol --tr-level=2
```

结果：

`return_uint_add_rhs_mulmod_01` 当前输出：

```solidity
contract Decompiled {
    function addrhsmulmod(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        return arg0 + mulmod(arg1, arg2, arg3);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.24 秒。

性能：

EVM smoke 用时 `elapsed=30.29 user=34.41 sys=0.71 maxrss=983664`。

## 2026-07-01：return uint add lhs addmod

问题：

`return_uint_add_lhs_addmod_01` 的源码语义是 `return addmod(left, right, modulus) + base;`。前面已经固化了 `addmod` 作为加法右操作数，这里补左操作数方向，确认函数调用表达式不会退回临时变量。

改动：

- [test/evm/solidity-source/cases/return_uint_add_lhs_addmod_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_add_lhs_addmod_01.sol:1) 新增源码证据，源码第 6 行包含 `return addmod(left, right, modulus) + base;`。
- [test/evm/solidity-source/ir/return_uint_add_lhs_addmod_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_add_lhs_addmod_01.ll:9) 新增冻结 IR，`public_addlhsaddmod_uint256_uint256_uint256_uint256__0x2a()` 先调用 `evm_addmod`，再执行 `add`。
- [test/evm/solidity-source/expected/return_uint_add_lhs_addmod_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_add_lhs_addmod_01.sol:1) 固化输出，函数体包含 `return addmod(arg0, arg1, arg2) + arg3;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:212) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_add_lhs_addmod_01.ll -o /tmp/return_uint_add_lhs_addmod_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_add_lhs_addmod_01.ll -o /tmp/return_uint_add_lhs_addmod_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-add-lhs-addmod-smoke.sol --tr-level=2
```

结果：

`return_uint_add_lhs_addmod_01` 当前输出：

```solidity
contract Decompiled {
    function addlhsaddmod(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        return addmod(arg0, arg1, arg2) + arg3;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.37 秒。

性能：

EVM smoke 用时 `elapsed=31.07 user=35.20 sys=0.75 maxrss=982644`。

## 2026-07-01：return uint add lhs mulmod

问题：

`return_uint_add_lhs_mulmod_01` 的源码语义是 `return mulmod(left, right, modulus) + base;`。前面已经固化了 `mulmod` 作为加法右操作数，这里补左操作数方向，确认函数调用表达式不会退回临时变量。

改动：

- [test/evm/solidity-source/cases/return_uint_add_lhs_mulmod_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_add_lhs_mulmod_01.sol:1) 新增源码证据，源码第 6 行包含 `return mulmod(left, right, modulus) + base;`。
- [test/evm/solidity-source/ir/return_uint_add_lhs_mulmod_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_add_lhs_mulmod_01.ll:9) 新增冻结 IR，`public_addlhsmulmod_uint256_uint256_uint256_uint256__0x2a()` 先调用 `evm_mulmod`，再执行 `add`。
- [test/evm/solidity-source/expected/return_uint_add_lhs_mulmod_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_add_lhs_mulmod_01.sol:1) 固化输出，函数体包含 `return mulmod(arg0, arg1, arg2) + arg3;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:218) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_add_lhs_mulmod_01.ll -o /tmp/return_uint_add_lhs_mulmod_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_add_lhs_mulmod_01.ll -o /tmp/return_uint_add_lhs_mulmod_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-add-lhs-mulmod-smoke.sol --tr-level=2
```

结果：

`return_uint_add_lhs_mulmod_01` 当前输出：

```solidity
contract Decompiled {
    function addlhsmulmod(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        return mulmod(arg0, arg1, arg2) + arg3;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.47 秒。

性能：

EVM smoke 用时 `elapsed=30.47 user=34.58 sys=0.70 maxrss=982064`。

## 2026-07-01：return uint not addmod

问题：

`return_uint_not_addmod_01` 的源码语义是 `return ~addmod(left, right, modulus);`。这个例子补充 `addmod` 作为按位取反操作数时的打印，确认三参数内建调用在一元表达式里不会被错误拆成临时变量或多余括号。

改动：

- [test/evm/solidity-source/cases/return_uint_not_addmod_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_not_addmod_01.sol:1) 新增源码证据，源码第 5 行函数 `notaddmod()` 在第 6 行返回 `~addmod(left, right, modulus)`。
- [test/evm/solidity-source/ir/return_uint_not_addmod_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_not_addmod_01.ll:9) 新增冻结 IR，`public_notaddmod_uint256_uint256_uint256__0x2a()` 第 11 行调用 `evm_addmod`，第 12 行用 `xor -1` 表示 EVM `NOT`。
- [test/evm/solidity-source/expected/return_uint_not_addmod_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_not_addmod_01.sol:1) 固化输出，函数体第 4 行包含 `return ~addmod(arg0, arg1, arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:225) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_not_addmod_01.ll -o /tmp/return_uint_not_addmod_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_not_addmod_01.ll -o /tmp/return_uint_not_addmod_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-not-addmod-smoke.sol --tr-level=2
```

结果：

`return_uint_not_addmod_01` 当前输出：

```solidity
contract Decompiled {
    function notaddmod(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return ~addmod(arg0, arg1, arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.58 秒。

性能：

EVM smoke 用时 `elapsed=31.65 user=35.35 sys=0.87 maxrss=954776`。

## 2026-07-01：return uint not mulmod

问题：

`return_uint_not_mulmod_01` 的源码语义是 `return ~mulmod(left, right, modulus);`。前面已经固化了 `~addmod(...)`，这里补 `mulmod` 方向，确认三参数内建调用作为一元 `~` 操作数时同样能直接打印。

改动：

- [test/evm/solidity-source/cases/return_uint_not_mulmod_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_uint_not_mulmod_01.sol:1) 新增源码证据，源码第 5 行函数 `notmulmod()` 在第 6 行返回 `~mulmod(left, right, modulus)`。
- [test/evm/solidity-source/ir/return_uint_not_mulmod_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_uint_not_mulmod_01.ll:9) 新增冻结 IR，`public_notmulmod_uint256_uint256_uint256__0x2a()` 第 11 行调用 `evm_mulmod`，第 12 行用 `xor -1` 表示 EVM `NOT`。
- [test/evm/solidity-source/expected/return_uint_not_mulmod_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_uint_not_mulmod_01.sol:1) 固化输出，函数体第 4 行包含 `return ~mulmod(arg0, arg1, arg2);`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:231) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_uint_not_mulmod_01.ll -o /tmp/return_uint_not_mulmod_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_uint_not_mulmod_01.ll -o /tmp/return_uint_not_mulmod_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-not-mulmod-smoke.sol --tr-level=2
```

结果：

`return_uint_not_mulmod_01` 当前输出：

```solidity
contract Decompiled {
    function notmulmod(uint256 arg0, uint256 arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        return ~mulmod(arg0, arg1, arg2);
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.70 秒。

性能：

EVM smoke 用时 `elapsed=30.46 user=34.52 sys=0.77 maxrss=982028`。

## 2026-07-01：return bool eq uint

问题：

`return_bool_eq_uint_01` 的源码语义是 `return left == right;`。修复前当前输出是：

```solidity
contract Decompiled {
    function iseq(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return evm.bool;
    }
}
```

也就是 `icmp/zext i1` 没有打印成比较表达式，单 word ABI bool 返回也被 `Reader::readReturns()` 继续当成 `uint256`。

改动：

- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:329) 新增 `icmpPredicateText()` 和 `icmpPredicatePrecedence()`，把 LLVM `icmp` 谓词映射到 Solidity 比较运算符。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:453) 扩展 `formatReturnValue()`，先剥离 `zext i1`，再把 `ICmpInst` 打印成 `arg0 == arg1` 这类表达式。
- [external/NotDec-llvm2c/lib/Solidity/Reader.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/Reader.cpp:21) 新增 `constantIntValue()`、`constantIntToPtrValue()`、`isAbiBoolWord()`、`findStoredReturnValue()`、`returnsSingleBoolWord()`，只在单 word 返回值来自 `i1` 或 `zext i1` 时识别为 `bool`。
- [external/NotDec-llvm2c/lib/Solidity/Reader.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/Reader.cpp:227) 更新 `Reader::readReturns()`，单 word bool 返回打印为 `returns (bool ret0)`，其他静态返回仍保持 `uint256`。
- [test/evm/solidity-source/cases/return_bool_eq_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_eq_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `iseq()` 在第 6 行返回 `left == right`。
- [test/evm/solidity-source/ir/return_bool_eq_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_eq_uint_01.ll:8) 新增冻结 IR，`public_iseq_uint256_uint256__0x2a()` 第 10 行生成 `icmp eq`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_eq_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_eq_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 == arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:237) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_eq_uint_01.ll -o /tmp/return_bool_eq_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_eq_uint_01.ll -o /tmp/return_bool_eq_uint_01.after.sol --tr-level=2
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-eq-smoke.sol --tr-level=2
```

结果：

`return_bool_eq_uint_01` 当前输出：

```solidity
contract Decompiled {
    function iseq(uint256 arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return arg0 == arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.13 秒。

`notdec.evm.solidity_rewrite` 通过，用时 102.13 秒。

性能：

EVM smoke 用时 `elapsed=31.45 user=35.36 sys=0.79 maxrss=951060`。

评估：

实现效果：8/10，补上了最小 bool 比较返回链路。复杂度：3/10，只增加局部打印和局部返回类型识别。维护成本：3/10，后续要继续用小用例补 `!=`、`<`、组合比较和更完整的 ABI 返回类型。

## 2026-07-01：return bool lt uint

问题：

`return_bool_lt_uint_01` 的源码语义是 `return left < right;`。前面已经修复并固化了 `==`，这里补 unsigned `<`，确认 `icmp ult` 经过 `zext i1` 后能打印成 bool 返回表达式。

改动：

- [test/evm/solidity-source/cases/return_bool_lt_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_lt_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `islt()` 在第 6 行返回 `left < right`。
- [test/evm/solidity-source/ir/return_bool_lt_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_lt_uint_01.ll:8) 新增冻结 IR，`public_islt_uint256_uint256__0x2a()` 第 10 行生成 `icmp ult`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_lt_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_lt_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 < arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:243) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_lt_uint_01.ll -o /tmp/return_bool_lt_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_lt_uint_01.ll -o /tmp/return_bool_lt_uint_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-lt-smoke.sol --tr-level=2
```

结果：

`return_bool_lt_uint_01` 当前输出：

```solidity
contract Decompiled {
    function islt(uint256 arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return arg0 < arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 4.93 秒。

性能：

EVM smoke 用时 `elapsed=30.20 user=34.39 sys=0.63 maxrss=982748`。

## 2026-07-01：return bool ne uint

问题：

`return_bool_ne_uint_01` 的源码语义是 `return left != right;`。前面已经固化了 `==` 和 `<`，这里补 `icmp ne`，确认 bool 返回链路能打印不等比较。

改动：

- [test/evm/solidity-source/cases/return_bool_ne_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_ne_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `isne()` 在第 6 行返回 `left != right`。
- [test/evm/solidity-source/ir/return_bool_ne_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_ne_uint_01.ll:8) 新增冻结 IR，`public_isne_uint256_uint256__0x2a()` 第 10 行生成 `icmp ne`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_ne_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_ne_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 != arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:249) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_ne_uint_01.ll -o /tmp/return_bool_ne_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_ne_uint_01.ll -o /tmp/return_bool_ne_uint_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-ne-smoke.sol --tr-level=2
```

结果：

`return_bool_ne_uint_01` 当前输出：

```solidity
contract Decompiled {
    function isne(uint256 arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return arg0 != arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.07 秒。

性能：

EVM smoke 用时 `elapsed=30.39 user=34.51 sys=0.69 maxrss=983392`。

## 2026-07-01：return bool slt int

问题：

`return_bool_slt_int_01` 的源码语义是 `return left < right;`，参数类型是 `int256`。前面已经固化了 unsigned `<`，这里补 signed `<`，确认 `icmp slt` 和 ABI `int256` 参数能一起打印成 Solidity signed 比较。

改动：

- [test/evm/solidity-source/cases/return_bool_slt_int_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_slt_int_01.sol:1) 新增源码证据，源码第 5 行函数 `isslt()` 使用 `int256` 参数，第 6 行返回 `left < right`。
- [test/evm/solidity-source/ir/return_bool_slt_int_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_slt_int_01.ll:8) 新增冻结 IR，`public_isslt_int256_int256__0x2a()` 第 10 行生成 `icmp slt`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_slt_int_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_slt_int_01.sol:1) 固化输出，函数参数类型为 `int256`，返回类型为 `bool`，第 4 行是 `return arg0 < arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:255) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_slt_int_01.ll -o /tmp/return_bool_slt_int_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_slt_int_01.ll -o /tmp/return_bool_slt_int_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-slt-smoke.sol --tr-level=2
```

结果：

`return_bool_slt_int_01` 当前输出：

```solidity
contract Decompiled {
    function isslt(int256 arg0, int256 arg1) public returns (bool ret0) {
        // block_0:
        return arg0 < arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.17 秒。

性能：

EVM smoke 用时 `elapsed=30.56 user=34.71 sys=0.72 maxrss=983560`。

## 2026-07-01：return bool gt uint

问题：

`return_bool_gt_uint_01` 的源码语义是 `return left > right;`。前面已经固化了 unsigned `<` 和 signed `<`，这里补 unsigned `>`，确认 `icmp ugt` 经过 `zext i1` 后能打印成 bool 返回表达式。

改动：

- [test/evm/solidity-source/cases/return_bool_gt_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_gt_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `isgt()` 在第 6 行返回 `left > right`。
- [test/evm/solidity-source/ir/return_bool_gt_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_gt_uint_01.ll:8) 新增冻结 IR，`public_isgt_uint256_uint256__0x2a()` 第 10 行生成 `icmp ugt`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_gt_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_gt_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 > arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:261) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_gt_uint_01.ll -o /tmp/return_bool_gt_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_gt_uint_01.ll -o /tmp/return_bool_gt_uint_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-gt-smoke.sol --tr-level=2
```

结果：

`return_bool_gt_uint_01` 当前输出：

```solidity
contract Decompiled {
    function isgt(uint256 arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return arg0 > arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.27 秒。

性能：

EVM smoke 用时 `elapsed=30.70 user=34.77 sys=0.73 maxrss=982284`。

## 2026-07-01：return bool le uint

问题：

`return_bool_le_uint_01` 的源码语义是 `return left <= right;`。前面已经固化了 unsigned `<` 和 `>`，这里补 unsigned `<=`，确认 `icmp ule` 经过 `zext i1` 后能打印成 bool 返回表达式。

改动：

- [test/evm/solidity-source/cases/return_bool_le_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_le_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `isle()` 在第 6 行返回 `left <= right`。
- [test/evm/solidity-source/ir/return_bool_le_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_le_uint_01.ll:8) 新增冻结 IR，`public_isle_uint256_uint256__0x2a()` 第 10 行生成 `icmp ule`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_le_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_le_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 <= arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:267) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_le_uint_01.ll -o /tmp/return_bool_le_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_le_uint_01.ll -o /tmp/return_bool_le_uint_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-le-smoke.sol --tr-level=2
```

结果：

`return_bool_le_uint_01` 当前输出：

```solidity
contract Decompiled {
    function isle(uint256 arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return arg0 <= arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.39 秒。

性能：

EVM smoke 用时 `elapsed=30.51 user=34.58 sys=0.72 maxrss=980968`。

## 2026-07-01：return bool ge uint

问题：

`return_bool_ge_uint_01` 的源码语义是 `return left >= right;`。前面已经固化了 unsigned `<`、`>` 和 `<=`，这里补 unsigned `>=`，确认 `icmp uge` 经过 `zext i1` 后能打印成 bool 返回表达式。

改动：

- [test/evm/solidity-source/cases/return_bool_ge_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_ge_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `isge()` 在第 6 行返回 `left >= right`。
- [test/evm/solidity-source/ir/return_bool_ge_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_ge_uint_01.ll:8) 新增冻结 IR，`public_isge_uint256_uint256__0x2a()` 第 10 行生成 `icmp uge`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_ge_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_ge_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 >= arg1;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:273) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_ge_uint_01.ll -o /tmp/return_bool_ge_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_ge_uint_01.ll -o /tmp/return_bool_ge_uint_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-ge-smoke.sol --tr-level=2
```

结果：

`return_bool_ge_uint_01` 当前输出：

```solidity
contract Decompiled {
    function isge(uint256 arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return arg0 >= arg1;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.48 秒。

性能：

EVM smoke 用时 `elapsed=30.23 user=34.39 sys=0.68 maxrss=985384`。

## 2026-07-01：return bool eq zero uint

问题：

`return_bool_eq_zero_uint_01` 的源码语义是 `return value == 0;`。这个例子对应常见 EVM `ISZERO` 形状，确认 `icmp eq value, 0` 经过 `zext i1` 后能打印成 bool 返回表达式。

改动：

- [test/evm/solidity-source/cases/return_bool_eq_zero_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_eq_zero_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `iszero()` 在第 6 行返回 `value == 0`。
- [test/evm/solidity-source/ir/return_bool_eq_zero_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_eq_zero_uint_01.ll:8) 新增冻结 IR，`public_iszero_uint256__0x2a()` 第 10 行生成 `icmp eq i256 %arg0, 0`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_eq_zero_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_eq_zero_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 == 0;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:279) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_eq_zero_uint_01.ll -o /tmp/return_bool_eq_zero_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_eq_zero_uint_01.ll -o /tmp/return_bool_eq_zero_uint_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-eq-zero-smoke.sol --tr-level=2
```

结果：

`return_bool_eq_zero_uint_01` 当前输出：

```solidity
contract Decompiled {
    function iszero(uint256 arg0) public returns (bool ret0) {
        // block_0:
        return arg0 == 0;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.64 秒。

性能：

EVM smoke 用时 `elapsed=30.17 user=34.27 sys=0.72 maxrss=982676`。

## 2026-07-01：return bool ne zero uint

问题：

`return_bool_ne_zero_uint_01` 的源码语义是 `return value != 0;`。前面已经固化了 `value == 0` 的 `ISZERO` 形状，这里补非零判断，确认 `icmp ne value, 0` 经过 `zext i1` 后能打印成 bool 返回表达式。

改动：

- [test/evm/solidity-source/cases/return_bool_ne_zero_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/return_bool_ne_zero_uint_01.sol:1) 新增源码证据，源码第 5 行函数 `isnonzero()` 在第 6 行返回 `value != 0`。
- [test/evm/solidity-source/ir/return_bool_ne_zero_uint_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/return_bool_ne_zero_uint_01.ll:8) 新增冻结 IR，`public_isnonzero_uint256__0x2a()` 第 10 行生成 `icmp ne i256 %arg0, 0`，第 11 行 `zext i1` 成 ABI word。
- [test/evm/solidity-source/expected/return_bool_ne_zero_uint_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/return_bool_ne_zero_uint_01.sol:1) 固化输出，函数返回类型为 `bool`，第 4 行是 `return arg0 != 0;`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:285) 把新 case 接入 `notdec.evm.solidity_source`。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/return_bool_ne_zero_uint_01.ll -o /tmp/return_bool_ne_zero_uint_01.bc
./build/bin/notdec test/evm/solidity-source/ir/return_bool_ne_zero_uint_01.ll -o /tmp/return_bool_ne_zero_uint_01.after.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-bool-ne-zero-smoke.sol --tr-level=2
```

结果：

`return_bool_ne_zero_uint_01` 当前输出：

```solidity
contract Decompiled {
    function isnonzero(uint256 arg0) public returns (bool ret0) {
        // block_0:
        return arg0 != 0;
    }
}
```

`notdec.evm.solidity_source` 通过，用时 5.91 秒。

性能：

EVM smoke 用时 `elapsed=30.71 user=34.86 sys=0.75 maxrss=983916`。
