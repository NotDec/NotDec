# EVM Solidity Pattern Corpus Compile Suite

The pattern suite checks IR metadata only.  This suite runs the Solidity
backend over the whole pattern corpus and ratchets the output baseline:

- every generated `.sol` must compile with the configured solc;
- `TODO: unresolved value`, `false /* TODO` and `// goto block_` occurrence
  counts must stay within the budgets in `compile-budget.json`.  Goto targets
  are resolved per generated function, because rendered block labels restart at
  0 in every function and a file-global match would hide real jumps.

It is opt-in because it needs solc.  Configure with:

```bash
cmake -S . -B build -DNOTDEC_SOLC=/path/to/solc
ctest --test-dir build -R notdec.evm.solidity_pattern_compile --output-on-failure
```

When a change improves a metric, lower the budget in the same commit so the
improvement is locked in.  Raising a budget requires a concrete reason recorded
in `logs/`.
