# Recovery Pass Workdir 日志分段

日期：2026-04-17

## 1. 目的

把 `LinearAllocationRecovery`、`StackPointerFinderAnalysis`、`MemsetMatcher`、
`MemcpyMatcher` 的诊断统一写入 workdir 下的
`01-recovery-passes.log`，并且让每个 pass 的输出都有明显分隔，便于直接按段查看。

## 2. 具体修改

1. 在 [include/notdec/Utils/Utils.h:38](/sn640/NotDec/include/notdec/Utils/Utils.h#L38)
   到 [include/notdec/Utils/Utils.h:40](/sn640/NotDec/include/notdec/Utils/Utils.h#L40)
   新增 `appendWorkDirLog()` / `appendRecoveryPassLog()` 声明，统一约定恢复阶段日志文件名为
   `01-recovery-passes.log`。
2. 在 [src/Utils/Utils.cpp:46](/sn640/NotDec/src/Utils/Utils.cpp#L46)
   到 [src/Utils/Utils.cpp:64](/sn640/NotDec/src/Utils/Utils.cpp#L64)
   实现 `notdec::appendWorkDirLog()`：
   - 仅在配置了 workdir 时生效
   - 自动创建 workdir
   - 以 append 模式写入日志文件
3. 在 [src/Passes/StackPointerFinder.cpp:95](/sn640/NotDec/src/Passes/StackPointerFinder.cpp#L95)
   到 [src/Passes/StackPointerFinder.cpp:159](/sn640/NotDec/src/Passes/StackPointerFinder.cpp#L159)
   的 `StackPointerFinderAnalysis::run()` 中：
   - 把原先直接打印到终端的栈指针猜测信息改为写入
     `01-recovery-passes.log`
   - 在 [src/Passes/StackPointerFinder.cpp:97](/sn640/NotDec/src/Passes/StackPointerFinder.cpp#L97)
     增加 `============== StackPointerFinderAnalysis ===============`
     分隔头
4. 在 [src/Passes/StackAlloca.cpp:195](/sn640/NotDec/src/Passes/StackAlloca.cpp#L195)
   到 [src/Passes/StackAlloca.cpp:323](/sn640/NotDec/src/Passes/StackAlloca.cpp#L323)
   的 `LinearAllocationRecovery::run()` 中，把栈恢复相关错误与状态输出统一改为
   `appendRecoveryPassLog()`；同时在
   [src/Passes/StackAlloca.cpp:115](/sn640/NotDec/src/Passes/StackAlloca.cpp#L115)
   到 [src/Passes/StackAlloca.cpp:145](/sn640/NotDec/src/Passes/StackAlloca.cpp#L145)
   的 `LinearAllocationRecovery::matchDynamicAllocas()` 里，也把
   `unrecognized sp modification` 诊断收进同一日志。
5. 在 [src/Passes/MemOpMatcher.cpp:27](/sn640/NotDec/src/Passes/MemOpMatcher.cpp#L27)
   到 [src/Passes/MemOpMatcher.cpp:35](/sn640/NotDec/src/Passes/MemOpMatcher.cpp#L35)
   新增 `appendMemOpPassBanner()`，并在：
   - [src/Passes/MemOpMatcher.cpp:109](/sn640/NotDec/src/Passes/MemOpMatcher.cpp#L109)
     到 [src/Passes/MemOpMatcher.cpp:249](/sn640/NotDec/src/Passes/MemOpMatcher.cpp#L249)
     的 `MemsetMatcher::run()`
   - [src/Passes/MemOpMatcher.cpp:252](/sn640/NotDec/src/Passes/MemOpMatcher.cpp#L252)
     到 [src/Passes/MemOpMatcher.cpp:389](/sn640/NotDec/src/Passes/MemOpMatcher.cpp#L389)
     的 `MemcpyMatcher::run()`
   中按函数首次命中时输出
   `============== MemsetMatcher [func] ===============` /
   `============== MemcpyMatcher [func] ===============`
   分隔头，再追加具体 merge 记录。

## 3. 验证

使用下面命令验证：

```bash
./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  --emit-tr-input-ir=/tmp/fortune.tr-input.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-fortune-work
```

验证结果：

1. 终端不再直接打印 `LinearAllocationRecovery` / `StackPointerFinderAnalysis` /
   `MemsetMatcher` 的这批诊断
2. `/tmp/notdec-fortune-work/01-recovery-passes.log` 中已出现按 pass 分段的日志
3. workdir 中同时保留 `00-lifted.ll`、`01-1-BeforeStackAlloca.ll`、
   `02-mlsub-input.ll` 等既有产物
