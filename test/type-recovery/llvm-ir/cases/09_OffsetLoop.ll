; ModuleID = '/sn640/NotDec/test/backend/BottomUp1.ll'
source_filename = "/sn640/NotDec/test/backend/BottomUp1.ll"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stack1 = alloca [16 x i8], align 4
  %stack1p = ptrtoint ptr %stack1 to i32
  br label %loop

loop:
  %p1 = phi i32 [ %stack1p, %allocator ], [ %p2, %loop ]
  %p2 = add i32 %p1, 4
  %new_9 = inttoptr i32 %p2 to ptr
  store i32 0, ptr %new_9, align 4
  %cmp = icmp slt i32 %p2, %_arg_0
  br i1 %cmp, label %loop, label %exit

exit:
  ret i32 0
}
