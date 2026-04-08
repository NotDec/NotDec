; ModuleID = '/sn640/NotDec/test/backend/BottomUp1.ll'
source_filename = "/sn640/NotDec/test/backend/BottomUp1.ll"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

@__notdec_mem0 = global [16777216 x i8] zeroinitializer

define internal i32 @passMem1() {
allocator:
  %memptr1 = inttoptr i32 1024 to i32*
  %f1 = load i32, i32* %memptr1, align 4
  ret i32 %f1
}

define internal i32 @main(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %memptr1 = inttoptr i32 1024 to i32*
  store i32 0, i32* %memptr1, align 4
  %r = call i32 @passMem1()
  ret i32 %r
}
