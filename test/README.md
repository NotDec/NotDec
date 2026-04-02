## Test Layout

This repository now separates tests by purpose:

- `unittests/`
  - Small C++ unit tests built with GoogleTest.
- `test/`
  - Integration and regression tests that exercise executables and pipelines.

This mirrors the usual LLVM split:

- `unittests/` for library-level logic
- `test/` for end-to-end or pass-pipeline behavior

## Current Suites

- `test/decompile/llvm-ir/`
  - LLVM IR based decompiler cases.
  - `cases/` stores checked-in inputs.
  - `expected/` stores current golden outputs.
  - `legacy/` keeps older backend artifacts that are useful for reference but are
    not treated as authoritative test oracles.

## Adding New Integration Tests

1. Put the input under the suite's `cases/` directory.
2. Add or update the expected output under `expected/`.
3. Register the case in the suite manifest.
4. Run the suite through `ctest`.

The `tr-level=2` decompiler suite currently supports `pass`, `xfail`, and
`skip` states so known bugs can be tracked without pretending they are fixed.
