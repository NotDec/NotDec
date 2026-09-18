# Solidity 后端：private helper 函数与 SSA 局部变量物化（思路 B）

## 本次用户原始 prompt

> 直接按照思路B走吧。有关于基于 SSA value转局部变量的设计要参考那边llvm2c转C的那边。特别是关于某个 SSA value 是可以直接作为表达式，还是说要作为一个局部变量缓存临时结果，那边有一些比较好的判断流程，抄过来。

上下文补充：思路 B 指保留 `private__*` helper，在 Solidity 后端把它们输出成函数，
把 helper 调用结果物化成局部变量；不再走思路 A 的 IR 内联。

## 背景

evm2llvm 会把 Solidity 的内部函数、ABI 解码等共享代码 outline 成
`private__<name>_<offset>(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 ...)`。
当前 Solidity 后端只读 `public_*` 入口函数，private helper：

- 函数体完全不输出；
- 调用结果一律落到 `UnresolvedValueExpr`，例如
  `return 0 /* TODO: unresolved value: private__0x1b7_0x1b7 */;`
- void helper 的调用被静默丢弃，等价于丢语义。

统计：103 个 pattern case 里 79 个含 private helper，共 3425 个定义、9602 个调用点，
是当前覆盖面最大的结构性缺口。

路线 A（在 IR 层内联 private helper）已实测变差并完全回滚：编译通过 103→93，
`// goto` 423→4755，pattern suite 103→32；晚期内联还丢 `abi_return`（66→30）。
结论记录在 `logs/20260917-04-evm-solidity-next-steps.md`。

## codegen 依据

1. Solidity 源码里 `function f() internal { ... }` 在 EVM 里就是一段被多个调用点共享的
   代码，编译器（以及 evm2llvm 的 outline）保留成独立函数；后端把它还原成 Solidity 内部
   函数是语义上最直接的对应，而不是把调用方展开。
2. 调用结果在 Solidity 里要么直接作为表达式参与外层表达式（单次使用、同一基本块），
   要么必须先落到局部变量再使用（多次使用或跨块），否则会重复求值或改变求值顺序。
   llvm2c 的 C 后端已经有成型的判断流程，本次按用户要求直接移植。

## 参考实现（`external/NotDec-llvm2c` C 后端）

`lib/notdec-llvm2c/StructuralAnalysis.cpp`：

- `SAFuncContext::addExprOrStmt(Value &V, Stmt &Stmt, CFGBlock &Block, QualType Ty)`：
  1. 非 Instruction 或 Argument → 永远作为表达式记录到 `ExprMap`；
  2. `V.getNumUses() == 0` → 作为语句（只保留副作用）；
  3. `LoadInst` → 不折叠，必须 `cacheExpr`；理由是两条使用之间可能发生 store，
     折叠会让两次使用读到同一个值；
  4. `onlyUsedInCurrentBlock(Inst) || isAddrOf(&Expr)` → 折叠，使用点直接内联该表达式；
  5. 其它 → `cacheExpr(...)` 生成 `T temp_N = expr;`，`ExprMap[V]` 记录对该局部变量的引用。
- `onlyUsedInCurrentBlock(Inst)` = `hasOneUseIgnoreCast(Inst)` 且该唯一使用与定义在同一基本块；
- `hasOneUseIgnoreCast(Val)`：唯一使用；若该唯一使用是 CastInst，则要求这个 cast 也只有
  一个使用（即穿过 cast 链后仍然只有一个消费者）；
- `cacheExpr(...)`：用 `ValueNamer::getTempName` 取稳定名字（`temp_<n>`），
  在当前位置生成带初始化的变量声明并追加到当前 CFGBlock；PHI 则是在 CFG entry 声明、
  在前驱赋值。

这套规则的要点是"保守但可用"：能折叠的只在单使用同块时折叠；有内存语义疑问（load）
一律物化；其余一律物化，从而保证 SSA 的一次性求值语义。

## 目标与预期效果

- private helper 输出成 Solidity 函数，签名去掉 4 个运行时指针参数
  （`%mem/%calldata/%returndata/%env`，在 Solidity 里是隐式的）；
- helper 调用点输出 `T v = helper(args);` 或折叠成表达式；
- SSA value 的"表达式 vs 局部变量"判断按上面 C 后端的流程实现；
- 量化目标（重跑四套 suite 后确认）：
  - `notdec.evm.solidity_pattern_compile`：0 编译失败，预算只降不升（升高必须在本日志说明）；
  - `notdec.evm.solidity_source` 不退化；
  - "声明了返回值却没有 return"的函数（基线 16）尽量降到 0；
  - unresolved 计数不因为输出 helper 体失控：只输出能完整渲染的 helper 体，否则保持现状。

## 技术路线

1. **判断流程**：在 Solidity 后端加一个小的 SSA 使用分析，等价移植
   `hasOneUseIgnoreCast` / `onlyUsedInCurrentBlock` / load-不折叠 三条规则，
   对每个 value 给出 `Fold | Cache` 决策。
2. **局部变量**：`collectStatements` 在定义点插入
   `VariableDeclarationStatement{uint256 temp_N = <expr>}`，并登记 value→名字映射；
   `valueExpr` 优先查该映射，命中返回标识符。命名要稳定，且不与
   `slot_N`/已恢复参数名冲突。
3. **helper 函数输出**：Reader 收集被调用到的 `private__*` 函数（传递闭包），
   跳过前 4 个运行时指针参数生成 Solidity 签名，用同一套 BodyBuilder 生成函数体；
   函数体不能完整渲染（含 unresolved，或依赖 calldata/memory 模型）时先跳过整个 helper。
4. **调用渲染**：值调用 → 表达式或局部变量初始化；void 调用 → 表达式语句；
   ABI 解码等已经有 `notdec.solidity.calldata.index` metadata 恢复的调用保持原样，
   避免同一语义两套表达。
5. **测试**：新增/更新 `test/evm/solidity-source/` golden（先用 `0011_multi_public`
   里的 `private__0x1b7_0x1b7`：由 sload/balance/checked sub 组成，现有能力已能完整渲染）；
   pattern suite 增加 helper 数与调用数 oracle；compile suite 预算棘轮按实际结果下调。

## 风险

- **作用域**：局部变量声明在某个分支块、使用在 join 之后会越界。SSA 支配性保证不会出现
  "未定义路径"，但结构化渲染可能把声明放进 `if` 花括号里。先用 solc 全量验证，
  必要时把跨块局部变量的声明提到函数入口（C 后端对 PHI 就是这么做的：
  在 CFG entry `prependStmt`，在前驱赋值）。
- **helper 体质量**：helper 体内可能有 calldata/memory/keccak 等当前无法表达的操作。
  策略是先只输出"能完整渲染"的 helper，其余保持现状，避免把 TODO 从调用点搬到函数体里
  造成 unresolved 计数虚高。
- **重复语义**：ABI 解码 helper 已经有 metadata 恢复路径，思路 B 不能给同一结果
  再生成一份 helper 调用。
- **递归/互相调用**：helper 之间可能互相调用，需要传递闭包与稳定命名。
- **指标波动**：输出 helper 会改变 unresolved / condition TODO 统计口径，
  棘轮值变化必须逐项说明原因，不能为了数字放宽规则。

## 判断标准

- 新增 source golden 通过，且 solc 编译通过；
- 四个 suite（pattern / rewrite / source / pattern_compile）全部通过，compile suite 0 失败；
- 至少一个真实 case 从"返回值 TODO"变成"helper 调用 + 局部变量"；
- 输出 IR 全部通过项目 LLVM 22 `llvm-as`；
- 记录耗时与暂不支持的形态。

## 暂不处理的形态

- helper 体内含 calldata 解码、memory 读写、keccak 计算 slot、external call 结果等
  当前没有表达能力的操作（保持调用点 unresolved）；
- 多返回值（LLVM struct return）helper；
- 递归 helper。


## 实现记录（2026-09-18）

### 改动文件与函数

`external/NotDec-llvm2c/include/notdec-backends/Solidity/BodyBuilder.h`

- 新增 `HelperRenderInfo` / `HelperRenderMap`：记录 helper 的 Solidity 名字和要丢掉的
  运行时参数个数（evm2llvm 固定前 4 个 `mem/calldata/returndata/env`）。
- `BodyBuilder::readBody` 增加 `const HelperRenderMap *Helpers` 参数。
- 新增 `BodyBuilder::blockIsFullyRecovered(const Block &)`。

`external/NotDec-llvm2c/include/notdec-backends/Solidity/Reader.h`、
`lib/Solidity/Reader.cpp`

- `isHelperRenderCandidate`：只接 `private__*`、前 4 参数是指针、其余形参 i256、
  返回 void 或 i256，且不含 `evm_signextend/evm_sdiv/evm_smod/evm_sar`。
- `collectHelperCandidates`：从 public 入口出发收集直接调用的传递闭包。
- `readHelperFunction` / `readHelperReturns`：签名去掉 4 个运行时指针，其余形参命名
  `argN`（uint256），`i256` 返回声明成 `uint256 ret0`，`Visibility = internal`。
- `readContract` 定点迭代：用当前可输出集合渲染全部候选，凡
  `!blockIsFullyRecovered` 的成批剔除后重试，直到稳定；被剔除 helper 的调用点保持原来的
  `unresolved` 行为，避免把 TODO 搬进函数体。
- public 入口用最终集合渲染，所以调用点与 helper 定义始终一致。

`external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp`

- `hasOneUseIgnoreCast` / `onlyUsedInCurrentBlock`：从 C 后端
  `StructuralAnalysis.cpp:addExprOrStmt` 的判断流程原样移植。
- `valueNeedsMaterialization`：helper 调用结果不是"唯一使用且同基本块"时就缓存；
  0 使用走调用语句；`LoadInst` 规则本轮未套用到表达式（见"计划调整"）。
- `renderedHelperCallExpr`：`call i256 @private__X(mem, calldata, returndata, env, a0…)`
  → `X(a0…)`；实参逐项做 `wordOperandExpr` 归一化，只要有一项 unresolved 就返回
  nullopt，调用点保持原 TODO。
- `readBody` 预扫描：为需要缓存的调用分配稳定的 `temp_N`（与参数名/状态变量名查重），
  入口块声明、定义点赋值；对应 C 后端 `VarDecls` 放函数开头、`cacheExpr` 在定义点赋值的
  做法。声明放函数作用域还避免了分支作用域与 dephication 复制导致的重复声明。
- `collectStatements`：入口块输出 `uint256 temp_N;`；定义点输出
  `temp_N = <call>;`（初始化必须直接取调用表达式，不能走 `valueExpr`，否则会渲染成
  自己）；void 或结果无人使用的 helper 调用输出调用语句。
- `valueExpr`：先查缓存表（命中返回局部变量名），CallInst 路径接上 helper 调用表达式。
- `wordOperandExpr`：word 运算符操作数与 helper 实参的 address/bool 归一化
  （`uint256(uint160(x))`、`(x) ? 1 : 0`）。
- i1 xor 渲染成 `(a) != (b)`（Solidity 没有 `bool ^ bool`）。
- `evm_balance(env, addr)` 渲染成 `address(uint160(addr)).balance`。
- `storageHelperExpr` / `storageRefExpr` / `storageContainerShapeForRef`：
  mapping/array 整槽读取没有可表达的 word 形式，返回 unresolved；但索引基址仍解析出变量名
  （否则 `slot_N[key]` 全丢）；`evm_sload` 也参与容器形状判断。
- `blockIsFullyRecovered`：新增语句遍历器，把 unresolved 值、condition TODO、
  `// goto` / switch / body TODO 注释都算作未完全恢复。

### 验证命令与结果

```bash
ninja -C build-notdec-nothreads notdec
ctest --test-dir build-notdec-nothreads -R "notdec.evm.solidity" --output-on-failure
```

- `notdec.evm.solidity_source`：88/88（新增 golden 见下）；
- `notdec.evm.solidity_patterns`、`notdec.evm.solidity_rewrite`：通过；
- `notdec.evm.solidity_pattern_compile`：0 编译失败，六项预算全部通过：
  unresolved 387→264，condition TODO 789→729，goto 424（不变），
  real jump 376→343（口径修正后），dangling 36→69（口径修正后）。
- 覆盖：77/103 个 case 现在输出 helper 函数，共 1234 个 `function private__*`；
  2124 个 `private__` 记号减去 1234 个定义，即约 890 个新的 helper 调用点。
- 样例：`0011_multi_public` 从 `return 0 /* TODO: unresolved value: private__0x1b7_0x1b7 */;`
  变成 `return private__0x1b7_0x1b7(245);` 并输出该 helper 的函数体。

新增测试：

- `test/evm/solidity-source/ir/private_helper_call_public_entry_01.ll` +
  `expected/private_helper_call_public_entry_01.sol` +
  `cases/private_helper_call_public_entry_01.sol` + manifest 条目。
  同一个 case 覆盖三条路径：单使用同块折叠（`slot_3 = private__sum_0x100(1, arg0);`）、
  多使用缓存（`uint256 temp_0; temp_0 = private__sum_0x100(5, arg0);`）、
  void helper 调用语句（`private__record_0x200(temp_0);`）。
- `test/evm/solidity-patterns/compile-budget.json` 与 `README.md` 更新。

### 指标口径修正

`count_goto_kinds` 原来在整份文件里收集 `// block_N` 标签，而每个函数的 block 编号都从 0
开始，跨函数同名标签会把 dangling 误判成 real。改成按函数体匹配后，baseline 与当前输出都是
total 424 / fallthrough 12 / dangling 69 / real 343，说明 helper 输出没有让 goto 结构退化；
预算据此下调 real（376→343）、上调 dangling（36→69，口径修正而非退化）。

### 计划调整与暂不支持

- 折叠/缓存判断本轮只作用于 helper 调用结果。纯算术与 evm 内建表达式仍按表达式折叠：
  它们无副作用，多个使用点重复打印语义等价；若后续要对 load 与跨块值统一启用，
  需要单独评估（C 后端对 `LoadInst` 一律不折叠，本轮没有套用）。
- helper 体必须完全恢复才输出（无 unresolved、无 TODO、无 goto）；否则调用点保持原状。
- 暂不输出：含 `evm_signextend/evm_sdiv/evm_smod/evm_sar` 的 helper（会渲染成 `intN(...)`，
  与 uint256 签名不匹配）；helper 体内的 calldata 读取、memory 读写、struct 返回。


### 收尾指标与耗时（2026-09-18）

- helper 覆盖作为**下限**写入 compile suite 预算：
  `min_helper_functions = 1339`、`min_helper_call_sites = 1301`
  （103 个 case，77 个输出 helper）。plan 里写的是"pattern suite 增加 helper 数 oracle"，
  实际放在 compile suite：pattern suite 只查 IR metadata，而 helper 计数是 `.sol` 属性，
  放这里才能和 solc 编译一起回归。
- "声明了返回值却没有 return" 的函数保持 baseline 的 16（8 个 case）。剩余的都是本轮
  明确跳过的形态（calldata 解码 helper、memory helper、signed 运算 helper、struct 返回），
  不是 Route B 引入的退化。
- 四套 suite 耗时（本机，同一 workdir）：
  patterns ≈28s、pattern_compile ≈33s、rewrite ≈6.6s、source ≈2.8s；
  改动前后在噪声范围内（baseline 分别约 31s / 35s / 6.9s / 3.0s），没有明显上涨。
- 输出 IR 的 `llvm-as` 检查由 pattern/rewrite suite 覆盖，四套全绿。

