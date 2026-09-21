# Pre-TR helper inlining experiment (2026-09-20)

## solc mechanism that creates the helpers
- libyul/optimiser/FullInliner.cpp shallInline(): recursive/side-effecting-args never; size<=1 always;
  single-use always; otherwise inline only if size < 6/8 (or constant-arg < 12/16) -- bigger
  multi-called functions stay as Yul functions.
- libyul/backends/evm/EVMCodeTransform.cpp:244: a kept function call emits a returnLabel
  (pushed on the stack) + jump -> decompiled as the (mem,calldata,returndata,env,...,target/pc) helper.
- EquivalentFunctionCombiner merges equivalent Yul functions; legacy libevmasm/BlockDeduplicator
  merges identical assembly blocks.

## Experiment A: broad pre-TR inline (opt -passes=inline, triple patched to host and back)
entries 2325 | record==annotated 2245 | record> 75 | record< 5 | params 1999 | typed 1985 (99.3%)
missing 14 | extra 121/76 | IR 571203 -> 562057 lines | pattern suite 19/103 PASS

## Experiment B: targeted inline (only calldata-reader chains; all other defined fns noinline)
entries 2316 | record==annotated 2259 | record> 37 | record< 20 | params 2163 | typed 2120 (98.0%)
missing 43 | extra 47/38 | selected 1098 helper functions | IR 571203 -> 669225 (+17%)
pattern suite 38/103 PASS; compile budget:
  compile_failures 3 (budget 0), unresolved 286 (264), condition_todo 3475 (729),
  goto 3105 (424), real_jump 2683 (343), dangling 407 (69), helpers 1050 (floor 1339),
  call_sites 972 (floor 1301)

## Baseline (views, no inlining)
entries 2316 | record==annotated 1858 | record> 150 | record< 308 | params 1780 | typed 1579 (88.7%)
missing 201 | extra 270/150

## Conclusion
Inlining restores the pre-dedup (source-like) shape: helper-decoded words become direct constant-offset
reads, so the per-entry record and the parameter table are nearly complete.  But it also duplicates
shared revert/bounds checks and enlarges functions, which breaks the structurer and every pattern
oracle calibrated on the outlined shape.  Pipeline-wide inlining is therefore not adoptable as-is;
the ABI gain has to be captured either on a sidecar (inlined) module used only for type recovery, or
by materializing the decoder's absolute offsets into the entry view as TR-only evidence.