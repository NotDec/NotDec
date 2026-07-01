# EVM Solidity Source 回归测试目录

## 原始需求

把整个测试流程都最好固化下来，在当前项目找个地方创建一个文件夹专门负责放已经验证过的代码和测试。先不要把代码全部挪过来，而是每次修复完一个小的solidity源码，反编译结果够好了再放进来，固化为可以自动运行的测试。看看怎么做比较合适

按这个推进吧，搭建框架，然后找个最简单例子试试

## 实现

- `test/CMakeLists.txt:71` 新增 `add_evm_solidity_source_suite`，`test/CMakeLists.txt:103` 注册 `notdec.evm.solidity_source`。
- `test/README.md:28` 和 `test/README.md:51` 登记新的 `test/evm/solidity-source/` 布局和 suite。
- `test/run_evm_solidity_source_suite.py:15` 新增 manifest 驱动 runner。它运行 `notdec input.ll -o out.sol`，对输出和 `expected/*.sol` 做去行尾空格后的全文 diff，并把命令、输出、diff 写入 case 日志。
- `test/evm/solidity-source/README.md:1` 说明新增目录只收已经人工确认过的小 Solidity case，默认测试只跑冻结 IR，不自动调用 `solc` 或 Gigahorse。
- `test/evm/solidity-source/manifest.json:1` 新增首个 suite manifest。
- `test/evm/solidity-source/cases/empty_runtime.sol:1`、`bytecode/empty_runtime.hex:1`、`ir/empty_runtime.ll:1`、`expected/empty_runtime.sol:1` 新增首个最小 smoke。源码用 constructor 返回单字节 runtime `0x00`，冻结 IR 是一个 `STOP` 入口，期望输出是空 `contract Decompiled`。

## 验证

```bash
python3 test/run_evm_solidity_source_suite.py --binary build/bin/notdec --manifest test/evm/solidity-source/manifest.json --project-root . --workdir /tmp/notdec-solidity-source-suite
python3 -m py_compile test/run_evm_solidity_source_suite.py
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/empty_runtime.ll -o /tmp/notdec-empty-runtime.bc
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -S . -B ./build -G Ninja
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R 'notdec.evm.solidity_(source|rewrite)' --output-on-failure
```

结果：全部通过。组合跑 `solidity_rewrite` 和新 suite 用时约 99.75 秒，其中新 suite 约 0.14 秒。

## 性能和维护判断

这次只新增测试 runner 和一个极小冻结 IR case，不改 pass pipeline、类型恢复或 backend 逻辑。默认 CTest 多一个 0.14 秒左右的测试，对现有链路性能没有影响。后续每修好一个小 Solidity 源码，再把源码、可选 bytecode、冻结 IR 和 expected 输出作为一条 case 加进来。

## 实现记录：新增 revert string case

按小用例继续补了一条更有语义的 Solidity source 回归：

- [test/evm/solidity-source/cases/revert_error_string_01.sol](/sn640/NotDec/test/evm/solidity-source/cases/revert_error_string_01.sol:1) 新增源码证据，核心语句是 `require(false, "short");`。
- [test/evm/solidity-source/ir/revert_error_string_01.ll](/sn640/NotDec/test/evm/solidity-source/ir/revert_error_string_01.ll:1) 复用已验证的冻结 EVM LLVM IR。
- [test/evm/solidity-source/expected/revert_error_string_01.sol](/sn640/NotDec/test/evm/solidity-source/expected/revert_error_string_01.sol:1) 固化当前后端输出。当前还保留 fallback 和 body TODO，但 `run()` 已能打印 `require(false, "short");`。
- [test/evm/solidity-source/manifest.json](/sn640/NotDec/test/evm/solidity-source/manifest.json:14) 把 `revert_error_string_01` 接入自动 suite。

验证：

```bash
./llvm-22.1.0.obj/bin/llvm-as test/evm/solidity-source/ir/revert_error_string_01.ll -o /tmp/revert_error_string_01.bc
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
```

结果：全部通过，2 个 source case 总用时 0.44 秒。
