## Realworld Type Recovery Suite

This suite keeps larger real-world recovery cases that are grounded by a nearby
LLVM IR truth module instead of a plain `.htypes` snapshot.

Current scope:

- `cases/fortune.o3.wasm.ll`
  - The current in-tree real-world recovery input used by the suite.
  - Despite the plain name, this file now stores the frozen `mlsub` stage-B
    input for `fortune`, consumed with `--frozen-tr-input-ir`.
- `support/fortune.o3.wasm.extra.json`
  - Extra `PNDiff` constraints pinned to the frozen IR anchor.
  - The anchor hash is generated from the suite runner's absolute-path-loaded
    module text, so it intentionally matches the `ctest` invocation shape.
  - Forces two residual `Add` constraints in `add_file` / `get_tbl` to
    `number`.
- `truth/fortune.ll`
  - Copied from `/sn640/NotDec-Exp/ICSE-HOWARD/bin/fortune.ll`.
  - Used as the debug-info ground truth for semantic comparison.

The manifest currently compares only whitelisted fields for a few stable roots.
That keeps the suite useful while `fortune` still has partially recovered large
records in `ValueTypes.txt` / `.htypes`.
