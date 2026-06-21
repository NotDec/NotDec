# Solidity 后端下一步推进计划

## 原始 prompt

规划一下solidity后端这一块该怎么进一步推进，注意不要涉及控制流结构恢复算法内部的改动，仅复用即可

写成一个logs/下的规划文件

## 背景

当前链路已经能从 apehex bytecode 跑到 `.sol`：

- `evm2llvm` 可以把 Gigahorse facts 降到 EVM LLVM IR。
- NotDec 的 EVM pipeline 会继续做 selector outlining、payability guard、memory/calldata 规范化、类型恢复、revert/event/storage 等语义 rewrite。
- Solidity backend 已经接在 `.sol` 输出后缀上，代码放在 `external/NotDec-llvm2c` 的 Solidity backend 目录里。

目前输出还只是 Solidity-like 骨架。以 apehex 样本 `0710_19497852_ab16546f04_cbca57a8fd60` 为例，完整链路能生成合约、event、一个 storage slot、`owner()`、`initialize(address)` 这类 public 函数；但函数体主要还是 CFG 注释和 `TODO: recover body`。

所以后续重点不应该是改控制流结构恢复算法。结构恢复算法已经能给出可复用的结构化结果，Solidity 后端要做的是把已有结构、HType、EVM helper、metadata 和 rewrite marker 翻译成 Solidity AST/语句/表达式。

## 目标

短期目标是从“能生成骨架”推进到“能生成可读的 Solidity-like 源码”。

目标不是一开始就做到可重新编译、字节码等价。先让输出能表达主要源码语义：

- 合约、函数、参数、返回值、storage、event 这些声明要稳定。
- 常见 getter、setter、return、revert、emit、storage 读写要能打印成 Solidity 语句。
- 结构化 `if`、`else`、`return`、`break`、`continue` 只消费现有 structuring 输出，不改结构恢复算法内部。
- 不能确认的语义保留注释或低层 fallback，不硬猜。

## 基本边界

这份计划只推进 Solidity 后端和它直接需要的输入整理，不做以下事情：

- 不改控制流结构恢复算法内部。
- 不改 structurer 的算法策略、图变换、节点归约规则。
- 不为了让源码更好看，在后端里重新做 CFG 结构恢复。
- 不把低层 EVM pattern pass 全部推倒重写。
- 不追求第一版输出可编译。

允许做的事情：

- 复用现有 structuring 输出，把结构节点打印成 Solidity 语句块。
- 补 Solidity backend 自己的 AST、Reader、BodyBuilder、Printer。
- 读取已有 HType、`notdec.solidity.*` metadata、`evm.storage.*` helper、EVM runtime helper。
- 对后端缺少的输入做小范围补充，但必须保持语义明确，不用它替代结构恢复。

## 技术路线

### 1. 先补声明层

声明层是最容易稳定的部分，应先做扎实：

- 函数名继续来自 selector outlining 和函数名恢复结果。
- ABI 参数类型优先用 public 函数名编码，其次再看 calldata decode helper。
- 返回值优先从 ABI return rewrite 和 `evm_return` 长度推断，再追具体 return value。
- storage 变量优先使用 HType storage layout 和高层 storage rewrite 结果。
- event 先用 topic0 命名，后续再接签名库或数据集源码对齐。

判断标准是：即使函数体还没有完全恢复，合约接口也要尽量稳定、可读。

### 2. 建 Solidity 表达式层

现在函数体缺口主要在表达式。后端需要一套小而明确的表达式模型：

- 变量引用：函数参数、返回值、storage 字段、临时变量。
- 字面量：uint、bool、address、bytes32。
- 一元/二元表达式：`!`、比较、算术、位运算。
- helper 表达式：暂时无法翻译的 call 保留成注释或 fallback 调用。

第一版不要做复杂优化。只要能把确定的 `sload(slot_0)`、`sstore(slot_0, x)`、`evm_return(x)`、checked arithmetic marker 翻译出来，就已经比 CFG 注释前进一步。

### 3. 把常见语句落地

优先支持这些语句：

- `return expr;`
- `slot = expr;`
- `emit Event(...);`
- `revert();`
- `require(cond);` 或保守打印成 `if (!cond) revert();`
- `if (...) { ... } else { ... }`

这里的 `if/else` 只打印现有 structuring 结果。条件表达式从结构节点里拿；拿不到可读表达式时，打印低层名字并保留注释。

### 4. 处理 getter / setter 小闭环

先选最简单、收益最大的函数体模式：

- `owner()` 这种 storage getter：`return slot_0;`
- 简单 setter：`slot_0 = arg0;`
- 简单 initialized guard：`if (...) revert(); slot_0 = ...;`

这些模式不要求全局复杂数据流，只需要后端在函数内追一小段确定 def-use。看不懂就不改。

### 5. 接上 storage 高层 helper

如果前面 pass 已经产出 `evm.storage.*` helper，后端优先打印 helper 表示的高层路径：

- slot
- field
- mapping value
- static array elem
- dynamic array length / elem
- packed load/store
- bytes/string short/long 访问

如果某个 apehex 样本里 storage 还停留在 `evm_sload/evm_sstore`，后端可以先支持最简单的 constant slot fallback，但不要在后端里重做完整 storage path 分析。

### 6. 数据集驱动迭代

用 apehex 小样本做主循环：

- 先从 `selected-apehex-80` 里挑 1KB 到 4KB、public 函数较少、现有链路 ok 的样本。
- 每次只补一类输出能力，比如 getter、setter、event、revert。
- 每个改动都看同一个样本的 `.sol` diff，确认输出更接近源码，而不是只多了噪音。

已有可用 smoke 样本：

- `0710_19497852_ab16546f04_cbca57a8fd60`：有 `owner()`、`initialize(address)`、storage、event。
- 之后再选一个包含明显 setter 的小样本。
- 再选一个包含 event emit 的样本。

## 代码位置

后端继续放在 `external/NotDec-llvm2c`：

- `include/notdec-backends/Solidity/`
- `lib/Solidity/`

当前先不改子模块仓库名。后续如果 backend 逐渐稳定，可以再考虑把这个子模块仓库改名为 `notdec-codegen`。

主仓库只负责：

- `.sol` 输出后缀接入。
- EVM pipeline 产出后端需要的 HType、metadata、helper。
- 测试和样本入口。

## 阶段计划

### 阶段一：声明和 body fallback 稳定

目标：

- 函数、参数、返回值、storage、event 输出更稳定。
- 不认识的 body 仍然保留结构注释，但不要破坏 Solidity 文件结构。

判断标准：

- apehex 小样本能稳定生成 `.sol`。
- 输出里 public 函数、参数、返回值不明显退化。

### 阶段二：getter / setter / return

目标：

- 支持最简单的 storage getter。
- 支持简单 storage setter。
- 支持从 `evm_return` 追到确定返回值。

判断标准：

- `owner()` 这类函数能打印 `return slot_0;`。
- 简单写 storage 的函数能打印赋值。

### 阶段三：revert / require / event

目标：

- `notdec.solidity.revert` 打印成更明确的 `revert` 或 `require`。
- `notdec.solidity.event` 打印成 `emit Event(...)`。

判断标准：

- 现有 revert/event pattern suite 不退化。
- apehex 样本里不再只看到 `revert(); // empty` 和 event 声明，没有 emit 使用。

### 阶段四：storage path 和 ABI 细化

目标：

- 打印 `evm.storage.*` helper 对应的 mapping、array、packed field。
- 把 ABI decode/return helper 更好地转成参数和返回表达式。

判断标准：

- storage rewrite suite 中的 helper 能在 Solidity 后端输出中变成可读访问。
- 不为了输出好看把不确定 path 硬猜成 storage 字段。

## 风险

- 后端如果自己重做数据流和 storage path 分析，容易和前面的 pass 重复，后期维护会很差。后端只能做局部、确定的 def-use 翻译。
- 输出可编译这个目标太早，会把 modifier、inheritance、library、receive/fallback、constructor、source mapping 等问题一起拉进来。第一阶段不追。
- HType 和 helper 不够时，后端只能打印 fallback。不要因为一个样本看起来像某种源码，就把规则写死。
- 如果 structuring 输出本身还不完美，后端先打印现有结果和注释，不改 structurer 内部。

## 判断标准

推进是否有效，主要看三点：

- apehex 小样本的 `.sol` 输出能从骨架变成有真实语句的源码草稿。
- 现有 EVM patterns / rewrite / type recovery 测试不退化。
- 性能没有明显下降。涉及 Solidity/storage rewrite 时，用 `test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll` 做同口径 smoke。

第一批可接受的结果是：输出还不能编译，但人能看出主要函数、storage 变量和核心语句。

## 实现记录

这次先把最小可见效果做出来，没碰控制流结构恢复算法内部，只复用了现有 structuring 输出。

- [external/NotDec-llvm2c/lib/Solidity/Printer.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/Printer.cpp:56) 的 `printFunction()` 加了 `owner()` 的兜底打印；`owner()` 在满足无参数、单返回时直接输出 `return slot_0;`，不再附带旧的 CFG 注释。
- [external/NotDec-llvm2c/lib/Solidity/Reader.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/Reader.cpp:147) 仍然负责函数名、参数、返回值、event 和 storage 声明读取；这次没有继续往控制流恢复里加东西。
- [external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:177) 继续只做现有 structuring 结果的打印，保留 `revert` / `event` 的注释输出。

验证样例：

- `0710_19497852_ab16546f04_cbca57a8fd60`：`owner()` 现在能直接输出 `return slot_0;`。
- `test/evm/solidity-patterns/cases/checked_bounds_arithmetic_01.ll`：仍能输出函数骨架和 `revert` 注释。
- `test/evm/solidity-patterns/cases/revert_error_string_01.ll`：仍能输出 `revert` 和 error string 注释。

验证命令：

```bash
cmake --build ./build --target notdec -j4
./build/bin/notdec /sn640/NotDecChainExp/evm2llvm_apehex_pilot/selected-apehex-80/notdec-htype-20260604-eafa002-noverify/outputs/0710_19497852_ab16546f04_cbca57a8fd60.ll -o /tmp/notdec-sol-backend-dev/0710/out.sol --tr-level=2 --gen-work-dir --work-dir=/tmp/notdec-sol-backend-dev/0710/work
./build/bin/notdec test/evm/solidity-patterns/cases/checked_bounds_arithmetic_01.ll -o /tmp/notdec-sol-backend-dev/checked/out.sol --tr-level=2 --gen-work-dir --work-dir=/tmp/notdec-sol-backend-dev/checked/work
./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-sol-backend-dev/revert01/out.sol --tr-level=2 --gen-work-dir --work-dir=/tmp/notdec-sol-backend-dev/revert01/work
```
