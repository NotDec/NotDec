# Phoenix guarded terminal fold ASan 修复

## 原始需求

调研一下这个ASAN的问题，看看能不能修复

## 根因

`external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp` 的
`foldGuardedTerminalIntoSequence()` 里先拿了 `Prefix` 指针，随后调用
`buildSequenceFromRange()` 和 `Tree.addNode()`。`StructuredTree::addNode()` 会向
`std::vector<StructuredNode>` 追加节点并可能扩容，旧的 `Prefix` 指针会悬空。
ASan 栈里报的 `StructuredNode` copy use-after-free 正好对应后续 `*Prefix`。

## 修改

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2556`
  在新增节点前先把 `Prefix` 拷贝成 `StructuredNode FoldedPrefix`，后续只使用这份值。

这不改变结构恢复策略，只修掉 vector 扩容后的悬空指针。

## 验证

```bash
cmake --build ./build --target notdec -j4
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-phoenix-asan-smoke.sol --tr-level=2
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
ctest --test-dir build -R structuring-source-cases --output-on-failure
ctest --test-dir build -R 'structuring-smoke|structured-phoenix-available|shared-structurer-registry|structuring-analysis' --output-on-failure
```

结果：

- ASan 复现用例通过，`elapsed=30.21 user=34.41 sys=0.74 maxrss=982924`。
- `notdec.evm.solidity_source` 通过，0.16s。
- `notdec.evm.solidity_rewrite` 通过，100.20s。
- `structuring-source-cases` 通过，3.62s。
- `structured-phoenix-available`、`shared-structurer-registry`、`structuring-analysis` 通过。
- `structuring-smoke` 在当前工作树失败，失败内容集中在 C renderer 输出 oracle；这和本次 Phoenix 悬空指针修复不是同一类问题，本次没有处理。

## 评分

- 实现效果：8/10。原 ASan 用例已跑通。
- 复杂度：1/10。只提前一处值拷贝。
- 维护成本：1/10。没有引入新规则，也没有扩大 structuring 行为。
