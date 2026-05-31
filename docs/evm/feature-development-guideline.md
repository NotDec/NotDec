# EVM Solidity 功能开发规范

这份文档总结 `SelectorEntryOutliningPass`、`PayabilityGuardPass`、
`SolidityRevertPass` 的开发过程，作为后续新增 Solidity 底层模式功能的参考。

重点不是“先写 pass 再补测试”这么简单。之前几个功能的经验是：主体逻辑完成后，真正
容易出问题的是边界、oracle、真实 case 和测试框架是否跟上。

## 总体流程

一个新的 EVM Solidity 功能建议按这个顺序推进：

1. 明确 Solidity codegen 依据。
2. 定义当前明确支持的稳定形状，以及必须跳过的边界。
3. 先写结构化 matcher，不要把识别、标注、rewrite 混在一团。
4. 用 matcher 结果驱动 metadata、测试 oracle 和对外 rewrite。
5. 把测试框架补到能检查新语义，而不是只看总数。
6. 引入真实 case，必要时保留对应 Solidity 源码。
7. 跑 pattern suite、rewrite suite、`llvm-as` 和性能对比。
8. 记录暂不处理的形态，不能靠放宽规则硬拆。

## 先定依据和范围

每个功能开始前，先回答三个问题。

第一，Solidity 为什么会生成这段底层代码。

例如 payability 不是从样例里猜出来的，而是对照 Solidity codegen 里稳定的：

```text
if callvalue() { revert(0, 0) }
```

revert 也先对照 Solidity 的 Panic、Error、custom error、returndata bubble 生成逻辑，
再决定 matcher 优先级。

第二，当前明确支持哪些形态。

`PayabilityGuardPass` 当前支持直接形态：

```text
callvalue == 0 ? success : revert(0, 0)
callvalue != 0 ? revert(0, 0) : success
```

中间 block、复杂 bool 链、非空 revert 不在这条规则里。它们不是靠猜来处理，而是等有
真实 case 和明确判断标准后再扩展算法。

第三，哪些情况必须跳过。

`SelectorEntryOutliningPass` 的经验很明确：复杂 region 宁可跳过，也不能拆错。
多入口、live-out、外部 successor、共享 tail、PHI incoming 都要先变成明确规则。
后续每次放宽，都要用真实 case 和 oracle 证明。

## 主体逻辑之后要补什么

主体 matcher 或 rewrite 写完，只算完成一半。还要补这些东西。

## 结构化结果

不要只在遍历里直接打 metadata。应该先抽一个小结构保存识别结果。

`PayabilityGuardPass` 用 `PayabilityGuardMatch` 保存：

- guard block
- success block
- failure block
- `evm_callvalue`
- `icmp`
- branch

`SolidityRevertPass` 用 `SolidityRevertMatch` 保存：

- `evm_revert`
- kind
- selector
- panic code
- returndata copy
- Error(string) 长度和 literal
- custom error 静态参数个数

好处是后续 metadata、marker、CFG rewrite、oracle 都复用同一份判断，不会出现两套 matcher
互相打架。

## 明确 marker 和 metadata 分工

metadata 适合 debug、统计和测试锚点。marker 更适合作为 rewrite 或后续 lowering 的稳定接口。

但要注意：对于明确支持 rewrite 的 pass，只打标记、不改写原始低层操作，只能用于内部测试、
调试和过渡验证。对外输出不能长期同时保留原始低层操作和额外高层标记。这样会让同一段语义出现
两份表达：一份是低层 EVM helper / CFG，一份是恢复出的高层语义，后续 consumer 很难判断该信谁。

正确目标是：

- metadata 用来确认 matcher 命中、辅助 oracle 和排查误判。
- marker 表达 rewrite 后保留下来的高层语义。
- 原始低层操作能安全替换时就替换，不能安全替换时就跳过，不要对外输出“低层操作 + 高层标记”的冗余语义。
- 如果某类模式暂时只能打标记，文档和日志里要明确它还不是对外完成状态。

例子：

```text
notdec_solidity_cfg_rewrite_payability_guard(...)
notdec_solidity_rewrite_revert_panic(code)
notdec_solidity_rewrite_revert_returndata_bubble(...)
notdec_solidity_rewrite_revert_error_string(selector, string_length)
notdec_solidity_rewrite_revert_custom_error(selector, static_arg_word_count)
```

不要长期只保留“这个低层 helper 可以隐藏”这种弱 marker。marker 要表达当前已经恢复出的
关键语义，并且服务于实际 rewrite 后的语义表示。

## rewrite 要成为对外接口

支持 rewrite 的 pass，对外接口应当是 rewrite 后的 IR，而不是 metadata-only IR。

metadata-only 可以存在，但定位要清楚：

- 内部调试。
- 统计覆盖率。
- 给测试 oracle 做锚点。
- 在算法还没准备好 rewrite 时，短期观察 matcher 的误报和漏报。

一旦一个模式进入 rewrite 范围，就要尽量做到：

- 原始编译器低层保护、buffer 写入、helper 序列或 CFG 被删除、替换、outline，或者被后端明确隐藏。
- 高层 marker / intrinsic / helper 表达恢复出的语义。
- 测试检查 rewrite 结果，而不是只检查 metadata 存在。
- 未能证明安全的 case 直接跳过 rewrite，并通过 skipped reason 或 oracle 暴露原因。

保守 rewrite 和效果覆盖不是矛盾的。之前 selector outline 的过程就是靠不断加入真实 case，
逐步修正算法边界：单入口、region input、多个 region、共享 tail / PHI、空 reject 出口。
每一步都只放宽一个明确规则，同时扩充测试集。这样可以保持 rewrite 足够保守，又能在数据集上
取得越来越好的覆盖。

## CFG rewrite 要保守

`PayabilityGuardPass` 的 CFG rewrite 只做最小改动：

```text
conditional branch -> unconditional branch to success block
```

它不绕过 guard block，也不急着删 failure block。这样 PHI 和 dominance 风险低，死代码交给后续
cleanup。这里的重点是：对外控制流已经不再走 nonpayable 的低层 reject 分支，而不是只在原分支旁边
加一个标记。

`SelectorEntryOutliningPass` 更复杂，所以每次只放宽一种边界：

- 支持单入口、无 live-out 的 region。
- 支持 region input 作为 helper 参数。
- 支持多轮重扫，同一个 selector 拆多个 helper。
- 支持共享 tail / PHI 的窄形态。
- 支持空 reject 出口映射。

每次放宽都要有对应真实 case。不能为了提高 outline 数量，把 dispatcher 判断放宽到会误判业务分支。

## 测试用例怎么引入

测试用例分三类。

## 现有真实 IR case

第一优先使用 `test/evm/solidity-patterns/cases/` 里的真实 evm2llvm IR。

之前覆盖扩展是从 apehex 运行结果里筛 case，放进 manifest，并用 `patterns` 字段标注覆盖类别。
覆盖统计按 case 算，一个 case 对同一类只算一次，不按 metadata 命中次数重复算。

目标是每类至少 50 个 case。之前 58 个真实 case 覆盖了 selector、payability、revert、
ABI、storage、event、external call 等大类。

## rewrite 专项 case

结构性 rewrite 不适合只放在 pattern suite 里看 metadata 数量。

`SelectorEntryOutliningPass` 新增了 `test/evm/solidity-rewrite/`：

- 检查 outline helper 数量。
- 检查 selector 里 outline call 数量。
- 检查 outlined metadata。
- 检查 skipped reason。
- 每个输出都过 `llvm-as`。

开始只有少数代表 case，后面扩到当前 58 个 pattern case，再继续加入真实 fallback、
delegatecall、共享 tail、空 reject 出口样例。

## Solidity-generated case

当真实 case 里缺某种语义，或者手写 IR 不能代表真实 codegen 时，要补 Solidity 源码和生成后的 IR。

`SolidityRevertPass` 的 Error(string) / custom error 就是这样推进的：

1. 用最小 IR case 验证 matcher、marker 和 oracle 能力。
2. 替换为 Solidity 0.8.26 生成的 runtime bytecode 转换出来的 IR。
3. 保留 `.sol` 源码，CTest 仍只跑稳定 `.ll`。

没有把 Solidity 编译接进 CTest，是因为生成链路依赖 `npx solc`、Gigahorse 和 evm2llvm，
不适合放进常规测试。

## 测试框架怎么完善

新增功能时，不要只改 manifest。runner 也要跟着升级，让 oracle 能表达新语义。

## pattern suite

`test/run_evm_solidity_patterns_suite.py` 的职责是：

- 运行 notdec。
- 对输出 `.ll` 跑项目 LLVM 22 的 `llvm-as`。
- 按 manifest 检查 metadata、marker 和语义计数。
- 失败时输出 expected / actual。

之前逐步加过这些能力：

- metadata 计数。
- rewrite marker 计数。
- hidden metadata 计数。
- payability CFG rewrite marker 计数。
- revert kind 计数。
- panic code 计数。
- returndata bubble 计数。
- Error(string) 长度和 literal oracle。
- custom error 参数个数 oracle。
- marker 参数对 oracle。

新增功能时，优先让 runner 检查“这个功能真正恢复出的语义”，而不是只检查某个 metadata 总数。

## rewrite suite

`test/run_evm_solidity_rewrite_suite.py` 的职责更偏结构检查。

selector outline 用它检查：

- 是否生成 `notdec_solidity_selector_inline.*` helper。
- selector 函数里是否有对应 call。
- outline / skipped metadata 是否符合预期。
- skipped reason 是否符合预期。

如果后续功能也会明显改 CFG 或搬移代码，应考虑加 rewrite suite 或扩展现有 rewrite suite。
如果只是内部 matcher 观察或 metadata 调试，先放 pattern suite 通常够用。只要该功能声明支持
rewrite，就需要有测试检查 rewrite 后的结构或 marker 语义，不能只看 metadata。

## oracle 的粒度

只看总数不够。revert 的开发过程里，先补了 kind、panic code、returndata bubble oracle，
再改 pass。原因是如果只看 `notdec.solidity.revert` 总数，matcher 命中错 kind 也可能通过。

建议粒度：

- 第一层：metadata 或 marker 总数。
- 第二层：kind 计数，例如 `panic`、`error_string`、`returndata_bubble`。
- 第三层：关键参数，例如 panic code、selector、payload 长度。
- 第四层：必要时加位置 oracle，例如函数名、block 名、call site。

不要把自动分类结果直接当真。自动统计可以辅助整理 case，但写强 oracle 前要能回到具体 IR 位置确认。

## 验证要求

每次功能改动至少跑：

```bash
python3 -m json.tool test/evm/solidity-patterns/manifest.json
python3 -m py_compile test/run_evm_solidity_patterns_suite.py
cmake --build ./build --target all -j4
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
```

如果改了 rewrite 或 CFG：

```bash
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
```

如果新增独立 runner，要加 `py_compile`。

所有 suite 输出 `.ll` 必须用项目内 LLVM 22 验证，不要用系统 LLVM：

```bash
llvm-22.1.0.obj/bin/llvm-as out.ll -o out.bc
```

runner 已经会对 suite 输出做 `llvm-as` 检查；单个疑难 case 调试时也要手动抽查。

## 性能检查

每次改完都要记录同口径耗时。

之前 selector outline、payability CFG rewrite、revert matcher 都记录了
`notdec.evm.solidity_patterns` 和 `notdec.evm.solidity_rewrite` 的耗时变化。

如果只改 EVM Solidity matcher，通常说明“不涉及类型恢复、结构体合并、pointer analysis
或主类型恢复 pipeline”。但仍要看 suite 时间有没有明显上涨。

涉及 pass pipeline、类型恢复、结构体合并、pointer analysis 时，还要按项目规范对比
fortune 当前关注用例。

## 不要做的事

- 不要为了命中率把不确定 CFG 硬拆。
- 不要让两个 pass 各自重复识别同一种底层语义。
- 不要把 metadata-only 当最终成果。支持 rewrite 的 pass 对外必须有 rewrite 表达。
- 不要对外长期保留“原始低层操作 + 高层标记”的冗余语义。
- 不要只补 case 不补 oracle。
- 不要只补总数 oracle，不检查 kind 和关键参数。
- 不要把手写 IR 长期当真实 codegen 覆盖。
- 不要把依赖网络或重外部工具的生成流程接进常规 CTest。

## 后续功能的最小完成标准

新增一个 Solidity 底层模式，至少要满足：

- 有一段文档或日志说明 Solidity codegen 依据。
- matcher 有结构化结果。
- metadata 和 marker 分工明确。
- pattern suite 有 oracle。
- 如果支持 rewrite，有 rewrite suite 或等价结构检查。
- 对外输出不依赖 metadata-only 表达最终语义。
- 新增 case 写入 manifest 的 `patterns` 字段。
- 输出 IR 通过项目 LLVM 22 `llvm-as`。
- `notdec.evm.solidity_patterns` 通过。
- 相关 rewrite suite 不退化。
- 记录性能结果和暂不支持的形态。
