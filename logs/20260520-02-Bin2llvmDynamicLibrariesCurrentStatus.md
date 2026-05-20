# 2026-05-20 Bin2LLVM 动态库当前状态

## 用户原始 prompt

```text
logs/20260519-04-Bin2llvmDynamicLibraryExportScript.md 阅读这个计划，然后写一个新的logs/文件总结一下当前关注的这几个二进制库，其中哪些是能够找到动态链接库的，没找到的也可以再试一下找找。然后再总结一下，这些库目前转IR的情况，是否都能够正常转出来，有问题的话，修复其中比较简单的问题
```

## 输入和范围

这次先按 `/sn640/NotDec-Exp/Bench2/manifest/benchmark-targets.tsv` 里 `file_type` 含 `shared object` 的目标统计。

旧的动态库导出目录是 `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/`。它目前只覆盖了前一批 7 个目标，后面 manifest 新增的 ffmpeg 和 PHP 扩展还没有跑到这个目录。

## 动态库是否能找到

manifest 里的 16 个共享对象都能在 rootfs 里找到，没有缺文件：

| project | role | rootfs 路径 | 状态 |
|---|---|---|---|
| libuv | shared-library | `/usr/lib/x86_64-linux-gnu/libuv.so.1.0.0` | found |
| wolfssl | shared-library | `/usr/lib/x86_64-linux-gnu/libwolfssl.so.42.0.0` | found |
| libicu | data-library | `/usr/lib/x86_64-linux-gnu/libicudata.so.74.2` | found |
| libicu | i18n-library | `/usr/lib/x86_64-linux-gnu/libicui18n.so.74.2` | found |
| libicu | common-library | `/usr/lib/x86_64-linux-gnu/libicuuc.so.74.2` | found |
| python | shared-library | `/usr/lib/x86_64-linux-gnu/libpython3.12.so.1.0` | found |
| python | debug-shared-library | `/usr/lib/x86_64-linux-gnu/libpython3.12d.so.1.0` | found |
| ffmpeg | codec-library | `/usr/lib/x86_64-linux-gnu/libavcodec.so.60.31.102` | found |
| ffmpeg | format-library | `/usr/lib/x86_64-linux-gnu/libavformat.so.60.16.100` | found |
| ffmpeg | util-library | `/usr/lib/x86_64-linux-gnu/libavutil.so.58.29.100` | found |
| ffmpeg | filter-library | `/usr/lib/x86_64-linux-gnu/libavfilter.so.9.12.100` | found |
| ffmpeg | scale-library | `/usr/lib/x86_64-linux-gnu/libswscale.so.7.5.100` | found |
| ffmpeg | resample-library | `/usr/lib/x86_64-linux-gnu/libswresample.so.4.12.100` | found |
| php | extension-calendar | `/usr/lib/php/20230831/calendar.so` | found |
| php | extension-ffi | `/usr/lib/php/20230831/ffi.so` | found |
| php | extension-sockets | `/usr/lib/php/20230831/sockets.so` | found |

## 已有转 IR 状态

当前 `dynamic-libs/` 已有结果：

| 目标 | 状态 | 备注 |
|---|---|---|
| `libuv/shared-library` | complete | `module-all.ll`、`module-all.bc` 都存在；478/478 个函数 lower 成功。 |
| `libicu/data-library` | complete | `module-all.ll`、`module-all.bc` 都存在；5/5 个函数 lower 成功。 |
| `wolfssl/shared-library` | partial-ir | 本次修复后可生成 `.ll` 和 `.bc`；4037/4057 个函数 lower 成功，20 个函数因分支目标歧义保留声明。 |
| `libicu/common-library` | json-only | Ghidra 导出成功，4517/4517；checker 之前被大量重复函数名挡住，未继续 lower。 |
| `libicu/i18n-library` | json-only | Ghidra 导出成功，10748/10748；checker 之前被大量重复函数名挡住，未继续 lower。 |
| `python/shared-library` | json-only | Ghidra 导出成功，6966/6966；旧 checker 因 `default`、`caseD_6` 重名失败。JSON 约 1.7GB，后续需要单独跑。 |
| `python/debug-shared-library` | ghidra-log-only | 只有 Ghidra 日志，没有 JSON；这次未重跑。 |
| ffmpeg 6 个库 | not-run | 文件都找得到，但当前 `dynamic-libs/` 里还没有导出结果。 |
| PHP 3 个扩展 | not-run | 文件都找得到，但当前 `dynamic-libs/` 里还没有导出结果。 |

`/sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/summary.tsv` 已把 wolfssl 改成 `partial-ir`。

## 修复

这次只修两个小问题，没有改导出 schema。

1. [`external/NotDec-bin2llvm/tools/notdec-heritage-module-check.cpp`](/sn640/NotDec/external/NotDec-bin2llvm/tools/notdec-heritage-module-check.cpp:12)
   - 第 12-18 行：`CheckState` 增加 `DuplicateFunctionNameCount`。
   - 第 85-88 行：重复函数名不再作为硬错误，只计数。
   - 第 131-136 行：summary 打印重复函数名数量。
   - 原因：模块 lowering 已经按 entry 生成唯一 LLVM 符号，真实库里短函数名重复不应挡住 IR 生成；重复 entry 仍然是错误。
2. [`external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`](/sn640/NotDec/external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:2095)
   - 第 2095-2104 行：失败函数回退时保留原 `llvm::Function`，只 `deleteBody()`，不再 `eraseFromParent()`。
   - 原因：前面已经 lower 出来的函数可能引用了这个函数；删除符号会让 LLVM 在销毁 still-used value 时崩溃。

## 验证

编译：

```bash
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-module-llvm notdec-heritage-module-check -j4
```

wolfssl checker：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-check \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/wolfssl/shared-library/module-all.json
```

结果：

- functions: 4057
- externals: 146
- duplicate function names: 9
- unknown calls: 0
- status: ok

wolfssl lower：

```bash
/usr/bin/time -p /tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/wolfssl/shared-library/module-all.json \
  -o /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/wolfssl/shared-library/module-all.ll
```

结果：

- lowered function bodies: 4037
- failed function bodies: 20
- real 203.12s

`llvm-as`：

```bash
/usr/bin/time -p /sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/wolfssl/shared-library/module-all.ll \
  -o /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/wolfssl/shared-library/module-all.bc
```

结果：

- 通过
- real 39.71s

## 判断

当前动态库路径问题不是主要阻塞；manifest 里的共享对象都能找到。

真正阻塞分两类：

1. 已导出 JSON 但没继续 lower：主要是旧 checker 把重复函数名当硬错误。这个已修，后续可以重跑 libicu common/i18n 和 python shared。
2. lower 后仍有少量失败函数：wolfssl 现在能产出合法模块 IR，但 20 个函数因为分支目标歧义保留声明。这不是这次的小修范围。

评分：

- 实现效果：7/10。wolfssl 从 `json-only` 前进到可汇编的 `partial-ir`。
- 复杂度：3/10。只改 checker 的错误条件和失败回退方式。
- 维护成本：3/10。逻辑和现有模块符号规划一致，后续主要继续处理分支目标歧义。
