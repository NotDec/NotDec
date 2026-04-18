## Realworld Type Recovery Suite

This suite keeps larger real-world recovery cases that are grounded by a nearby
LLVM IR truth module instead of a plain `.htypes` snapshot.

Current scope:

- `cases/fortune.o3.wasm.ll`
  - The current in-tree real-world recovery input.
  - Renamed from the former split-import case `fortune.o3.wasm.4.ll`.
- `truth/fortune.ll`
  - Copied from `/sn640/NotDec-Exp/ICSE-HOWARD/bin/fortune.ll`.
  - Used as the debug-info ground truth for semantic comparison.

The manifest currently compares only whitelisted fields for a few stable roots.
That keeps the suite useful while `fortune` still has partially recovered large
records in `ValueTypes.txt` / `.htypes`.
