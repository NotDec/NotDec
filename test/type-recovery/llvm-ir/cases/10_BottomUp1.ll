target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"


declare void @isFloat(float)

define internal void @ptrFloat(i32 %_arg_0) {
allocator:
  %v_12 = inttoptr i32 %_arg_0 to float*
  %f1 = load float, float* %v_12, align 4
  call void @isFloat(float %f1)
  ret void
}

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stackPtr = alloca [4 x i8]
  %stack = ptrtoint [4 x i8]* %stackPtr to i32
  call void @ptrFloat(i32 %stack)
  ret i32 0
}

