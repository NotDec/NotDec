# 2026-08-03 递归字节游标 SCC 归一化

## 原始 prompt

> 那感觉这样折叠是没问题的，还是说具体的normalizeTailValueRecursiveRecords函数的具体细节可以改进？反倒是那边没有折叠的部分，得考虑怎么涵盖一下，最好折叠成char*数组buffer。为什么经常重新发现这个问题，是因为相关测试用例已经保存了预期的类型推理结果吗？
>
> 按照这个试一下吧

## 背景和目标

`normalizeTailValueRecursiveRecords()` 只处理 `struct R { T; R; }` 这类
单个 record 的尾部按值递归。`fortune::conv_pat()` 的输出游标有 `+1` 与两条
`+4` 回边，求解后是 record/union 组成的按值递归图，不能走旧规则；它本质上仍是
逐字节写入的 `char *` buffer。

本次只在 HType 后处理阶段处理这个失真的形状：若一个 record/union 强连通图的
非 padding 叶子全是 8-bit 标量或定长 byte array，则将图中的 aggregate 引用改为
`i8[]`。指针、宽字段、函数和其他类型一律拒绝，避免把链表或真实结构布局抹掉。

## 实现

- `src/TypeRecovery/mlsub/HTypeNormalize.cpp:403-575`
  - 新增 `ByteBufferRecursiveSCC`。
  - 从每个 record/union 出发检查全部按值 aggregate 边；要求至少有一个环，且访问到
    的每个 declaration 都能回到根，避免把普通的 byte-shaped helper record 一并删除。
  - 只接受 8-bit `top`/`bottom`/integer/type variable、大小严格匹配的定长 byte array
    与 padding；指针和宽字段都会使候选失败。
- `src/TypeRecovery/mlsub/HTypeNormalize.cpp:579-725`
  - 新增 `rewriteByteBufferType()` 与
    `normalizeByteBufferRecursiveSCCs()`。
  - 重写 value lower/upper、memory/storage、record field、union member 和 typedef，
    所以所有可达 HType 都不再引用已折叠的 declaration；外层指针保持不变，因此结果是
    `i8[]*` 而非少或多一层指针。
- `src/TypeRecovery/mlsub/HTypeNormalize.cpp:1077-1080`
  - 新步骤放在 tail-value 归一化之后、binder collapse 之前。旧规则仍优先处理它能
    精确表达的单 record 尾数组。
- `test/type-recovery/llvm-ir/expected/tr-level-2/21_PointerAnalysisBranchingFieldCycle.htypes`
  - 更新分支字节游标正例：递归 `rec_33/struct_0/union_0` 变为 `i8[]*`。
- `test/type-recovery/llvm-ir/cases/22_PointerAnalysisWideFieldCycle.ll`
  - 新增同形状的 `i32` 分支回边反例；其 golden 仍为 `struct_0*`。
- `test/type-recovery/llvm-ir/manifest.json:129-139`
  - 将反例纳入当前 frozen stage-B LLVM-IR 套件。

原来的 `21` golden 的确把递归声明图当作预期结果保存下来，所以只能防止结果变化，
不能表达“这里应是 byte buffer”。现在正例直接固定 `i8[]*`，反例固定非字节布局，
以后同一问题不会再被旧快照掩盖。

## 验证

已运行：

```bash
cmake --build build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
./llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-byte-buffer-after.ll -o /tmp/notdec-byte-buffer-after.bc
./llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-wide-field-cycle.ll -o /tmp/notdec-wide-field-cycle.bc
env NOTDEC_BINARYSUB_THREADS=8 /usr/bin/time -v ./build/bin/notdec \
  /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  --tr-level=2 --merge-struct-ptr-load-store -g --fast-work-dir \
  --work-dir=/tmp/notdec-byte-buffer-fortune.7DG5SG/work \
  --dump-htypes=/tmp/notdec-byte-buffer-fortune.7DG5SG/fortune.htypes \
  -o /tmp/notdec-byte-buffer-fortune.7DG5SG/fortune.out.ll
./llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-byte-buffer-fortune.7DG5SG/fortune.out.ll \
  -o /tmp/notdec-byte-buffer-fortune.7DG5SG/fortune.out.bc
```

结果：

- `notdec.type_recovery.llvm_ir.tr_level_2`：22/22 通过。
- 已有 `06_SimpleRecursive2` 的指针递归输出不变；新 `22` 的宽字段环保持
  `struct_0*`。
- fortune 源码 IR 成功，wall `4.75s`、峰值 RSS `497588 KB`。日志显示
  `normalized 8 recursive byte-buffer decl(s) to i8[]`；
  `conv_pat::%sp.188`、`%sp.2`、`%sp.1.lcssa`、返回值和 `arg0` 都是 `i8[]*`。
  这正是源码中 `+1/+4/+4` 写入循环。第一个只读扫描循环 `%sp.086` 仍为
  `struct_60*`，其图含有 pointer leaf，按本次边界不折叠。
- 全量 tr-level=2 CTest 中，SysY 的 9 个用例均因本机 `clang-14` 重生的输入 IR
  与历史 golden 的 block/value label 不同而失败；`00_main` 没有递归 aggregate，
  也没有触发本次日志。realworld fortune 在类型恢复前就因 extra-constraints 的
  `ir_anchor.sha256` 失配 abort。这两项未修改。

## 简评

- 实现效果：8/10。覆盖了旧 tail-array 规则遗漏的多分支 record/union 递归图，并在
  fortune 的目标写入游标上得到 `i8[]*`。
- 复杂度：6/10。新增一套 declaration 图检查与 HType rewrite，但只留在
  `HTypeNormalize.cpp`，没有影响约束求解和 pointer analysis。
- 维护成本：6/10。HType 新增容器种类时需要补 rewrite 分支；规则故意保守，可能漏掉
  带 typedef 或非 byte 证据的 buffer，但不会以猜测换取错误折叠。

更激进的方案是从 GEP/phi 的 def-use 直接识别 cursor，再在约束生成时标记 buffer。
它能覆盖更多图，但会把 HType 输出修正扩展到 pointer analysis 的语义规则；当前没有
足够的反例，先保留这个可验证的后处理范围。
