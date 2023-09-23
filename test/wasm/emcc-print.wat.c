declare  __wasi_fd_seek {}

declare  __wasi_fd_read {}

declare  __wasi_fd_close {}

declare  __wasi_proc_exit {}

declare  __wasi_fd_write {}

define __wasm_call_ctors {
allocator:
  call void @emscripten_stack_init()
  ret void
}

define main {
allocator:
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-4_ = alloca i32, align 4
  %stack_var_-16_ = alloca i32, align 4
  %0 = ptrtoint i32* %stack_var_-16_ to i32
  store i32 0, i32* %stack_var_-4_, align 4
  %1 = call i32 @printf(i32 1078, i32 0)
  %2 = call i32 @getchar()
  %3 = icmp eq i32 %2, 0
  br i1 %3, label %blk_exit2, label %brif_next

blk_exit:                                         ; preds = %brif_next, %blk_exit2
  %storemerge = phi i32 [ 1, %brif_next ], [ 0, %blk_exit2 ]
  br label %loop_entry

blk_exit2:                                        ; preds = %allocator
  %4 = call i32 @printf(i32 1112, i32 0)
  %5 = call i32 @printf(i32 1104, i32 0)
  br label %blk_exit

brif_next:                                        ; preds = %allocator
  br label %blk_exit

loop_entry:                                       ; preds = %blk_exit5, %brif_next8, %blk_exit
  %_local_11.0 = phi i32 [ %storemerge, %blk_exit ], [ 1, %brif_next8 ], [ 0, %blk_exit5 ]
  %cond = icmp eq i32 %_local_11.0, 0
  br i1 %cond, label %blk_exit6, label %blk_exit5

blk_exit4:                                        ; preds = %blk_exit6
  %6 = ptrtoint i32* %stack_var_0_ to i32
  ret i32 0

blk_exit5:                                        ; preds = %loop_entry
  %7 = call i32 @printf(i32 1091, i32 0)
  br label %loop_entry

blk_exit6:                                        ; preds = %loop_entry
  %8 = call i32 @getchar()
  %9 = icmp eq i32 %8, 0
  br i1 %9, label %blk_exit4, label %brif_next8

brif_next8:                                       ; preds = %blk_exit6
  %10 = call i32 @printf(i32 1084, i32 0)
  br label %loop_entry
}

define _start {
allocator:
  call void @__wasm_call_ctors()
  %0 = call i32 @main()
  call void @exit(i32 %0)
  unreachable
}

define getchar {
allocator:
  %loadResult = load i32, i32* bitcast (i8* getelementptr inbounds ([16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 1120) to i32*), align 16
  %0 = call i32 @fgetc(i32 %loadResult)
  ret i32 %0
}

define __stdio_seek {
allocator:
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-8_ = alloca i64, align 8
  %stack_var_-16_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = ptrtoint i32* %stack_var_-16_ to i32
  %calcOffset = add i32 %i32, 60
  %2 = sext i32 %calcOffset to i64
  %3 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %2
  %4 = bitcast i8* %3 to i32*
  %loadResult = load i32, i32* %4, align 4
  %5 = and i32 %i321, 255
  %6 = add i32 %0, -8
  %7 = call i32 @__wasi_fd_seek(i32 %loadResult, i64 %i64, i32 %5, i32 %6)
  %8 = call i32 @__wasi_syscall_ret(i32 %7)
  %9 = load i64, i64* %stack_var_-8_, align 4
  %10 = ptrtoint i32* %stack_var_0_ to i32
  %select_cond.not = icmp eq i32 %8, 0
  %11 = select i1 %select_cond.not, i64 %9, i64 -1
  ret i64 %11
}

define __stdio_read {
allocator:
  %stack_var_-20_ = alloca i32, align 4
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-8_ = alloca i32, align 4
  %stack_var_-4_ = alloca i32, align 4
  %stack_var_-12_ = alloca i32, align 4
  %stack_var_-16_ = alloca i32, align 4
  %stack_var_-32_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = ptrtoint i32* %stack_var_-32_ to i32
  store i32 %i321, i32* %stack_var_-16_, align 4
  %calcOffset3 = add i32 %i32, 48
  %2 = sext i32 %calcOffset3 to i64
  %3 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %2
  %4 = bitcast i8* %3 to i32*
  %loadResult = load i32, i32* %4, align 4
  %5 = icmp ne i32 %loadResult, 0
  %.neg = sext i1 %5 to i32
  %6 = add i32 %.neg, %i322
  store i32 %6, i32* %stack_var_-12_, align 4
  %calcOffset5 = add i32 %i32, 44
  %7 = sext i32 %calcOffset5 to i64
  %8 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %7
  %9 = bitcast i8* %8 to i32*
  %loadResult6 = load i32, i32* %9, align 4
  store i32 %loadResult, i32* %stack_var_-4_, align 4
  store i32 %loadResult6, i32* %stack_var_-8_, align 4
  %calcOffset11 = add i32 %i32, 60
  %10 = sext i32 %calcOffset11 to i64
  %11 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %10
  %12 = bitcast i8* %11 to i32*
  %loadResult12 = load i32, i32* %12, align 4
  %13 = add i32 %0, -16
  %14 = add i32 %0, -20
  %15 = call i32 @__wasi_fd_read(i32 %loadResult12, i32 %13, i32 2, i32 %14)
  %16 = call i32 @__wasi_syscall_ret(i32 %15)
  %brif_val.not = icmp eq i32 %16, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit10

blk_exit:                                         ; preds = %blk_exit28, %blk_exit9, %blk_exit10
  %_local_4.0 = phi i32 [ %i322, %blk_exit28 ], [ %25, %blk_exit9 ], [ %_local_4.1, %blk_exit10 ]
  %17 = ptrtoint i32* %stack_var_0_ to i32
  ret i32 %_local_4.0

blk_exit9:                                        ; preds = %brif_next
  %18 = load i32, i32* %stack_var_-12_, align 4
  %.not = icmp ugt i32 %25, %18
  br i1 %.not, label %brif_next23, label %blk_exit

blk_exit10:                                       ; preds = %brif_next, %allocator
  %_local_4.1 = phi i32 [ %25, %brif_next ], [ -1, %allocator ]
  %19 = and i32 %_local_4.1, 48
  %20 = xor i32 %19, 16
  %21 = sext i32 %i32 to i64
  %22 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %21
  %23 = bitcast i8* %22 to i32*
  %loadResult18 = load i32, i32* %23, align 4
  %24 = or i32 %20, %loadResult18
  store i32 %24, i32* %23, align 4
  br label %blk_exit

brif_next:                                        ; preds = %allocator
  %25 = load i32, i32* %stack_var_-20_, align 4
  %26 = icmp sgt i32 %25, 0
  br i1 %26, label %blk_exit9, label %blk_exit10

brif_next23:                                      ; preds = %blk_exit9
  %loadResult25 = load i32, i32* %9, align 4
  %calcOffset26 = add i32 %i32, 4
  %27 = sext i32 %calcOffset26 to i64
  %28 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %27
  %29 = bitcast i8* %28 to i32*
  store i32 %loadResult25, i32* %29, align 4
  %30 = sub i32 %25, %18
  %31 = add i32 %loadResult25, %30
  %calcOffset27 = add i32 %i32, 8
  %32 = sext i32 %calcOffset27 to i64
  %33 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %32
  %34 = bitcast i8* %33 to i32*
  store i32 %31, i32* %34, align 4
  %loadResult30 = load i32, i32* %4, align 4
  %35 = icmp eq i32 %loadResult30, 0
  br i1 %35, label %blk_exit28, label %brif_next32

blk_exit28:                                       ; preds = %brif_next32, %brif_next23
  br label %blk_exit

brif_next32:                                      ; preds = %brif_next23
  %36 = add i32 %loadResult25, 1
  store i32 %36, i32* %29, align 4
  %37 = add i32 %i322, %i321
  %38 = add i32 %37, -1
  %39 = sext i32 %loadResult25 to i64
  %40 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %39
  %loadResult3537 = load i8, i8* %40, align 1
  %41 = sext i32 %38 to i64
  %42 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %41
  store i8 %loadResult3537, i8* %42, align 1
  br label %blk_exit28
}

define dummy {
allocator:
  ret i32 %i32
}

define __stdio_close {
allocator:
  %calcOffset = add i32 %i32, 60
  %0 = sext i32 %calcOffset to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %2 = bitcast i8* %1 to i32*
  %loadResult = load i32, i32* %2, align 4
  %3 = call i32 @dummy(i32 %loadResult)
  %4 = call i32 @__wasi_fd_close(i32 %3)
  ret i32 %4
}

define __ofl_lock {
allocator:
  call void @__lock(i32 3176)
  ret i32 3184
}

define __stdio_exit {
allocator:
  %0 = call i32 @__ofl_lock()
  %1 = sext i32 %0 to i64
  %2 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %1
  %3 = bitcast i8* %2 to i32*
  %loadResult = load i32, i32* %3, align 4
  %4 = icmp eq i32 %loadResult, 0
  br i1 %4, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %loop_entry, %allocator
  %loadResult5 = load i32, i32* bitcast (i8* getelementptr inbounds ([16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 1760) to i32*), align 16
  call void @close_file(i32 %loadResult5)
  %loadResult6 = load i32, i32* bitcast (i8* getelementptr inbounds ([16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 1912) to i32*), align 8
  call void @close_file(i32 %loadResult6)
  ret void

loop_entry:                                       ; preds = %allocator, %loop_entry
  %_local_0.0 = phi i32 [ %loadResult, %allocator ], [ %loadResult2, %loop_entry ]
  call void @close_file(i32 %_local_0.0)
  %calcOffset1 = add i32 %_local_0.0, 56
  %5 = sext i32 %calcOffset1 to i64
  %6 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %5
  %7 = bitcast i8* %6 to i32*
  %loadResult2 = load i32, i32* %7, align 4
  %brif_val3.not = icmp eq i32 %loadResult2, 0
  br i1 %brif_val3.not, label %blk_exit, label %loop_entry
}

define close_file {
allocator:
  %0 = icmp eq i32 %i32, 0
  br i1 %0, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %brif_next18, %blk_exit4, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  %calcOffset = add i32 %i32, 76
  %1 = sext i32 %calcOffset to i64
  %2 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %1
  %3 = bitcast i8* %2 to i32*
  %loadResult = load i32, i32* %3, align 4
  %brif_val2.not = icmp sgt i32 %loadResult, -1
  br i1 %brif_val2.not, label %brif_next3, label %blk_exit1

blk_exit1:                                        ; preds = %brif_next3, %brif_next
  %calcOffset5 = add i32 %i32, 20
  %4 = sext i32 %calcOffset5 to i64
  %5 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %4
  %6 = bitcast i8* %5 to i32*
  %loadResult6 = load i32, i32* %6, align 4
  %calcOffset7 = add i32 %i32, 28
  %7 = sext i32 %calcOffset7 to i64
  %8 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %7
  %9 = bitcast i8* %8 to i32*
  %loadResult8 = load i32, i32* %9, align 4
  %.not = icmp ugt i32 %loadResult6, %loadResult8
  br i1 %.not, label %brif_next10, label %blk_exit4

brif_next3:                                       ; preds = %brif_next
  %10 = call i32 @__lockfile(i32 %i32)
  br label %blk_exit1

blk_exit4:                                        ; preds = %brif_next10, %blk_exit1
  %calcOffset13 = add i32 %i32, 4
  %11 = sext i32 %calcOffset13 to i64
  %12 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %11
  %13 = bitcast i8* %12 to i32*
  %loadResult14 = load i32, i32* %13, align 4
  %calcOffset15 = add i32 %i32, 8
  %14 = sext i32 %calcOffset15 to i64
  %15 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %14
  %16 = bitcast i8* %15 to i32*
  %loadResult16 = load i32, i32* %16, align 4
  %.not23 = icmp ult i32 %loadResult14, %loadResult16
  br i1 %.not23, label %brif_next18, label %blk_exit

brif_next10:                                      ; preds = %blk_exit1
  %calcOffset11 = add i32 %i32, 36
  %17 = sext i32 %calcOffset11 to i64
  %18 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %17
  %19 = bitcast i8* %18 to i32*
  %loadResult12 = load i32, i32* %19, align 4
  %20 = sext i32 %loadResult12 to i64
  %21 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %20
  %22 = bitcast void ()** %21 to i32 (i32, i32, i32)**
  %callind_funcptr22 = load i32 (i32, i32, i32)*, i32 (i32, i32, i32)** %22, align 8
  %23 = call i32 %callind_funcptr22(i32 %i32, i32 0, i32 0)
  br label %blk_exit4

brif_next18:                                      ; preds = %blk_exit4
  %24 = sub i32 %loadResult14, %loadResult16
  %25 = sext i32 %24 to i64
  %calcOffset19 = add i32 %i32, 40
  %26 = sext i32 %calcOffset19 to i64
  %27 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %26
  %28 = bitcast i8* %27 to i32*
  %loadResult20 = load i32, i32* %28, align 4
  %29 = sext i32 %loadResult20 to i64
  %30 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %29
  %31 = bitcast void ()** %30 to i64 (i32, i64, i32)**
  %callind_funcptr2124 = load i64 (i32, i64, i32)*, i64 (i32, i64, i32)** %31, align 8
  %32 = call i64 %callind_funcptr2124(i32 %i32, i64 %25, i32 1)
  br label %blk_exit
}

define __toread {
allocator:
  %calcOffset = add i32 %i32, 74
  %0 = sext i32 %calcOffset to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %loadResult = load i8, i8* %1, align 1
  %2 = add i8 %loadResult, -1
  %3 = or i8 %2, %loadResult
  store i8 %3, i8* %1, align 1
  %calcOffset2 = add i32 %i32, 20
  %4 = sext i32 %calcOffset2 to i64
  %5 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %4
  %6 = bitcast i8* %5 to i32*
  %loadResult3 = load i32, i32* %6, align 4
  %calcOffset4 = add i32 %i32, 28
  %7 = sext i32 %calcOffset4 to i64
  %8 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %7
  %9 = bitcast i8* %8 to i32*
  %loadResult5 = load i32, i32* %9, align 4
  %.not = icmp ugt i32 %loadResult3, %loadResult5
  br i1 %.not, label %brif_next, label %blk_exit

return:                                           ; preds = %blk_exit10, %brif_next14
  %return_0 = phi i32 [ -1, %brif_next14 ], [ %38, %blk_exit10 ]
  ret i32 %return_0

blk_exit:                                         ; preds = %brif_next, %allocator
  store i32 0, i32* %9, align 4
  %calcOffset9 = add i32 %i32, 16
  %10 = sext i32 %calcOffset9 to i64
  %11 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %10
  %12 = bitcast i8* %11 to i64*
  store i64 0, i64* %12, align 4
  %13 = sext i32 %i32 to i64
  %14 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %13
  %15 = bitcast i8* %14 to i32*
  %loadResult12 = load i32, i32* %15, align 4
  %16 = and i32 %loadResult12, 4
  %brif_val13.not.not = icmp eq i32 %16, 0
  br i1 %brif_val13.not.not, label %blk_exit10, label %brif_next14

brif_next:                                        ; preds = %allocator
  %calcOffset6 = add i32 %i32, 36
  %17 = sext i32 %calcOffset6 to i64
  %18 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %17
  %19 = bitcast i8* %18 to i32*
  %loadResult7 = load i32, i32* %19, align 4
  %20 = sext i32 %loadResult7 to i64
  %21 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %20
  %22 = bitcast void ()** %21 to i32 (i32, i32, i32)**
  %callind_funcptr22 = load i32 (i32, i32, i32)*, i32 (i32, i32, i32)** %22, align 8
  %23 = call i32 %callind_funcptr22(i32 %i32, i32 0, i32 0)
  br label %blk_exit

blk_exit10:                                       ; preds = %blk_exit
  %calcOffset16 = add i32 %i32, 44
  %24 = sext i32 %calcOffset16 to i64
  %25 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %24
  %26 = bitcast i8* %25 to i32*
  %loadResult17 = load i32, i32* %26, align 4
  %calcOffset18 = add i32 %i32, 48
  %27 = sext i32 %calcOffset18 to i64
  %28 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %27
  %29 = bitcast i8* %28 to i32*
  %loadResult19 = load i32, i32* %29, align 4
  %30 = add i32 %loadResult17, %loadResult19
  %calcOffset20 = add i32 %i32, 8
  %31 = sext i32 %calcOffset20 to i64
  %32 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %31
  %33 = bitcast i8* %32 to i32*
  store i32 %30, i32* %33, align 4
  %calcOffset21 = add i32 %i32, 4
  %34 = sext i32 %calcOffset21 to i64
  %35 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %34
  %36 = bitcast i8* %35 to i32*
  store i32 %30, i32* %36, align 4
  %37 = shl i32 %loadResult12, 27
  %38 = ashr i32 %37, 31
  br label %return

brif_next14:                                      ; preds = %blk_exit
  %39 = or i32 %loadResult12, 32
  store i32 %39, i32* %15, align 4
  br label %return
}

define __uflow {
allocator:
  %stack_var_-1_ = alloca i8, align 1
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-16_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = ptrtoint i32* %stack_var_-16_ to i32
  %2 = call i32 @__toread(i32 %i32)
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next2, %brif_next, %allocator
  %_local_2.0 = phi i32 [ %13, %brif_next2 ], [ -1, %brif_next ], [ -1, %allocator ]
  %3 = ptrtoint i32* %stack_var_0_ to i32
  ret i32 %_local_2.0

brif_next:                                        ; preds = %allocator
  %4 = add i32 %0, -1
  %calcOffset = add i32 %i32, 32
  %5 = sext i32 %calcOffset to i64
  %6 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %5
  %7 = bitcast i8* %6 to i32*
  %loadResult = load i32, i32* %7, align 4
  %8 = sext i32 %loadResult to i64
  %9 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %8
  %10 = bitcast void ()** %9 to i32 (i32, i32, i32)**
  %callind_funcptr5 = load i32 (i32, i32, i32)*, i32 (i32, i32, i32)** %10, align 8
  %11 = call i32 %callind_funcptr5(i32 %i32, i32 %4, i32 1)
  %.not = icmp eq i32 %11, 1
  br i1 %.not, label %brif_next2, label %blk_exit

brif_next2:                                       ; preds = %brif_next
  %12 = load i8, i8* %stack_var_-1_, align 1
  %13 = zext i8 %12 to i32
  br label %blk_exit
}

define fgetc {
allocator:
  %calcOffset = add i32 %i32, 76
  %0 = sext i32 %calcOffset to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %2 = bitcast i8* %1 to i32*
  %loadResult = load i32, i32* %2, align 4
  %brif_val.not = icmp sgt i32 %loadResult, -1
  br i1 %brif_val.not, label %brif_next, label %blk_exit1

return:                                           ; preds = %blk_exit14, %blk_exit4, %brif_next10
  %return_0 = phi i32 [ %20, %brif_next10 ], [ %16, %blk_exit4 ], [ %storemerge, %blk_exit14 ]
  ret i32 %return_0

blk_exit:                                         ; preds = %brif_next
  %calcOffset16 = add i32 %i32, 4
  %3 = sext i32 %calcOffset16 to i64
  %4 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %3
  %5 = bitcast i8* %4 to i32*
  %loadResult17 = load i32, i32* %5, align 4
  %calcOffset18 = add i32 %i32, 8
  %6 = sext i32 %calcOffset18 to i64
  %7 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %6
  %8 = bitcast i8* %7 to i32*
  %loadResult19 = load i32, i32* %8, align 4
  %.not25 = icmp ult i32 %loadResult17, %loadResult19
  br i1 %.not25, label %brif_next21, label %blk_exit15

blk_exit1:                                        ; preds = %brif_next, %allocator
  %calcOffset5 = add i32 %i32, 4
  %9 = sext i32 %calcOffset5 to i64
  %10 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %9
  %11 = bitcast i8* %10 to i32*
  %loadResult6 = load i32, i32* %11, align 4
  %calcOffset7 = add i32 %i32, 8
  %12 = sext i32 %calcOffset7 to i64
  %13 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %12
  %14 = bitcast i8* %13 to i32*
  %loadResult8 = load i32, i32* %14, align 4
  %.not = icmp ult i32 %loadResult6, %loadResult8
  br i1 %.not, label %brif_next10, label %blk_exit4

brif_next:                                        ; preds = %allocator
  %15 = call i32 @__lockfile(i32 %i32)
  %brif_val2.not = icmp eq i32 %15, 0
  br i1 %brif_val2.not, label %blk_exit1, label %blk_exit

blk_exit4:                                        ; preds = %blk_exit1
  %16 = call i32 @__uflow(i32 %i32)
  br label %return

brif_next10:                                      ; preds = %blk_exit1
  %17 = add i32 %loadResult6, 1
  store i32 %17, i32* %11, align 4
  %18 = sext i32 %loadResult6 to i64
  %19 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %18
  %loadResult13 = load i8, i8* %19, align 1
  %20 = zext i8 %loadResult13 to i32
  br label %return

blk_exit14:                                       ; preds = %blk_exit15, %brif_next21
  %storemerge = phi i32 [ %25, %brif_next21 ], [ %21, %blk_exit15 ]
  call void @__unlockfile(i32 %i32)
  br label %return

blk_exit15:                                       ; preds = %blk_exit
  %21 = call i32 @__uflow(i32 %i32)
  br label %blk_exit14

brif_next21:                                      ; preds = %blk_exit
  %22 = add i32 %loadResult17, 1
  store i32 %22, i32* %5, align 4
  %23 = sext i32 %loadResult17 to i64
  %24 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %23
  %loadResult24 = load i8, i8* %24, align 1
  %25 = zext i8 %loadResult24 to i32
  br label %blk_exit14
}

define __lock {
allocator:
  ret void
}

define __errno_location {
allocator:
  ret i32 3188
}

define __wasi_syscall_ret {
allocator:
  %brif_val.not = icmp eq i32 %i32, 0
  br i1 %brif_val.not, label %return, label %blk_exit

return:                                           ; preds = %allocator, %blk_exit
  %return_0 = phi i32 [ -1, %blk_exit ], [ 0, %allocator ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %0 = call i32 @__errno_location()
  %1 = sext i32 %0 to i64
  %2 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %1
  %3 = bitcast i8* %2 to i32*
  store i32 %i32, i32* %3, align 4
  br label %return
}

define _Exit {
allocator:
  call void @__wasi_proc_exit(i32 %i32)
  unreachable
}

define libc_exit_fini {
allocator:
  call void @_fini()
  ret void
}

define exit {
allocator:
  call void @_fini()
  call void @libc_exit_fini()
  call void @__stdio_exit()
  call void @_Exit(i32 %i32)
  unreachable
}

define _fini {
allocator:
  ret void
}

define __stdio_write {
allocator:
  %stack_var_-20_ = alloca i32, align 4
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-12_ = alloca i32, align 4
  %stack_var_-8_ = alloca i32, align 4
  %stack_var_-4_ = alloca i32, align 4
  %stack_var_-16_ = alloca i32, align 4
  %stack_var_-32_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = ptrtoint i32* %stack_var_-32_ to i32
  %calcOffset = add i32 %i32, 28
  %2 = sext i32 %calcOffset to i64
  %3 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %2
  %4 = bitcast i8* %3 to i32*
  %loadResult = load i32, i32* %4, align 4
  store i32 %loadResult, i32* %stack_var_-16_, align 4
  %calcOffset4 = add i32 %i32, 20
  %5 = sext i32 %calcOffset4 to i64
  %6 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %5
  %7 = bitcast i8* %6 to i32*
  %loadResult5 = load i32, i32* %7, align 4
  store i32 %i322, i32* %stack_var_-4_, align 4
  store i32 %i321, i32* %stack_var_-8_, align 4
  %8 = sub i32 %loadResult5, %loadResult
  store i32 %8, i32* %stack_var_-12_, align 4
  %9 = add i32 %8, %i322
  %10 = add i32 %0, -16
  %calcOffset12 = add i32 %i32, 60
  %11 = sext i32 %calcOffset12 to i64
  %12 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %11
  %13 = bitcast i8* %12 to i32*
  %loadResult13 = load i32, i32* %13, align 4
  %14 = add i32 %0, -20
  %15 = call i32 @__wasi_fd_write(i32 %loadResult13, i32 %10, i32 2, i32 %14)
  %16 = call i32 @__wasi_syscall_ret(i32 %15)
  %brif_val.not = icmp eq i32 %16, 0
  br i1 %brif_val.not, label %loop_entry, label %blk_exit11

blk_exit:                                         ; preds = %brif_next49, %blk_exit9, %blk_exit10
  %_local_4.0 = phi i32 [ %i322, %blk_exit10 ], [ 0, %blk_exit9 ], [ %66, %brif_next49 ]
  %17 = ptrtoint i32* %stack_var_0_ to i32
  ret i32 %_local_4.0

blk_exit9:                                        ; preds = %blk_exit11, %brif_next17
  %_local_7.0 = phi i32 [ %_local_7.1, %blk_exit11 ], [ %_local_7.2, %brif_next17 ]
  %_param_1.0 = phi i32 [ %_param_1.1, %blk_exit11 ], [ %_param_1.2, %brif_next17 ]
  store i32 0, i32* %4, align 4
  %calcOffset44 = add i32 %i32, 16
  %18 = sext i32 %calcOffset44 to i64
  %19 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %18
  %20 = bitcast i8* %19 to i64*
  store i64 0, i64* %20, align 4
  %21 = sext i32 %i32 to i64
  %22 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %21
  %23 = bitcast i8* %22 to i32*
  %loadResult46 = load i32, i32* %23, align 4
  %24 = or i32 %loadResult46, 32
  store i32 %24, i32* %23, align 4
  %25 = icmp eq i32 %_local_7.0, 2
  br i1 %25, label %blk_exit, label %brif_next49

blk_exit10:                                       ; preds = %blk_exit11, %loop_entry
  %calcOffset36 = add i32 %i32, 44
  %26 = sext i32 %calcOffset36 to i64
  %27 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %26
  %28 = bitcast i8* %27 to i32*
  %loadResult37 = load i32, i32* %28, align 4
  store i32 %loadResult37, i32* %4, align 4
  store i32 %loadResult37, i32* %7, align 4
  %calcOffset40 = add i32 %i32, 48
  %29 = sext i32 %calcOffset40 to i64
  %30 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %29
  %31 = bitcast i8* %30 to i32*
  %loadResult41 = load i32, i32* %31, align 4
  %32 = add i32 %loadResult37, %loadResult41
  %calcOffset42 = add i32 %i32, 16
  %33 = sext i32 %calcOffset42 to i64
  %34 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %33
  %35 = bitcast i8* %34 to i32*
  store i32 %32, i32* %35, align 4
  br label %blk_exit

blk_exit11:                                       ; preds = %brif_next19, %allocator
  %_local_6.0 = phi i32 [ %56, %brif_next19 ], [ %9, %allocator ]
  %_local_7.1 = phi i32 [ %59, %brif_next19 ], [ 2, %allocator ]
  %_param_1.1 = phi i32 [ %58, %brif_next19 ], [ %10, %allocator ]
  %.not = icmp eq i32 %_local_6.0, -1
  br i1 %.not, label %blk_exit10, label %blk_exit9

loop_entry:                                       ; preds = %allocator, %brif_next19
  %_local_6.1 = phi i32 [ %9, %allocator ], [ %56, %brif_next19 ]
  %_local_7.2 = phi i32 [ 2, %allocator ], [ %59, %brif_next19 ]
  %_param_1.2 = phi i32 [ %10, %allocator ], [ %58, %brif_next19 ]
  %36 = load i32, i32* %stack_var_-20_, align 4
  %37 = icmp eq i32 %_local_6.1, %36
  br i1 %37, label %blk_exit10, label %brif_next17

brif_next17:                                      ; preds = %loop_entry
  %brif_val18.not = icmp sgt i32 %36, -1
  br i1 %brif_val18.not, label %brif_next19, label %blk_exit9

brif_next19:                                      ; preds = %brif_next17
  %calcOffset20 = add i32 %_param_1.2, 4
  %38 = sext i32 %calcOffset20 to i64
  %39 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %38
  %40 = bitcast i8* %39 to i32*
  %loadResult21 = load i32, i32* %40, align 4
  %41 = icmp ugt i32 %36, %loadResult21
  %.neg = sext i1 %41 to i32
  %42 = select i1 %41, i32 8, i32 0
  %43 = add i32 %_param_1.2, %42
  %44 = sext i32 %43 to i64
  %45 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %44
  %46 = bitcast i8* %45 to i32*
  %loadResult23 = load i32, i32* %46, align 4
  %47 = select i1 %41, i32 %loadResult21, i32 0
  %48 = sub i32 %36, %47
  %49 = add i32 %loadResult23, %48
  store i32 %49, i32* %46, align 4
  %50 = select i1 %41, i32 12, i32 4
  %51 = add i32 %_param_1.2, %50
  %52 = sext i32 %51 to i64
  %53 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %52
  %54 = bitcast i8* %53 to i32*
  %loadResult27 = load i32, i32* %54, align 4
  %55 = sub i32 %loadResult27, %48
  store i32 %55, i32* %54, align 4
  %56 = sub i32 %_local_6.1, %36
  %loadResult30 = load i32, i32* %13, align 4
  %57 = add i32 %_param_1.2, 8
  %58 = select i1 %41, i32 %57, i32 %_param_1.2
  %59 = add i32 %_local_7.2, %.neg
  %60 = call i32 @__wasi_fd_write(i32 %loadResult30, i32 %58, i32 %59, i32 %14)
  %61 = call i32 @__wasi_syscall_ret(i32 %60)
  %62 = icmp eq i32 %61, 0
  br i1 %62, label %loop_entry, label %blk_exit11

brif_next49:                                      ; preds = %blk_exit9
  %calcOffset50 = add i32 %_param_1.0, 4
  %63 = sext i32 %calcOffset50 to i64
  %64 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %63
  %65 = bitcast i8* %64 to i32*
  %loadResult51 = load i32, i32* %65, align 4
  %66 = sub i32 %i322, %loadResult51
  br label %blk_exit
}

define __emscripten_stdout_close {
allocator:
  ret i32 0
}

define __emscripten_stdout_seek {
allocator:
  ret i64 0
}

define memset {
allocator:
  %0 = icmp eq i32 %i322, 0
  br i1 %0, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %loop_entry, %brif_next25, %brif_next19, %brif_next15, %brif_next11, %brif_next5, %brif_next, %allocator
  ret i32 %i32

brif_next:                                        ; preds = %allocator
  %1 = add i32 %i322, %i32
  %2 = add i32 %1, -1
  %3 = sext i32 %2 to i64
  %4 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %3
  %5 = trunc i32 %i321 to i8
  store i8 %5, i8* %4, align 1
  %6 = sext i32 %i32 to i64
  %7 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %6
  store i8 %5, i8* %7, align 1
  %8 = icmp ult i32 %i322, 3
  br i1 %8, label %blk_exit, label %brif_next5

brif_next5:                                       ; preds = %brif_next
  %9 = add i32 %1, -2
  %10 = sext i32 %9 to i64
  %11 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %10
  store i8 %5, i8* %11, align 1
  %calcOffset7 = add i32 %i32, 1
  %12 = sext i32 %calcOffset7 to i64
  %13 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %12
  store i8 %5, i8* %13, align 1
  %14 = add i32 %1, -3
  %15 = sext i32 %14 to i64
  %16 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %15
  store i8 %5, i8* %16, align 1
  %calcOffset9 = add i32 %i32, 2
  %17 = sext i32 %calcOffset9 to i64
  %18 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %17
  store i8 %5, i8* %18, align 1
  %19 = icmp ult i32 %i322, 7
  br i1 %19, label %blk_exit, label %brif_next11

brif_next11:                                      ; preds = %brif_next5
  %20 = add i32 %1, -4
  %21 = sext i32 %20 to i64
  %22 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %21
  store i8 %5, i8* %22, align 1
  %calcOffset13 = add i32 %i32, 3
  %23 = sext i32 %calcOffset13 to i64
  %24 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %23
  store i8 %5, i8* %24, align 1
  %25 = icmp ult i32 %i322, 9
  br i1 %25, label %blk_exit, label %brif_next15

brif_next15:                                      ; preds = %brif_next11
  %26 = sub i32 0, %i32
  %27 = and i32 %26, 3
  %28 = add i32 %27, %i32
  %29 = and i32 %i321, 255
  %30 = mul nuw i32 %29, 16843009
  %31 = sext i32 %28 to i64
  %32 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %31
  %33 = bitcast i8* %32 to i32*
  store i32 %30, i32* %33, align 4
  %34 = sub i32 %i322, %27
  %35 = and i32 %34, -4
  %36 = add i32 %28, %35
  %37 = add i32 %36, -4
  %38 = sext i32 %37 to i64
  %39 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %38
  %40 = bitcast i8* %39 to i32*
  store i32 %30, i32* %40, align 4
  %41 = icmp ult i32 %35, 9
  br i1 %41, label %blk_exit, label %brif_next19

brif_next19:                                      ; preds = %brif_next15
  %calcOffset20 = add i32 %28, 8
  %42 = sext i32 %calcOffset20 to i64
  %43 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %42
  %44 = bitcast i8* %43 to i32*
  store i32 %30, i32* %44, align 4
  %calcOffset21 = add i32 %28, 4
  %45 = sext i32 %calcOffset21 to i64
  %46 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %45
  %47 = bitcast i8* %46 to i32*
  store i32 %30, i32* %47, align 4
  %48 = add i32 %36, -8
  %49 = sext i32 %48 to i64
  %50 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %49
  %51 = bitcast i8* %50 to i32*
  store i32 %30, i32* %51, align 4
  %52 = add i32 %36, -12
  %53 = sext i32 %52 to i64
  %54 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %53
  %55 = bitcast i8* %54 to i32*
  store i32 %30, i32* %55, align 4
  %56 = icmp ult i32 %35, 25
  br i1 %56, label %blk_exit, label %brif_next25

brif_next25:                                      ; preds = %brif_next19
  %calcOffset26 = add i32 %28, 24
  %57 = sext i32 %calcOffset26 to i64
  %58 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %57
  %59 = bitcast i8* %58 to i32*
  store i32 %30, i32* %59, align 4
  %calcOffset27 = add i32 %28, 20
  %60 = sext i32 %calcOffset27 to i64
  %61 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %60
  %62 = bitcast i8* %61 to i32*
  store i32 %30, i32* %62, align 4
  %calcOffset28 = add i32 %28, 16
  %63 = sext i32 %calcOffset28 to i64
  %64 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %63
  %65 = bitcast i8* %64 to i32*
  store i32 %30, i32* %65, align 4
  %calcOffset29 = add i32 %28, 12
  %66 = sext i32 %calcOffset29 to i64
  %67 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %66
  %68 = bitcast i8* %67 to i32*
  store i32 %30, i32* %68, align 4
  %69 = add i32 %36, -16
  %70 = sext i32 %69 to i64
  %71 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %70
  %72 = bitcast i8* %71 to i32*
  store i32 %30, i32* %72, align 4
  %73 = add i32 %36, -20
  %74 = sext i32 %73 to i64
  %75 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %74
  %76 = bitcast i8* %75 to i32*
  store i32 %30, i32* %76, align 4
  %77 = add i32 %36, -24
  %78 = sext i32 %77 to i64
  %79 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %78
  %80 = bitcast i8* %79 to i32*
  store i32 %30, i32* %80, align 4
  %81 = add i32 %36, -28
  %82 = sext i32 %81 to i64
  %83 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %82
  %84 = bitcast i8* %83 to i32*
  store i32 %30, i32* %84, align 4
  %85 = and i32 %28, 4
  %86 = or i32 %85, 24
  %87 = sub i32 %35, %86
  %88 = icmp ult i32 %87, 32
  br i1 %88, label %blk_exit, label %brif_next35

brif_next35:                                      ; preds = %brif_next25
  %89 = zext i32 %30 to i64
  %90 = mul nuw i64 %89, 4294967297
  %91 = add i32 %28, %86
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %brif_next35
  %_param_2.0 = phi i32 [ %87, %brif_next35 ], [ %105, %loop_entry ]
  %_param_1.0 = phi i32 [ %91, %brif_next35 ], [ %104, %loop_entry ]
  %calcOffset36 = add i32 %_param_1.0, 24
  %92 = sext i32 %calcOffset36 to i64
  %93 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %92
  %94 = bitcast i8* %93 to i64*
  store i64 %90, i64* %94, align 4
  %calcOffset37 = add i32 %_param_1.0, 16
  %95 = sext i32 %calcOffset37 to i64
  %96 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %95
  %97 = bitcast i8* %96 to i64*
  store i64 %90, i64* %97, align 4
  %calcOffset38 = add i32 %_param_1.0, 8
  %98 = sext i32 %calcOffset38 to i64
  %99 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %98
  %100 = bitcast i8* %99 to i64*
  store i64 %90, i64* %100, align 4
  %101 = sext i32 %_param_1.0 to i64
  %102 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %101
  %103 = bitcast i8* %102 to i64*
  store i64 %90, i64* %103, align 4
  %104 = add i32 %_param_1.0, 32
  %105 = add i32 %_param_2.0, -32
  %106 = icmp ugt i32 %105, 31
  br i1 %106, label %loop_entry, label %blk_exit
}

define isdigit {
allocator:
  %0 = add i32 %i32, -48
  %1 = icmp ult i32 %0, 10
  %2 = zext i1 %1 to i32
  ret i32 %2
}

define memchr {
allocator:
  %0 = icmp ne i32 %i322, 0
  %1 = and i32 %i32, 3
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %blk_exit4, label %brif_next

return:                                           ; preds = %blk_exit32, %blk_exit4, %blk_exit15, %brif_next36
  %return_0 = phi i32 [ %_param_0.5, %brif_next36 ], [ 0, %blk_exit15 ], [ 0, %blk_exit4 ], [ 0, %blk_exit32 ]
  ret i32 %return_0

blk_exit3:                                        ; preds = %blk_exit4.blk_exit3_crit_edge, %loop_entry
  %loadResult17 = phi i8 [ %loadResult17.pre, %blk_exit4.blk_exit3_crit_edge ], [ %loadResult, %loop_entry ]
  %_param_2.0 = phi i32 [ %_param_2.1, %blk_exit4.blk_exit3_crit_edge ], [ %_param_2.2, %loop_entry ]
  %_param_0.0 = phi i32 [ %_param_0.1, %blk_exit4.blk_exit3_crit_edge ], [ %_param_0.2, %loop_entry ]
  %3 = trunc i32 %i321 to i8
  %4 = icmp eq i8 %loadResult17, %3
  br i1 %4, label %blk_exit15, label %brif_next19

blk_exit4:                                        ; preds = %brif_next8, %brif_next, %allocator
  %_local_3.0.in = phi i1 [ %0, %allocator ], [ %0, %brif_next ], [ %11, %brif_next8 ]
  %_param_2.1 = phi i32 [ %i322, %allocator ], [ 0, %brif_next ], [ %10, %brif_next8 ]
  %_param_0.1 = phi i32 [ %i32, %allocator ], [ %i32, %brif_next ], [ %12, %brif_next8 ]
  br i1 %_local_3.0.in, label %blk_exit4.blk_exit3_crit_edge, label %return

blk_exit4.blk_exit3_crit_edge:                    ; preds = %blk_exit4
  %.phi.trans.insert = sext i32 %_param_0.1 to i64
  %.phi.trans.insert39 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %.phi.trans.insert
  %loadResult17.pre = load i8, i8* %.phi.trans.insert39, align 1
  br label %blk_exit3

brif_next:                                        ; preds = %allocator
  %5 = icmp eq i32 %i322, 0
  br i1 %5, label %blk_exit4, label %brif_next6

brif_next6:                                       ; preds = %brif_next
  br label %loop_entry

loop_entry:                                       ; preds = %brif_next8, %brif_next6
  %_param_2.2 = phi i32 [ %i322, %brif_next6 ], [ %10, %brif_next8 ]
  %_param_0.2 = phi i32 [ %i32, %brif_next6 ], [ %12, %brif_next8 ]
  %6 = sext i32 %_param_0.2 to i64
  %7 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %6
  %loadResult = load i8, i8* %7, align 1
  %8 = trunc i32 %i321 to i8
  %9 = icmp eq i8 %loadResult, %8
  br i1 %9, label %blk_exit3, label %brif_next8

brif_next8:                                       ; preds = %loop_entry
  %10 = add i32 %_param_2.2, -1
  %11 = icmp ne i32 %10, 0
  %12 = add i32 %_param_0.2, 1
  %13 = and i32 %12, 3
  %14 = icmp ne i32 %13, 0
  %or.cond = select i1 %14, i1 %11, i1 false
  br i1 %or.cond, label %loop_entry, label %blk_exit4

blk_exit15:                                       ; preds = %brif_next26, %loop_entry22, %brif_next19, %blk_exit3
  %_param_2.3 = phi i32 [ %_param_2.0, %blk_exit3 ], [ %_param_2.0, %brif_next19 ], [ %28, %brif_next26 ], [ %_param_2.4, %loop_entry22 ]
  %_param_0.3 = phi i32 [ %_param_0.0, %blk_exit3 ], [ %_param_0.0, %brif_next19 ], [ %27, %brif_next26 ], [ %_param_0.4, %loop_entry22 ]
  %15 = icmp eq i32 %_param_2.3, 0
  br i1 %15, label %return, label %brif_next30

brif_next19:                                      ; preds = %blk_exit3
  %16 = icmp ult i32 %_param_2.0, 4
  br i1 %16, label %blk_exit15, label %brif_next21

brif_next21:                                      ; preds = %brif_next19
  %17 = and i32 %i321, 255
  %18 = mul nuw i32 %17, 16843009
  br label %loop_entry22

loop_entry22:                                     ; preds = %brif_next26, %brif_next21
  %_param_2.4 = phi i32 [ %_param_2.0, %brif_next21 ], [ %28, %brif_next26 ]
  %_param_0.4 = phi i32 [ %_param_0.0, %brif_next21 ], [ %27, %brif_next26 ]
  %19 = sext i32 %_param_0.4 to i64
  %20 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %19
  %21 = bitcast i8* %20 to i32*
  %loadResult24 = load i32, i32* %21, align 4
  %22 = xor i32 %loadResult24, %18
  %23 = xor i32 %22, -1
  %24 = add i32 %22, -16843009
  %25 = and i32 %24, %23
  %26 = and i32 %25, -2139062144
  %brif_val25.not = icmp eq i32 %26, 0
  br i1 %brif_val25.not, label %brif_next26, label %blk_exit15

brif_next26:                                      ; preds = %loop_entry22
  %27 = add i32 %_param_0.4, 4
  %28 = add i32 %_param_2.4, -4
  %29 = icmp ugt i32 %28, 3
  br i1 %29, label %loop_entry22, label %blk_exit15

brif_next30:                                      ; preds = %blk_exit15
  br label %loop_entry31

loop_entry31:                                     ; preds = %blk_exit32, %brif_next30
  %_param_2.5 = phi i32 [ %_param_2.3, %brif_next30 ], [ %34, %blk_exit32 ]
  %_param_0.5 = phi i32 [ %_param_0.3, %brif_next30 ], [ %33, %blk_exit32 ]
  %30 = sext i32 %_param_0.5 to i64
  %31 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %30
  %loadResult34 = load i8, i8* %31, align 1
  %32 = trunc i32 %i321 to i8
  %.not = icmp eq i8 %loadResult34, %32
  br i1 %.not, label %brif_next36, label %blk_exit32

blk_exit32:                                       ; preds = %loop_entry31
  %33 = add i32 %_param_0.5, 1
  %34 = add i32 %_param_2.5, -1
  %brif_val37.not = icmp eq i32 %34, 0
  br i1 %brif_val37.not, label %return, label %loop_entry31

brif_next36:                                      ; preds = %loop_entry31
  br label %return
}

define __memcpy {
allocator:
  %0 = add i32 %i32, %i322
  %1 = xor i32 %i321, %i32
  %2 = and i32 %1, 3
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit5

blk_exit:                                         ; preds = %loop_entry93, %loop_entry20, %blk_exit35, %blk_exit15
  %_param_2.0 = phi i32 [ %145, %loop_entry93 ], [ %_param_2.4, %blk_exit35 ], [ %i32, %blk_exit15 ], [ %37, %loop_entry20 ]
  %_param_1.0 = phi i32 [ %144, %loop_entry93 ], [ %_param_1.4, %blk_exit35 ], [ %i321, %blk_exit15 ], [ %36, %loop_entry20 ]
  %.not110 = icmp ult i32 %_param_2.0, %0
  br i1 %.not110, label %loop_entry102, label %blk_exit99

blk_exit3:                                        ; preds = %brif_next12, %blk_exit4, %loop_entry
  %_param_2.1 = phi i32 [ %i32, %blk_exit4 ], [ %14, %loop_entry ], [ %14, %brif_next12 ]
  %_param_1.1 = phi i32 [ %i321, %blk_exit4 ], [ %13, %loop_entry ], [ %13, %brif_next12 ]
  %3 = and i32 %0, -4
  %4 = icmp ult i32 %3, 64
  br i1 %4, label %blk_exit35, label %brif_next37

blk_exit4:                                        ; preds = %brif_next7, %brif_next
  br label %blk_exit3

blk_exit5:                                        ; preds = %allocator
  %5 = icmp ult i32 %0, 4
  br i1 %5, label %blk_exit15, label %brif_next17

brif_next:                                        ; preds = %allocator
  %6 = and i32 %i32, 3
  %7 = icmp eq i32 %6, 0
  br i1 %7, label %blk_exit4, label %brif_next7

brif_next7:                                       ; preds = %brif_next
  %8 = icmp slt i32 %i322, 1
  br i1 %8, label %blk_exit4, label %brif_next9

brif_next9:                                       ; preds = %brif_next7
  br label %loop_entry

loop_entry:                                       ; preds = %brif_next12, %brif_next9
  %_param_2.2 = phi i32 [ %i32, %brif_next9 ], [ %14, %brif_next12 ]
  %_param_1.2 = phi i32 [ %i321, %brif_next9 ], [ %13, %brif_next12 ]
  %9 = sext i32 %_param_1.2 to i64
  %10 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %9
  %loadResult108 = load i8, i8* %10, align 1
  %11 = sext i32 %_param_2.2 to i64
  %12 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %11
  store i8 %loadResult108, i8* %12, align 1
  %13 = add i32 %_param_1.2, 1
  %14 = add i32 %_param_2.2, 1
  %15 = and i32 %14, 3
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %blk_exit3, label %brif_next12

brif_next12:                                      ; preds = %loop_entry
  %17 = icmp ult i32 %14, %0
  br i1 %17, label %loop_entry, label %blk_exit3

blk_exit15:                                       ; preds = %brif_next17, %blk_exit5
  br label %blk_exit

brif_next17:                                      ; preds = %blk_exit5
  %18 = add i32 %0, -4
  %19 = icmp ult i32 %18, %i32
  br i1 %19, label %blk_exit15, label %brif_next19

brif_next19:                                      ; preds = %brif_next17
  br label %loop_entry20

loop_entry20:                                     ; preds = %loop_entry20, %brif_next19
  %_param_2.3 = phi i32 [ %i32, %brif_next19 ], [ %37, %loop_entry20 ]
  %_param_1.3 = phi i32 [ %i321, %brif_next19 ], [ %36, %loop_entry20 ]
  %20 = sext i32 %_param_1.3 to i64
  %21 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %20
  %loadResult22113 = load i8, i8* %21, align 1
  %22 = sext i32 %_param_2.3 to i64
  %23 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %22
  store i8 %loadResult22113, i8* %23, align 1
  %calcOffset24 = add i32 %_param_1.3, 1
  %24 = sext i32 %calcOffset24 to i64
  %25 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %24
  %loadResult25114 = load i8, i8* %25, align 1
  %calcOffset26 = add i32 %_param_2.3, 1
  %26 = sext i32 %calcOffset26 to i64
  %27 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %26
  store i8 %loadResult25114, i8* %27, align 1
  %calcOffset27 = add i32 %_param_1.3, 2
  %28 = sext i32 %calcOffset27 to i64
  %29 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %28
  %loadResult28115 = load i8, i8* %29, align 1
  %calcOffset29 = add i32 %_param_2.3, 2
  %30 = sext i32 %calcOffset29 to i64
  %31 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %30
  store i8 %loadResult28115, i8* %31, align 1
  %calcOffset30 = add i32 %_param_1.3, 3
  %32 = sext i32 %calcOffset30 to i64
  %33 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %32
  %loadResult31116 = load i8, i8* %33, align 1
  %calcOffset32 = add i32 %_param_2.3, 3
  %34 = sext i32 %calcOffset32 to i64
  %35 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %34
  store i8 %loadResult31116, i8* %35, align 1
  %36 = add i32 %_param_1.3, 4
  %37 = add i32 %_param_2.3, 4
  %.not117 = icmp ugt i32 %37, %18
  br i1 %.not117, label %blk_exit, label %loop_entry20

blk_exit35:                                       ; preds = %loop_entry40, %brif_next37, %blk_exit3
  %_param_2.4 = phi i32 [ %_param_2.1, %blk_exit3 ], [ %_param_2.1, %brif_next37 ], [ %137, %loop_entry40 ]
  %_param_1.4 = phi i32 [ %_param_1.1, %blk_exit3 ], [ %_param_1.1, %brif_next37 ], [ %136, %loop_entry40 ]
  %.not109 = icmp ult i32 %_param_2.4, %3
  br i1 %.not109, label %loop_entry93, label %blk_exit

brif_next37:                                      ; preds = %blk_exit3
  %38 = add i32 %3, -64
  %39 = icmp ugt i32 %_param_2.1, %38
  br i1 %39, label %blk_exit35, label %loop_entry40

loop_entry40:                                     ; preds = %brif_next37, %loop_entry40
  %_param_2.5 = phi i32 [ %_param_2.1, %brif_next37 ], [ %137, %loop_entry40 ]
  %_param_1.5 = phi i32 [ %_param_1.1, %brif_next37 ], [ %136, %loop_entry40 ]
  %40 = sext i32 %_param_1.5 to i64
  %41 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %40
  %42 = bitcast i8* %41 to i32*
  %loadResult42 = load i32, i32* %42, align 4
  %43 = sext i32 %_param_2.5 to i64
  %44 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %43
  %45 = bitcast i8* %44 to i32*
  store i32 %loadResult42, i32* %45, align 4
  %calcOffset44 = add i32 %_param_1.5, 4
  %46 = sext i32 %calcOffset44 to i64
  %47 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %46
  %48 = bitcast i8* %47 to i32*
  %loadResult45 = load i32, i32* %48, align 4
  %calcOffset46 = add i32 %_param_2.5, 4
  %49 = sext i32 %calcOffset46 to i64
  %50 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %49
  %51 = bitcast i8* %50 to i32*
  store i32 %loadResult45, i32* %51, align 4
  %calcOffset47 = add i32 %_param_1.5, 8
  %52 = sext i32 %calcOffset47 to i64
  %53 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %52
  %54 = bitcast i8* %53 to i32*
  %loadResult48 = load i32, i32* %54, align 4
  %calcOffset49 = add i32 %_param_2.5, 8
  %55 = sext i32 %calcOffset49 to i64
  %56 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %55
  %57 = bitcast i8* %56 to i32*
  store i32 %loadResult48, i32* %57, align 4
  %calcOffset50 = add i32 %_param_1.5, 12
  %58 = sext i32 %calcOffset50 to i64
  %59 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %58
  %60 = bitcast i8* %59 to i32*
  %loadResult51 = load i32, i32* %60, align 4
  %calcOffset52 = add i32 %_param_2.5, 12
  %61 = sext i32 %calcOffset52 to i64
  %62 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %61
  %63 = bitcast i8* %62 to i32*
  store i32 %loadResult51, i32* %63, align 4
  %calcOffset53 = add i32 %_param_1.5, 16
  %64 = sext i32 %calcOffset53 to i64
  %65 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %64
  %66 = bitcast i8* %65 to i32*
  %loadResult54 = load i32, i32* %66, align 4
  %calcOffset55 = add i32 %_param_2.5, 16
  %67 = sext i32 %calcOffset55 to i64
  %68 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %67
  %69 = bitcast i8* %68 to i32*
  store i32 %loadResult54, i32* %69, align 4
  %calcOffset56 = add i32 %_param_1.5, 20
  %70 = sext i32 %calcOffset56 to i64
  %71 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %70
  %72 = bitcast i8* %71 to i32*
  %loadResult57 = load i32, i32* %72, align 4
  %calcOffset58 = add i32 %_param_2.5, 20
  %73 = sext i32 %calcOffset58 to i64
  %74 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %73
  %75 = bitcast i8* %74 to i32*
  store i32 %loadResult57, i32* %75, align 4
  %calcOffset59 = add i32 %_param_1.5, 24
  %76 = sext i32 %calcOffset59 to i64
  %77 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %76
  %78 = bitcast i8* %77 to i32*
  %loadResult60 = load i32, i32* %78, align 4
  %calcOffset61 = add i32 %_param_2.5, 24
  %79 = sext i32 %calcOffset61 to i64
  %80 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %79
  %81 = bitcast i8* %80 to i32*
  store i32 %loadResult60, i32* %81, align 4
  %calcOffset62 = add i32 %_param_1.5, 28
  %82 = sext i32 %calcOffset62 to i64
  %83 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %82
  %84 = bitcast i8* %83 to i32*
  %loadResult63 = load i32, i32* %84, align 4
  %calcOffset64 = add i32 %_param_2.5, 28
  %85 = sext i32 %calcOffset64 to i64
  %86 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %85
  %87 = bitcast i8* %86 to i32*
  store i32 %loadResult63, i32* %87, align 4
  %calcOffset65 = add i32 %_param_1.5, 32
  %88 = sext i32 %calcOffset65 to i64
  %89 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %88
  %90 = bitcast i8* %89 to i32*
  %loadResult66 = load i32, i32* %90, align 4
  %calcOffset67 = add i32 %_param_2.5, 32
  %91 = sext i32 %calcOffset67 to i64
  %92 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %91
  %93 = bitcast i8* %92 to i32*
  store i32 %loadResult66, i32* %93, align 4
  %calcOffset68 = add i32 %_param_1.5, 36
  %94 = sext i32 %calcOffset68 to i64
  %95 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %94
  %96 = bitcast i8* %95 to i32*
  %loadResult69 = load i32, i32* %96, align 4
  %calcOffset70 = add i32 %_param_2.5, 36
  %97 = sext i32 %calcOffset70 to i64
  %98 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %97
  %99 = bitcast i8* %98 to i32*
  store i32 %loadResult69, i32* %99, align 4
  %calcOffset71 = add i32 %_param_1.5, 40
  %100 = sext i32 %calcOffset71 to i64
  %101 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %100
  %102 = bitcast i8* %101 to i32*
  %loadResult72 = load i32, i32* %102, align 4
  %calcOffset73 = add i32 %_param_2.5, 40
  %103 = sext i32 %calcOffset73 to i64
  %104 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %103
  %105 = bitcast i8* %104 to i32*
  store i32 %loadResult72, i32* %105, align 4
  %calcOffset74 = add i32 %_param_1.5, 44
  %106 = sext i32 %calcOffset74 to i64
  %107 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %106
  %108 = bitcast i8* %107 to i32*
  %loadResult75 = load i32, i32* %108, align 4
  %calcOffset76 = add i32 %_param_2.5, 44
  %109 = sext i32 %calcOffset76 to i64
  %110 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %109
  %111 = bitcast i8* %110 to i32*
  store i32 %loadResult75, i32* %111, align 4
  %calcOffset77 = add i32 %_param_1.5, 48
  %112 = sext i32 %calcOffset77 to i64
  %113 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %112
  %114 = bitcast i8* %113 to i32*
  %loadResult78 = load i32, i32* %114, align 4
  %calcOffset79 = add i32 %_param_2.5, 48
  %115 = sext i32 %calcOffset79 to i64
  %116 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %115
  %117 = bitcast i8* %116 to i32*
  store i32 %loadResult78, i32* %117, align 4
  %calcOffset80 = add i32 %_param_1.5, 52
  %118 = sext i32 %calcOffset80 to i64
  %119 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %118
  %120 = bitcast i8* %119 to i32*
  %loadResult81 = load i32, i32* %120, align 4
  %calcOffset82 = add i32 %_param_2.5, 52
  %121 = sext i32 %calcOffset82 to i64
  %122 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %121
  %123 = bitcast i8* %122 to i32*
  store i32 %loadResult81, i32* %123, align 4
  %calcOffset83 = add i32 %_param_1.5, 56
  %124 = sext i32 %calcOffset83 to i64
  %125 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %124
  %126 = bitcast i8* %125 to i32*
  %loadResult84 = load i32, i32* %126, align 4
  %calcOffset85 = add i32 %_param_2.5, 56
  %127 = sext i32 %calcOffset85 to i64
  %128 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %127
  %129 = bitcast i8* %128 to i32*
  store i32 %loadResult84, i32* %129, align 4
  %calcOffset86 = add i32 %_param_1.5, 60
  %130 = sext i32 %calcOffset86 to i64
  %131 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %130
  %132 = bitcast i8* %131 to i32*
  %loadResult87 = load i32, i32* %132, align 4
  %calcOffset88 = add i32 %_param_2.5, 60
  %133 = sext i32 %calcOffset88 to i64
  %134 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %133
  %135 = bitcast i8* %134 to i32*
  store i32 %loadResult87, i32* %135, align 4
  %136 = add i32 %_param_1.5, 64
  %137 = add i32 %_param_2.5, 64
  %.not = icmp ugt i32 %137, %38
  br i1 %.not, label %blk_exit35, label %loop_entry40

loop_entry93:                                     ; preds = %blk_exit35, %loop_entry93
  %_param_2.6 = phi i32 [ %_param_2.4, %blk_exit35 ], [ %145, %loop_entry93 ]
  %_param_1.6 = phi i32 [ %_param_1.4, %blk_exit35 ], [ %144, %loop_entry93 ]
  %138 = sext i32 %_param_1.6 to i64
  %139 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %138
  %140 = bitcast i8* %139 to i32*
  %loadResult95 = load i32, i32* %140, align 4
  %141 = sext i32 %_param_2.6 to i64
  %142 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %141
  %143 = bitcast i8* %142 to i32*
  store i32 %loadResult95, i32* %143, align 4
  %144 = add i32 %_param_1.6, 4
  %145 = add i32 %_param_2.6, 4
  %146 = icmp ult i32 %145, %3
  br i1 %146, label %loop_entry93, label %blk_exit

blk_exit99:                                       ; preds = %loop_entry102, %blk_exit
  ret i32 %i32

loop_entry102:                                    ; preds = %blk_exit, %loop_entry102
  %_param_2.7 = phi i32 [ %_param_2.0, %blk_exit ], [ %152, %loop_entry102 ]
  %_param_1.7 = phi i32 [ %_param_1.0, %blk_exit ], [ %151, %loop_entry102 ]
  %147 = sext i32 %_param_1.7 to i64
  %148 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %147
  %loadResult104111 = load i8, i8* %148, align 1
  %149 = sext i32 %_param_2.7 to i64
  %150 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %149
  store i8 %loadResult104111, i8* %150, align 1
  %151 = add i32 %_param_1.7, 1
  %152 = add i32 %_param_2.7, 1
  %.not112 = icmp eq i32 %152, %0
  br i1 %.not112, label %blk_exit99, label %loop_entry102
}

define emscripten_stack_init {
allocator:
  store i32 5247184, i32* @__stack_base, align 4
  store i32 4304, i32* @__stack_end, align 4
  ret void
}

define emscripten_stack_get_free {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = load i32, i32* @__stack_end, align 4
  %2 = sub i32 %0, %1
  ret i32 %2
}

define emscripten_stack_get_end {
allocator:
  %0 = load i32, i32* @__stack_end, align 4
  ret i32 %0
}

define __pthread_self {
allocator:
  ret i32 1916
}

define wcrtomb {
allocator:
  %0 = icmp eq i32 %i32, 0
  br i1 %0, label %blk_exit3, label %brif_next

return:                                           ; preds = %blk_exit, %blk_exit3, %brif_next31, %blk_exit21, %brif_next17
  %return_0 = phi i32 [ 2, %brif_next17 ], [ 3, %blk_exit21 ], [ 4, %brif_next31 ], [ %_local_3.0, %blk_exit3 ], [ 1, %blk_exit ]
  ret i32 %return_0

blk_exit:                                         ; preds = %brif_next11, %brif_next
  %1 = sext i32 %i32 to i64
  %2 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %1
  %3 = trunc i32 %i321 to i8
  store i8 %3, i8* %2, align 1
  br label %return

blk_exit3:                                        ; preds = %blk_exit6, %allocator
  %_local_3.0 = phi i32 [ 1, %allocator ], [ -1, %blk_exit6 ]
  br label %return

brif_next:                                        ; preds = %allocator
  %4 = icmp ult i32 %i321, 128
  br i1 %4, label %blk_exit, label %brif_next5

brif_next5:                                       ; preds = %brif_next
  %5 = call i32 @__pthread_self()
  %calcOffset = add i32 %5, 168
  %6 = sext i32 %calcOffset to i64
  %7 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %6
  %8 = bitcast i8* %7 to i32*
  %loadResult = load i32, i32* %8, align 4
  %9 = sext i32 %loadResult to i64
  %10 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %9
  %11 = bitcast i8* %10 to i32*
  %loadResult9 = load i32, i32* %11, align 4
  %brif_val10.not = icmp eq i32 %loadResult9, 0
  br i1 %brif_val10.not, label %brif_next11, label %blk_exit7

blk_exit6:                                        ; preds = %blk_exit29, %brif_next13
  br label %blk_exit3

blk_exit7:                                        ; preds = %brif_next5
  %12 = icmp ugt i32 %i321, 2047
  br i1 %12, label %blk_exit15, label %brif_next17

brif_next11:                                      ; preds = %brif_next5
  %13 = and i32 %i321, -128
  %14 = icmp eq i32 %13, 57216
  br i1 %14, label %blk_exit, label %brif_next13

brif_next13:                                      ; preds = %brif_next11
  %15 = call i32 @__errno_location()
  %16 = sext i32 %15 to i64
  %17 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %16
  %18 = bitcast i8* %17 to i32*
  store i32 25, i32* %18, align 4
  br label %blk_exit6

blk_exit15:                                       ; preds = %blk_exit7
  %19 = icmp ult i32 %i321, 55296
  br i1 %19, label %blk_exit21, label %brif_next23

brif_next17:                                      ; preds = %blk_exit7
  %calcOffset18 = add i32 %i32, 1
  %20 = sext i32 %calcOffset18 to i64
  %21 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %20
  %22 = trunc i32 %i321 to i8
  %23 = and i8 %22, 63
  %24 = or i8 %23, -128
  store i8 %24, i8* %21, align 1
  %25 = lshr i32 %i321, 6
  %26 = sext i32 %i32 to i64
  %27 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %26
  %28 = trunc i32 %25 to i8
  %29 = or i8 %28, -64
  store i8 %29, i8* %27, align 1
  br label %return

blk_exit20:                                       ; preds = %brif_next23
  %30 = add i32 %i321, -1114112
  %31 = icmp ult i32 %30, -1048576
  br i1 %31, label %blk_exit29, label %brif_next31

blk_exit21:                                       ; preds = %brif_next23, %blk_exit15
  %calcOffset26 = add i32 %i32, 2
  %32 = sext i32 %calcOffset26 to i64
  %33 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %32
  %34 = trunc i32 %i321 to i8
  %35 = and i8 %34, 63
  %36 = or i8 %35, -128
  store i8 %36, i8* %33, align 1
  %37 = lshr i32 %i321, 12
  %38 = sext i32 %i32 to i64
  %39 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %38
  %40 = trunc i32 %37 to i8
  %41 = or i8 %40, -32
  store i8 %41, i8* %39, align 1
  %42 = lshr i32 %i321, 6
  %calcOffset28 = add i32 %i32, 1
  %43 = sext i32 %calcOffset28 to i64
  %44 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %43
  %45 = trunc i32 %42 to i8
  %46 = and i8 %45, 63
  %47 = or i8 %46, -128
  store i8 %47, i8* %44, align 1
  br label %return

brif_next23:                                      ; preds = %blk_exit15
  %48 = and i32 %i321, -8192
  %.not = icmp eq i32 %48, 57344
  br i1 %.not, label %blk_exit21, label %blk_exit20

blk_exit29:                                       ; preds = %blk_exit20
  %49 = call i32 @__errno_location()
  %50 = sext i32 %49 to i64
  %51 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %50
  %52 = bitcast i8* %51 to i32*
  store i32 25, i32* %52, align 4
  br label %blk_exit6

brif_next31:                                      ; preds = %blk_exit20
  %calcOffset32 = add i32 %i32, 3
  %53 = sext i32 %calcOffset32 to i64
  %54 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %53
  %55 = trunc i32 %i321 to i8
  %56 = and i8 %55, 63
  %57 = or i8 %56, -128
  store i8 %57, i8* %54, align 1
  %58 = lshr i32 %i321, 18
  %59 = sext i32 %i32 to i64
  %60 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %59
  %61 = trunc i32 %58 to i8
  %62 = or i8 %61, -16
  store i8 %62, i8* %60, align 1
  %63 = lshr i32 %i321, 6
  %calcOffset34 = add i32 %i32, 2
  %64 = sext i32 %calcOffset34 to i64
  %65 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %64
  %66 = trunc i32 %63 to i8
  %67 = and i8 %66, 63
  %68 = or i8 %67, -128
  store i8 %68, i8* %65, align 1
  %69 = lshr i32 %i321, 12
  %calcOffset35 = add i32 %i32, 1
  %70 = sext i32 %calcOffset35 to i64
  %71 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %70
  %72 = trunc i32 %69 to i8
  %73 = and i8 %72, 63
  %74 = or i8 %73, -128
  store i8 %74, i8* %71, align 1
  br label %return
}

define wctomb {
allocator:
  %brif_val.not = icmp eq i32 %i32, 0
  br i1 %brif_val.not, label %return, label %blk_exit

return:                                           ; preds = %allocator, %blk_exit
  %return_0 = phi i32 [ %0, %blk_exit ], [ 0, %allocator ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %0 = call i32 @wcrtomb(i32 %i32, i32 %i321, i32 0)
  br label %return
}

define frexp {
allocator:
  %0 = bitcast double %f64 to i64
  %1 = lshr i64 %0, 52
  %2 = trunc i64 %1 to i32
  %3 = and i32 %2, 2047
  %4 = icmp eq i32 %3, 2047
  br i1 %4, label %blk_exit, label %brif_next

return:                                           ; preds = %blk_exit, %blk_exit4
  %return_0 = phi double [ %return_012, %blk_exit ], [ %return_011, %blk_exit4 ]
  ret double %return_0

blk_exit:                                         ; preds = %blk_exit1, %allocator
  %return_012 = phi double [ %11, %blk_exit1 ], [ %f64, %allocator ]
  br label %return

brif_next:                                        ; preds = %allocator
  %brif_val2.not = icmp eq i32 %3, 0
  br i1 %brif_val2.not, label %brif_next3, label %blk_exit1

blk_exit1:                                        ; preds = %brif_next
  %5 = add nsw i32 %3, -1022
  %6 = sext i32 %i32 to i64
  %7 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %6
  %8 = bitcast i8* %7 to i32*
  store i32 %5, i32* %8, align 4
  %9 = and i64 %0, -9218868437227405313
  %10 = or i64 %9, 4602678819172646912
  %11 = bitcast i64 %10 to double
  br label %blk_exit

brif_next3:                                       ; preds = %brif_next
  %12 = fcmp oeq double %f64, 0.000000e+00
  br i1 %12, label %blk_exit5, label %brif_next7

blk_exit4:                                        ; preds = %blk_exit5, %brif_next7
  %.pre-phi = phi i64 [ %17, %blk_exit5 ], [ %.pre, %brif_next7 ]
  %return_011 = phi double [ %16, %blk_exit5 ], [ %f64, %brif_next7 ]
  %storemerge = phi i32 [ %20, %blk_exit5 ], [ 0, %brif_next7 ]
  %13 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %.pre-phi
  %14 = bitcast i8* %13 to i32*
  store i32 %storemerge, i32* %14, align 4
  br label %return

blk_exit5:                                        ; preds = %brif_next3
  %15 = fmul double %f64, 0x43F0000000000000
  %16 = call double @frexp(double %15, i32 %i32)
  %17 = sext i32 %i32 to i64
  %18 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %17
  %19 = bitcast i8* %18 to i32*
  %loadResult = load i32, i32* %19, align 4
  %20 = add i32 %loadResult, -64
  br label %blk_exit4

brif_next7:                                       ; preds = %brif_next3
  %.pre = sext i32 %i32 to i64
  br label %blk_exit4
}

define __ashlti3 {
allocator:
  %0 = and i32 %i322, 64
  %brif_val.not.not = icmp eq i32 %0, 0
  br i1 %brif_val.not.not, label %blk_exit3, label %brif_next

blk_exit:                                         ; preds = %brif_next5, %blk_exit3, %brif_next
  %_param_1.0 = phi i64 [ %i64, %blk_exit3 ], [ %17, %brif_next5 ], [ 0, %brif_next ]
  %_param_2.0 = phi i64 [ %i641, %blk_exit3 ], [ %16, %brif_next5 ], [ %10, %brif_next ]
  %1 = sext i32 %i32 to i64
  %2 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %1
  %3 = bitcast i8* %2 to i64*
  store i64 %_param_1.0, i64* %3, align 4
  %calcOffset6 = add i32 %i32, 8
  %4 = sext i32 %calcOffset6 to i64
  %5 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %4
  %6 = bitcast i8* %5 to i64*
  store i64 %_param_2.0, i64* %6, align 4
  ret void

blk_exit3:                                        ; preds = %allocator
  %7 = icmp eq i32 %i322, 0
  br i1 %7, label %blk_exit, label %brif_next5

brif_next:                                        ; preds = %allocator
  %8 = add i32 %i322, -64
  %9 = zext i32 %8 to i64
  %10 = shl i64 %i64, %9
  br label %blk_exit

brif_next5:                                       ; preds = %blk_exit3
  %11 = sub i32 64, %i322
  %12 = zext i32 %11 to i64
  %13 = lshr i64 %i64, %12
  %14 = zext i32 %i322 to i64
  %15 = shl i64 %i641, %14
  %16 = or i64 %13, %15
  %17 = shl i64 %i64, %14
  br label %blk_exit
}

define __lshrti3 {
allocator:
  %0 = and i32 %i322, 64
  %brif_val.not.not = icmp eq i32 %0, 0
  br i1 %brif_val.not.not, label %blk_exit3, label %brif_next

blk_exit:                                         ; preds = %brif_next5, %blk_exit3, %brif_next
  %_param_1.0 = phi i64 [ %i64, %blk_exit3 ], [ %16, %brif_next5 ], [ %10, %brif_next ]
  %_param_2.0 = phi i64 [ %i641, %blk_exit3 ], [ %17, %brif_next5 ], [ 0, %brif_next ]
  %1 = sext i32 %i32 to i64
  %2 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %1
  %3 = bitcast i8* %2 to i64*
  store i64 %_param_1.0, i64* %3, align 4
  %calcOffset6 = add i32 %i32, 8
  %4 = sext i32 %calcOffset6 to i64
  %5 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %4
  %6 = bitcast i8* %5 to i64*
  store i64 %_param_2.0, i64* %6, align 4
  ret void

blk_exit3:                                        ; preds = %allocator
  %7 = icmp eq i32 %i322, 0
  br i1 %7, label %blk_exit, label %brif_next5

brif_next:                                        ; preds = %allocator
  %8 = add i32 %i322, -64
  %9 = zext i32 %8 to i64
  %10 = lshr i64 %i641, %9
  br label %blk_exit

brif_next5:                                       ; preds = %blk_exit3
  %11 = sub i32 64, %i322
  %12 = zext i32 %11 to i64
  %13 = shl i64 %i641, %12
  %14 = zext i32 %i322 to i64
  %15 = lshr i64 %i64, %14
  %16 = or i64 %13, %15
  %17 = lshr i64 %i641, %14
  br label %blk_exit
}

define __trunctfdf2 {
allocator:
  %stack_var_-8_ = alloca i64, align 8
  %stack_var_-16_ = alloca i64, align 8
  %stack_var_-24_ = alloca i64, align 8
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-32_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -32
  %2 = ptrtoint i32* %stack_var_-32_ to i32
  %3 = and i64 %i641, 9223372036854775807
  %4 = add nsw i64 %3, -4323737117252386816
  %5 = add nsw i64 %3, -4899634919602388992
  %.not = icmp ult i64 %4, %5
  br i1 %.not, label %brif_next, label %blk_exit2

blk_exit:                                         ; preds = %brif_next25, %blk_exit17, %brif_next23, %brif_next12, %blk_exit8, %brif_next10, %brif_next7, %blk_exit3, %brif_next5
  %_local_4.0 = phi i64 [ %19, %brif_next7 ], [ %16, %blk_exit3 ], [ %17, %brif_next5 ], [ 9218868437227405312, %blk_exit8 ], [ 0, %brif_next12 ], [ %46, %brif_next25 ], [ %35, %blk_exit17 ], [ %44, %brif_next23 ], [ %23, %brif_next10 ]
  %6 = ptrtoint i32* %stack_var_0_ to i32
  %7 = and i64 %i641, -9223372036854775808
  %8 = or i64 %_local_4.0, %7
  %9 = bitcast i64 %8 to double
  ret double %9

blk_exit2:                                        ; preds = %allocator
  %10 = icmp eq i64 %i64, 0
  %11 = icmp ult i64 %3, 9223090561878065152
  %12 = icmp eq i64 %3, 9223090561878065152
  %.v = select i1 %12, i1 %10, i1 %11
  br i1 %.v, label %blk_exit8, label %brif_next10

brif_next:                                        ; preds = %allocator
  %13 = call i64 @llvm.fshl.i64(i64 %i641, i64 %i64, i64 4)
  %14 = and i64 %i64, 1152921504606846975
  %15 = icmp ult i64 %14, 576460752303423489
  br i1 %15, label %blk_exit3, label %brif_next5

blk_exit3:                                        ; preds = %brif_next
  %16 = add i64 %13, 4611686018427387904
  %.not26 = icmp eq i64 %14, 576460752303423488
  br i1 %.not26, label %brif_next7, label %blk_exit

brif_next5:                                       ; preds = %brif_next
  %17 = add i64 %13, 4611686018427387905
  br label %blk_exit

brif_next7:                                       ; preds = %blk_exit3
  %18 = and i64 %13, 1
  %19 = add i64 %16, %18
  br label %blk_exit

blk_exit8:                                        ; preds = %blk_exit2
  %20 = icmp ugt i64 %3, 4899634919602388991
  br i1 %20, label %blk_exit, label %brif_next12

brif_next10:                                      ; preds = %blk_exit2
  %21 = call i64 @llvm.fshl.i64(i64 %i641, i64 %i64, i64 4)
  %22 = and i64 %21, 2251799813685247
  %23 = or i64 %22, 9221120237041090560
  br label %blk_exit

brif_next12:                                      ; preds = %blk_exit8
  %24 = lshr i64 %3, 48
  %25 = trunc i64 %24 to i32
  %26 = icmp ult i32 %25, 15249
  br i1 %26, label %blk_exit, label %brif_next14

brif_next14:                                      ; preds = %brif_next12
  %27 = add i32 %0, -16
  %28 = and i64 %i641, 281474976710655
  %29 = or i64 %28, 281474976710656
  %30 = add nsw i32 %25, -15233
  call void @__ashlti3(i32 %27, i64 %i64, i64 %29, i32 %30)
  %31 = sub nsw i32 15361, %25
  call void @__lshrti3(i32 %1, i64 %i64, i64 %29, i32 %31)
  %32 = load i32, i32* %stack_var_-32_, align 4
  %33 = sext i32 %32 to i64
  %34 = load i64, i64* %stack_var_-24_, align 4
  %35 = call i64 @llvm.fshl.i64(i64 %34, i64 %33, i64 4)
  %36 = and i64 %33, 1152921504606846975
  %37 = load i64, i64* %stack_var_-16_, align 4
  %38 = load i64, i64* %stack_var_-8_, align 4
  %39 = or i64 %37, %38
  %40 = icmp ne i64 %39, 0
  %41 = zext i1 %40 to i64
  %42 = or i64 %36, %41
  %43 = icmp ult i64 %42, 576460752303423489
  br i1 %43, label %blk_exit17, label %brif_next23

blk_exit17:                                       ; preds = %brif_next14
  %.not27 = icmp eq i64 %42, 576460752303423488
  br i1 %.not27, label %brif_next25, label %blk_exit

brif_next23:                                      ; preds = %brif_next14
  %44 = add i64 %35, 1
  br label %blk_exit

brif_next25:                                      ; preds = %blk_exit17
  %45 = and i64 %35, 1
  %46 = add i64 %45, %35
  br label %blk_exit
}

define __towrite {
allocator:
  %calcOffset = add i32 %i32, 74
  %0 = sext i32 %calcOffset to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %loadResult = load i8, i8* %1, align 1
  %2 = add i8 %loadResult, -1
  %3 = or i8 %2, %loadResult
  store i8 %3, i8* %1, align 1
  %4 = sext i32 %i32 to i64
  %5 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %4
  %6 = bitcast i8* %5 to i32*
  %loadResult3 = load i32, i32* %6, align 4
  %7 = and i32 %loadResult3, 8
  %brif_val.not.not = icmp eq i32 %7, 0
  br i1 %brif_val.not.not, label %blk_exit, label %brif_next

return:                                           ; preds = %blk_exit, %brif_next
  %return_0 = phi i32 [ -1, %brif_next ], [ 0, %blk_exit ]
  ret i32 %return_0

blk_exit:                                         ; preds = %allocator
  %calcOffset5 = add i32 %i32, 4
  %8 = sext i32 %calcOffset5 to i64
  %9 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %8
  %10 = bitcast i8* %9 to i64*
  store i64 0, i64* %10, align 4
  %calcOffset6 = add i32 %i32, 44
  %11 = sext i32 %calcOffset6 to i64
  %12 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %11
  %13 = bitcast i8* %12 to i32*
  %loadResult7 = load i32, i32* %13, align 4
  %calcOffset8 = add i32 %i32, 28
  %14 = sext i32 %calcOffset8 to i64
  %15 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %14
  %16 = bitcast i8* %15 to i32*
  store i32 %loadResult7, i32* %16, align 4
  %calcOffset9 = add i32 %i32, 20
  %17 = sext i32 %calcOffset9 to i64
  %18 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %17
  %19 = bitcast i8* %18 to i32*
  store i32 %loadResult7, i32* %19, align 4
  %calcOffset10 = add i32 %i32, 48
  %20 = sext i32 %calcOffset10 to i64
  %21 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %20
  %22 = bitcast i8* %21 to i32*
  %loadResult11 = load i32, i32* %22, align 4
  %23 = add i32 %loadResult7, %loadResult11
  %calcOffset12 = add i32 %i32, 16
  %24 = sext i32 %calcOffset12 to i64
  %25 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %24
  %26 = bitcast i8* %25 to i32*
  store i32 %23, i32* %26, align 4
  br label %return

brif_next:                                        ; preds = %allocator
  %27 = or i32 %loadResult3, 32
  store i32 %27, i32* %6, align 4
  br label %return
}

define __fwritex {
allocator:
  %calcOffset = add i32 %i322, 16
  %0 = sext i32 %calcOffset to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %2 = bitcast i8* %1 to i32*
  %loadResult = load i32, i32* %2, align 4
  %brif_val.not = icmp eq i32 %loadResult, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit3

return:                                           ; preds = %blk_exit, %brif_next12
  %return_0 = phi i32 [ %16, %brif_next12 ], [ %_local_4.0, %blk_exit ]
  ret i32 %return_0

blk_exit:                                         ; preds = %blk_exit15, %brif_next27, %brif_next
  %_local_4.0 = phi i32 [ %19, %blk_exit15 ], [ %30, %brif_next27 ], [ 0, %brif_next ]
  br label %return

blk_exit3:                                        ; preds = %brif_next5, %allocator
  %_local_3.0 = phi i32 [ %loadResult7, %brif_next5 ], [ %loadResult, %allocator ]
  %calcOffset9 = add i32 %i322, 20
  %3 = sext i32 %calcOffset9 to i64
  %4 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %3
  %5 = bitcast i8* %4 to i32*
  %loadResult10 = load i32, i32* %5, align 4
  %6 = sub i32 %_local_3.0, %loadResult10
  %.not = icmp ult i32 %6, %i321
  br i1 %.not, label %brif_next12, label %blk_exit8

brif_next:                                        ; preds = %allocator
  %7 = call i32 @__towrite(i32 %i322)
  %brif_val4.not = icmp eq i32 %7, 0
  br i1 %brif_val4.not, label %brif_next5, label %blk_exit

brif_next5:                                       ; preds = %brif_next
  %loadResult7 = load i32, i32* %2, align 4
  br label %blk_exit3

blk_exit8:                                        ; preds = %blk_exit3
  %calcOffset17 = add i32 %i322, 75
  %8 = sext i32 %calcOffset17 to i64
  %9 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %8
  %loadResult18 = load i8, i8* %9, align 1
  %brif_val19.not = icmp slt i8 %loadResult18, 0
  br i1 %brif_val19.not, label %brif_next20, label %blk_exit16

brif_next12:                                      ; preds = %blk_exit3
  %calcOffset13 = add i32 %i322, 36
  %10 = sext i32 %calcOffset13 to i64
  %11 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %10
  %12 = bitcast i8* %11 to i32*
  %loadResult14 = load i32, i32* %12, align 4
  %13 = sext i32 %loadResult14 to i64
  %14 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %13
  %15 = bitcast void ()** %14 to i32 (i32, i32, i32)**
  %callind_funcptr38 = load i32 (i32, i32, i32)*, i32 (i32, i32, i32)** %15, align 8
  %16 = call i32 %callind_funcptr38(i32 %i322, i32 %i32, i32 %i321)
  br label %return

blk_exit15:                                       ; preds = %brif_next32, %brif_next23, %brif_next20
  %_local_3.1 = phi i32 [ 0, %brif_next20 ], [ 0, %brif_next23 ], [ %_local_4.1, %brif_next32 ]
  %_local_5.0 = phi i32 [ %loadResult10, %brif_next20 ], [ %loadResult10, %brif_next23 ], [ %loadResult34, %brif_next32 ]
  %_param_1.0 = phi i32 [ %i321, %brif_next20 ], [ %i321, %brif_next23 ], [ %33, %brif_next32 ]
  %_param_0.0 = phi i32 [ %i32, %brif_next20 ], [ %i32, %brif_next23 ], [ %32, %brif_next32 ]
  %17 = call i32 @__memcpy(i32 %_local_5.0, i32 %_param_0.0, i32 %_param_1.0)
  %loadResult36 = load i32, i32* %5, align 4
  %18 = add i32 %loadResult36, %_param_1.0
  store i32 %18, i32* %5, align 4
  %19 = add i32 %_local_3.1, %_param_1.0
  br label %blk_exit

blk_exit16:                                       ; preds = %blk_exit8
  br label %loop_entry

brif_next20:                                      ; preds = %blk_exit8
  br label %blk_exit15

loop_entry:                                       ; preds = %blk_exit21, %blk_exit16
  %_local_4.1 = phi i32 [ %i321, %blk_exit16 ], [ %20, %blk_exit21 ]
  %brif_val22.not = icmp eq i32 %_local_4.1, 0
  br i1 %brif_val22.not, label %brif_next23, label %blk_exit21

blk_exit21:                                       ; preds = %loop_entry
  %20 = add i32 %_local_4.1, -1
  %21 = add i32 %20, %i32
  %22 = sext i32 %21 to i64
  %23 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %22
  %loadResult25 = load i8, i8* %23, align 1
  %.not40 = icmp eq i8 %loadResult25, 10
  br i1 %.not40, label %brif_next27, label %loop_entry

brif_next23:                                      ; preds = %loop_entry
  br label %blk_exit15

brif_next27:                                      ; preds = %blk_exit21
  %calcOffset28 = add i32 %i322, 36
  %24 = sext i32 %calcOffset28 to i64
  %25 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %24
  %26 = bitcast i8* %25 to i32*
  %loadResult29 = load i32, i32* %26, align 4
  %27 = sext i32 %loadResult29 to i64
  %28 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %27
  %29 = bitcast void ()** %28 to i32 (i32, i32, i32)**
  %callind_funcptr3041 = load i32 (i32, i32, i32)*, i32 (i32, i32, i32)** %29, align 8
  %30 = call i32 %callind_funcptr3041(i32 %i322, i32 %i32, i32 %_local_4.1)
  %31 = icmp ult i32 %30, %_local_4.1
  br i1 %31, label %blk_exit, label %brif_next32

brif_next32:                                      ; preds = %brif_next27
  %32 = add i32 %_local_4.1, %i32
  %33 = sub i32 %i321, %_local_4.1
  %loadResult34 = load i32, i32* %5, align 4
  br label %blk_exit15
}

define __vfprintf_internal {
allocator:
  %stack_var_-128_ = alloca i32, align 4
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-8_ = alloca i32, align 4
  %stack_var_-4_ = alloca i32, align 4
  %stack_var_-208_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = ptrtoint i32* %stack_var_-208_ to i32
  store i32 %i322, i32* %stack_var_-4_, align 4
  %2 = add i32 %0, -48
  %3 = call i32 @memset(i32 %2, i32 0, i32 40)
  %4 = load i32, i32* %stack_var_-4_, align 4
  %calcOffset6 = add i32 %0, -8
  store i32 %4, i32* %stack_var_-8_, align 4
  %5 = add i32 %0, -128
  %6 = call i32 @printf_core(i32 0, i32 %i321, i32 %calcOffset6, i32 %5, i32 %2, i32 %i323, i32 %i324)
  %brif_val.not = icmp slt i32 %6, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

blk_exit:                                         ; preds = %brif_next50, %blk_exit21, %brif_next
  %_param_1.0 = phi i32 [ -1, %brif_next ], [ %26, %blk_exit21 ], [ %26, %brif_next50 ]
  %7 = ptrtoint i32* %stack_var_0_ to i32
  ret i32 %_param_1.0

blk_exit7:                                        ; preds = %allocator
  %calcOffset9 = add i32 %i32, 76
  %8 = sext i32 %calcOffset9 to i64
  %9 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %8
  %10 = bitcast i8* %9 to i32*
  %loadResult10 = load i32, i32* %10, align 4
  %brif_val11.not = icmp sgt i32 %loadResult10, -1
  br i1 %brif_val11.not, label %brif_next12, label %blk_exit8

brif_next:                                        ; preds = %allocator
  br label %blk_exit

blk_exit8:                                        ; preds = %brif_next12, %blk_exit7
  %_param_2.0 = phi i32 [ %17, %brif_next12 ], [ 0, %blk_exit7 ]
  %11 = sext i32 %i32 to i64
  %12 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %11
  %13 = bitcast i8* %12 to i32*
  %loadResult14 = load i32, i32* %13, align 4
  %calcOffset16 = add i32 %i32, 74
  %14 = sext i32 %calcOffset16 to i64
  %15 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %14
  %loadResult17 = load i8, i8* %15, align 1
  %16 = icmp sgt i8 %loadResult17, 0
  br i1 %16, label %blk_exit15, label %brif_next19

brif_next12:                                      ; preds = %blk_exit7
  %17 = call i32 @__lockfile(i32 %i32)
  br label %blk_exit8

blk_exit15:                                       ; preds = %brif_next19, %blk_exit8
  %18 = and i32 %loadResult14, 32
  %calcOffset23 = add i32 %i32, 48
  %19 = sext i32 %calcOffset23 to i64
  %20 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %19
  %21 = bitcast i8* %20 to i32*
  %loadResult24 = load i32, i32* %21, align 4
  %22 = icmp eq i32 %loadResult24, 0
  br i1 %22, label %blk_exit22, label %brif_next26

brif_next19:                                      ; preds = %blk_exit8
  %23 = and i32 %loadResult14, -33
  store i32 %23, i32* %13, align 4
  br label %blk_exit15

blk_exit21:                                       ; preds = %brif_next35, %blk_exit22, %brif_next26
  %_param_1.1 = phi i32 [ %44, %blk_exit22 ], [ %54, %brif_next35 ], [ %46, %brif_next26 ]
  %loadResult46 = load i32, i32* %13, align 4
  %24 = or i32 %loadResult46, %18
  store i32 %24, i32* %13, align 4
  %25 = and i32 %loadResult46, 32
  %select_cond48.not = icmp eq i32 %25, 0
  %26 = select i1 %select_cond48.not, i32 %_param_1.1, i32 -1
  %27 = icmp eq i32 %_param_2.0, 0
  br i1 %27, label %blk_exit, label %brif_next50

blk_exit22:                                       ; preds = %blk_exit15
  store i32 80, i32* %21, align 4
  %calcOffset28 = add i32 %i32, 16
  %28 = sext i32 %calcOffset28 to i64
  %29 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %28
  %30 = bitcast i8* %29 to i32*
  %31 = ptrtoint i32* %stack_var_-128_ to i32
  store i32 %31, i32* %30, align 4
  %calcOffset29 = add i32 %i32, 28
  %32 = sext i32 %calcOffset29 to i64
  %33 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %32
  %34 = bitcast i8* %33 to i32*
  %35 = ptrtoint i32* %stack_var_-208_ to i32
  store i32 %35, i32* %34, align 4
  %calcOffset30 = add i32 %i32, 20
  %36 = sext i32 %calcOffset30 to i64
  %37 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %36
  %38 = bitcast i8* %37 to i32*
  %39 = ptrtoint i32* %stack_var_-208_ to i32
  store i32 %39, i32* %38, align 4
  %calcOffset31 = add i32 %i32, 44
  %40 = sext i32 %calcOffset31 to i64
  %41 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %40
  %42 = bitcast i8* %41 to i32*
  %loadResult32 = load i32, i32* %42, align 4
  %43 = ptrtoint i32* %stack_var_-208_ to i32
  store i32 %43, i32* %42, align 4
  %44 = call i32 @printf_core(i32 %i32, i32 %i321, i32 %calcOffset6, i32 %5, i32 %2, i32 %i323, i32 %i324)
  %45 = icmp eq i32 %loadResult32, 0
  br i1 %45, label %blk_exit21, label %brif_next35

brif_next26:                                      ; preds = %blk_exit15
  %46 = call i32 @printf_core(i32 %i32, i32 %i321, i32 %calcOffset6, i32 %5, i32 %2, i32 %i323, i32 %i324)
  br label %blk_exit21

brif_next35:                                      ; preds = %blk_exit22
  %calcOffset36 = add i32 %i32, 36
  %47 = sext i32 %calcOffset36 to i64
  %48 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %47
  %49 = bitcast i8* %48 to i32*
  %loadResult37 = load i32, i32* %49, align 4
  %50 = sext i32 %loadResult37 to i64
  %51 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %50
  %52 = bitcast void ()** %51 to i32 (i32, i32, i32)**
  %callind_funcptr51 = load i32 (i32, i32, i32)*, i32 (i32, i32, i32)** %52, align 8
  %53 = call i32 %callind_funcptr51(i32 %i32, i32 0, i32 0)
  store i32 0, i32* %21, align 4
  store i32 %loadResult32, i32* %42, align 4
  store i32 0, i32* %34, align 4
  store i32 0, i32* %30, align 4
  %loadResult43 = load i32, i32* %38, align 4
  store i32 0, i32* %38, align 4
  %select_cond.not = icmp eq i32 %loadResult43, 0
  %54 = select i1 %select_cond.not, i32 -1, i32 %44
  br label %blk_exit21

brif_next50:                                      ; preds = %blk_exit21
  call void @__unlockfile(i32 %i32)
  br label %blk_exit
}

define printf_core {
allocator:
  %stack_var_-72_ = alloca i32, align 4
  %stack_var_-68_ = alloca i32, align 4
  %stack_var_-25_ = alloca i8, align 1
  %stack_var_-16_ = alloca i64, align 8
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-4_ = alloca i32, align 4
  %stack_var_-80_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = ptrtoint i32* %stack_var_-80_ to i32
  store i32 %i321, i32* %stack_var_-4_, align 4
  %2 = add i32 %0, -25
  %3 = add i32 %0, -24
  br label %loop_entry

blk_exit:                                         ; preds = %brif_next347, %blk_exit13, %blk_exit16, %brif_next356, %blk_exit342, %blk_exit17, %blk_exit306, %blk_exit179, %brif_next181, %loop_entry165
  %_local_20.0 = phi i32 [ 0, %blk_exit13 ], [ 1, %blk_exit342 ], [ 1, %brif_next356 ], [ -1, %blk_exit16 ], [ 1, %brif_next347 ], [ %_local_11.1, %blk_exit17 ], [ -1, %loop_entry165 ], [ -1, %blk_exit306 ], [ -1, %blk_exit179 ], [ -1, %brif_next181 ]
  %4 = ptrtoint i32* %stack_var_0_ to i32
  ret i32 %_local_20.0

loop_entry:                                       ; preds = %blk_exit14, %blk_exit200, %blk_exit201, %blk_exit224, %blk_exit225, %blk_exit226, %blk_exit227, %blk_exit228, %blk_exit229, %blk_exit230, %blk_exit212, %blk_exit178, %blk_exit40, %allocator
  %_local_11.0 = phi i32 [ 0, %allocator ], [ %_local_11.1, %blk_exit178 ], [ %_local_11.1, %blk_exit201 ], [ %_local_11.1, %blk_exit14 ], [ %_local_11.1, %blk_exit200 ], [ %_local_11.1, %blk_exit212 ], [ %_local_11.1, %blk_exit225 ], [ %_local_11.1, %blk_exit224 ], [ %_local_11.1, %blk_exit226 ], [ %_local_11.1, %blk_exit227 ], [ %_local_11.1, %blk_exit228 ], [ %_local_11.1, %blk_exit229 ], [ %_local_11.1, %blk_exit230 ], [ %_local_11.1, %blk_exit40 ]
  %_local_10.0 = phi i32 [ 0, %allocator ], [ %_local_10.2, %blk_exit178 ], [ %_local_10.2, %blk_exit201 ], [ %_local_10.2, %blk_exit14 ], [ %_local_10.2, %blk_exit200 ], [ %_local_10.2, %blk_exit212 ], [ %_local_10.2, %blk_exit225 ], [ %_local_10.2, %blk_exit224 ], [ %_local_10.2, %blk_exit226 ], [ %_local_10.2, %blk_exit227 ], [ %_local_10.2, %blk_exit228 ], [ %_local_10.2, %blk_exit229 ], [ %_local_10.2, %blk_exit230 ], [ %_local_10.0, %blk_exit40 ]
  %_param_1.0 = phi i32 [ 0, %allocator ], [ 0, %blk_exit178 ], [ %176, %blk_exit201 ], [ %20, %blk_exit14 ], [ %173, %blk_exit200 ], [ 0, %blk_exit212 ], [ 0, %blk_exit225 ], [ 0, %blk_exit224 ], [ 0, %blk_exit226 ], [ 0, %blk_exit227 ], [ 0, %blk_exit228 ], [ 0, %blk_exit229 ], [ 0, %blk_exit230 ], [ %26, %blk_exit40 ]
  %brif_val.not = icmp sgt i32 %_local_11.0, -1
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

blk_exit7:                                        ; preds = %blk_exit8, %brif_next10, %loop_entry
  %_local_11.1 = phi i32 [ -1, %brif_next10 ], [ %10, %blk_exit8 ], [ %_local_11.0, %loop_entry ]
  %calcOffset12 = add i32 %0, -4
  %5 = load i32, i32* %stack_var_-4_, align 4
  %6 = sext i32 %5 to i64
  %7 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %6
  %loadResult19 = load i8, i8* %7, align 1
  %8 = icmp eq i8 %loadResult19, 0
  br i1 %8, label %blk_exit17, label %loop_entry22

brif_next:                                        ; preds = %loop_entry
  %9 = sub i32 2147483647, %_local_11.0
  %.not = icmp sgt i32 %_param_1.0, %9
  br i1 %.not, label %brif_next10, label %blk_exit8

blk_exit8:                                        ; preds = %brif_next
  %10 = add i32 %_param_1.0, %_local_11.0
  br label %blk_exit7

brif_next10:                                      ; preds = %brif_next
  %11 = call i32 @__errno_location()
  %12 = sext i32 %11 to i64
  %13 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %12
  %14 = bitcast i8* %13 to i32*
  store i32 61, i32* %14, align 4
  br label %blk_exit7

blk_exit13:                                       ; preds = %brif_next339, %blk_exit184
  br label %blk_exit

blk_exit14:                                       ; preds = %blk_exit15, %blk_exit199, %blk_exit205, %blk_exit214, %blk_exit214, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177
  %_local_16.0 = phi i32 [ %_local_16.1, %blk_exit15 ], [ 1024, %blk_exit214 ], [ 1024, %blk_exit214 ], [ 1024, %blk_exit205 ], [ 1024, %blk_exit199 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ], [ 1024, %blk_exit177 ]
  %_local_13.0 = phi i32 [ %_local_13.1, %blk_exit15 ], [ %143, %blk_exit214 ], [ %143, %blk_exit214 ], [ %141, %blk_exit205 ], [ %141, %blk_exit199 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ]
  %_local_12.0 = phi i32 [ %_local_12.1, %blk_exit15 ], [ %5, %blk_exit214 ], [ %5, %blk_exit214 ], [ %184, %blk_exit205 ], [ %2, %blk_exit199 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ], [ %5, %blk_exit177 ]
  %_local_17.0 = phi i32 [ %3, %blk_exit15 ], [ %3, %blk_exit214 ], [ %3, %blk_exit214 ], [ %187, %blk_exit205 ], [ %3, %blk_exit199 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ], [ %3, %blk_exit177 ]
  %_local_19.0 = phi i32 [ %_local_19.1, %blk_exit15 ], [ %_local_19.2, %blk_exit214 ], [ %_local_19.2, %blk_exit214 ], [ %189, %blk_exit205 ], [ 1, %blk_exit199 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ]
  %_local_20.1 = phi i32 [ %_local_20.2, %blk_exit15 ], [ 0, %blk_exit214 ], [ 0, %blk_exit214 ], [ 0, %blk_exit205 ], [ 0, %blk_exit199 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ], [ 0, %blk_exit177 ]
  %15 = sub i32 %_local_17.0, %_local_12.0
  %16 = icmp slt i32 %_local_19.0, %15
  %17 = select i1 %16, i32 %15, i32 %_local_19.0
  %18 = add i32 %_local_20.1, %17
  %19 = icmp slt i32 %_local_18.0, %18
  %20 = select i1 %19, i32 %18, i32 %_local_18.0
  call void @pad(i32 %i32, i32 32, i32 %20, i32 %18, i32 %_local_13.0)
  call void @out(i32 %i32, i32 %_local_16.0, i32 %_local_20.1)
  %21 = xor i32 %_local_13.0, 65536
  call void @pad(i32 %i32, i32 48, i32 %20, i32 %18, i32 %21)
  call void @pad(i32 %i32, i32 48, i32 %17, i32 %15, i32 0)
  call void @out(i32 %i32, i32 %_local_12.0, i32 %15)
  %22 = xor i32 %_local_13.0, 8192
  call void @pad(i32 %i32, i32 32, i32 %20, i32 %18, i32 %22)
  br label %loop_entry

blk_exit15:                                       ; preds = %blk_exit219, %blk_exit277, %brif_next283
  %_local_16.1 = phi i32 [ 1024, %blk_exit219 ], [ %_local_16.3, %blk_exit277 ], [ %_local_16.3, %brif_next283 ]
  %_local_13.1 = phi i32 [ %143, %blk_exit219 ], [ %191, %blk_exit277 ], [ %191, %brif_next283 ]
  %_local_12.1 = phi i32 [ %5, %blk_exit219 ], [ %_local_12.2, %blk_exit277 ], [ %3, %brif_next283 ]
  %_local_19.1 = phi i32 [ %_local_19.2, %blk_exit219 ], [ %268, %blk_exit277 ], [ 0, %brif_next283 ]
  %_local_20.2 = phi i32 [ 0, %blk_exit219 ], [ %_local_20.3, %blk_exit277 ], [ %_local_20.3, %brif_next283 ]
  br label %blk_exit14

blk_exit16:                                       ; preds = %loop_entry352, %blk_exit129, %blk_exit78, %blk_exit82
  br label %blk_exit

blk_exit17:                                       ; preds = %blk_exit7
  %brif_val338.not = icmp eq i32 %i32, 0
  br i1 %brif_val338.not, label %brif_next339, label %blk_exit

loop_entry22:                                     ; preds = %blk_exit7, %blk_exit23
  %_local_13.2.in = phi i8 [ %loadResult19, %blk_exit7 ], [ %loadResult337, %blk_exit23 ]
  %_param_1.1 = phi i32 [ %5, %blk_exit7 ], [ %23, %blk_exit23 ]
  %brif_val26.not = icmp eq i8 %_local_13.2.in, 0
  br i1 %brif_val26.not, label %brif_next27, label %blk_exit25

blk_exit23:                                       ; preds = %blk_exit25
  %23 = add i32 %_param_1.1, 1
  store i32 %23, i32* %stack_var_-4_, align 4
  %24 = sext i32 %23 to i64
  %25 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %24
  %loadResult337 = load i8, i8* %25, align 1
  br label %loop_entry22

blk_exit24:                                       ; preds = %brif_next34, %loop_entry30, %brif_next27
  %_local_13.3 = phi i32 [ %_param_1.1, %brif_next27 ], [ %31, %brif_next34 ], [ %_local_13.4, %loop_entry30 ]
  %26 = sub i32 %_local_13.3, %5
  %27 = icmp eq i32 %i32, 0
  br i1 %27, label %blk_exit40, label %brif_next42

blk_exit25:                                       ; preds = %loop_entry22
  %.not376 = icmp eq i8 %_local_13.2.in, 37
  br i1 %.not376, label %brif_next29, label %blk_exit23

brif_next27:                                      ; preds = %loop_entry22
  br label %blk_exit24

brif_next29:                                      ; preds = %blk_exit25
  br label %loop_entry30

loop_entry30:                                     ; preds = %brif_next34, %brif_next29
  %_local_13.4 = phi i32 [ %_param_1.1, %brif_next29 ], [ %31, %brif_next34 ]
  %_param_1.2 = phi i32 [ %_param_1.1, %brif_next29 ], [ %30, %brif_next34 ]
  %calcOffset31 = add i32 %_param_1.2, 1
  %28 = sext i32 %calcOffset31 to i64
  %29 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %28
  %loadResult32 = load i8, i8* %29, align 1
  %.not377 = icmp eq i8 %loadResult32, 37
  br i1 %.not377, label %brif_next34, label %blk_exit24

brif_next34:                                      ; preds = %loop_entry30
  %30 = add i32 %_param_1.2, 2
  store i32 %30, i32* %stack_var_-4_, align 4
  %31 = add i32 %_local_13.4, 1
  %32 = sext i32 %30 to i64
  %33 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %32
  %loadResult37 = load i8, i8* %33, align 1
  %34 = icmp eq i8 %loadResult37, 37
  br i1 %34, label %loop_entry30, label %blk_exit24

blk_exit40:                                       ; preds = %brif_next42, %blk_exit24
  %brif_val43.not = icmp eq i32 %26, 0
  br i1 %brif_val43.not, label %brif_next44, label %loop_entry

brif_next42:                                      ; preds = %blk_exit24
  call void @out(i32 %i32, i32 %5, i32 %26)
  br label %blk_exit40

brif_next44:                                      ; preds = %blk_exit40
  %35 = load i32, i32* %stack_var_-4_, align 4
  %calcOffset47 = add i32 %35, 1
  %36 = sext i32 %calcOffset47 to i64
  %37 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %36
  %loadResult48 = load i8, i8* %37, align 1
  %38 = sext i8 %loadResult48 to i32
  %isdigittmp = add nsw i32 %38, -48
  %isdigit = icmp ugt i32 %isdigittmp, 9
  br i1 %isdigit, label %blk_exit51, label %brif_next53

blk_exit51:                                       ; preds = %brif_next57, %brif_next53, %brif_next44
  %_local_16.2 = phi i32 [ -1, %brif_next44 ], [ %isdigittmp, %brif_next57 ], [ -1, %brif_next53 ]
  %_local_13.5 = phi i32 [ 1, %brif_next44 ], [ 3, %brif_next57 ], [ 1, %brif_next53 ]
  %_local_10.1 = phi i32 [ %_local_10.0, %brif_next44 ], [ 1, %brif_next57 ], [ %_local_10.0, %brif_next53 ]
  %39 = add i32 %35, %_local_13.5
  store i32 %39, i32* %stack_var_-4_, align 4
  %40 = sext i32 %39 to i64
  %41 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %40
  %loadResult64 = load i8, i8* %41, align 1
  %42 = sext i8 %loadResult64 to i32
  %43 = add nsw i32 %42, -32
  %44 = icmp ult i32 %43, 32
  br i1 %44, label %blk_exit62, label %brif_next66

brif_next53:                                      ; preds = %brif_next44
  %calcOffset54 = add i32 %35, 2
  %45 = sext i32 %calcOffset54 to i64
  %46 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %45
  %loadResult55 = load i8, i8* %46, align 1
  %.not361 = icmp eq i8 %loadResult55, 36
  br i1 %.not361, label %brif_next57, label %blk_exit51

brif_next57:                                      ; preds = %brif_next53
  br label %blk_exit51

blk_exit61:                                       ; preds = %brif_next74, %loop_entry69, %blk_exit62, %brif_next66
  %_local_15.0 = phi i32 [ %54, %loop_entry69 ], [ %54, %brif_next74 ], [ %42, %blk_exit62 ], [ %42, %brif_next66 ]
  %loadResult88 = phi i32 [ %50, %loop_entry69 ], [ %50, %brif_next74 ], [ %39, %blk_exit62 ], [ %39, %brif_next66 ]
  %_local_17.1 = phi i32 [ %51, %loop_entry69 ], [ %51, %brif_next74 ], [ 0, %blk_exit62 ], [ 0, %brif_next66 ]
  %.not362 = icmp eq i32 %_local_15.0, 42
  br i1 %.not362, label %brif_next80, label %blk_exit78

blk_exit62:                                       ; preds = %blk_exit51
  %47 = shl i32 1, %43
  %48 = lshr i32 -75914, %43
  %49 = and i32 %48, 1
  %brif_val67.not = icmp eq i32 %49, 0
  br i1 %brif_val67.not, label %loop_entry69, label %blk_exit61

brif_next66:                                      ; preds = %blk_exit51
  br label %blk_exit61

loop_entry69:                                     ; preds = %blk_exit62, %brif_next74
  %_local_14.0 = phi i32 [ %47, %blk_exit62 ], [ %57, %brif_next74 ]
  %_local_17.2 = phi i32 [ 0, %blk_exit62 ], [ %51, %brif_next74 ]
  %_param_1.3 = phi i32 [ %39, %blk_exit62 ], [ %50, %brif_next74 ]
  %50 = add i32 %_param_1.3, 1
  store i32 %50, i32* %stack_var_-4_, align 4
  %51 = or i32 %_local_14.0, %_local_17.2
  %52 = sext i32 %50 to i64
  %53 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %52
  %loadResult72 = load i8, i8* %53, align 1
  %54 = sext i8 %loadResult72 to i32
  %55 = add nsw i32 %54, -32
  %56 = icmp ugt i32 %55, 31
  br i1 %56, label %blk_exit61, label %brif_next74

brif_next74:                                      ; preds = %loop_entry69
  %57 = shl i32 1, %55
  %58 = and i32 %57, 75913
  %brif_val75.not = icmp eq i32 %58, 0
  br i1 %brif_val75.not, label %blk_exit61, label %loop_entry69

blk_exit77:                                       ; preds = %brif_next116, %brif_next114, %blk_exit81
  %_local_10.2 = phi i32 [ %_local_10.3, %brif_next114 ], [ %_local_10.3, %blk_exit81 ], [ %_local_10.1, %brif_next116 ]
  %_local_17.3 = phi i32 [ %91, %brif_next114 ], [ %_local_17.1, %blk_exit81 ], [ %_local_17.1, %brif_next116 ]
  %_local_18.0 = phi i32 [ %90, %brif_next114 ], [ %_local_18.1, %blk_exit81 ], [ %61, %brif_next116 ]
  %loadResult135 = phi i32 [ %_param_1.5, %brif_next114 ], [ %_param_1.5, %blk_exit81 ], [ %92, %brif_next116 ]
  %59 = sext i32 %loadResult135 to i64
  %60 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %59
  %loadResult121 = load i8, i8* %60, align 1
  %.not366 = icmp eq i8 %loadResult121, 46
  br i1 %.not366, label %brif_next123, label %blk_exit119

blk_exit78:                                       ; preds = %blk_exit61
  %61 = call i32 @getint(i32 %calcOffset12)
  %brif_val115.not = icmp sgt i32 %61, -1
  br i1 %brif_val115.not, label %brif_next116, label %blk_exit16

brif_next80:                                      ; preds = %blk_exit61
  %calcOffset83 = add i32 %loadResult88, 1
  %62 = sext i32 %calcOffset83 to i64
  %63 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %62
  %loadResult84 = load i8, i8* %63, align 1
  %64 = sext i8 %loadResult84 to i32
  %65 = add nsw i32 %64, -58
  %isdigit364 = icmp ult i32 %65, -10
  br i1 %isdigit364, label %blk_exit82, label %brif_next86

blk_exit81:                                       ; preds = %blk_exit102, %brif_next92
  %_local_10.3 = phi i32 [ 0, %blk_exit102 ], [ 1, %brif_next92 ]
  %_local_18.1 = phi i32 [ %_local_18.2, %blk_exit102 ], [ %loadResult99, %brif_next92 ]
  %_param_1.5 = phi i32 [ %.pre-phi, %blk_exit102 ], [ %74, %brif_next92 ]
  store i32 %_param_1.5, i32* %stack_var_-4_, align 4
  %brif_val113.not = icmp slt i32 %_local_18.1, 0
  br i1 %brif_val113.not, label %brif_next114, label %blk_exit77

blk_exit82:                                       ; preds = %brif_next86, %brif_next80
  %brif_val100.not = icmp eq i32 %_local_10.1, 0
  br i1 %brif_val100.not, label %brif_next101, label %blk_exit16

brif_next86:                                      ; preds = %brif_next80
  %calcOffset89 = add i32 %loadResult88, 2
  %66 = sext i32 %calcOffset89 to i64
  %67 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %66
  %loadResult90 = load i8, i8* %67, align 1
  %.not365 = icmp eq i8 %loadResult90, 36
  br i1 %.not365, label %brif_next92, label %blk_exit82

brif_next92:                                      ; preds = %brif_next86
  %68 = shl nsw i32 %64, 2
  %69 = add i32 %68, %i324
  %70 = add i32 %69, -192
  %71 = sext i32 %70 to i64
  %72 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %71
  %73 = bitcast i8* %72 to i32*
  store i32 10, i32* %73, align 4
  %74 = add i32 %loadResult88, 3
  %loadResult97 = load i8, i8* %63, align 1
  %75 = sext i8 %loadResult97 to i32
  %76 = shl nsw i32 %75, 3
  %77 = add i32 %76, %i323
  %78 = add i32 %77, -384
  %79 = sext i32 %78 to i64
  %80 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %79
  %81 = bitcast i8* %80 to i32*
  %loadResult99 = load i32, i32* %81, align 4
  br label %blk_exit81

brif_next101:                                     ; preds = %blk_exit82
  br i1 %27, label %blk_exit102, label %brif_next104

blk_exit102:                                      ; preds = %brif_next104, %brif_next101
  %.pre-phi = phi i32 [ %.pre, %brif_next104 ], [ %calcOffset83, %brif_next101 ]
  %_local_18.2 = phi i32 [ %loadResult109, %brif_next104 ], [ 0, %brif_next101 ]
  br label %blk_exit81

brif_next104:                                     ; preds = %brif_next101
  %82 = sext i32 %i322 to i64
  %83 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %82
  %84 = bitcast i8* %83 to i32*
  %loadResult106 = load i32, i32* %84, align 4
  %85 = add i32 %loadResult106, 4
  store i32 %85, i32* %84, align 4
  %86 = sext i32 %loadResult106 to i64
  %87 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %86
  %88 = bitcast i8* %87 to i32*
  %loadResult109 = load i32, i32* %88, align 4
  %89 = load i32, i32* %stack_var_-4_, align 4
  %.pre = add i32 %89, 1
  br label %blk_exit102

brif_next114:                                     ; preds = %blk_exit81
  %90 = sub i32 0, %_local_18.1
  %91 = or i32 %_local_17.1, 8192
  br label %blk_exit77

brif_next116:                                     ; preds = %blk_exit78
  %92 = load i32, i32* %stack_var_-4_, align 4
  br label %blk_exit77

blk_exit119:                                      ; preds = %blk_exit124, %blk_exit150, %brif_next139, %blk_exit77
  %_local_19.2 = phi i32 [ %storemerge374, %blk_exit150 ], [ %loadResult146, %brif_next139 ], [ %95, %blk_exit124 ], [ -1, %blk_exit77 ]
  %_param_1.6 = phi i32 [ %.pre-phi382, %blk_exit150 ], [ %116, %brif_next139 ], [ %96, %blk_exit124 ], [ %loadResult135, %blk_exit77 ]
  br label %loop_entry165

brif_next123:                                     ; preds = %blk_exit77
  %calcOffset125 = add i32 %loadResult135, 1
  %93 = sext i32 %calcOffset125 to i64
  %94 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %93
  %loadResult126 = load i8, i8* %94, align 1
  %.not367 = icmp eq i8 %loadResult126, 42
  br i1 %.not367, label %brif_next128, label %blk_exit124

blk_exit124:                                      ; preds = %brif_next123
  store i32 %calcOffset125, i32* %stack_var_-4_, align 4
  %95 = call i32 @getint(i32 %calcOffset12)
  %96 = load i32, i32* %stack_var_-4_, align 4
  br label %blk_exit119

brif_next128:                                     ; preds = %brif_next123
  %calcOffset130 = add i32 %loadResult135, 2
  %97 = sext i32 %calcOffset130 to i64
  %98 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %97
  %loadResult131 = load i8, i8* %98, align 1
  %99 = sext i8 %loadResult131 to i32
  %100 = add nsw i32 %99, -58
  %isdigit369 = icmp ult i32 %100, -10
  br i1 %isdigit369, label %blk_exit129, label %brif_next133

blk_exit129:                                      ; preds = %brif_next133, %brif_next128
  %brif_val148.not = icmp eq i32 %_local_10.2, 0
  br i1 %brif_val148.not, label %brif_next149, label %blk_exit16

brif_next133:                                     ; preds = %brif_next128
  %calcOffset136 = add i32 %loadResult135, 3
  %101 = sext i32 %calcOffset136 to i64
  %102 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %101
  %loadResult137 = load i8, i8* %102, align 1
  %.not370 = icmp eq i8 %loadResult137, 36
  br i1 %.not370, label %brif_next139, label %blk_exit129

brif_next139:                                     ; preds = %brif_next133
  %103 = shl nsw i32 %99, 2
  %104 = add i32 %103, %i324
  %105 = add i32 %104, -192
  %106 = sext i32 %105 to i64
  %107 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %106
  %108 = bitcast i8* %107 to i32*
  store i32 10, i32* %108, align 4
  %loadResult144 = load i8, i8* %98, align 1
  %109 = sext i8 %loadResult144 to i32
  %110 = shl nsw i32 %109, 3
  %111 = add i32 %110, %i323
  %112 = add i32 %111, -384
  %113 = sext i32 %112 to i64
  %114 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %113
  %115 = bitcast i8* %114 to i32*
  %loadResult146 = load i32, i32* %115, align 4
  %116 = add i32 %loadResult135, 4
  store i32 %116, i32* %stack_var_-4_, align 4
  br label %blk_exit119

brif_next149:                                     ; preds = %blk_exit129
  br i1 %27, label %brif_next153, label %blk_exit151

blk_exit150:                                      ; preds = %blk_exit151, %brif_next153
  %.pre-phi382 = phi i32 [ %.pre381, %blk_exit151 ], [ %calcOffset130, %brif_next153 ]
  %storemerge374 = phi i32 [ %loadResult158, %blk_exit151 ], [ 0, %brif_next153 ]
  store i32 %.pre-phi382, i32* %stack_var_-4_, align 4
  br label %blk_exit119

blk_exit151:                                      ; preds = %brif_next149
  %117 = sext i32 %i322 to i64
  %118 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %117
  %119 = bitcast i8* %118 to i32*
  %loadResult155 = load i32, i32* %119, align 4
  %120 = add i32 %loadResult155, 4
  store i32 %120, i32* %119, align 4
  %121 = sext i32 %loadResult155 to i64
  %122 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %121
  %123 = bitcast i8* %122 to i32*
  %loadResult158 = load i32, i32* %123, align 4
  %124 = load i32, i32* %stack_var_-4_, align 4
  %.pre381 = add i32 %124, 2
  br label %blk_exit150

brif_next153:                                     ; preds = %brif_next149
  br label %blk_exit150

loop_entry165:                                    ; preds = %brif_next169, %blk_exit119
  %_local_13.7 = phi i32 [ 0, %blk_exit119 ], [ %137, %brif_next169 ]
  %_param_1.7 = phi i32 [ %_param_1.6, %blk_exit119 ], [ %130, %brif_next169 ]
  %125 = sext i32 %_param_1.7 to i64
  %126 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %125
  %loadResult167 = load i8, i8* %126, align 1
  %127 = sext i8 %loadResult167 to i32
  %128 = add nsw i32 %127, -123
  %129 = icmp ult i32 %128, -58
  br i1 %129, label %blk_exit, label %brif_next169

brif_next169:                                     ; preds = %loop_entry165
  %130 = add i32 %_param_1.7, 1
  store i32 %130, i32* %stack_var_-4_, align 4
  %loadResult172 = load i8, i8* %126, align 1
  %131 = sext i8 %loadResult172 to i32
  %132 = mul nuw nsw i32 %_local_13.7, 58
  %133 = add nsw i32 %132, %131
  %134 = add nsw i32 %133, 1071
  %135 = zext i32 %134 to i64
  %136 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %135
  %loadResult174 = load i8, i8* %136, align 1
  %137 = zext i8 %loadResult174 to i32
  %138 = add nsw i32 %137, -1
  %139 = icmp ult i32 %138, 8
  br i1 %139, label %loop_entry165, label %brif_next176

brif_next176:                                     ; preds = %brif_next169
  %140 = icmp eq i8 %loadResult174, 19
  br i1 %140, label %blk_exit179, label %brif_next181

blk_exit177:                                      ; preds = %blk_exit178, %brif_next192
  %_local_15.1 = phi i32 [ %130, %blk_exit178 ], [ %165, %brif_next192 ]
  %141 = and i32 %_local_17.3, -65537
  %142 = and i32 %_local_17.3, 8192
  %select_cond.not = icmp eq i32 %142, 0
  %143 = select i1 %select_cond.not, i32 %_local_17.3, i32 %141
  %144 = add i32 %_local_15.1, -1
  %145 = sext i32 %144 to i64
  %146 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %145
  %loadResult216 = load i8, i8* %146, align 1
  %147 = sext i8 %loadResult216 to i32
  %148 = and i32 %147, -33
  %149 = and i32 %147, 15
  %150 = icmp eq i32 %149, 3
  %151 = select i1 %150, i32 %148, i32 %147
  %select_cond218.not = icmp eq i32 %_local_13.7, 0
  %152 = select i1 %select_cond218.not, i32 %147, i32 %151
  switch i32 %152, label %blk_exit214 [
    i32 88, label %blk_exit210
    i32 89, label %blk_exit14
    i32 90, label %blk_exit14
    i32 91, label %blk_exit14
    i32 92, label %blk_exit14
    i32 93, label %blk_exit14
    i32 94, label %blk_exit14
    i32 95, label %blk_exit14
    i32 96, label %blk_exit14
    i32 97, label %blk_exit200
    i32 98, label %blk_exit14
    i32 99, label %blk_exit199
    i32 100, label %blk_exit208
    i32 101, label %blk_exit200
    i32 102, label %blk_exit200
    i32 103, label %blk_exit200
    i32 104, label %blk_exit14
    i32 105, label %blk_exit208
    i32 106, label %blk_exit14
    i32 107, label %blk_exit14
    i32 108, label %blk_exit14
    i32 109, label %blk_exit14
    i32 110, label %blk_exit212
    i32 111, label %blk_exit209
    i32 112, label %blk_exit211
    i32 113, label %blk_exit14
    i32 114, label %blk_exit14
    i32 115, label %blk_exit205
    i32 116, label %blk_exit14
    i32 117, label %blk_exit213
    i32 118, label %blk_exit14
    i32 119, label %blk_exit14
    i32 120, label %blk_exit210
  ]

blk_exit178:                                      ; preds = %blk_exit179, %brif_next186
  br i1 %27, label %loop_entry, label %blk_exit177

blk_exit179:                                      ; preds = %brif_next176
  %brif_val195.not = icmp slt i32 %_local_16.2, 0
  br i1 %brif_val195.not, label %blk_exit178, label %blk_exit

brif_next181:                                     ; preds = %brif_next176
  %153 = icmp eq i8 %loadResult174, 0
  br i1 %153, label %blk_exit, label %brif_next183

brif_next183:                                     ; preds = %brif_next181
  %brif_val185.not = icmp sgt i32 %_local_16.2, -1
  br i1 %brif_val185.not, label %brif_next186, label %blk_exit184

blk_exit184:                                      ; preds = %brif_next183
  br i1 %27, label %blk_exit13, label %brif_next192

brif_next186:                                     ; preds = %brif_next183
  %154 = shl nsw i32 %_local_16.2, 2
  %155 = add i32 %154, %i324
  %156 = sext i32 %155 to i64
  %157 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %156
  %158 = bitcast i8* %157 to i32*
  store i32 %137, i32* %158, align 4
  %159 = shl nsw i32 %_local_16.2, 3
  %160 = add i32 %159, %i323
  %161 = sext i32 %160 to i64
  %162 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %161
  %163 = bitcast i8* %162 to i64*
  %loadResult189 = load i64, i64* %163, align 4
  store i64 %loadResult189, i64* %stack_var_-16_, align 4
  br label %blk_exit178

brif_next192:                                     ; preds = %blk_exit184
  %164 = add i32 %0, -16
  call void @pop_arg(i32 %164, i32 %137, i32 %i322, i32 %i326)
  %165 = load i32, i32* %stack_var_-4_, align 4
  br label %blk_exit177

blk_exit199:                                      ; preds = %blk_exit177
  %166 = load i64, i64* %stack_var_-16_, align 4
  %167 = trunc i64 %166 to i8
  store i8 %167, i8* %stack_var_-25_, align 1
  br label %blk_exit14

blk_exit200:                                      ; preds = %blk_exit214, %blk_exit214, %blk_exit214, %blk_exit214, %blk_exit177, %blk_exit177, %blk_exit177, %blk_exit177
  %168 = load i64, i64* %stack_var_-16_, align 4
  %169 = bitcast i64 %168 to double
  %170 = sext i32 %i325 to i64
  %171 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %170
  %172 = bitcast void ()** %171 to i32 (i32, double, i32, i32, i32, i32)**
  %callind_funcptr372 = load i32 (i32, double, i32, i32, i32, i32)*, i32 (i32, double, i32, i32, i32, i32)** %172, align 8
  %173 = call i32 %callind_funcptr372(i32 %i32, double %169, i32 %_local_18.0, i32 %_local_19.2, i32 %143, i32 %152)
  br label %loop_entry

blk_exit201:                                      ; preds = %brif_next326, %brif_next324, %loop_entry320, %brif_next317, %blk_exit290
  %_param_1.8 = phi i32 [ 0, %blk_exit290 ], [ 0, %brif_next317 ], [ %_param_1.10, %loop_entry320 ], [ %_param_1.10, %brif_next324 ], [ %_param_1.10, %brif_next326 ]
  %174 = xor i32 %143, 8192
  call void @pad(i32 %i32, i32 32, i32 %_local_18.0, i32 %_param_1.8, i32 %174)
  %175 = icmp sgt i32 %_local_18.0, %_param_1.8
  %176 = select i1 %175, i32 %_local_18.0, i32 %_param_1.8
  br label %loop_entry

blk_exit202:                                      ; preds = %blk_exit203, %brif_next292
  %_local_19.3 = phi i32 [ %_local_19.2, %brif_next292 ], [ -1, %blk_exit203 ]
  %storemerge = phi i32 [ %270, %brif_next292 ], [ %calcOffset298, %blk_exit203 ]
  br label %loop_entry301

blk_exit203:                                      ; preds = %blk_exit214
  store i32 0, i32* %stack_var_-68_, align 4
  %177 = load i64, i64* %stack_var_-16_, align 4
  %calcOffset298 = add i32 %0, -72
  %178 = trunc i64 %177 to i32
  store i32 %178, i32* %stack_var_-72_, align 4
  %179 = ptrtoint i32* %stack_var_-72_ to i32
  %180 = sext i32 %179 to i64
  store i64 %180, i64* %stack_var_-16_, align 4
  br label %blk_exit202

blk_exit204:                                      ; preds = %blk_exit219
  %181 = icmp eq i32 %_local_19.2, 0
  br i1 %181, label %blk_exit290, label %brif_next292

blk_exit205:                                      ; preds = %blk_exit177
  %182 = load i64, i64* %stack_var_-16_, align 4
  %183 = trunc i64 %182 to i32
  %select_cond287.not = icmp eq i32 %183, 0
  %184 = select i1 %select_cond287.not, i32 1071, i32 %183
  %185 = call i32 @memchr(i32 %184, i32 0, i32 %_local_19.2)
  %186 = add i32 %184, %_local_19.2
  %select_cond288.not = icmp eq i32 %185, 0
  %187 = select i1 %select_cond288.not, i32 %186, i32 %185
  %188 = sub i32 %185, %184
  %189 = select i1 %select_cond288.not, i32 %_local_19.2, i32 %188
  br label %blk_exit14

blk_exit206:                                      ; preds = %blk_exit207, %brif_next264, %blk_exit209, %brif_next260, %brif_next258, %blk_exit210
  %.pre-phi385 = phi i64 [ %.pre384.pre-phi, %blk_exit207 ], [ %198, %brif_next264 ], [ %198, %blk_exit209 ], [ %202, %brif_next260 ], [ %202, %brif_next258 ], [ %202, %blk_exit210 ]
  %_local_16.3 = phi i32 [ %_local_16.4, %blk_exit207 ], [ 1024, %brif_next264 ], [ 1024, %blk_exit209 ], [ %254, %brif_next260 ], [ 1024, %brif_next258 ], [ 1024, %blk_exit210 ]
  %_local_13.8 = phi i32 [ %143, %blk_exit207 ], [ %143, %brif_next264 ], [ %143, %blk_exit209 ], [ %_local_13.9, %brif_next260 ], [ %_local_13.9, %brif_next258 ], [ %_local_13.9, %blk_exit210 ]
  %_local_12.2 = phi i32 [ %195, %blk_exit207 ], [ %200, %brif_next264 ], [ %200, %blk_exit209 ], [ %205, %brif_next260 ], [ %205, %brif_next258 ], [ %205, %blk_exit210 ]
  %_local_19.4 = phi i32 [ %_local_19.2, %blk_exit207 ], [ %258, %brif_next264 ], [ %_local_19.2, %blk_exit209 ], [ %_local_19.5, %brif_next260 ], [ %_local_19.5, %brif_next258 ], [ %_local_19.5, %blk_exit210 ]
  %_local_20.3 = phi i32 [ %_local_20.4, %blk_exit207 ], [ 0, %brif_next264 ], [ 0, %blk_exit209 ], [ 2, %brif_next260 ], [ 0, %brif_next258 ], [ 0, %blk_exit210 ]
  %190 = and i32 %_local_13.8, -65537
  %select_cond276.not = icmp slt i32 %_local_19.4, 0
  %191 = select i1 %select_cond276.not, i32 %_local_13.8, i32 %190
  %192 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %.pre-phi385
  %193 = bitcast i8* %192 to i64*
  %loadResult279 = load i64, i64* %193, align 4
  %194 = icmp ne i64 %loadResult279, 0
  %brif_val282 = icmp ne i32 %_local_19.4, 0
  %or.cond = select i1 %194, i1 true, i1 %brif_val282
  br i1 %or.cond, label %blk_exit277, label %brif_next283

blk_exit207:                                      ; preds = %blk_exit272, %brif_next274, %brif_next270, %blk_exit213
  %.pre384.pre-phi = phi i64 [ %196, %blk_exit272 ], [ %196, %brif_next274 ], [ %196, %brif_next270 ], [ %211, %blk_exit213 ]
  %_local_16.4 = phi i32 [ %262, %blk_exit272 ], [ 1025, %brif_next274 ], [ 1024, %brif_next270 ], [ 1024, %blk_exit213 ]
  %_local_20.4 = phi i32 [ %261, %blk_exit272 ], [ 1, %brif_next274 ], [ 1, %brif_next270 ], [ 0, %blk_exit213 ]
  %_local_22.0 = phi i64 [ %197, %blk_exit272 ], [ %197, %brif_next274 ], [ %260, %brif_next270 ], [ %212, %blk_exit213 ]
  %195 = call i32 @fmt_u(i64 %_local_22.0, i32 %3)
  br label %blk_exit206

blk_exit208:                                      ; preds = %blk_exit177, %blk_exit177
  %calcOffset267 = add i32 %0, -16
  %196 = sext i32 %calcOffset267 to i64
  %197 = load i64, i64* %stack_var_-16_, align 4
  %brif_val269.not = icmp slt i64 %197, 0
  br i1 %brif_val269.not, label %brif_next270, label %blk_exit266

blk_exit209:                                      ; preds = %blk_exit177
  %calcOffset261 = add i32 %0, -16
  %198 = sext i32 %calcOffset261 to i64
  %199 = load i64, i64* %stack_var_-16_, align 4
  %200 = call i32 @fmt_o(i64 %199, i32 %3)
  %201 = and i32 %143, 8
  %brif_val263.not.not = icmp eq i32 %201, 0
  br i1 %brif_val263.not.not, label %blk_exit206, label %brif_next264

blk_exit210:                                      ; preds = %blk_exit211, %blk_exit177, %blk_exit177
  %_local_13.9 = phi i32 [ %210, %blk_exit211 ], [ %143, %blk_exit177 ], [ %143, %blk_exit177 ]
  %_local_19.5 = phi i32 [ %209, %blk_exit211 ], [ %_local_19.2, %blk_exit177 ], [ %_local_19.2, %blk_exit177 ]
  %_param_1.9 = phi i32 [ 120, %blk_exit211 ], [ %152, %blk_exit177 ], [ %152, %blk_exit177 ]
  %calcOffset253 = add i32 %0, -16
  %202 = sext i32 %calcOffset253 to i64
  %203 = load i64, i64* %stack_var_-16_, align 4
  %204 = and i32 %_param_1.9, 32
  %205 = call i32 @fmt_x(i64 %203, i32 %3, i32 %204)
  %206 = load i64, i64* %stack_var_-16_, align 4
  %207 = icmp eq i64 %206, 0
  br i1 %207, label %blk_exit206, label %brif_next258

blk_exit211:                                      ; preds = %blk_exit177
  %208 = icmp ugt i32 %_local_19.2, 8
  %209 = select i1 %208, i32 %_local_19.2, i32 8
  %210 = or i32 %143, 8
  br label %blk_exit210

blk_exit212:                                      ; preds = %blk_exit177
  %trunc = trunc i32 %_local_13.7 to i8
  switch i8 %trunc, label %loop_entry [
    i8 0, label %blk_exit230
    i8 1, label %blk_exit229
    i8 2, label %blk_exit228
    i8 3, label %blk_exit227
    i8 4, label %blk_exit226
    i8 7, label %blk_exit224
    i8 6, label %blk_exit225
  ]

blk_exit213:                                      ; preds = %blk_exit177
  %calcOffset222 = add i32 %0, -16
  %211 = sext i32 %calcOffset222 to i64
  %212 = load i64, i64* %stack_var_-16_, align 4
  br label %blk_exit207

blk_exit214:                                      ; preds = %blk_exit177
  switch i32 %152, label %blk_exit219 [
    i32 65, label %blk_exit200
    i32 66, label %blk_exit14
    i32 67, label %blk_exit203
    i32 68, label %blk_exit14
    i32 69, label %blk_exit200
    i32 70, label %blk_exit200
    i32 71, label %blk_exit200
  ]

blk_exit219:                                      ; preds = %blk_exit214
  %213 = icmp eq i32 %152, 83
  br i1 %213, label %blk_exit204, label %blk_exit15

blk_exit224:                                      ; preds = %blk_exit212
  %214 = load i64, i64* %stack_var_-16_, align 4
  %215 = trunc i64 %214 to i32
  %216 = sext i32 %_local_11.1 to i64
  %217 = sext i32 %215 to i64
  %218 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %217
  %219 = bitcast i8* %218 to i64*
  store i64 %216, i64* %219, align 4
  br label %loop_entry

blk_exit225:                                      ; preds = %blk_exit212
  %220 = load i64, i64* %stack_var_-16_, align 4
  %221 = trunc i64 %220 to i32
  %222 = sext i32 %221 to i64
  %223 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %222
  %224 = bitcast i8* %223 to i32*
  store i32 %_local_11.1, i32* %224, align 4
  br label %loop_entry

blk_exit226:                                      ; preds = %blk_exit212
  %225 = load i64, i64* %stack_var_-16_, align 4
  %226 = trunc i64 %225 to i32
  %227 = sext i32 %226 to i64
  %228 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %227
  %229 = trunc i32 %_local_11.1 to i8
  store i8 %229, i8* %228, align 1
  br label %loop_entry

blk_exit227:                                      ; preds = %blk_exit212
  %230 = load i64, i64* %stack_var_-16_, align 4
  %231 = trunc i64 %230 to i32
  %232 = sext i32 %231 to i64
  %233 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %232
  %234 = trunc i32 %_local_11.1 to i16
  %235 = bitcast i8* %233 to i16*
  store i16 %234, i16* %235, align 2
  br label %loop_entry

blk_exit228:                                      ; preds = %blk_exit212
  %236 = load i64, i64* %stack_var_-16_, align 4
  %237 = trunc i64 %236 to i32
  %238 = sext i32 %_local_11.1 to i64
  %239 = sext i32 %237 to i64
  %240 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %239
  %241 = bitcast i8* %240 to i64*
  store i64 %238, i64* %241, align 4
  br label %loop_entry

blk_exit229:                                      ; preds = %blk_exit212
  %242 = load i64, i64* %stack_var_-16_, align 4
  %243 = trunc i64 %242 to i32
  %244 = sext i32 %243 to i64
  %245 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %244
  %246 = bitcast i8* %245 to i32*
  store i32 %_local_11.1, i32* %246, align 4
  br label %loop_entry

blk_exit230:                                      ; preds = %blk_exit212
  %247 = load i64, i64* %stack_var_-16_, align 4
  %248 = trunc i64 %247 to i32
  %249 = sext i32 %248 to i64
  %250 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %249
  %251 = bitcast i8* %250 to i32*
  store i32 %_local_11.1, i32* %251, align 4
  br label %loop_entry

brif_next258:                                     ; preds = %blk_exit210
  %252 = and i32 %_local_13.9, 8
  %brif_val259.not.not = icmp eq i32 %252, 0
  br i1 %brif_val259.not.not, label %blk_exit206, label %brif_next260

brif_next260:                                     ; preds = %brif_next258
  %253 = lshr i32 %_param_1.9, 4
  %254 = add nuw nsw i32 %253, 1024
  br label %blk_exit206

brif_next264:                                     ; preds = %blk_exit209
  %255 = sub i32 %3, %200
  %256 = add i32 %255, 1
  %257 = icmp sgt i32 %_local_19.2, %255
  %258 = select i1 %257, i32 %_local_19.2, i32 %256
  br label %blk_exit206

blk_exit266:                                      ; preds = %blk_exit208
  %259 = and i32 %143, 2048
  %brif_val273.not.not = icmp eq i32 %259, 0
  br i1 %brif_val273.not.not, label %blk_exit272, label %brif_next274

brif_next270:                                     ; preds = %blk_exit208
  %260 = sub i64 0, %197
  store i64 %260, i64* %stack_var_-16_, align 4
  br label %blk_exit207

blk_exit272:                                      ; preds = %blk_exit266
  %261 = and i32 %143, 1
  %select_cond275.not = icmp eq i32 %261, 0
  %262 = select i1 %select_cond275.not, i32 1024, i32 1026
  br label %blk_exit207

brif_next274:                                     ; preds = %blk_exit266
  br label %blk_exit207

blk_exit277:                                      ; preds = %blk_exit206
  %263 = sub i32 %3, %_local_12.2
  %264 = icmp eq i64 %loadResult279, 0
  %265 = zext i1 %264 to i32
  %266 = add i32 %263, %265
  %267 = icmp sgt i32 %_local_19.4, %266
  %268 = select i1 %267, i32 %_local_19.4, i32 %266
  br label %blk_exit15

brif_next283:                                     ; preds = %blk_exit206
  br label %blk_exit15

blk_exit290:                                      ; preds = %blk_exit204
  call void @pad(i32 %i32, i32 32, i32 %_local_18.0, i32 0, i32 %143)
  br label %blk_exit201

brif_next292:                                     ; preds = %blk_exit204
  %269 = load i64, i64* %stack_var_-16_, align 4
  %270 = trunc i64 %269 to i32
  br label %blk_exit202

blk_exit300:                                      ; preds = %blk_exit306, %brif_next310, %loop_entry301
  %_param_1.10 = phi i32 [ %_param_1.11, %loop_entry301 ], [ %_param_1.11, %blk_exit306 ], [ %280, %brif_next310 ]
  call void @pad(i32 %i32, i32 32, i32 %_local_18.0, i32 %_param_1.10, i32 %143)
  %brif_val316.not = icmp eq i32 %_param_1.10, 0
  br i1 %brif_val316.not, label %brif_next317, label %blk_exit315

loop_entry301:                                    ; preds = %brif_next310, %blk_exit202
  %_local_14.1 = phi i32 [ %storemerge, %blk_exit202 ], [ %279, %brif_next310 ]
  %_param_1.11 = phi i32 [ 0, %blk_exit202 ], [ %280, %brif_next310 ]
  %271 = sext i32 %_local_14.1 to i64
  %272 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %271
  %273 = bitcast i8* %272 to i32*
  %loadResult303 = load i32, i32* %273, align 4
  %274 = icmp eq i32 %loadResult303, 0
  br i1 %274, label %blk_exit300, label %brif_next305

brif_next305:                                     ; preds = %loop_entry301
  %275 = add i32 %0, -76
  %276 = call i32 @wctomb(i32 %275, i32 %loadResult303)
  %brif_val307.not = icmp sgt i32 %276, -1
  br i1 %brif_val307.not, label %brif_next308, label %blk_exit306

blk_exit306:                                      ; preds = %brif_next308, %brif_next305
  %brif_val313.not = icmp sgt i32 %276, -1
  br i1 %brif_val313.not, label %blk_exit300, label %blk_exit

brif_next308:                                     ; preds = %brif_next305
  %277 = sub i32 %_local_19.3, %_param_1.11
  %278 = icmp ugt i32 %276, %277
  br i1 %278, label %blk_exit306, label %brif_next310

brif_next310:                                     ; preds = %brif_next308
  %279 = add i32 %_local_14.1, 4
  %280 = add i32 %276, %_param_1.11
  %281 = icmp ugt i32 %_local_19.3, %280
  br i1 %281, label %loop_entry301, label %blk_exit300

blk_exit315:                                      ; preds = %blk_exit300
  %282 = load i64, i64* %stack_var_-16_, align 4
  %283 = trunc i64 %282 to i32
  br label %loop_entry320

brif_next317:                                     ; preds = %blk_exit300
  br label %blk_exit201

loop_entry320:                                    ; preds = %brif_next326, %blk_exit315
  %_local_15.2 = phi i32 [ %283, %blk_exit315 ], [ %292, %brif_next326 ]
  %_local_14.2 = phi i32 [ 0, %blk_exit315 ], [ %290, %brif_next326 ]
  %284 = sext i32 %_local_15.2 to i64
  %285 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %284
  %286 = bitcast i8* %285 to i32*
  %loadResult322 = load i32, i32* %286, align 4
  %287 = icmp eq i32 %loadResult322, 0
  br i1 %287, label %blk_exit201, label %brif_next324

brif_next324:                                     ; preds = %loop_entry320
  %288 = add i32 %0, -76
  %289 = call i32 @wctomb(i32 %288, i32 %loadResult322)
  %290 = add i32 %289, %_local_14.2
  %291 = icmp sgt i32 %290, %_param_1.10
  br i1 %291, label %blk_exit201, label %brif_next326

brif_next326:                                     ; preds = %brif_next324
  call void @out(i32 %i32, i32 %288, i32 %289)
  %292 = add i32 %_local_15.2, 4
  %293 = icmp ult i32 %290, %_param_1.10
  br i1 %293, label %loop_entry320, label %blk_exit201

brif_next339:                                     ; preds = %blk_exit17
  %294 = icmp eq i32 %_local_10.0, 0
  br i1 %294, label %blk_exit13, label %brif_next341

brif_next341:                                     ; preds = %brif_next339
  br label %loop_entry343

blk_exit342:                                      ; preds = %loop_entry343
  %295 = icmp ugt i32 %_param_1.12, 9
  br i1 %295, label %blk_exit, label %loop_entry352

loop_entry343:                                    ; preds = %brif_next347, %brif_next341
  %_param_1.12 = phi i32 [ 1, %brif_next341 ], [ %304, %brif_next347 ]
  %296 = shl i32 %_param_1.12, 2
  %297 = add i32 %296, %i324
  %298 = sext i32 %297 to i64
  %299 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %298
  %300 = bitcast i8* %299 to i32*
  %loadResult345 = load i32, i32* %300, align 4
  %301 = icmp eq i32 %loadResult345, 0
  br i1 %301, label %blk_exit342, label %brif_next347

brif_next347:                                     ; preds = %loop_entry343
  %302 = shl i32 %_param_1.12, 3
  %303 = add i32 %302, %i323
  call void @pop_arg(i32 %303, i32 %loadResult345, i32 %i322, i32 %i326)
  %304 = add i32 %_param_1.12, 1
  %.not378 = icmp eq i32 %304, 10
  br i1 %.not378, label %blk_exit, label %loop_entry343

loop_entry352:                                    ; preds = %brif_next356, %blk_exit342
  %_param_1.13 = phi i32 [ %_param_1.12, %blk_exit342 ], [ %310, %brif_next356 ]
  %305 = shl i32 %_param_1.13, 2
  %306 = add i32 %305, %i324
  %307 = sext i32 %306 to i64
  %308 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %307
  %309 = bitcast i8* %308 to i32*
  %loadResult354 = load i32, i32* %309, align 4
  %brif_val355.not = icmp eq i32 %loadResult354, 0
  br i1 %brif_val355.not, label %brif_next356, label %blk_exit16

brif_next356:                                     ; preds = %loop_entry352
  %310 = add i32 %_param_1.13, 1
  %311 = icmp eq i32 %310, 10
  br i1 %311, label %blk_exit, label %loop_entry352
}

define out {
allocator:
  %0 = sext i32 %i32 to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %loadResult = load i8, i8* %1, align 1
  %2 = and i8 %loadResult, 32
  %brif_val.not = icmp eq i8 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  %3 = call i32 @__fwritex(i32 %i321, i32 %i322, i32 %i32)
  br label %blk_exit
}

define getint {
allocator:
  %0 = sext i32 %i32 to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %2 = bitcast i8* %1 to i32*
  %loadResult = load i32, i32* %2, align 4
  %3 = sext i32 %loadResult to i64
  %4 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %3
  %loadResult2 = load i8, i8* %4, align 1
  %5 = sext i8 %loadResult2 to i32
  %6 = add nsw i32 %5, -58
  %isdigit = icmp ult i32 %6, -10
  br i1 %isdigit, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %loop_entry, %allocator
  %_local_1.0 = phi i32 [ 0, %allocator ], [ %11, %loop_entry ]
  ret i32 %_local_1.0

loop_entry:                                       ; preds = %loop_entry.loop_entry_crit_edge, %allocator
  %loadResult6 = phi i8 [ %loadResult2, %allocator ], [ %loadResult9, %loop_entry.loop_entry_crit_edge ]
  %loadResult4 = phi i32 [ %loadResult, %allocator ], [ %8, %loop_entry.loop_entry_crit_edge ]
  %_local_1.1 = phi i32 [ 0, %allocator ], [ %11, %loop_entry.loop_entry_crit_edge ]
  %7 = sext i8 %loadResult6 to i32
  %8 = add i32 %loadResult4, 1
  store i32 %8, i32* %2, align 4
  %9 = mul i32 %_local_1.1, 10
  %10 = add i32 %9, %7
  %11 = add i32 %10, -48
  %12 = sext i32 %8 to i64
  %13 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %12
  %loadResult9 = load i8, i8* %13, align 1
  %14 = sext i8 %loadResult9 to i32
  %isdigittmp = add nsw i32 %14, -48
  %isdigit12 = icmp ult i32 %isdigittmp, 10
  br i1 %isdigit12, label %loop_entry.loop_entry_crit_edge, label %blk_exit

loop_entry.loop_entry_crit_edge:                  ; preds = %loop_entry
  br label %loop_entry
}

define pop_arg {
allocator:
  %0 = icmp ugt i32 %i321, 20
  br i1 %0, label %return, label %brif_next

return:                                           ; preds = %allocator, %brif_next, %blk_exit4, %blk_exit5, %blk_exit6, %blk_exit7, %blk_exit8, %blk_exit9, %blk_exit10, %blk_exit11, %blk_exit12, %blk_exit13
  ret void

brif_next:                                        ; preds = %allocator
  switch i32 %i321, label %return [
    i32 9, label %blk_exit13
    i32 10, label %blk_exit12
    i32 11, label %blk_exit11
    i32 12, label %blk_exit10
    i32 13, label %blk_exit9
    i32 14, label %blk_exit8
    i32 15, label %blk_exit7
    i32 16, label %blk_exit6
    i32 17, label %blk_exit5
    i32 18, label %blk_exit4
  ]

blk_exit4:                                        ; preds = %brif_next
  %1 = sext i32 %i323 to i64
  %2 = getelementptr [10 x void ()*], [10 x void ()*]* @T0, i64 0, i64 %1
  %3 = bitcast void ()** %2 to void (i32, i32)**
  %callind_funcptr66 = load void (i32, i32)*, void (i32, i32)** %3, align 8
  call void %callind_funcptr66(i32 %i32, i32 %i322)
  br label %return

blk_exit5:                                        ; preds = %brif_next
  %4 = sext i32 %i322 to i64
  %5 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %4
  %6 = bitcast i8* %5 to i32*
  %loadResult61 = load i32, i32* %6, align 4
  %7 = add i32 %loadResult61, 7
  %8 = and i32 %7, -8
  %9 = add i32 %8, 8
  store i32 %9, i32* %6, align 4
  %10 = sext i32 %8 to i64
  %11 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %10
  %12 = bitcast i8* %11 to double*
  %loadResult64 = load double, double* %12, align 8
  %13 = sext i32 %i32 to i64
  %14 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %13
  %15 = bitcast i8* %14 to double*
  store double %loadResult64, double* %15, align 8
  br label %return

blk_exit6:                                        ; preds = %brif_next
  %16 = sext i32 %i322 to i64
  %17 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %16
  %18 = bitcast i8* %17 to i32*
  %loadResult55 = load i32, i32* %18, align 4
  %19 = add i32 %loadResult55, 4
  store i32 %19, i32* %18, align 4
  %20 = sext i32 %loadResult55 to i64
  %21 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %20
  %loadResult58 = load i8, i8* %21, align 1
  %22 = zext i8 %loadResult58 to i64
  %23 = sext i32 %i32 to i64
  %24 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %23
  %25 = bitcast i8* %24 to i64*
  store i64 %22, i64* %25, align 4
  br label %return

blk_exit7:                                        ; preds = %brif_next
  %26 = sext i32 %i322 to i64
  %27 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %26
  %28 = bitcast i8* %27 to i32*
  %loadResult49 = load i32, i32* %28, align 4
  %29 = add i32 %loadResult49, 4
  store i32 %29, i32* %28, align 4
  %30 = sext i32 %loadResult49 to i64
  %31 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %30
  %loadResult52 = load i8, i8* %31, align 1
  %32 = sext i8 %loadResult52 to i64
  %33 = sext i32 %i32 to i64
  %34 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %33
  %35 = bitcast i8* %34 to i64*
  store i64 %32, i64* %35, align 4
  br label %return

blk_exit8:                                        ; preds = %brif_next
  %36 = sext i32 %i322 to i64
  %37 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %36
  %38 = bitcast i8* %37 to i32*
  %loadResult43 = load i32, i32* %38, align 4
  %39 = add i32 %loadResult43, 4
  store i32 %39, i32* %38, align 4
  %40 = sext i32 %loadResult43 to i64
  %41 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %40
  %42 = bitcast i8* %41 to i16*
  %loadResult46 = load i16, i16* %42, align 2
  %43 = zext i16 %loadResult46 to i64
  %44 = sext i32 %i32 to i64
  %45 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %44
  %46 = bitcast i8* %45 to i64*
  store i64 %43, i64* %46, align 4
  br label %return

blk_exit9:                                        ; preds = %brif_next
  %47 = sext i32 %i322 to i64
  %48 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %47
  %49 = bitcast i8* %48 to i32*
  %loadResult37 = load i32, i32* %49, align 4
  %50 = add i32 %loadResult37, 4
  store i32 %50, i32* %49, align 4
  %51 = sext i32 %loadResult37 to i64
  %52 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %51
  %53 = bitcast i8* %52 to i16*
  %loadResult40 = load i16, i16* %53, align 2
  %54 = sext i16 %loadResult40 to i64
  %55 = sext i32 %i32 to i64
  %56 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %55
  %57 = bitcast i8* %56 to i64*
  store i64 %54, i64* %57, align 4
  br label %return

blk_exit10:                                       ; preds = %brif_next
  %58 = sext i32 %i322 to i64
  %59 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %58
  %60 = bitcast i8* %59 to i32*
  %loadResult31 = load i32, i32* %60, align 4
  %61 = add i32 %loadResult31, 7
  %62 = and i32 %61, -8
  %63 = add i32 %62, 8
  store i32 %63, i32* %60, align 4
  %64 = sext i32 %62 to i64
  %65 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %64
  %66 = bitcast i8* %65 to i64*
  %loadResult34 = load i64, i64* %66, align 8
  %67 = sext i32 %i32 to i64
  %68 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %67
  %69 = bitcast i8* %68 to i64*
  store i64 %loadResult34, i64* %69, align 4
  br label %return

blk_exit11:                                       ; preds = %brif_next
  %70 = sext i32 %i322 to i64
  %71 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %70
  %72 = bitcast i8* %71 to i32*
  %loadResult25 = load i32, i32* %72, align 4
  %73 = add i32 %loadResult25, 4
  store i32 %73, i32* %72, align 4
  %74 = sext i32 %loadResult25 to i64
  %75 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %74
  %76 = bitcast i8* %75 to i32*
  %loadResult28 = load i32, i32* %76, align 4
  %77 = zext i32 %loadResult28 to i64
  %78 = sext i32 %i32 to i64
  %79 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %78
  %80 = bitcast i8* %79 to i64*
  store i64 %77, i64* %80, align 4
  br label %return

blk_exit12:                                       ; preds = %brif_next
  %81 = sext i32 %i322 to i64
  %82 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %81
  %83 = bitcast i8* %82 to i32*
  %loadResult19 = load i32, i32* %83, align 4
  %84 = add i32 %loadResult19, 4
  store i32 %84, i32* %83, align 4
  %85 = sext i32 %loadResult19 to i64
  %86 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %85
  %87 = bitcast i8* %86 to i32*
  %loadResult22 = load i32, i32* %87, align 4
  %88 = sext i32 %loadResult22 to i64
  %89 = sext i32 %i32 to i64
  %90 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %89
  %91 = bitcast i8* %90 to i64*
  store i64 %88, i64* %91, align 4
  br label %return

blk_exit13:                                       ; preds = %brif_next
  %92 = sext i32 %i322 to i64
  %93 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %92
  %94 = bitcast i8* %93 to i32*
  %loadResult = load i32, i32* %94, align 4
  %95 = add i32 %loadResult, 4
  store i32 %95, i32* %94, align 4
  %96 = sext i32 %loadResult to i64
  %97 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %96
  %98 = bitcast i8* %97 to i32*
  %loadResult16 = load i32, i32* %98, align 4
  %99 = sext i32 %i32 to i64
  %100 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %99
  %101 = bitcast i8* %100 to i32*
  store i32 %loadResult16, i32* %101, align 4
  br label %return
}

define fmt_x {
allocator:
  %0 = icmp eq i64 %i64, 0
  br i1 %0, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %loop_entry, %allocator
  %_param_1.0 = phi i32 [ %i32, %allocator ], [ %1, %loop_entry ]
  ret i32 %_param_1.0

loop_entry:                                       ; preds = %allocator, %loop_entry
  %_param_1.1 = phi i32 [ %i32, %allocator ], [ %1, %loop_entry ]
  %_param_0.0 = phi i64 [ %i64, %allocator ], [ %12, %loop_entry ]
  %1 = add i32 %_param_1.1, -1
  %2 = trunc i64 %_param_0.0 to i32
  %3 = and i32 %2, 15
  %4 = or i32 %3, 1600
  %5 = zext i32 %4 to i64
  %6 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %5
  %loadResult = load i8, i8* %6, align 1
  %7 = sext i32 %1 to i64
  %8 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %7
  %9 = trunc i32 %i321 to i8
  %10 = or i8 %loadResult, %9
  store i8 %10, i8* %8, align 1
  %11 = icmp ugt i64 %_param_0.0, 15
  %12 = lshr i64 %_param_0.0, 4
  br i1 %11, label %loop_entry, label %blk_exit
}

define fmt_o {
allocator:
  %0 = icmp eq i64 %i64, 0
  br i1 %0, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %loop_entry, %allocator
  %_param_1.0 = phi i32 [ %i32, %allocator ], [ %1, %loop_entry ]
  ret i32 %_param_1.0

loop_entry:                                       ; preds = %allocator, %loop_entry
  %_param_1.1 = phi i32 [ %i32, %allocator ], [ %1, %loop_entry ]
  %_param_0.0 = phi i64 [ %i64, %allocator ], [ %8, %loop_entry ]
  %1 = add i32 %_param_1.1, -1
  %2 = trunc i64 %_param_0.0 to i8
  %3 = and i8 %2, 7
  %4 = or i8 %3, 48
  %5 = sext i32 %1 to i64
  %6 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %5
  store i8 %4, i8* %6, align 1
  %7 = icmp ugt i64 %_param_0.0, 7
  %8 = lshr i64 %_param_0.0, 3
  br i1 %7, label %loop_entry, label %blk_exit
}

define fmt_u {
allocator:
  %0 = icmp ugt i64 %i64, 4294967295
  br i1 %0, label %loop_entry, label %brif_next

blk_exit:                                         ; preds = %loop_entry, %brif_next
  %_local_2.0.off0 = phi i32 [ %extract.t, %loop_entry ], [ %extract.t12, %brif_next ]
  %_param_1.0 = phi i32 [ %2, %loop_entry ], [ %i32, %brif_next ]
  %1 = icmp eq i32 %_local_2.0.off0, 0
  br i1 %1, label %blk_exit4, label %loop_entry7

brif_next:                                        ; preds = %allocator
  %extract.t12 = trunc i64 %i64 to i32
  br label %blk_exit

loop_entry:                                       ; preds = %allocator, %loop_entry
  %_param_1.1 = phi i32 [ %i32, %allocator ], [ %2, %loop_entry ]
  %_param_0.0 = phi i64 [ %i64, %allocator ], [ %3, %loop_entry ]
  %2 = add i32 %_param_1.1, -1
  %3 = udiv i64 %_param_0.0, 10
  %.neg11 = mul i64 %3, -10
  %4 = add i64 %.neg11, %_param_0.0
  %5 = trunc i64 %4 to i8
  %6 = or i8 %5, 48
  %7 = sext i32 %2 to i64
  %8 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %7
  store i8 %6, i8* %8, align 1
  %9 = icmp ugt i64 %_param_0.0, 42949672959
  %extract.t = trunc i64 %3 to i32
  br i1 %9, label %loop_entry, label %blk_exit

blk_exit4:                                        ; preds = %loop_entry7, %blk_exit
  %_param_1.2 = phi i32 [ %_param_1.0, %blk_exit ], [ %10, %loop_entry7 ]
  ret i32 %_param_1.2

loop_entry7:                                      ; preds = %blk_exit, %loop_entry7
  %_local_3.0 = phi i32 [ %_local_2.0.off0, %blk_exit ], [ %11, %loop_entry7 ]
  %_param_1.3 = phi i32 [ %_param_1.0, %blk_exit ], [ %10, %loop_entry7 ]
  %10 = add i32 %_param_1.3, -1
  %11 = udiv i32 %_local_3.0, 10
  %.neg = mul i32 %11, -10
  %12 = add i32 %.neg, %_local_3.0
  %13 = sext i32 %10 to i64
  %14 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %13
  %15 = trunc i32 %12 to i8
  %16 = or i8 %15, 48
  store i8 %16, i8* %14, align 1
  %17 = icmp ugt i32 %_local_3.0, 9
  br i1 %17, label %loop_entry7, label %blk_exit4
}

define pad {
allocator:
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-256_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -256
  %2 = ptrtoint i32* %stack_var_-256_ to i32
  %3 = and i32 %i324, 73728
  %brif_val.not = icmp eq i32 %3, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %blk_exit7, %brif_next, %allocator
  %4 = ptrtoint i32* %stack_var_0_ to i32
  ret void

brif_next:                                        ; preds = %allocator
  %.not = icmp sgt i32 %i322, %i323
  br i1 %.not, label %brif_next6, label %blk_exit

brif_next6:                                       ; preds = %brif_next
  %5 = and i32 %i321, 255
  %6 = sub i32 %i322, %i323
  %7 = icmp ult i32 %6, 256
  %8 = select i1 %7, i32 %6, i32 256
  %9 = call i32 @memset(i32 %1, i32 %5, i32 %8)
  br i1 %7, label %blk_exit7, label %loop_entry

blk_exit7:                                        ; preds = %loop_entry, %brif_next6
  %_param_2.0 = phi i32 [ %6, %brif_next6 ], [ %10, %loop_entry ]
  call void @out(i32 %i32, i32 %1, i32 %_param_2.0)
  br label %blk_exit

loop_entry:                                       ; preds = %brif_next6, %loop_entry
  %_param_2.1 = phi i32 [ %6, %brif_next6 ], [ %10, %loop_entry ]
  call void @out(i32 %i32, i32 %1, i32 256)
  %10 = add i32 %_param_2.1, -256
  %11 = icmp ugt i32 %10, 255
  br i1 %11, label %loop_entry, label %blk_exit7
}

define vfprintf {
allocator:
  %0 = call i32 @__vfprintf_internal(i32 %i32, i32 %i321, i32 %i322, i32 8, i32 9)
  ret i32 %0
}

define fmt_fp {
allocator:
  %stack_var_-536_ = alloca i8, align 1
  %stack_var_-545_ = alloca i32, align 4
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-516_ = alloca i32, align 4
  %stack_var_-560_ = alloca i32, align 4
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -560
  %2 = ptrtoint i32* %stack_var_-560_ to i32
  store i32 0, i32* %stack_var_-516_, align 4
  %3 = call i64 @__DOUBLE_BITS(double %f64)
  %brif_val.not = icmp slt i64 %3, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit5

blk_exit:                                         ; preds = %blk_exit6, %brif_next8, %brif_next
  %_local_10.0 = phi i32 [ 1034, %brif_next ], [ %9, %blk_exit6 ], [ 1037, %brif_next8 ]
  %_local_9.0 = phi i32 [ 1, %brif_next ], [ %8, %blk_exit6 ], [ 1, %brif_next8 ]
  %_local_8.0 = phi i64 [ %7, %brif_next ], [ %3, %blk_exit6 ], [ %3, %brif_next8 ]
  %_local_7.0 = phi i32 [ 0, %brif_next ], [ %10, %blk_exit6 ], [ 0, %brif_next8 ]
  %_param_1.0 = phi double [ %6, %brif_next ], [ %f64, %blk_exit6 ], [ %f64, %brif_next8 ]
  %4 = and i64 %_local_8.0, 9218868437227405312
  %.not = icmp eq i64 %4, 9218868437227405312
  br i1 %.not, label %brif_next12, label %blk_exit10

blk_exit5:                                        ; preds = %allocator
  %5 = and i32 %i323, 2048
  %brif_val7.not.not = icmp eq i32 %5, 0
  br i1 %brif_val7.not.not, label %blk_exit6, label %brif_next8

brif_next:                                        ; preds = %allocator
  %6 = fneg double %f64
  %7 = call i64 @__DOUBLE_BITS(double %6)
  br label %blk_exit

blk_exit6:                                        ; preds = %blk_exit5
  %8 = and i32 %i323, 1
  %select_cond.not = icmp eq i32 %8, 0
  %9 = select i1 %select_cond.not, i32 1035, i32 1040
  %10 = xor i32 %8, 1
  br label %blk_exit

brif_next8:                                       ; preds = %blk_exit5
  br label %blk_exit

blk_exit9:                                        ; preds = %blk_exit340, %blk_exit218, %brif_next12
  %_local_11.0 = phi i32 [ %19, %brif_next12 ], [ %393, %blk_exit340 ], [ %261, %blk_exit218 ]
  %11 = ptrtoint i32* %stack_var_0_ to i32
  %12 = icmp slt i32 %_local_11.0, %i321
  %13 = select i1 %12, i32 %i321, i32 %_local_11.0
  ret i32 %13

blk_exit10:                                       ; preds = %blk_exit
  %14 = add i32 %0, -544
  %15 = add i32 %0, -516
  %16 = call double @frexp(double %_param_1.0, i32 %15)
  %17 = fadd double %16, %16
  %18 = fcmp oeq double %17, 0.000000e+00
  br i1 %18, label %blk_exit19, label %brif_next21

brif_next12:                                      ; preds = %blk_exit
  %19 = add nuw nsw i32 %_local_9.0, 3
  %20 = and i32 %i323, -65537
  call void @pad(i32 %i32, i32 32, i32 %i321, i32 %19, i32 %20)
  call void @out(i32 %i32, i32 %_local_10.0, i32 %_local_9.0)
  %21 = and i32 %i324, 32
  %select_cond13.not = icmp eq i32 %21, 0
  %22 = select i1 %select_cond13.not, i32 1061, i32 1053
  %23 = select i1 %select_cond13.not, i32 1065, i32 1057
  %24 = fcmp ord double %_param_1.0, 0.000000e+00
  %25 = select i1 %24, i32 %22, i32 %23
  call void @out(i32 %i32, i32 %25, i32 3)
  %26 = xor i32 %i323, 8192
  call void @pad(i32 %i32, i32 32, i32 %i321, i32 %19, i32 %26)
  br label %blk_exit9

blk_exit16:                                       ; preds = %brif_next21, %blk_exit19
  %.mask = and i32 %i324, 32
  %isneg.not = icmp eq i32 %.mask, 0
  %27 = select i1 %isneg.not, i32 0, i32 9
  %28 = add nuw nsw i32 %_local_10.0, %27
  %29 = icmp ugt i32 %i322, 11
  br i1 %29, label %blk_exit295, label %brif_next297

blk_exit17:                                       ; preds = %blk_exit18, %brif_next27
  %_local_16.0 = phi i32 [ %41, %brif_next27 ], [ %31, %blk_exit18 ]
  %_local_15.0 = phi i32 [ %40, %brif_next27 ], [ %33, %blk_exit18 ]
  %_local_14.0 = phi i32 [ %35, %brif_next27 ], [ %39, %blk_exit18 ]
  %_param_1.1 = phi double [ %17, %brif_next27 ], [ %34, %blk_exit18 ]
  %select_cond33.not353 = icmp slt i32 %_local_16.0, 0
  %.v = select i1 %select_cond33.not353, i32 48, i32 336
  %30 = add i32 %1, %.v
  br label %loop_entry

blk_exit18:                                       ; preds = %brif_next21
  %31 = add i32 %37, -29
  %32 = ptrtoint i32* %stack_var_-545_ to i32
  store i32 %32, i32* %stack_var_-516_, align 4
  %select_cond32.not = icmp sgt i32 %i322, -1
  %33 = select i1 %select_cond32.not, i32 %i322, i32 6
  %34 = fmul double %17, 0x41B0000000000000
  br label %blk_exit17

blk_exit19:                                       ; preds = %blk_exit10
  %35 = or i32 %i324, 32
  %36 = icmp eq i32 %35, 97
  br i1 %36, label %blk_exit16, label %brif_next27

brif_next21:                                      ; preds = %blk_exit10
  %37 = load i32, i32* %stack_var_-516_, align 4
  %38 = add i32 %37, -1
  store i32 %38, i32* %stack_var_-516_, align 4
  %39 = or i32 %i324, 32
  %.not347 = icmp eq i32 %39, 97
  br i1 %.not347, label %blk_exit16, label %blk_exit18

brif_next27:                                      ; preds = %blk_exit19
  %select_cond28.not = icmp sgt i32 %i322, -1
  %40 = select i1 %select_cond28.not, i32 %i322, i32 6
  %41 = load i32, i32* %stack_var_-516_, align 4
  br label %blk_exit17

loop_entry:                                       ; preds = %blk_exit34, %blk_exit17
  %_local_18.0 = phi i32 [ %30, %blk_exit17 ], [ %48, %blk_exit34 ]
  %_param_1.2 = phi double [ %_param_1.1, %blk_exit17 ], [ %51, %blk_exit34 ]
  %42 = fcmp uge double %_param_1.2, 0x41F0000000000000
  %43 = fcmp ult double %_param_1.2, 0.000000e+00
  %44 = or i1 %42, %43
  br i1 %44, label %blk_exit35, label %brif_next37

blk_exit34:                                       ; preds = %blk_exit35, %brif_next37
  %storemerge354 = phi i32 [ %53, %brif_next37 ], [ 0, %blk_exit35 ]
  %45 = sext i32 %_local_18.0 to i64
  %46 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %45
  %47 = bitcast i8* %46 to i32*
  store i32 %storemerge354, i32* %47, align 4
  %48 = add i32 %_local_18.0, 4
  %49 = uitofp i32 %storemerge354 to double
  %50 = fsub double %_param_1.2, %49
  %51 = fmul double %50, 1.000000e+09
  %52 = fcmp oeq double %51, 0.000000e+00
  br i1 %52, label %loop_entry, label %brif_next40

blk_exit35:                                       ; preds = %loop_entry
  br label %blk_exit34

brif_next37:                                      ; preds = %loop_entry
  %53 = fptoui double %_param_1.2 to i32
  br label %blk_exit34

brif_next40:                                      ; preds = %blk_exit34
  %54 = icmp sgt i32 %_local_16.0, 0
  br i1 %54, label %blk_exit42, label %brif_next44

blk_exit41:                                       ; preds = %blk_exit59, %brif_next44
  %_local_19.0 = phi i32 [ %_local_19.2, %blk_exit59 ], [ %30, %brif_next44 ]
  %_local_12.0 = phi i32 [ %_local_18.2, %blk_exit59 ], [ %48, %brif_next44 ]
  %_param_3.0 = phi i32 [ %78, %blk_exit59 ], [ %_local_16.0, %brif_next44 ]
  %brif_val73.not = icmp slt i32 %_param_3.0, 0
  br i1 %brif_val73.not, label %brif_next74, label %blk_exit72

blk_exit42:                                       ; preds = %brif_next40
  br label %loop_entry45

brif_next44:                                      ; preds = %brif_next40
  br label %blk_exit41

loop_entry45:                                     ; preds = %blk_exit59, %blk_exit42
  %_local_19.1 = phi i32 [ %30, %blk_exit42 ], [ %_local_19.2, %blk_exit59 ]
  %_local_18.1 = phi i32 [ %48, %blk_exit42 ], [ %_local_18.2, %blk_exit59 ]
  %_param_3.1 = phi i32 [ %_local_16.0, %blk_exit42 ], [ %78, %blk_exit59 ]
  %55 = icmp slt i32 %_param_3.1, 29
  %56 = select i1 %55, i32 %_param_3.1, i32 29
  %57 = add i32 %_local_18.1, -4
  %58 = icmp ult i32 %57, %_local_19.1
  br i1 %58, label %blk_exit47, label %brif_next49

blk_exit47:                                       ; preds = %brif_next57, %brif_next55, %loop_entry45
  %_local_19.2 = phi i32 [ %_local_19.1, %loop_entry45 ], [ %_local_19.1, %brif_next55 ], [ %73, %brif_next57 ]
  br label %loop_entry60

brif_next49:                                      ; preds = %loop_entry45
  %59 = zext i32 %56 to i64
  br label %loop_entry50

loop_entry50:                                     ; preds = %loop_entry50, %brif_next49
  %_local_12.1 = phi i32 [ %57, %brif_next49 ], [ %70, %loop_entry50 ]
  %_local_8.1 = phi i64 [ 0, %brif_next49 ], [ %67, %loop_entry50 ]
  %60 = sext i32 %_local_12.1 to i64
  %61 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %60
  %62 = bitcast i8* %61 to i32*
  %loadResult52 = load i32, i32* %62, align 4
  %63 = zext i32 %loadResult52 to i64
  %64 = shl i64 %63, %59
  %65 = and i64 %_local_8.1, 4294967295
  %66 = add i64 %64, %65
  %67 = udiv i64 %66, 1000000000
  %.neg377 = mul i64 %67, -1000000000
  %68 = add i64 %.neg377, %66
  %69 = trunc i64 %68 to i32
  store i32 %69, i32* %62, align 4
  %70 = add i32 %_local_12.1, -4
  %.not378 = icmp ult i32 %70, %_local_19.1
  br i1 %.not378, label %brif_next55, label %loop_entry50

brif_next55:                                      ; preds = %loop_entry50
  %71 = trunc i64 %67 to i32
  %72 = icmp eq i32 %71, 0
  br i1 %72, label %blk_exit47, label %brif_next57

brif_next57:                                      ; preds = %brif_next55
  %73 = add i32 %_local_19.1, -4
  %74 = sext i32 %73 to i64
  %75 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %74
  %76 = bitcast i8* %75 to i32*
  store i32 %71, i32* %76, align 4
  br label %blk_exit47

blk_exit59:                                       ; preds = %brif_next62, %loop_entry60
  %77 = load i32, i32* %stack_var_-516_, align 4
  %78 = sub i32 %77, %56
  store i32 %78, i32* %stack_var_-516_, align 4
  %79 = icmp sgt i32 %78, 0
  br i1 %79, label %loop_entry45, label %blk_exit41

loop_entry60:                                     ; preds = %brif_next62, %blk_exit47
  %_local_18.2 = phi i32 [ %_local_18.1, %blk_exit47 ], [ %80, %brif_next62 ]
  %.not379 = icmp ugt i32 %_local_18.2, %_local_19.2
  br i1 %.not379, label %brif_next62, label %blk_exit59

brif_next62:                                      ; preds = %loop_entry60
  %80 = add i32 %_local_18.2, -4
  %81 = sext i32 %80 to i64
  %82 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %81
  %83 = bitcast i8* %82 to i32*
  %loadResult64 = load i32, i32* %83, align 4
  %84 = icmp eq i32 %loadResult64, 0
  br i1 %84, label %loop_entry60, label %blk_exit59

blk_exit72:                                       ; preds = %blk_exit77, %blk_exit41
  %_local_22.0 = phi i32 [ %89, %blk_exit77 ], [ 0, %blk_exit41 ]
  %_local_19.3 = phi i32 [ %97, %blk_exit77 ], [ %_local_19.0, %blk_exit41 ]
  %_local_12.2 = phi i32 [ %104, %blk_exit77 ], [ %_local_12.0, %blk_exit41 ]
  %.not356 = icmp ult i32 %_local_19.3, %_local_12.2
  br i1 %.not356, label %brif_next103, label %blk_exit101

brif_next74:                                      ; preds = %blk_exit41
  %85 = add i32 %_local_15.0, 25
  %86 = sdiv i32 %85, 9
  %87 = add nsw i32 %86, 1
  %88 = icmp eq i32 %_local_14.0, 102
  %89 = zext i1 %88 to i32
  br label %loop_entry75

loop_entry75:                                     ; preds = %blk_exit77, %brif_next74
  %_local_19.4 = phi i32 [ %_local_19.0, %brif_next74 ], [ %97, %blk_exit77 ]
  %_local_12.3 = phi i32 [ %_local_12.0, %brif_next74 ], [ %104, %blk_exit77 ]
  %_param_3.2 = phi i32 [ %_param_3.0, %brif_next74 ], [ %94, %blk_exit77 ]
  %90 = sub i32 0, %_param_3.2
  %91 = icmp slt i32 %90, 9
  %92 = select i1 %91, i32 %90, i32 9
  %.not355 = icmp ult i32 %_local_19.4, %_local_12.3
  br i1 %.not355, label %brif_next80, label %blk_exit78

blk_exit77:                                       ; preds = %blk_exit78, %brif_next90, %brif_next86
  %_local_18.3 = phi i32 [ %loadResult88, %brif_next86 ], [ %loadResult88, %brif_next90 ], [ %loadResult93, %blk_exit78 ]
  %_local_12.4 = phi i32 [ %_local_12.3, %brif_next86 ], [ %127, %brif_next90 ], [ %_local_12.3, %blk_exit78 ]
  %93 = load i32, i32* %stack_var_-516_, align 4
  %94 = add i32 %93, %92
  store i32 %94, i32* %stack_var_-516_, align 4
  %95 = icmp eq i32 %_local_18.3, 0
  %96 = select i1 %95, i32 4, i32 0
  %97 = add i32 %_local_19.4, %96
  %98 = select i1 %88, i32 %30, i32 %97
  %99 = shl nsw i32 %87, 2
  %100 = add i32 %98, %99
  %101 = sub i32 %_local_12.4, %98
  %102 = ashr i32 %101, 2
  %103 = icmp sgt i32 %102, %87
  %104 = select i1 %103, i32 %100, i32 %_local_12.4
  %brif_val99.not = icmp sgt i32 %94, -1
  br i1 %brif_val99.not, label %blk_exit72, label %loop_entry75

blk_exit78:                                       ; preds = %loop_entry75
  %105 = sext i32 %_local_19.4 to i64
  %106 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %105
  %107 = bitcast i8* %106 to i32*
  %loadResult93 = load i32, i32* %107, align 4
  br label %blk_exit77

brif_next80:                                      ; preds = %loop_entry75
  %108 = lshr i32 1000000000, %92
  %109 = shl i32 -1, %92
  %110 = xor i32 %109, -1
  br label %loop_entry81

loop_entry81:                                     ; preds = %loop_entry81, %brif_next80
  %_local_18.4 = phi i32 [ %_local_19.4, %brif_next80 ], [ %118, %loop_entry81 ]
  %_param_3.3 = phi i32 [ 0, %brif_next80 ], [ %117, %loop_entry81 ]
  %111 = sext i32 %_local_18.4 to i64
  %112 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %111
  %113 = bitcast i8* %112 to i32*
  %loadResult83 = load i32, i32* %113, align 4
  %114 = lshr i32 %loadResult83, %92
  %115 = add i32 %114, %_param_3.3
  store i32 %115, i32* %113, align 4
  %116 = and i32 %loadResult83, %110
  %117 = mul i32 %116, %108
  %118 = add i32 %_local_18.4, 4
  %119 = icmp ult i32 %118, %_local_12.3
  br i1 %119, label %loop_entry81, label %brif_next86

brif_next86:                                      ; preds = %loop_entry81
  %120 = sext i32 %_local_19.4 to i64
  %121 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %120
  %122 = bitcast i8* %121 to i32*
  %loadResult88 = load i32, i32* %122, align 4
  %123 = icmp eq i32 %117, 0
  br i1 %123, label %blk_exit77, label %brif_next90

brif_next90:                                      ; preds = %brif_next86
  %124 = sext i32 %_local_12.3 to i64
  %125 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %124
  %126 = bitcast i8* %125 to i32*
  store i32 %117, i32* %126, align 4
  %127 = add i32 %_local_12.3, 4
  br label %blk_exit77

blk_exit101:                                      ; preds = %loop_entry108, %brif_next103, %blk_exit72
  %_local_18.5 = phi i32 [ %141, %brif_next103 ], [ %146, %loop_entry108 ], [ 0, %blk_exit72 ]
  %128 = icmp eq i32 %_local_14.0, 102
  %129 = select i1 %128, i32 0, i32 %_local_18.5
  %130 = sub i32 %_local_15.0, %129
  %131 = icmp eq i32 %_local_14.0, 103
  %132 = icmp ne i32 %_local_15.0, 0
  %133 = and i1 %131, %132
  %.neg = sext i1 %133 to i32
  %134 = add i32 %130, %.neg
  %135 = sub i32 %_local_12.2, %30
  %136 = ashr i32 %135, 2
  %137 = mul i32 %136, 9
  %138 = add i32 %137, -9
  %.not358 = icmp slt i32 %134, %138
  br i1 %.not358, label %brif_next114, label %blk_exit111

brif_next103:                                     ; preds = %blk_exit72
  %139 = sub i32 %30, %_local_19.3
  %140 = ashr i32 %139, 2
  %141 = mul i32 %140, 9
  %142 = sext i32 %_local_19.3 to i64
  %143 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %142
  %144 = bitcast i8* %143 to i32*
  %loadResult105 = load i32, i32* %144, align 4
  %145 = icmp ult i32 %loadResult105, 10
  br i1 %145, label %blk_exit101, label %loop_entry108

loop_entry108:                                    ; preds = %brif_next103, %loop_entry108
  %_local_18.6 = phi i32 [ %141, %brif_next103 ], [ %146, %loop_entry108 ]
  %_param_3.4 = phi i32 [ 10, %brif_next103 ], [ %147, %loop_entry108 ]
  %146 = add i32 %_local_18.6, 1
  %147 = mul i32 %_param_3.4, 10
  %.not357 = icmp ult i32 %loadResult105, %147
  br i1 %.not357, label %blk_exit101, label %loop_entry108

blk_exit111:                                      ; preds = %blk_exit122, %blk_exit101
  %_local_19.5 = phi i32 [ %_local_19.6, %blk_exit122 ], [ %_local_19.3, %blk_exit101 ]
  %_local_18.7 = phi i32 [ %_local_18.8, %blk_exit122 ], [ %_local_18.5, %blk_exit101 ]
  %_local_12.5 = phi i32 [ %169, %blk_exit122 ], [ %_local_12.2, %blk_exit101 ]
  br label %loop_entry168

brif_next114:                                     ; preds = %blk_exit101
  %148 = add i32 %134, 9216
  %149 = sdiv i32 %148, 9
  %150 = shl nsw i32 %149, 2
  %151 = add i32 %0, -512
  %select_cond115.not = icmp sgt i32 %_local_16.0, -1
  %152 = select i1 %select_cond115.not, i32 292, i32 4
  %153 = add i32 %151, %152
  %154 = add i32 %150, %153
  %155 = add i32 %154, -4096
  %.neg359 = mul i32 %149, -9
  %156 = add i32 %.neg359, %148
  %157 = icmp sgt i32 %156, 7
  br i1 %157, label %blk_exit116, label %loop_entry119

blk_exit116:                                      ; preds = %loop_entry119, %brif_next114
  %_param_3.5 = phi i32 [ 10, %brif_next114 ], [ %165, %loop_entry119 ]
  %158 = add i32 %154, -4092
  %159 = sext i32 %155 to i64
  %160 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %159
  %161 = bitcast i8* %160 to i32*
  %loadResult125 = load i32, i32* %161, align 4
  %162 = udiv i32 %loadResult125, %_param_3.5
  %163 = mul i32 %162, %_param_3.5
  %164 = sub i32 %loadResult125, %163
  %brif_val126.not = icmp eq i32 %164, 0
  br i1 %brif_val126.not, label %brif_next127, label %blk_exit123

loop_entry119:                                    ; preds = %brif_next114, %loop_entry119
  %_param_3.6 = phi i32 [ 10, %brif_next114 ], [ %165, %loop_entry119 ]
  %_local_25.0 = phi i32 [ %156, %brif_next114 ], [ %166, %loop_entry119 ]
  %165 = mul i32 %_param_3.6, 10
  %166 = add i32 %_local_25.0, 1
  %.not360 = icmp eq i32 %166, 8
  br i1 %.not360, label %blk_exit116, label %loop_entry119

blk_exit122:                                      ; preds = %loop_entry163, %blk_exit145, %blk_exit134, %brif_next127
  %_local_19.6 = phi i32 [ %_local_19.3, %brif_next127 ], [ %_local_19.3, %blk_exit134 ], [ %_local_19.7, %blk_exit145 ], [ %_local_19.7, %loop_entry163 ]
  %_local_18.8 = phi i32 [ %_local_18.5, %brif_next127 ], [ %_local_18.5, %blk_exit134 ], [ %190, %blk_exit145 ], [ %208, %loop_entry163 ]
  %_local_11.1 = phi i32 [ %155, %brif_next127 ], [ %155, %blk_exit134 ], [ %_local_11.2, %blk_exit145 ], [ %_local_11.2, %loop_entry163 ]
  %167 = add i32 %_local_11.1, 4
  %168 = icmp ugt i32 %_local_12.2, %167
  %169 = select i1 %168, i32 %167, i32 %_local_12.2
  br label %blk_exit111

blk_exit123:                                      ; preds = %brif_next127, %blk_exit116
  %170 = icmp eq i32 %158, %_local_12.2
  %171 = lshr exact i32 %_param_3.5, 1
  %172 = icmp eq i32 %164, %171
  %173 = select i1 %172, i1 %170, i1 false
  %174 = select i1 %173, double 1.000000e+00, double 1.500000e+00
  %175 = icmp ult i32 %164, %171
  %176 = select i1 %175, double 5.000000e-01, double %174
  %177 = and i32 %162, 1
  %select_cond133.not = icmp eq i32 %177, 0
  %178 = select i1 %select_cond133.not, double 0x4340000000000000, double 0x4340000000000001
  %brif_val135.not = icmp eq i32 %_local_7.0, 0
  br i1 %brif_val135.not, label %brif_next136, label %blk_exit134

brif_next127:                                     ; preds = %blk_exit116
  %179 = icmp eq i32 %158, %_local_12.2
  br i1 %179, label %blk_exit122, label %blk_exit123

blk_exit134:                                      ; preds = %brif_next140, %brif_next136, %blk_exit123
  %_param_1.3 = phi double [ %185, %brif_next140 ], [ %178, %brif_next136 ], [ %178, %blk_exit123 ]
  %_local_26.0 = phi double [ %184, %brif_next140 ], [ %176, %brif_next136 ], [ %176, %blk_exit123 ]
  store i32 %163, i32* %161, align 4
  %180 = fadd double %_param_1.3, %_local_26.0
  %181 = fcmp oeq double %180, %_param_1.3
  br i1 %181, label %blk_exit122, label %brif_next143

brif_next136:                                     ; preds = %blk_exit123
  %182 = zext i32 %_local_10.0 to i64
  %183 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %182
  %loadResult138 = load i8, i8* %183, align 1
  %.not361 = icmp eq i8 %loadResult138, 45
  br i1 %.not361, label %brif_next140, label %blk_exit134

brif_next140:                                     ; preds = %brif_next136
  %184 = fneg double %176
  %185 = fneg double %178
  br label %blk_exit134

brif_next143:                                     ; preds = %blk_exit134
  %186 = add i32 %163, %_param_3.5
  store i32 %186, i32* %161, align 4
  %187 = icmp ult i32 %186, 1000000000
  br i1 %187, label %blk_exit145, label %loop_entry148

blk_exit145:                                      ; preds = %blk_exit150, %brif_next143
  %_local_19.7 = phi i32 [ %_local_19.3, %brif_next143 ], [ %_local_19.9, %blk_exit150 ]
  %_local_11.2 = phi i32 [ %155, %brif_next143 ], [ %198, %blk_exit150 ]
  %188 = sub i32 %30, %_local_19.7
  %189 = ashr i32 %188, 2
  %190 = mul i32 %189, 9
  %191 = sext i32 %_local_19.7 to i64
  %192 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %191
  %193 = bitcast i8* %192 to i32*
  %loadResult160 = load i32, i32* %193, align 4
  %194 = icmp ult i32 %loadResult160, 10
  br i1 %194, label %blk_exit122, label %loop_entry163

loop_entry148:                                    ; preds = %brif_next143, %blk_exit150
  %_local_19.8 = phi i32 [ %_local_19.3, %brif_next143 ], [ %_local_19.9, %blk_exit150 ]
  %_local_11.3 = phi i32 [ %155, %brif_next143 ], [ %198, %blk_exit150 ]
  %195 = sext i32 %_local_11.3 to i64
  %196 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %195
  %197 = bitcast i8* %196 to i32*
  store i32 0, i32* %197, align 4
  %198 = add i32 %_local_11.3, -4
  %.not362 = icmp ult i32 %198, %_local_19.8
  br i1 %.not362, label %brif_next152, label %blk_exit150

blk_exit150:                                      ; preds = %brif_next152, %loop_entry148
  %_local_19.9 = phi i32 [ %204, %brif_next152 ], [ %_local_19.8, %loop_entry148 ]
  %199 = sext i32 %198 to i64
  %200 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %199
  %201 = bitcast i8* %200 to i32*
  %loadResult155 = load i32, i32* %201, align 4
  %202 = add i32 %loadResult155, 1
  store i32 %202, i32* %201, align 4
  %203 = icmp ugt i32 %202, 999999999
  br i1 %203, label %loop_entry148, label %blk_exit145

brif_next152:                                     ; preds = %loop_entry148
  %204 = add i32 %_local_19.8, -4
  %205 = sext i32 %204 to i64
  %206 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %205
  %207 = bitcast i8* %206 to i32*
  store i32 0, i32* %207, align 4
  br label %blk_exit150

loop_entry163:                                    ; preds = %blk_exit145, %loop_entry163
  %_local_18.9 = phi i32 [ %190, %blk_exit145 ], [ %208, %loop_entry163 ]
  %_param_3.7 = phi i32 [ 10, %blk_exit145 ], [ %209, %loop_entry163 ]
  %208 = add i32 %_local_18.9, 1
  %209 = mul i32 %_param_3.7, 10
  %.not363 = icmp ult i32 %loadResult160, %209
  br i1 %.not363, label %blk_exit122, label %loop_entry163

blk_exit167:                                      ; preds = %brif_next170, %loop_entry168
  br i1 %131, label %blk_exit176, label %brif_next178

loop_entry168:                                    ; preds = %brif_next170, %blk_exit111
  %_local_12.6 = phi i32 [ %_local_12.5, %blk_exit111 ], [ %210, %brif_next170 ]
  %.not381 = icmp ugt i32 %_local_12.6, %_local_19.5
  br i1 %.not381, label %brif_next170, label %blk_exit167

brif_next170:                                     ; preds = %loop_entry168
  %210 = add i32 %_local_12.6, -4
  %211 = sext i32 %210 to i64
  %212 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %211
  %213 = bitcast i8* %212 to i32*
  %loadResult172 = load i32, i32* %213, align 4
  %214 = icmp eq i32 %loadResult172, 0
  br i1 %214, label %loop_entry168, label %blk_exit167

blk_exit175:                                      ; preds = %blk_exit196, %brif_next198, %blk_exit176, %brif_next178
  %_local_15.1 = phi i32 [ %257, %brif_next198 ], [ %251, %blk_exit196 ], [ %225, %blk_exit176 ], [ %_local_15.0, %brif_next178 ]
  %_param_5.0 = phi i32 [ %227, %brif_next198 ], [ %227, %blk_exit196 ], [ %227, %blk_exit176 ], [ %i324, %brif_next178 ]
  %_local_24.0 = phi i32 [ 0, %brif_next198 ], [ 0, %blk_exit196 ], [ %228, %blk_exit176 ], [ %229, %brif_next178 ]
  %215 = or i32 %_local_15.1, %_local_24.0
  %216 = icmp ne i32 %215, 0
  %217 = zext i1 %216 to i32
  %218 = and i32 %_param_5.0, -33
  %.not364 = icmp eq i32 %218, 70
  br i1 %.not364, label %brif_next206, label %blk_exit204

blk_exit176:                                      ; preds = %blk_exit167
  %219 = xor i32 %_local_18.7, -1
  %select_cond179.not = icmp eq i32 %_local_15.0, 0
  %220 = select i1 %select_cond179.not, i32 1, i32 %_local_15.0
  %221 = icmp sgt i32 %220, %_local_18.7
  %222 = icmp sgt i32 %_local_18.7, -5
  %223 = and i1 %221, %222
  %224 = select i1 %223, i32 %219, i32 -1
  %225 = add i32 %224, %220
  %226 = select i1 %223, i32 -1, i32 -2
  %227 = add i32 %226, %i324
  %228 = and i32 %i323, 8
  %brif_val182.not = icmp eq i32 %228, 0
  br i1 %brif_val182.not, label %brif_next183, label %blk_exit175

brif_next178:                                     ; preds = %blk_exit167
  %229 = and i32 %i323, 8
  br label %blk_exit175

brif_next183:                                     ; preds = %blk_exit176
  br i1 %.not381, label %brif_next186, label %blk_exit184

blk_exit184:                                      ; preds = %brif_next195, %brif_next190, %brif_next186, %brif_next183
  %_local_12.7 = phi i32 [ -9, %brif_next186 ], [ %244, %brif_next195 ], [ 0, %brif_next190 ], [ -9, %brif_next183 ]
  %230 = sub i32 %_local_12.6, %30
  %231 = ashr i32 %230, 2
  %232 = mul i32 %231, 9
  %233 = and i32 %227, -33
  %.not376 = icmp eq i32 %233, 70
  br i1 %.not376, label %brif_next198, label %blk_exit196

brif_next186:                                     ; preds = %brif_next183
  %234 = add i32 %_local_12.6, -4
  %235 = sext i32 %234 to i64
  %236 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %235
  %237 = bitcast i8* %236 to i32*
  %loadResult188 = load i32, i32* %237, align 4
  %238 = icmp eq i32 %loadResult188, 0
  br i1 %238, label %blk_exit184, label %brif_next190

brif_next190:                                     ; preds = %brif_next186
  %239 = urem i32 %loadResult188, 10
  %brif_val191.not = icmp eq i32 %239, 0
  br i1 %brif_val191.not, label %loop_entry193, label %blk_exit184

loop_entry193:                                    ; preds = %brif_next190, %loop_entry193
  %_local_12.8 = phi i32 [ 0, %brif_next190 ], [ %240, %loop_entry193 ]
  %_param_3.8 = phi i32 [ 10, %brif_next190 ], [ %241, %loop_entry193 ]
  %240 = add i32 %_local_12.8, 1
  %241 = mul i32 %_param_3.8, 10
  %242 = urem i32 %loadResult188, %241
  %243 = icmp eq i32 %242, 0
  br i1 %243, label %loop_entry193, label %brif_next195

brif_next195:                                     ; preds = %loop_entry193
  %244 = xor i32 %_local_12.8, -1
  br label %blk_exit184

blk_exit196:                                      ; preds = %blk_exit184
  %245 = add i32 %_local_18.7, %232
  %246 = add i32 %245, %_local_12.7
  %247 = add i32 %246, -9
  %248 = icmp sgt i32 %247, 0
  %249 = select i1 %248, i32 %247, i32 0
  %250 = icmp slt i32 %225, %249
  %251 = select i1 %250, i32 %225, i32 %249
  br label %blk_exit175

brif_next198:                                     ; preds = %blk_exit184
  %252 = add i32 %232, %_local_12.7
  %253 = add i32 %252, -9
  %254 = icmp sgt i32 %253, 0
  %255 = select i1 %254, i32 %253, i32 0
  %256 = icmp slt i32 %225, %255
  %257 = select i1 %256, i32 %225, i32 %255
  br label %blk_exit175

blk_exit203:                                      ; preds = %blk_exit208, %brif_next206
  %_local_22.1 = phi i32 [ %_local_22.0, %brif_next206 ], [ %270, %blk_exit208 ]
  %storemerge365 = phi i32 [ %269, %brif_next206 ], [ %278, %blk_exit208 ]
  %258 = add i32 %_local_9.0, %_local_15.1
  %259 = add i32 %258, %217
  %260 = add i32 %259, %storemerge365
  %261 = add i32 %260, 1
  call void @pad(i32 %i32, i32 32, i32 %i321, i32 %261, i32 %i323)
  call void @out(i32 %i32, i32 %_local_10.0, i32 %_local_9.0)
  %262 = xor i32 %i323, 65536
  call void @pad(i32 %i32, i32 48, i32 %i321, i32 %261, i32 %262)
  br i1 %.not364, label %brif_next223, label %blk_exit221

blk_exit204:                                      ; preds = %blk_exit175
  %263 = call i32 @llvm.abs.i32(i32 %_local_18.7, i1 false)
  %264 = zext i32 %263 to i64
  %265 = call i32 @fmt_u(i64 %264, i32 %14)
  %266 = sub i32 %14, %265
  %267 = icmp sgt i32 %266, 1
  br i1 %267, label %blk_exit208, label %loop_entry211

brif_next206:                                     ; preds = %blk_exit175
  %268 = icmp sgt i32 %_local_18.7, 0
  %269 = select i1 %268, i32 %_local_18.7, i32 0
  br label %blk_exit203

blk_exit208:                                      ; preds = %loop_entry211, %blk_exit204
  %_local_12.9 = phi i32 [ %265, %blk_exit204 ], [ %279, %loop_entry211 ]
  %270 = add i32 %_local_12.9, -2
  %271 = sext i32 %270 to i64
  %272 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %271
  %273 = trunc i32 %_param_5.0 to i8
  store i8 %273, i8* %272, align 1
  %274 = add i32 %_local_12.9, -1
  %select_cond216.not = icmp sgt i32 %_local_18.7, -1
  %275 = sext i32 %274 to i64
  %276 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %275
  %277 = select i1 %select_cond216.not, i8 43, i8 45
  store i8 %277, i8* %276, align 1
  %278 = sub i32 %14, %270
  br label %blk_exit203

loop_entry211:                                    ; preds = %blk_exit204, %loop_entry211
  %_local_12.10 = phi i32 [ %265, %blk_exit204 ], [ %279, %loop_entry211 ]
  %279 = add i32 %_local_12.10, -1
  %280 = sext i32 %279 to i64
  %281 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %280
  store i8 48, i8* %281, align 1
  %282 = sub i32 %14, %279
  %283 = icmp slt i32 %282, 2
  br i1 %283, label %loop_entry211, label %blk_exit208

blk_exit218:                                      ; preds = %blk_exit219, %blk_exit264
  %284 = xor i32 %i323, 8192
  call void @pad(i32 %i32, i32 32, i32 %i321, i32 %261, i32 %284)
  br label %blk_exit9

blk_exit219:                                      ; preds = %brif_next261, %blk_exit220, %blk_exit250, %brif_next242
  %_local_12.11 = phi i32 [ 0, %brif_next242 ], [ %_local_15.1, %blk_exit220 ], [ %310, %brif_next261 ], [ %310, %blk_exit250 ]
  %285 = add i32 %_local_12.11, 9
  call void @pad(i32 %i32, i32 48, i32 %285, i32 9, i32 0)
  br label %blk_exit218

blk_exit220:                                      ; preds = %brif_next246, %brif_next244
  br label %blk_exit219

blk_exit221:                                      ; preds = %blk_exit203
  %brif_val265.not = icmp sgt i32 %_local_15.1, -1
  br i1 %brif_val265.not, label %brif_next266, label %blk_exit264

brif_next223:                                     ; preds = %blk_exit203
  %286 = or i32 %14, 8
  %287 = or i32 %14, 9
  %288 = icmp ugt i32 %_local_19.5, %30
  %289 = select i1 %288, i32 %30, i32 %_local_19.5
  br label %loop_entry225

loop_entry225:                                    ; preds = %blk_exit228, %brif_next223
  %_local_18.10 = phi i32 [ %289, %brif_next223 ], [ %297, %blk_exit228 ]
  %290 = sext i32 %_local_18.10 to i64
  %291 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %290
  %292 = bitcast i8* %291 to i32*
  %loadResult227 = load i32, i32* %292, align 4
  %293 = zext i32 %loadResult227 to i64
  %294 = call i32 @fmt_u(i64 %293, i32 %287)
  %295 = icmp eq i32 %_local_18.10, %289
  br i1 %295, label %blk_exit229, label %brif_next231

blk_exit228:                                      ; preds = %loop_entry234, %brif_next239, %blk_exit229, %brif_next231
  %_local_12.12 = phi i32 [ %286, %brif_next239 ], [ %294, %blk_exit229 ], [ %298, %loop_entry234 ], [ %294, %brif_next231 ]
  %296 = sub i32 %287, %_local_12.12
  call void @out(i32 %i32, i32 %_local_12.12, i32 %296)
  %297 = add i32 %_local_18.10, 4
  %.not368 = icmp ugt i32 %297, %30
  br i1 %.not368, label %brif_next242, label %loop_entry225

blk_exit229:                                      ; preds = %loop_entry225
  %.not372 = icmp eq i32 %294, %287
  br i1 %.not372, label %brif_next239, label %blk_exit228

brif_next231:                                     ; preds = %loop_entry225
  %.not367 = icmp ugt i32 %294, %14
  br i1 %.not367, label %loop_entry234, label %blk_exit228

loop_entry234:                                    ; preds = %brif_next231, %loop_entry234
  %_local_12.13 = phi i32 [ %294, %brif_next231 ], [ %298, %loop_entry234 ]
  %298 = add i32 %_local_12.13, -1
  %299 = sext i32 %298 to i64
  %300 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %299
  store i8 48, i8* %300, align 1
  %301 = icmp ugt i32 %298, %14
  br i1 %301, label %loop_entry234, label %blk_exit228

brif_next239:                                     ; preds = %blk_exit229
  store i8 48, i8* %stack_var_-536_, align 1
  br label %blk_exit228

brif_next242:                                     ; preds = %blk_exit228
  br i1 %216, label %brif_next244, label %blk_exit219

brif_next244:                                     ; preds = %brif_next242
  call void @out(i32 %i32, i32 1069, i32 1)
  %.not369 = icmp ult i32 %297, %_local_12.6
  br i1 %.not369, label %brif_next246, label %blk_exit220

brif_next246:                                     ; preds = %brif_next244
  %302 = icmp slt i32 %_local_15.1, 1
  br i1 %302, label %blk_exit220, label %loop_entry249

loop_entry249:                                    ; preds = %brif_next246, %brif_next261
  %_local_18.11 = phi i32 [ %297, %brif_next246 ], [ %311, %brif_next261 ]
  %_local_15.2 = phi i32 [ %_local_15.1, %brif_next246 ], [ %310, %brif_next261 ]
  %303 = sext i32 %_local_18.11 to i64
  %304 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %303
  %305 = bitcast i8* %304 to i32*
  %loadResult252 = load i32, i32* %305, align 4
  %306 = zext i32 %loadResult252 to i64
  %307 = call i32 @fmt_u(i64 %306, i32 %287)
  %.not370 = icmp ugt i32 %307, %14
  br i1 %.not370, label %loop_entry255, label %blk_exit250

blk_exit250:                                      ; preds = %loop_entry255, %loop_entry249
  %_local_12.14 = phi i32 [ %312, %loop_entry255 ], [ %307, %loop_entry249 ]
  %308 = icmp slt i32 %_local_15.2, 9
  %309 = select i1 %308, i32 %_local_15.2, i32 9
  call void @out(i32 %i32, i32 %_local_12.14, i32 %309)
  %310 = add i32 %_local_15.2, -9
  %311 = add i32 %_local_18.11, 4
  %.not371 = icmp ult i32 %311, %_local_12.6
  br i1 %.not371, label %brif_next261, label %blk_exit219

loop_entry255:                                    ; preds = %loop_entry249, %loop_entry255
  %_local_12.15 = phi i32 [ %307, %loop_entry249 ], [ %312, %loop_entry255 ]
  %312 = add i32 %_local_12.15, -1
  %313 = sext i32 %312 to i64
  %314 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %313
  store i8 48, i8* %314, align 1
  %315 = icmp ugt i32 %312, %14
  br i1 %315, label %loop_entry255, label %blk_exit250

brif_next261:                                     ; preds = %blk_exit250
  %316 = icmp sgt i32 %_local_15.2, 9
  br i1 %316, label %loop_entry249, label %blk_exit219

blk_exit264:                                      ; preds = %brif_next292, %blk_exit275, %blk_exit221
  %_local_15.3 = phi i32 [ %333, %brif_next292 ], [ %333, %blk_exit275 ], [ %_local_15.1, %blk_exit221 ]
  %317 = add i32 %_local_15.3, 18
  call void @pad(i32 %i32, i32 48, i32 %317, i32 18, i32 0)
  %318 = sub i32 %14, %_local_22.1
  call void @out(i32 %i32, i32 %_local_22.1, i32 %318)
  br label %blk_exit218

brif_next266:                                     ; preds = %blk_exit221
  %319 = add i32 %_local_19.5, 4
  %320 = icmp ugt i32 %_local_12.6, %_local_19.5
  %321 = select i1 %320, i32 %_local_12.6, i32 %319
  %322 = or i32 %14, 9
  %323 = or i32 %14, 8
  br label %loop_entry268

loop_entry268:                                    ; preds = %brif_next292, %brif_next266
  %_local_18.12 = phi i32 [ %_local_19.5, %brif_next266 ], [ %334, %brif_next292 ]
  %_local_15.4 = phi i32 [ %_local_15.1, %brif_next266 ], [ %333, %brif_next292 ]
  %324 = sext i32 %_local_18.12 to i64
  %325 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %324
  %326 = bitcast i8* %325 to i32*
  %loadResult271 = load i32, i32* %326, align 4
  %327 = zext i32 %loadResult271 to i64
  %328 = call i32 @fmt_u(i64 %327, i32 %322)
  %.not373 = icmp eq i32 %328, %322
  br i1 %.not373, label %brif_next273, label %blk_exit269

blk_exit269:                                      ; preds = %brif_next273, %loop_entry268
  %_local_12.16 = phi i32 [ %323, %brif_next273 ], [ %328, %loop_entry268 ]
  %329 = icmp eq i32 %_local_18.12, %_local_19.5
  br i1 %329, label %blk_exit276, label %brif_next278

brif_next273:                                     ; preds = %loop_entry268
  store i8 48, i8* %stack_var_-536_, align 1
  br label %blk_exit269

blk_exit275:                                      ; preds = %loop_entry281, %blk_exit285, %brif_next287, %brif_next278
  %_local_12.17 = phi i32 [ %335, %blk_exit285 ], [ %335, %brif_next287 ], [ %337, %loop_entry281 ], [ %_local_12.16, %brif_next278 ]
  %330 = sub i32 %322, %_local_12.17
  %331 = icmp sgt i32 %_local_15.4, %330
  %332 = select i1 %331, i32 %330, i32 %_local_15.4
  call void @out(i32 %i32, i32 %_local_12.17, i32 %332)
  %333 = sub i32 %_local_15.4, %330
  %334 = add i32 %_local_18.12, 4
  %.not375 = icmp ult i32 %334, %321
  br i1 %.not375, label %brif_next292, label %blk_exit264

blk_exit276:                                      ; preds = %blk_exit269
  call void @out(i32 %i32, i32 %_local_12.16, i32 1)
  %335 = add i32 %_local_12.16, 1
  %336 = icmp sgt i32 %_local_15.4, 0
  br i1 %336, label %blk_exit285, label %brif_next287

brif_next278:                                     ; preds = %blk_exit269
  %.not374 = icmp ugt i32 %_local_12.16, %14
  br i1 %.not374, label %loop_entry281, label %blk_exit275

loop_entry281:                                    ; preds = %brif_next278, %loop_entry281
  %_local_12.18 = phi i32 [ %_local_12.16, %brif_next278 ], [ %337, %loop_entry281 ]
  %337 = add i32 %_local_12.18, -1
  %338 = sext i32 %337 to i64
  %339 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %338
  store i8 48, i8* %339, align 1
  %340 = icmp ugt i32 %337, %14
  br i1 %340, label %loop_entry281, label %blk_exit275

blk_exit285:                                      ; preds = %brif_next287, %blk_exit276
  call void @out(i32 %i32, i32 1069, i32 1)
  br label %blk_exit275

brif_next287:                                     ; preds = %blk_exit276
  %341 = icmp eq i32 %_local_24.0, 0
  br i1 %341, label %blk_exit275, label %blk_exit285

brif_next292:                                     ; preds = %blk_exit275
  %brif_val293.not = icmp slt i32 %333, 0
  br i1 %brif_val293.not, label %blk_exit264, label %loop_entry268

blk_exit295:                                      ; preds = %blk_exit303, %brif_next307, %brif_next297, %blk_exit16
  %_param_1.4 = phi double [ %17, %blk_exit16 ], [ %17, %brif_next297 ], [ %356, %brif_next307 ], [ %352, %blk_exit303 ]
  %342 = load i32, i32* %stack_var_-516_, align 4
  %343 = call i32 @llvm.abs.i32(i32 %342, i1 false)
  %344 = zext i32 %343 to i64
  %345 = call i32 @fmt_u(i64 %344, i32 %14)
  %.not349 = icmp eq i32 %345, %14
  br i1 %.not349, label %brif_next312, label %blk_exit308

brif_next297:                                     ; preds = %blk_exit16
  br i1 false, label %blk_exit295, label %brif_next299

brif_next299:                                     ; preds = %brif_next297
  %346 = sub i32 12, %i322
  br label %loop_entry300

loop_entry300:                                    ; preds = %loop_entry300, %brif_next299
  %_local_12.19 = phi i32 [ %346, %brif_next299 ], [ %348, %loop_entry300 ]
  %_local_26.1 = phi double [ 8.000000e+00, %brif_next299 ], [ %347, %loop_entry300 ]
  %347 = fmul double %_local_26.1, 1.600000e+01
  %348 = add i32 %_local_12.19, -1
  %brif_val301.not = icmp eq i32 %348, 0
  br i1 %brif_val301.not, label %brif_next302, label %loop_entry300

brif_next302:                                     ; preds = %loop_entry300
  %349 = zext i32 %28 to i64
  %350 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %349
  %loadResult305 = load i8, i8* %350, align 1
  %.not348 = icmp eq i8 %loadResult305, 45
  br i1 %.not348, label %brif_next307, label %blk_exit303

blk_exit303:                                      ; preds = %brif_next302
  %351 = fadd double %17, %347
  %352 = fsub double %351, %347
  br label %blk_exit295

brif_next307:                                     ; preds = %brif_next302
  %353 = fneg double %17
  %354 = fsub double %353, %347
  %355 = fadd double %347, %354
  %356 = fneg double %355
  br label %blk_exit295

blk_exit308:                                      ; preds = %brif_next312, %blk_exit295
  %_local_12.20 = phi i32 [ %calcOffset313, %brif_next312 ], [ %345, %blk_exit295 ]
  %357 = or i32 %_local_9.0, 2
  %358 = load i32, i32* %stack_var_-516_, align 4
  %359 = add i32 %_local_12.20, -2
  %360 = sext i32 %359 to i64
  %361 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %360
  %362 = trunc i32 %i324 to i8
  %363 = add i8 %362, 15
  store i8 %363, i8* %361, align 1
  %364 = add i32 %_local_12.20, -1
  %select_cond317.not = icmp sgt i32 %358, -1
  %365 = sext i32 %364 to i64
  %366 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %365
  %367 = select i1 %select_cond317.not, i8 43, i8 45
  store i8 %367, i8* %366, align 1
  %368 = and i32 %i323, 8
  br label %loop_entry319

brif_next312:                                     ; preds = %blk_exit295
  %calcOffset313 = add i32 %0, -545
  %369 = sext i8 48 to i32
  store i32 %369, i32* %stack_var_-545_, align 4
  br label %blk_exit308

loop_entry319:                                    ; preds = %blk_exit327, %blk_exit308
  %_local_18.13 = phi i32 [ %14, %blk_exit308 ], [ %_local_18.14, %blk_exit327 ]
  %_param_1.5 = phi double [ %_param_1.4, %blk_exit308 ], [ %381, %blk_exit327 ]
  %370 = call double @llvm.fabs.f64(double %_param_1.5)
  %371 = fcmp uge double %370, 0x41E0000000000000
  br i1 %371, label %blk_exit321, label %brif_next323

blk_exit320:                                      ; preds = %blk_exit321, %brif_next323
  %storemerge = phi i32 [ %384, %brif_next323 ], [ -2147483648, %blk_exit321 ]
  %372 = add i32 %storemerge, 1600
  %373 = sext i32 %372 to i64
  %374 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %373
  %loadResult325 = load i8, i8* %374, align 1
  %375 = sext i32 %_local_18.13 to i64
  %376 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %375
  %377 = trunc i32 %.mask to i8
  %378 = or i8 %loadResult325, %377
  store i8 %378, i8* %376, align 1
  %379 = sitofp i32 %storemerge to double
  %380 = fsub double %_param_1.5, %379
  %381 = fmul double %380, 1.600000e+01
  %382 = add i32 %_local_18.13, 1
  %383 = sub i32 %382, %14
  %.not350 = icmp eq i32 %383, 1
  br i1 %.not350, label %brif_next329, label %blk_exit327

blk_exit321:                                      ; preds = %loop_entry319
  br label %blk_exit320

brif_next323:                                     ; preds = %loop_entry319
  %384 = fptosi double %_param_1.5 to i32
  br label %blk_exit320

blk_exit327:                                      ; preds = %blk_exit330, %brif_next334, %blk_exit320
  %_local_18.14 = phi i32 [ %389, %blk_exit330 ], [ %382, %brif_next334 ], [ %382, %blk_exit320 ]
  %385 = fcmp oeq double %381, 0.000000e+00
  br i1 %385, label %loop_entry319, label %brif_next339

brif_next329:                                     ; preds = %blk_exit320
  %386 = fcmp oeq double %381, 0.000000e+00
  br i1 %386, label %blk_exit330, label %brif_next332

blk_exit330:                                      ; preds = %brif_next334, %brif_next332, %brif_next329
  %387 = sext i32 %382 to i64
  %388 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %387
  store i8 46, i8* %388, align 1
  %389 = add i32 %_local_18.13, 2
  br label %blk_exit327

brif_next332:                                     ; preds = %brif_next329
  %390 = icmp sgt i32 %i322, 0
  br i1 %390, label %blk_exit330, label %brif_next334

brif_next334:                                     ; preds = %brif_next332
  %391 = icmp eq i32 %368, 0
  br i1 %391, label %blk_exit327, label %blk_exit330

brif_next339:                                     ; preds = %blk_exit327
  %392 = icmp eq i32 %i322, 0
  br i1 %392, label %brif_next339.blk_exit341_crit_edge, label %brif_next343

brif_next339.blk_exit341_crit_edge:               ; preds = %brif_next339
  %.pre380 = sub i32 %_local_18.14, %14
  br label %blk_exit341

blk_exit340:                                      ; preds = %blk_exit341, %brif_next345
  %.pre-phi = phi i32 [ %.pre.pre-phi, %blk_exit341 ], [ %401, %brif_next345 ]
  %storemerge352 = phi i32 [ %400, %blk_exit341 ], [ %405, %brif_next345 ]
  %393 = add i32 %storemerge352, %357
  call void @pad(i32 %i32, i32 32, i32 %i321, i32 %393, i32 %i323)
  call void @out(i32 %i32, i32 %28, i32 %357)
  %394 = xor i32 %i323, 65536
  call void @pad(i32 %i32, i32 48, i32 %i321, i32 %393, i32 %394)
  call void @out(i32 %i32, i32 %14, i32 %.pre-phi)
  %395 = sub i32 %14, %359
  %396 = add i32 %.pre-phi, %395
  %397 = sub i32 %storemerge352, %396
  call void @pad(i32 %i32, i32 48, i32 %397, i32 0, i32 0)
  call void @out(i32 %i32, i32 %359, i32 %395)
  %398 = xor i32 %i323, 8192
  call void @pad(i32 %i32, i32 32, i32 %i321, i32 %393, i32 %398)
  br label %blk_exit9

blk_exit341:                                      ; preds = %brif_next339.blk_exit341_crit_edge, %brif_next343
  %.pre.pre-phi = phi i32 [ %.pre380, %brif_next339.blk_exit341_crit_edge ], [ %401, %brif_next343 ]
  %399 = sub i32 2, %_local_12.20
  %400 = add i32 %399, %_local_18.14
  br label %blk_exit340

brif_next343:                                     ; preds = %brif_next339
  %401 = sub i32 %_local_18.14, %14
  %402 = add i32 %401, -2
  %.not351 = icmp slt i32 %402, %i322
  br i1 %.not351, label %brif_next345, label %blk_exit341

brif_next345:                                     ; preds = %brif_next343
  %403 = add i32 %14, %i322
  %404 = sub i32 %403, %359
  %405 = add i32 %404, 2
  br label %blk_exit340
}

define pop_arg_long_double {
allocator:
  %0 = sext i32 %i321 to i64
  %1 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %0
  %2 = bitcast i8* %1 to i32*
  %loadResult = load i32, i32* %2, align 4
  %3 = add i32 %loadResult, 7
  %4 = and i32 %3, -8
  %5 = add i32 %4, 16
  store i32 %5, i32* %2, align 4
  %6 = sext i32 %4 to i64
  %7 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %6
  %8 = bitcast i8* %7 to i64*
  %loadResult4 = load i64, i64* %8, align 8
  %9 = add i32 %4, 8
  %10 = sext i32 %9 to i64
  %11 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %10
  %12 = bitcast i8* %11 to i64*
  %loadResult6 = load i64, i64* %12, align 8
  %13 = call double @__trunctfdf2(i64 %loadResult4, i64 %loadResult6)
  %14 = sext i32 %i32 to i64
  %15 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %14
  %16 = bitcast i8* %15 to double*
  store double %13, double* %16, align 8
  ret void
}

define __DOUBLE_BITS {
allocator:
  %.cast = bitcast double %f64 to i64
  ret i64 %.cast
}

define printf {
allocator:
  %stack_var_0_ = alloca i32, align 4
  %stack_var_-4_ = alloca i32, align 4
  %stack_var_-16_ = alloca i32, align 4
  %0 = ptrtoint i32* %stack_var_-16_ to i32
  store i32 %i321, i32* %stack_var_-4_, align 4
  %loadResult = load i32, i32* bitcast (i8* getelementptr inbounds ([16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 1124) to i32*), align 4
  %1 = call i32 @vfprintf(i32 %loadResult, i32 %i32, i32 %i321)
  %2 = ptrtoint i32* %stack_var_0_ to i32
  ret i32 %1
}

define __lockfile {
allocator:
  ret i32 1
}

define __unlockfile {
allocator:
  ret void
}

define stackSave {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  ret i32 %0
}

define stackRestore {
allocator:
  ret void
}

define stackAlloc {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = sub i32 %0, %i32
  %2 = and i32 %1, -16
  ret i32 %2
}

declare  llvm.fabs.f64 {}

declare  llvm.fshl.i64 {}

declare  llvm.abs.i32 {}

