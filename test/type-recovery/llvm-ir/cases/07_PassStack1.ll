target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal i32 @fun_1(i32 %_arg_0) {
entry:
  %stack = alloca [0 x i8]
  %addr = inttoptr i32 %_arg_0 to ptr
  store i32 %_arg_0, ptr %addr
  store i32 %_arg_0, ptr %stack
  ret i32 %_arg_0
}

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
entry:
  %stack = alloca [0 x i8]
  %stacki = ptrtoint ptr %stack to i32
  %a = call i32 @fun_1(i32 %stacki)
  ret i32 %a
}
