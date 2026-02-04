target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stack = alloca [8 x i8]
  %stackPtr = ptrtoint [8 x i8]* %stack to i32
  %var1 = add i32 %stackPtr, -8
  %ptr1 = inttoptr i32 %var1 to i32*
  %val1 = load i32, i32* %ptr1
  %eq1 = icmp eq i32 %val1, %_arg_0
  %var2 = add i32 %stackPtr, -4
  %ptr2 = inttoptr i32 %var2 to float*
  %fval2 = load float, float* %ptr2
  %val2 = fptosi float %fval2 to i32
  %eq2 = icmp eq i32 %val2, %_arg_1
  %eq3 = or i1 %eq1, %eq2
  br i1 %eq3, label %if_true, label %if_false

if_true:
  ret i32 1

if_false:
  ret i32 0
}

