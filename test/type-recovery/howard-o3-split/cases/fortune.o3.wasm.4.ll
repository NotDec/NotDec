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
define dso_local i32 @main(i32 %_arg_0, i32 %_arg_1) #0 {
allocator:
  %0 = load i32, i32* @__stack_pointer, align 4
  %1 = add i32 %0, -2960
  store i32 %1, i32* @__stack_pointer, align 4
  %2 = call i32 @getenv(i32 1497)
  store i32 %2, i32* inttoptr (i32 2224 to i32*), align 16
  %brif_val.not = icmp eq i32 %2, 0
  br i1 %brif_val.not, label %brif_next, label %blk_exit

blk_exit:                                         ; preds = %brif_next4, %brif_next2, %brif_next, %allocator
  br label %loop_entry

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

blk_exit5:                                        ; preds = %loop_entry
  %6 = load i32, i32* null, align 2147483648
  %7 = sub i32 %_arg_0, %6
  %8 = icmp eq i32 %7, 0
  br i1 %8, label %blk_exit43, label %brif_next45

blk_exit6:                                        ; preds = %loop_entry
  call void @usage()
  unreachable

loop_entry:                                       ; preds = %blk_exit18, %blk_exit17, %blk_exit16, %blk_exit15, %blk_exit14, %blk_exit13, %blk_exit12, %blk_exit11, %blk_exit10, %blk_exit9, %blk_exit7, %loop_entry, %blk_exit
  %_local_4.0 = phi i32 [ 0, %blk_exit ], [ %_local_4.0, %blk_exit18 ], [ %_local_4.0, %blk_exit7 ], [ %_local_4.0, %blk_exit17 ], [ %_local_4.0, %blk_exit16 ], [ %_local_4.0, %loop_entry ], [ %_local_4.0, %blk_exit15 ], [ %12, %blk_exit10 ], [ %_local_4.0, %blk_exit14 ], [ %_local_4.0, %blk_exit13 ], [ %_local_4.0, %blk_exit12 ], [ %_local_4.0, %blk_exit9 ], [ %_local_4.0, %blk_exit11 ]
  %_local_3.0 = phi i32 [ 0, %blk_exit ], [ %_local_3.0, %blk_exit18 ], [ %_local_3.0, %blk_exit7 ], [ %_local_3.0, %blk_exit17 ], [ %_local_3.0, %blk_exit16 ], [ 1, %loop_entry ], [ %_local_3.0, %blk_exit15 ], [ %_local_3.0, %blk_exit10 ], [ %_local_3.0, %blk_exit14 ], [ %_local_3.0, %blk_exit13 ], [ %_local_3.0, %blk_exit12 ], [ %_local_3.0, %blk_exit9 ], [ %_local_3.0, %blk_exit11 ]
  %9 = call i32 @getopt(i32 %_arg_0, i32 %_arg_1, i32 1024)
  switch i32 %9, label %blk_exit6 [
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

blk_exit7:                                        ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2295 to i8*), align 1
  br label %loop_entry

blk_exit8:                                        ; preds = %loop_entry
  call void @program_version()
  %calcOffset = add i32 %0, -2592
  %10 = inttoptr i32 %calcOffset to i32*
  store i32 2304, i32* %10, align 4
  %11 = call i32 @printf(i32 1755, i32 %calcOffset)
  call void @exit(i32 0)
  unreachable

blk_exit9:                                        ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2294 to i8*), align 2
  br label %loop_entry

blk_exit10:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2228 to i8*), align 4
  %12 = load i32, i32* null, align 2147483648
  br label %loop_entry

blk_exit11:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2280 to i8*), align 8
  br label %loop_entry

blk_exit12:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2273 to i8*), align 1
  store i8 0, i8* inttoptr (i32 2274 to i8*), align 2
  br label %loop_entry

blk_exit13:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2293 to i8*), align 1
  br label %loop_entry

blk_exit14:                                       ; preds = %loop_entry
  %13 = load i32, i32* null, align 2147483648
  %14 = call i32 @atoi(i32 %13)
  store i32 %14, i32* inttoptr (i32 2208 to i32*), align 32
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

blk_exit18:                                       ; preds = %loop_entry
  store i8 1, i8* inttoptr (i32 2292 to i8*), align 4
  br label %loop_entry

blk_exit19:                                       ; preds = %blk_exit731, %blk_exit683, %brif_next318, %blk_exit20
  %_local_5.0 = phi i32 [ 0, %blk_exit683 ], [ 0, %blk_exit731 ], [ 0, %blk_exit20 ], [ %345, %brif_next318 ]
  %15 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  call void @free_desc(i32 %15)
  %16 = load i32, i32* inttoptr (i32 2288 to i32*), align 16
  call void @free(i32 %16)
  call void @exit(i32 %_local_5.0)
  unreachable

blk_exit20:                                       ; preds = %brif_next403, %blk_exit367, %blk_exit21
  %_param_0.0 = phi i32 [ %17, %blk_exit21 ], [ 0, %blk_exit367 ], [ %_local_3.17, %brif_next403 ]
  call void @print_list(i32 %_param_0.0, i32 0)
  br label %blk_exit19

blk_exit21:                                       ; preds = %brif_next390, %blk_exit370, %blk_exit369
  %17 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit20

blk_exit22:                                       ; preds = %brif_next739
  %calcOffset769 = add i32 %772, 24
  %18 = inttoptr i32 %calcOffset769 to i32*
  %19 = load i32, i32* %18, align 4
  call void @perror(i32 %19)
  call void @exit(i32 1)
  unreachable

blk_exit23:                                       ; preds = %brif_next673
  %calcOffset768 = add i32 %671, 24
  %20 = inttoptr i32 %calcOffset768 to i32*
  %21 = load i32, i32* %20, align 4
  call void @perror(i32 %21)
  call void @exit(i32 1)
  unreachable

blk_exit24:                                       ; preds = %brif_next647
  %calcOffset767 = add i32 %674, 24
  %22 = inttoptr i32 %calcOffset767 to i32*
  %23 = load i32, i32* %22, align 4
  call void @perror(i32 %23)
  call void @exit(i32 1)
  unreachable

blk_exit25:                                       ; preds = %brif_next613
  %calcOffset766 = add i32 %628, 24
  %24 = inttoptr i32 %calcOffset766 to i32*
  %25 = load i32, i32* %24, align 4
  call void @perror(i32 %25)
  call void @exit(i32 1)
  unreachable

blk_exit26:                                       ; preds = %brif_next598, %blk_exit587
  call void @exit(i32 1)
  unreachable

blk_exit27:                                       ; preds = %brif_next590
  call void @exit(i32 1)
  unreachable

blk_exit28:                                       ; preds = %blk_exit489
  call void @__assert_fail(i32 1229, i32 1342, i32 1621, i32 1081)
  unreachable

blk_exit29:                                       ; preds = %blk_exit406
  %calcOffset765 = add i32 %0, -2912
  %26 = inttoptr i32 %calcOffset765 to i32*
  store i32 1814, i32* %26, align 4
  %27 = load i32, i32* null, align 2147483648
  %28 = call i32 @fprintf(i32 %27, i32 1191, i32 %calcOffset765)
  call void @exit(i32 1)
  unreachable

blk_exit30:                                       ; preds = %blk_exit334
  %29 = load i32, i32* null, align 2147483648
  %30 = call i32 @fprintf(i32 %29, i32 1655, i32 0)
  call void @exit(i32 1)
  unreachable

blk_exit31:                                       ; preds = %blk_exit321
  %calcOffset764 = add i32 %0, -2928
  %31 = inttoptr i32 %calcOffset764 to i32*
  store i32 %_param_1.4, i32* %31, align 4
  %32 = load i32, i32* null, align 2147483648
  %33 = call i32 @fprintf(i32 %32, i32 2012, i32 %calcOffset764)
  call void @exit(i32 1)
  unreachable

blk_exit32:                                       ; preds = %brif_next331
  %calcOffset763 = add i32 %0, -2880
  %34 = inttoptr i32 %calcOffset763 to i32*
  store i32 %358, i32* %34, align 4
  %35 = load i32, i32* null, align 2147483648
  %36 = call i32 @fprintf(i32 %35, i32 2122, i32 %calcOffset763)
  call void @exit(i32 1)
  unreachable

blk_exit33:                                       ; preds = %brif_next316
  %calcOffset762 = add i32 %0, -2944
  %37 = inttoptr i32 %calcOffset762 to i32*
  store i32 1987, i32* %37, align 4
  %38 = load i32, i32* null, align 2147483648
  %39 = call i32 @fprintf(i32 %38, i32 1191, i32 %calcOffset762)
  call void @exit(i32 1)
  unreachable

blk_exit34:                                       ; preds = %blk_exit39
  %calcOffset761 = add i32 %0, -2864
  %40 = inttoptr i32 %calcOffset761 to i32*
  store i32 1759, i32* %40, align 4
  %41 = load i32, i32* null, align 2147483648
  %42 = call i32 @fprintf(i32 %41, i32 1191, i32 %calcOffset761)
  call void @exit(i32 1)
  unreachable

blk_exit35:                                       ; preds = %blk_exit42, %blk_exit38, %blk_exit36
  %43 = call i32 @setlocale(i32 6, i32 2206)
  %44 = load i8, i8* inttoptr (i32 2228 to i8*), align 4
  %45 = icmp eq i8 %44, 0
  br i1 %45, label %blk_exit314, label %brif_next316

blk_exit36:                                       ; preds = %blk_exit284
  call void @free(i32 %51)
  br label %blk_exit35

blk_exit37:                                       ; preds = %blk_exit284, %blk_exit38
  %_local_4.1 = phi i32 [ %_local_4.0, %blk_exit38 ], [ %51, %blk_exit284 ]
  %calcOffset313 = add i32 %0, -2848
  %46 = inttoptr i32 %calcOffset313 to i32*
  store i32 %_local_4.1, i32* %46, align 4
  %47 = load i32, i32* null, align 2147483648
  %48 = call i32 @fprintf(i32 %47, i32 1742, i32 %calcOffset313)
  call void @exit(i32 1)
  unreachable

blk_exit38:                                       ; preds = %brif_next264
  %49 = call i32 @regcomp(i32 2232, i32 %_local_4.0, i32 8)
  %50 = icmp eq i32 %49, 0
  br i1 %50, label %blk_exit35, label %blk_exit37

blk_exit39:                                       ; preds = %brif_next277, %brif_next266
  %_param_1.0 = phi i32 [ 1, %brif_next266 ], [ %303, %brif_next277 ]
  %51 = call i32 @malloc(i32 %_param_1.0)
  %52 = icmp eq i32 %51, 0
  br i1 %52, label %blk_exit34, label %brif_next283

blk_exit41:                                       ; preds = %blk_exit218, %blk_exit203, %blk_exit111, %blk_exit49
  %53 = load i8, i8* inttoptr (i32 2296 to i8*), align 8
  %brif_val271.not = icmp eq i8 %53, 0
  br i1 %brif_val271.not, label %brif_next272, label %blk_exit270

blk_exit42:                                       ; preds = %blk_exit218, %blk_exit110, %brif_next45
  %54 = icmp eq i32 %_local_4.0, 0
  br i1 %54, label %blk_exit35, label %brif_next264

blk_exit43:                                       ; preds = %blk_exit5
  %55 = load i8, i8* inttoptr (i32 2292 to i8*), align 4
  %56 = icmp eq i8 %55, 0
  br i1 %56, label %blk_exit219, label %brif_next221

brif_next45:                                      ; preds = %blk_exit5
  %57 = icmp slt i32 %7, 1
  br i1 %57, label %blk_exit42, label %brif_next47

brif_next47:                                      ; preds = %brif_next45
  %58 = shl i32 %6, 2
  %59 = add i32 %58, %_arg_1
  br label %loop_entry51

blk_exit48:                                       ; preds = %brif_next181
  %calcOffset217 = add i32 %0, -2768
  %60 = inttoptr i32 %calcOffset217 to i32*
  store i32 1987, i32* %60, align 4
  %61 = load i32, i32* null, align 2147483648
  %62 = call i32 @fprintf(i32 %61, i32 1191, i32 %calcOffset217)
  call void @exit(i32 1)
  unreachable

blk_exit49:                                       ; preds = %brif_next78, %blk_exit69, %brif_next66, %blk_exit50
  store i8 1, i8* inttoptr (i32 2296 to i8*), align 8
  br label %blk_exit41

blk_exit50:                                       ; preds = %loop_entry58
  %63 = load i32, i32* null, align 2147483648
  %64 = call i32 @fprintf(i32 %63, i32 1914, i32 0)
  %65 = load i32, i32* %70, align 4
  %calcOffset216 = add i32 %0, -2816
  %66 = inttoptr i32 %calcOffset216 to i32*
  store i32 %65, i32* %66, align 4
  %67 = call i32 @fprintf(i32 %63, i32 2159, i32 %calcOffset216)
  br label %blk_exit49

loop_entry51:                                     ; preds = %blk_exit110, %brif_next47
  %_local_8.0 = phi i32 [ 0, %brif_next47 ], [ %146, %blk_exit110 ]
  %68 = shl i32 %_local_8.0, 2
  %69 = add i32 %59, %68
  %70 = inttoptr i32 %69 to i32*
  %71 = load i32, i32* %70, align 4
  %72 = inttoptr i32 %71 to i8*
  %73 = load i8, i8* %72, align 1
  %74 = sext i8 %73 to i32
  %75 = add nsw i32 %74, -48
  %76 = icmp ult i32 %75, 10
  br i1 %76, label %blk_exit53, label %blk_exit52

blk_exit52:                                       ; preds = %blk_exit76, %blk_exit68, %blk_exit64, %loop_entry51
  %_local_11.0 = phi i32 [ -1, %blk_exit64 ], [ -1, %loop_entry51 ], [ %83, %blk_exit76 ], [ %83, %blk_exit68 ]
  %_local_10.0 = phi i32 [ %71, %blk_exit64 ], [ %71, %loop_entry51 ], [ %105, %blk_exit76 ], [ %87, %blk_exit68 ]
  %_local_8.1 = phi i32 [ %_local_8.0, %blk_exit64 ], [ %_local_8.0, %loop_entry51 ], [ %100, %blk_exit76 ], [ %_local_8.0, %blk_exit68 ]
  %77 = call i32 @strlen(i32 %_local_10.0)
  %78 = icmp ult i32 %77, 3
  br i1 %78, label %blk_exit81, label %brif_next83

blk_exit53:                                       ; preds = %loop_entry51
  %79 = add i32 %71, 1
  br label %loop_entry58

loop_entry58:                                     ; preds = %brif_next60, %blk_exit53
  %_local_3.1 = phi i32 [ %74, %blk_exit53 ], [ %86, %brif_next60 ]
  %_param_1.1 = phi i32 [ 0, %blk_exit53 ], [ %83, %brif_next60 ]
  %_param_0.1 = phi i32 [ %79, %blk_exit53 ], [ %87, %brif_next60 ]
  %80 = mul i32 %_param_1.1, 10
  %81 = add i32 %80, %_local_3.1
  %82 = icmp sgt i32 %81, 148
  br i1 %82, label %blk_exit50, label %brif_next60

brif_next60:                                      ; preds = %loop_entry58
  %83 = add i32 %81, -48
  %84 = inttoptr i32 %_param_0.1 to i8*
  %85 = load i8, i8* %84, align 1
  %86 = sext i8 %85 to i32
  %87 = add i32 %_param_0.1, 1
  %88 = add nsw i32 %86, -48
  %89 = icmp ult i32 %88, 10
  br i1 %89, label %loop_entry58, label %brif_next63

brif_next63:                                      ; preds = %brif_next60
  %90 = icmp sgt i32 %81, 47
  br i1 %90, label %blk_exit64, label %brif_next66

blk_exit64:                                       ; preds = %brif_next63
  switch i8 %85, label %blk_exit52 [
    i8 37, label %blk_exit68
    i8 46, label %blk_exit69
  ]

brif_next66:                                      ; preds = %brif_next63
  %calcOffset67 = add i32 %0, -2800
  %91 = inttoptr i32 %calcOffset67 to i32*
  store i32 %71, i32* %91, align 4
  %92 = load i32, i32* null, align 2147483648
  %93 = call i32 @fprintf(i32 %92, i32 2159, i32 %calcOffset67)
  br label %blk_exit49

blk_exit68:                                       ; preds = %blk_exit64
  %94 = inttoptr i32 %87 to i8*
  %95 = load i8, i8* %94, align 1
  %96 = icmp eq i8 %95, 0
  br i1 %96, label %blk_exit72, label %blk_exit52

blk_exit69:                                       ; preds = %blk_exit64
  %calcOffset70 = add i32 %0, -2624
  %97 = inttoptr i32 %calcOffset70 to i32*
  store i32 1625, i32* %97, align 4
  %98 = load i32, i32* null, align 2147483648
  %99 = call i32 @fprintf(i32 %98, i32 1191, i32 %calcOffset70)
  br label %blk_exit49

blk_exit72:                                       ; preds = %blk_exit68
  %100 = add i32 %_local_8.0, 1
  %101 = icmp slt i32 %100, %7
  br i1 %101, label %blk_exit76, label %brif_next78

blk_exit76:                                       ; preds = %blk_exit72
  %102 = shl i32 %100, 2
  %103 = add i32 %59, %102
  %104 = inttoptr i32 %103 to i32*
  %105 = load i32, i32* %104, align 4
  br label %blk_exit52

brif_next78:                                      ; preds = %blk_exit72
  %calcOffset79 = add i32 %0, -2608
  %106 = inttoptr i32 %calcOffset79 to i32*
  store i32 1710, i32* %106, align 4
  %107 = load i32, i32* null, align 2147483648
  %108 = call i32 @fprintf(i32 %107, i32 1191, i32 %calcOffset79)
  br label %blk_exit49

blk_exit81:                                       ; preds = %brif_next89, %brif_next86, %brif_next83, %blk_exit52
  %109 = phi i1 [ false, %brif_next89 ], [ true, %brif_next86 ], [ true, %brif_next83 ], [ true, %blk_exit52 ]
  %_local_3.2 = phi i32 [ 1282, %brif_next89 ], [ 1121, %brif_next86 ], [ 1121, %brif_next83 ], [ 1121, %blk_exit52 ]
  %_param_1.2 = phi i32 [ 1246, %brif_next89 ], [ 1089, %brif_next86 ], [ 1089, %brif_next83 ], [ 1089, %blk_exit52 ]
  %110 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val93.not = icmp eq i32 %110, 0
  br i1 %brif_val93.not, label %brif_next94, label %blk_exit92

brif_next83:                                      ; preds = %blk_exit52
  %111 = add i32 %77, %_local_10.0
  %112 = add i32 %111, -2
  %113 = inttoptr i32 %112 to i8*
  %114 = load i8, i8* %113, align 1
  %.not = icmp eq i8 %114, 45
  br i1 %.not, label %brif_next86, label %blk_exit81

brif_next86:                                      ; preds = %brif_next83
  %115 = add i32 %111, -1
  %116 = inttoptr i32 %115 to i8*
  %117 = load i8, i8* %116, align 1
  %.not770 = icmp eq i8 %117, 111
  br i1 %.not770, label %brif_next89, label %blk_exit81

brif_next89:                                      ; preds = %brif_next86
  store i8 0, i8* %113, align 1
  br label %blk_exit81

blk_exit91:                                       ; preds = %brif_next103, %blk_exit97, %brif_next94
  %118 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %119 = icmp eq i32 %118, 0
  br i1 %119, label %blk_exit111, label %brif_next113

blk_exit92:                                       ; preds = %blk_exit81
  %120 = call i32 @strncmp(i32 %_local_10.0, i32 1559, i32 1)
  %121 = icmp eq i32 %120, 0
  br i1 %121, label %blk_exit97, label %brif_next99

brif_next94:                                      ; preds = %blk_exit81
  %calcOffset95 = add i32 %0, -2704
  %122 = inttoptr i32 %calcOffset95 to i32*
  store i32 %_param_1.2, i32* %122, align 4
  %123 = add i32 %0, -1536
  %124 = call i32 @snprintf(i32 %123, i32 512, i32 1191, i32 %calcOffset95)
  %calcOffset96 = add i32 %0, -2720
  %125 = inttoptr i32 %calcOffset96 to i32*
  store i32 %_local_3.2, i32* %125, align 4
  %126 = add i32 %0, -2048
  %127 = call i32 @snprintf(i32 %126, i32 512, i32 1191, i32 %calcOffset96)
  br label %blk_exit91

blk_exit97:                                       ; preds = %brif_next101, %brif_next99, %blk_exit92
  %calcOffset108 = add i32 %0, -2672
  %128 = inttoptr i32 %calcOffset108 to i32*
  store i32 %_local_10.0, i32* %128, align 4
  %129 = add i32 %0, -1536
  %130 = call i32 @snprintf(i32 %129, i32 512, i32 1191, i32 %calcOffset108)
  %calcOffset109 = add i32 %0, -2688
  %131 = inttoptr i32 %calcOffset109 to i32*
  store i32 %_local_10.0, i32* %131, align 4
  %132 = add i32 %0, -2048
  %133 = call i32 @snprintf(i32 %132, i32 512, i32 1191, i32 %calcOffset109)
  br label %blk_exit91

brif_next99:                                      ; preds = %blk_exit92
  %134 = call i32 @strncmp(i32 %_local_10.0, i32 1558, i32 2)
  %135 = icmp eq i32 %134, 0
  br i1 %135, label %blk_exit97, label %brif_next101

brif_next101:                                     ; preds = %brif_next99
  %136 = call i32 @strncmp(i32 %_local_10.0, i32 1557, i32 3)
  %137 = icmp eq i32 %136, 0
  br i1 %137, label %blk_exit97, label %brif_next103

brif_next103:                                     ; preds = %brif_next101
  %calcOffset104 = add i32 %0, -2636
  %138 = inttoptr i32 %calcOffset104 to i32*
  store i32 %_local_10.0, i32* %138, align 4
  %calcOffset105 = add i32 %0, -2640
  %139 = inttoptr i32 %calcOffset105 to i32*
  store i32 %_param_1.2, i32* %139, align 4
  %140 = add i32 %0, -1536
  %141 = call i32 @snprintf(i32 %140, i32 512, i32 1174, i32 %calcOffset105)
  %calcOffset106 = add i32 %0, -2652
  %142 = inttoptr i32 %calcOffset106 to i32*
  store i32 %_local_10.0, i32* %142, align 4
  %calcOffset107 = add i32 %0, -2656
  %143 = inttoptr i32 %calcOffset107 to i32*
  store i32 %_local_3.2, i32* %143, align 4
  %144 = add i32 %0, -2048
  %145 = call i32 @snprintf(i32 %144, i32 512, i32 1174, i32 %calcOffset107)
  br label %blk_exit91

blk_exit110:                                      ; preds = %blk_exit203, %brif_next183, %blk_exit157, %blk_exit111
  %146 = add i32 %_local_8.1, 1
  %147 = icmp slt i32 %146, %7
  br i1 %147, label %loop_entry51, label %blk_exit42

blk_exit111:                                      ; preds = %blk_exit91
  %148 = add i32 %0, -1536
  %149 = call i32 @add_file(i32 %_local_11.0, i32 %148, i32 0, i32 2268, i32 3328, i32 0)
  %150 = icmp eq i32 %149, 0
  br i1 %150, label %blk_exit41, label %blk_exit110

brif_next113:                                     ; preds = %blk_exit91
  %151 = add i32 %0, -2560
  %152 = call i32 @strncpy(i32 %151, i32 %118, i32 512)
  %calcOffset114 = add i32 %0, -2049
  %153 = inttoptr i32 %calcOffset114 to i8*
  store i8 0, i8* %153, align 1
  br label %loop_entry117

blk_exit115:                                      ; preds = %blk_exit145, %brif_next136, %blk_exit116
  %_local_12.0 = phi i32 [ %159, %blk_exit116 ], [ %183, %blk_exit145 ], [ 1, %brif_next136 ]
  %154 = add i32 %0, -1536
  %155 = add i32 %0, -2048
  %156 = call i32 @strncmp(i32 %154, i32 %155, i32 512)
  %157 = icmp eq i32 %156, 0
  br i1 %157, label %blk_exit160, label %brif_next162

blk_exit116:                                      ; preds = %blk_exit145, %brif_next121
  %.pre-phi810 = phi i32 [ %calcOffset152, %blk_exit145 ], [ %calcOffset124, %brif_next121 ]
  %.pre-phi = phi i32 [ %176, %blk_exit145 ], [ %calcOffset122, %brif_next121 ]
  %158 = add i32 %0, -1536
  %159 = call i32 @add_file(i32 100, i32 %158, i32 0, i32 %.pre-phi, i32 %.pre-phi810, i32 0)
  br label %blk_exit115

loop_entry117:                                    ; preds = %blk_exit134, %brif_next113
  %_local_3.3 = phi i32 [ %151, %brif_next113 ], [ %storemerge799, %blk_exit134 ]
  %160 = inttoptr i32 %_local_3.3 to i8*
  %161 = load i8, i8* %160, align 1
  %brif_val120.not = icmp eq i8 %161, 0
  br i1 %brif_val120.not, label %brif_next121, label %blk_exit118

blk_exit118:                                      ; preds = %loop_entry117
  %162 = call i32 @strchr(i32 %_local_3.3, i32 58)
  %brif_val128.not = icmp eq i32 %162, 0
  br i1 %brif_val128.not, label %blk_exit126, label %blk_exit127

brif_next121:                                     ; preds = %loop_entry117
  %calcOffset122 = add i32 %0, -2564
  %163 = inttoptr i32 %calcOffset122 to i32*
  store i32 0, i32* %163, align 4
  %calcOffset123 = add i32 %0, -2568
  %164 = inttoptr i32 %calcOffset123 to i32*
  store i32 0, i32* %164, align 4
  %calcOffset124 = add i32 %0, -2572
  %165 = inttoptr i32 %calcOffset124 to i32*
  store i32 0, i32* %165, align 4
  %calcOffset125 = add i32 %0, -2576
  %166 = inttoptr i32 %calcOffset125 to i32*
  store i32 0, i32* %166, align 4
  br label %blk_exit116

blk_exit126:                                      ; preds = %blk_exit127, %blk_exit118
  %storemerge799 = phi i32 [ %175, %blk_exit127 ], [ 0, %blk_exit118 ]
  %calcOffset131 = add i32 %0, -2728
  %167 = inttoptr i32 %calcOffset131 to i32*
  store i32 %_local_10.0, i32* %167, align 4
  %calcOffset132 = add i32 %0, -2732
  %168 = inttoptr i32 %calcOffset132 to i32*
  store i32 %_local_3.3, i32* %168, align 4
  %calcOffset133 = add i32 %0, -2736
  %169 = inttoptr i32 %calcOffset133 to i32*
  store i32 %_param_1.2, i32* %169, align 4
  %170 = add i32 %0, -1024
  %171 = call i32 @snprintf(i32 %170, i32 1024, i32 1171, i32 %calcOffset133)
  %172 = call i32 @add_file(i32 %_local_11.0, i32 %170, i32 0, i32 2268, i32 3328, i32 0)
  %173 = icmp eq i32 %172, 0
  br i1 %173, label %blk_exit134, label %brif_next136

blk_exit127:                                      ; preds = %blk_exit118
  %174 = inttoptr i32 %162 to i8*
  store i8 0, i8* %174, align 1
  %175 = add i32 %162, 1
  br label %blk_exit126

blk_exit134:                                      ; preds = %blk_exit126
  %176 = add i32 %0, -2564
  %177 = call i32 @strncpy(i32 %176, i32 %_local_3.3, i32 2)
  %calcOffset141 = add i32 %0, -2752
  %178 = inttoptr i32 %calcOffset141 to i32*
  store i32 %_param_1.2, i32* %178, align 4
  %calcOffset142 = add i32 %0, -2562
  %179 = inttoptr i32 %calcOffset142 to i8*
  store i8 0, i8* %179, align 1
  %calcOffset143 = add i32 %0, -2744
  %180 = inttoptr i32 %calcOffset143 to i32*
  store i32 %_local_10.0, i32* %180, align 4
  %calcOffset144 = add i32 %0, -2748
  %181 = inttoptr i32 %calcOffset144 to i32*
  store i32 %176, i32* %181, align 4
  %182 = call i32 @snprintf(i32 %170, i32 1024, i32 1171, i32 %calcOffset141)
  %183 = call i32 @add_file(i32 %_local_11.0, i32 %170, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val146.not = icmp eq i32 %183, 0
  %brif_val146.not.not = xor i1 %brif_val146.not, true
  %brif_val148.not = icmp eq i32 %storemerge799, 0
  %or.cond814 = select i1 %brif_val146.not.not, i1 true, i1 %brif_val148.not
  br i1 %or.cond814, label %blk_exit145, label %loop_entry117

brif_next136:                                     ; preds = %blk_exit126
  %calcOffset137 = add i32 %0, -2564
  %184 = inttoptr i32 %calcOffset137 to i32*
  store i32 0, i32* %184, align 4
  %calcOffset138 = add i32 %0, -2568
  %185 = inttoptr i32 %calcOffset138 to i32*
  store i32 0, i32* %185, align 4
  %calcOffset139 = add i32 %0, -2572
  %186 = inttoptr i32 %calcOffset139 to i32*
  store i32 0, i32* %186, align 4
  %calcOffset140 = add i32 %0, -2576
  %187 = inttoptr i32 %calcOffset140 to i32*
  store i32 0, i32* %187, align 4
  br label %blk_exit115

blk_exit145:                                      ; preds = %blk_exit134
  %188 = inttoptr i32 %176 to i32*
  store i32 0, i32* %188, align 4
  %calcOffset151 = add i32 %0, -2568
  %189 = inttoptr i32 %calcOffset151 to i32*
  store i32 0, i32* %189, align 4
  %calcOffset152 = add i32 %0, -2572
  %190 = inttoptr i32 %calcOffset152 to i32*
  store i32 0, i32* %190, align 4
  %calcOffset153 = add i32 %0, -2576
  %191 = inttoptr i32 %calcOffset153 to i32*
  store i32 0, i32* %191, align 4
  br i1 %brif_val146.not, label %blk_exit116, label %blk_exit115

blk_exit156:                                      ; preds = %brif_next178, %blk_exit160, %blk_exit157
  br i1 %109, label %blk_exit203, label %brif_next205

blk_exit157:                                      ; preds = %brif_next176, %brif_next171, %blk_exit159, %blk_exit158
  %brif_val201.not = icmp eq i32 %_local_12.0, 0
  br i1 %brif_val201.not, label %blk_exit156, label %blk_exit110

blk_exit158:                                      ; preds = %brif_next168
  store i32 %197, i32* inttoptr (i32 3328 to i32*), align 256
  %calcOffset200 = add i32 %0, -2564
  %192 = inttoptr i32 %calcOffset200 to i32*
  %193 = load i32, i32* %192, align 4
  store i32 %193, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit157

blk_exit159:                                      ; preds = %brif_next162
  %194 = call i32 @strncmp(i32 %154, i32 %155, i32 512)
  %195 = icmp eq i32 %194, 0
  br i1 %195, label %blk_exit157, label %brif_next176

blk_exit160:                                      ; preds = %brif_next162, %blk_exit115
  %calcOffset166 = add i32 %0, -2572
  %196 = inttoptr i32 %calcOffset166 to i32*
  %197 = load i32, i32* %196, align 4
  %198 = icmp eq i32 %197, 0
  br i1 %198, label %blk_exit156, label %brif_next168

brif_next162:                                     ; preds = %blk_exit115
  %199 = add i32 %0, -2568
  %200 = add i32 %0, -2576
  %201 = call i32 @add_file(i32 100, i32 %155, i32 0, i32 %199, i32 %200, i32 0)
  %202 = inttoptr i32 %199 to i32*
  %203 = load i32, i32* %202, align 4
  %brif_val164.not = icmp eq i32 %203, 0
  br i1 %brif_val164.not, label %blk_exit160, label %blk_exit159

brif_next168:                                     ; preds = %blk_exit160
  %204 = inttoptr i32 %197 to i32*
  store i32 %_local_11.0, i32* %204, align 4
  %205 = load i32, i32* inttoptr (i32 3328 to i32*), align 256
  %206 = icmp eq i32 %205, 0
  br i1 %206, label %blk_exit158, label %brif_next171

brif_next171:                                     ; preds = %brif_next168
  %calcOffset172 = add i32 %0, -2564
  %207 = inttoptr i32 %calcOffset172 to i32*
  %208 = load i32, i32* %207, align 4
  %calcOffset173 = add i32 %205, 76
  %209 = inttoptr i32 %calcOffset173 to i32*
  store i32 %208, i32* %209, align 4
  store i32 %197, i32* inttoptr (i32 3328 to i32*), align 256
  %210 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %calcOffset174 = add i32 %208, 80
  %211 = inttoptr i32 %calcOffset174 to i32*
  store i32 %210, i32* %211, align 4
  br label %blk_exit157

brif_next176:                                     ; preds = %blk_exit159
  %212 = call i32 @strcmp(i32 %_local_10.0, i32 1225)
  %brif_val177.not = icmp eq i32 %212, 0
  br i1 %brif_val177.not, label %brif_next178, label %blk_exit157

brif_next178:                                     ; preds = %brif_next176
  %calcOffset179 = add i32 %0, -2564
  %213 = inttoptr i32 %calcOffset179 to i32*
  %214 = load i32, i32* %213, align 4
  %215 = icmp eq i32 %214, 0
  br i1 %215, label %blk_exit156, label %brif_next181

brif_next181:                                     ; preds = %brif_next178
  %216 = call i32 @malloc(i32 84)
  %217 = icmp eq i32 %216, 0
  br i1 %217, label %blk_exit48, label %brif_next183

brif_next183:                                     ; preds = %brif_next181
  %calcOffset184 = add i32 %216, 12
  %218 = inttoptr i32 %calcOffset184 to i64*
  store i64 4294967295, i64* %218, align 8
  %calcOffset185 = add i32 %216, 4
  %219 = inttoptr i32 %calcOffset185 to i64*
  store i64 -1, i64* %219, align 8
  %calcOffset186 = add i32 %216, 56
  %220 = inttoptr i32 %calcOffset186 to i64*
  store i64 0, i64* %220, align 8
  %calcOffset187 = add i32 %216, 48
  %221 = inttoptr i32 %calcOffset187 to i64*
  store i64 0, i64* %221, align 8
  %calcOffset188 = add i32 %216, 40
  %222 = inttoptr i32 %calcOffset188 to i64*
  store i64 0, i64* %222, align 8
  %calcOffset189 = add i32 %216, 72
  %223 = inttoptr i32 %calcOffset189 to i64*
  store i64 0, i64* %223, align 8
  %calcOffset190 = add i32 %216, 20
  %224 = inttoptr i32 %calcOffset190 to i64*
  store i64 0, i64* %224, align 8
  %calcOffset191 = add i32 %216, 80
  %225 = inttoptr i32 %calcOffset191 to i32*
  store i32 0, i32* %225, align 4
  %calcOffset192 = add i32 %214, 72
  %226 = inttoptr i32 %calcOffset192 to i32*
  store i32 %216, i32* %226, align 4
  %calcOffset193 = add i32 %216, 36
  %227 = inttoptr i32 %calcOffset193 to i8*
  store i8 0, i8* %227, align 1
  %calcOffset194 = add i32 %216, 28
  %228 = inttoptr i32 %calcOffset194 to i64*
  store i64 0, i64* %228, align 8
  %229 = inttoptr i32 %216 to i32*
  store i32 %_local_11.0, i32* %229, align 4
  %calcOffset196 = add i32 %216, 68
  %230 = inttoptr i32 %calcOffset196 to i32*
  store i32 %214, i32* %230, align 4
  %calcOffset197 = add i32 %214, 76
  %231 = inttoptr i32 %calcOffset197 to i32*
  store i32 %203, i32* %231, align 4
  %calcOffset198 = add i32 %203, 80
  %232 = inttoptr i32 %calcOffset198 to i32*
  store i32 %214, i32* %232, align 4
  %calcOffset199 = add i32 %203, 72
  %233 = inttoptr i32 %calcOffset199 to i32*
  store i32 %216, i32* %233, align 4
  store i32 %216, i32* inttoptr (i32 2268 to i32*), align 4
  store i32 %216, i32* inttoptr (i32 3328 to i32*), align 256
  br label %blk_exit110

blk_exit203:                                      ; preds = %brif_next205, %blk_exit156
  %234 = call i32 @getenv(i32 1518)
  %calcOffset207 = add i32 %0, -2780
  %235 = inttoptr i32 %calcOffset207 to i32*
  store i32 %_local_10.0, i32* %235, align 4
  %calcOffset208 = add i32 %0, -2784
  %236 = inttoptr i32 %calcOffset208 to i32*
  store i32 %234, i32* %236, align 4
  %237 = call i32 @snprintf(i32 %155, i32 512, i32 1174, i32 %calcOffset208)
  %238 = call i32 @add_file(i32 %_local_11.0, i32 %155, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val209.not = icmp eq i32 %238, 0
  br i1 %brif_val209.not, label %blk_exit41, label %blk_exit110

brif_next205:                                     ; preds = %blk_exit156
  %239 = add i32 %77, %_local_10.0
  %240 = add i32 %239, -2
  %241 = inttoptr i32 %240 to i8*
  store i8 45, i8* %241, align 1
  br label %blk_exit203

blk_exit218:                                      ; preds = %blk_exit258, %blk_exit255, %brif_next251, %blk_exit245, %blk_exit231, %blk_exit225
  %_param_0.2 = phi i32 [ %287, %blk_exit258 ], [ %285, %blk_exit255 ], [ %283, %brif_next251 ], [ %277, %blk_exit245 ], [ %261, %blk_exit231 ], [ %254, %blk_exit225 ]
  %242 = icmp eq i32 %_param_0.2, 0
  br i1 %242, label %blk_exit41, label %blk_exit42

blk_exit219:                                      ; preds = %blk_exit43
  %243 = load i8, i8* inttoptr (i32 2293 to i8*), align 1
  %244 = icmp eq i8 %243, 0
  br i1 %244, label %blk_exit228, label %brif_next230

brif_next221:                                     ; preds = %blk_exit43
  %245 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %246 = call i32 @add_file(i32 -1, i32 1282, i32 0, i32 2268, i32 3328, i32 0)
  %247 = or i32 %245, %246
  %248 = call i32 @strcmp(i32 1089, i32 1121)
  %249 = icmp eq i32 %248, 0
  br i1 %249, label %blk_exit222, label %brif_next224

blk_exit222:                                      ; preds = %brif_next224, %brif_next221
  %_param_0.3 = phi i32 [ 0, %brif_next221 ], [ %253, %brif_next224 ]
  %250 = or i32 %247, %_param_0.3
  %251 = call i32 @strcmp(i32 1246, i32 1282)
  %252 = icmp eq i32 %251, 0
  br i1 %252, label %blk_exit225, label %brif_next227

brif_next224:                                     ; preds = %brif_next221
  %253 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit222

blk_exit225:                                      ; preds = %brif_next227, %blk_exit222
  %_local_3.4 = phi i32 [ 0, %blk_exit222 ], [ %255, %brif_next227 ]
  %254 = or i32 %250, %_local_3.4
  br label %blk_exit218

brif_next227:                                     ; preds = %blk_exit222
  %255 = call i32 @add_file(i32 -1, i32 1246, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit225

blk_exit228:                                      ; preds = %blk_exit219
  %256 = load i32, i32* inttoptr (i32 2224 to i32*), align 16
  %257 = icmp eq i32 %256, 0
  br i1 %257, label %blk_exit234, label %brif_next236

brif_next230:                                     ; preds = %blk_exit219
  %258 = call i32 @add_file(i32 -1, i32 1282, i32 0, i32 2268, i32 3328, i32 0)
  %259 = call i32 @strcmp(i32 1246, i32 1282)
  %260 = icmp eq i32 %259, 0
  br i1 %260, label %blk_exit231, label %brif_next233

blk_exit231:                                      ; preds = %brif_next233, %brif_next230
  %_local_3.5 = phi i32 [ 0, %brif_next230 ], [ %262, %brif_next233 ]
  %261 = or i32 %_local_3.5, %258
  br label %blk_exit218

brif_next233:                                     ; preds = %brif_next230
  %262 = call i32 @add_file(i32 -1, i32 1246, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit231

blk_exit234:                                      ; preds = %blk_exit228
  %263 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %264 = call i32 @strcmp(i32 1089, i32 1121)
  %265 = icmp eq i32 %264, 0
  br i1 %265, label %blk_exit258, label %brif_next260

brif_next236:                                     ; preds = %blk_exit228
  %266 = add i32 %0, -2560
  %267 = call i32 @strncpy(i32 %266, i32 %256, i32 512)
  %calcOffset237 = add i32 %0, -2049
  %268 = inttoptr i32 %calcOffset237 to i8*
  store i8 0, i8* %268, align 1
  br label %loop_entry239

blk_exit238:                                      ; preds = %brif_next241, %loop_entry239
  %269 = call i32 @add_file(i32 -1, i32 1121, i32 0, i32 2268, i32 3328, i32 0)
  %270 = call i32 @strcmp(i32 1089, i32 1121)
  %271 = icmp eq i32 %270, 0
  br i1 %271, label %blk_exit255, label %brif_next257

loop_entry239:                                    ; preds = %brif_next251, %brif_next236
  %_local_3.6 = phi i32 [ %266, %brif_next236 ], [ %storemerge801, %brif_next251 ]
  %272 = icmp eq i32 %_local_3.6, 0
  br i1 %272, label %blk_exit238, label %brif_next241

brif_next241:                                     ; preds = %loop_entry239
  %273 = inttoptr i32 %_local_3.6 to i8*
  %274 = load i8, i8* %273, align 1
  %275 = icmp eq i8 %274, 0
  br i1 %275, label %blk_exit238, label %brif_next244

brif_next244:                                     ; preds = %brif_next241
  %276 = call i32 @strchr(i32 %_local_3.6, i32 58)
  %brif_val247.not = icmp eq i32 %276, 0
  br i1 %brif_val247.not, label %blk_exit245, label %blk_exit246

blk_exit245:                                      ; preds = %blk_exit246, %brif_next244
  %storemerge801 = phi i32 [ %279, %blk_exit246 ], [ 0, %brif_next244 ]
  %277 = call i32 @add_file(i32 -1, i32 %_local_3.6, i32 0, i32 2268, i32 3328, i32 0)
  %brif_val250.not = icmp eq i32 %277, 0
  br i1 %brif_val250.not, label %brif_next251, label %blk_exit218

blk_exit246:                                      ; preds = %brif_next244
  %278 = inttoptr i32 %276 to i8*
  store i8 0, i8* %278, align 1
  %279 = add i32 %276, 1
  br label %blk_exit245

brif_next251:                                     ; preds = %blk_exit245
  %280 = add i32 %0, -2564
  %281 = call i32 @strncpy(i32 %280, i32 %_local_3.6, i32 2)
  %calcOffset252 = add i32 %0, -2562
  %282 = inttoptr i32 %calcOffset252 to i8*
  store i8 0, i8* %282, align 1
  %283 = call i32 @add_file(i32 -1, i32 %280, i32 0, i32 2268, i32 3328, i32 0)
  %284 = icmp eq i32 %283, 0
  br i1 %284, label %loop_entry239, label %blk_exit218

blk_exit255:                                      ; preds = %brif_next257, %blk_exit238
  %_local_3.7 = phi i32 [ 0, %blk_exit238 ], [ %286, %brif_next257 ]
  %285 = or i32 %_local_3.7, %269
  br label %blk_exit218

brif_next257:                                     ; preds = %blk_exit238
  %286 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit255

blk_exit258:                                      ; preds = %brif_next260, %blk_exit234
  %_local_3.8 = phi i32 [ 0, %blk_exit234 ], [ %288, %brif_next260 ]
  %287 = or i32 %_local_3.8, %263
  br label %blk_exit218

brif_next260:                                     ; preds = %blk_exit234
  %288 = call i32 @add_file(i32 -1, i32 1089, i32 0, i32 2268, i32 3328, i32 0)
  br label %blk_exit258

brif_next264:                                     ; preds = %blk_exit42
  %brif_val265.not.not = icmp eq i32 %_local_3.0, 0
  br i1 %brif_val265.not.not, label %blk_exit38, label %brif_next266

brif_next266:                                     ; preds = %brif_next264
  %289 = inttoptr i32 %_local_4.0 to i8*
  %290 = load i8, i8* %289, align 1
  %291 = icmp eq i8 %290, 0
  br i1 %291, label %blk_exit39, label %loop_entry275

blk_exit270:                                      ; preds = %brif_next272, %blk_exit41
  call void @exit(i32 1)
  unreachable

brif_next272:                                     ; preds = %blk_exit41
  %292 = inttoptr i32 %1 to i32*
  store i32 1795, i32* %292, align 4
  %293 = load i32, i32* null, align 2147483648
  %294 = call i32 @fprintf(i32 %293, i32 1191, i32 %1)
  br label %blk_exit270

blk_exit274:                                      ; preds = %loop_entry275
  %calcOffset281 = add i32 %0, -2832
  %295 = inttoptr i32 %calcOffset281 to i32*
  store i32 2075, i32* %295, align 4
  %296 = load i32, i32* null, align 2147483648
  %297 = call i32 @fprintf(i32 %296, i32 1191, i32 %calcOffset281)
  call void @exit(i32 1)
  unreachable

loop_entry275:                                    ; preds = %brif_next277, %brif_next266
  %_local_5.1 = phi i32 [ %303, %brif_next277 ], [ 1, %brif_next266 ]
  %_local_3.9.in = phi i8 [ %305, %brif_next277 ], [ %290, %brif_next266 ]
  %_param_0.4.in = phi i32 [ %_param_0.4, %brif_next277 ], [ %_local_4.0, %brif_next266 ]
  %_param_0.4 = add i32 %_param_0.4.in, 1
  %298 = or i8 %_local_3.9.in, 32
  %299 = sext i8 %298 to i32
  %300 = add nsw i32 %299, -97
  %301 = icmp ult i32 %300, 26
  %302 = select i1 %301, i32 4, i32 1
  %303 = add i32 %302, %_local_5.1
  %.not771 = icmp ult i32 %_local_5.1, %303
  br i1 %.not771, label %brif_next277, label %blk_exit274

brif_next277:                                     ; preds = %loop_entry275
  %304 = inttoptr i32 %_param_0.4 to i8*
  %305 = load i8, i8* %304, align 1
  %306 = icmp eq i8 %305, 0
  br i1 %306, label %blk_exit39, label %loop_entry275

brif_next283:                                     ; preds = %blk_exit39
  %307 = load i8, i8* %289, align 1
  %308 = icmp eq i8 %307, 0
  br i1 %308, label %blk_exit284, label %loop_entry288

blk_exit284:                                      ; preds = %blk_exit289, %brif_next283
  %_local_3.10 = phi i32 [ %51, %brif_next283 ], [ %_local_3.12, %blk_exit289 ]
  %309 = inttoptr i32 %_local_3.10 to i8*
  store i8 0, i8* %309, align 1
  %310 = call i32 @regcomp(i32 2232, i32 %51, i32 8)
  %311 = icmp eq i32 %310, 0
  br i1 %311, label %blk_exit36, label %blk_exit37

loop_entry288:                                    ; preds = %blk_exit289, %brif_next283
  %_local_5.2.in = phi i8 [ %316, %blk_exit289 ], [ %307, %brif_next283 ]
  %_local_3.11 = phi i32 [ %_local_3.12, %blk_exit289 ], [ %51, %brif_next283 ]
  %_param_1.3.in = phi i32 [ %_param_1.3, %blk_exit289 ], [ %_local_4.0, %brif_next283 ]
  %_param_1.3 = add i32 %_param_1.3.in, 1
  %312 = sext i8 %_local_5.2.in to i32
  %313 = add nsw i32 %312, -123
  %314 = icmp ult i32 %313, -26
  br i1 %314, label %blk_exit290, label %brif_next292

blk_exit289:                                      ; preds = %brif_next299, %blk_exit297, %brif_next292
  %_local_3.12 = phi i32 [ %327, %blk_exit297 ], [ %334, %brif_next299 ], [ %325, %brif_next292 ]
  %315 = inttoptr i32 %_param_1.3 to i8*
  %316 = load i8, i8* %315, align 1
  %brif_val306.not = icmp eq i8 %316, 0
  br i1 %brif_val306.not, label %blk_exit284, label %loop_entry288

blk_exit290:                                      ; preds = %loop_entry288
  %317 = add nsw i32 %312, -91
  %318 = icmp ult i32 %317, -26
  br i1 %318, label %blk_exit297, label %brif_next299

brif_next292:                                     ; preds = %loop_entry288
  %calcOffset293 = add i32 %_local_3.11, 1
  %319 = inttoptr i32 %calcOffset293 to i8*
  store i8 %_local_5.2.in, i8* %319, align 1
  %320 = inttoptr i32 %_local_3.11 to i8*
  store i8 91, i8* %320, align 1
  %321 = call i32 @toupper(i32 %312)
  %calcOffset295 = add i32 %_local_3.11, 3
  %322 = inttoptr i32 %calcOffset295 to i8*
  store i8 93, i8* %322, align 1
  %calcOffset296 = add i32 %_local_3.11, 2
  %323 = inttoptr i32 %calcOffset296 to i8*
  %324 = trunc i32 %321 to i8
  store i8 %324, i8* %323, align 1
  %325 = add i32 %_local_3.11, 4
  br label %blk_exit289

blk_exit297:                                      ; preds = %blk_exit290
  %326 = inttoptr i32 %_local_3.11 to i8*
  store i8 %_local_5.2.in, i8* %326, align 1
  %327 = add i32 %_local_3.11, 1
  br label %blk_exit289

brif_next299:                                     ; preds = %blk_exit290
  %calcOffset300 = add i32 %_local_3.11, 1
  %328 = inttoptr i32 %calcOffset300 to i8*
  store i8 %_local_5.2.in, i8* %328, align 1
  %329 = inttoptr i32 %_local_3.11 to i8*
  store i8 91, i8* %329, align 1
  %330 = call i32 @tolower(i32 %312)
  %calcOffset302 = add i32 %_local_3.11, 3
  %331 = inttoptr i32 %calcOffset302 to i8*
  store i8 93, i8* %331, align 1
  %calcOffset303 = add i32 %_local_3.11, 2
  %332 = inttoptr i32 %calcOffset303 to i8*
  %333 = trunc i32 %330 to i8
  store i8 %333, i8* %332, align 1
  %334 = add i32 %_local_3.11, 4
  br label %blk_exit289

blk_exit314:                                      ; preds = %blk_exit35
  %335 = load i32, i32* inttoptr (i32 3328 to i32*), align 256
  %336 = icmp eq i32 %335, 0
  br i1 %336, label %blk_exit321, label %brif_next323

brif_next316:                                     ; preds = %blk_exit35
  %337 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %338 = call i32 @maxlen_in_list(i32 %337)
  store i32 %338, i32* inttoptr (i32 2284 to i32*), align 4
  %339 = add i32 %338, 10
  %340 = call i32 @malloc(i32 %339)
  %341 = icmp eq i32 %340, 0
  br i1 %341, label %blk_exit33, label %brif_next318

brif_next318:                                     ; preds = %brif_next316
  store i32 %340, i32* inttoptr (i32 2288 to i32*), align 16
  %calcOffset319 = add i32 %0, -1024
  %342 = inttoptr i32 %calcOffset319 to i8*
  store i8 0, i8* %342, align 1
  %343 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  call void @matches_in_list(i32 %343, i32 %calcOffset319)
  %344 = load i8, i8* %342, align 1
  %345 = zext i8 %344 to i32
  call void @regfree(i32 2232)
  br label %blk_exit19

blk_exit321:                                      ; preds = %brif_next331, %blk_exit314
  %_local_5.3 = phi i32 [ 0, %blk_exit314 ], [ %355, %brif_next331 ]
  %_local_4.2 = phi i32 [ 0, %blk_exit314 ], [ %356, %brif_next331 ]
  %_param_1.4 = phi i32 [ 0, %blk_exit314 ], [ %358, %brif_next331 ]
  %346 = icmp ne i32 %_param_1.4, 100
  %347 = icmp eq i32 %_local_4.2, 0
  %or.cond808 = select i1 %346, i1 %347, i1 false
  br i1 %or.cond808, label %blk_exit31, label %blk_exit334

brif_next323:                                     ; preds = %blk_exit314
  %348 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %349 = and i8 %348, 1
  br label %loop_entry324

loop_entry324:                                    ; preds = %loop_entry324, %brif_next323
  %_local_5.4 = phi i32 [ 0, %brif_next323 ], [ %355, %loop_entry324 ]
  %_local_4.3 = phi i32 [ 0, %brif_next323 ], [ %356, %loop_entry324 ]
  %_local_3.13 = phi i32 [ %335, %brif_next323 ], [ %360, %loop_entry324 ]
  %_param_1.5 = phi i32 [ 0, %brif_next323 ], [ %358, %loop_entry324 ]
  %select_cond325.not = icmp eq i8 %349, 0
  %350 = select i1 %select_cond325.not, i32 %_local_5.4, i32 %_local_3.13
  %351 = inttoptr i32 %_local_3.13 to i32*
  %352 = load i32, i32* %351, align 4
  %353 = icmp eq i32 %352, -1
  %354 = zext i1 %353 to i32
  %355 = select i1 %353, i32 %350, i32 %_local_5.4
  %356 = add i32 %_local_4.3, %354
  %357 = select i1 %353, i32 0, i32 %352
  %358 = add i32 %357, %_param_1.5
  %calcOffset329 = add i32 %_local_3.13, 80
  %359 = inttoptr i32 %calcOffset329 to i32*
  %360 = load i32, i32* %359, align 4
  %brif_val330.not = icmp eq i32 %360, 0
  br i1 %brif_val330.not, label %brif_next331, label %loop_entry324

brif_next331:                                     ; preds = %loop_entry324
  %361 = icmp sgt i32 %358, 100
  br i1 %361, label %blk_exit32, label %blk_exit321

blk_exit334:                                      ; preds = %blk_exit321
  %or.cond809 = select i1 %346, i1 true, i1 %347
  br i1 %or.cond809, label %blk_exit339, label %blk_exit30

blk_exit339:                                      ; preds = %blk_exit334
  store i32 %_param_1.4, i32* inttoptr (i32 3332 to i32*), align 4
  %362 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %363 = icmp eq i8 %362, 0
  %or.cond = select i1 %363, i1 true, i1 %347
  br i1 %or.cond, label %blk_exit344, label %brif_next348

blk_exit344:                                      ; preds = %blk_exit349, %blk_exit339
  %364 = load i8, i8* inttoptr (i32 2264 to i8*), align 8
  %365 = icmp eq i8 %364, 0
  br i1 %365, label %blk_exit364, label %brif_next366

brif_next348:                                     ; preds = %blk_exit339
  %366 = sub i32 100, %_param_1.4
  %367 = icmp ult i32 %_local_4.2, 2
  br i1 %367, label %blk_exit349, label %brif_next351

blk_exit349:                                      ; preds = %blk_exit355, %brif_next351, %brif_next348
  %_local_3.14 = phi i32 [ %366, %brif_next348 ], [ %366, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %368 = inttoptr i32 %_local_5.3 to i32*
  store i32 %_local_3.14, i32* %368, align 4
  br label %blk_exit344

brif_next351:                                     ; preds = %brif_next348
  %369 = udiv i32 %366, %_local_4.2
  %370 = icmp eq i32 %335, %_local_5.3
  br i1 %370, label %blk_exit349, label %loop_entry354

loop_entry354:                                    ; preds = %blk_exit355, %brif_next351
  %_local_12.1 = phi i32 [ %335, %brif_next351 ], [ %374, %blk_exit355 ]
  %_local_3.15 = phi i32 [ %366, %brif_next351 ], [ %_local_3.16, %blk_exit355 ]
  %371 = inttoptr i32 %_local_12.1 to i32*
  %372 = load i32, i32* %371, align 4
  %.not773 = icmp eq i32 %372, -1
  br i1 %.not773, label %brif_next358, label %blk_exit355

blk_exit355:                                      ; preds = %brif_next358, %loop_entry354
  %_local_3.16 = phi i32 [ %375, %brif_next358 ], [ %_local_3.15, %loop_entry354 ]
  %calcOffset360 = add i32 %_local_12.1, 80
  %373 = inttoptr i32 %calcOffset360 to i32*
  %374 = load i32, i32* %373, align 4
  %.not774 = icmp eq i32 %374, %_local_5.3
  br i1 %.not774, label %blk_exit349, label %loop_entry354

brif_next358:                                     ; preds = %loop_entry354
  store i32 %369, i32* %371, align 4
  %375 = sub i32 %_local_3.15, %369
  br label %blk_exit355

blk_exit364:                                      ; preds = %blk_exit344
  %376 = call i64 @time(i32 0)
  %377 = call i32 @getpid()
  %378 = trunc i64 %376 to i32
  %379 = add i32 %377, %378
  call void @srandom(i32 %379)
  br label %loop_entry404

brif_next366:                                     ; preds = %blk_exit344
  %380 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %381 = load i8, i8* inttoptr (i32 3336 to i8*), align 8
  %brif_val371.not = icmp eq i8 %381, 0
  br i1 %brif_val371.not, label %brif_next372, label %blk_exit370

blk_exit367:                                      ; preds = %blk_exit370, %blk_exit368
  %_local_3.17 = phi i32 [ %383, %blk_exit368 ], [ %380, %blk_exit370 ]
  store i32 0, i32* inttoptr (i32 3356 to i32*), align 4
  store i32 0, i32* inttoptr (i32 3352 to i32*), align 8
  %382 = icmp eq i32 %_local_3.17, 0
  br i1 %382, label %blk_exit20, label %loop_entry399

blk_exit368:                                      ; preds = %brif_next390, %blk_exit369
  %383 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  br label %blk_exit367

blk_exit369:                                      ; preds = %brif_next372
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  %384 = icmp eq i8 %362, 0
  br i1 %384, label %blk_exit21, label %blk_exit368

blk_exit370:                                      ; preds = %brif_next366
  %brif_val393.not = icmp eq i8 %362, 0
  br i1 %brif_val393.not, label %blk_exit21, label %blk_exit367

brif_next372:                                     ; preds = %brif_next366
  store i32 -1, i32* inttoptr (i32 3348 to i32*), align 4
  store i32 0, i32* inttoptr (i32 3344 to i32*), align 16
  store i32 0, i32* inttoptr (i32 3340 to i32*), align 4
  %385 = icmp eq i32 %380, 0
  br i1 %385, label %blk_exit369, label %loop_entry375

loop_entry375:                                    ; preds = %blk_exit376, %brif_next372
  %_local_3.18 = phi i32 [ %380, %brif_next372 ], [ %389, %blk_exit376 ]
  call void @get_tbl(i32 %_local_3.18)
  %386 = inttoptr i32 %_local_3.18 to i32*
  %387 = load i32, i32* %386, align 4
  %.not775 = icmp eq i32 %387, -1
  br i1 %.not775, label %brif_next379, label %blk_exit376

blk_exit376:                                      ; preds = %brif_next387, %blk_exit381, %loop_entry375
  %calcOffset388 = add i32 %_local_3.18, 76
  %388 = inttoptr i32 %calcOffset388 to i32*
  %389 = load i32, i32* %388, align 4
  %brif_val389.not = icmp eq i32 %389, 0
  br i1 %brif_val389.not, label %brif_next390, label %loop_entry375

brif_next379:                                     ; preds = %loop_entry375
  %390 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %391 = add i32 %_local_3.18, 44
  %392 = inttoptr i32 %391 to i32*
  %393 = load i32, i32* %392, align 4
  %394 = add i32 %390, %393
  store i32 %394, i32* inttoptr (i32 3340 to i32*), align 4
  %395 = load i32, i32* inttoptr (i32 3344 to i32*), align 16
  %396 = add i32 %_local_3.18, 48
  %397 = inttoptr i32 %396 to i32*
  %398 = load i32, i32* %397, align 4
  %.not776 = icmp ult i32 %395, %398
  br i1 %.not776, label %brif_next384, label %blk_exit381

blk_exit381:                                      ; preds = %brif_next384, %brif_next379
  %399 = load i32, i32* inttoptr (i32 3348 to i32*), align 4
  %400 = add i32 %_local_3.18, 52
  %401 = inttoptr i32 %400 to i32*
  %402 = load i32, i32* %401, align 4
  %.not777 = icmp ugt i32 %399, %402
  br i1 %.not777, label %brif_next387, label %blk_exit376

brif_next384:                                     ; preds = %brif_next379
  store i32 %398, i32* inttoptr (i32 3344 to i32*), align 16
  br label %blk_exit381

brif_next387:                                     ; preds = %blk_exit381
  store i32 %402, i32* inttoptr (i32 3348 to i32*), align 4
  br label %blk_exit376

brif_next390:                                     ; preds = %blk_exit376
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  %403 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %404 = icmp eq i8 %403, 0
  br i1 %404, label %blk_exit21, label %blk_exit368

loop_entry399:                                    ; preds = %loop_entry399, %blk_exit367
  %_local_5.5 = phi i32 [ %410, %loop_entry399 ], [ %_local_3.17, %blk_exit367 ]
  %_param_1.6 = phi i32 [ %408, %loop_entry399 ], [ 0, %blk_exit367 ]
  %_param_0.5 = phi i32 [ %405, %loop_entry399 ], [ 0, %blk_exit367 ]
  %405 = add i32 %_param_0.5, 1
  %calcOffset400 = add i32 %_local_5.5, 64
  %406 = inttoptr i32 %calcOffset400 to i32*
  %407 = load i32, i32* %406, align 4
  %408 = add i32 %407, %_param_1.6
  %calcOffset401 = add i32 %_local_5.5, 76
  %409 = inttoptr i32 %calcOffset401 to i32*
  %410 = load i32, i32* %409, align 4
  %brif_val402.not = icmp eq i32 %410, 0
  br i1 %brif_val402.not, label %brif_next403, label %loop_entry399

brif_next403:                                     ; preds = %loop_entry399
  store i32 %408, i32* inttoptr (i32 3352 to i32*), align 8
  store i32 %405, i32* inttoptr (i32 3356 to i32*), align 4
  br label %blk_exit20

loop_entry404:                                    ; preds = %blk_exit639, %blk_exit605, %blk_exit364
  %411 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %412 = inttoptr i32 %411 to i32*
  %413 = load i32, i32* %412, align 4
  %calcOffset409 = add i32 %411, 76
  %414 = inttoptr i32 %calcOffset409 to i32*
  %415 = load i32, i32* %414, align 4
  %416 = icmp eq i32 %415, 0
  br i1 %416, label %blk_exit408, label %brif_next411

blk_exit406:                                      ; preds = %loop_entry481, %blk_exit466, %brif_next442, %blk_exit408
  %_local_3.19 = phi i32 [ %_local_3.21, %brif_next442 ], [ %_local_3.20, %blk_exit466 ], [ %490, %loop_entry481 ], [ %_local_3.21, %blk_exit408 ]
  call void @get_tbl(i32 %_local_3.19)
  %417 = add i32 %_local_3.19, 44
  %418 = inttoptr i32 %417 to i32*
  %419 = load i32, i32* %418, align 4
  %420 = icmp eq i32 %419, 0
  br i1 %420, label %blk_exit29, label %brif_next488

blk_exit407:                                      ; preds = %brif_next442, %brif_next411
  %_local_3.20 = phi i32 [ %_local_3.21, %brif_next442 ], [ %411, %brif_next411 ]
  %421 = load i8, i8* inttoptr (i32 3336 to i8*), align 8
  %brif_val447.not = icmp eq i8 %421, 0
  br i1 %brif_val447.not, label %brif_next448, label %blk_exit446

blk_exit408:                                      ; preds = %brif_next438, %loop_entry434, %blk_exit427, %blk_exit415, %loop_entry404
  %_local_5.6 = phi i32 [ %413, %loop_entry404 ], [ -1, %loop_entry434 ], [ %444, %brif_next438 ], [ -1, %blk_exit415 ], [ %428, %blk_exit427 ]
  %_local_3.21 = phi i32 [ %411, %loop_entry404 ], [ %442, %loop_entry434 ], [ %442, %brif_next438 ], [ %426, %blk_exit415 ], [ %426, %blk_exit427 ]
  %.not779 = icmp eq i32 %_local_5.6, -1
  br i1 %.not779, label %brif_next442, label %blk_exit406

brif_next411:                                     ; preds = %loop_entry404
  %422 = icmp eq i32 %413, -1
  br i1 %422, label %blk_exit407, label %brif_next413

brif_next413:                                     ; preds = %brif_next411
  %calcOffset414 = add i32 %0, -1024
  %423 = inttoptr i32 %calcOffset414 to i64*
  store i64 0, i64* %423, align 8
  %424 = call i32 @getenv(i32 1447)
  %425 = icmp eq i32 %424, 0
  br i1 %425, label %blk_exit416, label %brif_next418

blk_exit415:                                      ; preds = %brif_next425, %blk_exit419, %brif_next418
  %_param_0.6 = phi i32 [ %433, %blk_exit419 ], [ %440, %brif_next425 ], [ %431, %brif_next418 ]
  %426 = load i32, i32* inttoptr (i32 2268 to i32*), align 4
  %427 = inttoptr i32 %426 to i32*
  %428 = load i32, i32* %427, align 4
  %.not778 = icmp eq i32 %428, -1
  br i1 %.not778, label %blk_exit408, label %blk_exit427

blk_exit416:                                      ; preds = %brif_next413
  %429 = call i32 @getenv(i32 1522)
  %brif_val420.not = icmp eq i32 %429, 0
  br i1 %brif_val420.not, label %brif_next421, label %blk_exit419

brif_next418:                                     ; preds = %brif_next413
  %430 = call i32 @atol(i32 %424)
  %431 = urem i32 %430, 100
  br label %blk_exit415

blk_exit419:                                      ; preds = %brif_next423, %brif_next421, %blk_exit416
  %432 = call i32 @random()
  %433 = urem i32 %432, 100
  br label %blk_exit415

brif_next421:                                     ; preds = %blk_exit416
  %434 = call i32 @fopen(i32 1208, i32 1414)
  %435 = icmp eq i32 %434, 0
  br i1 %435, label %blk_exit419, label %brif_next423

brif_next423:                                     ; preds = %brif_next421
  %436 = call i32 @fread(i32 %calcOffset414, i32 8, i32 1, i32 %434)
  %437 = call i32 @fclose(i32 %434)
  %.not798 = icmp eq i32 %436, 1
  br i1 %.not798, label %brif_next425, label %blk_exit419

brif_next425:                                     ; preds = %brif_next423
  %438 = load i64, i64* %423, align 8
  %439 = urem i64 %438, 100
  %440 = trunc i64 %439 to i32
  br label %blk_exit415

blk_exit427:                                      ; preds = %blk_exit415
  %.not796 = icmp slt i32 %_param_0.6, %428
  br i1 %.not796, label %blk_exit408, label %loop_entry434

loop_entry434:                                    ; preds = %brif_next438, %blk_exit427
  %_local_3.22 = phi i32 [ %426, %blk_exit427 ], [ %442, %brif_next438 ]
  %_param_1.7 = phi i32 [ %428, %blk_exit427 ], [ %444, %brif_next438 ]
  %_param_0.7 = phi i32 [ %_param_0.6, %blk_exit427 ], [ %446, %brif_next438 ]
  %calcOffset435 = add i32 %_local_3.22, 76
  %441 = inttoptr i32 %calcOffset435 to i32*
  %442 = load i32, i32* %441, align 4
  %443 = inttoptr i32 %442 to i32*
  %444 = load i32, i32* %443, align 4
  %445 = icmp eq i32 %444, -1
  br i1 %445, label %blk_exit408, label %brif_next438

brif_next438:                                     ; preds = %loop_entry434
  %446 = sub i32 %_param_0.7, %_param_1.7
  %.not797 = icmp slt i32 %446, %444
  br i1 %.not797, label %blk_exit408, label %loop_entry434

brif_next442:                                     ; preds = %blk_exit408
  %calcOffset443 = add i32 %_local_3.21, 76
  %447 = inttoptr i32 %calcOffset443 to i32*
  %448 = load i32, i32* %447, align 4
  %449 = icmp eq i32 %448, 0
  br i1 %449, label %blk_exit406, label %blk_exit407

blk_exit446:                                      ; preds = %brif_next464, %blk_exit407
  %450 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %calcOffset465 = add i32 %0, -1024
  %451 = inttoptr i32 %calcOffset465 to i64*
  store i64 0, i64* %451, align 8
  %452 = call i32 @getenv(i32 1447)
  %453 = icmp eq i32 %452, 0
  br i1 %453, label %blk_exit467, label %brif_next469

brif_next448:                                     ; preds = %blk_exit407
  store i32 -1, i32* inttoptr (i32 3348 to i32*), align 4
  store i32 0, i32* inttoptr (i32 3344 to i32*), align 16
  store i32 0, i32* inttoptr (i32 3340 to i32*), align 4
  br label %loop_entry449

loop_entry449:                                    ; preds = %blk_exit450, %brif_next448
  %_local_5.7 = phi i32 [ %_local_3.20, %brif_next448 ], [ %457, %blk_exit450 ]
  call void @get_tbl(i32 %_local_5.7)
  %454 = inttoptr i32 %_local_5.7 to i32*
  %455 = load i32, i32* %454, align 4
  %.not780 = icmp eq i32 %455, -1
  br i1 %.not780, label %brif_next453, label %blk_exit450

blk_exit450:                                      ; preds = %brif_next461, %blk_exit455, %loop_entry449
  %calcOffset462 = add i32 %_local_5.7, 76
  %456 = inttoptr i32 %calcOffset462 to i32*
  %457 = load i32, i32* %456, align 4
  %brif_val463.not = icmp eq i32 %457, 0
  br i1 %brif_val463.not, label %brif_next464, label %loop_entry449

brif_next453:                                     ; preds = %loop_entry449
  %458 = load i32, i32* inttoptr (i32 3340 to i32*), align 4
  %459 = add i32 %_local_5.7, 44
  %460 = inttoptr i32 %459 to i32*
  %461 = load i32, i32* %460, align 4
  %462 = add i32 %458, %461
  store i32 %462, i32* inttoptr (i32 3340 to i32*), align 4
  %463 = load i32, i32* inttoptr (i32 3344 to i32*), align 16
  %464 = add i32 %_local_5.7, 48
  %465 = inttoptr i32 %464 to i32*
  %466 = load i32, i32* %465, align 4
  %.not781 = icmp ult i32 %463, %466
  br i1 %.not781, label %brif_next458, label %blk_exit455

blk_exit455:                                      ; preds = %brif_next458, %brif_next453
  %467 = load i32, i32* inttoptr (i32 3348 to i32*), align 4
  %468 = add i32 %_local_5.7, 52
  %469 = inttoptr i32 %468 to i32*
  %470 = load i32, i32* %469, align 4
  %.not782 = icmp ugt i32 %467, %470
  br i1 %.not782, label %brif_next461, label %blk_exit450

brif_next458:                                     ; preds = %brif_next453
  store i32 %466, i32* inttoptr (i32 3344 to i32*), align 16
  br label %blk_exit455

brif_next461:                                     ; preds = %blk_exit455
  store i32 %470, i32* inttoptr (i32 3348 to i32*), align 4
  br label %blk_exit450

brif_next464:                                     ; preds = %blk_exit450
  store i8 1, i8* inttoptr (i32 3336 to i8*), align 8
  br label %blk_exit446

blk_exit466:                                      ; preds = %brif_next476, %blk_exit470, %brif_next469
  %_local_5.8 = phi i32 [ %479, %blk_exit470 ], [ %487, %brif_next476 ], [ %477, %brif_next469 ]
  %471 = add i32 %_local_3.20, 44
  %472 = inttoptr i32 %471 to i32*
  %473 = load i32, i32* %472, align 4
  %474 = icmp slt i32 %_local_5.8, %473
  br i1 %474, label %blk_exit406, label %loop_entry481

blk_exit467:                                      ; preds = %blk_exit446
  %475 = call i32 @getenv(i32 1522)
  %brif_val471.not = icmp eq i32 %475, 0
  br i1 %brif_val471.not, label %brif_next472, label %blk_exit470

brif_next469:                                     ; preds = %blk_exit446
  %476 = call i32 @atol(i32 %452)
  %477 = urem i32 %476, %450
  br label %blk_exit466

blk_exit470:                                      ; preds = %brif_next474, %brif_next472, %blk_exit467
  %478 = call i32 @random()
  %479 = urem i32 %478, %450
  br label %blk_exit466

brif_next472:                                     ; preds = %blk_exit467
  %480 = call i32 @fopen(i32 1208, i32 1414)
  %481 = icmp eq i32 %480, 0
  br i1 %481, label %blk_exit470, label %brif_next474

brif_next474:                                     ; preds = %brif_next472
  %482 = call i32 @fread(i32 %calcOffset465, i32 8, i32 1, i32 %480)
  %483 = call i32 @fclose(i32 %480)
  %.not795 = icmp eq i32 %482, 1
  br i1 %.not795, label %brif_next476, label %blk_exit470

brif_next476:                                     ; preds = %brif_next474
  %484 = load i64, i64* %451, align 8
  %485 = zext i32 %450 to i64
  %486 = urem i64 %484, %485
  %487 = trunc i64 %486 to i32
  br label %blk_exit466

loop_entry481:                                    ; preds = %loop_entry481, %blk_exit466
  %_local_5.9 = phi i32 [ %_local_5.8, %blk_exit466 ], [ %488, %loop_entry481 ]
  %_local_3.23 = phi i32 [ %_local_3.20, %blk_exit466 ], [ %490, %loop_entry481 ]
  %_param_0.8 = phi i32 [ %473, %blk_exit466 ], [ %493, %loop_entry481 ]
  %488 = sub i32 %_local_5.9, %_param_0.8
  %calcOffset482 = add i32 %_local_3.23, 76
  %489 = inttoptr i32 %calcOffset482 to i32*
  %490 = load i32, i32* %489, align 4
  %491 = add i32 %490, 44
  %492 = inttoptr i32 %491 to i32*
  %493 = load i32, i32* %492, align 4
  %.not783 = icmp slt i32 %488, %493
  br i1 %.not783, label %blk_exit406, label %loop_entry481

brif_next488:                                     ; preds = %blk_exit406
  %calcOffset490 = add i32 %_local_3.19, 68
  %494 = inttoptr i32 %calcOffset490 to i32*
  %495 = load i32, i32* %494, align 4
  %496 = icmp eq i32 %495, 0
  br i1 %496, label %blk_exit489, label %loop_entry493

blk_exit489:                                      ; preds = %blk_exit494, %brif_next488
  %_local_3.24 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  store i32 %_local_3.24, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset561 = add i32 %_local_3.24, 36
  %497 = inttoptr i32 %calcOffset561 to i8*
  %498 = load i8, i8* %497, align 1
  %499 = icmp eq i8 %498, 0
  br i1 %499, label %blk_exit28, label %brif_next563

loop_entry493:                                    ; preds = %blk_exit494, %brif_next488
  %_local_3.25 = phi i32 [ %_local_3.19, %brif_next488 ], [ %_local_3.26, %blk_exit494 ]
  %500 = load i8, i8* inttoptr (i32 2272 to i8*), align 32
  %501 = icmp eq i8 %500, 0
  br i1 %501, label %blk_exit495, label %brif_next497

blk_exit494:                                      ; preds = %loop_entry553, %blk_exit537, %loop_entry524, %blk_exit515, %blk_exit500
  %_local_3.26 = phi i32 [ %557, %blk_exit537 ], [ %577, %loop_entry553 ], [ %516, %blk_exit500 ], [ %_local_3.27, %blk_exit515 ], [ %554, %loop_entry524 ]
  %calcOffset558 = add i32 %_local_3.26, 68
  %502 = inttoptr i32 %calcOffset558 to i32*
  %503 = load i32, i32* %502, align 4
  %brif_val559.not = icmp eq i32 %503, 0
  br i1 %brif_val559.not, label %blk_exit489, label %loop_entry493

blk_exit495:                                      ; preds = %loop_entry493
  call void @get_tbl(i32 %_local_3.25)
  %504 = add i32 %_local_3.25, 44
  %505 = inttoptr i32 %504 to i32*
  %506 = load i32, i32* %505, align 4
  %calcOffset536 = add i32 %0, -1024
  %507 = inttoptr i32 %calcOffset536 to i64*
  store i64 0, i64* %507, align 8
  %508 = call i32 @getenv(i32 1447)
  %509 = icmp eq i32 %508, 0
  br i1 %509, label %blk_exit538, label %brif_next540

brif_next497:                                     ; preds = %loop_entry493
  %calcOffset498 = add i32 %_local_3.25, 64
  %510 = inttoptr i32 %calcOffset498 to i32*
  %511 = load i32, i32* %510, align 4
  %calcOffset499 = add i32 %0, -1024
  %512 = inttoptr i32 %calcOffset499 to i64*
  store i64 0, i64* %512, align 8
  %513 = call i32 @getenv(i32 1447)
  %514 = icmp eq i32 %513, 0
  br i1 %514, label %blk_exit501, label %brif_next503

blk_exit500:                                      ; preds = %brif_next510, %blk_exit504, %brif_next503
  %_local_5.10 = phi i32 [ %522, %blk_exit504 ], [ %530, %brif_next510 ], [ %520, %brif_next503 ]
  %calcOffset512 = add i32 %_local_3.25, 68
  %515 = inttoptr i32 %calcOffset512 to i32*
  %516 = load i32, i32* %515, align 4
  %517 = icmp eq i32 %_local_5.10, 0
  br i1 %517, label %blk_exit494, label %brif_next514

blk_exit501:                                      ; preds = %brif_next497
  %518 = call i32 @getenv(i32 1522)
  %brif_val505.not = icmp eq i32 %518, 0
  br i1 %brif_val505.not, label %brif_next506, label %blk_exit504

brif_next503:                                     ; preds = %brif_next497
  %519 = call i32 @atol(i32 %513)
  %520 = urem i32 %519, %511
  br label %blk_exit500

blk_exit504:                                      ; preds = %brif_next508, %brif_next506, %blk_exit501
  %521 = call i32 @random()
  %522 = urem i32 %521, %511
  br label %blk_exit500

brif_next506:                                     ; preds = %blk_exit501
  %523 = call i32 @fopen(i32 1208, i32 1414)
  %524 = icmp eq i32 %523, 0
  br i1 %524, label %blk_exit504, label %brif_next508

brif_next508:                                     ; preds = %brif_next506
  %525 = call i32 @fread(i32 %calcOffset499, i32 8, i32 1, i32 %523)
  %526 = call i32 @fclose(i32 %523)
  %.not792 = icmp eq i32 %525, 1
  br i1 %.not792, label %brif_next510, label %blk_exit504

brif_next510:                                     ; preds = %brif_next508
  %527 = load i64, i64* %512, align 8
  %528 = zext i32 %511 to i64
  %529 = urem i64 %527, %528
  %530 = trunc i64 %529 to i32
  br label %blk_exit500

brif_next514:                                     ; preds = %blk_exit500
  %531 = add i32 %_local_5.10, -1
  %532 = and i32 %_local_5.10, 7
  %533 = icmp eq i32 %532, 0
  br i1 %533, label %blk_exit515, label %loop_entry518

blk_exit515:                                      ; preds = %loop_entry518, %brif_next514
  %_local_5.11 = phi i32 [ %_local_5.10, %brif_next514 ], [ %535, %loop_entry518 ]
  %_local_3.27 = phi i32 [ %516, %brif_next514 ], [ %537, %loop_entry518 ]
  %534 = icmp ult i32 %531, 7
  br i1 %534, label %blk_exit494, label %loop_entry524

loop_entry518:                                    ; preds = %loop_entry518, %brif_next514
  %_local_5.12 = phi i32 [ %_local_5.10, %brif_next514 ], [ %535, %loop_entry518 ]
  %_local_3.28 = phi i32 [ %516, %brif_next514 ], [ %537, %loop_entry518 ]
  %_param_0.9 = phi i32 [ %532, %brif_next514 ], [ %538, %loop_entry518 ]
  %535 = add i32 %_local_5.12, -1
  %calcOffset519 = add i32 %_local_3.28, 76
  %536 = inttoptr i32 %calcOffset519 to i32*
  %537 = load i32, i32* %536, align 4
  %538 = add i32 %_param_0.9, -1
  %brif_val520.not = icmp eq i32 %538, 0
  br i1 %brif_val520.not, label %blk_exit515, label %loop_entry518

loop_entry524:                                    ; preds = %loop_entry524, %blk_exit515
  %_local_5.13 = phi i32 [ %_local_5.11, %blk_exit515 ], [ %555, %loop_entry524 ]
  %_local_3.29 = phi i32 [ %_local_3.27, %blk_exit515 ], [ %554, %loop_entry524 ]
  %calcOffset525 = add i32 %_local_3.29, 76
  %539 = inttoptr i32 %calcOffset525 to i32*
  %540 = load i32, i32* %539, align 4
  %calcOffset526 = add i32 %540, 76
  %541 = inttoptr i32 %calcOffset526 to i32*
  %542 = load i32, i32* %541, align 4
  %calcOffset527 = add i32 %542, 76
  %543 = inttoptr i32 %calcOffset527 to i32*
  %544 = load i32, i32* %543, align 4
  %calcOffset528 = add i32 %544, 76
  %545 = inttoptr i32 %calcOffset528 to i32*
  %546 = load i32, i32* %545, align 4
  %calcOffset529 = add i32 %546, 76
  %547 = inttoptr i32 %calcOffset529 to i32*
  %548 = load i32, i32* %547, align 4
  %calcOffset530 = add i32 %548, 76
  %549 = inttoptr i32 %calcOffset530 to i32*
  %550 = load i32, i32* %549, align 4
  %calcOffset531 = add i32 %550, 76
  %551 = inttoptr i32 %calcOffset531 to i32*
  %552 = load i32, i32* %551, align 4
  %calcOffset532 = add i32 %552, 76
  %553 = inttoptr i32 %calcOffset532 to i32*
  %554 = load i32, i32* %553, align 4
  %555 = add i32 %_local_5.13, -8
  %brif_val533.not = icmp eq i32 %555, 0
  br i1 %brif_val533.not, label %blk_exit494, label %loop_entry524

blk_exit537:                                      ; preds = %brif_next547, %blk_exit541, %brif_next540
  %_local_5.14 = phi i32 [ %566, %blk_exit541 ], [ %574, %brif_next547 ], [ %564, %brif_next540 ]
  %calcOffset549 = add i32 %_local_3.25, 68
  %556 = inttoptr i32 %calcOffset549 to i32*
  %557 = load i32, i32* %556, align 4
  %558 = add i32 %557, 44
  %559 = inttoptr i32 %558 to i32*
  %560 = load i32, i32* %559, align 4
  %561 = icmp slt i32 %_local_5.14, %560
  br i1 %561, label %blk_exit494, label %loop_entry553

blk_exit538:                                      ; preds = %blk_exit495
  %562 = call i32 @getenv(i32 1522)
  %brif_val542.not = icmp eq i32 %562, 0
  br i1 %brif_val542.not, label %brif_next543, label %blk_exit541

brif_next540:                                     ; preds = %blk_exit495
  %563 = call i32 @atol(i32 %508)
  %564 = urem i32 %563, %506
  br label %blk_exit537

blk_exit541:                                      ; preds = %brif_next545, %brif_next543, %blk_exit538
  %565 = call i32 @random()
  %566 = urem i32 %565, %506
  br label %blk_exit537

brif_next543:                                     ; preds = %blk_exit538
  %567 = call i32 @fopen(i32 1208, i32 1414)
  %568 = icmp eq i32 %567, 0
  br i1 %568, label %blk_exit541, label %brif_next545

brif_next545:                                     ; preds = %brif_next543
  %569 = call i32 @fread(i32 %calcOffset536, i32 8, i32 1, i32 %567)
  %570 = call i32 @fclose(i32 %567)
  %.not794 = icmp eq i32 %569, 1
  br i1 %.not794, label %brif_next547, label %blk_exit541

brif_next547:                                     ; preds = %brif_next545
  %571 = load i64, i64* %507, align 8
  %572 = zext i32 %506 to i64
  %573 = urem i64 %571, %572
  %574 = trunc i64 %573 to i32
  br label %blk_exit537

loop_entry553:                                    ; preds = %loop_entry553, %blk_exit537
  %_local_5.15 = phi i32 [ %_local_5.14, %blk_exit537 ], [ %575, %loop_entry553 ]
  %_local_3.30 = phi i32 [ %557, %blk_exit537 ], [ %577, %loop_entry553 ]
  %_param_0.10 = phi i32 [ %560, %blk_exit537 ], [ %580, %loop_entry553 ]
  %575 = sub i32 %_local_5.15, %_param_0.10
  %calcOffset554 = add i32 %_local_3.30, 76
  %576 = inttoptr i32 %calcOffset554 to i32*
  %577 = load i32, i32* %576, align 4
  %578 = add i32 %577, 44
  %579 = inttoptr i32 %578 to i32*
  %580 = load i32, i32* %579, align 4
  %.not793 = icmp slt i32 %575, %580
  br i1 %.not793, label %blk_exit494, label %loop_entry553

brif_next563:                                     ; preds = %blk_exit489
  %calcOffset565 = add i32 %_local_3.24, 12
  %581 = inttoptr i32 %calcOffset565 to i32*
  %582 = load i32, i32* %581, align 4
  %.not784 = icmp eq i32 %582, -1
  br i1 %.not784, label %brif_next567, label %brif_next563.blk_exit564_crit_edge

brif_next563.blk_exit564_crit_edge:               ; preds = %brif_next563
  %.pre = add i32 %_local_3.24, 44
  %.pre812 = inttoptr i32 %.pre to i32*
  br label %blk_exit564

blk_exit564:                                      ; preds = %brif_next579, %blk_exit573, %brif_next572, %brif_next563.blk_exit564_crit_edge
  %.pre-phi813 = phi i32* [ %.pre812, %brif_next563.blk_exit564_crit_edge ], [ %587, %blk_exit573 ], [ %587, %brif_next579 ], [ %587, %brif_next572 ]
  %_local_5.16 = phi i32 [ %582, %brif_next563.blk_exit564_crit_edge ], [ %596, %blk_exit573 ], [ %604, %brif_next579 ], [ %594, %brif_next572 ]
  %583 = add i32 %_local_5.16, 1
  store i32 %583, i32* %581, align 4
  %584 = load i32, i32* %.pre-phi813, align 4
  %585 = icmp slt i32 %583, %584
  br i1 %585, label %blk_exit582, label %brif_next585

brif_next567:                                     ; preds = %brif_next563
  %586 = add i32 %_local_3.24, 44
  %587 = inttoptr i32 %586 to i32*
  %588 = load i32, i32* %587, align 4
  %calcOffset569 = add i32 %0, -1024
  %589 = inttoptr i32 %calcOffset569 to i64*
  store i64 0, i64* %589, align 8
  %590 = call i32 @getenv(i32 1447)
  %591 = icmp eq i32 %590, 0
  br i1 %591, label %blk_exit570, label %brif_next572

blk_exit570:                                      ; preds = %brif_next567
  %592 = call i32 @getenv(i32 1522)
  %brif_val574.not = icmp eq i32 %592, 0
  br i1 %brif_val574.not, label %brif_next575, label %blk_exit573

brif_next572:                                     ; preds = %brif_next567
  %593 = call i32 @atol(i32 %590)
  %594 = urem i32 %593, %588
  br label %blk_exit564

blk_exit573:                                      ; preds = %brif_next577, %brif_next575, %blk_exit570
  %595 = call i32 @random()
  %596 = urem i32 %595, %588
  br label %blk_exit564

brif_next575:                                     ; preds = %blk_exit570
  %597 = call i32 @fopen(i32 1208, i32 1414)
  %598 = icmp eq i32 %597, 0
  br i1 %598, label %blk_exit573, label %brif_next577

brif_next577:                                     ; preds = %brif_next575
  %599 = call i32 @fread(i32 %calcOffset569, i32 8, i32 1, i32 %597)
  %600 = call i32 @fclose(i32 %597)
  %.not791 = icmp eq i32 %599, 1
  br i1 %.not791, label %brif_next579, label %blk_exit573

brif_next579:                                     ; preds = %brif_next577
  %601 = load i64, i64* %589, align 8
  %602 = zext i32 %588 to i64
  %603 = urem i64 %601, %602
  %604 = trunc i64 %603 to i32
  br label %blk_exit564

blk_exit582:                                      ; preds = %brif_next585, %blk_exit564
  %_local_5.17 = phi i32 [ %583, %blk_exit564 ], [ %607, %brif_next585 ]
  %calcOffset588 = add i32 %_local_3.24, 8
  %605 = inttoptr i32 %calcOffset588 to i32*
  %606 = load i32, i32* %605, align 4
  %brif_val589.not = icmp slt i32 %606, 0
  br i1 %brif_val589.not, label %brif_next590, label %blk_exit587

brif_next585:                                     ; preds = %blk_exit564
  %607 = sub i32 %583, %584
  store i32 %607, i32* %581, align 4
  br label %blk_exit582

blk_exit587:                                      ; preds = %brif_next594, %blk_exit582
  %_local_5.18 = phi i32 [ %617, %brif_next594 ], [ %_local_5.17, %blk_exit582 ]
  %_param_0.11 = phi i32 [ %616, %brif_next594 ], [ %606, %blk_exit582 ]
  %608 = shl i32 %_local_5.18, 2
  %609 = add i32 %608, 24
  %610 = zext i32 %609 to i64
  %611 = call i64 @lseek(i32 %_param_0.11, i64 %610, i32 0)
  %612 = load i32, i32* %605, align 4
  %613 = call i32 @read(i32 %612, i32 3360, i32 4)
  %brif_val597.not = icmp sgt i32 %613, -1
  br i1 %brif_val597.not, label %brif_next598, label %blk_exit26

brif_next590:                                     ; preds = %blk_exit582
  %calcOffset591 = add i32 %_local_3.24, 28
  %614 = inttoptr i32 %calcOffset591 to i32*
  %615 = load i32, i32* %614, align 4
  %616 = call i32 @open(i32 %615, i32 67108864, i32 0)
  store i32 %616, i32* %605, align 4
  %brif_val593.not = icmp sgt i32 %616, -1
  br i1 %brif_val593.not, label %brif_next594, label %blk_exit27

brif_next594:                                     ; preds = %brif_next590
  %617 = load i32, i32* %581, align 4
  br label %blk_exit587

brif_next598:                                     ; preds = %blk_exit587
  %618 = load i32, i32* %605, align 4
  %619 = call i32 @read(i32 %618, i32 3364, i32 4)
  %brif_val600.not = icmp sgt i32 %619, -1
  br i1 %brif_val600.not, label %brif_next601, label %blk_exit26

brif_next601:                                     ; preds = %brif_next598
  %620 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %621 = call i32 @ntohl(i32 %620)
  store i32 %621, i32* inttoptr (i32 3360 to i32*), align 32
  %622 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %623 = call i32 @ntohl(i32 %622)
  store i32 %623, i32* inttoptr (i32 3364 to i32*), align 4
  %624 = load i8, i8* inttoptr (i32 2273 to i8*), align 1
  %625 = icmp eq i8 %624, 0
  br i1 %625, label %blk_exit602, label %brif_next604

blk_exit602:                                      ; preds = %blk_exit605, %brif_next601
  %626 = load i8, i8* inttoptr (i32 2274 to i8*), align 2
  %627 = icmp eq i8 %626, 0
  br i1 %627, label %blk_exit636, label %brif_next638

brif_next604:                                     ; preds = %brif_next601
  %628 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %629 = add i32 %628, 56
  %630 = inttoptr i32 %629 to i8*
  %631 = load i8, i8* %630, align 1
  %632 = and i8 %631, 3
  %brif_val608.not = icmp eq i8 %632, 0
  br i1 %brif_val608.not, label %brif_next609, label %blk_exit606

blk_exit605:                                      ; preds = %brif_next627, %blk_exit623, %blk_exit610, %brif_next609
  %_local_3.31 = phi i32 [ %639, %brif_next609 ], [ 0, %blk_exit610 ], [ %_local_3.32, %brif_next627 ], [ %664, %blk_exit623 ]
  store i32 %_local_3.31, i32* inttoptr (i32 2284 to i32*), align 4
  %633 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %634 = icmp sgt i32 %_local_3.31, %633
  br i1 %634, label %loop_entry404, label %blk_exit602

blk_exit606:                                      ; preds = %brif_next604
  %calcOffset611 = add i32 %628, 16
  %635 = inttoptr i32 %calcOffset611 to i32*
  %636 = load i32, i32* %635, align 4
  %brif_val612.not = icmp eq i32 %636, 0
  br i1 %brif_val612.not, label %brif_next613, label %blk_exit610

brif_next609:                                     ; preds = %brif_next604
  %637 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %638 = sub i32 %623, %637
  %639 = add i32 %638, -2
  br label %blk_exit605

blk_exit610:                                      ; preds = %brif_next617, %blk_exit606
  %_param_0.12 = phi i32 [ %654, %brif_next617 ], [ %636, %blk_exit606 ]
  %640 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %641 = call i32 @fseek(i32 %_param_0.12, i32 %640, i32 0)
  %642 = add i32 %0, -1024
  %643 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset619 = add i32 %643, 16
  %644 = inttoptr i32 %calcOffset619 to i32*
  %645 = load i32, i32* %644, align 4
  %646 = call i32 @fgets(i32 %642, i32 1024, i32 %645)
  %647 = icmp eq i32 %646, 0
  br i1 %647, label %blk_exit605, label %loop_entry622

brif_next613:                                     ; preds = %blk_exit606
  %calcOffset614 = add i32 %628, 4
  %648 = inttoptr i32 %calcOffset614 to i32*
  %649 = load i32, i32* %648, align 4
  %650 = call i32 @fdopen(i32 %649, i32 1194)
  store i32 %650, i32* %635, align 4
  %651 = icmp eq i32 %650, 0
  br i1 %651, label %blk_exit25, label %brif_next617

brif_next617:                                     ; preds = %brif_next613
  %652 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset618 = add i32 %652, 16
  %653 = inttoptr i32 %calcOffset618 to i32*
  %654 = load i32, i32* %653, align 4
  br label %blk_exit610

loop_entry622:                                    ; preds = %blk_exit623, %blk_exit610
  %_local_3.32 = phi i32 [ %664, %blk_exit623 ], [ 0, %blk_exit610 ]
  %655 = inttoptr i32 %642 to i8*
  %656 = load i8, i8* %655, align 1
  %657 = sext i8 %656 to i32
  %658 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %659 = add i32 %658, 60
  %660 = inttoptr i32 %659 to i8*
  %661 = load i8, i8* %660, align 1
  %662 = zext i8 %661 to i32
  %.not790 = icmp eq i32 %657, %662
  br i1 %.not790, label %brif_next627, label %blk_exit623

blk_exit623:                                      ; preds = %brif_next627, %loop_entry622
  %663 = call i32 @strlen(i32 %642)
  %664 = add i32 %663, %_local_3.32
  %calcOffset631 = add i32 %658, 16
  %665 = inttoptr i32 %calcOffset631 to i32*
  %666 = load i32, i32* %665, align 4
  %667 = call i32 @fgets(i32 %642, i32 1024, i32 %666)
  %brif_val632.not = icmp eq i32 %667, 0
  br i1 %brif_val632.not, label %blk_exit605, label %loop_entry622

brif_next627:                                     ; preds = %loop_entry622
  %calcOffset628 = add i32 %0, -1023
  %668 = inttoptr i32 %calcOffset628 to i8*
  %669 = load i8, i8* %668, align 1
  %670 = icmp eq i8 %669, 10
  br i1 %670, label %blk_exit605, label %blk_exit623

blk_exit636:                                      ; preds = %blk_exit639, %blk_exit602
  %671 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset671 = add i32 %671, 16
  %672 = inttoptr i32 %calcOffset671 to i32*
  %673 = load i32, i32* %672, align 4
  %brif_val672.not = icmp eq i32 %673, 0
  br i1 %brif_val672.not, label %brif_next673, label %blk_exit670

brif_next638:                                     ; preds = %blk_exit602
  %674 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %675 = add i32 %674, 56
  %676 = inttoptr i32 %675 to i8*
  %677 = load i8, i8* %676, align 1
  %678 = and i8 %677, 3
  %brif_val642.not = icmp eq i8 %678, 0
  br i1 %brif_val642.not, label %brif_next643, label %blk_exit640

blk_exit639:                                      ; preds = %brif_next661, %blk_exit657, %blk_exit644, %brif_next643
  %_local_3.33 = phi i32 [ %685, %brif_next643 ], [ 0, %blk_exit644 ], [ %_local_3.34, %brif_next661 ], [ %710, %blk_exit657 ]
  store i32 %_local_3.33, i32* inttoptr (i32 2284 to i32*), align 4
  %679 = load i32, i32* inttoptr (i32 2208 to i32*), align 32
  %.not785 = icmp sgt i32 %_local_3.33, %679
  br i1 %.not785, label %blk_exit636, label %loop_entry404

blk_exit640:                                      ; preds = %brif_next638
  %calcOffset645 = add i32 %674, 16
  %680 = inttoptr i32 %calcOffset645 to i32*
  %681 = load i32, i32* %680, align 4
  %brif_val646.not = icmp eq i32 %681, 0
  br i1 %brif_val646.not, label %brif_next647, label %blk_exit644

brif_next643:                                     ; preds = %brif_next638
  %682 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %683 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %684 = sub i32 %682, %683
  %685 = add i32 %684, -2
  br label %blk_exit639

blk_exit644:                                      ; preds = %brif_next651, %blk_exit640
  %_local_5.19 = phi i32 [ %700, %brif_next651 ], [ %681, %blk_exit640 ]
  %686 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %687 = call i32 @fseek(i32 %_local_5.19, i32 %686, i32 0)
  %688 = add i32 %0, -1024
  %689 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset653 = add i32 %689, 16
  %690 = inttoptr i32 %calcOffset653 to i32*
  %691 = load i32, i32* %690, align 4
  %692 = call i32 @fgets(i32 %688, i32 1024, i32 %691)
  %693 = icmp eq i32 %692, 0
  br i1 %693, label %blk_exit639, label %loop_entry656

brif_next647:                                     ; preds = %blk_exit640
  %calcOffset648 = add i32 %674, 4
  %694 = inttoptr i32 %calcOffset648 to i32*
  %695 = load i32, i32* %694, align 4
  %696 = call i32 @fdopen(i32 %695, i32 1194)
  store i32 %696, i32* %680, align 4
  %697 = icmp eq i32 %696, 0
  br i1 %697, label %blk_exit24, label %brif_next651

brif_next651:                                     ; preds = %brif_next647
  %698 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset652 = add i32 %698, 16
  %699 = inttoptr i32 %calcOffset652 to i32*
  %700 = load i32, i32* %699, align 4
  br label %blk_exit644

loop_entry656:                                    ; preds = %blk_exit657, %blk_exit644
  %_local_3.34 = phi i32 [ %710, %blk_exit657 ], [ 0, %blk_exit644 ]
  %701 = inttoptr i32 %688 to i8*
  %702 = load i8, i8* %701, align 1
  %703 = sext i8 %702 to i32
  %704 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %705 = add i32 %704, 60
  %706 = inttoptr i32 %705 to i8*
  %707 = load i8, i8* %706, align 1
  %708 = zext i8 %707 to i32
  %.not789 = icmp eq i32 %703, %708
  br i1 %.not789, label %brif_next661, label %blk_exit657

blk_exit657:                                      ; preds = %brif_next661, %loop_entry656
  %709 = call i32 @strlen(i32 %688)
  %710 = add i32 %709, %_local_3.34
  %calcOffset665 = add i32 %704, 16
  %711 = inttoptr i32 %calcOffset665 to i32*
  %712 = load i32, i32* %711, align 4
  %713 = call i32 @fgets(i32 %688, i32 1024, i32 %712)
  %brif_val666.not = icmp eq i32 %713, 0
  br i1 %brif_val666.not, label %blk_exit639, label %loop_entry656

brif_next661:                                     ; preds = %loop_entry656
  %calcOffset662 = add i32 %0, -1023
  %714 = inttoptr i32 %calcOffset662 to i8*
  %715 = load i8, i8* %714, align 1
  %716 = icmp eq i8 %715, 10
  br i1 %716, label %blk_exit639, label %blk_exit657

blk_exit670:                                      ; preds = %brif_next673, %blk_exit636
  %_local_3.35 = phi i32 [ %723, %brif_next673 ], [ %673, %blk_exit636 ]
  %717 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %718 = call i32 @fseek(i32 %_local_3.35, i32 %717, i32 0)
  %719 = load i8, i8* inttoptr (i32 2295 to i8*), align 1
  %720 = icmp eq i8 %719, 0
  br i1 %720, label %blk_exit678, label %brif_next680

brif_next673:                                     ; preds = %blk_exit636
  %calcOffset674 = add i32 %671, 4
  %721 = inttoptr i32 %calcOffset674 to i32*
  %722 = load i32, i32* %721, align 4
  %723 = call i32 @fdopen(i32 %722, i32 1194)
  store i32 %723, i32* %672, align 4
  %724 = icmp eq i32 %723, 0
  br i1 %724, label %blk_exit23, label %blk_exit670

blk_exit678:                                      ; preds = %brif_next680, %blk_exit670
  store i32 0, i32* inttoptr (i32 2284 to i32*), align 4
  %725 = add i32 %0, -1024
  %726 = load i32, i32* %672, align 4
  %727 = call i32 @fgets(i32 %725, i32 1024, i32 %726)
  %brif_val686.not = icmp eq i32 %727, 0
  br i1 %brif_val686.not, label %brif_next687, label %blk_exit684

brif_next680:                                     ; preds = %blk_exit670
  %calcOffset681 = add i32 %671, 20
  %728 = inttoptr i32 %calcOffset681 to i32*
  %729 = load i32, i32* %728, align 4
  %calcOffset682 = add i32 %0, -2896
  %730 = inttoptr i32 %calcOffset682 to i32*
  store i32 %729, i32* %730, align 4
  %731 = call i32 @printf(i32 2066, i32 %calcOffset682)
  br label %blk_exit678

blk_exit683:                                      ; preds = %blk_exit719, %brif_next693, %brif_next687
  %_local_11.1 = phi i32 [ %736, %brif_next687 ], [ %735, %brif_next693 ], [ %735, %blk_exit719 ]
  %732 = call i32 @fflush(i32 %_local_11.1)
  %733 = load i8, i8* inttoptr (i32 2280 to i8*), align 8
  %734 = icmp eq i8 %733, 0
  br i1 %734, label %blk_exit19, label %brif_next730

blk_exit684:                                      ; preds = %blk_exit678
  %735 = load i32, i32* null, align 2147483648
  br label %loop_entry688

brif_next687:                                     ; preds = %blk_exit678
  %736 = load i32, i32* null, align 2147483648
  br label %blk_exit683

loop_entry688:                                    ; preds = %blk_exit719, %blk_exit684
  %737 = inttoptr i32 %725 to i8*
  %738 = load i8, i8* %737, align 1
  %calcOffset691 = add i32 %671, 60
  %739 = inttoptr i32 %calcOffset691 to i8*
  %740 = load i8, i8* %739, align 1
  %.not787 = icmp eq i8 %738, %740
  br i1 %.not787, label %brif_next693, label %blk_exit689

blk_exit689:                                      ; preds = %brif_next693, %loop_entry688
  %calcOffset698 = add i32 %671, 56
  %741 = inttoptr i32 %calcOffset698 to i8*
  %742 = load i8, i8* %741, align 1
  %743 = and i8 %742, 4
  %brif_val699.not.not = icmp eq i8 %743, 0
  %744 = icmp eq i8 %738, 0
  %or.cond804 = select i1 %brif_val699.not.not, i1 true, i1 %744
  br i1 %or.cond804, label %blk_exit697, label %loop_entry703

brif_next693:                                     ; preds = %loop_entry688
  %calcOffset694 = add i32 %0, -1023
  %745 = inttoptr i32 %calcOffset694 to i8*
  %746 = load i8, i8* %745, align 1
  %747 = icmp eq i8 %746, 10
  br i1 %747, label %blk_exit683, label %blk_exit689

blk_exit697:                                      ; preds = %blk_exit704, %blk_exit689
  %calcOffset721 = add i32 %671, 38
  %748 = inttoptr i32 %calcOffset721 to i8*
  %749 = load i8, i8* %748, align 1
  %750 = icmp eq i8 %749, 0
  br i1 %750, label %blk_exit720, label %brif_next723

loop_entry703:                                    ; preds = %blk_exit704, %blk_exit689
  %_local_5.20 = phi i32 [ %755, %blk_exit704 ], [ %725, %blk_exit689 ]
  %_local_3.36.in = phi i8 [ %757, %blk_exit704 ], [ %738, %blk_exit689 ]
  %_local_3.36 = zext i8 %_local_3.36.in to i32
  %751 = shl nuw i32 %_local_3.36, 24
  %752 = sext i8 %_local_3.36.in to i32
  %753 = add nsw i32 %752, -65
  %754 = icmp ult i32 %753, 26
  %brif_val709.not = icmp sgt i32 %751, -16777216
  %or.cond805 = select i1 %754, i1 %brif_val709.not, i1 false
  br i1 %or.cond805, label %blk_exit705, label %blk_exit706

blk_exit704:                                      ; preds = %blk_exit706, %blk_exit705
  %755 = add i32 %_local_5.20, 1
  %756 = inttoptr i32 %755 to i8*
  %757 = load i8, i8* %756, align 1
  %brif_val717.not = icmp eq i8 %757, 0
  br i1 %brif_val717.not, label %blk_exit697, label %loop_entry703

blk_exit705:                                      ; preds = %blk_exit706, %loop_entry703
  %storemerge788 = phi i8 [ 65, %loop_entry703 ], [ 97, %blk_exit706 ]
  %storemerge = phi i8 [ -52, %loop_entry703 ], [ -84, %blk_exit706 ]
  %758 = add i8 %storemerge, %_local_3.36.in
  %759 = urem i8 %758, 26
  %760 = add nuw nsw i8 %759, %storemerge788
  %761 = inttoptr i32 %_local_5.20 to i8*
  store i8 %760, i8* %761, align 1
  br label %blk_exit704

blk_exit706:                                      ; preds = %loop_entry703
  %762 = add nsw i32 %752, -97
  %763 = icmp ult i32 %762, 26
  br i1 %763, label %blk_exit705, label %blk_exit704

blk_exit719:                                      ; preds = %brif_next725, %blk_exit720
  %764 = load i32, i32* inttoptr (i32 2284 to i32*), align 4
  %765 = add i32 %764, 1
  store i32 %765, i32* inttoptr (i32 2284 to i32*), align 4
  %766 = load i32, i32* %672, align 4
  %767 = call i32 @fgets(i32 %725, i32 1024, i32 %766)
  %brif_val727.not = icmp eq i32 %767, 0
  br i1 %brif_val727.not, label %blk_exit683, label %loop_entry688

blk_exit720:                                      ; preds = %brif_next723, %blk_exit697
  %768 = call i32 @fputs(i32 %725, i32 %735)
  br label %blk_exit719

brif_next723:                                     ; preds = %blk_exit697
  %769 = load i8, i8* inttoptr (i32 2294 to i8*), align 2
  %brif_val724.not = icmp eq i8 %769, 0
  br i1 %brif_val724.not, label %brif_next725, label %blk_exit720

brif_next725:                                     ; preds = %brif_next723
  %770 = call i32 @strdup(i32 %725)
  %771 = call i32 @fputs(i32 %770, i32 %735)
  call void @free(i32 %770)
  br label %blk_exit719

brif_next730:                                     ; preds = %blk_exit683
  %772 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %773 = add i32 %772, 56
  %774 = inttoptr i32 %773 to i8*
  %775 = load i8, i8* %774, align 1
  %776 = and i8 %775, 3
  %brif_val734.not = icmp eq i8 %776, 0
  br i1 %brif_val734.not, label %brif_next735, label %blk_exit732

blk_exit731:                                      ; preds = %brif_next753, %blk_exit749, %blk_exit736, %brif_next735
  %_local_3.37 = phi i32 [ %786, %brif_next735 ], [ 0, %blk_exit736 ], [ %_local_3.38, %brif_next753 ], [ %810, %blk_exit749 ]
  store i32 %_local_3.37, i32* inttoptr (i32 2284 to i32*), align 4
  %777 = sdiv i32 %_local_3.37, 20
  %778 = icmp sgt i32 %_local_3.37, 139
  %779 = select i1 %778, i32 %777, i32 6
  %780 = call i32 @sleep(i32 %779)
  br label %blk_exit19

blk_exit732:                                      ; preds = %brif_next730
  %calcOffset737 = add i32 %772, 16
  %781 = inttoptr i32 %calcOffset737 to i32*
  %782 = load i32, i32* %781, align 4
  %brif_val738.not = icmp eq i32 %782, 0
  br i1 %brif_val738.not, label %brif_next739, label %blk_exit736

brif_next735:                                     ; preds = %brif_next730
  %783 = load i32, i32* inttoptr (i32 3364 to i32*), align 4
  %784 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %785 = sub i32 %783, %784
  %786 = add i32 %785, -2
  br label %blk_exit731

blk_exit736:                                      ; preds = %brif_next743, %blk_exit732
  %_local_5.21 = phi i32 [ %800, %brif_next743 ], [ %782, %blk_exit732 ]
  %787 = load i32, i32* inttoptr (i32 3360 to i32*), align 32
  %788 = call i32 @fseek(i32 %_local_5.21, i32 %787, i32 0)
  %789 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset745 = add i32 %789, 16
  %790 = inttoptr i32 %calcOffset745 to i32*
  %791 = load i32, i32* %790, align 4
  %792 = call i32 @fgets(i32 %725, i32 1024, i32 %791)
  %793 = icmp eq i32 %792, 0
  br i1 %793, label %blk_exit731, label %loop_entry748

brif_next739:                                     ; preds = %blk_exit732
  %calcOffset740 = add i32 %772, 4
  %794 = inttoptr i32 %calcOffset740 to i32*
  %795 = load i32, i32* %794, align 4
  %796 = call i32 @fdopen(i32 %795, i32 1194)
  store i32 %796, i32* %781, align 4
  %797 = icmp eq i32 %796, 0
  br i1 %797, label %blk_exit22, label %brif_next743

brif_next743:                                     ; preds = %brif_next739
  %798 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %calcOffset744 = add i32 %798, 16
  %799 = inttoptr i32 %calcOffset744 to i32*
  %800 = load i32, i32* %799, align 4
  br label %blk_exit736

loop_entry748:                                    ; preds = %blk_exit749, %blk_exit736
  %_local_3.38 = phi i32 [ %810, %blk_exit749 ], [ 0, %blk_exit736 ]
  %801 = inttoptr i32 %725 to i8*
  %802 = load i8, i8* %801, align 1
  %803 = sext i8 %802 to i32
  %804 = load i32, i32* inttoptr (i32 2276 to i32*), align 4
  %805 = add i32 %804, 60
  %806 = inttoptr i32 %805 to i8*
  %807 = load i8, i8* %806, align 1
  %808 = zext i8 %807 to i32
  %.not786 = icmp eq i32 %803, %808
  br i1 %.not786, label %brif_next753, label %blk_exit749

blk_exit749:                                      ; preds = %brif_next753, %loop_entry748
  %809 = call i32 @strlen(i32 %725)
  %810 = add i32 %809, %_local_3.38
  %calcOffset757 = add i32 %804, 16
  %811 = inttoptr i32 %calcOffset757 to i32*
  %812 = load i32, i32* %811, align 4
  %813 = call i32 @fgets(i32 %725, i32 1024, i32 %812)
  %brif_val758.not = icmp eq i32 %813, 0
  br i1 %brif_val758.not, label %blk_exit731, label %loop_entry748

brif_next753:                                     ; preds = %loop_entry748
  %calcOffset754 = add i32 %0, -1023
  %814 = inttoptr i32 %calcOffset754 to i8*
  %815 = load i8, i8* %814, align 1
  %816 = icmp eq i8 %815, 10
  br i1 %816, label %blk_exit731, label %blk_exit749
}

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
define i32 @__main_void() #0 {
allocator:
  %0 = call i32 @main(i32 0, i32 0)
  ret i32 %0
}

attributes #0 = { null_pointer_is_valid }
