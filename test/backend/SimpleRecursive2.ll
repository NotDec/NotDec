target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal void @rec1(i32 %_arg_0) {
allocator:
  %v3 = inttoptr i32 %_arg_0 to i32*
  %v4 = load i32, i32* %v3, align 4
  call void @rec1(i32 %v4)
  ret void
}

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stackPtr = alloca [8 x i8]
  %stack = ptrtoint [8 x i8]* %stackPtr to i32
  call void @rec1(i32 %stack)
  ret i32 0
}

