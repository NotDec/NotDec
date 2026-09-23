# EVM Solidity Pattern Corpus Compile Suite

The pattern suite checks IR metadata only.  This suite runs the Solidity
backend over the whole pattern corpus and ratchets the output baseline:

- every generated `.sol` must compile with the configured solc;
- the number of rendered `private__*` helper functions and helper call sites
  must stay at or above the floors in `compile-budget.json`, so recovered
  helper structure cannot disappear silently;
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

## Pattern suite shape (flattened)

The EVM pipeline has one shape: `EvmHelperInliningPass` always expands the
calldata chain before type recovery, so the recovered calldata record sees the
absolute per-entry offsets.  (The pre-inlining outlined shape no longer exists;
the earlier logs that calibrate it are historical.)  `notdec.evm.solidity_patterns`
compares the flattened output with two adjustments:

- Keys whose count grows with the number of textual sites (`rewrite_marker:*`,
  `revert_kind:*`, `checked_bounds_kind:*`, `notdec.solidity.*`, ...) are lower
  bounds: the inliner duplicates the shared bound/revert scaffolding at every
  call site, so a decrease is a lost recognition and an increase is a duplicated
  site.  `is_site_count_key()` in the runner is the single place that decides
  this.
- A case may carry an `"inline"` block with the expectations that legitimately
  differ between the shapes (the ABI parameter table of an entry that gains its
  absolute reads, decoder helpers that are inlined away, a checked-bounds kind
  that becomes more precise once a formal length is a real calldata load).  Each
  block carries a comment with the IR evidence for the change.
