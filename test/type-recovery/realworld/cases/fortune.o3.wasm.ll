; ModuleID = 'test/lifting/wasm/cases/fortune.o3.wasm'
source_filename = "test/lifting/wasm/cases/fortune.o3.wasm"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

@__stack_pointer = internal global i32 68912, !notdec.stackpointer !0
@__notdec_mem0 = global [131072 x i8] zeroinitializer
@__notdec_mem0_0x400 = internal constant [1183 x i8] c"acefhilm:n:osuvw\00fortune: bad juju in is_existant\00%s.dat\00get_pos\00/usr/local/share/games/fortunes\00/usr/local/local/share/games/fortunes\00ins,pas\00%*s\00%s/%s/%s\00%s version %s\00r\00p\00o\00ins.ftn\00/dev/urandom\00sml\00all\00fp->read_tbl\00i\00h\00/usr/local/share/games/fortunes/off\00/usr/local/local/share/games/fortunes/off\00fortune-mod\00ins.c\00/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c\00rb\00fortune [-a\00sw]\00 [-m pattern]\00FORTUNE_MOD_RAND_HARD_CODED_VALS\00LC_MESSAGES\00NULL\00LC_ALL\00LANG\00LANGUAGE\00PWD\00FORTUNE_MOD_USE_SRAND\00%s.u8\003.24.0\00../\00%5.2f%%\00Out of RAM!\00fortune:%s not a fortune file or directory\0A\00percentages must be integers\0A\00fortune: no probability left to put in residual files\0A\00percentages must precede files\0A\00bad pattern: %s\0A\00pattern too long for ignoring case\0A\00No fortunes found\0A\00fortune: no fortune found\0A\00fortune: %s corrupted\0A\00(%s)\0A%c\0A\00 [-n number] [ [#%] file/directory/all]\0A\00percentages must be <= 100\0A\00fortune: %s: No fortune files in directory.\0A\00fortune: out of memory.\0A\00fortune: no place to put residual probability (%d%%)\0A\00(%s)\0A%%\0A\00pattern too long for ignoring case; overflow!\0A\00fortune: probabilities sum to %d%%!\0A\00Overflow percentage detected at argument \22%s\22!\0A\00", section ".addr_0x400", align 1
@__notdec_mem0_0x8a0 = internal global [4 x i8] c"\A0\00\00\00", section ".addr_0x8a0", align 1
@table_0 = global [2 x ptr] [ptr null, ptr @names_compare]

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
define dso_local i32 @main(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %stack = alloca [2960 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint ptr %stack to i32
  %stack_end = add i32 %stack_addr, -2960
  %0 = call i32 @getenv(i32 1497)
  store i32 %0, ptr inttoptr (i32 2224 to ptr), align 4
  %brif_val.not = icmp eq i32 %0, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

brif_next:                                        ; preds = %allocator
  %1 = call i32 @getenv(i32 1480)
  store i32 %1, ptr inttoptr (i32 2224 to ptr), align 4
  %brif_val1.not = icmp eq i32 %1, 0
  br i1 %brif_val1.not, label %brif_next2, label %blk_exit

brif_next2:                                       ; preds = %brif_next
  %2 = call i32 @getenv(i32 1509)
  store i32 %2, ptr inttoptr (i32 2224 to ptr), align 4
  %brif_val3.not = icmp eq i32 %2, 0
  br i1 %brif_val3.not, label %brif_next4, label %blk_exit

brif_next4:                                       ; preds = %brif_next2
  %3 = call i32 @getenv(i32 1504)
  store i32 %3, ptr inttoptr (i32 2224 to ptr), align 4
  br label %blk_exit

blk_exit:                                         ; preds = %brif_next4, %brif_next2, %brif_next, %allocator
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %blk_exit7, %blk_exit9, %blk_exit10, %blk_exit11, %blk_exit12, %blk_exit13, %blk_exit14, %blk_exit15, %blk_exit16, %blk_exit17, %blk_exit18, %blk_exit
  %_local_4.0 = phi i32 [ 0, %blk_exit ], [ %_local_4.0, %blk_exit11 ], [ %_local_4.0, %blk_exit9 ], [ %_local_4.0, %blk_exit12 ], [ %_local_4.0, %blk_exit13 ], [ %_local_4.0, %blk_exit14 ], [ %826, %blk_exit10 ], [ %_local_4.0, %blk_exit15 ], [ %_local_4.0, %loop_entry ], [ %_local_4.0, %blk_exit16 ], [ %_local_4.0, %blk_exit17 ], [ %_local_4.0, %blk_exit7 ], [ %_local_4.0, %blk_exit18 ]
  %_local_3.0 = phi i32 [ 0, %blk_exit ], [ %_local_3.0, %blk_exit11 ], [ %_local_3.0, %blk_exit9 ], [ %_local_3.0, %blk_exit12 ], [ %_local_3.0, %blk_exit13 ], [ %_local_3.0, %blk_exit14 ], [ %_local_3.0, %blk_exit10 ], [ %_local_3.0, %blk_exit15 ], [ 1, %loop_entry ], [ %_local_3.0, %blk_exit16 ], [ %_local_3.0, %blk_exit17 ], [ %_local_3.0, %blk_exit7 ], [ %_local_3.0, %blk_exit18 ]
  %4 = call i32 @getopt(i32 %_arg_0, i32 %_arg_1, i32 1024)
  switch i32 %4, label %blk_exit6 [
    i32 -1, label %blk_exit5
    i32 119, label %blk_exit11
    i32 118, label %blk_exit8
    i32 117, label %blk_exit9
    i32 115, label %blk_exit12
    i32 111, label %blk_exit13
    i32 110, label %blk_exit14
    i32 109, label %blk_exit10
    i32 108, label %blk_exit15
    i32 105, label %loop_entry
    i32 102, label %blk_exit16
    i32 101, label %blk_exit17
    i32 99, label %blk_exit7
    i32 97, label %blk_exit18
  ]

blk_exit6:                                        ; preds = %loop_entry
  call void @usage()
  unreachable

blk_exit5:                                        ; preds = %loop_entry
  %5 = load i32, ptr null, align 4
  %6 = sub i32 %_arg_0, %5
  %7 = icmp eq i32 %_arg_0, %5
  br i1 %7, label %blk_exit43, label %brif_next45

blk_exit43:                                       ; preds = %blk_exit5
  %8 = load i8, ptr inttoptr (i32 2292 to ptr), align 1
  %9 = icmp eq i8 %8, 0
  br i1 %9, label %blk_exit219, label %brif_next221

blk_exit219:                                      ; preds = %blk_exit43
  %10 = load i8, ptr inttoptr (i32 2293 to ptr), align 1
  %11 = icmp eq i8 %10, 0
  br i1 %11, label %blk_exit228, label %brif_next230

blk_exit228:                                      ; preds = %blk_exit219
  %12 = load i32, ptr inttoptr (i32 2224 to ptr), align 4
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %blk_exit234, label %brif_next236

blk_exit234:                                      ; preds = %blk_exit228
  %14 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %15 = call i32 @strcmp(i32 1089, i32 1121)
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %blk_exit218, label %brif_next260

brif_next260:                                     ; preds = %blk_exit234
  %17 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  %18 = or i32 %17, %14
  br label %blk_exit218

brif_next236:                                     ; preds = %blk_exit228
  %19 = add i32 %stack_addr, -2560
  %20 = call i32 @strncpy(i32 %19, i32 %12, i32 512)
  %calcOffset237 = add i32 %stack_addr, -2049
  %21 = inttoptr i32 %calcOffset237 to ptr
  store i8 0, ptr %21, align 1
  br label %loop_entry239

loop_entry239:                                    ; preds = %brif_next251, %brif_next236
  %_local_3.6 = phi i32 [ %19, %brif_next236 ], [ %storemerge772, %brif_next251 ]
  %22 = icmp eq i32 %_local_3.6, 0
  br i1 %22, label %blk_exit238, label %brif_next241

blk_exit238:                                      ; preds = %brif_next241, %loop_entry239
  %23 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %24 = call i32 @strcmp(i32 1089, i32 1121)
  %25 = icmp eq i32 %24, 0
  br i1 %25, label %blk_exit218, label %brif_next257

brif_next257:                                     ; preds = %blk_exit238
  %26 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  %27 = or i32 %26, %23
  br label %blk_exit218

brif_next241:                                     ; preds = %loop_entry239
  %28 = inttoptr i32 %_local_3.6 to ptr
  %29 = load i8, ptr %28, align 1
  %30 = icmp eq i8 %29, 0
  br i1 %30, label %blk_exit238, label %brif_next244

brif_next244:                                     ; preds = %brif_next241
  %31 = call i32 @strchr(i32 %_local_3.6, i32 58)
  %brif_val247.not = icmp eq i32 %31, 0
  br i1 %brif_val247.not, label %blk_exit245, label %blk_exit246

blk_exit245:                                      ; preds = %brif_next244, %blk_exit246
  %storemerge772 = phi i32 [ %39, %blk_exit246 ], [ 0, %brif_next244 ]
  %32 = call i32 @add_file(i32 -1, i32 %_local_3.6, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val250.not = icmp eq i32 %32, 0
  br i1 %brif_val250.not, label %brif_next251, label %blk_exit218

brif_next251:                                     ; preds = %blk_exit245
  %33 = add i32 %stack_addr, -2564
  %34 = call i32 @strncpy(i32 %33, i32 %_local_3.6, i32 2)
  %calcOffset252 = add i32 %stack_addr, -2562
  %35 = inttoptr i32 %calcOffset252 to ptr
  store i8 0, ptr %35, align 1
  %36 = call i32 @add_file(i32 -1, i32 %33, i32 0, i32 2268, i32 3328, i32 0)
  %37 = icmp eq i32 %36, 0
  br i1 %37, label %loop_entry239, label %blk_exit218

blk_exit246:                                      ; preds = %brif_next244
  %38 = inttoptr i32 %31 to ptr
  store i8 0, ptr %38, align 1
  %39 = add i32 %31, 1
  br label %blk_exit245

brif_next230:                                     ; preds = %blk_exit219
  %40 = call i32 @add_file(i32 -1, i32 1282, i32 0, i32 2268, i32 3328, i32 0)
  %41 = call i32 @strcmp(i32 1246, i32 1282)
  %42 = icmp eq i32 %41, 0
  br i1 %42, label %blk_exit218, label %brif_next233

brif_next233:                                     ; preds = %brif_next230
  %43 = call i32 @add_file(i32 -1, i32 1246, i32 0, i32 2268, i32 3328, i32 0)
  %44 = or i32 %43, %40
  br label %blk_exit218

blk_exit218:                                      ; preds = %blk_exit234, %brif_next260, %blk_exit238, %brif_next257, %brif_next230, %brif_next233, %brif_next251, %blk_exit245, %blk_exit225
  %_param_0.2 = phi i32 [ %27, %brif_next257 ], [ %44, %brif_next233 ], [ %36, %brif_next251 ], [ %32, %blk_exit245 ], [ %54, %blk_exit225 ], [ %40, %brif_next230 ], [ %23, %blk_exit238 ], [ %14, %blk_exit234 ], [ %18, %brif_next260 ]
  %45 = icmp eq i32 %_param_0.2, 0
  br i1 %45, label %blk_exit41, label %blk_exit42

brif_next221:                                     ; preds = %blk_exit43
  %46 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %47 = call i32 @add_file(i32 -1, i32 1282, i32 0, i32 2268, i32 3328, i32 0)
  %48 = or i32 %46, %47
  %49 = call i32 @strcmp(i32 1089, i32 1121)
  %50 = icmp eq i32 %49, 0
  br i1 %50, label %blk_exit222, label %brif_next224

blk_exit222:                                      ; preds = %brif_next224, %brif_next221
  %_param_0.3 = phi i32 [ 0, %brif_next221 ], [ %56, %brif_next224 ]
  %51 = or i32 %48, %_param_0.3
  %52 = call i32 @strcmp(i32 1246, i32 1282)
  %53 = icmp eq i32 %52, 0
  br i1 %53, label %blk_exit225, label %brif_next227

blk_exit225:                                      ; preds = %brif_next227, %blk_exit222
  %_local_3.4 = phi i32 [ 0, %blk_exit222 ], [ %55, %brif_next227 ]
  %54 = or i32 %51, %_local_3.4
  br label %blk_exit218

brif_next227:                                     ; preds = %blk_exit222
  %55 = call i32 @add_file(i32 -1, i32 1246, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit225

brif_next224:                                     ; preds = %brif_next221
  %56 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit222

blk_exit41:                                       ; preds = %blk_exit203, %blk_exit218, %blk_exit49, %blk_exit111
  %57 = load i8, ptr inttoptr (i32 2296 to ptr), align 1
  %brif_val271.not = icmp eq i8 %57, 0
  br i1 %brif_val271.not, label %brif_next272, label %blk_exit270

brif_next272:                                     ; preds = %blk_exit41
  %58 = inttoptr i32 %stack_end to ptr
  store i32 1795, ptr %58, align 4
  %59 = load i32, ptr null, align 4
  %60 = call i32 @fprintf(i32 %59, i32 1191, i32 %stack_end)
  br label %blk_exit270

blk_exit270:                                      ; preds = %brif_next272, %blk_exit41
  call void @exit(i32 1) #3
  unreachable

blk_exit42:                                       ; preds = %blk_exit218, %blk_exit110, %brif_next45
  %61 = icmp eq i32 %_local_4.0, 0
  br i1 %61, label %blk_exit35, label %brif_next264

blk_exit35:                                       ; preds = %blk_exit36, %blk_exit38, %blk_exit42
  %62 = call i32 @setlocale(i32 6, i32 2206)
  %63 = load i8, ptr inttoptr (i32 2228 to ptr), align 1
  %64 = icmp eq i8 %63, 0
  br i1 %64, label %blk_exit314, label %brif_next316

blk_exit314:                                      ; preds = %blk_exit35
  %65 = load i32, ptr inttoptr (i32 3328 to ptr), align 4
  %66 = icmp eq i32 %65, 0
  br i1 %66, label %blk_exit321, label %brif_next323

blk_exit321:                                      ; preds = %brif_next331, %blk_exit314
  %_local_5.3 = phi i32 [ 0, %blk_exit314 ], [ %560, %brif_next331 ]
  %_local_4.2 = phi i32 [ 0, %blk_exit314 ], [ %561, %brif_next331 ]
  %_param_1.4 = phi i32 [ 0, %blk_exit314 ], [ %563, %brif_next331 ]
  %67 = icmp ne i32 %_param_1.4, 100
  %68 = icmp eq i32 %_local_4.2, 0
  %or.cond808 = select i1 %67, i1 %68, i1 false
  br i1 %or.cond808, label %blk_exit31, label %blk_exit334

blk_exit31:                                       ; preds = %blk_exit321
  %calcOffset764 = add i32 %stack_addr, -2928
  %69 = inttoptr i32 %calcOffset764 to ptr
  store i32 %_param_1.4, ptr %69, align 4
  %70 = load i32, ptr null, align 4
  %71 = call i32 @fprintf(i32 %70, i32 2012, i32 %calcOffset764)
  call void @exit(i32 1) #3
  unreachable

blk_exit334:                                      ; preds = %blk_exit321
  %or.cond809 = select i1 %67, i1 true, i1 %68
  br i1 %or.cond809, label %blk_exit339, label %blk_exit30

blk_exit339:                                      ; preds = %blk_exit334
  store i32 %_param_1.4, ptr inttoptr (i32 3332 to ptr), align 4
  %72 = load i8, ptr inttoptr (i32 2272 to ptr), align 1
  %73 = icmp eq i8 %72, 0
  %or.cond = select i1 %73, i1 true, i1 %68
  br i1 %or.cond, label %blk_exit344, label %brif_next348

blk_exit344:                                      ; preds = %blk_exit349, %blk_exit339
  %74 = load i8, ptr inttoptr (i32 2264 to ptr), align 1
  %75 = icmp eq i8 %74, 0
  br i1 %75, label %blk_exit364, label %brif_next366

blk_exit364:                                      ; preds = %blk_exit344
  %76 = call i64 @time(i32 0)
  %77 = call i32 @getpid()
  %78 = trunc i64 %76 to i32
  %79 = add i32 %77, %78
  call void @srandom(i32 %79)
  br label %loop_entry404

loop_entry404:                                    ; preds = %blk_exit639, %blk_exit605, %blk_exit364
  %80 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  %81 = inttoptr i32 %80 to ptr
  %82 = load i32, ptr %81, align 4
  %calcOffset409 = add i32 %80, 76
  %83 = inttoptr i32 %calcOffset409 to ptr
  %84 = load i32, ptr %83, align 4
  %85 = icmp eq i32 %84, 0
  br i1 %85, label %blk_exit408, label %brif_next411

blk_exit408:                                      ; preds = %blk_exit427, %blk_exit415, %brif_next438, %loop_entry434, %loop_entry404
  %_local_5.6 = phi i32 [ %82, %loop_entry404 ], [ %504, %brif_next438 ], [ -1, %blk_exit415 ], [ -1, %loop_entry434 ], [ %500, %blk_exit427 ]
  %_local_3.21 = phi i32 [ %80, %loop_entry404 ], [ %502, %brif_next438 ], [ %498, %blk_exit415 ], [ %502, %loop_entry434 ], [ %498, %blk_exit427 ]
  %.not784 = icmp eq i32 %_local_5.6, -1
  br i1 %.not784, label %brif_next442, label %blk_exit406

brif_next442:                                     ; preds = %blk_exit408
  %calcOffset443 = add i32 %_local_3.21, 76
  %86 = inttoptr i32 %calcOffset443 to ptr
  %87 = load i32, ptr %86, align 4
  %88 = icmp eq i32 %87, 0
  br i1 %88, label %blk_exit406, label %blk_exit407

blk_exit406:                                      ; preds = %loop_entry481, %blk_exit466, %brif_next442, %blk_exit408
  %_local_3.19 = phi i32 [ %_local_3.21, %brif_next442 ], [ %_local_3.20, %blk_exit466 ], [ %475, %loop_entry481 ], [ %_local_3.21, %blk_exit408 ]
  call void @get_tbl(i32 %_local_3.19)
  %89 = add i32 %_local_3.19, 44
  %90 = inttoptr i32 %89 to ptr
  %91 = load i32, ptr %90, align 4
  %92 = icmp eq i32 %91, 0
  br i1 %92, label %blk_exit29, label %brif_next488

blk_exit29:                                       ; preds = %blk_exit406
  %calcOffset765 = add i32 %stack_addr, -2912
  %93 = inttoptr i32 %calcOffset765 to ptr
  store i32 1814, ptr %93, align 4
  %94 = load i32, ptr null, align 4
  %95 = call i32 @fprintf(i32 %94, i32 1191, i32 %calcOffset765)
  call void @exit(i32 1) #3
  unreachable

brif_next488:                                     ; preds = %blk_exit406
  %calcOffset490 = add i32 %_local_3.19, 68
  %96 = inttoptr i32 %calcOffset490 to ptr
  %97 = load i32, ptr %96, align 4
  %98 = icmp eq i32 %97, 0
  br i1 %98, label %blk_exit489, label %loop_entry493

blk_exit489:                                      ; preds = %blk_exit494, %brif_next488
  %_local_3.24 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  store i32 %_local_3.24, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset561 = add i32 %_local_3.24, 36
  %99 = inttoptr i32 %calcOffset561 to ptr
  %100 = load i8, ptr %99, align 1
  %101 = icmp eq i8 %100, 0
  br i1 %101, label %blk_exit28, label %brif_next563

blk_exit28:                                       ; preds = %blk_exit489
  call void @__assert_fail(i32 1229, i32 1342, i32 1621, i32 1081)
  unreachable

brif_next563:                                     ; preds = %blk_exit489
  %calcOffset565 = add i32 %_local_3.24, 12
  %102 = inttoptr i32 %calcOffset565 to ptr
  %103 = load i32, ptr %102, align 4
  %.not793 = icmp eq i32 %103, -1
  br i1 %.not793, label %brif_next567, label %brif_next563.blk_exit564_crit_edge

brif_next567:                                     ; preds = %brif_next563
  %104 = add i32 %_local_3.24, 44
  %105 = inttoptr i32 %104 to ptr
  %106 = load i32, ptr %105, align 4
  %calcOffset569 = add i32 %stack_addr, -1024
  %107 = inttoptr i32 %calcOffset569 to ptr
  %108 = ptrtoint ptr %stack to i32
  %109 = add i32 %108, -1024
  %110 = inttoptr i32 %109 to ptr
  call void @llvm.memset.p0.i64(ptr %110, i8 0, i64 8, i1 true)
  %111 = call i32 @getenv(i32 1447)
  %112 = icmp eq i32 %111, 0
  br i1 %112, label %blk_exit570, label %brif_next572

blk_exit570:                                      ; preds = %brif_next567
  %113 = call i32 @getenv(i32 1522)
  %brif_val574.not = icmp eq i32 %113, 0
  br i1 %brif_val574.not, label %brif_next575, label %blk_exit573

brif_next575:                                     ; preds = %blk_exit570
  %114 = call i32 @fopen(i32 1208, i32 1414)
  %115 = icmp eq i32 %114, 0
  br i1 %115, label %blk_exit573, label %brif_next577

brif_next577:                                     ; preds = %brif_next575
  %116 = call i32 @fread(i32 %calcOffset569, i32 8, i32 1, i32 %114)
  %117 = call i32 @fclose(i32 %114)
  %.not794 = icmp eq i32 %116, 1
  br i1 %.not794, label %brif_next579, label %blk_exit573

brif_next579:                                     ; preds = %brif_next577
  %118 = load i64, ptr %107, align 8
  %119 = zext i32 %106 to i64
  %120 = urem i64 %118, %119
  %121 = trunc nuw i64 %120 to i32
  br label %blk_exit564

blk_exit573:                                      ; preds = %brif_next577, %brif_next575, %blk_exit570
  %122 = call i32 @random()
  %123 = urem i32 %122, %106
  br label %blk_exit564

brif_next572:                                     ; preds = %brif_next567
  %124 = call i32 @atol(i32 %111)
  %125 = urem i32 %124, %106
  br label %blk_exit564

blk_exit564:                                      ; preds = %brif_next563.blk_exit564_crit_edge, %blk_exit573, %brif_next579, %brif_next572
  %.pre-phi813 = phi ptr [ %.pre812, %brif_next563.blk_exit564_crit_edge ], [ %105, %blk_exit573 ], [ %105, %brif_next579 ], [ %105, %brif_next572 ]
  %_local_5.16 = phi i32 [ %103, %brif_next563.blk_exit564_crit_edge ], [ %123, %blk_exit573 ], [ %121, %brif_next579 ], [ %125, %brif_next572 ]
  %126 = add i32 %_local_5.16, 1
  store i32 %126, ptr %102, align 4
  %127 = load i32, ptr %.pre-phi813, align 4
  %128 = icmp slt i32 %126, %127
  br i1 %128, label %blk_exit582, label %brif_next585

blk_exit582:                                      ; preds = %brif_next585, %blk_exit564
  %_local_5.17 = phi i32 [ %126, %blk_exit564 ], [ %346, %brif_next585 ]
  %calcOffset588 = add i32 %_local_3.24, 8
  %129 = inttoptr i32 %calcOffset588 to ptr
  %130 = load i32, ptr %129, align 4
  %131 = icmp sgt i32 %130, -1
  br i1 %131, label %blk_exit587, label %brif_next590

blk_exit587:                                      ; preds = %brif_next594, %blk_exit582
  %_local_5.18 = phi i32 [ %_local_5.17, %blk_exit582 ], [ %345, %brif_next594 ]
  %_param_0.11 = phi i32 [ %130, %blk_exit582 ], [ %344, %brif_next594 ]
  %132 = shl i32 %_local_5.18, 2
  %133 = add i32 %132, 24
  %134 = zext i32 %133 to i64
  %135 = call i64 @lseek(i32 %_param_0.11, i64 %134, i32 0)
  %136 = load i32, ptr %129, align 4
  %137 = call i32 @read(i32 %136, i32 3360, i32 4)
  %brif_val597.not = icmp sgt i32 %137, -1
  br i1 %brif_val597.not, label %brif_next598, label %blk_exit26

brif_next598:                                     ; preds = %blk_exit587
  %138 = load i32, ptr %129, align 4
  %139 = call i32 @read(i32 %138, i32 3364, i32 4)
  %brif_val600.not = icmp sgt i32 %139, -1
  br i1 %brif_val600.not, label %brif_next601, label %blk_exit26

brif_next601:                                     ; preds = %brif_next598
  %140 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %141 = call i32 @ntohl(i32 %140)
  store i32 %141, ptr inttoptr (i32 3360 to ptr), align 4
  %142 = load i32, ptr inttoptr (i32 3364 to ptr), align 4
  %143 = call i32 @ntohl(i32 %142)
  store i32 %143, ptr inttoptr (i32 3364 to ptr), align 4
  %144 = load i8, ptr inttoptr (i32 2273 to ptr), align 1
  %145 = icmp eq i8 %144, 0
  br i1 %145, label %blk_exit602, label %brif_next604

blk_exit602:                                      ; preds = %blk_exit605, %brif_next601
  %146 = load i8, ptr inttoptr (i32 2274 to ptr), align 1
  %147 = icmp eq i8 %146, 0
  br i1 %147, label %blk_exit636, label %brif_next638

blk_exit636:                                      ; preds = %blk_exit639, %blk_exit602
  %148 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset671 = add i32 %148, 16
  %149 = inttoptr i32 %calcOffset671 to ptr
  %150 = load i32, ptr %149, align 4
  %brif_val672.not = icmp eq i32 %150, 0
  br i1 %brif_val672.not, label %brif_next673, label %blk_exit670

brif_next673:                                     ; preds = %blk_exit636
  %calcOffset674 = add i32 %148, 4
  %151 = inttoptr i32 %calcOffset674 to ptr
  %152 = load i32, ptr %151, align 4
  %153 = call i32 @fdopen(i32 %152, i32 1194)
  store i32 %153, ptr %149, align 4
  %154 = icmp eq i32 %153, 0
  br i1 %154, label %blk_exit23, label %blk_exit670

blk_exit23:                                       ; preds = %brif_next673
  %calcOffset768 = add i32 %148, 24
  %155 = inttoptr i32 %calcOffset768 to ptr
  %156 = load i32, ptr %155, align 4
  call void @perror(i32 %156)
  call void @exit(i32 1) #3
  unreachable

blk_exit670:                                      ; preds = %brif_next673, %blk_exit636
  %_local_3.35 = phi i32 [ %153, %brif_next673 ], [ %150, %blk_exit636 ]
  %157 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %158 = call i32 @fseek(i32 %_local_3.35, i32 %157, i32 0)
  %159 = load i8, ptr inttoptr (i32 2295 to ptr), align 1
  %160 = icmp eq i8 %159, 0
  br i1 %160, label %blk_exit678, label %brif_next680

blk_exit678:                                      ; preds = %brif_next680, %blk_exit670
  store i32 0, ptr inttoptr (i32 2284 to ptr), align 4
  %161 = add i32 %stack_addr, -1024
  %162 = load i32, ptr %149, align 4
  %163 = call i32 @fgets(i32 %161, i32 1024, i32 %162)
  %brif_val686.not = icmp eq i32 %163, 0
  br i1 %brif_val686.not, label %brif_next687, label %blk_exit684

brif_next687:                                     ; preds = %blk_exit678
  %164 = load i32, ptr null, align 4
  br label %blk_exit683

blk_exit683:                                      ; preds = %blk_exit719, %brif_next693, %brif_next687
  %_local_11.1 = phi i32 [ %164, %brif_next687 ], [ %215, %brif_next693 ], [ %215, %blk_exit719 ]
  %165 = call i32 @fflush(i32 %_local_11.1)
  %166 = load i8, ptr inttoptr (i32 2280 to ptr), align 1
  %167 = icmp eq i8 %166, 0
  br i1 %167, label %blk_exit19, label %brif_next730

brif_next730:                                     ; preds = %blk_exit683
  %168 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %169 = add i32 %168, 56
  %170 = inttoptr i32 %169 to ptr
  %171 = load i8, ptr %170, align 1
  %172 = and i8 %171, 3
  %brif_val734.not = icmp eq i8 %172, 0
  br i1 %brif_val734.not, label %brif_next735, label %blk_exit732

brif_next735:                                     ; preds = %brif_next730
  %173 = load i32, ptr inttoptr (i32 3364 to ptr), align 4
  %174 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %175 = sub i32 %173, %174
  %176 = add i32 %175, -2
  br label %blk_exit731

blk_exit731:                                      ; preds = %blk_exit749, %brif_next753, %blk_exit736, %brif_next735
  %_local_3.37 = phi i32 [ %176, %brif_next735 ], [ 0, %blk_exit736 ], [ %_local_3.38, %brif_next753 ], [ %211, %blk_exit749 ]
  store i32 %_local_3.37, ptr inttoptr (i32 2284 to ptr), align 4
  %177 = sdiv i32 %_local_3.37, 20
  %178 = icmp sgt i32 %_local_3.37, 139
  %179 = select i1 %178, i32 %177, i32 6
  %180 = call i32 @sleep(i32 %179)
  br label %blk_exit19

blk_exit732:                                      ; preds = %brif_next730
  %calcOffset737 = add i32 %168, 16
  %181 = inttoptr i32 %calcOffset737 to ptr
  %182 = load i32, ptr %181, align 4
  %brif_val738.not = icmp eq i32 %182, 0
  br i1 %brif_val738.not, label %brif_next739, label %blk_exit736

brif_next739:                                     ; preds = %blk_exit732
  %calcOffset740 = add i32 %168, 4
  %183 = inttoptr i32 %calcOffset740 to ptr
  %184 = load i32, ptr %183, align 4
  %185 = call i32 @fdopen(i32 %184, i32 1194)
  store i32 %185, ptr %181, align 4
  %186 = icmp eq i32 %185, 0
  br i1 %186, label %blk_exit22, label %brif_next743

blk_exit22:                                       ; preds = %brif_next739
  %calcOffset769 = add i32 %168, 24
  %187 = inttoptr i32 %calcOffset769 to ptr
  %188 = load i32, ptr %187, align 4
  call void @perror(i32 %188)
  call void @exit(i32 1) #3
  unreachable

brif_next743:                                     ; preds = %brif_next739
  %189 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset744 = add i32 %189, 16
  %190 = inttoptr i32 %calcOffset744 to ptr
  %191 = load i32, ptr %190, align 4
  br label %blk_exit736

blk_exit736:                                      ; preds = %brif_next743, %blk_exit732
  %_local_5.21 = phi i32 [ %191, %brif_next743 ], [ %182, %blk_exit732 ]
  %192 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %193 = call i32 @fseek(i32 %_local_5.21, i32 %192, i32 0)
  %194 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset745 = add i32 %194, 16
  %195 = inttoptr i32 %calcOffset745 to ptr
  %196 = load i32, ptr %195, align 4
  %197 = call i32 @fgets(i32 %161, i32 1024, i32 %196)
  %198 = icmp eq i32 %197, 0
  br i1 %198, label %blk_exit731, label %loop_entry748

loop_entry748:                                    ; preds = %blk_exit736, %blk_exit749
  %_local_3.38 = phi i32 [ %211, %blk_exit749 ], [ 0, %blk_exit736 ]
  %199 = inttoptr i32 %161 to ptr
  %200 = load i8, ptr %199, align 1
  %201 = sext i8 %200 to i32
  %202 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %203 = add i32 %202, 60
  %204 = inttoptr i32 %203 to ptr
  %205 = load i8, ptr %204, align 1
  %206 = zext i8 %205 to i32
  %.not801 = icmp eq i32 %201, %206
  br i1 %.not801, label %brif_next753, label %blk_exit749

brif_next753:                                     ; preds = %loop_entry748
  %calcOffset754 = add i32 %stack_addr, -1023
  %207 = inttoptr i32 %calcOffset754 to ptr
  %208 = load i8, ptr %207, align 1
  %209 = icmp eq i8 %208, 10
  br i1 %209, label %blk_exit731, label %blk_exit749

blk_exit749:                                      ; preds = %brif_next753, %loop_entry748
  %210 = call i32 @strlen(i32 %161)
  %211 = add i32 %210, %_local_3.38
  %calcOffset757 = add i32 %202, 16
  %212 = inttoptr i32 %calcOffset757 to ptr
  %213 = load i32, ptr %212, align 4
  %214 = call i32 @fgets(i32 %161, i32 1024, i32 %213)
  %brif_val758.not = icmp eq i32 %214, 0
  br i1 %brif_val758.not, label %blk_exit731, label %loop_entry748

blk_exit684:                                      ; preds = %blk_exit678
  %215 = load i32, ptr null, align 4
  br label %loop_entry688

loop_entry688:                                    ; preds = %blk_exit719, %blk_exit684
  %216 = inttoptr i32 %161 to ptr
  %217 = load i8, ptr %216, align 1
  %calcOffset691 = add i32 %148, 60
  %218 = inttoptr i32 %calcOffset691 to ptr
  %219 = load i8, ptr %218, align 1
  %.not798 = icmp eq i8 %217, %219
  br i1 %.not798, label %brif_next693, label %blk_exit689

brif_next693:                                     ; preds = %loop_entry688
  %calcOffset694 = add i32 %stack_addr, -1023
  %220 = inttoptr i32 %calcOffset694 to ptr
  %221 = load i8, ptr %220, align 1
  %222 = icmp eq i8 %221, 10
  br i1 %222, label %blk_exit683, label %blk_exit689

blk_exit689:                                      ; preds = %brif_next693, %loop_entry688
  %calcOffset698 = add i32 %148, 56
  %223 = inttoptr i32 %calcOffset698 to ptr
  %224 = load i8, ptr %223, align 1
  %225 = and i8 %224, 4
  %226 = icmp eq i8 %225, 0
  %227 = icmp eq i8 %217, 0
  %or.cond804 = or i1 %226, %227
  br i1 %or.cond804, label %blk_exit697, label %loop_entry703

blk_exit697:                                      ; preds = %blk_exit704, %blk_exit689
  %calcOffset721 = add i32 %148, 38
  %228 = inttoptr i32 %calcOffset721 to ptr
  %229 = load i8, ptr %228, align 1
  %230 = icmp eq i8 %229, 0
  br i1 %230, label %blk_exit720, label %brif_next723

blk_exit720:                                      ; preds = %brif_next723, %blk_exit697
  %231 = call i32 @fputs(i32 %161, i32 %215)
  br label %blk_exit719

blk_exit719:                                      ; preds = %blk_exit720, %brif_next725
  %232 = load i32, ptr inttoptr (i32 2284 to ptr), align 4
  %233 = add i32 %232, 1
  store i32 %233, ptr inttoptr (i32 2284 to ptr), align 4
  %234 = load i32, ptr %149, align 4
  %235 = call i32 @fgets(i32 %161, i32 1024, i32 %234)
  %brif_val727.not = icmp eq i32 %235, 0
  br i1 %brif_val727.not, label %blk_exit683, label %loop_entry688

brif_next723:                                     ; preds = %blk_exit697
  %236 = load i8, ptr inttoptr (i32 2294 to ptr), align 1
  %brif_val724.not = icmp eq i8 %236, 0
  br i1 %brif_val724.not, label %brif_next725, label %blk_exit720

brif_next725:                                     ; preds = %brif_next723
  %237 = call i32 @strdup(i32 %161)
  %238 = call i32 @fputs(i32 %237, i32 %215)
  call void @free(i32 %237)
  br label %blk_exit719

loop_entry703:                                    ; preds = %blk_exit689, %blk_exit704
  %_local_5.20 = phi i32 [ %244, %blk_exit704 ], [ %161, %blk_exit689 ]
  %_local_3.36.in = phi i8 [ %246, %blk_exit704 ], [ %217, %blk_exit689 ]
  %239 = sext i8 %_local_3.36.in to i32
  %240 = add nsw i32 %239, -65
  %or.cond805 = icmp ult i32 %240, 26
  br i1 %or.cond805, label %blk_exit705, label %blk_exit706

blk_exit705:                                      ; preds = %blk_exit706, %loop_entry703
  %storemerge800 = phi i8 [ 65, %loop_entry703 ], [ 97, %blk_exit706 ]
  %storemerge799 = phi i8 [ -52, %loop_entry703 ], [ -84, %blk_exit706 ]
  %.narrow = add i8 %storemerge799, %_local_3.36.in
  %241 = urem i8 %.narrow, 26
  %242 = add nuw nsw i8 %storemerge800, %241
  %243 = inttoptr i32 %_local_5.20 to ptr
  store i8 %242, ptr %243, align 1
  br label %blk_exit704

blk_exit704:                                      ; preds = %blk_exit705, %blk_exit706
  %244 = add i32 %_local_5.20, 1
  %245 = inttoptr i32 %244 to ptr
  %246 = load i8, ptr %245, align 1
  %brif_val717.not = icmp eq i8 %246, 0
  br i1 %brif_val717.not, label %blk_exit697, label %loop_entry703

blk_exit706:                                      ; preds = %loop_entry703
  %247 = add nsw i32 %239, -97
  %or.cond806 = icmp ult i32 %247, 26
  br i1 %or.cond806, label %blk_exit705, label %blk_exit704

brif_next680:                                     ; preds = %blk_exit670
  %calcOffset681 = add i32 %148, 20
  %248 = inttoptr i32 %calcOffset681 to ptr
  %249 = load i32, ptr %248, align 4
  %calcOffset682 = add i32 %stack_addr, -2896
  %250 = inttoptr i32 %calcOffset682 to ptr
  store i32 %249, ptr %250, align 4
  %251 = call i32 @printf(i32 2066, i32 %calcOffset682)
  br label %blk_exit678

brif_next638:                                     ; preds = %blk_exit602
  %252 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %253 = add i32 %252, 56
  %254 = inttoptr i32 %253 to ptr
  %255 = load i8, ptr %254, align 1
  %256 = and i8 %255, 3
  %brif_val642.not = icmp eq i8 %256, 0
  br i1 %brif_val642.not, label %brif_next643, label %blk_exit640

brif_next643:                                     ; preds = %brif_next638
  %257 = load i32, ptr inttoptr (i32 3364 to ptr), align 4
  %258 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %259 = sub i32 %257, %258
  %260 = add i32 %259, -2
  br label %blk_exit639

blk_exit639:                                      ; preds = %blk_exit657, %brif_next661, %blk_exit644, %brif_next643
  %_local_3.33 = phi i32 [ %260, %brif_next643 ], [ 0, %blk_exit644 ], [ %_local_3.34, %brif_next661 ], [ %293, %blk_exit657 ]
  store i32 %_local_3.33, ptr inttoptr (i32 2284 to ptr), align 4
  %261 = load i32, ptr inttoptr (i32 2208 to ptr), align 4
  %.not797 = icmp sgt i32 %_local_3.33, %261
  br i1 %.not797, label %blk_exit636, label %loop_entry404

blk_exit640:                                      ; preds = %brif_next638
  %calcOffset645 = add i32 %252, 16
  %262 = inttoptr i32 %calcOffset645 to ptr
  %263 = load i32, ptr %262, align 4
  %brif_val646.not = icmp eq i32 %263, 0
  br i1 %brif_val646.not, label %brif_next647, label %blk_exit644

brif_next647:                                     ; preds = %blk_exit640
  %calcOffset648 = add i32 %252, 4
  %264 = inttoptr i32 %calcOffset648 to ptr
  %265 = load i32, ptr %264, align 4
  %266 = call i32 @fdopen(i32 %265, i32 1194)
  store i32 %266, ptr %262, align 4
  %267 = icmp eq i32 %266, 0
  br i1 %267, label %blk_exit24, label %brif_next651

blk_exit24:                                       ; preds = %brif_next647
  %calcOffset767 = add i32 %252, 24
  %268 = inttoptr i32 %calcOffset767 to ptr
  %269 = load i32, ptr %268, align 4
  call void @perror(i32 %269)
  call void @exit(i32 1) #3
  unreachable

brif_next651:                                     ; preds = %brif_next647
  %270 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset652 = add i32 %270, 16
  %271 = inttoptr i32 %calcOffset652 to ptr
  %272 = load i32, ptr %271, align 4
  br label %blk_exit644

blk_exit644:                                      ; preds = %brif_next651, %blk_exit640
  %_local_5.19 = phi i32 [ %272, %brif_next651 ], [ %263, %blk_exit640 ]
  %273 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %274 = call i32 @fseek(i32 %_local_5.19, i32 %273, i32 0)
  %275 = add i32 %stack_addr, -1024
  %276 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset653 = add i32 %276, 16
  %277 = inttoptr i32 %calcOffset653 to ptr
  %278 = load i32, ptr %277, align 4
  %279 = call i32 @fgets(i32 %275, i32 1024, i32 %278)
  %280 = icmp eq i32 %279, 0
  br i1 %280, label %blk_exit639, label %loop_entry656

loop_entry656:                                    ; preds = %blk_exit644, %blk_exit657
  %_local_3.34 = phi i32 [ %293, %blk_exit657 ], [ 0, %blk_exit644 ]
  %281 = inttoptr i32 %275 to ptr
  %282 = load i8, ptr %281, align 1
  %283 = sext i8 %282 to i32
  %284 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %285 = add i32 %284, 60
  %286 = inttoptr i32 %285 to ptr
  %287 = load i8, ptr %286, align 1
  %288 = zext i8 %287 to i32
  %.not796 = icmp eq i32 %283, %288
  br i1 %.not796, label %brif_next661, label %blk_exit657

brif_next661:                                     ; preds = %loop_entry656
  %calcOffset662 = add i32 %stack_addr, -1023
  %289 = inttoptr i32 %calcOffset662 to ptr
  %290 = load i8, ptr %289, align 1
  %291 = icmp eq i8 %290, 10
  br i1 %291, label %blk_exit639, label %blk_exit657

blk_exit657:                                      ; preds = %brif_next661, %loop_entry656
  %292 = call i32 @strlen(i32 %275)
  %293 = add i32 %292, %_local_3.34
  %calcOffset665 = add i32 %284, 16
  %294 = inttoptr i32 %calcOffset665 to ptr
  %295 = load i32, ptr %294, align 4
  %296 = call i32 @fgets(i32 %275, i32 1024, i32 %295)
  %brif_val666.not = icmp eq i32 %296, 0
  br i1 %brif_val666.not, label %blk_exit639, label %loop_entry656

brif_next604:                                     ; preds = %brif_next601
  %297 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %298 = add i32 %297, 56
  %299 = inttoptr i32 %298 to ptr
  %300 = load i8, ptr %299, align 1
  %301 = and i8 %300, 3
  %brif_val608.not = icmp eq i8 %301, 0
  br i1 %brif_val608.not, label %brif_next609, label %blk_exit606

brif_next609:                                     ; preds = %brif_next604
  %302 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %303 = sub i32 %143, %302
  %304 = add i32 %303, -2
  br label %blk_exit605

blk_exit605:                                      ; preds = %blk_exit623, %brif_next627, %blk_exit610, %brif_next609
  %_local_3.31 = phi i32 [ %304, %brif_next609 ], [ 0, %blk_exit610 ], [ %_local_3.32, %brif_next627 ], [ %338, %blk_exit623 ]
  store i32 %_local_3.31, ptr inttoptr (i32 2284 to ptr), align 4
  %305 = load i32, ptr inttoptr (i32 2208 to ptr), align 4
  %306 = icmp sgt i32 %_local_3.31, %305
  br i1 %306, label %loop_entry404, label %blk_exit602

blk_exit606:                                      ; preds = %brif_next604
  %calcOffset611 = add i32 %297, 16
  %307 = inttoptr i32 %calcOffset611 to ptr
  %308 = load i32, ptr %307, align 4
  %brif_val612.not = icmp eq i32 %308, 0
  br i1 %brif_val612.not, label %brif_next613, label %blk_exit610

brif_next613:                                     ; preds = %blk_exit606
  %calcOffset614 = add i32 %297, 4
  %309 = inttoptr i32 %calcOffset614 to ptr
  %310 = load i32, ptr %309, align 4
  %311 = call i32 @fdopen(i32 %310, i32 1194)
  store i32 %311, ptr %307, align 4
  %312 = icmp eq i32 %311, 0
  br i1 %312, label %blk_exit25, label %brif_next617

blk_exit25:                                       ; preds = %brif_next613
  %calcOffset766 = add i32 %297, 24
  %313 = inttoptr i32 %calcOffset766 to ptr
  %314 = load i32, ptr %313, align 4
  call void @perror(i32 %314)
  call void @exit(i32 1) #3
  unreachable

brif_next617:                                     ; preds = %brif_next613
  %315 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset618 = add i32 %315, 16
  %316 = inttoptr i32 %calcOffset618 to ptr
  %317 = load i32, ptr %316, align 4
  br label %blk_exit610

blk_exit610:                                      ; preds = %brif_next617, %blk_exit606
  %_param_0.12 = phi i32 [ %317, %brif_next617 ], [ %308, %blk_exit606 ]
  %318 = load i32, ptr inttoptr (i32 3360 to ptr), align 4
  %319 = call i32 @fseek(i32 %_param_0.12, i32 %318, i32 0)
  %320 = add i32 %stack_addr, -1024
  %321 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %calcOffset619 = add i32 %321, 16
  %322 = inttoptr i32 %calcOffset619 to ptr
  %323 = load i32, ptr %322, align 4
  %324 = call i32 @fgets(i32 %320, i32 1024, i32 %323)
  %325 = icmp eq i32 %324, 0
  br i1 %325, label %blk_exit605, label %loop_entry622

loop_entry622:                                    ; preds = %blk_exit610, %blk_exit623
  %_local_3.32 = phi i32 [ %338, %blk_exit623 ], [ 0, %blk_exit610 ]
  %326 = inttoptr i32 %320 to ptr
  %327 = load i8, ptr %326, align 1
  %328 = sext i8 %327 to i32
  %329 = load i32, ptr inttoptr (i32 2276 to ptr), align 4
  %330 = add i32 %329, 60
  %331 = inttoptr i32 %330 to ptr
  %332 = load i8, ptr %331, align 1
  %333 = zext i8 %332 to i32
  %.not795 = icmp eq i32 %328, %333
  br i1 %.not795, label %brif_next627, label %blk_exit623

brif_next627:                                     ; preds = %loop_entry622
  %calcOffset628 = add i32 %stack_addr, -1023
  %334 = inttoptr i32 %calcOffset628 to ptr
  %335 = load i8, ptr %334, align 1
  %336 = icmp eq i8 %335, 10
  br i1 %336, label %blk_exit605, label %blk_exit623

blk_exit623:                                      ; preds = %brif_next627, %loop_entry622
  %337 = call i32 @strlen(i32 %320)
  %338 = add i32 %337, %_local_3.32
  %calcOffset631 = add i32 %329, 16
  %339 = inttoptr i32 %calcOffset631 to ptr
  %340 = load i32, ptr %339, align 4
  %341 = call i32 @fgets(i32 %320, i32 1024, i32 %340)
  %brif_val632.not = icmp eq i32 %341, 0
  br i1 %brif_val632.not, label %blk_exit605, label %loop_entry622

blk_exit26:                                       ; preds = %brif_next598, %blk_exit587
  call void @exit(i32 1) #3
  unreachable

brif_next590:                                     ; preds = %blk_exit582
  %calcOffset591 = add i32 %_local_3.24, 28
  %342 = inttoptr i32 %calcOffset591 to ptr
  %343 = load i32, ptr %342, align 4
  %344 = call i32 @open(i32 %343, i32 67108864, i32 0)
  store i32 %344, ptr %129, align 4
  %brif_val593.not = icmp sgt i32 %344, -1
  br i1 %brif_val593.not, label %brif_next594, label %blk_exit27

brif_next594:                                     ; preds = %brif_next590
  %345 = load i32, ptr %102, align 4
  br label %blk_exit587

blk_exit27:                                       ; preds = %brif_next590
  call void @exit(i32 1) #3
  unreachable

brif_next585:                                     ; preds = %blk_exit564
  %346 = sub i32 %126, %127
  store i32 %346, ptr %102, align 4
  br label %blk_exit582

brif_next563.blk_exit564_crit_edge:               ; preds = %brif_next563
  %.pre = add i32 %_local_3.24, 44
  %.pre812 = inttoptr i32 %.pre to ptr
  br label %blk_exit564

loop_entry493:                                    ; preds = %brif_next488, %blk_exit494
  %_local_3.25 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  %347 = load i8, ptr inttoptr (i32 2272 to ptr), align 1
  %348 = icmp eq i8 %347, 0
  br i1 %348, label %blk_exit495, label %brif_next497

blk_exit495:                                      ; preds = %loop_entry493
  call void @get_tbl(i32 %_local_3.25)
  %349 = add i32 %_local_3.25, 44
  %350 = inttoptr i32 %349 to ptr
  %351 = load i32, ptr %350, align 4
  %calcOffset536 = add i32 %stack_addr, -1024
  %352 = inttoptr i32 %calcOffset536 to ptr
  %353 = ptrtoint ptr %stack to i32
  %354 = add i32 %353, -1024
  %355 = inttoptr i32 %354 to ptr
  call void @llvm.memset.p0.i64(ptr %355, i8 0, i64 8, i1 true)
  %356 = call i32 @getenv(i32 1447)
  %357 = icmp eq i32 %356, 0
  br i1 %357, label %blk_exit538, label %brif_next540

blk_exit538:                                      ; preds = %blk_exit495
  %358 = call i32 @getenv(i32 1522)
  %brif_val542.not = icmp eq i32 %358, 0
  br i1 %brif_val542.not, label %brif_next543, label %blk_exit541

brif_next543:                                     ; preds = %blk_exit538
  %359 = call i32 @fopen(i32 1208, i32 1414)
  %360 = icmp eq i32 %359, 0
  br i1 %360, label %blk_exit541, label %brif_next545

brif_next545:                                     ; preds = %brif_next543
  %361 = call i32 @fread(i32 %calcOffset536, i32 8, i32 1, i32 %359)
  %362 = call i32 @fclose(i32 %359)
  %.not791 = icmp eq i32 %361, 1
  br i1 %.not791, label %brif_next547, label %blk_exit541

brif_next547:                                     ; preds = %brif_next545
  %363 = load i64, ptr %352, align 8
  %364 = zext i32 %351 to i64
  %365 = urem i64 %363, %364
  %366 = trunc nuw i64 %365 to i32
  br label %blk_exit537

blk_exit541:                                      ; preds = %brif_next545, %brif_next543, %blk_exit538
  %367 = call i32 @random()
  %368 = urem i32 %367, %351
  br label %blk_exit537

blk_exit537:                                      ; preds = %blk_exit541, %brif_next547, %brif_next540
  %_local_5.14 = phi i32 [ %368, %blk_exit541 ], [ %366, %brif_next547 ], [ %382, %brif_next540 ]
  %calcOffset549 = add i32 %_local_3.25, 68
  %369 = inttoptr i32 %calcOffset549 to ptr
  %370 = load i32, ptr %369, align 4
  %371 = add i32 %370, 44
  %372 = inttoptr i32 %371 to ptr
  %373 = load i32, ptr %372, align 4
  %374 = icmp slt i32 %_local_5.14, %373
  br i1 %374, label %blk_exit494, label %loop_entry553

loop_entry553:                                    ; preds = %blk_exit537, %loop_entry553
  %_local_5.15 = phi i32 [ %_local_5.14, %blk_exit537 ], [ %375, %loop_entry553 ]
  %_local_3.30 = phi i32 [ %370, %blk_exit537 ], [ %377, %loop_entry553 ]
  %_param_0.10 = phi i32 [ %373, %blk_exit537 ], [ %380, %loop_entry553 ]
  %375 = sub i32 %_local_5.15, %_param_0.10
  %calcOffset554 = add i32 %_local_3.30, 76
  %376 = inttoptr i32 %calcOffset554 to ptr
  %377 = load i32, ptr %376, align 4
  %378 = add i32 %377, 44
  %379 = inttoptr i32 %378 to ptr
  %380 = load i32, ptr %379, align 4
  %.not792 = icmp slt i32 %375, %380
  br i1 %.not792, label %blk_exit494, label %loop_entry553

brif_next540:                                     ; preds = %blk_exit495
  %381 = call i32 @atol(i32 %356)
  %382 = urem i32 %381, %351
  br label %blk_exit537

blk_exit494:                                      ; preds = %loop_entry553, %loop_entry524, %blk_exit537, %blk_exit515, %blk_exit500
  %_local_3.26 = phi i32 [ %370, %blk_exit537 ], [ %377, %loop_entry553 ], [ %405, %blk_exit500 ], [ %_local_3.27, %blk_exit515 ], [ %425, %loop_entry524 ]
  %calcOffset558 = add i32 %_local_3.26, 68
  %383 = inttoptr i32 %calcOffset558 to ptr
  %384 = load i32, ptr %383, align 4
  %brif_val559.not = icmp eq i32 %384, 0
  br i1 %brif_val559.not, label %blk_exit489, label %loop_entry493

brif_next497:                                     ; preds = %loop_entry493
  %calcOffset498 = add i32 %_local_3.25, 64
  %385 = inttoptr i32 %calcOffset498 to ptr
  %386 = load i32, ptr %385, align 4
  %calcOffset499 = add i32 %stack_addr, -1024
  %387 = inttoptr i32 %calcOffset499 to ptr
  %388 = ptrtoint ptr %stack to i32
  %389 = add i32 %388, -1024
  %390 = inttoptr i32 %389 to ptr
  call void @llvm.memset.p0.i64(ptr %390, i8 0, i64 8, i1 true)
  %391 = call i32 @getenv(i32 1447)
  %392 = icmp eq i32 %391, 0
  br i1 %392, label %blk_exit501, label %brif_next503

blk_exit501:                                      ; preds = %brif_next497
  %393 = call i32 @getenv(i32 1522)
  %brif_val505.not = icmp eq i32 %393, 0
  br i1 %brif_val505.not, label %brif_next506, label %blk_exit504

brif_next506:                                     ; preds = %blk_exit501
  %394 = call i32 @fopen(i32 1208, i32 1414)
  %395 = icmp eq i32 %394, 0
  br i1 %395, label %blk_exit504, label %brif_next508

brif_next508:                                     ; preds = %brif_next506
  %396 = call i32 @fread(i32 %calcOffset499, i32 8, i32 1, i32 %394)
  %397 = call i32 @fclose(i32 %394)
  %.not790 = icmp eq i32 %396, 1
  br i1 %.not790, label %brif_next510, label %blk_exit504

brif_next510:                                     ; preds = %brif_next508
  %398 = load i64, ptr %387, align 8
  %399 = zext i32 %386 to i64
  %400 = urem i64 %398, %399
  %401 = trunc nuw i64 %400 to i32
  br label %blk_exit500

blk_exit504:                                      ; preds = %brif_next508, %brif_next506, %blk_exit501
  %402 = call i32 @random()
  %403 = urem i32 %402, %386
  br label %blk_exit500

blk_exit500:                                      ; preds = %blk_exit504, %brif_next510, %brif_next503
  %_local_5.10 = phi i32 [ %403, %blk_exit504 ], [ %401, %brif_next510 ], [ %432, %brif_next503 ]
  %calcOffset512 = add i32 %_local_3.25, 68
  %404 = inttoptr i32 %calcOffset512 to ptr
  %405 = load i32, ptr %404, align 4
  %406 = icmp eq i32 %_local_5.10, 0
  br i1 %406, label %blk_exit494, label %brif_next514

brif_next514:                                     ; preds = %blk_exit500
  %407 = and i32 %_local_5.10, 7
  %408 = icmp eq i32 %407, 0
  br i1 %408, label %blk_exit515, label %loop_entry518

blk_exit515:                                      ; preds = %loop_entry518, %brif_next514
  %_local_5.11 = phi i32 [ %_local_5.10, %brif_next514 ], [ %427, %loop_entry518 ]
  %_local_3.27 = phi i32 [ %405, %brif_next514 ], [ %429, %loop_entry518 ]
  %409 = icmp ult i32 %_local_5.10, 8
  br i1 %409, label %blk_exit494, label %loop_entry524

loop_entry524:                                    ; preds = %blk_exit515, %loop_entry524
  %_local_5.13 = phi i32 [ %_local_5.11, %blk_exit515 ], [ %426, %loop_entry524 ]
  %_local_3.29 = phi i32 [ %_local_3.27, %blk_exit515 ], [ %425, %loop_entry524 ]
  %calcOffset525 = add i32 %_local_3.29, 76
  %410 = inttoptr i32 %calcOffset525 to ptr
  %411 = load i32, ptr %410, align 4
  %calcOffset526 = add i32 %411, 76
  %412 = inttoptr i32 %calcOffset526 to ptr
  %413 = load i32, ptr %412, align 4
  %calcOffset527 = add i32 %413, 76
  %414 = inttoptr i32 %calcOffset527 to ptr
  %415 = load i32, ptr %414, align 4
  %calcOffset528 = add i32 %415, 76
  %416 = inttoptr i32 %calcOffset528 to ptr
  %417 = load i32, ptr %416, align 4
  %calcOffset529 = add i32 %417, 76
  %418 = inttoptr i32 %calcOffset529 to ptr
  %419 = load i32, ptr %418, align 4
  %calcOffset530 = add i32 %419, 76
  %420 = inttoptr i32 %calcOffset530 to ptr
  %421 = load i32, ptr %420, align 4
  %calcOffset531 = add i32 %421, 76
  %422 = inttoptr i32 %calcOffset531 to ptr
  %423 = load i32, ptr %422, align 4
  %calcOffset532 = add i32 %423, 76
  %424 = inttoptr i32 %calcOffset532 to ptr
  %425 = load i32, ptr %424, align 4
  %426 = add i32 %_local_5.13, -8
  %brif_val533.not = icmp eq i32 %426, 0
  br i1 %brif_val533.not, label %blk_exit494, label %loop_entry524

loop_entry518:                                    ; preds = %brif_next514, %loop_entry518
  %_local_5.12 = phi i32 [ %_local_5.10, %brif_next514 ], [ %427, %loop_entry518 ]
  %_local_3.28 = phi i32 [ %405, %brif_next514 ], [ %429, %loop_entry518 ]
  %_param_0.9 = phi i32 [ %407, %brif_next514 ], [ %430, %loop_entry518 ]
  %427 = add i32 %_local_5.12, -1
  %calcOffset519 = add i32 %_local_3.28, 76
  %428 = inttoptr i32 %calcOffset519 to ptr
  %429 = load i32, ptr %428, align 4
  %430 = add i32 %_param_0.9, -1
  %brif_val520.not = icmp eq i32 %430, 0
  br i1 %brif_val520.not, label %blk_exit515, label %loop_entry518

brif_next503:                                     ; preds = %brif_next497
  %431 = call i32 @atol(i32 %391)
  %432 = urem i32 %431, %386
  br label %blk_exit500

blk_exit407:                                      ; preds = %brif_next442, %brif_next411
  %_local_3.20 = phi i32 [ %_local_3.21, %brif_next442 ], [ %80, %brif_next411 ]
  %433 = load i8, ptr inttoptr (i32 3336 to ptr), align 1
  %brif_val447.not = icmp eq i8 %433, 0
  br i1 %brif_val447.not, label %brif_next448, label %blk_exit446

brif_next448:                                     ; preds = %blk_exit407
  store i32 -1, ptr inttoptr (i32 3348 to ptr), align 4
  call void @llvm.memset.p0.i64(ptr inttoptr (i32 3340 to ptr), i8 0, i64 8, i1 true)
  br label %loop_entry449

loop_entry449:                                    ; preds = %blk_exit450, %brif_next448
  %_local_5.7 = phi i32 [ %_local_3.20, %brif_next448 ], [ %450, %blk_exit450 ]
  call void @get_tbl(i32 %_local_5.7)
  %434 = inttoptr i32 %_local_5.7 to ptr
  %435 = load i32, ptr %434, align 4
  %.not785 = icmp eq i32 %435, -1
  br i1 %.not785, label %brif_next453, label %blk_exit450

brif_next453:                                     ; preds = %loop_entry449
  %436 = load i32, ptr inttoptr (i32 3340 to ptr), align 4
  %437 = add i32 %_local_5.7, 44
  %438 = inttoptr i32 %437 to ptr
  %439 = load i32, ptr %438, align 4
  %440 = add i32 %436, %439
  store i32 %440, ptr inttoptr (i32 3340 to ptr), align 4
  %441 = load i32, ptr inttoptr (i32 3344 to ptr), align 4
  %442 = add i32 %_local_5.7, 48
  %443 = inttoptr i32 %442 to ptr
  %444 = load i32, ptr %443, align 4
  %.not786 = icmp ult i32 %441, %444
  br i1 %.not786, label %brif_next458, label %blk_exit455

brif_next458:                                     ; preds = %brif_next453
  store i32 %444, ptr inttoptr (i32 3344 to ptr), align 4
  br label %blk_exit455

blk_exit455:                                      ; preds = %brif_next458, %brif_next453
  %445 = load i32, ptr inttoptr (i32 3348 to ptr), align 4
  %446 = add i32 %_local_5.7, 52
  %447 = inttoptr i32 %446 to ptr
  %448 = load i32, ptr %447, align 4
  %.not787 = icmp ugt i32 %445, %448
  br i1 %.not787, label %brif_next461, label %blk_exit450

brif_next461:                                     ; preds = %blk_exit455
  store i32 %448, ptr inttoptr (i32 3348 to ptr), align 4
  br label %blk_exit450

blk_exit450:                                      ; preds = %brif_next461, %blk_exit455, %loop_entry449
  %calcOffset462 = add i32 %_local_5.7, 76
  %449 = inttoptr i32 %calcOffset462 to ptr
  %450 = load i32, ptr %449, align 4
  %brif_val463.not = icmp eq i32 %450, 0
  br i1 %brif_val463.not, label %brif_next464, label %loop_entry449

brif_next464:                                     ; preds = %blk_exit450
  store i8 1, ptr inttoptr (i32 3336 to ptr), align 1
  br label %blk_exit446

blk_exit446:                                      ; preds = %brif_next464, %blk_exit407
  %451 = load i32, ptr inttoptr (i32 3340 to ptr), align 4
  %calcOffset465 = add i32 %stack_addr, -1024
  %452 = inttoptr i32 %calcOffset465 to ptr
  %453 = ptrtoint ptr %stack to i32
  %454 = add i32 %453, -1024
  %455 = inttoptr i32 %454 to ptr
  call void @llvm.memset.p0.i64(ptr %455, i8 0, i64 8, i1 true)
  %456 = call i32 @getenv(i32 1447)
  %457 = icmp eq i32 %456, 0
  br i1 %457, label %blk_exit467, label %brif_next469

blk_exit467:                                      ; preds = %blk_exit446
  %458 = call i32 @getenv(i32 1522)
  %brif_val471.not = icmp eq i32 %458, 0
  br i1 %brif_val471.not, label %brif_next472, label %blk_exit470

brif_next472:                                     ; preds = %blk_exit467
  %459 = call i32 @fopen(i32 1208, i32 1414)
  %460 = icmp eq i32 %459, 0
  br i1 %460, label %blk_exit470, label %brif_next474

brif_next474:                                     ; preds = %brif_next472
  %461 = call i32 @fread(i32 %calcOffset465, i32 8, i32 1, i32 %459)
  %462 = call i32 @fclose(i32 %459)
  %.not788 = icmp eq i32 %461, 1
  br i1 %.not788, label %brif_next476, label %blk_exit470

brif_next476:                                     ; preds = %brif_next474
  %463 = load i64, ptr %452, align 8
  %464 = zext i32 %451 to i64
  %465 = urem i64 %463, %464
  %466 = trunc nuw i64 %465 to i32
  br label %blk_exit466

blk_exit470:                                      ; preds = %brif_next474, %brif_next472, %blk_exit467
  %467 = call i32 @random()
  %468 = urem i32 %467, %451
  br label %blk_exit466

blk_exit466:                                      ; preds = %blk_exit470, %brif_next476, %brif_next469
  %_local_5.8 = phi i32 [ %468, %blk_exit470 ], [ %466, %brif_next476 ], [ %480, %brif_next469 ]
  %469 = add i32 %_local_3.20, 44
  %470 = inttoptr i32 %469 to ptr
  %471 = load i32, ptr %470, align 4
  %472 = icmp slt i32 %_local_5.8, %471
  br i1 %472, label %blk_exit406, label %loop_entry481

loop_entry481:                                    ; preds = %blk_exit466, %loop_entry481
  %_local_5.9 = phi i32 [ %_local_5.8, %blk_exit466 ], [ %473, %loop_entry481 ]
  %_local_3.23 = phi i32 [ %_local_3.20, %blk_exit466 ], [ %475, %loop_entry481 ]
  %_param_0.8 = phi i32 [ %471, %blk_exit466 ], [ %478, %loop_entry481 ]
  %473 = sub i32 %_local_5.9, %_param_0.8
  %calcOffset482 = add i32 %_local_3.23, 76
  %474 = inttoptr i32 %calcOffset482 to ptr
  %475 = load i32, ptr %474, align 4
  %476 = add i32 %475, 44
  %477 = inttoptr i32 %476 to ptr
  %478 = load i32, ptr %477, align 4
  %.not789 = icmp slt i32 %473, %478
  br i1 %.not789, label %blk_exit406, label %loop_entry481

brif_next469:                                     ; preds = %blk_exit446
  %479 = call i32 @atol(i32 %456)
  %480 = urem i32 %479, %451
  br label %blk_exit466

brif_next411:                                     ; preds = %loop_entry404
  %481 = icmp eq i32 %82, -1
  br i1 %481, label %blk_exit407, label %brif_next413

brif_next413:                                     ; preds = %brif_next411
  %calcOffset414 = add i32 %stack_addr, -1024
  %482 = inttoptr i32 %calcOffset414 to ptr
  %483 = ptrtoint ptr %stack to i32
  %484 = add i32 %483, -1024
  %485 = inttoptr i32 %484 to ptr
  call void @llvm.memset.p0.i64(ptr %485, i8 0, i64 8, i1 true)
  %486 = call i32 @getenv(i32 1447)
  %487 = icmp eq i32 %486, 0
  br i1 %487, label %blk_exit416, label %brif_next418

blk_exit416:                                      ; preds = %brif_next413
  %488 = call i32 @getenv(i32 1522)
  %brif_val420.not = icmp eq i32 %488, 0
  br i1 %brif_val420.not, label %brif_next421, label %blk_exit419

brif_next421:                                     ; preds = %blk_exit416
  %489 = call i32 @fopen(i32 1208, i32 1414)
  %490 = icmp eq i32 %489, 0
  br i1 %490, label %blk_exit419, label %brif_next423

brif_next423:                                     ; preds = %brif_next421
  %491 = call i32 @fread(i32 %calcOffset414, i32 8, i32 1, i32 %489)
  %492 = call i32 @fclose(i32 %489)
  %.not780 = icmp eq i32 %491, 1
  br i1 %.not780, label %brif_next425, label %blk_exit419

brif_next425:                                     ; preds = %brif_next423
  %493 = load i64, ptr %482, align 8
  %494 = urem i64 %493, 100
  %495 = trunc nuw nsw i64 %494 to i32
  br label %blk_exit415

blk_exit419:                                      ; preds = %brif_next423, %brif_next421, %blk_exit416
  %496 = call i32 @random()
  %497 = urem i32 %496, 100
  br label %blk_exit415

blk_exit415:                                      ; preds = %blk_exit419, %brif_next425, %brif_next418
  %_param_0.6 = phi i32 [ %497, %blk_exit419 ], [ %495, %brif_next425 ], [ %508, %brif_next418 ]
  %498 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  %499 = inttoptr i32 %498 to ptr
  %500 = load i32, ptr %499, align 4
  %.not781 = icmp eq i32 %500, -1
  br i1 %.not781, label %blk_exit408, label %blk_exit427

blk_exit427:                                      ; preds = %blk_exit415
  %.not782 = icmp slt i32 %_param_0.6, %500
  br i1 %.not782, label %blk_exit408, label %loop_entry434

loop_entry434:                                    ; preds = %blk_exit427, %brif_next438
  %_local_3.22 = phi i32 [ %498, %blk_exit427 ], [ %502, %brif_next438 ]
  %_param_1.7 = phi i32 [ %500, %blk_exit427 ], [ %504, %brif_next438 ]
  %_param_0.7 = phi i32 [ %_param_0.6, %blk_exit427 ], [ %506, %brif_next438 ]
  %calcOffset435 = add i32 %_local_3.22, 76
  %501 = inttoptr i32 %calcOffset435 to ptr
  %502 = load i32, ptr %501, align 4
  %503 = inttoptr i32 %502 to ptr
  %504 = load i32, ptr %503, align 4
  %505 = icmp eq i32 %504, -1
  br i1 %505, label %blk_exit408, label %brif_next438

brif_next438:                                     ; preds = %loop_entry434
  %506 = sub i32 %_param_0.7, %_param_1.7
  %.not783 = icmp slt i32 %506, %504
  br i1 %.not783, label %blk_exit408, label %loop_entry434

brif_next418:                                     ; preds = %brif_next413
  %507 = call i32 @atol(i32 %486)
  %508 = urem i32 %507, 100
  br label %blk_exit415

brif_next366:                                     ; preds = %blk_exit344
  %509 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  %510 = load i8, ptr inttoptr (i32 3336 to ptr), align 1
  %brif_val371.not = icmp eq i8 %510, 0
  br i1 %brif_val371.not, label %brif_next372, label %blk_exit370

brif_next372:                                     ; preds = %brif_next366
  store i32 -1, ptr inttoptr (i32 3348 to ptr), align 4
  call void @llvm.memset.p0.i64(ptr inttoptr (i32 3340 to ptr), i8 0, i64 8, i1 true)
  %511 = icmp eq i32 %509, 0
  br i1 %511, label %blk_exit369, label %loop_entry375

blk_exit369:                                      ; preds = %brif_next372
  store i8 1, ptr inttoptr (i32 3336 to ptr), align 1
  %512 = icmp eq i8 %72, 0
  br i1 %512, label %blk_exit21, label %blk_exit368

blk_exit368:                                      ; preds = %blk_exit369, %brif_next390
  %513 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  br label %blk_exit367

loop_entry375:                                    ; preds = %brif_next372, %blk_exit376
  %_local_3.18 = phi i32 [ %509, %brif_next372 ], [ %530, %blk_exit376 ]
  call void @get_tbl(i32 %_local_3.18)
  %514 = inttoptr i32 %_local_3.18 to ptr
  %515 = load i32, ptr %514, align 4
  %.not777 = icmp eq i32 %515, -1
  br i1 %.not777, label %brif_next379, label %blk_exit376

brif_next379:                                     ; preds = %loop_entry375
  %516 = load i32, ptr inttoptr (i32 3340 to ptr), align 4
  %517 = add i32 %_local_3.18, 44
  %518 = inttoptr i32 %517 to ptr
  %519 = load i32, ptr %518, align 4
  %520 = add i32 %516, %519
  store i32 %520, ptr inttoptr (i32 3340 to ptr), align 4
  %521 = load i32, ptr inttoptr (i32 3344 to ptr), align 4
  %522 = add i32 %_local_3.18, 48
  %523 = inttoptr i32 %522 to ptr
  %524 = load i32, ptr %523, align 4
  %.not778 = icmp ult i32 %521, %524
  br i1 %.not778, label %brif_next384, label %blk_exit381

brif_next384:                                     ; preds = %brif_next379
  store i32 %524, ptr inttoptr (i32 3344 to ptr), align 4
  br label %blk_exit381

blk_exit381:                                      ; preds = %brif_next384, %brif_next379
  %525 = load i32, ptr inttoptr (i32 3348 to ptr), align 4
  %526 = add i32 %_local_3.18, 52
  %527 = inttoptr i32 %526 to ptr
  %528 = load i32, ptr %527, align 4
  %.not779 = icmp ugt i32 %525, %528
  br i1 %.not779, label %brif_next387, label %blk_exit376

brif_next387:                                     ; preds = %blk_exit381
  store i32 %528, ptr inttoptr (i32 3348 to ptr), align 4
  br label %blk_exit376

blk_exit376:                                      ; preds = %brif_next387, %blk_exit381, %loop_entry375
  %calcOffset388 = add i32 %_local_3.18, 76
  %529 = inttoptr i32 %calcOffset388 to ptr
  %530 = load i32, ptr %529, align 4
  %brif_val389.not = icmp eq i32 %530, 0
  br i1 %brif_val389.not, label %brif_next390, label %loop_entry375

brif_next390:                                     ; preds = %blk_exit376
  store i8 1, ptr inttoptr (i32 3336 to ptr), align 1
  %531 = load i8, ptr inttoptr (i32 2272 to ptr), align 1
  %532 = icmp eq i8 %531, 0
  br i1 %532, label %blk_exit21, label %blk_exit368

blk_exit21:                                       ; preds = %blk_exit370, %blk_exit369, %brif_next390
  %533 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  br label %blk_exit20

blk_exit20:                                       ; preds = %blk_exit21, %brif_next403, %blk_exit367
  %_param_0.0 = phi i32 [ %533, %blk_exit21 ], [ 0, %blk_exit367 ], [ %_local_3.17, %brif_next403 ]
  call void @print_list(i32 %_param_0.0, i32 0)
  br label %blk_exit19

blk_exit367:                                      ; preds = %blk_exit368, %blk_exit370
  %_local_3.17 = phi i32 [ %513, %blk_exit368 ], [ %509, %blk_exit370 ]
  call void @llvm.memset.p0.i64(ptr inttoptr (i32 3352 to ptr), i8 0, i64 8, i1 true)
  %534 = icmp eq i32 %_local_3.17, 0
  br i1 %534, label %blk_exit20, label %loop_entry399

loop_entry399:                                    ; preds = %blk_exit367, %loop_entry399
  %_local_5.5 = phi i32 [ %540, %loop_entry399 ], [ %_local_3.17, %blk_exit367 ]
  %_param_1.6 = phi i32 [ %538, %loop_entry399 ], [ 0, %blk_exit367 ]
  %_param_0.5 = phi i32 [ %535, %loop_entry399 ], [ 0, %blk_exit367 ]
  %535 = add i32 %_param_0.5, 1
  %calcOffset400 = add i32 %_local_5.5, 64
  %536 = inttoptr i32 %calcOffset400 to ptr
  %537 = load i32, ptr %536, align 4
  %538 = add i32 %537, %_param_1.6
  %calcOffset401 = add i32 %_local_5.5, 76
  %539 = inttoptr i32 %calcOffset401 to ptr
  %540 = load i32, ptr %539, align 4
  %brif_val402.not = icmp eq i32 %540, 0
  br i1 %brif_val402.not, label %brif_next403, label %loop_entry399

brif_next403:                                     ; preds = %loop_entry399
  store i32 %538, ptr inttoptr (i32 3352 to ptr), align 4
  store i32 %535, ptr inttoptr (i32 3356 to ptr), align 4
  br label %blk_exit20

blk_exit370:                                      ; preds = %brif_next366
  %brif_val393.not = icmp eq i8 %72, 0
  br i1 %brif_val393.not, label %blk_exit21, label %blk_exit367

brif_next348:                                     ; preds = %blk_exit339
  %541 = sub i32 100, %_param_1.4
  %542 = icmp ult i32 %_local_4.2, 2
  br i1 %542, label %blk_exit349, label %brif_next351

blk_exit349:                                      ; preds = %blk_exit355, %brif_next351, %brif_next348
  %_local_3.14 = phi i32 [ %541, %brif_next348 ], [ %541, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %543 = inttoptr i32 %_local_5.3 to ptr
  store i32 %_local_3.14, ptr %543, align 4
  br label %blk_exit344

brif_next351:                                     ; preds = %brif_next348
  %544 = udiv i32 %541, %_local_4.2
  %545 = icmp eq i32 %65, %_local_5.3
  br i1 %545, label %blk_exit349, label %loop_entry354

loop_entry354:                                    ; preds = %brif_next351, %blk_exit355
  %_local_12.1 = phi i32 [ %65, %brif_next351 ], [ %550, %blk_exit355 ]
  %_local_3.15 = phi i32 [ %541, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %546 = inttoptr i32 %_local_12.1 to ptr
  %547 = load i32, ptr %546, align 4
  %.not775 = icmp eq i32 %547, -1
  br i1 %.not775, label %brif_next358, label %blk_exit355

brif_next358:                                     ; preds = %loop_entry354
  store i32 %544, ptr %546, align 4
  %548 = sub i32 %_local_3.15, %544
  br label %blk_exit355

blk_exit355:                                      ; preds = %brif_next358, %loop_entry354
  %_local_3.16 = phi i32 [ %548, %brif_next358 ], [ %_local_3.15, %loop_entry354 ]
  %calcOffset360 = add i32 %_local_12.1, 80
  %549 = inttoptr i32 %calcOffset360 to ptr
  %550 = load i32, ptr %549, align 4
  %.not776 = icmp eq i32 %550, %_local_5.3
  br i1 %.not776, label %blk_exit349, label %loop_entry354

blk_exit30:                                       ; preds = %blk_exit334
  %551 = load i32, ptr null, align 4
  %552 = call i32 @fprintf(i32 %551, i32 1655, i32 0)
  call void @exit(i32 1) #3
  unreachable

brif_next323:                                     ; preds = %blk_exit314
  %553 = load i8, ptr inttoptr (i32 2272 to ptr), align 1
  %554 = and i8 %553, 1
  br label %loop_entry324

loop_entry324:                                    ; preds = %loop_entry324, %brif_next323
  %_local_5.4 = phi i32 [ 0, %brif_next323 ], [ %560, %loop_entry324 ]
  %_local_4.3 = phi i32 [ 0, %brif_next323 ], [ %561, %loop_entry324 ]
  %_local_3.13 = phi i32 [ %65, %brif_next323 ], [ %565, %loop_entry324 ]
  %_param_1.5 = phi i32 [ 0, %brif_next323 ], [ %563, %loop_entry324 ]
  %select_cond325.not = icmp eq i8 %554, 0
  %555 = select i1 %select_cond325.not, i32 %_local_5.4, i32 %_local_3.13
  %556 = inttoptr i32 %_local_3.13 to ptr
  %557 = load i32, ptr %556, align 4
  %558 = icmp eq i32 %557, -1
  %559 = zext i1 %558 to i32
  %560 = select i1 %558, i32 %555, i32 %_local_5.4
  %561 = add i32 %_local_4.3, %559
  %562 = select i1 %558, i32 0, i32 %557
  %563 = add i32 %562, %_param_1.5
  %calcOffset329 = add i32 %_local_3.13, 80
  %564 = inttoptr i32 %calcOffset329 to ptr
  %565 = load i32, ptr %564, align 4
  %brif_val330.not = icmp eq i32 %565, 0
  br i1 %brif_val330.not, label %brif_next331, label %loop_entry324

brif_next331:                                     ; preds = %loop_entry324
  %566 = icmp sgt i32 %563, 100
  br i1 %566, label %blk_exit32, label %blk_exit321

blk_exit32:                                       ; preds = %brif_next331
  %calcOffset763 = add i32 %stack_addr, -2880
  %567 = inttoptr i32 %calcOffset763 to ptr
  store i32 %563, ptr %567, align 4
  %568 = load i32, ptr null, align 4
  %569 = call i32 @fprintf(i32 %568, i32 2122, i32 %calcOffset763)
  call void @exit(i32 1) #3
  unreachable

blk_exit19:                                       ; preds = %blk_exit20, %blk_exit731, %blk_exit683, %brif_next318
  %_local_5.0 = phi i32 [ 0, %blk_exit683 ], [ 0, %blk_exit731 ], [ 0, %blk_exit20 ], [ %583, %brif_next318 ]
  %570 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  call void @free_desc(i32 %570)
  %571 = load i32, ptr inttoptr (i32 2288 to ptr), align 4
  call void @free(i32 %571)
  call void @exit(i32 %_local_5.0)
  unreachable

brif_next316:                                     ; preds = %blk_exit35
  %572 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  %573 = call i32 @maxlen_in_list(i32 %572)
  store i32 %573, ptr inttoptr (i32 2284 to ptr), align 4
  %574 = add i32 %573, 10
  %575 = call i32 @malloc(i32 %574)
  %576 = icmp eq i32 %575, 0
  br i1 %576, label %blk_exit33, label %brif_next318

blk_exit33:                                       ; preds = %brif_next316
  %calcOffset762 = add i32 %stack_addr, -2944
  %577 = inttoptr i32 %calcOffset762 to ptr
  store i32 1987, ptr %577, align 4
  %578 = load i32, ptr null, align 4
  %579 = call i32 @fprintf(i32 %578, i32 1191, i32 %calcOffset762)
  call void @exit(i32 1) #3
  unreachable

brif_next318:                                     ; preds = %brif_next316
  store i32 %575, ptr inttoptr (i32 2288 to ptr), align 4
  %calcOffset319 = add i32 %stack_addr, -1024
  %580 = inttoptr i32 %calcOffset319 to ptr
  store i8 0, ptr %580, align 1
  %581 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  call void @matches_in_list(i32 %581, i32 %calcOffset319)
  %582 = load i8, ptr %580, align 1
  %583 = zext i8 %582 to i32
  call void @regfree(i32 2232)
  br label %blk_exit19

brif_next264:                                     ; preds = %blk_exit42
  %brif_val265.not.not = icmp eq i32 %_local_3.0, 0
  br i1 %brif_val265.not.not, label %blk_exit38, label %brif_next266

blk_exit38:                                       ; preds = %brif_next264
  %584 = call i32 @regcomp(i32 2232, i32 %_local_4.0, i32 8)
  %585 = icmp eq i32 %584, 0
  br i1 %585, label %blk_exit35, label %blk_exit37

blk_exit37:                                       ; preds = %blk_exit284, %blk_exit38
  %_local_4.1 = phi i32 [ %_local_4.0, %blk_exit38 ], [ %592, %blk_exit284 ]
  %calcOffset313 = add i32 %stack_addr, -2848
  %586 = inttoptr i32 %calcOffset313 to ptr
  store i32 %_local_4.1, ptr %586, align 4
  %587 = load i32, ptr null, align 4
  %588 = call i32 @fprintf(i32 %587, i32 1742, i32 %calcOffset313)
  call void @exit(i32 1) #3
  unreachable

brif_next266:                                     ; preds = %brif_next264
  %589 = inttoptr i32 %_local_4.0 to ptr
  %590 = load i8, ptr %589, align 1
  %591 = icmp eq i8 %590, 0
  br i1 %591, label %blk_exit39, label %loop_entry275

blk_exit39:                                       ; preds = %brif_next277, %brif_next266
  %_param_1.0 = phi i32 [ 1, %brif_next266 ], [ %630, %brif_next277 ]
  %592 = call i32 @malloc(i32 %_param_1.0)
  %593 = icmp eq i32 %592, 0
  br i1 %593, label %blk_exit34, label %brif_next283

blk_exit34:                                       ; preds = %blk_exit39
  %calcOffset761 = add i32 %stack_addr, -2864
  %594 = inttoptr i32 %calcOffset761 to ptr
  store i32 1759, ptr %594, align 4
  %595 = load i32, ptr null, align 4
  %596 = call i32 @fprintf(i32 %595, i32 1191, i32 %calcOffset761)
  call void @exit(i32 1) #3
  unreachable

brif_next283:                                     ; preds = %blk_exit39
  %597 = load i8, ptr %589, align 1
  %598 = icmp eq i8 %597, 0
  br i1 %598, label %blk_exit284, label %loop_entry288

blk_exit284:                                      ; preds = %blk_exit289, %brif_next283
  %_local_3.10 = phi i32 [ %592, %brif_next283 ], [ %_local_3.12, %blk_exit289 ]
  %599 = inttoptr i32 %_local_3.10 to ptr
  store i8 0, ptr %599, align 1
  %600 = call i32 @regcomp(i32 2232, i32 %592, i32 8)
  %601 = icmp eq i32 %600, 0
  br i1 %601, label %blk_exit36, label %blk_exit37

blk_exit36:                                       ; preds = %blk_exit284
  call void @free(i32 %592)
  br label %blk_exit35

loop_entry288:                                    ; preds = %brif_next283, %blk_exit289
  %_local_5.2.in = phi i8 [ %617, %blk_exit289 ], [ %597, %brif_next283 ]
  %_local_3.11 = phi i32 [ %_local_3.12, %blk_exit289 ], [ %592, %brif_next283 ]
  %_param_1.3.in = phi i32 [ %_param_1.3, %blk_exit289 ], [ %_local_4.0, %brif_next283 ]
  %_param_1.3 = add i32 %_param_1.3.in, 1
  %602 = sext i8 %_local_5.2.in to i32
  %603 = add nsw i32 %602, -123
  %604 = icmp ult i32 %603, -26
  br i1 %604, label %blk_exit290, label %brif_next292

blk_exit290:                                      ; preds = %loop_entry288
  %605 = add nsw i32 %602, -91
  %606 = icmp ult i32 %605, -26
  br i1 %606, label %blk_exit297, label %brif_next299

blk_exit297:                                      ; preds = %blk_exit290
  %607 = inttoptr i32 %_local_3.11 to ptr
  store i8 %_local_5.2.in, ptr %607, align 1
  %608 = add i32 %_local_3.11, 1
  br label %blk_exit289

brif_next299:                                     ; preds = %blk_exit290
  %calcOffset300 = add i32 %_local_3.11, 1
  %609 = inttoptr i32 %calcOffset300 to ptr
  store i8 %_local_5.2.in, ptr %609, align 1
  %610 = inttoptr i32 %_local_3.11 to ptr
  store i8 91, ptr %610, align 1
  %611 = call i32 @tolower(i32 %602)
  %calcOffset302 = add i32 %_local_3.11, 3
  %612 = inttoptr i32 %calcOffset302 to ptr
  store i8 93, ptr %612, align 1
  %calcOffset303 = add i32 %_local_3.11, 2
  %613 = inttoptr i32 %calcOffset303 to ptr
  %614 = trunc i32 %611 to i8
  store i8 %614, ptr %613, align 1
  %615 = add i32 %_local_3.11, 4
  br label %blk_exit289

blk_exit289:                                      ; preds = %blk_exit297, %brif_next299, %brif_next292
  %_local_3.12 = phi i32 [ %608, %blk_exit297 ], [ %615, %brif_next299 ], [ %624, %brif_next292 ]
  %616 = inttoptr i32 %_param_1.3 to ptr
  %617 = load i8, ptr %616, align 1
  %brif_val306.not = icmp eq i8 %617, 0
  br i1 %brif_val306.not, label %blk_exit284, label %loop_entry288

brif_next292:                                     ; preds = %loop_entry288
  %calcOffset293 = add i32 %_local_3.11, 1
  %618 = inttoptr i32 %calcOffset293 to ptr
  store i8 %_local_5.2.in, ptr %618, align 1
  %619 = inttoptr i32 %_local_3.11 to ptr
  store i8 91, ptr %619, align 1
  %620 = call i32 @toupper(i32 %602)
  %calcOffset295 = add i32 %_local_3.11, 3
  %621 = inttoptr i32 %calcOffset295 to ptr
  store i8 93, ptr %621, align 1
  %calcOffset296 = add i32 %_local_3.11, 2
  %622 = inttoptr i32 %calcOffset296 to ptr
  %623 = trunc i32 %620 to i8
  store i8 %623, ptr %622, align 1
  %624 = add i32 %_local_3.11, 4
  br label %blk_exit289

loop_entry275:                                    ; preds = %brif_next266, %brif_next277
  %_local_5.1 = phi i32 [ %630, %brif_next277 ], [ 1, %brif_next266 ]
  %_local_3.9.in = phi i8 [ %632, %brif_next277 ], [ %590, %brif_next266 ]
  %_param_0.4.in = phi i32 [ %_param_0.4, %brif_next277 ], [ %_local_4.0, %brif_next266 ]
  %625 = or i8 %_local_3.9.in, 32
  %626 = sext i8 %625 to i32
  %627 = add nsw i32 %626, -97
  %628 = icmp ult i32 %627, 26
  %629 = select i1 %628, i32 4, i32 1
  %630 = add i32 %629, %_local_5.1
  %.not773 = icmp ult i32 %_local_5.1, %630
  br i1 %.not773, label %brif_next277, label %blk_exit274

brif_next277:                                     ; preds = %loop_entry275
  %_param_0.4 = add i32 %_param_0.4.in, 1
  %631 = inttoptr i32 %_param_0.4 to ptr
  %632 = load i8, ptr %631, align 1
  %633 = icmp eq i8 %632, 0
  br i1 %633, label %blk_exit39, label %loop_entry275

blk_exit274:                                      ; preds = %loop_entry275
  %calcOffset281 = add i32 %stack_addr, -2832
  %634 = inttoptr i32 %calcOffset281 to ptr
  store i32 2075, ptr %634, align 4
  %635 = load i32, ptr null, align 4
  %636 = call i32 @fprintf(i32 %635, i32 1191, i32 %calcOffset281)
  call void @exit(i32 1) #3
  unreachable

brif_next45:                                      ; preds = %blk_exit5
  %637 = icmp slt i32 %6, 1
  br i1 %637, label %blk_exit42, label %loop_entry51

loop_entry51:                                     ; preds = %brif_next45, %blk_exit110
  %_local_8.0 = phi i32 [ %699, %blk_exit110 ], [ 0, %brif_next45 ]
  %638 = add i32 %5, %_local_8.0
  %639 = shl i32 %638, 2
  %640 = add i32 %639, %_arg_1
  %641 = inttoptr i32 %640 to ptr
  %642 = load i32, ptr %641, align 4
  %643 = inttoptr i32 %642 to ptr
  %644 = load i8, ptr %643, align 1
  %645 = sext i8 %644 to i32
  %646 = add nsw i32 %645, -48
  %647 = icmp ult i32 %646, 10
  br i1 %647, label %blk_exit53, label %blk_exit52

blk_exit53:                                       ; preds = %loop_entry51
  %648 = add i32 %642, 1
  br label %loop_entry58

loop_entry58:                                     ; preds = %brif_next60, %blk_exit53
  %_local_3.1 = phi i32 [ %645, %blk_exit53 ], [ %660, %brif_next60 ]
  %_param_1.1 = phi i32 [ 0, %blk_exit53 ], [ %657, %brif_next60 ]
  %_param_0.1 = phi i32 [ %648, %blk_exit53 ], [ %661, %brif_next60 ]
  %649 = mul i32 %_param_1.1, 10
  %650 = add i32 %649, %_local_3.1
  %651 = icmp sgt i32 %650, 148
  br i1 %651, label %blk_exit50, label %brif_next60

blk_exit50:                                       ; preds = %loop_entry58
  %652 = load i32, ptr null, align 4
  %653 = call i32 @fprintf(i32 %652, i32 1914, i32 0)
  %654 = load i32, ptr %641, align 4
  %calcOffset216 = add i32 %stack_addr, -2816
  %655 = inttoptr i32 %calcOffset216 to ptr
  store i32 %654, ptr %655, align 4
  %656 = call i32 @fprintf(i32 %652, i32 2159, i32 %calcOffset216)
  br label %blk_exit49

blk_exit49:                                       ; preds = %blk_exit50, %brif_next78, %blk_exit69, %brif_next66
  store i8 1, ptr inttoptr (i32 2296 to ptr), align 1
  br label %blk_exit41

brif_next60:                                      ; preds = %loop_entry58
  %657 = add i32 %650, -48
  %658 = inttoptr i32 %_param_0.1 to ptr
  %659 = load i8, ptr %658, align 1
  %660 = sext i8 %659 to i32
  %661 = add i32 %_param_0.1, 1
  %662 = add nsw i32 %660, -48
  %663 = icmp ult i32 %662, 10
  br i1 %663, label %loop_entry58, label %brif_next63

brif_next63:                                      ; preds = %brif_next60
  %664 = icmp sgt i32 %650, 47
  br i1 %664, label %blk_exit64, label %brif_next66

blk_exit64:                                       ; preds = %brif_next63
  switch i8 %659, label %blk_exit52 [
    i8 37, label %blk_exit68
    i8 46, label %blk_exit69
  ]

blk_exit68:                                       ; preds = %blk_exit64
  %665 = inttoptr i32 %661 to ptr
  %666 = load i8, ptr %665, align 1
  %667 = icmp eq i8 %666, 0
  br i1 %667, label %blk_exit72, label %blk_exit52

blk_exit72:                                       ; preds = %blk_exit68
  %668 = add i32 %_local_8.0, 1
  %669 = icmp slt i32 %668, %6
  br i1 %669, label %blk_exit76, label %brif_next78

blk_exit76:                                       ; preds = %blk_exit72
  %670 = add i32 %5, %668
  %671 = shl i32 %670, 2
  %672 = add i32 %671, %_arg_1
  %673 = inttoptr i32 %672 to ptr
  %674 = load i32, ptr %673, align 4
  br label %blk_exit52

brif_next78:                                      ; preds = %blk_exit72
  %calcOffset79 = add i32 %stack_addr, -2608
  %675 = inttoptr i32 %calcOffset79 to ptr
  store i32 1710, ptr %675, align 4
  %676 = load i32, ptr null, align 4
  %677 = call i32 @fprintf(i32 %676, i32 1191, i32 %calcOffset79)
  br label %blk_exit49

blk_exit69:                                       ; preds = %blk_exit64
  %calcOffset70 = add i32 %stack_addr, -2624
  %678 = inttoptr i32 %calcOffset70 to ptr
  store i32 1625, ptr %678, align 4
  %679 = load i32, ptr null, align 4
  %680 = call i32 @fprintf(i32 %679, i32 1191, i32 %calcOffset70)
  br label %blk_exit49

brif_next66:                                      ; preds = %brif_next63
  %calcOffset67 = add i32 %stack_addr, -2800
  %681 = inttoptr i32 %calcOffset67 to ptr
  store i32 %642, ptr %681, align 4
  %682 = load i32, ptr null, align 4
  %683 = call i32 @fprintf(i32 %682, i32 2159, i32 %calcOffset67)
  br label %blk_exit49

blk_exit52:                                       ; preds = %blk_exit68, %blk_exit76, %loop_entry51, %blk_exit64
  %_local_11.0 = phi i32 [ -1, %blk_exit64 ], [ -1, %loop_entry51 ], [ %657, %blk_exit76 ], [ %657, %blk_exit68 ]
  %_local_10.0 = phi i32 [ %642, %blk_exit64 ], [ %642, %loop_entry51 ], [ %674, %blk_exit76 ], [ %661, %blk_exit68 ]
  %_local_8.1 = phi i32 [ %_local_8.0, %blk_exit64 ], [ %_local_8.0, %loop_entry51 ], [ %668, %blk_exit76 ], [ %_local_8.0, %blk_exit68 ]
  %684 = call i32 @strlen(i32 %_local_10.0)
  %685 = icmp ult i32 %684, 3
  br i1 %685, label %blk_exit81, label %brif_next83

blk_exit81:                                       ; preds = %brif_next89, %brif_next86, %brif_next83, %blk_exit52
  %686 = phi i1 [ true, %blk_exit52 ], [ false, %brif_next89 ], [ true, %brif_next86 ], [ true, %brif_next83 ]
  %_local_3.2 = phi i32 [ 1121, %blk_exit52 ], [ 1282, %brif_next89 ], [ 1121, %brif_next86 ], [ 1121, %brif_next83 ]
  %_param_1.2 = phi i32 [ 1089, %blk_exit52 ], [ 1246, %brif_next89 ], [ 1089, %brif_next86 ], [ 1089, %brif_next83 ]
  %687 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val93.not = icmp eq i32 %687, 0
  br i1 %brif_val93.not, label %brif_next94, label %blk_exit92

brif_next94:                                      ; preds = %blk_exit81
  %calcOffset95 = add i32 %stack_addr, -2704
  %688 = inttoptr i32 %calcOffset95 to ptr
  store i32 %_param_1.2, ptr %688, align 4
  %689 = add i32 %stack_addr, -1536
  %690 = call i32 @snprintf(i32 %689, i32 512, i32 1191, i32 %calcOffset95)
  %calcOffset96 = add i32 %stack_addr, -2720
  %691 = inttoptr i32 %calcOffset96 to ptr
  store i32 %_local_3.2, ptr %691, align 4
  %692 = add i32 %stack_addr, -2048
  %693 = call i32 @snprintf(i32 %692, i32 512, i32 1191, i32 %calcOffset96)
  br label %blk_exit91

blk_exit91:                                       ; preds = %blk_exit97, %brif_next103, %brif_next94
  %694 = load i32, ptr inttoptr (i32 2224 to ptr), align 4
  %695 = icmp eq i32 %694, 0
  br i1 %695, label %blk_exit111, label %brif_next113

blk_exit111:                                      ; preds = %blk_exit91
  %696 = add i32 %stack_addr, -1536
  %697 = call i32 @add_file(i32 %_local_11.0, i32 %696, i32 0, i32 2268, i32 3328, i32 0)
  %698 = icmp eq i32 %697, 0
  br i1 %698, label %blk_exit41, label %blk_exit110

blk_exit110:                                      ; preds = %blk_exit111, %blk_exit203, %blk_exit157, %brif_next183
  %699 = add i32 %_local_8.1, 1
  %700 = icmp slt i32 %699, %6
  br i1 %700, label %loop_entry51, label %blk_exit42

brif_next113:                                     ; preds = %blk_exit91
  %701 = add i32 %stack_addr, -2560
  %702 = call i32 @strncpy(i32 %701, i32 %694, i32 512)
  %calcOffset114 = add i32 %stack_addr, -2049
  %703 = inttoptr i32 %calcOffset114 to ptr
  store i8 0, ptr %703, align 1
  br label %loop_entry117

loop_entry117:                                    ; preds = %blk_exit134, %brif_next113
  %_local_3.3 = phi i32 [ %701, %brif_next113 ], [ %storemerge771, %blk_exit134 ]
  %704 = inttoptr i32 %_local_3.3 to ptr
  %705 = load i8, ptr %704, align 1
  %brif_val120.not = icmp eq i8 %705, 0
  br i1 %brif_val120.not, label %brif_next121, label %blk_exit118

brif_next121:                                     ; preds = %loop_entry117
  %calcOffset122 = add i32 %stack_addr, -2564
  %calcOffset124 = add i32 %stack_addr, -2572
  %706 = ptrtoint ptr %stack to i32
  %707 = add i32 %706, -2576
  %708 = inttoptr i32 %707 to ptr
  call void @llvm.memset.p0.i64(ptr %708, i8 0, i64 16, i1 true)
  br label %blk_exit116

blk_exit116:                                      ; preds = %blk_exit145, %brif_next121
  %.pre-phi810 = phi i32 [ %calcOffset152, %blk_exit145 ], [ %calcOffset124, %brif_next121 ]
  %.pre-phi = phi i32 [ %779, %blk_exit145 ], [ %calcOffset122, %brif_next121 ]
  %709 = add i32 %stack_addr, -1536
  %710 = call i32 @add_file(i32 100, i32 %709, i32 0, i32 %.pre-phi, i32 %.pre-phi810, i32 0)
  %711 = icmp eq i32 %710, 0
  br label %blk_exit115

blk_exit115:                                      ; preds = %blk_exit116, %blk_exit145, %brif_next136
  %_local_12.0 = phi i1 [ %711, %blk_exit116 ], [ false, %blk_exit145 ], [ false, %brif_next136 ]
  %712 = add i32 %stack_addr, -1536
  %713 = add i32 %stack_addr, -2048
  %714 = call i32 @strncmp(i32 %712, i32 %713, i32 512)
  %715 = icmp eq i32 %714, 0
  br i1 %715, label %blk_exit160, label %brif_next162

blk_exit160:                                      ; preds = %brif_next162, %blk_exit115
  %calcOffset166 = add i32 %stack_addr, -2572
  %716 = inttoptr i32 %calcOffset166 to ptr
  %717 = load i32, ptr %716, align 4
  %718 = icmp eq i32 %717, 0
  br i1 %718, label %blk_exit156, label %brif_next168

brif_next168:                                     ; preds = %blk_exit160
  %719 = inttoptr i32 %717 to ptr
  store i32 %_local_11.0, ptr %719, align 4
  %720 = load i32, ptr inttoptr (i32 3328 to ptr), align 4
  %721 = icmp eq i32 %720, 0
  br i1 %721, label %blk_exit158, label %brif_next171

blk_exit158:                                      ; preds = %brif_next168
  store i32 %717, ptr inttoptr (i32 3328 to ptr), align 4
  %calcOffset200 = add i32 %stack_addr, -2564
  %722 = inttoptr i32 %calcOffset200 to ptr
  %723 = load i32, ptr %722, align 4
  store i32 %723, ptr inttoptr (i32 2268 to ptr), align 4
  br label %blk_exit157

brif_next171:                                     ; preds = %brif_next168
  %calcOffset172 = add i32 %stack_addr, -2564
  %724 = inttoptr i32 %calcOffset172 to ptr
  %725 = load i32, ptr %724, align 4
  %calcOffset173 = add i32 %720, 76
  %726 = inttoptr i32 %calcOffset173 to ptr
  store i32 %725, ptr %726, align 4
  store i32 %717, ptr inttoptr (i32 3328 to ptr), align 4
  %727 = load i32, ptr inttoptr (i32 2268 to ptr), align 4
  %calcOffset174 = add i32 %725, 80
  %728 = inttoptr i32 %calcOffset174 to ptr
  store i32 %727, ptr %728, align 4
  br label %blk_exit157

blk_exit156:                                      ; preds = %blk_exit157, %brif_next178, %blk_exit160
  br i1 %686, label %blk_exit203, label %brif_next205

blk_exit203:                                      ; preds = %brif_next205, %blk_exit156
  %729 = call i32 @getenv(i32 1518)
  %calcOffset207 = add i32 %stack_addr, -2780
  %730 = inttoptr i32 %calcOffset207 to ptr
  store i32 %_local_10.0, ptr %730, align 4
  %calcOffset208 = add i32 %stack_addr, -2784
  %731 = inttoptr i32 %calcOffset208 to ptr
  store i32 %729, ptr %731, align 4
  %732 = call i32 @snprintf(i32 %713, i32 512, i32 1174, i32 %calcOffset208)
  %733 = call i32 @add_file(i32 %_local_11.0, i32 %713, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val209.not = icmp eq i32 %733, 0
  br i1 %brif_val209.not, label %blk_exit41, label %blk_exit110

brif_next205:                                     ; preds = %blk_exit156
  %734 = add i32 %684, %_local_10.0
  %735 = add i32 %734, -2
  %736 = inttoptr i32 %735 to ptr
  store i8 45, ptr %736, align 1
  br label %blk_exit203

blk_exit157:                                      ; preds = %blk_exit158, %brif_next176, %blk_exit159, %brif_next171
  br i1 %_local_12.0, label %blk_exit156, label %blk_exit110

brif_next162:                                     ; preds = %blk_exit115
  %737 = add i32 %stack_addr, -2568
  %738 = add i32 %stack_addr, -2576
  %739 = call i32 @add_file(i32 100, i32 %713, i32 0, i32 %737, i32 %738, i32 0)
  %740 = inttoptr i32 %737 to ptr
  %741 = load i32, ptr %740, align 4
  %brif_val164.not = icmp eq i32 %741, 0
  br i1 %brif_val164.not, label %blk_exit160, label %blk_exit159

blk_exit159:                                      ; preds = %brif_next162
  %742 = call i32 @strncmp(i32 %712, i32 %713, i32 512)
  %743 = icmp eq i32 %742, 0
  br i1 %743, label %blk_exit157, label %brif_next176

brif_next176:                                     ; preds = %blk_exit159
  %744 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val177.not = icmp eq i32 %744, 0
  br i1 %brif_val177.not, label %brif_next178, label %blk_exit157

brif_next178:                                     ; preds = %brif_next176
  %calcOffset179 = add i32 %stack_addr, -2564
  %745 = inttoptr i32 %calcOffset179 to ptr
  %746 = load i32, ptr %745, align 4
  %747 = icmp eq i32 %746, 0
  br i1 %747, label %blk_exit156, label %brif_next181

brif_next181:                                     ; preds = %brif_next178
  %748 = call i32 @malloc(i32 84)
  %749 = icmp eq i32 %748, 0
  br i1 %749, label %blk_exit48, label %brif_next183

blk_exit48:                                       ; preds = %brif_next181
  %calcOffset217 = add i32 %stack_addr, -2768
  %750 = inttoptr i32 %calcOffset217 to ptr
  store i32 1987, ptr %750, align 4
  %751 = load i32, ptr null, align 4
  %752 = call i32 @fprintf(i32 %751, i32 1191, i32 %calcOffset217)
  call void @exit(i32 1) #3
  unreachable

brif_next183:                                     ; preds = %brif_next181
  %753 = add i32 %748, 12
  %754 = inttoptr i32 %753 to ptr
  call void @llvm.memset.p0.i64(ptr %754, i8 -1, i64 8, i1 true)
  %755 = add i32 %748, 4
  %756 = inttoptr i32 %755 to ptr
  call void @llvm.memset.p0.i64(ptr %756, i8 -1, i64 8, i1 true)
  %757 = add i32 %748, 40
  %758 = inttoptr i32 %757 to ptr
  call void @llvm.memset.p0.i64(ptr %758, i8 0, i64 24, i1 true)
  %759 = add i32 %748, 20
  %760 = inttoptr i32 %759 to ptr
  call void @llvm.memset.p0.i64(ptr %760, i8 0, i64 8, i1 true)
  %761 = add i32 %748, 72
  %762 = inttoptr i32 %761 to ptr
  call void @llvm.memset.p0.i64(ptr %762, i8 0, i64 12, i1 true)
  %calcOffset192 = add i32 %746, 72
  %763 = inttoptr i32 %calcOffset192 to ptr
  store i32 %748, ptr %763, align 4
  %764 = add i32 %748, 28
  %765 = inttoptr i32 %764 to ptr
  call void @llvm.memset.p0.i64(ptr %765, i8 0, i64 9, i1 true)
  %766 = inttoptr i32 %748 to ptr
  store i32 %_local_11.0, ptr %766, align 4
  %calcOffset196 = add i32 %748, 68
  %767 = inttoptr i32 %calcOffset196 to ptr
  store i32 %746, ptr %767, align 4
  %calcOffset197 = add i32 %746, 76
  %768 = inttoptr i32 %calcOffset197 to ptr
  store i32 %741, ptr %768, align 4
  %calcOffset198 = add i32 %741, 80
  %769 = inttoptr i32 %calcOffset198 to ptr
  store i32 %746, ptr %769, align 4
  %calcOffset199 = add i32 %741, 72
  %770 = inttoptr i32 %calcOffset199 to ptr
  store i32 %748, ptr %770, align 4
  store i32 %748, ptr inttoptr (i32 2268 to ptr), align 4
  store i32 %748, ptr inttoptr (i32 3328 to ptr), align 4
  br label %blk_exit110

blk_exit118:                                      ; preds = %loop_entry117
  %771 = call i32 @strchr(i32 %_local_3.3, i32 58)
  %brif_val128.not = icmp eq i32 %771, 0
  br i1 %brif_val128.not, label %blk_exit126, label %blk_exit127

blk_exit126:                                      ; preds = %blk_exit118, %blk_exit127
  %storemerge771 = phi i32 [ %794, %blk_exit127 ], [ 0, %blk_exit118 ]
  %calcOffset131 = add i32 %stack_addr, -2728
  %772 = inttoptr i32 %calcOffset131 to ptr
  store i32 %_local_10.0, ptr %772, align 4
  %calcOffset132 = add i32 %stack_addr, -2732
  %773 = inttoptr i32 %calcOffset132 to ptr
  store i32 %_local_3.3, ptr %773, align 4
  %calcOffset133 = add i32 %stack_addr, -2736
  %774 = inttoptr i32 %calcOffset133 to ptr
  store i32 %_param_1.2, ptr %774, align 4
  %775 = add i32 %stack_addr, -1024
  %776 = call i32 @snprintf(i32 %775, i32 1024, i32 1171, i32 %calcOffset133)
  %777 = call i32 @add_file(i32 %_local_11.0, i32 %775, i32 0, i32 2268, i32 3328, i32 0)
  %778 = icmp eq i32 %777, 0
  br i1 %778, label %blk_exit134, label %brif_next136

blk_exit134:                                      ; preds = %blk_exit126
  %779 = add i32 %stack_addr, -2564
  %780 = call i32 @strncpy(i32 %779, i32 %_local_3.3, i32 2)
  %calcOffset141 = add i32 %stack_addr, -2752
  %781 = inttoptr i32 %calcOffset141 to ptr
  store i32 %_param_1.2, ptr %781, align 4
  %calcOffset142 = add i32 %stack_addr, -2562
  %782 = inttoptr i32 %calcOffset142 to ptr
  store i8 0, ptr %782, align 1
  %calcOffset143 = add i32 %stack_addr, -2744
  %783 = inttoptr i32 %calcOffset143 to ptr
  store i32 %_local_10.0, ptr %783, align 4
  %calcOffset144 = add i32 %stack_addr, -2748
  %784 = inttoptr i32 %calcOffset144 to ptr
  store i32 %779, ptr %784, align 4
  %785 = call i32 @snprintf(i32 %775, i32 1024, i32 1171, i32 %calcOffset141)
  %786 = call i32 @add_file(i32 %_local_11.0, i32 %775, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val146.not = icmp eq i32 %786, 0
  %brif_val146.not.not = xor i1 %brif_val146.not, true
  %brif_val148.not = icmp eq i32 %storemerge771, 0
  %or.cond873 = or i1 %brif_val146.not.not, %brif_val148.not
  br i1 %or.cond873, label %blk_exit145, label %loop_entry117

blk_exit145:                                      ; preds = %blk_exit134
  %calcOffset152 = add i32 %stack_addr, -2572
  %787 = ptrtoint ptr %stack to i32
  %788 = add i32 %787, -2576
  %789 = inttoptr i32 %788 to ptr
  call void @llvm.memset.p0.i64(ptr %789, i8 0, i64 16, i1 true)
  br i1 %brif_val146.not, label %blk_exit116, label %blk_exit115

brif_next136:                                     ; preds = %blk_exit126
  %790 = ptrtoint ptr %stack to i32
  %791 = add i32 %790, -2576
  %792 = inttoptr i32 %791 to ptr
  call void @llvm.memset.p0.i64(ptr %792, i8 0, i64 16, i1 true)
  br label %blk_exit115

blk_exit127:                                      ; preds = %blk_exit118
  %793 = inttoptr i32 %771 to ptr
  store i8 0, ptr %793, align 1
  %794 = add i32 %771, 1
  br label %blk_exit126

blk_exit92:                                       ; preds = %blk_exit81
  %795 = call i32 @strncmp(i32 %_local_10.0, i32 1559, i32 1)
  %796 = icmp eq i32 %795, 0
  br i1 %796, label %blk_exit97, label %brif_next99

blk_exit97:                                       ; preds = %brif_next101, %brif_next99, %blk_exit92
  %calcOffset108 = add i32 %stack_addr, -2672
  %797 = inttoptr i32 %calcOffset108 to ptr
  store i32 %_local_10.0, ptr %797, align 4
  %798 = add i32 %stack_addr, -1536
  %799 = call i32 @snprintf(i32 %798, i32 512, i32 1191, i32 %calcOffset108)
  %calcOffset109 = add i32 %stack_addr, -2688
  %800 = inttoptr i32 %calcOffset109 to ptr
  store i32 %_local_10.0, ptr %800, align 4
  %801 = add i32 %stack_addr, -2048
  %802 = call i32 @snprintf(i32 %801, i32 512, i32 1191, i32 %calcOffset109)
  br label %blk_exit91

brif_next99:                                      ; preds = %blk_exit92
  %803 = call i32 @strncmp(i32 %_local_10.0, i32 1558, i32 2)
  %804 = icmp eq i32 %803, 0
  br i1 %804, label %blk_exit97, label %brif_next101

brif_next101:                                     ; preds = %brif_next99
  %805 = call i32 @strncmp(i32 %_local_10.0, i32 1557, i32 3)
  %806 = icmp eq i32 %805, 0
  br i1 %806, label %blk_exit97, label %brif_next103

brif_next103:                                     ; preds = %brif_next101
  %calcOffset104 = add i32 %stack_addr, -2636
  %807 = inttoptr i32 %calcOffset104 to ptr
  store i32 %_local_10.0, ptr %807, align 4
  %calcOffset105 = add i32 %stack_addr, -2640
  %808 = inttoptr i32 %calcOffset105 to ptr
  store i32 %_param_1.2, ptr %808, align 4
  %809 = add i32 %stack_addr, -1536
  %810 = call i32 @snprintf(i32 %809, i32 512, i32 1174, i32 %calcOffset105)
  %calcOffset106 = add i32 %stack_addr, -2652
  %811 = inttoptr i32 %calcOffset106 to ptr
  store i32 %_local_10.0, ptr %811, align 4
  %calcOffset107 = add i32 %stack_addr, -2656
  %812 = inttoptr i32 %calcOffset107 to ptr
  store i32 %_local_3.2, ptr %812, align 4
  %813 = add i32 %stack_addr, -2048
  %814 = call i32 @snprintf(i32 %813, i32 512, i32 1174, i32 %calcOffset107)
  br label %blk_exit91

brif_next83:                                      ; preds = %blk_exit52
  %815 = add i32 %684, %_local_10.0
  %816 = add i32 %815, -2
  %817 = inttoptr i32 %816 to ptr
  %818 = load i8, ptr %817, align 1
  %.not = icmp eq i8 %818, 45
  br i1 %.not, label %brif_next86, label %blk_exit81

brif_next86:                                      ; preds = %brif_next83
  %819 = add i32 %815, -1
  %820 = inttoptr i32 %819 to ptr
  %821 = load i8, ptr %820, align 1
  %.not770 = icmp eq i8 %821, 111
  br i1 %.not770, label %brif_next89, label %blk_exit81

brif_next89:                                      ; preds = %brif_next86
  store i8 0, ptr %817, align 1
  br label %blk_exit81

blk_exit11:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2280 to ptr), align 1
  br label %loop_entry

blk_exit8:                                        ; preds = %loop_entry
  call void @program_version()
  %calcOffset = add i32 %stack_addr, -2592
  %822 = inttoptr i32 %calcOffset to ptr
  store i32 2304, ptr %822, align 4
  %823 = call i32 @printf(i32 1755, i32 %calcOffset)
  call void @exit(i32 0)
  unreachable

blk_exit9:                                        ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2294 to ptr), align 1
  br label %loop_entry

blk_exit12:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2273 to ptr), align 1
  store i8 0, ptr inttoptr (i32 2274 to ptr), align 1
  br label %loop_entry

blk_exit13:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2293 to ptr), align 1
  br label %loop_entry

blk_exit14:                                       ; preds = %loop_entry
  %824 = load i32, ptr null, align 4
  %825 = call i32 @atoi(i32 %824)
  store i32 %825, ptr inttoptr (i32 2208 to ptr), align 4
  br label %loop_entry

blk_exit10:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2228 to ptr), align 1
  %826 = load i32, ptr null, align 4
  br label %loop_entry

blk_exit15:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2274 to ptr), align 1
  store i8 0, ptr inttoptr (i32 2273 to ptr), align 1
  br label %loop_entry

blk_exit16:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2264 to ptr), align 1
  br label %loop_entry

blk_exit17:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2272 to ptr), align 1
  br label %loop_entry

blk_exit7:                                        ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2295 to ptr), align 1
  br label %loop_entry

blk_exit18:                                       ; preds = %loop_entry
  store i8 1, ptr inttoptr (i32 2292 to ptr), align 1
  br label %loop_entry
}

; Function Attrs: null_pointer_is_valid
define internal void @program_version() #0 {
allocator:
  %stack = alloca [16 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint ptr %stack to i32
  %stack_end = add i32 %stack_addr, -16
  %calcOffset = add i32 %stack_addr, -12
  %0 = inttoptr i32 %calcOffset to ptr
  store i32 1550, ptr %0, align 4
  %1 = inttoptr i32 %stack_end to ptr
  store i32 1324, ptr %1, align 4
  %2 = call i32 @snprintf(i32 2304, i32 1024, i32 1180, i32 %stack_end)
  ret void
}

; Function Attrs: null_pointer_is_valid
define internal void @usage() #0 {
allocator:
  %stack = alloca [144 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint ptr %stack to i32
  %stack_end = add i32 %stack_addr, -144
  call void @program_version()
  %calcOffset = add i32 %stack_addr, -16
  %0 = inttoptr i32 %calcOffset to ptr
  store i32 2304, ptr %0, align 4
  %1 = load i32, ptr null, align 4
  %2 = add i32 %stack_addr, -16
  %3 = call i32 @fprintf(i32 %1, i32 1755, i32 %2)
  %calcOffset1 = add i32 %stack_addr, -32
  %4 = inttoptr i32 %calcOffset1 to ptr
  store i32 1417, ptr %4, align 4
  %5 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset1)
  %calcOffset2 = add i32 %stack_addr, -48
  %6 = inttoptr i32 %calcOffset2 to ptr
  store i32 1322, ptr %6, align 4
  %7 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset2)
  %calcOffset3 = add i32 %stack_addr, -64
  %8 = inttoptr i32 %calcOffset3 to ptr
  store i32 1242, ptr %8, align 4
  %9 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset3)
  %calcOffset4 = add i32 %stack_addr, -80
  %10 = inttoptr i32 %calcOffset4 to ptr
  store i32 1240, ptr %10, align 4
  %11 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset4)
  %calcOffset5 = add i32 %stack_addr, -96
  %12 = inttoptr i32 %calcOffset5 to ptr
  store i32 1198, ptr %12, align 4
  %13 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset5)
  %calcOffset6 = add i32 %stack_addr, -112
  %14 = inttoptr i32 %calcOffset6 to ptr
  store i32 1429, ptr %14, align 4
  %15 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset6)
  %calcOffset7 = add i32 %stack_addr, -128
  %16 = inttoptr i32 %calcOffset7 to ptr
  store i32 1433, ptr %16, align 4
  %17 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset7)
  %18 = inttoptr i32 %stack_end to ptr
  store i32 1873, ptr %18, align 4
  %19 = call i32 @fprintf(i32 %1, i32 1191, i32 %stack_end)
  call void @exit(i32 1) #3
  unreachable
}

; Function Attrs: null_pointer_is_valid
define internal range(i32 0, 2) i32 @add_file(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3, i32 %_arg_4, i32 %_arg_5) #0 {
allocator:
  %stack = alloca [1696 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint ptr %stack to i32
  %stack_end = add i32 %stack_addr, -1696
  %brif_val.not = icmp eq i32 %_arg_2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

brif_next:                                        ; preds = %allocator
  %0 = call i32 @strdup(i32 %_arg_1)
  br label %blk_exit6

blk_exit6:                                        ; preds = %brif_next9, %brif_next
  %_local_7.0 = phi i32 [ %0, %brif_next ], [ %200, %brif_next9 ]
  %1 = inttoptr i32 %_local_7.0 to ptr
  %2 = load i8, ptr %1, align 1
  %.not = icmp eq i8 %2, 47
  br i1 %.not, label %brif_next15, label %blk_exit6.blk_exit12_crit_edge

brif_next15:                                      ; preds = %blk_exit6
  %3 = add i32 %stack_addr, -1536
  %4 = call i32 @stat(i32 %_local_7.0, i32 %3)
  %5 = icmp eq i32 %4, 0
  br i1 %5, label %blk_exit12, label %brif_next17

brif_next17:                                      ; preds = %brif_next15
  %6 = load i32, ptr null, align 4
  switch i32 %6, label %blk_exit18 [
    i32 44, label %blk_exit11
    i32 54, label %blk_exit11
  ]

blk_exit18:                                       ; preds = %brif_next17
  call void @perror(i32 1041)
  call void @exit(i32 1) #3
  unreachable

blk_exit12:                                       ; preds = %blk_exit6.blk_exit12_crit_edge, %brif_next15
  %.pre-phi234 = phi i32 [ %.pre, %blk_exit6.blk_exit12_crit_edge ], [ %3, %brif_next15 ]
  %7 = icmp eq i32 %_arg_5, 0
  %8 = call i32 @stat(i32 %_local_7.0, i32 %.pre-phi234)
  %9 = icmp slt i32 %8, 0
  %calcOffset19 = add i32 %stack_addr, -1512
  %10 = inttoptr i32 %calcOffset19 to ptr
  %11 = load i32, ptr %10, align 4
  %12 = and i32 %11, 57344
  %13 = icmp ne i32 %12, 16384
  %.not233 = or i1 %9, %13
  %.not230 = or i1 %7, %.not233
  %brif_val22.not = icmp sgt i32 %8, -1
  %or.cond = and i1 %.not230, %brif_val22.not
  br i1 %or.cond, label %brif_next23, label %blk_exit11

brif_next23:                                      ; preds = %blk_exit12
  %14 = call i32 @open(i32 %_local_7.0, i32 67108864, i32 0)
  %brif_val26.not = icmp sgt i32 %14, -1
  br i1 %brif_val26.not, label %brif_next27, label %blk_exit25

brif_next27:                                      ; preds = %brif_next23
  %15 = load i8, ptr %1, align 1
  %16 = icmp eq i8 %15, 47
  br i1 %16, label %blk_exit24, label %blk_exit25

blk_exit24:                                       ; preds = %brif_next27
  %17 = call i32 @malloc(i32 84)
  %18 = icmp eq i32 %17, 0
  br i1 %18, label %blk_exit4, label %brif_next61

blk_exit4:                                        ; preds = %blk_exit24
  %calcOffset219 = add i32 %stack_addr, -1664
  %19 = inttoptr i32 %calcOffset219 to ptr
  store i32 1987, ptr %19, align 4
  %20 = load i32, ptr null, align 4
  %21 = call i32 @fprintf(i32 %20, i32 1191, i32 %calcOffset219)
  call void @exit(i32 1) #3
  unreachable

brif_next61:                                      ; preds = %blk_exit24
  %22 = add i32 %17, 40
  %23 = inttoptr i32 %22 to ptr
  call void @llvm.memset.p0.i64(ptr %23, i8 0, i64 24, i1 true)
  %calcOffset65 = add i32 %17, 36
  %24 = inttoptr i32 %calcOffset65 to ptr
  store i8 0, ptr %24, align 1
  %calcOffset66 = add i32 %17, 16
  %25 = inttoptr i32 %calcOffset66 to ptr
  store i32 0, ptr %25, align 4
  %26 = add i32 %17, 8
  %27 = inttoptr i32 %26 to ptr
  call void @llvm.memset.p0.i64(ptr %27, i8 -1, i64 8, i1 true)
  %calcOffset68 = add i32 %17, 76
  %28 = inttoptr i32 %calcOffset68 to ptr
  %29 = add i32 %17, 76
  %30 = inttoptr i32 %29 to ptr
  call void @llvm.memset.p0.i64(ptr %30, i8 0, i64 8, i1 true)
  %calcOffset69 = add i32 %17, 20
  %31 = inttoptr i32 %calcOffset69 to ptr
  %calcOffset70 = add i32 %17, 68
  %32 = add i32 %17, 68
  %33 = inttoptr i32 %32 to ptr
  call void @llvm.memset.p0.i64(ptr %33, i8 0, i64 8, i1 true)
  %calcOffset71 = add i32 %17, 28
  %34 = inttoptr i32 %calcOffset71 to ptr
  %35 = add i32 %17, 20
  %36 = inttoptr i32 %35 to ptr
  call void @llvm.memset.p0.i64(ptr %36, i8 0, i64 16, i1 true)
  %calcOffset72 = add i32 %17, 4
  %37 = inttoptr i32 %calcOffset72 to ptr
  store i32 %14, ptr %37, align 4
  %38 = inttoptr i32 %17 to ptr
  store i32 %_arg_0, ptr %38, align 4
  %39 = call i32 @strdup(i32 %_arg_1)
  store i32 %39, ptr %31, align 4
  %40 = call i32 @strdup(i32 %_local_7.0)
  %calcOffset75 = add i32 %17, 38
  %41 = inttoptr i32 %calcOffset75 to ptr
  store i8 0, ptr %41, align 1
  %calcOffset76 = add i32 %17, 24
  %42 = inttoptr i32 %calcOffset76 to ptr
  store i32 %40, ptr %42, align 4
  %43 = call i32 @strlen(i32 %_local_7.0)
  %44 = add i32 %43, 6
  %45 = call i32 @malloc(i32 %44)
  %46 = icmp eq i32 %45, 0
  br i1 %46, label %blk_exit3, label %brif_next78

blk_exit3:                                        ; preds = %brif_next61
  %calcOffset220 = add i32 %stack_addr, -1648
  %47 = inttoptr i32 %calcOffset220 to ptr
  store i32 1987, ptr %47, align 4
  %48 = load i32, ptr null, align 4
  %49 = call i32 @fprintf(i32 %48, i32 1191, i32 %calcOffset220)
  call void @exit(i32 1) #3
  unreachable

brif_next78:                                      ; preds = %brif_next61
  %calcOffset79 = add i32 %stack_addr, -1568
  %50 = inttoptr i32 %calcOffset79 to ptr
  store i32 %_local_7.0, ptr %50, align 4
  %51 = add i32 %43, 5
  %52 = call i32 @snprintf(i32 %45, i32 %51, i32 1544, i32 %calcOffset79)
  %53 = call i32 @stat(i32 %45, i32 %.pre-phi234)
  %brif_val81.not = icmp eq i32 %53, 0
  br i1 %brif_val81.not, label %brif_next82, label %blk_exit80

brif_next82:                                      ; preds = %brif_next78
  store i8 1, ptr %41, align 1
  br label %blk_exit80

blk_exit80:                                       ; preds = %brif_next82, %brif_next78
  call void @free(i32 %45)
  %calcOffset84 = add i32 %17, 72
  %54 = inttoptr i32 %calcOffset84 to ptr
  store i32 %_arg_5, ptr %54, align 4
  %.not222 = icmp eq i32 %12, 16384
  %or.cond227 = or i1 %9, %.not222
  br i1 %or.cond227, label %blk_exit90, label %blk_exit89

blk_exit90:                                       ; preds = %blk_exit80
  %55 = load i32, ptr %37, align 4
  %56 = call i32 @close(i32 %55)
  store i32 -1, ptr %37, align 4
  %57 = load i32, ptr %42, align 4
  %58 = call i32 @opendir(i32 %57)
  %brif_val99.not = icmp eq i32 %58, 0
  br i1 %brif_val99.not, label %brif_next100, label %blk_exit97

brif_next100:                                     ; preds = %blk_exit90
  %59 = load i32, ptr %42, align 4
  call void @perror(i32 %59)
  br label %blk_exit88

blk_exit97:                                       ; preds = %blk_exit90
  %calcOffset102 = add i32 %17, 64
  %60 = inttoptr i32 %calcOffset102 to ptr
  store i32 0, ptr %60, align 4
  %calcOffset103 = add i32 %stack_addr, -512
  %61 = inttoptr i32 %calcOffset103 to ptr
  store i32 0, ptr %61, align 4
  %62 = call i32 @malloc(i32 800)
  %63 = icmp eq i32 %62, 0
  br i1 %63, label %blk_exit2, label %loop_entry107

blk_exit2:                                        ; preds = %blk_exit97
  call void @perror(i32 1569)
  call void @exit(i32 -1) #3
  unreachable

loop_entry107:                                    ; preds = %blk_exit97, %blk_exit113, %brif_next109
  %_local_8.1 = phi i32 [ %106, %blk_exit113 ], [ %_local_8.1, %brif_next109 ], [ 0, %blk_exit97 ]
  %_local_10.0 = phi i32 [ %_local_10.1, %blk_exit113 ], [ %_local_10.0, %brif_next109 ], [ %62, %blk_exit97 ]
  %_local_11.0 = phi i32 [ %_local_11.1, %blk_exit113 ], [ %_local_11.0, %brif_next109 ], [ 200, %blk_exit97 ]
  %64 = call i32 @readdir(i32 %58)
  %65 = icmp eq i32 %64, 0
  br i1 %65, label %blk_exit106, label %brif_next109

blk_exit106:                                      ; preds = %loop_entry107
  %66 = call i32 @closedir(i32 %58)
  call void @qsort(i32 %_local_10.0, i32 %_local_8.1, i32 4, i32 1)
  %67 = icmp eq i32 %_local_8.1, 0
  br i1 %67, label %blk_exit119, label %loop_entry122

blk_exit119:                                      ; preds = %blk_exit123, %blk_exit106
  call void @free(i32 %_local_10.0)
  %68 = load i32, ptr %60, align 4
  %brif_val134.not = icmp eq i32 %68, 0
  br i1 %brif_val134.not, label %brif_next135, label %blk_exit85

brif_next135:                                     ; preds = %blk_exit119
  %69 = load i32, ptr %42, align 4
  %70 = call i32 @strcmp(i32 1121, i32 %69)
  %71 = icmp eq i32 %70, 0
  br i1 %71, label %blk_exit87, label %brif_next138

blk_exit87:                                       ; preds = %brif_next138, %brif_next135
  call void @free(i32 %_local_7.0)
  %72 = load i32, ptr %34, align 4
  call void @free(i32 %72)
  %calcOffset192 = add i32 %17, 32
  %73 = inttoptr i32 %calcOffset192 to ptr
  %74 = load i32, ptr %73, align 4
  call void @free(i32 %74)
  %75 = load i32, ptr %31, align 4
  call void @free(i32 %75)
  %76 = load i32, ptr %42, align 4
  call void @free(i32 %76)
  %77 = load i32, ptr %37, align 4
  %78 = icmp sgt i32 %77, -1
  br i1 %78, label %blk_exit195, label %blk_exit11

blk_exit195:                                      ; preds = %blk_exit87
  %79 = call i32 @close(i32 %77)
  br label %blk_exit11

brif_next138:                                     ; preds = %brif_next135
  %80 = call i32 @strcmp(i32 1282, i32 %69)
  %81 = icmp eq i32 %80, 0
  br i1 %81, label %blk_exit87, label %brif_next140

brif_next140:                                     ; preds = %brif_next138
  %calcOffset141 = add i32 %stack_addr, -1584
  %82 = inttoptr i32 %calcOffset141 to ptr
  store i32 %69, ptr %82, align 4
  %83 = load i32, ptr null, align 4
  %84 = call i32 @fprintf(i32 %83, i32 1942, i32 %calcOffset141)
  br label %blk_exit88

loop_entry122:                                    ; preds = %blk_exit106, %blk_exit123
  %_local_8.2 = phi i32 [ %92, %blk_exit123 ], [ %_local_8.1, %blk_exit106 ]
  %_param_1.0 = phi i32 [ %91, %blk_exit123 ], [ %_local_10.0, %blk_exit106 ]
  %85 = inttoptr i32 %_param_1.0 to ptr
  %86 = load i32, ptr %85, align 4
  %87 = load i32, ptr %42, align 4
  %88 = call i32 @add_file(i32 -1, i32 %86, i32 %87, i32 %calcOffset70, i32 %calcOffset103, i32 %17)
  %89 = icmp eq i32 %88, 0
  br i1 %89, label %blk_exit123, label %brif_next127

blk_exit123:                                      ; preds = %brif_next127, %loop_entry122
  %90 = load i32, ptr %85, align 4
  call void @free(i32 %90)
  %91 = add i32 %_param_1.0, 4
  %92 = add i32 %_local_8.2, -1
  %brif_val131.not = icmp eq i32 %92, 0
  br i1 %brif_val131.not, label %blk_exit119, label %loop_entry122

brif_next127:                                     ; preds = %loop_entry122
  %93 = load i32, ptr %60, align 4
  %94 = add i32 %93, 1
  store i32 %94, ptr %60, align 4
  br label %blk_exit123

brif_next109:                                     ; preds = %loop_entry107
  %calcOffset110 = add i32 %64, 9
  %95 = inttoptr i32 %calcOffset110 to ptr
  %96 = load i8, ptr %95, align 1
  %97 = icmp eq i8 %96, 0
  br i1 %97, label %loop_entry107, label %brif_next112

brif_next112:                                     ; preds = %brif_next109
  %98 = call i32 @strdup(i32 %calcOffset110)
  %.not223 = icmp eq i32 %_local_8.1, %_local_11.0
  br i1 %.not223, label %brif_next115, label %blk_exit113

brif_next115:                                     ; preds = %brif_next112
  %99 = add i32 %_local_8.1, 200
  %100 = shl i32 %99, 2
  %101 = call i32 @realloc(i32 %_local_10.0, i32 %100)
  %102 = icmp eq i32 %101, 0
  br i1 %102, label %blk_exit1, label %blk_exit113

blk_exit1:                                        ; preds = %brif_next115
  call void @perror(i32 1569)
  call void @exit(i32 -1) #3
  unreachable

blk_exit113:                                      ; preds = %brif_next115, %brif_next112
  %_local_10.1 = phi i32 [ %101, %brif_next115 ], [ %_local_10.0, %brif_next112 ]
  %_local_11.1 = phi i32 [ %99, %brif_next115 ], [ %_local_11.0, %brif_next112 ]
  %103 = shl i32 %_local_8.1, 2
  %104 = add i32 %_local_10.1, %103
  %105 = inttoptr i32 %104 to ptr
  store i32 %98, ptr %105, align 4
  %106 = add i32 %_local_8.1, 1
  br label %loop_entry107

blk_exit88:                                       ; preds = %brif_next178, %brif_next171, %brif_next169, %brif_next167, %brif_next165, %brif_next163, %brif_next161, %brif_next159, %brif_next157, %brif_next155, %brif_next153, %brif_next151, %brif_next149, %brif_next147, %blk_exit89, %brif_next140, %brif_next100
  br i1 %7, label %brif_next181, label %blk_exit179

brif_next181:                                     ; preds = %blk_exit88
  %calcOffset182 = add i32 %stack_addr, -1632
  %107 = inttoptr i32 %calcOffset182 to ptr
  store i32 %_local_7.0, ptr %107, align 4
  %108 = load i32, ptr null, align 4
  %109 = call i32 @fprintf(i32 %108, i32 1581, i32 %calcOffset182)
  br label %blk_exit179

blk_exit179:                                      ; preds = %brif_next181, %blk_exit88
  call void @free(i32 %_local_7.0)
  %110 = load i32, ptr %34, align 4
  call void @free(i32 %110)
  %calcOffset184 = add i32 %17, 32
  %111 = inttoptr i32 %calcOffset184 to ptr
  %112 = load i32, ptr %111, align 4
  call void @free(i32 %112)
  %113 = load i32, ptr %31, align 4
  call void @free(i32 %113)
  %114 = load i32, ptr %42, align 4
  call void @free(i32 %114)
  %115 = load i32, ptr %37, align 4
  %116 = icmp sgt i32 %115, -1
  br i1 %116, label %blk_exit187, label %blk_exit11

blk_exit187:                                      ; preds = %blk_exit179
  %117 = call i32 @close(i32 %115)
  br label %blk_exit11

blk_exit85:                                       ; preds = %blk_exit86, %blk_exit119
  %118 = inttoptr i32 %_arg_3 to ptr
  %119 = load i32, ptr %118, align 4
  %brif_val203.not = icmp eq i32 %119, 0
  br i1 %brif_val203.not, label %brif_next204, label %blk_exit201

brif_next204:                                     ; preds = %blk_exit85
  %120 = inttoptr i32 %_arg_4 to ptr
  store i32 %17, ptr %120, align 4
  br label %blk_exit200

blk_exit200:                                      ; preds = %blk_exit206, %brif_next209, %brif_next204
  %.pre-phi = phi ptr [ %118, %blk_exit206 ], [ %122, %brif_next209 ], [ %118, %brif_next204 ]
  store i32 %17, ptr %.pre-phi, align 4
  br label %blk_exit11

blk_exit201:                                      ; preds = %blk_exit85
  %121 = load i32, ptr %38, align 4
  %.not224 = icmp eq i32 %121, -1
  br i1 %.not224, label %brif_next209, label %blk_exit206

brif_next209:                                     ; preds = %blk_exit201
  %122 = inttoptr i32 %_arg_4 to ptr
  %123 = load i32, ptr %122, align 4
  %calcOffset211 = add i32 %123, 76
  %124 = inttoptr i32 %calcOffset211 to ptr
  store i32 %17, ptr %124, align 4
  %125 = load i32, ptr %122, align 4
  %calcOffset213 = add i32 %17, 80
  %126 = inttoptr i32 %calcOffset213 to ptr
  store i32 %125, ptr %126, align 4
  br label %blk_exit200

blk_exit206:                                      ; preds = %blk_exit201
  %calcOffset214 = add i32 %119, 80
  %127 = inttoptr i32 %calcOffset214 to ptr
  store i32 %17, ptr %127, align 4
  %128 = load i32, ptr %118, align 4
  store i32 %128, ptr %28, align 4
  br label %blk_exit200

blk_exit89:                                       ; preds = %blk_exit80
  %129 = call i32 @strrchr(i32 %_local_7.0, i32 47)
  %130 = add i32 %129, 1
  %select_cond.not = icmp eq i32 %129, 0
  %131 = select i1 %select_cond.not, i32 %_local_7.0, i32 %130
  %132 = inttoptr i32 %131 to ptr
  %133 = load i8, ptr %132, align 1
  %134 = icmp eq i8 %133, 46
  br i1 %134, label %blk_exit88, label %brif_next144

brif_next144:                                     ; preds = %blk_exit89
  %135 = call i32 @strrchr(i32 %131, i32 46)
  %136 = icmp eq i32 %135, 0
  br i1 %136, label %blk_exit145, label %brif_next147

blk_exit145:                                      ; preds = %brif_next171, %brif_next144
  %137 = call i32 @strlen(i32 %_local_7.0)
  %138 = add i32 %137, 7
  %139 = call i32 @malloc(i32 %138)
  %140 = icmp eq i32 %139, 0
  br i1 %140, label %blk_exit, label %brif_next175

blk_exit:                                         ; preds = %blk_exit145
  %calcOffset221 = add i32 %stack_addr, -1616
  %141 = inttoptr i32 %calcOffset221 to ptr
  store i32 1987, ptr %141, align 4
  %142 = load i32, ptr null, align 4
  %143 = call i32 @fprintf(i32 %142, i32 1191, i32 %calcOffset221)
  call void @exit(i32 1) #3
  unreachable

brif_next175:                                     ; preds = %blk_exit145
  %calcOffset176 = add i32 %stack_addr, -1600
  %144 = inttoptr i32 %calcOffset176 to ptr
  store i32 %_local_7.0, ptr %144, align 4
  %145 = add i32 %137, 6
  %146 = call i32 @snprintf(i32 %139, i32 %145, i32 1074, i32 %calcOffset176)
  %147 = call i32 @access(i32 %139, i32 4)
  %148 = icmp sgt i32 %147, -1
  br i1 %148, label %blk_exit86, label %brif_next178

blk_exit86:                                       ; preds = %brif_next175
  store i32 %139, ptr %34, align 4
  br label %blk_exit85

brif_next178:                                     ; preds = %brif_next175
  call void @free(i32 %139)
  br label %blk_exit88

brif_next147:                                     ; preds = %brif_next144
  %149 = add i32 %135, 1
  %150 = call i32 @strcmp(i32 %149, i32 1077)
  %151 = icmp eq i32 %150, 0
  br i1 %151, label %blk_exit88, label %brif_next149

brif_next149:                                     ; preds = %brif_next147
  %152 = call i32 @strcmp(i32 %149, i32 1085)
  %153 = icmp eq i32 %152, 0
  br i1 %153, label %blk_exit88, label %brif_next151

brif_next151:                                     ; preds = %brif_next149
  %154 = call i32 @strcmp(i32 %149, i32 1412)
  %155 = icmp eq i32 %154, 0
  br i1 %155, label %blk_exit88, label %brif_next153

brif_next153:                                     ; preds = %brif_next151
  %156 = call i32 @strcmp(i32 %149, i32 1244)
  %157 = icmp eq i32 %156, 0
  br i1 %157, label %blk_exit88, label %brif_next155

brif_next155:                                     ; preds = %brif_next153
  %158 = call i32 @strcmp(i32 %149, i32 1196)
  %159 = icmp eq i32 %158, 0
  br i1 %159, label %blk_exit88, label %brif_next157

brif_next157:                                     ; preds = %brif_next155
  %160 = call i32 @strcmp(i32 %149, i32 1242)
  %161 = icmp eq i32 %160, 0
  br i1 %161, label %blk_exit88, label %brif_next159

brif_next159:                                     ; preds = %brif_next157
  %162 = call i32 @strcmp(i32 %149, i32 1322)
  %163 = icmp eq i32 %162, 0
  br i1 %163, label %blk_exit88, label %brif_next161

brif_next161:                                     ; preds = %brif_next159
  %164 = call i32 @strcmp(i32 %149, i32 1163)
  %165 = icmp eq i32 %164, 0
  br i1 %165, label %blk_exit88, label %brif_next163

brif_next163:                                     ; preds = %brif_next161
  %166 = call i32 @strcmp(i32 %149, i32 1204)
  %167 = icmp eq i32 %166, 0
  br i1 %167, label %blk_exit88, label %brif_next165

brif_next165:                                     ; preds = %brif_next163
  %168 = call i32 @strcmp(i32 %149, i32 1336)
  %169 = icmp eq i32 %168, 0
  br i1 %169, label %blk_exit88, label %brif_next167

brif_next167:                                     ; preds = %brif_next165
  %170 = call i32 @strcmp(i32 %149, i32 1159)
  %171 = icmp eq i32 %170, 0
  br i1 %171, label %blk_exit88, label %brif_next169

brif_next169:                                     ; preds = %brif_next167
  %172 = call i32 @strcmp(i32 %149, i32 1200)
  %173 = icmp eq i32 %172, 0
  br i1 %173, label %blk_exit88, label %brif_next171

brif_next171:                                     ; preds = %brif_next169
  %174 = call i32 @strcmp(i32 %149, i32 1221)
  %175 = icmp eq i32 %174, 0
  br i1 %175, label %blk_exit88, label %blk_exit145

blk_exit25:                                       ; preds = %brif_next27, %brif_next23
  %or.cond226 = select i1 %brif_val.not, i1 %7, i1 false
  br i1 %or.cond226, label %brif_next34, label %blk_exit11

brif_next34:                                      ; preds = %blk_exit25
  %176 = load i32, ptr inttoptr (i32 2224 to ptr), align 4
  %177 = icmp eq i32 %176, 0
  br i1 %177, label %blk_exit35, label %brif_next37

blk_exit35:                                       ; preds = %brif_next34
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

brif_next37:                                      ; preds = %brif_next34
  %178 = add i32 %stack_addr, -512
  %179 = call i32 @strncpy(i32 %178, i32 %176, i32 512)
  %calcOffset38 = add i32 %stack_addr, -1
  %180 = inttoptr i32 %calcOffset38 to ptr
  store i8 0, ptr %180, align 1
  br label %loop_entry

loop_entry:                                       ; preds = %brif_next55, %brif_next37
  %_param_2.0 = phi i32 [ %178, %brif_next37 ], [ %storemerge, %brif_next55 ]
  %181 = inttoptr i32 %_param_2.0 to ptr
  %182 = load i8, ptr %181, align 1
  %183 = icmp eq i8 %182, 0
  br i1 %183, label %blk_exit40, label %brif_next43

blk_exit40:                                       ; preds = %blk_exit53, %loop_entry
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

brif_next43:                                      ; preds = %loop_entry
  %184 = call i32 @strchr(i32 %_param_2.0, i32 58)
  %brif_val46.not = icmp eq i32 %184, 0
  br i1 %brif_val46.not, label %blk_exit44, label %blk_exit45

blk_exit44:                                       ; preds = %brif_next43, %blk_exit45
  %storemerge = phi i32 [ %195, %blk_exit45 ], [ 0, %brif_next43 ]
  %calcOffset49 = add i32 %stack_addr, -1676
  %185 = inttoptr i32 %calcOffset49 to ptr
  store i32 %_param_2.0, ptr %185, align 4
  %calcOffset50 = add i32 %stack_addr, -1680
  %186 = inttoptr i32 %calcOffset50 to ptr
  store i32 1089, ptr %186, align 4
  %187 = call i32 @snprintf(i32 %.pre-phi234, i32 1024, i32 1174, i32 %calcOffset50)
  %188 = call i32 @strncmp(i32 %_local_7.0, i32 %_param_2.0, i32 2)
  %189 = icmp eq i32 %188, 0
  br i1 %189, label %blk_exit11, label %brif_next52

brif_next52:                                      ; preds = %blk_exit44
  %190 = call i32 @strlen(i32 1089)
  %191 = add i32 %190, 3
  %192 = call i32 @strncmp(i32 %_local_7.0, i32 %.pre-phi234, i32 %191)
  %193 = icmp eq i32 %192, 0
  br i1 %193, label %blk_exit11, label %brif_next55

brif_next55:                                      ; preds = %brif_next52
  %brif_val56.not = icmp eq i32 %storemerge, 0
  br i1 %brif_val56.not, label %blk_exit53, label %loop_entry

blk_exit53:                                       ; preds = %brif_next55
  br i1 %193, label %blk_exit11, label %blk_exit40

blk_exit45:                                       ; preds = %brif_next43
  %194 = inttoptr i32 %184 to ptr
  store i8 0, ptr %194, align 1
  %195 = add i32 %184, 1
  br label %blk_exit44

blk_exit11:                                       ; preds = %brif_next52, %blk_exit87, %blk_exit179, %blk_exit44, %blk_exit53, %blk_exit40, %brif_next17, %blk_exit200, %blk_exit195, %blk_exit187, %blk_exit35, %blk_exit25, %blk_exit12, %brif_next17
  %_local_8.0 = phi i32 [ 0, %blk_exit187 ], [ 1, %blk_exit87 ], [ 1, %blk_exit195 ], [ 0, %blk_exit179 ], [ 1, %blk_exit200 ], [ 0, %blk_exit35 ], [ 0, %brif_next17 ], [ 0, %blk_exit25 ], [ 0, %blk_exit12 ], [ 0, %brif_next17 ], [ 0, %blk_exit40 ], [ 0, %blk_exit53 ], [ 0, %blk_exit44 ], [ 0, %brif_next52 ]
  %_local_7.1 = phi i32 [ %17, %blk_exit187 ], [ %17, %blk_exit87 ], [ %17, %blk_exit195 ], [ %17, %blk_exit179 ], [ %_local_7.0, %blk_exit200 ], [ %_local_7.0, %blk_exit35 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %blk_exit25 ], [ %_local_7.0, %blk_exit12 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %blk_exit40 ], [ %_local_7.0, %blk_exit53 ], [ %_local_7.0, %blk_exit44 ], [ %_local_7.0, %brif_next52 ]
  call void @free(i32 %_local_7.1)
  ret i32 %_local_8.0

blk_exit6.blk_exit12_crit_edge:                   ; preds = %blk_exit6
  %.pre = add i32 %stack_addr, -1536
  br label %blk_exit12

blk_exit7:                                        ; preds = %allocator
  %196 = call i32 @strlen(i32 %_arg_2)
  %197 = call i32 @strlen(i32 %_arg_1)
  %198 = add i32 %196, %197
  %199 = add i32 %198, 13
  %200 = call i32 @malloc(i32 %199)
  %201 = icmp eq i32 %200, 0
  br i1 %201, label %blk_exit5, label %brif_next9

blk_exit5:                                        ; preds = %blk_exit7
  %202 = inttoptr i32 %stack_end to ptr
  store i32 1987, ptr %202, align 4
  %203 = load i32, ptr null, align 4
  %204 = call i32 @fprintf(i32 %203, i32 1191, i32 %stack_end)
  call void @exit(i32 1) #3
  unreachable

brif_next9:                                       ; preds = %blk_exit7
  %calcOffset = add i32 %stack_addr, -1548
  %205 = inttoptr i32 %calcOffset to ptr
  store i32 %_arg_1, ptr %205, align 4
  %calcOffset10 = add i32 %stack_addr, -1552
  %206 = inttoptr i32 %calcOffset10 to ptr
  store i32 %_arg_2, ptr %206, align 4
  %207 = add i32 %198, 12
  %208 = call i32 @snprintf(i32 %200, i32 %207, i32 1174, i32 %calcOffset10)
  br label %blk_exit6
}

; Function Attrs: null_pointer_is_valid
define internal range(i32 0, -2147483648) i32 @maxlen_in_list(i32 %_arg_0) #0 {
allocator:
  %brif_val.not = icmp eq i32 %_arg_0, 0
  br i1 %brif_val.not, label %return, label %loop_entry

return:                                           ; preds = %blk_exit1, %allocator
  %return_0 = phi i32 [ 0, %allocator ], [ %6, %blk_exit1 ]
  ret i32 %return_0

loop_entry:                                       ; preds = %allocator, %blk_exit1
  %_local_1.0 = phi i32 [ %6, %blk_exit1 ], [ 0, %allocator ]
  %_param_0.0 = phi i32 [ %8, %blk_exit1 ], [ %_arg_0, %allocator ]
  %calcOffset = add i32 %_param_0.0, 68
  %0 = inttoptr i32 %calcOffset to ptr
  %1 = load i32, ptr %0, align 4
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %blk_exit2, label %brif_next4

blk_exit2:                                        ; preds = %loop_entry
  call void @get_tbl(i32 %_param_0.0)
  %3 = add i32 %_param_0.0, 48
  %4 = inttoptr i32 %3 to ptr
  %5 = load i32, ptr %4, align 4
  br label %blk_exit1

blk_exit1:                                        ; preds = %blk_exit2, %brif_next4
  %storemerge = phi i32 [ %9, %brif_next4 ], [ %5, %blk_exit2 ]
  %6 = call i32 @llvm.smax.i32(i32 %storemerge, i32 %_local_1.0)
  %calcOffset6 = add i32 %_param_0.0, 76
  %7 = inttoptr i32 %calcOffset6 to ptr
  %8 = load i32, ptr %7, align 4
  %brif_val7.not = icmp eq i32 %8, 0
  br i1 %brif_val7.not, label %return, label %loop_entry

brif_next4:                                       ; preds = %loop_entry
  %9 = call i32 @maxlen_in_list(i32 %1)
  br label %blk_exit1
}

; Function Attrs: null_pointer_is_valid
define internal void @matches_in_list(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %stack = alloca [32 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint ptr %stack to i32
  %stack_end = add i32 %stack_addr, -32
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit1, label %brif_next

blk_exit1:                                        ; preds = %blk_exit2, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  %1 = load i32, ptr null, align 4
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit2, %brif_next
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %77, %blk_exit2 ]
  %calcOffset = add i32 %_param_0.0, 68
  %2 = inttoptr i32 %calcOffset to ptr
  %3 = load i32, ptr %2, align 4
  %4 = icmp eq i32 %3, 0
  br i1 %4, label %blk_exit3, label %brif_next5

blk_exit3:                                        ; preds = %loop_entry
  %calcOffset7 = add i32 %_param_0.0, 16
  %5 = inttoptr i32 %calcOffset7 to ptr
  %6 = load i32, ptr %5, align 4
  %brif_val8.not = icmp eq i32 %6, 0
  br i1 %brif_val8.not, label %brif_next9, label %blk_exit6

brif_next9:                                       ; preds = %blk_exit3
  %calcOffset10 = add i32 %_param_0.0, 4
  %7 = inttoptr i32 %calcOffset10 to ptr
  %8 = load i32, ptr %7, align 4
  %9 = call i32 @fdopen(i32 %8, i32 1194)
  store i32 %9, ptr %5, align 4
  %10 = icmp eq i32 %9, 0
  br i1 %10, label %blk_exit, label %blk_exit6

blk_exit:                                         ; preds = %brif_next9
  %calcOffset92 = add i32 %_param_0.0, 24
  %11 = inttoptr i32 %calcOffset92 to ptr
  %12 = load i32, ptr %11, align 4
  call void @perror(i32 %12)
  call void @exit(i32 1) #3
  unreachable

blk_exit6:                                        ; preds = %brif_next9, %blk_exit3
  %_local_6.0 = phi i32 [ %9, %brif_next9 ], [ %6, %blk_exit3 ]
  %13 = load i32, ptr inttoptr (i32 2288 to ptr), align 4
  %14 = load i32, ptr inttoptr (i32 2284 to ptr), align 4
  %15 = call i32 @fgets(i32 %13, i32 %14, i32 %_local_6.0)
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %blk_exit2, label %loop_entry16

loop_entry16:                                     ; preds = %blk_exit6, %blk_exit17
  %_local_7.0 = phi i32 [ %_local_7.1, %blk_exit17 ], [ 0, %blk_exit6 ]
  %_local_5.0 = phi i32 [ %storemerge94, %blk_exit17 ], [ %13, %blk_exit6 ]
  %17 = inttoptr i32 %_local_5.0 to ptr
  %18 = load i8, ptr %17, align 1
  %calcOffset21 = add i32 %_param_0.0, 60
  %19 = inttoptr i32 %calcOffset21 to ptr
  %20 = load i8, ptr %19, align 1
  %.not = icmp eq i8 %18, %20
  br i1 %.not, label %brif_next23, label %blk_exit19

brif_next23:                                      ; preds = %loop_entry16
  %calcOffset24 = add i32 %_local_5.0, 1
  %21 = inttoptr i32 %calcOffset24 to ptr
  %22 = load i8, ptr %21, align 1
  %23 = icmp eq i8 %22, 10
  br i1 %23, label %blk_exit18, label %blk_exit19

blk_exit18:                                       ; preds = %brif_next23
  store i8 0, ptr %17, align 1
  %24 = load i32, ptr inttoptr (i32 2288 to ptr), align 4
  %calcOffset29 = add i32 %_param_0.0, 38
  %25 = inttoptr i32 %calcOffset29 to ptr
  %26 = load i8, ptr %25, align 1
  %27 = icmp eq i8 %26, 0
  br i1 %27, label %blk_exit28, label %brif_next31

blk_exit28:                                       ; preds = %brif_next33, %brif_next31, %blk_exit18
  %_local_8.0 = phi i32 [ %24, %blk_exit18 ], [ %70, %brif_next33 ], [ %24, %brif_next31 ]
  %28 = sub i32 %_local_5.0, %24
  %calcOffset35 = add i32 %_param_0.0, 56
  %29 = inttoptr i32 %calcOffset35 to ptr
  %30 = load i8, ptr %29, align 1
  %31 = and i8 %30, 4
  %32 = icmp eq i8 %31, 0
  br i1 %32, label %blk_exit34, label %brif_next37

blk_exit34:                                       ; preds = %blk_exit42, %brif_next37, %blk_exit28
  %33 = load i32, ptr inttoptr (i32 2208 to ptr), align 4
  %34 = icmp slt i32 %28, %33
  br i1 %34, label %blk_exit58, label %brif_next60

blk_exit58:                                       ; preds = %brif_next60, %blk_exit34
  %35 = icmp sgt i32 %28, %33
  br i1 %35, label %blk_exit63, label %brif_next65

blk_exit63:                                       ; preds = %brif_next65, %blk_exit58
  %36 = call i32 @regexec(i32 2232, i32 %_local_8.0, i32 0, i32 0, i32 0)
  %brif_val68.not = icmp eq i32 %36, 0
  br i1 %brif_val68.not, label %brif_next69, label %blk_exit57

brif_next69:                                      ; preds = %blk_exit63
  %brif_val71.not = icmp eq i32 %_local_7.0, 0
  br i1 %brif_val71.not, label %brif_next72, label %blk_exit70

brif_next72:                                      ; preds = %brif_next69
  %calcOffset73 = add i32 %_param_0.0, 20
  %37 = inttoptr i32 %calcOffset73 to ptr
  %38 = load i32, ptr %37, align 4
  %39 = load i8, ptr %19, align 1
  %40 = zext i8 %39 to i32
  %calcOffset75 = add i32 %stack_addr, -12
  %41 = inttoptr i32 %calcOffset75 to ptr
  store i32 %40, ptr %41, align 4
  %calcOffset76 = add i32 %stack_addr, -16
  %42 = inttoptr i32 %calcOffset76 to ptr
  store i32 %38, ptr %42, align 4
  %43 = call i32 @fprintf(i32 %1, i32 1864, i32 %calcOffset76)
  %44 = inttoptr i32 %_arg_1 to ptr
  store i8 1, ptr %44, align 1
  br label %blk_exit70

blk_exit70:                                       ; preds = %brif_next72, %brif_next69
  %45 = call i32 @fputs(i32 %_local_8.0, i32 %1)
  %46 = load i8, ptr %19, align 1
  %47 = zext i8 %46 to i32
  %48 = inttoptr i32 %stack_end to ptr
  store i32 %47, ptr %48, align 4
  %49 = call i32 @printf(i32 1869, i32 %stack_end)
  br label %blk_exit57

brif_next65:                                      ; preds = %blk_exit58
  %50 = load i8, ptr inttoptr (i32 2274 to ptr), align 1
  %brif_val66.not = icmp eq i8 %50, 0
  br i1 %brif_val66.not, label %blk_exit63, label %blk_exit57

blk_exit57:                                       ; preds = %blk_exit70, %blk_exit63, %brif_next65, %brif_next60
  %_local_7.2 = phi i32 [ 1, %blk_exit70 ], [ %_local_7.0, %blk_exit63 ], [ %_local_7.0, %brif_next65 ], [ %_local_7.0, %brif_next60 ]
  %51 = load i8, ptr %25, align 1
  %52 = icmp eq i8 %51, 0
  br i1 %52, label %blk_exit80, label %brif_next83

blk_exit80:                                       ; preds = %brif_next85, %brif_next83, %blk_exit57
  %53 = load i32, ptr inttoptr (i32 2288 to ptr), align 4
  br label %blk_exit17

brif_next83:                                      ; preds = %blk_exit57
  %54 = load i8, ptr inttoptr (i32 2294 to ptr), align 1
  %brif_val84.not = icmp eq i8 %54, 0
  br i1 %brif_val84.not, label %brif_next85, label %blk_exit80

brif_next85:                                      ; preds = %brif_next83
  call void @free(i32 %_local_8.0)
  br label %blk_exit80

brif_next60:                                      ; preds = %blk_exit34
  %55 = load i8, ptr inttoptr (i32 2273 to ptr), align 1
  %brif_val61.not = icmp eq i8 %55, 0
  br i1 %brif_val61.not, label %blk_exit58, label %blk_exit57

brif_next37:                                      ; preds = %blk_exit28
  %56 = inttoptr i32 %_local_8.0 to ptr
  %57 = load i8, ptr %56, align 1
  %58 = icmp eq i8 %57, 0
  br i1 %58, label %blk_exit34, label %loop_entry41

loop_entry41:                                     ; preds = %brif_next37, %blk_exit42
  %_local_6.1 = phi i32 [ %63, %blk_exit42 ], [ %_local_8.0, %brif_next37 ]
  %_local_5.1.in = phi i8 [ %65, %blk_exit42 ], [ %57, %brif_next37 ]
  %59 = add i8 %_local_5.1.in, -91
  %60 = icmp ult i8 %59, -26
  br i1 %60, label %blk_exit44, label %blk_exit43

blk_exit44:                                       ; preds = %loop_entry41
  %61 = add i8 %_local_5.1.in, -123
  %62 = icmp ult i8 %61, -26
  br i1 %62, label %blk_exit42, label %blk_exit43

blk_exit42:                                       ; preds = %blk_exit43, %blk_exit44
  %63 = add i32 %_local_6.1, 1
  %64 = inttoptr i32 %63 to ptr
  %65 = load i8, ptr %64, align 1
  %brif_val55.not = icmp eq i8 %65, 0
  br i1 %brif_val55.not, label %blk_exit34, label %loop_entry41

blk_exit43:                                       ; preds = %blk_exit44, %loop_entry41
  %storemerge93 = phi i8 [ 65, %loop_entry41 ], [ 97, %blk_exit44 ]
  %storemerge = phi i8 [ -52, %loop_entry41 ], [ -84, %blk_exit44 ]
  %.narrow = add i8 %storemerge, %_local_5.1.in
  %66 = urem i8 %.narrow, 26
  %67 = add nuw nsw i8 %storemerge93, %66
  %68 = inttoptr i32 %_local_6.1 to ptr
  store i8 %67, ptr %68, align 1
  br label %blk_exit42

brif_next31:                                      ; preds = %blk_exit18
  %69 = load i8, ptr inttoptr (i32 2294 to ptr), align 1
  %brif_val32.not = icmp eq i8 %69, 0
  br i1 %brif_val32.not, label %brif_next33, label %blk_exit28

brif_next33:                                      ; preds = %brif_next31
  %70 = call i32 @strdup(i32 %24)
  br label %blk_exit28

blk_exit17:                                       ; preds = %blk_exit80, %blk_exit19
  %_local_7.1 = phi i32 [ %_local_7.2, %blk_exit80 ], [ %_local_7.0, %blk_exit19 ]
  %storemerge94 = phi i32 [ %53, %blk_exit80 ], [ %75, %blk_exit19 ]
  %71 = load i32, ptr inttoptr (i32 2284 to ptr), align 4
  %72 = load i32, ptr %5, align 4
  %73 = call i32 @fgets(i32 %storemerge94, i32 %71, i32 %72)
  %brif_val87.not = icmp eq i32 %73, 0
  br i1 %brif_val87.not, label %blk_exit2, label %loop_entry16

blk_exit19:                                       ; preds = %brif_next23, %loop_entry16
  %74 = call i32 @strlen(i32 %_local_5.0)
  %75 = add i32 %_local_5.0, %74
  br label %blk_exit17

blk_exit2:                                        ; preds = %blk_exit17, %blk_exit6, %brif_next5
  %calcOffset89 = add i32 %_param_0.0, 76
  %76 = inttoptr i32 %calcOffset89 to ptr
  %77 = load i32, ptr %76, align 4
  %brif_val90.not = icmp eq i32 %77, 0
  br i1 %brif_val90.not, label %blk_exit1, label %loop_entry

brif_next5:                                       ; preds = %loop_entry
  call void @matches_in_list(i32 %3, i32 %_arg_1)
  br label %blk_exit2
}

; Function Attrs: null_pointer_is_valid
define internal void @get_tbl(i32 %_arg_0) #0 {
allocator:
  %stack = alloca [96 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint ptr %stack to i32
  %stack_end = add i32 %stack_addr, -96
  %calcOffset = add i32 %_arg_0, 36
  %0 = inttoptr i32 %calcOffset to ptr
  %1 = load i8, ptr %0, align 1
  %brif_val.not = icmp eq i8 %1, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

brif_next:                                        ; preds = %allocator
  %calcOffset10 = add i32 %_arg_0, 68
  %2 = inttoptr i32 %calcOffset10 to ptr
  %3 = load i32, ptr %2, align 4
  %brif_val11.not = icmp eq i32 %3, 0
  br i1 %brif_val11.not, label %brif_next12, label %blk_exit9

brif_next12:                                      ; preds = %brif_next
  %calcOffset13 = add i32 %_arg_0, 28
  %4 = inttoptr i32 %calcOffset13 to ptr
  %5 = load i32, ptr %4, align 4
  %6 = call i32 @open(i32 %5, i32 67108864, i32 0)
  %brif_val14.not = icmp sgt i32 %6, -1
  br i1 %brif_val14.not, label %brif_next15, label %blk_exit6

brif_next15:                                      ; preds = %brif_next12
  %7 = add i32 %_arg_0, 40
  %8 = call i32 @read(i32 %6, i32 %7, i32 4)
  %.not = icmp eq i32 %8, 4
  br i1 %.not, label %brif_next17, label %blk_exit5

brif_next17:                                      ; preds = %brif_next15
  %9 = add i32 %_arg_0, 44
  %10 = call i32 @read(i32 %6, i32 %9, i32 4)
  %.not72 = icmp eq i32 %10, 4
  br i1 %.not72, label %brif_next19, label %blk_exit4

brif_next19:                                      ; preds = %brif_next17
  %11 = add i32 %_arg_0, 48
  %12 = call i32 @read(i32 %6, i32 %11, i32 4)
  %.not73 = icmp eq i32 %12, 4
  br i1 %.not73, label %brif_next21, label %blk_exit3

brif_next21:                                      ; preds = %brif_next19
  %13 = add i32 %_arg_0, 52
  %14 = call i32 @read(i32 %6, i32 %13, i32 4)
  %.not74 = icmp eq i32 %14, 4
  br i1 %.not74, label %brif_next23, label %blk_exit2

brif_next23:                                      ; preds = %brif_next21
  %15 = add i32 %_arg_0, 56
  %16 = call i32 @read(i32 %6, i32 %15, i32 4)
  %.not75 = icmp eq i32 %16, 4
  br i1 %.not75, label %brif_next25, label %blk_exit1

brif_next25:                                      ; preds = %brif_next23
  %17 = add i32 %_arg_0, 60
  %18 = call i32 @read(i32 %6, i32 %17, i32 4)
  %.not76 = icmp eq i32 %18, 4
  br i1 %.not76, label %brif_next27, label %blk_exit

brif_next27:                                      ; preds = %brif_next25
  %19 = inttoptr i32 %7 to ptr
  %20 = load i32, ptr %19, align 4
  %21 = call i32 @ntohl(i32 %20)
  store i32 %21, ptr %19, align 4
  %22 = inttoptr i32 %9 to ptr
  %23 = load i32, ptr %22, align 4
  %24 = call i32 @ntohl(i32 %23)
  store i32 %24, ptr %22, align 4
  %25 = inttoptr i32 %11 to ptr
  %26 = load i32, ptr %25, align 4
  %27 = call i32 @ntohl(i32 %26)
  store i32 %27, ptr %25, align 4
  %28 = inttoptr i32 %13 to ptr
  %29 = load i32, ptr %28, align 4
  %30 = call i32 @ntohl(i32 %29)
  store i32 %30, ptr %28, align 4
  %31 = inttoptr i32 %15 to ptr
  %32 = load i32, ptr %31, align 4
  %33 = call i32 @ntohl(i32 %32)
  store i32 %33, ptr %31, align 4
  %34 = call i32 @close(i32 %6)
  br label %blk_exit8

blk_exit:                                         ; preds = %brif_next25
  %calcOffset70 = add i32 %_arg_0, 24
  %35 = inttoptr i32 %calcOffset70 to ptr
  %36 = load i32, ptr %35, align 4
  %37 = inttoptr i32 %stack_end to ptr
  store i32 %36, ptr %37, align 4
  %38 = load i32, ptr null, align 4
  %39 = call i32 @fprintf(i32 %38, i32 1841, i32 %stack_end)
  call void @exit(i32 1) #3
  unreachable

blk_exit1:                                        ; preds = %brif_next23
  %calcOffset68 = add i32 %_arg_0, 24
  %40 = inttoptr i32 %calcOffset68 to ptr
  %41 = load i32, ptr %40, align 4
  %calcOffset69 = add i32 %stack_addr, -80
  %42 = inttoptr i32 %calcOffset69 to ptr
  store i32 %41, ptr %42, align 4
  %43 = load i32, ptr null, align 4
  %44 = call i32 @fprintf(i32 %43, i32 1841, i32 %calcOffset69)
  call void @exit(i32 1) #3
  unreachable

blk_exit2:                                        ; preds = %brif_next21
  %calcOffset66 = add i32 %_arg_0, 24
  %45 = inttoptr i32 %calcOffset66 to ptr
  %46 = load i32, ptr %45, align 4
  %calcOffset67 = add i32 %stack_addr, -64
  %47 = inttoptr i32 %calcOffset67 to ptr
  store i32 %46, ptr %47, align 4
  %48 = load i32, ptr null, align 4
  %49 = call i32 @fprintf(i32 %48, i32 1841, i32 %calcOffset67)
  call void @exit(i32 1) #3
  unreachable

blk_exit3:                                        ; preds = %brif_next19
  %calcOffset64 = add i32 %_arg_0, 24
  %50 = inttoptr i32 %calcOffset64 to ptr
  %51 = load i32, ptr %50, align 4
  %calcOffset65 = add i32 %stack_addr, -48
  %52 = inttoptr i32 %calcOffset65 to ptr
  store i32 %51, ptr %52, align 4
  %53 = load i32, ptr null, align 4
  %54 = call i32 @fprintf(i32 %53, i32 1841, i32 %calcOffset65)
  call void @exit(i32 1) #3
  unreachable

blk_exit4:                                        ; preds = %brif_next17
  %calcOffset62 = add i32 %_arg_0, 24
  %55 = inttoptr i32 %calcOffset62 to ptr
  %56 = load i32, ptr %55, align 4
  %calcOffset63 = add i32 %stack_addr, -32
  %57 = inttoptr i32 %calcOffset63 to ptr
  store i32 %56, ptr %57, align 4
  %58 = load i32, ptr null, align 4
  %59 = call i32 @fprintf(i32 %58, i32 1841, i32 %calcOffset63)
  call void @exit(i32 1) #3
  unreachable

blk_exit5:                                        ; preds = %brif_next15
  %calcOffset60 = add i32 %_arg_0, 24
  %60 = inttoptr i32 %calcOffset60 to ptr
  %61 = load i32, ptr %60, align 4
  %calcOffset61 = add i32 %stack_addr, -16
  %62 = inttoptr i32 %calcOffset61 to ptr
  store i32 %61, ptr %62, align 4
  %63 = load i32, ptr null, align 4
  %64 = call i32 @fprintf(i32 %63, i32 1841, i32 %calcOffset61)
  call void @exit(i32 1) #3
  unreachable

blk_exit6:                                        ; preds = %brif_next12
  %65 = load i32, ptr %4, align 4
  call void @perror(i32 %65)
  call void @exit(i32 1) #3
  unreachable

blk_exit8:                                        ; preds = %blk_exit49, %brif_next27
  store i8 1, ptr %0, align 1
  br label %blk_exit7

blk_exit9:                                        ; preds = %brif_next
  %66 = add i32 %_arg_0, 52
  %67 = inttoptr i32 %66 to ptr
  store i32 -1, ptr %67, align 4
  %68 = add i32 %_arg_0, 44
  %69 = inttoptr i32 %68 to ptr
  %70 = add i32 %_arg_0, 44
  %71 = inttoptr i32 %70 to ptr
  call void @llvm.memset.p0.i64(ptr %71, i8 0, i64 8, i1 true)
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit49, %blk_exit9
  %_local_2.0 = phi i32 [ %3, %blk_exit9 ], [ %87, %blk_exit49 ]
  call void @get_tbl(i32 %_local_2.0)
  %72 = load i32, ptr %69, align 4
  %73 = add i32 %_local_2.0, 44
  %74 = inttoptr i32 %73 to ptr
  %75 = load i32, ptr %74, align 4
  %76 = add i32 %72, %75
  store i32 %76, ptr %69, align 4
  %calcOffset44 = add i32 %_arg_0, 48
  %77 = inttoptr i32 %calcOffset44 to ptr
  %78 = load i32, ptr %77, align 4
  %79 = add i32 %_local_2.0, 48
  %80 = inttoptr i32 %79 to ptr
  %81 = load i32, ptr %80, align 4
  %.not77 = icmp ult i32 %78, %81
  br i1 %.not77, label %brif_next47, label %blk_exit43

brif_next47:                                      ; preds = %loop_entry
  store i32 %81, ptr %77, align 4
  br label %blk_exit43

blk_exit43:                                       ; preds = %brif_next47, %loop_entry
  %82 = load i32, ptr %67, align 4
  %83 = add i32 %_local_2.0, 52
  %84 = inttoptr i32 %83 to ptr
  %85 = load i32, ptr %84, align 4
  %.not78 = icmp ugt i32 %82, %85
  br i1 %.not78, label %brif_next53, label %blk_exit49

brif_next53:                                      ; preds = %blk_exit43
  store i32 %85, ptr %67, align 4
  br label %blk_exit49

blk_exit49:                                       ; preds = %brif_next53, %blk_exit43
  %calcOffset55 = add i32 %_local_2.0, 76
  %86 = inttoptr i32 %calcOffset55 to ptr
  %87 = load i32, ptr %86, align 4
  %brif_val56.not = icmp eq i32 %87, 0
  br i1 %brif_val56.not, label %blk_exit8, label %loop_entry

blk_exit7:                                        ; preds = %blk_exit8, %allocator
  ret void
}

; Function Attrs: null_pointer_is_valid
define internal void @print_list(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %stack = alloca [96 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint ptr %stack to i32
  %stack_end = add i32 %stack_addr, -96
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %blk_exit40, %blk_exit17, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  %1 = add i32 %_arg_1, 1
  %2 = shl i32 %_arg_1, 2
  %3 = load i32, ptr null, align 4
  %4 = icmp eq i32 %_arg_1, 0
  br i1 %4, label %loop_entry24, label %loop_entry

loop_entry24:                                     ; preds = %brif_next, %blk_exit40
  %_param_0.1 = phi i32 [ %_arg_0, %brif_next ], [ %38, %blk_exit40 ]
  %calcOffset25 = add i32 %stack_addr, -60
  %5 = inttoptr i32 %calcOffset25 to ptr
  store i32 2206, ptr %5, align 4
  %calcOffset26 = add i32 %stack_addr, -64
  %6 = inttoptr i32 %calcOffset26 to ptr
  store i32 %2, ptr %6, align 4
  %7 = call i32 @fprintf(i32 %3, i32 1167, i32 %calcOffset26)
  %8 = inttoptr i32 %_param_0.1 to ptr
  %9 = load i32, ptr %8, align 4
  %10 = icmp eq i32 %9, -1
  br i1 %10, label %blk_exit28, label %brif_next31

blk_exit28:                                       ; preds = %loop_entry24
  %11 = load i8, ptr inttoptr (i32 2272 to ptr), align 1
  %brif_val33.not = icmp eq i8 %11, 0
  br i1 %brif_val33.not, label %brif_next34, label %blk_exit32

brif_next34:                                      ; preds = %blk_exit28
  %12 = load i32, ptr inttoptr (i32 3332 to ptr), align 4
  %13 = sitofp i32 %12 to double
  %14 = fsub double 1.000000e+02, %13
  %15 = add i32 %_param_0.1, 44
  %16 = inttoptr i32 %15 to ptr
  %17 = load i32, ptr %16, align 4
  %18 = uitofp i32 %17 to double
  %19 = fmul double %14, %18
  %20 = load i32, ptr inttoptr (i32 3340 to ptr), align 4
  %21 = uitofp i32 %20 to double
  %22 = fdiv double %19, %21
  br label %blk_exit27

blk_exit32:                                       ; preds = %blk_exit28
  %23 = load i32, ptr inttoptr (i32 3356 to ptr), align 4
  %24 = sitofp i32 %23 to double
  %25 = fdiv double 1.000000e+02, %24
  br label %blk_exit27

blk_exit27:                                       ; preds = %blk_exit32, %brif_next34, %brif_next31
  %_local_6.1 = phi double [ %22, %brif_next34 ], [ %25, %blk_exit32 ], [ %39, %brif_next31 ]
  %calcOffset36 = add i32 %stack_addr, -80
  %26 = inttoptr i32 %calcOffset36 to ptr
  store double %_local_6.1, ptr %26, align 8
  %27 = call i32 @fprintf(i32 %3, i32 1561, i32 %calcOffset36)
  %calcOffset37 = add i32 %_param_0.1, 20
  %28 = inttoptr i32 %calcOffset37 to ptr
  %29 = load i32, ptr %28, align 4
  %select_cond38.not = icmp eq i32 %29, 0
  %30 = select i1 %select_cond38.not, i32 1492, i32 %29
  %31 = inttoptr i32 %stack_end to ptr
  store i32 %30, ptr %31, align 4
  %32 = call i32 @fprintf(i32 %3, i32 1190, i32 %stack_end)
  %33 = call i32 @putc(i32 10, i32 %3)
  %calcOffset41 = add i32 %_param_0.1, 68
  %34 = inttoptr i32 %calcOffset41 to ptr
  %35 = load i32, ptr %34, align 4
  %36 = icmp eq i32 %35, 0
  br i1 %36, label %blk_exit40, label %brif_next43

blk_exit40:                                       ; preds = %brif_next43, %blk_exit27
  %calcOffset44 = add i32 %_param_0.1, 76
  %37 = inttoptr i32 %calcOffset44 to ptr
  %38 = load i32, ptr %37, align 4
  %brif_val45.not = icmp eq i32 %38, 0
  br i1 %brif_val45.not, label %blk_exit, label %loop_entry24

brif_next43:                                      ; preds = %blk_exit27
  call void @print_list(i32 %35, i32 %1)
  br label %blk_exit40

brif_next31:                                      ; preds = %loop_entry24
  %39 = sitofp i32 %9 to double
  br label %blk_exit27

loop_entry:                                       ; preds = %brif_next, %blk_exit17
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %72, %blk_exit17 ]
  %calcOffset = add i32 %stack_addr, -12
  %40 = inttoptr i32 %calcOffset to ptr
  store i32 2206, ptr %40, align 4
  %calcOffset4 = add i32 %stack_addr, -16
  %41 = inttoptr i32 %calcOffset4 to ptr
  store i32 %2, ptr %41, align 4
  %42 = call i32 @fprintf(i32 %3, i32 1167, i32 %calcOffset4)
  %43 = inttoptr i32 %_param_0.0 to ptr
  %44 = load i32, ptr %43, align 4
  %.not = icmp eq i32 %44, -1
  br i1 %.not, label %brif_next9, label %blk_exit6

brif_next9:                                       ; preds = %loop_entry
  %45 = load i8, ptr inttoptr (i32 2272 to ptr), align 1
  %brif_val11.not = icmp eq i8 %45, 0
  br i1 %brif_val11.not, label %brif_next12, label %blk_exit10

brif_next12:                                      ; preds = %brif_next9
  %46 = load i32, ptr inttoptr (i32 3332 to ptr), align 4
  %47 = sitofp i32 %46 to double
  %48 = fsub double 1.000000e+02, %47
  %49 = add i32 %_param_0.0, 44
  %50 = inttoptr i32 %49 to ptr
  %51 = load i32, ptr %50, align 4
  %52 = uitofp i32 %51 to double
  %53 = fmul double %48, %52
  %54 = load i32, ptr inttoptr (i32 3340 to ptr), align 4
  %55 = uitofp i32 %54 to double
  %56 = fdiv double %53, %55
  br label %blk_exit5

blk_exit10:                                       ; preds = %brif_next9
  %57 = load i32, ptr inttoptr (i32 3352 to ptr), align 4
  %58 = sitofp i32 %57 to double
  %59 = fdiv double 1.000000e+02, %58
  br label %blk_exit5

blk_exit5:                                        ; preds = %blk_exit6, %blk_exit10, %brif_next12
  %_local_6.0 = phi double [ %56, %brif_next12 ], [ %59, %blk_exit10 ], [ %73, %blk_exit6 ]
  %calcOffset14 = add i32 %stack_addr, -32
  %60 = inttoptr i32 %calcOffset14 to ptr
  store double %_local_6.0, ptr %60, align 8
  %61 = call i32 @fprintf(i32 %3, i32 1561, i32 %calcOffset14)
  %calcOffset15 = add i32 %_param_0.0, 20
  %62 = inttoptr i32 %calcOffset15 to ptr
  %63 = load i32, ptr %62, align 4
  %select_cond.not = icmp eq i32 %63, 0
  %64 = select i1 %select_cond.not, i32 1492, i32 %63
  %calcOffset16 = add i32 %stack_addr, -48
  %65 = inttoptr i32 %calcOffset16 to ptr
  store i32 %64, ptr %65, align 4
  %66 = call i32 @fprintf(i32 %3, i32 1190, i32 %calcOffset16)
  %67 = call i32 @putc(i32 10, i32 %3)
  %calcOffset18 = add i32 %_param_0.0, 68
  %68 = inttoptr i32 %calcOffset18 to ptr
  %69 = load i32, ptr %68, align 4
  %70 = icmp eq i32 %69, 0
  br i1 %70, label %blk_exit17, label %brif_next20

blk_exit17:                                       ; preds = %brif_next20, %blk_exit5
  %calcOffset21 = add i32 %_param_0.0, 76
  %71 = inttoptr i32 %calcOffset21 to ptr
  %72 = load i32, ptr %71, align 4
  %brif_val22.not = icmp eq i32 %72, 0
  br i1 %brif_val22.not, label %blk_exit, label %loop_entry

brif_next20:                                      ; preds = %blk_exit5
  call void @print_list(i32 %69, i32 %1)
  br label %blk_exit17

blk_exit6:                                        ; preds = %loop_entry
  %73 = sitofp i32 %44 to double
  br label %blk_exit5
}

; Function Attrs: null_pointer_is_valid
define internal void @free_desc(i32 %_arg_0) #0 {
allocator:
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit, label %loop_entry

blk_exit:                                         ; preds = %blk_exit5, %allocator
  ret void

loop_entry:                                       ; preds = %allocator, %blk_exit5
  %_param_0.0 = phi i32 [ %_arg_0, %allocator ], [ %15, %blk_exit5 ]
  %calcOffset = add i32 %_param_0.0, 68
  %1 = inttoptr i32 %calcOffset to ptr
  %2 = load i32, ptr %1, align 4
  call void @free_desc(i32 %2)
  %calcOffset1 = add i32 %_param_0.0, 28
  %3 = inttoptr i32 %calcOffset1 to ptr
  %4 = load i32, ptr %3, align 4
  call void @free(i32 %4)
  %calcOffset2 = add i32 %_param_0.0, 32
  %5 = inttoptr i32 %calcOffset2 to ptr
  %6 = load i32, ptr %5, align 4
  call void @free(i32 %6)
  %calcOffset3 = add i32 %_param_0.0, 20
  %7 = inttoptr i32 %calcOffset3 to ptr
  %8 = load i32, ptr %7, align 4
  call void @free(i32 %8)
  %calcOffset4 = add i32 %_param_0.0, 24
  %9 = inttoptr i32 %calcOffset4 to ptr
  %10 = load i32, ptr %9, align 4
  call void @free(i32 %10)
  %calcOffset6 = add i32 %_param_0.0, 16
  %11 = inttoptr i32 %calcOffset6 to ptr
  %12 = load i32, ptr %11, align 4
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %blk_exit5, label %brif_next8

blk_exit5:                                        ; preds = %brif_next8, %loop_entry
  %calcOffset10 = add i32 %_param_0.0, 76
  %14 = inttoptr i32 %calcOffset10 to ptr
  %15 = load i32, ptr %14, align 4
  call void @free(i32 %_param_0.0)
  %brif_val11.not = icmp eq i32 %15, 0
  br i1 %brif_val11.not, label %blk_exit, label %loop_entry

brif_next8:                                       ; preds = %loop_entry
  %16 = call i32 @fclose(i32 %12)
  store i32 0, ptr %11, align 4
  br label %blk_exit5
}

; Function Attrs: null_pointer_is_valid
define internal i32 @names_compare(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = inttoptr i32 %_arg_0 to ptr
  %1 = load i32, ptr %0, align 4
  %2 = inttoptr i32 %_arg_1 to ptr
  %3 = load i32, ptr %2, align 4
  %4 = call i32 @strcmp(i32 %1, i32 %3)
  ret i32 %4
}

; Function Attrs: null_pointer_is_valid
define i32 @__main_void() #0 {
allocator:
  %0 = call i32 @main(i32 0, i32 0)
  ret i32 %0
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smax.i32(i32, i32) #1

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr writeonly captures(none), i8, i64, i1 immarg) #2

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #3 = { cold }

!0 = !{!"true"}
!1 = !{!"negative"}
