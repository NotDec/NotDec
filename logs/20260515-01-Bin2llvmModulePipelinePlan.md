# 2026-05-15 bin2llvm 模块级链路计划

## 背景

`external/NotDec-bin2llvm` 当前主链路仍是函数级：

1. Ghidra headless 对一个函数导出 heritage P-Code JSON。
2. native 侧读取这个 JSON。
3. `notdec-heritage-llvm` 生成一个 LLVM module。
4. 这个 module 里主要只有一个有函数体的 LLVM function。

Bench2 真实项目测试说明，这条链路已经能在小函数上跑通：

- `vsftpd`
- `libuv`
- `memcached`
- `lighttpd`
- `tmux`
- `openssh`
- `wolfssl`
- `redis`
- `libicu`
- `python`
- `wrk`
- `ffmpeg`

但它还不是完整二进制到完整 LLVM module 的链路。当前主要缺口是：

1. JSON schema 只表达单个函数。
2. lowering 只围绕一个函数建 body。
3. 外部导入函数只是 call-site 临时 `getOrInsertFunction(...)`，没有统一声明表。
4. 内部函数之间没有模块级地址和符号解析。
5. 一个函数 lowering 失败会让整个输出失败，不适合真实项目。

Bench2 暴露出的真实问题也说明，下一步不应继续只堆 opcode，而应该先把模块边界做稳。

## 目标

第一阶段目标不是完整反编译整个 ELF，而是先生成一个结构正确、能被 LLVM 接受的模块级 IR：

1. 一个输入 ELF / shared object 对应一个 LLVM module。
2. module 里包含多个内部函数。
3. 成功 lowering 的内部函数有 body。
4. 暂时无法 lowering 的内部函数保留为 declaration。
5. 动态导入函数、外部库函数、未知 call target 都转成无函数体的 LLVM external declaration。
6. 单个函数失败不阻断整个模块输出。
7. 输出 `.ll` 能通过 `llvm-as`。

示例形态：

```llvm
define i32 @sub_00106740(i32 %argc, ptr %argv) {
  ...
}

declare i32 @printf(...)
declare i32 @SSL_read(...)
declare i64 @notdec_unknown_00123456(...)

@notdec_ram = external global [1048576 x i8]
```

## 不做的事

第一阶段先不做这些：

1. 不恢复完整 C prototype。
2. 不精确实现所有 ABI。
3. 不把 ELF section、GOT、PLT、relocation 全部还原成准确 LLVM global。
4. 不要求所有函数都能 lower 成 body。
5. 不接 NotDec 主 pass pipeline。
6. 不处理完整 debug type 恢复。

这些都重要，但如果和模块链路一起做，会让问题边界太大。

## 总体方案

新增模块级 schema，同时保留现有函数级 schema。

当前函数级 JSON：

```json
{
  "schema": "notdec.heritage-pcode.v0",
  "program": {},
  "function": {},
  "blocks": [],
  "ops": [],
  "varnodes": []
}
```

新增模块级 JSON：

```json
{
  "schema": "notdec.heritage-module.v0",
  "program": {},
  "functions": [
    {
      "name": "sub_00106740",
      "entry": "ram:00106740",
      "status": "ok",
      "returnType": "int",
      "params": [],
      "blocks": [],
      "ops": [],
      "varnodes": []
    }
  ],
  "externals": [
    {
      "name": "printf",
      "address": "EXTERNAL:...",
      "returnType": "int",
      "params": [],
      "source": "import"
    }
  ],
  "failures": [
    {
      "entry": "ram:00123456",
      "name": "bad_func",
      "stage": "decompile",
      "message": "..."
    }
  ]
}
```

`functions[]` 中每个元素尽量复用现有 `HeritageProgram` 的字段，避免重新发明一套 IR。

## Ghidra 导出层

新增一个模块导出入口，例如：

```bash
ExportHeritageModule.java /tmp/module.json [function-limit] [timeout-sec]
```

导出流程：

1. 遍历 `currentProgram.getFunctionManager().getFunctions(true)`。
2. 跳过 external function。
3. 可选跳过 thunk / PLT stub，第一版建议默认跳过 thunk。
4. 对每个真实函数调用 decompiler，拿 `HighFunction`。
5. 成功就写入 `functions[]`。
6. 失败就写入 `failures[]`，继续下一个函数。
7. 从 external manager、import table、symbol table、CALL target 中收集 `externals[]`。

函数筛选需要支持简单参数：

1. `--all`：导出所有函数。
2. `--limit=N`：只导出前 N 个函数，用于 Bench2 快速测试。
3. `--min-size` / `--max-size` 后续再考虑，不作为第一步。

Ghidra 大 ELF auto-analysis 是现实瓶颈。第一版先接受这个成本，但日志里必须记录：

1. Ghidra analysis 时间。
2. decompile 成功函数数。
3. decompile 失败函数数。
4. 导出 JSON 大小。

## Native 数据结构

新增模块级数据结构，不直接破坏现有函数级结构：

```cpp
struct HeritageExternalFunction {
  std::string Name;
  std::string Address;
  std::string ReturnType;
  std::vector<HeritageParam> Params;
  std::string Source;
};

struct HeritageModuleFunction {
  HeritageFunction Function;
  std::vector<HeritageBlock> Blocks;
  std::vector<HeritageOp> Ops;
  std::vector<HeritageVarnode> Varnodes;
  std::string Status;
  std::string ErrorMessage;
};

struct HeritageModule {
  std::string Schema;
  HeritageProgramInfo Program;
  std::vector<HeritageModuleFunction> Functions;
  std::vector<HeritageExternalFunction> Externals;
};
```

实现时可以先用更少字段，但边界要按这个方向走。

## LLVM lowering

模块 lowering 分两遍。

第一遍只建声明：

1. 为 `functions[]` 中每个内部函数创建 LLVM `Function`。
2. 为 `externals[]` 中每个外部函数创建 LLVM declaration。
3. 为 call target 中找不到的地址创建 `notdec_unknown_<addr>` declaration。
4. 建立地址到 LLVM function 的 map。
5. 建立名字到 LLVM function 的 map。

第二遍填 body：

1. 对 `status == ok` 的内部函数调用现有函数级 lowering。
2. lowering 成功就填入 body。
3. lowering 失败就删除半成品 basic block，保留 declaration，并记录失败。
4. module 最后统一跑 verifier。

这样一个坏函数不会破坏整个模块。

## 外部函数规则

导入函数和外部库函数统一转成没有函数体的 LLVM declaration。

类型来源优先级：

1. Ghidra decompiler call prototype。
2. Ghidra function signature。
3. imported symbol data type。
4. ELF dynsym 名字。
5. 完全未知地址。

第一版类型可以保守：

1. 已知 `void` 就用 `void`。
2. 已知整数宽度就用对应 `iN`。
3. 指针先用地址宽度整数或 `ptr`，取决于当前 LLVM opaque pointer 配置。
4. 不确定参数时用 vararg function type。

例如：

```llvm
declare i32 @printf(...)
declare i32 @SSL_read(...)
declare void @exit(...)
declare i64 @notdec_unknown_00123456(...)
```

这里要避免同名函数被不同 call site 插入成冲突类型。建议第一版对外部函数统一用 vararg，等 schema 稳定后再收紧。

## 内部函数命名

真实项目里常见问题：

1. stripped binary 没有稳定函数名。
2. C++ mangled name 很长。
3. LTO 后缀包含特殊字符。
4. 不同地址可能有同名 local symbol。

命名规则建议：

1. 有唯一合法名字：使用清洗后的名字。
2. 名字冲突：追加地址，例如 `foo_00101234`。
3. 没名字：使用 `sub_00101234`。
4. 原始名字和入口地址写进 metadata 或注释。

第一版只要保证 LLVM 名字合法且稳定。

## 调用解析

`CALL` lowering 的目标选择：

1. `callTarget` 是内部函数入口地址：调用对应内部 LLVM function。
2. `callTargetName` 命中 external declaration：调用 external。
3. `callTargetName` 命中内部函数名：调用内部函数。
4. 只有地址但不在内部函数表：创建 `notdec_unknown_<addr>` declaration。
5. `CALLIND` 第一版继续走 helper，占位为 `notdec_heritage_CALLIND_*`。

这样能先覆盖普通 direct call。

## 真实内存和 global

当前函数级 lowering 里，`LOAD` / `STORE` 的临时内存模型是一个外部全局 byte array：

```llvm
@notdec_ram = external global [1048576 x i8]
```

访问时先把 P-Code 地址 resize 成 64 位，再对 `@notdec_ram` 做 GEP，最后 load/store。

模块级第一版建议顺手改成更接近真实地址语义的 `inttoptr` 模式：

```llvm
%ptr = inttoptr i64 %addr to ptr
%val = load i32, ptr %ptr, align 1
store i32 %val, ptr %ptr, align 1
```

原因：

1. 对真实二进制来说，P-Code 里的 RAM 地址本来就是地址，不是 `@notdec_ram` 的数组下标。
2. `@notdec_ram[1048576]` 会人为制造 1MiB 边界，真实项目地址很容易超过这个范围。
3. 模块级 IR 里 direct address、external memory、stack-like pointer 都更适合先表示成指针访问。
4. 后续如果要按 ELF segment 恢复 global，可以再把部分 `inttoptr` 访问替换成更精确的 global/GEP。

这个改动不等于精确内存建模，只是把临时模型从“一个假数组”改成“地址转指针后访问”。第一版仍然统一 `align 1`，
并且暂不区分 stack、heap、global、MMIO。

不要在模块链路第一步里同时做精确 global。后续可以按顺序扩展：

1. 把 ELF loaded segment 建成 LLVM global array。
2. 把只读数据、字符串单独拆出。
3. 把 GOT/PLT relocation 映射到 external declaration。
4. 把 stack/local 继续交给 Ghidra heritage 后 P-Code 表达。

初始化内存可以比普通未知内存更早建模。建议在模块 JSON 里导出 memory map：

```json
{
  "memoryRanges": [
    {
      "name": ".rodata",
      "start": "0x200000",
      "end": "0x201000",
      "permissions": "r--",
      "initialized": true,
      "bytes": "..."
    },
    {
      "name": ".bss",
      "start": "0x404000",
      "end": "0x405000",
      "permissions": "rw-",
      "initialized": false
    }
  ]
}
```

native lowering 对 initialized range 创建 LLVM global：

```llvm
@notdec_ram_0x200000_0x201000 = private constant [4096 x i8] c"..."
```

命名规则：

1. 用 `notdec_ram_<start>_<end>`，例如 `notdec_ram_0x200000_0x201000`。
2. 如果 LLVM 名字里不想保留 `x` 或大小写，可以统一成 `notdec_ram_200000_201000`。
3. 原始 section 名、权限、文件偏移、是否可写不要塞进名字，放 metadata。

metadata 建议先挂在 global 上：

```llvm
@notdec_ram_0x200000_0x201000 = private constant [4096 x i8] c"...", !notdec.mem !0

!0 = !{
  !"range",
  i64 2097152,
  i64 2101248,
  !"name",
  !".rodata",
  !"perm",
  !"r--",
  !"initialized",
  i1 true
}
```

如果 LLVM IR 语法上挂自定义 metadata 不方便，退一步可以先建模块级 named metadata：

```llvm
!notdec.memory = !{!0, !1}
!0 = !{!"notdec_ram_0x200000_0x201000", i64 2097152, i64 2101248, !".rodata", !"r--", i1 true}
!1 = !{!"notdec_ram_0x404000_0x405000", i64 4210688, i64 4214784, !".bss", !"rw-", i1 false}
```

第一版 lowering 统一用 `inttoptr` 做 load/store，不根据地址是否落在 initialized range 内改写成 global/GEP。
initialized memory global 只负责保存初始字节和范围信息，不参与当前访问改写。后续如果要把内存访问和这些
global 对接，单独做一轮处理。

更稳的顺序是：

1. 先导出 memory map。
2. 先生成 initialized memory global 和 `!notdec.memory`。
3. `LOAD/STORE` 继续走 `inttoptr`，保证语义简单。

这样 metadata 先把范围信息保存下来，后面的分析或优化可以使用，但不会阻塞模块级 IR 先跑通。

## 实施顺序

### 阶段 1：模块 JSON 导出

目标：

1. 新增模块导出脚本。
2. 能导出 Bench2 一个小项目的多个函数。
3. 能收集 external/import function 列表。
4. 单函数 decompile 失败不终止。

判断标准：

1. JSON 中 `functions[]` 数量大于 1。
2. JSON 中 `externals[]` 有 libc/OpenSSL 等导入函数。
3. 日志能看到成功/失败统计。

### 阶段 2：模块 JSON loader 和 checker

目标：

1. native 侧读取 `notdec.heritage-module.v0`。
2. 新增 `notdec-heritage-module-check`。
3. 输出函数数、external 数、call target 命中率、失败数。

判断标准：

1. checker 能读模块 JSON。
2. 能检查 block/op/varnode 引用。
3. 能发现重复函数名和重复地址。

### 阶段 3：只生成 declarations 的 LLVM module

目标：

1. 新增 `notdec-heritage-module-llvm`。
2. 只生成内部函数 declaration 和 external declaration。
3. 不填函数体。

判断标准：

1. 输出 `.ll` 能通过 `llvm-as`。
2. external function 全部没有 body。
3. 内部函数名稳定、无冲突。

### 阶段 4：填入成功函数 body

目标：

1. 复用当前 `HeritageToLLVM`。
2. 对能 lowering 的函数填 body。
3. 对失败函数保留 declaration。
4. 输出 module 仍通过 verifier。

判断标准：

1. 至少一个 Bench2 项目输出多个 defined functions。
2. module 同时包含 defined functions 和 external declarations。
3. lowering 失败数写进日志，不阻断输出。

### 阶段 5：修真实控制流问题

Bench2 已经暴露的问题优先级：

1. `CBRANCH target block is unknown`
2. PHI 没有放在 basic block 顶部
3. PHI incoming value 不支配 use
4. Ghidra 大 ELF auto-analysis 超时
5. DWARF external debug info 发现但加载失败

这一步再开始修 lowering 细节，不要在阶段 1 到 3 混进去。

## 2026-05-15 实施记录：阶段 1 到 3 已完成

本次先只做模块壳，不填函数体。

已改文件：

1. `external/NotDec-bin2llvm/ghidra_scripts/ExportHeritageModule.java`
   - 第 58 行 `run()`：新增模块级导出入口，预先登记选中函数入口；单函数失败写入 `failures[]`，继续导出后续函数。
   - 第 134 行 `parseOptions(...)`：支持 `--all`、`--limit=N`、`--timeout=N`、`--style=name`，默认最多 20 个函数。
   - 第 169 行 `selectFunctions(...)`：只选择非 external、非 thunk 函数，并收集 function manager 里的 external。
   - 第 208 行 `writeProgram(...)` 和第 218 行 `writeFunctionObject(...)`：输出 `notdec.heritage-module.v0`，每个函数复用原单函数 heritage 字段。
2. `external/NotDec-bin2llvm/include/notdec-bin2llvm/HeritagePcode.h`
   - 第 62 行新增 `HeritageProgramInfo`。
   - 第 82 行新增 `HeritageModuleFunction`，第 91 行新增 `HeritageExternalFunction`，第 99 行新增 `HeritageModuleFailure`，第 109 行新增 `HeritageModule`。
   - 第 121 行新增 `loadHeritageModuleFromJson(...)`。
3. `external/NotDec-bin2llvm/lib/HeritagePcode.cpp`
   - 第 80 行 `readProgramInfo(...)`、第 95 行 `readParam(...)`、第 109 行 `readFunctionObject(...)`：把原单函数字段读取拆成可复用函数。
   - 第 255 行 `readModuleFunction(...)`：读取 `functions[]` 中单个函数并建立索引。
   - 第 273 行 `readExternalFunction(...)` 和第 302 行 `readFailure(...)`：读取外部声明和失败记录。
   - 第 366 行 `loadHeritageModuleFromJson(...)`：新增模块 JSON loader。
4. `external/NotDec-bin2llvm/include/notdec-bin2llvm/HeritageToLLVM.h`
   - 第 24 行新增 `buildHeritageDeclarationModule(...)` 声明。
5. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 32 行 `typeForSourceType(...)`：集中处理第一版保守 LLVM 类型。
   - 第 53 行 `sanitizeSymbolName(...)`、第 73 行 `addressSuffix(...)`、第 89 行 `uniqueSymbolName(...)`：保证模块内函数名合法且不冲突。
   - 第 1354 行 `buildHeritageDeclarationModule(...)`：只生成内部函数 declaration 和 external vararg declaration。
6. `external/NotDec-bin2llvm/tools/notdec-heritage-module-check.cpp`
   - 第 24 行 `checkFunctionRefs(...)`：检查 block/op/varnode 引用。
   - 第 76 行 `checkModuleSymbols(...)`：检查重复函数名和重复入口。
   - 第 97 行 `countCalls(...)`：统计 direct call 的内部、外部、未知命中数。
7. `external/NotDec-bin2llvm/tools/notdec-heritage-module-llvm.cpp`
   - 第 56 行 `main(...)`：读取模块 JSON，生成 declaration-only `.ll`，并跑 LLVM verifier。
8. `external/NotDec-bin2llvm/tools/CMakeLists.txt`
   - 第 28 行新增 `notdec-heritage-module-check`。
   - 第 37 行新增 `notdec-heritage-module-llvm`。
9. `external/NotDec-bin2llvm/ghidra_scripts/README.md`
   - 第 33 行新增 `ExportHeritageModule.java` 用法和 native 工具链。

验证：

1. 配置并编译：

```bash
cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-build -G Ninja
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-module-check notdec-heritage-module-llvm -j4
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-check notdec-heritage-llvm -j4
```

2. 用现有单函数 JSON 拼出 `/tmp/notdec-heritage-module-smoke.json`，结果：

```text
heritage-module check
  functions: 2
  externals: 1
  failures: 0
  direct calls: 0
  resolved internal calls: 0
  resolved external calls: 0
  unknown calls: 0
  status: ok
```

3. `notdec-heritage-module-llvm` 生成的 declaration-only IR：

```llvm
declare i32 @structops()
declare i32 @floatops()
declare i32 @printf(...)
```

4. `llvm-as /tmp/notdec-heritage-module-smoke.ll -o /tmp/notdec-heritage-module-smoke.bc` 通过。
5. 旧单函数工具 `notdec-heritage-check`、`notdec-heritage-llvm` 对 `/tmp/notdec-heritage-structops.json` 仍通过，生成 IR 也能 `llvm-as`。

未验证：

1. `ExportHeritageModule.java` 还没有用真实 Ghidra headless 跑 Bench2；这里只做了脚本实现和 native 链路 smoke test。
2. 阶段 4 没开始，所以模块 `.ll` 目前只有 declarations，没有函数 body。

性能：

这次只影响 `external/NotDec-bin2llvm`，没有接 NotDec 主 pass pipeline，不跑 `fortune.o3.wasm.ll` 同口径性能。当前 smoke test 只验证 loader/checker/LLVM declaration 生成，未做 Bench2 计时。

阶段评分：

实现效果：7/10。阶段 1 到 3 的骨架已经可编译、可读模块 JSON、可输出能 `llvm-as` 的 `.ll`。

复杂度：5/10。主要是 schema 容器、loader 和符号去重，暂时没有引入新的 lowering 复杂度。

维护成本：6/10。单函数 schema 保留，模块函数复用单函数结构，后续阶段 4 可以在这个边界上继续填 body。

## 风险

1. Ghidra 对大 ELF 的 auto-analysis 时间很长，`vim` 已经出现 6 分钟仍未进入导出脚本。
2. 不同函数 call site 看到的外部函数类型可能不一致，容易产生 LLVM 函数类型冲突。
3. Ghidra 对 stripped binary 的函数边界不一定稳定。
4. 真实函数里 PHI 和控制流比当前小样例复杂，模块化后失败会更多。
5. 如果一开始追求精确 ELF global，会拖慢模块链路验证。

## 验收方式

第一轮用 Bench2 顺序测试：

1. `vsftpd`
2. `libuv`
3. `memcached`
4. `lighttpd`
5. `tmux`
6. `openssh`
7. `wolfssl`
8. `redis`
9. `libicu`
10. `vim`
11. `python`
12. `wrk`
13. `ffmpeg`
14. `php`

每个项目至少输出：

1. 模块 JSON。
2. checker 日志。
3. `.ll`。
4. `llvm-as` 结果。
5. 成功 lower 的函数数。
6. 保留 declaration 的内部函数数。
7. external declaration 数。
8. 失败函数数和前几个失败原因。

## 性能关注

这次计划主要影响 `external/NotDec-bin2llvm`，不接 NotDec 主 pass pipeline，所以不需要跑
`fortune.o3.wasm.ll` 同口径性能。

如果后续把模块级 bin2llvm 接入主项目 CLI，再按项目规范补 fortune 运行时间对比。

Bench2 自身需要记录：

1. Ghidra import + analysis 时间。
2. JSON 导出时间。
3. native loader/checker 时间。
4. LLVM lowering 时间。
5. `llvm-as` 时间。
6. 输出 JSON / `.ll` / `.bc` 大小。

## 方案评分

实现效果：8/10。能把当前函数级原型推进到真实模块形态，也能把导入函数统一表达成 LLVM external declaration。

复杂度：6/10。主要复杂度来自 schema 和跨函数符号表，不是新算法；控制流和 PHI 问题可以留到模块壳稳定后修。

维护成本：7/10。模块级结构会长期存在，需要保持 schema 清楚。保留函数级结构可以降低迁移成本。

更好的方案可能是直接接 C++ libdecomp `Funcdata` 和 Ghidra program database，少走 JSON。但当前 JSON 路线已经能用
Bench2 快速验证，先做模块壳更稳。
