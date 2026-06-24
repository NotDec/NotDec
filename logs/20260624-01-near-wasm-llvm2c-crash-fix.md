# NEAR wasm llvm2c crash fixes

## 背景

`test-near/contract.wasm` 反编译到 C 时先后暴露两个 llvm2c 崩溃：

- `demoteSSAFixHT` 中，未命名 PHI 经 `demoteSSA` 变成 `%.reg2mem*`，和原来的空名字映射不上，Release 下 `map::at` 直接崩。
- `CFGBuilder::visitCallInst` 中，`llvm.ucmp.*` 这类未专门支持的 intrinsic 没有生成 `FunctionDecl`，Release 下后续直接解引用空声明崩。

## 修改

子模块 `external/NotDec-llvm2c`：

- `include/notdec-llvm2c/StructuralAnalysis.h:540` 新增 `SAContext::declareFunctionFromIR` 声明。
- `include/notdec-llvm2c/StructuralAnalysis.h:564` 让 `getFunctionDecl` 对缺失声明返回 `nullptr`，不再直接 `.at`。
- `lib/notdec-llvm2c/StructuralAnalysis.cpp:889` 在 `CFGBuilder::visitCallInst` 增加默认表达式 fallback。
- `lib/notdec-llvm2c/StructuralAnalysis.cpp:910` direct call 缺少 `FunctionDecl` 时按 IR 函数签名补声明。
- `lib/notdec-llvm2c/StructuralAnalysis.cpp:958` 间接调用 callee 或参数恢复失败时输出 warning 并保守 fallback。
- `lib/notdec-llvm2c/StructuralAnalysis.cpp:1513` 在 `demoteSSAFixHT` 中给未命名 PHI 先补稳定名字。
- `lib/notdec-llvm2c/StructuralAnalysis.cpp:1598` 只处理 `.reg2mem` alloca，并用 `find` 跳过无法匹配的名字。
- `lib/notdec-llvm2c/StructuralAnalysis.cpp:1778` 新增 `getFallbackQualType`。
- `lib/notdec-llvm2c/StructuralAnalysis.cpp:1801` 实现 `SAContext::declareFunctionFromIR`，用于未覆盖 intrinsic 的普通声明生成。

## 验证

- `cmake --build ./build-release --target notdec -j4`
- frozen MLsub 输入：
  `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1 ./build-release/bin/notdec test-near/contract.wasm.notdec/02-mlsub-input.ll -o test-near/frozen-after-decl-fix.c --tr-level=2 --frozen-tr-input-ir --gen-work-dir`
  结果：退出 0，生成 3.2M C，`elapsed=386.45 user=399.62 sys=5.79 maxrss=713272`。
- 完整 wasm 输入：
  `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1 ./build-release/bin/notdec test-near/contract.wasm -o test-near/contract.release-after-llvm2c-fix.c --tr-level=2 --gen-work-dir`
  结果：退出 0，生成 3.2M C，`elapsed=407.23 user=420.26 sys=6.17 maxrss=732344`。

## 影响

实现效果：8/10。当前用例能完整出 C，并保留 warning 提示 fallback 位置。

复杂度：4/10。新增逻辑集中在 llvm2c call lowering 和 SSA demote 类型映射，没有改 pass pipeline。

维护成本：4/10。fallback 声明能覆盖更多未知 intrinsic，但语义只按 IR 类型保守生成，不等于专门 intrinsic 语义恢复。
