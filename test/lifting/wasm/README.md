## Wasm Lifting Suite

This suite tracks frontend lifting regressions starting from raw `.wasm` input.

- `cases/`
  - Checked-in frontend inputs.
- `expected/`
  - Small module-summary oracles for the lifted `.ll` output.
- `truth/`
  - Nearby LLVM IR ground truth kept for provenance and future richer checks.

Current scope:

- `fortune.o3.wasm`
  - Imported from `/sn640/NotDec-Exp/ICSE-HOWARD/bin/fortune.o3.wasm`
  - Checked with a module-summary oracle instead of a full-text `.ll` snapshot
    so unrelated IR formatting churn does not break the test.
- `truth/fortune.ll`
  - Copied from `/sn640/NotDec-Exp/ICSE-HOWARD/bin/fortune.ll`
  - Reserved for future semantic lifting comparisons.
