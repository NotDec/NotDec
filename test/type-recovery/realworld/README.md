## Realworld Type Recovery Suite

This suite keeps larger real-world recovery cases whose oracle comes from the
input wasm's own DWARF instead of a nearby LLVM IR truth module or a plain
`.htypes` snapshot.

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
- `../../lifting/wasm/cases/fortune.o3.wasm`
  - The authoritative DWARF source for the oracle.
  - Parsed at test time into a normalized truth JSON plus a human-readable
    text dump with the same memory/functions/decls split used for manual
    comparison.

The manifest currently mixes three kinds of checks:

- global pointer-slot layout checks
  - The checker looks at the recovered slot at the DWARF absolute address, then
    compares the pointed-to `struct_*` slice against the DWARF pointee type.
- global record layout checks
  - Used for direct aggregate globals such as `Noprob_tbl`.
- layout-only checks for selected function parameters already recovered as
  `struct_*` pointers

That keeps the suite useful while `fortune` still has partially recovered large
records in `ValueTypes.txt` / `.htypes`, especially for function-parameter
struct slices whose field types are not yet stable enough for exact matching.
Each run now also writes a human-readable `*.compare.md` report beside the JSON
and text reports to make manual inspection cheaper.
