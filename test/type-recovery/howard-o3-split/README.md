## HOWARD O3 Split IR Cases

This suite imports the split LLVM IR corpus from:

- `/sn640/NotDec-Exp/ICSE-HOWARD/splited/splitted_irs`

Only `*.o3*.ll` inputs are included here. The suite currently tracks 92 O3
split IR cases across `fortune`, `grep`, `gzip`, `lighttpd`, and `wget`.

## Current Status

This is an import-first suite.

- `cases/`
  - The full checked-in O3 LLVM IR corpus.
- `expected/tr-level-2/`
  - Reserved for future HType snapshots as cases are promoted to `pass`.
- `manifest.json`
  - All imported cases are currently marked `skip` so the corpus is available
    in-tree without blocking current `ctest` runs.

An initial manual baseline probe on the first batch of cases mostly hit
`PNDiff` assertion failures during type recovery, so we are landing the corpus
first and will classify cases into `xfail` and `pass` incrementally.
