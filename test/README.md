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
  - Supports `htype-snapshot` and `wasm-dwarf-compare` oracle kinds.
- `test/run_lifting_suite.py`
  - Shared manifest-driven runner for frontend lifting suites.
  - Supports module-summary and workdir-log oracles for frontend + recovery output.
- `test/lifting/<arch>/`
  - Self-contained lifting suites grouped by frontend/input architecture.
  - `cases/` stores checked-in raw inputs such as `.wasm`.
  - `expected/` stores suite-local lifting oracles.
- `test/evm/`
  - EVM IR pass-pipeline suites.
  - `cases/` stores checked-in evm2llvm LLVM IR inputs.
  - Oracles check targeted EVM metadata instead of full IR text.
  - `solidity-source/` stores manually accepted Solidity source regressions
    with frozen evm2llvm IR and expected Solidity-like output.
- `test/type-recovery/<suite>/`
  - Self-contained type recovery suites.
  - `manifest.json` defines the checked cases and per-suite setup.
  - `cases/` stores checked-in source inputs.
  - `expected/` stores golden HType snapshots for snapshot-oracle suites.
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
- `test/evm/solidity-patterns/`
  - EVM Solidity compiler-pattern pass cases.
  - Currently checks nonpayable guard metadata counts on selected apehex
    evm2llvm outputs.
- `test/evm/solidity-source/`
  - Small Solidity source regressions accepted after manual review.
  - Default CTest uses frozen evm2llvm IR and compares final `.sol` output.
- `test/type-recovery/llvm-ir/`
  - Handwritten LLVM IR type recovery regression cases.
- `test/type-recovery/sysy/`
  - SysY source cases compiled to LLVM IR before type recovery checks.
- `test/type-recovery/realworld/`
  - Real-world larger recovery cases grounded by raw-input wasm DWARF.
  - Currently keeps only `fortune.o3.wasm.ll`.
  - The checked-in `fortune.o3.wasm.ll` is the frozen `mlsub` stage-B input
    consumed with `--frozen-tr-input-ir`.

## Adding New Integration Tests

1. Put the input under the suite's `cases/` directory.
2. Add suite-local support files under `support/` only if the case cannot be
   prepared without them.
3. Add or update the expected output under `expected/`.
4. Register the case in the suite manifest.
5. Run the suite through `ctest`.

The type recovery runner currently supports `pass`, `xfail`, and `skip` states
so known bugs can be tracked without pretending they are fixed. Snapshot suites
still compare `.htypes` text directly, while wasm-DWARF suites extract truth
from the raw input's DWARF and compare it semantically against the recovered
`.htypes`. The current wasm-DWARF oracle supports direct aggregate globals,
global pointer slots, and recovered function-parameter layout slices. The
lifting runner mirrors the same manifest-driven style, but keeps its oracles
smaller than a full-text `.ll` diff by checking summaries or selected workdir
logs instead.

## Running and interpreting type-recovery tests

Use the configured build directory and select the current suites with CTest:

- `notdec.lifting.wasm`
- `notdec.type_recovery.llvm_ir.tr_level_2`
- `notdec.type_recovery.sysy.tr_level_2`
- `notdec.type_recovery.realworld.tr_level_2`

```bash
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' --output-on-failure
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy|realworld).tr_level_2' --output-on-failure
cmake --build build --target TypeBuilderTest binarysub -j4
./build/bin/TypeBuilderTest
./build/binarysub
```

`test/legacy/` and suite-local `legacy/` directories are historical references,
not current goldens. Do not assume the obsolete `--only-opt` call in `test.sh`
is a supported CLI option. Before a broad run, confirm required external data,
dependencies, and submodule state.

For type-recovery assertions, compare the suite's HType oracle or workdir HType
artifacts. Output LLVM IR only establishes verifier validity; it is not a
type-inference oracle.
