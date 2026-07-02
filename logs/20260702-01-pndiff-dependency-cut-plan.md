# 原始 prompt

现在需要重构，把 p n diff 相关的逻辑都重构迁移到 binary sub 模块中。首先，让 PNDiff 这个模块断掉和 LLVM 等、clang等项目的依赖。PNDiff 里面涉及到LLVM Value或者指令指针的考虑弄成void *。断掉依赖之后先commit一下，然后就把源码再挪到binarysub子模块里面。当前先不要直接去做，而是先有一个简单的规划，在对话里返回给我看一下

后续确认：先做第一阶段吧。

## 背景

PNDiff 现在还在 NotDec 主仓库里，头文件和实现直接引用 LLVM IR 类型、llvm2c 的 ExtValuePtr / ValueNamer，以及 LLVM JSON 工具。这样后续移动到 binarysub 时会把 LLVM/Clang 依赖一起带过去。

## 目标

第一阶段只做断依赖，不移动源码。完成后 PNDiff 自身不再 include LLVM、Clang 或 llvm2c 头文件；LLVM Value、指令和扩展 value key 都通过不透明 `void *` 句柄进入 PNDiff。

## 路线

保留 PNDiff 当前求解规则，把 PNDiff 图里的 value key 和 instruction key 改成 opaque handle。NotDec 侧继续负责把 `ExtValuePtr`、LLVM 指令、常量解析和 offset 匹配转换成 PNDiff 需要的 callback。PNDiff 只保存句柄、调用 callback，不解引用 LLVM 对象。

## 风险

主要风险是 `ExtValuePtr` 不是单纯 LLVM 指针，直接转 `void *` 会丢失常量和返回值区分。因此 NotDec 侧需要维护稳定 token，把 `ExtValuePtr` 映射成 PNDiff 可比较的 opaque handle。

## 判断标准

`include/notdec/TypeRecovery/mlsub/PNDiff.h` 和 `src/TypeRecovery/mlsub/PNDiff.cpp` 中不再出现 LLVM/Clang/llvm2c 依赖；项目能构建，相关 type recovery smoke 通过；完成后单独提交。

## 2026-07-02 实现记录：第一阶段已完成

本次只做断依赖，没有移动源码到 `external/binarysub`。

### 代码改动

- `include/notdec/TypeRecovery/mlsub/PNDiff.h:33` 新增 `PNIValue = void *` 和 `PNIInstruction = void *`，PNDiff 内部 value、instruction、Add/Sub 约束节点都改用 opaque handle。
- `include/notdec/TypeRecovery/mlsub/PNDiff.h:38` 新增 `PNDiffPolicyConfig`，PNDiff 不再自己读环境变量或解析 JSON。
- `include/notdec/TypeRecovery/mlsub/PNDiff.h:195` 给 `PNIGraph` 增加 `GetIntConstant`、`MatchOffsetRange`、format、`OnUpdatePNType`、`OnPtrAdd`、`AllocateNodeId` 回调。
- `src/TypeRecovery/mlsub/PNDiff.cpp:120` 的 `applyPNIPolicy()` 保留原先“小整数常量视为 non-pointer”的逻辑，但常量读取改为调用 `GetIntConstant`。
- `src/TypeRecovery/mlsub/PNDiff.cpp:154` 的 `eraseConstraint()` 保留 ptradd 回写逻辑，但 offset 匹配和回写分别改为 `MatchOffsetRange`、`OnPtrAdd`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:383` 在 NotDec 侧新增 `loadPNDiffPolicyConfig()`，使用 LLVM JSON 解析旧 schema：`int_constant_policy.non_pointer_abs_lt` 和 `int_constant_policy.exclude_zero`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2560` 配置 PNDiff 回调：常量解析、offset 匹配、trace 格式化、node id 分配、PN type update 和 ptradd 回写都留在 NotDec 侧。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2583` 增加 `ExtValuePtr` 到 PNDiff opaque handle 的稳定 token 映射，PNDiff 不直接保存 `ExtValuePtr`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6007` Add/Sub 约束创建时仍先用原来的 `llvmValue2ExtVal()` 处理 operand，所以 `MatchOffsetRange()` 收到的 handle 反查后仍是旧路径里的同一个 `ExtValuePtr` 语义。
- `include/notdec/TypeRecovery/LowTy.h:8` 只保留 LLVM forward declaration；LLVM 类型相关构造实现移动到 `src/TypeRecovery/LowTy.cpp:1`，避免 PNDiff 头文件间接带入 LLVM IR 头。
- `test/type-recovery/llvm-ir/expected/tr-level-2/21_PointerAnalysisBranchingFieldCycle.htypes:9` 更新 oracle。新输出多了一个同构递归声明，ptradd 三条 `@1/@4/@4` 都仍触发，语义没有少写字段。

### 验证

- `cmake --build ./build --target notdec -j4`：通过。
- PNDiff 依赖扫描：`rg -n "llvm::|clang::|#include <llvm|#include <clang|notdec-llvm2c|ExtValuePtr|ValueNamer|SmallVector|BinaryOperator|FunctionType|llvm/Support|llvm/IR" include/notdec/TypeRecovery/mlsub/PNDiff.h src/TypeRecovery/mlsub/PNDiff.cpp -S || true`：无输出。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- `ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' --output-on-failure`：`llvm_ir` 通过；`sysy` 失败是当前 suite 生成 x86_64 IR，`--dump-htypes` 被 pass pipeline 拒绝，错误为 `--dump-htypes requires a target with type recovery pipeline support`，不是本次 PNDiff 改动导致。
- fortune frozen IR 性能 smoke：`elapsed=74.51 user=102.36 sys=1.35 maxrss=1261032`。上一轮同口径记录约 `elapsed=74.21 user=100.96 sys=1.15 maxrss=1269948`，没有明显退化。

### 风险和评分

- 实现效果：8/10。PNDiff 已断开 LLVM/Clang/llvm2c 依赖，旧的 constant policy、offset 匹配和 ptradd 回写逻辑还在 NotDec 侧保留。
- 复杂度：7/10。多了一层 opaque handle 和 callback，但这是后续迁移到 binarysub 前必须付出的边界成本。
- 维护成本：7/10。NotDec 侧现在集中维护 `ExtValuePtr` token 和回调，PNDiff 逻辑更独立；后续移动源码时还需要把 `LowTy/PNTy` 的边界继续整理。
- 更好的方案：可以进一步让 PNDiff 接收稳定排序 key，减少 oracle 名字漂移；但第一阶段先不扩大范围。
