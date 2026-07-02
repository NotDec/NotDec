# 原始 prompt

> 接下来是不是可以直接做第二阶段了？把 PNDiff 挪进 binarysub 子模块里
>
> LowTy整个是不是可以也挪过去，先断掉和LLVM的依赖，比如fromLLVMTy这种就挪到别的地方？规划一下，然后和我确认一下
>
> 按照这个推进吧

# 背景

第一阶段已经让 PNDiff 不再直接 include LLVM、Clang 和 llvm2c。它现在通过 opaque handle 和 callback 访问 NotDec 侧对象。

第二阶段要把 PNDiff 挪进 `external/binarysub`。直接移动还不够，因为 PNDiff 依赖 `PNTy / PtrOrNum`，这些类型现在在 NotDec 的 `LowTy` 里。`LowTy` 里又混着 LLVM 类型转换函数，如果整体原样迁移，会把 LLVM 依赖带进 binarysub。

# 目标

把纯 PN 类型和 PNDiff 算法迁入 binarysub，同时保持 LLVM 相关逻辑在 NotDec 侧：

- binarysub 拥有 `PtrOrNum / PNTy` 和 PNDiff 算法。
- binarysub 不能依赖 LLVM、Clang、llvm2c 或 NotDec 主仓库头文件。
- NotDec 侧继续负责 LLVM `Type` 到 `PNTy` 的转换。
- policy 文件解析、LLVM constant 解析、offset 匹配、ptradd 回写继续在 NotDec 侧。

# 路线

1. 在 binarysub 新增纯 `LowTy`，只包含 PN lattice 本身。
2. 把 PNDiff 迁入 binarysub，并改为 include binarysub 的 `LowTy`。
3. NotDec 侧保留 LLVM adapter，把 `fromLLVMTy`、`llvmType2Elem`、`Elem2LLVMType`、`getSize` 等 LLVM 绑定函数留在主仓库。
4. `MLsubGenerator` 继续通过 adapter 构造 `PNTy`，其余 PNDiff callback 不改语义。
5. 修改 CMake：binarysub 编译新增源码，NotDec 不再编译旧 PNDiff 源码。
6. 先提交 binarysub，再提交顶层 submodule 指针、NotDec 适配和本日志。

# 风险

- `PNTy` namespace 变动可能影响已有 include 和 using。
- `LowTy` 拆分时容易误删 `meet/join` 这类非 LLVM 逻辑。
- PNDiff 搬到子模块后，不能再 include `Utils/DSUMap.h`，需要在 binarysub 内提供等价小工具或迁移依赖。
- 结果差异如果只来自节点编号或遍历顺序，优先确认不是语义变化，再决定是否更新 oracle。

# 判断标准

- `external/binarysub` 内新 PNDiff/LowTy 不出现 LLVM、Clang、llvm2c、NotDec 主仓库 include。
- `notdec` 和 `binarysub` 能构建。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune frozen IR 同口径性能 smoke 无明显退化。

# 实现记录

## 已完成：迁移纯 PN 类型和 PNDiff

- `external/binarysub/include/binarysub/LowTy.h:1` 新增纯 PN lattice 头文件，保留 `PtrOrNum`、`PNTy`、`join/meet`、`fromIPChar` 等不依赖 LLVM 的逻辑。
- `external/binarysub/src/LowTy.cpp:1` 新增纯 PN lattice 实现，删除原 `LowTy.cpp` 里的 LLVM `Type` 解析代码。
- `external/binarysub/include/binarysub/PNDiff.h:1` 新增 PNDiff 头文件，改为 include `binarysub/LowTy.h`、`binarysub/Range.h` 和 `binarysub/DSUMap.h`。
- `external/binarysub/src/PNDiff.cpp:1` 新增 PNDiff 实现，逻辑沿用主仓库旧实现，继续通过 callback 处理 policy、constant、offset range 和 ptradd 回写。
- `external/binarysub/include/binarysub/DSUMap.h:1` 迁入 PNDiff 需要的 DSUMap 小工具，避免 binarysub 反向 include NotDec 主仓库工具头。
- `external/binarysub/CMakeLists.txt:15` 把 `src/LowTy.cpp` 和 `src/PNDiff.cpp` 编进 `binarysub_lib`。

## 已完成：NotDec 侧保留 LLVM adapter

- `include/notdec/TypeRecovery/LowTy.h:8` 改为 include `binarysub/LowTy.h`，只额外声明 LLVM 相关 adapter。
- `src/TypeRecovery/LowTy.cpp:13` 保留 `getSize(llvm::Type *)`、`fromLLVMTy()`、`isPtrOrNum()`、`Elem2LLVMType()`、`llvmType2Elem()`。
- `src/TypeRecovery/LowTy.cpp:117` 新增 `makePNTyFromLLVMType()`，先解析 LLVM 类型，再构造 binarysub 的 `PNTy`；这里修过一次 `NotPN` 构造顺序，避免空 `Elem` 触发原断言。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:42` 改为 include `binarysub/PNDiff.h`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:17` 显式 include NotDec 侧 `LowTy` adapter。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2599` 把 `PNTy(getType(...))` 改为 `retypd::makePNTyFromLLVMType(...)`。
- `src/CMakeLists.txt:28` 删除主仓库旧 `TypeRecovery/mlsub/PNDiff.cpp` 编译入口。
- 删除旧 `include/notdec/TypeRecovery/mlsub/PNDiff.h` 和 `src/TypeRecovery/mlsub/PNDiff.cpp`。

## 验证

- 构建通过：
  - `cmake --build ./build --target notdec binarysub -j4`
- 依赖扫描通过：
  - 新 `external/binarysub/include/binarysub/LowTy.h`
  - 新 `external/binarysub/src/LowTy.cpp`
  - 新 `external/binarysub/include/binarysub/PNDiff.h`
  - 新 `external/binarysub/src/PNDiff.cpp`
  - 新 `external/binarysub/include/binarysub/DSUMap.h`
  - 扫描项包括 `llvm::`、`clang::`、`#include <llvm`、`#include <clang`、`notdec-llvm2c`、`ExtValuePtr`、`ValueNamer`、`notdec/TypeRecovery`、`TypeRecovery/`，无输出。
- 回归通过：
  - `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`
- 性能 smoke：
  - 按用户要求跳过，未运行 fortune frozen IR。

## 评分

- 实现效果：8/10。PNDiff 和纯 LowTy 已进入 binarysub，LLVM 解析留在 NotDec 侧，回归通过。
- 复杂度：6/10。迁移后边界更清楚，但暂时保留 `notdec::retypd` / `notdec::mlsub` 命名空间，避免一次性扩大改动。
- 维护成本：5/10。后续如果要让 binarysub 更独立，可以再统一 namespace，并把 DSUMap 放到更通用的 binarysub utils 位置。
