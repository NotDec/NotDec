; ModuleID = './test/wasm/stack-seq-param.wat'
source_filename = "./test/wasm/stack-seq-param.wat"

@mem0 = external global [65536 x i8]
@mem0.1 = internal global [65536 x i8] zeroinitializer

declare i32 @"$test"(float, i32)

define i32 @0() {
allocator:
  br label %entry

entry:                                            ; preds = %allocator
  %0 = call i32 @"$test"(float 2.900000e+01, i32 0)
  br label %return

return:                                           ; preds = %entry
  %entry_0 = phi i32 [ %0, %entry ]
  ret i32 %entry_0
}
