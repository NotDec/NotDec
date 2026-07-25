# free(phi) 拆分实现

## 背景

fortune 的 `add_file` 里出现了只为 `free(void*)` 服务的 PHI：
`FILEDESC *fp` 和 `char *path` 先合到同一个 SSA 值，再传给 `free`。这会让 MLsub
看到假的同函数值流，进而把 `FILEDESC*` 结构体证据传到字符串指针上。

## 实现

- `src/Passes/PassManager.cpp:90` 增加 `isDirectFreeCall()`，只识别直接调用 `free` 且只有一个实参的调用。
- `src/Passes/PassManager.cpp:96` 增加 `isFirstRealInstructionInBlock()`，要求 `free(phi)` 是 PHI 后第一个真实指令，避免移动跨过有副作用的指令。
- `src/Passes/PassManager.cpp:109` 增加 `successorIndicesTo()`，用于处理一个 predecessor 到同一个 join block 的多条边。
- `src/Passes/PassManager.cpp:125` 新增 `FreePhiSplitPass`：
  - 只处理 `free` 实参是同 block PHI、且 PHI 只有这个 use 的情况。
  - 如果 predecessor 有多个 successor，先用 `SplitCriticalEdge()` 拆边。
  - 在每条进入 join block 的边上 clone 一条 `free(incoming)`。
  - 删除原来的 `free(phi)`；如果 PHI 已无 use，也删除 PHI。
- `include/notdec/Passes/PassManager.h:89` 给 `add_type_recovery_passes()` 增加 `SplitFreePhi` 开关，默认开启。
- `src/Passes/PassManager.cpp:456` 在类型推理前置流程里保留固定运行入口。
- `src/Passes/PassManager.cpp:527` EVM 路径显式传 `SplitFreePhi=false`，不跑这个 pass。
- `src/Passes/PassManager.cpp:547` x64/source IR 路径使用默认开启。
- `src/Passes/PassManager.cpp:574` Wasm/frozen type recovery 路径使用默认开启。

## 验证

- 构建：
  - `cmake --build ./build --target notdec -j4`
  - 结果：通过。
- fortune frozen type recovery：
  - `./build/bin/notdec /tmp/notdec-one-sided-no-cross-trace-20260725-a/work/02-mlsub-input.ll --tr-level=2 --frozen-tr-input-ir --merge-struct-ptr-load-store -g --work-dir=/tmp/notdec-free-phi-split-20260725-b/work -o /tmp/notdec-free-phi-split-20260725-b/out.ll`
  - 结果：通过；日志显示 `Info: split free(phi) in add_file: 1 call(s), 13 edge-local free call(s)`。
- IR 检查：
  - `./llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-free-phi-split-20260725-b/out.ll -o /tmp/notdec-free-phi-split-20260725-b/out.bc`
  - 结果：通过。
- 回归：
  - `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`
  - 结果：通过。
- fortune 源 IR 入口：
  - `./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll --tr-level=2 --merge-struct-ptr-load-store -g --work-dir=/tmp/notdec-free-phi-split-20260725-c/work --dump-htypes=/tmp/notdec-free-phi-split-20260725-c/htypes.txt -o /tmp/notdec-free-phi-split-20260725-c/out.ll`
  - 结果：通过；这条带 debug info 的源码级 IR 会固定运行 `FreePhiSplitPass`，只是没有匹配到 `free(phi)` 形状，所以没有拆分日志。

## 效果

fortune 的 `add_file::%bb.loop_entry122.i3/i4` 回到字符串指针：

- `add_file::%bb.loop_entry122.i3 => ptr<load=i8, store=i8, psize=8>`
- `add_file::%bb.loop_entry122.i4 => ptr<load=i8, store=i8, psize=8>`
- `add_file::arg1/arg2` 也保持为字符串指针。

## 评分

- 实现效果：8/10。解决当前 `free(phi)` 导致的错误合并，且验证了 fortune 错例。
- 复杂度：7/10。逻辑集中在一个小 pass，但 CFG 拆边本身有一定理解成本。
- 维护成本：7/10。规则保守，触发条件窄；后续如果要支持 `invoke free` 或间接 free，需要单独扩展。
