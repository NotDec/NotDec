target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"


define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
entry:
  %a = inttoptr i32 1024 to i32*
  %b = load i32, i32* %a, align 4
  ret i32 %b
}
