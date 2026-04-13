## Debug-Info Ground-Truth Type Recovery Suite

This suite checks type recovery against debug metadata instead of handwritten
`.htypes` snapshots.

Layout:

- `cases/`
  - Author-facing C sources.
- `truth/`
  - Checked-in `*.dbg.ll` files compiled from `cases/`.
  - These are the authoritative ground-truth inputs for the suite.
- `manifest.json`
  - Declares the tracked cases, how stripped inputs are prepared, and which
    roots are compared.

Current first-stage constraints:

- Cases target `wasm32-unknown-wasi`.
- The runner strips debug info from `truth/*.dbg.ll` with `opt-14 -strip-debug`
  before invoking `notdec`.
- Roots are currently compared against the recovered `[memory]` decl, so the
  initial cases intentionally anchor struct pointers at stable absolute
  addresses.
- The comparator focuses on field presence, relative offsets, primitive fields,
  and pointer-shaped fields.

Refresh a truth IR file with:

```bash
python3 test/tools/gen_debug_info_case.py \
  --case test/type-recovery/debug-info-c/cases/01_absolute_pair.c \
  --output test/type-recovery/debug-info-c/truth/01_absolute_pair.dbg.ll
```

Check that checked-in truth IR is still in sync with the source:

```bash
python3 test/tools/gen_debug_info_case.py \
  --case test/type-recovery/debug-info-c/cases/01_absolute_pair.c \
  --output test/type-recovery/debug-info-c/truth/01_absolute_pair.dbg.ll \
  --check
```
