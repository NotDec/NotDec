; ModuleID = '/sn640/NotDec/test/lifting/wasm/cases/fortune.o3.wasm'
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
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -2960
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @getenv(i32 1497)
  store i32 %2, i32* inttoptr (i32 2224 to i32*), align 16
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

brif_next:                                        ; preds = %allocator
  %3 = call i32 @getenv(i32 1480)
  store i32 %3, i32* inttoptr (i32 2224 to i32*), align 16
  %brif_val1.not = icmp eq i32 %3, 0
  br i1 %brif_val1.not, label %brif_next2, label %blk_exit

brif_next2:                                       ; preds = %brif_next
  %4 = call i32 @getenv(i32 1509)
  store i32 %4, i32* inttoptr (i32 2224 to i32*), align 16
  %brif_val3.not = icmp eq i32 %4, 0
  br i1 %brif_val3.not, label %brif_next4, label %blk_exit

brif_next4:                                       ; preds = %brif_next2
  %5 = call i32 @getenv(i32 1504)
  store i32 %5, i32* inttoptr (i32 2224 to i32*), align 16
  br label %blk_exit

blk_exit:                                         ; preds = %brif_next4, %brif_next2, %brif_next, %allocator
  br label %loop_entry

loop_entry:                                       ; preds = %loop_entry, %blk_exit7, %blk_exit9, %blk_exit10, %blk_exit11, %blk_exit12, %blk_exit13, %blk_exit14, %blk_exit15, %blk_exit16, %blk_exit17, %blk_exit18, %blk_exit
  %_local_4.0 = phi i32 [ 0, %blk_exit ], [ %_local_4.0, %blk_exit18 ], [ %_local_4.0, %blk_exit7 ], [ %_local_4.0, %blk_exit17 ], [ %_local_4.0, %blk_exit16 ], [ %_local_4.0, %loop_entry ], [ %_local_4.0, %blk_exit15 ], [ %822, %blk_exit10 ], [ %_local_4.0, %blk_exit14 ], [ %_local_4.0, %blk_exit13 ], [ %_local_4.0, %blk_exit12 ], [ %_local_4.0, %blk_exit9 ], [ %_local_4.0, %blk_exit11 ]
  %_local_3.0 = phi i32 [ 0, %blk_exit ], [ %_local_3.0, %blk_exit18 ], [ %_local_3.0, %blk_exit7 ], [ %_local_3.0, %blk_exit17 ], [ %_local_3.0, %blk_exit16 ], [ 1, %loop_entry ], [ %_local_3.0, %blk_exit15 ], [ %_local_3.0, %blk_exit10 ], [ %_local_3.0, %blk_exit14 ], [ %_local_3.0, %blk_exit13 ], [ %_local_3.0, %blk_exit12 ], [ %_local_3.0, %blk_exit9 ], [ %_local_3.0, %blk_exit11 ]
  %6 = call i32 @getopt(i32 %_arg_0, i32 %_arg_1, i32 1024)
  switch i32 %6, label %blk_exit6 [
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
  %7 = load i32, i32* null, align 2147483648
  %8 = sub i32 %_arg_0, %7
  %9 = icmp eq i32 %8, 0
  br i1 %9, label %blk_exit43, label %brif_next45

blk_exit43:                                       ; preds = %blk_exit5
  %10 = load i8, i8* inttoptr (i32 2292 to i8*), align 4
  %11 = icmp eq i8 %10, 0
  br i1 %11, label %blk_exit219, label %brif_next221

blk_exit219:                                      ; preds = %blk_exit43
  %12 = load i8, i8* inttoptr (i32 2293 to i8*), align 1
  %13 = icmp eq i8 %12, 0
  br i1 %13, label %blk_exit228, label %brif_next230

blk_exit228:                                      ; preds = %blk_exit219
  %14 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %15 = icmp eq i32 %14, 0
  br i1 %15, label %blk_exit234, label %brif_next236

blk_exit234:                                      ; preds = %blk_exit228
  %16 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %17 = call i32 @strcmp(i32 1089, i32 1121)
  %18 = icmp eq i32 %17, 0
  br i1 %18, label %blk_exit258, label %brif_next260

blk_exit258:                                      ; preds = %brif_next260, %blk_exit234
  %_local_3.8 = phi i32 [ 0, %blk_exit234 ], [ %20, %brif_next260 ]
  %19 = or i32 %_local_3.8, %16
  br label %blk_exit218

brif_next260:                                     ; preds = %blk_exit234
  %20 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit258

brif_next236:                                     ; preds = %blk_exit228
  %21 = add i32 %0, -2560
  %22 = call i32 @strncpy(i32 %21, i32 %14, i32 512)
  %calcOffset237 = add i32 %0, -2049
  %23 = inttoptr i32 %calcOffset237 to i8*
  store i8 0, i8* %23, align 1
  br label %loop_entry239

loop_entry239:                                    ; preds = %brif_next251, %brif_next236
  %_local_3.6 = phi i32 [ %21, %brif_next236 ], [ %storemerge801, %brif_next251 ]
  %24 = icmp eq i32 %_local_3.6, 0
  br i1 %24, label %blk_exit238, label %brif_next241

blk_exit238:                                      ; preds = %brif_next241, %loop_entry239
  %25 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %26 = call i32 @strcmp(i32 1089, i32 1121)
  %27 = icmp eq i32 %26, 0
  br i1 %27, label %blk_exit255, label %brif_next257

blk_exit255:                                      ; preds = %brif_next257, %blk_exit238
  %_local_3.7 = phi i32 [ 0, %blk_exit238 ], [ %29, %brif_next257 ]
  %28 = or i32 %_local_3.7, %25
  br label %blk_exit218

brif_next257:                                     ; preds = %blk_exit238
  %29 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit255

brif_next241:                                     ; preds = %loop_entry239
  %30 = inttoptr i32 %_local_3.6 to i8*
  %31 = load i8, i8* %30, align 1
  %32 = icmp eq i8 %31, 0
  br i1 %32, label %blk_exit238, label %brif_next244

brif_next244:                                     ; preds = %brif_next241
  %33 = call i32 @strchr(i32 %_local_3.6, i32 58)
  %brif_val247.not = icmp eq i32 %33, 0
  br i1 %brif_val247.not, label %blk_exit245, label %blk_exit246

blk_exit245:                                      ; preds = %brif_next244, %blk_exit246
  %storemerge801 = phi i32 [ %41, %blk_exit246 ], [ 0, %brif_next244 ]
  %34 = call i32 @add_file(i32 -1, i32 %_local_3.6, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val250.not = icmp eq i32 %34, 0
  br i1 %brif_val250.not, label %brif_next251, label %blk_exit218

brif_next251:                                     ; preds = %blk_exit245
  %35 = add i32 %0, -2564
  %36 = call i32 @strncpy(i32 %35, i32 %_local_3.6, i32 2)
  %calcOffset252 = add i32 %0, -2562
  %37 = inttoptr i32 %calcOffset252 to i8*
  store i8 0, i8* %37, align 1
  %38 = call i32 @add_file(i32 -1, i32 %35, i32 0, i32 2268, i32 3328, i32 0)
  %39 = icmp eq i32 %38, 0
  br i1 %39, label %loop_entry239, label %blk_exit218

blk_exit246:                                      ; preds = %brif_next244
  %40 = inttoptr i32 %33 to i8*
  store i8 0, i8* %40, align 1
  %41 = add i32 %33, 1
  br label %blk_exit245

brif_next230:                                     ; preds = %blk_exit219
  %42 = call i32 @add_file(i32 -1, i32 1282, i32 0, i32 2268, i32 3328, i32 0)
  %43 = call i32 @strcmp(i32 1246, i32 1282)
  %44 = icmp eq i32 %43, 0
  br i1 %44, label %blk_exit231, label %brif_next233

blk_exit231:                                      ; preds = %brif_next233, %brif_next230
  %_local_3.5 = phi i32 [ 0, %brif_next230 ], [ %46, %brif_next233 ]
  %45 = or i32 %_local_3.5, %42
  br label %blk_exit218

brif_next233:                                     ; preds = %brif_next230
  %46 = call i32 @add_file(i32 -1, i32 1246, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit231

blk_exit218:                                      ; preds = %brif_next251, %blk_exit258, %blk_exit255, %blk_exit245, %blk_exit231, %blk_exit225
  %_param_0.2 = phi i32 [ %19, %blk_exit258 ], [ %28, %blk_exit255 ], [ %38, %brif_next251 ], [ %34, %blk_exit245 ], [ %45, %blk_exit231 ], [ %56, %blk_exit225 ]
  %47 = icmp eq i32 %_param_0.2, 0
  br i1 %47, label %blk_exit41, label %blk_exit42

brif_next221:                                     ; preds = %blk_exit43
  %48 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %49 = call i32 @add_file(i32 -1, i32 1282, i32 0, i32 2268, i32 3328, i32 0)
  %50 = or i32 %48, %49
  %51 = call i32 @strcmp(i32 1089, i32 1121)
  %52 = icmp eq i32 %51, 0
  br i1 %52, label %blk_exit222, label %brif_next224

blk_exit222:                                      ; preds = %brif_next224, %brif_next221
  %_param_0.3 = phi i32 [ 0, %brif_next221 ], [ %58, %brif_next224 ]
  %53 = or i32 %50, %_param_0.3
  %54 = call i32 @strcmp(i32 1246, i32 1282)
  %55 = icmp eq i32 %54, 0
  br i1 %55, label %blk_exit225, label %brif_next227

blk_exit225:                                      ; preds = %brif_next227, %blk_exit222
  %_local_3.4 = phi i32 [ 0, %blk_exit222 ], [ %57, %brif_next227 ]
  %56 = or i32 %53, %_local_3.4
  br label %blk_exit218

brif_next227:                                     ; preds = %blk_exit222
  %57 = call i32 @add_file(i32 -1, i32 1246, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit225

brif_next224:                                     ; preds = %brif_next221
  %58 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit222

blk_exit41:                                       ; preds = %blk_exit203, %blk_exit218, %blk_exit49, %blk_exit111
  %59 = load i8, i8* inttoptr (i32 2296 to i8*), align 8
  %brif_val271.not = icmp eq i8 %59, 0
  br i1 %brif_val271.not, label %brif_next272, label %blk_exit270

brif_next272:                                     ; preds = %blk_exit41
  %60 = inttoptr i32 %1 to i32*
  store i32 1795, i32* %60, align 4
  %61 = load i32, i32* null, align 2147483648
  %62 = call i32 @fprintf(i32 %61, i32 1191, i32 %1)
  br label %blk_exit270

blk_exit270:                                      ; preds = %brif_next272, %blk_exit41
  call void @exit(i32 1)
  unreachable

blk_exit42:                                       ; preds = %blk_exit218, %blk_exit110, %brif_next45
  %63 = icmp eq i32 %_local_4.0, 0
  br i1 %63, label %blk_exit35, label %brif_next264

blk_exit35:                                       ; preds = %blk_exit36, %blk_exit38, %blk_exit42
  %64 = call i32 @setlocale(i32 6, i32 2206)
  %65 = load i8, i8* inttoptr (i32 2228 to i8*), align 4
  %66 = icmp eq i8 %65, 0
  br i1 %66, label %blk_exit314, label %brif_next316

blk_exit314:                                      ; preds = %blk_exit35
  %67 = load i32, i32* inttoptr (i32 3328 to i32*), align 256
  %68 = icmp eq i32 %67, 0
  br i1 %68, label %blk_exit321, label %brif_next323

blk_exit321:                                      ; preds = %brif_next331, %blk_exit314
  %_local_5.3 = phi i32 [ 0, %blk_exit314 ], [ %560, %brif_next331 ]
  %_local_4.2 = phi i32 [ 0, %blk_exit314 ], [ %561, %brif_next331 ]
  %_param_1.4 = phi i32 [ 0, %blk_exit314 ], [ %563, %brif_next331 ]
  %69 = icmp ne i32 %_param_1.4, 100
  %70 = icmp eq i32 %_local_4.2, 0
  %or.cond808 = select i1 %69, i1 %70, i1 false
  br i1 %or.cond808, label %blk_exit31, label %blk_exit334

blk_exit31:                                       ; preds = %blk_exit321
  %calcOffset764 = add i32 %0, -2928
  %71 = inttoptr i32 %calcOffset764 to i32*
  store i32 %_param_1.4, i32* %71, align 4
  %72 = load i32, i32* null, align 2147483648
  %73 = call i32 @fprintf(i32 %72, i32 2012, i32 %calcOffset764)
  call void @exit(i32 1)
  unreachable

blk_exit334:                                      ; preds = %blk_exit321
  %or.cond809 = select i1 %69, i1 true, i1 %70
  br i1 %or.cond809, label %blk_exit339, label %blk_exit30

blk_exit339:                                      ; preds = %blk_exit334
  store i32 %_param_1.4, i32* inttoptr (i32 3332 to i32*), align 4
  %74 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %75 = icmp eq i8 %74, 0
  %or.cond = select i1 %75, i1 true, i1 %70
  br i1 %or.cond, label %blk_exit344, label %brif_next348

blk_exit344:                                      ; preds = %blk_exit349, %blk_exit339
  %76 = load i8, i8* inttoptr (i32 2264 to i8*), align 8
  %77 = icmp eq i8 %76, 0
  br i1 %77, label %blk_exit364, label %brif_next366

blk_exit364:                                      ; preds = %blk_exit344
  %78 = call i64 @time(i32 0)
  %79 = call i32 @getpid()
  %80 = trunc i64 %78 to i32
  %81 = add i32 %79, %80
  call void @srandom(i32 %81)
  br label %loop_entry404

loop_entry404:                                    ; preds = %blk_exit639, %blk_exit605, %blk_exit364
  %82 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %83 = inttoptr i32 %82 to i32*
  %84 = load i32, i32* %83, align 4
  %calcOffset409 = add i32 %82, 76
  %85 = inttoptr i32 %calcOffset409 to i32*
  %86 = load i32, i32* %85, align 4
  %87 = icmp eq i32 %86, 0
  br i1 %87, label %blk_exit408, label %brif_next411

blk_exit408:                                      ; preds = %blk_exit427, %blk_exit415, %brif_next438, %loop_entry434, %loop_entry404
  %_local_5.6 = phi i32 [ %84, %loop_entry404 ], [ -1, %loop_entry434 ], [ %504, %brif_next438 ], [ -1, %blk_exit415 ], [ %500, %blk_exit427 ]
  %_local_3.21 = phi i32 [ %82, %loop_entry404 ], [ %502, %loop_entry434 ], [ %502, %brif_next438 ], [ %498, %blk_exit415 ], [ %498, %blk_exit427 ]
  %.not779 = icmp eq i32 %_local_5.6, -1
  br i1 %.not779, label %brif_next442, label %blk_exit406

brif_next442:                                     ; preds = %blk_exit408
  %calcOffset443 = add i32 %_local_3.21, 76
  %88 = inttoptr i32 %calcOffset443 to i32*
  %89 = load i32, i32* %88, align 4
  %90 = icmp eq i32 %89, 0
  br i1 %90, label %blk_exit406, label %blk_exit407

blk_exit406:                                      ; preds = %loop_entry481, %blk_exit466, %brif_next442, %blk_exit408
  %_local_3.19 = phi i32 [ %_local_3.21, %brif_next442 ], [ %_local_3.20, %blk_exit466 ], [ %476, %loop_entry481 ], [ %_local_3.21, %blk_exit408 ]
  call void @get_tbl(i32 %_local_3.19)
  %91 = add i32 %_local_3.19, 44
  %92 = inttoptr i32 %91 to i32*
  %93 = load i32, i32* %92, align 4
  %94 = icmp eq i32 %93, 0
  br i1 %94, label %blk_exit29, label %brif_next488

blk_exit29:                                       ; preds = %blk_exit406
  %calcOffset765 = add i32 %0, -2912
  %95 = inttoptr i32 %calcOffset765 to i32*
  store i32 1814, i32* %95, align 4
  %96 = load i32, i32* null, align 2147483648
  %97 = call i32 @fprintf(i32 %96, i32 1191, i32 %calcOffset765)
  call void @exit(i32 1)
  unreachable

brif_next488:                                     ; preds = %blk_exit406
  %calcOffset490 = add i32 %_local_3.19, 68
  %98 = inttoptr i32 %calcOffset490 to i32*
  %99 = load i32, i32* %98, align 4
  %100 = icmp eq i32 %99, 0
  br i1 %100, label %blk_exit489, label %loop_entry493

blk_exit489:                                      ; preds = %blk_exit494, %brif_next488
  %_local_3.24 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  store i32 %_local_3.24, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset561 = add i32 %_local_3.24, 36
  %101 = inttoptr i32 %calcOffset561 to i8*
  %102 = load i8, i8* %101, align 1
  %103 = icmp eq i8 %102, 0
  br i1 %103, label %blk_exit28, label %brif_next563

blk_exit28:                                       ; preds = %blk_exit489
  call void @__assert_fail(i32 1229, i32 1342, i32 1621, i32 1081)
  unreachable

brif_next563:                                     ; preds = %blk_exit489
  %calcOffset565 = add i32 %_local_3.24, 12
  %104 = inttoptr i32 %calcOffset565 to i32*
  %105 = load i32, i32* %104, align 4
  %.not784 = icmp eq i32 %105, -1
  br i1 %.not784, label %brif_next567, label %brif_next563.blk_exit564_crit_edge

brif_next567:                                     ; preds = %brif_next563
  %106 = add i32 %_local_3.24, 44
  %107 = inttoptr i32 %106 to i32*
  %108 = load i32, i32* %107, align 4
  %calcOffset569 = add i32 %0, -1024
  %109 = inttoptr i32 %calcOffset569 to i64*
  %110 = add i32 %0, -1024
  %111 = inttoptr i32 %110 to i8*
  call void @llvm.memset.p0i8.i64(i8* %111, i8 0, i64 8, i1 true)
  %112 = call i32 @getenv(i32 1447)
  %113 = icmp eq i32 %112, 0
  br i1 %113, label %blk_exit570, label %brif_next572

blk_exit570:                                      ; preds = %brif_next567
  %114 = call i32 @getenv(i32 1522)
  %brif_val574.not = icmp eq i32 %114, 0
  br i1 %brif_val574.not, label %brif_next575, label %blk_exit573

brif_next575:                                     ; preds = %blk_exit570
  %115 = call i32 @fopen(i32 1208, i32 1414)
  %116 = icmp eq i32 %115, 0
  br i1 %116, label %blk_exit573, label %brif_next577

brif_next577:                                     ; preds = %brif_next575
  %117 = call i32 @fread(i32 %calcOffset569, i32 8, i32 1, i32 %115)
  %118 = call i32 @fclose(i32 %115)
  %.not791 = icmp eq i32 %117, 1
  br i1 %.not791, label %brif_next579, label %blk_exit573

brif_next579:                                     ; preds = %brif_next577
  %119 = load i64, i64* %109, align 8
  %120 = zext i32 %108 to i64
  %121 = urem i64 %119, %120
  %122 = trunc i64 %121 to i32
  br label %blk_exit564

blk_exit573:                                      ; preds = %brif_next577, %brif_next575, %blk_exit570
  %123 = call i32 @random()
  %124 = urem i32 %123, %108
  br label %blk_exit564

brif_next572:                                     ; preds = %brif_next567
  %125 = call i32 @atol(i32 %112)
  %126 = urem i32 %125, %108
  br label %blk_exit564

blk_exit564:                                      ; preds = %brif_next563.blk_exit564_crit_edge, %blk_exit573, %brif_next579, %brif_next572
  %.pre-phi813 = phi i32* [ %.pre812, %brif_next563.blk_exit564_crit_edge ], [ %107, %blk_exit573 ], [ %107, %brif_next579 ], [ %107, %brif_next572 ]
  %_local_5.16 = phi i32 [ %105, %brif_next563.blk_exit564_crit_edge ], [ %124, %blk_exit573 ], [ %122, %brif_next579 ], [ %126, %brif_next572 ]
  %127 = add i32 %_local_5.16, 1
  store i32 %127, i32* %104, align 4
  %128 = load i32, i32* %.pre-phi813, align 4
  %129 = icmp slt i32 %127, %128
  br i1 %129, label %blk_exit582, label %brif_next585

blk_exit582:                                      ; preds = %brif_next585, %blk_exit564
  %_local_5.17 = phi i32 [ %127, %blk_exit564 ], [ %349, %brif_next585 ]
  %calcOffset588 = add i32 %_local_3.24, 8
  %130 = inttoptr i32 %calcOffset588 to i32*
  %131 = load i32, i32* %130, align 4
  %brif_val589.not = icmp slt i32 %131, 0
  br i1 %brif_val589.not, label %brif_next590, label %blk_exit587

brif_next590:                                     ; preds = %blk_exit582
  %calcOffset591 = add i32 %_local_3.24, 28
  %132 = inttoptr i32 %calcOffset591 to i32*
  %133 = load i32, i32* %132, align 4
  %134 = call i32 @open(i32 %133, i32 67108864, i32 0)
  store i32 %134, i32* %130, align 4
  %brif_val593.not = icmp sgt i32 %134, -1
  br i1 %brif_val593.not, label %brif_next594, label %blk_exit27

brif_next594:                                     ; preds = %brif_next590
  %135 = load i32, i32* %104, align 4
  br label %blk_exit587

blk_exit27:                                       ; preds = %brif_next590
  call void @exit(i32 1)
  unreachable

blk_exit587:                                      ; preds = %brif_next594, %blk_exit582
  %_local_5.18 = phi i32 [ %135, %brif_next594 ], [ %_local_5.17, %blk_exit582 ]
  %_param_0.11 = phi i32 [ %134, %brif_next594 ], [ %131, %blk_exit582 ]
  %136 = shl i32 %_local_5.18, 2
  %137 = add i32 %136, 24
  %138 = zext i32 %137 to i64
  %139 = call i64 @lseek(i32 %_param_0.11, i64 %138, i32 0)
  %140 = load i32, i32* %130, align 4
  %141 = call i32 @read(i32 %140, i32 3360, i32 4)
  %brif_val597.not = icmp sgt i32 %141, -1
  br i1 %brif_val597.not, label %brif_next598, label %blk_exit26

brif_next598:                                     ; preds = %blk_exit587
  %142 = load i32, i32* %130, align 4
  %143 = call i32 @read(i32 %142, i32 3364, i32 4)
  %brif_val600.not = icmp sgt i32 %143, -1
  br i1 %brif_val600.not, label %brif_next601, label %blk_exit26

brif_next601:                                     ; preds = %brif_next598
  %144 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %145 = call i32 @ntohl(i32 %144)
  store i32 %145, i32* inttoptr (i32 3360 to i32*), align 32
  %146 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %147 = call i32 @ntohl(i32 %146)
  store i32 %147, i32* inttoptr (i32 3364 to i32*), align 4
  %148 = load i8, i8* inttoptr (i32 2273 to i8*), align 1
  %149 = icmp eq i8 %148, 0
  br i1 %149, label %blk_exit602, label %brif_next604

blk_exit602:                                      ; preds = %blk_exit605, %brif_next601
  %150 = load i8, i8* inttoptr (i32 2274 to i8*), align 2
  %151 = icmp eq i8 %150, 0
  br i1 %151, label %blk_exit636, label %brif_next638

blk_exit636:                                      ; preds = %blk_exit639, %blk_exit602
  %152 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset671 = add i32 %152, 16
  %153 = inttoptr i32 %calcOffset671 to i32*
  %154 = load i32, i32* %153, align 4
  %brif_val672.not = icmp eq i32 %154, 0
  br i1 %brif_val672.not, label %brif_next673, label %blk_exit670

brif_next673:                                     ; preds = %blk_exit636
  %calcOffset674 = add i32 %152, 4
  %155 = inttoptr i32 %calcOffset674 to i32*
  %156 = load i32, i32* %155, align 4
  %157 = call i32 @fdopen(i32 %156, i32 1194)
  store i32 %157, i32* %153, align 4
  %158 = icmp eq i32 %157, 0
  br i1 %158, label %blk_exit23, label %blk_exit670

blk_exit23:                                       ; preds = %brif_next673
  %calcOffset768 = add i32 %152, 24
  %159 = inttoptr i32 %calcOffset768 to i32*
  %160 = load i32, i32* %159, align 4
  call void @perror(i32 %160)
  call void @exit(i32 1)
  unreachable

blk_exit670:                                      ; preds = %brif_next673, %blk_exit636
  %_local_3.35 = phi i32 [ %157, %brif_next673 ], [ %154, %blk_exit636 ]
  %161 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %162 = call i32 @fseek(i32 %_local_3.35, i32 %161, i32 0)
  %163 = load i8, i8* inttoptr (i32 2295 to i8*), align 1
  %164 = icmp eq i8 %163, 0
  br i1 %164, label %blk_exit678, label %brif_next680

blk_exit678:                                      ; preds = %brif_next680, %blk_exit670
  store i32 0, i32* inttoptr (i32 2284 to i32*), align 4
  %165 = add i32 %0, -1024
  %166 = load i32, i32* %153, align 4
  %167 = call i32 @fgets(i32 %165, i32 1024, i32 %166)
  %brif_val686.not = icmp eq i32 %167, 0
  br i1 %brif_val686.not, label %brif_next687, label %blk_exit684

brif_next687:                                     ; preds = %blk_exit678
  %168 = load i32, i32* null, align 2147483648
  br label %blk_exit683

blk_exit683:                                      ; preds = %blk_exit719, %brif_next693, %brif_next687
  %_local_11.1 = phi i32 [ %168, %brif_next687 ], [ %219, %brif_next693 ], [ %219, %blk_exit719 ]
  %169 = call i32 @fflush(i32 %_local_11.1)
  %170 = load i8, i8* inttoptr (i32 2280 to i8*), align 8
  %171 = icmp eq i8 %170, 0
  br i1 %171, label %blk_exit19, label %brif_next730

brif_next730:                                     ; preds = %blk_exit683
  %172 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %173 = add i32 %172, 56
  %174 = inttoptr i32 %173 to i8*
  %175 = load i8, i8* %174, align 1
  %176 = and i8 %175, 3
  %brif_val734.not = icmp eq i8 %176, 0
  br i1 %brif_val734.not, label %brif_next735, label %blk_exit732

brif_next735:                                     ; preds = %brif_next730
  %177 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %178 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %179 = sub i32 %177, %178
  %180 = add i32 %179, -2
  br label %blk_exit731

blk_exit731:                                      ; preds = %blk_exit749, %brif_next753, %blk_exit736, %brif_next735
  %_local_3.37 = phi i32 [ %180, %brif_next735 ], [ 0, %blk_exit736 ], [ %_local_3.38, %brif_next753 ], [ %215, %blk_exit749 ]
  store i32 %_local_3.37, i32* inttoptr (i32 2284 to i32*), align 4
  %181 = sdiv i32 %_local_3.37, 20
  %182 = icmp sgt i32 %_local_3.37, 139
  %183 = select i1 %182, i32 %181, i32 6
  %184 = call i32 @sleep(i32 %183)
  br label %blk_exit19

blk_exit732:                                      ; preds = %brif_next730
  %calcOffset737 = add i32 %172, 16
  %185 = inttoptr i32 %calcOffset737 to i32*
  %186 = load i32, i32* %185, align 4
  %brif_val738.not = icmp eq i32 %186, 0
  br i1 %brif_val738.not, label %brif_next739, label %blk_exit736

brif_next739:                                     ; preds = %blk_exit732
  %calcOffset740 = add i32 %172, 4
  %187 = inttoptr i32 %calcOffset740 to i32*
  %188 = load i32, i32* %187, align 4
  %189 = call i32 @fdopen(i32 %188, i32 1194)
  store i32 %189, i32* %185, align 4
  %190 = icmp eq i32 %189, 0
  br i1 %190, label %blk_exit22, label %brif_next743

blk_exit22:                                       ; preds = %brif_next739
  %calcOffset769 = add i32 %172, 24
  %191 = inttoptr i32 %calcOffset769 to i32*
  %192 = load i32, i32* %191, align 4
  call void @perror(i32 %192)
  call void @exit(i32 1)
  unreachable

brif_next743:                                     ; preds = %brif_next739
  %193 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset744 = add i32 %193, 16
  %194 = inttoptr i32 %calcOffset744 to i32*
  %195 = load i32, i32* %194, align 4
  br label %blk_exit736

blk_exit736:                                      ; preds = %brif_next743, %blk_exit732
  %_local_5.21 = phi i32 [ %195, %brif_next743 ], [ %186, %blk_exit732 ]
  %196 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %197 = call i32 @fseek(i32 %_local_5.21, i32 %196, i32 0)
  %198 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset745 = add i32 %198, 16
  %199 = inttoptr i32 %calcOffset745 to i32*
  %200 = load i32, i32* %199, align 4
  %201 = call i32 @fgets(i32 %165, i32 1024, i32 %200)
  %202 = icmp eq i32 %201, 0
  br i1 %202, label %blk_exit731, label %loop_entry748

loop_entry748:                                    ; preds = %blk_exit736, %blk_exit749
  %_local_3.38 = phi i32 [ %215, %blk_exit749 ], [ 0, %blk_exit736 ]
  %203 = inttoptr i32 %165 to i8*
  %204 = load i8, i8* %203, align 1
  %205 = sext i8 %204 to i32
  %206 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %207 = add i32 %206, 60
  %208 = inttoptr i32 %207 to i8*
  %209 = load i8, i8* %208, align 1
  %210 = zext i8 %209 to i32
  %.not786 = icmp eq i32 %205, %210
  br i1 %.not786, label %brif_next753, label %blk_exit749

brif_next753:                                     ; preds = %loop_entry748
  %calcOffset754 = add i32 %0, -1023
  %211 = inttoptr i32 %calcOffset754 to i8*
  %212 = load i8, i8* %211, align 1
  %213 = icmp eq i8 %212, 10
  br i1 %213, label %blk_exit731, label %blk_exit749

blk_exit749:                                      ; preds = %brif_next753, %loop_entry748
  %214 = call i32 @strlen(i32 %165)
  %215 = add i32 %214, %_local_3.38
  %calcOffset757 = add i32 %206, 16
  %216 = inttoptr i32 %calcOffset757 to i32*
  %217 = load i32, i32* %216, align 4
  %218 = call i32 @fgets(i32 %165, i32 1024, i32 %217)
  %brif_val758.not = icmp eq i32 %218, 0
  br i1 %brif_val758.not, label %blk_exit731, label %loop_entry748

blk_exit684:                                      ; preds = %blk_exit678
  %219 = load i32, i32* null, align 2147483648
  br label %loop_entry688

loop_entry688:                                    ; preds = %blk_exit719, %blk_exit684
  %220 = inttoptr i32 %165 to i8*
  %221 = load i8, i8* %220, align 1
  %calcOffset691 = add i32 %152, 60
  %222 = inttoptr i32 %calcOffset691 to i8*
  %223 = load i8, i8* %222, align 1
  %.not787 = icmp eq i8 %221, %223
  br i1 %.not787, label %brif_next693, label %blk_exit689

brif_next693:                                     ; preds = %loop_entry688
  %calcOffset694 = add i32 %0, -1023
  %224 = inttoptr i32 %calcOffset694 to i8*
  %225 = load i8, i8* %224, align 1
  %226 = icmp eq i8 %225, 10
  br i1 %226, label %blk_exit683, label %blk_exit689

blk_exit689:                                      ; preds = %brif_next693, %loop_entry688
  %calcOffset698 = add i32 %152, 56
  %227 = inttoptr i32 %calcOffset698 to i8*
  %228 = load i8, i8* %227, align 1
  %229 = and i8 %228, 4
  %brif_val699.not.not = icmp eq i8 %229, 0
  %230 = icmp eq i8 %221, 0
  %or.cond804 = select i1 %brif_val699.not.not, i1 true, i1 %230
  br i1 %or.cond804, label %blk_exit697, label %loop_entry703

blk_exit697:                                      ; preds = %blk_exit704, %blk_exit689
  %calcOffset721 = add i32 %152, 38
  %231 = inttoptr i32 %calcOffset721 to i8*
  %232 = load i8, i8* %231, align 1
  %233 = icmp eq i8 %232, 0
  br i1 %233, label %blk_exit720, label %brif_next723

blk_exit720:                                      ; preds = %brif_next723, %blk_exit697
  %234 = call i32 @fputs(i32 %165, i32 %219)
  br label %blk_exit719

blk_exit719:                                      ; preds = %blk_exit720, %brif_next725
  %235 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %236 = add i32 %235, 1
  store i32 %236, i32* inttoptr (i32 2284 to i32*), align 4
  %237 = load i32, i32* %153, align 4
  %238 = call i32 @fgets(i32 %165, i32 1024, i32 %237)
  %brif_val727.not = icmp eq i32 %238, 0
  br i1 %brif_val727.not, label %blk_exit683, label %loop_entry688

brif_next723:                                     ; preds = %blk_exit697
  %239 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val724.not = icmp eq i8 %239, 0
  br i1 %brif_val724.not, label %brif_next725, label %blk_exit720

brif_next725:                                     ; preds = %brif_next723
  %240 = call i32 @strdup(i32 %165)
  %241 = call i32 @fputs(i32 %240, i32 %219)
  call void @free(i32 %240)
  br label %blk_exit719

loop_entry703:                                    ; preds = %blk_exit689, %blk_exit704
  %_local_5.20 = phi i32 [ %250, %blk_exit704 ], [ %165, %blk_exit689 ]
  %_local_3.36.in = phi i8 [ %252, %blk_exit704 ], [ %221, %blk_exit689 ]
  %_local_3.36 = zext i8 %_local_3.36.in to i32
  %242 = shl nuw i32 %_local_3.36, 24
  %243 = sext i8 %_local_3.36.in to i32
  %244 = add nsw i32 %243, -65
  %245 = icmp ult i32 %244, 26
  %brif_val709.not = icmp sgt i32 %242, -16777216
  %or.cond805 = select i1 %245, i1 %brif_val709.not, i1 false
  br i1 %or.cond805, label %blk_exit705, label %blk_exit706

blk_exit705:                                      ; preds = %blk_exit706, %loop_entry703
  %storemerge788 = phi i8 [ 65, %loop_entry703 ], [ 97, %blk_exit706 ]
  %storemerge = phi i8 [ -52, %loop_entry703 ], [ -84, %blk_exit706 ]
  %246 = add i8 %storemerge, %_local_3.36.in
  %247 = urem i8 %246, 26
  %248 = add nuw nsw i8 %247, %storemerge788
  %249 = inttoptr i32 %_local_5.20 to i8*
  store i8 %248, i8* %249, align 1
  br label %blk_exit704

blk_exit704:                                      ; preds = %blk_exit705, %blk_exit706
  %250 = add i32 %_local_5.20, 1
  %251 = inttoptr i32 %250 to i8*
  %252 = load i8, i8* %251, align 1
  %brif_val717.not = icmp eq i8 %252, 0
  br i1 %brif_val717.not, label %blk_exit697, label %loop_entry703

blk_exit706:                                      ; preds = %loop_entry703
  %253 = add nsw i32 %243, -97
  %254 = icmp ult i32 %253, 26
  br i1 %254, label %blk_exit705, label %blk_exit704

brif_next680:                                     ; preds = %blk_exit670
  %calcOffset681 = add i32 %152, 20
  %255 = inttoptr i32 %calcOffset681 to i32*
  %256 = load i32, i32* %255, align 4
  %calcOffset682 = add i32 %0, -2896
  %257 = inttoptr i32 %calcOffset682 to i32*
  store i32 %256, i32* %257, align 4
  %258 = call i32 @printf(i32 2066, i32 %calcOffset682)
  br label %blk_exit678

brif_next638:                                     ; preds = %blk_exit602
  %259 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %260 = add i32 %259, 56
  %261 = inttoptr i32 %260 to i8*
  %262 = load i8, i8* %261, align 1
  %263 = and i8 %262, 3
  %brif_val642.not = icmp eq i8 %263, 0
  br i1 %brif_val642.not, label %brif_next643, label %blk_exit640

brif_next643:                                     ; preds = %brif_next638
  %264 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %265 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %266 = sub i32 %264, %265
  %267 = add i32 %266, -2
  br label %blk_exit639

blk_exit639:                                      ; preds = %blk_exit657, %brif_next661, %blk_exit644, %brif_next643
  %_local_3.33 = phi i32 [ %267, %brif_next643 ], [ 0, %blk_exit644 ], [ %_local_3.34, %brif_next661 ], [ %300, %blk_exit657 ]
  store i32 %_local_3.33, i32* inttoptr (i32 2284 to i32*), align 4
  %268 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %.not785 = icmp sgt i32 %_local_3.33, %268
  br i1 %.not785, label %blk_exit636, label %loop_entry404

blk_exit640:                                      ; preds = %brif_next638
  %calcOffset645 = add i32 %259, 16
  %269 = inttoptr i32 %calcOffset645 to i32*
  %270 = load i32, i32* %269, align 4
  %brif_val646.not = icmp eq i32 %270, 0
  br i1 %brif_val646.not, label %brif_next647, label %blk_exit644

brif_next647:                                     ; preds = %blk_exit640
  %calcOffset648 = add i32 %259, 4
  %271 = inttoptr i32 %calcOffset648 to i32*
  %272 = load i32, i32* %271, align 4
  %273 = call i32 @fdopen(i32 %272, i32 1194)
  store i32 %273, i32* %269, align 4
  %274 = icmp eq i32 %273, 0
  br i1 %274, label %blk_exit24, label %brif_next651

blk_exit24:                                       ; preds = %brif_next647
  %calcOffset767 = add i32 %259, 24
  %275 = inttoptr i32 %calcOffset767 to i32*
  %276 = load i32, i32* %275, align 4
  call void @perror(i32 %276)
  call void @exit(i32 1)
  unreachable

brif_next651:                                     ; preds = %brif_next647
  %277 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset652 = add i32 %277, 16
  %278 = inttoptr i32 %calcOffset652 to i32*
  %279 = load i32, i32* %278, align 4
  br label %blk_exit644

blk_exit644:                                      ; preds = %brif_next651, %blk_exit640
  %_local_5.19 = phi i32 [ %279, %brif_next651 ], [ %270, %blk_exit640 ]
  %280 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %281 = call i32 @fseek(i32 %_local_5.19, i32 %280, i32 0)
  %282 = add i32 %0, -1024
  %283 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset653 = add i32 %283, 16
  %284 = inttoptr i32 %calcOffset653 to i32*
  %285 = load i32, i32* %284, align 4
  %286 = call i32 @fgets(i32 %282, i32 1024, i32 %285)
  %287 = icmp eq i32 %286, 0
  br i1 %287, label %blk_exit639, label %loop_entry656

loop_entry656:                                    ; preds = %blk_exit644, %blk_exit657
  %_local_3.34 = phi i32 [ %300, %blk_exit657 ], [ 0, %blk_exit644 ]
  %288 = inttoptr i32 %282 to i8*
  %289 = load i8, i8* %288, align 1
  %290 = sext i8 %289 to i32
  %291 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %292 = add i32 %291, 60
  %293 = inttoptr i32 %292 to i8*
  %294 = load i8, i8* %293, align 1
  %295 = zext i8 %294 to i32
  %.not789 = icmp eq i32 %290, %295
  br i1 %.not789, label %brif_next661, label %blk_exit657

brif_next661:                                     ; preds = %loop_entry656
  %calcOffset662 = add i32 %0, -1023
  %296 = inttoptr i32 %calcOffset662 to i8*
  %297 = load i8, i8* %296, align 1
  %298 = icmp eq i8 %297, 10
  br i1 %298, label %blk_exit639, label %blk_exit657

blk_exit657:                                      ; preds = %brif_next661, %loop_entry656
  %299 = call i32 @strlen(i32 %282)
  %300 = add i32 %299, %_local_3.34
  %calcOffset665 = add i32 %291, 16
  %301 = inttoptr i32 %calcOffset665 to i32*
  %302 = load i32, i32* %301, align 4
  %303 = call i32 @fgets(i32 %282, i32 1024, i32 %302)
  %brif_val666.not = icmp eq i32 %303, 0
  br i1 %brif_val666.not, label %blk_exit639, label %loop_entry656

brif_next604:                                     ; preds = %brif_next601
  %304 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %305 = add i32 %304, 56
  %306 = inttoptr i32 %305 to i8*
  %307 = load i8, i8* %306, align 1
  %308 = and i8 %307, 3
  %brif_val608.not = icmp eq i8 %308, 0
  br i1 %brif_val608.not, label %brif_next609, label %blk_exit606

brif_next609:                                     ; preds = %brif_next604
  %309 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %310 = sub i32 %147, %309
  %311 = add i32 %310, -2
  br label %blk_exit605

blk_exit605:                                      ; preds = %blk_exit623, %brif_next627, %blk_exit610, %brif_next609
  %_local_3.31 = phi i32 [ %311, %brif_next609 ], [ 0, %blk_exit610 ], [ %_local_3.32, %brif_next627 ], [ %345, %blk_exit623 ]
  store i32 %_local_3.31, i32* inttoptr (i32 2284 to i32*), align 4
  %312 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %313 = icmp sgt i32 %_local_3.31, %312
  br i1 %313, label %loop_entry404, label %blk_exit602

blk_exit606:                                      ; preds = %brif_next604
  %calcOffset611 = add i32 %304, 16
  %314 = inttoptr i32 %calcOffset611 to i32*
  %315 = load i32, i32* %314, align 4
  %brif_val612.not = icmp eq i32 %315, 0
  br i1 %brif_val612.not, label %brif_next613, label %blk_exit610

brif_next613:                                     ; preds = %blk_exit606
  %calcOffset614 = add i32 %304, 4
  %316 = inttoptr i32 %calcOffset614 to i32*
  %317 = load i32, i32* %316, align 4
  %318 = call i32 @fdopen(i32 %317, i32 1194)
  store i32 %318, i32* %314, align 4
  %319 = icmp eq i32 %318, 0
  br i1 %319, label %blk_exit25, label %brif_next617

blk_exit25:                                       ; preds = %brif_next613
  %calcOffset766 = add i32 %304, 24
  %320 = inttoptr i32 %calcOffset766 to i32*
  %321 = load i32, i32* %320, align 4
  call void @perror(i32 %321)
  call void @exit(i32 1)
  unreachable

brif_next617:                                     ; preds = %brif_next613
  %322 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset618 = add i32 %322, 16
  %323 = inttoptr i32 %calcOffset618 to i32*
  %324 = load i32, i32* %323, align 4
  br label %blk_exit610

blk_exit610:                                      ; preds = %brif_next617, %blk_exit606
  %_param_0.12 = phi i32 [ %324, %brif_next617 ], [ %315, %blk_exit606 ]
  %325 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %326 = call i32 @fseek(i32 %_param_0.12, i32 %325, i32 0)
  %327 = add i32 %0, -1024
  %328 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset619 = add i32 %328, 16
  %329 = inttoptr i32 %calcOffset619 to i32*
  %330 = load i32, i32* %329, align 4
  %331 = call i32 @fgets(i32 %327, i32 1024, i32 %330)
  %332 = icmp eq i32 %331, 0
  br i1 %332, label %blk_exit605, label %loop_entry622

loop_entry622:                                    ; preds = %blk_exit610, %blk_exit623
  %_local_3.32 = phi i32 [ %345, %blk_exit623 ], [ 0, %blk_exit610 ]
  %333 = inttoptr i32 %327 to i8*
  %334 = load i8, i8* %333, align 1
  %335 = sext i8 %334 to i32
  %336 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %337 = add i32 %336, 60
  %338 = inttoptr i32 %337 to i8*
  %339 = load i8, i8* %338, align 1
  %340 = zext i8 %339 to i32
  %.not790 = icmp eq i32 %335, %340
  br i1 %.not790, label %brif_next627, label %blk_exit623

brif_next627:                                     ; preds = %loop_entry622
  %calcOffset628 = add i32 %0, -1023
  %341 = inttoptr i32 %calcOffset628 to i8*
  %342 = load i8, i8* %341, align 1
  %343 = icmp eq i8 %342, 10
  br i1 %343, label %blk_exit605, label %blk_exit623

blk_exit623:                                      ; preds = %brif_next627, %loop_entry622
  %344 = call i32 @strlen(i32 %327)
  %345 = add i32 %344, %_local_3.32
  %calcOffset631 = add i32 %336, 16
  %346 = inttoptr i32 %calcOffset631 to i32*
  %347 = load i32, i32* %346, align 4
  %348 = call i32 @fgets(i32 %327, i32 1024, i32 %347)
  %brif_val632.not = icmp eq i32 %348, 0
  br i1 %brif_val632.not, label %blk_exit605, label %loop_entry622

blk_exit26:                                       ; preds = %brif_next598, %blk_exit587
  call void @exit(i32 1)
  unreachable

brif_next585:                                     ; preds = %blk_exit564
  %349 = sub i32 %127, %128
  store i32 %349, i32* %104, align 4
  br label %blk_exit582

brif_next563.blk_exit564_crit_edge:               ; preds = %brif_next563
  %.pre = add i32 %_local_3.24, 44
  %.pre812 = inttoptr i32 %.pre to i32*
  br label %blk_exit564

loop_entry493:                                    ; preds = %brif_next488, %blk_exit494
  %_local_3.25 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  %350 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %351 = icmp eq i8 %350, 0
  br i1 %351, label %blk_exit495, label %brif_next497

blk_exit495:                                      ; preds = %loop_entry493
  call void @get_tbl(i32 %_local_3.25)
  %352 = add i32 %_local_3.25, 44
  %353 = inttoptr i32 %352 to i32*
  %354 = load i32, i32* %353, align 4
  %calcOffset536 = add i32 %0, -1024
  %355 = inttoptr i32 %calcOffset536 to i64*
  %356 = add i32 %0, -1024
  %357 = inttoptr i32 %356 to i8*
  call void @llvm.memset.p0i8.i64(i8* %357, i8 0, i64 8, i1 true)
  %358 = call i32 @getenv(i32 1447)
  %359 = icmp eq i32 %358, 0
  br i1 %359, label %blk_exit538, label %brif_next540

blk_exit538:                                      ; preds = %blk_exit495
  %360 = call i32 @getenv(i32 1522)
  %brif_val542.not = icmp eq i32 %360, 0
  br i1 %brif_val542.not, label %brif_next543, label %blk_exit541

brif_next543:                                     ; preds = %blk_exit538
  %361 = call i32 @fopen(i32 1208, i32 1414)
  %362 = icmp eq i32 %361, 0
  br i1 %362, label %blk_exit541, label %brif_next545

brif_next545:                                     ; preds = %brif_next543
  %363 = call i32 @fread(i32 %calcOffset536, i32 8, i32 1, i32 %361)
  %364 = call i32 @fclose(i32 %361)
  %.not794 = icmp eq i32 %363, 1
  br i1 %.not794, label %brif_next547, label %blk_exit541

brif_next547:                                     ; preds = %brif_next545
  %365 = load i64, i64* %355, align 8
  %366 = zext i32 %354 to i64
  %367 = urem i64 %365, %366
  %368 = trunc i64 %367 to i32
  br label %blk_exit537

blk_exit541:                                      ; preds = %brif_next545, %brif_next543, %blk_exit538
  %369 = call i32 @random()
  %370 = urem i32 %369, %354
  br label %blk_exit537

blk_exit537:                                      ; preds = %blk_exit541, %brif_next547, %brif_next540
  %_local_5.14 = phi i32 [ %370, %blk_exit541 ], [ %368, %brif_next547 ], [ %384, %brif_next540 ]
  %calcOffset549 = add i32 %_local_3.25, 68
  %371 = inttoptr i32 %calcOffset549 to i32*
  %372 = load i32, i32* %371, align 4
  %373 = add i32 %372, 44
  %374 = inttoptr i32 %373 to i32*
  %375 = load i32, i32* %374, align 4
  %376 = icmp slt i32 %_local_5.14, %375
  br i1 %376, label %blk_exit494, label %loop_entry553

loop_entry553:                                    ; preds = %blk_exit537, %loop_entry553
  %_local_5.15 = phi i32 [ %_local_5.14, %blk_exit537 ], [ %377, %loop_entry553 ]
  %_local_3.30 = phi i32 [ %372, %blk_exit537 ], [ %379, %loop_entry553 ]
  %_param_0.10 = phi i32 [ %375, %blk_exit537 ], [ %382, %loop_entry553 ]
  %377 = sub i32 %_local_5.15, %_param_0.10
  %calcOffset554 = add i32 %_local_3.30, 76
  %378 = inttoptr i32 %calcOffset554 to i32*
  %379 = load i32, i32* %378, align 4
  %380 = add i32 %379, 44
  %381 = inttoptr i32 %380 to i32*
  %382 = load i32, i32* %381, align 4
  %.not793 = icmp slt i32 %377, %382
  br i1 %.not793, label %blk_exit494, label %loop_entry553

brif_next540:                                     ; preds = %blk_exit495
  %383 = call i32 @atol(i32 %358)
  %384 = urem i32 %383, %354
  br label %blk_exit537

blk_exit494:                                      ; preds = %loop_entry553, %loop_entry524, %blk_exit537, %blk_exit515, %blk_exit500
  %_local_3.26 = phi i32 [ %372, %blk_exit537 ], [ %379, %loop_entry553 ], [ %406, %blk_exit500 ], [ %_local_3.27, %blk_exit515 ], [ %427, %loop_entry524 ]
  %calcOffset558 = add i32 %_local_3.26, 68
  %385 = inttoptr i32 %calcOffset558 to i32*
  %386 = load i32, i32* %385, align 4
  %brif_val559.not = icmp eq i32 %386, 0
  br i1 %brif_val559.not, label %blk_exit489, label %loop_entry493

brif_next497:                                     ; preds = %loop_entry493
  %calcOffset498 = add i32 %_local_3.25, 64
  %387 = inttoptr i32 %calcOffset498 to i32*
  %388 = load i32, i32* %387, align 4
  %calcOffset499 = add i32 %0, -1024
  %389 = inttoptr i32 %calcOffset499 to i64*
  %390 = add i32 %0, -1024
  %391 = inttoptr i32 %390 to i8*
  call void @llvm.memset.p0i8.i64(i8* %391, i8 0, i64 8, i1 true)
  %392 = call i32 @getenv(i32 1447)
  %393 = icmp eq i32 %392, 0
  br i1 %393, label %blk_exit501, label %brif_next503

blk_exit501:                                      ; preds = %brif_next497
  %394 = call i32 @getenv(i32 1522)
  %brif_val505.not = icmp eq i32 %394, 0
  br i1 %brif_val505.not, label %brif_next506, label %blk_exit504

brif_next506:                                     ; preds = %blk_exit501
  %395 = call i32 @fopen(i32 1208, i32 1414)
  %396 = icmp eq i32 %395, 0
  br i1 %396, label %blk_exit504, label %brif_next508

brif_next508:                                     ; preds = %brif_next506
  %397 = call i32 @fread(i32 %calcOffset499, i32 8, i32 1, i32 %395)
  %398 = call i32 @fclose(i32 %395)
  %.not792 = icmp eq i32 %397, 1
  br i1 %.not792, label %brif_next510, label %blk_exit504

brif_next510:                                     ; preds = %brif_next508
  %399 = load i64, i64* %389, align 8
  %400 = zext i32 %388 to i64
  %401 = urem i64 %399, %400
  %402 = trunc i64 %401 to i32
  br label %blk_exit500

blk_exit504:                                      ; preds = %brif_next508, %brif_next506, %blk_exit501
  %403 = call i32 @random()
  %404 = urem i32 %403, %388
  br label %blk_exit500

blk_exit500:                                      ; preds = %blk_exit504, %brif_next510, %brif_next503
  %_local_5.10 = phi i32 [ %404, %blk_exit504 ], [ %402, %brif_next510 ], [ %434, %brif_next503 ]
  %calcOffset512 = add i32 %_local_3.25, 68
  %405 = inttoptr i32 %calcOffset512 to i32*
  %406 = load i32, i32* %405, align 4
  %407 = icmp eq i32 %_local_5.10, 0
  br i1 %407, label %blk_exit494, label %brif_next514

brif_next514:                                     ; preds = %blk_exit500
  %408 = add i32 %_local_5.10, -1
  %409 = and i32 %_local_5.10, 7
  %410 = icmp eq i32 %409, 0
  br i1 %410, label %blk_exit515, label %loop_entry518

blk_exit515:                                      ; preds = %loop_entry518, %brif_next514
  %_local_5.11 = phi i32 [ %_local_5.10, %brif_next514 ], [ %429, %loop_entry518 ]
  %_local_3.27 = phi i32 [ %406, %brif_next514 ], [ %431, %loop_entry518 ]
  %411 = icmp ult i32 %408, 7
  br i1 %411, label %blk_exit494, label %loop_entry524

loop_entry524:                                    ; preds = %blk_exit515, %loop_entry524
  %_local_5.13 = phi i32 [ %_local_5.11, %blk_exit515 ], [ %428, %loop_entry524 ]
  %_local_3.29 = phi i32 [ %_local_3.27, %blk_exit515 ], [ %427, %loop_entry524 ]
  %calcOffset525 = add i32 %_local_3.29, 76
  %412 = inttoptr i32 %calcOffset525 to i32*
  %413 = load i32, i32* %412, align 4
  %calcOffset526 = add i32 %413, 76
  %414 = inttoptr i32 %calcOffset526 to i32*
  %415 = load i32, i32* %414, align 4
  %calcOffset527 = add i32 %415, 76
  %416 = inttoptr i32 %calcOffset527 to i32*
  %417 = load i32, i32* %416, align 4
  %calcOffset528 = add i32 %417, 76
  %418 = inttoptr i32 %calcOffset528 to i32*
  %419 = load i32, i32* %418, align 4
  %calcOffset529 = add i32 %419, 76
  %420 = inttoptr i32 %calcOffset529 to i32*
  %421 = load i32, i32* %420, align 4
  %calcOffset530 = add i32 %421, 76
  %422 = inttoptr i32 %calcOffset530 to i32*
  %423 = load i32, i32* %422, align 4
  %calcOffset531 = add i32 %423, 76
  %424 = inttoptr i32 %calcOffset531 to i32*
  %425 = load i32, i32* %424, align 4
  %calcOffset532 = add i32 %425, 76
  %426 = inttoptr i32 %calcOffset532 to i32*
  %427 = load i32, i32* %426, align 4
  %428 = add i32 %_local_5.13, -8
  %brif_val533.not = icmp eq i32 %428, 0
  br i1 %brif_val533.not, label %blk_exit494, label %loop_entry524

loop_entry518:                                    ; preds = %brif_next514, %loop_entry518
  %_local_5.12 = phi i32 [ %_local_5.10, %brif_next514 ], [ %429, %loop_entry518 ]
  %_local_3.28 = phi i32 [ %406, %brif_next514 ], [ %431, %loop_entry518 ]
  %_param_0.9 = phi i32 [ %409, %brif_next514 ], [ %432, %loop_entry518 ]
  %429 = add i32 %_local_5.12, -1
  %calcOffset519 = add i32 %_local_3.28, 76
  %430 = inttoptr i32 %calcOffset519 to i32*
  %431 = load i32, i32* %430, align 4
  %432 = add i32 %_param_0.9, -1
  %brif_val520.not = icmp eq i32 %432, 0
  br i1 %brif_val520.not, label %blk_exit515, label %loop_entry518

brif_next503:                                     ; preds = %brif_next497
  %433 = call i32 @atol(i32 %392)
  %434 = urem i32 %433, %388
  br label %blk_exit500

blk_exit407:                                      ; preds = %brif_next442, %brif_next411
  %_local_3.20 = phi i32 [ %_local_3.21, %brif_next442 ], [ %82, %brif_next411 ]
  %435 = load i8, i8* inttoptr (i32 3336 to i8*), align 8
  %brif_val447.not = icmp eq i8 %435, 0
  br i1 %brif_val447.not, label %brif_next448, label %blk_exit446

brif_next448:                                     ; preds = %blk_exit407
  store i32 -1, i32* inttoptr (i32 3348 to i32*), align 4
  call void @llvm.memset.p0i8.i64(i8* inttoptr (i32 3340 to i8*), i8 0, i64 8, i1 true)
  br label %loop_entry449

loop_entry449:                                    ; preds = %blk_exit450, %brif_next448
  %_local_5.7 = phi i32 [ %_local_3.20, %brif_next448 ], [ %452, %blk_exit450 ]
  call void @get_tbl(i32 %_local_5.7)
  %436 = inttoptr i32 %_local_5.7 to i32*
  %437 = load i32, i32* %436, align 4
  %.not780 = icmp eq i32 %437, -1
  br i1 %.not780, label %brif_next453, label %blk_exit450

brif_next453:                                     ; preds = %loop_entry449
  %438 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %439 = add i32 %_local_5.7, 44
  %440 = inttoptr i32 %439 to i32*
  %441 = load i32, i32* %440, align 4
  %442 = add i32 %438, %441
  store i32 %442, i32* inttoptr (i32 3340 to i32*), align 4
  %443 = load i32, i32* inttoptr (i32 3344 to i32*), align 16
  %444 = add i32 %_local_5.7, 48
  %445 = inttoptr i32 %444 to i32*
  %446 = load i32, i32* %445, align 4
  %.not781 = icmp ult i32 %443, %446
  br i1 %.not781, label %brif_next458, label %blk_exit455

brif_next458:                                     ; preds = %brif_next453
  store i32 %446, i32* inttoptr (i32 3344 to i32*), align 16
  br label %blk_exit455

blk_exit455:                                      ; preds = %brif_next458, %brif_next453
  %447 = load i32, i32* inttoptr (i32 3348 to i32*), align 4
  %448 = add i32 %_local_5.7, 52
  %449 = inttoptr i32 %448 to i32*
  %450 = load i32, i32* %449, align 4
  %.not782 = icmp ugt i32 %447, %450
  br i1 %.not782, label %brif_next461, label %blk_exit450

brif_next461:                                     ; preds = %blk_exit455
  store i32 %450, i32* inttoptr (i32 3348 to i32*), align 4
  br label %blk_exit450

blk_exit450:                                      ; preds = %brif_next461, %blk_exit455, %loop_entry449
  %calcOffset462 = add i32 %_local_5.7, 76
  %451 = inttoptr i32 %calcOffset462 to i32*
  %452 = load i32, i32* %451, align 4
  %brif_val463.not = icmp eq i32 %452, 0
  br i1 %brif_val463.not, label %brif_next464, label %loop_entry449

brif_next464:                                     ; preds = %blk_exit450
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  br label %blk_exit446

blk_exit446:                                      ; preds = %brif_next464, %blk_exit407
  %453 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %calcOffset465 = add i32 %0, -1024
  %454 = inttoptr i32 %calcOffset465 to i64*
  %455 = add i32 %0, -1024
  %456 = inttoptr i32 %455 to i8*
  call void @llvm.memset.p0i8.i64(i8* %456, i8 0, i64 8, i1 true)
  %457 = call i32 @getenv(i32 1447)
  %458 = icmp eq i32 %457, 0
  br i1 %458, label %blk_exit467, label %brif_next469

blk_exit467:                                      ; preds = %blk_exit446
  %459 = call i32 @getenv(i32 1522)
  %brif_val471.not = icmp eq i32 %459, 0
  br i1 %brif_val471.not, label %brif_next472, label %blk_exit470

brif_next472:                                     ; preds = %blk_exit467
  %460 = call i32 @fopen(i32 1208, i32 1414)
  %461 = icmp eq i32 %460, 0
  br i1 %461, label %blk_exit470, label %brif_next474

brif_next474:                                     ; preds = %brif_next472
  %462 = call i32 @fread(i32 %calcOffset465, i32 8, i32 1, i32 %460)
  %463 = call i32 @fclose(i32 %460)
  %.not795 = icmp eq i32 %462, 1
  br i1 %.not795, label %brif_next476, label %blk_exit470

brif_next476:                                     ; preds = %brif_next474
  %464 = load i64, i64* %454, align 8
  %465 = zext i32 %453 to i64
  %466 = urem i64 %464, %465
  %467 = trunc i64 %466 to i32
  br label %blk_exit466

blk_exit470:                                      ; preds = %brif_next474, %brif_next472, %blk_exit467
  %468 = call i32 @random()
  %469 = urem i32 %468, %453
  br label %blk_exit466

blk_exit466:                                      ; preds = %blk_exit470, %brif_next476, %brif_next469
  %_local_5.8 = phi i32 [ %469, %blk_exit470 ], [ %467, %brif_next476 ], [ %481, %brif_next469 ]
  %470 = add i32 %_local_3.20, 44
  %471 = inttoptr i32 %470 to i32*
  %472 = load i32, i32* %471, align 4
  %473 = icmp slt i32 %_local_5.8, %472
  br i1 %473, label %blk_exit406, label %loop_entry481

loop_entry481:                                    ; preds = %blk_exit466, %loop_entry481
  %_local_5.9 = phi i32 [ %_local_5.8, %blk_exit466 ], [ %474, %loop_entry481 ]
  %_local_3.23 = phi i32 [ %_local_3.20, %blk_exit466 ], [ %476, %loop_entry481 ]
  %_param_0.8 = phi i32 [ %472, %blk_exit466 ], [ %479, %loop_entry481 ]
  %474 = sub i32 %_local_5.9, %_param_0.8
  %calcOffset482 = add i32 %_local_3.23, 76
  %475 = inttoptr i32 %calcOffset482 to i32*
  %476 = load i32, i32* %475, align 4
  %477 = add i32 %476, 44
  %478 = inttoptr i32 %477 to i32*
  %479 = load i32, i32* %478, align 4
  %.not783 = icmp slt i32 %474, %479
  br i1 %.not783, label %blk_exit406, label %loop_entry481

brif_next469:                                     ; preds = %blk_exit446
  %480 = call i32 @atol(i32 %457)
  %481 = urem i32 %480, %453
  br label %blk_exit466

brif_next411:                                     ; preds = %loop_entry404
  %482 = icmp eq i32 %84, -1
  br i1 %482, label %blk_exit407, label %brif_next413

brif_next413:                                     ; preds = %brif_next411
  %calcOffset414 = add i32 %0, -1024
  %483 = inttoptr i32 %calcOffset414 to i64*
  %484 = add i32 %0, -1024
  %485 = inttoptr i32 %484 to i8*
  call void @llvm.memset.p0i8.i64(i8* %485, i8 0, i64 8, i1 true)
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
  %.not798 = icmp eq i32 %491, 1
  br i1 %.not798, label %brif_next425, label %blk_exit419

brif_next425:                                     ; preds = %brif_next423
  %493 = load i64, i64* %483, align 8
  %494 = urem i64 %493, 100
  %495 = trunc i64 %494 to i32
  br label %blk_exit415

blk_exit419:                                      ; preds = %brif_next423, %brif_next421, %blk_exit416
  %496 = call i32 @random()
  %497 = urem i32 %496, 100
  br label %blk_exit415

blk_exit415:                                      ; preds = %blk_exit419, %brif_next425, %brif_next418
  %_param_0.6 = phi i32 [ %497, %blk_exit419 ], [ %495, %brif_next425 ], [ %508, %brif_next418 ]
  %498 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %499 = inttoptr i32 %498 to i32*
  %500 = load i32, i32* %499, align 4
  %.not778 = icmp eq i32 %500, -1
  br i1 %.not778, label %blk_exit408, label %blk_exit427

blk_exit427:                                      ; preds = %blk_exit415
  %.not796 = icmp slt i32 %_param_0.6, %500
  br i1 %.not796, label %blk_exit408, label %loop_entry434

loop_entry434:                                    ; preds = %blk_exit427, %brif_next438
  %_local_3.22 = phi i32 [ %498, %blk_exit427 ], [ %502, %brif_next438 ]
  %_param_1.7 = phi i32 [ %500, %blk_exit427 ], [ %504, %brif_next438 ]
  %_param_0.7 = phi i32 [ %_param_0.6, %blk_exit427 ], [ %506, %brif_next438 ]
  %calcOffset435 = add i32 %_local_3.22, 76
  %501 = inttoptr i32 %calcOffset435 to i32*
  %502 = load i32, i32* %501, align 4
  %503 = inttoptr i32 %502 to i32*
  %504 = load i32, i32* %503, align 4
  %505 = icmp eq i32 %504, -1
  br i1 %505, label %blk_exit408, label %brif_next438

brif_next438:                                     ; preds = %loop_entry434
  %506 = sub i32 %_param_0.7, %_param_1.7
  %.not797 = icmp slt i32 %506, %504
  br i1 %.not797, label %blk_exit408, label %loop_entry434

brif_next418:                                     ; preds = %brif_next413
  %507 = call i32 @atol(i32 %486)
  %508 = urem i32 %507, 100
  br label %blk_exit415

brif_next366:                                     ; preds = %blk_exit344
  %509 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %510 = load i8, i8* inttoptr (i32 3336 to i8*), align 8
  %brif_val371.not = icmp eq i8 %510, 0
  br i1 %brif_val371.not, label %brif_next372, label %blk_exit370

brif_next372:                                     ; preds = %brif_next366
  store i32 -1, i32* inttoptr (i32 3348 to i32*), align 4
  call void @llvm.memset.p0i8.i64(i8* inttoptr (i32 3340 to i8*), i8 0, i64 8, i1 true)
  %511 = icmp eq i32 %509, 0
  br i1 %511, label %blk_exit369, label %loop_entry375

blk_exit369:                                      ; preds = %brif_next372
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  %512 = icmp eq i8 %74, 0
  br i1 %512, label %blk_exit21, label %blk_exit368

blk_exit368:                                      ; preds = %blk_exit369, %brif_next390
  %513 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit367

loop_entry375:                                    ; preds = %brif_next372, %blk_exit376
  %_local_3.18 = phi i32 [ %509, %brif_next372 ], [ %530, %blk_exit376 ]
  call void @get_tbl(i32 %_local_3.18)
  %514 = inttoptr i32 %_local_3.18 to i32*
  %515 = load i32, i32* %514, align 4
  %.not775 = icmp eq i32 %515, -1
  br i1 %.not775, label %brif_next379, label %blk_exit376

brif_next379:                                     ; preds = %loop_entry375
  %516 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %517 = add i32 %_local_3.18, 44
  %518 = inttoptr i32 %517 to i32*
  %519 = load i32, i32* %518, align 4
  %520 = add i32 %516, %519
  store i32 %520, i32* inttoptr (i32 3340 to i32*), align 4
  %521 = load i32, i32* inttoptr (i32 3344 to i32*), align 16
  %522 = add i32 %_local_3.18, 48
  %523 = inttoptr i32 %522 to i32*
  %524 = load i32, i32* %523, align 4
  %.not776 = icmp ult i32 %521, %524
  br i1 %.not776, label %brif_next384, label %blk_exit381

brif_next384:                                     ; preds = %brif_next379
  store i32 %524, i32* inttoptr (i32 3344 to i32*), align 16
  br label %blk_exit381

blk_exit381:                                      ; preds = %brif_next384, %brif_next379
  %525 = load i32, i32* inttoptr (i32 3348 to i32*), align 4
  %526 = add i32 %_local_3.18, 52
  %527 = inttoptr i32 %526 to i32*
  %528 = load i32, i32* %527, align 4
  %.not777 = icmp ugt i32 %525, %528
  br i1 %.not777, label %brif_next387, label %blk_exit376

brif_next387:                                     ; preds = %blk_exit381
  store i32 %528, i32* inttoptr (i32 3348 to i32*), align 4
  br label %blk_exit376

blk_exit376:                                      ; preds = %brif_next387, %blk_exit381, %loop_entry375
  %calcOffset388 = add i32 %_local_3.18, 76
  %529 = inttoptr i32 %calcOffset388 to i32*
  %530 = load i32, i32* %529, align 4
  %brif_val389.not = icmp eq i32 %530, 0
  br i1 %brif_val389.not, label %brif_next390, label %loop_entry375

brif_next390:                                     ; preds = %blk_exit376
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  %531 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %532 = icmp eq i8 %531, 0
  br i1 %532, label %blk_exit21, label %blk_exit368

blk_exit21:                                       ; preds = %blk_exit370, %blk_exit369, %brif_next390
  %533 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit20

blk_exit20:                                       ; preds = %blk_exit21, %brif_next403, %blk_exit367
  %_param_0.0 = phi i32 [ %533, %blk_exit21 ], [ 0, %blk_exit367 ], [ %_local_3.17, %brif_next403 ]
  call void @print_list(i32 %_param_0.0, i32 0)
  br label %blk_exit19

blk_exit367:                                      ; preds = %blk_exit368, %blk_exit370
  %_local_3.17 = phi i32 [ %513, %blk_exit368 ], [ %509, %blk_exit370 ]
  call void @llvm.memset.p0i8.i64(i8* inttoptr (i32 3352 to i8*), i8 0, i64 8, i1 true)
  %534 = icmp eq i32 %_local_3.17, 0
  br i1 %534, label %blk_exit20, label %loop_entry399

loop_entry399:                                    ; preds = %blk_exit367, %loop_entry399
  %_local_5.5 = phi i32 [ %540, %loop_entry399 ], [ %_local_3.17, %blk_exit367 ]
  %_param_1.6 = phi i32 [ %538, %loop_entry399 ], [ 0, %blk_exit367 ]
  %_param_0.5 = phi i32 [ %535, %loop_entry399 ], [ 0, %blk_exit367 ]
  %535 = add i32 %_param_0.5, 1
  %calcOffset400 = add i32 %_local_5.5, 64
  %536 = inttoptr i32 %calcOffset400 to i32*
  %537 = load i32, i32* %536, align 4
  %538 = add i32 %537, %_param_1.6
  %calcOffset401 = add i32 %_local_5.5, 76
  %539 = inttoptr i32 %calcOffset401 to i32*
  %540 = load i32, i32* %539, align 4
  %brif_val402.not = icmp eq i32 %540, 0
  br i1 %brif_val402.not, label %brif_next403, label %loop_entry399

brif_next403:                                     ; preds = %loop_entry399
  store i32 %538, i32* inttoptr (i32 3352 to i32*), align 8
  store i32 %535, i32* inttoptr (i32 3356 to i32*), align 4
  br label %blk_exit20

blk_exit370:                                      ; preds = %brif_next366
  %brif_val393.not = icmp eq i8 %74, 0
  br i1 %brif_val393.not, label %blk_exit21, label %blk_exit367

brif_next348:                                     ; preds = %blk_exit339
  %541 = sub i32 100, %_param_1.4
  %542 = icmp ult i32 %_local_4.2, 2
  br i1 %542, label %blk_exit349, label %brif_next351

blk_exit349:                                      ; preds = %blk_exit355, %brif_next351, %brif_next348
  %_local_3.14 = phi i32 [ %541, %brif_next348 ], [ %541, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %543 = inttoptr i32 %_local_5.3 to i32*
  store i32 %_local_3.14, i32* %543, align 4
  br label %blk_exit344

brif_next351:                                     ; preds = %brif_next348
  %544 = udiv i32 %541, %_local_4.2
  %545 = icmp eq i32 %67, %_local_5.3
  br i1 %545, label %blk_exit349, label %loop_entry354

loop_entry354:                                    ; preds = %brif_next351, %blk_exit355
  %_local_12.1 = phi i32 [ %67, %brif_next351 ], [ %550, %blk_exit355 ]
  %_local_3.15 = phi i32 [ %541, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %546 = inttoptr i32 %_local_12.1 to i32*
  %547 = load i32, i32* %546, align 4
  %.not773 = icmp eq i32 %547, -1
  br i1 %.not773, label %brif_next358, label %blk_exit355

brif_next358:                                     ; preds = %loop_entry354
  store i32 %544, i32* %546, align 4
  %548 = sub i32 %_local_3.15, %544
  br label %blk_exit355

blk_exit355:                                      ; preds = %brif_next358, %loop_entry354
  %_local_3.16 = phi i32 [ %548, %brif_next358 ], [ %_local_3.15, %loop_entry354 ]
  %calcOffset360 = add i32 %_local_12.1, 80
  %549 = inttoptr i32 %calcOffset360 to i32*
  %550 = load i32, i32* %549, align 4
  %.not774 = icmp eq i32 %550, %_local_5.3
  br i1 %.not774, label %blk_exit349, label %loop_entry354

blk_exit30:                                       ; preds = %blk_exit334
  %551 = load i32, i32* null, align 2147483648
  %552 = call i32 @fprintf(i32 %551, i32 1655, i32 0)
  call void @exit(i32 1)
  unreachable

brif_next323:                                     ; preds = %blk_exit314
  %553 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %554 = and i8 %553, 1
  br label %loop_entry324

loop_entry324:                                    ; preds = %loop_entry324, %brif_next323
  %_local_5.4 = phi i32 [ 0, %brif_next323 ], [ %560, %loop_entry324 ]
  %_local_4.3 = phi i32 [ 0, %brif_next323 ], [ %561, %loop_entry324 ]
  %_local_3.13 = phi i32 [ %67, %brif_next323 ], [ %565, %loop_entry324 ]
  %_param_1.5 = phi i32 [ 0, %brif_next323 ], [ %563, %loop_entry324 ]
  %select_cond325.not = icmp eq i8 %554, 0
  %555 = select i1 %select_cond325.not, i32 %_local_5.4, i32 %_local_3.13
  %556 = inttoptr i32 %_local_3.13 to i32*
  %557 = load i32, i32* %556, align 4
  %558 = icmp eq i32 %557, -1
  %559 = zext i1 %558 to i32
  %560 = select i1 %558, i32 %555, i32 %_local_5.4
  %561 = add i32 %_local_4.3, %559
  %562 = select i1 %558, i32 0, i32 %557
  %563 = add i32 %562, %_param_1.5
  %calcOffset329 = add i32 %_local_3.13, 80
  %564 = inttoptr i32 %calcOffset329 to i32*
  %565 = load i32, i32* %564, align 4
  %brif_val330.not = icmp eq i32 %565, 0
  br i1 %brif_val330.not, label %brif_next331, label %loop_entry324

brif_next331:                                     ; preds = %loop_entry324
  %566 = icmp sgt i32 %563, 100
  br i1 %566, label %blk_exit32, label %blk_exit321

blk_exit32:                                       ; preds = %brif_next331
  %calcOffset763 = add i32 %0, -2880
  %567 = inttoptr i32 %calcOffset763 to i32*
  store i32 %563, i32* %567, align 4
  %568 = load i32, i32* null, align 2147483648
  %569 = call i32 @fprintf(i32 %568, i32 2122, i32 %calcOffset763)
  call void @exit(i32 1)
  unreachable

blk_exit19:                                       ; preds = %blk_exit20, %blk_exit731, %blk_exit683, %brif_next318
  %_local_5.0 = phi i32 [ 0, %blk_exit683 ], [ 0, %blk_exit731 ], [ 0, %blk_exit20 ], [ %583, %brif_next318 ]
  %570 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  call void @free_desc(i32 %570)
  %571 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  call void @free(i32 %571)
  call void @exit(i32 %_local_5.0)
  unreachable

brif_next316:                                     ; preds = %blk_exit35
  %572 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %573 = call i32 @maxlen_in_list(i32 %572)
  store i32 %573, i32* inttoptr (i32 2284 to i32*), align 4
  %574 = add i32 %573, 10
  %575 = call i32 @malloc(i32 %574)
  %576 = icmp eq i32 %575, 0
  br i1 %576, label %blk_exit33, label %brif_next318

blk_exit33:                                       ; preds = %brif_next316
  %calcOffset762 = add i32 %0, -2944
  %577 = inttoptr i32 %calcOffset762 to i32*
  store i32 1987, i32* %577, align 4
  %578 = load i32, i32* null, align 2147483648
  %579 = call i32 @fprintf(i32 %578, i32 1191, i32 %calcOffset762)
  call void @exit(i32 1)
  unreachable

brif_next318:                                     ; preds = %brif_next316
  store i32 %575, i32* inttoptr (i32 2288 to i32*), align 16
  %calcOffset319 = add i32 %0, -1024
  %580 = inttoptr i32 %calcOffset319 to i8*
  store i8 0, i8* %580, align 1
  %581 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  call void @matches_in_list(i32 %581, i32 %calcOffset319)
  %582 = load i8, i8* %580, align 1
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
  %calcOffset313 = add i32 %0, -2848
  %586 = inttoptr i32 %calcOffset313 to i32*
  store i32 %_local_4.1, i32* %586, align 4
  %587 = load i32, i32* null, align 2147483648
  %588 = call i32 @fprintf(i32 %587, i32 1742, i32 %calcOffset313)
  call void @exit(i32 1)
  unreachable

brif_next266:                                     ; preds = %brif_next264
  %589 = inttoptr i32 %_local_4.0 to i8*
  %590 = load i8, i8* %589, align 1
  %591 = icmp eq i8 %590, 0
  br i1 %591, label %blk_exit39, label %loop_entry275

blk_exit39:                                       ; preds = %brif_next277, %brif_next266
  %_param_1.0 = phi i32 [ 1, %brif_next266 ], [ %630, %brif_next277 ]
  %592 = call i32 @malloc(i32 %_param_1.0)
  %593 = icmp eq i32 %592, 0
  br i1 %593, label %blk_exit34, label %brif_next283

blk_exit34:                                       ; preds = %blk_exit39
  %calcOffset761 = add i32 %0, -2864
  %594 = inttoptr i32 %calcOffset761 to i32*
  store i32 1759, i32* %594, align 4
  %595 = load i32, i32* null, align 2147483648
  %596 = call i32 @fprintf(i32 %595, i32 1191, i32 %calcOffset761)
  call void @exit(i32 1)
  unreachable

brif_next283:                                     ; preds = %blk_exit39
  %597 = load i8, i8* %589, align 1
  %598 = icmp eq i8 %597, 0
  br i1 %598, label %blk_exit284, label %loop_entry288

blk_exit284:                                      ; preds = %blk_exit289, %brif_next283
  %_local_3.10 = phi i32 [ %592, %brif_next283 ], [ %_local_3.12, %blk_exit289 ]
  %599 = inttoptr i32 %_local_3.10 to i8*
  store i8 0, i8* %599, align 1
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
  %607 = inttoptr i32 %_local_3.11 to i8*
  store i8 %_local_5.2.in, i8* %607, align 1
  %608 = add i32 %_local_3.11, 1
  br label %blk_exit289

brif_next299:                                     ; preds = %blk_exit290
  %calcOffset300 = add i32 %_local_3.11, 1
  %609 = inttoptr i32 %calcOffset300 to i8*
  store i8 %_local_5.2.in, i8* %609, align 1
  %610 = inttoptr i32 %_local_3.11 to i8*
  store i8 91, i8* %610, align 1
  %611 = call i32 @tolower(i32 %602)
  %calcOffset302 = add i32 %_local_3.11, 3
  %612 = inttoptr i32 %calcOffset302 to i8*
  store i8 93, i8* %612, align 1
  %calcOffset303 = add i32 %_local_3.11, 2
  %613 = inttoptr i32 %calcOffset303 to i8*
  %614 = trunc i32 %611 to i8
  store i8 %614, i8* %613, align 1
  %615 = add i32 %_local_3.11, 4
  br label %blk_exit289

blk_exit289:                                      ; preds = %blk_exit297, %brif_next299, %brif_next292
  %_local_3.12 = phi i32 [ %608, %blk_exit297 ], [ %615, %brif_next299 ], [ %624, %brif_next292 ]
  %616 = inttoptr i32 %_param_1.3 to i8*
  %617 = load i8, i8* %616, align 1
  %brif_val306.not = icmp eq i8 %617, 0
  br i1 %brif_val306.not, label %blk_exit284, label %loop_entry288

brif_next292:                                     ; preds = %loop_entry288
  %calcOffset293 = add i32 %_local_3.11, 1
  %618 = inttoptr i32 %calcOffset293 to i8*
  store i8 %_local_5.2.in, i8* %618, align 1
  %619 = inttoptr i32 %_local_3.11 to i8*
  store i8 91, i8* %619, align 1
  %620 = call i32 @toupper(i32 %602)
  %calcOffset295 = add i32 %_local_3.11, 3
  %621 = inttoptr i32 %calcOffset295 to i8*
  store i8 93, i8* %621, align 1
  %calcOffset296 = add i32 %_local_3.11, 2
  %622 = inttoptr i32 %calcOffset296 to i8*
  %623 = trunc i32 %620 to i8
  store i8 %623, i8* %622, align 1
  %624 = add i32 %_local_3.11, 4
  br label %blk_exit289

loop_entry275:                                    ; preds = %brif_next266, %brif_next277
  %_local_5.1 = phi i32 [ %630, %brif_next277 ], [ 1, %brif_next266 ]
  %_local_3.9.in = phi i8 [ %632, %brif_next277 ], [ %590, %brif_next266 ]
  %_param_0.4.in = phi i32 [ %_param_0.4, %brif_next277 ], [ %_local_4.0, %brif_next266 ]
  %_param_0.4 = add i32 %_param_0.4.in, 1
  %625 = or i8 %_local_3.9.in, 32
  %626 = sext i8 %625 to i32
  %627 = add nsw i32 %626, -97
  %628 = icmp ult i32 %627, 26
  %629 = select i1 %628, i32 4, i32 1
  %630 = add i32 %629, %_local_5.1
  %.not771 = icmp ult i32 %_local_5.1, %630
  br i1 %.not771, label %brif_next277, label %blk_exit274

brif_next277:                                     ; preds = %loop_entry275
  %631 = inttoptr i32 %_param_0.4 to i8*
  %632 = load i8, i8* %631, align 1
  %633 = icmp eq i8 %632, 0
  br i1 %633, label %blk_exit39, label %loop_entry275

blk_exit274:                                      ; preds = %loop_entry275
  %calcOffset281 = add i32 %0, -2832
  %634 = inttoptr i32 %calcOffset281 to i32*
  store i32 2075, i32* %634, align 4
  %635 = load i32, i32* null, align 2147483648
  %636 = call i32 @fprintf(i32 %635, i32 1191, i32 %calcOffset281)
  call void @exit(i32 1)
  unreachable

brif_next45:                                      ; preds = %blk_exit5
  %637 = icmp slt i32 %8, 1
  br i1 %637, label %blk_exit42, label %brif_next47

brif_next47:                                      ; preds = %brif_next45
  %638 = shl i32 %7, 2
  %639 = add i32 %638, %_arg_1
  br label %loop_entry51

loop_entry51:                                     ; preds = %blk_exit110, %brif_next47
  %_local_8.0 = phi i32 [ 0, %brif_next47 ], [ %699, %blk_exit110 ]
  %640 = shl i32 %_local_8.0, 2
  %641 = add i32 %639, %640
  %642 = inttoptr i32 %641 to i32*
  %643 = load i32, i32* %642, align 4
  %644 = inttoptr i32 %643 to i8*
  %645 = load i8, i8* %644, align 1
  %646 = sext i8 %645 to i32
  %647 = add nsw i32 %646, -48
  %648 = icmp ult i32 %647, 10
  br i1 %648, label %blk_exit53, label %blk_exit52

blk_exit53:                                       ; preds = %loop_entry51
  %649 = add i32 %643, 1
  br label %loop_entry58

loop_entry58:                                     ; preds = %brif_next60, %blk_exit53
  %_local_3.1 = phi i32 [ %646, %blk_exit53 ], [ %661, %brif_next60 ]
  %_param_1.1 = phi i32 [ 0, %blk_exit53 ], [ %658, %brif_next60 ]
  %_param_0.1 = phi i32 [ %649, %blk_exit53 ], [ %662, %brif_next60 ]
  %650 = mul i32 %_param_1.1, 10
  %651 = add i32 %650, %_local_3.1
  %652 = icmp sgt i32 %651, 148
  br i1 %652, label %blk_exit50, label %brif_next60

blk_exit50:                                       ; preds = %loop_entry58
  %653 = load i32, i32* null, align 2147483648
  %654 = call i32 @fprintf(i32 %653, i32 1914, i32 0)
  %655 = load i32, i32* %642, align 4
  %calcOffset216 = add i32 %0, -2816
  %656 = inttoptr i32 %calcOffset216 to i32*
  store i32 %655, i32* %656, align 4
  %657 = call i32 @fprintf(i32 %653, i32 2159, i32 %calcOffset216)
  br label %blk_exit49

blk_exit49:                                       ; preds = %blk_exit50, %brif_next78, %blk_exit69, %brif_next66
  store i8 1, i8* inttoptr (i32 2296 to i8*), align 8
  br label %blk_exit41

brif_next60:                                      ; preds = %loop_entry58
  %658 = add i32 %651, -48
  %659 = inttoptr i32 %_param_0.1 to i8*
  %660 = load i8, i8* %659, align 1
  %661 = sext i8 %660 to i32
  %662 = add i32 %_param_0.1, 1
  %663 = add nsw i32 %661, -48
  %664 = icmp ult i32 %663, 10
  br i1 %664, label %loop_entry58, label %brif_next63

brif_next63:                                      ; preds = %brif_next60
  %665 = icmp sgt i32 %651, 47
  br i1 %665, label %blk_exit64, label %brif_next66

blk_exit64:                                       ; preds = %brif_next63
  switch i8 %660, label %blk_exit52 [
    i8 37, label %blk_exit68
    i8 46, label %blk_exit69
  ]

blk_exit68:                                       ; preds = %blk_exit64
  %666 = inttoptr i32 %662 to i8*
  %667 = load i8, i8* %666, align 1
  %668 = icmp eq i8 %667, 0
  br i1 %668, label %blk_exit72, label %blk_exit52

blk_exit72:                                       ; preds = %blk_exit68
  %669 = add i32 %_local_8.0, 1
  %670 = icmp slt i32 %669, %8
  br i1 %670, label %blk_exit76, label %brif_next78

blk_exit76:                                       ; preds = %blk_exit72
  %671 = shl i32 %669, 2
  %672 = add i32 %639, %671
  %673 = inttoptr i32 %672 to i32*
  %674 = load i32, i32* %673, align 4
  br label %blk_exit52

brif_next78:                                      ; preds = %blk_exit72
  %calcOffset79 = add i32 %0, -2608
  %675 = inttoptr i32 %calcOffset79 to i32*
  store i32 1710, i32* %675, align 4
  %676 = load i32, i32* null, align 2147483648
  %677 = call i32 @fprintf(i32 %676, i32 1191, i32 %calcOffset79)
  br label %blk_exit49

blk_exit69:                                       ; preds = %blk_exit64
  %calcOffset70 = add i32 %0, -2624
  %678 = inttoptr i32 %calcOffset70 to i32*
  store i32 1625, i32* %678, align 4
  %679 = load i32, i32* null, align 2147483648
  %680 = call i32 @fprintf(i32 %679, i32 1191, i32 %calcOffset70)
  br label %blk_exit49

brif_next66:                                      ; preds = %brif_next63
  %calcOffset67 = add i32 %0, -2800
  %681 = inttoptr i32 %calcOffset67 to i32*
  store i32 %643, i32* %681, align 4
  %682 = load i32, i32* null, align 2147483648
  %683 = call i32 @fprintf(i32 %682, i32 2159, i32 %calcOffset67)
  br label %blk_exit49

blk_exit52:                                       ; preds = %blk_exit68, %blk_exit76, %loop_entry51, %blk_exit64
  %_local_11.0 = phi i32 [ -1, %blk_exit64 ], [ -1, %loop_entry51 ], [ %658, %blk_exit76 ], [ %658, %blk_exit68 ]
  %_local_10.0 = phi i32 [ %643, %blk_exit64 ], [ %643, %loop_entry51 ], [ %674, %blk_exit76 ], [ %662, %blk_exit68 ]
  %_local_8.1 = phi i32 [ %_local_8.0, %blk_exit64 ], [ %_local_8.0, %loop_entry51 ], [ %669, %blk_exit76 ], [ %_local_8.0, %blk_exit68 ]
  %684 = call i32 @strlen(i32 %_local_10.0)
  %685 = icmp ult i32 %684, 3
  br i1 %685, label %blk_exit81, label %brif_next83

blk_exit81:                                       ; preds = %brif_next89, %brif_next86, %brif_next83, %blk_exit52
  %686 = phi i1 [ false, %brif_next89 ], [ true, %brif_next86 ], [ true, %brif_next83 ], [ true, %blk_exit52 ]
  %_local_3.2 = phi i32 [ 1282, %brif_next89 ], [ 1121, %brif_next86 ], [ 1121, %brif_next83 ], [ 1121, %blk_exit52 ]
  %_param_1.2 = phi i32 [ 1246, %brif_next89 ], [ 1089, %brif_next86 ], [ 1089, %brif_next83 ], [ 1089, %blk_exit52 ]
  %687 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val93.not = icmp eq i32 %687, 0
  br i1 %brif_val93.not, label %brif_next94, label %blk_exit92

brif_next94:                                      ; preds = %blk_exit81
  %calcOffset95 = add i32 %0, -2704
  %688 = inttoptr i32 %calcOffset95 to i32*
  store i32 %_param_1.2, i32* %688, align 4
  %689 = add i32 %0, -1536
  %690 = call i32 @snprintf(i32 %689, i32 512, i32 1191, i32 %calcOffset95)
  %calcOffset96 = add i32 %0, -2720
  %691 = inttoptr i32 %calcOffset96 to i32*
  store i32 %_local_3.2, i32* %691, align 4
  %692 = add i32 %0, -2048
  %693 = call i32 @snprintf(i32 %692, i32 512, i32 1191, i32 %calcOffset96)
  br label %blk_exit91

blk_exit91:                                       ; preds = %blk_exit97, %brif_next103, %brif_next94
  %694 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %695 = icmp eq i32 %694, 0
  br i1 %695, label %blk_exit111, label %brif_next113

blk_exit111:                                      ; preds = %blk_exit91
  %696 = add i32 %0, -1536
  %697 = call i32 @add_file(i32 %_local_11.0, i32 %696, i32 0, i32 2268, i32 3328, i32 0)
  %698 = icmp eq i32 %697, 0
  br i1 %698, label %blk_exit41, label %blk_exit110

blk_exit110:                                      ; preds = %blk_exit111, %blk_exit203, %blk_exit157, %brif_next183
  %699 = add i32 %_local_8.1, 1
  %700 = icmp slt i32 %699, %8
  br i1 %700, label %loop_entry51, label %blk_exit42

brif_next113:                                     ; preds = %blk_exit91
  %701 = add i32 %0, -2560
  %702 = call i32 @strncpy(i32 %701, i32 %694, i32 512)
  %calcOffset114 = add i32 %0, -2049
  %703 = inttoptr i32 %calcOffset114 to i8*
  store i8 0, i8* %703, align 1
  br label %loop_entry117

loop_entry117:                                    ; preds = %blk_exit134, %brif_next113
  %_local_3.3 = phi i32 [ %701, %brif_next113 ], [ %storemerge799, %blk_exit134 ]
  %704 = inttoptr i32 %_local_3.3 to i8*
  %705 = load i8, i8* %704, align 1
  %brif_val120.not = icmp eq i8 %705, 0
  br i1 %brif_val120.not, label %brif_next121, label %blk_exit118

brif_next121:                                     ; preds = %loop_entry117
  %calcOffset122 = add i32 %0, -2564
  %calcOffset124 = add i32 %0, -2572
  %706 = add i32 %0, -2576
  %707 = inttoptr i32 %706 to i8*
  call void @llvm.memset.p0i8.i64(i8* %707, i8 0, i64 16, i1 true)
  br label %blk_exit116

blk_exit116:                                      ; preds = %blk_exit145, %brif_next121
  %.pre-phi810 = phi i32 [ %calcOffset152, %blk_exit145 ], [ %calcOffset124, %brif_next121 ]
  %.pre-phi = phi i32 [ %777, %blk_exit145 ], [ %calcOffset122, %brif_next121 ]
  %708 = add i32 %0, -1536
  %709 = call i32 @add_file(i32 100, i32 %708, i32 0, i32 %.pre-phi, i32 %.pre-phi810, i32 0)
  br label %blk_exit115

blk_exit115:                                      ; preds = %blk_exit116, %blk_exit145, %brif_next136
  %_local_12.0 = phi i32 [ %709, %blk_exit116 ], [ %784, %blk_exit145 ], [ 1, %brif_next136 ]
  %710 = add i32 %0, -1536
  %711 = add i32 %0, -2048
  %712 = call i32 @strncmp(i32 %710, i32 %711, i32 512)
  %713 = icmp eq i32 %712, 0
  br i1 %713, label %blk_exit160, label %brif_next162

blk_exit160:                                      ; preds = %brif_next162, %blk_exit115
  %calcOffset166 = add i32 %0, -2572
  %714 = inttoptr i32 %calcOffset166 to i32*
  %715 = load i32, i32* %714, align 4
  %716 = icmp eq i32 %715, 0
  br i1 %716, label %blk_exit156, label %brif_next168

brif_next168:                                     ; preds = %blk_exit160
  %717 = inttoptr i32 %715 to i32*
  store i32 %_local_11.0, i32* %717, align 4
  %718 = load i32, i32* inttoptr (i32 3328 to i32*), align 256
  %719 = icmp eq i32 %718, 0
  br i1 %719, label %blk_exit158, label %brif_next171

blk_exit158:                                      ; preds = %brif_next168
  store i32 %715, i32* inttoptr (i32 3328 to i32*), align 256
  %calcOffset200 = add i32 %0, -2564
  %720 = inttoptr i32 %calcOffset200 to i32*
  %721 = load i32, i32* %720, align 4
  store i32 %721, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit157

brif_next171:                                     ; preds = %brif_next168
  %calcOffset172 = add i32 %0, -2564
  %722 = inttoptr i32 %calcOffset172 to i32*
  %723 = load i32, i32* %722, align 4
  %calcOffset173 = add i32 %718, 76
  %724 = inttoptr i32 %calcOffset173 to i32*
  store i32 %723, i32* %724, align 4
  store i32 %715, i32* inttoptr (i32 3328 to i32*), align 256
  %725 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %calcOffset174 = add i32 %723, 80
  %726 = inttoptr i32 %calcOffset174 to i32*
  store i32 %725, i32* %726, align 4
  br label %blk_exit157

blk_exit156:                                      ; preds = %blk_exit157, %brif_next178, %blk_exit160
  br i1 %686, label %blk_exit203, label %brif_next205

blk_exit203:                                      ; preds = %brif_next205, %blk_exit156
  %727 = call i32 @getenv(i32 1518)
  %calcOffset207 = add i32 %0, -2780
  %728 = inttoptr i32 %calcOffset207 to i32*
  store i32 %_local_10.0, i32* %728, align 4
  %calcOffset208 = add i32 %0, -2784
  %729 = inttoptr i32 %calcOffset208 to i32*
  store i32 %727, i32* %729, align 4
  %730 = call i32 @snprintf(i32 %711, i32 512, i32 1174, i32 %calcOffset208)
  %731 = call i32 @add_file(i32 %_local_11.0, i32 %711, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val209.not = icmp eq i32 %731, 0
  br i1 %brif_val209.not, label %blk_exit41, label %blk_exit110

brif_next205:                                     ; preds = %blk_exit156
  %732 = add i32 %684, %_local_10.0
  %733 = add i32 %732, -2
  %734 = inttoptr i32 %733 to i8*
  store i8 45, i8* %734, align 1
  br label %blk_exit203

blk_exit157:                                      ; preds = %blk_exit158, %brif_next176, %blk_exit159, %brif_next171
  %brif_val201.not = icmp eq i32 %_local_12.0, 0
  br i1 %brif_val201.not, label %blk_exit156, label %blk_exit110

brif_next162:                                     ; preds = %blk_exit115
  %735 = add i32 %0, -2568
  %736 = add i32 %0, -2576
  %737 = call i32 @add_file(i32 100, i32 %711, i32 0, i32 %735, i32 %736, i32 0)
  %738 = inttoptr i32 %735 to i32*
  %739 = load i32, i32* %738, align 4
  %brif_val164.not = icmp eq i32 %739, 0
  br i1 %brif_val164.not, label %blk_exit160, label %blk_exit159

blk_exit159:                                      ; preds = %brif_next162
  %740 = call i32 @strncmp(i32 %710, i32 %711, i32 512)
  %741 = icmp eq i32 %740, 0
  br i1 %741, label %blk_exit157, label %brif_next176

brif_next176:                                     ; preds = %blk_exit159
  %742 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val177.not = icmp eq i32 %742, 0
  br i1 %brif_val177.not, label %brif_next178, label %blk_exit157

brif_next178:                                     ; preds = %brif_next176
  %calcOffset179 = add i32 %0, -2564
  %743 = inttoptr i32 %calcOffset179 to i32*
  %744 = load i32, i32* %743, align 4
  %745 = icmp eq i32 %744, 0
  br i1 %745, label %blk_exit156, label %brif_next181

brif_next181:                                     ; preds = %brif_next178
  %746 = call i32 @malloc(i32 84)
  %747 = icmp eq i32 %746, 0
  br i1 %747, label %blk_exit48, label %brif_next183

blk_exit48:                                       ; preds = %brif_next181
  %calcOffset217 = add i32 %0, -2768
  %748 = inttoptr i32 %calcOffset217 to i32*
  store i32 1987, i32* %748, align 4
  %749 = load i32, i32* null, align 2147483648
  %750 = call i32 @fprintf(i32 %749, i32 1191, i32 %calcOffset217)
  call void @exit(i32 1)
  unreachable

brif_next183:                                     ; preds = %brif_next181
  %751 = add i32 %746, 12
  %752 = inttoptr i32 %751 to i8*
  call void @llvm.memset.p0i8.i64(i8* %752, i8 -1, i64 8, i1 true)
  %753 = add i32 %746, 4
  %754 = inttoptr i32 %753 to i8*
  call void @llvm.memset.p0i8.i64(i8* %754, i8 -1, i64 8, i1 true)
  %755 = add i32 %746, 40
  %756 = inttoptr i32 %755 to i8*
  call void @llvm.memset.p0i8.i64(i8* %756, i8 0, i64 24, i1 true)
  %757 = add i32 %746, 20
  %758 = inttoptr i32 %757 to i8*
  call void @llvm.memset.p0i8.i64(i8* %758, i8 0, i64 8, i1 true)
  %759 = add i32 %746, 72
  %760 = inttoptr i32 %759 to i8*
  call void @llvm.memset.p0i8.i64(i8* %760, i8 0, i64 12, i1 true)
  %calcOffset192 = add i32 %744, 72
  %761 = inttoptr i32 %calcOffset192 to i32*
  store i32 %746, i32* %761, align 4
  %762 = add i32 %746, 28
  %763 = inttoptr i32 %762 to i8*
  call void @llvm.memset.p0i8.i64(i8* %763, i8 0, i64 9, i1 true)
  %764 = inttoptr i32 %746 to i32*
  store i32 %_local_11.0, i32* %764, align 4
  %calcOffset196 = add i32 %746, 68
  %765 = inttoptr i32 %calcOffset196 to i32*
  store i32 %744, i32* %765, align 4
  %calcOffset197 = add i32 %744, 76
  %766 = inttoptr i32 %calcOffset197 to i32*
  store i32 %739, i32* %766, align 4
  %calcOffset198 = add i32 %739, 80
  %767 = inttoptr i32 %calcOffset198 to i32*
  store i32 %744, i32* %767, align 4
  %calcOffset199 = add i32 %739, 72
  %768 = inttoptr i32 %calcOffset199 to i32*
  store i32 %746, i32* %768, align 4
  store i32 %746, i32* inttoptr (i32 2268 to i32*), align 4
  store i32 %746, i32* inttoptr (i32 3328 to i32*), align 256
  br label %blk_exit110

blk_exit118:                                      ; preds = %loop_entry117
  %769 = call i32 @strchr(i32 %_local_3.3, i32 58)
  %brif_val128.not = icmp eq i32 %769, 0
  br i1 %brif_val128.not, label %blk_exit126, label %blk_exit127

blk_exit126:                                      ; preds = %blk_exit118, %blk_exit127
  %storemerge799 = phi i32 [ %790, %blk_exit127 ], [ 0, %blk_exit118 ]
  %calcOffset131 = add i32 %0, -2728
  %770 = inttoptr i32 %calcOffset131 to i32*
  store i32 %_local_10.0, i32* %770, align 4
  %calcOffset132 = add i32 %0, -2732
  %771 = inttoptr i32 %calcOffset132 to i32*
  store i32 %_local_3.3, i32* %771, align 4
  %calcOffset133 = add i32 %0, -2736
  %772 = inttoptr i32 %calcOffset133 to i32*
  store i32 %_param_1.2, i32* %772, align 4
  %773 = add i32 %0, -1024
  %774 = call i32 @snprintf(i32 %773, i32 1024, i32 1171, i32 %calcOffset133)
  %775 = call i32 @add_file(i32 %_local_11.0, i32 %773, i32 0, i32 2268, i32 3328, i32 0)
  %776 = icmp eq i32 %775, 0
  br i1 %776, label %blk_exit134, label %brif_next136

blk_exit134:                                      ; preds = %blk_exit126
  %777 = add i32 %0, -2564
  %778 = call i32 @strncpy(i32 %777, i32 %_local_3.3, i32 2)
  %calcOffset141 = add i32 %0, -2752
  %779 = inttoptr i32 %calcOffset141 to i32*
  store i32 %_param_1.2, i32* %779, align 4
  %calcOffset142 = add i32 %0, -2562
  %780 = inttoptr i32 %calcOffset142 to i8*
  store i8 0, i8* %780, align 1
  %calcOffset143 = add i32 %0, -2744
  %781 = inttoptr i32 %calcOffset143 to i32*
  store i32 %_local_10.0, i32* %781, align 4
  %calcOffset144 = add i32 %0, -2748
  %782 = inttoptr i32 %calcOffset144 to i32*
  store i32 %777, i32* %782, align 4
  %783 = call i32 @snprintf(i32 %773, i32 1024, i32 1171, i32 %calcOffset141)
  %784 = call i32 @add_file(i32 %_local_11.0, i32 %773, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val146.not = icmp eq i32 %784, 0
  %brif_val146.not.not = xor i1 %brif_val146.not, true
  %brif_val148.not = icmp eq i32 %storemerge799, 0
  %or.cond814 = select i1 %brif_val146.not.not, i1 true, i1 %brif_val148.not
  br i1 %or.cond814, label %blk_exit145, label %loop_entry117

blk_exit145:                                      ; preds = %blk_exit134
  %calcOffset152 = add i32 %0, -2572
  %785 = add i32 %0, -2576
  %786 = inttoptr i32 %785 to i8*
  call void @llvm.memset.p0i8.i64(i8* %786, i8 0, i64 16, i1 true)
  br i1 %brif_val146.not, label %blk_exit116, label %blk_exit115

brif_next136:                                     ; preds = %blk_exit126
  %787 = add i32 %0, -2576
  %788 = inttoptr i32 %787 to i8*
  call void @llvm.memset.p0i8.i64(i8* %788, i8 0, i64 16, i1 true)
  br label %blk_exit115

blk_exit127:                                      ; preds = %blk_exit118
  %789 = inttoptr i32 %769 to i8*
  store i8 0, i8* %789, align 1
  %790 = add i32 %769, 1
  br label %blk_exit126

blk_exit92:                                       ; preds = %blk_exit81
  %791 = call i32 @strncmp(i32 %_local_10.0, i32 1559, i32 1)
  %792 = icmp eq i32 %791, 0
  br i1 %792, label %blk_exit97, label %brif_next99

blk_exit97:                                       ; preds = %brif_next101, %brif_next99, %blk_exit92
  %calcOffset108 = add i32 %0, -2672
  %793 = inttoptr i32 %calcOffset108 to i32*
  store i32 %_local_10.0, i32* %793, align 4
  %794 = add i32 %0, -1536
  %795 = call i32 @snprintf(i32 %794, i32 512, i32 1191, i32 %calcOffset108)
  %calcOffset109 = add i32 %0, -2688
  %796 = inttoptr i32 %calcOffset109 to i32*
  store i32 %_local_10.0, i32* %796, align 4
  %797 = add i32 %0, -2048
  %798 = call i32 @snprintf(i32 %797, i32 512, i32 1191, i32 %calcOffset109)
  br label %blk_exit91

brif_next99:                                      ; preds = %blk_exit92
  %799 = call i32 @strncmp(i32 %_local_10.0, i32 1558, i32 2)
  %800 = icmp eq i32 %799, 0
  br i1 %800, label %blk_exit97, label %brif_next101

brif_next101:                                     ; preds = %brif_next99
  %801 = call i32 @strncmp(i32 %_local_10.0, i32 1557, i32 3)
  %802 = icmp eq i32 %801, 0
  br i1 %802, label %blk_exit97, label %brif_next103

brif_next103:                                     ; preds = %brif_next101
  %calcOffset104 = add i32 %0, -2636
  %803 = inttoptr i32 %calcOffset104 to i32*
  store i32 %_local_10.0, i32* %803, align 4
  %calcOffset105 = add i32 %0, -2640
  %804 = inttoptr i32 %calcOffset105 to i32*
  store i32 %_param_1.2, i32* %804, align 4
  %805 = add i32 %0, -1536
  %806 = call i32 @snprintf(i32 %805, i32 512, i32 1174, i32 %calcOffset105)
  %calcOffset106 = add i32 %0, -2652
  %807 = inttoptr i32 %calcOffset106 to i32*
  store i32 %_local_10.0, i32* %807, align 4
  %calcOffset107 = add i32 %0, -2656
  %808 = inttoptr i32 %calcOffset107 to i32*
  store i32 %_local_3.2, i32* %808, align 4
  %809 = add i32 %0, -2048
  %810 = call i32 @snprintf(i32 %809, i32 512, i32 1174, i32 %calcOffset107)
  br label %blk_exit91

brif_next83:                                      ; preds = %blk_exit52
  %811 = add i32 %684, %_local_10.0
  %812 = add i32 %811, -2
  %813 = inttoptr i32 %812 to i8*
  %814 = load i8, i8* %813, align 1
  %.not = icmp eq i8 %814, 45
  br i1 %.not, label %brif_next86, label %blk_exit81

brif_next86:                                      ; preds = %brif_next83
  %815 = add i32 %811, -1
  %816 = inttoptr i32 %815 to i8*
  %817 = load i8, i8* %816, align 1
  %.not770 = icmp eq i8 %817, 111
  br i1 %.not770, label %brif_next89, label %blk_exit81

brif_next89:                                      ; preds = %brif_next86
  store i8 0, i8* %813, align 1
  br label %blk_exit81

blk_exit11:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2280 to i8*), align 8
  br label %loop_entry

blk_exit8:                                        ; preds = %loop_entry
  call void @program_version()
  %calcOffset = add i32 %0, -2592
  %818 = inttoptr i32 %calcOffset to i32*
  store i32 2304, i32* %818, align 4
  %819 = call i32 @printf(i32 1755, i32 %calcOffset)
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
  %820 = load i32, i32* null, align 2147483648
  %821 = call i32 @atoi(i32 %820)
  store i32 %821, i32* inttoptr (i32 2208 to i32*), align 32
  br label %loop_entry

blk_exit10:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2228 to i8*), align 4
  %822 = load i32, i32* null, align 2147483648
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
  %stack = alloca [1696 x i8], align 1, !notdec.stack_direction !1
  %stack_addr = ptrtoint [1696 x i8]* %stack to i32
  %stack_end = add i32 %stack_addr, -1696
  %brif_val.not = icmp eq i32 %_arg_2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit7

brif_next:                                        ; preds = %allocator
  %0 = call i32 @strdup(i32 %_arg_1)
  br label %blk_exit6

blk_exit6:                                        ; preds = %brif_next9, %brif_next
  %_local_7.0 = phi i32 [ %0, %brif_next ], [ %203, %brif_next9 ]
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
  %32 = add i32 %21, 76
  %33 = inttoptr i32 %32 to i8*
  call void @llvm.memset.p0i8.i64(i8* %33, i8 0, i64 8, i1 true)
  %calcOffset69 = add i32 %21, 20
  %calcOffset70 = add i32 %21, 68
  %34 = add i32 %21, 68
  %35 = inttoptr i32 %34 to i8*
  call void @llvm.memset.p0i8.i64(i8* %35, i8 0, i64 8, i1 true)
  %calcOffset71 = add i32 %21, 28
  %36 = add i32 %21, 20
  %37 = inttoptr i32 %36 to i8*
  call void @llvm.memset.p0i8.i64(i8* %37, i8 0, i64 16, i1 true)
  %calcOffset72 = add i32 %21, 4
  %38 = inttoptr i32 %calcOffset72 to i32*
  store i32 %18, i32* %38, align 4
  %39 = inttoptr i32 %21 to i32*
  store i32 %_arg_0, i32* %39, align 4
  %40 = call i32 @strdup(i32 %_arg_1)
  %41 = inttoptr i32 %calcOffset69 to i32*
  store i32 %40, i32* %41, align 4
  %42 = call i32 @strdup(i32 %_local_7.0)
  %calcOffset75 = add i32 %21, 38
  %43 = inttoptr i32 %calcOffset75 to i8*
  store i8 0, i8* %43, align 1
  %calcOffset76 = add i32 %21, 24
  %44 = inttoptr i32 %calcOffset76 to i32*
  store i32 %42, i32* %44, align 4
  %45 = call i32 @strlen(i32 %_local_7.0)
  %46 = add i32 %45, 6
  %47 = call i32 @malloc(i32 %46)
  %48 = icmp eq i32 %47, 0
  br i1 %48, label %blk_exit3, label %brif_next78

blk_exit3:                                        ; preds = %brif_next61
  %calcOffset220 = add i32 %stack_addr, -1648
  %49 = inttoptr i32 %calcOffset220 to i32*
  store i32 1987, i32* %49, align 4
  %50 = load i32, i32* null, align 2147483648
  %51 = call i32 @fprintf(i32 %50, i32 1191, i32 %calcOffset220)
  call void @exit(i32 1)
  unreachable

brif_next78:                                      ; preds = %brif_next61
  %calcOffset79 = add i32 %stack_addr, -1568
  %52 = inttoptr i32 %calcOffset79 to i32*
  store i32 %_local_7.0, i32* %52, align 4
  %53 = add i32 %45, 5
  %54 = call i32 @snprintf(i32 %47, i32 %53, i32 1544, i32 %calcOffset79)
  %55 = call i32 @stat(i32 %47, i32 %.pre-phi230)
  %brif_val81.not = icmp eq i32 %55, 0
  br i1 %brif_val81.not, label %brif_next82, label %blk_exit80

brif_next82:                                      ; preds = %brif_next78
  store i8 1, i8* %43, align 1
  br label %blk_exit80

blk_exit80:                                       ; preds = %brif_next82, %brif_next78
  call void @free(i32 %47)
  %calcOffset84 = add i32 %21, 72
  %56 = inttoptr i32 %calcOffset84 to i32*
  store i32 %_arg_5, i32* %56, align 4
  %brif_val91.not = icmp slt i32 %9, 0
  %or.cond229 = select i1 %brif_val91.not, i1 true, i1 %14
  br i1 %or.cond229, label %blk_exit90, label %blk_exit89

blk_exit90:                                       ; preds = %blk_exit80
  %57 = load i32, i32* %38, align 4
  %58 = call i32 @close(i32 %57)
  store i32 -1, i32* %38, align 4
  %59 = load i32, i32* %44, align 4
  %60 = call i32 @opendir(i32 %59)
  %brif_val99.not = icmp eq i32 %60, 0
  br i1 %brif_val99.not, label %brif_next100, label %blk_exit97

brif_next100:                                     ; preds = %blk_exit90
  %61 = load i32, i32* %44, align 4
  call void @perror(i32 %61)
  br label %blk_exit88

blk_exit97:                                       ; preds = %blk_exit90
  %calcOffset102 = add i32 %21, 64
  %62 = inttoptr i32 %calcOffset102 to i32*
  store i32 0, i32* %62, align 4
  %calcOffset103 = add i32 %stack_addr, -512
  %63 = inttoptr i32 %calcOffset103 to i32*
  store i32 0, i32* %63, align 4
  %64 = call i32 @malloc(i32 800)
  %65 = icmp eq i32 %64, 0
  br i1 %65, label %blk_exit2, label %loop_entry107

blk_exit2:                                        ; preds = %blk_exit97
  call void @perror(i32 1569)
  call void @exit(i32 -1)
  unreachable

loop_entry107:                                    ; preds = %blk_exit97, %blk_exit113, %brif_next109
  %_local_8.1 = phi i32 [ %_local_8.1, %brif_next109 ], [ %108, %blk_exit113 ], [ 0, %blk_exit97 ]
  %_local_10.0 = phi i32 [ %_local_10.0, %brif_next109 ], [ %_local_10.1, %blk_exit113 ], [ %64, %blk_exit97 ]
  %_local_11.0 = phi i32 [ %_local_11.0, %brif_next109 ], [ %_local_11.1, %blk_exit113 ], [ 200, %blk_exit97 ]
  %66 = call i32 @readdir(i32 %60)
  %67 = icmp eq i32 %66, 0
  br i1 %67, label %blk_exit106, label %brif_next109

blk_exit106:                                      ; preds = %loop_entry107
  %68 = call i32 @closedir(i32 %60)
  call void @qsort(i32 %_local_10.0, i32 %_local_8.1, i32 4, i32 1)
  %69 = icmp eq i32 %_local_8.1, 0
  br i1 %69, label %blk_exit119, label %loop_entry122

blk_exit119:                                      ; preds = %blk_exit123, %blk_exit106
  call void @free(i32 %_local_10.0)
  %70 = load i32, i32* %62, align 4
  %brif_val134.not = icmp eq i32 %70, 0
  br i1 %brif_val134.not, label %brif_next135, label %blk_exit85

brif_next135:                                     ; preds = %blk_exit119
  %71 = load i32, i32* %44, align 4
  %72 = call i32 @strcmp(i32 1121, i32 %71)
  %73 = icmp eq i32 %72, 0
  br i1 %73, label %blk_exit87, label %brif_next138

blk_exit87:                                       ; preds = %brif_next138, %brif_next135
  call void @free(i32 %_local_7.0)
  %74 = inttoptr i32 %calcOffset71 to i32*
  %75 = load i32, i32* %74, align 4
  call void @free(i32 %75)
  %calcOffset192 = add i32 %21, 32
  %76 = inttoptr i32 %calcOffset192 to i32*
  %77 = load i32, i32* %76, align 4
  call void @free(i32 %77)
  %78 = load i32, i32* %41, align 4
  call void @free(i32 %78)
  %79 = load i32, i32* %44, align 4
  call void @free(i32 %79)
  %80 = load i32, i32* %38, align 4
  %brif_val197.not = icmp slt i32 %80, 0
  br i1 %brif_val197.not, label %blk_exit11, label %blk_exit195

blk_exit195:                                      ; preds = %blk_exit87
  %81 = call i32 @close(i32 %80)
  br label %blk_exit11

brif_next138:                                     ; preds = %brif_next135
  %82 = call i32 @strcmp(i32 1282, i32 %71)
  %83 = icmp eq i32 %82, 0
  br i1 %83, label %blk_exit87, label %brif_next140

brif_next140:                                     ; preds = %brif_next138
  %calcOffset141 = add i32 %stack_addr, -1584
  %84 = inttoptr i32 %calcOffset141 to i32*
  store i32 %71, i32* %84, align 4
  %85 = load i32, i32* null, align 2147483648
  %86 = call i32 @fprintf(i32 %85, i32 1942, i32 %calcOffset141)
  br label %blk_exit88

loop_entry122:                                    ; preds = %blk_exit106, %blk_exit123
  %_local_8.2 = phi i32 [ %94, %blk_exit123 ], [ %_local_8.1, %blk_exit106 ]
  %_param_1.0 = phi i32 [ %93, %blk_exit123 ], [ %_local_10.0, %blk_exit106 ]
  %87 = inttoptr i32 %_param_1.0 to i32*
  %88 = load i32, i32* %87, align 4
  %89 = load i32, i32* %44, align 4
  %90 = call i32 @add_file(i32 -1, i32 %88, i32 %89, i32 %calcOffset70, i32 %calcOffset103, i32 %21)
  %91 = icmp eq i32 %90, 0
  br i1 %91, label %blk_exit123, label %brif_next127

blk_exit123:                                      ; preds = %brif_next127, %loop_entry122
  %92 = load i32, i32* %87, align 4
  call void @free(i32 %92)
  %93 = add i32 %_param_1.0, 4
  %94 = add i32 %_local_8.2, -1
  %brif_val131.not = icmp eq i32 %94, 0
  br i1 %brif_val131.not, label %blk_exit119, label %loop_entry122

brif_next127:                                     ; preds = %loop_entry122
  %95 = load i32, i32* %62, align 4
  %96 = add i32 %95, 1
  store i32 %96, i32* %62, align 4
  br label %blk_exit123

brif_next109:                                     ; preds = %loop_entry107
  %calcOffset110 = add i32 %66, 9
  %97 = inttoptr i32 %calcOffset110 to i8*
  %98 = load i8, i8* %97, align 1
  %99 = icmp eq i8 %98, 0
  br i1 %99, label %loop_entry107, label %brif_next112

brif_next112:                                     ; preds = %brif_next109
  %100 = call i32 @strdup(i32 %calcOffset110)
  %.not224 = icmp eq i32 %_local_8.1, %_local_11.0
  br i1 %.not224, label %brif_next115, label %blk_exit113

brif_next115:                                     ; preds = %brif_next112
  %101 = add i32 %_local_8.1, 200
  %102 = shl i32 %101, 2
  %103 = call i32 @realloc(i32 %_local_10.0, i32 %102)
  %104 = icmp eq i32 %103, 0
  br i1 %104, label %blk_exit1, label %blk_exit113

blk_exit1:                                        ; preds = %brif_next115
  call void @perror(i32 1569)
  call void @exit(i32 -1)
  unreachable

blk_exit113:                                      ; preds = %brif_next115, %brif_next112
  %_local_10.1 = phi i32 [ %103, %brif_next115 ], [ %_local_10.0, %brif_next112 ]
  %_local_11.1 = phi i32 [ %101, %brif_next115 ], [ %_local_11.0, %brif_next112 ]
  %105 = shl i32 %_local_8.1, 2
  %106 = add i32 %_local_10.1, %105
  %107 = inttoptr i32 %106 to i32*
  store i32 %100, i32* %107, align 4
  %108 = add i32 %_local_8.1, 1
  br label %loop_entry107

blk_exit88:                                       ; preds = %brif_next178, %brif_next171, %brif_next169, %brif_next167, %brif_next165, %brif_next163, %brif_next161, %brif_next159, %brif_next157, %brif_next155, %brif_next153, %brif_next151, %brif_next149, %brif_next147, %blk_exit89, %brif_next140, %brif_next100
  %brif_val180.not = icmp eq i32 %_arg_5, 0
  br i1 %brif_val180.not, label %brif_next181, label %blk_exit179

brif_next181:                                     ; preds = %blk_exit88
  %calcOffset182 = add i32 %stack_addr, -1632
  %109 = inttoptr i32 %calcOffset182 to i32*
  store i32 %_local_7.0, i32* %109, align 4
  %110 = load i32, i32* null, align 2147483648
  %111 = call i32 @fprintf(i32 %110, i32 1581, i32 %calcOffset182)
  br label %blk_exit179

blk_exit179:                                      ; preds = %brif_next181, %blk_exit88
  call void @free(i32 %_local_7.0)
  %112 = inttoptr i32 %calcOffset71 to i32*
  %113 = load i32, i32* %112, align 4
  call void @free(i32 %113)
  %calcOffset184 = add i32 %21, 32
  %114 = inttoptr i32 %calcOffset184 to i32*
  %115 = load i32, i32* %114, align 4
  call void @free(i32 %115)
  %116 = load i32, i32* %41, align 4
  call void @free(i32 %116)
  %117 = load i32, i32* %44, align 4
  call void @free(i32 %117)
  %118 = load i32, i32* %38, align 4
  %brif_val189.not = icmp slt i32 %118, 0
  br i1 %brif_val189.not, label %blk_exit11, label %blk_exit187

blk_exit187:                                      ; preds = %blk_exit179
  %119 = call i32 @close(i32 %118)
  br label %blk_exit11

blk_exit85:                                       ; preds = %blk_exit86, %blk_exit119
  %120 = inttoptr i32 %_arg_3 to i32*
  %121 = load i32, i32* %120, align 4
  %brif_val203.not = icmp eq i32 %121, 0
  br i1 %brif_val203.not, label %brif_next204, label %blk_exit201

brif_next204:                                     ; preds = %blk_exit85
  %122 = inttoptr i32 %_arg_4 to i32*
  store i32 %21, i32* %122, align 4
  br label %blk_exit200

blk_exit200:                                      ; preds = %blk_exit206, %brif_next209, %brif_next204
  %.pre-phi = phi i32* [ %120, %blk_exit206 ], [ %124, %brif_next209 ], [ %120, %brif_next204 ]
  store i32 %21, i32* %.pre-phi, align 4
  br label %blk_exit11

blk_exit201:                                      ; preds = %blk_exit85
  %123 = load i32, i32* %39, align 4
  %.not226 = icmp eq i32 %123, -1
  br i1 %.not226, label %brif_next209, label %blk_exit206

brif_next209:                                     ; preds = %blk_exit201
  %124 = inttoptr i32 %_arg_4 to i32*
  %125 = load i32, i32* %124, align 4
  %calcOffset211 = add i32 %125, 76
  %126 = inttoptr i32 %calcOffset211 to i32*
  store i32 %21, i32* %126, align 4
  %127 = load i32, i32* %124, align 4
  %calcOffset213 = add i32 %21, 80
  %128 = inttoptr i32 %calcOffset213 to i32*
  store i32 %127, i32* %128, align 4
  br label %blk_exit200

blk_exit206:                                      ; preds = %blk_exit201
  %calcOffset214 = add i32 %121, 80
  %129 = inttoptr i32 %calcOffset214 to i32*
  store i32 %21, i32* %129, align 4
  %130 = load i32, i32* %120, align 4
  %131 = inttoptr i32 %calcOffset68 to i32*
  store i32 %130, i32* %131, align 4
  br label %blk_exit200

blk_exit89:                                       ; preds = %blk_exit80
  %132 = call i32 @strrchr(i32 %_local_7.0, i32 47)
  %133 = add i32 %132, 1
  %select_cond.not = icmp eq i32 %132, 0
  %134 = select i1 %select_cond.not, i32 %_local_7.0, i32 %133
  %135 = inttoptr i32 %134 to i8*
  %136 = load i8, i8* %135, align 1
  %137 = icmp eq i8 %136, 46
  br i1 %137, label %blk_exit88, label %brif_next144

brif_next144:                                     ; preds = %blk_exit89
  %138 = call i32 @strrchr(i32 %134, i32 46)
  %139 = icmp eq i32 %138, 0
  br i1 %139, label %blk_exit145, label %brif_next147

blk_exit145:                                      ; preds = %brif_next171, %brif_next144
  %140 = call i32 @strlen(i32 %_local_7.0)
  %141 = add i32 %140, 7
  %142 = call i32 @malloc(i32 %141)
  %143 = icmp eq i32 %142, 0
  br i1 %143, label %blk_exit, label %brif_next175

blk_exit:                                         ; preds = %blk_exit145
  %calcOffset221 = add i32 %stack_addr, -1616
  %144 = inttoptr i32 %calcOffset221 to i32*
  store i32 1987, i32* %144, align 4
  %145 = load i32, i32* null, align 2147483648
  %146 = call i32 @fprintf(i32 %145, i32 1191, i32 %calcOffset221)
  call void @exit(i32 1)
  unreachable

brif_next175:                                     ; preds = %blk_exit145
  %calcOffset176 = add i32 %stack_addr, -1600
  %147 = inttoptr i32 %calcOffset176 to i32*
  store i32 %_local_7.0, i32* %147, align 4
  %148 = add i32 %140, 6
  %149 = call i32 @snprintf(i32 %142, i32 %148, i32 1074, i32 %calcOffset176)
  %150 = call i32 @access(i32 %142, i32 4)
  %brif_val177.not = icmp slt i32 %150, 0
  br i1 %brif_val177.not, label %brif_next178, label %blk_exit86

brif_next178:                                     ; preds = %brif_next175
  call void @free(i32 %142)
  br label %blk_exit88

blk_exit86:                                       ; preds = %brif_next175
  %151 = inttoptr i32 %calcOffset71 to i32*
  store i32 %142, i32* %151, align 4
  br label %blk_exit85

brif_next147:                                     ; preds = %brif_next144
  %152 = add i32 %138, 1
  %153 = call i32 @strcmp(i32 %152, i32 1077)
  %154 = icmp eq i32 %153, 0
  br i1 %154, label %blk_exit88, label %brif_next149

brif_next149:                                     ; preds = %brif_next147
  %155 = call i32 @strcmp(i32 %152, i32 1085)
  %156 = icmp eq i32 %155, 0
  br i1 %156, label %blk_exit88, label %brif_next151

brif_next151:                                     ; preds = %brif_next149
  %157 = call i32 @strcmp(i32 %152, i32 1412)
  %158 = icmp eq i32 %157, 0
  br i1 %158, label %blk_exit88, label %brif_next153

brif_next153:                                     ; preds = %brif_next151
  %159 = call i32 @strcmp(i32 %152, i32 1244)
  %160 = icmp eq i32 %159, 0
  br i1 %160, label %blk_exit88, label %brif_next155

brif_next155:                                     ; preds = %brif_next153
  %161 = call i32 @strcmp(i32 %152, i32 1196)
  %162 = icmp eq i32 %161, 0
  br i1 %162, label %blk_exit88, label %brif_next157

brif_next157:                                     ; preds = %brif_next155
  %163 = call i32 @strcmp(i32 %152, i32 1242)
  %164 = icmp eq i32 %163, 0
  br i1 %164, label %blk_exit88, label %brif_next159

brif_next159:                                     ; preds = %brif_next157
  %165 = call i32 @strcmp(i32 %152, i32 1322)
  %166 = icmp eq i32 %165, 0
  br i1 %166, label %blk_exit88, label %brif_next161

brif_next161:                                     ; preds = %brif_next159
  %167 = call i32 @strcmp(i32 %152, i32 1163)
  %168 = icmp eq i32 %167, 0
  br i1 %168, label %blk_exit88, label %brif_next163

brif_next163:                                     ; preds = %brif_next161
  %169 = call i32 @strcmp(i32 %152, i32 1204)
  %170 = icmp eq i32 %169, 0
  br i1 %170, label %blk_exit88, label %brif_next165

brif_next165:                                     ; preds = %brif_next163
  %171 = call i32 @strcmp(i32 %152, i32 1336)
  %172 = icmp eq i32 %171, 0
  br i1 %172, label %blk_exit88, label %brif_next167

brif_next167:                                     ; preds = %brif_next165
  %173 = call i32 @strcmp(i32 %152, i32 1159)
  %174 = icmp eq i32 %173, 0
  br i1 %174, label %blk_exit88, label %brif_next169

brif_next169:                                     ; preds = %brif_next167
  %175 = call i32 @strcmp(i32 %152, i32 1200)
  %176 = icmp eq i32 %175, 0
  br i1 %176, label %blk_exit88, label %brif_next171

brif_next171:                                     ; preds = %brif_next169
  %177 = call i32 @strcmp(i32 %152, i32 1221)
  %178 = icmp eq i32 %177, 0
  br i1 %178, label %blk_exit88, label %blk_exit145

blk_exit25:                                       ; preds = %brif_next27, %brif_next23
  %brif_val33.not = icmp eq i32 %_arg_5, 0
  %or.cond228 = select i1 %brif_val.not, i1 %brif_val33.not, i1 false
  br i1 %or.cond228, label %brif_next34, label %blk_exit11

brif_next34:                                      ; preds = %blk_exit25
  %179 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %180 = icmp eq i32 %179, 0
  br i1 %180, label %blk_exit35, label %brif_next37

blk_exit35:                                       ; preds = %brif_next34
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

brif_next37:                                      ; preds = %brif_next34
  %181 = add i32 %stack_addr, -512
  %182 = call i32 @strncpy(i32 %181, i32 %179, i32 512)
  %calcOffset38 = add i32 %stack_addr, -1
  %183 = inttoptr i32 %calcOffset38 to i8*
  store i8 0, i8* %183, align 1
  br label %loop_entry

loop_entry:                                       ; preds = %brif_next55, %brif_next37
  %_param_2.0 = phi i32 [ %181, %brif_next37 ], [ %storemerge, %brif_next55 ]
  %184 = inttoptr i32 %_param_2.0 to i8*
  %185 = load i8, i8* %184, align 1
  %186 = icmp eq i8 %185, 0
  br i1 %186, label %blk_exit40, label %brif_next43

blk_exit40:                                       ; preds = %blk_exit53, %loop_entry
  call void @perror(i32 %_local_7.0)
  br label %blk_exit11

brif_next43:                                      ; preds = %loop_entry
  %187 = call i32 @strchr(i32 %_param_2.0, i32 58)
  %brif_val46.not = icmp eq i32 %187, 0
  br i1 %brif_val46.not, label %blk_exit44, label %blk_exit45

blk_exit44:                                       ; preds = %brif_next43, %blk_exit45
  %storemerge = phi i32 [ %198, %blk_exit45 ], [ 0, %brif_next43 ]
  %calcOffset49 = add i32 %stack_addr, -1676
  %188 = inttoptr i32 %calcOffset49 to i32*
  store i32 %_param_2.0, i32* %188, align 4
  %calcOffset50 = add i32 %stack_addr, -1680
  %189 = inttoptr i32 %calcOffset50 to i32*
  store i32 1089, i32* %189, align 4
  %190 = call i32 @snprintf(i32 %.pre-phi230, i32 1024, i32 1174, i32 %calcOffset50)
  %191 = call i32 @strncmp(i32 %_local_7.0, i32 %_param_2.0, i32 2)
  %192 = icmp eq i32 %191, 0
  br i1 %192, label %blk_exit11, label %brif_next52

brif_next52:                                      ; preds = %blk_exit44
  %193 = call i32 @strlen(i32 1089)
  %194 = add i32 %193, 3
  %195 = call i32 @strncmp(i32 %_local_7.0, i32 %.pre-phi230, i32 %194)
  %196 = icmp eq i32 %195, 0
  br i1 %196, label %blk_exit11, label %brif_next55

brif_next55:                                      ; preds = %brif_next52
  %brif_val56.not = icmp eq i32 %storemerge, 0
  br i1 %brif_val56.not, label %blk_exit53, label %loop_entry

blk_exit53:                                       ; preds = %brif_next55
  br i1 %196, label %blk_exit11, label %blk_exit40

blk_exit45:                                       ; preds = %brif_next43
  %197 = inttoptr i32 %187 to i8*
  store i8 0, i8* %197, align 1
  %198 = add i32 %187, 1
  br label %blk_exit44

blk_exit11:                                       ; preds = %brif_next52, %blk_exit87, %blk_exit179, %blk_exit44, %blk_exit53, %blk_exit40, %brif_next17, %blk_exit200, %blk_exit195, %blk_exit187, %blk_exit35, %blk_exit25, %blk_exit12, %brif_next17
  %_local_8.0 = phi i32 [ 0, %blk_exit187 ], [ 1, %blk_exit195 ], [ 1, %blk_exit200 ], [ 0, %blk_exit35 ], [ 0, %blk_exit25 ], [ 0, %blk_exit12 ], [ 0, %brif_next17 ], [ 0, %brif_next17 ], [ 0, %blk_exit40 ], [ 0, %blk_exit53 ], [ 0, %blk_exit44 ], [ 0, %blk_exit179 ], [ 1, %blk_exit87 ], [ 0, %brif_next52 ]
  %_local_7.1 = phi i32 [ %21, %blk_exit187 ], [ %21, %blk_exit195 ], [ %_local_7.0, %blk_exit200 ], [ %_local_7.0, %blk_exit35 ], [ %_local_7.0, %blk_exit25 ], [ %_local_7.0, %blk_exit12 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %brif_next17 ], [ %_local_7.0, %blk_exit40 ], [ %_local_7.0, %blk_exit53 ], [ %_local_7.0, %blk_exit44 ], [ %21, %blk_exit179 ], [ %21, %blk_exit87 ], [ %_local_7.0, %brif_next52 ]
  call void @free(i32 %_local_7.1)
  ret i32 %_local_8.0

blk_exit6.blk_exit12_crit_edge:                   ; preds = %blk_exit6
  %.pre = add i32 %stack_addr, -1536
  br label %blk_exit12

blk_exit7:                                        ; preds = %allocator
  %199 = call i32 @strlen(i32 %_arg_2)
  %200 = call i32 @strlen(i32 %_arg_1)
  %201 = add i32 %199, %200
  %202 = add i32 %201, 13
  %203 = call i32 @malloc(i32 %202)
  %204 = icmp eq i32 %203, 0
  br i1 %204, label %blk_exit5, label %brif_next9

blk_exit5:                                        ; preds = %blk_exit7
  %205 = inttoptr i32 %stack_end to i32*
  store i32 1987, i32* %205, align 4
  %206 = load i32, i32* null, align 2147483648
  %207 = call i32 @fprintf(i32 %206, i32 1191, i32 %stack_end)
  call void @exit(i32 1)
  unreachable

brif_next9:                                       ; preds = %blk_exit7
  %calcOffset = add i32 %stack_addr, -1548
  %208 = inttoptr i32 %calcOffset to i32*
  store i32 %_arg_1, i32* %208, align 4
  %calcOffset10 = add i32 %stack_addr, -1552
  %209 = inttoptr i32 %calcOffset10 to i32*
  store i32 %_arg_2, i32* %209, align 4
  %210 = add i32 %201, 12
  %211 = call i32 @snprintf(i32 %203, i32 %210, i32 1174, i32 %calcOffset10)
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

loop_entry:                                       ; preds = %allocator, %blk_exit1
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

blk_exit1:                                        ; preds = %blk_exit2, %brif_next4
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
  %_param_0.0 = phi i32 [ %_arg_0, %brif_next ], [ %79, %blk_exit2 ]
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

loop_entry16:                                     ; preds = %blk_exit6, %blk_exit17
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
  %_local_8.0 = phi i32 [ %24, %blk_exit18 ], [ %72, %brif_next33 ], [ %24, %brif_next31 ]
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

blk_exit57:                                       ; preds = %blk_exit70, %blk_exit63, %brif_next65, %brif_next60
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

loop_entry41:                                     ; preds = %brif_next37, %blk_exit42
  %_local_6.1 = phi i32 [ %65, %blk_exit42 ], [ %_local_8.0, %brif_next37 ]
  %_local_5.1.in = phi i8 [ %67, %blk_exit42 ], [ %56, %brif_next37 ]
  %_local_5.1 = zext i8 %_local_5.1.in to i32
  %58 = add nsw i32 %_local_5.1, -91
  %59 = icmp uge i32 %58, -26
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

blk_exit42:                                       ; preds = %blk_exit43, %blk_exit44
  %65 = add i32 %_local_6.1, 1
  %66 = inttoptr i32 %65 to i8*
  %67 = load i8, i8* %66, align 1
  %brif_val55.not = icmp eq i8 %67, 0
  br i1 %brif_val55.not, label %blk_exit34, label %loop_entry41

blk_exit44:                                       ; preds = %loop_entry41
  %68 = add nsw i32 %_local_5.1, -123
  %69 = icmp uge i32 %68, -26
  %70 = and i32 %_local_5.1, 128
  %brif_val51.not = icmp eq i32 %70, 0
  %or.cond95 = and i1 %69, %brif_val51.not
  br i1 %or.cond95, label %blk_exit43, label %blk_exit42

brif_next31:                                      ; preds = %blk_exit18
  %71 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val32.not = icmp eq i8 %71, 0
  br i1 %brif_val32.not, label %brif_next33, label %blk_exit28

brif_next33:                                      ; preds = %brif_next31
  %72 = call i32 @strdup(i32 %24)
  br label %blk_exit28

blk_exit17:                                       ; preds = %blk_exit80, %blk_exit19
  %_local_7.1 = phi i32 [ %_local_7.2, %blk_exit80 ], [ %_local_7.0, %blk_exit19 ]
  %storemerge = phi i32 [ %52, %blk_exit80 ], [ %77, %blk_exit19 ]
  %73 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %74 = load i32, i32* %5, align 4
  %75 = call i32 @fgets(i32 %storemerge, i32 %73, i32 %74)
  %brif_val87.not = icmp eq i32 %75, 0
  br i1 %brif_val87.not, label %blk_exit2, label %loop_entry16

blk_exit19:                                       ; preds = %brif_next23, %loop_entry16
  %76 = call i32 @strlen(i32 %_local_5.0)
  %77 = add i32 %_local_5.0, %76
  br label %blk_exit17

blk_exit2:                                        ; preds = %blk_exit17, %blk_exit6, %brif_next5
  %calcOffset89 = add i32 %_param_0.0, 76
  %78 = inttoptr i32 %calcOffset89 to i32*
  %79 = load i32, i32* %78, align 4
  %brif_val90.not = icmp eq i32 %79, 0
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
  %69 = add i32 %_arg_0, 44
  %70 = inttoptr i32 %69 to i8*
  call void @llvm.memset.p0i8.i64(i8* %70, i8 0, i64 8, i1 true)
  br label %loop_entry

loop_entry:                                       ; preds = %blk_exit49, %blk_exit9
  %_local_2.0 = phi i32 [ %3, %blk_exit9 ], [ %87, %blk_exit49 ]
  call void @get_tbl(i32 %_local_2.0)
  %71 = inttoptr i32 %68 to i32*
  %72 = load i32, i32* %71, align 4
  %73 = add i32 %_local_2.0, 44
  %74 = inttoptr i32 %73 to i32*
  %75 = load i32, i32* %74, align 4
  %76 = add i32 %72, %75
  store i32 %76, i32* %71, align 4
  %calcOffset44 = add i32 %_arg_0, 48
  %77 = inttoptr i32 %calcOffset44 to i32*
  %78 = load i32, i32* %77, align 4
  %79 = add i32 %_local_2.0, 48
  %80 = inttoptr i32 %79 to i32*
  %81 = load i32, i32* %80, align 4
  %.not77 = icmp ult i32 %78, %81
  br i1 %.not77, label %brif_next47, label %blk_exit43

brif_next47:                                      ; preds = %loop_entry
  store i32 %81, i32* %77, align 4
  br label %blk_exit43

blk_exit43:                                       ; preds = %brif_next47, %loop_entry
  %82 = load i32, i32* %67, align 4
  %83 = add i32 %_local_2.0, 52
  %84 = inttoptr i32 %83 to i32*
  %85 = load i32, i32* %84, align 4
  %.not78 = icmp ugt i32 %82, %85
  br i1 %.not78, label %brif_next53, label %blk_exit49

brif_next53:                                      ; preds = %blk_exit43
  store i32 %85, i32* %67, align 4
  br label %blk_exit49

blk_exit49:                                       ; preds = %brif_next53, %blk_exit43
  %calcOffset55 = add i32 %_local_2.0, 76
  %86 = inttoptr i32 %calcOffset55 to i32*
  %87 = load i32, i32* %86, align 4
  %brif_val56.not = icmp eq i32 %87, 0
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

blk_exit:                                         ; preds = %blk_exit40, %blk_exit17, %allocator
  ret void

brif_next:                                        ; preds = %allocator
  %1 = add i32 %_arg_1, 1
  %2 = shl i32 %_arg_1, 2
  %3 = load i32, i32* null, align 2147483648
  %4 = icmp eq i32 %_arg_1, 0
  br i1 %4, label %loop_entry24, label %loop_entry

loop_entry24:                                     ; preds = %brif_next, %blk_exit40
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

blk_exit27:                                       ; preds = %blk_exit32, %brif_next34, %brif_next31
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

loop_entry:                                       ; preds = %brif_next, %blk_exit17
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

loop_entry:                                       ; preds = %allocator, %blk_exit5
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
