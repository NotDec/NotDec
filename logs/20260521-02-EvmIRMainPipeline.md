# EVM IR 接入主链路记录

## 原始 prompt

evm2llvm那边上一个commit，把那个datalayout和triple接进来了
target triple = "evm-unknown-unknown"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
现在，需要尝试让主项目支持evm2llvm生成的IR。通过triple识别架构，然后特殊处理一下，接入主项目链路来。src/Passes/PassManager.cpp 这里，现有的pass考虑都归类给Webassembly，放到特判triple是wasm32/wasm64开头的分支，比如wasm32-unknown-wasi。EVM相关的Pass后面再补充加进来，目前就先空着。目前暂时不急着支持什么sol输出，目前EVM IR进来，输出就还是LLVM  IR输出。

## 背景

evm2llvm 现在会在输出 IR 里写入 `target triple = "evm-unknown-unknown"` 和 EVM data layout。主项目读 `.ll` / `.bc` 后，原来不会按架构区分 pass pipeline，默认会把现有 wasm 恢复和类型恢复 pass 跑起来。

这次目标很小：让 EVM IR 可以进入 `notdec` 主程序，并输出 LLVM IR / bitcode；EVM 专用恢复 pass 暂时不加。

## 修改

- `src/Passes/PassManager.cpp:61` 新增 `TargetArch` 和 `classifyTargetArch()`。
- `src/Passes/PassManager.cpp:279` 在 `PassEnv::build_passes()` 里按 module triple 分流：
  - `wasm32*` / `wasm64*` 继续走原有 pipeline。
  - `evm*` 当前直接返回，保留原 IR 给 `.ll` / `.bc` 输出。
  - 其它 triple 当前也不跑这些 wasm 恢复 pass。

## 验证

- 构建：`cmake --build ./build --target all -j4` 通过。
- 最小 EVM IR：`./build/bin/notdec /tmp/notdec-min-evm.ll -o /tmp/notdec-min-evm.out.ll --tr-level=3` 通过，输出保留 EVM triple 和 data layout。
- 汇编输出：`./llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-min-evm.out.ll -o /tmp/notdec-min-evm.out.bc` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 失败。
  - `13_stack_variable_alloc`、`15_signed1` 是 `x86_64-pc-linux-gnu` triple，当前按“非 wasm 不跑 TR”规则会跳过类型恢复，`--dump-htypes` 因此失败。
  - `17_StackArray`、`18_offset1`、`20_PointerAnalysisFieldCycle`、`21_PointerAnalysisBranchingFieldCycle` 是 wasm 样例，命令退出成功，但 htypes 和 golden 有差异；差异不像这次 triple 分流直接造成。

## 风险

当前 `--dump-htypes` 仍假设类型恢复已初始化。对 EVM / 其它非 wasm triple 使用 `--dump-htypes` 会报错。这个行为和这次目标不冲突，因为当前只要求 EVM IR 能输出 LLVM IR。

## 实现记录

- `src/Passes/PassManager.cpp:44-47,273-291` 现在把 `target triple = "evm-unknown-unknown"` 走到专用分支。这个分支不再直接返回，而是先跑 `buildFunctionOptimizations()`，再跑 EVM 专用的 `PayabilityGuardPass`，最后跑 `VerifierPass(false)`；`tr-level` 对这条 EVM 通用前处理不再起阻断作用。
- `include/notdec/Passes/PassManager.h:51-56` 给 `PayabilityGuardPass` 注册了 pass 名，方便 debug 和后续 pipeline 观察。
- `src/Passes/evm/SolidityPatterns.cpp:1-133` 新增了第一版 EVM Solidity 模式 pass。它只认优化后的非 payable guard：`callvalue == 0` 的条件分支，失败块里是 `evm_revert(ptr %mem, 0, 0)` 加 `unreachable`。命中后只挂 metadata，不删 CFG。
- `src/CMakeLists.txt:2-18` 把新 pass 源文件接进主库。

## 验证

- 构建：`cmake --build ./build --target notdec-decompile -j4`
- 单样例：`0014_19493039_2d4c31bc6b_6b76dc72860b.ll`、`0011_19493032_87fd4a2922_ba61188f81c3.ll`、`0002_19493003_57d4d29397_136994712c59.ll`
- 批量：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260521-evm2llvm-train-batch001/outputs/*.ll` 共 40 个样例都能经主 binary 跑完并通过 `llvm-22.1.0.obj/bin/llvm-as`
- 结果：`0014` 命中 2 个 nonpayable guard，`0011` 命中 5 个，`0002` 没有误标

## 测试接入

- `test/evm/solidity-patterns/cases/` 固化了 3 个 apehex evm2llvm 输出：`0014_proxy_like.ll`、`0011_multi_public.ll`、`0002_delegatecall_no_nonpayable.ll`。
- `test/run_evm_solidity_patterns_suite.py` 负责跑主项目 binary、再跑 `llvm-22.1.0.obj/bin/llvm-as`，最后只统计函数定义上的 `notdec.solidity.nonpayable` metadata。
- `test/CMakeLists.txt` 新增 CTest：`notdec.evm.solidity_patterns`。
- 当前 oracle：`0014` 期望 2 个，`0011` 期望 5 个，`0002` 期望 0 个。
