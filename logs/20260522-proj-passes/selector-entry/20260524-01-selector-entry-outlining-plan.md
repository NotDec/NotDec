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

实现优先使用 LLVM `CodeExtractor`。如果 region 不满足约束，就只标 metadata / skipped 原因，不强拆。

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
