# EVM Solidity 后端：calldata guard 恢复、direct-slot store 与 require 折叠

## 用户原始 prompt

下面的修复bin2llvm里面的evm链路的任务做到一半了，看看怎么接上继续：

用一个很小的例子来讲：calldata_min_size_public_entry_01。这个 case 正好把当前 Solidity 后端最典型的几个缺口都暴露出来了：条件恢复、calldata 参数恢复、storage store、控制流收尾。

一、当前生成结果
现在这个 case 已经能被 solc 编译：

contract Decompiled {
    uint256 public slot_0;

    function calldata_min_size(uint256 arg0) public {
        // block_0:
        if (false /* TODO: too.short */) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // empty
        }
        // TODO: recover remaining body
    }
}
但从语义上看，它几乎什么都没恢复。

对应最终 IR 简化后是：

entry:
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %too.short = icmp ult i256 %size, 36
  br i1 %too.short, label %revert, label %body

body:
  %calldata.checked = call ptr @notdec_evm_calldata_min_size(ptr %calldata, i256 36)
  %base = ptrtoint ptr %calldata.checked to i256
  %addr = add i256 %base, 4
  %ptr = inttoptr i256 %addr to ptr
  %word.load = load i256, ptr %ptr
  %0 = call i256 @evm.storage.slot(i256 0)
  call void @evm.storage.store(i256 %0, i256 %word.load)
  ret void

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable

也就是真实逻辑是：

function calldata_min_size(uint256 arg0) public {
    if (msg.data.length < 36) {
        revert();
    }
    slot_0 = arg0;
}

更 Solidity-like 的最终形态可以是：

function calldata_min_size(uint256 arg0) public {
    require(msg.data.length >= 36);
    slot_0 = arg0;
}

所以下一步不是“让它能编译”，而是把它从占位输出推进到真实语义。

二、差距拆解
这个例子可以拆成 5 个具体改进点：

1. 条件没有恢复。%too.short = icmp ult %size, 36 是完整可翻译的，但 getCondition() 直接把它包成 TodoConditionExpr，打印 false /* TODO */。
2. calldata 单词读取没有恢复。notdec_evm_calldata_min_size(calldata, 36) + ptrtoint/add 4/inttoptr/load 实际就是 ABI 参数 arg0，但现在 LoadInst 走了 unresolved fallback。
3. storage store 被关闭。evm.storage.store(slot(0), word.load) 本来是 slot_0 = arg0;，但 formatStorageStore() 目前直接返回 nullopt。
4. 分支极性/控制流还没闭环。当前输出里 true 分支是空的、false 分支是 revert；而 IR 是 too.short == true 时进入 revert。恢复条件表达式后必须同时确认/修正 branch polarity，否则会生成语义相反的代码。
5. TODO / 空 block 收尾。// TODO: recover remaining body 和空的 // block_1: 应该在主要语句恢复完后消失；简单 guard 可以进一步折叠成 require(...)。

三、逐层怎么改

第 1 步：evm_calldatasize -> msg.data.length
文件：external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp 的 evmEnvBuiltinExpr()。
加一个 evm_calldatasize 分支返回 msg.data.length。

第 2 步：getCondition() 用 typed expression，而不是永远 TODO
文件：BodyBuilder.cpp 的 SolidityPayloadProvider::getCondition()。
先 valueExpr(V)，如果没有 unresolved 就用它，否则退回 TodoConditionExpr。
需要一个递归的 containsUnresolvedValue(ExprPtr)：如果表达式里还有 0 /* TODO: unresolved value */，就退回原来的 TODO condition，避免把假条件伪装成真条件。

第 3 步：把 calldata load 认成 arg0
Reader 已经根据函数名 calldata_min_size_uint256 声明了 function calldata_min_size(uint256 arg0) public。
BodyBuilder 里已经有 ActiveArgumentNames，只是 %word.load 还没被认成 arg0。
IR 形状：
  %calldata.checked = call ptr @notdec_evm_calldata_min_size(%calldata, 36)
  %base = ptrtoint %calldata.checked
  %addr = add %base, 4
  %ptr  = inttoptr %addr
  %load = load i256, %ptr
可以写一个 matcher：matchCalldataArgumentIndex，校验 load/inttoptr/add/ptrtoint/notdec_evm_calldata_min_size 链，offset>=4 且 (offset-4)%32==0，返回 (offset-4)/32，再到 valueExpr() 的 unresolved fallback 之前映射成 ActiveArgumentNames[kRuntimeArgs+i]。
更工程化的做法是在 EvmCalldataAccessPass 匹配到 guard 时给后面的 calldata load 写 metadata notdec.solidity.calldata.index = i，Solidity 后端只读 metadata。推荐长期用 metadata 方案，短期可以先用 BodyBuilder-local matcher 验证。

第 4 步：打开简单的 storage store
文件：BodyBuilder.cpp 的 formatStorageStore()。
只放开最安全的形状：evm.storage.store 或 evm_sstore；LHS 是 direct slot，不是 mapping/array；RHS 表达式不含 unresolved fallback。mapping/array/packed store 仍然先不动。

第 5 步：修掉 branch polarity
当前输出 if (false /* TODO: too.short */) { block_1 } else { block_2: revert }，而 IR 是 too.short 为真时应该进 revert。问题在 lib/Structuring/LLVMFunctionCFGBuilder.cpp 的 conditionCompareFromICmp() 附近：代码注释写着 TrueTargetIndex = 1，即约定 successor #1 是 true 边；但 LLVM BranchInst 的 successor 顺序需要和 Block.Successors/TrueTargetIndex 统一。这个例子正好可以作为最小回归。

第 6 步：收尾 TODO / 简化成 require
readBody() 里目前会在最后追加 // TODO: recover remaining body。等简单 store 恢复后，可以扩展 isTerminalStatement()：如果最后一条是 IfStatement，且 Then/Else 都存在并且分支内部都以 revert/return 终止，就认为整个函数也终止；不再追加 TODO。再进一步，可以在 Solidity 后端加一个小优化：if (cond) { revert(); } 且没有 else/payload 时，打印成 require(!cond)。对这个例子就是 require(msg.data.length >= 36); slot_0 = arg0;

四、这个例子改完后的目标输出

contract Decompiled {
    uint256 public slot_0;

    function calldata_min_size(uint256 arg0) public {
        require(msg.data.length >= 36);
        slot_0 = arg0;
    }
}

五、实施顺序和建议
1. 先加 evm_calldatasize -> msg.data.length（很小，安全）。
2. 加 containsUnresolvedValue() + 改造 getCondition()（小，收益大）。
3. 修 branch polarity（必须先写测试锁死 IR 方向，避免全局控制流翻车）。
4. 加 calldata argument metadata 或 matcher（中等，本案例最关键）。
5. 对 direct slot / 非 array/mapping / RHS resolved 的 store 放开 formatStorageStore()（小）。
6. 扩展 terminal 判断和 require 折叠（小，收尾）。

这个例子跑通后，PR/commit 可以切成很小的几步：
- solidity: lower evm_calldatasize and typed conditions
- evm: annotate calldata argument loads
- solidity: enable direct-slot stores
- solidity: simplify guard-if-revert to require

最后把目标 .sol 作为 golden，加一条 solc --bin 编译检查，就能把这类语义改进稳定锁住。


## 接力时的状态

上一轮（llvm2c 86f52de / 顶层 8f8590b3，见
logs/20260917-02-solidity-storage-helper-mapping.md）已经完成 storage helper
表达式映射，但 Solidity 后端工作区里还留着一批未提交的下一轮改动：

- `evm_calldatasize -> msg.data.length`；
- `getCondition()` 改用 typed expression（`containsUnresolvedValue()` 兜底）；
- `matchCalldataArgumentIndex()` 把 checked calldata 单词读取映射回 ABI 参数；
- 直接 slot 的 `formatStorageStore()` 重新打开；
- `LLVMFunctionCFGBuilder` 的 branch successor 极性修正；
- 附带 `ParameterTypeMap` / address cast / `wordifyOperand` / `evalConstantWord`。

用该状态跑 `calldata_min_size_public_entry_01` 得到：

    contract Decompiled {
        uint256 public slot_0;

        function calldata_min_size(uint256 arg0) public {
            // block_0:
            if (msg.data.length < 36) {
              // block_2:
              revert(); // empty
            }
            else {
              // block_1:
              slot_0 = arg0;
            }
            // TODO: recover remaining body
        }
    }

即第 1-5 步已经落地且极性正确，剩下第 6 步（guard 折叠成 require、去掉
过期的 remaining-body 标记），并且这批改动还没有被完整回归过。

## 本轮改动

### 1. guard-if-revert 折叠成 require

`lib/Solidity/BodyBuilder.cpp`

- 新增 `soleNonCommentStatement()`（忽略 block label 注释）、
  `invertedComparisonOperator()`、`negateConditionExpr()`、
  `guardRevertRequire()`。
- `renderStructuredNode()` 的 `StructuredNodeKind::If` 分支在渲染完 Then/Else
  后尝试折叠：只有一个分支是纯 guard revert 时，输出
  `require(!cond)`（Then 是 revert）或 `require(cond)`（Else 是 revert），并把
  另一个分支的语句原地内联。
- 只折叠 payload 为空的 revert（`Comment == "empty"`）和已恢复出字面量的
  `require(false, "...")`。panic / custom error / returndata bubble 的 revert
  保留原 if，避免用 require 丢掉错误数据。
- 条件取反优先反转比较运算符（`<` -> `>=` 等），其它条件退回 `!(...)`；
  双重否定直接消掉。

### 2. 去掉过期的 remaining-body 标记

`lib/Solidity/BodyBuilder.cpp`

- 新增 `BodyCompletion` + `analyzeBodyCompletion()`：递归分析结构化树，用
  Goto 节点判断是否还有无法结构化的跳转，用最后一个 BasicBlock 对应 CFG
  块的 `Terminator == Return/Unreachable` 判断是否所有路径都到函数出口。
  这里必须读 CFG terminator，因为 PhoenixStructurer 只在部分路径生成显式
  Return/Unreachable 节点（本次用 `NOTDEC_SOLIDITY_TREE=1` 的临时 dump 确认过
  calldata case 的树里没有这两种节点）。
- `readBody()` 只在 `HasGoto || !ReachesExit` 时才追加
  `// TODO: recover remaining body`。恢复出的赋值语句不是 terminal，但函数
  末尾的隐式 return 是，所以不能再只看最后一条语句。

### 3. 回归修复：i1 常量打印成 bool

`formatInteger()` 对 1-bit 值返回 `true`/`false`。typed condition 打开后，
pattern case `24541` 里一个常量 i1 条件被打印成 `if (0)`，solc 报
`Type int_const 0 is not implicitly convertible to expected type bool`。

### 4. 回归修复：address 下标显式转 uint256

`storageHelperExpr()` 的 `evm.storage.map.value` / `array.elem` 两处
`IndexAccessExpr` 对下标调用 `wordCastAddressExpr()`。
`slot_0[msg.sender]` 之前打印出 address 下标，而 `TypePrinter` 恢复出的
mapping/array 类型只会是整数或数组，solc 报
`Type address is not implicitly convertible to expected type uint256`。

### 5. 清理

- 删除上一轮变成 dead code 的 `sanitizeSolidityIdentifier()` /
  `llvmValueName()`（raw SSA 值现在统一走 `UnresolvedValueExpr`），构建不再有
  unused-function warning。

### 6. 测试

- `test/evm/solidity-source/` 新增 `calldata_min_size_public_entry_01`
  （cases / ir / expected / manifest），golden 即本轮目标输出。
- `run_evm_solidity_source_suite.py` 新增可选 `--solc`：
  给出 solc 时每个生成的 `.sol` 都要通过 `solc --bin`，日志里记录 solc 段。
- `test/CMakeLists.txt` 新增 `NOTDEC_SOLC` cache 变量，配置时给出才会启用；
  默认仍然不依赖 solc。
- `test/evm/solidity-source/README.md` 记录启用方式。
- `apehex_simple_emitter_01`、`apehex_gasleft_return_01` golden 去掉过期的
  `// TODO: recover remaining body`（两份输出结构上都已经到达函数出口）。

## 验证

```bash
cmake -S . -B build-notdec-nothreads -DNOTDEC_SOLC=/sn640/EthIR/source/solcv8.25
ninja -C build-notdec-nothreads notdec
ctest --test-dir build-notdec-nothreads -R "notdec.evm" --output-on-failure
ctest --test-dir build-notdec-nothreads -R "notdec.type_recovery.evm|notdec.lifting.wasm" --output-on-failure
```

结果：

- `notdec.evm.solidity_patterns` 25.12s 通过；
- `notdec.evm.solidity_rewrite` 6.24s 通过；
- `notdec.evm.solidity_source` 2.47s 通过，79/79 都通过 `solc --bin`；
- `notdec.type_recovery.evm.tr_level_2`、`notdec.lifting.wasm` 通过。

103 个 pattern case 的 solc 审计：

```bash
for case in test/evm/solidity-patterns/cases/*.ll; do
  build-notdec-nothreads/bin/notdec "$case" -o /tmp/out.sol --tr-level=2
  /sn640/EthIR/source/solcv8.25 --bin /tmp/out.sol
done
```

- HEAD（86f52de）基线：103/103 可编译；
- 仅接力工作区状态：99/103（上面第 3、4 项回归）；
- 本轮修复后：**103/103 可编译**。

`calldata_min_size_public_entry_01` 当前输出：

    contract Decompiled {
        uint256 public slot_0;

        function calldata_min_size(uint256 arg0) public {
            // block_0:
            require(msg.data.length >= 36);
            // block_1:
            slot_0 = arg0;
        }
    }

与 prompt 的目标一致（block label 注释是本后端所有输出都保留的既有形式）。

## 说明与后续

- polarity 修正的影响面：`LLVMFunctionCFGBuilder` 只被 Solidity 后端
  （`BodyBuilder`）和 structuring 单测使用；C 后端 `StructuredGoto` 走自己的
  `buildCFG()`，不受影响。成功路径由 103 个 pattern + 79 个 source case 覆盖。
  standalone `external/NotDec-llvm2c/build` 的测试 target 依赖主项目的
  `notdec` target，独立配置会失败（历史遗留，本次未处理），因此
  `structuring-analysis-test` 本轮没有单独运行。
- calldata 参数映射仍是 BodyBuilder-local matcher，长期应改成
  `EvmCalldataAccessPass` 写 `notdec.solidity.calldata.index` metadata，后端只读
  metadata。
- 后续同族问题：selfdestruct 仍未恢复（gasleft kill() 的 then 分支为空）、
  private call 内联、dynamic ABI return。

