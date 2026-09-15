# NotDec type-recovery Pass library plan

User prompt:

> 按这个推进吧，也没必要用现有 NotDec CLI 作为 oracle，那个也不一定准

## Background

Microsub2 now emits an LLVM module with an IDA-derived target triple and data
layout.  The next integration step is to link NotDec's type recovery into
Microsub2 directly and let Microsub2 compose the optimization/type-recovery
passes itself.  The CLI stage-B flow is no longer the intended contract; it is
not a type-recovery oracle.

Today NotDec already exposes type recovery as an LLVM pass
(`mlsub::MLsubRecoveryMain`) and composes it in
`PassEnv::add_type_recovery_passes()` together with `ScalarizerPass`,
`FreePhiSplitPass`, and `DangerousTypePatternScan`.  The problem is packaging:
the implementation lives inside the monolithic `notdec-core` target, which
also links Wasm, LLVM2C, Solidity, Clang tooling and other frontends.

## Goal

Add a `notdec-typerecovery` static library target that contains only the
type-recovery pass, its required support code, and `DangerousTypePatternScan`.
The public interface remains the existing pass types:

* `notdec::mlsub::MLsubRecovery`
* `notdec::mlsub::MLsubRecoveryMain`
* `notdec::mlsub::Result` / `notdec::llvm2c::HTypeResult`
* `notdec::passes::DangerousTypePatternScan`

Microsub2 can then run `ModulePassManager` with its own pass order and call
`MLsubRecovery::getResult()` after the pass manager has run.

## Route

1. Move the type-recovery sources out of `notdec-core` into a new
   `notdec-typerecovery` target.
2. Split the clang-free work-dir/file/join helpers out of `Utils.cpp` so the
   new target does not need Clang tooling.
3. Remove type-recovery headers' dependency on `notdec-llvm2c/Interface.h` and
   unused Clang includes; depend only on `notdec-backends/Core/HTypeResult.h`,
   `binarysub`, and `ExtValuePtr`/`ValueNamer`.
4. Keep `SolidarityPatterns.cpp` with the new target because
   `MLsubGenerator.cpp` currently uses its EVM field matchers.  That file has
   no non-LLVM `notdec::` dependencies.
5. Make `notdec-core` link `notdec-typerecovery` PUBLIC and remove the moved
   sources so the CLI executable still builds.
6. Validate the standalone pass-library target first; Microsub2 build-system
   wiring is a follow-up step.

## Risks and stop criteria

* Moving source files can expose hidden dependencies or duplicate symbols.
  Stop if the new target drags in Clang/Wasm/LLVM2C backends or if the CLI
  build regresses.
* Work-dir/environment globals should not become a new public behavior.  The
  first extraction only splits implementation; an options cleanup can follow
  when Microsub2 actually sets up the pass pipeline.
* Verification: `notdec-typerecovery` and `notdec` both build; existing
  `notdec` smoke/runtime behavior is unchanged.

## Implementation record

Implemented in this batch:

* `src/CMakeLists.txt` now defines `notdec-evm-pattern-utils` and
  `notdec-typerecovery`.
  - `notdec-evm-pattern-utils` contains `MemoryBufferAnalysis.cpp`,
    `SolidityPatterns.cpp`, and `StorageAccessMatchers.cpp`.  These are the
    EVM field-matching helpers referenced from `MLsubGenerator.cpp`; they have
    no Clang/Solidity-backend dependency.
  - `notdec-typerecovery` contains the MLsub sources, `LowTy.cpp`,
    `DangerousTypePatternScan.cpp`, and `Utils/RuntimeSupport.cpp`.
  - `notdec-typerecovery` links `binarysub_lib`, `notdec-backend-core`,
    `notdec-evm-pattern-utils`, and the shared LLVM dependency.
* `notdec-core` no longer compiles the type-recovery sources, the two
  DangerousTypePatternScan/EVm-utils sources, or the moved Utils helpers; it
  links `notdec-typerecovery` PUBLIC instead.  The CLI executable is
  unchanged from the user's point of view.
* `src/Utils/Utils.cpp` and new `src/Utils/RuntimeSupport.cpp`:
  `getDefaultWorkDir`, `set/getWorkDir*`, `appendWorkDirLog`, `readFileToString`,
  `join`, `printModule`, and `getSuffix` moved to the Clang-free
  `RuntimeSupport.cpp`.  `Utils.cpp` keeps only the clang AST `toString`
  helper and small utility functions still needed by the CLI.
* Type-recovery headers now include
  `notdec-backends/Core/HTypeResult.h` directly instead of
  `notdec-llvm2c/Interface.h`, and `MLsubGenerator.h` no longer pulls in
  Clang ASTUnit/Tooling headers.  `MLsubGenerator.cpp`,
  `HTypeDebug.cpp`, and `MergePolicyEval.cpp` no longer include
  `notdec-llvm2c/Utils.h`; they use the lean `notdec::join`/`getLLVMTypeSize`
  equivalents.

## Validation

* `cmake --build build-notdec-nothreads2 --target notdec-typerecovery -j2`
  succeeds and produces `src/libnotdec-typerecovery.a`.
* `cmake --build build-notdec-nothreads2 --target notdec -j2` succeeds, so
  the normal CLI still links.
* `nm -C -u build-notdec-nothreads2/src/libnotdec-typerecovery.a | grep
  'clang::'` is empty; the archive does not reference Clang.
* A temporary standalone consumer linked only
  `libnotdec-typerecovery.a`, `libnotdec-evm-pattern-utils.a`,
  `libnotdec-backend-core.a`, `libbinarysub_lib.a`, LLVM and TBB.  It:
  - composes `ScalarizerPass`, `DangerousTypePatternScan`, and
    `MLsubRecoveryMain` in a `ModulePassManager`;
  - calls `MLsubRecovery::getResult()`;
  - reads `!microsub2.lvar.id` metadata from allocas and prints per-lvar
    lower/upper HType strings for the MicroSub2 `s001` module.
  This is the same pass composition intended for Microsub2.
* The rebuilt NotDec CLI, run on the same MicroSub2 `.ll`, still completes
  with `Constraint generation done! SCC count:1`.

## Follow-up

The library target still uses the process-wide NotDec workdir/environment
state (as before) and the Pass manager composition is left to the consumer.
Microsub2 wiring and exclusive option cleanup will be handled in the
integration batch.
