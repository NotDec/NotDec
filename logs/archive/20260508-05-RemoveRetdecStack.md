# 2026-05-08 删除主项目 retdec-stack 路径

## 背景

主项目里还保留了一套 `retdec-stack` 栈恢复实现，但当前默认路径早就是
`notdec`。这套代码一方面已经不再被实际选用，另一方面在 LLVM 22 迁移里继续制造
额外编译面，尤其是 `retdec-ir-modifier.cpp`、`retdec-stack.cpp`、
`retdec-symbolic-tree.cpp` 这几处。

这次不再继续兼容它，直接从主链路彻底删除。

## 目标

1. 删掉主项目里对 `retdec-stack` 的编译、include、CLI、配置入口。
2. 删掉 `src/Passes/retdec-stack/` 和 `include/notdec/Passes/retdec-stack/`
   下全部源码。
3. 保证删除后构建失败面不再包含 `retdec`，只剩现有 LLVM 22 迁移问题。
4. 这批删除单独提交一个 commit。

## 实现

### 1. 摘掉主链路入口

1. `src/CMakeLists.txt:2-22`
   - 从 `notdec` 静态库源码列表里删掉
     `Passes/retdec-stack/*.cpp` 7 个编译单元。
   - 影响 target：`notdec`。

2. `src/Passes/PassManager.cpp:42-49`
   - 删掉 `retdec-stack` 相关 4 个头文件 include。
   - 这说明 pass pipeline 已经不再依赖这套实现。

3. `src/NotDec.cpp:74-77,254-259`
   - 删掉 CLI 选项 `--stack-recovery-algo`。
   - 构造 `notdec::Options` 时不再写入 `.stackRec`。
   - 涉及函数：`main`。

4. `include/notdec/DecompilerContext.h:14-20`
   - 从 `Options` 里删掉 `stackRec` 字段。
   - 涉及类型：`notdec::Options`。

### 2. 删除 retdec-stack 源码

下面这些文件都是整文件删除：

1. `include/notdec/Passes/retdec-stack/retdec-abi.h` 原 `1-72` 行
2. `include/notdec/Passes/retdec-stack/retdec-ir-modifier.h` 原 `1-66` 行
3. `include/notdec/Passes/retdec-stack/retdec-reaching-definition.h` 原 `1-175` 行
4. `include/notdec/Passes/retdec-stack/retdec-stack-pointer-op-remove.h` 原 `1-41` 行
5. `include/notdec/Passes/retdec-stack/retdec-stack.h` 原 `1-56` 行
6. `include/notdec/Passes/retdec-stack/retdec-symbolic-tree-match.h` 原 `1-1242` 行
7. `include/notdec/Passes/retdec-stack/retdec-symbolic-tree.h` 原 `1-200` 行
8. `include/notdec/Passes/retdec-stack/retdec-utils.h` 原 `1-28` 行
9. `src/Passes/retdec-stack/retdec-abi.cpp` 原 `1-71` 行
10. `src/Passes/retdec-stack/retdec-ir-modifier.cpp` 原 `1-355` 行
11. `src/Passes/retdec-stack/retdec-reaching-definition.cpp` 原 `1-759` 行
12. `src/Passes/retdec-stack/retdec-stack-pointer-op-remove.cpp` 原 `1-198` 行
13. `src/Passes/retdec-stack/retdec-stack.cpp` 原 `1-287` 行
14. `src/Passes/retdec-stack/retdec-symbolic-tree.cpp` 原 `1-824` 行
15. `src/Passes/retdec-stack/retdec-utils.cpp` 原 `1-24` 行

涉及范围：

- `retdec` 命名空间下这套栈恢复实现的全部类、函数、辅助匹配器和工具函数。
- 其中包括 ABI 适配、IR modifier、reaching definition、symbolic tree、
  stack pass、本地工具函数等整套实现。

说明：

- `docs/src/retdec.md` 和 `docs/src/SUMMARY.md` 这次没动。它们现在只是外部资料笔记，
  不参与主项目构建，也不是 CLI/源码入口。

## 验证

### 配置

```bash
cmake -S . -B ./build
cmake -S . -B ./build-main-no-llvm2c -DNOTDEC_ENABLE_LLVM2C=OFF
```

结果：两套构建目录都能重新生成成功。

### 构建图检查

```bash
rg -n "retdec-stack" build/build.ninja build-main-no-llvm2c/build.ninja
```

结果：没有命中，说明 `retdec-stack` 源文件已经完全退出构建图。

### 编译

```bash
cmake --build ./build-main-no-llvm2c --target notdec -j4
cmake --build ./build --target notdec -j4
```

结果：

- 两边都不再出现 `retdec-stack` 编译错误。
- 当前失败面只剩已有的 LLVM 22 迁移问题，主要集中在
  `src/Passes/DSROA.cpp`。

## 性能

这次是删死代码和删旧入口，没有涉及类型恢复、pointer analysis 或 pass 执行逻辑的
性能对比。当前还没恢复到可运行完整主流程，所以没有 fortune 时间数据。

## 当前评价

实现效果：9/10

- 旧入口、旧配置、旧源码一起删掉了，边界清楚。
- 构建面明显收敛，后续可以专心处理 `DSROA.cpp` 和 `llvm2c` 的 LLVM 22 迁移。

复杂度：9/10

- 方案很直接，没有留下“默认不用但还一直编”的半残状态。
- 唯一保留的是 docs 里的 retdec 资料笔记，但这部分不参与代码路径。

维护成本：9/10

- 后面少维护一整套不再使用的栈恢复实现。
- 也少了一批 LLVM 版本迁移噪音。

有没有更好的方案：

- 有一个更保守的方案，只把它从构建里摘掉、不删源码。
- 但那会继续留下无用 CLI/配置和一堆没人维护的旧文件，后面还是会反复干扰判断。
- 既然已经确认要放弃这条路径，直接删干净更合适。
