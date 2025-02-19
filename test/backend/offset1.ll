; ModuleID = 'test/backend/offset1.wat'
source_filename = "test/backend/offset1.wat"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

@__notdec_mem0 = external global [65536 x i8]
@env.__stack_pointer = external global i32
@env.__indirect_function_table = external global [0 x void ()*]
@__notdec_mem0_0x0 = internal global [4 x i8] c"%p\0A\00", section ".addr_0x0", align 1
@env.__indirect_function_table.1 = external global [0 x void ()*]

declare i32 @env.printf(i32, i32)

define internal void @do_printf(i32 %_arg_0) {
allocator:
  %stack_addr2 = alloca [16 x i8], align 1
  %stack = ptrtoint [16 x i8]* %stack_addr2 to i32
  %stack_end = add i32 %stack, -16
  %calcOffset = add i32 %stack, -4
  %v_12 = inttoptr i32 %calcOffset to i32*
  store i32 %_arg_0, i32* %v_12, align 4
  %v_17 = inttoptr i32 %stack_end to i32*
  store i32 %calcOffset, i32* %v_17, align 4
  %0 = call i32 @env.printf(i32 0, i32 %stack_end)
  ret void
}

define internal i32 @get_s(i32 %_arg_0, i32 %_arg_1) {
allocator:
  %stack_addr5 = alloca [0 x i8], align 1
  %stack = ptrtoint [0 x i8]* %stack_addr5 to i32
  %calcOffset = add i32 %stack, -4
  %v_81 = inttoptr i32 %calcOffset to i32*
  store i32 %_arg_0, i32* %v_81, align 4
  %calcOffset1 = add i32 %stack, -8
  %v_89 = inttoptr i32 %calcOffset1 to i32*
  store i32 %_arg_1, i32* %v_89, align 4
  %calcOffset2 = add i32 %stack, -4
  %v_95 = inttoptr i32 %calcOffset2 to i32*
  %loadResult = load i32, i32* %v_95, align 4
  %add_98 = add i32 %loadResult, 2
  %calcOffset3 = add i32 %stack, -8
  %v_103 = inttoptr i32 %calcOffset3 to i32*
  %loadResult4 = load i32, i32* %v_103, align 4
  %0 = shl i32 %loadResult4, 2
  %add_107 = add i32 %add_98, %0
  ret i32 %add_107
}

define internal void @test1() {
allocator:
  %stack_addr8 = alloca [32 x i8], align 1
  %stack = ptrtoint [32 x i8]* %stack_addr8 to i32
  %add_184 = add i32 %stack, -4
  call void @do_printf(i32 %add_184)
  %add_193 = add i32 %stack, -20
  call void @do_printf(i32 %add_193)
  %calcOffset = add i32 %stack, -24
  %v_200 = inttoptr i32 %calcOffset to i32*
  store i32 0, i32* %v_200, align 4
  %calcOffset1 = add i32 %stack, -24
  %v_206 = inttoptr i32 %calcOffset1 to i32*
  br label %brif_next

blk_exit:                                         ; preds = %brif_next
  ret void

brif_next:                                        ; preds = %allocator, %brif_next
  %loadResult39 = phi i32 [ 0, %allocator ], [ %loadResult.pre, %brif_next ]
  %add_210 = add i32 %stack, -14
  %0 = shl i32 %loadResult39, 2
  %add_215 = add i32 %add_210, %0
  call void @do_printf(i32 %add_215)
  %loadResult5 = load i32, i32* %v_206, align 4
  %add_219 = add i32 %loadResult5, 1
  store i32 %add_219, i32* %v_206, align 4
  %loadResult.pre = load i32, i32* %v_200, align 4
  %1 = icmp sgt i32 %loadResult.pre, 1
  br i1 %1, label %blk_exit, label %brif_next
}

define internal void @test2() {
allocator:
  %stack_addr8 = alloca [32 x i8], align 1
  %stack = ptrtoint [32 x i8]* %stack_addr8 to i32
  %add_310 = add i32 %stack, -4
  call void @do_printf(i32 %add_310)
  %add_319 = add i32 %stack, -20
  call void @do_printf(i32 %add_319)
  %calcOffset = add i32 %stack, -24
  %v_326 = inttoptr i32 %calcOffset to i32*
  store i32 0, i32* %v_326, align 4
  %calcOffset1 = add i32 %stack, -24
  %v_332 = inttoptr i32 %calcOffset1 to i32*
  br label %brif_next

blk_exit:                                         ; preds = %brif_next
  ret void

brif_next:                                        ; preds = %allocator, %brif_next
  %loadResult39 = phi i32 [ 0, %allocator ], [ %loadResult.pre, %brif_next ]
  %add_336 = add i32 %stack, -16
  %0 = call i32 @get_s(i32 %add_336, i32 %loadResult39)
  call void @do_printf(i32 %0)
  %loadResult5 = load i32, i32* %v_332, align 4
  %add_352 = add i32 %loadResult5, 1
  store i32 %add_352, i32* %v_332, align 4
  %loadResult.pre = load i32, i32* %v_326, align 4
  %1 = icmp sgt i32 %loadResult.pre, 1
  br i1 %1, label %blk_exit, label %brif_next
}

define internal i32 @__original_main() {
allocator:
  call void @test1()
  call void @test2()
  ret i32 0
}
