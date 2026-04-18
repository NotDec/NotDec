## Wasm Lifting Suite

This suite tracks frontend lifting regressions starting from raw `.wasm` input.

- `cases/`
  - Checked-in frontend inputs.
- `expected/`
  - Suite-local oracles for workdir logs emitted by the lifting + recovery run.
- `truth/`
  - Nearby LLVM IR ground truth kept for provenance and future richer checks.

Current scope:

- `fortune.o3.wasm`
  - Imported from `/sn640/NotDec-Exp/ICSE-HOWARD/bin/fortune.o3.wasm`
  - Checked with a workdir-log oracle instead of a full-text `.ll` snapshot.
  - The oracle currently looks at:
    - residual `Add/Sub` count in `PNDiff.warn.txt`
    - stack-pointer recovery errors in `01-recovery-passes.log`
    - `MemsetMatcher` merge activity in `01-recovery-passes.log`
- `truth/fortune.ll`
  - Copied from `/sn640/NotDec-Exp/ICSE-HOWARD/bin/fortune.ll`
  - Reserved for future semantic lifting comparisons.
