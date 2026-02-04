target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"


declare void @isFloat(float)
declare void @isInt(i32)

define internal i32 @ptrFloat(i32 %_arg_0) {
allocator:
  %v_122 = inttoptr i32 %_arg_0 to float*
  %f1 = load float, float* %v_122, align 4
  call void @isFloat(float %f1)
  %vret1 = call i32 @pass(i32 %_arg_0)
  ret i32 %vret1
}

define internal i32 @ptrInt(i32 %_arg_0) {
allocator:
  %v_121 = inttoptr i32 %_arg_0 to i32*
  %int1 = load i32, i32* %v_121, align 4
  %int2 = mul i32 %int1, 5
  call void @isInt(i32 %int2)
  %vret2 = call i32 @pass(i32 %_arg_0)
  ret i32 %vret2
}

define internal i32 @pass(i32 %_arg_0) {
allocator:
  ret i32 %_arg_0
}

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stackPtr1 = alloca [4 x i8]
  %stack1 = ptrtoint [4 x i8]* %stackPtr1 to i32
  %val1 = call i32 @ptrFloat(i32 %stack1)
  %eq1 = icmp eq i32 %val1, %_arg_0
  %stackPtr2 = alloca [4 x i8]
  %stack2 = ptrtoint [4 x i8]* %stackPtr2 to i32
  %val2 = call i32 @ptrInt(i32 %stack2)
  %eq2 = icmp eq i32 %val2, %_arg_1
  %eq3 = or i1 %eq1, %eq2
  br i1 %eq3, label %if_true, label %if_false

if_true:
  ret i32 1

if_false:
  ret i32 0
}

