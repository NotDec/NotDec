## LLVM IR Type Recovery Cases

This suite stores LLVM IR driven type recovery regression tests.

- `cases/`
  - Checked-in LLVM IR inputs for `notdec`.
- `expected/type-recovery-tr-level-2/`
  - Golden HType snapshots for `notdec --tr-level=2 --dump-htypes`.
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

`type-recovery-tr-level-2.json` is the authoritative list of cases in the
current suite.

- `pass`
  - The case must recover HTypes successfully and match the checked-in golden
    snapshot.
- `xfail`
  - The case is a known type recovery failure at `tr-level=2`.
- `skip`
  - The case is tracked but not executed.
