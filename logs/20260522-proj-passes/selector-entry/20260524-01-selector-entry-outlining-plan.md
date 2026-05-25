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

2026-05-24：继续提高 outline 成功数量。

- `src/Passes/evm/SolidityPatterns.cpp:404` 新增 `dependsOnCalldataSizeValue`，追 `evm_calldatasize` 时不再穿过普通 call 的参数。
- `src/Passes/evm/SolidityPatterns.cpp:432` 收紧 `isSelectorSizeGate`，避免把 body 里依赖 calldata size 参数的 `delegatecall` 分支误当 dispatcher。
- `src/Passes/evm/SolidityPatterns.cpp:982` 调整 `SelectorEntryOutliningPass::run`，每成功 outline 一个 region 就重算 CFG 和候选，支持同一个 selector 里拆多个独立 inline body。
- `src/Passes/evm/SolidityPatterns.cpp:1011` 临时 skip 不立即写入 IR；只有这一轮没有任何 region 能 outline 时，才写 `selector_outline_skipped`，避免“先拆共享 tail 后可拆”的路径留下假 skip。
- `test/evm/solidity-rewrite/cases/0023_19493052_5c9e137a4b_8640b1f47e0e.ll:136` 新增真实 fallback/receive 样例，当前能 outline 两个 helper。
- `test/evm/solidity-rewrite/manifest.json:30` 更新 `0002_delegatecall_no_nonpayable` oracle：现在同一个 selector 里能拆两个 helper。
- `test/evm/solidity-rewrite/manifest.json:41` 新增 `0023_19493052_5c9e137a4b_8640b1f47e0e` rewrite oracle，期望两个 helper、两个 call、无 skipped。
- `test/evm/solidity-patterns/manifest.json:98` 更新 `0002_delegatecall_no_nonpayable` 的 patterns oracle：outline 后 selector 里少一个 ABI decode 标注，外部调用已搬到 helper，`selector_inlined_body` 为 0。

当前结果：

- rewrite suite 从 58 个 case 增加到 59 个 case。
- outline 成功 case 从 2 个增加到 3 个：`0014_proxy_like`、`0002_delegatecall_no_nonpayable`、`0023_19493052_5c9e137a4b_8640b1f47e0e`。
- `0002_delegatecall_no_nonpayable` 和 `0023_19493052_5c9e137a4b_8640b1f47e0e` 都能在一个 selector 中生成 2 个 outlined helper。
- `0046/0047` 这类 receive/fallback 先分叉、再通过 PHI 汇合到共同 delegatecall tail 的样例还不能安全拆。现在会保守留下 `live_out` skip，后续需要专门处理共享 tail/PHI 复制，不能靠放宽 dispatcher 误判来拆。

验证：

- `cmake --build ./build --target all -j4`：通过。
- `llvm-22.1.0.obj/bin/llvm-as /tmp/0046.after2.ll -o /tmp/0046.after2.bc`：通过。
- `llvm-22.1.0.obj/bin/llvm-as /tmp/0047.after2.ll -o /tmp/0047.after2.bc`：通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(rewrite|patterns)' --output-on-failure`：通过。
  - `notdec.evm.solidity_patterns`：58/58 passed，85.10s。
  - `notdec.evm.solidity_rewrite`：59/59 passed，84.18s。

性能和维护成本：

- 同口径 patterns suite 从上一轮 85.97s 到 85.10s，没有看到性能下降。
- 实现效果：7/10。已经支持同一 selector 多个独立 inline body；共享 merge/PHI 还没处理。
- 复杂度：6/10。多轮重扫 CFG 比单轮复杂一点，但逻辑仍集中在 `SelectorEntryOutliningPass::run`。
- 维护成本：6/10。后续要处理共享 tail 时，需要明确 PHI incoming 裁剪/复制策略，不能用更宽松的 dispatcher 判断绕过去。

2026-05-24：支持共享 tail / PHI 的窄形态。

- `src/Passes/evm/SolidityPatterns.cpp:530` 新增 `hasSharedTailEntry`，识别 region 内有 block 被 region 外路径共同进入的情况。
- `src/Passes/evm/SolidityPatterns.cpp:540` 调整 `collectRegionInputs`，PHI 只收集来自 region 内 incoming block 的值，避免把另一条 selector 路径的定义错误传给 helper。
- `src/Passes/evm/SolidityPatterns.cpp:573` 新增 `regionInputsAvailableAtEntry`，用 LLVM `DominatorTree` 检查 helper 参数在所有外部入口边之前都可用。
- `src/Passes/evm/SolidityPatterns.cpp:637` 新增 `pruneOutsidePhiIncoming`，clone helper 时删除共享 tail PHI 里来自其他路径的 incoming。
- `src/Passes/evm/SolidityPatterns.cpp:682` 放宽 `getOutlineSkipReason` 对 multi-entry block 的处理。现在这类 block 按共享 tail 处理，后面的 live-out / outside-successor 检查仍保留。
- `src/Passes/evm/SolidityPatterns.cpp:791` 调整 `replaceRegionWithCall`，替换当前路径后只删除当前路径独占块；对仍被另一条路径使用的共享 tail 及其 return/revert 后继保留。
- `src/Passes/evm/SolidityPatterns.cpp:808` 删除当前路径独占块前，先把保留 tail PHI 中来自待删除块的 incoming 移除，避免 dangling use。
- `test/evm/solidity-rewrite/cases/0046_19493140_f89daa079d_8aad1d5bf751.ll:136` 新增真实共享 tail 样例。
- `test/evm/solidity-rewrite/manifest.json:52` 新增 `0046_19493140_f89daa079d_8aad1d5bf751` oracle，期望两个 helper、两个 call、无 skipped。

当前结果：

- rewrite suite 从 59 个 case 增加到 60 个 case。
- outline 成功 case 从 3 个增加到 4 个：新增 `0046_19493140_f89daa079d_8aad1d5bf751`。
- 额外验证同形态 `0047_19493140_2a60672937_8aad1d5bf751` 也能拆出 2 个 helper。
- 前 40 个带 selector inline 调用的真实输出扫描：40 个都能通过 `llvm-as`；5 个 case outline，合计 9 个 helper；剩余 skipped 1 个。

验证：

- `cmake --build ./build --target all -j4`：通过。
- `./build/bin/notdec test/evm/solidity-rewrite/cases/0046_19493140_f89daa079d_8aad1d5bf751.ll -o /tmp/0046.shared2.ll --tr-level=0`：通过。
- `llvm-22.1.0.obj/bin/llvm-as /tmp/0046.shared2.ll -o /tmp/0046.shared2.bc`：通过。
- `./build/bin/notdec /sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260521-evm2llvm-train-batch002/outputs/0047_19493140_2a60672937_8aad1d5bf751.ll -o /tmp/0047.shared.ll --tr-level=0`：通过。
- `llvm-22.1.0.obj/bin/llvm-as /tmp/0047.shared.ll -o /tmp/0047.shared.bc`：通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(rewrite|patterns)' --output-on-failure`：通过。
  - `notdec.evm.solidity_patterns`：58/58 passed，85.29s。
  - `notdec.evm.solidity_rewrite`：60/60 passed，84.84s。

性能和维护成本：

- patterns suite 从上一轮 85.10s 到 85.29s，基本持平。
- 实现效果：8/10。已经覆盖独立 region、多 region、共享 PHI tail 三类主要形态。
- 复杂度：7/10。替换时需要区分删除块和保留 tail，理解成本上升，但仍局限在 outline clone/rewrite 逻辑。
- 维护成本：7/10。后续继续放宽前，需要补更多真实 CFG case；不能把所有 multi-entry 都当安全共享 tail。

2026-05-24：分析 skipped case 并扩大扫描。

- `src/Passes/evm/SolidityPatterns.cpp:614` 放宽 `regionHasOutsideSuccessor`，把 `revert(0,0)` 空拒绝块作为可映射出口，不再因此 skip。
- `src/Passes/evm/SolidityPatterns.cpp:644` 新增 `mapEmptyRejectExits`，clone helper 时为 region 外的空拒绝块 clone 一个 helper-local reject block。
- `src/Passes/evm/SolidityPatterns.cpp:771` 在 `cloneSelectorRegion` 中调用 `mapEmptyRejectExits`。
- `test/evm/solidity-rewrite/cases/0022_19493052_21bd0f78bf_e92711815df7.ll:136` 新增真实空 reject 出口样例。
- `test/evm/solidity-rewrite/manifest.json:41` 新增 `0022_19493052_21bd0f78bf_e92711815df7` oracle，期望 1 个 helper、无 skipped。

扫描结论：

- 修复前，前 200 个带 selector inline 调用的真实输出全部通过 `llvm-as`；20 个 case outline，合计 28 个 helper；8 个 skipped，全部是 `outside_successor`。
- `0022_19493052_21bd0f78bf_e92711815df7` 是最小 skipped 样例：body 正常走 `delegatecall`，失败的 nonpayable 分支跳到 `revert(0,0)` 空拒绝块。
- 修复后，同一批 200 个样例全部通过 `llvm-as`；26 个 case outline，合计 36 个 helper；skipped 降到 0。
- 剩余未 outline 的主要是“有 `selector_inlined_body` marker，但当前 dispatcher/region 边界规则没有把它们当作可拆候选”，不是 skipped 类问题。

验证：

- `cmake --build ./build --target all -j4`：通过。
- `./build/bin/notdec /sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260521-evm2llvm-train-batch001/outputs/0022_19493052_21bd0f78bf_e92711815df7.ll -o /tmp/0022.reject.ll --tr-level=0`：通过。
- `llvm-22.1.0.obj/bin/llvm-as /tmp/0022.reject.ll -o /tmp/0022.reject.bc`：通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(rewrite|patterns)' --output-on-failure`：通过。
  - `notdec.evm.solidity_patterns`：58/58 passed，85.29s。
  - `notdec.evm.solidity_rewrite`：61/61 passed，84.86s。

性能和维护成本：

- patterns suite 从上一轮 85.29s 到本轮 85.29s，持平。
- 实现效果：8/10。当前扫描口径下 skipped 已清零；下一步重点应转向未进入候选的 marker。
- 复杂度：7/10。新增空 reject 出口映射，和已有 void-return 出口映射同类。
- 维护成本：7/10。要避免继续把任意外部 successor 都当出口；目前只支持 `ret void` 和 `revert(0,0)` 两种明确形态。

2026-05-25：继续分析 skipped 和未进入候选的样例。

- 重新扫描前 200 个带 selector inline 调用的真实输出：200 个全部通过 `notdec` 和 `llvm-as`，skipped 仍为 0。
- 当前 26 个 case outline，合计 36 个 helper；176 个 case 仍保留 `selector_inlined_body` marker。
- `0298_19494123_f892a1f7d9_943b732c640f` 有 5 个 marker，但入口按 `caller/origin` 分支，不是 selector/calldata dispatcher。它是负例，不应该为了提高 outline 数量去拆。
- `0185_19493516_e7180ca8be_c7c648477e0c` 有 2 个 marker，原始 IR 里有 calldata-size 分支，但优化后两条边合并成直线，outline pass 看不到 dispatcher successor。这类属于“入口边界被优化掉”，需要单独设计，不能和 skipped 混在一起处理。
- `test/evm/solidity-rewrite/cases/0298_19494123_f892a1f7d9_943b732c640f.ll:136` 新增负例样例。
- `test/evm/solidity-rewrite/manifest.json:74` 新增 `0298_19494123_f892a1f7d9_943b732c640f` oracle，期望不 outline、不 skipped，但保留 5 个 `selector_inlined_body` marker。

验证：

- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`：通过，62/62 passed，85.92s。

下一步判断：

- skipped 类问题当前已经处理完。继续提高 outline 数量时，重点不是放宽 skip，而是处理“未进入候选”的形态。
- 可以优先研究 `0185` 这类优化后入口边界消失的 fallback-only / receive-only 形态，但需要先设计 entry-rooted outline，不能直接复用 dispatcher successor 替换逻辑。

2026-05-25：新增 entry-rooted whole-selector outline 的窄形态。

- `src/Passes/evm/SolidityPatterns.cpp:702` 新增 `hasWholeSelectorOutlineShape`，只接受没有 dispatcher rewrite 成功、函数内有 `evm_calldatasize`、有 body signal、没有 `caller/origin` 分支、也没有 `mstore(64,128)` prologue 的 selector。
- `src/Passes/evm/SolidityPatterns.cpp:857` 新增 `replaceWholeFunctionWithCall`，把原 selector 函数体替换成 `call helper; ret void`。
- `src/Passes/evm/SolidityPatterns.cpp:1208` 在普通 dispatcher outline 没有改动时，尝试 whole-selector outline。
- `test/evm/solidity-rewrite/cases/0003_19493003_0a0ab7aaf4_d8a74687f33e.ll:136` 新增最小 fallback proxy 样例。
- `test/evm/solidity-rewrite/manifest.json:41` 新增 `0003_19493003_0a0ab7aaf4_d8a74687f33e` oracle，期望 1 个 helper、无 skipped。

边界判断：

- `0003_19493003_0a0ab7aaf4_d8a74687f33e` 能从整个 selector 函数 outline 成 helper。
- `0298_19494123_f892a1f7d9_943b732c640f` 仍不 outline，因为它是 `caller/origin` 分支负例。
- `0185_19493516_e7180ca8be_c7c648477e0c` 暂时仍不 outline，因为它有 free-memory prologue。直接搬整函数会影响后续 prologue / ABI decode annotation，需要另做 helper-context annotation 设计。

扫描和验证：

- 前 200 个带 selector inline 调用的真实输出：200 个全部通过 `notdec` 和 `llvm-as`；outline case 从 26 增加到 195，helper 从 36 增加到 205；skipped 仍为 0。
- `ctest --test-dir build -R 'notdec.evm.solidity_(rewrite|patterns)' --output-on-failure`：通过。
  - `notdec.evm.solidity_patterns`：58/58 passed，85.24s。
  - `notdec.evm.solidity_rewrite`：62/62 passed，85.11s。
- 新增 `0003` 后单独跑 `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`：通过，63/63 passed，85.19s。

性能和维护成本：

- patterns suite 从上一轮 85.29s 到本轮 85.24s，基本持平。
- 实现效果：9/10。当前 200 样例里绝大多数 fallback proxy 直线形态都能 outline。
- 复杂度：7/10。整函数替换逻辑独立，和 region rewrite 分开，复杂度可控。
- 维护成本：7/10。whole-selector 规则必须保持窄；带 prologue 的形态要等 helper 上下文 annotation 设计清楚后再放开。

2026-05-25：把 outline 出来的 helper 改成 public entry 上下文。

- `src/Passes/evm/SolidityPatterns.cpp:773` 把 helper 名从 `notdec_solidity_selector_inline.<selector>` 改为 `public__notdec_solidity_selector_inline.body`。
- 这样后续 `AbiDecodePass` 这类按 `isPublicEntryFunction` 判断的 pass 会处理拆出来的业务函数。
- helper 名里不能带原 selector 函数名，因为原名含 `function_selector`，会让 helper 被 `isSelectorFunction` 误判成 selector，再次进入 outline。
- `test/run_evm_solidity_rewrite_suite.py:46` 增加 outline helper / call 计数函数，兼容旧的 `notdec_solidity_selector_inline.*` 和新的 `public__notdec_solidity_selector_inline.*`。
- `test/evm/solidity-patterns/manifest.json:27` 和 `:98` 调整 `0014_proxy_like`、`0002_delegatecall_no_nonpayable` 的 `abi_decode` 期望。helper 变成 public entry 后，`AbiDecodePass` 会在 helper 内多标一次。

验证：

- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`：通过，63/63 passed，86.41s。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，58/58 passed，86.38s。
- 扫描 `/sn640/NotDecChainExp/evm2llvm_apehex_pilot` 下按路径排序前 200 个输出：200 个 `notdec` 通过，200 个 `llvm-as` 通过；120 个 case outline，合计 125 个 helper；`bad_helper_names_with_function_selector=0`。
- 这批更宽口径里有 2 个 `outside_successor` skipped，两个样例都已经各自成功 outline 了 1 个 helper；不是改名引入的重复 outline。

性能和维护成本：

- patterns suite 从上一轮 85.24s 到本轮 86.38s，增加约 1.3%，在测试波动范围内。
- 实现效果：8/10。拆出的 helper 已能进入 public-entry 后续 pass，且避免 selector 误判。
- 复杂度：6/10。改动主要是命名和测试计数。
- 维护成本：6/10。后续如果改 `isPublicEntryFunction` 的判定规则，需要一起复查 helper 命名。
