# 2026-05-19 bin2llvm 动态库批量导出脚本与重跑记录

## 用户原始 prompt

```text
有动态链接库的尝试运行一下导出，转为IR，放到一个文件夹里。写一个脚本处理这个，方便后续重新导出
当前做的事情写到一个单独的logs/文件里面
```

## 背景

前面几轮 Bin2LLVM 主要在独立二进制上碰问题。后来把分析方向改成先看动态库，发现这条链路更稳，也更适合先把整个模块导出来再看。

这次要做的不是再分析一个单点样本，而是把“动态库优先”的跑法固定下来，方便以后重复导出和对比。

## 目标

1. 从 Bench2 现有清单里自动挑出动态库目标。
2. 把这些目标统一导出到一个单独目录。
3. 每个目标都保留 JSON、LL、BC 和日志，方便后续复跑和排查。
4. 把这条跑法写进 Bench2 的 plan 里，后面不再靠口头记忆。

## 方案

这次只做最小的一层包装，不改 Bin2LLVM 的核心转换逻辑。

核心思路是：

1. 读 `Bench2/manifest/benchmark-targets.tsv`。
2. 只保留 `file_type` 里带 `shared object` 的目标。
3. 把 manifest 里的 `rootfs_path` 统一映射到 `Bench2/rootfs/` 下的本地文件。
4. 对每个目标依次跑：
   - Ghidra headless 导出模块 JSON
   - `notdec-heritage-module-check`
   - `notdec-heritage-module-llvm`
   - `llvm-as`
5. 输出统一放到 `Bench2/bin2llvm-ir/dynamic-libs/`。

## 落地

### 新脚本

新增了 [`Bench2/scripts/export-bin2llvm-dynamic-libs.py`](/sn640/NotDec-Exp/Bench2/scripts/export-bin2llvm-dynamic-libs.py#L1)

主要内容在：

1. `DEFAULT_*` 路径常量，固定了 manifest、rootfs、Ghidra、native tool、LLVM 位置。
2. `choose_targets(...)`，只筛 `shared object`。
3. `run_cmd(...)`，把每一步命令和 stdout/stderr 写进独立日志。
4. `main(...)`，按 `project/role` 建目录，逐个导出、校验、lower、`llvm-as`。

这个脚本做了两点比较关键的处理：

1. `rootfs_path` 不是直接拿来跑，而是先映射到 `Bench2/rootfs/...`。
2. 每个目标都单独落目录，不混在一起，后面重跑时好定位。

### 相关文件

这次相关的文件和目录主要有：

1. [`Bench2/manifest/benchmark-targets.tsv`](/sn640/NotDec-Exp/Bench2/manifest/benchmark-targets.tsv)：
   作为目标清单，脚本就是从这里筛 `shared object`。
2. [`Bench2/rootfs/`](/sn640/NotDec-Exp/Bench2/rootfs)：
   真实输入都从这里取，不直接用 manifest 里的原始路径。
3. [`Bench2/bin2llvm-ir/dynamic-libs/`](/sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs)：
   动态库导出的统一输出目录。
4. [`Bench2/bin2llvm-ghidra-projects/dynamic-libs/`](/sn640/NotDec-Exp/Bench2/bin2llvm-ghidra-projects/dynamic-libs)：
   Ghidra headless 的工程目录。
5. [`external/NotDec-bin2llvm/ghidra_scripts/ExportHeritageModule.java`](/sn640/NotDec/external/NotDec-bin2llvm/ghidra_scripts/ExportHeritageModule.java#L1)：
   负责把 Ghidra 里的 `HighFunction` 导成模块 JSON。
6. [`external/NotDec-bin2llvm/DEBUG.md`](/sn640/NotDec/external/NotDec-bin2llvm/DEBUG.md#L1)：
   记录了这台机器上手跑时的命令骨架。
7. [`Bench2/plan.md`](/sn640/NotDec-Exp/Bench2/plan.md#L7)：
   记录这次分析方向改成优先动态库。

### 当前跑法

现在这条链路就是按下面这套跑：

```bash
python3 /sn640/NotDec-Exp/Bench2/scripts/export-bin2llvm-dynamic-libs.py --all --timeout=120
```

脚本内部实际做的事是：

1. 读取 `Bench2/manifest/benchmark-targets.tsv`。
2. 找出所有 `file_type` 含 `shared object` 的条目。
3. 把 `/usr/lib/...` 这类路径映射到 `/sn640/NotDec-Exp/Bench2/rootfs/usr/lib/...`。
4. 用 `analyzeHeadless` 跑 `ExportHeritageModule.java`。
5. 再跑 `notdec-heritage-module-check`。
6. 再跑 `notdec-heritage-module-llvm`。
7. 最后用 `llvm-as`。

每个目标的输出都放在一个独立目录里，目录名是：

```text
Bench2/bin2llvm-ir/dynamic-libs/<project>/<role>/
```

目录里保留：

1. `module-all.json`
2. `module-all.ll`
3. `module-all.bc`
4. `module-all.ghidra.log`
5. `module-all.check.log`
6. `module-all.lower.log`
7. `module-all.llvm-as.log`
8. `target.txt`

### 计划说明

在 [`Bench2/plan.md`](/sn640/NotDec-Exp/Bench2/plan.md#L7) 开头补了一句：

1. 后续 Bin2LLVM 优先看动态库。
2. 重跑时直接用这个脚本，把结果写到 `Bench2/bin2llvm-ir/dynamic-libs/`。

## 结果

这次已经跑出一批结果，目录结构已经落到 `Bench2/bin2llvm-ir/dynamic-libs/`。

### 已完整跑通

1. `libuv` 的 `shared-library`：JSON、LL、BC 都有，`llvm-as` 通过。
2. `libicu` 的 `data-library`：JSON、LL、BC 都有，`llvm-as` 通过。

### 已导出 JSON，但还没完整收尾

1. `libicu` 的 `i18n-library`
2. `libicu` 的 `common-library`
3. `python` 的 `shared-library`
4. `python` 的 `debug-shared-library`
5. `wolfssl` 的 `shared-library`

这些目标里，有的是我中途停掉了批次，有的是后续 `check` 阶段还没完全收尾。

### 已看到的问题

1. `wolfssl` 在 `notdec-heritage-module-check` 阶段报了重名：
   - `duplicate function name: default`
   - `duplicate function name: caseD_0`
2. `python` 的 Ghidra 导出是成功的，但它本身太大，后续 lower/check 会比较慢。

## 判断

这次的主要收获不是某个单点样本，而是把动态库批量导出这条路固定住了。

目前看：

1. `libuv`、`libicu data` 这类库已经能稳定跑成完整模块。
2. 大库可以继续跑，但应该拆成单独批次，不要和别的样本混在一起。
3. `wolfssl` 的问题不是导出链路断了，而是模块里重名函数先挡住了后续检查。

## 后续

后面如果继续跑，建议直接从这个脚本出发：

1. 先跑小一点、已经验证过的共享库。
2. 再单独处理 `python`、`ffmpeg` 这种大库。
3. `wolfssl` 先看是不是要单独做去重或跳过规则，再决定是否继续收尾。

## 2026-05-19 追加：修复模块级寄存器全局变量重复

### 用户追加 prompt

```text
/sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/libuv/shared-library/module-all.ll这里开头，各个寄存器怎么有这么多重名的全局变量。比如R13，应该只有一个。其次，拆分方面也不是很对，比如怎么有R13D这种全局变量，应该根据那个分割原则，单独分割出R13（如果已经有了就复用），然后访问R13的一部分。下面是寄存器分割原则，尝试放到bin2llvm项目的核心文档里面，比如AGENTS.md或者README.md，看看哪里合适。然后尝试修复这个问题

寄存器内存段的拆分一定要按照这种方式：按照必定不可能被重叠访问的边界去切分，即不可能会有这样的访问范围，使得该边界在该访问内部。比如RAX可以被EAX，AX，AL等方式访问一部分，那么就按最大的RAX单独划分出来，作为全局变量。更小的访问就基于这个大的全局变量做切割再访问。(另外，访问EAX时会清空RAX高位这种语义已经被PCode考虑了，不需要再调研看是否考虑到)
```

### 问题原因

`HeritageLowerer` 原来每个函数单独创建一份 `RegisterStorage`。模块级输出时，不同函数第一次访问同一个寄存器都会尝试建 global，LLVM 只能改名成 `@R13.1`、`@ZF.2` 这类名字。

另一个问题是：如果某个函数里只出现 `R13D`，函数级 `RegisterStorage` 只看得到 4 字节访问，就会建 `@R13D`，没有把它归到 8 字节的 `@R13`。

### 实现

1. [`external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`](/sn640/NotDec/external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:40)
   增加 x86-64 子寄存器到最大承载寄存器的表，覆盖 `EAX/AX/AL/AH`、`R8D/R8W/R8B` 到 `R15D/R15W/R15B` 等常见别名。
2. [`external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`](/sn640/NotDec/external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:217)
   `registerInfosForHeritageProgram(...)` 在 x86-64 输入里保留原 varnode，同时额外补入最大承载寄存器。例如 `R13D` 会补一个 `R13`、size 8 的 `RegisterInfo`。这里只影响 LLVM global 的承载对象，不重复模拟 EAX 清高位语义。
3. [`external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`](/sn640/NotDec/external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:253)
   新增 `registerInfosForHeritageModule(...)`，从整个 module 的所有函数收集 register varnode。
4. [`external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`](/sn640/NotDec/external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:306)
   `HeritageLowerer` 改为可接收外部共享 `RegisterStorage`；单函数路径仍然自己持有一份，不改变旧入口行为。
5. [`external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`](/sn640/NotDec/external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:2009)
   `buildHeritageModuleWithBodies(...)` 在模块级创建一份共享 `RegisterStorage`，所有函数 lower 时复用它。
6. [`external/NotDec-bin2llvm/ARCHITECTURE.md`](/sn640/NotDec/external/NotDec-bin2llvm/ARCHITECTURE.md:238)
   增加“寄存器”小节，记录模块级共享、按不被重叠访问跨过的边界切分、子寄存器访问基于最大寄存器切片这几个规则。

### 验证

编译：

```bash
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-module-llvm -j4
```

重跑 libuv：

```bash
/usr/bin/time -p /tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/libuv/shared-library/module-all.json \
  -o /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/libuv/shared-library/module-all.ll
```

结果：

- lowered function bodies: 478
- failed function bodies: 0
- real 14.51s

再跑：

```bash
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/libuv/shared-library/module-all.ll \
  -o /sn640/NotDec-Exp/Bench2/bin2llvm-ir/dynamic-libs/libuv/shared-library/module-all.bc
```

通过。

检查 `module-all.ll` 开头，现在只有一份：

```llvm
@R13 = external global i64, !notdec.register !10
```

没有再出现 `@R13.1`、`@R13D`、`@R14D`、`@R15D`、`@R12D`、`@EAX` 这类 register global。

### 评分

- 实现效果：8/10。libuv 这个当前样本已经修掉重复寄存器 global 和 32 位子寄存器 global。
- 复杂度：6/10。新增了 x86-64 alias 表，有一点维护成本，但位置集中，逻辑清楚。
- 后期维护成本：6/10。后续如果支持更多架构，需要为对应架构补 alias 规则；通用 `RegisterStorage` 不需要改。

更好的方案是让 Ghidra 导出 register bank 的完整定义，而不是在 C++ 里写 x86-64 alias 表。当前先用小范围规则修复真实样本，避免扩大 Ghidra 导出 schema。
