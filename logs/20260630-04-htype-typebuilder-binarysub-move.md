# HType 和 TypeBuilder 迁到 BinarySub

本次用户要求：

> 依赖关系变了的话，代码是不是还没挪过去，是不是可以继续推进，把这两块代码挪过去啊

## 背景

上一轮已经把依赖关系铺好：主项目先加载 `external/binarysub`，`external/NotDec-llvm2c` 可以复用上层 `binarysub_lib`，单独编译时也能指定 BinarySub 路径或默认找相邻 `../binarysub`。

这次继续把 HType 和 UType -> HType 的转换实现移到 `external/binarysub`。目标是让这两块核心类型代码由 BinarySub 提供，主项目和 LLVM2C 都只依赖它，不再各自维护实现副本。

## 改动

- `external/binarysub/include/binarysub/HType.h:1`、`external/binarysub/src/HType.cpp:1`：新增 HType 类型系统实现，保留 `notdec::ast` 命名空间，先不扩大调用点改动。
- `external/binarysub/include/binarysub/Range.h:1`、`external/binarysub/src/Range.cpp:1`：新增 `OffsetRange` 等纯范围工具。
- `external/binarysub/include/binarysub/StructManager.h:1`、`external/binarysub/src/StructManager.cpp:1`：新增 `SimpleRange`、`BytesManager`、`TypeInfo` 等纯数据结构和非 LLVM 实现；BinarySub 公共头不再出现 LLVM/Clang 名字。
- `external/binarysub/include/binarysub/TypeBuilder.h:1`、`external/binarysub/src/TypeBuilder.cpp:1`：新增 UType -> HType 转换实现。`TypeBuilderContext` 在 `external/binarysub/include/binarysub/TypeBuilder.h:19` 增加可选 `WorkDir`，避免直接依赖主项目的 `getWorkDirOpt()`。
- `external/binarysub/include/binarysub/FreshName.h:1`、`external/binarysub/src/FreshName.cpp:1`：新增纯名字生成器。`TypeBuilder` 用它替代 llvm2c 的 `ValueNamer`，`ValueNamer` 也复用同一个计数器。
- `external/binarysub/CMakeLists.txt:16`：`binarysub_lib` 编译 HType、Range、StructManager、TypeBuilder、FreshName。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:1`、`Range.h:1`、`StructManager.h:1`：旧头文件改成兼容 wrapper，include `binarysub/...`。
- `external/NotDec-llvm2c/lib/Core/CMakeLists.txt:1`：`notdec-backend-core` 不再编译旧 HType 和 Range 实现，只保留 `StructManager.cpp` 里的 LLVM adapter。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/StructManager.h:4` 和 `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/StructManager.cpp:10`：只保留 `createBytesManager(llvm::Module&)` adapter，因为这部分仍然需要 LLVM IR。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ValueNamer.h:9`：`ValueNamer` 的普通 ID 生成改用 `FreshName`。
- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:1`：主项目旧 TypeBuilder 头改成 wrapper。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp`：删除旧实现副本。
- `src/CMakeLists.txt:28`：`notdec-core` 不再编译旧 TypeBuilder.cpp。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3173`：构造 `TypeBuilderContext` 时传入 `notdec::getWorkDirOpt()`，保留原 trace 日志位置。
- `include/notdec/TypeRecovery/mlsub/{PAPath.h,PNDiff.h,PointerAnalysis.h,Schema.h}` 和 `src/TypeRecovery/mlsub/{HTypeDebug.cpp,HTypeNormalize.cpp,MLsubGenerator.cpp,PNDiff.cpp}`：主项目内部改为直接 include `binarysub/HType.h` 或 `binarysub/Range.h`。

## 取舍

- 这次只做物理迁移和依赖方向调整，暂时保留 `notdec::ast`、`notdec::mlsub` 命名空间，避免一次性改大量调用点。
- `createBytesManager(llvm::Module&)` 没有迁进 BinarySub。它需要 LLVM IR 头和模块遍历，继续作为 LLVM2C 侧 adapter。
- 旧 llvm2c include 路径保留 wrapper，降低外部代码和已有 include 的破坏面。

## 验证

- `cmake --build ./build --target binarysub_lib -j4`：通过。
- `cmake --build ./build --target notdec -j4`：通过。
- `cmake --build ./build --target binarysub -j4 && ./build/binarysub`：通过。
- `cmake -S external/NotDec-llvm2c -B /tmp/notdec-llvm2c-standalone-explicit-build -G Ninja -DNOTDEC_LLVM_INSTALL_DIR=/sn640/NotDec/llvm-22.1.0.obj -DNOTDEC_LLVM2C_ENABLE_TESTS=OFF -DNOTDEC_LLVM2C_BINARYSUB_DIR=/sn640/NotDec/external/binarysub && cmake --build /tmp/notdec-llvm2c-standalone-explicit-build --target notdec-backend-core -j4`：通过。
- `cmake -S external/NotDec-llvm2c -B /tmp/notdec-llvm2c-standalone-sibling-build -G Ninja -DNOTDEC_LLVM_INSTALL_DIR=/sn640/NotDec/llvm-22.1.0.obj -DNOTDEC_LLVM2C_ENABLE_TESTS=OFF && cmake --build /tmp/notdec-llvm2c-standalone-sibling-build --target notdec-backend-core -j4`：通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：20/21 通过；`21_PointerAnalysisBranchingFieldCycle` 仍失败，diff 是已知递归 record 编号/拆分问题。
- `ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure`：NotDec 运行成功，DWARF oracle 仍在 `@File_list` 缺字段处失败，和迁移前已知状态一致。
- fortune frozen IR 性能 smoke：
  `env NOTDEC_EXTRA_CONSTRAINTS=test/type-recovery/realworld/support/fortune.o3.wasm.extra.json ./build/bin/notdec /sn640/NotDec/test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-htype-move/fortune.out.ll --tr-level=2 --frozen-tr-input-ir --dump-htypes /tmp/notdec-fortune-htype-move/fortune.htypes`，`/usr/bin/time` 记录 `elapsed=74.21 user=100.96 sys=1.15 maxrss=1269948`。没有看到性能退化。

## 评价

- 实现效果：8/10。HType 和 TypeBuilder 已经由 BinarySub 提供，主项目和 LLVM2C 都通过 BinarySub 使用。
- 复杂度：5/10。保留 wrapper 和原命名空间，避免了大范围调用点改名；新增的 `FreshName` 和 `WorkDir` 参数是为切断 llvm2c/主项目反向依赖必须做的小改动。
- 维护成本：5/10。短期有兼容 wrapper，后续可以逐步把剩余 include 统一到 `binarysub/...`，再考虑命名空间是否要调整。
