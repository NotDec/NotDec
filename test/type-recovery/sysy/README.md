## SysY Type Recovery Cases

This suite stores SysY source snippets that are compiled to LLVM IR before
running `notdec` type recovery checks.

- `cases/`
  - Checked-in SysY `.c` inputs.
- `support/`
  - Headers needed to compile the SysY cases to non-opaque LLVM IR.
- `expected/tr-level-2/`
  - Golden HType snapshots for `notdec --tr-level=2 --dump-htypes`.

## Why This Suite Exists

The old `test/wasm/sysy/` directory mixed source inputs, generated wasm/wat/ll
artifacts, ad-hoc scripts, and experimental outputs. This suite keeps only the
stable regression inputs and expected HType snapshots in the main test layout.

`manifest.json` is the authoritative list of tracked cases. It currently uses
`clang-14` to compile each SysY source file into LLVM IR before invoking
`notdec`.
