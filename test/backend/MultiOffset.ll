; ModuleID = '/sn640/NotDec/test/backend/BottomUp1.ll'
source_filename = "/sn640/NotDec/test/backend/BottomUp1.ll"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stack1 = alloca [8 x i8], align 4
  %stack_1p = ptrtoint [8 x i8]* %stack1 to i32
  %Off1 = add i32 %stack_1p, 4
  %mem1 = inttoptr i32 %stack_1p to i32*
  store i32 0, i32* %mem1, align 4
  %mem2 = inttoptr i32 %Off1 to i32*
  store i32 1, i32* %mem2, align 4
  ret i32 %stack_1p
}
