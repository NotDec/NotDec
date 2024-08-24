# Changelog & TODO

## TODO

- Split into more cmake sub-project.
- Add more comments.
- Write docs in English.

**Backend**
- make null pointer constant really NULL instead of 0.
- add typedef for boolean constant
- handle unordered float pointer operation

**Type recovery**
- Check for any possible name collision
- how to handle dynamic call using function pointer? how to traverse reverse post order?
- Add a test case for basic block not in topo order. Check whether opt passes will reorder it.

## Changelog

**2024-07-15**

If address sanitizer is enabled, my VSCode clangd language server does not work and reports errors that included file: 'sanitizer/asan_interface.h' not found. But the project just builds and works fine.

- To find the header file, create a file called test.c. Run `clang++ -E -v -### -x c++ test.c`. And I found that the header is under `/usr/lib/llvm-14/lib/clang/14.0.0/include`

1. Try to rebuild my debug version of LLVM with compiler-rt. 
   1. Changed the build script: `-DLLVM_ENABLE_PROJECTS="clang;compiler-rt"`
   2. Changed the cmake file in llvm compiler rt cmake files according to this [issue](https://github.com/llvm/llvm-project/issues/67085). 

These does not work.

Final solution: Add `-DCMAKE_CXX_FLAGS=-fsanitize=address -idirafter /usr/lib/llvm-14/lib/clang/14.0.0/include`

**2024-04-26**

1. migrate structural analysis from reko.
2. add reduceSelf: A -> B -> C, B is an empty block, and B has only one succ and only one pred.
3. consider reducing return stmt before SSA destruction. Because: `tmp1 = 10; goto ret; ret: tmp0 = tmp1; return tmp0`

**2024-04-27**
1. The simplify CFG pass in LLVM has a branch-fold-threshold. We prefer to set it to 0, so that the folding of some (not all because of the threshold) logical and/or is not done in the IR level. However it is a static clang opt; it can be set by cmdline but not API. We will have to leave it enable.

**2024-04-28**
1. The simplify CFG pass will merge all return block to create a common exit block, which is undesirable.
2. the ideal process: fully optimized IR in SSA -> return duplication -> SSA destruction -> structural analysis
3. our statement inserter requires the CFG is simplified, because in the case of `A: %1 = bitcast 0 to i32; B: ret %1`, we will find the `%1` used by other blocks and create a variable for it.
