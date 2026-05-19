# 2026-05-19 gtirb2llvm 子模块和 GTIRB 转 LLVM IR 调研

## 原始 prompt

尝试基于https://github.com/GrammaTech/gtirb项目，创建一个项目到external/gtirb2llvm项目，注册为主项目的git submodule，然后里面也不要是detached head模式，而是main branch。远程地址是 git@github.com:am009/gtirb2llvm.git 这个新项目把gtirb （https://github.com/GrammaTech/gtirb.git） 作为git子模块管理。
先clone下来，然后调研一下如何基于gtirb做二进制到LLVM IR的转换。

## 背景和目标

目标是先把 `gtirb2llvm` 项目放进 NotDec 的 `external/`，并把上游 GTIRB 作为它自己的子模块。当前只做项目骨架和路线调研，不实现二进制 lifter。

## 实现记录

1. 顶层 `.gitmodules:18-21`
   - 新增 `external/gtirb2llvm` 子模块。
   - 远程为 `git@github.com:am009/gtirb2llvm.git`。
   - 跟踪分支为 `main`。

2. `external/gtirb2llvm/.gitmodules:1-4`
   - 新增内部子模块 `external/gtirb`。
   - 远程为 `https://github.com/GrammaTech/gtirb.git`。
   - 跟踪分支为 `master`，因为上游 GTIRB 当前默认分支是 `master`。

3. `external/gtirb2llvm/README.md:1-21`
   - 说明当前项目定位：GTIRB 负责二进制结构事实，LLVM IR lowering 仍需要指令语义来源。

4. `external/gtirb2llvm/CMakeLists.txt:1-13`
   - 加最小 CMake 项目。
   - 默认不构建 GTIRB，避免一开始引入重依赖。
   - 通过 `GTIRB2LLVM_BUILD_GTIRB=ON` 可显式进入 GTIRB 子模块。

5. `external/gtirb2llvm/docs/gtirb-to-llvm-ir.md:1-97`
   - 记录调研结论：GTIRB 不是完整语义 lifter。
   - 建议第一阶段先做 `.gtirb` dump，验证 module/function/block/symbol/CFG/AuxData 是否够用。
   - 第二阶段再接 NotDec-bin2llvm 现有 SLEIGH/P-Code lowering。

## 验证

```bash
cmake -S external/gtirb2llvm -B /tmp/gtirb2llvm-build -G Ninja
cmake --build /tmp/gtirb2llvm-build
git -C external/gtirb2llvm branch --show-current
git -C external/gtirb2llvm submodule status
```

结果：

- CMake 配置通过。
- 默认构建没有目标，`ninja: no work to do.`。
- `external/gtirb2llvm` 当前在 `main` 分支。
- 内部 `external/gtirb` 子模块指向 `e0869b51cd0fa1eb80e3af66da5e0723410a0379`。

## 性能影响

这次只新增独立子模块和文档，没有接 NotDec 主 pass pipeline，没有影响类型恢复、结构体合并或 pointer analysis，所以不跑 `fortune.o3.wasm.ll` 同口径性能。

## 判断

下一步不要直接写 GTIRB 到 LLVM IR 的完整 lowering。先做一个只读 dump 工具，把真实 `.gtirb` 里的函数、块、CFG、符号和 AuxData 打出来，再判断 GTIRB 信息和当前 Bench2 需求是否对得上。
