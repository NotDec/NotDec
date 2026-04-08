; ModuleID = '../result/fortune.o3.wasm.ll'
source_filename = "bin/fortune.o3.wasm"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

@__stack_pointer = internal global i32 68912
@__notdec_mem0 = global [131072 x i8] zeroinitializer
@__notdec_mem0_0x400 = internal constant [1183 x i8] c"acefhilm:n:osuvw\00fortune: bad juju in is_existant\00%s.dat\00get_pos\00/usr/local/share/games/fortunes\00/usr/local/local/share/games/fortunes\00ins,pas\00%*s\00%s/%s/%s\00%s version %s\00r\00p\00o\00ins.ftn\00/dev/urandom\00sml\00all\00fp->read_tbl\00i\00h\00/usr/local/share/games/fortunes/off\00/usr/local/local/share/games/fortunes/off\00fortune-mod\00ins.c\00/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c\00rb\00fortune [-a\00sw]\00 [-m pattern]\00FORTUNE_MOD_RAND_HARD_CODED_VALS\00LC_MESSAGES\00NULL\00LC_ALL\00LANG\00LANGUAGE\00PWD\00FORTUNE_MOD_USE_SRAND\00%s.u8\003.24.0\00../\00%5.2f%%\00Out of RAM!\00fortune:%s not a fortune file or directory\0A\00percentages must be integers\0A\00fortune: no probability left to put in residual files\0A\00percentages must precede files\0A\00bad pattern: %s\0A\00pattern too long for ignoring case\0A\00No fortunes found\0A\00fortune: no fortune found\0A\00fortune: %s corrupted\0A\00(%s)\0A%c\0A\00 [-n number] [ [#%] file/directory/all]\0A\00percentages must be <= 100\0A\00fortune: %s: No fortune files in directory.\0A\00fortune: out of memory.\0A\00fortune: no place to put residual probability (%d%%)\0A\00(%s)\0A%%\0A\00pattern too long for ignoring case; overflow!\0A\00fortune: probabilities sum to %d%%!\0A\00Overflow percentage detected at argument \22%s\22!\0A\00", section ".addr_0x400", align 1
@__notdec_mem0_0x8a0 = internal global [4 x i8] c"\A0\00\00\00", section ".addr_0x8a0", align 1
@table_0 = global [2 x void ()*] [void ()* null, void ()* bitcast (i32 (i32, i32)* @names_compare to void ()*)]

declare i32 @getenv(i32)

declare i32 @getopt(i32, i32, i32)

declare i32 @atoi(i32)

declare i32 @printf(i32, i32)

declare void @exit(i32)

declare i32 @fprintf(i32, i32, i32)

declare i32 @strlen(i32)

declare i32 @strcmp(i32, i32)

declare i32 @snprintf(i32, i32, i32, i32)

declare i32 @strncmp(i32, i32, i32)

declare i32 @strncpy(i32, i32, i32)

declare i32 @strchr(i32, i32)

declare i32 @malloc(i32)

declare i32 @toupper(i32)

declare i32 @tolower(i32)

declare i32 @regcomp(i32, i32, i32)

declare void @free(i32)

declare i32 @setlocale(i32, i32)

declare void @regfree(i32)

declare i64 @time(i32)

declare i32 @getpid()

declare void @srandom(i32)

declare i32 @atol(i32)

declare i32 @fopen(i32, i32)

declare i32 @fread(i32, i32, i32, i32)

declare i32 @fclose(i32)

declare i32 @random()

declare i32 @open(i32, i32, i32)

declare i64 @lseek(i32, i64, i32)

declare i32 @read(i32, i32, i32)

declare i32 @ntohl(i32)

declare i32 @fdopen(i32, i32)

declare i32 @fseek(i32, i32, i32)

declare i32 @fgets(i32, i32, i32)

declare i32 @strdup(i32)

declare i32 @fputs(i32, i32)

declare i32 @fflush(i32)

declare i32 @sleep(i32)

declare void @__assert_fail(i32, i32, i32, i32)

declare void @perror(i32)

declare i32 @putc(i32, i32)

declare i32 @stat(i32, i32)

declare i32 @close(i32)

declare i32 @opendir(i32)

declare i32 @readdir(i32)

declare i32 @realloc(i32, i32)

declare i32 @closedir(i32)

declare void @qsort(i32, i32, i32, i32)

declare i32 @strrchr(i32, i32)

declare i32 @access(i32, i32)

declare i32 @regexec(i32, i32, i32, i32, i32)

; Function Attrs: null_pointer_is_valid
declare dso_local i32 @main(i32, i32) #0

; Function Attrs: null_pointer_is_valid
define internal void @program_version() #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -16
  store i32 %1, i32* @__stack_pointer, align 4
  %calcOffset = add i32 %0, -12
  %2 = inttoptr i32 %calcOffset to i32*
  store i32 1550, i32* %2, align 4
  %3 = inttoptr i32 %1 to i32*
  store i32 1324, i32* %3, align 4
  %4 = call i32 @snprintf(i32 2304, i32 1024, i32 1180, i32 %1)
  store i32 %0, i32* @__stack_pointer, align 4
  ret void
}

; Function Attrs: null_pointer_is_valid
define internal void @usage() #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -144
  store i32 %1, i32* @__stack_pointer, align 4
  call void @program_version()
  %calcOffset = add i32 %0, -16
  %2 = inttoptr i32 %calcOffset to i32*
  store i32 2304, i32* %2, align 4
  %3 = load i32, i32* null, align 2147483648
  %4 = add i32 %0, -16
  %5 = call i32 @fprintf(i32 %3, i32 1755, i32 %4)
  %calcOffset1 = add i32 %0, -32
  %6 = inttoptr i32 %calcOffset1 to i32*
  store i32 1417, i32* %6, align 4
  %7 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset1)
  %calcOffset2 = add i32 %0, -48
  %8 = inttoptr i32 %calcOffset2 to i32*
  store i32 1322, i32* %8, align 4
  %9 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset2)
  %calcOffset3 = add i32 %0, -64
  %10 = inttoptr i32 %calcOffset3 to i32*
  store i32 1242, i32* %10, align 4
  %11 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset3)
  %calcOffset4 = add i32 %0, -80
  %12 = inttoptr i32 %calcOffset4 to i32*
  store i32 1240, i32* %12, align 4
  %13 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset4)
  %calcOffset5 = add i32 %0, -96
  %14 = inttoptr i32 %calcOffset5 to i32*
  store i32 1198, i32* %14, align 4
  %15 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset5)
  %calcOffset6 = add i32 %0, -112
  %16 = inttoptr i32 %calcOffset6 to i32*
  store i32 1429, i32* %16, align 4
  %17 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset6)
  %calcOffset7 = add i32 %0, -128
  %18 = inttoptr i32 %calcOffset7 to i32*
  store i32 1433, i32* %18, align 4
  %19 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset7)
  %20 = inttoptr i32 %1 to i32*
  store i32 1873, i32* %20, align 4
  %21 = call i32 @fprintf(i32 %3, i32 1191, i32 %1)
  call void @exit(i32 1)
  unreachable
}

; Function Attrs: null_pointer_is_valid
define internal i32 @add_file(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3, i32 %_arg_4, i32 %_arg_5) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -1696
  store i32 %1, i32* @__stack_pointer, align 4
  %brif_val.not = icmp eq i32 %_arg_2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

blk_exit:                                         ; preds = %blk_exit145
  %calcOffset221 = add i32 %0, -1616
  %2 = inttoptr i32 %calcOffset221 to i32*
  store i32 1987, i32* %2, align 4
  %3 = load i32, i32* null, align 2147483648
  %4 = call i32 @fprintf(i32 %3, i32 1191, i32 %calcOffset221)
  call void @exit(i32 1)
  unreachable

blk_exit1:                                        ; preds = %brif_next115
  call void @perror(i32 1569)
  call void @exit(i32 -1)
  unreachable

blk_exit2:                                        ; preds = %blk_exit97
  call void @perror(i32 1569)
  call void @exit(i32 -1)
  unreachable

blk_exit3:                                        ; preds = %brif_next61
  %calcOffset220 = add i32 %0, -1648
  %5 = inttoptr i32 %calcOffset220 to i32*
  store i32 1987, i32* %5, align 4
  %6 = load i32, i32* null, align 2147483648
  %7 = call i32 @fprintf(i32 %6, i32 1191, i32 %calcOffset220)
  call void @exit(i32 1)
  unreachable

blk_exit4:                                        ; preds = %blk_exit24
  %calcOffset219 = add i32 %0, -1664
  %8 = inttoptr i32 %calcOffset219 to i32*
  store i32 1987, i32* %8, align 4
  %9 = load i32, i32* null, align 2147483648
  %10 = call i32 @fprintf(i32 %9, i32 1191, i32 %calcOffset219)
  call void @exit(i32 1)
  unreachable

blk_exit5:                                        ; preds = %blk_exit7
  %11 = inttoptr i32 %1 to i32*
  store i32 1987, i32* %11, align 4
  %12 = load i32, i32* null, align 2147483648
  %13 = call i32 @fprintf(i32 %12, i32 1191, i32 %1)
  call void @exit(i32 1)
  unreachable

blk_exit6:                                        ; preds = %brif_next9, %brif_next
  %_local_7.0 = phi i32 [ %22, %brif_next ], [ %20, %brif_next9 ]
  %14 = inttoptr i32 %_local_7.0 to i8*
  %15 = load i8, i8* %14, align 1
  %.not = icmp eq i8 %15, 47
  br i1 %.not, label %brif_next15, label %blk_exit6.blk_exit12_crit_edge

blk_exit6.blk_exit12_crit_edge:                   ; preds = %blk_exit6
  %.pre = add i32 %0, -1536
  br label %blk_exit12

blk_exit7:                                        ; preds = %allocator
  %16 = call i32 @strlen(i32 %_arg_2)
  %17 = call i32 @strlen(i32 %_arg_1)
  %18 = add i32 %16, %17
  %19 = add i32 %18, 13
  %20 = call i32 @malloc(i32 %19)
  %21 = icmp eq i32 %20, 0
  br i1 %21, label %blk_exit5, label %brif_next9

brif_next:                                        ; preds = %allocator
  %22 = call i32 @strdup(i32 %_arg_1)
  br label %blk_exit6

brif_next9:                                       ; preds = %blk_exit7
  %calcOffset = add i32 %0, -1548
  %23 = inttoptr i32 %calcOffset to i32*
  store i32 %_arg_1, i32* %23, align 4
  %calcOffset10 = add i32 %0, -1552
  %24 = inttoptr i32 %calcOffset10 to i32*
  store i32 %_arg_2, i32* %24, align 4
  %25 = add i32 %18, 12
  %26 = call i32 @snprintf(i32 %20, i32 %25, i32 1174, i32 %calcOffset10)
  br label %blk_exit6

blk_exit11:                                       ; preds = %blk_exit200, %blk_exit195, %blk_exit187, %blk_exit179, %blk_exit87, %blk_exit53, %brif_next52, %blk_exit44, %blk_exit40, %blk_exit35, %blk_exit25, %brif_next17, %brif_next17, %blk_exit12
  %_local_8.0 = phi i32 [ 0, %blk_exit187 ], [ 1, %blk_exit195 ], [ 1, %blk_exit200 ], [ 0, %blk_exit35 ], [ 0, %blk_exit25 ], [ 0, %blk_exit12 ], [ 0, %brif_next17 ], [ 0, %brif_next17 ], [ 0, %blk_exit40 ], [ 0, %blk_exit53 ], [ 0, %blk_exit44 ], [ 0, %blk_exit179 ], [ 1, %blk_exit87 ], [ 0, %brif_next52 ]
  %_local_7.1 = phi i32 [ %43, %blk_exit187 ], [ %43, %blk_exit195 ], [ %_local_7.0, %blk_exit200 ], [ %_local_7.0, %blk_exit35 ], [ %_local_7.0, %blk_exit25 ], [ %_local_7.0, %blk_exit12 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %blk_exit40 ], [ %_local_7.0, %blk_exit53 ], [ %_local_7.0, %blk_exit44 ], [ %43, %blk_exit179 ], [ %43, %blk_exit87 ], [ %_local_7.0, %brif_next52 ]
  call void @free(i32 %_local_7.1)
  store i32 %0, i32* @__stack_pointer, align 4
  ret i32 %_local_8.0

blk_exit12:                                       ; preds = %brif_next15, %blk_exit6.blk_exit12_crit_edge
  %.pre-phi230 = phi i32 [ %.pre, %blk_exit6.blk_exit12_crit_edge ], [ %38, %brif_next15 ]
  %27 = icmp ne i32 %_arg_5, 0
  %28 = zext i1 %27 to i32
  %29 = call i32 @stat(i32 %_local_7.0, i32 %.pre-phi230)
  %30 = xor i32 %29, -1
  %.lobit.not = lshr i32 %30, 31
  %calcOffset19 = add i32 %0, -1512
  %31 = inttoptr i32 %calcOffset19 to i32*
  %32 = load i32, i32* %31, align 4
  %33 = and i32 %32, 57344
  %34 = icmp eq i32 %33, 16384
  %35 = zext i1 %34 to i32
  %36 = and i32 %.lobit.not, %35
  %37 = and i32 %36, %28
  %brif_val20.not = icmp eq i32 %37, 0
  %brif_val22.not = icmp sgt i32 %29, -1
  %or.cond = select i1 %brif_val20.not, i1 %brif_val22.not, i1 false
  br i1 %or.cond, label %brif_next23, label %blk_exit11

brif_next15:                                      ; preds = %blk_exit6
  %38 = add i32 %0, -1536
  %39 = call i32 @stat(i32 %_local_7.0, i32 %38)
  %40 = icmp eq i32 %39, 0
  br i1 %40, label %blk_exit12, label %brif_next17

brif_next17:                                      ; preds = %brif_next15
  %41 = load i32, i32* null, align 2147483648
  switch i32 %41, label %blk_exit18 [
    i32 44, label %blk_exit11
    i32 54, label %blk_exit11
  ]

blk_exit18:                                       ; preds = %brif_next17
  call void @perror(i32 1041)
  call void @exit(i32 1)
  unreachable

brif_next23:                                      ; preds = %blk_exit12
  %42 = call i32 @open(i32 %_local_7.0, i32 67108864, i32 0)
  %brif_val26.not = icmp sgt i32 %42, -1
  br i1 %brif_val26.not, label %brif_next27, label %blk_exit25

blk_exit24:                                       ; preds = %brif_next27
  %43 = call i32 @malloc(i32 84)
  %44 = icmp eq i32 %43, 0
  br i1 %44, label %blk_exit4, label %brif_next61

blk_exit25:                                       ; preds = %brif_next27, %brif_next23
  %brif_val33.not = icmp eq i32 %_arg_5, 0
  %or.cond228 = select i1 %brif_val.not, i1 %brif_val33.not, i1 false
  br i1 %or.cond228, label %brif_next34, label %blk_exit11

brif_next27:                                      ; preds = %brif_next23
  %45 = load i8, i8* %14, align 1
  %46 = icmp eq i8 %45, 47
  br i1 %46, label %blk_exit24, label %blk_exit25

brif_next34:                                      ; preds = %blk_exit25
  %47 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %48 = icmp eq i32 %47, 0
  br i1 %48, label %blk_exit35, label %brif_next37

blk_exit35:                                       ; preds = %brif_next34
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

brif_next37:                                      ; preds = %brif_next34
  %49 = add i32 %0, -512
  %50 = call i32 @strncpy(i32 %49, i32 %47, i32 512)
  %calcOffset38 = add i32 %0, -1
  %51 = inttoptr i32 %calcOffset38 to i8*
  store i8 0, i8* %51, align 1
  br label %loop_entry

blk_exit40:                                       ; preds = %blk_exit53, %loop_entry
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

loop_entry:                                       ; preds = %brif_next55, %brif_next37
  %_param_2.0 = phi i32 [ %49, %brif_next37 ], [ %storemerge, %brif_next55 ]
  %52 = inttoptr i32 %_param_2.0 to i8*
  %53 = load i8, i8* %52, align 1
  %54 = icmp eq i8 %53, 0
  br i1 %54, label %blk_exit40, label %brif_next43

brif_next43:                                      ; preds = %loop_entry
  %55 = call i32 @strchr(i32 %_param_2.0, i32 58)
  %brif_val46.not = icmp eq i32 %55, 0
  br i1 %brif_val46.not, label %blk_exit44, label %blk_exit45

blk_exit44:                                       ; preds = %blk_exit45, %brif_next43
  %storemerge = phi i32 [ %62, %blk_exit45 ], [ 0, %brif_next43 ]
  %calcOffset49 = add i32 %0, -1676
  %56 = inttoptr i32 %calcOffset49 to i32*
  store i32 %_param_2.0, i32* %56, align 4
  %calcOffset50 = add i32 %0, -1680
  %57 = inttoptr i32 %calcOffset50 to i32*
  store i32 1089, i32* %57, align 4
  %58 = call i32 @snprintf(i32 %.pre-phi230, i32 1024, i32 1174, i32 %calcOffset50)
  %59 = call i32 @strncmp(i32 %_local_7.0, i32 %_param_2.0, i32 2)
  %60 = icmp eq i32 %59, 0
  br i1 %60, label %blk_exit11, label %brif_next52

blk_exit45:                                       ; preds = %brif_next43
  %61 = inttoptr i32 %55 to i8*
  store i8 0, i8* %61, align 1
  %62 = add i32 %55, 1
  br label %blk_exit44

brif_next52:                                      ; preds = %blk_exit44
  %63 = call i32 @strlen(i32 1089)
  %64 = add i32 %63, 3
  %65 = call i32 @strncmp(i32 %_local_7.0, i32 %.pre-phi230, i32 %64)
  %66 = icmp eq i32 %65, 0
  br i1 %66, label %blk_exit11, label %brif_next55

blk_exit53:                                       ; preds = %brif_next55
  br i1 %66, label %blk_exit11, label %blk_exit40

brif_next55:                                      ; preds = %brif_next52
  %brif_val56.not = icmp eq i32 %storemerge, 0
  br i1 %brif_val56.not, label %blk_exit53, label %loop_entry

brif_next61:                                      ; preds = %blk_exit24
  %calcOffset62 = add i32 %43, 56
  %67 = inttoptr i32 %calcOffset62 to i64*
  store i64 0, i64* %67, align 8
  %calcOffset63 = add i32 %43, 48
  %68 = inttoptr i32 %calcOffset63 to i64*
  store i64 0, i64* %68, align 8
  %calcOffset64 = add i32 %43, 40
  %69 = inttoptr i32 %calcOffset64 to i64*
  store i64 0, i64* %69, align 8
  %calcOffset65 = add i32 %43, 36
  %70 = inttoptr i32 %calcOffset65 to i8*
  store i8 0, i8* %70, align 1
  %calcOffset66 = add i32 %43, 16
  %71 = inttoptr i32 %calcOffset66 to i32*
  store i32 0, i32* %71, align 4
  %calcOffset67 = add i32 %43, 8
  %72 = inttoptr i32 %calcOffset67 to i64*
  store i64 -1, i64* %72, align 8
  %calcOffset68 = add i32 %43, 76
  %73 = inttoptr i32 %calcOffset68 to i64*
  store i64 0, i64* %73, align 8
  %calcOffset69 = add i32 %43, 20
  %74 = inttoptr i32 %calcOffset69 to i64*
  store i64 0, i64* %74, align 8
  %calcOffset70 = add i32 %43, 68
  %75 = inttoptr i32 %calcOffset70 to i64*
  store i64 0, i64* %75, align 8
  %calcOffset71 = add i32 %43, 28
  %76 = inttoptr i32 %calcOffset71 to i64*
  store i64 0, i64* %76, align 8
  %calcOffset72 = add i32 %43, 4
  %77 = inttoptr i32 %calcOffset72 to i32*
  store i32 %42, i32* %77, align 4
  %78 = inttoptr i32 %43 to i32*
  store i32 %_arg_0, i32* %78, align 4
  %79 = call i32 @strdup(i32 %_arg_1)
  %80 = inttoptr i32 %calcOffset69 to i32*
  store i32 %79, i32* %80, align 4
  %81 = call i32 @strdup(i32 %_local_7.0)
  %calcOffset75 = add i32 %43, 38
  %82 = inttoptr i32 %calcOffset75 to i8*
  store i8 0, i8* %82, align 1
  %calcOffset76 = add i32 %43, 24
  %83 = inttoptr i32 %calcOffset76 to i32*
  store i32 %81, i32* %83, align 4
  %84 = call i32 @strlen(i32 %_local_7.0)
  %85 = add i32 %84, 6
  %86 = call i32 @malloc(i32 %85)
  %87 = icmp eq i32 %86, 0
  br i1 %87, label %blk_exit3, label %brif_next78

brif_next78:                                      ; preds = %brif_next61
  %calcOffset79 = add i32 %0, -1568
  %88 = inttoptr i32 %calcOffset79 to i32*
  store i32 %_local_7.0, i32* %88, align 4
  %89 = add i32 %84, 5
  %90 = call i32 @snprintf(i32 %86, i32 %89, i32 1544, i32 %calcOffset79)
  %91 = call i32 @stat(i32 %86, i32 %.pre-phi230)
  %brif_val81.not = icmp eq i32 %91, 0
  br i1 %brif_val81.not, label %brif_next82, label %blk_exit80

blk_exit80:                                       ; preds = %brif_next82, %brif_next78
  call void @free(i32 %86)
  %calcOffset84 = add i32 %43, 72
  %92 = inttoptr i32 %calcOffset84 to i32*
  store i32 %_arg_5, i32* %92, align 4
  %brif_val91.not = icmp slt i32 %29, 0
  %or.cond229 = select i1 %brif_val91.not, i1 true, i1 %34
  br i1 %or.cond229, label %blk_exit90, label %blk_exit89

brif_next82:                                      ; preds = %brif_next78
  store i8 1, i8* %82, align 1
  br label %blk_exit80

blk_exit85:                                       ; preds = %blk_exit119, %blk_exit86
  %93 = inttoptr i32 %_arg_3 to i32*
  %94 = load i32, i32* %93, align 4
  %brif_val203.not = icmp eq i32 %94, 0
  br i1 %brif_val203.not, label %brif_next204, label %blk_exit201

blk_exit86:                                       ; preds = %brif_next175
  %95 = inttoptr i32 %calcOffset71 to i32*
  store i32 %157, i32* %95, align 4
  br label %blk_exit85

blk_exit87:                                       ; preds = %brif_next138, %brif_next135
  call void @free(i32 %_local_7.0)
  %96 = inttoptr i32 %calcOffset71 to i32*
  %97 = load i32, i32* %96, align 4
  call void @free(i32 %97)
  %calcOffset192 = add i32 %43, 32
  %98 = inttoptr i32 %calcOffset192 to i32*
  %99 = load i32, i32* %98, align 4
  call void @free(i32 %99)
  %100 = load i32, i32* %80, align 4
  call void @free(i32 %100)
  %101 = load i32, i32* %83, align 4
  call void @free(i32 %101)
  %102 = load i32, i32* %77, align 4
  %brif_val197.not = icmp slt i32 %102, 0
  br i1 %brif_val197.not, label %blk_exit11, label %blk_exit195

blk_exit88:                                       ; preds = %brif_next178, %brif_next171, %brif_next169, %brif_next167, %brif_next165, %brif_next163, %brif_next161, %brif_next159, %brif_next157, %brif_next155, %brif_next153, %brif_next151, %brif_next149, %brif_next147, %brif_next140, %brif_next100, %blk_exit89
  %brif_val180.not = icmp eq i32 %_arg_5, 0
  br i1 %brif_val180.not, label %brif_next181, label %blk_exit179

blk_exit89:                                       ; preds = %blk_exit80
  %103 = call i32 @strrchr(i32 %_local_7.0, i32 47)
  %104 = add i32 %103, 1
  %select_cond.not = icmp eq i32 %103, 0
  %105 = select i1 %select_cond.not, i32 %_local_7.0, i32 %104
  %106 = inttoptr i32 %105 to i8*
  %107 = load i8, i8* %106, align 1
  %108 = icmp eq i8 %107, 46
  br i1 %108, label %blk_exit88, label %brif_next144

blk_exit90:                                       ; preds = %blk_exit80
  %109 = load i32, i32* %77, align 4
  %110 = call i32 @close(i32 %109)
  store i32 -1, i32* %77, align 4
  %111 = load i32, i32* %83, align 4
  %112 = call i32 @opendir(i32 %111)
  %brif_val99.not = icmp eq i32 %112, 0
  br i1 %brif_val99.not, label %brif_next100, label %blk_exit97

blk_exit97:                                       ; preds = %blk_exit90
  %calcOffset102 = add i32 %43, 64
  %113 = inttoptr i32 %calcOffset102 to i32*
  store i32 0, i32* %113, align 4
  %calcOffset103 = add i32 %0, -512
  %114 = inttoptr i32 %calcOffset103 to i32*
  store i32 0, i32* %114, align 4
  %115 = call i32 @malloc(i32 800)
  %116 = icmp eq i32 %115, 0
  br i1 %116, label %blk_exit2, label %loop_entry107

brif_next100:                                     ; preds = %blk_exit90
  %117 = load i32, i32* %83, align 4
  call void @perror(i32 %117)
  br label %blk_exit88

blk_exit106:                                      ; preds = %loop_entry107
  %118 = call i32 @closedir(i32 %112)
  call void @qsort(i32 %_local_10.0, i32 %_local_8.1, i32 4, i32 1)
  %119 = icmp eq i32 %_local_8.1, 0
  br i1 %119, label %blk_exit119, label %loop_entry122

loop_entry107:                                    ; preds = %blk_exit113, %brif_next109, %blk_exit97
  %_local_8.1 = phi i32 [ %_local_8.1, %brif_next109 ], [ %129, %blk_exit113 ], [ 0, %blk_exit97 ]
  %_local_10.0 = phi i32 [ %_local_10.0, %brif_next109 ], [ %_local_10.1, %blk_exit113 ], [ %115, %blk_exit97 ]
  %_local_11.0 = phi i32 [ %_local_11.0, %brif_next109 ], [ %_local_11.1, %blk_exit113 ], [ 200, %blk_exit97 ]
  %120 = call i32 @readdir(i32 %112)
  %121 = icmp eq i32 %120, 0
  br i1 %121, label %blk_exit106, label %brif_next109

brif_next109:                                     ; preds = %loop_entry107
  %calcOffset110 = add i32 %120, 9
  %122 = inttoptr i32 %calcOffset110 to i8*
  %123 = load i8, i8* %122, align 1
  %124 = icmp eq i8 %123, 0
  br i1 %124, label %loop_entry107, label %brif_next112

brif_next112:                                     ; preds = %brif_next109
  %125 = call i32 @strdup(i32 %calcOffset110)
  %.not224 = icmp eq i32 %_local_8.1, %_local_11.0
  br i1 %.not224, label %brif_next115, label %blk_exit113

blk_exit113:                                      ; preds = %brif_next115, %brif_next112
  %_local_10.1 = phi i32 [ %132, %brif_next115 ], [ %_local_10.0, %brif_next112 ]
  %_local_11.1 = phi i32 [ %130, %brif_next115 ], [ %_local_11.0, %brif_next112 ]
  %126 = shl i32 %_local_8.1, 2
  %127 = add i32 %_local_10.1, %126
  %128 = inttoptr i32 %127 to i32*
  store i32 %125, i32* %128, align 4
  %129 = add i32 %_local_8.1, 1
  br label %loop_entry107

brif_next115:                                     ; preds = %brif_next112
  %130 = add i32 %_local_8.1, 200
  %131 = shl i32 %130, 2
  %132 = call i32 @realloc(i32 %_local_10.0, i32 %131)
  %133 = icmp eq i32 %132, 0
  br i1 %133, label %blk_exit1, label %blk_exit113

blk_exit119:                                      ; preds = %blk_exit123, %blk_exit106
  call void @free(i32 %_local_10.0)
  %134 = load i32, i32* %113, align 4
  %brif_val134.not = icmp eq i32 %134, 0
  br i1 %brif_val134.not, label %brif_next135, label %blk_exit85

loop_entry122:                                    ; preds = %blk_exit123, %blk_exit106
  %_local_8.2 = phi i32 [ %142, %blk_exit123 ], [ %_local_8.1, %blk_exit106 ]
  %_param_1.0 = phi i32 [ %141, %blk_exit123 ], [ %_local_10.0, %blk_exit106 ]
  %135 = inttoptr i32 %_param_1.0 to i32*
  %136 = load i32, i32* %135, align 4
  %137 = load i32, i32* %83, align 4
  %138 = call i32 @add_file(i32 -1, i32 %136, i32 %137, i32 %calcOffset70, i32 %calcOffset103, i32 %43)
  %139 = icmp eq i32 %138, 0
  br i1 %139, label %blk_exit123, label %brif_next127

blk_exit123:                                      ; preds = %brif_next127, %loop_entry122
  %140 = load i32, i32* %135, align 4
  call void @free(i32 %140)
  %141 = add i32 %_param_1.0, 4
  %142 = add i32 %_local_8.2, -1
  %brif_val131.not = icmp eq i32 %142, 0
  br i1 %brif_val131.not, label %blk_exit119, label %loop_entry122

brif_next127:                                     ; preds = %loop_entry122
  %143 = load i32, i32* %113, align 4
  %144 = add i32 %143, 1
  store i32 %144, i32* %113, align 4
  br label %blk_exit123

brif_next135:                                     ; preds = %blk_exit119
  %145 = load i32, i32* %83, align 4
  %146 = call i32 @strcmp(i32 1121, i32 %145)
  %147 = icmp eq i32 %146, 0
  br i1 %147, label %blk_exit87, label %brif_next138

brif_next138:                                     ; preds = %brif_next135
  %148 = call i32 @strcmp(i32 1282, i32 %145)
  %149 = icmp eq i32 %148, 0
  br i1 %149, label %blk_exit87, label %brif_next140

brif_next140:                                     ; preds = %brif_next138
  %calcOffset141 = add i32 %0, -1584
  %150 = inttoptr i32 %calcOffset141 to i32*
  store i32 %145, i32* %150, align 4
  %151 = load i32, i32* null, align 2147483648
  %152 = call i32 @fprintf(i32 %151, i32 1942, i32 %calcOffset141)
  br label %blk_exit88

brif_next144:                                     ; preds = %blk_exit89
  %153 = call i32 @strrchr(i32 %105, i32 46)
  %154 = icmp eq i32 %153, 0
  br i1 %154, label %blk_exit145, label %brif_next147

blk_exit145:                                      ; preds = %brif_next171, %brif_next144
  %155 = call i32 @strlen(i32 %_local_7.0)
  %156 = add i32 %155, 7
  %157 = call i32 @malloc(i32 %156)
  %158 = icmp eq i32 %157, 0
  br i1 %158, label %blk_exit, label %brif_next175

brif_next147:                                     ; preds = %brif_next144
  %159 = add i32 %153, 1
  %160 = call i32 @strcmp(i32 %159, i32 1077)
  %161 = icmp eq i32 %160, 0
  br i1 %161, label %blk_exit88, label %brif_next149

brif_next149:                                     ; preds = %brif_next147
  %162 = call i32 @strcmp(i32 %159, i32 1085)
  %163 = icmp eq i32 %162, 0
  br i1 %163, label %blk_exit88, label %brif_next151

brif_next151:                                     ; preds = %brif_next149
  %164 = call i32 @strcmp(i32 %159, i32 1412)
  %165 = icmp eq i32 %164, 0
  br i1 %165, label %blk_exit88, label %brif_next153

brif_next153:                                     ; preds = %brif_next151
  %166 = call i32 @strcmp(i32 %159, i32 1244)
  %167 = icmp eq i32 %166, 0
  br i1 %167, label %blk_exit88, label %brif_next155

brif_next155:                                     ; preds = %brif_next153
  %168 = call i32 @strcmp(i32 %159, i32 1196)
  %169 = icmp eq i32 %168, 0
  br i1 %169, label %blk_exit88, label %brif_next157

brif_next157:                                     ; preds = %brif_next155
  %170 = call i32 @strcmp(i32 %159, i32 1242)
  %171 = icmp eq i32 %170, 0
  br i1 %171, label %blk_exit88, label %brif_next159

brif_next159:                                     ; preds = %brif_next157
  %172 = call i32 @strcmp(i32 %159, i32 1322)
  %173 = icmp eq i32 %172, 0
  br i1 %173, label %blk_exit88, label %brif_next161

brif_next161:                                     ; preds = %brif_next159
  %174 = call i32 @strcmp(i32 %159, i32 1163)
  %175 = icmp eq i32 %174, 0
  br i1 %175, label %blk_exit88, label %brif_next163

brif_next163:                                     ; preds = %brif_next161
  %176 = call i32 @strcmp(i32 %159, i32 1204)
  %177 = icmp eq i32 %176, 0
  br i1 %177, label %blk_exit88, label %brif_next165

brif_next165:                                     ; preds = %brif_next163
  %178 = call i32 @strcmp(i32 %159, i32 1336)
  %179 = icmp eq i32 %178, 0
  br i1 %179, label %blk_exit88, label %brif_next167

brif_next167:                                     ; preds = %brif_next165
  %180 = call i32 @strcmp(i32 %159, i32 1159)
  %181 = icmp eq i32 %180, 0
  br i1 %181, label %blk_exit88, label %brif_next169

brif_next169:                                     ; preds = %brif_next167
  %182 = call i32 @strcmp(i32 %159, i32 1200)
  %183 = icmp eq i32 %182, 0
  br i1 %183, label %blk_exit88, label %brif_next171

brif_next171:                                     ; preds = %brif_next169
  %184 = call i32 @strcmp(i32 %159, i32 1221)
  %185 = icmp eq i32 %184, 0
  br i1 %185, label %blk_exit88, label %blk_exit145

brif_next175:                                     ; preds = %blk_exit145
  %calcOffset176 = add i32 %0, -1600
  %186 = inttoptr i32 %calcOffset176 to i32*
  store i32 %_local_7.0, i32* %186, align 4
  %187 = add i32 %155, 6
  %188 = call i32 @snprintf(i32 %157, i32 %187, i32 1074, i32 %calcOffset176)
  %189 = call i32 @access(i32 %157, i32 4)
  %brif_val177.not = icmp slt i32 %189, 0
  br i1 %brif_val177.not, label %brif_next178, label %blk_exit86

brif_next178:                                     ; preds = %brif_next175
  call void @free(i32 %157)
  br label %blk_exit88

blk_exit179:                                      ; preds = %brif_next181, %blk_exit88
  call void @free(i32 %_local_7.0)
  %190 = inttoptr i32 %calcOffset71 to i32*
  %191 = load i32, i32* %190, align 4
  call void @free(i32 %191)
  %calcOffset184 = add i32 %43, 32
  %192 = inttoptr i32 %calcOffset184 to i32*
  %193 = load i32, i32* %192, align 4
  call void @free(i32 %193)
  %194 = load i32, i32* %80, align 4
  call void @free(i32 %194)
  %195 = load i32, i32* %83, align 4
  call void @free(i32 %195)
  %196 = load i32, i32* %77, align 4
  %brif_val189.not = icmp slt i32 %196, 0
  br i1 %brif_val189.not, label %blk_exit11, label %blk_exit187

brif_next181:                                     ; preds = %blk_exit88
  %calcOffset182 = add i32 %0, -1632
  %197 = inttoptr i32 %calcOffset182 to i32*
  store i32 %_local_7.0, i32* %197, align 4
  %198 = load i32, i32* null, align 2147483648
  %199 = call i32 @fprintf(i32 %198, i32 1581, i32 %calcOffset182)
  br label %blk_exit179

blk_exit187:                                      ; preds = %blk_exit179
  %200 = call i32 @close(i32 %196)
  br label %blk_exit11

blk_exit195:                                      ; preds = %blk_exit87
  %201 = call i32 @close(i32 %102)
  br label %blk_exit11

blk_exit200:                                      ; preds = %brif_next209, %blk_exit206, %brif_next204
  %.pre-phi = phi i32* [ %93, %blk_exit206 ], [ %207, %brif_next209 ], [ %93, %brif_next204 ]
  store i32 %43, i32* %.pre-phi, align 4
  br label %blk_exit11

blk_exit201:                                      ; preds = %blk_exit85
  %202 = load i32, i32* %78, align 4
  %.not226 = icmp eq i32 %202, -1
  br i1 %.not226, label %brif_next209, label %blk_exit206

brif_next204:                                     ; preds = %blk_exit85
  %203 = inttoptr i32 %_arg_4 to i32*
  store i32 %43, i32* %203, align 4
  br label %blk_exit200

blk_exit206:                                      ; preds = %blk_exit201
  %calcOffset214 = add i32 %94, 80
  %204 = inttoptr i32 %calcOffset214 to i32*
  store i32 %43, i32* %204, align 4
  %205 = load i32, i32* %93, align 4
  %206 = inttoptr i32 %calcOffset68 to i32*
  store i32 %205, i32* %206, align 4
  br label %blk_exit200

brif_next209:                                     ; preds = %blk_exit201
  %207 = inttoptr i32 %_arg_4 to i32*
  %208 = load i32, i32* %207, align 4
  %calcOffset211 = add i32 %208, 76
  %209 = inttoptr i32 %calcOffset211 to i32*
  store i32 %43, i32* %209, align 4
  %210 = load i32, i32* %207, align 4
  %calcOffset213 = add i32 %43, 80
  %211 = inttoptr i32 %calcOffset213 to i32*
  store i32 %210, i32* %211, align 4
  br label %blk_exit200
}

; Function Attrs: null_pointer_is_valid
define internal i32 @maxlen_in_list(i32 %_arg_0) #0 {
allocator:
  %brif_val.not = icmp eq i32 %_arg_0, 0
  br i1 %brif_val.not, label %return, label %loop_entry

return:                                           ; preds = %blk_exit1, %allocator
  %return_0 = phi i32 [ 0, %allocator ], [ %4, %blk_exit1 ]
  ret i32 %return_0

loop_entry:                                       ; preds = %blk_exit1, %allocator
  %_local_1.0 = phi i32 [ %4, %blk_exit1 ], [ 0, %allocator ]
  %_param_0.0 = phi i32 [ %6, %blk_exit1 ], [ %_arg_0, %allocator ]
  %calcOffset = add i32 %_param_0.0, 68
  %0 = inttoptr i32 %calcOffset to i32*
  %1 = load i32, i32* %0, align 4
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %blk_exit2, label %brif_next4

blk_exit1:                                        ; preds = %brif_next4, %blk_exit2
  %storemerge = phi i32 [ %10, %brif_next4 ], [ %9, %blk_exit2 ]
  %3 = icmp sgt i32 %storemerge, %_local_1.0
  %4 = select i1 %3, i32 %storemerge, i32 %_local_1.0
  %calcOffset6 = add i32 %_param_0.0, 76
  %5 = inttoptr i32 %calcOffset6 to i32*
  %6 = load i32, i32* %5, align 4
  %brif_val7.not = icmp eq i32 %6, 0
  br i1 %brif_val7.not, label %return, label %loop_entry

blk_exit2:                                        ; preds = %loop_entry
  call void @get_tbl(i32 %_param_0.0)
  %7 = add i32 %_param_0.0, 48
  %8 = inttoptr i32 %7 to i32*
  %9 = load i32, i32* %8, align 4
  br label %blk_exit1

brif_next4:                                       ; preds = %loop_entry
  %10 = call i32 @maxlen_in_list(i32 %1)
  br label %blk_exit1
}

; Function Attrs: null_pointer_is_valid
define internal void @matches_in_list(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -32
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = icmp eq i32 %_arg_0, 0
  br i1 %2, label %blk_exit1, label %brif_next

blk_exit:                                         ; preds = %brif_next9
  %calcOffset92 = add i32 %_param_0.0, 24
  %3 = inttoptr i32 %calcOffset92 to i32*
  %4 = load i32, i32* %3, align 4
  call void @perror(i32 %4)
  call void @exit(i32 1)
  unreachable

blk_exit1:                                        ; preds = %blk_exit2, %allocator
  store i32 %0, i32* @__stack_pointer, align 4
  ret void

brif_next:                                        ; preds = %allocator
  %5 = load i32, i32* null, align 2147483648
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit2, %brif_next
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %10, %blk_exit2 ]
  %calcOffset = add i32 %_param_0.0, 68
  %6 = inttoptr i32 %calcOffset to i32*
  %7 = load i32, i32* %6, align 4
  %8 = icmp eq i32 %7, 0
  br i1 %8, label %blk_exit3, label %brif_next5

blk_exit2:                                        ; preds = %blk_exit17, %blk_exit6, %brif_next5
  %calcOffset89 = add i32 %_param_0.0, 76
  %9 = inttoptr i32 %calcOffset89 to i32*
  %10 = load i32, i32* %9, align 4
  %brif_val90.not = icmp eq i32 %10, 0
  br i1 %brif_val90.not, label %blk_exit1, label %loop_entry

blk_exit3:                                        ; preds = %loop_entry
  %calcOffset7 = add i32 %_param_0.0, 16
  %11 = inttoptr i32 %calcOffset7 to i32*
  %12 = load i32, i32* %11, align 4
  %brif_val8.not = icmp eq i32 %12, 0
  br i1 %brif_val8.not, label %brif_next9, label %blk_exit6

brif_next5:                                       ; preds = %loop_entry
  call void @matches_in_list(i32 %7, i32 %_arg_1)
  br label %blk_exit2

blk_exit6:                                        ; preds = %brif_next9, %blk_exit3
  %_local_6.0 = phi i32 [ %19, %brif_next9 ], [ %12, %blk_exit3 ]
  %13 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  %14 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %15 = call i32 @fgets(i32 %13, i32 %14, i32 %_local_6.0)
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %blk_exit2, label %loop_entry16

brif_next9:                                       ; preds = %blk_exit3
  %calcOffset10 = add i32 %_param_0.0, 4
  %17 = inttoptr i32 %calcOffset10 to i32*
  %18 = load i32, i32* %17, align 4
  %19 = call i32 @fdopen(i32 %18, i32 1194)
  store i32 %19, i32* %11, align 4
  %20 = icmp eq i32 %19, 0
  br i1 %20, label %blk_exit, label %blk_exit6

loop_entry16:                                     ; preds = %blk_exit17, %blk_exit6
  %_local_7.0 = phi i32 [ %_local_7.1, %blk_exit17 ], [ 0, %blk_exit6 ]
  %_local_5.0 = phi i32 [ %storemerge, %blk_exit17 ], [ %13, %blk_exit6 ]
  %21 = inttoptr i32 %_local_5.0 to i8*
  %22 = load i8, i8* %21, align 1
  %calcOffset21 = add i32 %_param_0.0, 60
  %23 = inttoptr i32 %calcOffset21 to i8*
  %24 = load i8, i8* %23, align 1
  %.not = icmp eq i8 %22, %24
  br i1 %.not, label %brif_next23, label %blk_exit19

blk_exit17:                                       ; preds = %blk_exit80, %blk_exit19
  %_local_7.1 = phi i32 [ %_local_7.2, %blk_exit80 ], [ %_local_7.0, %blk_exit19 ]
  %storemerge = phi i32 [ %80, %blk_exit80 ], [ %33, %blk_exit19 ]
  %25 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %26 = load i32, i32* %11, align 4
  %27 = call i32 @fgets(i32 %storemerge, i32 %25, i32 %26)
  %brif_val87.not = icmp eq i32 %27, 0
  br i1 %brif_val87.not, label %blk_exit2, label %loop_entry16

blk_exit18:                                       ; preds = %brif_next23
  store i8 0, i8* %21, align 1
  %28 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  %calcOffset29 = add i32 %_param_0.0, 38
  %29 = inttoptr i32 %calcOffset29 to i8*
  %30 = load i8, i8* %29, align 1
  %31 = icmp eq i8 %30, 0
  br i1 %31, label %blk_exit28, label %brif_next31

blk_exit19:                                       ; preds = %brif_next23, %loop_entry16
  %32 = call i32 @strlen(i32 %_local_5.0)
  %33 = add i32 %_local_5.0, %32
  br label %blk_exit17

brif_next23:                                      ; preds = %loop_entry16
  %calcOffset24 = add i32 %_local_5.0, 1
  %34 = inttoptr i32 %calcOffset24 to i8*
  %35 = load i8, i8* %34, align 1
  %36 = icmp eq i8 %35, 10
  br i1 %36, label %blk_exit18, label %blk_exit19

blk_exit28:                                       ; preds = %brif_next33, %brif_next31, %blk_exit18
  %_local_8.0 = phi i32 [ %28, %blk_exit18 ], [ %42, %brif_next33 ], [ %28, %brif_next31 ]
  %37 = sub i32 %_local_5.0, %28
  %calcOffset35 = add i32 %_param_0.0, 56
  %38 = inttoptr i32 %calcOffset35 to i8*
  %39 = load i8, i8* %38, align 1
  %40 = and i8 %39, 4
  %brif_val36.not.not = icmp eq i8 %40, 0
  br i1 %brif_val36.not.not, label %blk_exit34, label %brif_next37

brif_next31:                                      ; preds = %blk_exit18
  %41 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val32.not = icmp eq i8 %41, 0
  br i1 %brif_val32.not, label %brif_next33, label %blk_exit28

brif_next33:                                      ; preds = %brif_next31
  %42 = call i32 @strdup(i32 %28)
  br label %blk_exit28

blk_exit34:                                       ; preds = %blk_exit42, %brif_next37, %blk_exit28
  %43 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %44 = icmp slt i32 %37, %43
  br i1 %44, label %blk_exit58, label %brif_next60

brif_next37:                                      ; preds = %blk_exit28
  %45 = inttoptr i32 %_local_8.0 to i8*
  %46 = load i8, i8* %45, align 1
  %47 = icmp eq i8 %46, 0
  br i1 %47, label %blk_exit34, label %loop_entry41

loop_entry41:                                     ; preds = %blk_exit42, %brif_next37
  %_local_6.1 = phi i32 [ %51, %blk_exit42 ], [ %_local_8.0, %brif_next37 ]
  %_local_5.1.in = phi i8 [ %53, %blk_exit42 ], [ %46, %brif_next37 ]
  %_local_5.1 = zext i8 %_local_5.1.in to i32
  %48 = add nsw i32 %_local_5.1, -91
  %49 = icmp uge i32 %48, -26
  %50 = and i32 %_local_5.1, 128
  %brif_val47.not = icmp eq i32 %50, 0
  %or.cond = and i1 %49, %brif_val47.not
  br i1 %or.cond, label %blk_exit43, label %blk_exit44

blk_exit42:                                       ; preds = %blk_exit44, %blk_exit43
  %51 = add i32 %_local_6.1, 1
  %52 = inttoptr i32 %51 to i8*
  %53 = load i8, i8* %52, align 1
  %brif_val55.not = icmp eq i8 %53, 0
  br i1 %brif_val55.not, label %blk_exit34, label %loop_entry41

blk_exit43:                                       ; preds = %blk_exit44, %loop_entry41
  %storemerge94 = phi i8 [ 65, %loop_entry41 ], [ 97, %blk_exit44 ]
  %storemerge93 = phi i8 [ -52, %loop_entry41 ], [ -84, %blk_exit44 ]
  %54 = add i8 %storemerge93, %_local_5.1.in
  %55 = urem i8 %54, 26
  %56 = add nuw nsw i8 %55, %storemerge94
  %57 = inttoptr i32 %_local_6.1 to i8*
  store i8 %56, i8* %57, align 1
  br label %blk_exit42

blk_exit44:                                       ; preds = %loop_entry41
  %58 = add nsw i32 %_local_5.1, -123
  %59 = icmp uge i32 %58, -26
  %60 = and i32 %_local_5.1, 128
  %brif_val51.not = icmp eq i32 %60, 0
  %or.cond95 = and i1 %59, %brif_val51.not
  br i1 %or.cond95, label %blk_exit43, label %blk_exit42

blk_exit57:                                       ; preds = %blk_exit70, %brif_next65, %blk_exit63, %brif_next60
  %_local_7.2 = phi i32 [ 1, %blk_exit70 ], [ %_local_7.0, %blk_exit63 ], [ %_local_7.0, %brif_next65 ], [ %_local_7.0, %brif_next60 ]
  %61 = load i8, i8* %29, align 1
  %62 = icmp eq i8 %61, 0
  br i1 %62, label %blk_exit80, label %brif_next83

blk_exit58:                                       ; preds = %brif_next60, %blk_exit34
  %63 = icmp sgt i32 %37, %43
  br i1 %63, label %blk_exit63, label %brif_next65

brif_next60:                                      ; preds = %blk_exit34
  %64 = load i8, i8* inttoptr (i32 2273 to i8*), align 1
  %brif_val61.not = icmp eq i8 %64, 0
  br i1 %brif_val61.not, label %blk_exit58, label %blk_exit57

blk_exit63:                                       ; preds = %brif_next65, %blk_exit58
  %65 = call i32 @regexec(i32 2232, i32 %_local_8.0, i32 0, i32 0, i32 0)
  %brif_val68.not = icmp eq i32 %65, 0
  br i1 %brif_val68.not, label %brif_next69, label %blk_exit57

brif_next65:                                      ; preds = %blk_exit58
  %66 = load i8, i8* inttoptr (i32 2274 to i8*), align 2
  %brif_val66.not = icmp eq i8 %66, 0
  br i1 %brif_val66.not, label %blk_exit63, label %blk_exit57

brif_next69:                                      ; preds = %blk_exit63
  %brif_val71.not = icmp eq i32 %_local_7.0, 0
  br i1 %brif_val71.not, label %brif_next72, label %blk_exit70

blk_exit70:                                       ; preds = %brif_next72, %brif_next69
  %67 = call i32 @fputs(i32 %_local_8.0, i32 %5)
  %68 = load i8, i8* %23, align 1
  %69 = zext i8 %68 to i32
  %70 = inttoptr i32 %1 to i32*
  store i32 %69, i32* %70, align 4
  %71 = call i32 @printf(i32 1869, i32 %1)
  br label %blk_exit57

brif_next72:                                      ; preds = %brif_next69
  %calcOffset73 = add i32 %_param_0.0, 20
  %72 = inttoptr i32 %calcOffset73 to i32*
  %73 = load i32, i32* %72, align 4
  %74 = load i8, i8* %23, align 1
  %75 = zext i8 %74 to i32
  %calcOffset75 = add i32 %0, -12
  %76 = inttoptr i32 %calcOffset75 to i32*
  store i32 %75, i32* %76, align 4
  %calcOffset76 = add i32 %0, -16
  %77 = inttoptr i32 %calcOffset76 to i32*
  store i32 %73, i32* %77, align 4
  %78 = call i32 @fprintf(i32 %5, i32 1864, i32 %calcOffset76)
  %79 = inttoptr i32 %_arg_1 to i8*
  store i8 1, i8* %79, align 1
  br label %blk_exit70

blk_exit80:                                       ; preds = %brif_next85, %brif_next83, %blk_exit57
  %80 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  br label %blk_exit17

brif_next83:                                      ; preds = %blk_exit57
  %81 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val84.not = icmp eq i8 %81, 0
  br i1 %brif_val84.not, label %brif_next85, label %blk_exit80

brif_next85:                                      ; preds = %brif_next83
  call void @free(i32 %_local_8.0)
  br label %blk_exit80
}

; Function Attrs: null_pointer_is_valid
define internal void @get_tbl(i32 %_arg_0) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -96
  store i32 %1, i32* @__stack_pointer, align 4
  %calcOffset = add i32 %_arg_0, 36
  %2 = inttoptr i32 %calcOffset to i8*
  %3 = load i8, i8* %2, align 1
  %brif_val.not = icmp eq i8 %3, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

blk_exit:                                         ; preds = %brif_next25
  %calcOffset70 = add i32 %_arg_0, 24
  %4 = inttoptr i32 %calcOffset70 to i32*
  %5 = load i32, i32* %4, align 4
  %6 = inttoptr i32 %1 to i32*
  store i32 %5, i32* %6, align 4
  %7 = load i32, i32* null, align 2147483648
  %8 = call i32 @fprintf(i32 %7, i32 1841, i32 %1)
  call void @exit(i32 1)
  unreachable

blk_exit1:                                        ; preds = %brif_next23
  %calcOffset68 = add i32 %_arg_0, 24
  %9 = inttoptr i32 %calcOffset68 to i32*
  %10 = load i32, i32* %9, align 4
  %calcOffset69 = add i32 %0, -80
  %11 = inttoptr i32 %calcOffset69 to i32*
  store i32 %10, i32* %11, align 4
  %12 = load i32, i32* null, align 2147483648
  %13 = call i32 @fprintf(i32 %12, i32 1841, i32 %calcOffset69)
  call void @exit(i32 1)
  unreachable

blk_exit2:                                        ; preds = %brif_next21
  %calcOffset66 = add i32 %_arg_0, 24
  %14 = inttoptr i32 %calcOffset66 to i32*
  %15 = load i32, i32* %14, align 4
  %calcOffset67 = add i32 %0, -64
  %16 = inttoptr i32 %calcOffset67 to i32*
  store i32 %15, i32* %16, align 4
  %17 = load i32, i32* null, align 2147483648
  %18 = call i32 @fprintf(i32 %17, i32 1841, i32 %calcOffset67)
  call void @exit(i32 1)
  unreachable

blk_exit3:                                        ; preds = %brif_next19
  %calcOffset64 = add i32 %_arg_0, 24
  %19 = inttoptr i32 %calcOffset64 to i32*
  %20 = load i32, i32* %19, align 4
  %calcOffset65 = add i32 %0, -48
  %21 = inttoptr i32 %calcOffset65 to i32*
  store i32 %20, i32* %21, align 4
  %22 = load i32, i32* null, align 2147483648
  %23 = call i32 @fprintf(i32 %22, i32 1841, i32 %calcOffset65)
  call void @exit(i32 1)
  unreachable

blk_exit4:                                        ; preds = %brif_next17
  %calcOffset62 = add i32 %_arg_0, 24
  %24 = inttoptr i32 %calcOffset62 to i32*
  %25 = load i32, i32* %24, align 4
  %calcOffset63 = add i32 %0, -32
  %26 = inttoptr i32 %calcOffset63 to i32*
  store i32 %25, i32* %26, align 4
  %27 = load i32, i32* null, align 2147483648
  %28 = call i32 @fprintf(i32 %27, i32 1841, i32 %calcOffset63)
  call void @exit(i32 1)
  unreachable

blk_exit5:                                        ; preds = %brif_next15
  %calcOffset60 = add i32 %_arg_0, 24
  %29 = inttoptr i32 %calcOffset60 to i32*
  %30 = load i32, i32* %29, align 4
  %calcOffset61 = add i32 %0, -16
  %31 = inttoptr i32 %calcOffset61 to i32*
  store i32 %30, i32* %31, align 4
  %32 = load i32, i32* null, align 2147483648
  %33 = call i32 @fprintf(i32 %32, i32 1841, i32 %calcOffset61)
  call void @exit(i32 1)
  unreachable

blk_exit6:                                        ; preds = %brif_next12
  %34 = load i32, i32* %41, align 4
  call void @perror(i32 %34)
  call void @exit(i32 1)
  unreachable

blk_exit7:                                        ; preds = %blk_exit8, %allocator
  store i32 %0, i32* @__stack_pointer, align 4
  ret void

brif_next:                                        ; preds = %allocator
  %calcOffset10 = add i32 %_arg_0, 68
  %35 = inttoptr i32 %calcOffset10 to i32*
  %36 = load i32, i32* %35, align 4
  %brif_val11.not = icmp eq i32 %36, 0
  br i1 %brif_val11.not, label %brif_next12, label %blk_exit9

blk_exit8:                                        ; preds = %blk_exit49, %brif_next27
  store i8 1, i8* %2, align 1
  br label %blk_exit7

blk_exit9:                                        ; preds = %brif_next
  %37 = add i32 %_arg_0, 52
  %38 = inttoptr i32 %37 to i32*
  store i32 -1, i32* %38, align 4
  %39 = add i32 %_arg_0, 44
  %40 = inttoptr i32 %39 to i64*
  store i64 0, i64* %40, align 8
  br label %loop_entry

brif_next12:                                      ; preds = %brif_next
  %calcOffset13 = add i32 %_arg_0, 28
  %41 = inttoptr i32 %calcOffset13 to i32*
  %42 = load i32, i32* %41, align 4
  %43 = call i32 @open(i32 %42, i32 67108864, i32 0)
  %brif_val14.not = icmp sgt i32 %43, -1
  br i1 %brif_val14.not, label %brif_next15, label %blk_exit6

brif_next15:                                      ; preds = %brif_next12
  %44 = add i32 %_arg_0, 40
  %45 = call i32 @read(i32 %43, i32 %44, i32 4)
  %.not = icmp eq i32 %45, 4
  br i1 %.not, label %brif_next17, label %blk_exit5

brif_next17:                                      ; preds = %brif_next15
  %46 = add i32 %_arg_0, 44
  %47 = call i32 @read(i32 %43, i32 %46, i32 4)
  %.not72 = icmp eq i32 %47, 4
  br i1 %.not72, label %brif_next19, label %blk_exit4

brif_next19:                                      ; preds = %brif_next17
  %48 = add i32 %_arg_0, 48
  %49 = call i32 @read(i32 %43, i32 %48, i32 4)
  %.not73 = icmp eq i32 %49, 4
  br i1 %.not73, label %brif_next21, label %blk_exit3

brif_next21:                                      ; preds = %brif_next19
  %50 = add i32 %_arg_0, 52
  %51 = call i32 @read(i32 %43, i32 %50, i32 4)
  %.not74 = icmp eq i32 %51, 4
  br i1 %.not74, label %brif_next23, label %blk_exit2

brif_next23:                                      ; preds = %brif_next21
  %52 = add i32 %_arg_0, 56
  %53 = call i32 @read(i32 %43, i32 %52, i32 4)
  %.not75 = icmp eq i32 %53, 4
  br i1 %.not75, label %brif_next25, label %blk_exit1

brif_next25:                                      ; preds = %brif_next23
  %54 = add i32 %_arg_0, 60
  %55 = call i32 @read(i32 %43, i32 %54, i32 4)
  %.not76 = icmp eq i32 %55, 4
  br i1 %.not76, label %brif_next27, label %blk_exit

brif_next27:                                      ; preds = %brif_next25
  %56 = inttoptr i32 %44 to i32*
  %57 = load i32, i32* %56, align 4
  %58 = call i32 @ntohl(i32 %57)
  store i32 %58, i32* %56, align 4
  %59 = inttoptr i32 %46 to i32*
  %60 = load i32, i32* %59, align 4
  %61 = call i32 @ntohl(i32 %60)
  store i32 %61, i32* %59, align 4
  %62 = inttoptr i32 %48 to i32*
  %63 = load i32, i32* %62, align 4
  %64 = call i32 @ntohl(i32 %63)
  store i32 %64, i32* %62, align 4
  %65 = inttoptr i32 %50 to i32*
  %66 = load i32, i32* %65, align 4
  %67 = call i32 @ntohl(i32 %66)
  store i32 %67, i32* %65, align 4
  %68 = inttoptr i32 %52 to i32*
  %69 = load i32, i32* %68, align 4
  %70 = call i32 @ntohl(i32 %69)
  store i32 %70, i32* %68, align 4
  %71 = call i32 @close(i32 %43)
  br label %blk_exit8

loop_entry:                                       ; preds = %blk_exit49, %blk_exit9
  %_local_2.0 = phi i32 [ %36, %blk_exit9 ], [ %88, %blk_exit49 ]
  call void @get_tbl(i32 %_local_2.0)
  %72 = inttoptr i32 %39 to i32*
  %73 = load i32, i32* %72, align 4
  %74 = add i32 %_local_2.0, 44
  %75 = inttoptr i32 %74 to i32*
  %76 = load i32, i32* %75, align 4
  %77 = add i32 %73, %76
  store i32 %77, i32* %72, align 4
  %calcOffset44 = add i32 %_arg_0, 48
  %78 = inttoptr i32 %calcOffset44 to i32*
  %79 = load i32, i32* %78, align 4
  %80 = add i32 %_local_2.0, 48
  %81 = inttoptr i32 %80 to i32*
  %82 = load i32, i32* %81, align 4
  %.not77 = icmp ult i32 %79, %82
  br i1 %.not77, label %brif_next47, label %blk_exit43

blk_exit43:                                       ; preds = %brif_next47, %loop_entry
  %83 = load i32, i32* %38, align 4
  %84 = add i32 %_local_2.0, 52
  %85 = inttoptr i32 %84 to i32*
  %86 = load i32, i32* %85, align 4
  %.not78 = icmp ugt i32 %83, %86
  br i1 %.not78, label %brif_next53, label %blk_exit49

brif_next47:                                      ; preds = %loop_entry
  store i32 %82, i32* %78, align 4
  br label %blk_exit43

blk_exit49:                                       ; preds = %brif_next53, %blk_exit43
  %calcOffset55 = add i32 %_local_2.0, 76
  %87 = inttoptr i32 %calcOffset55 to i32*
  %88 = load i32, i32* %87, align 4
  %brif_val56.not = icmp eq i32 %88, 0
  br i1 %brif_val56.not, label %blk_exit8, label %loop_entry

brif_next53:                                      ; preds = %blk_exit43
  store i32 %86, i32* %38, align 4
  br label %blk_exit49
}

; Function Attrs: null_pointer_is_valid
define internal void @print_list(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -96
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = icmp eq i32 %_arg_0, 0
  br i1 %2, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %blk_exit40, %blk_exit17, %allocator
  store i32 %0, i32* @__stack_pointer, align 4
  ret void

brif_next:                                        ; preds = %allocator
  %3 = add i32 %_arg_1, 1
  %4 = shl i32 %_arg_1, 2
  %5 = load i32, i32* null, align 2147483648
  %6 = icmp eq i32 %_arg_1, 0
  br i1 %6, label %loop_entry24, label %loop_entry

loop_entry:                                       ; preds = %blk_exit17, %brif_next
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %40, %blk_exit17 ]
  %calcOffset = add i32 %0, -12
  %7 = inttoptr i32 %calcOffset to i32*
  store i32 2206, i32* %7, align 4
  %calcOffset4 = add i32 %0, -16
  %8 = inttoptr i32 %calcOffset4 to i32*
  store i32 %4, i32* %8, align 4
  %9 = call i32 @fprintf(i32 %5, i32 1167, i32 %calcOffset4)
  %10 = inttoptr i32 %_param_0.0 to i32*
  %11 = load i32, i32* %10, align 4
  %.not = icmp eq i32 %11, -1
  br i1 %.not, label %brif_next9, label %blk_exit6

blk_exit5:                                        ; preds = %brif_next12, %blk_exit10, %blk_exit6
  %_local_6.0 = phi double [ %38, %brif_next12 ], [ %27, %blk_exit10 ], [ %23, %blk_exit6 ]
  %calcOffset14 = add i32 %0, -32
  %12 = inttoptr i32 %calcOffset14 to double*
  store double %_local_6.0, double* %12, align 8
  %13 = call i32 @fprintf(i32 %5, i32 1561, i32 %calcOffset14)
  %calcOffset15 = add i32 %_param_0.0, 20
  %14 = inttoptr i32 %calcOffset15 to i32*
  %15 = load i32, i32* %14, align 4
  %select_cond.not = icmp eq i32 %15, 0
  %16 = select i1 %select_cond.not, i32 1492, i32 %15
  %calcOffset16 = add i32 %0, -48
  %17 = inttoptr i32 %calcOffset16 to i32*
  store i32 %16, i32* %17, align 4
  %18 = call i32 @fprintf(i32 %5, i32 1190, i32 %calcOffset16)
  %19 = call i32 @putc(i32 10, i32 %5)
  %calcOffset18 = add i32 %_param_0.0, 68
  %20 = inttoptr i32 %calcOffset18 to i32*
  %21 = load i32, i32* %20, align 4
  %22 = icmp eq i32 %21, 0
  br i1 %22, label %blk_exit17, label %brif_next20

blk_exit6:                                        ; preds = %loop_entry
  %23 = sitofp i32 %11 to double
  br label %blk_exit5

brif_next9:                                       ; preds = %loop_entry
  %24 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %brif_val11.not = icmp eq i8 %24, 0
  br i1 %brif_val11.not, label %brif_next12, label %blk_exit10

blk_exit10:                                       ; preds = %brif_next9
  %25 = load i32, i32* inttoptr (i32 3352 to i32*), align 8
  %26 = sitofp i32 %25 to double
  %27 = fdiv double 1.000000e+02, %26
  br label %blk_exit5

brif_next12:                                      ; preds = %brif_next9
  %28 = load i32, i32* inttoptr (i32 3332 to i32*), align 4
  %29 = sitofp i32 %28 to double
  %30 = fsub double 1.000000e+02, %29
  %31 = add i32 %_param_0.0, 44
  %32 = inttoptr i32 %31 to i32*
  %33 = load i32, i32* %32, align 4
  %34 = uitofp i32 %33 to double
  %35 = fmul double %30, %34
  %36 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %37 = uitofp i32 %36 to double
  %38 = fdiv double %35, %37
  br label %blk_exit5

blk_exit17:                                       ; preds = %brif_next20, %blk_exit5
  %calcOffset21 = add i32 %_param_0.0, 76
  %39 = inttoptr i32 %calcOffset21 to i32*
  %40 = load i32, i32* %39, align 4
  %brif_val22.not = icmp eq i32 %40, 0
  br i1 %brif_val22.not, label %blk_exit, label %loop_entry

brif_next20:                                      ; preds = %blk_exit5
  call void @print_list(i32 %21, i32 %3)
  br label %blk_exit17

loop_entry24:                                     ; preds = %blk_exit40, %brif_next
  %_param_0.1 = phi i32 [ %_arg_0, %brif_next ], [ %75, %blk_exit40 ]
  %calcOffset25 = add i32 %0, -60
  %41 = inttoptr i32 %calcOffset25 to i32*
  store i32 2206, i32* %41, align 4
  %calcOffset26 = add i32 %0, -64
  %42 = inttoptr i32 %calcOffset26 to i32*
  store i32 %4, i32* %42, align 4
  %43 = call i32 @fprintf(i32 %5, i32 1167, i32 %calcOffset26)
  %44 = inttoptr i32 %_param_0.1 to i32*
  %45 = load i32, i32* %44, align 4
  %46 = icmp eq i32 %45, -1
  br i1 %46, label %blk_exit28, label %brif_next31

blk_exit27:                                       ; preds = %brif_next34, %blk_exit32, %brif_next31
  %_local_6.1 = phi double [ %73, %brif_next34 ], [ %62, %blk_exit32 ], [ %59, %brif_next31 ]
  %calcOffset36 = add i32 %0, -80
  %47 = inttoptr i32 %calcOffset36 to double*
  store double %_local_6.1, double* %47, align 8
  %48 = call i32 @fprintf(i32 %5, i32 1561, i32 %calcOffset36)
  %calcOffset37 = add i32 %_param_0.1, 20
  %49 = inttoptr i32 %calcOffset37 to i32*
  %50 = load i32, i32* %49, align 4
  %select_cond38.not = icmp eq i32 %50, 0
  %51 = select i1 %select_cond38.not, i32 1492, i32 %50
  %52 = inttoptr i32 %1 to i32*
  store i32 %51, i32* %52, align 4
  %53 = call i32 @fprintf(i32 %5, i32 1190, i32 %1)
  %54 = call i32 @putc(i32 10, i32 %5)
  %calcOffset41 = add i32 %_param_0.1, 68
  %55 = inttoptr i32 %calcOffset41 to i32*
  %56 = load i32, i32* %55, align 4
  %57 = icmp eq i32 %56, 0
  br i1 %57, label %blk_exit40, label %brif_next43

blk_exit28:                                       ; preds = %loop_entry24
  %58 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %brif_val33.not = icmp eq i8 %58, 0
  br i1 %brif_val33.not, label %brif_next34, label %blk_exit32

brif_next31:                                      ; preds = %loop_entry24
  %59 = sitofp i32 %45 to double
  br label %blk_exit27

blk_exit32:                                       ; preds = %blk_exit28
  %60 = load i32, i32* inttoptr (i32 3356 to i32*), align 4
  %61 = sitofp i32 %60 to double
  %62 = fdiv double 1.000000e+02, %61
  br label %blk_exit27

brif_next34:                                      ; preds = %blk_exit28
  %63 = load i32, i32* inttoptr (i32 3332 to i32*), align 4
  %64 = sitofp i32 %63 to double
  %65 = fsub double 1.000000e+02, %64
  %66 = add i32 %_param_0.1, 44
  %67 = inttoptr i32 %66 to i32*
  %68 = load i32, i32* %67, align 4
  %69 = uitofp i32 %68 to double
  %70 = fmul double %65, %69
  %71 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %72 = uitofp i32 %71 to double
  %73 = fdiv double %70, %72
  br label %blk_exit27

blk_exit40:                                       ; preds = %brif_next43, %blk_exit27
  %calcOffset44 = add i32 %_param_0.1, 76
  %74 = inttoptr i32 %calcOffset44 to i32*
  %75 = load i32, i32* %74, align 4
  %brif_val45.not = icmp eq i32 %75, 0
  br i1 %brif_val45.not, label %blk_exit, label %loop_entry24

brif_next43:                                      ; preds = %blk_exit27
  call void @print_list(i32 %56, i32 %3)
  br label %blk_exit40
}

; Function Attrs: null_pointer_is_valid
define internal void @free_desc(i32 %_arg_0) #0 {
allocator:
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %blk_exit5, %allocator
  ret void

loop_entry:                                       ; preds = %blk_exit5, %allocator
  %_param_0.0 = phi i32 [ %_arg_0, %allocator ], [ %15, %blk_exit5 ]
  %calcOffset = add i32 %_param_0.0, 68
  %1 = inttoptr i32 %calcOffset to i32*
  %2 = load i32, i32* %1, align 4
  call void @free_desc(i32 %2)
  %calcOffset1 = add i32 %_param_0.0, 28
  %3 = inttoptr i32 %calcOffset1 to i32*
  %4 = load i32, i32* %3, align 4
  call void @free(i32 %4)
  %calcOffset2 = add i32 %_param_0.0, 32
  %5 = inttoptr i32 %calcOffset2 to i32*
  %6 = load i32, i32* %5, align 4
  call void @free(i32 %6)
  %calcOffset3 = add i32 %_param_0.0, 20
  %7 = inttoptr i32 %calcOffset3 to i32*
  %8 = load i32, i32* %7, align 4
  call void @free(i32 %8)
  %calcOffset4 = add i32 %_param_0.0, 24
  %9 = inttoptr i32 %calcOffset4 to i32*
  %10 = load i32, i32* %9, align 4
  call void @free(i32 %10)
  %calcOffset6 = add i32 %_param_0.0, 16
  %11 = inttoptr i32 %calcOffset6 to i32*
  %12 = load i32, i32* %11, align 4
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %blk_exit5, label %brif_next8

blk_exit5:                                        ; preds = %brif_next8, %loop_entry
  %calcOffset10 = add i32 %_param_0.0, 76
  %14 = inttoptr i32 %calcOffset10 to i32*
  %15 = load i32, i32* %14, align 4
  call void @free(i32 %_param_0.0)
  %brif_val11.not = icmp eq i32 %15, 0
  br i1 %brif_val11.not, label %blk_exit, label %loop_entry

brif_next8:                                       ; preds = %loop_entry
  %16 = call i32 @fclose(i32 %12)
  store i32 0, i32* %11, align 4
  br label %blk_exit5
}

; Function Attrs: null_pointer_is_valid
define internal i32 @names_compare(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = inttoptr i32 %_arg_0 to i32*
  %1 = load i32, i32* %0, align 4
  %2 = inttoptr i32 %_arg_1 to i32*
  %3 = load i32, i32* %2, align 4
  %4 = call i32 @strcmp(i32 %1, i32 %3)
  ret i32 %4
}

; Function Attrs: null_pointer_is_valid
declare i32 @__main_void() #0

attributes #0 = { null_pointer_is_valid }
