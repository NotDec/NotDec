; ModuleID = 'test/wasm/hello.wat'
source_filename = "test/wasm/hello.wat"

@mem0 = external global [65536 x i8]
@mem0.1 = internal global [65536 x i8] zeroinitializer

declare void @"$log"(i32, i32)

define void @0() {
allocator:
  br label %entry

entry:                                            ; preds = %allocator
  call void @"$log"(i32 0, i32 29)
  br label %return

return:                                           ; preds = %entry
  ret void
}
