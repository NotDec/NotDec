target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"


define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %iptr = alloca i32
  store i32 0, ptr %iptr
  %i1 = load i32, ptr %iptr
  ; store i32 1, ptr %iptr
  ret i32 %i1
}
