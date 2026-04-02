target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"


declare void @isFloat(float)

define internal void @ptrFloat(i32 %_arg_0) {
allocator:
  %v1 = add i32 %_arg_0, 4
  %v2 = inttoptr i32 %v1 to float*
  %f1 = load float, float* %v2, align 4
  call void @isFloat(float %f1)
  %v3 = inttoptr i32 %_arg_0 to i32*
  store i32 %_arg_0, i32* %v3, align 4
  ret void
}

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stackPtr = alloca [8 x i8]
  %stack = ptrtoint [8 x i8]* %stackPtr to i32
  call void @ptrFloat(i32 %stack)
  ret i32 0
}

