## Test Layout

This repository now separates tests by purpose:

- `unittests/`
  - Small C++ unit tests built with GoogleTest.
- `test/`
  - Integration and regression tests that exercise executables and pipelines.

This mirrors the usual LLVM split:

- `unittests/` for library-level logic
- `test/` for end-to-end or pass-pipeline behavior

## Current Integration Layout

- `test/run_type_recovery_suite.py`
  - Shared manifest-driven runner for HType regression suites.
  - Supports `htype-snapshot` and `debug-struct-compare` oracle kinds.
- `test/run_lifting_suite.py`
  - Shared manifest-driven runner for frontend lifting suites.
  - Supports module-summary and workdir-log oracles for frontend + recovery output.
- `test/lifting/<arch>/`
  - Self-contained lifting suites grouped by frontend/input architecture.
  - `cases/` stores checked-in raw inputs such as `.wasm`.
  - `expected/` stores suite-local lifting oracles.
  - `truth/` stores nearby ground-truth IR kept for semantic follow-up checks.
- `test/type-recovery/<suite>/`
  - Self-contained type recovery suites.
  - `manifest.json` defines the checked cases and per-suite setup.
  - `cases/` stores checked-in source inputs.
  - `expected/` stores golden HType snapshots for snapshot-oracle suites.
  - `truth/` stores checked-in debug-info LLVM IR for debug-ground-truth suites.
  - `support/` is optional for suite-local headers or helper inputs.
  - `legacy/` is optional for migrated artifacts that are reference-only.
- `test/legacy/`
  - Historical scripts, datasets, and generated artifacts that are not treated
    as current `ctest` suites.

## Current Suites

- `test/lifting/wasm/`
  - Raw WebAssembly lifting regression cases.
  - Currently tracks `fortune.o3.wasm` with a workdir-log oracle that checks
    `PNDiff.warn.txt` and `01-recovery-passes.log`.
- `test/type-recovery/llvm-ir/`
  - Handwritten LLVM IR type recovery regression cases.
- `test/type-recovery/sysy/`
  - SysY source cases compiled to LLVM IR before type recovery checks.
- `test/type-recovery/realworld/`
  - Real-world larger recovery cases grounded by nearby LLVM IR truth modules.
  - Currently keeps only `fortune.o3.wasm.ll` plus `truth/fortune.ll`.
  - The checked-in `fortune.o3.wasm.ll` is the frozen `mlsub` stage-B input
    consumed with `--frozen-tr-input-ir`.
- `test/type-recovery/debug-info-c/`
  - C-authored struct-layout recovery cases with checked-in debug-info LLVM IR
    ground truth.

## Adding New Integration Tests

1. Put the input under the suite's `cases/` directory.
2. Add suite-local support files under `support/` only if the case cannot be
   prepared without them.
3. Add or update the expected output under `expected/`.
   For debug-info suites, refresh the checked-in `truth/*.dbg.ll` or nearby
   truth module instead.
4. Register the case in the suite manifest.
5. Run the suite through `ctest`.

The type recovery runner currently supports `pass`, `xfail`, and `skip` states
so known bugs can be tracked without pretending they are fixed. Snapshot suites
still compare `.htypes` text directly, while debug-ground-truth suites extract
truth from debug metadata and compare it semantically against the recovered
`.htypes`. The lifting runner mirrors the same manifest-driven style, but keeps
its oracles smaller than a full-text `.ll` diff by checking summaries or
selected workdir logs instead.
