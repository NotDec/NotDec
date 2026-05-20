# Evm2llvm Poison Warning

## 这次做了什么

检查 `external/NotDec-evm2llvm` 里显式生成 poison / undef 的位置。当前只发现一个：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:410`
  - 多返回值 `RETURNPRIVATE` 用 `llvm::PoisonValue::get(returnType)` 作为 struct aggregate 初值。
  - 后续用 `insertvalue` 把每个返回字段写进去。

这条路径不是 0394 会走的路径，但它会在多返回 private function fixture 里生成：

```llvm
insertvalue { i256, i256 } poison, ...
```

## 修改

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:18`
  - 引入 `llvm/Support/raw_ostream.h`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:409-415`
  - 生成 poison seed 前打印 warning。
  - warning 带 function id、block id、statement id，提示检查返回 aggregate 字段是否都被定义。
- `external/NotDec-evm2llvm/test/CMakeLists.txt:43-46`
  - 新增 `private_call_multi_return.check-poison-warning`，确认生成 poison 的路径会同时打印 warning。

## 验证

- `cmake --build build-evm2llvm --target evm2llvm -j4`
- `ctest --test-dir build-evm2llvm -R 'evm2llvm.fixture' --output-on-failure`
- 单独确认 warning：
  - `./build-evm2llvm/bin/evm2llvm --facts external/NotDec-evm2llvm/test/fixtures/private_call_multi_return --output /tmp/private_call_multi_return.poison.ll`
  - 输出 warning：`warning: evm2llvm emits poison seed for multi-value RETURNPRIVATE ...`
- 0394 不触发 warning，并且生成结果通过 LLVM 22 `llvm-as` 和 `opt -passes=verify`。

## 性能

只在多返回 `RETURNPRIVATE` 路径多打一条 stderr warning，不涉及类型恢复、结构体合并、pointer analysis 或 pass pipeline，所以没有跑 fortune。
