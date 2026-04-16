# MLsub Signature Override Bootstrap

日期：2026-04-16

## 本次完成

本次把 `NOTDEC_SIGNATURE_OVERRIDE` 真正接到了 MLsub 主链路上，不再像之前那样在 `tr-level>=2` 时被静默忽略。

核心修改如下：

1. 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:340](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L340) 到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:385](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L385) 扩展 `notdec::mlsub::MLsubRecovery`
   - 新增 `SigFile`
   - 新增 `SignatureOverrideDoc`
   - 新增 `SignatureOverrideFuncs`
   - 新增 `OverrideTypeRecipe`
   - 新增 `OverrideBuildContext`
   - 新增函数声明：
     - `MLsubRecovery::loadSignatureFile()`
     - `MLsubRecovery::getSignatureOverrideSpec()`
     - `MLsubRecovery::buildOverrideType()`
     - `MLsubRecovery::applyOverrideRecipe()`
     - `MLsubRecovery::applySignatureOverride()`

2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:52](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L52) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:116](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L116) 新增 MLsub signature override JSON loader 的公共校验辅助函数
   - `failSignatureOverride()`
   - `requireObject()`
   - `requireArray()`
   - `requireString()`
   - `requireInteger()`
   - `appendJSONPath()`
   - `appendJSONIndexPath()`
   - `appendOverrideConstraints()`

3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:356](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L356) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:473](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L473) 修改 `MLsubRecovery::run()`
   - 在进入 call graph / SCC / bottom-up 之前读取 `SigFile`
   - 每次运行前清空 `SignatureOverrideFuncs`
   - 每次运行前重置 `SignatureOverrideDoc`

4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:475](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L475) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:747](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L747) 实现 MLsub-only signature override 解析与应用
   - `MLsubRecovery::loadSignatureFile()`
     - 只接受 `version=1`
     - 只接受顶层 `functions`
     - 预校验函数存在性、参数个数、`ret` 字段、`constraints` 类型
   - `MLsubRecovery::getSignatureOverrideSpec()`
   - `MLsubRecovery::buildOverrideType()`
     - 支持 `primitive`
     - 支持 `var`
     - 支持 `ref`
     - 支持 `ptr`
     - 支持 `record`
     - 支持 `null` 返回值
     - 对 semantic primitive canonical name 走 `binarysub::globalPrimitiveSemanticRegistry()` 校验
   - `MLsubRecovery::applyOverrideRecipe()`
   - `MLsubRecovery::applySignatureOverride()`
     - 先灌 recipe 附加约束
     - 再执行 `OverrideFuncTy <: F`

5. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:783](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L783) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:793](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L793) 修改 `MLsubRecovery::bottomUpPhase()`
   - 在 `G->run()` 之后
   - 在 `unhandledCalls` 多态实例化之前
   - 对当前 SCC 中命中的函数执行 `applySignatureOverride()`

## 验证

1. 构建验证
   - 运行：
   ```bash
   cmake --build ./build --target notdec-decompile -j2
   ```
   - 结果：通过，成功链接 `src/libnotdec.a` 和 `build/bin/notdec`

2. smoke 运行验证
   - 临时 override 文件：`/tmp/mlsub_sig_smoke.json`
   - 运行：
   ```bash
   env NOTDEC_SIGNATURE_OVERRIDE=/tmp/mlsub_sig_smoke.json \
     ./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
     -o /tmp/mlsub_sig_smoke.ll \
     --tr-level=2 \
     --dump-htypes /tmp/mlsub_sig_smoke.htypes \
     --gen-work-dir \
     --work-dir=/tmp/mlsub_sig_smoke_work
   ```
   - 结果：
     - stderr 中出现 `Loading MLsub signature override from: /tmp/mlsub_sig_smoke.json`
     - stderr 中出现 `Applying MLsub signature override to main`
     - 生成 `/tmp/mlsub_sig_smoke.htypes`
   - 输出证据：
     - `/tmp/mlsub_sig_smoke.htypes` 中 `@main` 显示为：
       - 第一个参数带 `ptr<load=i8, store=i8, psize=32>`
       - 第二个参数与返回值共享变量并受 `i32` 约束

## 关于旧非-MLsub 路径

当前主 pipeline 已经只走 MLsub：

- [src/Passes/PassManager.cpp:343](/sn640/NotDec/src/Passes/PassManager.cpp#L343) 到 [src/Passes/PassManager.cpp:364](/sn640/NotDec/src/Passes/PassManager.cpp#L364)
  - `tr-level>=2` 时挂的是 `mlsub::MLsubRecoveryMain`
  - 没有再挂旧 `TypeRecovery` / `GraphTypePass`

但旧 retypd / `ConstraintGenerator` 路径现在还不能直接删，因为它还在编译依赖链里：

- [src/CMakeLists.txt:8](/sn640/NotDec/src/CMakeLists.txt#L8)
  - 仍然编译 `Passes/ConstraintGenerator.cpp`
- [src/CMakeLists.txt:9](/sn640/NotDec/src/CMakeLists.txt#L9)
  - 仍然编译 `Passes/GraphTypePass.cpp`
- [src/CMakeLists.txt:20](/sn640/NotDec/src/CMakeLists.txt#L20) 到 [src/CMakeLists.txt:29](/sn640/NotDec/src/CMakeLists.txt#L29)
  - 仍然编译整批旧 `TypeRecovery/*` retypd 相关源文件

另外当前还有若干 MLsub 周边文件直接 `#include "Passes/ConstraintGenerator.h"`，例如：

- `include/notdec/TypeRecovery/mlsub/PNDiff.h`
- `src/Passes/MemOpMatcher.cpp`
- `src/TypeRecovery/PointerNumberIdentification.cpp`

所以后续如果要删旧路径，建议分两步：

1. 先把 MLsub 仍在复用的低层公共类型从 `ConstraintGenerator.h` 拆出来
2. 再从 `src/CMakeLists.txt` 删除旧 retypd / graph pass 编译项
