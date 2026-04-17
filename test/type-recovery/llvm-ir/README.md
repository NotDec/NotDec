## LLVM IR Type Recovery Cases

This suite stores LLVM IR driven type recovery regression tests.

- `cases/`
  - Checked-in LLVM IR inputs for `notdec`.
- `expected/tr-level-2/`
  - Golden HType snapshots for `notdec --tr-level=2 --frozen-tr-input-ir --dump-htypes`.
- `legacy/`
  - Older backend artifacts kept for reference while the suite is being
    normalized.
  - Historical C output snapshots now live under `legacy/c-output-tr-level-2/`
    so `expected/` only contains the active HType oracle.

## Current Execution Model

This suite now runs in frozen stage-B mode.

- `manifest.json`
  - Passes `--frozen-tr-input-ir` by default.
- `cases/*.ll`
  - Are treated as already-frozen LLVM IR inputs for type recovery.
- The suite purpose
  - Is to exercise type inference itself, without re-running the pre-type-recovery
    normalization and transformation passes.

## Why This Layout

The old `test/backend/` directory mixed several roles:

- LLVM IR inputs
- historical generated C output
- handwritten experiments

That made it hard to tell which files were real oracles. The new layout keeps
inputs, expected outputs, and legacy references separate.

## Manifest Conventions

`manifest.json` is the authoritative list of cases in the current suite.

- `pass`
  - The case must recover HTypes successfully and match the checked-in golden
    snapshot.
- `xfail`
  - The case is a known type recovery failure at `tr-level=2`.
- `skip`
  - The case is tracked but not executed.
