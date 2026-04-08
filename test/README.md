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
- `test/type-recovery/<suite>/`
  - Self-contained type recovery suites.
  - `manifest.json` defines the checked cases and per-suite setup.
  - `cases/` stores checked-in source inputs.
  - `expected/` stores golden HType snapshots.
  - `support/` is optional for suite-local headers or helper inputs.
  - `legacy/` is optional for migrated artifacts that are reference-only.
- `test/legacy/`
  - Historical scripts, datasets, and generated artifacts that are not treated
    as current `ctest` suites.

## Current Suites

- `test/type-recovery/llvm-ir/`
  - Handwritten LLVM IR type recovery regression cases.
- `test/type-recovery/sysy/`
  - SysY source cases compiled to LLVM IR before type recovery checks.

## Adding New Integration Tests

1. Put the input under the suite's `cases/` directory.
2. Add suite-local support files under `support/` only if the case cannot be
   prepared without them.
3. Add or update the expected output under `expected/`.
4. Register the case in the suite manifest.
5. Run the suite through `ctest`.

The type recovery runner currently supports `pass`, `xfail`, and `skip` states
so known bugs can be tracked without pretending they are fixed.
