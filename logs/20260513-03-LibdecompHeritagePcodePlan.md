# 2026-05-13 libdecomp heritage P-Code 转 LLVM IR 计划

## 背景

`external/NotDec-bin2llvm` 现在已经有一条 native 闭环：

```text
hex bytes -> sleigh/libsla -> raw P-Code -> LLVM IR
```

这条路能快速验证 opcode lowering，但马上会碰到寄存器状态问题。已有
`20260513-02-Bin2llvmRegisterPlan.md` 计划用 register slot + `mem2reg`
自己处理跨基本块寄存器 SSA、重叠寄存器和 call clobber。

考虑敏捷开发，可以先试另一条路：让 Ghidra 的 libdecomp 先跑函数级分析，
尤其是 `Heritage`，拿到已经 SSA 化、带 `CPUI_MULTIEQUAL`、并且能标出函数
参数和返回值的 P-Code，再转 LLVM IR。

这个方向的核心收益是少做一轮寄存器 SSA 和 ABI 推断。当前先作为规划方案，
不替换已有 raw P-Code lowering。

## 目标

第一阶段目标不是做完整 C 反编译，而是拿到更适合 LLVM lowering 的函数级
P-Code。

期望效果：

1. 输入是真实函数，而不是单条指令或短字节串。
2. libdecomp 已经完成基本块、支配关系、heritage SSA。
3. 控制流汇合处以 `CPUI_MULTIEQUAL` 明确表示 PHI。
4. 函数入口参数能显式导出，尽量不再表现为裸寄存器读。
5. 函数返回值能显式导出，尽量不再表现为裸返回寄存器写。
6. LLVM lowering 优先消费“SSA P-Code + 函数签名”，而不是自己恢复寄存器状态。

这里说的“不带寄存器”不是要求 libdecomp 内部完全没有 register address space。
更现实的定义是：LLVM lowering 的函数接口、局部值和返回值不再依赖物理寄存器
slot；如果导出的 P-Code 里仍有 register-space varnode，也必须带有足够的
SSA 和 prototype 信息，可以被明确映射成 LLVM 参数、局部 SSA 值或返回值。

## 当前判断

这条路值得先做 spike，但不能直接当成主路线替换现有 lowering。

原因：

1. libdecomp 的 `Heritage` 本身正好解决当前最难的寄存器 SSA 问题。
2. `ActionPrototypeTypes`、`ActionActiveParam`、`ActionActiveReturn` 等 action
   能给参数和返回值恢复提供现成入口。
3. Ghidra C++ 侧 `Funcdata::encode(...)` 已经能导出分析后的函数数据，Java
   decompiler 也就是通过这类结果和前端通信。
4. 但 libdecomp 依赖 `Architecture`、`Scope`、函数库、compiler spec、内存
   image 和 action pipeline，接入成本明显高于 raw P-Code。
5. 如果只为跑通一个最小原型，先复用 Ghidra headless/Java decompiler 导出的
   HighFunction 也可能更快；C++ libdecomp 原生接入可以作为后续收敛目标。

因此这次计划采用“小步验证”：先证明能拿到 heritage 后 P-Code 和 prototype
信息，再决定是否把它接入 LLVM lowering。

## 输入输出边界

### 输入

第一批输入先限定为：

1. x86-64 ELF 小样例。
2. 单个函数入口地址。
3. Ghidra 能识别的 language/compiler spec。
4. 可选：显式指定调用约定或函数签名，用来对比 prototype recovery 结果。

不要一开始接 LIEF、函数发现、全程序批处理。函数范围和入口地址先由 Ghidra
或测试脚本提供。

### 输出

建议先定义一个导出文件，名字暂定为 `heritage-pcode`。内容只包含 LLVM
lowering 必需信息：

1. 函数名、入口地址、返回类型、参数列表。
2. 基本块列表和后继关系。
3. P-Code op 列表，保留 op 地址和序号。
4. varnode 的 space、offset、size、SSA id、可读名称。
5. `CPUI_MULTIEQUAL`、`CPUI_INDIRECT`、`CALL`、`RETURN` 等函数级语义。
6. prototype 恢复结果：输入参数、返回值、call site 参数和返回值。

格式先不要追求稳定 ABI。可以先用文本或简单 JSON，便于 diff 和调试。等语义
确认后，再考虑 packed encode 或二进制格式。

## 技术路线

### 路线 A：先用 Ghidra headless 导出分析后结果

这是最快的验证路线。

做法：

1. 写一个 GhidraScript，调用 Ghidra decompiler 跑指定函数。
2. 从 `HighFunction` 或 decompiler result 中导出 P-Code、local symbols、
   function prototype、参数、返回值。
3. 把导出结果传给现有 native CLI。
4. native 侧先只打印和校验，不马上生成 LLVM IR。

优点是接入快，不需要马上在 C++ 里搭完整 `ArchitectureGhidra` 环境。

缺点是 Java/GhidraScript 和 native C++ 之间会多一个导出格式，后续如果要纯
native 路线，还要再迁移一次。

### 路线 B：C++ 侧直接接 libdecomp `Funcdata`

这是更贴近最终 native 工具的路线。

做法：

1. 复用 `sleigh::decomp`，创建 libdecomp 需要的 `Architecture` 环境。
2. 加载 language、compiler spec、函数入口和内存镜像。
3. 选择一个较小的 action pipeline，至少跑到 `ActionHeritage`，再逐步加入
   prototype 相关 action。
4. 从 `Funcdata` 遍历 basic block、`PcodeOp`、`Varnode`、prototype 信息。
5. 导出同一份 `heritage-pcode` 数据。

优点是最终边界更干净，能和 `external/NotDec-bin2llvm` 的 C++ lowering
直接合并。

缺点是 libdecomp 初始化和数据库环境比较重，第一步容易卡在工程接入，而不是
LLVM lowering 本身。

### 当前建议

先走路线 A 做 1-2 天 spike。验明三件事：

1. Ghidra 输出里是否能稳定拿到参数、返回值和 heritage 后 P-Code。
2. 导出的 P-Code 是否已经足够少寄存器化，能直接映射到 LLVM 参数/返回值。
3. `CPUI_MULTIEQUAL`、`CPUI_INDIRECT`、call site 信息是否能按预期保留下来。

如果结果好，再把导出 schema 固化，并开始路线 B 的 C++ 原生接入。否则回到
`20260513-02-Bin2llvmRegisterPlan.md` 的 register slot + `mem2reg` 方案。

## LLVM lowering 变化

如果 heritage P-Code 路线可行，LLVM lowering 的重点会变化。

原 raw P-Code 路线需要处理：

1. register varnode 跨块状态。
2. 重叠寄存器。
3. call clobber。
4. ABI 参数和返回值。

heritage P-Code 路线希望把这些前移给 Ghidra。LLVM lowering 主要处理：

1. `CPUI_MULTIEQUAL` 转 LLVM PHI。
2. 普通 SSA varnode 转 LLVM SSA value。
3. 函数参数 varnode 转 LLVM function argument。
4. `RETURN` 转 LLVM return。
5. `CALL` 使用 libdecomp 恢复的 call prototype 生成 LLVM call。
6. `CPUI_INDIRECT` 暂时保守处理，先保留为未知副作用或显式内存屏障。
7. `LOAD/STORE` 继续走现有内存模型，后续再接真实 section/stack。

这样可以把当前最难的寄存器语义问题变成导出质量问题。

## 实施顺序

### 阶段 0：确认能拿到样例函数

目标：

1. 选一个 x86-64 小 ELF 样例，包含参数、返回值、分支和一次调用。
2. 用 Ghidra headless 能稳定打开并定位函数。
3. 记录 language/compiler spec 和调用约定。

判断标准：

1. 同一个函数每次导出地址和基本块一致。
2. 失败时能明确区分是导入问题、函数识别问题还是 decompiler 问题。

### 阶段 1：导出 heritage 后 P-Code

目标：

1. 跑 Ghidra decompiler 分析指定函数。
2. 导出 basic block、P-Code op、varnode、`MULTIEQUAL`。
3. 输出文本文件，方便和 Ghidra GUI 结果对比。

判断标准：

1. 分支汇合处能看到 `CPUI_MULTIEQUAL`。
2. 同一个变量的多次定义有不同 SSA 标识。
3. raw register read/write 明显少于当前 raw P-Code 输入。

### 阶段 2：导出参数和返回值

目标：

1. 导出函数 prototype。
2. 标明每个参数对应的 varnode。
3. 标明返回值对应的 varnode 或 return op 输入。
4. 导出 call site 的参数和返回值信息。

判断标准：

1. `int add(int a, int b)` 这类样例能导出两个参数和一个返回值。
2. 参数不再需要 LLVM lowering 根据 `RDI/RSI/RAX` 猜。
3. call site 参数数量和 Ghidra GUI 反编译结果一致。

### 阶段 3：native 侧读取和校验

目标：

1. 在 `external/NotDec-bin2llvm` 新增读取 `heritage-pcode` 的 CLI。
2. 先只打印规整后的 op 和 varnode。
3. 校验 basic block、PHI 输入数量、参数引用、返回值引用。

判断标准：

1. malformed 输入能报清楚错误。
2. 能统计残留 register-space varnode 数量。
3. 能输出“是否满足 LLVM lowering 前置条件”。

### 阶段 4：转 LLVM IR 最小闭环

目标：

1. 用 `MULTIEQUAL` 生成 LLVM PHI。
2. 用 prototype 生成 LLVM function type 和 arguments。
3. 用 `RETURN` 生成返回值。
4. 先支持整数、分支、load/store、直接 call。

判断标准：

1. 输出 IR 通过 LLVM verifier 和 `llvm-as`。
2. 简单参数函数的 LLVM IR 不出现物理寄存器 slot。
3. 分支样例的 PHI 和 Ghidra `MULTIEQUAL` 对应。

### 阶段 5：决定是否转主路线

目标：

1. 对比 heritage P-Code 路线和 register slot 路线的实现成本。
2. 对比输出 IR 可读性和后续接 NotDec 类型恢复的难度。
3. 评估是否继续投入 C++ 原生 libdecomp 接入。

判断标准：

1. 如果多数样例已经能无寄存器 slot 生成 IR，优先推进 heritage 路线。
2. 如果导出结果仍大量依赖寄存器语义，回到 register slot + `mem2reg`。
3. 如果 Java 导出好用但 C++ libdecomp 接入太重，短期保留 Java 导出作为
   工具链入口。

## 风险

1. libdecomp 不是轻量库。`Funcdata` 依赖的环境比 libsla 多很多，C++ 原生接入
   可能比预期慢。
2. Ghidra 的 decompiler action 会做优化和简化。导出的 P-Code 可能已经不是
   低层指令语义，后续 LLVM lowering 要接受这一点。
3. 参数恢复不一定稳定。没有符号、调用约定不准、间接调用多时，prototype
   结果可能错。
4. `CPUI_INDIRECT`、spacebase、stack pointer、join space 这些语义不能简单
   当普通 opcode 处理。
5. 如果导出格式过早设计得太大，会变成另一个中间 IR。第一版必须够小。
6. 用 Java/GhidraScript 做 spike 很快，但会让工具链多一层运行时依赖。

## 和现有计划的关系

这份计划不否定 `20260513-02-Bin2llvmRegisterPlan.md`。

两者关系是：

1. register slot + `mem2reg` 是可控的自研底线方案。
2. libdecomp heritage 是借 Ghidra 分析结果加速的试验方案。
3. 如果 heritage 路线成功，raw P-Code lowering 仍可保留，用作无 Ghidra
   decompiler 环境时的 fallback。
4. 如果 heritage 路线失败，已有 register slot 计划可以继续推进，不会卡住。

## 验收样例

第一批建议四个小样例：

1. `int add(int a, int b)`：验证参数和返回值。
2. `int max(int a, int b)`：验证分支和 `MULTIEQUAL`。
3. `int loop(int n)`：验证循环 PHI。
4. `int caller(int x) { return callee(x + 1); }`：验证 call site 参数和返回值。

每个样例都保留：

1. 原始 C 源码。
2. 编译命令。
3. Ghidra 导出的 heritage P-Code。
4. 生成的 LLVM IR。
5. verifier/assembler 结果。

## 性能关注

这条路线暂时不影响 NotDec 主链路，也不跑当前 `fortune.o3.wasm.ll` 用例计时。

后续如果接入主链路或替换 `external/NotDec-bin2llvm` 的默认路径，需要记录：

1. 单函数 Ghidra decompiler 分析耗时。
2. heritage P-Code 导出耗时。
3. LLVM lowering 耗时。
4. 和 raw P-Code lowering 的同样例耗时差异。

如果未来影响 NotDec 类型恢复、结构体合并或 pointer analysis，再按项目规范对
`fortune` 当前关注口径做时间对比。

## 方案评分

实现效果：8/10。若 libdecomp 导出质量稳定，可以直接绕开寄存器 SSA、重叠寄存器
和 ABI 参数恢复这几个难点。

复杂度：7/10。LLVM lowering 会变简单，但 Ghidra decompiler 接入和导出格式会增加
理解成本。

维护成本：6/10。依赖 Ghidra action pipeline 的行为，后续 Ghidra 版本变化可能影响
输出，需要用样例锁住。

当前建议：先做路线 A 的最小 spike。不要马上重写 lowering，也不要急着在 C++ 里
完整复刻 Ghidra decompiler 环境。先用导出结果判断这条路值不值得继续。

## 实现记录（2026-05-13，Ghidra headless heritage P-Code 导出）

这次先推进路线 A，不碰 C++ libdecomp 原生接入，也不改 LLVM lowering。

### 已完成

1. 新增 GhidraScript：
   - `external/NotDec-bin2llvm/ghidra_scripts/ExportHeritagePcode.java:33-501`
   - 作用是调用 Ghidra decompiler，拿 `HighFunction`，导出 JSON。
2. 更新脚本文档：
   - `external/NotDec-bin2llvm/ghidra_scripts/README.md:1-31`

### 修改内容

1. `ExportHeritagePcode.java:39-67`
   - 新增脚本入口 `run()`。
   - 参数为输出 JSON、函数入口或函数名、simplification style、timeout。
2. `ExportHeritagePcode.java:69-99`
   - 新增 `resolveFunction(...)`。
   - 支持按地址、入口地址字符串或函数名定位函数。
3. `ExportHeritagePcode.java:101-123`
   - 新增 `decompile(...)`。
   - 使用 `DecompInterface`，默认 style 为 `decompile`，开启 syntax tree，关闭 C 输出。
4. `ExportHeritagePcode.java:125-189`
   - 新增 JSON 导出入口和 varnode 收集。
   - 统计残留 register-space varnode 数量。
5. `ExportHeritagePcode.java:191-235`
   - 导出 program、function、prototype、参数 storage、参数 varnode 和 HighVariable 名。
6. `ExportHeritagePcode.java:237-323`
   - 导出 basic block、入边、出边、block 内 op 顺序。
   - 导出 P-Code op、输入输出 varnode、direct call target/name。
7. `ExportHeritagePcode.java:325-379`
   - 导出 varnode 的 space、offset、size、寄存器名、HighVariable、类型、def。
   - 导出 block/op/varnode/MULTIEQUAL 统计。
8. `ExportHeritagePcode.java:381-501`
   - 新增 op、varnode、block id 辅助函数。
   - 新增 JSON 字符串转义。

### 验证

临时样例：

```c
int callee(int x) { return x + 3; }
int max2(int a, int b) { return a > b ? a : b; }
int caller(int x) { return callee(x + 1); }
int main(void) { return caller(max2(1, 2)); }
```

构建样例：

```bash
gcc -O0 -g /tmp/notdec-heritage-sample.c -o /tmp/notdec-heritage-sample
```

导出 `max2`：

```bash
mkdir -p /tmp/notdec-ghidra-heritage-proj
/sn640/ghidra/build/dist/ghidra_11.3.2_DEV/support/analyzeHeadless \
  /tmp/notdec-ghidra-heritage-proj NotDecHeritageMax2b \
  -import /tmp/notdec-heritage-sample \
  -scriptPath /sn640/NotDec/external/NotDec-bin2llvm/ghidra_scripts \
  -postScript ExportHeritagePcode.java /tmp/notdec-heritage-max2.json max2 \
  -deleteProject
```

导出 `caller`：

```bash
/sn640/ghidra/build/dist/ghidra_11.3.2_DEV/support/analyzeHeadless \
  /tmp/notdec-ghidra-heritage-proj NotDecHeritageCaller2 \
  -import /tmp/notdec-heritage-sample \
  -scriptPath /sn640/NotDec/external/NotDec-bin2llvm/ghidra_scripts \
  -postScript ExportHeritagePcode.java /tmp/notdec-heritage-caller.json caller \
  -deleteProject
```

结果：

1. 两条 headless 命令都成功，脚本能被 Ghidra 编译和执行。
2. `max2` 导出：
   - 参数：`a`、`b`
   - 返回类型：`int`
   - basic block：3
   - P-Code op：6
   - `MULTIEQUAL`：1
   - register-space varnode：3
3. `max2` 的 `MULTIEQUAL` 已出现在汇合块：
   - output：`vn:163`
   - inputs：`vn:108`、`vn:175`
4. `max2` 残留的 register-space varnode：
   - `EDI` input，HighVariable 为 `a`
   - `ESI` input，HighVariable 为 `b`
   - `EAX` output，类型为 `int`
5. `caller` 导出：
   - 参数：`x`
   - basic block：1
   - P-Code op：4
   - direct call target：`ram:00101129`
   - direct call target name：`callee`

### 当前判断

这条路线值得继续做下一步读取器。

原因：

1. 参数已经能从 `FunctionPrototype` 明确导出，不需要 lowering 根据 `EDI/ESI`
   自己猜参数。
2. 分支汇合已经有 `MULTIEQUAL`，可以直接转 LLVM PHI。
3. CALL 的目标、输出和输入都能从分析后 P-Code 拿到。
4. 残留寄存器没有完全消失，但已经能通过 HighVariable/prototype 映射解释。

### 当前限制

1. 还没有 native 侧 `heritage-pcode` JSON 读取器。
2. 还没有把 `MULTIEQUAL` 转 LLVM PHI。
3. call site 参数目前先依赖 CALL op 输入，没有额外导出 Ghidra 的 call prototype
   对象。
4. 这次只改 `external/NotDec-bin2llvm` 的脚本和文档，没有影响 NotDec 主链路，
   所以没有跑 `fortune.o3.wasm.ll` 计时。

### 下一步

1. 在 `external/NotDec-bin2llvm` 增加一个只读 JSON 校验 CLI。
2. 校验项先包括参数 varnode、block/op 引用、PHI 输入数量、残留 register 数。
3. 校验稳定后，再做 `MULTIEQUAL -> LLVM PHI` 的最小 lowering。

## 实现记录（2026-05-13，native heritage JSON 校验器）

这次继续推进上一节的下一步，只做 JSON 结构校验，不做 LLVM IR lowering。

### 已完成

1. 新增 native CLI：
   - `external/NotDec-bin2llvm/tools/notdec-heritage-check.cpp:1-349`
   - 使用 LLVM 自带 `llvm/Support/JSON.h` 和 `MemoryBuffer`，没有引入新依赖。
2. 接入 CMake：
   - `external/NotDec-bin2llvm/tools/CMakeLists.txt:10-17`
   - 新增 target `notdec-heritage-check`。

### 修改内容

1. `notdec-heritage-check.cpp:14-28`
   - 新增 `CheckState`。
   - 保存 block/op/varnode 集合、block 前驱数量、错误列表和摘要计数。
2. `notdec-heritage-check.cpp:68-133`
   - 新增 `collectBlocks(...)`、`collectVarnodes(...)`、`collectOps(...)`。
   - 先收集 id 集合，后续引用校验用。
3. `notdec-heritage-check.cpp:135-168`
   - 新增 `checkFunction(...)`。
   - 校验参数列表和参数 varnode 引用。
4. `notdec-heritage-check.cpp:170-220`
   - 新增 `checkBlockReferences(...)`。
   - 校验 block 入边、出边、op 引用都能解析到已有对象。
5. `notdec-heritage-check.cpp:222-281`
   - 新增 `checkOpReferences(...)`。
   - 校验 op 的 parent、input/output varnode。
   - 对 `MULTIEQUAL` 额外检查输入数量是否等于父 block 前驱数量。
6. `notdec-heritage-check.cpp:283-349`
   - 新增摘要输出和 `main(...)`。
   - 校验 schema 为 `notdec.heritage-pcode.v0`。

### 验证

构建：

```bash
cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-build-off -G Ninja
cmake --build /tmp/notdec-bin2llvm-build-off --target notdec-heritage-check -j4
```

用上一节导出的 JSON 校验：

```bash
/tmp/notdec-bin2llvm-build-off/bin/notdec-heritage-check /tmp/notdec-heritage-max2.json
/tmp/notdec-bin2llvm-build-off/bin/notdec-heritage-check /tmp/notdec-heritage-caller.json
```

结果：

1. `max2`：
   - blocks：3
   - ops：6
   - varnodes：8
   - params：2
   - missing param varnodes：0
   - register varnodes：3
   - `MULTIEQUAL`：1
   - direct calls：0
   - status：ok
2. `caller`：
   - blocks：1
   - ops：4
   - varnodes：7
   - params：1
   - missing param varnodes：0
   - register varnodes：4
   - `MULTIEQUAL`：0
   - direct calls：1
   - status：ok
3. 旧无 sleigh 最小 CLI 仍可构建：

```bash
cmake --build /tmp/notdec-bin2llvm-build-off --target notdec-bin2llvm -j4
```

结果为 `ninja: no work to do.`。

### 当前判断

JSON 导出格式已经够支撑下一步最小 lowering：

1. 参数 varnode 能被 native 侧解析和校验。
2. block/op/varnode 引用闭合。
3. `MULTIEQUAL` 的输入数量和父 block 前驱数量一致。
4. CALL 目标能被统计出来。

### 当前限制

1. 校验器还没有把 JSON 转成项目内 C++ 数据结构。
2. 还没有处理类型宽度到 LLVM type 的映射。
3. 还没有生成 LLVM PHI、参数和 return。
4. 这次仍只改 `external/NotDec-bin2llvm`，没有影响 NotDec 主链路，所以没有跑
   `fortune.o3.wasm.ll` 计时。

### 下一步

1. 把 checker 里的临时读取逻辑收敛成可复用的 `HeritagePcode` 数据结构。
2. 先做 `max2` 的 lowering：参数、比较、`MULTIEQUAL`、return。
3. 再做 `caller` 的 direct call lowering。
