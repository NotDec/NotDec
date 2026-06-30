# LLVM2C / BinarySub CMake 依赖预铺

本次用户要求：

> 继续推进重构，当前在LLVM2C这个项目里面，有HType类型。另外在主项目里面有负责u type转h type的相关的代码。我现在在考虑尝试能不能把这两部分给挪到binary sub这个仓库里面。这样的话，有一点就是 LLVM2C 这个项目它也要依赖于 BinarySub 项目。能不能让主项目和 LLVM2C 这个项目同时依赖BinarySub ，但是把 binary sub 依然放在当前的路径，不移动。
>
> 同时能保证LLVM2C项目它可以单独编译吗，比如直接要求指定binarysub路径或者默认放到相邻文件夹，像当前external里面这样，这样就同时支持主项目编译和里面编译

## 背景

后续要把 HType 和 UType -> HType 转换逻辑迁到 `external/binarysub`。这一步先只铺 CMake 依赖，不移动源码：主项目和 `external/NotDec-llvm2c` 都能依赖同一个 `binarysub_lib`，并保持 `external/binarysub` 路径不变。

## 改动

- `CMakeLists.txt:206`：顶层先设置 `BINARYSUB_ENABLE_TBB_PARALLEL`，再 `add_subdirectory(external/binarysub)`，保证后面的 `external/NotDec-llvm2c` 可以看到 `binarysub_lib` target。
- `external/NotDec-llvm2c/CMakeLists.txt:17`：新增 `NOTDEC_LLVM2C_BINARYSUB_DIR` cache path。
- `external/NotDec-llvm2c/CMakeLists.txt:21`：如果上层已经有 `binarysub_lib`，直接复用；如果没有，就优先找相邻 `../binarysub`，否则要求用户显式传 `-DNOTDEC_LLVM2C_BINARYSUB_DIR=/path/to/binarysub`。
- `external/NotDec-llvm2c/lib/Core/CMakeLists.txt:21`：`notdec-backend-core` 在 `binarysub_lib` 存在时 public link 它，为后面 HType / TypeBuilder 迁移留出 include/link 边界。

## 验证

- 顶层构建：
  `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -S . -B ./build -G Ninja && cmake --build ./build --target notdec -j4`：通过。
- LLVM2C 单独构建，显式 binarysub 路径：
  `cmake -S external/NotDec-llvm2c -B /tmp/notdec-llvm2c-standalone-build -G Ninja -DNOTDEC_LLVM_INSTALL_DIR=/sn640/NotDec/llvm-22.1.0.obj -DNOTDEC_LLVM2C_BINARYSUB_DIR=/sn640/NotDec/external/binarysub -DNOTDEC_LLVM2C_ENABLE_TESTS=OFF && cmake --build /tmp/notdec-llvm2c-standalone-build --target notdec-backend-core -j4`：通过。
- LLVM2C 单独构建，默认相邻 `../binarysub`：
  `cmake -S external/NotDec-llvm2c -B /tmp/notdec-llvm2c-standalone-sibling-build -G Ninja -DNOTDEC_LLVM_INSTALL_DIR=/sn640/NotDec/llvm-22.1.0.obj -DNOTDEC_LLVM2C_ENABLE_TESTS=OFF && cmake --build /tmp/notdec-llvm2c-standalone-sibling-build --target notdec-backend-core -j4`：通过。

## 评价

- 实现效果：8/10。依赖路径已经支持顶层复用和 llvm2c standalone 两种模式。
- 复杂度：4/10。只是 CMake target 顺序和 fallback 路径，没有源码行为变化。
- 维护成本：4/10。后续如果 binarysub target 改名，只需要同步这一个 target 名。
