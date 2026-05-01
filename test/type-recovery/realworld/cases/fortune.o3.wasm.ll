; ModuleID = 'test/type-recovery/realworld/cases/fortune.o3.wasm.ll'
source_filename = "/sn640/NotDec/test/lifting/wasm/cases/fortune.o3.wasm"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

@__stack_pointer = internal global i32 68912, !notdec.stackpointer !0
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
define dso_local i32 @main(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %stack = alloca [2960 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [2960 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -2960
  %0 = call i32 @getenv(i32 1497)
  store i32 %0, i32* inttoptr (i32 2224 to i32*), align 16
  %brif_val.not = icmp eq i32 %0, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

brif_next:                                        ; preds = %allocator
  %1 = call i32 @getenv(i32 1480)
  store i32 %1, i32* inttoptr (i32 2224 to i32*), align 16
  %brif_val1.not = icmp eq i32 %1, 0
  br i1 %brif_val1.not, label %brif_next2, label %blk_exit

brif_next2:                                       ; preds = %brif_next
  %2 = call i32 @getenv(i32 1509)
  store i32 %2, i32* inttoptr (i32 2224 to i32*), align 16
  %brif_val3.not = icmp eq i32 %2, 0
  br i1 %brif_val3.not, label %brif_next4, label %blk_exit

brif_next4:                                       ; preds = %brif_next2
  %3 = call i32 @getenv(i32 1504)
  store i32 %3, i32* inttoptr (i32 2224 to i32*), align 16
  br label %blk_exit

blk_exit:                                         ; preds = %brif_next4, %brif_next2, %brif_next, %allocator
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit18, %blk_exit7, %blk_exit17, %blk_exit16, %blk_exit15, %blk_exit10, %blk_exit14, %blk_exit13, %blk_exit12, %blk_exit9, %blk_exit11, %loop_entry, %blk_exit
  %_local_4.0 = phi i32 [ 0, %blk_exit ], [ %_local_4.0, %blk_exit18 ], [ %_local_4.0, %blk_exit7 ], [ %_local_4.0, %blk_exit17 ], [ %_local_4.0, %blk_exit16 ], [ %_local_4.0, %loop_entry ], [ %_local_4.0, %blk_exit15 ], [ %814, %blk_exit10 ], [ %_local_4.0, %blk_exit14 ], [ %_local_4.0, %blk_exit13 ], [ %_local_4.0, %blk_exit12 ], [ %_local_4.0, %blk_exit9 ], [ %_local_4.0, %blk_exit11 ]
  %_local_3.0 = phi i32 [ 0, %blk_exit ], [ %_local_3.0, %blk_exit18 ], [ %_local_3.0, %blk_exit7 ], [ %_local_3.0, %blk_exit17 ], [ %_local_3.0, %blk_exit16 ], [ 1, %loop_entry ], [ %_local_3.0, %blk_exit15 ], [ %_local_3.0, %blk_exit10 ], [ %_local_3.0, %blk_exit14 ], [ %_local_3.0, %blk_exit13 ], [ %_local_3.0, %blk_exit12 ], [ %_local_3.0, %blk_exit9 ], [ %_local_3.0, %blk_exit11 ]
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
  %5 = load i32, i32* null, align 2147483648
  %6 = sub i32 %_arg_0, %5
  %7 = icmp eq i32 %6, 0
  br i1 %7, label %blk_exit43, label %brif_next45

blk_exit43:                                       ; preds = %blk_exit5
  %8 = load i8, i8* inttoptr (i32 2292 to i8*), align 4
  %9 = icmp eq i8 %8, 0
  br i1 %9, label %blk_exit219, label %brif_next221

blk_exit219:                                      ; preds = %blk_exit43
  %10 = load i8, i8* inttoptr (i32 2293 to i8*), align 1
  %11 = icmp eq i8 %10, 0
  br i1 %11, label %blk_exit228, label %brif_next230

blk_exit228:                                      ; preds = %blk_exit219
  %12 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %blk_exit234, label %brif_next236

blk_exit234:                                      ; preds = %blk_exit228
  %14 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %15 = call i32 @strcmp(i32 1089, i32 1121)
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %blk_exit258, label %brif_next260

blk_exit258:                                      ; preds = %brif_next260, %blk_exit234
  %_local_3.8 = phi i32 [ 0, %blk_exit234 ], [ %18, %brif_next260 ]
  %17 = or i32 %_local_3.8, %14
  br label %blk_exit218

brif_next260:                                     ; preds = %blk_exit234
  %18 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit258

brif_next236:                                     ; preds = %blk_exit228
  %19 = add i32 %stack_addr, -2560
  %20 = call i32 @strncpy(i32 %19, i32 %12, i32 512)
  %calcOffset237 = add i32 %stack_addr, -2049
  %21 = inttoptr i32 %calcOffset237 to i8*
  store i8 0, i8* %21, align 1
  br label %loop_entry239

loop_entry239:                                    ; preds = %brif_next251, %brif_next236
  %_local_3.6 = phi i32 [ %19, %brif_next236 ], [ %storemerge801, %brif_next251 ]
  %22 = icmp eq i32 %_local_3.6, 0
  br i1 %22, label %blk_exit238, label %brif_next241

blk_exit238:                                      ; preds = %brif_next241, %loop_entry239
  %23 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %24 = call i32 @strcmp(i32 1089, i32 1121)
  %25 = icmp eq i32 %24, 0
  br i1 %25, label %blk_exit255, label %brif_next257

blk_exit255:                                      ; preds = %brif_next257, %blk_exit238
  %_local_3.7 = phi i32 [ 0, %blk_exit238 ], [ %27, %brif_next257 ]
  %26 = or i32 %_local_3.7, %23
  br label %blk_exit218

brif_next257:                                     ; preds = %blk_exit238
  %27 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit255

brif_next241:                                     ; preds = %loop_entry239
  %28 = inttoptr i32 %_local_3.6 to i8*
  %29 = load i8, i8* %28, align 1
  %30 = icmp eq i8 %29, 0
  br i1 %30, label %blk_exit238, label %brif_next244

brif_next244:                                     ; preds = %brif_next241
  %31 = call i32 @strchr(i32 %_local_3.6, i32 58)
  %brif_val247.not = icmp eq i32 %31, 0
  br i1 %brif_val247.not, label %blk_exit245, label %blk_exit246

blk_exit245:                                      ; preds = %blk_exit246, %brif_next244
  %storemerge801 = phi i32 [ %39, %blk_exit246 ], [ 0, %brif_next244 ]
  %32 = call i32 @add_file(i32 -1, i32 %_local_3.6, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val250.not = icmp eq i32 %32, 0
  br i1 %brif_val250.not, label %brif_next251, label %blk_exit218

brif_next251:                                     ; preds = %blk_exit245
  %33 = add i32 %stack_addr, -2564
  %34 = call i32 @strncpy(i32 %33, i32 %_local_3.6, i32 2)
  %calcOffset252 = add i32 %stack_addr, -2562
  %35 = inttoptr i32 %calcOffset252 to i8*
  store i8 0, i8* %35, align 1
  %36 = call i32 @add_file(i32 -1, i32 %33, i32 0, i32 2268, i32 3328, i32 0)
  %37 = icmp eq i32 %36, 0
  br i1 %37, label %loop_entry239, label %blk_exit218

blk_exit246:                                      ; preds = %brif_next244
  %38 = inttoptr i32 %31 to i8*
  store i8 0, i8* %38, align 1
  %39 = add i32 %31, 1
  br label %blk_exit245

brif_next230:                                     ; preds = %blk_exit219
  %40 = call i32 @add_file(i32 -1, i32 1282, i32 0, i32 2268, i32 3328, i32 0)
  %41 = call i32 @strcmp(i32 1246, i32 1282)
  %42 = icmp eq i32 %41, 0
  br i1 %42, label %blk_exit231, label %brif_next233

blk_exit231:                                      ; preds = %brif_next233, %brif_next230
  %_local_3.5 = phi i32 [ 0, %brif_next230 ], [ %44, %brif_next233 ]
  %43 = or i32 %_local_3.5, %40
  br label %blk_exit218

brif_next233:                                     ; preds = %brif_next230
  %44 = call i32 @add_file(i32 -1, i32 1246, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit231

blk_exit218:                                      ; preds = %blk_exit225, %blk_exit231, %brif_next251, %blk_exit245, %blk_exit255, %blk_exit258
  %_param_0.2 = phi i32 [ %17, %blk_exit258 ], [ %26, %blk_exit255 ], [ %36, %brif_next251 ], [ %32, %blk_exit245 ], [ %43, %blk_exit231 ], [ %54, %blk_exit225 ]
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

blk_exit41:                                       ; preds = %blk_exit203, %blk_exit111, %blk_exit49, %blk_exit218
  %57 = load i8, i8* inttoptr (i32 2296 to i8*), align 8
  %brif_val271.not = icmp eq i8 %57, 0
  br i1 %brif_val271.not, label %brif_next272, label %blk_exit270

brif_next272:                                     ; preds = %blk_exit41
  %58 = inttoptr i32 %stack_end to i32*
  store i32 1795, i32* %58, align 4
  %59 = load i32, i32* null, align 2147483648
  %60 = call i32 @fprintf(i32 %59, i32 1191, i32 %stack_end)
  br label %blk_exit270

blk_exit270:                                      ; preds = %brif_next272, %blk_exit41
  call void @exit(i32 1)
  unreachable

blk_exit42:                                       ; preds = %blk_exit110, %brif_next45, %blk_exit218
  %61 = icmp eq i32 %_local_4.0, 0
  br i1 %61, label %blk_exit35, label %brif_next264

blk_exit35:                                       ; preds = %blk_exit36, %blk_exit38, %blk_exit42
  %62 = call i32 @setlocale(i32 6, i32 2206)
  %63 = load i8, i8* inttoptr (i32 2228 to i8*), align 4
  %64 = icmp eq i8 %63, 0
  br i1 %64, label %blk_exit314, label %brif_next316

blk_exit314:                                      ; preds = %blk_exit35
  %65 = load i32, i32* inttoptr (i32 3328 to i32*), align 256
  %66 = icmp eq i32 %65, 0
  br i1 %66, label %blk_exit321, label %brif_next323

blk_exit321:                                      ; preds = %brif_next331, %blk_exit314
  %_local_5.3 = phi i32 [ 0, %blk_exit314 ], [ %552, %brif_next331 ]
  %_local_4.2 = phi i32 [ 0, %blk_exit314 ], [ %553, %brif_next331 ]
  %_param_1.4 = phi i32 [ 0, %blk_exit314 ], [ %555, %brif_next331 ]
  %67 = icmp ne i32 %_param_1.4, 100
  %68 = icmp eq i32 %_local_4.2, 0
  %or.cond808 = select i1 %67, i1 %68, i1 false
  br i1 %or.cond808, label %blk_exit31, label %blk_exit334

blk_exit31:                                       ; preds = %blk_exit321
  %calcOffset764 = add i32 %stack_addr, -2928
  %69 = inttoptr i32 %calcOffset764 to i32*
  store i32 %_param_1.4, i32* %69, align 4
  %70 = load i32, i32* null, align 2147483648
  %71 = call i32 @fprintf(i32 %70, i32 2012, i32 %calcOffset764)
  call void @exit(i32 1)
  unreachable

blk_exit334:                                      ; preds = %blk_exit321
  %or.cond809 = select i1 %67, i1 true, i1 %68
  br i1 %or.cond809, label %blk_exit339, label %blk_exit30

blk_exit339:                                      ; preds = %blk_exit334
  store i32 %_param_1.4, i32* inttoptr (i32 3332 to i32*), align 4
  %72 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %73 = icmp eq i8 %72, 0
  %or.cond = select i1 %73, i1 true, i1 %68
  br i1 %or.cond, label %blk_exit344, label %brif_next348

blk_exit344:                                      ; preds = %blk_exit349, %blk_exit339
  %74 = load i8, i8* inttoptr (i32 2264 to i8*), align 8
  %75 = icmp eq i8 %74, 0
  br i1 %75, label %blk_exit364, label %brif_next366

blk_exit364:                                      ; preds = %blk_exit344
  %76 = call i64 @time(i32 0)
  %77 = call i32 @getpid()
  %78 = trunc i64 %76 to i32
  %79 = add i32 %77, %78
  call void @srandom(i32 %79)
  br label %loop_entry404

loop_entry404:                                    ; preds = %blk_exit605, %blk_exit639, %blk_exit364
  %80 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %81 = inttoptr i32 %80 to i32*
  %82 = load i32, i32* %81, align 4
  %calcOffset409 = add i32 %80, 76
  %83 = inttoptr i32 %calcOffset409 to i32*
  %84 = load i32, i32* %83, align 4
  %85 = icmp eq i32 %84, 0
  br i1 %85, label %blk_exit408, label %brif_next411

blk_exit408:                                      ; preds = %brif_next438, %loop_entry434, %blk_exit427, %blk_exit415, %loop_entry404
  %_local_5.6 = phi i32 [ %82, %loop_entry404 ], [ -1, %loop_entry434 ], [ %497, %brif_next438 ], [ -1, %blk_exit415 ], [ %493, %blk_exit427 ]
  %_local_3.21 = phi i32 [ %80, %loop_entry404 ], [ %495, %loop_entry434 ], [ %495, %brif_next438 ], [ %491, %blk_exit415 ], [ %491, %blk_exit427 ]
  %.not779 = icmp eq i32 %_local_5.6, -1
  br i1 %.not779, label %brif_next442, label %blk_exit406

brif_next442:                                     ; preds = %blk_exit408
  %calcOffset443 = add i32 %_local_3.21, 76
  %86 = inttoptr i32 %calcOffset443 to i32*
  %87 = load i32, i32* %86, align 4
  %88 = icmp eq i32 %87, 0
  br i1 %88, label %blk_exit406, label %blk_exit407

blk_exit406:                                      ; preds = %loop_entry481, %blk_exit466, %brif_next442, %blk_exit408
  %_local_3.19 = phi i32 [ %_local_3.21, %brif_next442 ], [ %_local_3.20, %blk_exit466 ], [ %470, %loop_entry481 ], [ %_local_3.21, %blk_exit408 ]
  call void @get_tbl(i32 %_local_3.19)
  %89 = add i32 %_local_3.19, 44
  %90 = inttoptr i32 %89 to i32*
  %91 = load i32, i32* %90, align 4
  %92 = icmp eq i32 %91, 0
  br i1 %92, label %blk_exit29, label %brif_next488

blk_exit29:                                       ; preds = %blk_exit406
  %calcOffset765 = add i32 %stack_addr, -2912
  %93 = inttoptr i32 %calcOffset765 to i32*
  store i32 1814, i32* %93, align 4
  %94 = load i32, i32* null, align 2147483648
  %95 = call i32 @fprintf(i32 %94, i32 1191, i32 %calcOffset765)
  call void @exit(i32 1)
  unreachable

brif_next488:                                     ; preds = %blk_exit406
  %calcOffset490 = add i32 %_local_3.19, 68
  %96 = inttoptr i32 %calcOffset490 to i32*
  %97 = load i32, i32* %96, align 4
  %98 = icmp eq i32 %97, 0
  br i1 %98, label %blk_exit489, label %loop_entry493

blk_exit489:                                      ; preds = %blk_exit494, %brif_next488
  %_local_3.24 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  store i32 %_local_3.24, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset561 = add i32 %_local_3.24, 36
  %99 = inttoptr i32 %calcOffset561 to i8*
  %100 = load i8, i8* %99, align 1
  %101 = icmp eq i8 %100, 0
  br i1 %101, label %blk_exit28, label %brif_next563

blk_exit28:                                       ; preds = %blk_exit489
  call void @__assert_fail(i32 1229, i32 1342, i32 1621, i32 1081)
  unreachable

brif_next563:                                     ; preds = %blk_exit489
  %calcOffset565 = add i32 %_local_3.24, 12
  %102 = inttoptr i32 %calcOffset565 to i32*
  %103 = load i32, i32* %102, align 4
  %.not784 = icmp eq i32 %103, -1
  br i1 %.not784, label %brif_next567, label %brif_next563.blk_exit564_crit_edge

brif_next567:                                     ; preds = %brif_next563
  %104 = add i32 %_local_3.24, 44
  %105 = inttoptr i32 %104 to i32*
  %106 = load i32, i32* %105, align 4
  %calcOffset569 = add i32 %stack_addr, -1024
  %107 = inttoptr i32 %calcOffset569 to i64*
  %108 = inttoptr i32 %calcOffset569 to i8*
  call void @llvm.memset.p0i8.i64(i8* %108, i8 0, i64 8, i1 true)
  %109 = call i32 @getenv(i32 1447)
  %110 = icmp eq i32 %109, 0
  br i1 %110, label %blk_exit570, label %brif_next572

blk_exit570:                                      ; preds = %brif_next567
  %111 = call i32 @getenv(i32 1522)
  %brif_val574.not = icmp eq i32 %111, 0
  br i1 %brif_val574.not, label %brif_next575, label %blk_exit573

brif_next575:                                     ; preds = %blk_exit570
  %112 = call i32 @fopen(i32 1208, i32 1414)
  %113 = icmp eq i32 %112, 0
  br i1 %113, label %blk_exit573, label %brif_next577

brif_next577:                                     ; preds = %brif_next575
  %114 = call i32 @fread(i32 %calcOffset569, i32 8, i32 1, i32 %112)
  %115 = call i32 @fclose(i32 %112)
  %.not791 = icmp eq i32 %114, 1
  br i1 %.not791, label %brif_next579, label %blk_exit573

brif_next579:                                     ; preds = %brif_next577
  %116 = load i64, i64* %107, align 8
  %117 = zext i32 %106 to i64
  %118 = urem i64 %116, %117
  %119 = trunc i64 %118 to i32
  br label %blk_exit564

blk_exit573:                                      ; preds = %brif_next577, %brif_next575, %blk_exit570
  %120 = call i32 @random()
  %121 = urem i32 %120, %106
  br label %blk_exit564

brif_next572:                                     ; preds = %brif_next567
  %122 = call i32 @atol(i32 %109)
  %123 = urem i32 %122, %106
  br label %blk_exit564

blk_exit564:                                      ; preds = %brif_next563.blk_exit564_crit_edge, %brif_next572, %blk_exit573, %brif_next579
  %.pre-phi813 = phi i32* [ %.pre812, %brif_next563.blk_exit564_crit_edge ], [ %105, %blk_exit573 ], [ %105, %brif_next579 ], [ %105, %brif_next572 ]
  %_local_5.16 = phi i32 [ %103, %brif_next563.blk_exit564_crit_edge ], [ %121, %blk_exit573 ], [ %119, %brif_next579 ], [ %123, %brif_next572 ]
  %124 = add i32 %_local_5.16, 1
  store i32 %124, i32* %102, align 4
  %125 = load i32, i32* %.pre-phi813, align 4
  %126 = icmp slt i32 %124, %125
  br i1 %126, label %blk_exit582, label %brif_next585

blk_exit582:                                      ; preds = %brif_next585, %blk_exit564
  %_local_5.17 = phi i32 [ %124, %blk_exit564 ], [ %346, %brif_next585 ]
  %calcOffset588 = add i32 %_local_3.24, 8
  %127 = inttoptr i32 %calcOffset588 to i32*
  %128 = load i32, i32* %127, align 4
  %brif_val589.not = icmp slt i32 %128, 0
  br i1 %brif_val589.not, label %brif_next590, label %blk_exit587

brif_next590:                                     ; preds = %blk_exit582
  %calcOffset591 = add i32 %_local_3.24, 28
  %129 = inttoptr i32 %calcOffset591 to i32*
  %130 = load i32, i32* %129, align 4
  %131 = call i32 @open(i32 %130, i32 67108864, i32 0)
  store i32 %131, i32* %127, align 4
  %brif_val593.not = icmp sgt i32 %131, -1
  br i1 %brif_val593.not, label %brif_next594, label %blk_exit27

brif_next594:                                     ; preds = %brif_next590
  %132 = load i32, i32* %102, align 4
  br label %blk_exit587

blk_exit27:                                       ; preds = %brif_next590
  call void @exit(i32 1)
  unreachable

blk_exit587:                                      ; preds = %brif_next594, %blk_exit582
  %_local_5.18 = phi i32 [ %132, %brif_next594 ], [ %_local_5.17, %blk_exit582 ]
  %_param_0.11 = phi i32 [ %131, %brif_next594 ], [ %128, %blk_exit582 ]
  %133 = shl i32 %_local_5.18, 2
  %134 = add i32 %133, 24
  %135 = zext i32 %134 to i64
  %136 = call i64 @lseek(i32 %_param_0.11, i64 %135, i32 0)
  %137 = load i32, i32* %127, align 4
  %138 = call i32 @read(i32 %137, i32 3360, i32 4)
  %brif_val597.not = icmp sgt i32 %138, -1
  br i1 %brif_val597.not, label %brif_next598, label %blk_exit26

brif_next598:                                     ; preds = %blk_exit587
  %139 = load i32, i32* %127, align 4
  %140 = call i32 @read(i32 %139, i32 3364, i32 4)
  %brif_val600.not = icmp sgt i32 %140, -1
  br i1 %brif_val600.not, label %brif_next601, label %blk_exit26

brif_next601:                                     ; preds = %brif_next598
  %141 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %142 = call i32 @ntohl(i32 %141)
  store i32 %142, i32* inttoptr (i32 3360 to i32*), align 32
  %143 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %144 = call i32 @ntohl(i32 %143)
  store i32 %144, i32* inttoptr (i32 3364 to i32*), align 4
  %145 = load i8, i8* inttoptr (i32 2273 to i8*), align 1
  %146 = icmp eq i8 %145, 0
  br i1 %146, label %blk_exit602, label %brif_next604

blk_exit602:                                      ; preds = %blk_exit605, %brif_next601
  %147 = load i8, i8* inttoptr (i32 2274 to i8*), align 2
  %148 = icmp eq i8 %147, 0
  br i1 %148, label %blk_exit636, label %brif_next638

blk_exit636:                                      ; preds = %blk_exit639, %blk_exit602
  %149 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset671 = add i32 %149, 16
  %150 = inttoptr i32 %calcOffset671 to i32*
  %151 = load i32, i32* %150, align 4
  %brif_val672.not = icmp eq i32 %151, 0
  br i1 %brif_val672.not, label %brif_next673, label %blk_exit670

brif_next673:                                     ; preds = %blk_exit636
  %calcOffset674 = add i32 %149, 4
  %152 = inttoptr i32 %calcOffset674 to i32*
  %153 = load i32, i32* %152, align 4
  %154 = call i32 @fdopen(i32 %153, i32 1194)
  store i32 %154, i32* %150, align 4
  %155 = icmp eq i32 %154, 0
  br i1 %155, label %blk_exit23, label %blk_exit670

blk_exit23:                                       ; preds = %brif_next673
  %calcOffset768 = add i32 %149, 24
  %156 = inttoptr i32 %calcOffset768 to i32*
  %157 = load i32, i32* %156, align 4
  call void @perror(i32 %157)
  call void @exit(i32 1)
  unreachable

blk_exit670:                                      ; preds = %brif_next673, %blk_exit636
  %_local_3.35 = phi i32 [ %154, %brif_next673 ], [ %151, %blk_exit636 ]
  %158 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %159 = call i32 @fseek(i32 %_local_3.35, i32 %158, i32 0)
  %160 = load i8, i8* inttoptr (i32 2295 to i8*), align 1
  %161 = icmp eq i8 %160, 0
  br i1 %161, label %blk_exit678, label %brif_next680

blk_exit678:                                      ; preds = %brif_next680, %blk_exit670
  store i32 0, i32* inttoptr (i32 2284 to i32*), align 4
  %162 = add i32 %stack_addr, -1024
  %163 = load i32, i32* %150, align 4
  %164 = call i32 @fgets(i32 %162, i32 1024, i32 %163)
  %brif_val686.not = icmp eq i32 %164, 0
  br i1 %brif_val686.not, label %brif_next687, label %blk_exit684

brif_next687:                                     ; preds = %blk_exit678
  %165 = load i32, i32* null, align 2147483648
  br label %blk_exit683

blk_exit683:                                      ; preds = %blk_exit719, %brif_next693, %brif_next687
  %_local_11.1 = phi i32 [ %165, %brif_next687 ], [ %216, %brif_next693 ], [ %216, %blk_exit719 ]
  %166 = call i32 @fflush(i32 %_local_11.1)
  %167 = load i8, i8* inttoptr (i32 2280 to i8*), align 8
  %168 = icmp eq i8 %167, 0
  br i1 %168, label %blk_exit19, label %brif_next730

brif_next730:                                     ; preds = %blk_exit683
  %169 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %170 = add i32 %169, 56
  %171 = inttoptr i32 %170 to i8*
  %172 = load i8, i8* %171, align 1
  %173 = and i8 %172, 3
  %brif_val734.not = icmp eq i8 %173, 0
  br i1 %brif_val734.not, label %brif_next735, label %blk_exit732

brif_next735:                                     ; preds = %brif_next730
  %174 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %175 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %176 = sub i32 %174, %175
  %177 = add i32 %176, -2
  br label %blk_exit731

blk_exit731:                                      ; preds = %blk_exit749, %brif_next753, %blk_exit736, %brif_next735
  %_local_3.37 = phi i32 [ %177, %brif_next735 ], [ 0, %blk_exit736 ], [ %_local_3.38, %brif_next753 ], [ %212, %blk_exit749 ]
  store i32 %_local_3.37, i32* inttoptr (i32 2284 to i32*), align 4
  %178 = sdiv i32 %_local_3.37, 20
  %179 = icmp sgt i32 %_local_3.37, 139
  %180 = select i1 %179, i32 %178, i32 6
  %181 = call i32 @sleep(i32 %180)
  br label %blk_exit19

blk_exit732:                                      ; preds = %brif_next730
  %calcOffset737 = add i32 %169, 16
  %182 = inttoptr i32 %calcOffset737 to i32*
  %183 = load i32, i32* %182, align 4
  %brif_val738.not = icmp eq i32 %183, 0
  br i1 %brif_val738.not, label %brif_next739, label %blk_exit736

brif_next739:                                     ; preds = %blk_exit732
  %calcOffset740 = add i32 %169, 4
  %184 = inttoptr i32 %calcOffset740 to i32*
  %185 = load i32, i32* %184, align 4
  %186 = call i32 @fdopen(i32 %185, i32 1194)
  store i32 %186, i32* %182, align 4
  %187 = icmp eq i32 %186, 0
  br i1 %187, label %blk_exit22, label %brif_next743

blk_exit22:                                       ; preds = %brif_next739
  %calcOffset769 = add i32 %169, 24
  %188 = inttoptr i32 %calcOffset769 to i32*
  %189 = load i32, i32* %188, align 4
  call void @perror(i32 %189)
  call void @exit(i32 1)
  unreachable

brif_next743:                                     ; preds = %brif_next739
  %190 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset744 = add i32 %190, 16
  %191 = inttoptr i32 %calcOffset744 to i32*
  %192 = load i32, i32* %191, align 4
  br label %blk_exit736

blk_exit736:                                      ; preds = %brif_next743, %blk_exit732
  %_local_5.21 = phi i32 [ %192, %brif_next743 ], [ %183, %blk_exit732 ]
  %193 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %194 = call i32 @fseek(i32 %_local_5.21, i32 %193, i32 0)
  %195 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset745 = add i32 %195, 16
  %196 = inttoptr i32 %calcOffset745 to i32*
  %197 = load i32, i32* %196, align 4
  %198 = call i32 @fgets(i32 %162, i32 1024, i32 %197)
  %199 = icmp eq i32 %198, 0
  br i1 %199, label %blk_exit731, label %loop_entry748

loop_entry748:                                    ; preds = %blk_exit749, %blk_exit736
  %_local_3.38 = phi i32 [ %212, %blk_exit749 ], [ 0, %blk_exit736 ]
  %200 = inttoptr i32 %162 to i8*
  %201 = load i8, i8* %200, align 1
  %202 = sext i8 %201 to i32
  %203 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %204 = add i32 %203, 60
  %205 = inttoptr i32 %204 to i8*
  %206 = load i8, i8* %205, align 1
  %207 = zext i8 %206 to i32
  %.not786 = icmp eq i32 %202, %207
  br i1 %.not786, label %brif_next753, label %blk_exit749

brif_next753:                                     ; preds = %loop_entry748
  %calcOffset754 = add i32 %stack_addr, -1023
  %208 = inttoptr i32 %calcOffset754 to i8*
  %209 = load i8, i8* %208, align 1
  %210 = icmp eq i8 %209, 10
  br i1 %210, label %blk_exit731, label %blk_exit749

blk_exit749:                                      ; preds = %brif_next753, %loop_entry748
  %211 = call i32 @strlen(i32 %162)
  %212 = add i32 %211, %_local_3.38
  %calcOffset757 = add i32 %203, 16
  %213 = inttoptr i32 %calcOffset757 to i32*
  %214 = load i32, i32* %213, align 4
  %215 = call i32 @fgets(i32 %162, i32 1024, i32 %214)
  %brif_val758.not = icmp eq i32 %215, 0
  br i1 %brif_val758.not, label %blk_exit731, label %loop_entry748

blk_exit684:                                      ; preds = %blk_exit678
  %216 = load i32, i32* null, align 2147483648
  br label %loop_entry688

loop_entry688:                                    ; preds = %blk_exit719, %blk_exit684
  %217 = inttoptr i32 %162 to i8*
  %218 = load i8, i8* %217, align 1
  %calcOffset691 = add i32 %149, 60
  %219 = inttoptr i32 %calcOffset691 to i8*
  %220 = load i8, i8* %219, align 1
  %.not787 = icmp eq i8 %218, %220
  br i1 %.not787, label %brif_next693, label %blk_exit689

brif_next693:                                     ; preds = %loop_entry688
  %calcOffset694 = add i32 %stack_addr, -1023
  %221 = inttoptr i32 %calcOffset694 to i8*
  %222 = load i8, i8* %221, align 1
  %223 = icmp eq i8 %222, 10
  br i1 %223, label %blk_exit683, label %blk_exit689

blk_exit689:                                      ; preds = %brif_next693, %loop_entry688
  %calcOffset698 = add i32 %149, 56
  %224 = inttoptr i32 %calcOffset698 to i8*
  %225 = load i8, i8* %224, align 1
  %226 = and i8 %225, 4
  %brif_val699.not.not = icmp eq i8 %226, 0
  %227 = icmp eq i8 %218, 0
  %or.cond804 = select i1 %brif_val699.not.not, i1 true, i1 %227
  br i1 %or.cond804, label %blk_exit697, label %loop_entry703

blk_exit697:                                      ; preds = %blk_exit704, %blk_exit689
  %calcOffset721 = add i32 %149, 38
  %228 = inttoptr i32 %calcOffset721 to i8*
  %229 = load i8, i8* %228, align 1
  %230 = icmp eq i8 %229, 0
  br i1 %230, label %blk_exit720, label %brif_next723

blk_exit720:                                      ; preds = %brif_next723, %blk_exit697
  %231 = call i32 @fputs(i32 %162, i32 %216)
  br label %blk_exit719

blk_exit719:                                      ; preds = %brif_next725, %blk_exit720
  %232 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %233 = add i32 %232, 1
  store i32 %233, i32* inttoptr (i32 2284 to i32*), align 4
  %234 = load i32, i32* %150, align 4
  %235 = call i32 @fgets(i32 %162, i32 1024, i32 %234)
  %brif_val727.not = icmp eq i32 %235, 0
  br i1 %brif_val727.not, label %blk_exit683, label %loop_entry688

brif_next723:                                     ; preds = %blk_exit697
  %236 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val724.not = icmp eq i8 %236, 0
  br i1 %brif_val724.not, label %brif_next725, label %blk_exit720

brif_next725:                                     ; preds = %brif_next723
  %237 = call i32 @strdup(i32 %162)
  %238 = call i32 @fputs(i32 %237, i32 %216)
  call void @free(i32 %237)
  br label %blk_exit719

loop_entry703:                                    ; preds = %blk_exit704, %blk_exit689
  %_local_5.20 = phi i32 [ %247, %blk_exit704 ], [ %162, %blk_exit689 ]
  %_local_3.36.in = phi i8 [ %249, %blk_exit704 ], [ %218, %blk_exit689 ]
  %_local_3.36 = zext i8 %_local_3.36.in to i32
  %239 = shl nuw i32 %_local_3.36, 24
  %240 = sext i8 %_local_3.36.in to i32
  %241 = add nsw i32 %240, -65
  %242 = icmp ult i32 %241, 26
  %brif_val709.not = icmp sgt i32 %239, -16777216
  %or.cond805 = select i1 %242, i1 %brif_val709.not, i1 false
  br i1 %or.cond805, label %blk_exit705, label %blk_exit706

blk_exit705:                                      ; preds = %blk_exit706, %loop_entry703
  %storemerge788 = phi i8 [ 65, %loop_entry703 ], [ 97, %blk_exit706 ]
  %storemerge = phi i8 [ -52, %loop_entry703 ], [ -84, %blk_exit706 ]
  %243 = add i8 %storemerge, %_local_3.36.in
  %244 = urem i8 %243, 26
  %245 = add nuw nsw i8 %244, %storemerge788
  %246 = inttoptr i32 %_local_5.20 to i8*
  store i8 %245, i8* %246, align 1
  br label %blk_exit704

blk_exit704:                                      ; preds = %blk_exit706, %blk_exit705
  %247 = add i32 %_local_5.20, 1
  %248 = inttoptr i32 %247 to i8*
  %249 = load i8, i8* %248, align 1
  %brif_val717.not = icmp eq i8 %249, 0
  br i1 %brif_val717.not, label %blk_exit697, label %loop_entry703

blk_exit706:                                      ; preds = %loop_entry703
  %250 = add nsw i32 %240, -97
  %251 = icmp ult i32 %250, 26
  br i1 %251, label %blk_exit705, label %blk_exit704

brif_next680:                                     ; preds = %blk_exit670
  %calcOffset681 = add i32 %149, 20
  %252 = inttoptr i32 %calcOffset681 to i32*
  %253 = load i32, i32* %252, align 4
  %calcOffset682 = add i32 %stack_addr, -2896
  %254 = inttoptr i32 %calcOffset682 to i32*
  store i32 %253, i32* %254, align 4
  %255 = call i32 @printf(i32 2066, i32 %calcOffset682)
  br label %blk_exit678

brif_next638:                                     ; preds = %blk_exit602
  %256 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %257 = add i32 %256, 56
  %258 = inttoptr i32 %257 to i8*
  %259 = load i8, i8* %258, align 1
  %260 = and i8 %259, 3
  %brif_val642.not = icmp eq i8 %260, 0
  br i1 %brif_val642.not, label %brif_next643, label %blk_exit640

brif_next643:                                     ; preds = %brif_next638
  %261 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %262 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %263 = sub i32 %261, %262
  %264 = add i32 %263, -2
  br label %blk_exit639

blk_exit639:                                      ; preds = %blk_exit657, %brif_next661, %blk_exit644, %brif_next643
  %_local_3.33 = phi i32 [ %264, %brif_next643 ], [ 0, %blk_exit644 ], [ %_local_3.34, %brif_next661 ], [ %297, %blk_exit657 ]
  store i32 %_local_3.33, i32* inttoptr (i32 2284 to i32*), align 4
  %265 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %.not785 = icmp sgt i32 %_local_3.33, %265
  br i1 %.not785, label %blk_exit636, label %loop_entry404

blk_exit640:                                      ; preds = %brif_next638
  %calcOffset645 = add i32 %256, 16
  %266 = inttoptr i32 %calcOffset645 to i32*
  %267 = load i32, i32* %266, align 4
  %brif_val646.not = icmp eq i32 %267, 0
  br i1 %brif_val646.not, label %brif_next647, label %blk_exit644

brif_next647:                                     ; preds = %blk_exit640
  %calcOffset648 = add i32 %256, 4
  %268 = inttoptr i32 %calcOffset648 to i32*
  %269 = load i32, i32* %268, align 4
  %270 = call i32 @fdopen(i32 %269, i32 1194)
  store i32 %270, i32* %266, align 4
  %271 = icmp eq i32 %270, 0
  br i1 %271, label %blk_exit24, label %brif_next651

blk_exit24:                                       ; preds = %brif_next647
  %calcOffset767 = add i32 %256, 24
  %272 = inttoptr i32 %calcOffset767 to i32*
  %273 = load i32, i32* %272, align 4
  call void @perror(i32 %273)
  call void @exit(i32 1)
  unreachable

brif_next651:                                     ; preds = %brif_next647
  %274 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset652 = add i32 %274, 16
  %275 = inttoptr i32 %calcOffset652 to i32*
  %276 = load i32, i32* %275, align 4
  br label %blk_exit644

blk_exit644:                                      ; preds = %brif_next651, %blk_exit640
  %_local_5.19 = phi i32 [ %276, %brif_next651 ], [ %267, %blk_exit640 ]
  %277 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %278 = call i32 @fseek(i32 %_local_5.19, i32 %277, i32 0)
  %279 = add i32 %stack_addr, -1024
  %280 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset653 = add i32 %280, 16
  %281 = inttoptr i32 %calcOffset653 to i32*
  %282 = load i32, i32* %281, align 4
  %283 = call i32 @fgets(i32 %279, i32 1024, i32 %282)
  %284 = icmp eq i32 %283, 0
  br i1 %284, label %blk_exit639, label %loop_entry656

loop_entry656:                                    ; preds = %blk_exit657, %blk_exit644
  %_local_3.34 = phi i32 [ %297, %blk_exit657 ], [ 0, %blk_exit644 ]
  %285 = inttoptr i32 %279 to i8*
  %286 = load i8, i8* %285, align 1
  %287 = sext i8 %286 to i32
  %288 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %289 = add i32 %288, 60
  %290 = inttoptr i32 %289 to i8*
  %291 = load i8, i8* %290, align 1
  %292 = zext i8 %291 to i32
  %.not789 = icmp eq i32 %287, %292
  br i1 %.not789, label %brif_next661, label %blk_exit657

brif_next661:                                     ; preds = %loop_entry656
  %calcOffset662 = add i32 %stack_addr, -1023
  %293 = inttoptr i32 %calcOffset662 to i8*
  %294 = load i8, i8* %293, align 1
  %295 = icmp eq i8 %294, 10
  br i1 %295, label %blk_exit639, label %blk_exit657

blk_exit657:                                      ; preds = %brif_next661, %loop_entry656
  %296 = call i32 @strlen(i32 %279)
  %297 = add i32 %296, %_local_3.34
  %calcOffset665 = add i32 %288, 16
  %298 = inttoptr i32 %calcOffset665 to i32*
  %299 = load i32, i32* %298, align 4
  %300 = call i32 @fgets(i32 %279, i32 1024, i32 %299)
  %brif_val666.not = icmp eq i32 %300, 0
  br i1 %brif_val666.not, label %blk_exit639, label %loop_entry656

brif_next604:                                     ; preds = %brif_next601
  %301 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %302 = add i32 %301, 56
  %303 = inttoptr i32 %302 to i8*
  %304 = load i8, i8* %303, align 1
  %305 = and i8 %304, 3
  %brif_val608.not = icmp eq i8 %305, 0
  br i1 %brif_val608.not, label %brif_next609, label %blk_exit606

brif_next609:                                     ; preds = %brif_next604
  %306 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %307 = sub i32 %144, %306
  %308 = add i32 %307, -2
  br label %blk_exit605

blk_exit605:                                      ; preds = %blk_exit623, %brif_next627, %blk_exit610, %brif_next609
  %_local_3.31 = phi i32 [ %308, %brif_next609 ], [ 0, %blk_exit610 ], [ %_local_3.32, %brif_next627 ], [ %342, %blk_exit623 ]
  store i32 %_local_3.31, i32* inttoptr (i32 2284 to i32*), align 4
  %309 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %310 = icmp sgt i32 %_local_3.31, %309
  br i1 %310, label %loop_entry404, label %blk_exit602

blk_exit606:                                      ; preds = %brif_next604
  %calcOffset611 = add i32 %301, 16
  %311 = inttoptr i32 %calcOffset611 to i32*
  %312 = load i32, i32* %311, align 4
  %brif_val612.not = icmp eq i32 %312, 0
  br i1 %brif_val612.not, label %brif_next613, label %blk_exit610

brif_next613:                                     ; preds = %blk_exit606
  %calcOffset614 = add i32 %301, 4
  %313 = inttoptr i32 %calcOffset614 to i32*
  %314 = load i32, i32* %313, align 4
  %315 = call i32 @fdopen(i32 %314, i32 1194)
  store i32 %315, i32* %311, align 4
  %316 = icmp eq i32 %315, 0
  br i1 %316, label %blk_exit25, label %brif_next617

blk_exit25:                                       ; preds = %brif_next613
  %calcOffset766 = add i32 %301, 24
  %317 = inttoptr i32 %calcOffset766 to i32*
  %318 = load i32, i32* %317, align 4
  call void @perror(i32 %318)
  call void @exit(i32 1)
  unreachable

brif_next617:                                     ; preds = %brif_next613
  %319 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset618 = add i32 %319, 16
  %320 = inttoptr i32 %calcOffset618 to i32*
  %321 = load i32, i32* %320, align 4
  br label %blk_exit610

blk_exit610:                                      ; preds = %brif_next617, %blk_exit606
  %_param_0.12 = phi i32 [ %321, %brif_next617 ], [ %312, %blk_exit606 ]
  %322 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %323 = call i32 @fseek(i32 %_param_0.12, i32 %322, i32 0)
  %324 = add i32 %stack_addr, -1024
  %325 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset619 = add i32 %325, 16
  %326 = inttoptr i32 %calcOffset619 to i32*
  %327 = load i32, i32* %326, align 4
  %328 = call i32 @fgets(i32 %324, i32 1024, i32 %327)
  %329 = icmp eq i32 %328, 0
  br i1 %329, label %blk_exit605, label %loop_entry622

loop_entry622:                                    ; preds = %blk_exit623, %blk_exit610
  %_local_3.32 = phi i32 [ %342, %blk_exit623 ], [ 0, %blk_exit610 ]
  %330 = inttoptr i32 %324 to i8*
  %331 = load i8, i8* %330, align 1
  %332 = sext i8 %331 to i32
  %333 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %334 = add i32 %333, 60
  %335 = inttoptr i32 %334 to i8*
  %336 = load i8, i8* %335, align 1
  %337 = zext i8 %336 to i32
  %.not790 = icmp eq i32 %332, %337
  br i1 %.not790, label %brif_next627, label %blk_exit623

brif_next627:                                     ; preds = %loop_entry622
  %calcOffset628 = add i32 %stack_addr, -1023
  %338 = inttoptr i32 %calcOffset628 to i8*
  %339 = load i8, i8* %338, align 1
  %340 = icmp eq i8 %339, 10
  br i1 %340, label %blk_exit605, label %blk_exit623

blk_exit623:                                      ; preds = %brif_next627, %loop_entry622
  %341 = call i32 @strlen(i32 %324)
  %342 = add i32 %341, %_local_3.32
  %calcOffset631 = add i32 %333, 16
  %343 = inttoptr i32 %calcOffset631 to i32*
  %344 = load i32, i32* %343, align 4
  %345 = call i32 @fgets(i32 %324, i32 1024, i32 %344)
  %brif_val632.not = icmp eq i32 %345, 0
  br i1 %brif_val632.not, label %blk_exit605, label %loop_entry622

blk_exit26:                                       ; preds = %brif_next598, %blk_exit587
  call void @exit(i32 1)
  unreachable

brif_next585:                                     ; preds = %blk_exit564
  %346 = sub i32 %124, %125
  store i32 %346, i32* %102, align 4
  br label %blk_exit582

brif_next563.blk_exit564_crit_edge:               ; preds = %brif_next563
  %.pre = add i32 %_local_3.24, 44
  %.pre812 = inttoptr i32 %.pre to i32*
  br label %blk_exit564

loop_entry493:                                    ; preds = %blk_exit494, %brif_next488
  %_local_3.25 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  %347 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %348 = icmp eq i8 %347, 0
  br i1 %348, label %blk_exit495, label %brif_next497

blk_exit495:                                      ; preds = %loop_entry493
  call void @get_tbl(i32 %_local_3.25)
  %349 = add i32 %_local_3.25, 44
  %350 = inttoptr i32 %349 to i32*
  %351 = load i32, i32* %350, align 4
  %calcOffset536 = add i32 %stack_addr, -1024
  %352 = inttoptr i32 %calcOffset536 to i64*
  %353 = inttoptr i32 %calcOffset536 to i8*
  call void @llvm.memset.p0i8.i64(i8* %353, i8 0, i64 8, i1 true)
  %354 = call i32 @getenv(i32 1447)
  %355 = icmp eq i32 %354, 0
  br i1 %355, label %blk_exit538, label %brif_next540

blk_exit538:                                      ; preds = %blk_exit495
  %356 = call i32 @getenv(i32 1522)
  %brif_val542.not = icmp eq i32 %356, 0
  br i1 %brif_val542.not, label %brif_next543, label %blk_exit541

brif_next543:                                     ; preds = %blk_exit538
  %357 = call i32 @fopen(i32 1208, i32 1414)
  %358 = icmp eq i32 %357, 0
  br i1 %358, label %blk_exit541, label %brif_next545

brif_next545:                                     ; preds = %brif_next543
  %359 = call i32 @fread(i32 %calcOffset536, i32 8, i32 1, i32 %357)
  %360 = call i32 @fclose(i32 %357)
  %.not794 = icmp eq i32 %359, 1
  br i1 %.not794, label %brif_next547, label %blk_exit541

brif_next547:                                     ; preds = %brif_next545
  %361 = load i64, i64* %352, align 8
  %362 = zext i32 %351 to i64
  %363 = urem i64 %361, %362
  %364 = trunc i64 %363 to i32
  br label %blk_exit537

blk_exit541:                                      ; preds = %brif_next545, %brif_next543, %blk_exit538
  %365 = call i32 @random()
  %366 = urem i32 %365, %351
  br label %blk_exit537

blk_exit537:                                      ; preds = %brif_next540, %blk_exit541, %brif_next547
  %_local_5.14 = phi i32 [ %366, %blk_exit541 ], [ %364, %brif_next547 ], [ %380, %brif_next540 ]
  %calcOffset549 = add i32 %_local_3.25, 68
  %367 = inttoptr i32 %calcOffset549 to i32*
  %368 = load i32, i32* %367, align 4
  %369 = add i32 %368, 44
  %370 = inttoptr i32 %369 to i32*
  %371 = load i32, i32* %370, align 4
  %372 = icmp slt i32 %_local_5.14, %371
  br i1 %372, label %blk_exit494, label %loop_entry553

loop_entry553:                                    ; preds = %loop_entry553, %blk_exit537
  %_local_5.15 = phi i32 [ %_local_5.14, %blk_exit537 ], [ %373, %loop_entry553 ]
  %_local_3.30 = phi i32 [ %368, %blk_exit537 ], [ %375, %loop_entry553 ]
  %_param_0.10 = phi i32 [ %371, %blk_exit537 ], [ %378, %loop_entry553 ]
  %373 = sub i32 %_local_5.15, %_param_0.10
  %calcOffset554 = add i32 %_local_3.30, 76
  %374 = inttoptr i32 %calcOffset554 to i32*
  %375 = load i32, i32* %374, align 4
  %376 = add i32 %375, 44
  %377 = inttoptr i32 %376 to i32*
  %378 = load i32, i32* %377, align 4
  %.not793 = icmp slt i32 %373, %378
  br i1 %.not793, label %blk_exit494, label %loop_entry553

brif_next540:                                     ; preds = %blk_exit495
  %379 = call i32 @atol(i32 %354)
  %380 = urem i32 %379, %351
  br label %blk_exit537

blk_exit494:                                      ; preds = %loop_entry524, %blk_exit515, %blk_exit500, %loop_entry553, %blk_exit537
  %_local_3.26 = phi i32 [ %368, %blk_exit537 ], [ %375, %loop_entry553 ], [ %401, %blk_exit500 ], [ %_local_3.27, %blk_exit515 ], [ %422, %loop_entry524 ]
  %calcOffset558 = add i32 %_local_3.26, 68
  %381 = inttoptr i32 %calcOffset558 to i32*
  %382 = load i32, i32* %381, align 4
  %brif_val559.not = icmp eq i32 %382, 0
  br i1 %brif_val559.not, label %blk_exit489, label %loop_entry493

brif_next497:                                     ; preds = %loop_entry493
  %calcOffset498 = add i32 %_local_3.25, 64
  %383 = inttoptr i32 %calcOffset498 to i32*
  %384 = load i32, i32* %383, align 4
  %calcOffset499 = add i32 %stack_addr, -1024
  %385 = inttoptr i32 %calcOffset499 to i64*
  %386 = inttoptr i32 %calcOffset499 to i8*
  call void @llvm.memset.p0i8.i64(i8* %386, i8 0, i64 8, i1 true)
  %387 = call i32 @getenv(i32 1447)
  %388 = icmp eq i32 %387, 0
  br i1 %388, label %blk_exit501, label %brif_next503

blk_exit501:                                      ; preds = %brif_next497
  %389 = call i32 @getenv(i32 1522)
  %brif_val505.not = icmp eq i32 %389, 0
  br i1 %brif_val505.not, label %brif_next506, label %blk_exit504

brif_next506:                                     ; preds = %blk_exit501
  %390 = call i32 @fopen(i32 1208, i32 1414)
  %391 = icmp eq i32 %390, 0
  br i1 %391, label %blk_exit504, label %brif_next508

brif_next508:                                     ; preds = %brif_next506
  %392 = call i32 @fread(i32 %calcOffset499, i32 8, i32 1, i32 %390)
  %393 = call i32 @fclose(i32 %390)
  %.not792 = icmp eq i32 %392, 1
  br i1 %.not792, label %brif_next510, label %blk_exit504

brif_next510:                                     ; preds = %brif_next508
  %394 = load i64, i64* %385, align 8
  %395 = zext i32 %384 to i64
  %396 = urem i64 %394, %395
  %397 = trunc i64 %396 to i32
  br label %blk_exit500

blk_exit504:                                      ; preds = %brif_next508, %brif_next506, %blk_exit501
  %398 = call i32 @random()
  %399 = urem i32 %398, %384
  br label %blk_exit500

blk_exit500:                                      ; preds = %brif_next503, %blk_exit504, %brif_next510
  %_local_5.10 = phi i32 [ %399, %blk_exit504 ], [ %397, %brif_next510 ], [ %429, %brif_next503 ]
  %calcOffset512 = add i32 %_local_3.25, 68
  %400 = inttoptr i32 %calcOffset512 to i32*
  %401 = load i32, i32* %400, align 4
  %402 = icmp eq i32 %_local_5.10, 0
  br i1 %402, label %blk_exit494, label %brif_next514

brif_next514:                                     ; preds = %blk_exit500
  %403 = add i32 %_local_5.10, -1
  %404 = and i32 %_local_5.10, 7
  %405 = icmp eq i32 %404, 0
  br i1 %405, label %blk_exit515, label %loop_entry518

blk_exit515:                                      ; preds = %loop_entry518, %brif_next514
  %_local_5.11 = phi i32 [ %_local_5.10, %brif_next514 ], [ %424, %loop_entry518 ]
  %_local_3.27 = phi i32 [ %401, %brif_next514 ], [ %426, %loop_entry518 ]
  %406 = icmp ult i32 %403, 7
  br i1 %406, label %blk_exit494, label %loop_entry524

loop_entry524:                                    ; preds = %loop_entry524, %blk_exit515
  %_local_5.13 = phi i32 [ %_local_5.11, %blk_exit515 ], [ %423, %loop_entry524 ]
  %_local_3.29 = phi i32 [ %_local_3.27, %blk_exit515 ], [ %422, %loop_entry524 ]
  %calcOffset525 = add i32 %_local_3.29, 76
  %407 = inttoptr i32 %calcOffset525 to i32*
  %408 = load i32, i32* %407, align 4
  %calcOffset526 = add i32 %408, 76
  %409 = inttoptr i32 %calcOffset526 to i32*
  %410 = load i32, i32* %409, align 4
  %calcOffset527 = add i32 %410, 76
  %411 = inttoptr i32 %calcOffset527 to i32*
  %412 = load i32, i32* %411, align 4
  %calcOffset528 = add i32 %412, 76
  %413 = inttoptr i32 %calcOffset528 to i32*
  %414 = load i32, i32* %413, align 4
  %calcOffset529 = add i32 %414, 76
  %415 = inttoptr i32 %calcOffset529 to i32*
  %416 = load i32, i32* %415, align 4
  %calcOffset530 = add i32 %416, 76
  %417 = inttoptr i32 %calcOffset530 to i32*
  %418 = load i32, i32* %417, align 4
  %calcOffset531 = add i32 %418, 76
  %419 = inttoptr i32 %calcOffset531 to i32*
  %420 = load i32, i32* %419, align 4
  %calcOffset532 = add i32 %420, 76
  %421 = inttoptr i32 %calcOffset532 to i32*
  %422 = load i32, i32* %421, align 4
  %423 = add i32 %_local_5.13, -8
  %brif_val533.not = icmp eq i32 %423, 0
  br i1 %brif_val533.not, label %blk_exit494, label %loop_entry524

loop_entry518:                                    ; preds = %loop_entry518, %brif_next514
  %_local_5.12 = phi i32 [ %_local_5.10, %brif_next514 ], [ %424, %loop_entry518 ]
  %_local_3.28 = phi i32 [ %401, %brif_next514 ], [ %426, %loop_entry518 ]
  %_param_0.9 = phi i32 [ %404, %brif_next514 ], [ %427, %loop_entry518 ]
  %424 = add i32 %_local_5.12, -1
  %calcOffset519 = add i32 %_local_3.28, 76
  %425 = inttoptr i32 %calcOffset519 to i32*
  %426 = load i32, i32* %425, align 4
  %427 = add i32 %_param_0.9, -1
  %brif_val520.not = icmp eq i32 %427, 0
  br i1 %brif_val520.not, label %blk_exit515, label %loop_entry518

brif_next503:                                     ; preds = %brif_next497
  %428 = call i32 @atol(i32 %387)
  %429 = urem i32 %428, %384
  br label %blk_exit500

blk_exit407:                                      ; preds = %brif_next411, %brif_next442
  %_local_3.20 = phi i32 [ %_local_3.21, %brif_next442 ], [ %80, %brif_next411 ]
  %430 = load i8, i8* inttoptr (i32 3336 to i8*), align 8
  %brif_val447.not = icmp eq i8 %430, 0
  br i1 %brif_val447.not, label %brif_next448, label %blk_exit446

brif_next448:                                     ; preds = %blk_exit407
  store i32 -1, i32* inttoptr (i32 3348 to i32*), align 4
  call void @llvm.memset.p0i8.i64(i8* inttoptr (i32 3340 to i8*), i8 0, i64 8, i1 true)
  br label %loop_entry449

loop_entry449:                                    ; preds = %blk_exit450, %brif_next448
  %_local_5.7 = phi i32 [ %_local_3.20, %brif_next448 ], [ %447, %blk_exit450 ]
  call void @get_tbl(i32 %_local_5.7)
  %431 = inttoptr i32 %_local_5.7 to i32*
  %432 = load i32, i32* %431, align 4
  %.not780 = icmp eq i32 %432, -1
  br i1 %.not780, label %brif_next453, label %blk_exit450

brif_next453:                                     ; preds = %loop_entry449
  %433 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %434 = add i32 %_local_5.7, 44
  %435 = inttoptr i32 %434 to i32*
  %436 = load i32, i32* %435, align 4
  %437 = add i32 %433, %436
  store i32 %437, i32* inttoptr (i32 3340 to i32*), align 4
  %438 = load i32, i32* inttoptr (i32 3344 to i32*), align 16
  %439 = add i32 %_local_5.7, 48
  %440 = inttoptr i32 %439 to i32*
  %441 = load i32, i32* %440, align 4
  %.not781 = icmp ult i32 %438, %441
  br i1 %.not781, label %brif_next458, label %blk_exit455

brif_next458:                                     ; preds = %brif_next453
  store i32 %441, i32* inttoptr (i32 3344 to i32*), align 16
  br label %blk_exit455

blk_exit455:                                      ; preds = %brif_next458, %brif_next453
  %442 = load i32, i32* inttoptr (i32 3348 to i32*), align 4
  %443 = add i32 %_local_5.7, 52
  %444 = inttoptr i32 %443 to i32*
  %445 = load i32, i32* %444, align 4
  %.not782 = icmp ugt i32 %442, %445
  br i1 %.not782, label %brif_next461, label %blk_exit450

brif_next461:                                     ; preds = %blk_exit455
  store i32 %445, i32* inttoptr (i32 3348 to i32*), align 4
  br label %blk_exit450

blk_exit450:                                      ; preds = %brif_next461, %blk_exit455, %loop_entry449
  %calcOffset462 = add i32 %_local_5.7, 76
  %446 = inttoptr i32 %calcOffset462 to i32*
  %447 = load i32, i32* %446, align 4
  %brif_val463.not = icmp eq i32 %447, 0
  br i1 %brif_val463.not, label %brif_next464, label %loop_entry449

brif_next464:                                     ; preds = %blk_exit450
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  br label %blk_exit446

blk_exit446:                                      ; preds = %brif_next464, %blk_exit407
  %448 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %calcOffset465 = add i32 %stack_addr, -1024
  %449 = inttoptr i32 %calcOffset465 to i64*
  %450 = inttoptr i32 %calcOffset465 to i8*
  call void @llvm.memset.p0i8.i64(i8* %450, i8 0, i64 8, i1 true)
  %451 = call i32 @getenv(i32 1447)
  %452 = icmp eq i32 %451, 0
  br i1 %452, label %blk_exit467, label %brif_next469

blk_exit467:                                      ; preds = %blk_exit446
  %453 = call i32 @getenv(i32 1522)
  %brif_val471.not = icmp eq i32 %453, 0
  br i1 %brif_val471.not, label %brif_next472, label %blk_exit470

brif_next472:                                     ; preds = %blk_exit467
  %454 = call i32 @fopen(i32 1208, i32 1414)
  %455 = icmp eq i32 %454, 0
  br i1 %455, label %blk_exit470, label %brif_next474

brif_next474:                                     ; preds = %brif_next472
  %456 = call i32 @fread(i32 %calcOffset465, i32 8, i32 1, i32 %454)
  %457 = call i32 @fclose(i32 %454)
  %.not795 = icmp eq i32 %456, 1
  br i1 %.not795, label %brif_next476, label %blk_exit470

brif_next476:                                     ; preds = %brif_next474
  %458 = load i64, i64* %449, align 8
  %459 = zext i32 %448 to i64
  %460 = urem i64 %458, %459
  %461 = trunc i64 %460 to i32
  br label %blk_exit466

blk_exit470:                                      ; preds = %brif_next474, %brif_next472, %blk_exit467
  %462 = call i32 @random()
  %463 = urem i32 %462, %448
  br label %blk_exit466

blk_exit466:                                      ; preds = %brif_next469, %blk_exit470, %brif_next476
  %_local_5.8 = phi i32 [ %463, %blk_exit470 ], [ %461, %brif_next476 ], [ %475, %brif_next469 ]
  %464 = add i32 %_local_3.20, 44
  %465 = inttoptr i32 %464 to i32*
  %466 = load i32, i32* %465, align 4
  %467 = icmp slt i32 %_local_5.8, %466
  br i1 %467, label %blk_exit406, label %loop_entry481

loop_entry481:                                    ; preds = %loop_entry481, %blk_exit466
  %_local_5.9 = phi i32 [ %_local_5.8, %blk_exit466 ], [ %468, %loop_entry481 ]
  %_local_3.23 = phi i32 [ %_local_3.20, %blk_exit466 ], [ %470, %loop_entry481 ]
  %_param_0.8 = phi i32 [ %466, %blk_exit466 ], [ %473, %loop_entry481 ]
  %468 = sub i32 %_local_5.9, %_param_0.8
  %calcOffset482 = add i32 %_local_3.23, 76
  %469 = inttoptr i32 %calcOffset482 to i32*
  %470 = load i32, i32* %469, align 4
  %471 = add i32 %470, 44
  %472 = inttoptr i32 %471 to i32*
  %473 = load i32, i32* %472, align 4
  %.not783 = icmp slt i32 %468, %473
  br i1 %.not783, label %blk_exit406, label %loop_entry481

brif_next469:                                     ; preds = %blk_exit446
  %474 = call i32 @atol(i32 %451)
  %475 = urem i32 %474, %448
  br label %blk_exit466

brif_next411:                                     ; preds = %loop_entry404
  %476 = icmp eq i32 %82, -1
  br i1 %476, label %blk_exit407, label %brif_next413

brif_next413:                                     ; preds = %brif_next411
  %calcOffset414 = add i32 %stack_addr, -1024
  %477 = inttoptr i32 %calcOffset414 to i64*
  %478 = inttoptr i32 %calcOffset414 to i8*
  call void @llvm.memset.p0i8.i64(i8* %478, i8 0, i64 8, i1 true)
  %479 = call i32 @getenv(i32 1447)
  %480 = icmp eq i32 %479, 0
  br i1 %480, label %blk_exit416, label %brif_next418

blk_exit416:                                      ; preds = %brif_next413
  %481 = call i32 @getenv(i32 1522)
  %brif_val420.not = icmp eq i32 %481, 0
  br i1 %brif_val420.not, label %brif_next421, label %blk_exit419

brif_next421:                                     ; preds = %blk_exit416
  %482 = call i32 @fopen(i32 1208, i32 1414)
  %483 = icmp eq i32 %482, 0
  br i1 %483, label %blk_exit419, label %brif_next423

brif_next423:                                     ; preds = %brif_next421
  %484 = call i32 @fread(i32 %calcOffset414, i32 8, i32 1, i32 %482)
  %485 = call i32 @fclose(i32 %482)
  %.not798 = icmp eq i32 %484, 1
  br i1 %.not798, label %brif_next425, label %blk_exit419

brif_next425:                                     ; preds = %brif_next423
  %486 = load i64, i64* %477, align 8
  %487 = urem i64 %486, 100
  %488 = trunc i64 %487 to i32
  br label %blk_exit415

blk_exit419:                                      ; preds = %brif_next423, %brif_next421, %blk_exit416
  %489 = call i32 @random()
  %490 = urem i32 %489, 100
  br label %blk_exit415

blk_exit415:                                      ; preds = %brif_next418, %blk_exit419, %brif_next425
  %_param_0.6 = phi i32 [ %490, %blk_exit419 ], [ %488, %brif_next425 ], [ %501, %brif_next418 ]
  %491 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %492 = inttoptr i32 %491 to i32*
  %493 = load i32, i32* %492, align 4
  %.not778 = icmp eq i32 %493, -1
  br i1 %.not778, label %blk_exit408, label %blk_exit427

blk_exit427:                                      ; preds = %blk_exit415
  %.not796 = icmp slt i32 %_param_0.6, %493
  br i1 %.not796, label %blk_exit408, label %loop_entry434

loop_entry434:                                    ; preds = %brif_next438, %blk_exit427
  %_local_3.22 = phi i32 [ %491, %blk_exit427 ], [ %495, %brif_next438 ]
  %_param_1.7 = phi i32 [ %493, %blk_exit427 ], [ %497, %brif_next438 ]
  %_param_0.7 = phi i32 [ %_param_0.6, %blk_exit427 ], [ %499, %brif_next438 ]
  %calcOffset435 = add i32 %_local_3.22, 76
  %494 = inttoptr i32 %calcOffset435 to i32*
  %495 = load i32, i32* %494, align 4
  %496 = inttoptr i32 %495 to i32*
  %497 = load i32, i32* %496, align 4
  %498 = icmp eq i32 %497, -1
  br i1 %498, label %blk_exit408, label %brif_next438

brif_next438:                                     ; preds = %loop_entry434
  %499 = sub i32 %_param_0.7, %_param_1.7
  %.not797 = icmp slt i32 %499, %497
  br i1 %.not797, label %blk_exit408, label %loop_entry434

brif_next418:                                     ; preds = %brif_next413
  %500 = call i32 @atol(i32 %479)
  %501 = urem i32 %500, 100
  br label %blk_exit415

brif_next366:                                     ; preds = %blk_exit344
  %502 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %503 = load i8, i8* inttoptr (i32 3336 to i8*), align 8
  %brif_val371.not = icmp eq i8 %503, 0
  br i1 %brif_val371.not, label %brif_next372, label %blk_exit370

brif_next372:                                     ; preds = %brif_next366
  store i32 -1, i32* inttoptr (i32 3348 to i32*), align 4
  call void @llvm.memset.p0i8.i64(i8* inttoptr (i32 3340 to i8*), i8 0, i64 8, i1 true)
  %504 = icmp eq i32 %502, 0
  br i1 %504, label %blk_exit369, label %loop_entry375

blk_exit369:                                      ; preds = %brif_next372
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  br i1 %73, label %blk_exit21, label %blk_exit368

blk_exit368:                                      ; preds = %brif_next390, %blk_exit369
  %505 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit367

loop_entry375:                                    ; preds = %blk_exit376, %brif_next372
  %_local_3.18 = phi i32 [ %502, %brif_next372 ], [ %522, %blk_exit376 ]
  call void @get_tbl(i32 %_local_3.18)
  %506 = inttoptr i32 %_local_3.18 to i32*
  %507 = load i32, i32* %506, align 4
  %.not775 = icmp eq i32 %507, -1
  br i1 %.not775, label %brif_next379, label %blk_exit376

brif_next379:                                     ; preds = %loop_entry375
  %508 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %509 = add i32 %_local_3.18, 44
  %510 = inttoptr i32 %509 to i32*
  %511 = load i32, i32* %510, align 4
  %512 = add i32 %508, %511
  store i32 %512, i32* inttoptr (i32 3340 to i32*), align 4
  %513 = load i32, i32* inttoptr (i32 3344 to i32*), align 16
  %514 = add i32 %_local_3.18, 48
  %515 = inttoptr i32 %514 to i32*
  %516 = load i32, i32* %515, align 4
  %.not776 = icmp ult i32 %513, %516
  br i1 %.not776, label %brif_next384, label %blk_exit381

brif_next384:                                     ; preds = %brif_next379
  store i32 %516, i32* inttoptr (i32 3344 to i32*), align 16
  br label %blk_exit381

blk_exit381:                                      ; preds = %brif_next384, %brif_next379
  %517 = load i32, i32* inttoptr (i32 3348 to i32*), align 4
  %518 = add i32 %_local_3.18, 52
  %519 = inttoptr i32 %518 to i32*
  %520 = load i32, i32* %519, align 4
  %.not777 = icmp ugt i32 %517, %520
  br i1 %.not777, label %brif_next387, label %blk_exit376

brif_next387:                                     ; preds = %blk_exit381
  store i32 %520, i32* inttoptr (i32 3348 to i32*), align 4
  br label %blk_exit376

blk_exit376:                                      ; preds = %brif_next387, %blk_exit381, %loop_entry375
  %calcOffset388 = add i32 %_local_3.18, 76
  %521 = inttoptr i32 %calcOffset388 to i32*
  %522 = load i32, i32* %521, align 4
  %brif_val389.not = icmp eq i32 %522, 0
  br i1 %brif_val389.not, label %brif_next390, label %loop_entry375

brif_next390:                                     ; preds = %blk_exit376
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  %523 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %524 = icmp eq i8 %523, 0
  br i1 %524, label %blk_exit21, label %blk_exit368

blk_exit21:                                       ; preds = %blk_exit370, %brif_next390, %blk_exit369
  %525 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit20

blk_exit20:                                       ; preds = %brif_next403, %blk_exit367, %blk_exit21
  %_param_0.0 = phi i32 [ %525, %blk_exit21 ], [ 0, %blk_exit367 ], [ %_local_3.17, %brif_next403 ]
  call void @print_list(i32 %_param_0.0, i32 0)
  br label %blk_exit19

blk_exit367:                                      ; preds = %blk_exit370, %blk_exit368
  %_local_3.17 = phi i32 [ %505, %blk_exit368 ], [ %502, %blk_exit370 ]
  call void @llvm.memset.p0i8.i64(i8* inttoptr (i32 3352 to i8*), i8 0, i64 8, i1 true)
  %526 = icmp eq i32 %_local_3.17, 0
  br i1 %526, label %blk_exit20, label %loop_entry399

loop_entry399:                                    ; preds = %loop_entry399, %blk_exit367
  %_local_5.5 = phi i32 [ %532, %loop_entry399 ], [ %_local_3.17, %blk_exit367 ]
  %_param_1.6 = phi i32 [ %530, %loop_entry399 ], [ 0, %blk_exit367 ]
  %_param_0.5 = phi i32 [ %527, %loop_entry399 ], [ 0, %blk_exit367 ]
  %527 = add i32 %_param_0.5, 1
  %calcOffset400 = add i32 %_local_5.5, 64
  %528 = inttoptr i32 %calcOffset400 to i32*
  %529 = load i32, i32* %528, align 4
  %530 = add i32 %529, %_param_1.6
  %calcOffset401 = add i32 %_local_5.5, 76
  %531 = inttoptr i32 %calcOffset401 to i32*
  %532 = load i32, i32* %531, align 4
  %brif_val402.not = icmp eq i32 %532, 0
  br i1 %brif_val402.not, label %brif_next403, label %loop_entry399

brif_next403:                                     ; preds = %loop_entry399
  store i32 %530, i32* inttoptr (i32 3352 to i32*), align 8
  store i32 %527, i32* inttoptr (i32 3356 to i32*), align 4
  br label %blk_exit20

blk_exit370:                                      ; preds = %brif_next366
  br i1 %73, label %blk_exit21, label %blk_exit367

brif_next348:                                     ; preds = %blk_exit339
  %533 = sub i32 100, %_param_1.4
  %534 = icmp ult i32 %_local_4.2, 2
  br i1 %534, label %blk_exit349, label %brif_next351

blk_exit349:                                      ; preds = %blk_exit355, %brif_next351, %brif_next348
  %_local_3.14 = phi i32 [ %533, %brif_next348 ], [ %533, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %535 = inttoptr i32 %_local_5.3 to i32*
  store i32 %_local_3.14, i32* %535, align 4
  br label %blk_exit344

brif_next351:                                     ; preds = %brif_next348
  %536 = udiv i32 %533, %_local_4.2
  %537 = icmp eq i32 %65, %_local_5.3
  br i1 %537, label %blk_exit349, label %loop_entry354

loop_entry354:                                    ; preds = %blk_exit355, %brif_next351
  %_local_12.1 = phi i32 [ %65, %brif_next351 ], [ %542, %blk_exit355 ]
  %_local_3.15 = phi i32 [ %533, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %538 = inttoptr i32 %_local_12.1 to i32*
  %539 = load i32, i32* %538, align 4
  %.not773 = icmp eq i32 %539, -1
  br i1 %.not773, label %brif_next358, label %blk_exit355

brif_next358:                                     ; preds = %loop_entry354
  store i32 %536, i32* %538, align 4
  %540 = sub i32 %_local_3.15, %536
  br label %blk_exit355

blk_exit355:                                      ; preds = %brif_next358, %loop_entry354
  %_local_3.16 = phi i32 [ %540, %brif_next358 ], [ %_local_3.15, %loop_entry354 ]
  %calcOffset360 = add i32 %_local_12.1, 80
  %541 = inttoptr i32 %calcOffset360 to i32*
  %542 = load i32, i32* %541, align 4
  %.not774 = icmp eq i32 %542, %_local_5.3
  br i1 %.not774, label %blk_exit349, label %loop_entry354

blk_exit30:                                       ; preds = %blk_exit334
  %543 = load i32, i32* null, align 2147483648
  %544 = call i32 @fprintf(i32 %543, i32 1655, i32 0)
  call void @exit(i32 1)
  unreachable

brif_next323:                                     ; preds = %blk_exit314
  %545 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %546 = and i8 %545, 1
  br label %loop_entry324

loop_entry324:                                    ; preds = %loop_entry324, %brif_next323
  %_local_5.4 = phi i32 [ 0, %brif_next323 ], [ %552, %loop_entry324 ]
  %_local_4.3 = phi i32 [ 0, %brif_next323 ], [ %553, %loop_entry324 ]
  %_local_3.13 = phi i32 [ %65, %brif_next323 ], [ %557, %loop_entry324 ]
  %_param_1.5 = phi i32 [ 0, %brif_next323 ], [ %555, %loop_entry324 ]
  %select_cond325.not = icmp eq i8 %546, 0
  %547 = select i1 %select_cond325.not, i32 %_local_5.4, i32 %_local_3.13
  %548 = inttoptr i32 %_local_3.13 to i32*
  %549 = load i32, i32* %548, align 4
  %550 = icmp eq i32 %549, -1
  %551 = zext i1 %550 to i32
  %552 = select i1 %550, i32 %547, i32 %_local_5.4
  %553 = add i32 %_local_4.3, %551
  %554 = select i1 %550, i32 0, i32 %549
  %555 = add i32 %554, %_param_1.5
  %calcOffset329 = add i32 %_local_3.13, 80
  %556 = inttoptr i32 %calcOffset329 to i32*
  %557 = load i32, i32* %556, align 4
  %brif_val330.not = icmp eq i32 %557, 0
  br i1 %brif_val330.not, label %brif_next331, label %loop_entry324

brif_next331:                                     ; preds = %loop_entry324
  %558 = icmp sgt i32 %555, 100
  br i1 %558, label %blk_exit32, label %blk_exit321

blk_exit32:                                       ; preds = %brif_next331
  %calcOffset763 = add i32 %stack_addr, -2880
  %559 = inttoptr i32 %calcOffset763 to i32*
  store i32 %555, i32* %559, align 4
  %560 = load i32, i32* null, align 2147483648
  %561 = call i32 @fprintf(i32 %560, i32 2122, i32 %calcOffset763)
  call void @exit(i32 1)
  unreachable

blk_exit19:                                       ; preds = %brif_next318, %blk_exit20, %blk_exit731, %blk_exit683
  %_local_5.0 = phi i32 [ 0, %blk_exit683 ], [ 0, %blk_exit731 ], [ 0, %blk_exit20 ], [ %575, %brif_next318 ]
  %562 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  call void @free_desc(i32 %562)
  %563 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  call void @free(i32 %563)
  call void @exit(i32 %_local_5.0)
  unreachable

brif_next316:                                     ; preds = %blk_exit35
  %564 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %565 = call i32 @maxlen_in_list(i32 %564)
  store i32 %565, i32* inttoptr (i32 2284 to i32*), align 4
  %566 = add i32 %565, 10
  %567 = call i32 @malloc(i32 %566)
  %568 = icmp eq i32 %567, 0
  br i1 %568, label %blk_exit33, label %brif_next318

blk_exit33:                                       ; preds = %brif_next316
  %calcOffset762 = add i32 %stack_addr, -2944
  %569 = inttoptr i32 %calcOffset762 to i32*
  store i32 1987, i32* %569, align 4
  %570 = load i32, i32* null, align 2147483648
  %571 = call i32 @fprintf(i32 %570, i32 1191, i32 %calcOffset762)
  call void @exit(i32 1)
  unreachable

brif_next318:                                     ; preds = %brif_next316
  store i32 %567, i32* inttoptr (i32 2288 to i32*), align 16
  %calcOffset319 = add i32 %stack_addr, -1024
  %572 = inttoptr i32 %calcOffset319 to i8*
  store i8 0, i8* %572, align 1
  %573 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  call void @matches_in_list(i32 %573, i32 %calcOffset319)
  %574 = load i8, i8* %572, align 1
  %575 = zext i8 %574 to i32
  call void @regfree(i32 2232)
  br label %blk_exit19

brif_next264:                                     ; preds = %blk_exit42
  %brif_val265.not.not = icmp eq i32 %_local_3.0, 0
  br i1 %brif_val265.not.not, label %blk_exit38, label %brif_next266

blk_exit38:                                       ; preds = %brif_next264
  %576 = call i32 @regcomp(i32 2232, i32 %_local_4.0, i32 8)
  %577 = icmp eq i32 %576, 0
  br i1 %577, label %blk_exit35, label %blk_exit37

blk_exit37:                                       ; preds = %blk_exit284, %blk_exit38
  %_local_4.1 = phi i32 [ %_local_4.0, %blk_exit38 ], [ %584, %blk_exit284 ]
  %calcOffset313 = add i32 %stack_addr, -2848
  %578 = inttoptr i32 %calcOffset313 to i32*
  store i32 %_local_4.1, i32* %578, align 4
  %579 = load i32, i32* null, align 2147483648
  %580 = call i32 @fprintf(i32 %579, i32 1742, i32 %calcOffset313)
  call void @exit(i32 1)
  unreachable

brif_next266:                                     ; preds = %brif_next264
  %581 = inttoptr i32 %_local_4.0 to i8*
  %582 = load i8, i8* %581, align 1
  %583 = icmp eq i8 %582, 0
  br i1 %583, label %blk_exit39, label %loop_entry275

blk_exit39:                                       ; preds = %brif_next277, %brif_next266
  %_param_1.0 = phi i32 [ 1, %brif_next266 ], [ %622, %brif_next277 ]
  %584 = call i32 @malloc(i32 %_param_1.0)
  %585 = icmp eq i32 %584, 0
  br i1 %585, label %blk_exit34, label %brif_next283

blk_exit34:                                       ; preds = %blk_exit39
  %calcOffset761 = add i32 %stack_addr, -2864
  %586 = inttoptr i32 %calcOffset761 to i32*
  store i32 1759, i32* %586, align 4
  %587 = load i32, i32* null, align 2147483648
  %588 = call i32 @fprintf(i32 %587, i32 1191, i32 %calcOffset761)
  call void @exit(i32 1)
  unreachable

brif_next283:                                     ; preds = %blk_exit39
  %589 = load i8, i8* %581, align 1
  %590 = icmp eq i8 %589, 0
  br i1 %590, label %blk_exit284, label %loop_entry288

blk_exit284:                                      ; preds = %blk_exit289, %brif_next283
  %_local_3.10 = phi i32 [ %584, %brif_next283 ], [ %_local_3.12, %blk_exit289 ]
  %591 = inttoptr i32 %_local_3.10 to i8*
  store i8 0, i8* %591, align 1
  %592 = call i32 @regcomp(i32 2232, i32 %584, i32 8)
  %593 = icmp eq i32 %592, 0
  br i1 %593, label %blk_exit36, label %blk_exit37

blk_exit36:                                       ; preds = %blk_exit284
  call void @free(i32 %584)
  br label %blk_exit35

loop_entry288:                                    ; preds = %blk_exit289, %brif_next283
  %_local_5.2.in = phi i8 [ %609, %blk_exit289 ], [ %589, %brif_next283 ]
  %_local_3.11 = phi i32 [ %_local_3.12, %blk_exit289 ], [ %584, %brif_next283 ]
  %_param_1.3.in = phi i32 [ %_param_1.3, %blk_exit289 ], [ %_local_4.0, %brif_next283 ]
  %_param_1.3 = add i32 %_param_1.3.in, 1
  %594 = sext i8 %_local_5.2.in to i32
  %595 = add nsw i32 %594, -123
  %596 = icmp ult i32 %595, -26
  br i1 %596, label %blk_exit290, label %brif_next292

blk_exit290:                                      ; preds = %loop_entry288
  %597 = add nsw i32 %594, -91
  %598 = icmp ult i32 %597, -26
  br i1 %598, label %blk_exit297, label %brif_next299

blk_exit297:                                      ; preds = %blk_exit290
  %599 = inttoptr i32 %_local_3.11 to i8*
  store i8 %_local_5.2.in, i8* %599, align 1
  %600 = add i32 %_local_3.11, 1
  br label %blk_exit289

brif_next299:                                     ; preds = %blk_exit290
  %calcOffset300 = add i32 %_local_3.11, 1
  %601 = inttoptr i32 %calcOffset300 to i8*
  store i8 %_local_5.2.in, i8* %601, align 1
  %602 = inttoptr i32 %_local_3.11 to i8*
  store i8 91, i8* %602, align 1
  %603 = call i32 @tolower(i32 %594)
  %calcOffset302 = add i32 %_local_3.11, 3
  %604 = inttoptr i32 %calcOffset302 to i8*
  store i8 93, i8* %604, align 1
  %calcOffset303 = add i32 %_local_3.11, 2
  %605 = inttoptr i32 %calcOffset303 to i8*
  %606 = trunc i32 %603 to i8
  store i8 %606, i8* %605, align 1
  %607 = add i32 %_local_3.11, 4
  br label %blk_exit289

blk_exit289:                                      ; preds = %brif_next292, %brif_next299, %blk_exit297
  %_local_3.12 = phi i32 [ %600, %blk_exit297 ], [ %607, %brif_next299 ], [ %616, %brif_next292 ]
  %608 = inttoptr i32 %_param_1.3 to i8*
  %609 = load i8, i8* %608, align 1
  %brif_val306.not = icmp eq i8 %609, 0
  br i1 %brif_val306.not, label %blk_exit284, label %loop_entry288

brif_next292:                                     ; preds = %loop_entry288
  %calcOffset293 = add i32 %_local_3.11, 1
  %610 = inttoptr i32 %calcOffset293 to i8*
  store i8 %_local_5.2.in, i8* %610, align 1
  %611 = inttoptr i32 %_local_3.11 to i8*
  store i8 91, i8* %611, align 1
  %612 = call i32 @toupper(i32 %594)
  %calcOffset295 = add i32 %_local_3.11, 3
  %613 = inttoptr i32 %calcOffset295 to i8*
  store i8 93, i8* %613, align 1
  %calcOffset296 = add i32 %_local_3.11, 2
  %614 = inttoptr i32 %calcOffset296 to i8*
  %615 = trunc i32 %612 to i8
  store i8 %615, i8* %614, align 1
  %616 = add i32 %_local_3.11, 4
  br label %blk_exit289

loop_entry275:                                    ; preds = %brif_next277, %brif_next266
  %_local_5.1 = phi i32 [ %622, %brif_next277 ], [ 1, %brif_next266 ]
  %_local_3.9.in = phi i8 [ %624, %brif_next277 ], [ %582, %brif_next266 ]
  %_param_0.4.in = phi i32 [ %_param_0.4, %brif_next277 ], [ %_local_4.0, %brif_next266 ]
  %_param_0.4 = add i32 %_param_0.4.in, 1
  %617 = or i8 %_local_3.9.in, 32
  %618 = sext i8 %617 to i32
  %619 = add nsw i32 %618, -97
  %620 = icmp ult i32 %619, 26
  %621 = select i1 %620, i32 4, i32 1
  %622 = add i32 %621, %_local_5.1
  %.not771 = icmp ult i32 %_local_5.1, %622
  br i1 %.not771, label %brif_next277, label %blk_exit274

brif_next277:                                     ; preds = %loop_entry275
  %623 = inttoptr i32 %_param_0.4 to i8*
  %624 = load i8, i8* %623, align 1
  %625 = icmp eq i8 %624, 0
  br i1 %625, label %blk_exit39, label %loop_entry275

blk_exit274:                                      ; preds = %loop_entry275
  %calcOffset281 = add i32 %stack_addr, -2832
  %626 = inttoptr i32 %calcOffset281 to i32*
  store i32 2075, i32* %626, align 4
  %627 = load i32, i32* null, align 2147483648
  %628 = call i32 @fprintf(i32 %627, i32 1191, i32 %calcOffset281)
  call void @exit(i32 1)
  unreachable

brif_next45:                                      ; preds = %blk_exit5
  %629 = icmp slt i32 %6, 1
  br i1 %629, label %blk_exit42, label %brif_next47

brif_next47:                                      ; preds = %brif_next45
  %630 = shl i32 %5, 2
  %631 = add i32 %630, %_arg_1
  br label %loop_entry51

loop_entry51:                                     ; preds = %blk_exit110, %brif_next47
  %_local_8.0 = phi i32 [ 0, %brif_next47 ], [ %691, %blk_exit110 ]
  %632 = shl i32 %_local_8.0, 2
  %633 = add i32 %631, %632
  %634 = inttoptr i32 %633 to i32*
  %635 = load i32, i32* %634, align 4
  %636 = inttoptr i32 %635 to i8*
  %637 = load i8, i8* %636, align 1
  %638 = sext i8 %637 to i32
  %639 = add nsw i32 %638, -48
  %640 = icmp ult i32 %639, 10
  br i1 %640, label %blk_exit53, label %blk_exit52

blk_exit53:                                       ; preds = %loop_entry51
  %641 = add i32 %635, 1
  br label %loop_entry58

loop_entry58:                                     ; preds = %brif_next60, %blk_exit53
  %_local_3.1 = phi i32 [ %638, %blk_exit53 ], [ %653, %brif_next60 ]
  %_param_1.1 = phi i32 [ 0, %blk_exit53 ], [ %650, %brif_next60 ]
  %_param_0.1 = phi i32 [ %641, %blk_exit53 ], [ %654, %brif_next60 ]
  %642 = mul i32 %_param_1.1, 10
  %643 = add i32 %642, %_local_3.1
  %644 = icmp sgt i32 %643, 148
  br i1 %644, label %blk_exit50, label %brif_next60

blk_exit50:                                       ; preds = %loop_entry58
  %645 = load i32, i32* null, align 2147483648
  %646 = call i32 @fprintf(i32 %645, i32 1914, i32 0)
  %647 = load i32, i32* %634, align 4
  %calcOffset216 = add i32 %stack_addr, -2816
  %648 = inttoptr i32 %calcOffset216 to i32*
  store i32 %647, i32* %648, align 4
  %649 = call i32 @fprintf(i32 %645, i32 2159, i32 %calcOffset216)
  br label %blk_exit49

blk_exit49:                                       ; preds = %brif_next66, %blk_exit69, %brif_next78, %blk_exit50
  store i8 1, i8* inttoptr (i32 2296 to i8*), align 8
  br label %blk_exit41

brif_next60:                                      ; preds = %loop_entry58
  %650 = add i32 %643, -48
  %651 = inttoptr i32 %_param_0.1 to i8*
  %652 = load i8, i8* %651, align 1
  %653 = sext i8 %652 to i32
  %654 = add i32 %_param_0.1, 1
  %655 = add nsw i32 %653, -48
  %656 = icmp ult i32 %655, 10
  br i1 %656, label %loop_entry58, label %brif_next63

brif_next63:                                      ; preds = %brif_next60
  %657 = icmp sgt i32 %643, 47
  br i1 %657, label %blk_exit64, label %brif_next66

blk_exit64:                                       ; preds = %brif_next63
  switch i8 %652, label %blk_exit52 [
    i8 37, label %blk_exit68
    i8 46, label %blk_exit69
  ]

blk_exit68:                                       ; preds = %blk_exit64
  %658 = inttoptr i32 %654 to i8*
  %659 = load i8, i8* %658, align 1
  %660 = icmp eq i8 %659, 0
  br i1 %660, label %blk_exit72, label %blk_exit52

blk_exit72:                                       ; preds = %blk_exit68
  %661 = add i32 %_local_8.0, 1
  %662 = icmp slt i32 %661, %6
  br i1 %662, label %blk_exit76, label %brif_next78

blk_exit76:                                       ; preds = %blk_exit72
  %663 = shl i32 %661, 2
  %664 = add i32 %631, %663
  %665 = inttoptr i32 %664 to i32*
  %666 = load i32, i32* %665, align 4
  br label %blk_exit52

brif_next78:                                      ; preds = %blk_exit72
  %calcOffset79 = add i32 %stack_addr, -2608
  %667 = inttoptr i32 %calcOffset79 to i32*
  store i32 1710, i32* %667, align 4
  %668 = load i32, i32* null, align 2147483648
  %669 = call i32 @fprintf(i32 %668, i32 1191, i32 %calcOffset79)
  br label %blk_exit49

blk_exit69:                                       ; preds = %blk_exit64
  %calcOffset70 = add i32 %stack_addr, -2624
  %670 = inttoptr i32 %calcOffset70 to i32*
  store i32 1625, i32* %670, align 4
  %671 = load i32, i32* null, align 2147483648
  %672 = call i32 @fprintf(i32 %671, i32 1191, i32 %calcOffset70)
  br label %blk_exit49

brif_next66:                                      ; preds = %brif_next63
  %calcOffset67 = add i32 %stack_addr, -2800
  %673 = inttoptr i32 %calcOffset67 to i32*
  store i32 %635, i32* %673, align 4
  %674 = load i32, i32* null, align 2147483648
  %675 = call i32 @fprintf(i32 %674, i32 2159, i32 %calcOffset67)
  br label %blk_exit49

blk_exit52:                                       ; preds = %blk_exit76, %blk_exit68, %blk_exit64, %loop_entry51
  %_local_11.0 = phi i32 [ -1, %blk_exit64 ], [ -1, %loop_entry51 ], [ %650, %blk_exit76 ], [ %650, %blk_exit68 ]
  %_local_10.0 = phi i32 [ %635, %blk_exit64 ], [ %635, %loop_entry51 ], [ %666, %blk_exit76 ], [ %654, %blk_exit68 ]
  %_local_8.1 = phi i32 [ %_local_8.0, %blk_exit64 ], [ %_local_8.0, %loop_entry51 ], [ %661, %blk_exit76 ], [ %_local_8.0, %blk_exit68 ]
  %676 = call i32 @strlen(i32 %_local_10.0)
  %677 = icmp ult i32 %676, 3
  br i1 %677, label %blk_exit81, label %brif_next83

blk_exit81:                                       ; preds = %brif_next89, %brif_next86, %brif_next83, %blk_exit52
  %678 = phi i1 [ false, %brif_next89 ], [ true, %brif_next86 ], [ true, %brif_next83 ], [ true, %blk_exit52 ]
  %_local_3.2 = phi i32 [ 1282, %brif_next89 ], [ 1121, %brif_next86 ], [ 1121, %brif_next83 ], [ 1121, %blk_exit52 ]
  %_param_1.2 = phi i32 [ 1246, %brif_next89 ], [ 1089, %brif_next86 ], [ 1089, %brif_next83 ], [ 1089, %blk_exit52 ]
  %679 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val93.not = icmp eq i32 %679, 0
  br i1 %brif_val93.not, label %brif_next94, label %blk_exit92

brif_next94:                                      ; preds = %blk_exit81
  %calcOffset95 = add i32 %stack_addr, -2704
  %680 = inttoptr i32 %calcOffset95 to i32*
  store i32 %_param_1.2, i32* %680, align 4
  %681 = add i32 %stack_addr, -1536
  %682 = call i32 @snprintf(i32 %681, i32 512, i32 1191, i32 %calcOffset95)
  %calcOffset96 = add i32 %stack_addr, -2720
  %683 = inttoptr i32 %calcOffset96 to i32*
  store i32 %_local_3.2, i32* %683, align 4
  %684 = add i32 %stack_addr, -2048
  %685 = call i32 @snprintf(i32 %684, i32 512, i32 1191, i32 %calcOffset96)
  br label %blk_exit91

blk_exit91:                                       ; preds = %brif_next103, %blk_exit97, %brif_next94
  %686 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %687 = icmp eq i32 %686, 0
  br i1 %687, label %blk_exit111, label %brif_next113

blk_exit111:                                      ; preds = %blk_exit91
  %688 = add i32 %stack_addr, -1536
  %689 = call i32 @add_file(i32 %_local_11.0, i32 %688, i32 0, i32 2268, i32 3328, i32 0)
  %690 = icmp eq i32 %689, 0
  br i1 %690, label %blk_exit41, label %blk_exit110

blk_exit110:                                      ; preds = %brif_next183, %blk_exit157, %blk_exit203, %blk_exit111
  %691 = add i32 %_local_8.1, 1
  %692 = icmp slt i32 %691, %6
  br i1 %692, label %loop_entry51, label %blk_exit42

brif_next113:                                     ; preds = %blk_exit91
  %693 = add i32 %stack_addr, -2560
  %694 = call i32 @strncpy(i32 %693, i32 %686, i32 512)
  %calcOffset114 = add i32 %stack_addr, -2049
  %695 = inttoptr i32 %calcOffset114 to i8*
  store i8 0, i8* %695, align 1
  br label %loop_entry117

loop_entry117:                                    ; preds = %blk_exit134, %brif_next113
  %_local_3.3 = phi i32 [ %693, %brif_next113 ], [ %storemerge799, %blk_exit134 ]
  %696 = inttoptr i32 %_local_3.3 to i8*
  %697 = load i8, i8* %696, align 1
  %brif_val120.not = icmp eq i8 %697, 0
  br i1 %brif_val120.not, label %brif_next121, label %blk_exit118

brif_next121:                                     ; preds = %loop_entry117
  %calcOffset122 = add i32 %stack_addr, -2564
  %698 = add i32 %stack_addr, -2576
  %699 = inttoptr i32 %698 to i8*
  call void @llvm.memset.p0i8.i64(i8* %699, i8 0, i64 16, i1 true)
  br label %blk_exit116

blk_exit116:                                      ; preds = %blk_exit145, %brif_next121
  %.pre-phi = phi i32 [ %769, %blk_exit145 ], [ %calcOffset122, %brif_next121 ]
  %.pre-phi810 = add i32 %stack_addr, -2572
  %700 = add i32 %stack_addr, -1536
  %701 = call i32 @add_file(i32 100, i32 %700, i32 0, i32 %.pre-phi, i32 %.pre-phi810, i32 0)
  br label %blk_exit115

blk_exit115:                                      ; preds = %brif_next136, %blk_exit145, %blk_exit116
  %_local_12.0 = phi i32 [ %701, %blk_exit116 ], [ %776, %blk_exit145 ], [ 1, %brif_next136 ]
  %702 = add i32 %stack_addr, -1536
  %703 = add i32 %stack_addr, -2048
  %704 = call i32 @strncmp(i32 %702, i32 %703, i32 512)
  %705 = icmp eq i32 %704, 0
  br i1 %705, label %blk_exit160, label %brif_next162

blk_exit160:                                      ; preds = %brif_next162, %blk_exit115
  %calcOffset166 = add i32 %stack_addr, -2572
  %706 = inttoptr i32 %calcOffset166 to i32*
  %707 = load i32, i32* %706, align 4
  %708 = icmp eq i32 %707, 0
  br i1 %708, label %blk_exit156, label %brif_next168

brif_next168:                                     ; preds = %blk_exit160
  %709 = inttoptr i32 %707 to i32*
  store i32 %_local_11.0, i32* %709, align 4
  %710 = load i32, i32* inttoptr (i32 3328 to i32*), align 256
  %711 = icmp eq i32 %710, 0
  br i1 %711, label %blk_exit158, label %brif_next171

blk_exit158:                                      ; preds = %brif_next168
  store i32 %707, i32* inttoptr (i32 3328 to i32*), align 256
  %calcOffset200 = add i32 %stack_addr, -2564
  %712 = inttoptr i32 %calcOffset200 to i32*
  %713 = load i32, i32* %712, align 4
  store i32 %713, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit157

brif_next171:                                     ; preds = %brif_next168
  %calcOffset172 = add i32 %stack_addr, -2564
  %714 = inttoptr i32 %calcOffset172 to i32*
  %715 = load i32, i32* %714, align 4
  %calcOffset173 = add i32 %710, 76
  %716 = inttoptr i32 %calcOffset173 to i32*
  store i32 %715, i32* %716, align 4
  store i32 %707, i32* inttoptr (i32 3328 to i32*), align 256
  %717 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %calcOffset174 = add i32 %715, 80
  %718 = inttoptr i32 %calcOffset174 to i32*
  store i32 %717, i32* %718, align 4
  br label %blk_exit157

blk_exit156:                                      ; preds = %brif_next178, %blk_exit157, %blk_exit160
  br i1 %678, label %blk_exit203, label %brif_next205

blk_exit203:                                      ; preds = %brif_next205, %blk_exit156
  %719 = call i32 @getenv(i32 1518)
  %calcOffset207 = add i32 %stack_addr, -2780
  %720 = inttoptr i32 %calcOffset207 to i32*
  store i32 %_local_10.0, i32* %720, align 4
  %calcOffset208 = add i32 %stack_addr, -2784
  %721 = inttoptr i32 %calcOffset208 to i32*
  store i32 %719, i32* %721, align 4
  %722 = call i32 @snprintf(i32 %703, i32 512, i32 1174, i32 %calcOffset208)
  %723 = call i32 @add_file(i32 %_local_11.0, i32 %703, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val209.not = icmp eq i32 %723, 0
  br i1 %brif_val209.not, label %blk_exit41, label %blk_exit110

brif_next205:                                     ; preds = %blk_exit156
  %724 = add i32 %676, %_local_10.0
  %725 = add i32 %724, -2
  %726 = inttoptr i32 %725 to i8*
  store i8 45, i8* %726, align 1
  br label %blk_exit203

blk_exit157:                                      ; preds = %brif_next176, %blk_exit159, %brif_next171, %blk_exit158
  %brif_val201.not = icmp eq i32 %_local_12.0, 0
  br i1 %brif_val201.not, label %blk_exit156, label %blk_exit110

brif_next162:                                     ; preds = %blk_exit115
  %727 = add i32 %stack_addr, -2568
  %728 = add i32 %stack_addr, -2576
  %729 = call i32 @add_file(i32 100, i32 %703, i32 0, i32 %727, i32 %728, i32 0)
  %730 = inttoptr i32 %727 to i32*
  %731 = load i32, i32* %730, align 4
  %brif_val164.not = icmp eq i32 %731, 0
  br i1 %brif_val164.not, label %blk_exit160, label %blk_exit159

blk_exit159:                                      ; preds = %brif_next162
  %732 = call i32 @strncmp(i32 %702, i32 %703, i32 512)
  %733 = icmp eq i32 %732, 0
  br i1 %733, label %blk_exit157, label %brif_next176

brif_next176:                                     ; preds = %blk_exit159
  %734 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val177.not = icmp eq i32 %734, 0
  br i1 %brif_val177.not, label %brif_next178, label %blk_exit157

brif_next178:                                     ; preds = %brif_next176
  %calcOffset179 = add i32 %stack_addr, -2564
  %735 = inttoptr i32 %calcOffset179 to i32*
  %736 = load i32, i32* %735, align 4
  %737 = icmp eq i32 %736, 0
  br i1 %737, label %blk_exit156, label %brif_next181

brif_next181:                                     ; preds = %brif_next178
  %738 = call i32 @malloc(i32 84)
  %739 = icmp eq i32 %738, 0
  br i1 %739, label %blk_exit48, label %brif_next183

blk_exit48:                                       ; preds = %brif_next181
  %calcOffset217 = add i32 %stack_addr, -2768
  %740 = inttoptr i32 %calcOffset217 to i32*
  store i32 1987, i32* %740, align 4
  %741 = load i32, i32* null, align 2147483648
  %742 = call i32 @fprintf(i32 %741, i32 1191, i32 %calcOffset217)
  call void @exit(i32 1)
  unreachable

brif_next183:                                     ; preds = %brif_next181
  %743 = add i32 %738, 12
  %744 = inttoptr i32 %743 to i8*
  call void @llvm.memset.p0i8.i64(i8* %744, i8 -1, i64 8, i1 true)
  %745 = add i32 %738, 4
  %746 = inttoptr i32 %745 to i8*
  call void @llvm.memset.p0i8.i64(i8* %746, i8 -1, i64 8, i1 true)
  %747 = add i32 %738, 40
  %748 = inttoptr i32 %747 to i8*
  call void @llvm.memset.p0i8.i64(i8* %748, i8 0, i64 24, i1 true)
  %749 = add i32 %738, 20
  %750 = inttoptr i32 %749 to i8*
  call void @llvm.memset.p0i8.i64(i8* %750, i8 0, i64 8, i1 true)
  %751 = add i32 %738, 72
  %752 = inttoptr i32 %751 to i8*
  call void @llvm.memset.p0i8.i64(i8* %752, i8 0, i64 12, i1 true)
  %calcOffset192 = add i32 %736, 72
  %753 = inttoptr i32 %calcOffset192 to i32*
  store i32 %738, i32* %753, align 4
  %754 = add i32 %738, 28
  %755 = inttoptr i32 %754 to i8*
  call void @llvm.memset.p0i8.i64(i8* %755, i8 0, i64 9, i1 true)
  %756 = inttoptr i32 %738 to i32*
  store i32 %_local_11.0, i32* %756, align 4
  %calcOffset196 = add i32 %738, 68
  %757 = inttoptr i32 %calcOffset196 to i32*
  store i32 %736, i32* %757, align 4
  %calcOffset197 = add i32 %736, 76
  %758 = inttoptr i32 %calcOffset197 to i32*
  store i32 %731, i32* %758, align 4
  %calcOffset198 = add i32 %731, 80
  %759 = inttoptr i32 %calcOffset198 to i32*
  store i32 %736, i32* %759, align 4
  %calcOffset199 = add i32 %731, 72
  %760 = inttoptr i32 %calcOffset199 to i32*
  store i32 %738, i32* %760, align 4
  store i32 %738, i32* inttoptr (i32 2268 to i32*), align 4
  store i32 %738, i32* inttoptr (i32 3328 to i32*), align 256
  br label %blk_exit110

blk_exit118:                                      ; preds = %loop_entry117
  %761 = call i32 @strchr(i32 %_local_3.3, i32 58)
  %brif_val128.not = icmp eq i32 %761, 0
  br i1 %brif_val128.not, label %blk_exit126, label %blk_exit127

blk_exit126:                                      ; preds = %blk_exit127, %blk_exit118
  %storemerge799 = phi i32 [ %782, %blk_exit127 ], [ 0, %blk_exit118 ]
  %calcOffset131 = add i32 %stack_addr, -2728
  %762 = inttoptr i32 %calcOffset131 to i32*
  store i32 %_local_10.0, i32* %762, align 4
  %calcOffset132 = add i32 %stack_addr, -2732
  %763 = inttoptr i32 %calcOffset132 to i32*
  store i32 %_local_3.3, i32* %763, align 4
  %calcOffset133 = add i32 %stack_addr, -2736
  %764 = inttoptr i32 %calcOffset133 to i32*
  store i32 %_param_1.2, i32* %764, align 4
  %765 = add i32 %stack_addr, -1024
  %766 = call i32 @snprintf(i32 %765, i32 1024, i32 1171, i32 %calcOffset133)
  %767 = call i32 @add_file(i32 %_local_11.0, i32 %765, i32 0, i32 2268, i32 3328, i32 0)
  %768 = icmp eq i32 %767, 0
  br i1 %768, label %blk_exit134, label %brif_next136

blk_exit134:                                      ; preds = %blk_exit126
  %769 = add i32 %stack_addr, -2564
  %770 = call i32 @strncpy(i32 %769, i32 %_local_3.3, i32 2)
  %calcOffset141 = add i32 %stack_addr, -2752
  %771 = inttoptr i32 %calcOffset141 to i32*
  store i32 %_param_1.2, i32* %771, align 4
  %calcOffset142 = add i32 %stack_addr, -2562
  %772 = inttoptr i32 %calcOffset142 to i8*
  store i8 0, i8* %772, align 1
  %calcOffset143 = add i32 %stack_addr, -2744
  %773 = inttoptr i32 %calcOffset143 to i32*
  store i32 %_local_10.0, i32* %773, align 4
  %calcOffset144 = add i32 %stack_addr, -2748
  %774 = inttoptr i32 %calcOffset144 to i32*
  store i32 %769, i32* %774, align 4
  %775 = call i32 @snprintf(i32 %765, i32 1024, i32 1171, i32 %calcOffset141)
  %776 = call i32 @add_file(i32 %_local_11.0, i32 %765, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val146.not = icmp eq i32 %776, 0
  %brif_val146.not.not = xor i1 %brif_val146.not, true
  %brif_val148.not = icmp eq i32 %storemerge799, 0
  %or.cond814 = select i1 %brif_val146.not.not, i1 true, i1 %brif_val148.not
  br i1 %or.cond814, label %blk_exit145, label %loop_entry117

blk_exit145:                                      ; preds = %blk_exit134
  %777 = add i32 %stack_addr, -2576
  %778 = inttoptr i32 %777 to i8*
  call void @llvm.memset.p0i8.i64(i8* %778, i8 0, i64 16, i1 true)
  br i1 %brif_val146.not, label %blk_exit116, label %blk_exit115

brif_next136:                                     ; preds = %blk_exit126
  %779 = add i32 %stack_addr, -2576
  %780 = inttoptr i32 %779 to i8*
  call void @llvm.memset.p0i8.i64(i8* %780, i8 0, i64 16, i1 true)
  br label %blk_exit115

blk_exit127:                                      ; preds = %blk_exit118
  %781 = inttoptr i32 %761 to i8*
  store i8 0, i8* %781, align 1
  %782 = add i32 %761, 1
  br label %blk_exit126

blk_exit92:                                       ; preds = %blk_exit81
  %783 = call i32 @strncmp(i32 %_local_10.0, i32 1559, i32 1)
  %784 = icmp eq i32 %783, 0
  br i1 %784, label %blk_exit97, label %brif_next99

blk_exit97:                                       ; preds = %brif_next101, %brif_next99, %blk_exit92
  %calcOffset108 = add i32 %stack_addr, -2672
  %785 = inttoptr i32 %calcOffset108 to i32*
  store i32 %_local_10.0, i32* %785, align 4
  %786 = add i32 %stack_addr, -1536
  %787 = call i32 @snprintf(i32 %786, i32 512, i32 1191, i32 %calcOffset108)
  %calcOffset109 = add i32 %stack_addr, -2688
  %788 = inttoptr i32 %calcOffset109 to i32*
  store i32 %_local_10.0, i32* %788, align 4
  %789 = add i32 %stack_addr, -2048
  %790 = call i32 @snprintf(i32 %789, i32 512, i32 1191, i32 %calcOffset109)
  br label %blk_exit91

brif_next99:                                      ; preds = %blk_exit92
  %791 = call i32 @strncmp(i32 %_local_10.0, i32 1558, i32 2)
  %792 = icmp eq i32 %791, 0
  br i1 %792, label %blk_exit97, label %brif_next101

brif_next101:                                     ; preds = %brif_next99
  %793 = call i32 @strncmp(i32 %_local_10.0, i32 1557, i32 3)
  %794 = icmp eq i32 %793, 0
  br i1 %794, label %blk_exit97, label %brif_next103

brif_next103:                                     ; preds = %brif_next101
  %calcOffset104 = add i32 %stack_addr, -2636
  %795 = inttoptr i32 %calcOffset104 to i32*
  store i32 %_local_10.0, i32* %795, align 4
  %calcOffset105 = add i32 %stack_addr, -2640
  %796 = inttoptr i32 %calcOffset105 to i32*
  store i32 %_param_1.2, i32* %796, align 4
  %797 = add i32 %stack_addr, -1536
  %798 = call i32 @snprintf(i32 %797, i32 512, i32 1174, i32 %calcOffset105)
  %calcOffset106 = add i32 %stack_addr, -2652
  %799 = inttoptr i32 %calcOffset106 to i32*
  store i32 %_local_10.0, i32* %799, align 4
  %calcOffset107 = add i32 %stack_addr, -2656
  %800 = inttoptr i32 %calcOffset107 to i32*
  store i32 %_local_3.2, i32* %800, align 4
  %801 = add i32 %stack_addr, -2048
  %802 = call i32 @snprintf(i32 %801, i32 512, i32 1174, i32 %calcOffset107)
  br label %blk_exit91

brif_next83:                                      ; preds = %blk_exit52
  %803 = add i32 %676, %_local_10.0
  %804 = add i32 %803, -2
  %805 = inttoptr i32 %804 to i8*
  %806 = load i8, i8* %805, align 1
  %.not = icmp eq i8 %806, 45
  br i1 %.not, label %brif_next86, label %blk_exit81

brif_next86:                                      ; preds = %brif_next83
  %807 = add i32 %803, -1
  %808 = inttoptr i32 %807 to i8*
  %809 = load i8, i8* %808, align 1
  %.not770 = icmp eq i8 %809, 111
  br i1 %.not770, label %brif_next89, label %blk_exit81

brif_next89:                                      ; preds = %brif_next86
  store i8 0, i8* %805, align 1
  br label %blk_exit81

blk_exit11:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2280 to i8*), align 8
  br label %loop_entry

blk_exit8:                                        ; preds = %loop_entry
  call void @program_version()
  %calcOffset = add i32 %stack_addr, -2592
  %810 = inttoptr i32 %calcOffset to i32*
  store i32 2304, i32* %810, align 4
  %811 = call i32 @printf(i32 1755, i32 %calcOffset)
  call void @exit(i32 0)
  unreachable

blk_exit9:                                        ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2294 to i8*), align 2
  br label %loop_entry

blk_exit12:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2273 to i8*), align 1
  store i8 0, i8* inttoptr (i32 2274 to i8*), align 2
  br label %loop_entry

blk_exit13:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2293 to i8*), align 1
  br label %loop_entry

blk_exit14:                                       ; preds = %loop_entry
  %812 = load i32, i32* null, align 2147483648
  %813 = call i32 @atoi(i32 %812)
  store i32 %813, i32* inttoptr (i32 2208 to i32*), align 32
  br label %loop_entry

blk_exit10:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2228 to i8*), align 4
  %814 = load i32, i32* null, align 2147483648
  br label %loop_entry

blk_exit15:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2274 to i8*), align 2
  store i8 0, i8* inttoptr (i32 2273 to i8*), align 1
  br label %loop_entry

blk_exit16:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2264 to i8*), align 8
  br label %loop_entry

blk_exit17:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2272 to i8*), align 32
  br label %loop_entry

blk_exit7:                                        ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2295 to i8*), align 1
  br label %loop_entry

blk_exit18:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2292 to i8*), align 4
  br label %loop_entry
}

; Function Attrs: null_pointer_is_valid
define internal void @program_version() #0 {
allocator:
  %stack = alloca [16 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [16 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -16
  %calcOffset = add i32 %stack_addr, -12
  %0 = inttoptr i32 %calcOffset to i32*
  store i32 1550, i32* %0, align 4
  %1 = inttoptr i32 %stack_end to i32*
  store i32 1324, i32* %1, align 4
  %2 = call i32 @snprintf(i32 2304, i32 1024, i32 1180, i32 %stack_end)
  ret void
}

; Function Attrs: null_pointer_is_valid
define internal void @usage() #0 {
allocator:
  %stack = alloca [144 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [144 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -144
  call void @program_version()
  %calcOffset = add i32 %stack_addr, -16
  %0 = inttoptr i32 %calcOffset to i32*
  store i32 2304, i32* %0, align 4
  %1 = load i32, i32* null, align 2147483648
  %2 = call i32 @fprintf(i32 %1, i32 1755, i32 %calcOffset)
  %calcOffset1 = add i32 %stack_addr, -32
  %3 = inttoptr i32 %calcOffset1 to i32*
  store i32 1417, i32* %3, align 4
  %4 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset1)
  %calcOffset2 = add i32 %stack_addr, -48
  %5 = inttoptr i32 %calcOffset2 to i32*
  store i32 1322, i32* %5, align 4
  %6 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset2)
  %calcOffset3 = add i32 %stack_addr, -64
  %7 = inttoptr i32 %calcOffset3 to i32*
  store i32 1242, i32* %7, align 4
  %8 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset3)
  %calcOffset4 = add i32 %stack_addr, -80
  %9 = inttoptr i32 %calcOffset4 to i32*
  store i32 1240, i32* %9, align 4
  %10 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset4)
  %calcOffset5 = add i32 %stack_addr, -96
  %11 = inttoptr i32 %calcOffset5 to i32*
  store i32 1198, i32* %11, align 4
  %12 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset5)
  %calcOffset6 = add i32 %stack_addr, -112
  %13 = inttoptr i32 %calcOffset6 to i32*
  store i32 1429, i32* %13, align 4
  %14 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset6)
  %calcOffset7 = add i32 %stack_addr, -128
  %15 = inttoptr i32 %calcOffset7 to i32*
  store i32 1433, i32* %15, align 4
  %16 = call i32 @fprintf(i32 %1, i32 1191, i32 %calcOffset7)
  %17 = inttoptr i32 %stack_end to i32*
  store i32 1873, i32* %17, align 4
  %18 = call i32 @fprintf(i32 %1, i32 1191, i32 %stack_end)
  call void @exit(i32 1)
  unreachable
}

; Function Attrs: null_pointer_is_valid
define internal i32 @add_file(i32 %_arg_0, i32 %_arg_1, i32 %_arg_2, i32 %_arg_3, i32 %_arg_4, i32 %_arg_5) #0 {
allocator:
  %stack = alloca [1696 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [1696 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -1696
  %brif_val.not = icmp eq i32 %_arg_2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

brif_next:                                        ; preds = %allocator
  %0 = call i32 @strdup(i32 %_arg_1)
  br label %blk_exit6

blk_exit6:                                        ; preds = %brif_next9, %brif_next
  %_local_7.0 = phi i32 [ %0, %brif_next ], [ %200, %brif_next9 ]
  %1 = inttoptr i32 %_local_7.0 to i8*
  %2 = load i8, i8* %1, align 1
  %.not = icmp eq i8 %2, 47
  br i1 %.not, label %brif_next15, label %blk_exit6.blk_exit12_crit_edge

brif_next15:                                      ; preds = %blk_exit6
  %3 = add i32 %stack_addr, -1536
  %4 = call i32 @stat(i32 %_local_7.0, i32 %3)
  %5 = icmp eq i32 %4, 0
  br i1 %5, label %blk_exit12, label %brif_next17

brif_next17:                                      ; preds = %brif_next15
  %6 = load i32, i32* null, align 2147483648
  switch i32 %6, label %blk_exit18 [
    i32 44, label %blk_exit11
    i32 54, label %blk_exit11
  ]

blk_exit18:                                       ; preds = %brif_next17
  call void @perror(i32 1041)
  call void @exit(i32 1)
  unreachable

blk_exit12:                                       ; preds = %blk_exit6.blk_exit12_crit_edge, %brif_next15
  %.pre-phi230 = phi i32 [ %.pre, %blk_exit6.blk_exit12_crit_edge ], [ %3, %brif_next15 ]
  %7 = icmp ne i32 %_arg_5, 0
  %8 = zext i1 %7 to i32
  %9 = call i32 @stat(i32 %_local_7.0, i32 %.pre-phi230)
  %10 = xor i32 %9, -1
  %.lobit.not = lshr i32 %10, 31
  %calcOffset19 = add i32 %stack_addr, -1512
  %11 = inttoptr i32 %calcOffset19 to i32*
  %12 = load i32, i32* %11, align 4
  %13 = and i32 %12, 57344
  %14 = icmp eq i32 %13, 16384
  %15 = zext i1 %14 to i32
  %16 = and i32 %.lobit.not, %15
  %17 = and i32 %16, %8
  %brif_val20.not = icmp eq i32 %17, 0
  %brif_val22.not = icmp sgt i32 %9, -1
  %or.cond = select i1 %brif_val20.not, i1 %brif_val22.not, i1 false
  br i1 %or.cond, label %brif_next23, label %blk_exit11

brif_next23:                                      ; preds = %blk_exit12
  %18 = call i32 @open(i32 %_local_7.0, i32 67108864, i32 0)
  %brif_val26.not = icmp sgt i32 %18, -1
  br i1 %brif_val26.not, label %brif_next27, label %blk_exit25

brif_next27:                                      ; preds = %brif_next23
  %19 = load i8, i8* %1, align 1
  %20 = icmp eq i8 %19, 47
  br i1 %20, label %blk_exit24, label %blk_exit25

blk_exit24:                                       ; preds = %brif_next27
  %21 = call i32 @malloc(i32 84)
  %22 = icmp eq i32 %21, 0
  br i1 %22, label %blk_exit4, label %brif_next61

blk_exit4:                                        ; preds = %blk_exit24
  %calcOffset219 = add i32 %stack_addr, -1664
  %23 = inttoptr i32 %calcOffset219 to i32*
  store i32 1987, i32* %23, align 4
  %24 = load i32, i32* null, align 2147483648
  %25 = call i32 @fprintf(i32 %24, i32 1191, i32 %calcOffset219)
  call void @exit(i32 1)
  unreachable

brif_next61:                                      ; preds = %blk_exit24
  %26 = add i32 %21, 40
  %27 = inttoptr i32 %26 to i8*
  call void @llvm.memset.p0i8.i64(i8* %27, i8 0, i64 24, i1 true)
  %calcOffset65 = add i32 %21, 36
  %28 = inttoptr i32 %calcOffset65 to i8*
  store i8 0, i8* %28, align 1
  %calcOffset66 = add i32 %21, 16
  %29 = inttoptr i32 %calcOffset66 to i32*
  store i32 0, i32* %29, align 4
  %30 = add i32 %21, 8
  %31 = inttoptr i32 %30 to i8*
  call void @llvm.memset.p0i8.i64(i8* %31, i8 -1, i64 8, i1 true)
  %calcOffset68 = add i32 %21, 76
  %32 = inttoptr i32 %calcOffset68 to i8*
  call void @llvm.memset.p0i8.i64(i8* %32, i8 0, i64 8, i1 true)
  %calcOffset69 = add i32 %21, 20
  %calcOffset70 = add i32 %21, 68
  %33 = inttoptr i32 %calcOffset70 to i8*
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 8, i1 true)
  %calcOffset71 = add i32 %21, 28
  %34 = inttoptr i32 %calcOffset69 to i8*
  call void @llvm.memset.p0i8.i64(i8* %34, i8 0, i64 16, i1 true)
  %calcOffset72 = add i32 %21, 4
  %35 = inttoptr i32 %calcOffset72 to i32*
  store i32 %18, i32* %35, align 4
  %36 = inttoptr i32 %21 to i32*
  store i32 %_arg_0, i32* %36, align 4
  %37 = call i32 @strdup(i32 %_arg_1)
  %38 = inttoptr i32 %calcOffset69 to i32*
  store i32 %37, i32* %38, align 4
  %39 = call i32 @strdup(i32 %_local_7.0)
  %calcOffset75 = add i32 %21, 38
  %40 = inttoptr i32 %calcOffset75 to i8*
  store i8 0, i8* %40, align 1
  %calcOffset76 = add i32 %21, 24
  %41 = inttoptr i32 %calcOffset76 to i32*
  store i32 %39, i32* %41, align 4
  %42 = call i32 @strlen(i32 %_local_7.0)
  %43 = add i32 %42, 6
  %44 = call i32 @malloc(i32 %43)
  %45 = icmp eq i32 %44, 0
  br i1 %45, label %blk_exit3, label %brif_next78

blk_exit3:                                        ; preds = %brif_next61
  %calcOffset220 = add i32 %stack_addr, -1648
  %46 = inttoptr i32 %calcOffset220 to i32*
  store i32 1987, i32* %46, align 4
  %47 = load i32, i32* null, align 2147483648
  %48 = call i32 @fprintf(i32 %47, i32 1191, i32 %calcOffset220)
  call void @exit(i32 1)
  unreachable

brif_next78:                                      ; preds = %brif_next61
  %calcOffset79 = add i32 %stack_addr, -1568
  %49 = inttoptr i32 %calcOffset79 to i32*
  store i32 %_local_7.0, i32* %49, align 4
  %50 = add i32 %42, 5
  %51 = call i32 @snprintf(i32 %44, i32 %50, i32 1544, i32 %calcOffset79)
  %52 = call i32 @stat(i32 %44, i32 %.pre-phi230)
  %brif_val81.not = icmp eq i32 %52, 0
  br i1 %brif_val81.not, label %brif_next82, label %blk_exit80

brif_next82:                                      ; preds = %brif_next78
  store i8 1, i8* %40, align 1
  br label %blk_exit80

blk_exit80:                                       ; preds = %brif_next82, %brif_next78
  call void @free(i32 %44)
  %calcOffset84 = add i32 %21, 72
  %53 = inttoptr i32 %calcOffset84 to i32*
  store i32 %_arg_5, i32* %53, align 4
  %brif_val91.not = icmp slt i32 %9, 0
  %or.cond229 = select i1 %brif_val91.not, i1 true, i1 %14
  br i1 %or.cond229, label %blk_exit90, label %blk_exit89

blk_exit90:                                       ; preds = %blk_exit80
  %54 = load i32, i32* %35, align 4
  %55 = call i32 @close(i32 %54)
  store i32 -1, i32* %35, align 4
  %56 = load i32, i32* %41, align 4
  %57 = call i32 @opendir(i32 %56)
  %brif_val99.not = icmp eq i32 %57, 0
  br i1 %brif_val99.not, label %brif_next100, label %blk_exit97

brif_next100:                                     ; preds = %blk_exit90
  %58 = load i32, i32* %41, align 4
  call void @perror(i32 %58)
  br label %blk_exit88

blk_exit97:                                       ; preds = %blk_exit90
  %calcOffset102 = add i32 %21, 64
  %59 = inttoptr i32 %calcOffset102 to i32*
  store i32 0, i32* %59, align 4
  %calcOffset103 = add i32 %stack_addr, -512
  %60 = inttoptr i32 %calcOffset103 to i32*
  store i32 0, i32* %60, align 4
  %61 = call i32 @malloc(i32 800)
  %62 = icmp eq i32 %61, 0
  br i1 %62, label %blk_exit2, label %loop_entry107

blk_exit2:                                        ; preds = %blk_exit97
  call void @perror(i32 1569)
  call void @exit(i32 -1)
  unreachable

loop_entry107:                                    ; preds = %blk_exit113, %brif_next109, %blk_exit97
  %_local_8.1 = phi i32 [ %_local_8.1, %brif_next109 ], [ %105, %blk_exit113 ], [ 0, %blk_exit97 ]
  %_local_10.0 = phi i32 [ %_local_10.0, %brif_next109 ], [ %_local_10.1, %blk_exit113 ], [ %61, %blk_exit97 ]
  %_local_11.0 = phi i32 [ %_local_11.0, %brif_next109 ], [ %_local_11.1, %blk_exit113 ], [ 200, %blk_exit97 ]
  %63 = call i32 @readdir(i32 %57)
  %64 = icmp eq i32 %63, 0
  br i1 %64, label %blk_exit106, label %brif_next109

blk_exit106:                                      ; preds = %loop_entry107
  %65 = call i32 @closedir(i32 %57)
  call void @qsort(i32 %_local_10.0, i32 %_local_8.1, i32 4, i32 1)
  %66 = icmp eq i32 %_local_8.1, 0
  br i1 %66, label %blk_exit119, label %loop_entry122

blk_exit119:                                      ; preds = %blk_exit123, %blk_exit106
  call void @free(i32 %_local_10.0)
  %67 = load i32, i32* %59, align 4
  %brif_val134.not = icmp eq i32 %67, 0
  br i1 %brif_val134.not, label %brif_next135, label %blk_exit85

brif_next135:                                     ; preds = %blk_exit119
  %68 = load i32, i32* %41, align 4
  %69 = call i32 @strcmp(i32 1121, i32 %68)
  %70 = icmp eq i32 %69, 0
  br i1 %70, label %blk_exit87, label %brif_next138

blk_exit87:                                       ; preds = %brif_next138, %brif_next135
  call void @free(i32 %_local_7.0)
  %71 = inttoptr i32 %calcOffset71 to i32*
  %72 = load i32, i32* %71, align 4
  call void @free(i32 %72)
  %calcOffset192 = add i32 %21, 32
  %73 = inttoptr i32 %calcOffset192 to i32*
  %74 = load i32, i32* %73, align 4
  call void @free(i32 %74)
  %75 = load i32, i32* %38, align 4
  call void @free(i32 %75)
  %76 = load i32, i32* %41, align 4
  call void @free(i32 %76)
  %77 = load i32, i32* %35, align 4
  %brif_val197.not = icmp slt i32 %77, 0
  br i1 %brif_val197.not, label %blk_exit11, label %blk_exit195

blk_exit195:                                      ; preds = %blk_exit87
  %78 = call i32 @close(i32 %77)
  br label %blk_exit11

brif_next138:                                     ; preds = %brif_next135
  %79 = call i32 @strcmp(i32 1282, i32 %68)
  %80 = icmp eq i32 %79, 0
  br i1 %80, label %blk_exit87, label %brif_next140

brif_next140:                                     ; preds = %brif_next138
  %calcOffset141 = add i32 %stack_addr, -1584
  %81 = inttoptr i32 %calcOffset141 to i32*
  store i32 %68, i32* %81, align 4
  %82 = load i32, i32* null, align 2147483648
  %83 = call i32 @fprintf(i32 %82, i32 1942, i32 %calcOffset141)
  br label %blk_exit88

loop_entry122:                                    ; preds = %blk_exit123, %blk_exit106
  %_local_8.2 = phi i32 [ %91, %blk_exit123 ], [ %_local_8.1, %blk_exit106 ]
  %_param_1.0 = phi i32 [ %90, %blk_exit123 ], [ %_local_10.0, %blk_exit106 ]
  %84 = inttoptr i32 %_param_1.0 to i32*
  %85 = load i32, i32* %84, align 4
  %86 = load i32, i32* %41, align 4
  %87 = call i32 @add_file(i32 -1, i32 %85, i32 %86, i32 %calcOffset70, i32 %calcOffset103, i32 %21)
  %88 = icmp eq i32 %87, 0
  br i1 %88, label %blk_exit123, label %brif_next127

blk_exit123:                                      ; preds = %brif_next127, %loop_entry122
  %89 = load i32, i32* %84, align 4
  call void @free(i32 %89)
  %90 = add i32 %_param_1.0, 4
  %91 = add i32 %_local_8.2, -1
  %brif_val131.not = icmp eq i32 %91, 0
  br i1 %brif_val131.not, label %blk_exit119, label %loop_entry122

brif_next127:                                     ; preds = %loop_entry122
  %92 = load i32, i32* %59, align 4
  %93 = add i32 %92, 1
  store i32 %93, i32* %59, align 4
  br label %blk_exit123

brif_next109:                                     ; preds = %loop_entry107
  %calcOffset110 = add i32 %63, 9
  %94 = inttoptr i32 %calcOffset110 to i8*
  %95 = load i8, i8* %94, align 1
  %96 = icmp eq i8 %95, 0
  br i1 %96, label %loop_entry107, label %brif_next112

brif_next112:                                     ; preds = %brif_next109
  %97 = call i32 @strdup(i32 %calcOffset110)
  %.not224 = icmp eq i32 %_local_8.1, %_local_11.0
  br i1 %.not224, label %brif_next115, label %blk_exit113

brif_next115:                                     ; preds = %brif_next112
  %98 = add i32 %_local_8.1, 200
  %99 = shl i32 %98, 2
  %100 = call i32 @realloc(i32 %_local_10.0, i32 %99)
  %101 = icmp eq i32 %100, 0
  br i1 %101, label %blk_exit1, label %blk_exit113

blk_exit1:                                        ; preds = %brif_next115
  call void @perror(i32 1569)
  call void @exit(i32 -1)
  unreachable

blk_exit113:                                      ; preds = %brif_next115, %brif_next112
  %_local_10.1 = phi i32 [ %100, %brif_next115 ], [ %_local_10.0, %brif_next112 ]
  %_local_11.1 = phi i32 [ %98, %brif_next115 ], [ %_local_11.0, %brif_next112 ]
  %102 = shl i32 %_local_8.1, 2
  %103 = add i32 %_local_10.1, %102
  %104 = inttoptr i32 %103 to i32*
  store i32 %97, i32* %104, align 4
  %105 = add i32 %_local_8.1, 1
  br label %loop_entry107

blk_exit88:                                       ; preds = %brif_next171, %brif_next169, %brif_next167, %brif_next165, %brif_next163, %brif_next161, %brif_next159, %brif_next157, %brif_next155, %brif_next153, %brif_next151, %brif_next149, %brif_next147, %brif_next178, %blk_exit89, %brif_next140, %brif_next100
  %brif_val180.not = icmp eq i32 %_arg_5, 0
  br i1 %brif_val180.not, label %brif_next181, label %blk_exit179

brif_next181:                                     ; preds = %blk_exit88
  %calcOffset182 = add i32 %stack_addr, -1632
  %106 = inttoptr i32 %calcOffset182 to i32*
  store i32 %_local_7.0, i32* %106, align 4
  %107 = load i32, i32* null, align 2147483648
  %108 = call i32 @fprintf(i32 %107, i32 1581, i32 %calcOffset182)
  br label %blk_exit179

blk_exit179:                                      ; preds = %brif_next181, %blk_exit88
  call void @free(i32 %_local_7.0)
  %109 = inttoptr i32 %calcOffset71 to i32*
  %110 = load i32, i32* %109, align 4
  call void @free(i32 %110)
  %calcOffset184 = add i32 %21, 32
  %111 = inttoptr i32 %calcOffset184 to i32*
  %112 = load i32, i32* %111, align 4
  call void @free(i32 %112)
  %113 = load i32, i32* %38, align 4
  call void @free(i32 %113)
  %114 = load i32, i32* %41, align 4
  call void @free(i32 %114)
  %115 = load i32, i32* %35, align 4
  %brif_val189.not = icmp slt i32 %115, 0
  br i1 %brif_val189.not, label %blk_exit11, label %blk_exit187

blk_exit187:                                      ; preds = %blk_exit179
  %116 = call i32 @close(i32 %115)
  br label %blk_exit11

blk_exit85:                                       ; preds = %blk_exit86, %blk_exit119
  %117 = inttoptr i32 %_arg_3 to i32*
  %118 = load i32, i32* %117, align 4
  %brif_val203.not = icmp eq i32 %118, 0
  br i1 %brif_val203.not, label %brif_next204, label %blk_exit201

brif_next204:                                     ; preds = %blk_exit85
  %119 = inttoptr i32 %_arg_4 to i32*
  store i32 %21, i32* %119, align 4
  br label %blk_exit200

blk_exit200:                                      ; preds = %blk_exit206, %brif_next209, %brif_next204
  %.pre-phi = phi i32* [ %117, %blk_exit206 ], [ %121, %brif_next209 ], [ %117, %brif_next204 ]
  store i32 %21, i32* %.pre-phi, align 4
  br label %blk_exit11

blk_exit201:                                      ; preds = %blk_exit85
  %120 = load i32, i32* %36, align 4
  %.not226 = icmp eq i32 %120, -1
  br i1 %.not226, label %brif_next209, label %blk_exit206

brif_next209:                                     ; preds = %blk_exit201
  %121 = inttoptr i32 %_arg_4 to i32*
  %122 = load i32, i32* %121, align 4
  %calcOffset211 = add i32 %122, 76
  %123 = inttoptr i32 %calcOffset211 to i32*
  store i32 %21, i32* %123, align 4
  %124 = load i32, i32* %121, align 4
  %calcOffset213 = add i32 %21, 80
  %125 = inttoptr i32 %calcOffset213 to i32*
  store i32 %124, i32* %125, align 4
  br label %blk_exit200

blk_exit206:                                      ; preds = %blk_exit201
  %calcOffset214 = add i32 %118, 80
  %126 = inttoptr i32 %calcOffset214 to i32*
  store i32 %21, i32* %126, align 4
  %127 = load i32, i32* %117, align 4
  %128 = inttoptr i32 %calcOffset68 to i32*
  store i32 %127, i32* %128, align 4
  br label %blk_exit200

blk_exit89:                                       ; preds = %blk_exit80
  %129 = call i32 @strrchr(i32 %_local_7.0, i32 47)
  %130 = add i32 %129, 1
  %select_cond.not = icmp eq i32 %129, 0
  %131 = select i1 %select_cond.not, i32 %_local_7.0, i32 %130
  %132 = inttoptr i32 %131 to i8*
  %133 = load i8, i8* %132, align 1
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
  %141 = inttoptr i32 %calcOffset221 to i32*
  store i32 1987, i32* %141, align 4
  %142 = load i32, i32* null, align 2147483648
  %143 = call i32 @fprintf(i32 %142, i32 1191, i32 %calcOffset221)
  call void @exit(i32 1)
  unreachable

brif_next175:                                     ; preds = %blk_exit145
  %calcOffset176 = add i32 %stack_addr, -1600
  %144 = inttoptr i32 %calcOffset176 to i32*
  store i32 %_local_7.0, i32* %144, align 4
  %145 = add i32 %137, 6
  %146 = call i32 @snprintf(i32 %139, i32 %145, i32 1074, i32 %calcOffset176)
  %147 = call i32 @access(i32 %139, i32 4)
  %brif_val177.not = icmp slt i32 %147, 0
  br i1 %brif_val177.not, label %brif_next178, label %blk_exit86

brif_next178:                                     ; preds = %brif_next175
  call void @free(i32 %139)
  br label %blk_exit88

blk_exit86:                                       ; preds = %brif_next175
  %148 = inttoptr i32 %calcOffset71 to i32*
  store i32 %139, i32* %148, align 4
  br label %blk_exit85

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
  %brif_val33.not = icmp eq i32 %_arg_5, 0
  %or.cond228 = select i1 %brif_val.not, i1 %brif_val33.not, i1 false
  br i1 %or.cond228, label %brif_next34, label %blk_exit11

brif_next34:                                      ; preds = %blk_exit25
  %176 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %177 = icmp eq i32 %176, 0
  br i1 %177, label %blk_exit35, label %brif_next37

blk_exit35:                                       ; preds = %brif_next34
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

brif_next37:                                      ; preds = %brif_next34
  %178 = add i32 %stack_addr, -512
  %179 = call i32 @strncpy(i32 %178, i32 %176, i32 512)
  %calcOffset38 = add i32 %stack_addr, -1
  %180 = inttoptr i32 %calcOffset38 to i8*
  store i8 0, i8* %180, align 1
  br label %loop_entry

loop_entry:                                       ; preds = %brif_next55, %brif_next37
  %_param_2.0 = phi i32 [ %178, %brif_next37 ], [ %storemerge, %brif_next55 ]
  %181 = inttoptr i32 %_param_2.0 to i8*
  %182 = load i8, i8* %181, align 1
  %183 = icmp eq i8 %182, 0
  br i1 %183, label %blk_exit40, label %brif_next43

blk_exit40:                                       ; preds = %brif_next55, %loop_entry
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

brif_next43:                                      ; preds = %loop_entry
  %184 = call i32 @strchr(i32 %_param_2.0, i32 58)
  %brif_val46.not = icmp eq i32 %184, 0
  br i1 %brif_val46.not, label %blk_exit44, label %blk_exit45

blk_exit44:                                       ; preds = %blk_exit45, %brif_next43
  %storemerge = phi i32 [ %195, %blk_exit45 ], [ 0, %brif_next43 ]
  %calcOffset49 = add i32 %stack_addr, -1676
  %185 = inttoptr i32 %calcOffset49 to i32*
  store i32 %_param_2.0, i32* %185, align 4
  %calcOffset50 = add i32 %stack_addr, -1680
  %186 = inttoptr i32 %calcOffset50 to i32*
  store i32 1089, i32* %186, align 4
  %187 = call i32 @snprintf(i32 %.pre-phi230, i32 1024, i32 1174, i32 %calcOffset50)
  %188 = call i32 @strncmp(i32 %_local_7.0, i32 %_param_2.0, i32 2)
  %189 = icmp eq i32 %188, 0
  br i1 %189, label %blk_exit11, label %brif_next52

brif_next52:                                      ; preds = %blk_exit44
  %190 = call i32 @strlen(i32 1089)
  %191 = add i32 %190, 3
  %192 = call i32 @strncmp(i32 %_local_7.0, i32 %.pre-phi230, i32 %191)
  %193 = icmp eq i32 %192, 0
  br i1 %193, label %blk_exit11, label %brif_next55

brif_next55:                                      ; preds = %brif_next52
  %brif_val56.not = icmp eq i32 %storemerge, 0
  br i1 %brif_val56.not, label %blk_exit40, label %loop_entry

blk_exit45:                                       ; preds = %brif_next43
  %194 = inttoptr i32 %184 to i8*
  store i8 0, i8* %194, align 1
  %195 = add i32 %184, 1
  br label %blk_exit44

blk_exit11:                                       ; preds = %brif_next52, %blk_exit44, %blk_exit40, %blk_exit35, %blk_exit25, %blk_exit200, %blk_exit187, %blk_exit179, %blk_exit195, %blk_exit87, %blk_exit12, %brif_next17, %brif_next17
  %_local_8.0 = phi i32 [ 0, %blk_exit187 ], [ 1, %blk_exit195 ], [ 1, %blk_exit200 ], [ 0, %blk_exit35 ], [ 0, %blk_exit25 ], [ 0, %blk_exit12 ], [ 0, %brif_next17 ], [ 0, %brif_next17 ], [ 0, %blk_exit40 ], [ 0, %blk_exit44 ], [ 0, %blk_exit179 ], [ 1, %blk_exit87 ], [ 0, %brif_next52 ]
  %_local_7.1 = phi i32 [ %21, %blk_exit187 ], [ %21, %blk_exit195 ], [ %_local_7.0, %blk_exit200 ], [ %_local_7.0, %blk_exit35 ], [ %_local_7.0, %blk_exit25 ], [ %_local_7.0, %blk_exit12 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %blk_exit40 ], [ %_local_7.0, %blk_exit44 ], [ %21, %blk_exit179 ], [ %21, %blk_exit87 ], [ %_local_7.0, %brif_next52 ]
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
  %202 = inttoptr i32 %stack_end to i32*
  store i32 1987, i32* %202, align 4
  %203 = load i32, i32* null, align 2147483648
  %204 = call i32 @fprintf(i32 %203, i32 1191, i32 %stack_end)
  call void @exit(i32 1)
  unreachable

brif_next9:                                       ; preds = %blk_exit7
  %calcOffset = add i32 %stack_addr, -1548
  %205 = inttoptr i32 %calcOffset to i32*
  store i32 %_arg_1, i32* %205, align 4
  %calcOffset10 = add i32 %stack_addr, -1552
  %206 = inttoptr i32 %calcOffset10 to i32*
  store i32 %_arg_2, i32* %206, align 4
  %207 = add i32 %198, 12
  %208 = call i32 @snprintf(i32 %200, i32 %207, i32 1174, i32 %calcOffset10)
  br label %blk_exit6
}

; Function Attrs: null_pointer_is_valid
define internal i32 @maxlen_in_list(i32 %_arg_0) #0 {
allocator:
  %brif_val.not = icmp eq i32 %_arg_0, 0
  br i1 %brif_val.not, label %return, label %loop_entry

return:                                           ; preds = %blk_exit1, %allocator
  %return_0 = phi i32 [ 0, %allocator ], [ %7, %blk_exit1 ]
  ret i32 %return_0

loop_entry:                                       ; preds = %blk_exit1, %allocator
  %_local_1.0 = phi i32 [ %7, %blk_exit1 ], [ 0, %allocator ]
  %_param_0.0 = phi i32 [ %9, %blk_exit1 ], [ %_arg_0, %allocator ]
  %calcOffset = add i32 %_param_0.0, 68
  %0 = inttoptr i32 %calcOffset to i32*
  %1 = load i32, i32* %0, align 4
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %blk_exit2, label %brif_next4

blk_exit2:                                        ; preds = %loop_entry
  call void @get_tbl(i32 %_param_0.0)
  %3 = add i32 %_param_0.0, 48
  %4 = inttoptr i32 %3 to i32*
  %5 = load i32, i32* %4, align 4
  br label %blk_exit1

blk_exit1:                                        ; preds = %brif_next4, %blk_exit2
  %storemerge = phi i32 [ %10, %brif_next4 ], [ %5, %blk_exit2 ]
  %6 = icmp sgt i32 %storemerge, %_local_1.0
  %7 = select i1 %6, i32 %storemerge, i32 %_local_1.0
  %calcOffset6 = add i32 %_param_0.0, 76
  %8 = inttoptr i32 %calcOffset6 to i32*
  %9 = load i32, i32* %8, align 4
  %brif_val7.not = icmp eq i32 %9, 0
  br i1 %brif_val7.not, label %return, label %loop_entry

brif_next4:                                       ; preds = %loop_entry
  %10 = call i32 @maxlen_in_list(i32 %1)
  br label %blk_exit1
}

; Function Attrs: null_pointer_is_valid
define internal void @matches_in_list(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %stack = alloca [32 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [32 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -32
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit1, label %brif_next

blk_exit1:                                        ; preds = %blk_exit2, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  %1 = load i32, i32* null, align 2147483648
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit2, %brif_next
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %78, %blk_exit2 ]
  %calcOffset = add i32 %_param_0.0, 68
  %2 = inttoptr i32 %calcOffset to i32*
  %3 = load i32, i32* %2, align 4
  %4 = icmp eq i32 %3, 0
  br i1 %4, label %blk_exit3, label %brif_next5

blk_exit3:                                        ; preds = %loop_entry
  %calcOffset7 = add i32 %_param_0.0, 16
  %5 = inttoptr i32 %calcOffset7 to i32*
  %6 = load i32, i32* %5, align 4
  %brif_val8.not = icmp eq i32 %6, 0
  br i1 %brif_val8.not, label %brif_next9, label %blk_exit6

brif_next9:                                       ; preds = %blk_exit3
  %calcOffset10 = add i32 %_param_0.0, 4
  %7 = inttoptr i32 %calcOffset10 to i32*
  %8 = load i32, i32* %7, align 4
  %9 = call i32 @fdopen(i32 %8, i32 1194)
  store i32 %9, i32* %5, align 4
  %10 = icmp eq i32 %9, 0
  br i1 %10, label %blk_exit, label %blk_exit6

blk_exit:                                         ; preds = %brif_next9
  %calcOffset92 = add i32 %_param_0.0, 24
  %11 = inttoptr i32 %calcOffset92 to i32*
  %12 = load i32, i32* %11, align 4
  call void @perror(i32 %12)
  call void @exit(i32 1)
  unreachable

blk_exit6:                                        ; preds = %brif_next9, %blk_exit3
  %_local_6.0 = phi i32 [ %9, %brif_next9 ], [ %6, %blk_exit3 ]
  %13 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  %14 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %15 = call i32 @fgets(i32 %13, i32 %14, i32 %_local_6.0)
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %blk_exit2, label %loop_entry16

loop_entry16:                                     ; preds = %blk_exit17, %blk_exit6
  %_local_7.0 = phi i32 [ %_local_7.1, %blk_exit17 ], [ 0, %blk_exit6 ]
  %_local_5.0 = phi i32 [ %storemerge, %blk_exit17 ], [ %13, %blk_exit6 ]
  %17 = inttoptr i32 %_local_5.0 to i8*
  %18 = load i8, i8* %17, align 1
  %calcOffset21 = add i32 %_param_0.0, 60
  %19 = inttoptr i32 %calcOffset21 to i8*
  %20 = load i8, i8* %19, align 1
  %.not = icmp eq i8 %18, %20
  br i1 %.not, label %brif_next23, label %blk_exit19

brif_next23:                                      ; preds = %loop_entry16
  %calcOffset24 = add i32 %_local_5.0, 1
  %21 = inttoptr i32 %calcOffset24 to i8*
  %22 = load i8, i8* %21, align 1
  %23 = icmp eq i8 %22, 10
  br i1 %23, label %blk_exit18, label %blk_exit19

blk_exit18:                                       ; preds = %brif_next23
  store i8 0, i8* %17, align 1
  %24 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  %calcOffset29 = add i32 %_param_0.0, 38
  %25 = inttoptr i32 %calcOffset29 to i8*
  %26 = load i8, i8* %25, align 1
  %27 = icmp eq i8 %26, 0
  br i1 %27, label %blk_exit28, label %brif_next31

blk_exit28:                                       ; preds = %brif_next33, %brif_next31, %blk_exit18
  %_local_8.0 = phi i32 [ %24, %blk_exit18 ], [ %71, %brif_next33 ], [ %24, %brif_next31 ]
  %28 = sub i32 %_local_5.0, %24
  %calcOffset35 = add i32 %_param_0.0, 56
  %29 = inttoptr i32 %calcOffset35 to i8*
  %30 = load i8, i8* %29, align 1
  %31 = and i8 %30, 4
  %brif_val36.not.not = icmp eq i8 %31, 0
  br i1 %brif_val36.not.not, label %blk_exit34, label %brif_next37

blk_exit34:                                       ; preds = %blk_exit42, %brif_next37, %blk_exit28
  %32 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %33 = icmp slt i32 %28, %32
  br i1 %33, label %blk_exit58, label %brif_next60

blk_exit58:                                       ; preds = %brif_next60, %blk_exit34
  %34 = icmp sgt i32 %28, %32
  br i1 %34, label %blk_exit63, label %brif_next65

blk_exit63:                                       ; preds = %brif_next65, %blk_exit58
  %35 = call i32 @regexec(i32 2232, i32 %_local_8.0, i32 0, i32 0, i32 0)
  %brif_val68.not = icmp eq i32 %35, 0
  br i1 %brif_val68.not, label %brif_next69, label %blk_exit57

brif_next69:                                      ; preds = %blk_exit63
  %brif_val71.not = icmp eq i32 %_local_7.0, 0
  br i1 %brif_val71.not, label %brif_next72, label %blk_exit70

brif_next72:                                      ; preds = %brif_next69
  %calcOffset73 = add i32 %_param_0.0, 20
  %36 = inttoptr i32 %calcOffset73 to i32*
  %37 = load i32, i32* %36, align 4
  %38 = load i8, i8* %19, align 1
  %39 = zext i8 %38 to i32
  %calcOffset75 = add i32 %stack_addr, -12
  %40 = inttoptr i32 %calcOffset75 to i32*
  store i32 %39, i32* %40, align 4
  %calcOffset76 = add i32 %stack_addr, -16
  %41 = inttoptr i32 %calcOffset76 to i32*
  store i32 %37, i32* %41, align 4
  %42 = call i32 @fprintf(i32 %1, i32 1864, i32 %calcOffset76)
  %43 = inttoptr i32 %_arg_1 to i8*
  store i8 1, i8* %43, align 1
  br label %blk_exit70

blk_exit70:                                       ; preds = %brif_next72, %brif_next69
  %44 = call i32 @fputs(i32 %_local_8.0, i32 %1)
  %45 = load i8, i8* %19, align 1
  %46 = zext i8 %45 to i32
  %47 = inttoptr i32 %stack_end to i32*
  store i32 %46, i32* %47, align 4
  %48 = call i32 @printf(i32 1869, i32 %stack_end)
  br label %blk_exit57

brif_next65:                                      ; preds = %blk_exit58
  %49 = load i8, i8* inttoptr (i32 2274 to i8*), align 2
  %brif_val66.not = icmp eq i8 %49, 0
  br i1 %brif_val66.not, label %blk_exit63, label %blk_exit57

blk_exit57:                                       ; preds = %brif_next60, %brif_next65, %blk_exit70, %blk_exit63
  %_local_7.2 = phi i32 [ 1, %blk_exit70 ], [ %_local_7.0, %blk_exit63 ], [ %_local_7.0, %brif_next65 ], [ %_local_7.0, %brif_next60 ]
  %50 = load i8, i8* %25, align 1
  %51 = icmp eq i8 %50, 0
  br i1 %51, label %blk_exit80, label %brif_next83

blk_exit80:                                       ; preds = %brif_next85, %brif_next83, %blk_exit57
  %52 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  br label %blk_exit17

brif_next83:                                      ; preds = %blk_exit57
  %53 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val84.not = icmp eq i8 %53, 0
  br i1 %brif_val84.not, label %brif_next85, label %blk_exit80

brif_next85:                                      ; preds = %brif_next83
  call void @free(i32 %_local_8.0)
  br label %blk_exit80

brif_next60:                                      ; preds = %blk_exit34
  %54 = load i8, i8* inttoptr (i32 2273 to i8*), align 1
  %brif_val61.not = icmp eq i8 %54, 0
  br i1 %brif_val61.not, label %blk_exit58, label %blk_exit57

brif_next37:                                      ; preds = %blk_exit28
  %55 = inttoptr i32 %_local_8.0 to i8*
  %56 = load i8, i8* %55, align 1
  %57 = icmp eq i8 %56, 0
  br i1 %57, label %blk_exit34, label %loop_entry41

loop_entry41:                                     ; preds = %blk_exit42, %brif_next37
  %_local_6.1 = phi i32 [ %65, %blk_exit42 ], [ %_local_8.0, %brif_next37 ]
  %_local_5.1.in = phi i8 [ %67, %blk_exit42 ], [ %56, %brif_next37 ]
  %_local_5.1 = zext i8 %_local_5.1.in to i32
  %58 = add nsw i32 %_local_5.1, -65
  %59 = icmp ult i32 %58, 26
  %60 = and i32 %_local_5.1, 128
  %brif_val47.not = icmp eq i32 %60, 0
  %or.cond = and i1 %59, %brif_val47.not
  br i1 %or.cond, label %blk_exit43, label %blk_exit44

blk_exit43:                                       ; preds = %blk_exit44, %loop_entry41
  %storemerge94 = phi i8 [ 65, %loop_entry41 ], [ 97, %blk_exit44 ]
  %storemerge93 = phi i8 [ -52, %loop_entry41 ], [ -84, %blk_exit44 ]
  %61 = add i8 %storemerge93, %_local_5.1.in
  %62 = urem i8 %61, 26
  %63 = add nuw nsw i8 %62, %storemerge94
  %64 = inttoptr i32 %_local_6.1 to i8*
  store i8 %63, i8* %64, align 1
  br label %blk_exit42

blk_exit42:                                       ; preds = %blk_exit44, %blk_exit43
  %65 = add i32 %_local_6.1, 1
  %66 = inttoptr i32 %65 to i8*
  %67 = load i8, i8* %66, align 1
  %brif_val55.not = icmp eq i8 %67, 0
  br i1 %brif_val55.not, label %blk_exit34, label %loop_entry41

blk_exit44:                                       ; preds = %loop_entry41
  %68 = add nsw i32 %_local_5.1, -97
  %69 = icmp ult i32 %68, 26
  %or.cond95 = and i1 %69, %brif_val47.not
  br i1 %or.cond95, label %blk_exit43, label %blk_exit42

brif_next31:                                      ; preds = %blk_exit18
  %70 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val32.not = icmp eq i8 %70, 0
  br i1 %brif_val32.not, label %brif_next33, label %blk_exit28

brif_next33:                                      ; preds = %brif_next31
  %71 = call i32 @strdup(i32 %24)
  br label %blk_exit28

blk_exit17:                                       ; preds = %blk_exit19, %blk_exit80
  %_local_7.1 = phi i32 [ %_local_7.2, %blk_exit80 ], [ %_local_7.0, %blk_exit19 ]
  %storemerge = phi i32 [ %52, %blk_exit80 ], [ %76, %blk_exit19 ]
  %72 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %73 = load i32, i32* %5, align 4
  %74 = call i32 @fgets(i32 %storemerge, i32 %72, i32 %73)
  %brif_val87.not = icmp eq i32 %74, 0
  br i1 %brif_val87.not, label %blk_exit2, label %loop_entry16

blk_exit19:                                       ; preds = %brif_next23, %loop_entry16
  %75 = call i32 @strlen(i32 %_local_5.0)
  %76 = add i32 %_local_5.0, %75
  br label %blk_exit17

blk_exit2:                                        ; preds = %brif_next5, %blk_exit17, %blk_exit6
  %calcOffset89 = add i32 %_param_0.0, 76
  %77 = inttoptr i32 %calcOffset89 to i32*
  %78 = load i32, i32* %77, align 4
  %brif_val90.not = icmp eq i32 %78, 0
  br i1 %brif_val90.not, label %blk_exit1, label %loop_entry

brif_next5:                                       ; preds = %loop_entry
  call void @matches_in_list(i32 %3, i32 %_arg_1)
  br label %blk_exit2
}

; Function Attrs: null_pointer_is_valid
define internal void @get_tbl(i32 %_arg_0) #0 {
allocator:
  %stack = alloca [96 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [96 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -96
  %calcOffset = add i32 %_arg_0, 36
  %0 = inttoptr i32 %calcOffset to i8*
  %1 = load i8, i8* %0, align 1
  %brif_val.not = icmp eq i8 %1, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

brif_next:                                        ; preds = %allocator
  %calcOffset10 = add i32 %_arg_0, 68
  %2 = inttoptr i32 %calcOffset10 to i32*
  %3 = load i32, i32* %2, align 4
  %brif_val11.not = icmp eq i32 %3, 0
  br i1 %brif_val11.not, label %brif_next12, label %blk_exit9

brif_next12:                                      ; preds = %brif_next
  %calcOffset13 = add i32 %_arg_0, 28
  %4 = inttoptr i32 %calcOffset13 to i32*
  %5 = load i32, i32* %4, align 4
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
  %19 = inttoptr i32 %7 to i32*
  %20 = load i32, i32* %19, align 4
  %21 = call i32 @ntohl(i32 %20)
  store i32 %21, i32* %19, align 4
  %22 = inttoptr i32 %9 to i32*
  %23 = load i32, i32* %22, align 4
  %24 = call i32 @ntohl(i32 %23)
  store i32 %24, i32* %22, align 4
  %25 = inttoptr i32 %11 to i32*
  %26 = load i32, i32* %25, align 4
  %27 = call i32 @ntohl(i32 %26)
  store i32 %27, i32* %25, align 4
  %28 = inttoptr i32 %13 to i32*
  %29 = load i32, i32* %28, align 4
  %30 = call i32 @ntohl(i32 %29)
  store i32 %30, i32* %28, align 4
  %31 = inttoptr i32 %15 to i32*
  %32 = load i32, i32* %31, align 4
  %33 = call i32 @ntohl(i32 %32)
  store i32 %33, i32* %31, align 4
  %34 = call i32 @close(i32 %6)
  br label %blk_exit8

blk_exit:                                         ; preds = %brif_next25
  %calcOffset70 = add i32 %_arg_0, 24
  %35 = inttoptr i32 %calcOffset70 to i32*
  %36 = load i32, i32* %35, align 4
  %37 = inttoptr i32 %stack_end to i32*
  store i32 %36, i32* %37, align 4
  %38 = load i32, i32* null, align 2147483648
  %39 = call i32 @fprintf(i32 %38, i32 1841, i32 %stack_end)
  call void @exit(i32 1)
  unreachable

blk_exit1:                                        ; preds = %brif_next23
  %calcOffset68 = add i32 %_arg_0, 24
  %40 = inttoptr i32 %calcOffset68 to i32*
  %41 = load i32, i32* %40, align 4
  %calcOffset69 = add i32 %stack_addr, -80
  %42 = inttoptr i32 %calcOffset69 to i32*
  store i32 %41, i32* %42, align 4
  %43 = load i32, i32* null, align 2147483648
  %44 = call i32 @fprintf(i32 %43, i32 1841, i32 %calcOffset69)
  call void @exit(i32 1)
  unreachable

blk_exit2:                                        ; preds = %brif_next21
  %calcOffset66 = add i32 %_arg_0, 24
  %45 = inttoptr i32 %calcOffset66 to i32*
  %46 = load i32, i32* %45, align 4
  %calcOffset67 = add i32 %stack_addr, -64
  %47 = inttoptr i32 %calcOffset67 to i32*
  store i32 %46, i32* %47, align 4
  %48 = load i32, i32* null, align 2147483648
  %49 = call i32 @fprintf(i32 %48, i32 1841, i32 %calcOffset67)
  call void @exit(i32 1)
  unreachable

blk_exit3:                                        ; preds = %brif_next19
  %calcOffset64 = add i32 %_arg_0, 24
  %50 = inttoptr i32 %calcOffset64 to i32*
  %51 = load i32, i32* %50, align 4
  %calcOffset65 = add i32 %stack_addr, -48
  %52 = inttoptr i32 %calcOffset65 to i32*
  store i32 %51, i32* %52, align 4
  %53 = load i32, i32* null, align 2147483648
  %54 = call i32 @fprintf(i32 %53, i32 1841, i32 %calcOffset65)
  call void @exit(i32 1)
  unreachable

blk_exit4:                                        ; preds = %brif_next17
  %calcOffset62 = add i32 %_arg_0, 24
  %55 = inttoptr i32 %calcOffset62 to i32*
  %56 = load i32, i32* %55, align 4
  %calcOffset63 = add i32 %stack_addr, -32
  %57 = inttoptr i32 %calcOffset63 to i32*
  store i32 %56, i32* %57, align 4
  %58 = load i32, i32* null, align 2147483648
  %59 = call i32 @fprintf(i32 %58, i32 1841, i32 %calcOffset63)
  call void @exit(i32 1)
  unreachable

blk_exit5:                                        ; preds = %brif_next15
  %calcOffset60 = add i32 %_arg_0, 24
  %60 = inttoptr i32 %calcOffset60 to i32*
  %61 = load i32, i32* %60, align 4
  %calcOffset61 = add i32 %stack_addr, -16
  %62 = inttoptr i32 %calcOffset61 to i32*
  store i32 %61, i32* %62, align 4
  %63 = load i32, i32* null, align 2147483648
  %64 = call i32 @fprintf(i32 %63, i32 1841, i32 %calcOffset61)
  call void @exit(i32 1)
  unreachable

blk_exit6:                                        ; preds = %brif_next12
  %65 = load i32, i32* %4, align 4
  call void @perror(i32 %65)
  call void @exit(i32 1)
  unreachable

blk_exit8:                                        ; preds = %blk_exit49, %brif_next27
  store i8 1, i8* %0, align 1
  br label %blk_exit7

blk_exit9:                                        ; preds = %brif_next
  %66 = add i32 %_arg_0, 52
  %67 = inttoptr i32 %66 to i32*
  store i32 -1, i32* %67, align 4
  %68 = add i32 %_arg_0, 44
  %69 = inttoptr i32 %68 to i8*
  call void @llvm.memset.p0i8.i64(i8* %69, i8 0, i64 8, i1 true)
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit49, %blk_exit9
  %_local_2.0 = phi i32 [ %3, %blk_exit9 ], [ %86, %blk_exit49 ]
  call void @get_tbl(i32 %_local_2.0)
  %70 = inttoptr i32 %68 to i32*
  %71 = load i32, i32* %70, align 4
  %72 = add i32 %_local_2.0, 44
  %73 = inttoptr i32 %72 to i32*
  %74 = load i32, i32* %73, align 4
  %75 = add i32 %71, %74
  store i32 %75, i32* %70, align 4
  %calcOffset44 = add i32 %_arg_0, 48
  %76 = inttoptr i32 %calcOffset44 to i32*
  %77 = load i32, i32* %76, align 4
  %78 = add i32 %_local_2.0, 48
  %79 = inttoptr i32 %78 to i32*
  %80 = load i32, i32* %79, align 4
  %.not77 = icmp ult i32 %77, %80
  br i1 %.not77, label %brif_next47, label %blk_exit43

brif_next47:                                      ; preds = %loop_entry
  store i32 %80, i32* %76, align 4
  br label %blk_exit43

blk_exit43:                                       ; preds = %brif_next47, %loop_entry
  %81 = load i32, i32* %67, align 4
  %82 = add i32 %_local_2.0, 52
  %83 = inttoptr i32 %82 to i32*
  %84 = load i32, i32* %83, align 4
  %.not78 = icmp ugt i32 %81, %84
  br i1 %.not78, label %brif_next53, label %blk_exit49

brif_next53:                                      ; preds = %blk_exit43
  store i32 %84, i32* %67, align 4
  br label %blk_exit49

blk_exit49:                                       ; preds = %brif_next53, %blk_exit43
  %calcOffset55 = add i32 %_local_2.0, 76
  %85 = inttoptr i32 %calcOffset55 to i32*
  %86 = load i32, i32* %85, align 4
  %brif_val56.not = icmp eq i32 %86, 0
  br i1 %brif_val56.not, label %blk_exit8, label %loop_entry

blk_exit7:                                        ; preds = %blk_exit8, %allocator
  ret void
}

; Function Attrs: null_pointer_is_valid
define internal void @print_list(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %stack = alloca [96 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [96 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -96
  %0 = icmp eq i32 %_arg_0, 0
  br i1 %0, label %blk_exit, label %brif_next

blk_exit:                                         ; preds = %blk_exit17, %blk_exit40, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  %1 = add i32 %_arg_1, 1
  %2 = shl i32 %_arg_1, 2
  %3 = load i32, i32* null, align 2147483648
  %4 = icmp eq i32 %_arg_1, 0
  br i1 %4, label %loop_entry24, label %loop_entry

loop_entry24:                                     ; preds = %blk_exit40, %brif_next
  %_param_0.1 = phi i32 [ %_arg_0, %brif_next ], [ %38, %blk_exit40 ]
  %calcOffset25 = add i32 %stack_addr, -60
  %5 = inttoptr i32 %calcOffset25 to i32*
  store i32 2206, i32* %5, align 4
  %calcOffset26 = add i32 %stack_addr, -64
  %6 = inttoptr i32 %calcOffset26 to i32*
  store i32 %2, i32* %6, align 4
  %7 = call i32 @fprintf(i32 %3, i32 1167, i32 %calcOffset26)
  %8 = inttoptr i32 %_param_0.1 to i32*
  %9 = load i32, i32* %8, align 4
  %10 = icmp eq i32 %9, -1
  br i1 %10, label %blk_exit28, label %brif_next31

blk_exit28:                                       ; preds = %loop_entry24
  %11 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %brif_val33.not = icmp eq i8 %11, 0
  br i1 %brif_val33.not, label %brif_next34, label %blk_exit32

brif_next34:                                      ; preds = %blk_exit28
  %12 = load i32, i32* inttoptr (i32 3332 to i32*), align 4
  %13 = sitofp i32 %12 to double
  %14 = fsub double 1.000000e+02, %13
  %15 = add i32 %_param_0.1, 44
  %16 = inttoptr i32 %15 to i32*
  %17 = load i32, i32* %16, align 4
  %18 = uitofp i32 %17 to double
  %19 = fmul double %14, %18
  %20 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %21 = uitofp i32 %20 to double
  %22 = fdiv double %19, %21
  br label %blk_exit27

blk_exit32:                                       ; preds = %blk_exit28
  %23 = load i32, i32* inttoptr (i32 3356 to i32*), align 4
  %24 = sitofp i32 %23 to double
  %25 = fdiv double 1.000000e+02, %24
  br label %blk_exit27

blk_exit27:                                       ; preds = %brif_next31, %blk_exit32, %brif_next34
  %_local_6.1 = phi double [ %22, %brif_next34 ], [ %25, %blk_exit32 ], [ %39, %brif_next31 ]
  %calcOffset36 = add i32 %stack_addr, -80
  %26 = inttoptr i32 %calcOffset36 to double*
  store double %_local_6.1, double* %26, align 8
  %27 = call i32 @fprintf(i32 %3, i32 1561, i32 %calcOffset36)
  %calcOffset37 = add i32 %_param_0.1, 20
  %28 = inttoptr i32 %calcOffset37 to i32*
  %29 = load i32, i32* %28, align 4
  %select_cond38.not = icmp eq i32 %29, 0
  %30 = select i1 %select_cond38.not, i32 1492, i32 %29
  %31 = inttoptr i32 %stack_end to i32*
  store i32 %30, i32* %31, align 4
  %32 = call i32 @fprintf(i32 %3, i32 1190, i32 %stack_end)
  %33 = call i32 @putc(i32 10, i32 %3)
  %calcOffset41 = add i32 %_param_0.1, 68
  %34 = inttoptr i32 %calcOffset41 to i32*
  %35 = load i32, i32* %34, align 4
  %36 = icmp eq i32 %35, 0
  br i1 %36, label %blk_exit40, label %brif_next43

blk_exit40:                                       ; preds = %brif_next43, %blk_exit27
  %calcOffset44 = add i32 %_param_0.1, 76
  %37 = inttoptr i32 %calcOffset44 to i32*
  %38 = load i32, i32* %37, align 4
  %brif_val45.not = icmp eq i32 %38, 0
  br i1 %brif_val45.not, label %blk_exit, label %loop_entry24

brif_next43:                                      ; preds = %blk_exit27
  call void @print_list(i32 %35, i32 %1)
  br label %blk_exit40

brif_next31:                                      ; preds = %loop_entry24
  %39 = sitofp i32 %9 to double
  br label %blk_exit27

loop_entry:                                       ; preds = %blk_exit17, %brif_next
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %72, %blk_exit17 ]
  %calcOffset = add i32 %stack_addr, -12
  %40 = inttoptr i32 %calcOffset to i32*
  store i32 2206, i32* %40, align 4
  %calcOffset4 = add i32 %stack_addr, -16
  %41 = inttoptr i32 %calcOffset4 to i32*
  store i32 %2, i32* %41, align 4
  %42 = call i32 @fprintf(i32 %3, i32 1167, i32 %calcOffset4)
  %43 = inttoptr i32 %_param_0.0 to i32*
  %44 = load i32, i32* %43, align 4
  %.not = icmp eq i32 %44, -1
  br i1 %.not, label %brif_next9, label %blk_exit6

brif_next9:                                       ; preds = %loop_entry
  %45 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %brif_val11.not = icmp eq i8 %45, 0
  br i1 %brif_val11.not, label %brif_next12, label %blk_exit10

brif_next12:                                      ; preds = %brif_next9
  %46 = load i32, i32* inttoptr (i32 3332 to i32*), align 4
  %47 = sitofp i32 %46 to double
  %48 = fsub double 1.000000e+02, %47
  %49 = add i32 %_param_0.0, 44
  %50 = inttoptr i32 %49 to i32*
  %51 = load i32, i32* %50, align 4
  %52 = uitofp i32 %51 to double
  %53 = fmul double %48, %52
  %54 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %55 = uitofp i32 %54 to double
  %56 = fdiv double %53, %55
  br label %blk_exit5

blk_exit10:                                       ; preds = %brif_next9
  %57 = load i32, i32* inttoptr (i32 3352 to i32*), align 8
  %58 = sitofp i32 %57 to double
  %59 = fdiv double 1.000000e+02, %58
  br label %blk_exit5

blk_exit5:                                        ; preds = %blk_exit6, %blk_exit10, %brif_next12
  %_local_6.0 = phi double [ %56, %brif_next12 ], [ %59, %blk_exit10 ], [ %73, %blk_exit6 ]
  %calcOffset14 = add i32 %stack_addr, -32
  %60 = inttoptr i32 %calcOffset14 to double*
  store double %_local_6.0, double* %60, align 8
  %61 = call i32 @fprintf(i32 %3, i32 1561, i32 %calcOffset14)
  %calcOffset15 = add i32 %_param_0.0, 20
  %62 = inttoptr i32 %calcOffset15 to i32*
  %63 = load i32, i32* %62, align 4
  %select_cond.not = icmp eq i32 %63, 0
  %64 = select i1 %select_cond.not, i32 1492, i32 %63
  %calcOffset16 = add i32 %stack_addr, -48
  %65 = inttoptr i32 %calcOffset16 to i32*
  store i32 %64, i32* %65, align 4
  %66 = call i32 @fprintf(i32 %3, i32 1190, i32 %calcOffset16)
  %67 = call i32 @putc(i32 10, i32 %3)
  %calcOffset18 = add i32 %_param_0.0, 68
  %68 = inttoptr i32 %calcOffset18 to i32*
  %69 = load i32, i32* %68, align 4
  %70 = icmp eq i32 %69, 0
  br i1 %70, label %blk_exit17, label %brif_next20

blk_exit17:                                       ; preds = %brif_next20, %blk_exit5
  %calcOffset21 = add i32 %_param_0.0, 76
  %71 = inttoptr i32 %calcOffset21 to i32*
  %72 = load i32, i32* %71, align 4
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
define i32 @__main_void() #0 {
allocator:
  %0 = call i32 @main(i32 0, i32 0)
  ret i32 %0
}

; Function Attrs: argmemonly nofree nounwind willreturn writeonly
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #1

attributes #0 = { null_pointer_is_valid }
attributes #1 = { argmemonly nofree nounwind willreturn writeonly }

!0 = !{!"true"}
!1 = !{!"negative"}
