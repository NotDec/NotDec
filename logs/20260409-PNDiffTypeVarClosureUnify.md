# 2026-04-09 PNDiff Type Variable Closure Unify

## 背景

当前 `MLsub` 路径下，`binarysub::constrain()` 会在递归和变量上下界传播时生成新的
`SimpleType` pair，但这些内部 pair 之前不会反向驱动 `PNDiff` 节点统一。

这会导致：

- 结构上已经在 `binarysub` 中被认为应当联动的类型变量
- 在 `PNDiff` 里仍保持为分离节点

尤其是函数参数递归展开、以及变量上下界闭包传播出来的 pair，之前没有统一回去。

## 本次修改

1. 给 `external/binarysub` 的 `constrain()` 增加了可选 pair observer
   - 每当 worklist 真正弹出并处理一个 `(lhs, rhs)` pair 时触发
   - 不改变原有调用方语义，默认 observer 为空

2. 在 `ConstraintsGenerator` 中增加了开关：
   - `EnablePNDiffTypeVariableClosureUnification`
   - 默认值为 `true`

3. 在 `ConstraintsGenerator::addSubtype()` 中接入 observer
   - 对每个实际处理到的 pair，尝试执行
     `maybeUnifyPNDiffTypeVariablePair()`

4. 当前统一策略刻意收窄为：
   - 只处理两边都为 `VariableState` 的 pair
   - 通过 `V2N.rev()` 反查与该类型变量关联的 `ExtValuePtr`
   - 再把对应的 `PNINode` 统一起来

## 为什么只处理 VariableState

`SimpleType` 的 `value_ptr` 比较是按值比较，不是按对象 identity 比较。

如果对 `record` / `ptr_load` / `ptr_store` / `function` 这类结构节点也直接反查，
容易把“结构相等但来源不同”的节点混在一起。

而 `VariableState` 由 fresh id 构造，身份稳定，作为当前 bridge 更安全。

## 验证

本次增加了两个 gtest：

- `MLsub.PNDiffUnifiesRecursiveVariablePairsByDefault`
- `MLsub.PNDiffRecursiveVariablePairUnificationCanBeDisabled`

验证命令：

```bash
cmake --build build --target GraphTest
ctest --test-dir build -R '^MLsub\.(PNDiffUnifiesRecursiveVariablePairsByDefault|PNDiffRecursiveVariablePairUnificationCanBeDisabled)$' --output-on-failure
```

两项测试均通过。
