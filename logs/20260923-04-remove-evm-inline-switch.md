# 20260923-04 删除 NOTDEC_EVM_INLINE 开关：calldata 链内联是唯一形状

结论：`NOTDEC_EVM_INLINE`（`0` / `calldata` / `1`|`all` 三档）**已删除**，
`EvmHelperInliningPass` 固定执行 calldata 链范围的内联；`AbiReturnPass` 的
`InlineHelperEvidence` 参数随之去掉（那个证据分支现在无条件启用）。
之前给 pre-inline（outlined）形状加的预算棘轮
（`compile-budget-outlined.json` + `notdec.evm.solidity_pattern_compile_outlined`）
和 `notdec.evm.solidity_patterns_outlined` 一并删除。

## 1. 为什么删

- pre-inline 形状不是产品形状，而且**已知会退化 ABI 参数**：类型恢复把所有入口的
  `%calldata` 合并成一个模块级记录，共享 outline 解码 helper 的调用约束（实参≤形参、
  逆变）把各调用点的常量偏移汇进同一个形参，于是“按记录数 ABI 参数”在入口维度上失真
  （根因与最小复现见 20260920-01 §3）。
- 当前二进制实测（`logs/artifacts/20260920-worklist-24259/agg_abi_params.py`，2316 个 entry）：

  | 形状 | record==annotated | too_few |
  | --- | --- | --- |
  | 内联（产品） | 2264 / 2316 (97.8%) | 2 |
  | pre-inline | 1858 / 2316 (80.2%) | 308 |

  `too_few 308` 就是“不同入口的 calldata 类型被合掉”的直接体现。保留这个形状做 CI 口径，
  等于把优化压力引到一个不出、且语义更差的形状上。
- 内联是**绕过**这个问题：把 calldata reader 复制进每个入口，让偏移变成 entry-local 的
  绝对值；代价是扁平形状里共享 guard 脚手架被复制，所以 goto/TODO 更高（预算注释里已记录）。
- 真正的解法是 per-call-site 实例化（`KIND_MLSUB_POLYMORPHIC_FUNCTION`）：public entry 变体
  收益很大，但 case 24259 上约束 worklist 不收敛（25 分钟 / 3 GB），目前只在
  `NOTDEC_EVM_POLY_ENTRIES=1` 下可复现（`EvmCalldataAccessPass.cpp:60-78`）。这是下一步方向，
  不是回到 pre-inline 形状；`NOTDEC_EVM_POLY_ENTRIES` / `NOTDEC_EVM_POLY_STRICT` 这两个诊断开关保留。

## 2. 改了什么

| 位置 | 变化 |
| --- | --- |
| `src/Passes/evm/solidity-patterns/EvmHelperInliningPass.cpp` | 删 `InlineMode`/`getInlineMode()`/`isHelperInliningEnabled()`；`run()` 固定 pin entries + calldata scope；诊断开关改名 `NOTDEC_EVM_HELPER_INLINE_DIAG`（原名里带着已删开关） |
| `include/notdec/Passes/evm/SolidityPatterns.h` | 删 `isHelperInliningEnabled()`；`AbiReturnPass` 去掉 `InlineHelperEvidence` 成员与构造参数 |
| `src/Passes/PassManager.cpp` | `evm::AbiReturnPass(*TR)` |
| `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp` | 两字段 ABI 记录的等价证据分支无条件启用 |
| `test/CMakeLists.txt` | 删 `notdec.evm.solidity_patterns_outlined`、`notdec.evm.solidity_pattern_compile_outlined` 的注册与工厂函数 |
| `test/evm/solidity-patterns/compile-budget-outlined.json` | 删除 |
| `test/evm/solidity-patterns/README.md`、`compile-budget.json` | 文案更新（不再有“两个形状”） |

## 3. 对旧日志的更正

凡是提到 `NOTDEC_EVM_INLINE` 的日志都已加上更正说明或改写：
`20260923-01`（改名为 `20260923-01-outlined-diagnostic-snapshot.md`，只留诊断数据）、
`20260922-04`、`20260922-01`、`20260923-03`、`20260920-04`、`20260920-02`。
注意 `20260922-01` 的根因结论（失败形状是共享 join，不是 duplication）与本次删除无关，仍然有效；
被更正的是“outlined 是并列口径”这个框架。

## 4. 验证

- `ninja -C build-notdec-nothreads notdec` 通过。
- 103 case 编译预算逐项不变：goto 1787、real jump 1695、dangling 58、condition TODO 4078、
  unresolved 297、solc 失败 0；helper 1313 / 2442（floor 1276 / 2408）。
- `ctest -R notdec.evm.solidity`：patterns / pattern_compile / rewrite / source 全绿。
- `structuring-analysis-test` exit 0。
