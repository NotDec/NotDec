# Selector entry outlining plan

## 原始 prompt

按照这个方式推进吧

## 背景

当前 `SelectorInlinedLogicExtractionPass` 只标 selector 函数、free memory pointer prologue、明显 `call/log` 候选。rewrite 层也只是统一插入 marker 和 hidden metadata，还没有把 selector 入口里的 dispatcher 和内联业务代码分开。

这次直接做 rewrite / outline，不单独做 metadata-only 阶段。实现里仍然先恢复 selector region，再对能证明安全的区域做 outline。

## 目标

- 在 selector 函数里识别 dispatcher、selector call stub、fallback / receive / reject、内联业务区域。
- 对边界安全的内联业务区域生成单独函数。
- selector 原位置改成调用新函数再返回。
- 现有 Solidity pattern suite 不退化。
- 新增 rewrite suite，检查 outline 后 IR 结构。

## 路线

新增 `SelectorEntryOutliningPass`，放在 EVM local canonicalization 之后、其他 Solidity pattern pass 之前。

识别规则先保守：

- selector 函数名仍用 `public___function_selector___*`。
- dispatcher 支持 `calldatasize < 4`、`calldataload(0)`、`evm_shr(224, load)`、selector 常量比较链。
- call stub 是只调用 `public_*` 后 `ret void` 的块，不 outline。
- `evm_revert(0,0)` + `unreachable` 是 reject，不 outline。
- 其他从 dispatcher 落入、单入口、无回跳 dispatcher、无复杂 live-out 的区域可 outline。

实现使用本地 region clone / remap。之前试过 `CodeExtractor`，但 selector 里的多 return / 多出口规整后结构不符合目标，所以这里不用它直接改 CFG。如果 region 不满足约束，就只标 metadata / skipped 原因，不强拆。

## 测试

新增 `test/evm/solidity-rewrite/`：

- 复用当前代表性 case。
- runner 先检查 `llvm-as`。
- 再检查 outline 函数数量、selector 里 outline call 数量、必要 metadata 数量。

现有 `test/evm/solidity-patterns/` 继续检查识别数量，避免 rewrite 破坏已有 pass。

## 风险

- 最大风险是把业务逻辑误当 dispatcher 或把复杂 fallback region 拆错。
- 第一版必须保守：宁可跳过复杂 region，也不要拆错。
- outline 会移动指令，现有 metadata 数量可能变化；rewrite suite 要看结构，pattern suite 要确认已有统计不退化。

## 判断标准

- `notdec.evm.solidity_patterns` 通过。
- `notdec.evm.solidity_rewrite` 通过。
- 输出 IR 能用项目 LLVM 22 `llvm-as` 汇编。
- 日志记录 outline 成功数、跳过原因和性能影响。

## 实现记录

2026-05-24：已完成第一版保守 outline。

- `include/notdec/Passes/evm/SolidityPatterns.h:46` 新增 `SelectorEntryOutliningPass`。
- `src/Passes/PassManager.cpp:285` 把新 pass 放到 EVM local canonicalization 之后、其他 Solidity pattern pass 之前。
- `src/Passes/evm/SolidityPatterns.cpp:42` 新增 `NumSelectorOutlinedBodies` 和 `NumSelectorOutlineSkipped` 统计。
- `src/Passes/evm/SolidityPatterns.cpp:340` 到 `src/Passes/evm/SolidityPatterns.cpp:507` 新增 selector dispatcher、public call stub、reject block、region live-out / live-in 检查。
- `src/Passes/evm/SolidityPatterns.cpp:509` 新增 `cloneSelectorRegion`，把安全 region clone 到 `notdec_solidity_selector_inline.*` internal 函数。
- `src/Passes/evm/SolidityPatterns.cpp:555` 新增 `replaceRegionWithCall`，把 selector 里的原 region 替换成 outlined 函数调用后 `ret void`。
- `src/Passes/evm/SolidityPatterns.cpp:818` 实现 `SelectorEntryOutliningPass::run`。当前每个 selector 函数最多 outline 一个 region，避免同一轮里继续处理已删除 block。
- `test/run_evm_solidity_rewrite_suite.py:1` 新增 rewrite 专用 runner。
- `test/evm/solidity-rewrite/manifest.json:1` 新增两个代表 case：proxy fallback 需要 outline，纯 dispatcher 保持不 outline。
- `test/CMakeLists.txt:54` 注册 `notdec.evm.solidity_rewrite`。

当前通用性判断：

- 这版不是针对某个 block 名写规则。入口来自 selector dispatcher 的 CFG 分类，outline 前检查单入口、无 SSA live-out、无 unsupported live-in。
- 对多入口、复杂 live-in / live-out 的 region 只打 `notdec.solidity.selector_outline_skipped`，不强拆。
- `0014_proxy_like` 能把 proxy fallback 内联区域拆成独立函数；`0011_multi_public` 这类纯 public call stub dispatcher 不拆。

复杂度和维护成本：

- 实现效果：6/10。已经能拆最需要的 proxy fallback inline body，但每个 selector 目前只拆一个 region。
- 复杂度：5/10。使用 CFG 分类和 clone/remap，比只打 marker 复杂，但逻辑还集中在 selector-entry。
- 维护成本：5/10。后续需要逐步放宽 live-in / 多 region 支持，rewrite suite 能防止误拆。

验证：

- `cmake --build ./build --target all -j4`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`：2/2 passed，0.44s。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：58/58 passed，84.61s。

性能：

- 上一轮 rewrite marker 后 `notdec.evm.solidity_patterns` 约 84.58s。
- 本轮加入 selector outline 后为 84.61s，同口径基本无变化。

2026-05-24：继续完善 selector outline。

- `src/Passes/evm/SolidityPatterns.cpp:377` 收紧 `isSelectorSizeGate`，只把条件表达式依赖 `evm_calldatasize` 的块当 dispatcher，避免普通 body 里的 calldata size 使用被误归类。
- `src/Passes/evm/SolidityPatterns.cpp:420` 放宽 `isPublicCallStub`，支持 `public_*` call 后跳到 `common.ret` 的形状，不再把这类 dispatcher leaf 记成 skipped region。
- `src/Passes/evm/SolidityPatterns.cpp:473` 新增 `collectRegionInputs`，只做本地 region input 收集，不是通用 live-in analysis，也没有复用 LLVM 的 live-in 分析。
- `src/Passes/evm/SolidityPatterns.cpp:517` 新增 `regionHasOutsideSuccessor`，避免 clone 出来的 region 还跳回原函数里的外部块，修复后 58 个 case 不再触发跨函数 dominator assertion。
- `src/Passes/evm/SolidityPatterns.cpp:554` 和 `src/Passes/evm/SolidityPatterns.cpp:610` 支持把 region 外定义的 SSA input 作为 outlined helper 的额外参数传入。
- `test/evm/solidity-rewrite/manifest.json:17` 增加 `0002_delegatecall_no_nonpayable`，覆盖 delegatecall proxy 的 revert-bubble inline region。
- `test/evm/solidity-rewrite/manifest.json:37` 增加 `0448_19495059_065877b669_4f138305be23`，覆盖 binary-search dispatcher 中非闭合 region 的保守 skip。

全量扫描：

- 58 个 `test/evm/solidity-patterns` case 全部能生成 IR，并且全部通过项目 LLVM 22 `llvm-as`。
- 当前只有 `0014_proxy_like` 和 `0002_delegatecall_no_nonpayable` outline 出 selector inline helper，正好对应当前 oracle 里有 `selector_inlined_body` 的两个 case。
- 43 个 case 仍有 `selector_outline_skipped`，主要是 binary-search dispatcher 或非闭合 region；这些先保守跳过。

验证：

- `cmake --build ./build --target all -j4`：通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(rewrite|patterns)' --output-on-failure`：通过。
  - `notdec.evm.solidity_patterns`：58/58 passed，85.97s。
  - `notdec.evm.solidity_rewrite`：4/4 passed，1.63s。

性能：

- 上一轮 `notdec.evm.solidity_patterns` 为 84.61s。
- 本轮为 85.97s，增加约 1.6%。主要来自额外 region input / successor 检查和更多 skipped metadata。

2026-05-24：把 rewrite 回归扩到当前 58 个 case，并清理 skipped 噪声。

- `test/run_evm_solidity_rewrite_suite.py:37` 新增 skip reason 统计，runner 现在同时检查 outline 函数数、outline call 数、outline metadata、`selector_inlined_body` 和 `selector_outline_skipped` 原因。
- `test/evm/solidity-rewrite/manifest.json:1` 改为覆盖 `test/evm/solidity-patterns` 当前全部 58 个 case。
- `src/Passes/evm/SolidityPatterns.cpp:353` 新增 `dependsOnSelectorLoad`，`isSelectorCompareBranch` 只把依赖 `calldataload(0)` 的常量比较当 selector dispatcher，避免把 body 里的普通 `returndatasize == 0` 误归类为 dispatcher。
- `src/Passes/evm/SolidityPatterns.cpp:468` 新增 `isVoidReturnBlock`，`src/Passes/evm/SolidityPatterns.cpp:570` 新增 `mapVoidReturnExits`，outline 时把共享 `common.ret` 映射成新 helper 内自己的 return block，不删除 selector 里被 public call stub 复用的 return block。
- `src/Passes/evm/SolidityPatterns.cpp:585` 新增 body signal 过滤，只有 region 内有 `evm_call` / `evm_delegatecall` / `evm_log*` / `evm_return` / `evm_revert` 这类真实 body 信号时才尝试 outline 或记录 skipped。
- `src/Passes/evm/SolidityPatterns.cpp:545` 重新把 `revert(0,0)` empty reject 作为 region 边界，避免 binary-search dispatcher 的 reject 分支制造 skipped 噪声。
- `test/evm/solidity-patterns/manifest.json:27` 更新 `0014_proxy_like` 的 metadata oracle：outline 后 selector 函数里的 inline body 已拆走，`abi_decode` 也减少到当前实际输出。
- `test/evm/solidity-patterns/manifest.json:99` 更新 `0002_delegatecall_no_nonpayable` 的 `abi_return` oracle：outline helper 里新增一个 return site 标注。

当前 58 个 rewrite oracle 结论：

- 只有 `0014_proxy_like` 和 `0002_delegatecall_no_nonpayable` 生成 `notdec_solidity_selector_inline.*` helper。
- 其他 56 个 case 不 outline，也不再产生 `selector_outline_skipped` 噪声。
- `selector_outline_skipped` 当前为 0。后续如果出现 skipped，说明是真正有 body signal 但边界还不能安全拆的 case。

验证：

- `cmake --build ./build --target all -j4`：通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(rewrite|patterns)' --output-on-failure`：通过。
  - `notdec.evm.solidity_patterns`：58/58 passed，85.97s。
  - `notdec.evm.solidity_rewrite`：58/58 passed，85.57s。

性能：

- rewrite suite 现在跑 58 个 case，所以耗时从小样例 suite 的 1.63s 增加到 85.57s，这是测试覆盖扩大导致的。
- patterns suite 本轮 85.97s，和上一轮 85.97s 同口径持平。
