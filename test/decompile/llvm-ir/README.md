## LLVM IR Decompiler Cases

This suite stores LLVM IR driven decompiler regression tests.

- `cases/`
  - Checked-in inputs for the decompiler.
- `expected/tr-level-2/`
  - Golden outputs for `notdec --tr-level=2`.
- `legacy/`
  - Older backend artifacts kept for reference while the suite is being
    normalized.

## Why This Layout

The old `test/backend/` directory mixed several roles:

- LLVM IR inputs
- historical generated C output
- handwritten experiments

That made it hard to tell which files were real oracles. The new layout keeps
inputs, expected outputs, and legacy references separate.

## Manifest Conventions

`tr-level-2.json` is the authoritative list of cases in the current suite.

- `pass`
  - The case must decompile successfully and match the checked-in golden file.
- `xfail`
  - The case is a known failure at `tr-level=2`.
- `skip`
  - The case is tracked but not executed.
