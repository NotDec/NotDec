# 20260923-02 清掉剩余 TODO(structuring)：折叠后的形状重定 + 共享 phi 的局部变量

结论：`test/structuring/structuring_analysis_test.cpp` 里 6 处 `TODO(structuring)` 全部清除。
其中 5 处是"改动后预期过期"，按**当前形状重新定死**（并把原来 `if (...) return;` 这种
"断言全被跳过"的写法换成真正的断言）；1 处（共享 phi）是**后端真 bug**，已修：
phi 合并块的反 phi 化（dephication）变量此前只出现在赋值侧，读取侧渲染成
`return 0 /* TODO: unresolved value: p */`，而且 `p = a; / p = b;` 的 `p` 根本没有声明。

## 1. 共享 phi：dephication 变量现在有声明、能被读回

`LLVMFunctionCFGBuilder` 把每个 phi 变成反 phi 化变量：每条入边插一个 edge block
（`p = a;`），merge 块读回该变量。但 Solidity 后端只在赋值侧用了名字，读取侧
`valueExpr(PHINode)` 落到最后的兜底 → `UnresolvedValueExpr`，于是 merge 的 `return`
渲染成 `return 0 /* TODO: unresolved value: p */`，语义直接错了（返回值变成 0）。

修法（只在 Solidity 后端，不动共享的 CFG builder，C 路径完全不受影响）：

- `BodyBuilder.cpp` 新增 `PhiValueNameMap`（phi → 局部名）+ thread_local
  `ActivePhiValueNames`，在 `ActiveStorageSlotsScope` 里随其它 active map 一起进出。
- `readBody` 预扫函数里的 phi，把 **SSA 名本来就是合法 Solidity 标识符**、且不与
  形参/实参/已恢复 storage 变量/已物化 temp 冲突的 phi 登记成声明（走 `HoistedNames`，
  即在函数开头 `uint256 p;`）。
- `valueExpr` 命中该表时返回标识符；`getPhiAssignment` 的赋值目标也走同一个表。
- 名字不合法（如 `spec.select16`）或撞名的 phi **完全保持原行为**（不登记、不声明），
  所以不会新引入任何标识符文本、也不会因为改写名字而产生新的未声明引用。

顺带把测试改名：`testSolidityBodyBuilderReadsCopiedSharedPhiAssignments` →
`...ReadsSwitchSharedPhiAssignments`（该形状现在不再被复制，没有 `_copyN`）。

## 2. 三处 refineCyclic 期望：按折叠后的形状重定

`refineCyclic` 手搭 CFG 的期望此前全部靠 `if (!FoundLoop) return;` 兜底（等于没断言）。
现在实测（临时插桩 dump，已移除）：

| 测试 | 旧期望 | 现形状（已 pin） |
| --- | --- | --- |
| `testRefineCyclicStructuresHeaderExitLoopAsWhile`（原 `...PrefersDoWhileWhenLatchConditionWouldBecomeBreak`） | `DoWhile#2`，follow=3 | 循环 {1,2} 折叠成一个节点后图上没有回边，改走 header schema：`While#1`，follow=4，latch 的另一条出边 2→3 虚化成 break |
| `testRefineCyclicKeepsDanglingNonFollowExit` | 期望 refineCyclic 失败 / 无虚边 | `DoWhile#3`，follow=6；悬空出口 2→5 保留为虚边（正合测试名 keeps） |
| `testRefineCyclicPrefersMostCommonExitAsFollow` | `InfiniteLoop`，follow=6 | `DoWhile#4`，follow 仍是 6（最常出边），落败的 3→5 虚化 |

三个测试现在都断言：循环节点种类 + 条件/闩锁块 + 唯一后继（follow）的首块 + 虚边数量与
端点。这是"重定形状"而不是"数值修正"：种类和 follow 都变了。

## 3. 两处 deopt pass 期望：pin "pass 主动放弃"

`ReturnDuplicatorLow` / SAILR 去优化管线对 switch-case 终止分叉（`makeSwitchCaseTerminalForkFunction`）
现在**不复制**：结构器已经能直接折掉这个形状，试跑找不到"严格更少 goto"的改进，
于是 pass 返回 `Succeeded=false`、管线返回 `Changed=false`。两个测试改成直接断言这一点
（`assert(!Result.Succeeded)` / `assert(!Result.Changed)`），原来的复制断言保留在
`return` 之后，等 pass 再次接受该形状时启用。

## 4. 验证

- `structuring-analysis-test`：**exit 0**（全量单测，含上面所有改动）。
- 103 case 编译预算在 phi 改动后**逐项不变**：goto 1787 / real 1695 / dangling 58 /
  cond_todo 4078 / unresolved 297 / solc 0 失败。即该修复对当前语料是纯收益（测试形状正确了，
  语料没被扰动），不需要动预算。（当时一并测过的 pre-inline 形状数字见
  `20260923-01-outlined-diagnostic-snapshot.md`；该形状及对应预算已在 `20260923-04` 删除。）
