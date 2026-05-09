target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal void @rec1(i32 %_arg_0) {
allocator:
  %0 = inttoptr i32 %_arg_0 to ptr
  %1 = load i32, ptr %0, align 4
  call void @rec1(i32 %1)
  ret void
}

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %0 = alloca [8 x i8]
  %1 = ptrtoint ptr %0 to i32
  call void @rec1(i32 %1)
  ret i32 0
}
