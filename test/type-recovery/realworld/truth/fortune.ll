; ModuleID = '../../bin/fortune.bc'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20"
target triple = "wasm32-unknown-wasi"

%struct.re_pattern_buffer = type { i32, i8*, [4 x i8*], i32, i8 }
%struct.fd = type { i32, i32, i32, i32, %struct._IO_FILE*, i8*, i8*, i8*, i8*, i8, i8, i8, %struct.STRFILE, i32, %struct.fd*, %struct.fd*, %struct.fd*, %struct.fd* }
%struct._IO_FILE = type opaque
%struct.STRFILE = type { i32, i32, i32, i32, i32, [4 x i8] }
%struct.regmatch_t = type { i32, i32 }
%struct.stat = type { i64, i64, i64, i32, i32, i32, i32, i64, i64, i32, i64, %struct.timespec, %struct.timespec, %struct.timespec, [3 x i64] }
%struct.timespec = type { i64, i32 }
%struct._DIR = type opaque
%struct.dirent = type { i64, i8, [0 x i8] }

@.str = private unnamed_addr constant [7 x i8] c"LC_ALL\00", align 1
@env_lang = internal global i8* null, align 4, !dbg !0
@.str.1 = private unnamed_addr constant [12 x i8] c"LC_MESSAGES\00", align 1
@.str.2 = private unnamed_addr constant [9 x i8] c"LANGUAGE\00", align 1
@.str.3 = private unnamed_addr constant [5 x i8] c"LANG\00", align 1
@.str.4 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@Match = internal global i8 0, align 1, !dbg !116
@Re_pat = internal global %struct.re_pattern_buffer zeroinitializer, align 4, !dbg !30
@Find_files = internal global i8 0, align 1, !dbg !120
@File_list = internal global %struct.fd* null, align 4, !dbg !129
@Equal_probs = internal global i8 0, align 1, !dbg !131
@Short_only = internal global i8 0, align 1, !dbg !142
@SLEN = internal global i32 160, align 4, !dbg !144
@Long_only = internal global i8 0, align 1, !dbg !146
@Fortfile = internal global %struct.fd* null, align 4, !dbg !45
@Wait = internal global i8 0, align 1, !dbg !148
@Fort_len = internal global i32 0, align 4, !dbg !150
@Fortbuf = internal global i8* null, align 4, !dbg !152
@.str.5 = private unnamed_addr constant [17 x i8] c"acefhilm:n:osuvw\00", align 1
@All_forts = internal global i8 0, align 1, !dbg !86
@optarg = external global i8*, align 4
@Offend = internal global i8 0, align 1, !dbg !88
@No_recode = internal global i8 0, align 1, !dbg !90
@.str.6 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@Show_filename = internal global i8 0, align 1, !dbg !101
@optind = external global i32, align 4
@ErrorMessage = internal global i8 0, align 1, !dbg !114
@stderr = external constant %struct._IO_FILE*, align 4
@.str.7 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.str.8 = private unnamed_addr constant [19 x i8] c"No fortunes found\0A\00", align 1
@.str.9 = private unnamed_addr constant [17 x i8] c"bad pattern: %s\0A\00", align 1
@program_version.buf = internal global [1024 x i8] zeroinitializer, align 16, !dbg !92
@.str.10 = private unnamed_addr constant [14 x i8] c"%s version %s\00", align 1
@.str.11 = private unnamed_addr constant [12 x i8] c"fortune-mod\00", align 1
@.str.12 = private unnamed_addr constant [7 x i8] c"3.24.0\00", align 1
@.str.13 = private unnamed_addr constant [12 x i8] c"fortune [-a\00", align 1
@.str.14 = private unnamed_addr constant [2 x i8] c"f\00", align 1
@.str.15 = private unnamed_addr constant [2 x i8] c"i\00", align 1
@.str.16 = private unnamed_addr constant [2 x i8] c"l\00", align 1
@.str.17 = private unnamed_addr constant [2 x i8] c"o\00", align 1
@.str.18 = private unnamed_addr constant [4 x i8] c"sw]\00", align 1
@.str.19 = private unnamed_addr constant [14 x i8] c" [-m pattern]\00", align 1
@.str.20 = private unnamed_addr constant [41 x i8] c" [-n number] [ [#%] file/directory/all]\0A\00", align 1
@.str.21 = private unnamed_addr constant [38 x i8] c"/usr/local/local/share/games/fortunes\00", align 1
@.str.22 = private unnamed_addr constant [42 x i8] c"/usr/local/local/share/games/fortunes/off\00", align 1
@File_tail = internal global %struct.fd* null, align 4, !dbg !112
@.str.23 = private unnamed_addr constant [28 x i8] c"percentages must be <= 100\0A\00", align 1
@.str.24 = private unnamed_addr constant [48 x i8] c"Overflow percentage detected at argument \22%s\22!\0A\00", align 1
@.str.25 = private unnamed_addr constant [30 x i8] c"percentages must be integers\0A\00", align 1
@.str.26 = private unnamed_addr constant [32 x i8] c"percentages must precede files\0A\00", align 1
@.str.27 = private unnamed_addr constant [36 x i8] c"/usr/local/share/games/fortunes/off\00", align 1
@.str.28 = private unnamed_addr constant [32 x i8] c"/usr/local/share/games/fortunes\00", align 1
@.str.29 = private unnamed_addr constant [4 x i8] c"all\00", align 1
@.str.30 = private unnamed_addr constant [2 x i8] c"/\00", align 1
@.str.31 = private unnamed_addr constant [3 x i8] c"./\00", align 1
@.str.32 = private unnamed_addr constant [4 x i8] c"../\00", align 1
@.str.33 = private unnamed_addr constant [6 x i8] c"%s/%s\00", align 1
@.str.34 = private unnamed_addr constant [9 x i8] c"%s/%s/%s\00", align 1
@.str.35 = private unnamed_addr constant [4 x i8] c"PWD\00", align 1
@.str.36 = private unnamed_addr constant [6 x i8] c"%s.u8\00", align 1
@.str.37 = private unnamed_addr constant [44 x i8] c"fortune:%s not a fortune file or directory\0A\00", align 1
@.str.38 = private unnamed_addr constant [25 x i8] c"fortune: out of memory.\0A\00", align 1
@errno = external thread_local global i32, align 4
@.str.39 = private unnamed_addr constant [33 x i8] c"fortune: bad juju in is_existant\00", align 1
@.str.40 = private unnamed_addr constant [12 x i8] c"Out of RAM!\00", align 1
@.str.41 = private unnamed_addr constant [45 x i8] c"fortune: %s: No fortune files in directory.\0A\00", align 1
@is_fortfile.suflist = internal global [14 x i8*] [i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.42, i32 0, i32 0), i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.43, i32 0, i32 0), i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.44, i32 0, i32 0), i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.45, i32 0, i32 0), i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.46, i32 0, i32 0), i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.15, i32 0, i32 0), i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.14, i32 0, i32 0), i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.47, i32 0, i32 0), i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.48, i32 0, i32 0), i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str.49, i32 0, i32 0), i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.50, i32 0, i32 0), i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.51, i32 0, i32 0), i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.52, i32 0, i32 0), i8* null], align 16, !dbg !103
@.str.42 = private unnamed_addr constant [4 x i8] c"dat\00", align 1
@.str.43 = private unnamed_addr constant [4 x i8] c"pos\00", align 1
@.str.44 = private unnamed_addr constant [2 x i8] c"c\00", align 1
@.str.45 = private unnamed_addr constant [2 x i8] c"h\00", align 1
@.str.46 = private unnamed_addr constant [2 x i8] c"p\00", align 1
@.str.47 = private unnamed_addr constant [4 x i8] c"pas\00", align 1
@.str.48 = private unnamed_addr constant [4 x i8] c"ftn\00", align 1
@.str.49 = private unnamed_addr constant [6 x i8] c"ins.c\00", align 1
@.str.50 = private unnamed_addr constant [8 x i8] c"ins,pas\00", align 1
@.str.51 = private unnamed_addr constant [8 x i8] c"ins.ftn\00", align 1
@.str.52 = private unnamed_addr constant [4 x i8] c"sml\00", align 1
@.str.53 = private unnamed_addr constant [7 x i8] c"%s.dat\00", align 1
@.str.54 = private unnamed_addr constant [47 x i8] c"pattern too long for ignoring case; overflow!\0A\00", align 1
@.str.55 = private unnamed_addr constant [36 x i8] c"pattern too long for ignoring case\0A\00", align 1
@.str.56 = private unnamed_addr constant [23 x i8] c"fortune: %s corrupted\0A\00", align 1
@.str.57 = private unnamed_addr constant [9 x i8] c"(%s)\0A%c\0A\00", align 1
@stdout = external constant %struct._IO_FILE*, align 4
@.str.58 = private unnamed_addr constant [4 x i8] c"%c\0A\00", align 1
@.str.59 = private unnamed_addr constant [2 x i8] c"r\00", align 1
@.str.60 = private unnamed_addr constant [37 x i8] c"fortune: probabilities sum to %d%%!\0A\00", align 1
@.str.61 = private unnamed_addr constant [54 x i8] c"fortune: no place to put residual probability (%d%%)\0A\00", align 1
@.str.62 = private unnamed_addr constant [55 x i8] c"fortune: no probability left to put in residual files\0A\00", align 1
@Spec_prob = internal global i32 0, align 4, !dbg !118
@sum_noprobs.did_noprobs = internal global i8 0, align 1, !dbg !122
@Noprob_tbl = internal global %struct.STRFILE zeroinitializer, align 4, !dbg !127
@Num_kids = internal global i32 0, align 4, !dbg !133
@Num_files = internal global i32 0, align 4, !dbg !135
@.str.63 = private unnamed_addr constant [4 x i8] c"%*s\00", align 1
@.str.64 = private unnamed_addr constant [8 x i8] c"%5.2f%%\00", align 1
@.str.65 = private unnamed_addr constant [4 x i8] c" %s\00", align 1
@.str.66 = private unnamed_addr constant [5 x i8] c"NULL\00", align 1
@.str.67 = private unnamed_addr constant [27 x i8] c"fortune: no fortune found\0A\00", align 1
@.str.68 = private unnamed_addr constant [11 x i8] c"!fp->child\00", align 1
@.str.69 = private unnamed_addr constant [72 x i8] c"/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c\00", align 1
@__func__.get_fort = private unnamed_addr constant [9 x i8] c"get_fort\00", align 1
@Seekpts = internal global [2 x i32] zeroinitializer, align 4, !dbg !137
@.str.70 = private unnamed_addr constant [33 x i8] c"FORTUNE_MOD_RAND_HARD_CODED_VALS\00", align 1
@.str.71 = private unnamed_addr constant [22 x i8] c"FORTUNE_MOD_USE_SRAND\00", align 1
@.str.72 = private unnamed_addr constant [13 x i8] c"/dev/urandom\00", align 1
@.str.73 = private unnamed_addr constant [3 x i8] c"rb\00", align 1
@.str.74 = private unnamed_addr constant [13 x i8] c"fp->read_tbl\00", align 1
@__func__.get_pos = private unnamed_addr constant [8 x i8] c"get_pos\00", align 1
@.str.75 = private unnamed_addr constant [9 x i8] c"(%s)\0A%%\0A\00", align 1

; Function Attrs: noinline nounwind optnone
define hidden i32 @__main_argc_argv(i32 noundef %0, i8** noundef %1) #0 !dbg !158 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 4
  %6 = alloca i32, align 4
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  call void @llvm.dbg.declare(metadata i32* %4, metadata !161, metadata !DIExpression()), !dbg !162
  store i8** %1, i8*** %5, align 4
  call void @llvm.dbg.declare(metadata i8*** %5, metadata !163, metadata !DIExpression()), !dbg !164
  call void @llvm.dbg.declare(metadata i32* %6, metadata !165, metadata !DIExpression()), !dbg !166
  store i32 0, i32* %6, align 4, !dbg !166
  %7 = call i8* @getenv(i8* noundef getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i32 0, i32 0)) #9, !dbg !167
  store i8* %7, i8** @env_lang, align 4, !dbg !168
  %8 = load i8*, i8** @env_lang, align 4, !dbg !169
  %9 = icmp ne i8* %8, null, !dbg !169
  br i1 %9, label %12, label %10, !dbg !171

10:                                               ; preds = %2
  %11 = call i8* @getenv(i8* noundef getelementptr inbounds ([12 x i8], [12 x i8]* @.str.1, i32 0, i32 0)) #9, !dbg !172
  store i8* %11, i8** @env_lang, align 4, !dbg !174
  br label %12, !dbg !175

12:                                               ; preds = %10, %2
  %13 = load i8*, i8** @env_lang, align 4, !dbg !176
  %14 = icmp ne i8* %13, null, !dbg !176
  br i1 %14, label %17, label %15, !dbg !178

15:                                               ; preds = %12
  %16 = call i8* @getenv(i8* noundef getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i32 0, i32 0)) #9, !dbg !179
  store i8* %16, i8** @env_lang, align 4, !dbg !181
  br label %17, !dbg !182

17:                                               ; preds = %15, %12
  %18 = load i8*, i8** @env_lang, align 4, !dbg !183
  %19 = icmp ne i8* %18, null, !dbg !183
  br i1 %19, label %22, label %20, !dbg !185

20:                                               ; preds = %17
  %21 = call i8* @getenv(i8* noundef getelementptr inbounds ([5 x i8], [5 x i8]* @.str.3, i32 0, i32 0)) #9, !dbg !186
  store i8* %21, i8** @env_lang, align 4, !dbg !188
  br label %22, !dbg !189

22:                                               ; preds = %20, %17
  %23 = load i32, i32* %4, align 4, !dbg !190
  %24 = load i8**, i8*** %5, align 4, !dbg !191
  call void @getargs(i32 noundef %23, i8** noundef %24) #9, !dbg !192
  %25 = call i8* @setlocale(i32 noundef 6, i8* noundef getelementptr inbounds ([1 x i8], [1 x i8]* @.str.4, i32 0, i32 0)) #9, !dbg !193
  %26 = load i8, i8* @Match, align 1, !dbg !194
  %27 = trunc i8 %26 to i1, !dbg !194
  br i1 %27, label %28, label %31, !dbg !196

28:                                               ; preds = %22
  %29 = call zeroext i1 @find_matches() #9, !dbg !197
  %30 = zext i1 %29 to i32, !dbg !197
  store i32 %30, i32* %6, align 4, !dbg !199
  call void @regfree(%struct.re_pattern_buffer* noundef @Re_pat) #9, !dbg !200
  br label %73, !dbg !201

31:                                               ; preds = %22
  call void @init_prob() #9, !dbg !202
  %32 = load i8, i8* @Find_files, align 1, !dbg !203
  %33 = trunc i8 %32 to i1, !dbg !203
  br i1 %33, label %34, label %41, !dbg !205

34:                                               ; preds = %31
  %35 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !206
  call void @sum_noprobs(%struct.fd* noundef %35) #9, !dbg !208
  %36 = load i8, i8* @Equal_probs, align 1, !dbg !209
  %37 = trunc i8 %36 to i1, !dbg !209
  br i1 %37, label %38, label %39, !dbg !211

38:                                               ; preds = %34
  call void @calc_equal_probs() #9, !dbg !212
  br label %39, !dbg !214

39:                                               ; preds = %38, %34
  %40 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !215
  call void @print_list(%struct.fd* noundef %40, i32 noundef 0) #9, !dbg !216
  br label %72, !dbg !217

41:                                               ; preds = %31
  call void @call_srandom() #9, !dbg !218
  br label %42, !dbg !220

42:                                               ; preds = %59, %41
  call void @get_fort() #9, !dbg !221
  br label %43, !dbg !223

43:                                               ; preds = %42
  %44 = load i8, i8* @Short_only, align 1, !dbg !224
  %45 = trunc i8 %44 to i1, !dbg !224
  br i1 %45, label %46, label %50, !dbg !225

46:                                               ; preds = %43
  %47 = call i32 @fortlen() #9, !dbg !226
  %48 = load i32, i32* @SLEN, align 4, !dbg !227
  %49 = icmp sgt i32 %47, %48, !dbg !228
  br i1 %49, label %59, label %50, !dbg !229

50:                                               ; preds = %46, %43
  %51 = load i8, i8* @Long_only, align 1, !dbg !230
  %52 = trunc i8 %51 to i1, !dbg !230
  br i1 %52, label %53, label %57, !dbg !231

53:                                               ; preds = %50
  %54 = call i32 @fortlen() #9, !dbg !232
  %55 = load i32, i32* @SLEN, align 4, !dbg !233
  %56 = icmp sle i32 %54, %55, !dbg !234
  br label %57

57:                                               ; preds = %53, %50
  %58 = phi i1 [ false, %50 ], [ %56, %53 ], !dbg !235
  br label %59, !dbg !229

59:                                               ; preds = %57, %46
  %60 = phi i1 [ true, %46 ], [ %58, %57 ]
  br i1 %60, label %42, label %61, !dbg !223, !llvm.loop !236

61:                                               ; preds = %59
  %62 = load %struct.fd*, %struct.fd** @Fortfile, align 4, !dbg !239
  call void @display(%struct.fd* noundef %62) #9, !dbg !240
  %63 = load i8, i8* @Wait, align 1, !dbg !241
  %64 = trunc i8 %63 to i1, !dbg !241
  br i1 %64, label %65, label %71, !dbg !243

65:                                               ; preds = %61
  %66 = call i32 @fortlen() #9, !dbg !244
  %67 = load i32, i32* @Fort_len, align 4, !dbg !246
  %68 = sdiv i32 %67, 20, !dbg !247
  %69 = call i32 @mymax(i32 noundef %68, i32 noundef 6) #9, !dbg !248
  %70 = call i32 @sleep(i32 noundef %69) #9, !dbg !249
  br label %71, !dbg !250

71:                                               ; preds = %65, %61
  br label %72

72:                                               ; preds = %71, %39
  br label %73, !dbg !203

73:                                               ; preds = %72, %28
  call void @llvm.dbg.label(metadata !251), !dbg !252
  %74 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !253
  call void @free_desc(%struct.fd* noundef %74) #9, !dbg !254
  %75 = load i8*, i8** @Fortbuf, align 4, !dbg !255
  call void @free(i8* noundef %75) #9, !dbg !256
  %76 = load i32, i32* %6, align 4, !dbg !257
  call void @exit(i32 noundef %76) #10, !dbg !258
  unreachable, !dbg !258
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare i8* @getenv(i8* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal void @getargs(i32 noundef %0, i8** noundef %1) #0 !dbg !259 {
  %3 = alloca i32, align 4
  %4 = alloca i8**, align 4
  %5 = alloca i8, align 1
  %6 = alloca i8*, align 4
  %7 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  call void @llvm.dbg.declare(metadata i32* %3, metadata !262, metadata !DIExpression()), !dbg !263
  store i8** %1, i8*** %4, align 4
  call void @llvm.dbg.declare(metadata i8*** %4, metadata !264, metadata !DIExpression()), !dbg !265
  call void @llvm.dbg.declare(metadata i8* %5, metadata !266, metadata !DIExpression()), !dbg !267
  store i8 0, i8* %5, align 1, !dbg !267
  call void @llvm.dbg.declare(metadata i8** %6, metadata !268, metadata !DIExpression()), !dbg !269
  store i8* null, i8** %6, align 4, !dbg !269
  call void @llvm.dbg.declare(metadata i32* %7, metadata !270, metadata !DIExpression()), !dbg !271
  br label %8, !dbg !272

8:                                                ; preds = %35, %2
  %9 = load i32, i32* %3, align 4, !dbg !273
  %10 = load i8**, i8*** %4, align 4, !dbg !274
  %11 = call i32 @getopt(i32 noundef %9, i8** noundef %10, i8* noundef getelementptr inbounds ([17 x i8], [17 x i8]* @.str.5, i32 0, i32 0)) #9, !dbg !275
  store i32 %11, i32* %7, align 4, !dbg !276
  %12 = icmp ne i32 %11, -1, !dbg !277
  br i1 %12, label %13, label %36, !dbg !272

13:                                               ; preds = %8
  %14 = load i32, i32* %7, align 4, !dbg !278
  switch i32 %14, label %34 [
    i32 97, label %15
    i32 101, label %16
    i32 102, label %17
    i32 108, label %18
    i32 110, label %19
    i32 111, label %22
    i32 115, label %23
    i32 119, label %24
    i32 105, label %25
    i32 109, label %26
    i32 117, label %28
    i32 118, label %29
    i32 99, label %32
    i32 104, label %33
    i32 63, label %33
  ], !dbg !280

15:                                               ; preds = %13
  store i8 1, i8* @All_forts, align 1, !dbg !281
  br label %35, !dbg !283

16:                                               ; preds = %13
  store i8 1, i8* @Equal_probs, align 1, !dbg !284
  br label %35, !dbg !285

17:                                               ; preds = %13
  store i8 1, i8* @Find_files, align 1, !dbg !286
  br label %35, !dbg !287

18:                                               ; preds = %13
  store i8 1, i8* @Long_only, align 1, !dbg !288
  store i8 0, i8* @Short_only, align 1, !dbg !289
  br label %35, !dbg !290

19:                                               ; preds = %13
  %20 = load i8*, i8** @optarg, align 4, !dbg !291
  %21 = call i32 @atoi(i8* noundef %20) #9, !dbg !292
  store i32 %21, i32* @SLEN, align 4, !dbg !293
  br label %35, !dbg !294

22:                                               ; preds = %13
  store i8 1, i8* @Offend, align 1, !dbg !295
  br label %35, !dbg !296

23:                                               ; preds = %13
  store i8 1, i8* @Short_only, align 1, !dbg !297
  store i8 0, i8* @Long_only, align 1, !dbg !298
  br label %35, !dbg !299

24:                                               ; preds = %13
  store i8 1, i8* @Wait, align 1, !dbg !300
  br label %35, !dbg !301

25:                                               ; preds = %13
  store i8 1, i8* %5, align 1, !dbg !302
  br label %35, !dbg !303

26:                                               ; preds = %13
  store i8 1, i8* @Match, align 1, !dbg !304
  %27 = load i8*, i8** @optarg, align 4, !dbg !305
  store i8* %27, i8** %6, align 4, !dbg !306
  br label %35, !dbg !307

28:                                               ; preds = %13
  store i8 1, i8* @No_recode, align 1, !dbg !308
  br label %35, !dbg !309

29:                                               ; preds = %13
  %30 = call i8* @program_version() #9, !dbg !310
  %31 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.6, i32 0, i32 0), i8* noundef %30) #9, !dbg !311
  call void @exit(i32 noundef 0) #10, !dbg !312
  unreachable, !dbg !312

32:                                               ; preds = %13
  store i8 1, i8* @Show_filename, align 1, !dbg !313
  br label %35, !dbg !314

33:                                               ; preds = %13, %13
  br label %34, !dbg !315

34:                                               ; preds = %13, %33
  call void @usage() #10, !dbg !316
  unreachable, !dbg !316

35:                                               ; preds = %32, %28, %26, %25, %24, %23, %22, %19, %18, %17, %16, %15
  br label %8, !dbg !272, !llvm.loop !317

36:                                               ; preds = %8
  %37 = load i32, i32* @optind, align 4, !dbg !319
  %38 = load i32, i32* %3, align 4, !dbg !320
  %39 = sub nsw i32 %38, %37, !dbg !320
  store i32 %39, i32* %3, align 4, !dbg !320
  %40 = load i32, i32* @optind, align 4, !dbg !321
  %41 = load i8**, i8*** %4, align 4, !dbg !322
  %42 = getelementptr inbounds i8*, i8** %41, i32 %40, !dbg !322
  store i8** %42, i8*** %4, align 4, !dbg !322
  %43 = load i8**, i8*** %4, align 4, !dbg !323
  %44 = load i32, i32* %3, align 4, !dbg !325
  %45 = call i32 @form_file_list(i8** noundef %43, i32 noundef %44) #9, !dbg !326
  %46 = icmp ne i32 %45, 0, !dbg !326
  br i1 %46, label %54, label %47, !dbg !327

47:                                               ; preds = %36
  %48 = load i8, i8* @ErrorMessage, align 1, !dbg !328
  %49 = trunc i8 %48 to i1, !dbg !328
  br i1 %49, label %53, label %50, !dbg !331

50:                                               ; preds = %47
  %51 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !332
  %52 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %51, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([19 x i8], [19 x i8]* @.str.8, i32 0, i32 0)) #9, !dbg !334
  br label %53, !dbg !335

53:                                               ; preds = %50, %47
  call void @exit(i32 noundef 1) #10, !dbg !336
  unreachable, !dbg !336

54:                                               ; preds = %36
  %55 = load i8*, i8** %6, align 4, !dbg !337
  %56 = icmp ne i8* %55, null, !dbg !337
  br i1 %56, label %57, label %77, !dbg !339

57:                                               ; preds = %54
  %58 = load i8, i8* %5, align 1, !dbg !340
  %59 = trunc i8 %58 to i1, !dbg !340
  br i1 %59, label %60, label %63, !dbg !343

60:                                               ; preds = %57
  %61 = load i8*, i8** %6, align 4, !dbg !344
  %62 = call i8* @conv_pat(i8* noundef %61) #9, !dbg !346
  store i8* %62, i8** %6, align 4, !dbg !347
  br label %63, !dbg !348

63:                                               ; preds = %60, %57
  %64 = load i8*, i8** %6, align 4, !dbg !349
  %65 = call i32 @regcomp(%struct.re_pattern_buffer* noundef @Re_pat, i8* noundef %64, i32 noundef 8) #9, !dbg !349
  %66 = icmp ne i32 %65, 0, !dbg !349
  br i1 %66, label %67, label %71, !dbg !351

67:                                               ; preds = %63
  %68 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !352
  %69 = load i8*, i8** %6, align 4, !dbg !354
  %70 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %68, i8* noundef getelementptr inbounds ([17 x i8], [17 x i8]* @.str.9, i32 0, i32 0), i8* noundef %69) #9, !dbg !355
  call void @exit(i32 noundef 1) #10, !dbg !356
  unreachable, !dbg !356

71:                                               ; preds = %63
  %72 = load i8, i8* %5, align 1, !dbg !357
  %73 = trunc i8 %72 to i1, !dbg !357
  br i1 %73, label %74, label %76, !dbg !359

74:                                               ; preds = %71
  %75 = load i8*, i8** %6, align 4, !dbg !360
  call void @free(i8* noundef %75) #9, !dbg !362
  br label %76, !dbg !363

76:                                               ; preds = %74, %71
  br label %77, !dbg !364

77:                                               ; preds = %76, %54
  ret void, !dbg !365
}

declare i8* @setlocale(i32 noundef, i8* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal zeroext i1 @find_matches() #0 !dbg !366 {
  %1 = alloca i8, align 1
  %2 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !369
  %3 = call i32 @maxlen_in_list(%struct.fd* noundef %2) #9, !dbg !370
  store i32 %3, i32* @Fort_len, align 4, !dbg !371
  %4 = load i32, i32* @Fort_len, align 4, !dbg !372
  %5 = add i32 %4, 10, !dbg !373
  %6 = call i8* @do_malloc(i32 noundef %5) #9, !dbg !374
  store i8* %6, i8** @Fortbuf, align 4, !dbg !375
  call void @llvm.dbg.declare(metadata i8* %1, metadata !376, metadata !DIExpression()), !dbg !377
  store i8 0, i8* %1, align 1, !dbg !377
  %7 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !378
  call void @matches_in_list(%struct.fd* noundef %7, i8* noundef %1) #9, !dbg !379
  %8 = load i8, i8* %1, align 1, !dbg !380
  %9 = trunc i8 %8 to i1, !dbg !380
  ret i1 %9, !dbg !381
}

declare void @regfree(%struct.re_pattern_buffer* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal void @init_prob() #0 !dbg !382 {
  %1 = alloca %struct.fd*, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca %struct.fd*, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %1, metadata !385, metadata !DIExpression()), !dbg !386
  call void @llvm.dbg.declare(metadata i32* %2, metadata !387, metadata !DIExpression()), !dbg !388
  store i32 0, i32* %2, align 4, !dbg !388
  call void @llvm.dbg.declare(metadata i32* %3, metadata !389, metadata !DIExpression()), !dbg !390
  store i32 0, i32* %3, align 4, !dbg !390
  call void @llvm.dbg.declare(metadata i32* %4, metadata !391, metadata !DIExpression()), !dbg !392
  call void @llvm.dbg.declare(metadata %struct.fd** %5, metadata !393, metadata !DIExpression()), !dbg !394
  store %struct.fd* null, %struct.fd** %5, align 4, !dbg !394
  %6 = load %struct.fd*, %struct.fd** @File_tail, align 4, !dbg !395
  store %struct.fd* %6, %struct.fd** %1, align 4, !dbg !397
  br label %7, !dbg !398

7:                                                ; preds = %30, %0
  %8 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !399
  %9 = icmp ne %struct.fd* %8, null, !dbg !401
  br i1 %9, label %10, label %34, !dbg !401

10:                                               ; preds = %7
  %11 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !402
  %12 = getelementptr inbounds %struct.fd, %struct.fd* %11, i32 0, i32 0, !dbg !405
  %13 = load i32, i32* %12, align 4, !dbg !405
  %14 = icmp eq i32 %13, -1, !dbg !406
  br i1 %14, label %15, label %23, !dbg !407

15:                                               ; preds = %10
  %16 = load i32, i32* %3, align 4, !dbg !408
  %17 = add nsw i32 %16, 1, !dbg !408
  store i32 %17, i32* %3, align 4, !dbg !408
  %18 = load i8, i8* @Equal_probs, align 1, !dbg !410
  %19 = trunc i8 %18 to i1, !dbg !410
  br i1 %19, label %20, label %22, !dbg !412

20:                                               ; preds = %15
  %21 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !413
  store %struct.fd* %21, %struct.fd** %5, align 4, !dbg !415
  br label %22, !dbg !416

22:                                               ; preds = %20, %15
  br label %29, !dbg !417

23:                                               ; preds = %10
  %24 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !418
  %25 = getelementptr inbounds %struct.fd, %struct.fd* %24, i32 0, i32 0, !dbg !420
  %26 = load i32, i32* %25, align 4, !dbg !420
  %27 = load i32, i32* %2, align 4, !dbg !421
  %28 = add nsw i32 %27, %26, !dbg !421
  store i32 %28, i32* %2, align 4, !dbg !421
  br label %29

29:                                               ; preds = %23, %22
  br label %30, !dbg !422

30:                                               ; preds = %29
  %31 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !423
  %32 = getelementptr inbounds %struct.fd, %struct.fd* %31, i32 0, i32 17, !dbg !424
  %33 = load %struct.fd*, %struct.fd** %32, align 4, !dbg !424
  store %struct.fd* %33, %struct.fd** %1, align 4, !dbg !425
  br label %7, !dbg !426, !llvm.loop !427

34:                                               ; preds = %7
  %35 = load i32, i32* %2, align 4, !dbg !429
  %36 = icmp sgt i32 %35, 100, !dbg !431
  br i1 %36, label %37, label %41, !dbg !432

37:                                               ; preds = %34
  %38 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !433
  %39 = load i32, i32* %2, align 4, !dbg !435
  %40 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %38, i8* noundef getelementptr inbounds ([37 x i8], [37 x i8]* @.str.60, i32 0, i32 0), i32 noundef %39) #9, !dbg !436
  call void @exit(i32 noundef 1) #10, !dbg !437
  unreachable, !dbg !437

41:                                               ; preds = %34
  %42 = load i32, i32* %2, align 4, !dbg !438
  %43 = icmp slt i32 %42, 100, !dbg !440
  br i1 %43, label %44, label %51, !dbg !441

44:                                               ; preds = %41
  %45 = load i32, i32* %3, align 4, !dbg !442
  %46 = icmp eq i32 %45, 0, !dbg !443
  br i1 %46, label %47, label %51, !dbg !444

47:                                               ; preds = %44
  %48 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !445
  %49 = load i32, i32* %2, align 4, !dbg !447
  %50 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %48, i8* noundef getelementptr inbounds ([54 x i8], [54 x i8]* @.str.61, i32 0, i32 0), i32 noundef %49) #9, !dbg !448
  call void @exit(i32 noundef 1) #10, !dbg !449
  unreachable, !dbg !449

51:                                               ; preds = %44, %41
  %52 = load i32, i32* %2, align 4, !dbg !450
  %53 = icmp eq i32 %52, 100, !dbg !452
  br i1 %53, label %54, label %60, !dbg !453

54:                                               ; preds = %51
  %55 = load i32, i32* %3, align 4, !dbg !454
  %56 = icmp ne i32 %55, 0, !dbg !455
  br i1 %56, label %57, label %60, !dbg !456

57:                                               ; preds = %54
  %58 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !457
  %59 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %58, i8* noundef getelementptr inbounds ([55 x i8], [55 x i8]* @.str.62, i32 0, i32 0)) #9, !dbg !459
  call void @exit(i32 noundef 1) #10, !dbg !460
  unreachable, !dbg !460

60:                                               ; preds = %54, %51
  br label %61

61:                                               ; preds = %60
  br label %62

62:                                               ; preds = %61
  %63 = load i32, i32* %2, align 4, !dbg !461
  store i32 %63, i32* @Spec_prob, align 4, !dbg !462
  %64 = load i32, i32* %2, align 4, !dbg !463
  %65 = sub nsw i32 100, %64, !dbg !464
  store i32 %65, i32* %2, align 4, !dbg !465
  %66 = load i8, i8* @Equal_probs, align 1, !dbg !466
  %67 = trunc i8 %66 to i1, !dbg !466
  br i1 %67, label %68, label %107, !dbg !468

68:                                               ; preds = %62
  %69 = load i32, i32* %3, align 4, !dbg !469
  %70 = icmp ne i32 %69, 0, !dbg !472
  br i1 %70, label %71, label %105, !dbg !473

71:                                               ; preds = %68
  %72 = load i32, i32* %3, align 4, !dbg !474
  %73 = icmp sgt i32 %72, 1, !dbg !477
  br i1 %73, label %74, label %101, !dbg !478

74:                                               ; preds = %71
  %75 = load i32, i32* %2, align 4, !dbg !479
  %76 = load i32, i32* %3, align 4, !dbg !481
  %77 = sdiv i32 %75, %76, !dbg !482
  store i32 %77, i32* %4, align 4, !dbg !483
  %78 = load %struct.fd*, %struct.fd** @File_tail, align 4, !dbg !484
  store %struct.fd* %78, %struct.fd** %1, align 4, !dbg !486
  br label %79, !dbg !487

79:                                               ; preds = %96, %74
  %80 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !488
  %81 = load %struct.fd*, %struct.fd** %5, align 4, !dbg !490
  %82 = icmp ne %struct.fd* %80, %81, !dbg !491
  br i1 %82, label %83, label %100, !dbg !492

83:                                               ; preds = %79
  %84 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !493
  %85 = getelementptr inbounds %struct.fd, %struct.fd* %84, i32 0, i32 0, !dbg !496
  %86 = load i32, i32* %85, align 4, !dbg !496
  %87 = icmp eq i32 %86, -1, !dbg !497
  br i1 %87, label %88, label %95, !dbg !498

88:                                               ; preds = %83
  %89 = load i32, i32* %4, align 4, !dbg !499
  %90 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !501
  %91 = getelementptr inbounds %struct.fd, %struct.fd* %90, i32 0, i32 0, !dbg !502
  store i32 %89, i32* %91, align 4, !dbg !503
  %92 = load i32, i32* %4, align 4, !dbg !504
  %93 = load i32, i32* %2, align 4, !dbg !505
  %94 = sub nsw i32 %93, %92, !dbg !505
  store i32 %94, i32* %2, align 4, !dbg !505
  br label %95, !dbg !506

95:                                               ; preds = %88, %83
  br label %96, !dbg !507

96:                                               ; preds = %95
  %97 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !508
  %98 = getelementptr inbounds %struct.fd, %struct.fd* %97, i32 0, i32 17, !dbg !509
  %99 = load %struct.fd*, %struct.fd** %98, align 4, !dbg !509
  store %struct.fd* %99, %struct.fd** %1, align 4, !dbg !510
  br label %79, !dbg !511, !llvm.loop !512

100:                                              ; preds = %79
  br label %101, !dbg !514

101:                                              ; preds = %100, %71
  %102 = load i32, i32* %2, align 4, !dbg !515
  %103 = load %struct.fd*, %struct.fd** %5, align 4, !dbg !516
  %104 = getelementptr inbounds %struct.fd, %struct.fd* %103, i32 0, i32 0, !dbg !517
  store i32 %102, i32* %104, align 4, !dbg !518
  br label %106, !dbg !519

105:                                              ; preds = %68
  br label %106

106:                                              ; preds = %105, %101
  br label %107, !dbg !520

107:                                              ; preds = %106, %62
  ret void, !dbg !521
}

; Function Attrs: noinline nounwind optnone
define internal void @sum_noprobs(%struct.fd* noundef %0) #0 !dbg !124 {
  %2 = alloca %struct.fd*, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !522, metadata !DIExpression()), !dbg !523
  %3 = load i8, i8* @sum_noprobs.did_noprobs, align 1, !dbg !524
  %4 = trunc i8 %3 to i1, !dbg !524
  br i1 %4, label %5, label %6, !dbg !526

5:                                                ; preds = %1
  br label %24, !dbg !527

6:                                                ; preds = %1
  call void @zero_tbl(%struct.STRFILE* noundef @Noprob_tbl) #9, !dbg !529
  br label %7, !dbg !530

7:                                                ; preds = %19, %6
  %8 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !531
  %9 = icmp ne %struct.fd* %8, null, !dbg !530
  br i1 %9, label %10, label %23, !dbg !530

10:                                               ; preds = %7
  %11 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !532
  call void @get_tbl(%struct.fd* noundef %11) #9, !dbg !534
  %12 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !535
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 0, !dbg !537
  %14 = load i32, i32* %13, align 4, !dbg !537
  %15 = icmp eq i32 %14, -1, !dbg !538
  br i1 %15, label %16, label %19, !dbg !539

16:                                               ; preds = %10
  %17 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !540
  %18 = getelementptr inbounds %struct.fd, %struct.fd* %17, i32 0, i32 12, !dbg !542
  call void @sum_tbl(%struct.STRFILE* noundef @Noprob_tbl, %struct.STRFILE* noundef %18) #9, !dbg !543
  br label %19, !dbg !544

19:                                               ; preds = %16, %10
  %20 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !545
  %21 = getelementptr inbounds %struct.fd, %struct.fd* %20, i32 0, i32 16, !dbg !546
  %22 = load %struct.fd*, %struct.fd** %21, align 4, !dbg !546
  store %struct.fd* %22, %struct.fd** %2, align 4, !dbg !547
  br label %7, !dbg !530, !llvm.loop !548

23:                                               ; preds = %7
  store i8 1, i8* @sum_noprobs.did_noprobs, align 1, !dbg !550
  br label %24, !dbg !551

24:                                               ; preds = %23, %5
  ret void, !dbg !551
}

; Function Attrs: noinline nounwind optnone
define internal void @calc_equal_probs() #0 !dbg !552 {
  %1 = alloca %struct.fd*, align 4
  store i32 0, i32* @Num_kids, align 4, !dbg !553
  store i32 0, i32* @Num_files, align 4, !dbg !554
  call void @llvm.dbg.declare(metadata %struct.fd** %1, metadata !555, metadata !DIExpression()), !dbg !556
  %2 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !557
  store %struct.fd* %2, %struct.fd** %1, align 4, !dbg !556
  br label %3, !dbg !558

3:                                                ; preds = %6, %0
  %4 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !559
  %5 = icmp ne %struct.fd* %4, null, !dbg !558
  br i1 %5, label %6, label %17, !dbg !558

6:                                                ; preds = %3
  %7 = load i32, i32* @Num_files, align 4, !dbg !560
  %8 = add nsw i32 %7, 1, !dbg !560
  store i32 %8, i32* @Num_files, align 4, !dbg !560
  %9 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !562
  %10 = getelementptr inbounds %struct.fd, %struct.fd* %9, i32 0, i32 13, !dbg !563
  %11 = load i32, i32* %10, align 4, !dbg !563
  %12 = load i32, i32* @Num_kids, align 4, !dbg !564
  %13 = add nsw i32 %12, %11, !dbg !564
  store i32 %13, i32* @Num_kids, align 4, !dbg !564
  %14 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !565
  %15 = getelementptr inbounds %struct.fd, %struct.fd* %14, i32 0, i32 16, !dbg !566
  %16 = load %struct.fd*, %struct.fd** %15, align 4, !dbg !566
  store %struct.fd* %16, %struct.fd** %1, align 4, !dbg !567
  br label %3, !dbg !558, !llvm.loop !568

17:                                               ; preds = %3
  ret void, !dbg !570
}

; Function Attrs: noinline nounwind optnone
define internal void @print_list(%struct.fd* noundef %0, i32 noundef %1) #0 !dbg !571 {
  %3 = alloca %struct.fd*, align 4
  %4 = alloca i32, align 4
  store %struct.fd* %0, %struct.fd** %3, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %3, metadata !574, metadata !DIExpression()), !dbg !575
  store i32 %1, i32* %4, align 4
  call void @llvm.dbg.declare(metadata i32* %4, metadata !576, metadata !DIExpression()), !dbg !577
  br label %5, !dbg !578

5:                                                ; preds = %86, %2
  %6 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !579
  %7 = icmp ne %struct.fd* %6, null, !dbg !578
  br i1 %7, label %8, label %90, !dbg !578

8:                                                ; preds = %5
  %9 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !580
  %10 = load i32, i32* %4, align 4, !dbg !582
  %11 = mul nsw i32 %10, 4, !dbg !583
  %12 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %9, i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.63, i32 0, i32 0), i32 noundef %11, i8* noundef getelementptr inbounds ([1 x i8], [1 x i8]* @.str.4, i32 0, i32 0)) #9, !dbg !584
  %13 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !585
  %14 = getelementptr inbounds %struct.fd, %struct.fd* %13, i32 0, i32 0, !dbg !587
  %15 = load i32, i32* %14, align 4, !dbg !587
  %16 = icmp eq i32 %15, -1, !dbg !588
  br i1 %16, label %17, label %52, !dbg !589

17:                                               ; preds = %8
  %18 = load i8, i8* @Equal_probs, align 1, !dbg !590
  %19 = trunc i8 %18 to i1, !dbg !590
  br i1 %19, label %35, label %20, !dbg !593

20:                                               ; preds = %17
  %21 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !594
  %22 = load i32, i32* @Spec_prob, align 4, !dbg !596
  %23 = sitofp i32 %22 to double, !dbg !596
  %24 = fsub double 1.000000e+02, %23, !dbg !597
  %25 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !598
  %26 = getelementptr inbounds %struct.fd, %struct.fd* %25, i32 0, i32 12, !dbg !599
  %27 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %26, i32 0, i32 1, !dbg !600
  %28 = load i32, i32* %27, align 4, !dbg !600
  %29 = uitofp i32 %28 to double, !dbg !598
  %30 = fmul double %24, %29, !dbg !601
  %31 = load i32, i32* getelementptr inbounds (%struct.STRFILE, %struct.STRFILE* @Noprob_tbl, i32 0, i32 1), align 4, !dbg !602
  %32 = uitofp i32 %31 to double, !dbg !603
  %33 = fdiv double %30, %32, !dbg !604
  %34 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %21, i8* noundef getelementptr inbounds ([8 x i8], [8 x i8]* @.str.64, i32 0, i32 0), double noundef %33) #9, !dbg !605
  br label %51, !dbg !606

35:                                               ; preds = %17
  %36 = load i32, i32* %4, align 4, !dbg !607
  %37 = icmp eq i32 %36, 0, !dbg !609
  br i1 %37, label %38, label %44, !dbg !610

38:                                               ; preds = %35
  %39 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !611
  %40 = load i32, i32* @Num_files, align 4, !dbg !613
  %41 = sitofp i32 %40 to double, !dbg !613
  %42 = fdiv double 1.000000e+02, %41, !dbg !614
  %43 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %39, i8* noundef getelementptr inbounds ([8 x i8], [8 x i8]* @.str.64, i32 0, i32 0), double noundef %42) #9, !dbg !615
  br label %50, !dbg !616

44:                                               ; preds = %35
  %45 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !617
  %46 = load i32, i32* @Num_kids, align 4, !dbg !619
  %47 = sitofp i32 %46 to double, !dbg !619
  %48 = fdiv double 1.000000e+02, %47, !dbg !620
  %49 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %45, i8* noundef getelementptr inbounds ([8 x i8], [8 x i8]* @.str.64, i32 0, i32 0), double noundef %48) #9, !dbg !621
  br label %50

50:                                               ; preds = %44, %38
  br label %51

51:                                               ; preds = %50, %20
  br label %60, !dbg !622

52:                                               ; preds = %8
  %53 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !623
  %54 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !625
  %55 = getelementptr inbounds %struct.fd, %struct.fd* %54, i32 0, i32 0, !dbg !626
  %56 = load i32, i32* %55, align 4, !dbg !626
  %57 = sitofp i32 %56 to double, !dbg !625
  %58 = fmul double 1.000000e+00, %57, !dbg !627
  %59 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %53, i8* noundef getelementptr inbounds ([8 x i8], [8 x i8]* @.str.64, i32 0, i32 0), double noundef %58) #9, !dbg !628
  br label %60

60:                                               ; preds = %52, %51
  %61 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !629
  %62 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !630
  %63 = getelementptr inbounds %struct.fd, %struct.fd* %62, i32 0, i32 5, !dbg !630
  %64 = load i8*, i8** %63, align 4, !dbg !630
  %65 = icmp ne i8* %64, null, !dbg !630
  br i1 %65, label %67, label %66, !dbg !630

66:                                               ; preds = %60
  br label %71, !dbg !630

67:                                               ; preds = %60
  %68 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !630
  %69 = getelementptr inbounds %struct.fd, %struct.fd* %68, i32 0, i32 5, !dbg !630
  %70 = load i8*, i8** %69, align 4, !dbg !630
  br label %71, !dbg !630

71:                                               ; preds = %67, %66
  %72 = phi i8* [ getelementptr inbounds ([5 x i8], [5 x i8]* @.str.66, i32 0, i32 0), %66 ], [ %70, %67 ], !dbg !630
  %73 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %61, i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.65, i32 0, i32 0), i8* noundef %72) #9, !dbg !631
  %74 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !632
  %75 = call i32 @putc(i32 noundef 10, %struct._IO_FILE* noundef %74) #9, !dbg !633
  %76 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !634
  %77 = getelementptr inbounds %struct.fd, %struct.fd* %76, i32 0, i32 14, !dbg !636
  %78 = load %struct.fd*, %struct.fd** %77, align 4, !dbg !636
  %79 = icmp ne %struct.fd* %78, null, !dbg !634
  br i1 %79, label %80, label %86, !dbg !637

80:                                               ; preds = %71
  %81 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !638
  %82 = getelementptr inbounds %struct.fd, %struct.fd* %81, i32 0, i32 14, !dbg !640
  %83 = load %struct.fd*, %struct.fd** %82, align 4, !dbg !640
  %84 = load i32, i32* %4, align 4, !dbg !641
  %85 = add nsw i32 %84, 1, !dbg !642
  call void @print_list(%struct.fd* noundef %83, i32 noundef %85) #9, !dbg !643
  br label %86, !dbg !644

86:                                               ; preds = %80, %71
  %87 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !645
  %88 = getelementptr inbounds %struct.fd, %struct.fd* %87, i32 0, i32 16, !dbg !646
  %89 = load %struct.fd*, %struct.fd** %88, align 4, !dbg !646
  store %struct.fd* %89, %struct.fd** %3, align 4, !dbg !647
  br label %5, !dbg !578, !llvm.loop !648

90:                                               ; preds = %5
  ret void, !dbg !650
}

; Function Attrs: noinline nounwind optnone
define internal void @call_srandom() #0 !dbg !651 {
  %1 = call i64 @time(i64* noundef null) #9, !dbg !653
  %2 = call i32 @getpid() #9, !dbg !654
  %3 = sext i32 %2 to i64, !dbg !654
  %4 = add nsw i64 %1, %3, !dbg !655
  %5 = trunc i64 %4 to i32, !dbg !656
  call void @srandom(i32 noundef %5) #9, !dbg !657
  ret void, !dbg !658
}

; Function Attrs: noinline nounwind optnone
define internal void @get_fort() #0 !dbg !659 {
  %1 = alloca %struct.fd*, align 4
  %2 = alloca i32, align 4
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %6 = alloca i64, align 8
  call void @llvm.dbg.declare(metadata %struct.fd** %1, metadata !660, metadata !DIExpression()), !dbg !661
  call void @llvm.dbg.declare(metadata i32* %2, metadata !662, metadata !DIExpression()), !dbg !663
  %7 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !664
  %8 = getelementptr inbounds %struct.fd, %struct.fd* %7, i32 0, i32 16, !dbg !666
  %9 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !666
  %10 = icmp ne %struct.fd* %9, null, !dbg !664
  br i1 %10, label %11, label %16, !dbg !667

11:                                               ; preds = %0
  %12 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !668
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 0, !dbg !669
  %14 = load i32, i32* %13, align 4, !dbg !669
  %15 = icmp eq i32 %14, -1, !dbg !670
  br i1 %15, label %16, label %18, !dbg !671

16:                                               ; preds = %11, %0
  %17 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !672
  store %struct.fd* %17, %struct.fd** %1, align 4, !dbg !674
  br label %45, !dbg !675

18:                                               ; preds = %11
  %19 = call i32 @my_random(i32 noundef 100) #9, !dbg !676
  store i32 %19, i32* %2, align 4, !dbg !678
  %20 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !679
  store %struct.fd* %20, %struct.fd** %1, align 4, !dbg !681
  br label %21, !dbg !682

21:                                               ; preds = %40, %18
  %22 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !683
  %23 = getelementptr inbounds %struct.fd, %struct.fd* %22, i32 0, i32 0, !dbg !685
  %24 = load i32, i32* %23, align 4, !dbg !685
  %25 = icmp ne i32 %24, -1, !dbg !686
  br i1 %25, label %26, label %44, !dbg !687

26:                                               ; preds = %21
  %27 = load i32, i32* %2, align 4, !dbg !688
  %28 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !691
  %29 = getelementptr inbounds %struct.fd, %struct.fd* %28, i32 0, i32 0, !dbg !692
  %30 = load i32, i32* %29, align 4, !dbg !692
  %31 = icmp slt i32 %27, %30, !dbg !693
  br i1 %31, label %32, label %33, !dbg !694

32:                                               ; preds = %26
  br label %44, !dbg !695

33:                                               ; preds = %26
  %34 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !697
  %35 = getelementptr inbounds %struct.fd, %struct.fd* %34, i32 0, i32 0, !dbg !699
  %36 = load i32, i32* %35, align 4, !dbg !699
  %37 = load i32, i32* %2, align 4, !dbg !700
  %38 = sub nsw i32 %37, %36, !dbg !700
  store i32 %38, i32* %2, align 4, !dbg !700
  br label %39

39:                                               ; preds = %33
  br label %40, !dbg !701

40:                                               ; preds = %39
  %41 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !702
  %42 = getelementptr inbounds %struct.fd, %struct.fd* %41, i32 0, i32 16, !dbg !703
  %43 = load %struct.fd*, %struct.fd** %42, align 4, !dbg !703
  store %struct.fd* %43, %struct.fd** %1, align 4, !dbg !704
  br label %21, !dbg !705, !llvm.loop !706

44:                                               ; preds = %32, %21
  br label %45

45:                                               ; preds = %44, %16
  %46 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !708
  %47 = getelementptr inbounds %struct.fd, %struct.fd* %46, i32 0, i32 0, !dbg !710
  %48 = load i32, i32* %47, align 4, !dbg !710
  %49 = icmp ne i32 %48, -1, !dbg !711
  br i1 %49, label %50, label %52, !dbg !712

50:                                               ; preds = %45
  %51 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !713
  call void @get_tbl(%struct.fd* noundef %51) #9, !dbg !715
  br label %81, !dbg !716

52:                                               ; preds = %45
  %53 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !717
  %54 = getelementptr inbounds %struct.fd, %struct.fd* %53, i32 0, i32 16, !dbg !720
  %55 = load %struct.fd*, %struct.fd** %54, align 4, !dbg !720
  %56 = icmp ne %struct.fd* %55, null, !dbg !717
  br i1 %56, label %57, label %79, !dbg !721

57:                                               ; preds = %52
  %58 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !722
  call void @sum_noprobs(%struct.fd* noundef %58) #9, !dbg !724
  %59 = load i32, i32* getelementptr inbounds (%struct.STRFILE, %struct.STRFILE* @Noprob_tbl, i32 0, i32 1), align 4, !dbg !725
  %60 = call i32 @my_random(i32 noundef %59) #9, !dbg !726
  store i32 %60, i32* %2, align 4, !dbg !727
  br label %61, !dbg !728

61:                                               ; preds = %68, %57
  %62 = load i32, i32* %2, align 4, !dbg !729
  %63 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !730
  %64 = getelementptr inbounds %struct.fd, %struct.fd* %63, i32 0, i32 12, !dbg !731
  %65 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %64, i32 0, i32 1, !dbg !732
  %66 = load i32, i32* %65, align 4, !dbg !732
  %67 = icmp sge i32 %62, %66, !dbg !733
  br i1 %67, label %68, label %78, !dbg !728

68:                                               ; preds = %61
  %69 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !734
  %70 = getelementptr inbounds %struct.fd, %struct.fd* %69, i32 0, i32 12, !dbg !736
  %71 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %70, i32 0, i32 1, !dbg !737
  %72 = load i32, i32* %71, align 4, !dbg !737
  %73 = load i32, i32* %2, align 4, !dbg !738
  %74 = sub nsw i32 %73, %72, !dbg !738
  store i32 %74, i32* %2, align 4, !dbg !738
  %75 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !739
  %76 = getelementptr inbounds %struct.fd, %struct.fd* %75, i32 0, i32 16, !dbg !740
  %77 = load %struct.fd*, %struct.fd** %76, align 4, !dbg !740
  store %struct.fd* %77, %struct.fd** %1, align 4, !dbg !741
  br label %61, !dbg !728, !llvm.loop !742

78:                                               ; preds = %61
  br label %79, !dbg !744

79:                                               ; preds = %78, %52
  %80 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !745
  call void @get_tbl(%struct.fd* noundef %80) #9, !dbg !746
  br label %81

81:                                               ; preds = %79, %50
  %82 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !747
  %83 = getelementptr inbounds %struct.fd, %struct.fd* %82, i32 0, i32 12, !dbg !749
  %84 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %83, i32 0, i32 1, !dbg !750
  %85 = load i32, i32* %84, align 4, !dbg !750
  %86 = icmp eq i32 %85, 0, !dbg !751
  br i1 %86, label %87, label %90, !dbg !752

87:                                               ; preds = %81
  %88 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !753
  %89 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %88, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([27 x i8], [27 x i8]* @.str.67, i32 0, i32 0)) #9, !dbg !755
  call void @exit(i32 noundef 1) #10, !dbg !756
  unreachable, !dbg !756

90:                                               ; preds = %81
  %91 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !757
  %92 = getelementptr inbounds %struct.fd, %struct.fd* %91, i32 0, i32 14, !dbg !759
  %93 = load %struct.fd*, %struct.fd** %92, align 4, !dbg !759
  %94 = icmp ne %struct.fd* %93, null, !dbg !757
  br i1 %94, label %95, label %107, !dbg !760

95:                                               ; preds = %90
  %96 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !761
  %97 = call %struct.fd* @pick_child(%struct.fd* noundef %96) #9, !dbg !763
  store %struct.fd* %97, %struct.fd** %1, align 4, !dbg !764
  %98 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !765
  %99 = getelementptr inbounds %struct.fd, %struct.fd* %98, i32 0, i32 14, !dbg !765
  %100 = load %struct.fd*, %struct.fd** %99, align 4, !dbg !765
  %101 = icmp ne %struct.fd* %100, null, !dbg !765
  br i1 %101, label %102, label %104, !dbg !765

102:                                              ; preds = %95
  call void @__assert_fail(i8* noundef getelementptr inbounds ([11 x i8], [11 x i8]* @.str.68, i32 0, i32 0), i8* noundef getelementptr inbounds ([72 x i8], [72 x i8]* @.str.69, i32 0, i32 0), i32 noundef 1700, i8* noundef getelementptr inbounds ([9 x i8], [9 x i8]* @__func__.get_fort, i32 0, i32 0)) #10, !dbg !765
  unreachable, !dbg !765

103:                                              ; No predecessors!
  br label %104, !dbg !765

104:                                              ; preds = %103, %95
  %105 = phi i1 [ true, %95 ], [ false, %103 ]
  %106 = zext i1 %105 to i32, !dbg !765
  br label %107, !dbg !766

107:                                              ; preds = %104, %90
  %108 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !767
  %109 = getelementptr inbounds %struct.fd, %struct.fd* %108, i32 0, i32 14, !dbg !767
  %110 = load %struct.fd*, %struct.fd** %109, align 4, !dbg !767
  %111 = icmp ne %struct.fd* %110, null, !dbg !767
  br i1 %111, label %112, label %114, !dbg !767

112:                                              ; preds = %107
  call void @__assert_fail(i8* noundef getelementptr inbounds ([11 x i8], [11 x i8]* @.str.68, i32 0, i32 0), i8* noundef getelementptr inbounds ([72 x i8], [72 x i8]* @.str.69, i32 0, i32 0), i32 noundef 1702, i8* noundef getelementptr inbounds ([9 x i8], [9 x i8]* @__func__.get_fort, i32 0, i32 0)) #10, !dbg !767
  unreachable, !dbg !767

113:                                              ; No predecessors!
  br label %114, !dbg !767

114:                                              ; preds = %113, %107
  %115 = phi i1 [ true, %107 ], [ false, %113 ]
  %116 = zext i1 %115 to i32, !dbg !767
  %117 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !768
  store %struct.fd* %117, %struct.fd** @Fortfile, align 4, !dbg !769
  %118 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !770
  call void @get_pos(%struct.fd* noundef %118) #9, !dbg !771
  %119 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !772
  call void @open_dat(%struct.fd* noundef %119) #9, !dbg !773
  call void @llvm.dbg.declare(metadata i64* %3, metadata !774, metadata !DIExpression()), !dbg !776
  %120 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !776
  %121 = getelementptr inbounds %struct.fd, %struct.fd* %120, i32 0, i32 2, !dbg !776
  %122 = load i32, i32* %121, align 4, !dbg !776
  %123 = sext i32 %122 to i64, !dbg !776
  store i64 %123, i64* %3, align 8, !dbg !776
  call void @llvm.dbg.declare(metadata i64* %4, metadata !777, metadata !DIExpression()), !dbg !776
  %124 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !776
  %125 = getelementptr inbounds %struct.fd, %struct.fd* %124, i32 0, i32 3, !dbg !776
  %126 = load i32, i32* %125, align 4, !dbg !776
  %127 = mul i32 %126, 4, !dbg !776
  %128 = add i32 24, %127, !dbg !776
  %129 = zext i32 %128 to i64, !dbg !776
  store i64 %129, i64* %4, align 8, !dbg !776
  call void @llvm.dbg.declare(metadata i64* %5, metadata !778, metadata !DIExpression()), !dbg !776
  store i64 0, i64* %5, align 8, !dbg !776
  %130 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !776
  %131 = getelementptr inbounds %struct.fd, %struct.fd* %130, i32 0, i32 3, !dbg !776
  %132 = load i32, i32* %131, align 4, !dbg !776
  %133 = mul i32 %132, 4, !dbg !776
  %134 = add i32 24, %133, !dbg !776
  %135 = zext i32 %134 to i64, !dbg !776
  %136 = call i1 @llvm.is.constant.i64(i64 %135), !dbg !776
  br i1 %136, label %137, label %147, !dbg !776

137:                                              ; preds = %114
  %138 = load i64, i64* %4, align 8, !dbg !776
  %139 = icmp eq i64 %138, 0, !dbg !776
  br i1 %139, label %140, label %147, !dbg !776

140:                                              ; preds = %137
  %141 = load i64, i64* %5, align 8, !dbg !776
  %142 = icmp eq i64 %141, 1, !dbg !776
  br i1 %142, label %143, label %147, !dbg !776

143:                                              ; preds = %140
  %144 = load i64, i64* %3, align 8, !dbg !776
  %145 = trunc i64 %144 to i32, !dbg !776
  %146 = call i64 @__wasilibc_tell(i32 noundef %145) #9, !dbg !776
  br label %154, !dbg !776

147:                                              ; preds = %140, %137, %114
  %148 = load i64, i64* %3, align 8, !dbg !776
  %149 = trunc i64 %148 to i32, !dbg !776
  %150 = load i64, i64* %4, align 8, !dbg !776
  %151 = load i64, i64* %5, align 8, !dbg !776
  %152 = trunc i64 %151 to i32, !dbg !776
  %153 = call i64 @lseek(i32 noundef %149, i64 noundef %150, i32 noundef %152) #9, !dbg !776
  br label %154, !dbg !776

154:                                              ; preds = %147, %143
  %155 = phi i64 [ %146, %143 ], [ %153, %147 ], !dbg !776
  store i64 %155, i64* %6, align 8, !dbg !776
  %156 = load i64, i64* %6, align 8, !dbg !776
  %157 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !779
  %158 = getelementptr inbounds %struct.fd, %struct.fd* %157, i32 0, i32 2, !dbg !781
  %159 = load i32, i32* %158, align 4, !dbg !781
  %160 = call i32 @read(i32 noundef %159, i8* noundef bitcast ([2 x i32]* @Seekpts to i8*), i32 noundef 4) #9, !dbg !782
  %161 = icmp slt i32 %160, 0, !dbg !783
  br i1 %161, label %168, label %162, !dbg !784

162:                                              ; preds = %154
  %163 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !785
  %164 = getelementptr inbounds %struct.fd, %struct.fd* %163, i32 0, i32 2, !dbg !786
  %165 = load i32, i32* %164, align 4, !dbg !786
  %166 = call i32 @read(i32 noundef %165, i8* noundef bitcast (i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 1) to i8*), i32 noundef 4) #9, !dbg !787
  %167 = icmp slt i32 %166, 0, !dbg !788
  br i1 %167, label %168, label %169, !dbg !789

168:                                              ; preds = %162, %154
  call void @exit(i32 noundef 1) #10, !dbg !790
  unreachable, !dbg !790

169:                                              ; preds = %162
  %170 = load i32, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 0), align 4, !dbg !792
  %171 = call i32 @ntohl(i32 noundef %170) #9, !dbg !793
  store i32 %171, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 0), align 4, !dbg !794
  %172 = load i32, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 1), align 4, !dbg !795
  %173 = call i32 @ntohl(i32 noundef %172) #9, !dbg !796
  store i32 %173, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 1), align 4, !dbg !797
  ret void, !dbg !798
}

; Function Attrs: noinline nounwind optnone
define internal i32 @fortlen() #0 !dbg !799 {
  %1 = alloca i32, align 4
  %2 = alloca [1024 x i8], align 16
  call void @llvm.dbg.declare(metadata i32* %1, metadata !802, metadata !DIExpression()), !dbg !803
  call void @llvm.dbg.declare(metadata [1024 x i8]* %2, metadata !804, metadata !DIExpression()), !dbg !805
  %3 = load %struct.fd*, %struct.fd** @Fortfile, align 4, !dbg !806
  %4 = getelementptr inbounds %struct.fd, %struct.fd* %3, i32 0, i32 12, !dbg !808
  %5 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %4, i32 0, i32 4, !dbg !809
  %6 = load i32, i32* %5, align 4, !dbg !809
  %7 = and i32 %6, 3, !dbg !810
  %8 = icmp ne i32 %7, 0, !dbg !810
  br i1 %8, label %14, label %9, !dbg !811

9:                                                ; preds = %0
  %10 = load i32, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 1), align 4, !dbg !812
  %11 = load i32, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 0), align 4, !dbg !814
  %12 = sub nsw i32 %10, %11, !dbg !815
  %13 = sub nsw i32 %12, 2, !dbg !816
  store i32 %13, i32* %1, align 4, !dbg !817
  br label %55, !dbg !818

14:                                               ; preds = %0
  %15 = load %struct.fd*, %struct.fd** @Fortfile, align 4, !dbg !819
  call void @open_fp(%struct.fd* noundef %15) #9, !dbg !821
  %16 = load %struct.fd*, %struct.fd** @Fortfile, align 4, !dbg !822
  %17 = getelementptr inbounds %struct.fd, %struct.fd* %16, i32 0, i32 4, !dbg !823
  %18 = load %struct._IO_FILE*, %struct._IO_FILE** %17, align 4, !dbg !823
  %19 = load i32, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 0), align 4, !dbg !824
  %20 = call i32 @fseek(%struct._IO_FILE* noundef %18, i32 noundef %19, i32 noundef 0) #9, !dbg !825
  store i32 0, i32* %1, align 4, !dbg !826
  br label %21, !dbg !827

21:                                               ; preds = %49, %14
  %22 = getelementptr inbounds [1024 x i8], [1024 x i8]* %2, i32 0, i32 0, !dbg !828
  %23 = load %struct.fd*, %struct.fd** @Fortfile, align 4, !dbg !829
  %24 = getelementptr inbounds %struct.fd, %struct.fd* %23, i32 0, i32 4, !dbg !830
  %25 = load %struct._IO_FILE*, %struct._IO_FILE** %24, align 4, !dbg !830
  %26 = call i8* @fgets(i8* noundef %22, i32 noundef 1024, %struct._IO_FILE* noundef %25) #9, !dbg !831
  %27 = icmp ne i8* %26, null, !dbg !831
  br i1 %27, label %28, label %47, !dbg !832

28:                                               ; preds = %21
  %29 = getelementptr inbounds [1024 x i8], [1024 x i8]* %2, i32 0, i32 0, !dbg !833
  %30 = load i8, i8* %29, align 16, !dbg !833
  %31 = sext i8 %30 to i32, !dbg !833
  %32 = load %struct.fd*, %struct.fd** @Fortfile, align 4, !dbg !833
  %33 = getelementptr inbounds %struct.fd, %struct.fd* %32, i32 0, i32 12, !dbg !833
  %34 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %33, i32 0, i32 5, !dbg !833
  %35 = getelementptr inbounds [4 x i8], [4 x i8]* %34, i32 0, i32 0, !dbg !833
  %36 = load i8, i8* %35, align 4, !dbg !833
  %37 = zext i8 %36 to i32, !dbg !833
  %38 = icmp eq i32 %31, %37, !dbg !833
  br i1 %38, label %39, label %44, !dbg !833

39:                                               ; preds = %28
  %40 = getelementptr inbounds [1024 x i8], [1024 x i8]* %2, i32 0, i32 1, !dbg !833
  %41 = load i8, i8* %40, align 1, !dbg !833
  %42 = sext i8 %41 to i32, !dbg !833
  %43 = icmp eq i32 %42, 10, !dbg !833
  br label %44

44:                                               ; preds = %39, %28
  %45 = phi i1 [ false, %28 ], [ %43, %39 ], !dbg !834
  %46 = xor i1 %45, true, !dbg !835
  br label %47

47:                                               ; preds = %44, %21
  %48 = phi i1 [ false, %21 ], [ %46, %44 ], !dbg !834
  br i1 %48, label %49, label %54, !dbg !827

49:                                               ; preds = %47
  %50 = getelementptr inbounds [1024 x i8], [1024 x i8]* %2, i32 0, i32 0, !dbg !836
  %51 = call i32 @strlen(i8* noundef %50) #11, !dbg !838
  %52 = load i32, i32* %1, align 4, !dbg !839
  %53 = add i32 %52, %51, !dbg !839
  store i32 %53, i32* %1, align 4, !dbg !839
  br label %21, !dbg !827, !llvm.loop !840

54:                                               ; preds = %47
  br label %55

55:                                               ; preds = %54, %9
  %56 = load i32, i32* %1, align 4, !dbg !842
  store i32 %56, i32* @Fort_len, align 4, !dbg !843
  %57 = load i32, i32* %1, align 4, !dbg !844
  ret i32 %57, !dbg !845
}

; Function Attrs: noinline nounwind optnone
define internal void @display(%struct.fd* noundef %0) #0 !dbg !846 {
  %2 = alloca %struct.fd*, align 4
  %3 = alloca i8*, align 4
  %4 = alloca i8, align 1
  %5 = alloca [1024 x i8], align 16
  %6 = alloca i8*, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !847, metadata !DIExpression()), !dbg !848
  call void @llvm.dbg.declare(metadata i8** %3, metadata !849, metadata !DIExpression()), !dbg !850
  call void @llvm.dbg.declare(metadata i8* %4, metadata !851, metadata !DIExpression()), !dbg !852
  call void @llvm.dbg.declare(metadata [1024 x i8]* %5, metadata !853, metadata !DIExpression()), !dbg !855
  %7 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !856
  call void @open_fp(%struct.fd* noundef %7) #9, !dbg !857
  %8 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !858
  %9 = getelementptr inbounds %struct.fd, %struct.fd* %8, i32 0, i32 4, !dbg !859
  %10 = load %struct._IO_FILE*, %struct._IO_FILE** %9, align 4, !dbg !859
  %11 = load i32, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @Seekpts, i32 0, i32 0), align 4, !dbg !860
  %12 = call i32 @fseek(%struct._IO_FILE* noundef %10, i32 noundef %11, i32 noundef 0) #9, !dbg !861
  %13 = load i8, i8* @Show_filename, align 1, !dbg !862
  %14 = trunc i8 %13 to i1, !dbg !862
  br i1 %14, label %15, label %20, !dbg !864

15:                                               ; preds = %1
  %16 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !865
  %17 = getelementptr inbounds %struct.fd, %struct.fd* %16, i32 0, i32 5, !dbg !867
  %18 = load i8*, i8** %17, align 4, !dbg !867
  %19 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([9 x i8], [9 x i8]* @.str.75, i32 0, i32 0), i8* noundef %18) #9, !dbg !868
  br label %20, !dbg !869

20:                                               ; preds = %15, %1
  store i32 0, i32* @Fort_len, align 4, !dbg !870
  br label %21, !dbg !872

21:                                               ; preds = %148, %20
  %22 = getelementptr inbounds [1024 x i8], [1024 x i8]* %5, i32 0, i32 0, !dbg !873
  %23 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !875
  %24 = getelementptr inbounds %struct.fd, %struct.fd* %23, i32 0, i32 4, !dbg !876
  %25 = load %struct._IO_FILE*, %struct._IO_FILE** %24, align 4, !dbg !876
  %26 = call i8* @fgets(i8* noundef %22, i32 noundef 1024, %struct._IO_FILE* noundef %25) #9, !dbg !877
  %27 = icmp ne i8* %26, null, !dbg !877
  br i1 %27, label %28, label %47, !dbg !878

28:                                               ; preds = %21
  %29 = getelementptr inbounds [1024 x i8], [1024 x i8]* %5, i32 0, i32 0, !dbg !879
  %30 = load i8, i8* %29, align 16, !dbg !879
  %31 = zext i8 %30 to i32, !dbg !879
  %32 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !879
  %33 = getelementptr inbounds %struct.fd, %struct.fd* %32, i32 0, i32 12, !dbg !879
  %34 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %33, i32 0, i32 5, !dbg !879
  %35 = getelementptr inbounds [4 x i8], [4 x i8]* %34, i32 0, i32 0, !dbg !879
  %36 = load i8, i8* %35, align 4, !dbg !879
  %37 = zext i8 %36 to i32, !dbg !879
  %38 = icmp eq i32 %31, %37, !dbg !879
  br i1 %38, label %39, label %44, !dbg !879

39:                                               ; preds = %28
  %40 = getelementptr inbounds [1024 x i8], [1024 x i8]* %5, i32 0, i32 1, !dbg !879
  %41 = load i8, i8* %40, align 1, !dbg !879
  %42 = zext i8 %41 to i32, !dbg !879
  %43 = icmp eq i32 %42, 10, !dbg !879
  br label %44

44:                                               ; preds = %39, %28
  %45 = phi i1 [ false, %28 ], [ %43, %39 ], !dbg !880
  %46 = xor i1 %45, true, !dbg !881
  br label %47

47:                                               ; preds = %44, %21
  %48 = phi i1 [ false, %21 ], [ %46, %44 ], !dbg !880
  br i1 %48, label %49, label %151, !dbg !882

49:                                               ; preds = %47
  %50 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !883
  %51 = getelementptr inbounds %struct.fd, %struct.fd* %50, i32 0, i32 12, !dbg !886
  %52 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %51, i32 0, i32 4, !dbg !887
  %53 = load i32, i32* %52, align 4, !dbg !887
  %54 = and i32 %53, 4, !dbg !888
  %55 = icmp ne i32 %54, 0, !dbg !888
  br i1 %55, label %56, label %128, !dbg !889

56:                                               ; preds = %49
  %57 = getelementptr inbounds [1024 x i8], [1024 x i8]* %5, i32 0, i32 0, !dbg !890
  store i8* %57, i8** %3, align 4, !dbg !893
  br label %58, !dbg !894

58:                                               ; preds = %124, %56
  %59 = load i8*, i8** %3, align 4, !dbg !895
  %60 = load i8, i8* %59, align 1, !dbg !897
  store i8 %60, i8* %4, align 1, !dbg !898
  %61 = icmp ne i8 %60, 0, !dbg !899
  br i1 %61, label %62, label %127, !dbg !899

62:                                               ; preds = %58
  br i1 false, label %63, label %68, !dbg !900

63:                                               ; preds = %62
  %64 = load i8, i8* %4, align 1, !dbg !903
  %65 = sext i8 %64 to i32, !dbg !903
  %66 = call i32 @isupper(i32 noundef %65) #9, !dbg !903
  %67 = icmp ne i32 %66, 0, !dbg !903
  br i1 %67, label %73, label %92, !dbg !903

68:                                               ; preds = %62
  %69 = load i8, i8* %4, align 1, !dbg !903
  %70 = sext i8 %69 to i32, !dbg !903
  %71 = sub i32 %70, 65, !dbg !903
  %72 = icmp ult i32 %71, 26, !dbg !903
  br i1 %72, label %73, label %92, !dbg !900

73:                                               ; preds = %68, %63
  br i1 false, label %74, label %79, !dbg !904

74:                                               ; preds = %73
  %75 = load i8, i8* %4, align 1, !dbg !905
  %76 = sext i8 %75 to i32, !dbg !905
  %77 = call i32 @isascii(i32 noundef %76) #9, !dbg !905
  %78 = icmp ne i32 %77, 0, !dbg !905
  br i1 %78, label %83, label %92, !dbg !905

79:                                               ; preds = %73
  %80 = load i8, i8* %4, align 1, !dbg !905
  %81 = sext i8 %80 to i32, !dbg !905
  %82 = icmp ult i32 %81, 128, !dbg !905
  br i1 %82, label %83, label %92, !dbg !904

83:                                               ; preds = %79, %74
  %84 = load i8, i8* %4, align 1, !dbg !906
  %85 = sext i8 %84 to i32, !dbg !906
  %86 = sub nsw i32 %85, 65, !dbg !908
  %87 = add nsw i32 %86, 13, !dbg !909
  %88 = srem i32 %87, 26, !dbg !910
  %89 = add nsw i32 65, %88, !dbg !911
  %90 = trunc i32 %89 to i8, !dbg !912
  %91 = load i8*, i8** %3, align 4, !dbg !913
  store i8 %90, i8* %91, align 1, !dbg !914
  br label %123, !dbg !915

92:                                               ; preds = %79, %74, %68, %63
  br i1 false, label %93, label %98, !dbg !916

93:                                               ; preds = %92
  %94 = load i8, i8* %4, align 1, !dbg !918
  %95 = sext i8 %94 to i32, !dbg !918
  %96 = call i32 @islower(i32 noundef %95) #9, !dbg !918
  %97 = icmp ne i32 %96, 0, !dbg !918
  br i1 %97, label %103, label %122, !dbg !918

98:                                               ; preds = %92
  %99 = load i8, i8* %4, align 1, !dbg !918
  %100 = sext i8 %99 to i32, !dbg !918
  %101 = sub i32 %100, 97, !dbg !918
  %102 = icmp ult i32 %101, 26, !dbg !918
  br i1 %102, label %103, label %122, !dbg !916

103:                                              ; preds = %98, %93
  br i1 false, label %104, label %109, !dbg !919

104:                                              ; preds = %103
  %105 = load i8, i8* %4, align 1, !dbg !920
  %106 = sext i8 %105 to i32, !dbg !920
  %107 = call i32 @isascii(i32 noundef %106) #9, !dbg !920
  %108 = icmp ne i32 %107, 0, !dbg !920
  br i1 %108, label %113, label %122, !dbg !920

109:                                              ; preds = %103
  %110 = load i8, i8* %4, align 1, !dbg !920
  %111 = sext i8 %110 to i32, !dbg !920
  %112 = icmp ult i32 %111, 128, !dbg !920
  br i1 %112, label %113, label %122, !dbg !919

113:                                              ; preds = %109, %104
  %114 = load i8, i8* %4, align 1, !dbg !921
  %115 = sext i8 %114 to i32, !dbg !921
  %116 = sub nsw i32 %115, 97, !dbg !923
  %117 = add nsw i32 %116, 13, !dbg !924
  %118 = srem i32 %117, 26, !dbg !925
  %119 = add nsw i32 97, %118, !dbg !926
  %120 = trunc i32 %119 to i8, !dbg !927
  %121 = load i8*, i8** %3, align 4, !dbg !928
  store i8 %120, i8* %121, align 1, !dbg !929
  br label %122, !dbg !930

122:                                              ; preds = %113, %109, %104, %98, %93
  br label %123

123:                                              ; preds = %122, %83
  br label %124, !dbg !931

124:                                              ; preds = %123
  %125 = load i8*, i8** %3, align 4, !dbg !932
  %126 = getelementptr inbounds i8, i8* %125, i32 1, !dbg !932
  store i8* %126, i8** %3, align 4, !dbg !932
  br label %58, !dbg !933, !llvm.loop !934

127:                                              ; preds = %58
  br label %128, !dbg !936

128:                                              ; preds = %127, %49
  %129 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !937
  %130 = getelementptr inbounds %struct.fd, %struct.fd* %129, i32 0, i32 11, !dbg !939
  %131 = load i8, i8* %130, align 2, !dbg !939
  %132 = trunc i8 %131 to i1, !dbg !939
  br i1 %132, label %133, label %143, !dbg !940

133:                                              ; preds = %128
  %134 = load i8, i8* @No_recode, align 1, !dbg !941
  %135 = trunc i8 %134 to i1, !dbg !941
  br i1 %135, label %143, label %136, !dbg !942

136:                                              ; preds = %133
  call void @llvm.dbg.declare(metadata i8** %6, metadata !943, metadata !DIExpression()), !dbg !945
  %137 = getelementptr inbounds [1024 x i8], [1024 x i8]* %5, i32 0, i32 0, !dbg !946
  %138 = call i8* @my_recode_string(i8* noundef %137) #9, !dbg !947
  store i8* %138, i8** %6, align 4, !dbg !945
  %139 = load i8*, i8** %6, align 4, !dbg !948
  %140 = load %struct._IO_FILE*, %struct._IO_FILE** @stdout, align 4, !dbg !949
  %141 = call i32 @fputs(i8* noundef %139, %struct._IO_FILE* noundef %140) #9, !dbg !950
  %142 = load i8*, i8** %6, align 4, !dbg !951
  call void @free(i8* noundef %142) #9, !dbg !952
  br label %147, !dbg !953

143:                                              ; preds = %133, %128
  %144 = getelementptr inbounds [1024 x i8], [1024 x i8]* %5, i32 0, i32 0, !dbg !954
  %145 = load %struct._IO_FILE*, %struct._IO_FILE** @stdout, align 4, !dbg !955
  %146 = call i32 @fputs(i8* noundef %144, %struct._IO_FILE* noundef %145) #9, !dbg !956
  br label %147

147:                                              ; preds = %143, %136
  br label %148, !dbg !957

148:                                              ; preds = %147
  %149 = load i32, i32* @Fort_len, align 4, !dbg !958
  %150 = add nsw i32 %149, 1, !dbg !958
  store i32 %150, i32* @Fort_len, align 4, !dbg !958
  br label %21, !dbg !959, !llvm.loop !960

151:                                              ; preds = %47
  %152 = load %struct._IO_FILE*, %struct._IO_FILE** @stdout, align 4, !dbg !962
  %153 = call i32 @fflush(%struct._IO_FILE* noundef %152) #9, !dbg !963
  ret void, !dbg !964
}

; Function Attrs: noinline nounwind optnone
define internal i32 @mymax(i32 noundef %0, i32 noundef %1) #0 !dbg !965 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  call void @llvm.dbg.declare(metadata i32* %3, metadata !968, metadata !DIExpression()), !dbg !969
  store i32 %1, i32* %4, align 4
  call void @llvm.dbg.declare(metadata i32* %4, metadata !970, metadata !DIExpression()), !dbg !971
  %5 = load i32, i32* %3, align 4, !dbg !972
  %6 = load i32, i32* %4, align 4, !dbg !973
  %7 = icmp sge i32 %5, %6, !dbg !974
  br i1 %7, label %8, label %10, !dbg !972

8:                                                ; preds = %2
  %9 = load i32, i32* %3, align 4, !dbg !975
  br label %12, !dbg !972

10:                                               ; preds = %2
  %11 = load i32, i32* %4, align 4, !dbg !976
  br label %12, !dbg !972

12:                                               ; preds = %10, %8
  %13 = phi i32 [ %9, %8 ], [ %11, %10 ], !dbg !972
  ret i32 %13, !dbg !977
}

declare i32 @sleep(i32 noundef) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.label(metadata) #1

; Function Attrs: noinline nounwind optnone
define internal void @free_desc(%struct.fd* noundef %0) #0 !dbg !978 {
  %2 = alloca %struct.fd*, align 4
  %3 = alloca %struct.fd*, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !979, metadata !DIExpression()), !dbg !980
  br label %4, !dbg !981

4:                                                ; preds = %34, %1
  %5 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !982
  %6 = icmp ne %struct.fd* %5, null, !dbg !981
  br i1 %6, label %7, label %41, !dbg !981

7:                                                ; preds = %4
  %8 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !983
  %9 = getelementptr inbounds %struct.fd, %struct.fd* %8, i32 0, i32 14, !dbg !985
  %10 = load %struct.fd*, %struct.fd** %9, align 4, !dbg !985
  call void @free_desc(%struct.fd* noundef %10) #9, !dbg !986
  %11 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !987
  %12 = getelementptr inbounds %struct.fd, %struct.fd* %11, i32 0, i32 7, !dbg !988
  %13 = load i8*, i8** %12, align 4, !dbg !988
  call void @free(i8* noundef %13) #9, !dbg !989
  %14 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !990
  %15 = getelementptr inbounds %struct.fd, %struct.fd* %14, i32 0, i32 8, !dbg !991
  %16 = load i8*, i8** %15, align 4, !dbg !991
  call void @free(i8* noundef %16) #9, !dbg !992
  %17 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !993
  %18 = getelementptr inbounds %struct.fd, %struct.fd* %17, i32 0, i32 5, !dbg !994
  %19 = load i8*, i8** %18, align 4, !dbg !994
  call void @free(i8* noundef %19) #9, !dbg !995
  %20 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !996
  %21 = getelementptr inbounds %struct.fd, %struct.fd* %20, i32 0, i32 6, !dbg !997
  %22 = load i8*, i8** %21, align 4, !dbg !997
  call void @free(i8* noundef %22) #9, !dbg !998
  %23 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !999
  %24 = getelementptr inbounds %struct.fd, %struct.fd* %23, i32 0, i32 4, !dbg !1001
  %25 = load %struct._IO_FILE*, %struct._IO_FILE** %24, align 4, !dbg !1001
  %26 = icmp ne %struct._IO_FILE* %25, null, !dbg !999
  br i1 %26, label %27, label %34, !dbg !1002

27:                                               ; preds = %7
  %28 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1003
  %29 = getelementptr inbounds %struct.fd, %struct.fd* %28, i32 0, i32 4, !dbg !1005
  %30 = load %struct._IO_FILE*, %struct._IO_FILE** %29, align 4, !dbg !1005
  %31 = call i32 @fclose(%struct._IO_FILE* noundef %30) #9, !dbg !1006
  %32 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1007
  %33 = getelementptr inbounds %struct.fd, %struct.fd* %32, i32 0, i32 4, !dbg !1008
  store %struct._IO_FILE* null, %struct._IO_FILE** %33, align 4, !dbg !1009
  br label %34, !dbg !1010

34:                                               ; preds = %27, %7
  call void @llvm.dbg.declare(metadata %struct.fd** %3, metadata !1011, metadata !DIExpression()), !dbg !1013
  %35 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1014
  %36 = getelementptr inbounds %struct.fd, %struct.fd* %35, i32 0, i32 16, !dbg !1015
  %37 = load %struct.fd*, %struct.fd** %36, align 4, !dbg !1015
  store %struct.fd* %37, %struct.fd** %3, align 4, !dbg !1013
  %38 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1016
  %39 = bitcast %struct.fd* %38 to i8*, !dbg !1016
  call void @free(i8* noundef %39) #9, !dbg !1017
  %40 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1018
  store %struct.fd* %40, %struct.fd** %2, align 4, !dbg !1019
  br label %4, !dbg !981, !llvm.loop !1020

41:                                               ; preds = %4
  ret void, !dbg !1022
}

declare void @free(i8* noundef) #2

; Function Attrs: noreturn
declare void @exit(i32 noundef) #3

declare i32 @fclose(%struct._IO_FILE* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal void @open_fp(%struct.fd* noundef %0) #0 !dbg !1023 {
  %2 = alloca %struct.fd*, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !1024, metadata !DIExpression()), !dbg !1025
  %3 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1026
  %4 = getelementptr inbounds %struct.fd, %struct.fd* %3, i32 0, i32 4, !dbg !1028
  %5 = load %struct._IO_FILE*, %struct._IO_FILE** %4, align 4, !dbg !1028
  %6 = icmp ne %struct._IO_FILE* %5, null, !dbg !1026
  br i1 %6, label %19, label %7, !dbg !1029

7:                                                ; preds = %1
  %8 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1030
  %9 = getelementptr inbounds %struct.fd, %struct.fd* %8, i32 0, i32 1, !dbg !1031
  %10 = load i32, i32* %9, align 4, !dbg !1031
  %11 = call %struct._IO_FILE* @fdopen(i32 noundef %10, i8* noundef getelementptr inbounds ([2 x i8], [2 x i8]* @.str.59, i32 0, i32 0)) #9, !dbg !1032
  %12 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1033
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 4, !dbg !1034
  store %struct._IO_FILE* %11, %struct._IO_FILE** %13, align 4, !dbg !1035
  %14 = icmp ne %struct._IO_FILE* %11, null, !dbg !1035
  br i1 %14, label %19, label %15, !dbg !1036

15:                                               ; preds = %7
  %16 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1037
  %17 = getelementptr inbounds %struct.fd, %struct.fd* %16, i32 0, i32 6, !dbg !1039
  %18 = load i8*, i8** %17, align 4, !dbg !1039
  call void @perror(i8* noundef %18) #9, !dbg !1040
  call void @exit(i32 noundef 1) #10, !dbg !1041
  unreachable, !dbg !1041

19:                                               ; preds = %7, %1
  ret void, !dbg !1042
}

declare i32 @fseek(%struct._IO_FILE* noundef, i32 noundef, i32 noundef) #2

declare i32 @printf(i8* noundef, ...) #2

declare i8* @fgets(i8* noundef, i32 noundef, %struct._IO_FILE* noundef) #2

declare i32 @isupper(i32 noundef) #2

declare i32 @isascii(i32 noundef) #2

declare i32 @islower(i32 noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i8* @my_recode_string(i8* noundef %0) #0 !dbg !1043 {
  %2 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  call void @llvm.dbg.declare(metadata i8** %2, metadata !1046, metadata !DIExpression()), !dbg !1047
  %3 = load i8*, i8** %2, align 4, !dbg !1048
  %4 = call i8* @strdup(i8* noundef %3) #12, !dbg !1049
  ret i8* %4, !dbg !1050
}

declare i32 @fputs(i8* noundef, %struct._IO_FILE* noundef) #2

declare i32 @fflush(%struct._IO_FILE* noundef) #2

; Function Attrs: nounwind
declare i8* @strdup(i8* noundef) #4

declare %struct._IO_FILE* @fdopen(i32 noundef, i8* noundef) #2

declare void @perror(i8* noundef) #2

; Function Attrs: nocallback nounwind readonly willreturn
declare i32 @strlen(i8* noundef) #5

; Function Attrs: noinline nounwind optnone
define internal i32 @my_random(i32 noundef %0) #0 !dbg !1051 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i64, align 8
  %5 = alloca i8*, align 4
  %6 = alloca %struct._IO_FILE*, align 4
  store i32 %0, i32* %3, align 4
  call void @llvm.dbg.declare(metadata i32* %3, metadata !1055, metadata !DIExpression()), !dbg !1056
  call void @llvm.dbg.declare(metadata i64* %4, metadata !1057, metadata !DIExpression()), !dbg !1059
  store i64 0, i64* %4, align 8, !dbg !1059
  call void @llvm.dbg.declare(metadata i8** %5, metadata !1060, metadata !DIExpression()), !dbg !1061
  %7 = call i8* @getenv(i8* noundef getelementptr inbounds ([33 x i8], [33 x i8]* @.str.70, i32 0, i32 0)) #9, !dbg !1062
  store i8* %7, i8** %5, align 4, !dbg !1061
  %8 = load i8*, i8** %5, align 4, !dbg !1063
  %9 = icmp ne i8* %8, null, !dbg !1063
  br i1 %9, label %10, label %15, !dbg !1065

10:                                               ; preds = %1
  %11 = load i8*, i8** %5, align 4, !dbg !1066
  %12 = call i32 @atol(i8* noundef %11) #9, !dbg !1068
  %13 = load i32, i32* %3, align 4, !dbg !1069
  %14 = urem i32 %12, %13, !dbg !1070
  store i32 %14, i32* %2, align 4, !dbg !1071
  br label %44, !dbg !1071

15:                                               ; preds = %1
  %16 = call i8* @getenv(i8* noundef getelementptr inbounds ([22 x i8], [22 x i8]* @.str.71, i32 0, i32 0)) #9, !dbg !1072
  %17 = icmp ne i8* %16, null, !dbg !1072
  br i1 %17, label %18, label %19, !dbg !1074

18:                                               ; preds = %15
  br label %40, !dbg !1075

19:                                               ; preds = %15
  call void @llvm.dbg.declare(metadata %struct._IO_FILE** %6, metadata !1077, metadata !DIExpression()), !dbg !1079
  %20 = call %struct._IO_FILE* @fopen(i8* noundef getelementptr inbounds ([13 x i8], [13 x i8]* @.str.72, i32 0, i32 0), i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.73, i32 0, i32 0)) #9, !dbg !1080
  store %struct._IO_FILE* %20, %struct._IO_FILE** %6, align 4, !dbg !1079
  %21 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 4, !dbg !1081
  %22 = icmp ne %struct._IO_FILE* %21, null, !dbg !1081
  br i1 %22, label %24, label %23, !dbg !1083

23:                                               ; preds = %19
  br label %40, !dbg !1084

24:                                               ; preds = %19
  %25 = bitcast i64* %4 to i8*, !dbg !1086
  %26 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 4, !dbg !1088
  %27 = call i32 @fread(i8* noundef %25, i32 noundef 8, i32 noundef 1, %struct._IO_FILE* noundef %26) #9, !dbg !1089
  %28 = icmp ne i32 %27, 1, !dbg !1090
  br i1 %28, label %29, label %32, !dbg !1091

29:                                               ; preds = %24
  %30 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 4, !dbg !1092
  %31 = call i32 @fclose(%struct._IO_FILE* noundef %30) #9, !dbg !1094
  br label %40, !dbg !1095

32:                                               ; preds = %24
  %33 = load %struct._IO_FILE*, %struct._IO_FILE** %6, align 4, !dbg !1096
  %34 = call i32 @fclose(%struct._IO_FILE* noundef %33) #9, !dbg !1097
  %35 = load i64, i64* %4, align 8, !dbg !1098
  %36 = load i32, i32* %3, align 4, !dbg !1099
  %37 = zext i32 %36 to i64, !dbg !1099
  %38 = urem i64 %35, %37, !dbg !1100
  %39 = trunc i64 %38 to i32, !dbg !1098
  store i32 %39, i32* %2, align 4, !dbg !1101
  br label %44, !dbg !1101

40:                                               ; preds = %29, %23, %18
  call void @llvm.dbg.label(metadata !1102), !dbg !1103
  %41 = call i32 @random() #9, !dbg !1104
  %42 = load i32, i32* %3, align 4, !dbg !1105
  %43 = urem i32 %41, %42, !dbg !1106
  store i32 %43, i32* %2, align 4, !dbg !1107
  br label %44, !dbg !1107

44:                                               ; preds = %40, %32, %10
  %45 = load i32, i32* %2, align 4, !dbg !1108
  ret i32 %45, !dbg !1108
}

; Function Attrs: noinline nounwind optnone
define internal void @get_tbl(%struct.fd* noundef %0) #0 !dbg !1109 {
  %2 = alloca %struct.fd*, align 4
  %3 = alloca i32, align 4
  %4 = alloca %struct.fd*, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !1110, metadata !DIExpression()), !dbg !1111
  call void @llvm.dbg.declare(metadata i32* %3, metadata !1112, metadata !DIExpression()), !dbg !1113
  call void @llvm.dbg.declare(metadata %struct.fd** %4, metadata !1114, metadata !DIExpression()), !dbg !1115
  %5 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1116
  %6 = getelementptr inbounds %struct.fd, %struct.fd* %5, i32 0, i32 9, !dbg !1118
  %7 = load i8, i8* %6, align 4, !dbg !1118
  %8 = trunc i8 %7 to i1, !dbg !1118
  br i1 %8, label %9, label %10, !dbg !1119

9:                                                ; preds = %1
  br label %175, !dbg !1120

10:                                               ; preds = %1
  %11 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1122
  %12 = getelementptr inbounds %struct.fd, %struct.fd* %11, i32 0, i32 14, !dbg !1124
  %13 = load %struct.fd*, %struct.fd** %12, align 4, !dbg !1124
  %14 = icmp ne %struct.fd* %13, null, !dbg !1125
  br i1 %14, label %152, label %15, !dbg !1126

15:                                               ; preds = %10
  %16 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1127
  %17 = getelementptr inbounds %struct.fd, %struct.fd* %16, i32 0, i32 7, !dbg !1130
  %18 = load i8*, i8** %17, align 4, !dbg !1130
  %19 = call i32 @open4read(i8* noundef %18) #9, !dbg !1131
  store i32 %19, i32* %3, align 4, !dbg !1132
  %20 = icmp slt i32 %19, 0, !dbg !1133
  br i1 %20, label %21, label %25, !dbg !1134

21:                                               ; preds = %15
  %22 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1135
  %23 = getelementptr inbounds %struct.fd, %struct.fd* %22, i32 0, i32 7, !dbg !1137
  %24 = load i8*, i8** %23, align 4, !dbg !1137
  call void @perror(i8* noundef %24) #9, !dbg !1138
  call void @exit(i32 noundef 1) #10, !dbg !1139
  unreachable, !dbg !1139

25:                                               ; preds = %15
  %26 = load i32, i32* %3, align 4, !dbg !1140
  %27 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1142
  %28 = getelementptr inbounds %struct.fd, %struct.fd* %27, i32 0, i32 12, !dbg !1143
  %29 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %28, i32 0, i32 0, !dbg !1144
  %30 = bitcast i32* %29 to i8*, !dbg !1145
  %31 = call i32 @read(i32 noundef %26, i8* noundef %30, i32 noundef 4) #9, !dbg !1146
  %32 = icmp ne i32 %31, 4, !dbg !1147
  br i1 %32, label %33, label %39, !dbg !1148

33:                                               ; preds = %25
  %34 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1149
  %35 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1151
  %36 = getelementptr inbounds %struct.fd, %struct.fd* %35, i32 0, i32 6, !dbg !1152
  %37 = load i8*, i8** %36, align 4, !dbg !1152
  %38 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %34, i8* noundef getelementptr inbounds ([23 x i8], [23 x i8]* @.str.56, i32 0, i32 0), i8* noundef %37) #9, !dbg !1153
  call void @exit(i32 noundef 1) #10, !dbg !1154
  unreachable, !dbg !1154

39:                                               ; preds = %25
  %40 = load i32, i32* %3, align 4, !dbg !1155
  %41 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1157
  %42 = getelementptr inbounds %struct.fd, %struct.fd* %41, i32 0, i32 12, !dbg !1158
  %43 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %42, i32 0, i32 1, !dbg !1159
  %44 = bitcast i32* %43 to i8*, !dbg !1160
  %45 = call i32 @read(i32 noundef %40, i8* noundef %44, i32 noundef 4) #9, !dbg !1161
  %46 = icmp ne i32 %45, 4, !dbg !1162
  br i1 %46, label %47, label %53, !dbg !1163

47:                                               ; preds = %39
  %48 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1164
  %49 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1166
  %50 = getelementptr inbounds %struct.fd, %struct.fd* %49, i32 0, i32 6, !dbg !1167
  %51 = load i8*, i8** %50, align 4, !dbg !1167
  %52 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %48, i8* noundef getelementptr inbounds ([23 x i8], [23 x i8]* @.str.56, i32 0, i32 0), i8* noundef %51) #9, !dbg !1168
  call void @exit(i32 noundef 1) #10, !dbg !1169
  unreachable, !dbg !1169

53:                                               ; preds = %39
  %54 = load i32, i32* %3, align 4, !dbg !1170
  %55 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1172
  %56 = getelementptr inbounds %struct.fd, %struct.fd* %55, i32 0, i32 12, !dbg !1173
  %57 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %56, i32 0, i32 2, !dbg !1174
  %58 = bitcast i32* %57 to i8*, !dbg !1175
  %59 = call i32 @read(i32 noundef %54, i8* noundef %58, i32 noundef 4) #9, !dbg !1176
  %60 = icmp ne i32 %59, 4, !dbg !1177
  br i1 %60, label %61, label %67, !dbg !1178

61:                                               ; preds = %53
  %62 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1179
  %63 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1181
  %64 = getelementptr inbounds %struct.fd, %struct.fd* %63, i32 0, i32 6, !dbg !1182
  %65 = load i8*, i8** %64, align 4, !dbg !1182
  %66 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %62, i8* noundef getelementptr inbounds ([23 x i8], [23 x i8]* @.str.56, i32 0, i32 0), i8* noundef %65) #9, !dbg !1183
  call void @exit(i32 noundef 1) #10, !dbg !1184
  unreachable, !dbg !1184

67:                                               ; preds = %53
  %68 = load i32, i32* %3, align 4, !dbg !1185
  %69 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1187
  %70 = getelementptr inbounds %struct.fd, %struct.fd* %69, i32 0, i32 12, !dbg !1188
  %71 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %70, i32 0, i32 3, !dbg !1189
  %72 = bitcast i32* %71 to i8*, !dbg !1190
  %73 = call i32 @read(i32 noundef %68, i8* noundef %72, i32 noundef 4) #9, !dbg !1191
  %74 = icmp ne i32 %73, 4, !dbg !1192
  br i1 %74, label %75, label %81, !dbg !1193

75:                                               ; preds = %67
  %76 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1194
  %77 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1196
  %78 = getelementptr inbounds %struct.fd, %struct.fd* %77, i32 0, i32 6, !dbg !1197
  %79 = load i8*, i8** %78, align 4, !dbg !1197
  %80 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %76, i8* noundef getelementptr inbounds ([23 x i8], [23 x i8]* @.str.56, i32 0, i32 0), i8* noundef %79) #9, !dbg !1198
  call void @exit(i32 noundef 1) #10, !dbg !1199
  unreachable, !dbg !1199

81:                                               ; preds = %67
  %82 = load i32, i32* %3, align 4, !dbg !1200
  %83 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1202
  %84 = getelementptr inbounds %struct.fd, %struct.fd* %83, i32 0, i32 12, !dbg !1203
  %85 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %84, i32 0, i32 4, !dbg !1204
  %86 = bitcast i32* %85 to i8*, !dbg !1205
  %87 = call i32 @read(i32 noundef %82, i8* noundef %86, i32 noundef 4) #9, !dbg !1206
  %88 = icmp ne i32 %87, 4, !dbg !1207
  br i1 %88, label %89, label %95, !dbg !1208

89:                                               ; preds = %81
  %90 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1209
  %91 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1211
  %92 = getelementptr inbounds %struct.fd, %struct.fd* %91, i32 0, i32 6, !dbg !1212
  %93 = load i8*, i8** %92, align 4, !dbg !1212
  %94 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %90, i8* noundef getelementptr inbounds ([23 x i8], [23 x i8]* @.str.56, i32 0, i32 0), i8* noundef %93) #9, !dbg !1213
  call void @exit(i32 noundef 1) #10, !dbg !1214
  unreachable, !dbg !1214

95:                                               ; preds = %81
  %96 = load i32, i32* %3, align 4, !dbg !1215
  %97 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1217
  %98 = getelementptr inbounds %struct.fd, %struct.fd* %97, i32 0, i32 12, !dbg !1218
  %99 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %98, i32 0, i32 5, !dbg !1219
  %100 = bitcast [4 x i8]* %99 to i8*, !dbg !1220
  %101 = call i32 @read(i32 noundef %96, i8* noundef %100, i32 noundef 4) #9, !dbg !1221
  %102 = icmp ne i32 %101, 4, !dbg !1222
  br i1 %102, label %103, label %109, !dbg !1223

103:                                              ; preds = %95
  %104 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1224
  %105 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1226
  %106 = getelementptr inbounds %struct.fd, %struct.fd* %105, i32 0, i32 6, !dbg !1227
  %107 = load i8*, i8** %106, align 4, !dbg !1227
  %108 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %104, i8* noundef getelementptr inbounds ([23 x i8], [23 x i8]* @.str.56, i32 0, i32 0), i8* noundef %107) #9, !dbg !1228
  call void @exit(i32 noundef 1) #10, !dbg !1229
  unreachable, !dbg !1229

109:                                              ; preds = %95
  %110 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1230
  %111 = getelementptr inbounds %struct.fd, %struct.fd* %110, i32 0, i32 12, !dbg !1231
  %112 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %111, i32 0, i32 0, !dbg !1232
  %113 = load i32, i32* %112, align 4, !dbg !1232
  %114 = call i32 @ntohl(i32 noundef %113) #9, !dbg !1233
  %115 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1234
  %116 = getelementptr inbounds %struct.fd, %struct.fd* %115, i32 0, i32 12, !dbg !1235
  %117 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %116, i32 0, i32 0, !dbg !1236
  store i32 %114, i32* %117, align 4, !dbg !1237
  %118 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1238
  %119 = getelementptr inbounds %struct.fd, %struct.fd* %118, i32 0, i32 12, !dbg !1239
  %120 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %119, i32 0, i32 1, !dbg !1240
  %121 = load i32, i32* %120, align 4, !dbg !1240
  %122 = call i32 @ntohl(i32 noundef %121) #9, !dbg !1241
  %123 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1242
  %124 = getelementptr inbounds %struct.fd, %struct.fd* %123, i32 0, i32 12, !dbg !1243
  %125 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %124, i32 0, i32 1, !dbg !1244
  store i32 %122, i32* %125, align 4, !dbg !1245
  %126 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1246
  %127 = getelementptr inbounds %struct.fd, %struct.fd* %126, i32 0, i32 12, !dbg !1247
  %128 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %127, i32 0, i32 2, !dbg !1248
  %129 = load i32, i32* %128, align 4, !dbg !1248
  %130 = call i32 @ntohl(i32 noundef %129) #9, !dbg !1249
  %131 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1250
  %132 = getelementptr inbounds %struct.fd, %struct.fd* %131, i32 0, i32 12, !dbg !1251
  %133 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %132, i32 0, i32 2, !dbg !1252
  store i32 %130, i32* %133, align 4, !dbg !1253
  %134 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1254
  %135 = getelementptr inbounds %struct.fd, %struct.fd* %134, i32 0, i32 12, !dbg !1255
  %136 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %135, i32 0, i32 3, !dbg !1256
  %137 = load i32, i32* %136, align 4, !dbg !1256
  %138 = call i32 @ntohl(i32 noundef %137) #9, !dbg !1257
  %139 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1258
  %140 = getelementptr inbounds %struct.fd, %struct.fd* %139, i32 0, i32 12, !dbg !1259
  %141 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %140, i32 0, i32 3, !dbg !1260
  store i32 %138, i32* %141, align 4, !dbg !1261
  %142 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1262
  %143 = getelementptr inbounds %struct.fd, %struct.fd* %142, i32 0, i32 12, !dbg !1263
  %144 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %143, i32 0, i32 4, !dbg !1264
  %145 = load i32, i32* %144, align 4, !dbg !1264
  %146 = call i32 @ntohl(i32 noundef %145) #9, !dbg !1265
  %147 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1266
  %148 = getelementptr inbounds %struct.fd, %struct.fd* %147, i32 0, i32 12, !dbg !1267
  %149 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %148, i32 0, i32 4, !dbg !1268
  store i32 %146, i32* %149, align 4, !dbg !1269
  %150 = load i32, i32* %3, align 4, !dbg !1270
  %151 = call i32 @close(i32 noundef %150) #9, !dbg !1271
  br label %172, !dbg !1272

152:                                              ; preds = %10
  %153 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1273
  %154 = getelementptr inbounds %struct.fd, %struct.fd* %153, i32 0, i32 12, !dbg !1275
  call void @zero_tbl(%struct.STRFILE* noundef %154) #9, !dbg !1276
  %155 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1277
  %156 = getelementptr inbounds %struct.fd, %struct.fd* %155, i32 0, i32 14, !dbg !1279
  %157 = load %struct.fd*, %struct.fd** %156, align 4, !dbg !1279
  store %struct.fd* %157, %struct.fd** %4, align 4, !dbg !1280
  br label %158, !dbg !1281

158:                                              ; preds = %167, %152
  %159 = load %struct.fd*, %struct.fd** %4, align 4, !dbg !1282
  %160 = icmp ne %struct.fd* %159, null, !dbg !1284
  br i1 %160, label %161, label %171, !dbg !1284

161:                                              ; preds = %158
  %162 = load %struct.fd*, %struct.fd** %4, align 4, !dbg !1285
  call void @get_tbl(%struct.fd* noundef %162) #9, !dbg !1287
  %163 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1288
  %164 = getelementptr inbounds %struct.fd, %struct.fd* %163, i32 0, i32 12, !dbg !1289
  %165 = load %struct.fd*, %struct.fd** %4, align 4, !dbg !1290
  %166 = getelementptr inbounds %struct.fd, %struct.fd* %165, i32 0, i32 12, !dbg !1291
  call void @sum_tbl(%struct.STRFILE* noundef %164, %struct.STRFILE* noundef %166) #9, !dbg !1292
  br label %167, !dbg !1293

167:                                              ; preds = %161
  %168 = load %struct.fd*, %struct.fd** %4, align 4, !dbg !1294
  %169 = getelementptr inbounds %struct.fd, %struct.fd* %168, i32 0, i32 16, !dbg !1295
  %170 = load %struct.fd*, %struct.fd** %169, align 4, !dbg !1295
  store %struct.fd* %170, %struct.fd** %4, align 4, !dbg !1296
  br label %158, !dbg !1297, !llvm.loop !1298

171:                                              ; preds = %158
  br label %172

172:                                              ; preds = %171, %109
  %173 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1300
  %174 = getelementptr inbounds %struct.fd, %struct.fd* %173, i32 0, i32 9, !dbg !1301
  store i8 1, i8* %174, align 4, !dbg !1302
  br label %175, !dbg !1303

175:                                              ; preds = %172, %9
  ret void, !dbg !1303
}

declare i32 @fprintf(%struct._IO_FILE* noundef, i8* noundef, ...) #2

; Function Attrs: noinline nounwind optnone
define internal %struct.fd* @pick_child(%struct.fd* noundef %0) #0 !dbg !1304 {
  %2 = alloca %struct.fd*, align 4
  %3 = alloca %struct.fd*, align 4
  %4 = alloca i32, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !1307, metadata !DIExpression()), !dbg !1308
  call void @llvm.dbg.declare(metadata %struct.fd** %3, metadata !1309, metadata !DIExpression()), !dbg !1310
  call void @llvm.dbg.declare(metadata i32* %4, metadata !1311, metadata !DIExpression()), !dbg !1312
  %5 = load i8, i8* @Equal_probs, align 1, !dbg !1313
  %6 = trunc i8 %5 to i1, !dbg !1313
  br i1 %6, label %7, label %25, !dbg !1315

7:                                                ; preds = %1
  %8 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1316
  %9 = getelementptr inbounds %struct.fd, %struct.fd* %8, i32 0, i32 13, !dbg !1318
  %10 = load i32, i32* %9, align 4, !dbg !1318
  %11 = call i32 @my_random(i32 noundef %10) #9, !dbg !1319
  store i32 %11, i32* %4, align 4, !dbg !1320
  %12 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1321
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 14, !dbg !1323
  %14 = load %struct.fd*, %struct.fd** %13, align 4, !dbg !1323
  store %struct.fd* %14, %struct.fd** %3, align 4, !dbg !1324
  br label %15, !dbg !1325

15:                                               ; preds = %20, %7
  %16 = load i32, i32* %4, align 4, !dbg !1326
  %17 = add nsw i32 %16, -1, !dbg !1326
  store i32 %17, i32* %4, align 4, !dbg !1326
  %18 = icmp ne i32 %16, 0, !dbg !1328
  br i1 %18, label %19, label %24, !dbg !1328

19:                                               ; preds = %15
  br label %20, !dbg !1329

20:                                               ; preds = %19
  %21 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1331
  %22 = getelementptr inbounds %struct.fd, %struct.fd* %21, i32 0, i32 16, !dbg !1332
  %23 = load %struct.fd*, %struct.fd** %22, align 4, !dbg !1332
  store %struct.fd* %23, %struct.fd** %3, align 4, !dbg !1333
  br label %15, !dbg !1334, !llvm.loop !1335

24:                                               ; preds = %15
  br label %54, !dbg !1337

25:                                               ; preds = %1
  %26 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1338
  call void @get_tbl(%struct.fd* noundef %26) #9, !dbg !1340
  %27 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1341
  %28 = getelementptr inbounds %struct.fd, %struct.fd* %27, i32 0, i32 12, !dbg !1342
  %29 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %28, i32 0, i32 1, !dbg !1343
  %30 = load i32, i32* %29, align 4, !dbg !1343
  %31 = call i32 @my_random(i32 noundef %30) #9, !dbg !1344
  store i32 %31, i32* %4, align 4, !dbg !1345
  %32 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1346
  %33 = getelementptr inbounds %struct.fd, %struct.fd* %32, i32 0, i32 14, !dbg !1348
  %34 = load %struct.fd*, %struct.fd** %33, align 4, !dbg !1348
  store %struct.fd* %34, %struct.fd** %3, align 4, !dbg !1349
  br label %35, !dbg !1350

35:                                               ; preds = %49, %25
  %36 = load i32, i32* %4, align 4, !dbg !1351
  %37 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1353
  %38 = getelementptr inbounds %struct.fd, %struct.fd* %37, i32 0, i32 12, !dbg !1354
  %39 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %38, i32 0, i32 1, !dbg !1355
  %40 = load i32, i32* %39, align 4, !dbg !1355
  %41 = icmp sge i32 %36, %40, !dbg !1356
  br i1 %41, label %42, label %53, !dbg !1357

42:                                               ; preds = %35
  %43 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1358
  %44 = getelementptr inbounds %struct.fd, %struct.fd* %43, i32 0, i32 12, !dbg !1360
  %45 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %44, i32 0, i32 1, !dbg !1361
  %46 = load i32, i32* %45, align 4, !dbg !1361
  %47 = load i32, i32* %4, align 4, !dbg !1362
  %48 = sub i32 %47, %46, !dbg !1362
  store i32 %48, i32* %4, align 4, !dbg !1362
  br label %49, !dbg !1363

49:                                               ; preds = %42
  %50 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1364
  %51 = getelementptr inbounds %struct.fd, %struct.fd* %50, i32 0, i32 16, !dbg !1365
  %52 = load %struct.fd*, %struct.fd** %51, align 4, !dbg !1365
  store %struct.fd* %52, %struct.fd** %3, align 4, !dbg !1366
  br label %35, !dbg !1367, !llvm.loop !1368

53:                                               ; preds = %35
  br label %54

54:                                               ; preds = %53, %24
  %55 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1370
  %56 = getelementptr inbounds %struct.fd, %struct.fd* %55, i32 0, i32 14, !dbg !1371
  %57 = load %struct.fd*, %struct.fd** %56, align 4, !dbg !1371
  %58 = icmp ne %struct.fd* %57, null, !dbg !1370
  br i1 %58, label %59, label %62, !dbg !1370

59:                                               ; preds = %54
  %60 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1372
  %61 = call %struct.fd* @pick_child(%struct.fd* noundef %60) #9, !dbg !1373
  br label %64, !dbg !1370

62:                                               ; preds = %54
  %63 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1374
  br label %64, !dbg !1370

64:                                               ; preds = %62, %59
  %65 = phi %struct.fd* [ %61, %59 ], [ %63, %62 ], !dbg !1370
  ret %struct.fd* %65, !dbg !1375
}

; Function Attrs: noreturn
declare void @__assert_fail(i8* noundef, i8* noundef, i32 noundef, i8* noundef) #3

; Function Attrs: noinline nounwind optnone
define internal void @get_pos(%struct.fd* noundef %0) #0 !dbg !1376 {
  %2 = alloca %struct.fd*, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !1377, metadata !DIExpression()), !dbg !1378
  %3 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1379
  %4 = getelementptr inbounds %struct.fd, %struct.fd* %3, i32 0, i32 9, !dbg !1379
  %5 = load i8, i8* %4, align 4, !dbg !1379
  %6 = trunc i8 %5 to i1, !dbg !1379
  br i1 %6, label %9, label %7, !dbg !1379

7:                                                ; preds = %1
  call void @__assert_fail(i8* noundef getelementptr inbounds ([13 x i8], [13 x i8]* @.str.74, i32 0, i32 0), i8* noundef getelementptr inbounds ([72 x i8], [72 x i8]* @.str.69, i32 0, i32 0), i32 noundef 1621, i8* noundef getelementptr inbounds ([8 x i8], [8 x i8]* @__func__.get_pos, i32 0, i32 0)) #10, !dbg !1379
  unreachable, !dbg !1379

8:                                                ; No predecessors!
  br label %9, !dbg !1379

9:                                                ; preds = %8, %1
  %10 = phi i1 [ true, %1 ], [ false, %8 ]
  %11 = zext i1 %10 to i32, !dbg !1379
  %12 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1380
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 3, !dbg !1382
  %14 = load i32, i32* %13, align 4, !dbg !1382
  %15 = icmp eq i32 %14, -1, !dbg !1383
  br i1 %15, label %16, label %24, !dbg !1384

16:                                               ; preds = %9
  %17 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1385
  %18 = getelementptr inbounds %struct.fd, %struct.fd* %17, i32 0, i32 12, !dbg !1387
  %19 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %18, i32 0, i32 1, !dbg !1388
  %20 = load i32, i32* %19, align 4, !dbg !1388
  %21 = call i32 @my_random(i32 noundef %20) #9, !dbg !1389
  %22 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1390
  %23 = getelementptr inbounds %struct.fd, %struct.fd* %22, i32 0, i32 3, !dbg !1391
  store i32 %21, i32* %23, align 4, !dbg !1392
  br label %24, !dbg !1393

24:                                               ; preds = %16, %9
  %25 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1394
  %26 = getelementptr inbounds %struct.fd, %struct.fd* %25, i32 0, i32 3, !dbg !1396
  %27 = load i32, i32* %26, align 4, !dbg !1397
  %28 = add nsw i32 %27, 1, !dbg !1397
  store i32 %28, i32* %26, align 4, !dbg !1397
  %29 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1398
  %30 = getelementptr inbounds %struct.fd, %struct.fd* %29, i32 0, i32 12, !dbg !1399
  %31 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %30, i32 0, i32 1, !dbg !1400
  %32 = load i32, i32* %31, align 4, !dbg !1400
  %33 = icmp sge i32 %28, %32, !dbg !1401
  br i1 %33, label %34, label %43, !dbg !1402

34:                                               ; preds = %24
  %35 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1403
  %36 = getelementptr inbounds %struct.fd, %struct.fd* %35, i32 0, i32 12, !dbg !1405
  %37 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %36, i32 0, i32 1, !dbg !1406
  %38 = load i32, i32* %37, align 4, !dbg !1406
  %39 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1407
  %40 = getelementptr inbounds %struct.fd, %struct.fd* %39, i32 0, i32 3, !dbg !1408
  %41 = load i32, i32* %40, align 4, !dbg !1409
  %42 = sub i32 %41, %38, !dbg !1409
  store i32 %42, i32* %40, align 4, !dbg !1409
  br label %43, !dbg !1410

43:                                               ; preds = %34, %24
  ret void, !dbg !1411
}

; Function Attrs: noinline nounwind optnone
define internal void @open_dat(%struct.fd* noundef %0) #0 !dbg !1412 {
  %2 = alloca %struct.fd*, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !1413, metadata !DIExpression()), !dbg !1414
  %3 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1415
  %4 = getelementptr inbounds %struct.fd, %struct.fd* %3, i32 0, i32 2, !dbg !1417
  %5 = load i32, i32* %4, align 4, !dbg !1417
  %6 = icmp slt i32 %5, 0, !dbg !1418
  br i1 %6, label %7, label %16, !dbg !1419

7:                                                ; preds = %1
  %8 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1420
  %9 = getelementptr inbounds %struct.fd, %struct.fd* %8, i32 0, i32 7, !dbg !1421
  %10 = load i8*, i8** %9, align 4, !dbg !1421
  %11 = call i32 @open4read(i8* noundef %10) #9, !dbg !1422
  %12 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1423
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 2, !dbg !1424
  store i32 %11, i32* %13, align 4, !dbg !1425
  %14 = icmp slt i32 %11, 0, !dbg !1426
  br i1 %14, label %15, label %16, !dbg !1427

15:                                               ; preds = %7
  call void @exit(i32 noundef 1) #10, !dbg !1428
  unreachable, !dbg !1428

16:                                               ; preds = %7, %1
  ret void, !dbg !1430
}

; Function Attrs: convergent nofree nosync nounwind readnone willreturn
declare i1 @llvm.is.constant.i64(i64) #6

declare i64 @__wasilibc_tell(i32 noundef) #2

declare i64 @lseek(i32 noundef, i64 noundef, i32 noundef) #2

declare i32 @read(i32 noundef, i8* noundef, i32 noundef) #2

declare i32 @ntohl(i32 noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i32 @open4read(i8* noundef %0) #0 !dbg !1431 {
  %2 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  call void @llvm.dbg.declare(metadata i8** %2, metadata !1434, metadata !DIExpression()), !dbg !1435
  %3 = load i8*, i8** %2, align 4, !dbg !1436
  %4 = call i32 (i8*, i32, ...) @open(i8* noundef %3, i32 noundef 67108864) #9, !dbg !1437
  ret i32 %4, !dbg !1438
}

declare i32 @open(i8* noundef, i32 noundef, ...) #2

declare i32 @close(i32 noundef) #2

; Function Attrs: noinline nounwind optnone
define internal void @zero_tbl(%struct.STRFILE* noundef %0) #0 !dbg !1439 {
  %2 = alloca %struct.STRFILE*, align 4
  store %struct.STRFILE* %0, %struct.STRFILE** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.STRFILE** %2, metadata !1443, metadata !DIExpression()), !dbg !1444
  %3 = load %struct.STRFILE*, %struct.STRFILE** %2, align 4, !dbg !1445
  %4 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %3, i32 0, i32 1, !dbg !1446
  store i32 0, i32* %4, align 4, !dbg !1447
  %5 = load %struct.STRFILE*, %struct.STRFILE** %2, align 4, !dbg !1448
  %6 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %5, i32 0, i32 2, !dbg !1449
  store i32 0, i32* %6, align 4, !dbg !1450
  %7 = load %struct.STRFILE*, %struct.STRFILE** %2, align 4, !dbg !1451
  %8 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %7, i32 0, i32 3, !dbg !1452
  store i32 -1, i32* %8, align 4, !dbg !1453
  ret void, !dbg !1454
}

; Function Attrs: noinline nounwind optnone
define internal void @sum_tbl(%struct.STRFILE* noundef %0, %struct.STRFILE* noundef %1) #0 !dbg !1455 {
  %3 = alloca %struct.STRFILE*, align 4
  %4 = alloca %struct.STRFILE*, align 4
  store %struct.STRFILE* %0, %struct.STRFILE** %3, align 4
  call void @llvm.dbg.declare(metadata %struct.STRFILE** %3, metadata !1458, metadata !DIExpression()), !dbg !1459
  store %struct.STRFILE* %1, %struct.STRFILE** %4, align 4
  call void @llvm.dbg.declare(metadata %struct.STRFILE** %4, metadata !1460, metadata !DIExpression()), !dbg !1461
  %5 = load %struct.STRFILE*, %struct.STRFILE** %4, align 4, !dbg !1462
  %6 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %5, i32 0, i32 1, !dbg !1463
  %7 = load i32, i32* %6, align 4, !dbg !1463
  %8 = load %struct.STRFILE*, %struct.STRFILE** %3, align 4, !dbg !1464
  %9 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %8, i32 0, i32 1, !dbg !1465
  %10 = load i32, i32* %9, align 4, !dbg !1466
  %11 = add i32 %10, %7, !dbg !1466
  store i32 %11, i32* %9, align 4, !dbg !1466
  %12 = load %struct.STRFILE*, %struct.STRFILE** %3, align 4, !dbg !1467
  %13 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %12, i32 0, i32 2, !dbg !1469
  %14 = load i32, i32* %13, align 4, !dbg !1469
  %15 = load %struct.STRFILE*, %struct.STRFILE** %4, align 4, !dbg !1470
  %16 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %15, i32 0, i32 2, !dbg !1471
  %17 = load i32, i32* %16, align 4, !dbg !1471
  %18 = icmp ult i32 %14, %17, !dbg !1472
  br i1 %18, label %19, label %25, !dbg !1473

19:                                               ; preds = %2
  %20 = load %struct.STRFILE*, %struct.STRFILE** %4, align 4, !dbg !1474
  %21 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %20, i32 0, i32 2, !dbg !1476
  %22 = load i32, i32* %21, align 4, !dbg !1476
  %23 = load %struct.STRFILE*, %struct.STRFILE** %3, align 4, !dbg !1477
  %24 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %23, i32 0, i32 2, !dbg !1478
  store i32 %22, i32* %24, align 4, !dbg !1479
  br label %25, !dbg !1480

25:                                               ; preds = %19, %2
  %26 = load %struct.STRFILE*, %struct.STRFILE** %3, align 4, !dbg !1481
  %27 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %26, i32 0, i32 3, !dbg !1483
  %28 = load i32, i32* %27, align 4, !dbg !1483
  %29 = load %struct.STRFILE*, %struct.STRFILE** %4, align 4, !dbg !1484
  %30 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %29, i32 0, i32 3, !dbg !1485
  %31 = load i32, i32* %30, align 4, !dbg !1485
  %32 = icmp ugt i32 %28, %31, !dbg !1486
  br i1 %32, label %33, label %39, !dbg !1487

33:                                               ; preds = %25
  %34 = load %struct.STRFILE*, %struct.STRFILE** %4, align 4, !dbg !1488
  %35 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %34, i32 0, i32 3, !dbg !1490
  %36 = load i32, i32* %35, align 4, !dbg !1490
  %37 = load %struct.STRFILE*, %struct.STRFILE** %3, align 4, !dbg !1491
  %38 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %37, i32 0, i32 3, !dbg !1492
  store i32 %36, i32* %38, align 4, !dbg !1493
  br label %39, !dbg !1494

39:                                               ; preds = %33, %25
  ret void, !dbg !1495
}

declare i32 @atol(i8* noundef) #2

declare %struct._IO_FILE* @fopen(i8* noundef, i8* noundef) #2

declare i32 @fread(i8* noundef, i32 noundef, i32 noundef, %struct._IO_FILE* noundef) #2

declare i32 @random() #2

declare i64 @time(i64* noundef) #2

declare i32 @getpid() #2

declare void @srandom(i32 noundef) #2

declare i32 @putc(i32 noundef, %struct._IO_FILE* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i32 @maxlen_in_list(%struct.fd* noundef %0) #0 !dbg !1496 {
  %2 = alloca %struct.fd*, align 4
  %3 = alloca %struct.fd*, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store %struct.fd* %0, %struct.fd** %2, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %2, metadata !1499, metadata !DIExpression()), !dbg !1500
  call void @llvm.dbg.declare(metadata %struct.fd** %3, metadata !1501, metadata !DIExpression()), !dbg !1502
  call void @llvm.dbg.declare(metadata i32* %4, metadata !1503, metadata !DIExpression()), !dbg !1504
  call void @llvm.dbg.declare(metadata i32* %5, metadata !1505, metadata !DIExpression()), !dbg !1506
  store i32 0, i32* %5, align 4, !dbg !1506
  %6 = load %struct.fd*, %struct.fd** %2, align 4, !dbg !1507
  store %struct.fd* %6, %struct.fd** %3, align 4, !dbg !1509
  br label %7, !dbg !1510

7:                                                ; preds = %40, %1
  %8 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1511
  %9 = icmp ne %struct.fd* %8, null, !dbg !1513
  br i1 %9, label %10, label %44, !dbg !1513

10:                                               ; preds = %7
  %11 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1514
  %12 = getelementptr inbounds %struct.fd, %struct.fd* %11, i32 0, i32 14, !dbg !1517
  %13 = load %struct.fd*, %struct.fd** %12, align 4, !dbg !1517
  %14 = icmp ne %struct.fd* %13, null, !dbg !1514
  br i1 %14, label %15, label %25, !dbg !1518

15:                                               ; preds = %10
  %16 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1519
  %17 = getelementptr inbounds %struct.fd, %struct.fd* %16, i32 0, i32 14, !dbg !1522
  %18 = load %struct.fd*, %struct.fd** %17, align 4, !dbg !1522
  %19 = call i32 @maxlen_in_list(%struct.fd* noundef %18) #9, !dbg !1523
  store i32 %19, i32* %4, align 4, !dbg !1524
  %20 = load i32, i32* %5, align 4, !dbg !1525
  %21 = icmp sgt i32 %19, %20, !dbg !1526
  br i1 %21, label %22, label %24, !dbg !1527

22:                                               ; preds = %15
  %23 = load i32, i32* %4, align 4, !dbg !1528
  store i32 %23, i32* %5, align 4, !dbg !1530
  br label %24, !dbg !1531

24:                                               ; preds = %22, %15
  br label %39, !dbg !1532

25:                                               ; preds = %10
  %26 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1533
  call void @get_tbl(%struct.fd* noundef %26) #9, !dbg !1535
  %27 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1536
  %28 = getelementptr inbounds %struct.fd, %struct.fd* %27, i32 0, i32 12, !dbg !1538
  %29 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %28, i32 0, i32 2, !dbg !1539
  %30 = load i32, i32* %29, align 4, !dbg !1539
  %31 = load i32, i32* %5, align 4, !dbg !1540
  %32 = icmp sgt i32 %30, %31, !dbg !1541
  br i1 %32, label %33, label %38, !dbg !1542

33:                                               ; preds = %25
  %34 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1543
  %35 = getelementptr inbounds %struct.fd, %struct.fd* %34, i32 0, i32 12, !dbg !1545
  %36 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %35, i32 0, i32 2, !dbg !1546
  %37 = load i32, i32* %36, align 4, !dbg !1546
  store i32 %37, i32* %5, align 4, !dbg !1547
  br label %38, !dbg !1548

38:                                               ; preds = %33, %25
  br label %39

39:                                               ; preds = %38, %24
  br label %40, !dbg !1549

40:                                               ; preds = %39
  %41 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1550
  %42 = getelementptr inbounds %struct.fd, %struct.fd* %41, i32 0, i32 16, !dbg !1551
  %43 = load %struct.fd*, %struct.fd** %42, align 4, !dbg !1551
  store %struct.fd* %43, %struct.fd** %3, align 4, !dbg !1552
  br label %7, !dbg !1553, !llvm.loop !1554

44:                                               ; preds = %7
  %45 = load i32, i32* %5, align 4, !dbg !1556
  ret i32 %45, !dbg !1557
}

; Function Attrs: noinline nounwind optnone
define internal i8* @do_malloc(i32 noundef %0) #0 !dbg !1558 {
  %2 = alloca i32, align 4
  %3 = alloca i8*, align 4
  store i32 %0, i32* %2, align 4
  call void @llvm.dbg.declare(metadata i32* %2, metadata !1562, metadata !DIExpression()), !dbg !1563
  call void @llvm.dbg.declare(metadata i8** %3, metadata !1564, metadata !DIExpression()), !dbg !1565
  %4 = load i32, i32* %2, align 4, !dbg !1566
  %5 = call noalias i8* @malloc(i32 noundef %4) #9, !dbg !1567
  store i8* %5, i8** %3, align 4, !dbg !1565
  %6 = load i8*, i8** %3, align 4, !dbg !1568
  %7 = icmp ne i8* %6, null, !dbg !1568
  br i1 %7, label %11, label %8, !dbg !1570

8:                                                ; preds = %1
  %9 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1571
  %10 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %9, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([25 x i8], [25 x i8]* @.str.38, i32 0, i32 0)) #9, !dbg !1573
  call void @exit(i32 noundef 1) #10, !dbg !1574
  unreachable, !dbg !1574

11:                                               ; preds = %1
  %12 = load i8*, i8** %3, align 4, !dbg !1575
  ret i8* %12, !dbg !1576
}

; Function Attrs: noinline nounwind optnone
define internal void @matches_in_list(%struct.fd* noundef %0, i8* noundef %1) #0 !dbg !1577 {
  %3 = alloca %struct.fd*, align 4
  %4 = alloca i8*, align 4
  %5 = alloca i8*, align 4
  %6 = alloca i8*, align 4
  %7 = alloca i8, align 1
  %8 = alloca %struct.fd*, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i8*, align 4
  store %struct.fd* %0, %struct.fd** %3, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %3, metadata !1582, metadata !DIExpression()), !dbg !1583
  store i8* %1, i8** %4, align 4
  call void @llvm.dbg.declare(metadata i8** %4, metadata !1584, metadata !DIExpression()), !dbg !1585
  call void @llvm.dbg.declare(metadata i8** %5, metadata !1586, metadata !DIExpression()), !dbg !1587
  call void @llvm.dbg.declare(metadata i8** %6, metadata !1588, metadata !DIExpression()), !dbg !1589
  call void @llvm.dbg.declare(metadata i8* %7, metadata !1590, metadata !DIExpression()), !dbg !1591
  call void @llvm.dbg.declare(metadata %struct.fd** %8, metadata !1592, metadata !DIExpression()), !dbg !1593
  call void @llvm.dbg.declare(metadata i32* %9, metadata !1594, metadata !DIExpression()), !dbg !1595
  call void @llvm.dbg.declare(metadata i32* %10, metadata !1596, metadata !DIExpression()), !dbg !1597
  call void @llvm.dbg.declare(metadata i8** %11, metadata !1598, metadata !DIExpression()), !dbg !1599
  %12 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !1600
  store %struct.fd* %12, %struct.fd** %8, align 4, !dbg !1602
  br label %13, !dbg !1603

13:                                               ; preds = %217, %2
  %14 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1604
  %15 = icmp ne %struct.fd* %14, null, !dbg !1606
  br i1 %15, label %16, label %221, !dbg !1606

16:                                               ; preds = %13
  %17 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1607
  %18 = getelementptr inbounds %struct.fd, %struct.fd* %17, i32 0, i32 14, !dbg !1610
  %19 = load %struct.fd*, %struct.fd** %18, align 4, !dbg !1610
  %20 = icmp ne %struct.fd* %19, null, !dbg !1607
  br i1 %20, label %21, label %26, !dbg !1611

21:                                               ; preds = %16
  %22 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1612
  %23 = getelementptr inbounds %struct.fd, %struct.fd* %22, i32 0, i32 14, !dbg !1614
  %24 = load %struct.fd*, %struct.fd** %23, align 4, !dbg !1614
  %25 = load i8*, i8** %4, align 4, !dbg !1615
  call void @matches_in_list(%struct.fd* noundef %24, i8* noundef %25) #9, !dbg !1616
  br label %217, !dbg !1617

26:                                               ; preds = %16
  %27 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1618
  call void @open_fp(%struct.fd* noundef %27) #9, !dbg !1619
  %28 = load i8*, i8** @Fortbuf, align 4, !dbg !1620
  store i8* %28, i8** %5, align 4, !dbg !1621
  store i32 0, i32* %9, align 4, !dbg !1622
  br label %29, !dbg !1623

29:                                               ; preds = %215, %26
  %30 = load i8*, i8** %5, align 4, !dbg !1624
  %31 = load i32, i32* @Fort_len, align 4, !dbg !1625
  %32 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1626
  %33 = getelementptr inbounds %struct.fd, %struct.fd* %32, i32 0, i32 4, !dbg !1627
  %34 = load %struct._IO_FILE*, %struct._IO_FILE** %33, align 4, !dbg !1627
  %35 = call i8* @fgets(i8* noundef %30, i32 noundef %31, %struct._IO_FILE* noundef %34) #9, !dbg !1628
  %36 = icmp ne i8* %35, null, !dbg !1623
  br i1 %36, label %37, label %216, !dbg !1623

37:                                               ; preds = %29
  %38 = load i8*, i8** %5, align 4, !dbg !1629
  %39 = getelementptr inbounds i8, i8* %38, i32 0, !dbg !1629
  %40 = load i8, i8* %39, align 1, !dbg !1629
  %41 = zext i8 %40 to i32, !dbg !1629
  %42 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1629
  %43 = getelementptr inbounds %struct.fd, %struct.fd* %42, i32 0, i32 12, !dbg !1629
  %44 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %43, i32 0, i32 5, !dbg !1629
  %45 = getelementptr inbounds [4 x i8], [4 x i8]* %44, i32 0, i32 0, !dbg !1629
  %46 = load i8, i8* %45, align 4, !dbg !1629
  %47 = zext i8 %46 to i32, !dbg !1629
  %48 = icmp eq i32 %41, %47, !dbg !1629
  br i1 %48, label %49, label %55, !dbg !1629

49:                                               ; preds = %37
  %50 = load i8*, i8** %5, align 4, !dbg !1629
  %51 = getelementptr inbounds i8, i8* %50, i32 1, !dbg !1629
  %52 = load i8, i8* %51, align 1, !dbg !1629
  %53 = zext i8 %52 to i32, !dbg !1629
  %54 = icmp eq i32 %53, 10, !dbg !1629
  br i1 %54, label %60, label %55, !dbg !1632

55:                                               ; preds = %49, %37
  %56 = load i8*, i8** %5, align 4, !dbg !1633
  %57 = call i32 @strlen(i8* noundef %56) #11, !dbg !1635
  %58 = load i8*, i8** %5, align 4, !dbg !1636
  %59 = getelementptr inbounds i8, i8* %58, i32 %57, !dbg !1636
  store i8* %59, i8** %5, align 4, !dbg !1636
  br label %215, !dbg !1637

60:                                               ; preds = %49
  %61 = load i8*, i8** %5, align 4, !dbg !1638
  store i8 0, i8* %61, align 1, !dbg !1640
  %62 = load i8*, i8** %5, align 4, !dbg !1641
  %63 = load i8*, i8** @Fortbuf, align 4, !dbg !1642
  %64 = ptrtoint i8* %62 to i32, !dbg !1643
  %65 = ptrtoint i8* %63 to i32, !dbg !1643
  %66 = sub i32 %64, %65, !dbg !1643
  store i32 %66, i32* %10, align 4, !dbg !1644
  %67 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1645
  %68 = getelementptr inbounds %struct.fd, %struct.fd* %67, i32 0, i32 11, !dbg !1647
  %69 = load i8, i8* %68, align 2, !dbg !1647
  %70 = trunc i8 %69 to i1, !dbg !1647
  br i1 %70, label %71, label %77, !dbg !1648

71:                                               ; preds = %60
  %72 = load i8, i8* @No_recode, align 1, !dbg !1649
  %73 = trunc i8 %72 to i1, !dbg !1649
  br i1 %73, label %77, label %74, !dbg !1650

74:                                               ; preds = %71
  %75 = load i8*, i8** @Fortbuf, align 4, !dbg !1651
  %76 = call i8* @my_recode_string(i8* noundef %75) #9, !dbg !1653
  store i8* %76, i8** %11, align 4, !dbg !1654
  br label %79, !dbg !1655

77:                                               ; preds = %71, %60
  %78 = load i8*, i8** @Fortbuf, align 4, !dbg !1656
  store i8* %78, i8** %11, align 4, !dbg !1658
  br label %79

79:                                               ; preds = %77, %74
  %80 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1659
  %81 = getelementptr inbounds %struct.fd, %struct.fd* %80, i32 0, i32 12, !dbg !1661
  %82 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %81, i32 0, i32 4, !dbg !1662
  %83 = load i32, i32* %82, align 4, !dbg !1662
  %84 = and i32 %83, 4, !dbg !1663
  %85 = icmp ne i32 %84, 0, !dbg !1663
  br i1 %85, label %86, label %158, !dbg !1664

86:                                               ; preds = %79
  %87 = load i8*, i8** %11, align 4, !dbg !1665
  store i8* %87, i8** %6, align 4, !dbg !1668
  br label %88, !dbg !1669

88:                                               ; preds = %154, %86
  %89 = load i8*, i8** %6, align 4, !dbg !1670
  %90 = load i8, i8* %89, align 1, !dbg !1672
  store i8 %90, i8* %7, align 1, !dbg !1673
  %91 = icmp ne i8 %90, 0, !dbg !1674
  br i1 %91, label %92, label %157, !dbg !1674

92:                                               ; preds = %88
  br i1 false, label %93, label %98, !dbg !1675

93:                                               ; preds = %92
  %94 = load i8, i8* %7, align 1, !dbg !1678
  %95 = zext i8 %94 to i32, !dbg !1678
  %96 = call i32 @isupper(i32 noundef %95) #9, !dbg !1678
  %97 = icmp ne i32 %96, 0, !dbg !1678
  br i1 %97, label %103, label %122, !dbg !1678

98:                                               ; preds = %92
  %99 = load i8, i8* %7, align 1, !dbg !1678
  %100 = zext i8 %99 to i32, !dbg !1678
  %101 = sub i32 %100, 65, !dbg !1678
  %102 = icmp ult i32 %101, 26, !dbg !1678
  br i1 %102, label %103, label %122, !dbg !1675

103:                                              ; preds = %98, %93
  br i1 false, label %104, label %109, !dbg !1679

104:                                              ; preds = %103
  %105 = load i8, i8* %7, align 1, !dbg !1680
  %106 = zext i8 %105 to i32, !dbg !1680
  %107 = call i32 @isascii(i32 noundef %106) #9, !dbg !1680
  %108 = icmp ne i32 %107, 0, !dbg !1680
  br i1 %108, label %113, label %122, !dbg !1680

109:                                              ; preds = %103
  %110 = load i8, i8* %7, align 1, !dbg !1680
  %111 = zext i8 %110 to i32, !dbg !1680
  %112 = icmp ult i32 %111, 128, !dbg !1680
  br i1 %112, label %113, label %122, !dbg !1679

113:                                              ; preds = %109, %104
  %114 = load i8, i8* %7, align 1, !dbg !1681
  %115 = zext i8 %114 to i32, !dbg !1681
  %116 = sub nsw i32 %115, 65, !dbg !1683
  %117 = add nsw i32 %116, 13, !dbg !1684
  %118 = srem i32 %117, 26, !dbg !1685
  %119 = add nsw i32 65, %118, !dbg !1686
  %120 = trunc i32 %119 to i8, !dbg !1687
  %121 = load i8*, i8** %6, align 4, !dbg !1688
  store i8 %120, i8* %121, align 1, !dbg !1689
  br label %153, !dbg !1690

122:                                              ; preds = %109, %104, %98, %93
  br i1 false, label %123, label %128, !dbg !1691

123:                                              ; preds = %122
  %124 = load i8, i8* %7, align 1, !dbg !1693
  %125 = zext i8 %124 to i32, !dbg !1693
  %126 = call i32 @islower(i32 noundef %125) #9, !dbg !1693
  %127 = icmp ne i32 %126, 0, !dbg !1693
  br i1 %127, label %133, label %152, !dbg !1693

128:                                              ; preds = %122
  %129 = load i8, i8* %7, align 1, !dbg !1693
  %130 = zext i8 %129 to i32, !dbg !1693
  %131 = sub i32 %130, 97, !dbg !1693
  %132 = icmp ult i32 %131, 26, !dbg !1693
  br i1 %132, label %133, label %152, !dbg !1691

133:                                              ; preds = %128, %123
  br i1 false, label %134, label %139, !dbg !1694

134:                                              ; preds = %133
  %135 = load i8, i8* %7, align 1, !dbg !1695
  %136 = zext i8 %135 to i32, !dbg !1695
  %137 = call i32 @isascii(i32 noundef %136) #9, !dbg !1695
  %138 = icmp ne i32 %137, 0, !dbg !1695
  br i1 %138, label %143, label %152, !dbg !1695

139:                                              ; preds = %133
  %140 = load i8, i8* %7, align 1, !dbg !1695
  %141 = zext i8 %140 to i32, !dbg !1695
  %142 = icmp ult i32 %141, 128, !dbg !1695
  br i1 %142, label %143, label %152, !dbg !1694

143:                                              ; preds = %139, %134
  %144 = load i8, i8* %7, align 1, !dbg !1696
  %145 = zext i8 %144 to i32, !dbg !1696
  %146 = sub nsw i32 %145, 97, !dbg !1698
  %147 = add nsw i32 %146, 13, !dbg !1699
  %148 = srem i32 %147, 26, !dbg !1700
  %149 = add nsw i32 97, %148, !dbg !1701
  %150 = trunc i32 %149 to i8, !dbg !1702
  %151 = load i8*, i8** %6, align 4, !dbg !1703
  store i8 %150, i8* %151, align 1, !dbg !1704
  br label %152, !dbg !1705

152:                                              ; preds = %143, %139, %134, %128, %123
  br label %153

153:                                              ; preds = %152, %113
  br label %154, !dbg !1706

154:                                              ; preds = %153
  %155 = load i8*, i8** %6, align 4, !dbg !1707
  %156 = getelementptr inbounds i8, i8* %155, i32 1, !dbg !1707
  store i8* %156, i8** %6, align 4, !dbg !1707
  br label %88, !dbg !1708, !llvm.loop !1709

157:                                              ; preds = %88
  br label %158, !dbg !1711

158:                                              ; preds = %157, %79
  %159 = load i32, i32* %10, align 4, !dbg !1712
  %160 = load i32, i32* @SLEN, align 4, !dbg !1714
  %161 = icmp slt i32 %159, %160, !dbg !1715
  br i1 %161, label %165, label %162, !dbg !1716

162:                                              ; preds = %158
  %163 = load i8, i8* @Short_only, align 1, !dbg !1717
  %164 = trunc i8 %163 to i1, !dbg !1717
  br i1 %164, label %203, label %165, !dbg !1718

165:                                              ; preds = %162, %158
  %166 = load i32, i32* %10, align 4, !dbg !1719
  %167 = load i32, i32* @SLEN, align 4, !dbg !1720
  %168 = icmp sgt i32 %166, %167, !dbg !1721
  br i1 %168, label %172, label %169, !dbg !1722

169:                                              ; preds = %165
  %170 = load i8, i8* @Long_only, align 1, !dbg !1723
  %171 = trunc i8 %170 to i1, !dbg !1723
  br i1 %171, label %203, label %172, !dbg !1724

172:                                              ; preds = %169, %165
  %173 = load i8*, i8** %11, align 4, !dbg !1725
  %174 = call i32 @regexec(%struct.re_pattern_buffer* noundef @Re_pat, i8* noundef %173, i32 noundef 0, %struct.regmatch_t* noundef null, i32 noundef 0) #9, !dbg !1725
  %175 = icmp eq i32 %174, 0, !dbg !1725
  br i1 %175, label %176, label %203, !dbg !1726

176:                                              ; preds = %172
  %177 = load i32, i32* %9, align 4, !dbg !1727
  %178 = icmp ne i32 %177, 0, !dbg !1727
  br i1 %178, label %192, label %179, !dbg !1730

179:                                              ; preds = %176
  %180 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1731
  %181 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1733
  %182 = getelementptr inbounds %struct.fd, %struct.fd* %181, i32 0, i32 5, !dbg !1734
  %183 = load i8*, i8** %182, align 4, !dbg !1734
  %184 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1735
  %185 = getelementptr inbounds %struct.fd, %struct.fd* %184, i32 0, i32 12, !dbg !1736
  %186 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %185, i32 0, i32 5, !dbg !1737
  %187 = getelementptr inbounds [4 x i8], [4 x i8]* %186, i32 0, i32 0, !dbg !1735
  %188 = load i8, i8* %187, align 4, !dbg !1735
  %189 = zext i8 %188 to i32, !dbg !1735
  %190 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %180, i8* noundef getelementptr inbounds ([9 x i8], [9 x i8]* @.str.57, i32 0, i32 0), i8* noundef %183, i32 noundef %189) #9, !dbg !1738
  %191 = load i8*, i8** %4, align 4, !dbg !1739
  store i8 1, i8* %191, align 1, !dbg !1740
  store i32 1, i32* %9, align 4, !dbg !1741
  br label %192, !dbg !1742

192:                                              ; preds = %179, %176
  %193 = load i8*, i8** %11, align 4, !dbg !1743
  %194 = load %struct._IO_FILE*, %struct._IO_FILE** @stdout, align 4, !dbg !1744
  %195 = call i32 @fputs(i8* noundef %193, %struct._IO_FILE* noundef %194) #9, !dbg !1745
  %196 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1746
  %197 = getelementptr inbounds %struct.fd, %struct.fd* %196, i32 0, i32 12, !dbg !1747
  %198 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %197, i32 0, i32 5, !dbg !1748
  %199 = getelementptr inbounds [4 x i8], [4 x i8]* %198, i32 0, i32 0, !dbg !1746
  %200 = load i8, i8* %199, align 4, !dbg !1746
  %201 = zext i8 %200 to i32, !dbg !1746
  %202 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.58, i32 0, i32 0), i32 noundef %201) #9, !dbg !1749
  br label %203, !dbg !1750

203:                                              ; preds = %192, %172, %169, %162
  %204 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1751
  %205 = getelementptr inbounds %struct.fd, %struct.fd* %204, i32 0, i32 11, !dbg !1753
  %206 = load i8, i8* %205, align 2, !dbg !1753
  %207 = trunc i8 %206 to i1, !dbg !1753
  br i1 %207, label %208, label %213, !dbg !1754

208:                                              ; preds = %203
  %209 = load i8, i8* @No_recode, align 1, !dbg !1755
  %210 = trunc i8 %209 to i1, !dbg !1755
  br i1 %210, label %213, label %211, !dbg !1756

211:                                              ; preds = %208
  %212 = load i8*, i8** %11, align 4, !dbg !1757
  call void @free(i8* noundef %212) #9, !dbg !1759
  store i8* null, i8** %11, align 4, !dbg !1760
  br label %213, !dbg !1761

213:                                              ; preds = %211, %208, %203
  %214 = load i8*, i8** @Fortbuf, align 4, !dbg !1762
  store i8* %214, i8** %5, align 4, !dbg !1763
  br label %215

215:                                              ; preds = %213, %55
  br label %29, !dbg !1623, !llvm.loop !1764

216:                                              ; preds = %29
  br label %217, !dbg !1766

217:                                              ; preds = %216, %21
  %218 = load %struct.fd*, %struct.fd** %8, align 4, !dbg !1767
  %219 = getelementptr inbounds %struct.fd, %struct.fd* %218, i32 0, i32 16, !dbg !1768
  %220 = load %struct.fd*, %struct.fd** %219, align 4, !dbg !1768
  store %struct.fd* %220, %struct.fd** %8, align 4, !dbg !1769
  br label %13, !dbg !1770, !llvm.loop !1771

221:                                              ; preds = %13
  ret void, !dbg !1773
}

declare i32 @regexec(%struct.re_pattern_buffer* noundef, i8* noundef, i32 noundef, %struct.regmatch_t* noundef, i32 noundef) #2

declare noalias i8* @malloc(i32 noundef) #2

declare i32 @getopt(i32 noundef, i8** noundef, i8* noundef) #2

declare i32 @atoi(i8* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i8* @program_version() #0 !dbg !94 {
  %1 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef getelementptr inbounds ([1024 x i8], [1024 x i8]* @program_version.buf, i32 0, i32 0), i32 noundef 1024, i8* noundef getelementptr inbounds ([14 x i8], [14 x i8]* @.str.10, i32 0, i32 0), i8* noundef getelementptr inbounds ([12 x i8], [12 x i8]* @.str.11, i32 0, i32 0), i8* noundef getelementptr inbounds ([7 x i8], [7 x i8]* @.str.12, i32 0, i32 0)) #9, !dbg !1774
  ret i8* getelementptr inbounds ([1024 x i8], [1024 x i8]* @program_version.buf, i32 0, i32 0), !dbg !1775
}

; Function Attrs: noinline noreturn nounwind optnone
define internal void @usage() #7 !dbg !1776 {
  %1 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1777
  %2 = call i8* @program_version() #9, !dbg !1778
  %3 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %1, i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.6, i32 0, i32 0), i8* noundef %2) #9, !dbg !1779
  %4 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1780
  %5 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %4, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([12 x i8], [12 x i8]* @.str.13, i32 0, i32 0)) #9, !dbg !1781
  %6 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1782
  %7 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %6, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([2 x i8], [2 x i8]* @.str.14, i32 0, i32 0)) #9, !dbg !1783
  %8 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1784
  %9 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %8, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([2 x i8], [2 x i8]* @.str.15, i32 0, i32 0)) #9, !dbg !1785
  %10 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1786
  %11 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %10, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([2 x i8], [2 x i8]* @.str.16, i32 0, i32 0)) #9, !dbg !1787
  %12 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1788
  %13 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %12, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([2 x i8], [2 x i8]* @.str.17, i32 0, i32 0)) #9, !dbg !1789
  %14 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1790
  %15 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %14, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.18, i32 0, i32 0)) #9, !dbg !1791
  %16 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1792
  %17 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %16, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([14 x i8], [14 x i8]* @.str.19, i32 0, i32 0)) #9, !dbg !1793
  %18 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1794
  %19 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %18, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([41 x i8], [41 x i8]* @.str.20, i32 0, i32 0)) #9, !dbg !1795
  call void @exit(i32 noundef 1) #10, !dbg !1796
  unreachable, !dbg !1796
}

; Function Attrs: noinline nounwind optnone
define internal i32 @form_file_list(i8** noundef %0, i32 noundef %1) #0 !dbg !1797 {
  %3 = alloca i32, align 4
  %4 = alloca i8**, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i8*, align 4
  %9 = alloca [1024 x i8], align 16
  %10 = alloca [512 x i8], align 16
  %11 = alloca [512 x i8], align 16
  %12 = alloca i8*, align 4
  %13 = alloca [512 x i8], align 16
  %14 = alloca i32, align 4
  %15 = alloca i8*, align 4
  %16 = alloca [3 x i8], align 1
  %17 = alloca i32, align 4
  %18 = alloca i8, align 1
  %19 = alloca i8, align 1
  %20 = alloca i32, align 4
  %21 = alloca i8*, align 4
  %22 = alloca i8*, align 4
  %23 = alloca [512 x i8], align 16
  %24 = alloca i32, align 4
  %25 = alloca i8*, align 4
  %26 = alloca i8*, align 4
  %27 = alloca [3 x i8], align 1
  %28 = alloca %struct.fd*, align 4
  %29 = alloca %struct.fd*, align 4
  %30 = alloca %struct.fd*, align 4
  %31 = alloca %struct.fd*, align 4
  %32 = alloca %struct.fd*, align 4
  %33 = alloca %struct.fd*, align 4
  store i8** %0, i8*** %4, align 4
  call void @llvm.dbg.declare(metadata i8*** %4, metadata !1800, metadata !DIExpression()), !dbg !1801
  store i32 %1, i32* %5, align 4
  call void @llvm.dbg.declare(metadata i32* %5, metadata !1802, metadata !DIExpression()), !dbg !1803
  call void @llvm.dbg.declare(metadata i32* %6, metadata !1804, metadata !DIExpression()), !dbg !1805
  call void @llvm.dbg.declare(metadata i32* %7, metadata !1806, metadata !DIExpression()), !dbg !1807
  call void @llvm.dbg.declare(metadata i8** %8, metadata !1808, metadata !DIExpression()), !dbg !1809
  call void @llvm.dbg.declare(metadata [1024 x i8]* %9, metadata !1810, metadata !DIExpression()), !dbg !1811
  call void @llvm.dbg.declare(metadata [512 x i8]* %10, metadata !1812, metadata !DIExpression()), !dbg !1816
  call void @llvm.dbg.declare(metadata [512 x i8]* %11, metadata !1817, metadata !DIExpression()), !dbg !1818
  %34 = load i32, i32* %5, align 4, !dbg !1819
  %35 = icmp eq i32 %34, 0, !dbg !1821
  br i1 %35, label %36, label %104, !dbg !1822

36:                                               ; preds = %2
  %37 = load i8, i8* @All_forts, align 1, !dbg !1823
  %38 = trunc i8 %37 to i1, !dbg !1823
  br i1 %38, label %39, label %47, !dbg !1826

39:                                               ; preds = %36
  %40 = call i32 @top_level__add_file(i8* noundef getelementptr inbounds ([38 x i8], [38 x i8]* @.str.21, i32 0, i32 0)) #9, !dbg !1827
  %41 = call i32 @top_level__add_file(i8* noundef getelementptr inbounds ([42 x i8], [42 x i8]* @.str.22, i32 0, i32 0)) #9, !dbg !1829
  %42 = or i32 %40, %41, !dbg !1830
  %43 = call i32 @cond_top_level__FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR() #9, !dbg !1831
  %44 = or i32 %42, %43, !dbg !1832
  %45 = call i32 @cond_top_level__FORTUNE_SYSTEM_OFFENSIVE_FORTUNES_DIR() #9, !dbg !1833
  %46 = or i32 %44, %45, !dbg !1834
  store i32 %46, i32* %3, align 4, !dbg !1835
  br label %479, !dbg !1835

47:                                               ; preds = %36
  %48 = load i8, i8* @Offend, align 1, !dbg !1836
  %49 = trunc i8 %48 to i1, !dbg !1836
  br i1 %49, label %50, label %54, !dbg !1838

50:                                               ; preds = %47
  %51 = call i32 @top_level__add_file(i8* noundef getelementptr inbounds ([42 x i8], [42 x i8]* @.str.22, i32 0, i32 0)) #9, !dbg !1839
  %52 = call i32 @cond_top_level__FORTUNE_SYSTEM_OFFENSIVE_FORTUNES_DIR() #9, !dbg !1841
  %53 = or i32 %51, %52, !dbg !1842
  store i32 %53, i32* %3, align 4, !dbg !1843
  br label %479, !dbg !1843

54:                                               ; preds = %47
  %55 = load i8*, i8** @env_lang, align 4, !dbg !1844
  %56 = icmp ne i8* %55, null, !dbg !1844
  br i1 %56, label %57, label %102, !dbg !1847

57:                                               ; preds = %54
  call void @llvm.dbg.declare(metadata i8** %12, metadata !1848, metadata !DIExpression()), !dbg !1850
  call void @llvm.dbg.declare(metadata [512 x i8]* %13, metadata !1851, metadata !DIExpression()), !dbg !1852
  call void @llvm.dbg.declare(metadata i32* %14, metadata !1853, metadata !DIExpression()), !dbg !1854
  store i32 0, i32* %14, align 4, !dbg !1854
  call void @llvm.dbg.declare(metadata i8** %15, metadata !1855, metadata !DIExpression()), !dbg !1856
  %58 = getelementptr inbounds [512 x i8], [512 x i8]* %13, i32 0, i32 0, !dbg !1857
  %59 = load i8*, i8** @env_lang, align 4, !dbg !1858
  %60 = call i8* @strncpy(i8* noundef %58, i8* noundef %59, i32 noundef 512) #9, !dbg !1859
  %61 = getelementptr inbounds [512 x i8], [512 x i8]* %13, i32 0, i32 511, !dbg !1860
  store i8 0, i8* %61, align 1, !dbg !1861
  %62 = getelementptr inbounds [512 x i8], [512 x i8]* %13, i32 0, i32 0, !dbg !1862
  store i8* %62, i8** %12, align 4, !dbg !1863
  br label %63, !dbg !1864

63:                                               ; preds = %98, %57
  %64 = load i8*, i8** %12, align 4, !dbg !1865
  %65 = icmp ne i8* %64, null, !dbg !1865
  br i1 %65, label %66, label %71, !dbg !1866

66:                                               ; preds = %63
  %67 = load i8*, i8** %12, align 4, !dbg !1867
  %68 = load i8, i8* %67, align 1, !dbg !1868
  %69 = sext i8 %68 to i32, !dbg !1869
  %70 = icmp ne i32 %69, 0, !dbg !1866
  br label %71

71:                                               ; preds = %66, %63
  %72 = phi i1 [ false, %63 ], [ %70, %66 ], !dbg !1870
  br i1 %72, label %73, label %100, !dbg !1864

73:                                               ; preds = %71
  %74 = load i8*, i8** %12, align 4, !dbg !1871
  %75 = call i8* @strchr(i8* noundef %74, i32 noundef 58) #9, !dbg !1873
  store i8* %75, i8** %15, align 4, !dbg !1874
  %76 = load i8*, i8** %15, align 4, !dbg !1875
  %77 = icmp ne i8* %76, null, !dbg !1875
  br i1 %77, label %78, label %81, !dbg !1877

78:                                               ; preds = %73
  %79 = load i8*, i8** %15, align 4, !dbg !1878
  %80 = getelementptr inbounds i8, i8* %79, i32 1, !dbg !1878
  store i8* %80, i8** %15, align 4, !dbg !1878
  store i8 0, i8* %79, align 1, !dbg !1880
  br label %81, !dbg !1881

81:                                               ; preds = %78, %73
  %82 = load i8*, i8** %12, align 4, !dbg !1882
  %83 = call i32 @add_file(i32 noundef -1, i8* noundef %82, i8* noundef null, %struct.fd** noundef @File_list, %struct.fd** noundef @File_tail, %struct.fd* noundef null) #9, !dbg !1883
  store i32 %83, i32* %14, align 4, !dbg !1884
  %84 = load i32, i32* %14, align 4, !dbg !1885
  %85 = icmp ne i32 %84, 0, !dbg !1885
  br i1 %85, label %93, label %86, !dbg !1887

86:                                               ; preds = %81
  call void @llvm.dbg.declare(metadata [3 x i8]* %16, metadata !1888, metadata !DIExpression()), !dbg !1893
  %87 = getelementptr inbounds [3 x i8], [3 x i8]* %16, i32 0, i32 0, !dbg !1894
  %88 = load i8*, i8** %12, align 4, !dbg !1895
  %89 = call i8* @strncpy(i8* noundef %87, i8* noundef %88, i32 noundef 2) #9, !dbg !1896
  %90 = getelementptr inbounds [3 x i8], [3 x i8]* %16, i32 0, i32 2, !dbg !1897
  store i8 0, i8* %90, align 1, !dbg !1898
  %91 = getelementptr inbounds [3 x i8], [3 x i8]* %16, i32 0, i32 0, !dbg !1899
  %92 = call i32 @add_file(i32 noundef -1, i8* noundef %91, i8* noundef null, %struct.fd** noundef @File_list, %struct.fd** noundef @File_tail, %struct.fd* noundef null) #9, !dbg !1900
  store i32 %92, i32* %14, align 4, !dbg !1901
  br label %93, !dbg !1902

93:                                               ; preds = %86, %81
  %94 = load i32, i32* %14, align 4, !dbg !1903
  %95 = icmp ne i32 %94, 0, !dbg !1903
  br i1 %95, label %96, label %98, !dbg !1905

96:                                               ; preds = %93
  %97 = load i32, i32* %14, align 4, !dbg !1906
  store i32 %97, i32* %3, align 4, !dbg !1908
  br label %479, !dbg !1908

98:                                               ; preds = %93
  %99 = load i8*, i8** %15, align 4, !dbg !1909
  store i8* %99, i8** %12, align 4, !dbg !1910
  br label %63, !dbg !1864, !llvm.loop !1911

100:                                              ; preds = %71
  %101 = call i32 @top_level_FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR() #9, !dbg !1913
  store i32 %101, i32* %3, align 4, !dbg !1914
  br label %479, !dbg !1914

102:                                              ; preds = %54
  %103 = call i32 @top_level_FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR() #9, !dbg !1915
  store i32 %103, i32* %3, align 4, !dbg !1917
  br label %479, !dbg !1917

104:                                              ; preds = %2
  store i32 0, i32* %6, align 4, !dbg !1918
  br label %105, !dbg !1920

105:                                              ; preds = %475, %104
  %106 = load i32, i32* %6, align 4, !dbg !1921
  %107 = load i32, i32* %5, align 4, !dbg !1923
  %108 = icmp slt i32 %106, %107, !dbg !1924
  br i1 %108, label %109, label %478, !dbg !1925

109:                                              ; preds = %105
  store i32 -1, i32* %7, align 4, !dbg !1926
  store i8* null, i8** %8, align 4, !dbg !1928
  br i1 false, label %110, label %120, !dbg !1929

110:                                              ; preds = %109
  %111 = load i8**, i8*** %4, align 4, !dbg !1930
  %112 = load i32, i32* %6, align 4, !dbg !1930
  %113 = getelementptr inbounds i8*, i8** %111, i32 %112, !dbg !1930
  %114 = load i8*, i8** %113, align 4, !dbg !1930
  %115 = getelementptr inbounds i8, i8* %114, i32 0, !dbg !1930
  %116 = load i8, i8* %115, align 1, !dbg !1930
  %117 = sext i8 %116 to i32, !dbg !1930
  %118 = call i32 @isdigit(i32 noundef %117) #9, !dbg !1930
  %119 = icmp ne i32 %118, 0, !dbg !1930
  br i1 %119, label %135, label %130, !dbg !1930

120:                                              ; preds = %109
  %121 = load i8**, i8*** %4, align 4, !dbg !1930
  %122 = load i32, i32* %6, align 4, !dbg !1930
  %123 = getelementptr inbounds i8*, i8** %121, i32 %122, !dbg !1930
  %124 = load i8*, i8** %123, align 4, !dbg !1930
  %125 = getelementptr inbounds i8, i8* %124, i32 0, !dbg !1930
  %126 = load i8, i8* %125, align 1, !dbg !1930
  %127 = sext i8 %126 to i32, !dbg !1930
  %128 = sub i32 %127, 48, !dbg !1930
  %129 = icmp ult i32 %128, 10, !dbg !1930
  br i1 %129, label %135, label %130, !dbg !1929

130:                                              ; preds = %120, %110
  %131 = load i8**, i8*** %4, align 4, !dbg !1932
  %132 = load i32, i32* %6, align 4, !dbg !1934
  %133 = getelementptr inbounds i8*, i8** %131, i32 %132, !dbg !1932
  %134 = load i8*, i8** %133, align 4, !dbg !1932
  store i8* %134, i8** %8, align 4, !dbg !1935
  br label %228, !dbg !1936

135:                                              ; preds = %120, %110
  call void @llvm.dbg.declare(metadata i32* %17, metadata !1937, metadata !DIExpression()), !dbg !1940
  store i32 100, i32* %17, align 4, !dbg !1940
  call void @llvm.dbg.declare(metadata i8* %18, metadata !1941, metadata !DIExpression()), !dbg !1942
  store i8 0, i8* %18, align 1, !dbg !1942
  store i32 0, i32* %7, align 4, !dbg !1943
  %136 = load i8**, i8*** %4, align 4, !dbg !1944
  %137 = load i32, i32* %6, align 4, !dbg !1946
  %138 = getelementptr inbounds i8*, i8** %136, i32 %137, !dbg !1944
  %139 = load i8*, i8** %138, align 4, !dbg !1944
  store i8* %139, i8** %8, align 4, !dbg !1947
  br label %140, !dbg !1948

140:                                              ; preds = %161, %135
  %141 = load i8*, i8** %8, align 4, !dbg !1949
  %142 = load i8, i8* %141, align 1, !dbg !1949
  %143 = sext i8 %142 to i32, !dbg !1949
  %144 = sub i32 %143, 48, !dbg !1949
  %145 = icmp ult i32 %144, 10, !dbg !1949
  br i1 %145, label %146, label %164, !dbg !1951

146:                                              ; preds = %140
  %147 = load i32, i32* %7, align 4, !dbg !1952
  %148 = mul nsw i32 %147, 10, !dbg !1954
  %149 = load i8*, i8** %8, align 4, !dbg !1955
  %150 = load i8, i8* %149, align 1, !dbg !1956
  %151 = sext i8 %150 to i32, !dbg !1956
  %152 = add nsw i32 %148, %151, !dbg !1957
  %153 = sub nsw i32 %152, 48, !dbg !1958
  store i32 %153, i32* %7, align 4, !dbg !1959
  %154 = load i32, i32* %7, align 4, !dbg !1960
  %155 = icmp sgt i32 %154, 100, !dbg !1961
  %156 = zext i1 %155 to i8, !dbg !1962
  store i8 %156, i8* %18, align 1, !dbg !1962
  %157 = load i8, i8* %18, align 1, !dbg !1963
  %158 = trunc i8 %157 to i1, !dbg !1963
  br i1 %158, label %159, label %160, !dbg !1965

159:                                              ; preds = %146
  br label %164, !dbg !1966

160:                                              ; preds = %146
  br label %161, !dbg !1968

161:                                              ; preds = %160
  %162 = load i8*, i8** %8, align 4, !dbg !1969
  %163 = getelementptr inbounds i8, i8* %162, i32 1, !dbg !1969
  store i8* %163, i8** %8, align 4, !dbg !1969
  br label %140, !dbg !1970, !llvm.loop !1971

164:                                              ; preds = %159, %140
  %165 = load i8, i8* %18, align 1, !dbg !1973
  %166 = trunc i8 %165 to i1, !dbg !1973
  br i1 %166, label %170, label %167, !dbg !1975

167:                                              ; preds = %164
  %168 = load i32, i32* %7, align 4, !dbg !1976
  %169 = icmp sgt i32 %168, 100, !dbg !1977
  br i1 %169, label %170, label %179, !dbg !1978

170:                                              ; preds = %167, %164
  %171 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1979
  %172 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %171, i8* noundef getelementptr inbounds ([28 x i8], [28 x i8]* @.str.23, i32 0, i32 0)) #9, !dbg !1981
  %173 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1982
  %174 = load i8**, i8*** %4, align 4, !dbg !1983
  %175 = load i32, i32* %6, align 4, !dbg !1984
  %176 = getelementptr inbounds i8*, i8** %174, i32 %175, !dbg !1983
  %177 = load i8*, i8** %176, align 4, !dbg !1983
  %178 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %173, i8* noundef getelementptr inbounds ([48 x i8], [48 x i8]* @.str.24, i32 0, i32 0), i8* noundef %177) #9, !dbg !1985
  store i8 1, i8* @ErrorMessage, align 1, !dbg !1986
  store i32 0, i32* %3, align 4, !dbg !1987
  br label %479, !dbg !1987

179:                                              ; preds = %167
  %180 = load i32, i32* %7, align 4, !dbg !1988
  %181 = icmp slt i32 %180, 0, !dbg !1990
  br i1 %181, label %182, label %189, !dbg !1991

182:                                              ; preds = %179
  %183 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !1992
  %184 = load i8**, i8*** %4, align 4, !dbg !1994
  %185 = load i32, i32* %6, align 4, !dbg !1995
  %186 = getelementptr inbounds i8*, i8** %184, i32 %185, !dbg !1994
  %187 = load i8*, i8** %186, align 4, !dbg !1994
  %188 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %183, i8* noundef getelementptr inbounds ([48 x i8], [48 x i8]* @.str.24, i32 0, i32 0), i8* noundef %187) #9, !dbg !1996
  store i8 1, i8* @ErrorMessage, align 1, !dbg !1997
  store i32 0, i32* %3, align 4, !dbg !1998
  br label %479, !dbg !1998

189:                                              ; preds = %179
  %190 = load i8*, i8** %8, align 4, !dbg !1999
  %191 = load i8, i8* %190, align 1, !dbg !2001
  %192 = sext i8 %191 to i32, !dbg !2001
  %193 = icmp eq i32 %192, 46, !dbg !2002
  br i1 %193, label %194, label %197, !dbg !2003

194:                                              ; preds = %189
  %195 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !2004
  %196 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %195, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([30 x i8], [30 x i8]* @.str.25, i32 0, i32 0)) #9, !dbg !2006
  store i8 1, i8* @ErrorMessage, align 1, !dbg !2007
  store i32 0, i32* %3, align 4, !dbg !2008
  br label %479, !dbg !2008

197:                                              ; preds = %189
  %198 = load i8*, i8** %8, align 4, !dbg !2009
  %199 = load i8, i8* %198, align 1, !dbg !2011
  %200 = sext i8 %199 to i32, !dbg !2011
  %201 = icmp ne i32 %200, 37, !dbg !2012
  br i1 %201, label %202, label %207, !dbg !2013

202:                                              ; preds = %197
  store i32 -1, i32* %7, align 4, !dbg !2014
  %203 = load i8**, i8*** %4, align 4, !dbg !2016
  %204 = load i32, i32* %6, align 4, !dbg !2017
  %205 = getelementptr inbounds i8*, i8** %203, i32 %204, !dbg !2016
  %206 = load i8*, i8** %205, align 4, !dbg !2016
  store i8* %206, i8** %8, align 4, !dbg !2018
  br label %227, !dbg !2019

207:                                              ; preds = %197
  %208 = load i8*, i8** %8, align 4, !dbg !2020
  %209 = getelementptr inbounds i8, i8* %208, i32 1, !dbg !2020
  store i8* %209, i8** %8, align 4, !dbg !2020
  %210 = load i8, i8* %209, align 1, !dbg !2022
  %211 = sext i8 %210 to i32, !dbg !2022
  %212 = icmp eq i32 %211, 0, !dbg !2023
  br i1 %212, label %213, label %226, !dbg !2024

213:                                              ; preds = %207
  %214 = load i32, i32* %6, align 4, !dbg !2025
  %215 = add nsw i32 %214, 1, !dbg !2025
  store i32 %215, i32* %6, align 4, !dbg !2025
  %216 = load i32, i32* %5, align 4, !dbg !2028
  %217 = icmp sge i32 %215, %216, !dbg !2029
  br i1 %217, label %218, label %221, !dbg !2030

218:                                              ; preds = %213
  %219 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !2031
  %220 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %219, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([32 x i8], [32 x i8]* @.str.26, i32 0, i32 0)) #9, !dbg !2033
  store i8 1, i8* @ErrorMessage, align 1, !dbg !2034
  store i32 0, i32* %3, align 4, !dbg !2035
  br label %479, !dbg !2035

221:                                              ; preds = %213
  %222 = load i8**, i8*** %4, align 4, !dbg !2036
  %223 = load i32, i32* %6, align 4, !dbg !2037
  %224 = getelementptr inbounds i8*, i8** %222, i32 %223, !dbg !2036
  %225 = load i8*, i8** %224, align 4, !dbg !2036
  store i8* %225, i8** %8, align 4, !dbg !2038
  br label %226, !dbg !2039

226:                                              ; preds = %221, %207
  br label %227

227:                                              ; preds = %226, %202
  br label %228

228:                                              ; preds = %227, %130
  call void @llvm.dbg.declare(metadata i8* %19, metadata !2040, metadata !DIExpression()), !dbg !2041
  store i8 0, i8* %19, align 1, !dbg !2041
  call void @llvm.dbg.declare(metadata i32* %20, metadata !2042, metadata !DIExpression()), !dbg !2043
  %229 = load i8*, i8** %8, align 4, !dbg !2044
  %230 = call i32 @strlen(i8* noundef %229) #11, !dbg !2045
  store i32 %230, i32* %20, align 4, !dbg !2043
  %231 = load i32, i32* %20, align 4, !dbg !2046
  %232 = icmp uge i32 %231, 3, !dbg !2048
  br i1 %232, label %233, label %254, !dbg !2049

233:                                              ; preds = %228
  %234 = load i8*, i8** %8, align 4, !dbg !2050
  %235 = load i32, i32* %20, align 4, !dbg !2051
  %236 = sub i32 %235, 2, !dbg !2052
  %237 = getelementptr inbounds i8, i8* %234, i32 %236, !dbg !2050
  %238 = load i8, i8* %237, align 1, !dbg !2050
  %239 = sext i8 %238 to i32, !dbg !2050
  %240 = icmp eq i32 %239, 45, !dbg !2053
  br i1 %240, label %241, label %254, !dbg !2054

241:                                              ; preds = %233
  %242 = load i8*, i8** %8, align 4, !dbg !2055
  %243 = load i32, i32* %20, align 4, !dbg !2056
  %244 = sub i32 %243, 1, !dbg !2057
  %245 = getelementptr inbounds i8, i8* %242, i32 %244, !dbg !2055
  %246 = load i8, i8* %245, align 1, !dbg !2055
  %247 = sext i8 %246 to i32, !dbg !2055
  %248 = icmp eq i32 %247, 111, !dbg !2058
  br i1 %248, label %249, label %254, !dbg !2059

249:                                              ; preds = %241
  %250 = load i8*, i8** %8, align 4, !dbg !2060
  %251 = load i32, i32* %20, align 4, !dbg !2062
  %252 = sub i32 %251, 2, !dbg !2063
  %253 = getelementptr inbounds i8, i8* %250, i32 %252, !dbg !2060
  store i8 0, i8* %253, align 1, !dbg !2064
  store i8 1, i8* %19, align 1, !dbg !2065
  br label %254, !dbg !2066

254:                                              ; preds = %249, %241, %233, %228
  call void @llvm.dbg.declare(metadata i8** %21, metadata !2067, metadata !DIExpression()), !dbg !2068
  %255 = load i8, i8* %19, align 1, !dbg !2069
  %256 = trunc i8 %255 to i1, !dbg !2069
  %257 = zext i1 %256 to i64, !dbg !2069
  %258 = select i1 %256, i8* getelementptr inbounds ([36 x i8], [36 x i8]* @.str.27, i32 0, i32 0), i8* getelementptr inbounds ([32 x i8], [32 x i8]* @.str.28, i32 0, i32 0), !dbg !2069
  store i8* %258, i8** %21, align 4, !dbg !2068
  call void @llvm.dbg.declare(metadata i8** %22, metadata !2070, metadata !DIExpression()), !dbg !2071
  %259 = load i8, i8* %19, align 1, !dbg !2072
  %260 = trunc i8 %259 to i1, !dbg !2072
  %261 = zext i1 %260 to i64, !dbg !2072
  %262 = select i1 %260, i8* getelementptr inbounds ([42 x i8], [42 x i8]* @.str.22, i32 0, i32 0), i8* getelementptr inbounds ([38 x i8], [38 x i8]* @.str.21, i32 0, i32 0), !dbg !2072
  store i8* %262, i8** %22, align 4, !dbg !2071
  %263 = load i8*, i8** %8, align 4, !dbg !2073
  %264 = call i32 @strcmp(i8* noundef %263, i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.29, i32 0, i32 0)) #13, !dbg !2075
  %265 = icmp eq i32 %264, 0, !dbg !2076
  br i1 %265, label %266, label %273, !dbg !2077

266:                                              ; preds = %254
  %267 = getelementptr inbounds [512 x i8], [512 x i8]* %10, i32 0, i32 0, !dbg !2078
  %268 = load i8*, i8** %21, align 4, !dbg !2080
  %269 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %267, i32 noundef 512, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef %268) #9, !dbg !2081
  %270 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2082
  %271 = load i8*, i8** %22, align 4, !dbg !2083
  %272 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %270, i32 noundef 512, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef %271) #9, !dbg !2084
  br label %302, !dbg !2085

273:                                              ; preds = %254
  %274 = load i8*, i8** %8, align 4, !dbg !2086
  %275 = call i32 @strncmp(i8* noundef %274, i8* noundef getelementptr inbounds ([2 x i8], [2 x i8]* @.str.30, i32 0, i32 0), i32 noundef 1) #9, !dbg !2089
  %276 = icmp ne i32 %275, 0, !dbg !2090
  br i1 %276, label %277, label %294, !dbg !2091

277:                                              ; preds = %273
  %278 = load i8*, i8** %8, align 4, !dbg !2092
  %279 = call i32 @strncmp(i8* noundef %278, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.31, i32 0, i32 0), i32 noundef 2) #9, !dbg !2093
  %280 = icmp ne i32 %279, 0, !dbg !2094
  br i1 %280, label %281, label %294, !dbg !2095

281:                                              ; preds = %277
  %282 = load i8*, i8** %8, align 4, !dbg !2096
  %283 = call i32 @strncmp(i8* noundef %282, i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.32, i32 0, i32 0), i32 noundef 3) #9, !dbg !2097
  %284 = icmp ne i32 %283, 0, !dbg !2098
  br i1 %284, label %285, label %294, !dbg !2099

285:                                              ; preds = %281
  %286 = getelementptr inbounds [512 x i8], [512 x i8]* %10, i32 0, i32 0, !dbg !2100
  %287 = load i8*, i8** %21, align 4, !dbg !2102
  %288 = load i8*, i8** %8, align 4, !dbg !2103
  %289 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %286, i32 noundef 512, i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str.33, i32 0, i32 0), i8* noundef %287, i8* noundef %288) #9, !dbg !2104
  %290 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2105
  %291 = load i8*, i8** %22, align 4, !dbg !2106
  %292 = load i8*, i8** %8, align 4, !dbg !2107
  %293 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %290, i32 noundef 512, i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str.33, i32 0, i32 0), i8* noundef %291, i8* noundef %292) #9, !dbg !2108
  br label %301, !dbg !2109

294:                                              ; preds = %281, %277, %273
  %295 = getelementptr inbounds [512 x i8], [512 x i8]* %10, i32 0, i32 0, !dbg !2110
  %296 = load i8*, i8** %8, align 4, !dbg !2112
  %297 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %295, i32 noundef 512, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef %296) #9, !dbg !2113
  %298 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2114
  %299 = load i8*, i8** %8, align 4, !dbg !2115
  %300 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %298, i32 noundef 512, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef %299) #9, !dbg !2116
  br label %301

301:                                              ; preds = %294, %285
  br label %302

302:                                              ; preds = %301, %266
  %303 = load i8*, i8** @env_lang, align 4, !dbg !2117
  %304 = icmp ne i8* %303, null, !dbg !2117
  br i1 %304, label %305, label %467, !dbg !2119

305:                                              ; preds = %302
  call void @llvm.dbg.declare(metadata [512 x i8]* %23, metadata !2120, metadata !DIExpression()), !dbg !2122
  call void @llvm.dbg.declare(metadata i32* %24, metadata !2123, metadata !DIExpression()), !dbg !2124
  store i32 0, i32* %24, align 4, !dbg !2124
  %306 = getelementptr inbounds [512 x i8], [512 x i8]* %23, i32 0, i32 0, !dbg !2125
  %307 = load i8*, i8** @env_lang, align 4, !dbg !2126
  %308 = call i8* @strncpy(i8* noundef %306, i8* noundef %307, i32 noundef 512) #9, !dbg !2127
  %309 = getelementptr inbounds [512 x i8], [512 x i8]* %23, i32 0, i32 511, !dbg !2128
  store i8 0, i8* %309, align 1, !dbg !2129
  call void @llvm.dbg.declare(metadata i8** %25, metadata !2130, metadata !DIExpression()), !dbg !2131
  %310 = getelementptr inbounds [512 x i8], [512 x i8]* %23, i32 0, i32 0, !dbg !2132
  store i8* %310, i8** %25, align 4, !dbg !2131
  br label %311, !dbg !2133

311:                                              ; preds = %356, %305
  %312 = load i32, i32* %24, align 4, !dbg !2134
  %313 = icmp ne i32 %312, 0, !dbg !2134
  br i1 %313, label %322, label %314, !dbg !2135

314:                                              ; preds = %311
  %315 = load i8*, i8** %25, align 4, !dbg !2136
  %316 = icmp ne i8* %315, null, !dbg !2136
  br i1 %316, label %317, label %322, !dbg !2137

317:                                              ; preds = %314
  %318 = load i8*, i8** %25, align 4, !dbg !2138
  %319 = load i8, i8* %318, align 1, !dbg !2139
  %320 = sext i8 %319 to i32, !dbg !2140
  %321 = icmp ne i32 %320, 0, !dbg !2137
  br label %322

322:                                              ; preds = %317, %314, %311
  %323 = phi i1 [ false, %314 ], [ false, %311 ], [ %321, %317 ], !dbg !2141
  br i1 %323, label %324, label %358, !dbg !2133

324:                                              ; preds = %322
  call void @llvm.dbg.declare(metadata i8** %26, metadata !2142, metadata !DIExpression()), !dbg !2144
  %325 = load i8*, i8** %25, align 4, !dbg !2145
  %326 = call i8* @strchr(i8* noundef %325, i32 noundef 58) #9, !dbg !2146
  store i8* %326, i8** %26, align 4, !dbg !2144
  %327 = load i8*, i8** %26, align 4, !dbg !2147
  %328 = icmp ne i8* %327, null, !dbg !2147
  br i1 %328, label %329, label %332, !dbg !2149

329:                                              ; preds = %324
  %330 = load i8*, i8** %26, align 4, !dbg !2150
  %331 = getelementptr inbounds i8, i8* %330, i32 1, !dbg !2150
  store i8* %331, i8** %26, align 4, !dbg !2150
  store i8 0, i8* %330, align 1, !dbg !2152
  br label %332, !dbg !2153

332:                                              ; preds = %329, %324
  %333 = getelementptr inbounds [1024 x i8], [1024 x i8]* %9, i32 0, i32 0, !dbg !2154
  %334 = load i8*, i8** %21, align 4, !dbg !2155
  %335 = load i8*, i8** %25, align 4, !dbg !2156
  %336 = load i8*, i8** %8, align 4, !dbg !2157
  %337 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %333, i32 noundef 1024, i8* noundef getelementptr inbounds ([9 x i8], [9 x i8]* @.str.34, i32 0, i32 0), i8* noundef %334, i8* noundef %335, i8* noundef %336) #9, !dbg !2158
  %338 = load i32, i32* %7, align 4, !dbg !2159
  %339 = getelementptr inbounds [1024 x i8], [1024 x i8]* %9, i32 0, i32 0, !dbg !2160
  %340 = call i32 @add_file(i32 noundef %338, i8* noundef %339, i8* noundef null, %struct.fd** noundef @File_list, %struct.fd** noundef @File_tail, %struct.fd* noundef null) #9, !dbg !2161
  store i32 %340, i32* %24, align 4, !dbg !2162
  %341 = load i32, i32* %24, align 4, !dbg !2163
  %342 = icmp ne i32 %341, 0, !dbg !2163
  br i1 %342, label %356, label %343, !dbg !2165

343:                                              ; preds = %332
  call void @llvm.dbg.declare(metadata [3 x i8]* %27, metadata !2166, metadata !DIExpression()), !dbg !2168
  %344 = getelementptr inbounds [3 x i8], [3 x i8]* %27, i32 0, i32 0, !dbg !2169
  %345 = load i8*, i8** %25, align 4, !dbg !2170
  %346 = call i8* @strncpy(i8* noundef %344, i8* noundef %345, i32 noundef 2) #9, !dbg !2171
  %347 = getelementptr inbounds [3 x i8], [3 x i8]* %27, i32 0, i32 2, !dbg !2172
  store i8 0, i8* %347, align 1, !dbg !2173
  %348 = getelementptr inbounds [1024 x i8], [1024 x i8]* %9, i32 0, i32 0, !dbg !2174
  %349 = load i8*, i8** %21, align 4, !dbg !2175
  %350 = getelementptr inbounds [3 x i8], [3 x i8]* %27, i32 0, i32 0, !dbg !2176
  %351 = load i8*, i8** %8, align 4, !dbg !2177
  %352 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %348, i32 noundef 1024, i8* noundef getelementptr inbounds ([9 x i8], [9 x i8]* @.str.34, i32 0, i32 0), i8* noundef %349, i8* noundef %350, i8* noundef %351) #9, !dbg !2178
  %353 = load i32, i32* %7, align 4, !dbg !2179
  %354 = getelementptr inbounds [1024 x i8], [1024 x i8]* %9, i32 0, i32 0, !dbg !2180
  %355 = call i32 @add_file(i32 noundef %353, i8* noundef %354, i8* noundef null, %struct.fd** noundef @File_list, %struct.fd** noundef @File_tail, %struct.fd* noundef null) #9, !dbg !2181
  store i32 %355, i32* %24, align 4, !dbg !2182
  br label %356, !dbg !2183

356:                                              ; preds = %343, %332
  %357 = load i8*, i8** %26, align 4, !dbg !2184
  store i8* %357, i8** %25, align 4, !dbg !2185
  br label %311, !dbg !2133, !llvm.loop !2186

358:                                              ; preds = %322
  call void @llvm.dbg.declare(metadata %struct.fd** %28, metadata !2188, metadata !DIExpression()), !dbg !2189
  store %struct.fd* null, %struct.fd** %28, align 4, !dbg !2189
  call void @llvm.dbg.declare(metadata %struct.fd** %29, metadata !2190, metadata !DIExpression()), !dbg !2191
  store %struct.fd* null, %struct.fd** %29, align 4, !dbg !2191
  call void @llvm.dbg.declare(metadata %struct.fd** %30, metadata !2192, metadata !DIExpression()), !dbg !2193
  store %struct.fd* null, %struct.fd** %30, align 4, !dbg !2193
  call void @llvm.dbg.declare(metadata %struct.fd** %31, metadata !2194, metadata !DIExpression()), !dbg !2195
  store %struct.fd* null, %struct.fd** %31, align 4, !dbg !2195
  %359 = load i32, i32* %24, align 4, !dbg !2196
  %360 = icmp ne i32 %359, 0, !dbg !2196
  br i1 %360, label %364, label %361, !dbg !2198

361:                                              ; preds = %358
  %362 = getelementptr inbounds [512 x i8], [512 x i8]* %10, i32 0, i32 0, !dbg !2199
  %363 = call i32 @add_file(i32 noundef 100, i8* noundef %362, i8* noundef null, %struct.fd** noundef %28, %struct.fd** noundef %30, %struct.fd* noundef null) #9, !dbg !2201
  store i32 %363, i32* %24, align 4, !dbg !2202
  br label %364, !dbg !2203

364:                                              ; preds = %361, %358
  %365 = getelementptr inbounds [512 x i8], [512 x i8]* %10, i32 0, i32 0, !dbg !2204
  %366 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2206
  %367 = call i32 @strncmp(i8* noundef %365, i8* noundef %366, i32 noundef 512) #9, !dbg !2207
  %368 = icmp ne i32 %367, 0, !dbg !2207
  br i1 %368, label %369, label %372, !dbg !2208

369:                                              ; preds = %364
  %370 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2209
  %371 = call i32 @add_file(i32 noundef 100, i8* noundef %370, i8* noundef null, %struct.fd** noundef %29, %struct.fd** noundef %31, %struct.fd* noundef null) #9, !dbg !2211
  br label %372, !dbg !2212

372:                                              ; preds = %369, %364
  %373 = load %struct.fd*, %struct.fd** %29, align 4, !dbg !2213
  %374 = icmp ne %struct.fd* %373, null, !dbg !2213
  br i1 %374, label %399, label %375, !dbg !2215

375:                                              ; preds = %372
  %376 = load %struct.fd*, %struct.fd** %30, align 4, !dbg !2216
  %377 = icmp ne %struct.fd* %376, null, !dbg !2216
  br i1 %377, label %378, label %397, !dbg !2219

378:                                              ; preds = %375
  %379 = load i32, i32* %7, align 4, !dbg !2220
  %380 = load %struct.fd*, %struct.fd** %30, align 4, !dbg !2222
  %381 = getelementptr inbounds %struct.fd, %struct.fd* %380, i32 0, i32 0, !dbg !2223
  store i32 %379, i32* %381, align 4, !dbg !2224
  %382 = load %struct.fd*, %struct.fd** @File_tail, align 4, !dbg !2225
  %383 = icmp ne %struct.fd* %382, null, !dbg !2225
  br i1 %383, label %384, label %393, !dbg !2227

384:                                              ; preds = %378
  call void @llvm.dbg.declare(metadata %struct.fd** %32, metadata !2228, metadata !DIExpression()), !dbg !2230
  %385 = load %struct.fd*, %struct.fd** @File_tail, align 4, !dbg !2231
  store %struct.fd* %385, %struct.fd** %32, align 4, !dbg !2230
  %386 = load %struct.fd*, %struct.fd** @File_list, align 4, !dbg !2232
  %387 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2233
  %388 = getelementptr inbounds %struct.fd, %struct.fd* %387, i32 0, i32 17, !dbg !2234
  store %struct.fd* %386, %struct.fd** %388, align 4, !dbg !2235
  %389 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2236
  %390 = load %struct.fd*, %struct.fd** %32, align 4, !dbg !2237
  %391 = getelementptr inbounds %struct.fd, %struct.fd* %390, i32 0, i32 16, !dbg !2238
  store %struct.fd* %389, %struct.fd** %391, align 4, !dbg !2239
  %392 = load %struct.fd*, %struct.fd** %30, align 4, !dbg !2240
  store %struct.fd* %392, %struct.fd** @File_tail, align 4, !dbg !2241
  br label %396, !dbg !2242

393:                                              ; preds = %378
  %394 = load %struct.fd*, %struct.fd** %30, align 4, !dbg !2243
  store %struct.fd* %394, %struct.fd** @File_tail, align 4, !dbg !2245
  %395 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2246
  store %struct.fd* %395, %struct.fd** @File_list, align 4, !dbg !2247
  br label %396

396:                                              ; preds = %393, %384
  br label %398, !dbg !2248

397:                                              ; preds = %375
  store i32 0, i32* %24, align 4, !dbg !2249
  br label %398

398:                                              ; preds = %397, %396
  br label %443, !dbg !2251

399:                                              ; preds = %372
  %400 = getelementptr inbounds [512 x i8], [512 x i8]* %10, i32 0, i32 0, !dbg !2252
  %401 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2254
  %402 = call i32 @strncmp(i8* noundef %400, i8* noundef %401, i32 noundef 512) #9, !dbg !2255
  %403 = icmp ne i32 %402, 0, !dbg !2255
  br i1 %403, label %404, label %442, !dbg !2256

404:                                              ; preds = %399
  %405 = load i8*, i8** %8, align 4, !dbg !2257
  %406 = call i32 @strcmp(i8* noundef %405, i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.29, i32 0, i32 0)) #13, !dbg !2258
  %407 = icmp eq i32 %406, 0, !dbg !2259
  br i1 %407, label %408, label %442, !dbg !2260

408:                                              ; preds = %404
  %409 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2261
  %410 = icmp ne %struct.fd* %409, null, !dbg !2261
  br i1 %410, label %411, label %440, !dbg !2264

411:                                              ; preds = %408
  call void @llvm.dbg.declare(metadata %struct.fd** %33, metadata !2265, metadata !DIExpression()), !dbg !2267
  %412 = call %struct.fd* @new_fp() #9, !dbg !2268
  store %struct.fd* %412, %struct.fd** %33, align 4, !dbg !2267
  %413 = load i32, i32* %7, align 4, !dbg !2269
  %414 = load %struct.fd*, %struct.fd** %33, align 4, !dbg !2270
  %415 = getelementptr inbounds %struct.fd, %struct.fd* %414, i32 0, i32 0, !dbg !2271
  store i32 %413, i32* %415, align 4, !dbg !2272
  %416 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2273
  %417 = load %struct.fd*, %struct.fd** %33, align 4, !dbg !2274
  %418 = getelementptr inbounds %struct.fd, %struct.fd* %417, i32 0, i32 14, !dbg !2275
  store %struct.fd* %416, %struct.fd** %418, align 4, !dbg !2276
  %419 = load %struct.fd*, %struct.fd** %29, align 4, !dbg !2277
  %420 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2278
  %421 = getelementptr inbounds %struct.fd, %struct.fd* %420, i32 0, i32 16, !dbg !2279
  store %struct.fd* %419, %struct.fd** %421, align 4, !dbg !2280
  %422 = load %struct.fd*, %struct.fd** %29, align 4, !dbg !2281
  %423 = icmp ne %struct.fd* %422, null, !dbg !2281
  br i1 %423, label %424, label %428, !dbg !2283

424:                                              ; preds = %411
  %425 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2284
  %426 = load %struct.fd*, %struct.fd** %29, align 4, !dbg !2286
  %427 = getelementptr inbounds %struct.fd, %struct.fd* %426, i32 0, i32 17, !dbg !2287
  store %struct.fd* %425, %struct.fd** %427, align 4, !dbg !2288
  br label %428, !dbg !2289

428:                                              ; preds = %424, %411
  %429 = load %struct.fd*, %struct.fd** %33, align 4, !dbg !2290
  %430 = load %struct.fd*, %struct.fd** %28, align 4, !dbg !2291
  %431 = getelementptr inbounds %struct.fd, %struct.fd* %430, i32 0, i32 15, !dbg !2292
  store %struct.fd* %429, %struct.fd** %431, align 4, !dbg !2293
  %432 = load %struct.fd*, %struct.fd** %29, align 4, !dbg !2294
  %433 = icmp ne %struct.fd* %432, null, !dbg !2294
  br i1 %433, label %434, label %438, !dbg !2296

434:                                              ; preds = %428
  %435 = load %struct.fd*, %struct.fd** %33, align 4, !dbg !2297
  %436 = load %struct.fd*, %struct.fd** %29, align 4, !dbg !2299
  %437 = getelementptr inbounds %struct.fd, %struct.fd* %436, i32 0, i32 15, !dbg !2300
  store %struct.fd* %435, %struct.fd** %437, align 4, !dbg !2301
  br label %438, !dbg !2302

438:                                              ; preds = %434, %428
  %439 = load %struct.fd*, %struct.fd** %33, align 4, !dbg !2303
  store %struct.fd* %439, %struct.fd** @File_list, align 4, !dbg !2304
  store %struct.fd* %439, %struct.fd** @File_tail, align 4, !dbg !2305
  store i32 1, i32* %24, align 4, !dbg !2306
  br label %441, !dbg !2307

440:                                              ; preds = %408
  store i32 0, i32* %24, align 4, !dbg !2308
  br label %441

441:                                              ; preds = %440, %438
  br label %442, !dbg !2310

442:                                              ; preds = %441, %404, %399
  br label %443

443:                                              ; preds = %442, %398
  %444 = load i32, i32* %24, align 4, !dbg !2311
  %445 = icmp ne i32 %444, 0, !dbg !2311
  br i1 %445, label %462, label %446, !dbg !2313

446:                                              ; preds = %443
  %447 = load i8, i8* %19, align 1, !dbg !2314
  %448 = trunc i8 %447 to i1, !dbg !2314
  br i1 %448, label %449, label %454, !dbg !2317

449:                                              ; preds = %446
  %450 = load i8*, i8** %8, align 4, !dbg !2318
  %451 = load i32, i32* %20, align 4, !dbg !2320
  %452 = sub i32 %451, 2, !dbg !2321
  %453 = getelementptr inbounds i8, i8* %450, i32 %452, !dbg !2318
  store i8 45, i8* %453, align 1, !dbg !2322
  br label %454, !dbg !2323

454:                                              ; preds = %449, %446
  %455 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2324
  %456 = call i8* @getenv(i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.35, i32 0, i32 0)) #9, !dbg !2325
  %457 = load i8*, i8** %8, align 4, !dbg !2326
  %458 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %455, i32 noundef 512, i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str.33, i32 0, i32 0), i8* noundef %456, i8* noundef %457) #9, !dbg !2327
  %459 = load i32, i32* %7, align 4, !dbg !2328
  %460 = getelementptr inbounds [512 x i8], [512 x i8]* %11, i32 0, i32 0, !dbg !2329
  %461 = call i32 @add_file(i32 noundef %459, i8* noundef %460, i8* noundef null, %struct.fd** noundef @File_list, %struct.fd** noundef @File_tail, %struct.fd* noundef null) #9, !dbg !2330
  store i32 %461, i32* %24, align 4, !dbg !2331
  br label %462, !dbg !2332

462:                                              ; preds = %454, %443
  %463 = load i32, i32* %24, align 4, !dbg !2333
  %464 = icmp ne i32 %463, 0, !dbg !2333
  br i1 %464, label %466, label %465, !dbg !2335

465:                                              ; preds = %462
  store i32 0, i32* %3, align 4, !dbg !2336
  br label %479, !dbg !2336

466:                                              ; preds = %462
  br label %474, !dbg !2338

467:                                              ; preds = %302
  %468 = load i32, i32* %7, align 4, !dbg !2339
  %469 = getelementptr inbounds [512 x i8], [512 x i8]* %10, i32 0, i32 0, !dbg !2341
  %470 = call i32 @add_file(i32 noundef %468, i8* noundef %469, i8* noundef null, %struct.fd** noundef @File_list, %struct.fd** noundef @File_tail, %struct.fd* noundef null) #9, !dbg !2342
  %471 = icmp ne i32 %470, 0, !dbg !2342
  br i1 %471, label %473, label %472, !dbg !2343

472:                                              ; preds = %467
  store i32 0, i32* %3, align 4, !dbg !2344
  br label %479, !dbg !2344

473:                                              ; preds = %467
  br label %474

474:                                              ; preds = %473, %466
  br label %475, !dbg !2346

475:                                              ; preds = %474
  %476 = load i32, i32* %6, align 4, !dbg !2347
  %477 = add nsw i32 %476, 1, !dbg !2347
  store i32 %477, i32* %6, align 4, !dbg !2347
  br label %105, !dbg !2348, !llvm.loop !2349

478:                                              ; preds = %105
  store i32 1, i32* %3, align 4, !dbg !2351
  br label %479, !dbg !2351

479:                                              ; preds = %478, %472, %465, %218, %194, %182, %170, %102, %100, %96, %50, %39
  %480 = load i32, i32* %3, align 4, !dbg !2352
  ret i32 %480, !dbg !2352
}

; Function Attrs: noinline nounwind optnone
define internal i8* @conv_pat(i8* noundef %0) #0 !dbg !2353 {
  %2 = alloca i8*, align 4
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i8*, align 4
  %8 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  call void @llvm.dbg.declare(metadata i8** %2, metadata !2356, metadata !DIExpression()), !dbg !2357
  call void @llvm.dbg.declare(metadata i8** %3, metadata !2358, metadata !DIExpression()), !dbg !2359
  call void @llvm.dbg.declare(metadata i8** %4, metadata !2360, metadata !DIExpression()), !dbg !2361
  call void @llvm.dbg.declare(metadata i32* %5, metadata !2362, metadata !DIExpression()), !dbg !2363
  store i32 1, i32* %5, align 4, !dbg !2363
  %9 = load i8*, i8** %2, align 4, !dbg !2364
  store i8* %9, i8** %3, align 4, !dbg !2366
  br label %10, !dbg !2367

10:                                               ; preds = %44, %1
  %11 = load i8*, i8** %3, align 4, !dbg !2368
  %12 = load i8, i8* %11, align 1, !dbg !2370
  %13 = sext i8 %12 to i32, !dbg !2370
  %14 = icmp ne i32 %13, 0, !dbg !2371
  br i1 %14, label %15, label %47, !dbg !2372

15:                                               ; preds = %10
  call void @llvm.dbg.declare(metadata i32* %6, metadata !2373, metadata !DIExpression()), !dbg !2375
  %16 = load i32, i32* %5, align 4, !dbg !2376
  store i32 %16, i32* %6, align 4, !dbg !2375
  br i1 false, label %17, label %23, !dbg !2377

17:                                               ; preds = %15
  %18 = load i8*, i8** %3, align 4, !dbg !2378
  %19 = load i8, i8* %18, align 1, !dbg !2378
  %20 = sext i8 %19 to i32, !dbg !2378
  %21 = call i32 @isalpha(i32 noundef %20) #9, !dbg !2378
  %22 = icmp ne i32 %21, 0, !dbg !2378
  br i1 %22, label %30, label %33, !dbg !2378

23:                                               ; preds = %15
  %24 = load i8*, i8** %3, align 4, !dbg !2378
  %25 = load i8, i8* %24, align 1, !dbg !2378
  %26 = sext i8 %25 to i32, !dbg !2378
  %27 = or i32 %26, 32, !dbg !2378
  %28 = sub i32 %27, 97, !dbg !2378
  %29 = icmp ult i32 %28, 26, !dbg !2378
  br i1 %29, label %30, label %33, !dbg !2377

30:                                               ; preds = %23, %17
  %31 = load i32, i32* %5, align 4, !dbg !2380
  %32 = add i32 %31, 4, !dbg !2380
  store i32 %32, i32* %5, align 4, !dbg !2380
  br label %36, !dbg !2382

33:                                               ; preds = %23, %17
  %34 = load i32, i32* %5, align 4, !dbg !2383
  %35 = add i32 %34, 1, !dbg !2383
  store i32 %35, i32* %5, align 4, !dbg !2383
  br label %36

36:                                               ; preds = %33, %30
  %37 = load i32, i32* %6, align 4, !dbg !2385
  %38 = load i32, i32* %5, align 4, !dbg !2387
  %39 = icmp uge i32 %37, %38, !dbg !2388
  br i1 %39, label %40, label %43, !dbg !2389

40:                                               ; preds = %36
  %41 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !2390
  %42 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %41, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([47 x i8], [47 x i8]* @.str.54, i32 0, i32 0)) #9, !dbg !2392
  call void @exit(i32 noundef 1) #10, !dbg !2393
  unreachable, !dbg !2393

43:                                               ; preds = %36
  br label %44, !dbg !2394

44:                                               ; preds = %43
  %45 = load i8*, i8** %3, align 4, !dbg !2395
  %46 = getelementptr inbounds i8, i8* %45, i32 1, !dbg !2395
  store i8* %46, i8** %3, align 4, !dbg !2395
  br label %10, !dbg !2396, !llvm.loop !2397

47:                                               ; preds = %10
  %48 = load i32, i32* %5, align 4, !dbg !2399
  %49 = call noalias i8* @malloc(i32 noundef %48) #9, !dbg !2401
  store i8* %49, i8** %4, align 4, !dbg !2402
  %50 = icmp ne i8* %49, null, !dbg !2402
  br i1 %50, label %54, label %51, !dbg !2403

51:                                               ; preds = %47
  %52 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !2404
  %53 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %52, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.7, i32 0, i32 0), i8* noundef getelementptr inbounds ([36 x i8], [36 x i8]* @.str.55, i32 0, i32 0)) #9, !dbg !2406
  call void @exit(i32 noundef 1) #10, !dbg !2407
  unreachable, !dbg !2407

54:                                               ; preds = %47
  call void @llvm.dbg.declare(metadata i8** %7, metadata !2408, metadata !DIExpression()), !dbg !2409
  call void @llvm.dbg.declare(metadata i8** %8, metadata !2410, metadata !DIExpression()), !dbg !2411
  %55 = load i8*, i8** %2, align 4, !dbg !2412
  store i8* %55, i8** %8, align 4, !dbg !2411
  %56 = load i8*, i8** %4, align 4, !dbg !2413
  store i8* %56, i8** %7, align 4, !dbg !2415
  br label %57, !dbg !2416

57:                                               ; preds = %127, %54
  %58 = load i8*, i8** %8, align 4, !dbg !2417
  %59 = load i8, i8* %58, align 1, !dbg !2419
  %60 = sext i8 %59 to i32, !dbg !2419
  %61 = icmp ne i32 %60, 0, !dbg !2420
  br i1 %61, label %62, label %130, !dbg !2421

62:                                               ; preds = %57
  br i1 false, label %63, label %69, !dbg !2422

63:                                               ; preds = %62
  %64 = load i8*, i8** %8, align 4, !dbg !2424
  %65 = load i8, i8* %64, align 1, !dbg !2424
  %66 = sext i8 %65 to i32, !dbg !2424
  %67 = call i32 @islower(i32 noundef %66) #9, !dbg !2424
  %68 = icmp ne i32 %67, 0, !dbg !2424
  br i1 %68, label %75, label %91, !dbg !2424

69:                                               ; preds = %62
  %70 = load i8*, i8** %8, align 4, !dbg !2424
  %71 = load i8, i8* %70, align 1, !dbg !2424
  %72 = sext i8 %71 to i32, !dbg !2424
  %73 = sub i32 %72, 97, !dbg !2424
  %74 = icmp ult i32 %73, 26, !dbg !2424
  br i1 %74, label %75, label %91, !dbg !2422

75:                                               ; preds = %69, %63
  %76 = load i8*, i8** %7, align 4, !dbg !2426
  %77 = getelementptr inbounds i8, i8* %76, i32 1, !dbg !2426
  store i8* %77, i8** %7, align 4, !dbg !2426
  store i8 91, i8* %76, align 1, !dbg !2428
  %78 = load i8*, i8** %8, align 4, !dbg !2429
  %79 = load i8, i8* %78, align 1, !dbg !2430
  %80 = load i8*, i8** %7, align 4, !dbg !2431
  %81 = getelementptr inbounds i8, i8* %80, i32 1, !dbg !2431
  store i8* %81, i8** %7, align 4, !dbg !2431
  store i8 %79, i8* %80, align 1, !dbg !2432
  %82 = load i8*, i8** %8, align 4, !dbg !2433
  %83 = load i8, i8* %82, align 1, !dbg !2434
  %84 = sext i8 %83 to i32, !dbg !2434
  %85 = call i32 @toupper(i32 noundef %84) #9, !dbg !2435
  %86 = trunc i32 %85 to i8, !dbg !2436
  %87 = load i8*, i8** %7, align 4, !dbg !2437
  %88 = getelementptr inbounds i8, i8* %87, i32 1, !dbg !2437
  store i8* %88, i8** %7, align 4, !dbg !2437
  store i8 %86, i8* %87, align 1, !dbg !2438
  %89 = load i8*, i8** %7, align 4, !dbg !2439
  %90 = getelementptr inbounds i8, i8* %89, i32 1, !dbg !2439
  store i8* %90, i8** %7, align 4, !dbg !2439
  store i8 93, i8* %89, align 1, !dbg !2440
  br label %126, !dbg !2441

91:                                               ; preds = %69, %63
  br i1 false, label %92, label %98, !dbg !2442

92:                                               ; preds = %91
  %93 = load i8*, i8** %8, align 4, !dbg !2443
  %94 = load i8, i8* %93, align 1, !dbg !2443
  %95 = sext i8 %94 to i32, !dbg !2443
  %96 = call i32 @isupper(i32 noundef %95) #9, !dbg !2443
  %97 = icmp ne i32 %96, 0, !dbg !2443
  br i1 %97, label %104, label %120, !dbg !2443

98:                                               ; preds = %91
  %99 = load i8*, i8** %8, align 4, !dbg !2443
  %100 = load i8, i8* %99, align 1, !dbg !2443
  %101 = sext i8 %100 to i32, !dbg !2443
  %102 = sub i32 %101, 65, !dbg !2443
  %103 = icmp ult i32 %102, 26, !dbg !2443
  br i1 %103, label %104, label %120, !dbg !2442

104:                                              ; preds = %98, %92
  %105 = load i8*, i8** %7, align 4, !dbg !2445
  %106 = getelementptr inbounds i8, i8* %105, i32 1, !dbg !2445
  store i8* %106, i8** %7, align 4, !dbg !2445
  store i8 91, i8* %105, align 1, !dbg !2447
  %107 = load i8*, i8** %8, align 4, !dbg !2448
  %108 = load i8, i8* %107, align 1, !dbg !2449
  %109 = load i8*, i8** %7, align 4, !dbg !2450
  %110 = getelementptr inbounds i8, i8* %109, i32 1, !dbg !2450
  store i8* %110, i8** %7, align 4, !dbg !2450
  store i8 %108, i8* %109, align 1, !dbg !2451
  %111 = load i8*, i8** %8, align 4, !dbg !2452
  %112 = load i8, i8* %111, align 1, !dbg !2453
  %113 = sext i8 %112 to i32, !dbg !2453
  %114 = call i32 @tolower(i32 noundef %113) #9, !dbg !2454
  %115 = trunc i32 %114 to i8, !dbg !2455
  %116 = load i8*, i8** %7, align 4, !dbg !2456
  %117 = getelementptr inbounds i8, i8* %116, i32 1, !dbg !2456
  store i8* %117, i8** %7, align 4, !dbg !2456
  store i8 %115, i8* %116, align 1, !dbg !2457
  %118 = load i8*, i8** %7, align 4, !dbg !2458
  %119 = getelementptr inbounds i8, i8* %118, i32 1, !dbg !2458
  store i8* %119, i8** %7, align 4, !dbg !2458
  store i8 93, i8* %118, align 1, !dbg !2459
  br label %125, !dbg !2460

120:                                              ; preds = %98, %92
  %121 = load i8*, i8** %8, align 4, !dbg !2461
  %122 = load i8, i8* %121, align 1, !dbg !2463
  %123 = load i8*, i8** %7, align 4, !dbg !2464
  %124 = getelementptr inbounds i8, i8* %123, i32 1, !dbg !2464
  store i8* %124, i8** %7, align 4, !dbg !2464
  store i8 %122, i8* %123, align 1, !dbg !2465
  br label %125

125:                                              ; preds = %120, %104
  br label %126

126:                                              ; preds = %125, %75
  br label %127, !dbg !2466

127:                                              ; preds = %126
  %128 = load i8*, i8** %8, align 4, !dbg !2467
  %129 = getelementptr inbounds i8, i8* %128, i32 1, !dbg !2467
  store i8* %129, i8** %8, align 4, !dbg !2467
  br label %57, !dbg !2468, !llvm.loop !2469

130:                                              ; preds = %57
  %131 = load i8*, i8** %7, align 4, !dbg !2471
  store i8 0, i8* %131, align 1, !dbg !2472
  %132 = load i8*, i8** %4, align 4, !dbg !2473
  ret i8* %132, !dbg !2474
}

declare i32 @regcomp(%struct.re_pattern_buffer* noundef, i8* noundef, i32 noundef) #2

declare i32 @isalpha(i32 noundef) #2

declare i32 @toupper(i32 noundef) #2

declare i32 @tolower(i32 noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i32 @top_level__add_file(i8* noundef %0) #0 !dbg !2475 {
  %2 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  call void @llvm.dbg.declare(metadata i8** %2, metadata !2478, metadata !DIExpression()), !dbg !2479
  %3 = load i8*, i8** %2, align 4, !dbg !2480
  %4 = call i32 @add_file(i32 noundef -1, i8* noundef %3, i8* noundef null, %struct.fd** noundef @File_list, %struct.fd** noundef @File_tail, %struct.fd* noundef null) #9, !dbg !2481
  ret i32 %4, !dbg !2482
}

; Function Attrs: noinline nounwind optnone
define internal i32 @cond_top_level__FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR() #0 !dbg !2483 {
  %1 = call i32 @cond_top_level__add_file(i8* noundef getelementptr inbounds ([32 x i8], [32 x i8]* @.str.28, i32 0, i32 0), i8* noundef getelementptr inbounds ([38 x i8], [38 x i8]* @.str.21, i32 0, i32 0)) #9, !dbg !2484
  ret i32 %1, !dbg !2485
}

; Function Attrs: noinline nounwind optnone
define internal i32 @cond_top_level__FORTUNE_SYSTEM_OFFENSIVE_FORTUNES_DIR() #0 !dbg !2486 {
  %1 = call i32 @cond_top_level__add_file(i8* noundef getelementptr inbounds ([36 x i8], [36 x i8]* @.str.27, i32 0, i32 0), i8* noundef getelementptr inbounds ([42 x i8], [42 x i8]* @.str.22, i32 0, i32 0)) #9, !dbg !2487
  ret i32 %1, !dbg !2488
}

declare i8* @strncpy(i8* noundef, i8* noundef, i32 noundef) #2

declare i8* @strchr(i8* noundef, i32 noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i32 @add_file(i32 noundef %0, i8* noundef %1, i8* noundef %2, %struct.fd** noundef %3, %struct.fd** noundef %4, %struct.fd* noundef %5) #0 !dbg !2489 {
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i8*, align 4
  %10 = alloca i8*, align 4
  %11 = alloca %struct.fd**, align 4
  %12 = alloca %struct.fd**, align 4
  %13 = alloca %struct.fd*, align 4
  %14 = alloca %struct.fd*, align 4
  %15 = alloca i32, align 4
  %16 = alloca i8*, align 4
  %17 = alloca %struct.stat, align 8
  %18 = alloca i32, align 4
  %19 = alloca i32, align 4
  %20 = alloca i8, align 1
  %21 = alloca [512 x i8], align 16
  %22 = alloca [1024 x i8], align 16
  %23 = alloca i32, align 4
  %24 = alloca i8*, align 4
  %25 = alloca i8*, align 4
  %26 = alloca i32, align 4
  %27 = alloca i8*, align 4
  store i32 %0, i32* %8, align 4
  call void @llvm.dbg.declare(metadata i32* %8, metadata !2493, metadata !DIExpression()), !dbg !2494
  store i8* %1, i8** %9, align 4
  call void @llvm.dbg.declare(metadata i8** %9, metadata !2495, metadata !DIExpression()), !dbg !2496
  store i8* %2, i8** %10, align 4
  call void @llvm.dbg.declare(metadata i8** %10, metadata !2497, metadata !DIExpression()), !dbg !2498
  store %struct.fd** %3, %struct.fd*** %11, align 4
  call void @llvm.dbg.declare(metadata %struct.fd*** %11, metadata !2499, metadata !DIExpression()), !dbg !2500
  store %struct.fd** %4, %struct.fd*** %12, align 4
  call void @llvm.dbg.declare(metadata %struct.fd*** %12, metadata !2501, metadata !DIExpression()), !dbg !2502
  store %struct.fd* %5, %struct.fd** %13, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %13, metadata !2503, metadata !DIExpression()), !dbg !2504
  call void @llvm.dbg.declare(metadata %struct.fd** %14, metadata !2505, metadata !DIExpression()), !dbg !2506
  call void @llvm.dbg.declare(metadata i32* %15, metadata !2507, metadata !DIExpression()), !dbg !2508
  store i32 -1, i32* %15, align 4, !dbg !2508
  call void @llvm.dbg.declare(metadata i8** %16, metadata !2509, metadata !DIExpression()), !dbg !2510
  call void @llvm.dbg.declare(metadata %struct.stat* %17, metadata !2511, metadata !DIExpression()), !dbg !2552
  %28 = load i8*, i8** %10, align 4, !dbg !2553
  %29 = icmp ne i8* %28, null, !dbg !2553
  br i1 %29, label %33, label %30, !dbg !2555

30:                                               ; preds = %6
  %31 = load i8*, i8** %9, align 4, !dbg !2556
  %32 = call i8* @strdup(i8* noundef %31) #12, !dbg !2558
  store i8* %32, i8** %16, align 4, !dbg !2559
  br label %48, !dbg !2560

33:                                               ; preds = %6
  call void @llvm.dbg.declare(metadata i32* %18, metadata !2561, metadata !DIExpression()), !dbg !2563
  %34 = load i8*, i8** %10, align 4, !dbg !2564
  %35 = call i32 @strlen(i8* noundef %34) #11, !dbg !2565
  %36 = load i8*, i8** %9, align 4, !dbg !2566
  %37 = call i32 @strlen(i8* noundef %36) #11, !dbg !2567
  %38 = add i32 %35, %37, !dbg !2568
  %39 = add i32 %38, 12, !dbg !2569
  store i32 %39, i32* %18, align 4, !dbg !2563
  %40 = load i32, i32* %18, align 4, !dbg !2570
  %41 = add i32 %40, 1, !dbg !2571
  %42 = call i8* @do_malloc(i32 noundef %41) #9, !dbg !2572
  store i8* %42, i8** %16, align 4, !dbg !2573
  %43 = load i8*, i8** %16, align 4, !dbg !2574
  %44 = load i32, i32* %18, align 4, !dbg !2575
  %45 = load i8*, i8** %10, align 4, !dbg !2576
  %46 = load i8*, i8** %9, align 4, !dbg !2577
  %47 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %43, i32 noundef %44, i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str.33, i32 0, i32 0), i8* noundef %45, i8* noundef %46) #9, !dbg !2578
  br label %48

48:                                               ; preds = %33, %30
  %49 = load i8*, i8** %16, align 4, !dbg !2579
  %50 = load i8, i8* %49, align 1, !dbg !2581
  %51 = sext i8 %50 to i32, !dbg !2581
  %52 = icmp eq i32 %51, 47, !dbg !2582
  br i1 %52, label %53, label %59, !dbg !2583

53:                                               ; preds = %48
  %54 = load i8*, i8** %16, align 4, !dbg !2584
  %55 = call i32 @is_existant(i8* noundef %54) #9, !dbg !2585
  %56 = icmp ne i32 %55, 0, !dbg !2585
  br i1 %56, label %59, label %57, !dbg !2586

57:                                               ; preds = %53
  %58 = load i8*, i8** %16, align 4, !dbg !2587
  call void @free(i8* noundef %58) #9, !dbg !2589
  store i32 0, i32* %7, align 4, !dbg !2590
  br label %311, !dbg !2590

59:                                               ; preds = %53, %48
  call void @llvm.dbg.declare(metadata i32* %19, metadata !2591, metadata !DIExpression()), !dbg !2592
  %60 = load i8*, i8** %16, align 4, !dbg !2593
  %61 = call i32 @is_dir(i8* noundef %60) #9, !dbg !2594
  store i32 %61, i32* %19, align 4, !dbg !2592
  %62 = load i32, i32* %19, align 4, !dbg !2595
  %63 = icmp sgt i32 %62, 0, !dbg !2597
  br i1 %63, label %64, label %67, !dbg !2598

64:                                               ; preds = %59
  %65 = load %struct.fd*, %struct.fd** %13, align 4, !dbg !2599
  %66 = icmp ne %struct.fd* %65, null, !dbg !2599
  br i1 %66, label %70, label %67, !dbg !2600

67:                                               ; preds = %64, %59
  %68 = load i32, i32* %19, align 4, !dbg !2601
  %69 = icmp slt i32 %68, 0, !dbg !2602
  br i1 %69, label %70, label %72, !dbg !2603

70:                                               ; preds = %67, %64
  %71 = load i8*, i8** %16, align 4, !dbg !2604
  call void @free(i8* noundef %71) #9, !dbg !2606
  store i32 0, i32* %7, align 4, !dbg !2607
  br label %311, !dbg !2607

72:                                               ; preds = %67
  %73 = load i8*, i8** %16, align 4, !dbg !2608
  %74 = call i32 @open4read(i8* noundef %73) #9, !dbg !2610
  store i32 %74, i32* %15, align 4, !dbg !2611
  %75 = icmp slt i32 %74, 0, !dbg !2612
  br i1 %75, label %79, label %76, !dbg !2613

76:                                               ; preds = %72
  %77 = load i8*, i8** %16, align 4, !dbg !2614
  %78 = call zeroext i1 @path_is_absolute(i8* noundef %77) #9, !dbg !2615
  br i1 %78, label %152, label %79, !dbg !2616

79:                                               ; preds = %76, %72
  call void @llvm.dbg.declare(metadata i8* %20, metadata !2617, metadata !DIExpression()), !dbg !2619
  store i8 0, i8* %20, align 1, !dbg !2619
  %80 = load i8, i8* %20, align 1, !dbg !2620
  %81 = trunc i8 %80 to i1, !dbg !2620
  br i1 %81, label %147, label %82, !dbg !2622

82:                                               ; preds = %79
  %83 = load %struct.fd*, %struct.fd** %13, align 4, !dbg !2623
  %84 = icmp ne %struct.fd* %83, null, !dbg !2623
  br i1 %84, label %147, label %85, !dbg !2624

85:                                               ; preds = %82
  %86 = load i8*, i8** %10, align 4, !dbg !2625
  %87 = icmp ne i8* %86, null, !dbg !2625
  br i1 %87, label %147, label %88, !dbg !2626

88:                                               ; preds = %85
  %89 = load i8*, i8** @env_lang, align 4, !dbg !2627
  %90 = icmp ne i8* %89, null, !dbg !2627
  br i1 %90, label %91, label %144, !dbg !2630

91:                                               ; preds = %88
  call void @llvm.dbg.declare(metadata [512 x i8]* %21, metadata !2631, metadata !DIExpression()), !dbg !2633
  call void @llvm.dbg.declare(metadata [1024 x i8]* %22, metadata !2634, metadata !DIExpression()), !dbg !2635
  call void @llvm.dbg.declare(metadata i32* %23, metadata !2636, metadata !DIExpression()), !dbg !2637
  store i32 0, i32* %23, align 4, !dbg !2637
  %92 = getelementptr inbounds [512 x i8], [512 x i8]* %21, i32 0, i32 0, !dbg !2638
  %93 = load i8*, i8** @env_lang, align 4, !dbg !2639
  %94 = call i8* @strncpy(i8* noundef %92, i8* noundef %93, i32 noundef 512) #9, !dbg !2640
  %95 = getelementptr inbounds [512 x i8], [512 x i8]* %21, i32 0, i32 511, !dbg !2641
  store i8 0, i8* %95, align 1, !dbg !2642
  call void @llvm.dbg.declare(metadata i8** %24, metadata !2643, metadata !DIExpression()), !dbg !2644
  %96 = getelementptr inbounds [512 x i8], [512 x i8]* %21, i32 0, i32 0, !dbg !2645
  store i8* %96, i8** %24, align 4, !dbg !2644
  br label %97, !dbg !2646

97:                                               ; preds = %136, %91
  %98 = load i32, i32* %23, align 4, !dbg !2647
  %99 = icmp ne i32 %98, 0, !dbg !2647
  br i1 %99, label %108, label %100, !dbg !2648

100:                                              ; preds = %97
  %101 = load i8*, i8** %24, align 4, !dbg !2649
  %102 = icmp ne i8* %101, null, !dbg !2649
  br i1 %102, label %103, label %108, !dbg !2650

103:                                              ; preds = %100
  %104 = load i8*, i8** %24, align 4, !dbg !2651
  %105 = load i8, i8* %104, align 1, !dbg !2652
  %106 = sext i8 %105 to i32, !dbg !2653
  %107 = icmp ne i32 %106, 0, !dbg !2650
  br label %108

108:                                              ; preds = %103, %100, %97
  %109 = phi i1 [ false, %100 ], [ false, %97 ], [ %107, %103 ], !dbg !2654
  br i1 %109, label %110, label %138, !dbg !2646

110:                                              ; preds = %108
  call void @llvm.dbg.declare(metadata i8** %25, metadata !2655, metadata !DIExpression()), !dbg !2657
  %111 = load i8*, i8** %24, align 4, !dbg !2658
  %112 = call i8* @strchr(i8* noundef %111, i32 noundef 58) #9, !dbg !2659
  store i8* %112, i8** %25, align 4, !dbg !2657
  %113 = load i8*, i8** %25, align 4, !dbg !2660
  %114 = icmp ne i8* %113, null, !dbg !2660
  br i1 %114, label %115, label %118, !dbg !2662

115:                                              ; preds = %110
  %116 = load i8*, i8** %25, align 4, !dbg !2663
  %117 = getelementptr inbounds i8, i8* %116, i32 1, !dbg !2663
  store i8* %117, i8** %25, align 4, !dbg !2663
  store i8 0, i8* %116, align 1, !dbg !2665
  br label %118, !dbg !2666

118:                                              ; preds = %115, %110
  %119 = getelementptr inbounds [1024 x i8], [1024 x i8]* %22, i32 0, i32 0, !dbg !2667
  %120 = load i8*, i8** %24, align 4, !dbg !2668
  %121 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %119, i32 noundef 1024, i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str.33, i32 0, i32 0), i8* noundef getelementptr inbounds ([32 x i8], [32 x i8]* @.str.28, i32 0, i32 0), i8* noundef %120) #9, !dbg !2669
  %122 = load i8*, i8** %16, align 4, !dbg !2670
  %123 = load i8*, i8** %24, align 4, !dbg !2672
  %124 = call i32 @strncmp(i8* noundef %122, i8* noundef %123, i32 noundef 2) #9, !dbg !2673
  %125 = icmp eq i32 %124, 0, !dbg !2674
  br i1 %125, label %126, label %127, !dbg !2675

126:                                              ; preds = %118
  store i32 1, i32* %23, align 4, !dbg !2676
  br label %136, !dbg !2678

127:                                              ; preds = %118
  %128 = load i8*, i8** %16, align 4, !dbg !2679
  %129 = getelementptr inbounds [1024 x i8], [1024 x i8]* %22, i32 0, i32 0, !dbg !2681
  %130 = call i32 @strlen(i8* noundef getelementptr inbounds ([32 x i8], [32 x i8]* @.str.28, i32 0, i32 0)) #11, !dbg !2682
  %131 = add i32 %130, 3, !dbg !2683
  %132 = call i32 @strncmp(i8* noundef %128, i8* noundef %129, i32 noundef %131) #9, !dbg !2684
  %133 = icmp eq i32 %132, 0, !dbg !2685
  br i1 %133, label %134, label %135, !dbg !2686

134:                                              ; preds = %127
  store i32 1, i32* %23, align 4, !dbg !2687
  br label %135, !dbg !2689

135:                                              ; preds = %134, %127
  br label %136

136:                                              ; preds = %135, %126
  %137 = load i8*, i8** %25, align 4, !dbg !2690
  store i8* %137, i8** %24, align 4, !dbg !2691
  br label %97, !dbg !2646, !llvm.loop !2692

138:                                              ; preds = %108
  %139 = load i32, i32* %23, align 4, !dbg !2694
  %140 = icmp ne i32 %139, 0, !dbg !2694
  br i1 %140, label %143, label %141, !dbg !2696

141:                                              ; preds = %138
  %142 = load i8*, i8** %16, align 4, !dbg !2697
  call void @perror(i8* noundef %142) #9, !dbg !2699
  br label %143, !dbg !2700

143:                                              ; preds = %141, %138
  br label %146, !dbg !2701

144:                                              ; preds = %88
  %145 = load i8*, i8** %16, align 4, !dbg !2702
  call void @perror(i8* noundef %145) #9, !dbg !2704
  br label %146

146:                                              ; preds = %144, %143
  br label %147, !dbg !2705

147:                                              ; preds = %146, %85, %82, %79
  %148 = load i8*, i8** %16, align 4, !dbg !2706
  call void @free(i8* noundef %148) #9, !dbg !2707
  store i8* null, i8** %16, align 4, !dbg !2708
  %149 = load i8, i8* %20, align 1, !dbg !2709
  %150 = trunc i8 %149 to i1, !dbg !2709
  %151 = zext i1 %150 to i32, !dbg !2709
  store i32 %151, i32* %7, align 4, !dbg !2710
  br label %311, !dbg !2710

152:                                              ; preds = %76
  %153 = call %struct.fd* @new_fp() #9, !dbg !2711
  store %struct.fd* %153, %struct.fd** %14, align 4, !dbg !2712
  %154 = load i32, i32* %15, align 4, !dbg !2713
  %155 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2714
  %156 = getelementptr inbounds %struct.fd, %struct.fd* %155, i32 0, i32 1, !dbg !2715
  store i32 %154, i32* %156, align 4, !dbg !2716
  %157 = load i32, i32* %8, align 4, !dbg !2717
  %158 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2718
  %159 = getelementptr inbounds %struct.fd, %struct.fd* %158, i32 0, i32 0, !dbg !2719
  store i32 %157, i32* %159, align 4, !dbg !2720
  %160 = load i8*, i8** %9, align 4, !dbg !2721
  %161 = call i8* @strdup(i8* noundef %160) #12, !dbg !2722
  %162 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2723
  %163 = getelementptr inbounds %struct.fd, %struct.fd* %162, i32 0, i32 5, !dbg !2724
  store i8* %161, i8** %163, align 4, !dbg !2725
  %164 = load i8*, i8** %16, align 4, !dbg !2726
  %165 = call i8* @strdup(i8* noundef %164) #12, !dbg !2727
  %166 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2728
  %167 = getelementptr inbounds %struct.fd, %struct.fd* %166, i32 0, i32 6, !dbg !2729
  store i8* %165, i8** %167, align 4, !dbg !2730
  %168 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2731
  %169 = getelementptr inbounds %struct.fd, %struct.fd* %168, i32 0, i32 11, !dbg !2732
  store i8 0, i8* %169, align 2, !dbg !2733
  call void @llvm.dbg.declare(metadata i32* %26, metadata !2734, metadata !DIExpression()), !dbg !2735
  %170 = load i8*, i8** %16, align 4, !dbg !2736
  %171 = call i32 @strlen(i8* noundef %170) #11, !dbg !2737
  %172 = add i32 %171, 5, !dbg !2738
  store i32 %172, i32* %26, align 4, !dbg !2735
  call void @llvm.dbg.declare(metadata i8** %27, metadata !2739, metadata !DIExpression()), !dbg !2740
  %173 = load i32, i32* %26, align 4, !dbg !2741
  %174 = add i32 %173, 1, !dbg !2742
  %175 = call i8* @do_malloc(i32 noundef %174) #9, !dbg !2743
  store i8* %175, i8** %27, align 4, !dbg !2740
  %176 = load i8*, i8** %27, align 4, !dbg !2744
  %177 = load i32, i32* %26, align 4, !dbg !2745
  %178 = load i8*, i8** %16, align 4, !dbg !2746
  %179 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %176, i32 noundef %177, i8* noundef getelementptr inbounds ([6 x i8], [6 x i8]* @.str.36, i32 0, i32 0), i8* noundef %178) #9, !dbg !2747
  %180 = load i8*, i8** %27, align 4, !dbg !2748
  %181 = call i32 @stat(i8* noundef %180, %struct.stat* noundef %17) #9, !dbg !2750
  %182 = icmp eq i32 %181, 0, !dbg !2751
  br i1 %182, label %183, label %186, !dbg !2752

183:                                              ; preds = %152
  %184 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2753
  %185 = getelementptr inbounds %struct.fd, %struct.fd* %184, i32 0, i32 11, !dbg !2755
  store i8 1, i8* %185, align 2, !dbg !2756
  br label %186, !dbg !2757

186:                                              ; preds = %183, %152
  %187 = load i8*, i8** %27, align 4, !dbg !2758
  call void @free(i8* noundef %187) #9, !dbg !2759
  store i8* null, i8** %27, align 4, !dbg !2760
  %188 = load %struct.fd*, %struct.fd** %13, align 4, !dbg !2761
  %189 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2762
  %190 = getelementptr inbounds %struct.fd, %struct.fd* %189, i32 0, i32 15, !dbg !2763
  store %struct.fd* %188, %struct.fd** %190, align 4, !dbg !2764
  %191 = load i32, i32* %19, align 4, !dbg !2765
  %192 = icmp ne i32 %191, 0, !dbg !2765
  br i1 %192, label %193, label %197, !dbg !2767

193:                                              ; preds = %186
  %194 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2768
  %195 = call i32 @add_dir(%struct.fd* noundef %194) #9, !dbg !2769
  %196 = icmp ne i32 %195, 0, !dbg !2769
  br i1 %196, label %197, label %206, !dbg !2770

197:                                              ; preds = %193, %186
  %198 = load i32, i32* %19, align 4, !dbg !2771
  %199 = icmp ne i32 %198, 0, !dbg !2771
  br i1 %199, label %239, label %200, !dbg !2772

200:                                              ; preds = %197
  %201 = load i8*, i8** %16, align 4, !dbg !2773
  %202 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2774
  %203 = getelementptr inbounds %struct.fd, %struct.fd* %202, i32 0, i32 7, !dbg !2775
  %204 = call i32 @is_fortfile(i8* noundef %201, i8** noundef %203) #9, !dbg !2776
  %205 = icmp ne i32 %204, 0, !dbg !2776
  br i1 %205, label %239, label %206, !dbg !2777

206:                                              ; preds = %200, %193
  %207 = load %struct.fd*, %struct.fd** %13, align 4, !dbg !2778
  %208 = icmp ne %struct.fd* %207, null, !dbg !2778
  br i1 %208, label %213, label %209, !dbg !2781

209:                                              ; preds = %206
  %210 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !2782
  %211 = load i8*, i8** %16, align 4, !dbg !2784
  %212 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %210, i8* noundef getelementptr inbounds ([44 x i8], [44 x i8]* @.str.37, i32 0, i32 0), i8* noundef %211) #9, !dbg !2785
  br label %213, !dbg !2786

213:                                              ; preds = %209, %206
  %214 = load i8*, i8** %16, align 4, !dbg !2787
  call void @free(i8* noundef %214) #9, !dbg !2788
  store i8* null, i8** %16, align 4, !dbg !2789
  %215 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2790
  %216 = getelementptr inbounds %struct.fd, %struct.fd* %215, i32 0, i32 7, !dbg !2791
  %217 = load i8*, i8** %216, align 4, !dbg !2791
  call void @free(i8* noundef %217) #9, !dbg !2792
  %218 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2793
  %219 = getelementptr inbounds %struct.fd, %struct.fd* %218, i32 0, i32 8, !dbg !2794
  %220 = load i8*, i8** %219, align 4, !dbg !2794
  call void @free(i8* noundef %220) #9, !dbg !2795
  %221 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2796
  %222 = getelementptr inbounds %struct.fd, %struct.fd* %221, i32 0, i32 5, !dbg !2797
  %223 = load i8*, i8** %222, align 4, !dbg !2797
  call void @free(i8* noundef %223) #9, !dbg !2798
  %224 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2799
  %225 = getelementptr inbounds %struct.fd, %struct.fd* %224, i32 0, i32 6, !dbg !2800
  %226 = load i8*, i8** %225, align 4, !dbg !2800
  call void @free(i8* noundef %226) #9, !dbg !2801
  %227 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2802
  %228 = getelementptr inbounds %struct.fd, %struct.fd* %227, i32 0, i32 1, !dbg !2804
  %229 = load i32, i32* %228, align 4, !dbg !2804
  %230 = icmp sge i32 %229, 0, !dbg !2805
  br i1 %230, label %231, label %236, !dbg !2806

231:                                              ; preds = %213
  %232 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2807
  %233 = getelementptr inbounds %struct.fd, %struct.fd* %232, i32 0, i32 1, !dbg !2809
  %234 = load i32, i32* %233, align 4, !dbg !2809
  %235 = call i32 @close(i32 noundef %234) #9, !dbg !2810
  br label %236, !dbg !2811

236:                                              ; preds = %231, %213
  %237 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2812
  %238 = bitcast %struct.fd* %237 to i8*, !dbg !2812
  call void @free(i8* noundef %238) #9, !dbg !2813
  store i32 0, i32* %7, align 4, !dbg !2814
  br label %311, !dbg !2814

239:                                              ; preds = %200, %197
  %240 = load i32, i32* %19, align 4, !dbg !2815
  %241 = icmp ne i32 %240, 0, !dbg !2815
  br i1 %241, label %242, label %273, !dbg !2817

242:                                              ; preds = %239
  %243 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2818
  %244 = getelementptr inbounds %struct.fd, %struct.fd* %243, i32 0, i32 13, !dbg !2819
  %245 = load i32, i32* %244, align 4, !dbg !2819
  %246 = icmp eq i32 %245, 0, !dbg !2820
  br i1 %246, label %247, label %273, !dbg !2821

247:                                              ; preds = %242
  %248 = load i8*, i8** %16, align 4, !dbg !2822
  call void @free(i8* noundef %248) #9, !dbg !2824
  store i8* null, i8** %16, align 4, !dbg !2825
  %249 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2826
  %250 = getelementptr inbounds %struct.fd, %struct.fd* %249, i32 0, i32 7, !dbg !2827
  %251 = load i8*, i8** %250, align 4, !dbg !2827
  call void @free(i8* noundef %251) #9, !dbg !2828
  %252 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2829
  %253 = getelementptr inbounds %struct.fd, %struct.fd* %252, i32 0, i32 8, !dbg !2830
  %254 = load i8*, i8** %253, align 4, !dbg !2830
  call void @free(i8* noundef %254) #9, !dbg !2831
  %255 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2832
  %256 = getelementptr inbounds %struct.fd, %struct.fd* %255, i32 0, i32 5, !dbg !2833
  %257 = load i8*, i8** %256, align 4, !dbg !2833
  call void @free(i8* noundef %257) #9, !dbg !2834
  %258 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2835
  %259 = getelementptr inbounds %struct.fd, %struct.fd* %258, i32 0, i32 6, !dbg !2836
  %260 = load i8*, i8** %259, align 4, !dbg !2836
  call void @free(i8* noundef %260) #9, !dbg !2837
  %261 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2838
  %262 = getelementptr inbounds %struct.fd, %struct.fd* %261, i32 0, i32 1, !dbg !2840
  %263 = load i32, i32* %262, align 4, !dbg !2840
  %264 = icmp sge i32 %263, 0, !dbg !2841
  br i1 %264, label %265, label %270, !dbg !2842

265:                                              ; preds = %247
  %266 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2843
  %267 = getelementptr inbounds %struct.fd, %struct.fd* %266, i32 0, i32 1, !dbg !2845
  %268 = load i32, i32* %267, align 4, !dbg !2845
  %269 = call i32 @close(i32 noundef %268) #9, !dbg !2846
  br label %270, !dbg !2847

270:                                              ; preds = %265, %247
  %271 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2848
  %272 = bitcast %struct.fd* %271 to i8*, !dbg !2848
  call void @free(i8* noundef %272) #9, !dbg !2849
  store i32 1, i32* %7, align 4, !dbg !2850
  br label %311, !dbg !2850

273:                                              ; preds = %242, %239
  %274 = load %struct.fd**, %struct.fd*** %11, align 4, !dbg !2851
  %275 = load %struct.fd*, %struct.fd** %274, align 4, !dbg !2853
  %276 = icmp ne %struct.fd* %275, null, !dbg !2854
  br i1 %276, label %281, label %277, !dbg !2855

277:                                              ; preds = %273
  %278 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2856
  %279 = load %struct.fd**, %struct.fd*** %12, align 4, !dbg !2858
  store %struct.fd* %278, %struct.fd** %279, align 4, !dbg !2859
  %280 = load %struct.fd**, %struct.fd*** %11, align 4, !dbg !2860
  store %struct.fd* %278, %struct.fd** %280, align 4, !dbg !2861
  br label %309, !dbg !2862

281:                                              ; preds = %273
  %282 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2863
  %283 = getelementptr inbounds %struct.fd, %struct.fd* %282, i32 0, i32 0, !dbg !2865
  %284 = load i32, i32* %283, align 4, !dbg !2865
  %285 = icmp eq i32 %284, -1, !dbg !2866
  br i1 %285, label %286, label %297, !dbg !2867

286:                                              ; preds = %281
  %287 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2868
  %288 = load %struct.fd**, %struct.fd*** %12, align 4, !dbg !2870
  %289 = load %struct.fd*, %struct.fd** %288, align 4, !dbg !2871
  %290 = getelementptr inbounds %struct.fd, %struct.fd* %289, i32 0, i32 16, !dbg !2872
  store %struct.fd* %287, %struct.fd** %290, align 4, !dbg !2873
  %291 = load %struct.fd**, %struct.fd*** %12, align 4, !dbg !2874
  %292 = load %struct.fd*, %struct.fd** %291, align 4, !dbg !2875
  %293 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2876
  %294 = getelementptr inbounds %struct.fd, %struct.fd* %293, i32 0, i32 17, !dbg !2877
  store %struct.fd* %292, %struct.fd** %294, align 4, !dbg !2878
  %295 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2879
  %296 = load %struct.fd**, %struct.fd*** %12, align 4, !dbg !2880
  store %struct.fd* %295, %struct.fd** %296, align 4, !dbg !2881
  br label %308, !dbg !2882

297:                                              ; preds = %281
  %298 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2883
  %299 = load %struct.fd**, %struct.fd*** %11, align 4, !dbg !2885
  %300 = load %struct.fd*, %struct.fd** %299, align 4, !dbg !2886
  %301 = getelementptr inbounds %struct.fd, %struct.fd* %300, i32 0, i32 17, !dbg !2887
  store %struct.fd* %298, %struct.fd** %301, align 4, !dbg !2888
  %302 = load %struct.fd**, %struct.fd*** %11, align 4, !dbg !2889
  %303 = load %struct.fd*, %struct.fd** %302, align 4, !dbg !2890
  %304 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2891
  %305 = getelementptr inbounds %struct.fd, %struct.fd* %304, i32 0, i32 16, !dbg !2892
  store %struct.fd* %303, %struct.fd** %305, align 4, !dbg !2893
  %306 = load %struct.fd*, %struct.fd** %14, align 4, !dbg !2894
  %307 = load %struct.fd**, %struct.fd*** %11, align 4, !dbg !2895
  store %struct.fd* %306, %struct.fd** %307, align 4, !dbg !2896
  br label %308

308:                                              ; preds = %297, %286
  br label %309

309:                                              ; preds = %308, %277
  %310 = load i8*, i8** %16, align 4, !dbg !2897
  call void @free(i8* noundef %310) #9, !dbg !2898
  store i8* null, i8** %16, align 4, !dbg !2899
  store i32 1, i32* %7, align 4, !dbg !2900
  br label %311, !dbg !2900

311:                                              ; preds = %309, %270, %236, %147, %70, %57
  %312 = load i32, i32* %7, align 4, !dbg !2901
  ret i32 %312, !dbg !2901
}

; Function Attrs: noinline nounwind optnone
define internal i32 @top_level_FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR() #0 !dbg !2902 {
  %1 = call i32 @top_level__add_file(i8* noundef getelementptr inbounds ([38 x i8], [38 x i8]* @.str.21, i32 0, i32 0)) #9, !dbg !2903
  %2 = call i32 @cond_top_level__FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR() #9, !dbg !2904
  %3 = or i32 %1, %2, !dbg !2905
  ret i32 %3, !dbg !2906
}

declare i32 @isdigit(i32 noundef) #2

; Function Attrs: nounwind readonly willreturn
declare i32 @strcmp(i8* noundef, i8* noundef) #8

declare i32 @snprintf(i8* noundef, i32 noundef, i8* noundef, ...) #2

declare i32 @strncmp(i8* noundef, i8* noundef, i32 noundef) #2

; Function Attrs: noinline nounwind optnone
define internal %struct.fd* @new_fp() #0 !dbg !2907 {
  %1 = alloca %struct.fd*, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %1, metadata !2910, metadata !DIExpression()), !dbg !2911
  %2 = call i8* @do_malloc(i32 noundef 84) #9, !dbg !2912
  %3 = bitcast i8* %2 to %struct.fd*, !dbg !2912
  store %struct.fd* %3, %struct.fd** %1, align 4, !dbg !2911
  %4 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2913
  %5 = getelementptr inbounds %struct.fd, %struct.fd* %4, i32 0, i32 2, !dbg !2914
  store i32 -1, i32* %5, align 4, !dbg !2915
  %6 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2916
  %7 = getelementptr inbounds %struct.fd, %struct.fd* %6, i32 0, i32 3, !dbg !2917
  store i32 -1, i32* %7, align 4, !dbg !2918
  %8 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2919
  %9 = getelementptr inbounds %struct.fd, %struct.fd* %8, i32 0, i32 4, !dbg !2920
  store %struct._IO_FILE* null, %struct._IO_FILE** %9, align 4, !dbg !2921
  %10 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2922
  %11 = getelementptr inbounds %struct.fd, %struct.fd* %10, i32 0, i32 1, !dbg !2923
  store i32 -1, i32* %11, align 4, !dbg !2924
  %12 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2925
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 0, !dbg !2926
  store i32 -1, i32* %13, align 4, !dbg !2927
  %14 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2928
  %15 = getelementptr inbounds %struct.fd, %struct.fd* %14, i32 0, i32 9, !dbg !2929
  store i8 0, i8* %15, align 4, !dbg !2930
  %16 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2931
  %17 = getelementptr inbounds %struct.fd, %struct.fd* %16, i32 0, i32 12, !dbg !2932
  %18 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %17, i32 0, i32 0, !dbg !2933
  store i32 0, i32* %18, align 4, !dbg !2934
  %19 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2935
  %20 = getelementptr inbounds %struct.fd, %struct.fd* %19, i32 0, i32 12, !dbg !2936
  %21 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %20, i32 0, i32 1, !dbg !2937
  store i32 0, i32* %21, align 4, !dbg !2938
  %22 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2939
  %23 = getelementptr inbounds %struct.fd, %struct.fd* %22, i32 0, i32 12, !dbg !2940
  %24 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %23, i32 0, i32 2, !dbg !2941
  store i32 0, i32* %24, align 4, !dbg !2942
  %25 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2943
  %26 = getelementptr inbounds %struct.fd, %struct.fd* %25, i32 0, i32 12, !dbg !2944
  %27 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %26, i32 0, i32 3, !dbg !2945
  store i32 0, i32* %27, align 4, !dbg !2946
  %28 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2947
  %29 = getelementptr inbounds %struct.fd, %struct.fd* %28, i32 0, i32 12, !dbg !2948
  %30 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %29, i32 0, i32 4, !dbg !2949
  store i32 0, i32* %30, align 4, !dbg !2950
  %31 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2951
  %32 = getelementptr inbounds %struct.fd, %struct.fd* %31, i32 0, i32 12, !dbg !2952
  %33 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %32, i32 0, i32 5, !dbg !2953
  %34 = getelementptr inbounds [4 x i8], [4 x i8]* %33, i32 0, i32 0, !dbg !2951
  store i8 0, i8* %34, align 4, !dbg !2954
  %35 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2955
  %36 = getelementptr inbounds %struct.fd, %struct.fd* %35, i32 0, i32 12, !dbg !2956
  %37 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %36, i32 0, i32 5, !dbg !2957
  %38 = getelementptr inbounds [4 x i8], [4 x i8]* %37, i32 0, i32 1, !dbg !2955
  store i8 0, i8* %38, align 1, !dbg !2958
  %39 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2959
  %40 = getelementptr inbounds %struct.fd, %struct.fd* %39, i32 0, i32 12, !dbg !2960
  %41 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %40, i32 0, i32 5, !dbg !2961
  %42 = getelementptr inbounds [4 x i8], [4 x i8]* %41, i32 0, i32 2, !dbg !2959
  store i8 0, i8* %42, align 2, !dbg !2962
  %43 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2963
  %44 = getelementptr inbounds %struct.fd, %struct.fd* %43, i32 0, i32 12, !dbg !2964
  %45 = getelementptr inbounds %struct.STRFILE, %struct.STRFILE* %44, i32 0, i32 5, !dbg !2965
  %46 = getelementptr inbounds [4 x i8], [4 x i8]* %45, i32 0, i32 3, !dbg !2963
  store i8 0, i8* %46, align 1, !dbg !2966
  %47 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2967
  %48 = getelementptr inbounds %struct.fd, %struct.fd* %47, i32 0, i32 5, !dbg !2968
  store i8* null, i8** %48, align 4, !dbg !2969
  %49 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2970
  %50 = getelementptr inbounds %struct.fd, %struct.fd* %49, i32 0, i32 16, !dbg !2971
  store %struct.fd* null, %struct.fd** %50, align 4, !dbg !2972
  %51 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2973
  %52 = getelementptr inbounds %struct.fd, %struct.fd* %51, i32 0, i32 6, !dbg !2974
  store i8* null, i8** %52, align 4, !dbg !2975
  %53 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2976
  %54 = getelementptr inbounds %struct.fd, %struct.fd* %53, i32 0, i32 17, !dbg !2977
  store %struct.fd* null, %struct.fd** %54, align 4, !dbg !2978
  %55 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2979
  %56 = getelementptr inbounds %struct.fd, %struct.fd* %55, i32 0, i32 14, !dbg !2980
  store %struct.fd* null, %struct.fd** %56, align 4, !dbg !2981
  %57 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2982
  %58 = getelementptr inbounds %struct.fd, %struct.fd* %57, i32 0, i32 15, !dbg !2983
  store %struct.fd* null, %struct.fd** %58, align 4, !dbg !2984
  %59 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2985
  %60 = getelementptr inbounds %struct.fd, %struct.fd* %59, i32 0, i32 7, !dbg !2986
  store i8* null, i8** %60, align 4, !dbg !2987
  %61 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2988
  %62 = getelementptr inbounds %struct.fd, %struct.fd* %61, i32 0, i32 8, !dbg !2989
  store i8* null, i8** %62, align 4, !dbg !2990
  %63 = load %struct.fd*, %struct.fd** %1, align 4, !dbg !2991
  ret %struct.fd* %63, !dbg !2992
}

; Function Attrs: noinline nounwind optnone
define internal i32 @is_existant(i8* noundef %0) #0 !dbg !2993 {
  %2 = alloca i32, align 4
  %3 = alloca i8*, align 4
  %4 = alloca %struct.stat, align 8
  store i8* %0, i8** %3, align 4
  call void @llvm.dbg.declare(metadata i8** %3, metadata !2996, metadata !DIExpression()), !dbg !2997
  call void @llvm.dbg.declare(metadata %struct.stat* %4, metadata !2998, metadata !DIExpression()), !dbg !2999
  %5 = load i8*, i8** %3, align 4, !dbg !3000
  %6 = call i32 @stat(i8* noundef %5, %struct.stat* noundef %4) #9, !dbg !3002
  %7 = icmp eq i32 %6, 0, !dbg !3003
  br i1 %7, label %8, label %9, !dbg !3004

8:                                                ; preds = %1
  store i32 1, i32* %2, align 4, !dbg !3005
  br label %13, !dbg !3005

9:                                                ; preds = %1
  %10 = load i32, i32* @errno, align 4, !dbg !3007
  switch i32 %10, label %12 [
    i32 44, label %11
    i32 54, label %11
  ], !dbg !3008

11:                                               ; preds = %9, %9
  store i32 0, i32* %2, align 4, !dbg !3009
  br label %13, !dbg !3009

12:                                               ; preds = %9
  call void @perror(i8* noundef getelementptr inbounds ([33 x i8], [33 x i8]* @.str.39, i32 0, i32 0)) #9, !dbg !3011
  call void @exit(i32 noundef 1) #10, !dbg !3012
  unreachable, !dbg !3012

13:                                               ; preds = %11, %8
  %14 = load i32, i32* %2, align 4, !dbg !3013
  ret i32 %14, !dbg !3013
}

; Function Attrs: noinline nounwind optnone
define internal i32 @is_dir(i8* noundef %0) #0 !dbg !3014 {
  %2 = alloca i32, align 4
  %3 = alloca i8*, align 4
  %4 = alloca %struct.stat, align 8
  %5 = alloca i8, align 1
  store i8* %0, i8** %3, align 4
  call void @llvm.dbg.declare(metadata i8** %3, metadata !3015, metadata !DIExpression()), !dbg !3016
  call void @llvm.dbg.declare(metadata %struct.stat* %4, metadata !3017, metadata !DIExpression()), !dbg !3018
  %6 = load i8*, i8** %3, align 4, !dbg !3019
  %7 = call i32 @stat(i8* noundef %6, %struct.stat* noundef %4) #9, !dbg !3021
  %8 = icmp slt i32 %7, 0, !dbg !3022
  br i1 %8, label %9, label %10, !dbg !3023

9:                                                ; preds = %1
  store i32 -1, i32* %2, align 4, !dbg !3024
  br label %22, !dbg !3024

10:                                               ; preds = %1
  call void @llvm.dbg.declare(metadata i8* %5, metadata !3026, metadata !DIExpression()), !dbg !3028
  %11 = getelementptr inbounds %struct.stat, %struct.stat* %4, i32 0, i32 3, !dbg !3029
  %12 = load i32, i32* %11, align 8, !dbg !3029
  %13 = and i32 %12, 57344, !dbg !3029
  %14 = icmp eq i32 %13, 16384, !dbg !3029
  %15 = zext i1 %14 to i64, !dbg !3029
  %16 = select i1 %14, i32 1, i32 0, !dbg !3029
  %17 = icmp ne i32 %16, 0, !dbg !3030
  %18 = zext i1 %17 to i8, !dbg !3028
  store i8 %18, i8* %5, align 1, !dbg !3028
  %19 = load i8, i8* %5, align 1, !dbg !3031
  %20 = trunc i8 %19 to i1, !dbg !3031
  %21 = zext i1 %20 to i32, !dbg !3031
  store i32 %21, i32* %2, align 4, !dbg !3032
  br label %22, !dbg !3032

22:                                               ; preds = %10, %9
  %23 = load i32, i32* %2, align 4, !dbg !3033
  ret i32 %23, !dbg !3033
}

; Function Attrs: noinline nounwind optnone
define internal zeroext i1 @path_is_absolute(i8* noundef %0) #0 !dbg !3034 {
  %2 = alloca i1, align 1
  %3 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  call void @llvm.dbg.declare(metadata i8** %3, metadata !3037, metadata !DIExpression()), !dbg !3038
  %4 = load i8*, i8** %3, align 4, !dbg !3039
  %5 = getelementptr inbounds i8, i8* %4, i32 0, !dbg !3039
  %6 = load i8, i8* %5, align 1, !dbg !3039
  %7 = sext i8 %6 to i32, !dbg !3039
  %8 = icmp eq i32 %7, 47, !dbg !3041
  br i1 %8, label %9, label %10, !dbg !3042

9:                                                ; preds = %1
  store i1 true, i1* %2, align 1, !dbg !3043
  br label %11, !dbg !3043

10:                                               ; preds = %1
  store i1 false, i1* %2, align 1, !dbg !3045
  br label %11, !dbg !3045

11:                                               ; preds = %10, %9
  %12 = load i1, i1* %2, align 1, !dbg !3046
  ret i1 %12, !dbg !3046
}

declare i32 @stat(i8* noundef, %struct.stat* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i32 @add_dir(%struct.fd* noundef %0) #0 !dbg !3047 {
  %2 = alloca i32, align 4
  %3 = alloca %struct.fd*, align 4
  %4 = alloca %struct._DIR*, align 4
  %5 = alloca %struct.dirent*, align 4
  %6 = alloca i8**, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca %struct.fd*, align 4
  %11 = alloca i8*, align 4
  store %struct.fd* %0, %struct.fd** %3, align 4
  call void @llvm.dbg.declare(metadata %struct.fd** %3, metadata !3050, metadata !DIExpression()), !dbg !3051
  call void @llvm.dbg.declare(metadata %struct._DIR** %4, metadata !3052, metadata !DIExpression()), !dbg !3057
  call void @llvm.dbg.declare(metadata %struct.dirent** %5, metadata !3058, metadata !DIExpression()), !dbg !3069
  call void @llvm.dbg.declare(metadata i8*** %6, metadata !3070, metadata !DIExpression()), !dbg !3071
  call void @llvm.dbg.declare(metadata i32* %7, metadata !3072, metadata !DIExpression()), !dbg !3073
  call void @llvm.dbg.declare(metadata i32* %8, metadata !3074, metadata !DIExpression()), !dbg !3075
  call void @llvm.dbg.declare(metadata i32* %9, metadata !3076, metadata !DIExpression()), !dbg !3077
  %12 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3078
  %13 = getelementptr inbounds %struct.fd, %struct.fd* %12, i32 0, i32 1, !dbg !3079
  %14 = load i32, i32* %13, align 4, !dbg !3079
  %15 = call i32 @close(i32 noundef %14) #9, !dbg !3080
  %16 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3081
  %17 = getelementptr inbounds %struct.fd, %struct.fd* %16, i32 0, i32 1, !dbg !3082
  store i32 -1, i32* %17, align 4, !dbg !3083
  %18 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3084
  %19 = getelementptr inbounds %struct.fd, %struct.fd* %18, i32 0, i32 6, !dbg !3086
  %20 = load i8*, i8** %19, align 4, !dbg !3086
  %21 = call %struct._DIR* @opendir(i8* noundef %20) #9, !dbg !3087
  store %struct._DIR* %21, %struct._DIR** %4, align 4, !dbg !3088
  %22 = icmp ne %struct._DIR* %21, null, !dbg !3088
  br i1 %22, label %27, label %23, !dbg !3089

23:                                               ; preds = %1
  %24 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3090
  %25 = getelementptr inbounds %struct.fd, %struct.fd* %24, i32 0, i32 6, !dbg !3092
  %26 = load i8*, i8** %25, align 4, !dbg !3092
  call void @perror(i8* noundef %26) #9, !dbg !3093
  store i32 0, i32* %2, align 4, !dbg !3094
  br label %140, !dbg !3094

27:                                               ; preds = %1
  call void @llvm.dbg.declare(metadata %struct.fd** %10, metadata !3095, metadata !DIExpression()), !dbg !3096
  store %struct.fd* null, %struct.fd** %10, align 4, !dbg !3096
  %28 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3097
  %29 = getelementptr inbounds %struct.fd, %struct.fd* %28, i32 0, i32 13, !dbg !3098
  store i32 0, i32* %29, align 4, !dbg !3099
  store i32 200, i32* %9, align 4, !dbg !3100
  store i32 0, i32* %8, align 4, !dbg !3101
  %30 = load i32, i32* %9, align 4, !dbg !3102
  %31 = mul i32 4, %30, !dbg !3103
  %32 = call noalias i8* @malloc(i32 noundef %31) #9, !dbg !3104
  %33 = bitcast i8* %32 to i8**, !dbg !3104
  store i8** %33, i8*** %6, align 4, !dbg !3105
  %34 = load i8**, i8*** %6, align 4, !dbg !3106
  %35 = icmp ne i8** %34, null, !dbg !3106
  br i1 %35, label %37, label %36, !dbg !3108

36:                                               ; preds = %27
  call void @perror(i8* noundef getelementptr inbounds ([12 x i8], [12 x i8]* @.str.40, i32 0, i32 0)) #9, !dbg !3109
  call void @exit(i32 noundef -1) #10, !dbg !3111
  unreachable, !dbg !3111

37:                                               ; preds = %27
  br label %38, !dbg !3112

38:                                               ; preds = %71, %49, %37
  %39 = load %struct._DIR*, %struct._DIR** %4, align 4, !dbg !3113
  %40 = call %struct.dirent* @readdir(%struct._DIR* noundef %39) #9, !dbg !3114
  store %struct.dirent* %40, %struct.dirent** %5, align 4, !dbg !3115
  %41 = icmp ne %struct.dirent* %40, null, !dbg !3112
  br i1 %41, label %42, label %77, !dbg !3112

42:                                               ; preds = %38
  %43 = load %struct.dirent*, %struct.dirent** %5, align 4, !dbg !3116
  %44 = getelementptr inbounds %struct.dirent, %struct.dirent* %43, i32 0, i32 2, !dbg !3119
  %45 = getelementptr inbounds [0 x i8], [0 x i8]* %44, i32 0, i32 0, !dbg !3116
  %46 = load i8, i8* %45, align 1, !dbg !3116
  %47 = sext i8 %46 to i32, !dbg !3116
  %48 = icmp eq i32 %47, 0, !dbg !3120
  br i1 %48, label %49, label %50, !dbg !3121

49:                                               ; preds = %42
  br label %38, !dbg !3122, !llvm.loop !3124

50:                                               ; preds = %42
  call void @llvm.dbg.declare(metadata i8** %11, metadata !3126, metadata !DIExpression()), !dbg !3127
  %51 = load %struct.dirent*, %struct.dirent** %5, align 4, !dbg !3128
  %52 = getelementptr inbounds %struct.dirent, %struct.dirent* %51, i32 0, i32 2, !dbg !3129
  %53 = getelementptr inbounds [0 x i8], [0 x i8]* %52, i32 0, i32 0, !dbg !3128
  %54 = call i8* @strdup(i8* noundef %53) #12, !dbg !3130
  store i8* %54, i8** %11, align 4, !dbg !3127
  %55 = load i32, i32* %8, align 4, !dbg !3131
  %56 = load i32, i32* %9, align 4, !dbg !3133
  %57 = icmp eq i32 %55, %56, !dbg !3134
  br i1 %57, label %58, label %71, !dbg !3135

58:                                               ; preds = %50
  %59 = load i32, i32* %9, align 4, !dbg !3136
  %60 = add i32 %59, 200, !dbg !3136
  store i32 %60, i32* %9, align 4, !dbg !3136
  %61 = load i8**, i8*** %6, align 4, !dbg !3138
  %62 = bitcast i8** %61 to i8*, !dbg !3138
  %63 = load i32, i32* %9, align 4, !dbg !3139
  %64 = mul i32 4, %63, !dbg !3140
  %65 = call i8* @realloc(i8* noundef %62, i32 noundef %64) #9, !dbg !3141
  %66 = bitcast i8* %65 to i8**, !dbg !3141
  store i8** %66, i8*** %6, align 4, !dbg !3142
  %67 = load i8**, i8*** %6, align 4, !dbg !3143
  %68 = icmp ne i8** %67, null, !dbg !3143
  br i1 %68, label %70, label %69, !dbg !3145

69:                                               ; preds = %58
  call void @perror(i8* noundef getelementptr inbounds ([12 x i8], [12 x i8]* @.str.40, i32 0, i32 0)) #9, !dbg !3146
  call void @exit(i32 noundef -1) #10, !dbg !3148
  unreachable, !dbg !3148

70:                                               ; preds = %58
  br label %71, !dbg !3149

71:                                               ; preds = %70, %50
  %72 = load i8*, i8** %11, align 4, !dbg !3150
  %73 = load i8**, i8*** %6, align 4, !dbg !3151
  %74 = load i32, i32* %8, align 4, !dbg !3152
  %75 = add i32 %74, 1, !dbg !3152
  store i32 %75, i32* %8, align 4, !dbg !3152
  %76 = getelementptr inbounds i8*, i8** %73, i32 %74, !dbg !3151
  store i8* %72, i8** %76, align 4, !dbg !3153
  br label %38, !dbg !3112, !llvm.loop !3124

77:                                               ; preds = %38
  %78 = load %struct._DIR*, %struct._DIR** %4, align 4, !dbg !3154
  %79 = call i32 @closedir(%struct._DIR* noundef %78) #9, !dbg !3155
  %80 = load i8**, i8*** %6, align 4, !dbg !3156
  %81 = bitcast i8** %80 to i8*, !dbg !3156
  %82 = load i32, i32* %8, align 4, !dbg !3157
  call void @qsort(i8* noundef %81, i32 noundef %82, i32 noundef 4, i32 (i8*, i8*)* noundef @names_compare) #9, !dbg !3158
  store i32 0, i32* %7, align 4, !dbg !3159
  br label %83, !dbg !3161

83:                                               ; preds = %110, %77
  %84 = load i32, i32* %7, align 4, !dbg !3162
  %85 = load i32, i32* %8, align 4, !dbg !3164
  %86 = icmp ult i32 %84, %85, !dbg !3165
  br i1 %86, label %87, label %113, !dbg !3166

87:                                               ; preds = %83
  %88 = load i8**, i8*** %6, align 4, !dbg !3167
  %89 = load i32, i32* %7, align 4, !dbg !3170
  %90 = getelementptr inbounds i8*, i8** %88, i32 %89, !dbg !3167
  %91 = load i8*, i8** %90, align 4, !dbg !3167
  %92 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3171
  %93 = getelementptr inbounds %struct.fd, %struct.fd* %92, i32 0, i32 6, !dbg !3172
  %94 = load i8*, i8** %93, align 4, !dbg !3172
  %95 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3173
  %96 = getelementptr inbounds %struct.fd, %struct.fd* %95, i32 0, i32 14, !dbg !3174
  %97 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3175
  %98 = call i32 @add_file(i32 noundef -1, i8* noundef %91, i8* noundef %94, %struct.fd** noundef %96, %struct.fd** noundef %10, %struct.fd* noundef %97) #9, !dbg !3176
  %99 = icmp ne i32 %98, 0, !dbg !3176
  br i1 %99, label %100, label %105, !dbg !3177

100:                                              ; preds = %87
  %101 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3178
  %102 = getelementptr inbounds %struct.fd, %struct.fd* %101, i32 0, i32 13, !dbg !3180
  %103 = load i32, i32* %102, align 4, !dbg !3181
  %104 = add nsw i32 %103, 1, !dbg !3181
  store i32 %104, i32* %102, align 4, !dbg !3181
  br label %105, !dbg !3182

105:                                              ; preds = %100, %87
  %106 = load i8**, i8*** %6, align 4, !dbg !3183
  %107 = load i32, i32* %7, align 4, !dbg !3184
  %108 = getelementptr inbounds i8*, i8** %106, i32 %107, !dbg !3183
  %109 = load i8*, i8** %108, align 4, !dbg !3183
  call void @free(i8* noundef %109) #9, !dbg !3185
  br label %110, !dbg !3186

110:                                              ; preds = %105
  %111 = load i32, i32* %7, align 4, !dbg !3187
  %112 = add i32 %111, 1, !dbg !3187
  store i32 %112, i32* %7, align 4, !dbg !3187
  br label %83, !dbg !3188, !llvm.loop !3189

113:                                              ; preds = %83
  %114 = load i8**, i8*** %6, align 4, !dbg !3191
  %115 = bitcast i8** %114 to i8*, !dbg !3191
  call void @free(i8* noundef %115) #9, !dbg !3192
  store %struct._DIR* null, %struct._DIR** %4, align 4, !dbg !3193
  %116 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3194
  %117 = getelementptr inbounds %struct.fd, %struct.fd* %116, i32 0, i32 13, !dbg !3196
  %118 = load i32, i32* %117, align 4, !dbg !3196
  %119 = icmp eq i32 %118, 0, !dbg !3197
  br i1 %119, label %120, label %139, !dbg !3198

120:                                              ; preds = %113
  %121 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3199
  %122 = getelementptr inbounds %struct.fd, %struct.fd* %121, i32 0, i32 6, !dbg !3202
  %123 = load i8*, i8** %122, align 4, !dbg !3202
  %124 = call i32 @strcmp(i8* noundef getelementptr inbounds ([38 x i8], [38 x i8]* @.str.21, i32 0, i32 0), i8* noundef %123) #13, !dbg !3203
  %125 = icmp eq i32 %124, 0, !dbg !3204
  br i1 %125, label %132, label %126, !dbg !3205

126:                                              ; preds = %120
  %127 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3206
  %128 = getelementptr inbounds %struct.fd, %struct.fd* %127, i32 0, i32 6, !dbg !3207
  %129 = load i8*, i8** %128, align 4, !dbg !3207
  %130 = call i32 @strcmp(i8* noundef getelementptr inbounds ([42 x i8], [42 x i8]* @.str.22, i32 0, i32 0), i8* noundef %129) #13, !dbg !3208
  %131 = icmp eq i32 %130, 0, !dbg !3209
  br i1 %131, label %132, label %133, !dbg !3210

132:                                              ; preds = %126, %120
  store i32 1, i32* %2, align 4, !dbg !3211
  br label %140, !dbg !3211

133:                                              ; preds = %126
  %134 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 4, !dbg !3213
  %135 = load %struct.fd*, %struct.fd** %3, align 4, !dbg !3214
  %136 = getelementptr inbounds %struct.fd, %struct.fd* %135, i32 0, i32 6, !dbg !3215
  %137 = load i8*, i8** %136, align 4, !dbg !3215
  %138 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* noundef %134, i8* noundef getelementptr inbounds ([45 x i8], [45 x i8]* @.str.41, i32 0, i32 0), i8* noundef %137) #9, !dbg !3216
  store i32 0, i32* %2, align 4, !dbg !3217
  br label %140, !dbg !3217

139:                                              ; preds = %113
  store i32 1, i32* %2, align 4, !dbg !3218
  br label %140, !dbg !3218

140:                                              ; preds = %139, %133, %132, %23
  %141 = load i32, i32* %2, align 4, !dbg !3219
  ret i32 %141, !dbg !3219
}

; Function Attrs: noinline nounwind optnone
define internal i32 @is_fortfile(i8* noundef %0, i8** noundef %1) #0 !dbg !105 {
  %3 = alloca i32, align 4
  %4 = alloca i8*, align 4
  %5 = alloca i8**, align 4
  %6 = alloca i8*, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i8*, align 4
  store i8* %0, i8** %4, align 4
  call void @llvm.dbg.declare(metadata i8** %4, metadata !3220, metadata !DIExpression()), !dbg !3221
  store i8** %1, i8*** %5, align 4
  call void @llvm.dbg.declare(metadata i8*** %5, metadata !3222, metadata !DIExpression()), !dbg !3223
  call void @llvm.dbg.declare(metadata i8** %6, metadata !3224, metadata !DIExpression()), !dbg !3225
  %10 = load i8*, i8** %4, align 4, !dbg !3226
  %11 = call i8* @strrchr(i8* noundef %10, i32 noundef 47) #9, !dbg !3227
  store i8* %11, i8** %6, align 4, !dbg !3225
  %12 = load i8*, i8** %6, align 4, !dbg !3228
  %13 = icmp ne i8* %12, null, !dbg !3228
  br i1 %13, label %16, label %14, !dbg !3230

14:                                               ; preds = %2
  %15 = load i8*, i8** %4, align 4, !dbg !3231
  store i8* %15, i8** %6, align 4, !dbg !3233
  br label %19, !dbg !3234

16:                                               ; preds = %2
  %17 = load i8*, i8** %6, align 4, !dbg !3235
  %18 = getelementptr inbounds i8, i8* %17, i32 1, !dbg !3235
  store i8* %18, i8** %6, align 4, !dbg !3235
  br label %19

19:                                               ; preds = %16, %14
  %20 = load i8*, i8** %6, align 4, !dbg !3237
  %21 = load i8, i8* %20, align 1, !dbg !3239
  %22 = sext i8 %21 to i32, !dbg !3239
  %23 = icmp eq i32 %22, 46, !dbg !3240
  br i1 %23, label %24, label %25, !dbg !3241

24:                                               ; preds = %19
  store i32 0, i32* %3, align 4, !dbg !3242
  br label %75, !dbg !3242

25:                                               ; preds = %19
  %26 = load i8*, i8** %6, align 4, !dbg !3244
  %27 = call i8* @strrchr(i8* noundef %26, i32 noundef 46) #9, !dbg !3246
  store i8* %27, i8** %6, align 4, !dbg !3247
  %28 = icmp ne i8* %27, null, !dbg !3247
  br i1 %28, label %29, label %50, !dbg !3248

29:                                               ; preds = %25
  %30 = load i8*, i8** %6, align 4, !dbg !3249
  %31 = getelementptr inbounds i8, i8* %30, i32 1, !dbg !3249
  store i8* %31, i8** %6, align 4, !dbg !3249
  call void @llvm.dbg.declare(metadata i32* %7, metadata !3251, metadata !DIExpression()), !dbg !3253
  store i32 0, i32* %7, align 4, !dbg !3253
  br label %32, !dbg !3254

32:                                               ; preds = %46, %29
  %33 = load i32, i32* %7, align 4, !dbg !3255
  %34 = getelementptr inbounds [14 x i8*], [14 x i8*]* @is_fortfile.suflist, i32 0, i32 %33, !dbg !3257
  %35 = load i8*, i8** %34, align 4, !dbg !3257
  %36 = icmp ne i8* %35, null, !dbg !3258
  br i1 %36, label %37, label %49, !dbg !3258

37:                                               ; preds = %32
  %38 = load i8*, i8** %6, align 4, !dbg !3259
  %39 = load i32, i32* %7, align 4, !dbg !3262
  %40 = getelementptr inbounds [14 x i8*], [14 x i8*]* @is_fortfile.suflist, i32 0, i32 %39, !dbg !3263
  %41 = load i8*, i8** %40, align 4, !dbg !3263
  %42 = call i32 @strcmp(i8* noundef %38, i8* noundef %41) #13, !dbg !3264
  %43 = icmp eq i32 %42, 0, !dbg !3265
  br i1 %43, label %44, label %45, !dbg !3266

44:                                               ; preds = %37
  store i32 0, i32* %3, align 4, !dbg !3267
  br label %75, !dbg !3267

45:                                               ; preds = %37
  br label %46, !dbg !3269

46:                                               ; preds = %45
  %47 = load i32, i32* %7, align 4, !dbg !3270
  %48 = add nsw i32 %47, 1, !dbg !3270
  store i32 %48, i32* %7, align 4, !dbg !3270
  br label %32, !dbg !3271, !llvm.loop !3272

49:                                               ; preds = %32
  br label %50, !dbg !3274

50:                                               ; preds = %49, %25
  call void @llvm.dbg.declare(metadata i32* %8, metadata !3275, metadata !DIExpression()), !dbg !3276
  %51 = load i8*, i8** %4, align 4, !dbg !3277
  %52 = call i32 @strlen(i8* noundef %51) #11, !dbg !3278
  %53 = add i32 %52, 6, !dbg !3279
  store i32 %53, i32* %8, align 4, !dbg !3276
  call void @llvm.dbg.declare(metadata i8** %9, metadata !3280, metadata !DIExpression()), !dbg !3282
  %54 = load i32, i32* %8, align 4, !dbg !3283
  %55 = add i32 %54, 1, !dbg !3284
  %56 = call i8* @do_malloc(i32 noundef %55) #9, !dbg !3285
  store i8* %56, i8** %9, align 4, !dbg !3282
  %57 = load i8*, i8** %9, align 4, !dbg !3286
  %58 = load i32, i32* %8, align 4, !dbg !3287
  %59 = load i8*, i8** %4, align 4, !dbg !3288
  %60 = call i32 (i8*, i32, i8*, ...) @snprintf(i8* noundef %57, i32 noundef %58, i8* noundef getelementptr inbounds ([7 x i8], [7 x i8]* @.str.53, i32 0, i32 0), i8* noundef %59) #9, !dbg !3289
  %61 = load i8*, i8** %9, align 4, !dbg !3290
  %62 = call i32 @access(i8* noundef %61, i32 noundef 4) #9, !dbg !3292
  %63 = icmp slt i32 %62, 0, !dbg !3293
  br i1 %63, label %64, label %66, !dbg !3294

64:                                               ; preds = %50
  %65 = load i8*, i8** %9, align 4, !dbg !3295
  call void @free(i8* noundef %65) #9, !dbg !3297
  store i32 0, i32* %3, align 4, !dbg !3298
  br label %75, !dbg !3298

66:                                               ; preds = %50
  %67 = load i8**, i8*** %5, align 4, !dbg !3299
  %68 = icmp ne i8** %67, null, !dbg !3299
  br i1 %68, label %69, label %72, !dbg !3301

69:                                               ; preds = %66
  %70 = load i8*, i8** %9, align 4, !dbg !3302
  %71 = load i8**, i8*** %5, align 4, !dbg !3304
  store i8* %70, i8** %71, align 4, !dbg !3305
  br label %74, !dbg !3306

72:                                               ; preds = %66
  %73 = load i8*, i8** %9, align 4, !dbg !3307
  call void @free(i8* noundef %73) #9, !dbg !3309
  br label %74

74:                                               ; preds = %72, %69
  store i32 1, i32* %3, align 4, !dbg !3310
  br label %75, !dbg !3310

75:                                               ; preds = %74, %64, %44, %24
  %76 = load i32, i32* %3, align 4, !dbg !3311
  ret i32 %76, !dbg !3311
}

declare i8* @strrchr(i8* noundef, i32 noundef) #2

declare i32 @access(i8* noundef, i32 noundef) #2

declare %struct._DIR* @opendir(i8* noundef) #2

declare %struct.dirent* @readdir(%struct._DIR* noundef) #2

declare i8* @realloc(i8* noundef, i32 noundef) #2

declare i32 @closedir(%struct._DIR* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i32 @names_compare(i8* noundef %0, i8* noundef %1) #0 !dbg !3312 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  call void @llvm.dbg.declare(metadata i8** %3, metadata !3317, metadata !DIExpression()), !dbg !3318
  store i8* %1, i8** %4, align 4
  call void @llvm.dbg.declare(metadata i8** %4, metadata !3319, metadata !DIExpression()), !dbg !3320
  %5 = load i8*, i8** %3, align 4, !dbg !3321
  %6 = bitcast i8* %5 to i8**, !dbg !3322
  %7 = load i8*, i8** %6, align 4, !dbg !3323
  %8 = load i8*, i8** %4, align 4, !dbg !3324
  %9 = bitcast i8* %8 to i8**, !dbg !3325
  %10 = load i8*, i8** %9, align 4, !dbg !3326
  %11 = call i32 @strcmp(i8* noundef %7, i8* noundef %10) #13, !dbg !3327
  ret i32 %11, !dbg !3328
}

declare void @qsort(i8* noundef, i32 noundef, i32 noundef, i32 (i8*, i8*)* noundef) #2

; Function Attrs: noinline nounwind optnone
define internal i32 @cond_top_level__add_file(i8* noundef %0, i8* noundef %1) #0 !dbg !3329 {
  %3 = alloca i32, align 4
  %4 = alloca i8*, align 4
  %5 = alloca i8*, align 4
  store i8* %0, i8** %4, align 4
  call void @llvm.dbg.declare(metadata i8** %4, metadata !3332, metadata !DIExpression()), !dbg !3333
  store i8* %1, i8** %5, align 4
  call void @llvm.dbg.declare(metadata i8** %5, metadata !3334, metadata !DIExpression()), !dbg !3335
  %6 = load i8*, i8** %4, align 4, !dbg !3336
  %7 = load i8*, i8** %5, align 4, !dbg !3338
  %8 = call i32 @strcmp(i8* noundef %6, i8* noundef %7) #13, !dbg !3339
  %9 = icmp ne i32 %8, 0, !dbg !3339
  br i1 %9, label %11, label %10, !dbg !3340

10:                                               ; preds = %2
  store i32 0, i32* %3, align 4, !dbg !3341
  br label %14, !dbg !3341

11:                                               ; preds = %2
  %12 = load i8*, i8** %4, align 4, !dbg !3343
  %13 = call i32 @top_level__add_file(i8* noundef %12) #9, !dbg !3344
  store i32 %13, i32* %3, align 4, !dbg !3345
  br label %14, !dbg !3345

14:                                               ; preds = %11, %10
  %15 = load i32, i32* %3, align 4, !dbg !3346
  ret i32 %15, !dbg !3346
}

attributes #0 = { noinline nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { "frame-pointer"="none" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #3 = { noreturn "frame-pointer"="none" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #4 = { nounwind "frame-pointer"="none" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #5 = { nocallback nounwind readonly willreturn "frame-pointer"="none" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #6 = { convergent nofree nosync nounwind readnone willreturn }
attributes #7 = { noinline noreturn nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #8 = { nounwind readonly willreturn "frame-pointer"="none" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #9 = { nobuiltin "no-builtins" }
attributes #10 = { nobuiltin noreturn "no-builtins" }
attributes #11 = { nobuiltin nocallback nounwind readonly willreturn "no-builtins" }
attributes #12 = { nobuiltin nounwind "no-builtins" }
attributes #13 = { nobuiltin nounwind readonly willreturn "no-builtins" }

!llvm.dbg.cu = !{!2}
!llvm.ident = !{!154}
!llvm.module.flags = !{!155, !156, !157}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "env_lang", scope: !2, file: !32, line: 149, type: !8, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang version 14.0.4 (https://github.com/llvm/llvm-project 29f1039a7285a5c3a9c353d054140bf2556d4c4d)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !4, globals: !29, splitDebugInlining: false, nameTableKind: None)
!3 = !DIFile(filename: "/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c", directory: "/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/build")
!4 = !{!5, !6, !11, !10, !13, !14, !15, !8, !16, !18, !22, !23, !25, !27, !28}
!5 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!6 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !7, size: 32)
!7 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !8)
!8 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 32)
!9 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !10)
!10 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!11 = !DIDerivedType(tag: DW_TAG_typedef, name: "int32_t", file: !12, line: 140, baseType: !13)
!12 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/bits/alltypes.h", directory: "")
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint32_t", file: !12, line: 165, baseType: !5)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 32)
!16 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !17, size: 32)
!17 = !DIBasicType(name: "unsigned char", size: 8, encoding: DW_ATE_unsigned_char)
!18 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !19, size: 32)
!19 = !DIDerivedType(tag: DW_TAG_typedef, name: "time_t", file: !20, line: 5, baseType: !21)
!20 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_time_t.h", directory: "")
!21 = !DIBasicType(name: "long long", size: 64, encoding: DW_ATE_signed)
!22 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 32)
!23 = !DIDerivedType(tag: DW_TAG_typedef, name: "off_t", file: !24, line: 5, baseType: !21)
!24 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_off_t.h", directory: "")
!25 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !26, line: 46, baseType: !27)
!26 = !DIFile(filename: "/opt/wasi-sdk-16.0/lib/clang/14.0.4/include/stddef.h", directory: "")
!27 = !DIBasicType(name: "unsigned long", size: 32, encoding: DW_ATE_unsigned)
!28 = !DIBasicType(name: "long", size: 32, encoding: DW_ATE_signed)
!29 = !{!30, !45, !0, !86, !88, !90, !92, !101, !103, !112, !114, !116, !118, !120, !122, !127, !129, !131, !133, !135, !137, !142, !144, !146, !148, !150, !152}
!30 = !DIGlobalVariableExpression(var: !31, expr: !DIExpression())
!31 = distinct !DIGlobalVariable(name: "Re_pat", scope: !2, file: !32, line: 170, type: !33, isLocal: true, isDefinition: true)
!32 = !DIFile(filename: "fortune/fortune.c", directory: "/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod")
!33 = !DIDerivedType(tag: DW_TAG_typedef, name: "regex_t", file: !34, line: 20, baseType: !35)
!34 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/regex.h", directory: "")
!35 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "re_pattern_buffer", file: !34, line: 15, size: 256, elements: !36)
!36 = !{!37, !38, !39, !43, !44}
!37 = !DIDerivedType(tag: DW_TAG_member, name: "re_nsub", scope: !35, file: !34, line: 16, baseType: !25, size: 32)
!38 = !DIDerivedType(tag: DW_TAG_member, name: "__opaque", scope: !35, file: !34, line: 17, baseType: !22, size: 32, offset: 32)
!39 = !DIDerivedType(tag: DW_TAG_member, name: "__padding", scope: !35, file: !34, line: 17, baseType: !40, size: 128, offset: 64)
!40 = !DICompositeType(tag: DW_TAG_array_type, baseType: !22, size: 128, elements: !41)
!41 = !{!42}
!42 = !DISubrange(count: 4)
!43 = !DIDerivedType(tag: DW_TAG_member, name: "__nsub2", scope: !35, file: !34, line: 18, baseType: !25, size: 32, offset: 192)
!44 = !DIDerivedType(tag: DW_TAG_member, name: "__padding2", scope: !35, file: !34, line: 19, baseType: !10, size: 8, offset: 224)
!45 = !DIGlobalVariableExpression(var: !46, expr: !DIExpression())
!46 = distinct !DIGlobalVariable(name: "Fortfile", scope: !2, file: !32, line: 193, type: !47, isLocal: true, isDefinition: true)
!47 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !48, size: 32)
!48 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILEDESC", file: !32, line: 147, baseType: !49)
!49 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "fd", file: !32, line: 131, size: 672, elements: !50)
!50 = !{!51, !52, !53, !54, !55, !59, !60, !61, !62, !63, !65, !66, !67, !80, !81, !83, !84, !85}
!51 = !DIDerivedType(tag: DW_TAG_member, name: "percent", scope: !49, file: !32, line: 133, baseType: !13, size: 32)
!52 = !DIDerivedType(tag: DW_TAG_member, name: "fd", scope: !49, file: !32, line: 134, baseType: !13, size: 32, offset: 32)
!53 = !DIDerivedType(tag: DW_TAG_member, name: "datfd", scope: !49, file: !32, line: 134, baseType: !13, size: 32, offset: 64)
!54 = !DIDerivedType(tag: DW_TAG_member, name: "pos", scope: !49, file: !32, line: 135, baseType: !11, size: 32, offset: 96)
!55 = !DIDerivedType(tag: DW_TAG_member, name: "inf", scope: !49, file: !32, line: 136, baseType: !56, size: 32, offset: 128)
!56 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !57, size: 32)
!57 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !12, line: 361, baseType: !58)
!58 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_FILE", file: !12, line: 361, flags: DIFlagFwdDecl)
!59 = !DIDerivedType(tag: DW_TAG_member, name: "name", scope: !49, file: !32, line: 137, baseType: !15, size: 32, offset: 160)
!60 = !DIDerivedType(tag: DW_TAG_member, name: "path", scope: !49, file: !32, line: 138, baseType: !15, size: 32, offset: 192)
!61 = !DIDerivedType(tag: DW_TAG_member, name: "datfile", scope: !49, file: !32, line: 139, baseType: !15, size: 32, offset: 224)
!62 = !DIDerivedType(tag: DW_TAG_member, name: "posfile", scope: !49, file: !32, line: 139, baseType: !15, size: 32, offset: 256)
!63 = !DIDerivedType(tag: DW_TAG_member, name: "read_tbl", scope: !49, file: !32, line: 140, baseType: !64, size: 8, offset: 288)
!64 = !DIBasicType(name: "_Bool", size: 8, encoding: DW_ATE_boolean)
!65 = !DIDerivedType(tag: DW_TAG_member, name: "was_pos_file", scope: !49, file: !32, line: 141, baseType: !64, size: 8, offset: 296)
!66 = !DIDerivedType(tag: DW_TAG_member, name: "utf8_charset", scope: !49, file: !32, line: 142, baseType: !64, size: 8, offset: 304)
!67 = !DIDerivedType(tag: DW_TAG_member, name: "tbl", scope: !49, file: !32, line: 143, baseType: !68, size: 192, offset: 320)
!68 = !DIDerivedType(tag: DW_TAG_typedef, name: "STRFILE", file: !69, line: 58, baseType: !70)
!69 = !DIFile(filename: "util/strfile.h", directory: "/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod")
!70 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !69, line: 45, size: 192, elements: !71)
!71 = !{!72, !73, !74, !75, !76, !77}
!72 = !DIDerivedType(tag: DW_TAG_member, name: "str_version", scope: !70, file: !69, line: 48, baseType: !14, size: 32)
!73 = !DIDerivedType(tag: DW_TAG_member, name: "str_numstr", scope: !70, file: !69, line: 49, baseType: !14, size: 32, offset: 32)
!74 = !DIDerivedType(tag: DW_TAG_member, name: "str_longlen", scope: !70, file: !69, line: 50, baseType: !14, size: 32, offset: 64)
!75 = !DIDerivedType(tag: DW_TAG_member, name: "str_shortlen", scope: !70, file: !69, line: 51, baseType: !14, size: 32, offset: 96)
!76 = !DIDerivedType(tag: DW_TAG_member, name: "str_flags", scope: !70, file: !69, line: 55, baseType: !14, size: 32, offset: 128)
!77 = !DIDerivedType(tag: DW_TAG_member, name: "stuff", scope: !70, file: !69, line: 56, baseType: !78, size: 32, offset: 160)
!78 = !DICompositeType(tag: DW_TAG_array_type, baseType: !79, size: 32, elements: !41)
!79 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint8_t", file: !12, line: 155, baseType: !17)
!80 = !DIDerivedType(tag: DW_TAG_member, name: "num_children", scope: !49, file: !32, line: 144, baseType: !13, size: 32, offset: 512)
!81 = !DIDerivedType(tag: DW_TAG_member, name: "child", scope: !49, file: !32, line: 145, baseType: !82, size: 32, offset: 544)
!82 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !49, size: 32)
!83 = !DIDerivedType(tag: DW_TAG_member, name: "parent", scope: !49, file: !32, line: 145, baseType: !82, size: 32, offset: 576)
!84 = !DIDerivedType(tag: DW_TAG_member, name: "next", scope: !49, file: !32, line: 146, baseType: !82, size: 32, offset: 608)
!85 = !DIDerivedType(tag: DW_TAG_member, name: "prev", scope: !49, file: !32, line: 146, baseType: !82, size: 32, offset: 640)
!86 = !DIGlobalVariableExpression(var: !87, expr: !DIExpression())
!87 = distinct !DIGlobalVariable(name: "All_forts", scope: !2, file: !32, line: 156, type: !64, isLocal: true, isDefinition: true)
!88 = !DIGlobalVariableExpression(var: !89, expr: !DIExpression())
!89 = distinct !DIGlobalVariable(name: "Offend", scope: !2, file: !32, line: 155, type: !64, isLocal: true, isDefinition: true)
!90 = !DIGlobalVariableExpression(var: !91, expr: !DIExpression())
!91 = distinct !DIGlobalVariable(name: "No_recode", scope: !2, file: !32, line: 159, type: !64, isLocal: true, isDefinition: true)
!92 = !DIGlobalVariableExpression(var: !93, expr: !DIExpression())
!93 = distinct !DIGlobalVariable(name: "buf", scope: !94, file: !32, line: 240, type: !98, isLocal: true, isDefinition: true)
!94 = distinct !DISubprogram(name: "program_version", scope: !32, file: !32, line: 238, type: !95, scopeLine: 239, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!95 = !DISubroutineType(types: !96)
!96 = !{!15}
!97 = !{}
!98 = !DICompositeType(tag: DW_TAG_array_type, baseType: !10, size: 8192, elements: !99)
!99 = !{!100}
!100 = !DISubrange(count: 1024)
!101 = !DIGlobalVariableExpression(var: !102, expr: !DIExpression())
!102 = distinct !DIGlobalVariable(name: "Show_filename", scope: !2, file: !32, line: 158, type: !64, isLocal: true, isDefinition: true)
!103 = !DIGlobalVariableExpression(var: !104, expr: !DIExpression())
!104 = distinct !DIGlobalVariable(name: "suflist", scope: !105, file: !32, line: 509, type: !109, isLocal: true, isDefinition: true)
!105 = distinct !DISubprogram(name: "is_fortfile", scope: !32, file: !32, line: 506, type: !106, scopeLine: 507, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!106 = !DISubroutineType(types: !107)
!107 = !{!13, !7, !108}
!108 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 32)
!109 = !DICompositeType(tag: DW_TAG_array_type, baseType: !8, size: 448, elements: !110)
!110 = !{!111}
!111 = !DISubrange(count: 14)
!112 = !DIGlobalVariableExpression(var: !113, expr: !DIExpression())
!113 = distinct !DIGlobalVariable(name: "File_tail", scope: !2, file: !32, line: 192, type: !47, isLocal: true, isDefinition: true)
!114 = !DIGlobalVariableExpression(var: !115, expr: !DIExpression())
!115 = distinct !DIGlobalVariable(name: "ErrorMessage", scope: !2, file: !32, line: 161, type: !64, isLocal: true, isDefinition: true)
!116 = !DIGlobalVariableExpression(var: !117, expr: !DIExpression())
!117 = distinct !DIGlobalVariable(name: "Match", scope: !2, file: !32, line: 176, type: !64, isLocal: true, isDefinition: true)
!118 = !DIGlobalVariableExpression(var: !119, expr: !DIExpression())
!119 = distinct !DIGlobalVariable(name: "Spec_prob", scope: !2, file: !32, line: 185, type: !13, isLocal: true, isDefinition: true)
!120 = !DIGlobalVariableExpression(var: !121, expr: !DIExpression())
!121 = distinct !DIGlobalVariable(name: "Find_files", scope: !2, file: !32, line: 151, type: !64, isLocal: true, isDefinition: true)
!122 = !DIGlobalVariableExpression(var: !123, expr: !DIExpression())
!123 = distinct !DIGlobalVariable(name: "did_noprobs", scope: !124, file: !32, line: 1542, type: !64, isLocal: true, isDefinition: true)
!124 = distinct !DISubprogram(name: "sum_noprobs", scope: !32, file: !32, line: 1540, type: !125, scopeLine: 1541, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!125 = !DISubroutineType(types: !126)
!126 = !{null, !47}
!127 = !DIGlobalVariableExpression(var: !128, expr: !DIExpression())
!128 = distinct !DIGlobalVariable(name: "Noprob_tbl", scope: !2, file: !32, line: 195, type: !68, isLocal: true, isDefinition: true)
!129 = !DIGlobalVariableExpression(var: !130, expr: !DIExpression())
!130 = distinct !DIGlobalVariable(name: "File_list", scope: !2, file: !32, line: 191, type: !47, isLocal: true, isDefinition: true)
!131 = !DIGlobalVariableExpression(var: !132, expr: !DIExpression())
!132 = distinct !DIGlobalVariable(name: "Equal_probs", scope: !2, file: !32, line: 157, type: !64, isLocal: true, isDefinition: true)
!133 = !DIGlobalVariableExpression(var: !134, expr: !DIExpression())
!134 = distinct !DIGlobalVariable(name: "Num_kids", scope: !2, file: !32, line: 186, type: !13, isLocal: true, isDefinition: true)
!135 = !DIGlobalVariableExpression(var: !136, expr: !DIExpression())
!136 = distinct !DIGlobalVariable(name: "Num_files", scope: !2, file: !32, line: 186, type: !13, isLocal: true, isDefinition: true)
!137 = !DIGlobalVariableExpression(var: !138, expr: !DIExpression())
!138 = distinct !DIGlobalVariable(name: "Seekpts", scope: !2, file: !32, line: 189, type: !139, isLocal: true, isDefinition: true)
!139 = !DICompositeType(tag: DW_TAG_array_type, baseType: !11, size: 64, elements: !140)
!140 = !{!141}
!141 = !DISubrange(count: 2)
!142 = !DIGlobalVariableExpression(var: !143, expr: !DIExpression())
!143 = distinct !DIGlobalVariable(name: "Short_only", scope: !2, file: !32, line: 153, type: !64, isLocal: true, isDefinition: true)
!144 = !DIGlobalVariableExpression(var: !145, expr: !DIExpression())
!145 = distinct !DIGlobalVariable(name: "SLEN", scope: !2, file: !32, line: 187, type: !13, isLocal: true, isDefinition: true)
!146 = !DIGlobalVariableExpression(var: !147, expr: !DIExpression())
!147 = distinct !DIGlobalVariable(name: "Long_only", scope: !2, file: !32, line: 154, type: !64, isLocal: true, isDefinition: true)
!148 = !DIGlobalVariableExpression(var: !149, expr: !DIExpression())
!149 = distinct !DIGlobalVariable(name: "Wait", scope: !2, file: !32, line: 152, type: !64, isLocal: true, isDefinition: true)
!150 = !DIGlobalVariableExpression(var: !151, expr: !DIExpression())
!151 = distinct !DIGlobalVariable(name: "Fort_len", scope: !2, file: !32, line: 185, type: !13, isLocal: true, isDefinition: true)
!152 = !DIGlobalVariableExpression(var: !153, expr: !DIExpression())
!153 = distinct !DIGlobalVariable(name: "Fortbuf", scope: !2, file: !32, line: 183, type: !16, isLocal: true, isDefinition: true)
!154 = !{!"clang version 14.0.4 (https://github.com/llvm/llvm-project 29f1039a7285a5c3a9c353d054140bf2556d4c4d)"}
!155 = !{i32 7, !"Dwarf Version", i32 4}
!156 = !{i32 2, !"Debug Info Version", i32 3}
!157 = !{i32 1, !"wchar_size", i32 4}
!158 = distinct !DISubprogram(name: "main", linkageName: "__main_argc_argv", scope: !32, file: !32, line: 1958, type: !159, scopeLine: 1959, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !97)
!159 = !DISubroutineType(types: !160)
!160 = !{!13, !13, !108}
!161 = !DILocalVariable(name: "argc", arg: 1, scope: !158, file: !32, line: 1958, type: !13)
!162 = !DILocation(line: 1958, column: 14, scope: !158)
!163 = !DILocalVariable(name: "argv", arg: 2, scope: !158, file: !32, line: 1958, type: !108)
!164 = !DILocation(line: 1958, column: 26, scope: !158)
!165 = !DILocalVariable(name: "exit_code", scope: !158, file: !32, line: 1964, type: !13)
!166 = !DILocation(line: 1964, column: 9, scope: !158)
!167 = !DILocation(line: 1965, column: 16, scope: !158)
!168 = !DILocation(line: 1965, column: 14, scope: !158)
!169 = !DILocation(line: 1966, column: 10, scope: !170)
!170 = distinct !DILexicalBlock(scope: !158, file: !32, line: 1966, column: 9)
!171 = !DILocation(line: 1966, column: 9, scope: !158)
!172 = !DILocation(line: 1968, column: 20, scope: !173)
!173 = distinct !DILexicalBlock(scope: !170, file: !32, line: 1967, column: 5)
!174 = !DILocation(line: 1968, column: 18, scope: !173)
!175 = !DILocation(line: 1969, column: 5, scope: !173)
!176 = !DILocation(line: 1970, column: 10, scope: !177)
!177 = distinct !DILexicalBlock(scope: !158, file: !32, line: 1970, column: 9)
!178 = !DILocation(line: 1970, column: 9, scope: !158)
!179 = !DILocation(line: 1972, column: 20, scope: !180)
!180 = distinct !DILexicalBlock(scope: !177, file: !32, line: 1971, column: 5)
!181 = !DILocation(line: 1972, column: 18, scope: !180)
!182 = !DILocation(line: 1973, column: 5, scope: !180)
!183 = !DILocation(line: 1974, column: 10, scope: !184)
!184 = distinct !DILexicalBlock(scope: !158, file: !32, line: 1974, column: 9)
!185 = !DILocation(line: 1974, column: 9, scope: !158)
!186 = !DILocation(line: 1976, column: 20, scope: !187)
!187 = distinct !DILexicalBlock(scope: !184, file: !32, line: 1975, column: 5)
!188 = !DILocation(line: 1976, column: 18, scope: !187)
!189 = !DILocation(line: 1977, column: 5, scope: !187)
!190 = !DILocation(line: 1985, column: 13, scope: !158)
!191 = !DILocation(line: 1985, column: 19, scope: !158)
!192 = !DILocation(line: 1985, column: 5, scope: !158)
!193 = !DILocation(line: 1991, column: 5, scope: !158)
!194 = !DILocation(line: 2015, column: 9, scope: !195)
!195 = distinct !DILexicalBlock(scope: !158, file: !32, line: 2015, column: 9)
!196 = !DILocation(line: 2015, column: 9, scope: !158)
!197 = !DILocation(line: 2017, column: 21, scope: !198)
!198 = distinct !DILexicalBlock(scope: !195, file: !32, line: 2016, column: 5)
!199 = !DILocation(line: 2017, column: 19, scope: !198)
!200 = !DILocation(line: 2018, column: 9, scope: !198)
!201 = !DILocation(line: 2019, column: 9, scope: !198)
!202 = !DILocation(line: 2023, column: 5, scope: !158)
!203 = !DILocation(line: 2024, column: 9, scope: !204)
!204 = distinct !DILexicalBlock(scope: !158, file: !32, line: 2024, column: 9)
!205 = !DILocation(line: 2024, column: 9, scope: !158)
!206 = !DILocation(line: 2026, column: 21, scope: !207)
!207 = distinct !DILexicalBlock(scope: !204, file: !32, line: 2025, column: 5)
!208 = !DILocation(line: 2026, column: 9, scope: !207)
!209 = !DILocation(line: 2027, column: 13, scope: !210)
!210 = distinct !DILexicalBlock(scope: !207, file: !32, line: 2027, column: 13)
!211 = !DILocation(line: 2027, column: 13, scope: !207)
!212 = !DILocation(line: 2029, column: 13, scope: !213)
!213 = distinct !DILexicalBlock(scope: !210, file: !32, line: 2028, column: 9)
!214 = !DILocation(line: 2030, column: 9, scope: !213)
!215 = !DILocation(line: 2031, column: 20, scope: !207)
!216 = !DILocation(line: 2031, column: 9, scope: !207)
!217 = !DILocation(line: 2032, column: 5, scope: !207)
!218 = !DILocation(line: 2035, column: 9, scope: !219)
!219 = distinct !DILexicalBlock(scope: !204, file: !32, line: 2034, column: 5)
!220 = !DILocation(line: 2036, column: 9, scope: !219)
!221 = !DILocation(line: 2038, column: 13, scope: !222)
!222 = distinct !DILexicalBlock(scope: !219, file: !32, line: 2037, column: 9)
!223 = !DILocation(line: 2039, column: 9, scope: !222)
!224 = !DILocation(line: 2039, column: 19, scope: !219)
!225 = !DILocation(line: 2039, column: 30, scope: !219)
!226 = !DILocation(line: 2039, column: 33, scope: !219)
!227 = !DILocation(line: 2039, column: 45, scope: !219)
!228 = !DILocation(line: 2039, column: 43, scope: !219)
!229 = !DILocation(line: 2039, column: 51, scope: !219)
!230 = !DILocation(line: 2040, column: 19, scope: !219)
!231 = !DILocation(line: 2040, column: 29, scope: !219)
!232 = !DILocation(line: 2040, column: 32, scope: !219)
!233 = !DILocation(line: 2040, column: 45, scope: !219)
!234 = !DILocation(line: 2040, column: 42, scope: !219)
!235 = !DILocation(line: 0, scope: !219)
!236 = distinct !{!236, !220, !237, !238}
!237 = !DILocation(line: 2040, column: 50, scope: !219)
!238 = !{!"llvm.loop.mustprogress"}
!239 = !DILocation(line: 2042, column: 17, scope: !219)
!240 = !DILocation(line: 2042, column: 9, scope: !219)
!241 = !DILocation(line: 2044, column: 13, scope: !242)
!242 = distinct !DILexicalBlock(scope: !219, file: !32, line: 2044, column: 13)
!243 = !DILocation(line: 2044, column: 13, scope: !219)
!244 = !DILocation(line: 2046, column: 13, scope: !245)
!245 = distinct !DILexicalBlock(scope: !242, file: !32, line: 2045, column: 9)
!246 = !DILocation(line: 2047, column: 39, scope: !245)
!247 = !DILocation(line: 2047, column: 48, scope: !245)
!248 = !DILocation(line: 2047, column: 33, scope: !245)
!249 = !DILocation(line: 2047, column: 13, scope: !245)
!250 = !DILocation(line: 2048, column: 9, scope: !245)
!251 = !DILabel(scope: !158, name: "cleanup", file: !32, line: 2051)
!252 = !DILocation(line: 2051, column: 1, scope: !158)
!253 = !DILocation(line: 2060, column: 15, scope: !158)
!254 = !DILocation(line: 2060, column: 5, scope: !158)
!255 = !DILocation(line: 2061, column: 10, scope: !158)
!256 = !DILocation(line: 2061, column: 5, scope: !158)
!257 = !DILocation(line: 2062, column: 10, scope: !158)
!258 = !DILocation(line: 2062, column: 5, scope: !158)
!259 = distinct !DISubprogram(name: "getargs", scope: !32, file: !32, line: 1208, type: !260, scopeLine: 1209, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!260 = !DISubroutineType(types: !261)
!261 = !{null, !13, !108}
!262 = !DILocalVariable(name: "argc", arg: 1, scope: !259, file: !32, line: 1208, type: !13)
!263 = !DILocation(line: 1208, column: 25, scope: !259)
!264 = !DILocalVariable(name: "argv", arg: 2, scope: !259, file: !32, line: 1208, type: !108)
!265 = !DILocation(line: 1208, column: 38, scope: !259)
!266 = !DILocalVariable(name: "ignore_case", scope: !259, file: !32, line: 1211, type: !64)
!267 = !DILocation(line: 1211, column: 10, scope: !259)
!268 = !DILocalVariable(name: "pat", scope: !259, file: !32, line: 1212, type: !15)
!269 = !DILocation(line: 1212, column: 11, scope: !259)
!270 = !DILocalVariable(name: "ch", scope: !259, file: !32, line: 1215, type: !13)
!271 = !DILocation(line: 1215, column: 9, scope: !259)
!272 = !DILocation(line: 1225, column: 5, scope: !259)
!273 = !DILocation(line: 1225, column: 25, scope: !259)
!274 = !DILocation(line: 1225, column: 31, scope: !259)
!275 = !DILocation(line: 1225, column: 18, scope: !259)
!276 = !DILocation(line: 1225, column: 16, scope: !259)
!277 = !DILocation(line: 1226, column: 73, scope: !259)
!278 = !DILocation(line: 1228, column: 17, scope: !279)
!279 = distinct !DILexicalBlock(scope: !259, file: !32, line: 1227, column: 5)
!280 = !DILocation(line: 1228, column: 9, scope: !279)
!281 = !DILocation(line: 1231, column: 23, scope: !282)
!282 = distinct !DILexicalBlock(scope: !279, file: !32, line: 1229, column: 9)
!283 = !DILocation(line: 1232, column: 13, scope: !282)
!284 = !DILocation(line: 1239, column: 25, scope: !282)
!285 = !DILocation(line: 1240, column: 13, scope: !282)
!286 = !DILocation(line: 1242, column: 24, scope: !282)
!287 = !DILocation(line: 1243, column: 13, scope: !282)
!288 = !DILocation(line: 1245, column: 23, scope: !282)
!289 = !DILocation(line: 1246, column: 24, scope: !282)
!290 = !DILocation(line: 1247, column: 13, scope: !282)
!291 = !DILocation(line: 1249, column: 25, scope: !282)
!292 = !DILocation(line: 1249, column: 20, scope: !282)
!293 = !DILocation(line: 1249, column: 18, scope: !282)
!294 = !DILocation(line: 1250, column: 13, scope: !282)
!295 = !DILocation(line: 1253, column: 20, scope: !282)
!296 = !DILocation(line: 1261, column: 13, scope: !282)
!297 = !DILocation(line: 1263, column: 24, scope: !282)
!298 = !DILocation(line: 1264, column: 23, scope: !282)
!299 = !DILocation(line: 1265, column: 13, scope: !282)
!300 = !DILocation(line: 1267, column: 18, scope: !282)
!301 = !DILocation(line: 1268, column: 13, scope: !282)
!302 = !DILocation(line: 1278, column: 25, scope: !282)
!303 = !DILocation(line: 1279, column: 13, scope: !282)
!304 = !DILocation(line: 1281, column: 19, scope: !282)
!305 = !DILocation(line: 1282, column: 19, scope: !282)
!306 = !DILocation(line: 1282, column: 17, scope: !282)
!307 = !DILocation(line: 1283, column: 13, scope: !282)
!308 = !DILocation(line: 1286, column: 23, scope: !282)
!309 = !DILocation(line: 1287, column: 13, scope: !282)
!310 = !DILocation(line: 1289, column: 34, scope: !282)
!311 = !DILocation(line: 1289, column: 19, scope: !282)
!312 = !DILocation(line: 1290, column: 13, scope: !282)
!313 = !DILocation(line: 1292, column: 27, scope: !282)
!314 = !DILocation(line: 1293, column: 13, scope: !282)
!315 = !DILocation(line: 1295, column: 9, scope: !282)
!316 = !DILocation(line: 1297, column: 13, scope: !282)
!317 = distinct !{!317, !272, !318, !238}
!318 = !DILocation(line: 1299, column: 5, scope: !259)
!319 = !DILocation(line: 1300, column: 13, scope: !259)
!320 = !DILocation(line: 1300, column: 10, scope: !259)
!321 = !DILocation(line: 1301, column: 13, scope: !259)
!322 = !DILocation(line: 1301, column: 10, scope: !259)
!323 = !DILocation(line: 1303, column: 25, scope: !324)
!324 = distinct !DILexicalBlock(scope: !259, file: !32, line: 1303, column: 9)
!325 = !DILocation(line: 1303, column: 31, scope: !324)
!326 = !DILocation(line: 1303, column: 10, scope: !324)
!327 = !DILocation(line: 1303, column: 9, scope: !259)
!328 = !DILocation(line: 1305, column: 14, scope: !329)
!329 = distinct !DILexicalBlock(scope: !330, file: !32, line: 1305, column: 13)
!330 = distinct !DILexicalBlock(scope: !324, file: !32, line: 1304, column: 5)
!331 = !DILocation(line: 1305, column: 13, scope: !330)
!332 = !DILocation(line: 1307, column: 21, scope: !333)
!333 = distinct !DILexicalBlock(scope: !329, file: !32, line: 1306, column: 9)
!334 = !DILocation(line: 1307, column: 13, scope: !333)
!335 = !DILocation(line: 1308, column: 9, scope: !333)
!336 = !DILocation(line: 1309, column: 9, scope: !330)
!337 = !DILocation(line: 1321, column: 9, scope: !338)
!338 = distinct !DILexicalBlock(scope: !259, file: !32, line: 1321, column: 9)
!339 = !DILocation(line: 1321, column: 9, scope: !259)
!340 = !DILocation(line: 1323, column: 13, scope: !341)
!341 = distinct !DILexicalBlock(scope: !342, file: !32, line: 1323, column: 13)
!342 = distinct !DILexicalBlock(scope: !338, file: !32, line: 1322, column: 5)
!343 = !DILocation(line: 1323, column: 13, scope: !342)
!344 = !DILocation(line: 1325, column: 28, scope: !345)
!345 = distinct !DILexicalBlock(scope: !341, file: !32, line: 1324, column: 9)
!346 = !DILocation(line: 1325, column: 19, scope: !345)
!347 = !DILocation(line: 1325, column: 17, scope: !345)
!348 = !DILocation(line: 1326, column: 9, scope: !345)
!349 = !DILocation(line: 1327, column: 13, scope: !350)
!350 = distinct !DILexicalBlock(scope: !342, file: !32, line: 1327, column: 13)
!351 = !DILocation(line: 1327, column: 13, scope: !342)
!352 = !DILocation(line: 1329, column: 21, scope: !353)
!353 = distinct !DILexicalBlock(scope: !350, file: !32, line: 1328, column: 9)
!354 = !DILocation(line: 1329, column: 50, scope: !353)
!355 = !DILocation(line: 1329, column: 13, scope: !353)
!356 = !DILocation(line: 1330, column: 13, scope: !353)
!357 = !DILocation(line: 1332, column: 13, scope: !358)
!358 = distinct !DILexicalBlock(scope: !342, file: !32, line: 1332, column: 13)
!359 = !DILocation(line: 1332, column: 13, scope: !342)
!360 = !DILocation(line: 1334, column: 18, scope: !361)
!361 = distinct !DILexicalBlock(scope: !358, file: !32, line: 1333, column: 9)
!362 = !DILocation(line: 1334, column: 13, scope: !361)
!363 = !DILocation(line: 1335, column: 9, scope: !361)
!364 = !DILocation(line: 1336, column: 5, scope: !342)
!365 = !DILocation(line: 1338, column: 1, scope: !259)
!366 = distinct !DISubprogram(name: "find_matches", scope: !32, file: !32, line: 1853, type: !367, scopeLine: 1854, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!367 = !DISubroutineType(types: !368)
!368 = !{!64}
!369 = !DILocation(line: 1855, column: 31, scope: !366)
!370 = !DILocation(line: 1855, column: 16, scope: !366)
!371 = !DILocation(line: 1855, column: 14, scope: !366)
!372 = !DILocation(line: 1858, column: 39, scope: !366)
!373 = !DILocation(line: 1858, column: 48, scope: !366)
!374 = !DILocation(line: 1858, column: 15, scope: !366)
!375 = !DILocation(line: 1858, column: 13, scope: !366)
!376 = !DILocalVariable(name: "Found_one", scope: !366, file: !32, line: 1860, type: !64)
!377 = !DILocation(line: 1860, column: 10, scope: !366)
!378 = !DILocation(line: 1861, column: 21, scope: !366)
!379 = !DILocation(line: 1861, column: 5, scope: !366)
!380 = !DILocation(line: 1862, column: 12, scope: !366)
!381 = !DILocation(line: 1862, column: 5, scope: !366)
!382 = distinct !DISubprogram(name: "init_prob", scope: !32, file: !32, line: 1344, type: !383, scopeLine: 1345, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!383 = !DISubroutineType(types: !384)
!384 = !{null}
!385 = !DILocalVariable(name: "fp", scope: !382, file: !32, line: 1346, type: !47)
!386 = !DILocation(line: 1346, column: 15, scope: !382)
!387 = !DILocalVariable(name: "percent", scope: !382, file: !32, line: 1347, type: !13)
!388 = !DILocation(line: 1347, column: 9, scope: !382)
!389 = !DILocalVariable(name: "num_noprob", scope: !382, file: !32, line: 1347, type: !13)
!390 = !DILocation(line: 1347, column: 22, scope: !382)
!391 = !DILocalVariable(name: "frac", scope: !382, file: !32, line: 1347, type: !13)
!392 = !DILocation(line: 1347, column: 38, scope: !382)
!393 = !DILocalVariable(name: "last", scope: !382, file: !32, line: 1354, type: !47)
!394 = !DILocation(line: 1354, column: 15, scope: !382)
!395 = !DILocation(line: 1355, column: 15, scope: !396)
!396 = distinct !DILexicalBlock(scope: !382, file: !32, line: 1355, column: 5)
!397 = !DILocation(line: 1355, column: 13, scope: !396)
!398 = !DILocation(line: 1355, column: 10, scope: !396)
!399 = !DILocation(line: 1355, column: 26, scope: !400)
!400 = distinct !DILexicalBlock(scope: !396, file: !32, line: 1355, column: 5)
!401 = !DILocation(line: 1355, column: 5, scope: !396)
!402 = !DILocation(line: 1357, column: 13, scope: !403)
!403 = distinct !DILexicalBlock(scope: !404, file: !32, line: 1357, column: 13)
!404 = distinct !DILexicalBlock(scope: !400, file: !32, line: 1356, column: 5)
!405 = !DILocation(line: 1357, column: 17, scope: !403)
!406 = !DILocation(line: 1357, column: 25, scope: !403)
!407 = !DILocation(line: 1357, column: 13, scope: !404)
!408 = !DILocation(line: 1359, column: 23, scope: !409)
!409 = distinct !DILexicalBlock(scope: !403, file: !32, line: 1358, column: 9)
!410 = !DILocation(line: 1360, column: 17, scope: !411)
!411 = distinct !DILexicalBlock(scope: !409, file: !32, line: 1360, column: 17)
!412 = !DILocation(line: 1360, column: 17, scope: !409)
!413 = !DILocation(line: 1362, column: 24, scope: !414)
!414 = distinct !DILexicalBlock(scope: !411, file: !32, line: 1361, column: 13)
!415 = !DILocation(line: 1362, column: 22, scope: !414)
!416 = !DILocation(line: 1363, column: 13, scope: !414)
!417 = !DILocation(line: 1364, column: 9, scope: !409)
!418 = !DILocation(line: 1367, column: 24, scope: !419)
!419 = distinct !DILexicalBlock(scope: !403, file: !32, line: 1366, column: 9)
!420 = !DILocation(line: 1367, column: 28, scope: !419)
!421 = !DILocation(line: 1367, column: 21, scope: !419)
!422 = !DILocation(line: 1369, column: 5, scope: !404)
!423 = !DILocation(line: 1355, column: 35, scope: !400)
!424 = !DILocation(line: 1355, column: 39, scope: !400)
!425 = !DILocation(line: 1355, column: 33, scope: !400)
!426 = !DILocation(line: 1355, column: 5, scope: !400)
!427 = distinct !{!427, !401, !428, !238}
!428 = !DILocation(line: 1369, column: 5, scope: !396)
!429 = !DILocation(line: 1372, column: 9, scope: !430)
!430 = distinct !DILexicalBlock(scope: !382, file: !32, line: 1372, column: 9)
!431 = !DILocation(line: 1372, column: 17, scope: !430)
!432 = !DILocation(line: 1372, column: 9, scope: !382)
!433 = !DILocation(line: 1374, column: 17, scope: !434)
!434 = distinct !DILexicalBlock(scope: !430, file: !32, line: 1373, column: 5)
!435 = !DILocation(line: 1374, column: 66, scope: !434)
!436 = !DILocation(line: 1374, column: 9, scope: !434)
!437 = !DILocation(line: 1375, column: 9, scope: !434)
!438 = !DILocation(line: 1377, column: 14, scope: !439)
!439 = distinct !DILexicalBlock(scope: !430, file: !32, line: 1377, column: 14)
!440 = !DILocation(line: 1377, column: 22, scope: !439)
!441 = !DILocation(line: 1377, column: 28, scope: !439)
!442 = !DILocation(line: 1377, column: 31, scope: !439)
!443 = !DILocation(line: 1377, column: 42, scope: !439)
!444 = !DILocation(line: 1377, column: 14, scope: !430)
!445 = !DILocation(line: 1379, column: 17, scope: !446)
!446 = distinct !DILexicalBlock(scope: !439, file: !32, line: 1378, column: 5)
!447 = !DILocation(line: 1382, column: 13, scope: !446)
!448 = !DILocation(line: 1379, column: 9, scope: !446)
!449 = !DILocation(line: 1383, column: 9, scope: !446)
!450 = !DILocation(line: 1385, column: 14, scope: !451)
!451 = distinct !DILexicalBlock(scope: !439, file: !32, line: 1385, column: 14)
!452 = !DILocation(line: 1385, column: 22, scope: !451)
!453 = !DILocation(line: 1385, column: 29, scope: !451)
!454 = !DILocation(line: 1385, column: 32, scope: !451)
!455 = !DILocation(line: 1385, column: 43, scope: !451)
!456 = !DILocation(line: 1385, column: 14, scope: !439)
!457 = !DILocation(line: 1387, column: 17, scope: !458)
!458 = distinct !DILexicalBlock(scope: !451, file: !32, line: 1386, column: 5)
!459 = !DILocation(line: 1387, column: 9, scope: !458)
!460 = !DILocation(line: 1389, column: 9, scope: !458)
!461 = !DILocation(line: 1391, column: 17, scope: !382)
!462 = !DILocation(line: 1391, column: 15, scope: !382)
!463 = !DILocation(line: 1393, column: 21, scope: !382)
!464 = !DILocation(line: 1393, column: 19, scope: !382)
!465 = !DILocation(line: 1393, column: 13, scope: !382)
!466 = !DILocation(line: 1394, column: 9, scope: !467)
!467 = distinct !DILexicalBlock(scope: !382, file: !32, line: 1394, column: 9)
!468 = !DILocation(line: 1394, column: 9, scope: !382)
!469 = !DILocation(line: 1396, column: 13, scope: !470)
!470 = distinct !DILexicalBlock(scope: !471, file: !32, line: 1396, column: 13)
!471 = distinct !DILexicalBlock(scope: !467, file: !32, line: 1395, column: 5)
!472 = !DILocation(line: 1396, column: 24, scope: !470)
!473 = !DILocation(line: 1396, column: 13, scope: !471)
!474 = !DILocation(line: 1398, column: 17, scope: !475)
!475 = distinct !DILexicalBlock(scope: !476, file: !32, line: 1398, column: 17)
!476 = distinct !DILexicalBlock(scope: !470, file: !32, line: 1397, column: 9)
!477 = !DILocation(line: 1398, column: 28, scope: !475)
!478 = !DILocation(line: 1398, column: 17, scope: !476)
!479 = !DILocation(line: 1400, column: 24, scope: !480)
!480 = distinct !DILexicalBlock(scope: !475, file: !32, line: 1399, column: 13)
!481 = !DILocation(line: 1400, column: 34, scope: !480)
!482 = !DILocation(line: 1400, column: 32, scope: !480)
!483 = !DILocation(line: 1400, column: 22, scope: !480)
!484 = !DILocation(line: 1402, column: 27, scope: !485)
!485 = distinct !DILexicalBlock(scope: !480, file: !32, line: 1402, column: 17)
!486 = !DILocation(line: 1402, column: 25, scope: !485)
!487 = !DILocation(line: 1402, column: 22, scope: !485)
!488 = !DILocation(line: 1402, column: 38, scope: !489)
!489 = distinct !DILexicalBlock(scope: !485, file: !32, line: 1402, column: 17)
!490 = !DILocation(line: 1402, column: 44, scope: !489)
!491 = !DILocation(line: 1402, column: 41, scope: !489)
!492 = !DILocation(line: 1402, column: 17, scope: !485)
!493 = !DILocation(line: 1404, column: 25, scope: !494)
!494 = distinct !DILexicalBlock(scope: !495, file: !32, line: 1404, column: 25)
!495 = distinct !DILexicalBlock(scope: !489, file: !32, line: 1403, column: 17)
!496 = !DILocation(line: 1404, column: 29, scope: !494)
!497 = !DILocation(line: 1404, column: 37, scope: !494)
!498 = !DILocation(line: 1404, column: 25, scope: !495)
!499 = !DILocation(line: 1406, column: 39, scope: !500)
!500 = distinct !DILexicalBlock(scope: !494, file: !32, line: 1405, column: 21)
!501 = !DILocation(line: 1406, column: 25, scope: !500)
!502 = !DILocation(line: 1406, column: 29, scope: !500)
!503 = !DILocation(line: 1406, column: 37, scope: !500)
!504 = !DILocation(line: 1407, column: 36, scope: !500)
!505 = !DILocation(line: 1407, column: 33, scope: !500)
!506 = !DILocation(line: 1408, column: 21, scope: !500)
!507 = !DILocation(line: 1409, column: 17, scope: !495)
!508 = !DILocation(line: 1402, column: 55, scope: !489)
!509 = !DILocation(line: 1402, column: 59, scope: !489)
!510 = !DILocation(line: 1402, column: 53, scope: !489)
!511 = !DILocation(line: 1402, column: 17, scope: !489)
!512 = distinct !{!512, !492, !513, !238}
!513 = !DILocation(line: 1409, column: 17, scope: !485)
!514 = !DILocation(line: 1410, column: 13, scope: !480)
!515 = !DILocation(line: 1411, column: 29, scope: !476)
!516 = !DILocation(line: 1411, column: 13, scope: !476)
!517 = !DILocation(line: 1411, column: 19, scope: !476)
!518 = !DILocation(line: 1411, column: 27, scope: !476)
!519 = !DILocation(line: 1413, column: 9, scope: !476)
!520 = !DILocation(line: 1419, column: 5, scope: !471)
!521 = !DILocation(line: 1431, column: 1, scope: !382)
!522 = !DILocalVariable(name: "fp", arg: 1, scope: !124, file: !32, line: 1540, type: !47)
!523 = !DILocation(line: 1540, column: 35, scope: !124)
!524 = !DILocation(line: 1544, column: 9, scope: !525)
!525 = distinct !DILexicalBlock(scope: !124, file: !32, line: 1544, column: 9)
!526 = !DILocation(line: 1544, column: 9, scope: !124)
!527 = !DILocation(line: 1546, column: 9, scope: !528)
!528 = distinct !DILexicalBlock(scope: !525, file: !32, line: 1545, column: 5)
!529 = !DILocation(line: 1548, column: 5, scope: !124)
!530 = !DILocation(line: 1549, column: 5, scope: !124)
!531 = !DILocation(line: 1549, column: 12, scope: !124)
!532 = !DILocation(line: 1551, column: 17, scope: !533)
!533 = distinct !DILexicalBlock(scope: !124, file: !32, line: 1550, column: 5)
!534 = !DILocation(line: 1551, column: 9, scope: !533)
!535 = !DILocation(line: 1554, column: 13, scope: !536)
!536 = distinct !DILexicalBlock(scope: !533, file: !32, line: 1554, column: 13)
!537 = !DILocation(line: 1554, column: 17, scope: !536)
!538 = !DILocation(line: 1554, column: 25, scope: !536)
!539 = !DILocation(line: 1554, column: 13, scope: !533)
!540 = !DILocation(line: 1556, column: 35, scope: !541)
!541 = distinct !DILexicalBlock(scope: !536, file: !32, line: 1555, column: 9)
!542 = !DILocation(line: 1556, column: 39, scope: !541)
!543 = !DILocation(line: 1556, column: 13, scope: !541)
!544 = !DILocation(line: 1557, column: 9, scope: !541)
!545 = !DILocation(line: 1558, column: 14, scope: !533)
!546 = !DILocation(line: 1558, column: 18, scope: !533)
!547 = !DILocation(line: 1558, column: 12, scope: !533)
!548 = distinct !{!548, !530, !549, !238}
!549 = !DILocation(line: 1559, column: 5, scope: !124)
!550 = !DILocation(line: 1560, column: 17, scope: !124)
!551 = !DILocation(line: 1561, column: 1, scope: !124)
!552 = distinct !DISubprogram(name: "calc_equal_probs", scope: !32, file: !32, line: 275, type: !383, scopeLine: 276, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!553 = !DILocation(line: 277, column: 26, scope: !552)
!554 = !DILocation(line: 277, column: 15, scope: !552)
!555 = !DILocalVariable(name: "fiddlylist", scope: !552, file: !32, line: 278, type: !47)
!556 = !DILocation(line: 278, column: 15, scope: !552)
!557 = !DILocation(line: 278, column: 28, scope: !552)
!558 = !DILocation(line: 279, column: 5, scope: !552)
!559 = !DILocation(line: 279, column: 12, scope: !552)
!560 = !DILocation(line: 281, column: 18, scope: !561)
!561 = distinct !DILexicalBlock(scope: !552, file: !32, line: 280, column: 5)
!562 = !DILocation(line: 282, column: 21, scope: !561)
!563 = !DILocation(line: 282, column: 33, scope: !561)
!564 = !DILocation(line: 282, column: 18, scope: !561)
!565 = !DILocation(line: 283, column: 22, scope: !561)
!566 = !DILocation(line: 283, column: 34, scope: !561)
!567 = !DILocation(line: 283, column: 20, scope: !561)
!568 = distinct !{!568, !558, !569, !238}
!569 = !DILocation(line: 284, column: 5, scope: !552)
!570 = !DILocation(line: 285, column: 1, scope: !552)
!571 = distinct !DISubprogram(name: "print_list", scope: !32, file: !32, line: 291, type: !572, scopeLine: 292, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!572 = !DISubroutineType(types: !573)
!573 = !{null, !47, !13}
!574 = !DILocalVariable(name: "list", arg: 1, scope: !571, file: !32, line: 291, type: !47)
!575 = !DILocation(line: 291, column: 34, scope: !571)
!576 = !DILocalVariable(name: "lev", arg: 2, scope: !571, file: !32, line: 291, type: !13)
!577 = !DILocation(line: 291, column: 44, scope: !571)
!578 = !DILocation(line: 293, column: 5, scope: !571)
!579 = !DILocation(line: 293, column: 12, scope: !571)
!580 = !DILocation(line: 295, column: 17, scope: !581)
!581 = distinct !DILexicalBlock(scope: !571, file: !32, line: 294, column: 5)
!582 = !DILocation(line: 295, column: 32, scope: !581)
!583 = !DILocation(line: 295, column: 36, scope: !581)
!584 = !DILocation(line: 295, column: 9, scope: !581)
!585 = !DILocation(line: 296, column: 13, scope: !586)
!586 = distinct !DILexicalBlock(scope: !581, file: !32, line: 296, column: 13)
!587 = !DILocation(line: 296, column: 19, scope: !586)
!588 = !DILocation(line: 296, column: 27, scope: !586)
!589 = !DILocation(line: 296, column: 13, scope: !581)
!590 = !DILocation(line: 298, column: 18, scope: !591)
!591 = distinct !DILexicalBlock(scope: !592, file: !32, line: 298, column: 17)
!592 = distinct !DILexicalBlock(scope: !586, file: !32, line: 297, column: 9)
!593 = !DILocation(line: 298, column: 17, scope: !592)
!594 = !DILocation(line: 302, column: 25, scope: !595)
!595 = distinct !DILexicalBlock(scope: !591, file: !32, line: 299, column: 13)
!596 = !DILocation(line: 303, column: 30, scope: !595)
!597 = !DILocation(line: 303, column: 28, scope: !595)
!598 = !DILocation(line: 303, column: 43, scope: !595)
!599 = !DILocation(line: 303, column: 49, scope: !595)
!600 = !DILocation(line: 303, column: 53, scope: !595)
!601 = !DILocation(line: 303, column: 41, scope: !595)
!602 = !DILocation(line: 304, column: 36, scope: !595)
!603 = !DILocation(line: 304, column: 25, scope: !595)
!604 = !DILocation(line: 303, column: 64, scope: !595)
!605 = !DILocation(line: 302, column: 17, scope: !595)
!606 = !DILocation(line: 305, column: 13, scope: !595)
!607 = !DILocation(line: 306, column: 22, scope: !608)
!608 = distinct !DILexicalBlock(scope: !591, file: !32, line: 306, column: 22)
!609 = !DILocation(line: 306, column: 26, scope: !608)
!610 = !DILocation(line: 306, column: 22, scope: !591)
!611 = !DILocation(line: 308, column: 25, scope: !612)
!612 = distinct !DILexicalBlock(scope: !608, file: !32, line: 307, column: 13)
!613 = !DILocation(line: 308, column: 52, scope: !612)
!614 = !DILocation(line: 308, column: 50, scope: !612)
!615 = !DILocation(line: 308, column: 17, scope: !612)
!616 = !DILocation(line: 309, column: 13, scope: !612)
!617 = !DILocation(line: 312, column: 25, scope: !618)
!618 = distinct !DILexicalBlock(scope: !608, file: !32, line: 311, column: 13)
!619 = !DILocation(line: 312, column: 52, scope: !618)
!620 = !DILocation(line: 312, column: 50, scope: !618)
!621 = !DILocation(line: 312, column: 17, scope: !618)
!622 = !DILocation(line: 314, column: 9, scope: !592)
!623 = !DILocation(line: 317, column: 21, scope: !624)
!624 = distinct !DILexicalBlock(scope: !586, file: !32, line: 316, column: 9)
!625 = !DILocation(line: 317, column: 46, scope: !624)
!626 = !DILocation(line: 317, column: 52, scope: !624)
!627 = !DILocation(line: 317, column: 44, scope: !624)
!628 = !DILocation(line: 317, column: 13, scope: !624)
!629 = !DILocation(line: 319, column: 17, scope: !581)
!630 = !DILocation(line: 319, column: 32, scope: !581)
!631 = !DILocation(line: 319, column: 9, scope: !581)
!632 = !DILocation(line: 322, column: 20, scope: !581)
!633 = !DILocation(line: 322, column: 9, scope: !581)
!634 = !DILocation(line: 323, column: 13, scope: !635)
!635 = distinct !DILexicalBlock(scope: !581, file: !32, line: 323, column: 13)
!636 = !DILocation(line: 323, column: 19, scope: !635)
!637 = !DILocation(line: 323, column: 13, scope: !581)
!638 = !DILocation(line: 325, column: 24, scope: !639)
!639 = distinct !DILexicalBlock(scope: !635, file: !32, line: 324, column: 9)
!640 = !DILocation(line: 325, column: 30, scope: !639)
!641 = !DILocation(line: 325, column: 37, scope: !639)
!642 = !DILocation(line: 325, column: 41, scope: !639)
!643 = !DILocation(line: 325, column: 13, scope: !639)
!644 = !DILocation(line: 326, column: 9, scope: !639)
!645 = !DILocation(line: 327, column: 16, scope: !581)
!646 = !DILocation(line: 327, column: 22, scope: !581)
!647 = !DILocation(line: 327, column: 14, scope: !581)
!648 = distinct !{!648, !578, !649, !238}
!649 = !DILocation(line: 328, column: 5, scope: !571)
!650 = !DILocation(line: 329, column: 1, scope: !571)
!651 = distinct !DISubprogram(name: "call_srandom", scope: !652, file: !652, line: 69, type: !383, scopeLine: 70, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!652 = !DIFile(filename: "util/fortune-mod-common.h", directory: "/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod")
!653 = !DILocation(line: 71, column: 24, scope: !651)
!654 = !DILocation(line: 71, column: 47, scope: !651)
!655 = !DILocation(line: 71, column: 45, scope: !651)
!656 = !DILocation(line: 71, column: 13, scope: !651)
!657 = !DILocation(line: 71, column: 5, scope: !651)
!658 = !DILocation(line: 72, column: 1, scope: !651)
!659 = distinct !DISubprogram(name: "get_fort", scope: !32, file: !32, line: 1638, type: !383, scopeLine: 1639, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!660 = !DILocalVariable(name: "fp", scope: !659, file: !32, line: 1640, type: !47)
!661 = !DILocation(line: 1640, column: 15, scope: !659)
!662 = !DILocalVariable(name: "choice", scope: !659, file: !32, line: 1641, type: !13)
!663 = !DILocation(line: 1641, column: 9, scope: !659)
!664 = !DILocation(line: 1643, column: 10, scope: !665)
!665 = distinct !DILexicalBlock(scope: !659, file: !32, line: 1643, column: 9)
!666 = !DILocation(line: 1643, column: 21, scope: !665)
!667 = !DILocation(line: 1643, column: 26, scope: !665)
!668 = !DILocation(line: 1643, column: 29, scope: !665)
!669 = !DILocation(line: 1643, column: 40, scope: !665)
!670 = !DILocation(line: 1643, column: 48, scope: !665)
!671 = !DILocation(line: 1643, column: 9, scope: !659)
!672 = !DILocation(line: 1645, column: 14, scope: !673)
!673 = distinct !DILexicalBlock(scope: !665, file: !32, line: 1644, column: 5)
!674 = !DILocation(line: 1645, column: 12, scope: !673)
!675 = !DILocation(line: 1646, column: 5, scope: !673)
!676 = !DILocation(line: 1649, column: 23, scope: !677)
!677 = distinct !DILexicalBlock(scope: !665, file: !32, line: 1648, column: 5)
!678 = !DILocation(line: 1649, column: 16, scope: !677)
!679 = !DILocation(line: 1651, column: 19, scope: !680)
!680 = distinct !DILexicalBlock(scope: !677, file: !32, line: 1651, column: 9)
!681 = !DILocation(line: 1651, column: 17, scope: !680)
!682 = !DILocation(line: 1651, column: 14, scope: !680)
!683 = !DILocation(line: 1651, column: 30, scope: !684)
!684 = distinct !DILexicalBlock(scope: !680, file: !32, line: 1651, column: 9)
!685 = !DILocation(line: 1651, column: 34, scope: !684)
!686 = !DILocation(line: 1651, column: 42, scope: !684)
!687 = !DILocation(line: 1651, column: 9, scope: !680)
!688 = !DILocation(line: 1653, column: 17, scope: !689)
!689 = distinct !DILexicalBlock(scope: !690, file: !32, line: 1653, column: 17)
!690 = distinct !DILexicalBlock(scope: !684, file: !32, line: 1652, column: 9)
!691 = !DILocation(line: 1653, column: 26, scope: !689)
!692 = !DILocation(line: 1653, column: 30, scope: !689)
!693 = !DILocation(line: 1653, column: 24, scope: !689)
!694 = !DILocation(line: 1653, column: 17, scope: !690)
!695 = !DILocation(line: 1655, column: 17, scope: !696)
!696 = distinct !DILexicalBlock(scope: !689, file: !32, line: 1654, column: 13)
!697 = !DILocation(line: 1659, column: 27, scope: !698)
!698 = distinct !DILexicalBlock(scope: !689, file: !32, line: 1658, column: 13)
!699 = !DILocation(line: 1659, column: 31, scope: !698)
!700 = !DILocation(line: 1659, column: 24, scope: !698)
!701 = !DILocation(line: 1663, column: 9, scope: !690)
!702 = !DILocation(line: 1651, column: 59, scope: !684)
!703 = !DILocation(line: 1651, column: 63, scope: !684)
!704 = !DILocation(line: 1651, column: 57, scope: !684)
!705 = !DILocation(line: 1651, column: 9, scope: !684)
!706 = distinct !{!706, !687, !707, !238}
!707 = !DILocation(line: 1663, column: 9, scope: !680)
!708 = !DILocation(line: 1667, column: 9, scope: !709)
!709 = distinct !DILexicalBlock(scope: !659, file: !32, line: 1667, column: 9)
!710 = !DILocation(line: 1667, column: 13, scope: !709)
!711 = !DILocation(line: 1667, column: 21, scope: !709)
!712 = !DILocation(line: 1667, column: 9, scope: !659)
!713 = !DILocation(line: 1669, column: 17, scope: !714)
!714 = distinct !DILexicalBlock(scope: !709, file: !32, line: 1668, column: 5)
!715 = !DILocation(line: 1669, column: 9, scope: !714)
!716 = !DILocation(line: 1670, column: 5, scope: !714)
!717 = !DILocation(line: 1673, column: 13, scope: !718)
!718 = distinct !DILexicalBlock(scope: !719, file: !32, line: 1673, column: 13)
!719 = distinct !DILexicalBlock(scope: !709, file: !32, line: 1672, column: 5)
!720 = !DILocation(line: 1673, column: 17, scope: !718)
!721 = !DILocation(line: 1673, column: 13, scope: !719)
!722 = !DILocation(line: 1675, column: 25, scope: !723)
!723 = distinct !DILexicalBlock(scope: !718, file: !32, line: 1674, column: 9)
!724 = !DILocation(line: 1675, column: 13, scope: !723)
!725 = !DILocation(line: 1676, column: 49, scope: !723)
!726 = !DILocation(line: 1676, column: 28, scope: !723)
!727 = !DILocation(line: 1676, column: 20, scope: !723)
!728 = !DILocation(line: 1679, column: 13, scope: !723)
!729 = !DILocation(line: 1679, column: 20, scope: !723)
!730 = !DILocation(line: 1679, column: 35, scope: !723)
!731 = !DILocation(line: 1679, column: 39, scope: !723)
!732 = !DILocation(line: 1679, column: 43, scope: !723)
!733 = !DILocation(line: 1679, column: 27, scope: !723)
!734 = !DILocation(line: 1681, column: 32, scope: !735)
!735 = distinct !DILexicalBlock(scope: !723, file: !32, line: 1680, column: 13)
!736 = !DILocation(line: 1681, column: 36, scope: !735)
!737 = !DILocation(line: 1681, column: 40, scope: !735)
!738 = !DILocation(line: 1681, column: 24, scope: !735)
!739 = !DILocation(line: 1682, column: 22, scope: !735)
!740 = !DILocation(line: 1682, column: 26, scope: !735)
!741 = !DILocation(line: 1682, column: 20, scope: !735)
!742 = distinct !{!742, !728, !743, !238}
!743 = !DILocation(line: 1685, column: 13, scope: !723)
!744 = !DILocation(line: 1688, column: 9, scope: !723)
!745 = !DILocation(line: 1689, column: 17, scope: !719)
!746 = !DILocation(line: 1689, column: 9, scope: !719)
!747 = !DILocation(line: 1691, column: 9, scope: !748)
!748 = distinct !DILexicalBlock(scope: !659, file: !32, line: 1691, column: 9)
!749 = !DILocation(line: 1691, column: 13, scope: !748)
!750 = !DILocation(line: 1691, column: 17, scope: !748)
!751 = !DILocation(line: 1691, column: 28, scope: !748)
!752 = !DILocation(line: 1691, column: 9, scope: !659)
!753 = !DILocation(line: 1693, column: 17, scope: !754)
!754 = distinct !DILexicalBlock(scope: !748, file: !32, line: 1692, column: 5)
!755 = !DILocation(line: 1693, column: 9, scope: !754)
!756 = !DILocation(line: 1694, column: 9, scope: !754)
!757 = !DILocation(line: 1696, column: 9, scope: !758)
!758 = distinct !DILexicalBlock(scope: !659, file: !32, line: 1696, column: 9)
!759 = !DILocation(line: 1696, column: 13, scope: !758)
!760 = !DILocation(line: 1696, column: 9, scope: !659)
!761 = !DILocation(line: 1699, column: 25, scope: !762)
!762 = distinct !DILexicalBlock(scope: !758, file: !32, line: 1697, column: 5)
!763 = !DILocation(line: 1699, column: 14, scope: !762)
!764 = !DILocation(line: 1699, column: 12, scope: !762)
!765 = !DILocation(line: 1700, column: 9, scope: !762)
!766 = !DILocation(line: 1701, column: 5, scope: !762)
!767 = !DILocation(line: 1702, column: 5, scope: !659)
!768 = !DILocation(line: 1703, column: 16, scope: !659)
!769 = !DILocation(line: 1703, column: 14, scope: !659)
!770 = !DILocation(line: 1704, column: 13, scope: !659)
!771 = !DILocation(line: 1704, column: 5, scope: !659)
!772 = !DILocation(line: 1705, column: 14, scope: !659)
!773 = !DILocation(line: 1705, column: 5, scope: !659)
!774 = !DILocalVariable(name: "__f", scope: !775, file: !32, line: 1706, type: !23)
!775 = distinct !DILexicalBlock(scope: !659, file: !32, line: 1706, column: 5)
!776 = !DILocation(line: 1706, column: 5, scope: !775)
!777 = !DILocalVariable(name: "__o", scope: !775, file: !32, line: 1706, type: !23)
!778 = !DILocalVariable(name: "__w", scope: !775, file: !32, line: 1706, type: !23)
!779 = !DILocation(line: 1708, column: 15, scope: !780)
!780 = distinct !DILexicalBlock(scope: !659, file: !32, line: 1708, column: 9)
!781 = !DILocation(line: 1708, column: 19, scope: !780)
!782 = !DILocation(line: 1708, column: 10, scope: !780)
!783 = !DILocation(line: 1708, column: 58, scope: !780)
!784 = !DILocation(line: 1708, column: 63, scope: !780)
!785 = !DILocation(line: 1709, column: 15, scope: !780)
!786 = !DILocation(line: 1709, column: 19, scope: !780)
!787 = !DILocation(line: 1709, column: 10, scope: !780)
!788 = !DILocation(line: 1709, column: 58, scope: !780)
!789 = !DILocation(line: 1708, column: 9, scope: !659)
!790 = !DILocation(line: 1711, column: 9, scope: !791)
!791 = distinct !DILexicalBlock(scope: !780, file: !32, line: 1710, column: 5)
!792 = !DILocation(line: 1713, column: 43, scope: !659)
!793 = !DILocation(line: 1713, column: 27, scope: !659)
!794 = !DILocation(line: 1713, column: 16, scope: !659)
!795 = !DILocation(line: 1714, column: 43, scope: !659)
!796 = !DILocation(line: 1714, column: 27, scope: !659)
!797 = !DILocation(line: 1714, column: 16, scope: !659)
!798 = !DILocation(line: 1715, column: 1, scope: !659)
!799 = distinct !DISubprogram(name: "fortlen", scope: !32, file: !32, line: 1912, type: !800, scopeLine: 1913, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!800 = !DISubroutineType(types: !801)
!801 = !{!13}
!802 = !DILocalVariable(name: "nchar", scope: !799, file: !32, line: 1914, type: !13)
!803 = !DILocation(line: 1914, column: 9, scope: !799)
!804 = !DILocalVariable(name: "line", scope: !799, file: !32, line: 1915, type: !98)
!805 = !DILocation(line: 1915, column: 10, scope: !799)
!806 = !DILocation(line: 1917, column: 11, scope: !807)
!807 = distinct !DILexicalBlock(scope: !799, file: !32, line: 1917, column: 9)
!808 = !DILocation(line: 1917, column: 21, scope: !807)
!809 = !DILocation(line: 1917, column: 25, scope: !807)
!810 = !DILocation(line: 1917, column: 35, scope: !807)
!811 = !DILocation(line: 1917, column: 9, scope: !799)
!812 = !DILocation(line: 1919, column: 18, scope: !813)
!813 = distinct !DILexicalBlock(scope: !807, file: !32, line: 1918, column: 5)
!814 = !DILocation(line: 1919, column: 31, scope: !813)
!815 = !DILocation(line: 1919, column: 29, scope: !813)
!816 = !DILocation(line: 1919, column: 43, scope: !813)
!817 = !DILocation(line: 1919, column: 15, scope: !813)
!818 = !DILocation(line: 1920, column: 5, scope: !813)
!819 = !DILocation(line: 1923, column: 17, scope: !820)
!820 = distinct !DILexicalBlock(scope: !807, file: !32, line: 1922, column: 5)
!821 = !DILocation(line: 1923, column: 9, scope: !820)
!822 = !DILocation(line: 1924, column: 15, scope: !820)
!823 = !DILocation(line: 1924, column: 25, scope: !820)
!824 = !DILocation(line: 1924, column: 36, scope: !820)
!825 = !DILocation(line: 1924, column: 9, scope: !820)
!826 = !DILocation(line: 1925, column: 15, scope: !820)
!827 = !DILocation(line: 1926, column: 9, scope: !820)
!828 = !DILocation(line: 1926, column: 22, scope: !820)
!829 = !DILocation(line: 1926, column: 41, scope: !820)
!830 = !DILocation(line: 1926, column: 51, scope: !820)
!831 = !DILocation(line: 1926, column: 16, scope: !820)
!832 = !DILocation(line: 1926, column: 56, scope: !820)
!833 = !DILocation(line: 1927, column: 17, scope: !820)
!834 = !DILocation(line: 0, scope: !820)
!835 = !DILocation(line: 1927, column: 16, scope: !820)
!836 = !DILocation(line: 1929, column: 29, scope: !837)
!837 = distinct !DILexicalBlock(scope: !820, file: !32, line: 1928, column: 9)
!838 = !DILocation(line: 1929, column: 22, scope: !837)
!839 = !DILocation(line: 1929, column: 19, scope: !837)
!840 = distinct !{!840, !827, !841, !238}
!841 = !DILocation(line: 1930, column: 9, scope: !820)
!842 = !DILocation(line: 1932, column: 16, scope: !799)
!843 = !DILocation(line: 1932, column: 14, scope: !799)
!844 = !DILocation(line: 1933, column: 12, scope: !799)
!845 = !DILocation(line: 1933, column: 5, scope: !799)
!846 = distinct !DISubprogram(name: "display", scope: !32, file: !32, line: 1867, type: !125, scopeLine: 1868, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!847 = !DILocalVariable(name: "fp", arg: 1, scope: !846, file: !32, line: 1867, type: !47)
!848 = !DILocation(line: 1867, column: 31, scope: !846)
!849 = !DILocalVariable(name: "p", scope: !846, file: !32, line: 1869, type: !15)
!850 = !DILocation(line: 1869, column: 11, scope: !846)
!851 = !DILocalVariable(name: "ch", scope: !846, file: !32, line: 1869, type: !10)
!852 = !DILocation(line: 1869, column: 14, scope: !846)
!853 = !DILocalVariable(name: "line", scope: !846, file: !32, line: 1870, type: !854)
!854 = !DICompositeType(tag: DW_TAG_array_type, baseType: !17, size: 8192, elements: !99)
!855 = !DILocation(line: 1870, column: 19, scope: !846)
!856 = !DILocation(line: 1872, column: 13, scope: !846)
!857 = !DILocation(line: 1872, column: 5, scope: !846)
!858 = !DILocation(line: 1873, column: 11, scope: !846)
!859 = !DILocation(line: 1873, column: 15, scope: !846)
!860 = !DILocation(line: 1873, column: 26, scope: !846)
!861 = !DILocation(line: 1873, column: 5, scope: !846)
!862 = !DILocation(line: 1874, column: 9, scope: !863)
!863 = distinct !DILexicalBlock(scope: !846, file: !32, line: 1874, column: 9)
!864 = !DILocation(line: 1874, column: 9, scope: !846)
!865 = !DILocation(line: 1876, column: 30, scope: !866)
!866 = distinct !DILexicalBlock(scope: !863, file: !32, line: 1875, column: 5)
!867 = !DILocation(line: 1876, column: 34, scope: !866)
!868 = !DILocation(line: 1876, column: 9, scope: !866)
!869 = !DILocation(line: 1877, column: 5, scope: !866)
!870 = !DILocation(line: 1878, column: 19, scope: !871)
!871 = distinct !DILexicalBlock(scope: !846, file: !32, line: 1878, column: 5)
!872 = !DILocation(line: 1878, column: 10, scope: !871)
!873 = !DILocation(line: 1878, column: 38, scope: !874)
!874 = distinct !DILexicalBlock(scope: !871, file: !32, line: 1878, column: 5)
!875 = !DILocation(line: 1878, column: 57, scope: !874)
!876 = !DILocation(line: 1878, column: 61, scope: !874)
!877 = !DILocation(line: 1878, column: 24, scope: !874)
!878 = !DILocation(line: 1878, column: 66, scope: !874)
!879 = !DILocation(line: 1879, column: 25, scope: !874)
!880 = !DILocation(line: 0, scope: !874)
!881 = !DILocation(line: 1879, column: 24, scope: !874)
!882 = !DILocation(line: 1878, column: 5, scope: !871)
!883 = !DILocation(line: 1882, column: 13, scope: !884)
!884 = distinct !DILexicalBlock(scope: !885, file: !32, line: 1882, column: 13)
!885 = distinct !DILexicalBlock(scope: !874, file: !32, line: 1881, column: 5)
!886 = !DILocation(line: 1882, column: 17, scope: !884)
!887 = !DILocation(line: 1882, column: 21, scope: !884)
!888 = !DILocation(line: 1882, column: 31, scope: !884)
!889 = !DILocation(line: 1882, column: 13, scope: !885)
!890 = !DILocation(line: 1884, column: 30, scope: !891)
!891 = distinct !DILexicalBlock(scope: !892, file: !32, line: 1884, column: 13)
!892 = distinct !DILexicalBlock(scope: !884, file: !32, line: 1883, column: 9)
!893 = !DILocation(line: 1884, column: 20, scope: !891)
!894 = !DILocation(line: 1884, column: 18, scope: !891)
!895 = !DILocation(line: 1884, column: 43, scope: !896)
!896 = distinct !DILexicalBlock(scope: !891, file: !32, line: 1884, column: 13)
!897 = !DILocation(line: 1884, column: 42, scope: !896)
!898 = !DILocation(line: 1884, column: 40, scope: !896)
!899 = !DILocation(line: 1884, column: 13, scope: !891)
!900 = !DILocation(line: 1886, column: 33, scope: !901)
!901 = distinct !DILexicalBlock(scope: !902, file: !32, line: 1886, column: 21)
!902 = distinct !DILexicalBlock(scope: !896, file: !32, line: 1885, column: 13)
!903 = !DILocation(line: 1886, column: 21, scope: !901)
!904 = !DILocation(line: 1886, column: 21, scope: !902)
!905 = !DILocation(line: 1886, column: 36, scope: !901)
!906 = !DILocation(line: 1888, column: 33, scope: !907)
!907 = distinct !DILexicalBlock(scope: !901, file: !32, line: 1887, column: 17)
!908 = !DILocation(line: 1888, column: 36, scope: !907)
!909 = !DILocation(line: 1888, column: 42, scope: !907)
!910 = !DILocation(line: 1888, column: 48, scope: !907)
!911 = !DILocation(line: 1888, column: 30, scope: !907)
!912 = !DILocation(line: 1888, column: 26, scope: !907)
!913 = !DILocation(line: 1888, column: 22, scope: !907)
!914 = !DILocation(line: 1888, column: 24, scope: !907)
!915 = !DILocation(line: 1889, column: 17, scope: !907)
!916 = !DILocation(line: 1890, column: 38, scope: !917)
!917 = distinct !DILexicalBlock(scope: !901, file: !32, line: 1890, column: 26)
!918 = !DILocation(line: 1890, column: 26, scope: !917)
!919 = !DILocation(line: 1890, column: 26, scope: !901)
!920 = !DILocation(line: 1890, column: 41, scope: !917)
!921 = !DILocation(line: 1892, column: 33, scope: !922)
!922 = distinct !DILexicalBlock(scope: !917, file: !32, line: 1891, column: 17)
!923 = !DILocation(line: 1892, column: 36, scope: !922)
!924 = !DILocation(line: 1892, column: 42, scope: !922)
!925 = !DILocation(line: 1892, column: 48, scope: !922)
!926 = !DILocation(line: 1892, column: 30, scope: !922)
!927 = !DILocation(line: 1892, column: 26, scope: !922)
!928 = !DILocation(line: 1892, column: 22, scope: !922)
!929 = !DILocation(line: 1892, column: 24, scope: !922)
!930 = !DILocation(line: 1893, column: 17, scope: !922)
!931 = !DILocation(line: 1894, column: 13, scope: !902)
!932 = !DILocation(line: 1884, column: 47, scope: !896)
!933 = !DILocation(line: 1884, column: 13, scope: !896)
!934 = distinct !{!934, !899, !935, !238}
!935 = !DILocation(line: 1894, column: 13, scope: !891)
!936 = !DILocation(line: 1895, column: 9, scope: !892)
!937 = !DILocation(line: 1896, column: 13, scope: !938)
!938 = distinct !DILexicalBlock(scope: !885, file: !32, line: 1896, column: 13)
!939 = !DILocation(line: 1896, column: 17, scope: !938)
!940 = !DILocation(line: 1896, column: 30, scope: !938)
!941 = !DILocation(line: 1896, column: 35, scope: !938)
!942 = !DILocation(line: 1896, column: 13, scope: !885)
!943 = !DILocalVariable(name: "output", scope: !944, file: !32, line: 1898, type: !15)
!944 = distinct !DILexicalBlock(scope: !938, file: !32, line: 1897, column: 9)
!945 = !DILocation(line: 1898, column: 19, scope: !944)
!946 = !DILocation(line: 1898, column: 59, scope: !944)
!947 = !DILocation(line: 1898, column: 28, scope: !944)
!948 = !DILocation(line: 1899, column: 19, scope: !944)
!949 = !DILocation(line: 1899, column: 27, scope: !944)
!950 = !DILocation(line: 1899, column: 13, scope: !944)
!951 = !DILocation(line: 1900, column: 18, scope: !944)
!952 = !DILocation(line: 1900, column: 13, scope: !944)
!953 = !DILocation(line: 1901, column: 9, scope: !944)
!954 = !DILocation(line: 1903, column: 27, scope: !938)
!955 = !DILocation(line: 1903, column: 33, scope: !938)
!956 = !DILocation(line: 1903, column: 13, scope: !938)
!957 = !DILocation(line: 1904, column: 5, scope: !885)
!958 = !DILocation(line: 1880, column: 17, scope: !874)
!959 = !DILocation(line: 1878, column: 5, scope: !874)
!960 = distinct !{!960, !882, !961, !238}
!961 = !DILocation(line: 1904, column: 5, scope: !871)
!962 = !DILocation(line: 1905, column: 12, scope: !846)
!963 = !DILocation(line: 1905, column: 5, scope: !846)
!964 = !DILocation(line: 1906, column: 1, scope: !846)
!965 = distinct !DISubprogram(name: "mymax", scope: !32, file: !32, line: 1936, type: !966, scopeLine: 1936, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!966 = !DISubroutineType(types: !967)
!967 = !{!13, !13, !13}
!968 = !DILocalVariable(name: "i", arg: 1, scope: !965, file: !32, line: 1936, type: !13)
!969 = !DILocation(line: 1936, column: 22, scope: !965)
!970 = !DILocalVariable(name: "j", arg: 2, scope: !965, file: !32, line: 1936, type: !13)
!971 = !DILocation(line: 1936, column: 29, scope: !965)
!972 = !DILocation(line: 1936, column: 42, scope: !965)
!973 = !DILocation(line: 1936, column: 47, scope: !965)
!974 = !DILocation(line: 1936, column: 44, scope: !965)
!975 = !DILocation(line: 1936, column: 51, scope: !965)
!976 = !DILocation(line: 1936, column: 55, scope: !965)
!977 = !DILocation(line: 1936, column: 34, scope: !965)
!978 = distinct !DISubprogram(name: "free_desc", scope: !32, file: !32, line: 1938, type: !125, scopeLine: 1939, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!979 = !DILocalVariable(name: "ptr", arg: 1, scope: !978, file: !32, line: 1938, type: !47)
!980 = !DILocation(line: 1938, column: 33, scope: !978)
!981 = !DILocation(line: 1940, column: 5, scope: !978)
!982 = !DILocation(line: 1940, column: 12, scope: !978)
!983 = !DILocation(line: 1942, column: 19, scope: !984)
!984 = distinct !DILexicalBlock(scope: !978, file: !32, line: 1941, column: 5)
!985 = !DILocation(line: 1942, column: 24, scope: !984)
!986 = !DILocation(line: 1942, column: 9, scope: !984)
!987 = !DILocation(line: 1943, column: 14, scope: !984)
!988 = !DILocation(line: 1943, column: 19, scope: !984)
!989 = !DILocation(line: 1943, column: 9, scope: !984)
!990 = !DILocation(line: 1944, column: 14, scope: !984)
!991 = !DILocation(line: 1944, column: 19, scope: !984)
!992 = !DILocation(line: 1944, column: 9, scope: !984)
!993 = !DILocation(line: 1945, column: 14, scope: !984)
!994 = !DILocation(line: 1945, column: 19, scope: !984)
!995 = !DILocation(line: 1945, column: 9, scope: !984)
!996 = !DILocation(line: 1946, column: 14, scope: !984)
!997 = !DILocation(line: 1946, column: 19, scope: !984)
!998 = !DILocation(line: 1946, column: 9, scope: !984)
!999 = !DILocation(line: 1947, column: 13, scope: !1000)
!1000 = distinct !DILexicalBlock(scope: !984, file: !32, line: 1947, column: 13)
!1001 = !DILocation(line: 1947, column: 18, scope: !1000)
!1002 = !DILocation(line: 1947, column: 13, scope: !984)
!1003 = !DILocation(line: 1949, column: 20, scope: !1004)
!1004 = distinct !DILexicalBlock(scope: !1000, file: !32, line: 1948, column: 9)
!1005 = !DILocation(line: 1949, column: 25, scope: !1004)
!1006 = !DILocation(line: 1949, column: 13, scope: !1004)
!1007 = !DILocation(line: 1950, column: 13, scope: !1004)
!1008 = !DILocation(line: 1950, column: 18, scope: !1004)
!1009 = !DILocation(line: 1950, column: 22, scope: !1004)
!1010 = !DILocation(line: 1951, column: 9, scope: !1004)
!1011 = !DILocalVariable(name: "next", scope: !984, file: !32, line: 1952, type: !1012)
!1012 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !47)
!1013 = !DILocation(line: 1952, column: 25, scope: !984)
!1014 = !DILocation(line: 1952, column: 32, scope: !984)
!1015 = !DILocation(line: 1952, column: 37, scope: !984)
!1016 = !DILocation(line: 1953, column: 14, scope: !984)
!1017 = !DILocation(line: 1953, column: 9, scope: !984)
!1018 = !DILocation(line: 1954, column: 15, scope: !984)
!1019 = !DILocation(line: 1954, column: 13, scope: !984)
!1020 = distinct !{!1020, !981, !1021, !238}
!1021 = !DILocation(line: 1955, column: 5, scope: !978)
!1022 = !DILocation(line: 1956, column: 1, scope: !978)
!1023 = distinct !DISubprogram(name: "open_fp", scope: !32, file: !32, line: 1721, type: !125, scopeLine: 1722, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1024 = !DILocalVariable(name: "fp", arg: 1, scope: !1023, file: !32, line: 1721, type: !47)
!1025 = !DILocation(line: 1721, column: 31, scope: !1023)
!1026 = !DILocation(line: 1723, column: 10, scope: !1027)
!1027 = distinct !DILexicalBlock(scope: !1023, file: !32, line: 1723, column: 9)
!1028 = !DILocation(line: 1723, column: 14, scope: !1027)
!1029 = !DILocation(line: 1723, column: 18, scope: !1027)
!1030 = !DILocation(line: 1723, column: 40, scope: !1027)
!1031 = !DILocation(line: 1723, column: 44, scope: !1027)
!1032 = !DILocation(line: 1723, column: 33, scope: !1027)
!1033 = !DILocation(line: 1723, column: 23, scope: !1027)
!1034 = !DILocation(line: 1723, column: 27, scope: !1027)
!1035 = !DILocation(line: 1723, column: 31, scope: !1027)
!1036 = !DILocation(line: 1723, column: 9, scope: !1023)
!1037 = !DILocation(line: 1725, column: 16, scope: !1038)
!1038 = distinct !DILexicalBlock(scope: !1027, file: !32, line: 1724, column: 5)
!1039 = !DILocation(line: 1725, column: 20, scope: !1038)
!1040 = !DILocation(line: 1725, column: 9, scope: !1038)
!1041 = !DILocation(line: 1726, column: 9, scope: !1038)
!1042 = !DILocation(line: 1728, column: 1, scope: !1023)
!1043 = distinct !DISubprogram(name: "my_recode_string", scope: !32, file: !32, line: 205, type: !1044, scopeLine: 205, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1044 = !DISubroutineType(types: !1045)
!1045 = !{!15, !8}
!1046 = !DILocalVariable(name: "s", arg: 1, scope: !1043, file: !32, line: 205, type: !8)
!1047 = !DILocation(line: 205, column: 50, scope: !1043)
!1048 = !DILocation(line: 205, column: 69, scope: !1043)
!1049 = !DILocation(line: 205, column: 62, scope: !1043)
!1050 = !DILocation(line: 205, column: 55, scope: !1043)
!1051 = distinct !DISubprogram(name: "my_random", scope: !32, file: !32, line: 210, type: !1052, scopeLine: 211, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1052 = !DISubroutineType(types: !1053)
!1053 = !{!27, !1054}
!1054 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !27)
!1055 = !DILocalVariable(name: "base", arg: 1, scope: !1051, file: !32, line: 210, type: !1054)
!1056 = !DILocation(line: 210, column: 52, scope: !1051)
!1057 = !DILocalVariable(name: "l", scope: !1051, file: !32, line: 212, type: !1058)
!1058 = !DIBasicType(name: "unsigned long long", size: 64, encoding: DW_ATE_unsigned)
!1059 = !DILocation(line: 212, column: 24, scope: !1051)
!1060 = !DILocalVariable(name: "hard_coded_val", scope: !1051, file: !32, line: 213, type: !15)
!1061 = !DILocation(line: 213, column: 11, scope: !1051)
!1062 = !DILocation(line: 213, column: 28, scope: !1051)
!1063 = !DILocation(line: 214, column: 9, scope: !1064)
!1064 = distinct !DILexicalBlock(scope: !1051, file: !32, line: 214, column: 9)
!1065 = !DILocation(line: 214, column: 9, scope: !1051)
!1066 = !DILocation(line: 216, column: 37, scope: !1067)
!1067 = distinct !DILexicalBlock(scope: !1064, file: !32, line: 215, column: 5)
!1068 = !DILocation(line: 216, column: 32, scope: !1067)
!1069 = !DILocation(line: 216, column: 55, scope: !1067)
!1070 = !DILocation(line: 216, column: 53, scope: !1067)
!1071 = !DILocation(line: 216, column: 9, scope: !1067)
!1072 = !DILocation(line: 218, column: 9, scope: !1073)
!1073 = distinct !DILexicalBlock(scope: !1051, file: !32, line: 218, column: 9)
!1074 = !DILocation(line: 218, column: 9, scope: !1051)
!1075 = !DILocation(line: 220, column: 9, scope: !1076)
!1076 = distinct !DILexicalBlock(scope: !1073, file: !32, line: 219, column: 5)
!1077 = !DILocalVariable(name: "fp", scope: !1051, file: !32, line: 222, type: !1078)
!1078 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !56)
!1079 = !DILocation(line: 222, column: 17, scope: !1051)
!1080 = !DILocation(line: 222, column: 22, scope: !1051)
!1081 = !DILocation(line: 223, column: 10, scope: !1082)
!1082 = distinct !DILexicalBlock(scope: !1051, file: !32, line: 223, column: 9)
!1083 = !DILocation(line: 223, column: 9, scope: !1051)
!1084 = !DILocation(line: 225, column: 9, scope: !1085)
!1085 = distinct !DILexicalBlock(scope: !1082, file: !32, line: 224, column: 5)
!1086 = !DILocation(line: 227, column: 15, scope: !1087)
!1087 = distinct !DILexicalBlock(scope: !1051, file: !32, line: 227, column: 9)
!1088 = !DILocation(line: 227, column: 33, scope: !1087)
!1089 = !DILocation(line: 227, column: 9, scope: !1087)
!1090 = !DILocation(line: 227, column: 37, scope: !1087)
!1091 = !DILocation(line: 227, column: 9, scope: !1051)
!1092 = !DILocation(line: 229, column: 16, scope: !1093)
!1093 = distinct !DILexicalBlock(scope: !1087, file: !32, line: 228, column: 5)
!1094 = !DILocation(line: 229, column: 9, scope: !1093)
!1095 = !DILocation(line: 230, column: 9, scope: !1093)
!1096 = !DILocation(line: 232, column: 12, scope: !1051)
!1097 = !DILocation(line: 232, column: 5, scope: !1051)
!1098 = !DILocation(line: 233, column: 12, scope: !1051)
!1099 = !DILocation(line: 233, column: 16, scope: !1051)
!1100 = !DILocation(line: 233, column: 14, scope: !1051)
!1101 = !DILocation(line: 233, column: 5, scope: !1051)
!1102 = !DILabel(scope: !1051, name: "fallback", file: !32, line: 234)
!1103 = !DILocation(line: 234, column: 1, scope: !1051)
!1104 = !DILocation(line: 235, column: 29, scope: !1051)
!1105 = !DILocation(line: 235, column: 41, scope: !1051)
!1106 = !DILocation(line: 235, column: 39, scope: !1051)
!1107 = !DILocation(line: 235, column: 5, scope: !1051)
!1108 = !DILocation(line: 236, column: 1, scope: !1051)
!1109 = distinct !DISubprogram(name: "get_tbl", scope: !32, file: !32, line: 1465, type: !125, scopeLine: 1466, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1110 = !DILocalVariable(name: "fp", arg: 1, scope: !1109, file: !32, line: 1465, type: !47)
!1111 = !DILocation(line: 1465, column: 31, scope: !1109)
!1112 = !DILocalVariable(name: "fd", scope: !1109, file: !32, line: 1467, type: !13)
!1113 = !DILocation(line: 1467, column: 9, scope: !1109)
!1114 = !DILocalVariable(name: "child", scope: !1109, file: !32, line: 1468, type: !47)
!1115 = !DILocation(line: 1468, column: 15, scope: !1109)
!1116 = !DILocation(line: 1470, column: 9, scope: !1117)
!1117 = distinct !DILexicalBlock(scope: !1109, file: !32, line: 1470, column: 9)
!1118 = !DILocation(line: 1470, column: 13, scope: !1117)
!1119 = !DILocation(line: 1470, column: 9, scope: !1109)
!1120 = !DILocation(line: 1472, column: 9, scope: !1121)
!1121 = distinct !DILexicalBlock(scope: !1117, file: !32, line: 1471, column: 5)
!1122 = !DILocation(line: 1474, column: 11, scope: !1123)
!1123 = distinct !DILexicalBlock(scope: !1109, file: !32, line: 1474, column: 9)
!1124 = !DILocation(line: 1474, column: 15, scope: !1123)
!1125 = !DILocation(line: 1474, column: 10, scope: !1123)
!1126 = !DILocation(line: 1474, column: 9, scope: !1109)
!1127 = !DILocation(line: 1476, column: 29, scope: !1128)
!1128 = distinct !DILexicalBlock(scope: !1129, file: !32, line: 1476, column: 13)
!1129 = distinct !DILexicalBlock(scope: !1123, file: !32, line: 1475, column: 5)
!1130 = !DILocation(line: 1476, column: 33, scope: !1128)
!1131 = !DILocation(line: 1476, column: 19, scope: !1128)
!1132 = !DILocation(line: 1476, column: 17, scope: !1128)
!1133 = !DILocation(line: 1476, column: 43, scope: !1128)
!1134 = !DILocation(line: 1476, column: 13, scope: !1129)
!1135 = !DILocation(line: 1478, column: 20, scope: !1136)
!1136 = distinct !DILexicalBlock(scope: !1128, file: !32, line: 1477, column: 9)
!1137 = !DILocation(line: 1478, column: 24, scope: !1136)
!1138 = !DILocation(line: 1478, column: 13, scope: !1136)
!1139 = !DILocation(line: 1479, column: 13, scope: !1136)
!1140 = !DILocation(line: 1481, column: 18, scope: !1141)
!1141 = distinct !DILexicalBlock(scope: !1129, file: !32, line: 1481, column: 13)
!1142 = !DILocation(line: 1481, column: 23, scope: !1141)
!1143 = !DILocation(line: 1481, column: 27, scope: !1141)
!1144 = !DILocation(line: 1481, column: 31, scope: !1141)
!1145 = !DILocation(line: 1481, column: 22, scope: !1141)
!1146 = !DILocation(line: 1481, column: 13, scope: !1141)
!1147 = !DILocation(line: 1481, column: 72, scope: !1141)
!1148 = !DILocation(line: 1481, column: 13, scope: !1129)
!1149 = !DILocation(line: 1484, column: 21, scope: !1150)
!1150 = distinct !DILexicalBlock(scope: !1141, file: !32, line: 1483, column: 9)
!1151 = !DILocation(line: 1484, column: 56, scope: !1150)
!1152 = !DILocation(line: 1484, column: 60, scope: !1150)
!1153 = !DILocation(line: 1484, column: 13, scope: !1150)
!1154 = !DILocation(line: 1485, column: 13, scope: !1150)
!1155 = !DILocation(line: 1487, column: 18, scope: !1156)
!1156 = distinct !DILexicalBlock(scope: !1129, file: !32, line: 1487, column: 13)
!1157 = !DILocation(line: 1487, column: 23, scope: !1156)
!1158 = !DILocation(line: 1487, column: 27, scope: !1156)
!1159 = !DILocation(line: 1487, column: 31, scope: !1156)
!1160 = !DILocation(line: 1487, column: 22, scope: !1156)
!1161 = !DILocation(line: 1487, column: 13, scope: !1156)
!1162 = !DILocation(line: 1487, column: 70, scope: !1156)
!1163 = !DILocation(line: 1487, column: 13, scope: !1129)
!1164 = !DILocation(line: 1490, column: 21, scope: !1165)
!1165 = distinct !DILexicalBlock(scope: !1156, file: !32, line: 1489, column: 9)
!1166 = !DILocation(line: 1490, column: 56, scope: !1165)
!1167 = !DILocation(line: 1490, column: 60, scope: !1165)
!1168 = !DILocation(line: 1490, column: 13, scope: !1165)
!1169 = !DILocation(line: 1491, column: 13, scope: !1165)
!1170 = !DILocation(line: 1493, column: 18, scope: !1171)
!1171 = distinct !DILexicalBlock(scope: !1129, file: !32, line: 1493, column: 13)
!1172 = !DILocation(line: 1493, column: 23, scope: !1171)
!1173 = !DILocation(line: 1493, column: 27, scope: !1171)
!1174 = !DILocation(line: 1493, column: 31, scope: !1171)
!1175 = !DILocation(line: 1493, column: 22, scope: !1171)
!1176 = !DILocation(line: 1493, column: 13, scope: !1171)
!1177 = !DILocation(line: 1493, column: 72, scope: !1171)
!1178 = !DILocation(line: 1493, column: 13, scope: !1129)
!1179 = !DILocation(line: 1496, column: 21, scope: !1180)
!1180 = distinct !DILexicalBlock(scope: !1171, file: !32, line: 1495, column: 9)
!1181 = !DILocation(line: 1496, column: 56, scope: !1180)
!1182 = !DILocation(line: 1496, column: 60, scope: !1180)
!1183 = !DILocation(line: 1496, column: 13, scope: !1180)
!1184 = !DILocation(line: 1497, column: 13, scope: !1180)
!1185 = !DILocation(line: 1499, column: 18, scope: !1186)
!1186 = distinct !DILexicalBlock(scope: !1129, file: !32, line: 1499, column: 13)
!1187 = !DILocation(line: 1499, column: 23, scope: !1186)
!1188 = !DILocation(line: 1499, column: 27, scope: !1186)
!1189 = !DILocation(line: 1499, column: 31, scope: !1186)
!1190 = !DILocation(line: 1499, column: 22, scope: !1186)
!1191 = !DILocation(line: 1499, column: 13, scope: !1186)
!1192 = !DILocation(line: 1499, column: 74, scope: !1186)
!1193 = !DILocation(line: 1499, column: 13, scope: !1129)
!1194 = !DILocation(line: 1502, column: 21, scope: !1195)
!1195 = distinct !DILexicalBlock(scope: !1186, file: !32, line: 1501, column: 9)
!1196 = !DILocation(line: 1502, column: 56, scope: !1195)
!1197 = !DILocation(line: 1502, column: 60, scope: !1195)
!1198 = !DILocation(line: 1502, column: 13, scope: !1195)
!1199 = !DILocation(line: 1503, column: 13, scope: !1195)
!1200 = !DILocation(line: 1505, column: 18, scope: !1201)
!1201 = distinct !DILexicalBlock(scope: !1129, file: !32, line: 1505, column: 13)
!1202 = !DILocation(line: 1505, column: 23, scope: !1201)
!1203 = !DILocation(line: 1505, column: 27, scope: !1201)
!1204 = !DILocation(line: 1505, column: 31, scope: !1201)
!1205 = !DILocation(line: 1505, column: 22, scope: !1201)
!1206 = !DILocation(line: 1505, column: 13, scope: !1201)
!1207 = !DILocation(line: 1505, column: 68, scope: !1201)
!1208 = !DILocation(line: 1505, column: 13, scope: !1129)
!1209 = !DILocation(line: 1508, column: 21, scope: !1210)
!1210 = distinct !DILexicalBlock(scope: !1201, file: !32, line: 1507, column: 9)
!1211 = !DILocation(line: 1508, column: 56, scope: !1210)
!1212 = !DILocation(line: 1508, column: 60, scope: !1210)
!1213 = !DILocation(line: 1508, column: 13, scope: !1210)
!1214 = !DILocation(line: 1509, column: 13, scope: !1210)
!1215 = !DILocation(line: 1511, column: 18, scope: !1216)
!1216 = distinct !DILexicalBlock(scope: !1129, file: !32, line: 1511, column: 13)
!1217 = !DILocation(line: 1511, column: 23, scope: !1216)
!1218 = !DILocation(line: 1511, column: 27, scope: !1216)
!1219 = !DILocation(line: 1511, column: 31, scope: !1216)
!1220 = !DILocation(line: 1511, column: 22, scope: !1216)
!1221 = !DILocation(line: 1511, column: 13, scope: !1216)
!1222 = !DILocation(line: 1511, column: 60, scope: !1216)
!1223 = !DILocation(line: 1511, column: 13, scope: !1129)
!1224 = !DILocation(line: 1514, column: 21, scope: !1225)
!1225 = distinct !DILexicalBlock(scope: !1216, file: !32, line: 1513, column: 9)
!1226 = !DILocation(line: 1514, column: 56, scope: !1225)
!1227 = !DILocation(line: 1514, column: 60, scope: !1225)
!1228 = !DILocation(line: 1514, column: 13, scope: !1225)
!1229 = !DILocation(line: 1515, column: 13, scope: !1225)
!1230 = !DILocation(line: 1517, column: 37, scope: !1129)
!1231 = !DILocation(line: 1517, column: 41, scope: !1129)
!1232 = !DILocation(line: 1517, column: 45, scope: !1129)
!1233 = !DILocation(line: 1517, column: 31, scope: !1129)
!1234 = !DILocation(line: 1517, column: 9, scope: !1129)
!1235 = !DILocation(line: 1517, column: 13, scope: !1129)
!1236 = !DILocation(line: 1517, column: 17, scope: !1129)
!1237 = !DILocation(line: 1517, column: 29, scope: !1129)
!1238 = !DILocation(line: 1518, column: 36, scope: !1129)
!1239 = !DILocation(line: 1518, column: 40, scope: !1129)
!1240 = !DILocation(line: 1518, column: 44, scope: !1129)
!1241 = !DILocation(line: 1518, column: 30, scope: !1129)
!1242 = !DILocation(line: 1518, column: 9, scope: !1129)
!1243 = !DILocation(line: 1518, column: 13, scope: !1129)
!1244 = !DILocation(line: 1518, column: 17, scope: !1129)
!1245 = !DILocation(line: 1518, column: 28, scope: !1129)
!1246 = !DILocation(line: 1519, column: 37, scope: !1129)
!1247 = !DILocation(line: 1519, column: 41, scope: !1129)
!1248 = !DILocation(line: 1519, column: 45, scope: !1129)
!1249 = !DILocation(line: 1519, column: 31, scope: !1129)
!1250 = !DILocation(line: 1519, column: 9, scope: !1129)
!1251 = !DILocation(line: 1519, column: 13, scope: !1129)
!1252 = !DILocation(line: 1519, column: 17, scope: !1129)
!1253 = !DILocation(line: 1519, column: 29, scope: !1129)
!1254 = !DILocation(line: 1520, column: 38, scope: !1129)
!1255 = !DILocation(line: 1520, column: 42, scope: !1129)
!1256 = !DILocation(line: 1520, column: 46, scope: !1129)
!1257 = !DILocation(line: 1520, column: 32, scope: !1129)
!1258 = !DILocation(line: 1520, column: 9, scope: !1129)
!1259 = !DILocation(line: 1520, column: 13, scope: !1129)
!1260 = !DILocation(line: 1520, column: 17, scope: !1129)
!1261 = !DILocation(line: 1520, column: 30, scope: !1129)
!1262 = !DILocation(line: 1521, column: 35, scope: !1129)
!1263 = !DILocation(line: 1521, column: 39, scope: !1129)
!1264 = !DILocation(line: 1521, column: 43, scope: !1129)
!1265 = !DILocation(line: 1521, column: 29, scope: !1129)
!1266 = !DILocation(line: 1521, column: 9, scope: !1129)
!1267 = !DILocation(line: 1521, column: 13, scope: !1129)
!1268 = !DILocation(line: 1521, column: 17, scope: !1129)
!1269 = !DILocation(line: 1521, column: 27, scope: !1129)
!1270 = !DILocation(line: 1522, column: 15, scope: !1129)
!1271 = !DILocation(line: 1522, column: 9, scope: !1129)
!1272 = !DILocation(line: 1523, column: 5, scope: !1129)
!1273 = !DILocation(line: 1526, column: 19, scope: !1274)
!1274 = distinct !DILexicalBlock(scope: !1123, file: !32, line: 1525, column: 5)
!1275 = !DILocation(line: 1526, column: 23, scope: !1274)
!1276 = !DILocation(line: 1526, column: 9, scope: !1274)
!1277 = !DILocation(line: 1527, column: 22, scope: !1278)
!1278 = distinct !DILexicalBlock(scope: !1274, file: !32, line: 1527, column: 9)
!1279 = !DILocation(line: 1527, column: 26, scope: !1278)
!1280 = !DILocation(line: 1527, column: 20, scope: !1278)
!1281 = !DILocation(line: 1527, column: 14, scope: !1278)
!1282 = !DILocation(line: 1527, column: 33, scope: !1283)
!1283 = distinct !DILexicalBlock(scope: !1278, file: !32, line: 1527, column: 9)
!1284 = !DILocation(line: 1527, column: 9, scope: !1278)
!1285 = !DILocation(line: 1529, column: 21, scope: !1286)
!1286 = distinct !DILexicalBlock(scope: !1283, file: !32, line: 1528, column: 9)
!1287 = !DILocation(line: 1529, column: 13, scope: !1286)
!1288 = !DILocation(line: 1530, column: 22, scope: !1286)
!1289 = !DILocation(line: 1530, column: 26, scope: !1286)
!1290 = !DILocation(line: 1530, column: 32, scope: !1286)
!1291 = !DILocation(line: 1530, column: 39, scope: !1286)
!1292 = !DILocation(line: 1530, column: 13, scope: !1286)
!1293 = !DILocation(line: 1531, column: 9, scope: !1286)
!1294 = !DILocation(line: 1527, column: 48, scope: !1283)
!1295 = !DILocation(line: 1527, column: 55, scope: !1283)
!1296 = !DILocation(line: 1527, column: 46, scope: !1283)
!1297 = !DILocation(line: 1527, column: 9, scope: !1283)
!1298 = distinct !{!1298, !1284, !1299, !238}
!1299 = !DILocation(line: 1531, column: 9, scope: !1278)
!1300 = !DILocation(line: 1533, column: 5, scope: !1109)
!1301 = !DILocation(line: 1533, column: 9, scope: !1109)
!1302 = !DILocation(line: 1533, column: 18, scope: !1109)
!1303 = !DILocation(line: 1534, column: 1, scope: !1109)
!1304 = distinct !DISubprogram(name: "pick_child", scope: !32, file: !32, line: 1567, type: !1305, scopeLine: 1568, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1305 = !DISubroutineType(types: !1306)
!1306 = !{!47, !47}
!1307 = !DILocalVariable(name: "parent", arg: 1, scope: !1304, file: !32, line: 1567, type: !47)
!1308 = !DILocation(line: 1567, column: 39, scope: !1304)
!1309 = !DILocalVariable(name: "fp", scope: !1304, file: !32, line: 1569, type: !47)
!1310 = !DILocation(line: 1569, column: 15, scope: !1304)
!1311 = !DILocalVariable(name: "choice", scope: !1304, file: !32, line: 1570, type: !13)
!1312 = !DILocation(line: 1570, column: 9, scope: !1304)
!1313 = !DILocation(line: 1572, column: 9, scope: !1314)
!1314 = distinct !DILexicalBlock(scope: !1304, file: !32, line: 1572, column: 9)
!1315 = !DILocation(line: 1572, column: 9, scope: !1304)
!1316 = !DILocation(line: 1574, column: 48, scope: !1317)
!1317 = distinct !DILexicalBlock(scope: !1314, file: !32, line: 1573, column: 5)
!1318 = !DILocation(line: 1574, column: 56, scope: !1317)
!1319 = !DILocation(line: 1574, column: 23, scope: !1317)
!1320 = !DILocation(line: 1574, column: 16, scope: !1317)
!1321 = !DILocation(line: 1577, column: 19, scope: !1322)
!1322 = distinct !DILexicalBlock(scope: !1317, file: !32, line: 1577, column: 9)
!1323 = !DILocation(line: 1577, column: 27, scope: !1322)
!1324 = !DILocation(line: 1577, column: 17, scope: !1322)
!1325 = !DILocation(line: 1577, column: 14, scope: !1322)
!1326 = !DILocation(line: 1577, column: 40, scope: !1327)
!1327 = distinct !DILexicalBlock(scope: !1322, file: !32, line: 1577, column: 9)
!1328 = !DILocation(line: 1577, column: 9, scope: !1322)
!1329 = !DILocation(line: 1579, column: 13, scope: !1330)
!1330 = distinct !DILexicalBlock(scope: !1327, file: !32, line: 1578, column: 9)
!1331 = !DILocation(line: 1577, column: 49, scope: !1327)
!1332 = !DILocation(line: 1577, column: 53, scope: !1327)
!1333 = !DILocation(line: 1577, column: 47, scope: !1327)
!1334 = !DILocation(line: 1577, column: 9, scope: !1327)
!1335 = distinct !{!1335, !1328, !1336, !238}
!1336 = !DILocation(line: 1580, column: 9, scope: !1322)
!1337 = !DILocation(line: 1582, column: 5, scope: !1317)
!1338 = !DILocation(line: 1585, column: 17, scope: !1339)
!1339 = distinct !DILexicalBlock(scope: !1314, file: !32, line: 1584, column: 5)
!1340 = !DILocation(line: 1585, column: 9, scope: !1339)
!1341 = !DILocation(line: 1586, column: 34, scope: !1339)
!1342 = !DILocation(line: 1586, column: 42, scope: !1339)
!1343 = !DILocation(line: 1586, column: 46, scope: !1339)
!1344 = !DILocation(line: 1586, column: 24, scope: !1339)
!1345 = !DILocation(line: 1586, column: 16, scope: !1339)
!1346 = !DILocation(line: 1589, column: 19, scope: !1347)
!1347 = distinct !DILexicalBlock(scope: !1339, file: !32, line: 1589, column: 9)
!1348 = !DILocation(line: 1589, column: 27, scope: !1347)
!1349 = !DILocation(line: 1589, column: 17, scope: !1347)
!1350 = !DILocation(line: 1589, column: 14, scope: !1347)
!1351 = !DILocation(line: 1589, column: 34, scope: !1352)
!1352 = distinct !DILexicalBlock(scope: !1347, file: !32, line: 1589, column: 9)
!1353 = !DILocation(line: 1589, column: 49, scope: !1352)
!1354 = !DILocation(line: 1589, column: 53, scope: !1352)
!1355 = !DILocation(line: 1589, column: 57, scope: !1352)
!1356 = !DILocation(line: 1589, column: 41, scope: !1352)
!1357 = !DILocation(line: 1589, column: 9, scope: !1347)
!1358 = !DILocation(line: 1592, column: 23, scope: !1359)
!1359 = distinct !DILexicalBlock(scope: !1352, file: !32, line: 1591, column: 9)
!1360 = !DILocation(line: 1592, column: 27, scope: !1359)
!1361 = !DILocation(line: 1592, column: 31, scope: !1359)
!1362 = !DILocation(line: 1592, column: 20, scope: !1359)
!1363 = !DILocation(line: 1595, column: 9, scope: !1359)
!1364 = !DILocation(line: 1590, column: 18, scope: !1352)
!1365 = !DILocation(line: 1590, column: 22, scope: !1352)
!1366 = !DILocation(line: 1590, column: 16, scope: !1352)
!1367 = !DILocation(line: 1589, column: 9, scope: !1352)
!1368 = distinct !{!1368, !1357, !1369, !238}
!1369 = !DILocation(line: 1595, column: 9, scope: !1347)
!1370 = !DILocation(line: 1599, column: 13, scope: !1304)
!1371 = !DILocation(line: 1599, column: 17, scope: !1304)
!1372 = !DILocation(line: 1599, column: 37, scope: !1304)
!1373 = !DILocation(line: 1599, column: 26, scope: !1304)
!1374 = !DILocation(line: 1599, column: 44, scope: !1304)
!1375 = !DILocation(line: 1599, column: 5, scope: !1304)
!1376 = distinct !DISubprogram(name: "get_pos", scope: !32, file: !32, line: 1619, type: !125, scopeLine: 1620, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1377 = !DILocalVariable(name: "fp", arg: 1, scope: !1376, file: !32, line: 1619, type: !47)
!1378 = !DILocation(line: 1619, column: 31, scope: !1376)
!1379 = !DILocation(line: 1621, column: 5, scope: !1376)
!1380 = !DILocation(line: 1622, column: 9, scope: !1381)
!1381 = distinct !DILexicalBlock(scope: !1376, file: !32, line: 1622, column: 9)
!1382 = !DILocation(line: 1622, column: 13, scope: !1381)
!1383 = !DILocation(line: 1622, column: 17, scope: !1381)
!1384 = !DILocation(line: 1622, column: 9, scope: !1376)
!1385 = !DILocation(line: 1624, column: 39, scope: !1386)
!1386 = distinct !DILexicalBlock(scope: !1381, file: !32, line: 1623, column: 5)
!1387 = !DILocation(line: 1624, column: 43, scope: !1386)
!1388 = !DILocation(line: 1624, column: 47, scope: !1386)
!1389 = !DILocation(line: 1624, column: 29, scope: !1386)
!1390 = !DILocation(line: 1624, column: 9, scope: !1386)
!1391 = !DILocation(line: 1624, column: 13, scope: !1386)
!1392 = !DILocation(line: 1624, column: 17, scope: !1386)
!1393 = !DILocation(line: 1625, column: 5, scope: !1386)
!1394 = !DILocation(line: 1626, column: 12, scope: !1395)
!1395 = distinct !DILexicalBlock(scope: !1376, file: !32, line: 1626, column: 9)
!1396 = !DILocation(line: 1626, column: 16, scope: !1395)
!1397 = !DILocation(line: 1626, column: 9, scope: !1395)
!1398 = !DILocation(line: 1626, column: 33, scope: !1395)
!1399 = !DILocation(line: 1626, column: 37, scope: !1395)
!1400 = !DILocation(line: 1626, column: 41, scope: !1395)
!1401 = !DILocation(line: 1626, column: 21, scope: !1395)
!1402 = !DILocation(line: 1626, column: 9, scope: !1376)
!1403 = !DILocation(line: 1628, column: 20, scope: !1404)
!1404 = distinct !DILexicalBlock(scope: !1395, file: !32, line: 1627, column: 5)
!1405 = !DILocation(line: 1628, column: 24, scope: !1404)
!1406 = !DILocation(line: 1628, column: 28, scope: !1404)
!1407 = !DILocation(line: 1628, column: 9, scope: !1404)
!1408 = !DILocation(line: 1628, column: 13, scope: !1404)
!1409 = !DILocation(line: 1628, column: 17, scope: !1404)
!1410 = !DILocation(line: 1629, column: 5, scope: !1404)
!1411 = !DILocation(line: 1631, column: 1, scope: !1376)
!1412 = distinct !DISubprogram(name: "open_dat", scope: !32, file: !32, line: 1606, type: !125, scopeLine: 1607, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1413 = !DILocalVariable(name: "fp", arg: 1, scope: !1412, file: !32, line: 1606, type: !47)
!1414 = !DILocation(line: 1606, column: 32, scope: !1412)
!1415 = !DILocation(line: 1608, column: 9, scope: !1416)
!1416 = distinct !DILexicalBlock(scope: !1412, file: !32, line: 1608, column: 9)
!1417 = !DILocation(line: 1608, column: 13, scope: !1416)
!1418 = !DILocation(line: 1608, column: 19, scope: !1416)
!1419 = !DILocation(line: 1608, column: 23, scope: !1416)
!1420 = !DILocation(line: 1608, column: 49, scope: !1416)
!1421 = !DILocation(line: 1608, column: 53, scope: !1416)
!1422 = !DILocation(line: 1608, column: 39, scope: !1416)
!1423 = !DILocation(line: 1608, column: 27, scope: !1416)
!1424 = !DILocation(line: 1608, column: 31, scope: !1416)
!1425 = !DILocation(line: 1608, column: 37, scope: !1416)
!1426 = !DILocation(line: 1608, column: 63, scope: !1416)
!1427 = !DILocation(line: 1608, column: 9, scope: !1412)
!1428 = !DILocation(line: 1610, column: 9, scope: !1429)
!1429 = distinct !DILexicalBlock(scope: !1416, file: !32, line: 1609, column: 5)
!1430 = !DILocation(line: 1612, column: 1, scope: !1412)
!1431 = distinct !DISubprogram(name: "open4read", scope: !32, file: !32, line: 577, type: !1432, scopeLine: 578, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1432 = !DISubroutineType(types: !1433)
!1433 = !{!13, !7}
!1434 = !DILocalVariable(name: "path", arg: 1, scope: !1431, file: !32, line: 577, type: !7)
!1435 = !DILocation(line: 577, column: 40, scope: !1431)
!1436 = !DILocation(line: 579, column: 17, scope: !1431)
!1437 = !DILocation(line: 579, column: 12, scope: !1431)
!1438 = !DILocation(line: 579, column: 5, scope: !1431)
!1439 = distinct !DISubprogram(name: "zero_tbl", scope: !32, file: !32, line: 1437, type: !1440, scopeLine: 1438, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1440 = !DISubroutineType(types: !1441)
!1441 = !{null, !1442}
!1442 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !68, size: 32)
!1443 = !DILocalVariable(name: "tp", arg: 1, scope: !1439, file: !32, line: 1437, type: !1442)
!1444 = !DILocation(line: 1437, column: 31, scope: !1439)
!1445 = !DILocation(line: 1439, column: 5, scope: !1439)
!1446 = !DILocation(line: 1439, column: 9, scope: !1439)
!1447 = !DILocation(line: 1439, column: 20, scope: !1439)
!1448 = !DILocation(line: 1440, column: 5, scope: !1439)
!1449 = !DILocation(line: 1440, column: 9, scope: !1439)
!1450 = !DILocation(line: 1440, column: 21, scope: !1439)
!1451 = !DILocation(line: 1441, column: 5, scope: !1439)
!1452 = !DILocation(line: 1441, column: 9, scope: !1439)
!1453 = !DILocation(line: 1441, column: 22, scope: !1439)
!1454 = !DILocation(line: 1442, column: 1, scope: !1439)
!1455 = distinct !DISubprogram(name: "sum_tbl", scope: !32, file: !32, line: 1448, type: !1456, scopeLine: 1449, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1456 = !DISubroutineType(types: !1457)
!1457 = !{null, !1442, !1442}
!1458 = !DILocalVariable(name: "t1", arg: 1, scope: !1455, file: !32, line: 1448, type: !1442)
!1459 = !DILocation(line: 1448, column: 30, scope: !1455)
!1460 = !DILocalVariable(name: "t2", arg: 2, scope: !1455, file: !32, line: 1448, type: !1442)
!1461 = !DILocation(line: 1448, column: 43, scope: !1455)
!1462 = !DILocation(line: 1450, column: 23, scope: !1455)
!1463 = !DILocation(line: 1450, column: 27, scope: !1455)
!1464 = !DILocation(line: 1450, column: 5, scope: !1455)
!1465 = !DILocation(line: 1450, column: 9, scope: !1455)
!1466 = !DILocation(line: 1450, column: 20, scope: !1455)
!1467 = !DILocation(line: 1451, column: 9, scope: !1468)
!1468 = distinct !DILexicalBlock(scope: !1455, file: !32, line: 1451, column: 9)
!1469 = !DILocation(line: 1451, column: 13, scope: !1468)
!1470 = !DILocation(line: 1451, column: 27, scope: !1468)
!1471 = !DILocation(line: 1451, column: 31, scope: !1468)
!1472 = !DILocation(line: 1451, column: 25, scope: !1468)
!1473 = !DILocation(line: 1451, column: 9, scope: !1455)
!1474 = !DILocation(line: 1453, column: 27, scope: !1475)
!1475 = distinct !DILexicalBlock(scope: !1468, file: !32, line: 1452, column: 5)
!1476 = !DILocation(line: 1453, column: 31, scope: !1475)
!1477 = !DILocation(line: 1453, column: 9, scope: !1475)
!1478 = !DILocation(line: 1453, column: 13, scope: !1475)
!1479 = !DILocation(line: 1453, column: 25, scope: !1475)
!1480 = !DILocation(line: 1454, column: 5, scope: !1475)
!1481 = !DILocation(line: 1455, column: 9, scope: !1482)
!1482 = distinct !DILexicalBlock(scope: !1455, file: !32, line: 1455, column: 9)
!1483 = !DILocation(line: 1455, column: 13, scope: !1482)
!1484 = !DILocation(line: 1455, column: 28, scope: !1482)
!1485 = !DILocation(line: 1455, column: 32, scope: !1482)
!1486 = !DILocation(line: 1455, column: 26, scope: !1482)
!1487 = !DILocation(line: 1455, column: 9, scope: !1455)
!1488 = !DILocation(line: 1457, column: 28, scope: !1489)
!1489 = distinct !DILexicalBlock(scope: !1482, file: !32, line: 1456, column: 5)
!1490 = !DILocation(line: 1457, column: 32, scope: !1489)
!1491 = !DILocation(line: 1457, column: 9, scope: !1489)
!1492 = !DILocation(line: 1457, column: 13, scope: !1489)
!1493 = !DILocation(line: 1457, column: 26, scope: !1489)
!1494 = !DILocation(line: 1458, column: 5, scope: !1489)
!1495 = !DILocation(line: 1459, column: 1, scope: !1455)
!1496 = distinct !DISubprogram(name: "maxlen_in_list", scope: !32, file: !32, line: 1735, type: !1497, scopeLine: 1736, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1497 = !DISubroutineType(types: !1498)
!1498 = !{!13, !47}
!1499 = !DILocalVariable(name: "list", arg: 1, scope: !1496, file: !32, line: 1735, type: !47)
!1500 = !DILocation(line: 1735, column: 37, scope: !1496)
!1501 = !DILocalVariable(name: "fp", scope: !1496, file: !32, line: 1737, type: !47)
!1502 = !DILocation(line: 1737, column: 15, scope: !1496)
!1503 = !DILocalVariable(name: "len", scope: !1496, file: !32, line: 1738, type: !13)
!1504 = !DILocation(line: 1738, column: 9, scope: !1496)
!1505 = !DILocalVariable(name: "maxlen", scope: !1496, file: !32, line: 1738, type: !13)
!1506 = !DILocation(line: 1738, column: 14, scope: !1496)
!1507 = !DILocation(line: 1740, column: 15, scope: !1508)
!1508 = distinct !DILexicalBlock(scope: !1496, file: !32, line: 1740, column: 5)
!1509 = !DILocation(line: 1740, column: 13, scope: !1508)
!1510 = !DILocation(line: 1740, column: 10, scope: !1508)
!1511 = !DILocation(line: 1740, column: 21, scope: !1512)
!1512 = distinct !DILexicalBlock(scope: !1508, file: !32, line: 1740, column: 5)
!1513 = !DILocation(line: 1740, column: 5, scope: !1508)
!1514 = !DILocation(line: 1742, column: 13, scope: !1515)
!1515 = distinct !DILexicalBlock(scope: !1516, file: !32, line: 1742, column: 13)
!1516 = distinct !DILexicalBlock(scope: !1512, file: !32, line: 1741, column: 5)
!1517 = !DILocation(line: 1742, column: 17, scope: !1515)
!1518 = !DILocation(line: 1742, column: 13, scope: !1516)
!1519 = !DILocation(line: 1744, column: 39, scope: !1520)
!1520 = distinct !DILexicalBlock(scope: !1521, file: !32, line: 1744, column: 17)
!1521 = distinct !DILexicalBlock(scope: !1515, file: !32, line: 1743, column: 9)
!1522 = !DILocation(line: 1744, column: 43, scope: !1520)
!1523 = !DILocation(line: 1744, column: 24, scope: !1520)
!1524 = !DILocation(line: 1744, column: 22, scope: !1520)
!1525 = !DILocation(line: 1744, column: 53, scope: !1520)
!1526 = !DILocation(line: 1744, column: 51, scope: !1520)
!1527 = !DILocation(line: 1744, column: 17, scope: !1521)
!1528 = !DILocation(line: 1746, column: 26, scope: !1529)
!1529 = distinct !DILexicalBlock(scope: !1520, file: !32, line: 1745, column: 13)
!1530 = !DILocation(line: 1746, column: 24, scope: !1529)
!1531 = !DILocation(line: 1747, column: 13, scope: !1529)
!1532 = !DILocation(line: 1748, column: 9, scope: !1521)
!1533 = !DILocation(line: 1751, column: 21, scope: !1534)
!1534 = distinct !DILexicalBlock(scope: !1515, file: !32, line: 1750, column: 9)
!1535 = !DILocation(line: 1751, column: 13, scope: !1534)
!1536 = !DILocation(line: 1752, column: 22, scope: !1537)
!1537 = distinct !DILexicalBlock(scope: !1534, file: !32, line: 1752, column: 17)
!1538 = !DILocation(line: 1752, column: 26, scope: !1537)
!1539 = !DILocation(line: 1752, column: 30, scope: !1537)
!1540 = !DILocation(line: 1752, column: 44, scope: !1537)
!1541 = !DILocation(line: 1752, column: 42, scope: !1537)
!1542 = !DILocation(line: 1752, column: 17, scope: !1534)
!1543 = !DILocation(line: 1754, column: 31, scope: !1544)
!1544 = distinct !DILexicalBlock(scope: !1537, file: !32, line: 1753, column: 13)
!1545 = !DILocation(line: 1754, column: 35, scope: !1544)
!1546 = !DILocation(line: 1754, column: 39, scope: !1544)
!1547 = !DILocation(line: 1754, column: 24, scope: !1544)
!1548 = !DILocation(line: 1755, column: 13, scope: !1544)
!1549 = !DILocation(line: 1757, column: 5, scope: !1516)
!1550 = !DILocation(line: 1740, column: 30, scope: !1512)
!1551 = !DILocation(line: 1740, column: 34, scope: !1512)
!1552 = !DILocation(line: 1740, column: 28, scope: !1512)
!1553 = !DILocation(line: 1740, column: 5, scope: !1512)
!1554 = distinct !{!1554, !1513, !1555, !238}
!1555 = !DILocation(line: 1757, column: 5, scope: !1508)
!1556 = !DILocation(line: 1758, column: 12, scope: !1496)
!1557 = !DILocation(line: 1758, column: 5, scope: !1496)
!1558 = distinct !DISubprogram(name: "do_malloc", scope: !32, file: !32, line: 398, type: !1559, scopeLine: 399, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1559 = !DISubroutineType(types: !1560)
!1560 = !{!22, !1561}
!1561 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !25)
!1562 = !DILocalVariable(name: "size", arg: 1, scope: !1558, file: !32, line: 398, type: !1561)
!1563 = !DILocation(line: 398, column: 37, scope: !1558)
!1564 = !DILocalVariable(name: "new_buf", scope: !1558, file: !32, line: 400, type: !22)
!1565 = !DILocation(line: 400, column: 11, scope: !1558)
!1566 = !DILocation(line: 400, column: 28, scope: !1558)
!1567 = !DILocation(line: 400, column: 21, scope: !1558)
!1568 = !DILocation(line: 402, column: 10, scope: !1569)
!1569 = distinct !DILexicalBlock(scope: !1558, file: !32, line: 402, column: 9)
!1570 = !DILocation(line: 402, column: 9, scope: !1558)
!1571 = !DILocation(line: 404, column: 23, scope: !1572)
!1572 = distinct !DILexicalBlock(scope: !1569, file: !32, line: 403, column: 5)
!1573 = !DILocation(line: 404, column: 15, scope: !1572)
!1574 = !DILocation(line: 405, column: 9, scope: !1572)
!1575 = !DILocation(line: 407, column: 12, scope: !1558)
!1576 = !DILocation(line: 407, column: 5, scope: !1558)
!1577 = distinct !DISubprogram(name: "matches_in_list", scope: !32, file: !32, line: 1765, type: !1578, scopeLine: 1766, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1578 = !DISubroutineType(types: !1579)
!1579 = !{null, !47, !1580}
!1580 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !1581)
!1581 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !64, size: 32)
!1582 = !DILocalVariable(name: "list", arg: 1, scope: !1577, file: !32, line: 1765, type: !47)
!1583 = !DILocation(line: 1765, column: 39, scope: !1577)
!1584 = !DILocalVariable(name: "Found_one_ptr", arg: 2, scope: !1577, file: !32, line: 1765, type: !1580)
!1585 = !DILocation(line: 1765, column: 57, scope: !1577)
!1586 = !DILocalVariable(name: "sp", scope: !1577, file: !32, line: 1767, type: !16)
!1587 = !DILocation(line: 1767, column: 20, scope: !1577)
!1588 = !DILocalVariable(name: "p", scope: !1577, file: !32, line: 1768, type: !16)
!1589 = !DILocation(line: 1768, column: 20, scope: !1577)
!1590 = !DILocalVariable(name: "ch", scope: !1577, file: !32, line: 1769, type: !17)
!1591 = !DILocation(line: 1769, column: 19, scope: !1577)
!1592 = !DILocalVariable(name: "fp", scope: !1577, file: !32, line: 1770, type: !47)
!1593 = !DILocation(line: 1770, column: 15, scope: !1577)
!1594 = !DILocalVariable(name: "in_file", scope: !1577, file: !32, line: 1771, type: !13)
!1595 = !DILocation(line: 1771, column: 9, scope: !1577)
!1596 = !DILocalVariable(name: "nchar", scope: !1577, file: !32, line: 1771, type: !13)
!1597 = !DILocation(line: 1771, column: 18, scope: !1577)
!1598 = !DILocalVariable(name: "output", scope: !1577, file: !32, line: 1772, type: !15)
!1599 = !DILocation(line: 1772, column: 11, scope: !1577)
!1600 = !DILocation(line: 1774, column: 15, scope: !1601)
!1601 = distinct !DILexicalBlock(scope: !1577, file: !32, line: 1774, column: 5)
!1602 = !DILocation(line: 1774, column: 13, scope: !1601)
!1603 = !DILocation(line: 1774, column: 10, scope: !1601)
!1604 = !DILocation(line: 1774, column: 21, scope: !1605)
!1605 = distinct !DILexicalBlock(scope: !1601, file: !32, line: 1774, column: 5)
!1606 = !DILocation(line: 1774, column: 5, scope: !1601)
!1607 = !DILocation(line: 1776, column: 13, scope: !1608)
!1608 = distinct !DILexicalBlock(scope: !1609, file: !32, line: 1776, column: 13)
!1609 = distinct !DILexicalBlock(scope: !1605, file: !32, line: 1775, column: 5)
!1610 = !DILocation(line: 1776, column: 17, scope: !1608)
!1611 = !DILocation(line: 1776, column: 13, scope: !1609)
!1612 = !DILocation(line: 1778, column: 29, scope: !1613)
!1613 = distinct !DILexicalBlock(scope: !1608, file: !32, line: 1777, column: 9)
!1614 = !DILocation(line: 1778, column: 33, scope: !1613)
!1615 = !DILocation(line: 1778, column: 40, scope: !1613)
!1616 = !DILocation(line: 1778, column: 13, scope: !1613)
!1617 = !DILocation(line: 1779, column: 13, scope: !1613)
!1618 = !DILocation(line: 1782, column: 17, scope: !1609)
!1619 = !DILocation(line: 1782, column: 9, scope: !1609)
!1620 = !DILocation(line: 1783, column: 14, scope: !1609)
!1621 = !DILocation(line: 1783, column: 12, scope: !1609)
!1622 = !DILocation(line: 1784, column: 17, scope: !1609)
!1623 = !DILocation(line: 1785, column: 9, scope: !1609)
!1624 = !DILocation(line: 1785, column: 30, scope: !1609)
!1625 = !DILocation(line: 1785, column: 34, scope: !1609)
!1626 = !DILocation(line: 1785, column: 44, scope: !1609)
!1627 = !DILocation(line: 1785, column: 48, scope: !1609)
!1628 = !DILocation(line: 1785, column: 16, scope: !1609)
!1629 = !DILocation(line: 1787, column: 18, scope: !1630)
!1630 = distinct !DILexicalBlock(scope: !1631, file: !32, line: 1787, column: 17)
!1631 = distinct !DILexicalBlock(scope: !1609, file: !32, line: 1786, column: 9)
!1632 = !DILocation(line: 1787, column: 17, scope: !1631)
!1633 = !DILocation(line: 1789, column: 44, scope: !1634)
!1634 = distinct !DILexicalBlock(scope: !1630, file: !32, line: 1788, column: 13)
!1635 = !DILocation(line: 1789, column: 23, scope: !1634)
!1636 = !DILocation(line: 1789, column: 20, scope: !1634)
!1637 = !DILocation(line: 1790, column: 13, scope: !1634)
!1638 = !DILocation(line: 1793, column: 18, scope: !1639)
!1639 = distinct !DILexicalBlock(scope: !1630, file: !32, line: 1792, column: 13)
!1640 = !DILocation(line: 1793, column: 21, scope: !1639)
!1641 = !DILocation(line: 1794, column: 31, scope: !1639)
!1642 = !DILocation(line: 1794, column: 36, scope: !1639)
!1643 = !DILocation(line: 1794, column: 34, scope: !1639)
!1644 = !DILocation(line: 1794, column: 23, scope: !1639)
!1645 = !DILocation(line: 1796, column: 21, scope: !1646)
!1646 = distinct !DILexicalBlock(scope: !1639, file: !32, line: 1796, column: 21)
!1647 = !DILocation(line: 1796, column: 25, scope: !1646)
!1648 = !DILocation(line: 1796, column: 38, scope: !1646)
!1649 = !DILocation(line: 1796, column: 43, scope: !1646)
!1650 = !DILocation(line: 1796, column: 21, scope: !1639)
!1651 = !DILocation(line: 1798, column: 61, scope: !1652)
!1652 = distinct !DILexicalBlock(scope: !1646, file: !32, line: 1797, column: 17)
!1653 = !DILocation(line: 1798, column: 30, scope: !1652)
!1654 = !DILocation(line: 1798, column: 28, scope: !1652)
!1655 = !DILocation(line: 1799, column: 17, scope: !1652)
!1656 = !DILocation(line: 1802, column: 38, scope: !1657)
!1657 = distinct !DILexicalBlock(scope: !1646, file: !32, line: 1801, column: 17)
!1658 = !DILocation(line: 1802, column: 28, scope: !1657)
!1659 = !DILocation(line: 1806, column: 21, scope: !1660)
!1660 = distinct !DILexicalBlock(scope: !1639, file: !32, line: 1806, column: 21)
!1661 = !DILocation(line: 1806, column: 25, scope: !1660)
!1662 = !DILocation(line: 1806, column: 29, scope: !1660)
!1663 = !DILocation(line: 1806, column: 39, scope: !1660)
!1664 = !DILocation(line: 1806, column: 21, scope: !1639)
!1665 = !DILocation(line: 1808, column: 47, scope: !1666)
!1666 = distinct !DILexicalBlock(scope: !1667, file: !32, line: 1808, column: 21)
!1667 = distinct !DILexicalBlock(scope: !1660, file: !32, line: 1807, column: 17)
!1668 = !DILocation(line: 1808, column: 28, scope: !1666)
!1669 = !DILocation(line: 1808, column: 26, scope: !1666)
!1670 = !DILocation(line: 1808, column: 62, scope: !1671)
!1671 = distinct !DILexicalBlock(scope: !1666, file: !32, line: 1808, column: 21)
!1672 = !DILocation(line: 1808, column: 61, scope: !1671)
!1673 = !DILocation(line: 1808, column: 59, scope: !1671)
!1674 = !DILocation(line: 1808, column: 21, scope: !1666)
!1675 = !DILocation(line: 1810, column: 41, scope: !1676)
!1676 = distinct !DILexicalBlock(scope: !1677, file: !32, line: 1810, column: 29)
!1677 = distinct !DILexicalBlock(scope: !1671, file: !32, line: 1809, column: 21)
!1678 = !DILocation(line: 1810, column: 29, scope: !1676)
!1679 = !DILocation(line: 1810, column: 29, scope: !1677)
!1680 = !DILocation(line: 1810, column: 44, scope: !1676)
!1681 = !DILocation(line: 1812, column: 41, scope: !1682)
!1682 = distinct !DILexicalBlock(scope: !1676, file: !32, line: 1811, column: 25)
!1683 = !DILocation(line: 1812, column: 44, scope: !1682)
!1684 = !DILocation(line: 1812, column: 50, scope: !1682)
!1685 = !DILocation(line: 1812, column: 56, scope: !1682)
!1686 = !DILocation(line: 1812, column: 38, scope: !1682)
!1687 = !DILocation(line: 1812, column: 34, scope: !1682)
!1688 = !DILocation(line: 1812, column: 30, scope: !1682)
!1689 = !DILocation(line: 1812, column: 32, scope: !1682)
!1690 = !DILocation(line: 1813, column: 25, scope: !1682)
!1691 = !DILocation(line: 1814, column: 46, scope: !1692)
!1692 = distinct !DILexicalBlock(scope: !1676, file: !32, line: 1814, column: 34)
!1693 = !DILocation(line: 1814, column: 34, scope: !1692)
!1694 = !DILocation(line: 1814, column: 34, scope: !1676)
!1695 = !DILocation(line: 1814, column: 49, scope: !1692)
!1696 = !DILocation(line: 1816, column: 41, scope: !1697)
!1697 = distinct !DILexicalBlock(scope: !1692, file: !32, line: 1815, column: 25)
!1698 = !DILocation(line: 1816, column: 44, scope: !1697)
!1699 = !DILocation(line: 1816, column: 50, scope: !1697)
!1700 = !DILocation(line: 1816, column: 56, scope: !1697)
!1701 = !DILocation(line: 1816, column: 38, scope: !1697)
!1702 = !DILocation(line: 1816, column: 34, scope: !1697)
!1703 = !DILocation(line: 1816, column: 30, scope: !1697)
!1704 = !DILocation(line: 1816, column: 32, scope: !1697)
!1705 = !DILocation(line: 1817, column: 25, scope: !1697)
!1706 = !DILocation(line: 1818, column: 21, scope: !1677)
!1707 = !DILocation(line: 1808, column: 66, scope: !1671)
!1708 = !DILocation(line: 1808, column: 21, scope: !1671)
!1709 = distinct !{!1709, !1674, !1710, !238}
!1710 = !DILocation(line: 1818, column: 21, scope: !1666)
!1711 = !DILocation(line: 1819, column: 17, scope: !1667)
!1712 = !DILocation(line: 1822, column: 22, scope: !1713)
!1713 = distinct !DILexicalBlock(scope: !1639, file: !32, line: 1822, column: 21)
!1714 = !DILocation(line: 1822, column: 30, scope: !1713)
!1715 = !DILocation(line: 1822, column: 28, scope: !1713)
!1716 = !DILocation(line: 1822, column: 35, scope: !1713)
!1717 = !DILocation(line: 1822, column: 39, scope: !1713)
!1718 = !DILocation(line: 1822, column: 51, scope: !1713)
!1719 = !DILocation(line: 1823, column: 22, scope: !1713)
!1720 = !DILocation(line: 1823, column: 30, scope: !1713)
!1721 = !DILocation(line: 1823, column: 28, scope: !1713)
!1722 = !DILocation(line: 1823, column: 35, scope: !1713)
!1723 = !DILocation(line: 1823, column: 39, scope: !1713)
!1724 = !DILocation(line: 1823, column: 50, scope: !1713)
!1725 = !DILocation(line: 1823, column: 53, scope: !1713)
!1726 = !DILocation(line: 1822, column: 21, scope: !1639)
!1727 = !DILocation(line: 1825, column: 26, scope: !1728)
!1728 = distinct !DILexicalBlock(scope: !1729, file: !32, line: 1825, column: 25)
!1729 = distinct !DILexicalBlock(scope: !1713, file: !32, line: 1824, column: 17)
!1730 = !DILocation(line: 1825, column: 25, scope: !1729)
!1731 = !DILocation(line: 1828, column: 29, scope: !1732)
!1732 = distinct !DILexicalBlock(scope: !1728, file: !32, line: 1826, column: 21)
!1733 = !DILocation(line: 1828, column: 51, scope: !1732)
!1734 = !DILocation(line: 1828, column: 55, scope: !1732)
!1735 = !DILocation(line: 1828, column: 61, scope: !1732)
!1736 = !DILocation(line: 1828, column: 65, scope: !1732)
!1737 = !DILocation(line: 1828, column: 69, scope: !1732)
!1738 = !DILocation(line: 1827, column: 25, scope: !1732)
!1739 = !DILocation(line: 1829, column: 27, scope: !1732)
!1740 = !DILocation(line: 1829, column: 42, scope: !1732)
!1741 = !DILocation(line: 1830, column: 33, scope: !1732)
!1742 = !DILocation(line: 1831, column: 21, scope: !1732)
!1743 = !DILocation(line: 1832, column: 27, scope: !1729)
!1744 = !DILocation(line: 1832, column: 35, scope: !1729)
!1745 = !DILocation(line: 1832, column: 21, scope: !1729)
!1746 = !DILocation(line: 1833, column: 36, scope: !1729)
!1747 = !DILocation(line: 1833, column: 40, scope: !1729)
!1748 = !DILocation(line: 1833, column: 44, scope: !1729)
!1749 = !DILocation(line: 1833, column: 21, scope: !1729)
!1750 = !DILocation(line: 1834, column: 17, scope: !1729)
!1751 = !DILocation(line: 1836, column: 21, scope: !1752)
!1752 = distinct !DILexicalBlock(scope: !1639, file: !32, line: 1836, column: 21)
!1753 = !DILocation(line: 1836, column: 25, scope: !1752)
!1754 = !DILocation(line: 1836, column: 38, scope: !1752)
!1755 = !DILocation(line: 1836, column: 43, scope: !1752)
!1756 = !DILocation(line: 1836, column: 21, scope: !1639)
!1757 = !DILocation(line: 1838, column: 26, scope: !1758)
!1758 = distinct !DILexicalBlock(scope: !1752, file: !32, line: 1837, column: 17)
!1759 = !DILocation(line: 1838, column: 21, scope: !1758)
!1760 = !DILocation(line: 1839, column: 28, scope: !1758)
!1761 = !DILocation(line: 1840, column: 17, scope: !1758)
!1762 = !DILocation(line: 1842, column: 22, scope: !1639)
!1763 = !DILocation(line: 1842, column: 20, scope: !1639)
!1764 = distinct !{!1764, !1623, !1765, !238}
!1765 = !DILocation(line: 1844, column: 9, scope: !1609)
!1766 = !DILocation(line: 1845, column: 5, scope: !1609)
!1767 = !DILocation(line: 1774, column: 30, scope: !1605)
!1768 = !DILocation(line: 1774, column: 34, scope: !1605)
!1769 = !DILocation(line: 1774, column: 28, scope: !1605)
!1770 = !DILocation(line: 1774, column: 5, scope: !1605)
!1771 = distinct !{!1771, !1606, !1772, !238}
!1772 = !DILocation(line: 1845, column: 5, scope: !1601)
!1773 = !DILocation(line: 1846, column: 1, scope: !1577)
!1774 = !DILocation(line: 241, column: 11, scope: !94)
!1775 = !DILocation(line: 242, column: 5, scope: !94)
!1776 = distinct !DISubprogram(name: "usage", scope: !32, file: !32, line: 245, type: !383, scopeLine: 246, flags: DIFlagPrototyped | DIFlagNoReturn, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1777 = !DILocation(line: 247, column: 19, scope: !1776)
!1778 = !DILocation(line: 247, column: 35, scope: !1776)
!1779 = !DILocation(line: 247, column: 11, scope: !1776)
!1780 = !DILocation(line: 248, column: 19, scope: !1776)
!1781 = !DILocation(line: 248, column: 11, scope: !1776)
!1782 = !DILocation(line: 252, column: 19, scope: !1776)
!1783 = !DILocation(line: 252, column: 11, scope: !1776)
!1784 = !DILocation(line: 254, column: 19, scope: !1776)
!1785 = !DILocation(line: 254, column: 11, scope: !1776)
!1786 = !DILocation(line: 256, column: 19, scope: !1776)
!1787 = !DILocation(line: 256, column: 11, scope: !1776)
!1788 = !DILocation(line: 258, column: 19, scope: !1776)
!1789 = !DILocation(line: 258, column: 11, scope: !1776)
!1790 = !DILocation(line: 260, column: 19, scope: !1776)
!1791 = !DILocation(line: 260, column: 11, scope: !1776)
!1792 = !DILocation(line: 262, column: 19, scope: !1776)
!1793 = !DILocation(line: 262, column: 11, scope: !1776)
!1794 = !DILocation(line: 264, column: 19, scope: !1776)
!1795 = !DILocation(line: 264, column: 11, scope: !1776)
!1796 = !DILocation(line: 265, column: 5, scope: !1776)
!1797 = distinct !DISubprogram(name: "form_file_list", scope: !32, file: !32, line: 894, type: !1798, scopeLine: 895, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!1798 = !DISubroutineType(types: !1799)
!1799 = !{!13, !108, !13}
!1800 = !DILocalVariable(name: "files", arg: 1, scope: !1797, file: !32, line: 894, type: !108)
!1801 = !DILocation(line: 894, column: 34, scope: !1797)
!1802 = !DILocalVariable(name: "file_cnt", arg: 2, scope: !1797, file: !32, line: 894, type: !13)
!1803 = !DILocation(line: 894, column: 45, scope: !1797)
!1804 = !DILocalVariable(name: "i", scope: !1797, file: !32, line: 896, type: !13)
!1805 = !DILocation(line: 896, column: 9, scope: !1797)
!1806 = !DILocalVariable(name: "percent", scope: !1797, file: !32, line: 896, type: !13)
!1807 = !DILocation(line: 896, column: 12, scope: !1797)
!1808 = !DILocalVariable(name: "sp", scope: !1797, file: !32, line: 897, type: !15)
!1809 = !DILocation(line: 897, column: 11, scope: !1797)
!1810 = !DILocalVariable(name: "langdir", scope: !1797, file: !32, line: 898, type: !98)
!1811 = !DILocation(line: 898, column: 10, scope: !1797)
!1812 = !DILocalVariable(name: "fullpathname", scope: !1797, file: !32, line: 899, type: !1813)
!1813 = !DICompositeType(tag: DW_TAG_array_type, baseType: !10, size: 4096, elements: !1814)
!1814 = !{!1815}
!1815 = !DISubrange(count: 512)
!1816 = !DILocation(line: 899, column: 10, scope: !1797)
!1817 = !DILocalVariable(name: "locpathname", scope: !1797, file: !32, line: 899, type: !1813)
!1818 = !DILocation(line: 899, column: 29, scope: !1797)
!1819 = !DILocation(line: 901, column: 9, scope: !1820)
!1820 = distinct !DILexicalBlock(scope: !1797, file: !32, line: 901, column: 9)
!1821 = !DILocation(line: 901, column: 18, scope: !1820)
!1822 = !DILocation(line: 901, column: 9, scope: !1797)
!1823 = !DILocation(line: 903, column: 13, scope: !1824)
!1824 = distinct !DILexicalBlock(scope: !1825, file: !32, line: 903, column: 13)
!1825 = distinct !DILexicalBlock(scope: !1820, file: !32, line: 902, column: 5)
!1826 = !DILocation(line: 903, column: 13, scope: !1825)
!1827 = !DILocation(line: 906, column: 17, scope: !1828)
!1828 = distinct !DILexicalBlock(scope: !1824, file: !32, line: 904, column: 9)
!1829 = !DILocation(line: 907, column: 17, scope: !1828)
!1830 = !DILocation(line: 906, column: 77, scope: !1828)
!1831 = !DILocation(line: 908, column: 17, scope: !1828)
!1832 = !DILocation(line: 907, column: 75, scope: !1828)
!1833 = !DILocation(line: 909, column: 17, scope: !1828)
!1834 = !DILocation(line: 908, column: 77, scope: !1828)
!1835 = !DILocation(line: 905, column: 13, scope: !1828)
!1836 = !DILocation(line: 911, column: 18, scope: !1837)
!1837 = distinct !DILexicalBlock(scope: !1824, file: !32, line: 911, column: 18)
!1838 = !DILocation(line: 911, column: 18, scope: !1824)
!1839 = !DILocation(line: 913, column: 21, scope: !1840)
!1840 = distinct !DILexicalBlock(scope: !1837, file: !32, line: 912, column: 9)
!1841 = !DILocation(line: 914, column: 21, scope: !1840)
!1842 = !DILocation(line: 913, column: 79, scope: !1840)
!1843 = !DILocation(line: 913, column: 13, scope: !1840)
!1844 = !DILocation(line: 918, column: 17, scope: !1845)
!1845 = distinct !DILexicalBlock(scope: !1846, file: !32, line: 918, column: 17)
!1846 = distinct !DILexicalBlock(scope: !1837, file: !32, line: 917, column: 9)
!1847 = !DILocation(line: 918, column: 17, scope: !1846)
!1848 = !DILocalVariable(name: "lang", scope: !1849, file: !32, line: 920, type: !15)
!1849 = distinct !DILexicalBlock(scope: !1845, file: !32, line: 919, column: 13)
!1850 = !DILocation(line: 920, column: 23, scope: !1849)
!1851 = !DILocalVariable(name: "llang", scope: !1849, file: !32, line: 921, type: !1813)
!1852 = !DILocation(line: 921, column: 22, scope: !1849)
!1853 = !DILocalVariable(name: "ret", scope: !1849, file: !32, line: 922, type: !13)
!1854 = !DILocation(line: 922, column: 21, scope: !1849)
!1855 = !DILocalVariable(name: "p", scope: !1849, file: !32, line: 923, type: !15)
!1856 = !DILocation(line: 923, column: 23, scope: !1849)
!1857 = !DILocation(line: 925, column: 25, scope: !1849)
!1858 = !DILocation(line: 925, column: 32, scope: !1849)
!1859 = !DILocation(line: 925, column: 17, scope: !1849)
!1860 = !DILocation(line: 926, column: 17, scope: !1849)
!1861 = !DILocation(line: 926, column: 42, scope: !1849)
!1862 = !DILocation(line: 927, column: 24, scope: !1849)
!1863 = !DILocation(line: 927, column: 22, scope: !1849)
!1864 = !DILocation(line: 930, column: 17, scope: !1849)
!1865 = !DILocation(line: 930, column: 24, scope: !1849)
!1866 = !DILocation(line: 930, column: 29, scope: !1849)
!1867 = !DILocation(line: 930, column: 34, scope: !1849)
!1868 = !DILocation(line: 930, column: 33, scope: !1849)
!1869 = !DILocation(line: 930, column: 32, scope: !1849)
!1870 = !DILocation(line: 0, scope: !1849)
!1871 = !DILocation(line: 932, column: 32, scope: !1872)
!1872 = distinct !DILexicalBlock(scope: !1849, file: !32, line: 931, column: 17)
!1873 = !DILocation(line: 932, column: 25, scope: !1872)
!1874 = !DILocation(line: 932, column: 23, scope: !1872)
!1875 = !DILocation(line: 933, column: 25, scope: !1876)
!1876 = distinct !DILexicalBlock(scope: !1872, file: !32, line: 933, column: 25)
!1877 = !DILocation(line: 933, column: 25, scope: !1872)
!1878 = !DILocation(line: 935, column: 27, scope: !1879)
!1879 = distinct !DILexicalBlock(scope: !1876, file: !32, line: 934, column: 21)
!1880 = !DILocation(line: 935, column: 30, scope: !1879)
!1881 = !DILocation(line: 936, column: 21, scope: !1879)
!1882 = !DILocation(line: 940, column: 34, scope: !1872)
!1883 = !DILocation(line: 939, column: 27, scope: !1872)
!1884 = !DILocation(line: 939, column: 25, scope: !1872)
!1885 = !DILocation(line: 943, column: 26, scope: !1886)
!1886 = distinct !DILexicalBlock(scope: !1872, file: !32, line: 943, column: 25)
!1887 = !DILocation(line: 943, column: 25, scope: !1872)
!1888 = !DILocalVariable(name: "ll", scope: !1889, file: !32, line: 945, type: !1890)
!1889 = distinct !DILexicalBlock(scope: !1886, file: !32, line: 944, column: 21)
!1890 = !DICompositeType(tag: DW_TAG_array_type, baseType: !10, size: 24, elements: !1891)
!1891 = !{!1892}
!1892 = !DISubrange(count: 3)
!1893 = !DILocation(line: 945, column: 30, scope: !1889)
!1894 = !DILocation(line: 947, column: 33, scope: !1889)
!1895 = !DILocation(line: 947, column: 37, scope: !1889)
!1896 = !DILocation(line: 947, column: 25, scope: !1889)
!1897 = !DILocation(line: 948, column: 25, scope: !1889)
!1898 = !DILocation(line: 948, column: 31, scope: !1889)
!1899 = !DILocation(line: 950, column: 38, scope: !1889)
!1900 = !DILocation(line: 949, column: 31, scope: !1889)
!1901 = !DILocation(line: 949, column: 29, scope: !1889)
!1902 = !DILocation(line: 951, column: 21, scope: !1889)
!1903 = !DILocation(line: 954, column: 25, scope: !1904)
!1904 = distinct !DILexicalBlock(scope: !1872, file: !32, line: 954, column: 25)
!1905 = !DILocation(line: 954, column: 25, scope: !1872)
!1906 = !DILocation(line: 956, column: 32, scope: !1907)
!1907 = distinct !DILexicalBlock(scope: !1904, file: !32, line: 955, column: 21)
!1908 = !DILocation(line: 956, column: 25, scope: !1907)
!1909 = !DILocation(line: 958, column: 28, scope: !1872)
!1910 = !DILocation(line: 958, column: 26, scope: !1872)
!1911 = distinct !{!1911, !1864, !1912, !238}
!1912 = !DILocation(line: 959, column: 17, scope: !1849)
!1913 = !DILocation(line: 961, column: 24, scope: !1849)
!1914 = !DILocation(line: 961, column: 17, scope: !1849)
!1915 = !DILocation(line: 966, column: 24, scope: !1916)
!1916 = distinct !DILexicalBlock(scope: !1845, file: !32, line: 964, column: 13)
!1917 = !DILocation(line: 966, column: 17, scope: !1916)
!1918 = !DILocation(line: 971, column: 12, scope: !1919)
!1919 = distinct !DILexicalBlock(scope: !1797, file: !32, line: 971, column: 5)
!1920 = !DILocation(line: 971, column: 10, scope: !1919)
!1921 = !DILocation(line: 971, column: 17, scope: !1922)
!1922 = distinct !DILexicalBlock(scope: !1919, file: !32, line: 971, column: 5)
!1923 = !DILocation(line: 971, column: 21, scope: !1922)
!1924 = !DILocation(line: 971, column: 19, scope: !1922)
!1925 = !DILocation(line: 971, column: 5, scope: !1919)
!1926 = !DILocation(line: 973, column: 17, scope: !1927)
!1927 = distinct !DILexicalBlock(scope: !1922, file: !32, line: 972, column: 5)
!1928 = !DILocation(line: 974, column: 12, scope: !1927)
!1929 = !DILocation(line: 975, column: 13, scope: !1927)
!1930 = !DILocation(line: 975, column: 14, scope: !1931)
!1931 = distinct !DILexicalBlock(scope: !1927, file: !32, line: 975, column: 13)
!1932 = !DILocation(line: 977, column: 18, scope: !1933)
!1933 = distinct !DILexicalBlock(scope: !1931, file: !32, line: 976, column: 9)
!1934 = !DILocation(line: 977, column: 24, scope: !1933)
!1935 = !DILocation(line: 977, column: 16, scope: !1933)
!1936 = !DILocation(line: 978, column: 9, scope: !1933)
!1937 = !DILocalVariable(name: "MAX_PERCENT", scope: !1938, file: !32, line: 981, type: !1939)
!1938 = distinct !DILexicalBlock(scope: !1931, file: !32, line: 980, column: 9)
!1939 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !13)
!1940 = !DILocation(line: 981, column: 23, scope: !1938)
!1941 = !DILocalVariable(name: "percent_has_overflowed", scope: !1938, file: !32, line: 982, type: !64)
!1942 = !DILocation(line: 982, column: 18, scope: !1938)
!1943 = !DILocation(line: 983, column: 21, scope: !1938)
!1944 = !DILocation(line: 984, column: 23, scope: !1945)
!1945 = distinct !DILexicalBlock(scope: !1938, file: !32, line: 984, column: 13)
!1946 = !DILocation(line: 984, column: 29, scope: !1945)
!1947 = !DILocation(line: 984, column: 21, scope: !1945)
!1948 = !DILocation(line: 984, column: 18, scope: !1945)
!1949 = !DILocation(line: 984, column: 33, scope: !1950)
!1950 = distinct !DILexicalBlock(scope: !1945, file: !32, line: 984, column: 13)
!1951 = !DILocation(line: 984, column: 13, scope: !1945)
!1952 = !DILocation(line: 986, column: 27, scope: !1953)
!1953 = distinct !DILexicalBlock(scope: !1950, file: !32, line: 985, column: 13)
!1954 = !DILocation(line: 986, column: 35, scope: !1953)
!1955 = !DILocation(line: 986, column: 43, scope: !1953)
!1956 = !DILocation(line: 986, column: 42, scope: !1953)
!1957 = !DILocation(line: 986, column: 40, scope: !1953)
!1958 = !DILocation(line: 986, column: 46, scope: !1953)
!1959 = !DILocation(line: 986, column: 25, scope: !1953)
!1960 = !DILocation(line: 987, column: 43, scope: !1953)
!1961 = !DILocation(line: 987, column: 51, scope: !1953)
!1962 = !DILocation(line: 987, column: 40, scope: !1953)
!1963 = !DILocation(line: 988, column: 21, scope: !1964)
!1964 = distinct !DILexicalBlock(scope: !1953, file: !32, line: 988, column: 21)
!1965 = !DILocation(line: 988, column: 21, scope: !1953)
!1966 = !DILocation(line: 990, column: 21, scope: !1967)
!1967 = distinct !DILexicalBlock(scope: !1964, file: !32, line: 989, column: 17)
!1968 = !DILocation(line: 992, column: 13, scope: !1953)
!1969 = !DILocation(line: 984, column: 49, scope: !1950)
!1970 = !DILocation(line: 984, column: 13, scope: !1950)
!1971 = distinct !{!1971, !1951, !1972, !238}
!1972 = !DILocation(line: 992, column: 13, scope: !1945)
!1973 = !DILocation(line: 993, column: 17, scope: !1974)
!1974 = distinct !DILexicalBlock(scope: !1938, file: !32, line: 993, column: 17)
!1975 = !DILocation(line: 993, column: 40, scope: !1974)
!1976 = !DILocation(line: 993, column: 44, scope: !1974)
!1977 = !DILocation(line: 993, column: 52, scope: !1974)
!1978 = !DILocation(line: 993, column: 17, scope: !1938)
!1979 = !DILocation(line: 995, column: 25, scope: !1980)
!1980 = distinct !DILexicalBlock(scope: !1974, file: !32, line: 994, column: 13)
!1981 = !DILocation(line: 995, column: 17, scope: !1980)
!1982 = !DILocation(line: 996, column: 25, scope: !1980)
!1983 = !DILocation(line: 998, column: 21, scope: !1980)
!1984 = !DILocation(line: 998, column: 27, scope: !1980)
!1985 = !DILocation(line: 996, column: 17, scope: !1980)
!1986 = !DILocation(line: 999, column: 30, scope: !1980)
!1987 = !DILocation(line: 1000, column: 17, scope: !1980)
!1988 = !DILocation(line: 1002, column: 17, scope: !1989)
!1989 = distinct !DILexicalBlock(scope: !1938, file: !32, line: 1002, column: 17)
!1990 = !DILocation(line: 1002, column: 25, scope: !1989)
!1991 = !DILocation(line: 1002, column: 17, scope: !1938)
!1992 = !DILocation(line: 1004, column: 25, scope: !1993)
!1993 = distinct !DILexicalBlock(scope: !1989, file: !32, line: 1003, column: 13)
!1994 = !DILocation(line: 1006, column: 21, scope: !1993)
!1995 = !DILocation(line: 1006, column: 27, scope: !1993)
!1996 = !DILocation(line: 1004, column: 17, scope: !1993)
!1997 = !DILocation(line: 1007, column: 30, scope: !1993)
!1998 = !DILocation(line: 1008, column: 17, scope: !1993)
!1999 = !DILocation(line: 1010, column: 18, scope: !2000)
!2000 = distinct !DILexicalBlock(scope: !1938, file: !32, line: 1010, column: 17)
!2001 = !DILocation(line: 1010, column: 17, scope: !2000)
!2002 = !DILocation(line: 1010, column: 21, scope: !2000)
!2003 = !DILocation(line: 1010, column: 17, scope: !1938)
!2004 = !DILocation(line: 1012, column: 25, scope: !2005)
!2005 = distinct !DILexicalBlock(scope: !2000, file: !32, line: 1011, column: 13)
!2006 = !DILocation(line: 1012, column: 17, scope: !2005)
!2007 = !DILocation(line: 1013, column: 30, scope: !2005)
!2008 = !DILocation(line: 1014, column: 17, scope: !2005)
!2009 = !DILocation(line: 1021, column: 18, scope: !2010)
!2010 = distinct !DILexicalBlock(scope: !1938, file: !32, line: 1021, column: 17)
!2011 = !DILocation(line: 1021, column: 17, scope: !2010)
!2012 = !DILocation(line: 1021, column: 21, scope: !2010)
!2013 = !DILocation(line: 1021, column: 17, scope: !1938)
!2014 = !DILocation(line: 1023, column: 25, scope: !2015)
!2015 = distinct !DILexicalBlock(scope: !2010, file: !32, line: 1022, column: 13)
!2016 = !DILocation(line: 1024, column: 22, scope: !2015)
!2017 = !DILocation(line: 1024, column: 28, scope: !2015)
!2018 = !DILocation(line: 1024, column: 20, scope: !2015)
!2019 = !DILocation(line: 1025, column: 13, scope: !2015)
!2020 = !DILocation(line: 1026, column: 23, scope: !2021)
!2021 = distinct !DILexicalBlock(scope: !2010, file: !32, line: 1026, column: 22)
!2022 = !DILocation(line: 1026, column: 22, scope: !2021)
!2023 = !DILocation(line: 1026, column: 28, scope: !2021)
!2024 = !DILocation(line: 1026, column: 22, scope: !2010)
!2025 = !DILocation(line: 1028, column: 21, scope: !2026)
!2026 = distinct !DILexicalBlock(scope: !2027, file: !32, line: 1028, column: 21)
!2027 = distinct !DILexicalBlock(scope: !2021, file: !32, line: 1027, column: 13)
!2028 = !DILocation(line: 1028, column: 28, scope: !2026)
!2029 = !DILocation(line: 1028, column: 25, scope: !2026)
!2030 = !DILocation(line: 1028, column: 21, scope: !2027)
!2031 = !DILocation(line: 1030, column: 29, scope: !2032)
!2032 = distinct !DILexicalBlock(scope: !2026, file: !32, line: 1029, column: 17)
!2033 = !DILocation(line: 1030, column: 21, scope: !2032)
!2034 = !DILocation(line: 1031, column: 34, scope: !2032)
!2035 = !DILocation(line: 1032, column: 21, scope: !2032)
!2036 = !DILocation(line: 1034, column: 22, scope: !2027)
!2037 = !DILocation(line: 1034, column: 28, scope: !2027)
!2038 = !DILocation(line: 1034, column: 20, scope: !2027)
!2039 = !DILocation(line: 1035, column: 13, scope: !2027)
!2040 = !DILocalVariable(name: "offensive", scope: !1927, file: !32, line: 1039, type: !64)
!2041 = !DILocation(line: 1039, column: 14, scope: !1927)
!2042 = !DILocalVariable(name: "sp_len", scope: !1927, file: !32, line: 1040, type: !1561)
!2043 = !DILocation(line: 1040, column: 22, scope: !1927)
!2044 = !DILocation(line: 1040, column: 38, scope: !1927)
!2045 = !DILocation(line: 1040, column: 31, scope: !1927)
!2046 = !DILocation(line: 1041, column: 13, scope: !2047)
!2047 = distinct !DILexicalBlock(scope: !1927, file: !32, line: 1041, column: 13)
!2048 = !DILocation(line: 1041, column: 20, scope: !2047)
!2049 = !DILocation(line: 1041, column: 25, scope: !2047)
!2050 = !DILocation(line: 1041, column: 28, scope: !2047)
!2051 = !DILocation(line: 1041, column: 31, scope: !2047)
!2052 = !DILocation(line: 1041, column: 38, scope: !2047)
!2053 = !DILocation(line: 1041, column: 43, scope: !2047)
!2054 = !DILocation(line: 1041, column: 50, scope: !2047)
!2055 = !DILocation(line: 1041, column: 53, scope: !2047)
!2056 = !DILocation(line: 1041, column: 56, scope: !2047)
!2057 = !DILocation(line: 1041, column: 63, scope: !2047)
!2058 = !DILocation(line: 1041, column: 68, scope: !2047)
!2059 = !DILocation(line: 1041, column: 13, scope: !1927)
!2060 = !DILocation(line: 1043, column: 13, scope: !2061)
!2061 = distinct !DILexicalBlock(scope: !2047, file: !32, line: 1042, column: 9)
!2062 = !DILocation(line: 1043, column: 16, scope: !2061)
!2063 = !DILocation(line: 1043, column: 23, scope: !2061)
!2064 = !DILocation(line: 1043, column: 28, scope: !2061)
!2065 = !DILocation(line: 1044, column: 23, scope: !2061)
!2066 = !DILocation(line: 1045, column: 9, scope: !2061)
!2067 = !DILocalVariable(name: "fulldir", scope: !1927, file: !32, line: 1047, type: !8)
!2068 = !DILocation(line: 1047, column: 21, scope: !1927)
!2069 = !DILocation(line: 1047, column: 31, scope: !1927)
!2070 = !DILocalVariable(name: "locdir", scope: !1927, file: !32, line: 1050, type: !8)
!2071 = !DILocation(line: 1050, column: 21, scope: !1927)
!2072 = !DILocation(line: 1050, column: 30, scope: !1927)
!2073 = !DILocation(line: 1053, column: 20, scope: !2074)
!2074 = distinct !DILexicalBlock(scope: !1927, file: !32, line: 1053, column: 13)
!2075 = !DILocation(line: 1053, column: 13, scope: !2074)
!2076 = !DILocation(line: 1053, column: 31, scope: !2074)
!2077 = !DILocation(line: 1053, column: 13, scope: !1927)
!2078 = !DILocation(line: 1055, column: 22, scope: !2079)
!2079 = distinct !DILexicalBlock(scope: !2074, file: !32, line: 1054, column: 9)
!2080 = !DILocation(line: 1055, column: 64, scope: !2079)
!2081 = !DILocation(line: 1055, column: 13, scope: !2079)
!2082 = !DILocation(line: 1056, column: 22, scope: !2079)
!2083 = !DILocation(line: 1056, column: 62, scope: !2079)
!2084 = !DILocation(line: 1056, column: 13, scope: !2079)
!2085 = !DILocation(line: 1057, column: 9, scope: !2079)
!2086 = !DILocation(line: 1063, column: 25, scope: !2087)
!2087 = distinct !DILexicalBlock(scope: !2088, file: !32, line: 1063, column: 17)
!2088 = distinct !DILexicalBlock(scope: !2074, file: !32, line: 1062, column: 9)
!2089 = !DILocation(line: 1063, column: 17, scope: !2087)
!2090 = !DILocation(line: 1063, column: 37, scope: !2087)
!2091 = !DILocation(line: 1063, column: 42, scope: !2087)
!2092 = !DILocation(line: 1063, column: 53, scope: !2087)
!2093 = !DILocation(line: 1063, column: 45, scope: !2087)
!2094 = !DILocation(line: 1063, column: 66, scope: !2087)
!2095 = !DILocation(line: 1063, column: 71, scope: !2087)
!2096 = !DILocation(line: 1064, column: 25, scope: !2087)
!2097 = !DILocation(line: 1064, column: 17, scope: !2087)
!2098 = !DILocation(line: 1064, column: 39, scope: !2087)
!2099 = !DILocation(line: 1063, column: 17, scope: !2088)
!2100 = !DILocation(line: 1067, column: 21, scope: !2101)
!2101 = distinct !DILexicalBlock(scope: !2087, file: !32, line: 1065, column: 13)
!2102 = !DILocation(line: 1067, column: 66, scope: !2101)
!2103 = !DILocation(line: 1067, column: 75, scope: !2101)
!2104 = !DILocation(line: 1066, column: 17, scope: !2101)
!2105 = !DILocation(line: 1068, column: 26, scope: !2101)
!2106 = !DILocation(line: 1068, column: 69, scope: !2101)
!2107 = !DILocation(line: 1068, column: 77, scope: !2101)
!2108 = !DILocation(line: 1068, column: 17, scope: !2101)
!2109 = !DILocation(line: 1069, column: 13, scope: !2101)
!2110 = !DILocation(line: 1072, column: 26, scope: !2111)
!2111 = distinct !DILexicalBlock(scope: !2087, file: !32, line: 1071, column: 13)
!2112 = !DILocation(line: 1072, column: 68, scope: !2111)
!2113 = !DILocation(line: 1072, column: 17, scope: !2111)
!2114 = !DILocation(line: 1073, column: 26, scope: !2111)
!2115 = !DILocation(line: 1073, column: 66, scope: !2111)
!2116 = !DILocation(line: 1073, column: 17, scope: !2111)
!2117 = !DILocation(line: 1077, column: 13, scope: !2118)
!2118 = distinct !DILexicalBlock(scope: !1927, file: !32, line: 1077, column: 13)
!2119 = !DILocation(line: 1077, column: 13, scope: !1927)
!2120 = !DILocalVariable(name: "llang", scope: !2121, file: !32, line: 1079, type: !1813)
!2121 = distinct !DILexicalBlock(scope: !2118, file: !32, line: 1078, column: 9)
!2122 = !DILocation(line: 1079, column: 18, scope: !2121)
!2123 = !DILocalVariable(name: "ret", scope: !2121, file: !32, line: 1080, type: !13)
!2124 = !DILocation(line: 1080, column: 17, scope: !2121)
!2125 = !DILocation(line: 1082, column: 21, scope: !2121)
!2126 = !DILocation(line: 1082, column: 28, scope: !2121)
!2127 = !DILocation(line: 1082, column: 13, scope: !2121)
!2128 = !DILocation(line: 1083, column: 13, scope: !2121)
!2129 = !DILocation(line: 1083, column: 38, scope: !2121)
!2130 = !DILocalVariable(name: "lang", scope: !2121, file: !32, line: 1084, type: !15)
!2131 = !DILocation(line: 1084, column: 19, scope: !2121)
!2132 = !DILocation(line: 1084, column: 26, scope: !2121)
!2133 = !DILocation(line: 1087, column: 13, scope: !2121)
!2134 = !DILocation(line: 1087, column: 21, scope: !2121)
!2135 = !DILocation(line: 1087, column: 25, scope: !2121)
!2136 = !DILocation(line: 1087, column: 28, scope: !2121)
!2137 = !DILocation(line: 1087, column: 33, scope: !2121)
!2138 = !DILocation(line: 1087, column: 38, scope: !2121)
!2139 = !DILocation(line: 1087, column: 37, scope: !2121)
!2140 = !DILocation(line: 1087, column: 36, scope: !2121)
!2141 = !DILocation(line: 0, scope: !2121)
!2142 = !DILocalVariable(name: "p", scope: !2143, file: !32, line: 1089, type: !15)
!2143 = distinct !DILexicalBlock(scope: !2121, file: !32, line: 1088, column: 13)
!2144 = !DILocation(line: 1089, column: 23, scope: !2143)
!2145 = !DILocation(line: 1089, column: 34, scope: !2143)
!2146 = !DILocation(line: 1089, column: 27, scope: !2143)
!2147 = !DILocation(line: 1090, column: 21, scope: !2148)
!2148 = distinct !DILexicalBlock(scope: !2143, file: !32, line: 1090, column: 21)
!2149 = !DILocation(line: 1090, column: 21, scope: !2143)
!2150 = !DILocation(line: 1092, column: 23, scope: !2151)
!2151 = distinct !DILexicalBlock(scope: !2148, file: !32, line: 1091, column: 17)
!2152 = !DILocation(line: 1092, column: 26, scope: !2151)
!2153 = !DILocation(line: 1093, column: 17, scope: !2151)
!2154 = !DILocation(line: 1097, column: 21, scope: !2143)
!2155 = !DILocation(line: 1097, column: 59, scope: !2143)
!2156 = !DILocation(line: 1097, column: 68, scope: !2143)
!2157 = !DILocation(line: 1097, column: 74, scope: !2143)
!2158 = !DILocation(line: 1096, column: 17, scope: !2143)
!2159 = !DILocation(line: 1099, column: 21, scope: !2143)
!2160 = !DILocation(line: 1099, column: 30, scope: !2143)
!2161 = !DILocation(line: 1098, column: 23, scope: !2143)
!2162 = !DILocation(line: 1098, column: 21, scope: !2143)
!2163 = !DILocation(line: 1102, column: 22, scope: !2164)
!2164 = distinct !DILexicalBlock(scope: !2143, file: !32, line: 1102, column: 21)
!2165 = !DILocation(line: 1102, column: 21, scope: !2143)
!2166 = !DILocalVariable(name: "ll", scope: !2167, file: !32, line: 1104, type: !1890)
!2167 = distinct !DILexicalBlock(scope: !2164, file: !32, line: 1103, column: 17)
!2168 = !DILocation(line: 1104, column: 26, scope: !2167)
!2169 = !DILocation(line: 1106, column: 29, scope: !2167)
!2170 = !DILocation(line: 1106, column: 33, scope: !2167)
!2171 = !DILocation(line: 1106, column: 21, scope: !2167)
!2172 = !DILocation(line: 1107, column: 21, scope: !2167)
!2173 = !DILocation(line: 1107, column: 27, scope: !2167)
!2174 = !DILocation(line: 1109, column: 25, scope: !2167)
!2175 = !DILocation(line: 1109, column: 63, scope: !2167)
!2176 = !DILocation(line: 1109, column: 72, scope: !2167)
!2177 = !DILocation(line: 1109, column: 76, scope: !2167)
!2178 = !DILocation(line: 1108, column: 21, scope: !2167)
!2179 = !DILocation(line: 1111, column: 25, scope: !2167)
!2180 = !DILocation(line: 1111, column: 34, scope: !2167)
!2181 = !DILocation(line: 1110, column: 27, scope: !2167)
!2182 = !DILocation(line: 1110, column: 25, scope: !2167)
!2183 = !DILocation(line: 1112, column: 17, scope: !2167)
!2184 = !DILocation(line: 1114, column: 24, scope: !2143)
!2185 = !DILocation(line: 1114, column: 22, scope: !2143)
!2186 = distinct !{!2186, !2133, !2187, !238}
!2187 = !DILocation(line: 1115, column: 13, scope: !2121)
!2188 = !DILocalVariable(name: "system_dir_list", scope: !2121, file: !32, line: 1117, type: !47)
!2189 = !DILocation(line: 1117, column: 23, scope: !2121)
!2190 = !DILocalVariable(name: "local_list", scope: !2121, file: !32, line: 1117, type: !47)
!2191 = !DILocation(line: 1117, column: 48, scope: !2121)
!2192 = !DILocalVariable(name: "system_dir_tail", scope: !2121, file: !32, line: 1118, type: !47)
!2193 = !DILocation(line: 1118, column: 23, scope: !2121)
!2194 = !DILocalVariable(name: "local_tail", scope: !2121, file: !32, line: 1118, type: !47)
!2195 = !DILocation(line: 1118, column: 48, scope: !2121)
!2196 = !DILocation(line: 1119, column: 18, scope: !2197)
!2197 = distinct !DILexicalBlock(scope: !2121, file: !32, line: 1119, column: 17)
!2198 = !DILocation(line: 1119, column: 17, scope: !2121)
!2199 = !DILocation(line: 1121, column: 37, scope: !2200)
!2200 = distinct !DILexicalBlock(scope: !2197, file: !32, line: 1120, column: 13)
!2201 = !DILocation(line: 1121, column: 23, scope: !2200)
!2202 = !DILocation(line: 1121, column: 21, scope: !2200)
!2203 = !DILocation(line: 1123, column: 13, scope: !2200)
!2204 = !DILocation(line: 1124, column: 25, scope: !2205)
!2205 = distinct !DILexicalBlock(scope: !2121, file: !32, line: 1124, column: 17)
!2206 = !DILocation(line: 1124, column: 39, scope: !2205)
!2207 = !DILocation(line: 1124, column: 17, scope: !2205)
!2208 = !DILocation(line: 1124, column: 17, scope: !2121)
!2209 = !DILocation(line: 1127, column: 26, scope: !2210)
!2210 = distinct !DILexicalBlock(scope: !2205, file: !32, line: 1125, column: 13)
!2211 = !DILocation(line: 1126, column: 17, scope: !2210)
!2212 = !DILocation(line: 1128, column: 13, scope: !2210)
!2213 = !DILocation(line: 1129, column: 18, scope: !2214)
!2214 = distinct !DILexicalBlock(scope: !2121, file: !32, line: 1129, column: 17)
!2215 = !DILocation(line: 1129, column: 17, scope: !2121)
!2216 = !DILocation(line: 1131, column: 21, scope: !2217)
!2217 = distinct !DILexicalBlock(scope: !2218, file: !32, line: 1131, column: 21)
!2218 = distinct !DILexicalBlock(scope: !2214, file: !32, line: 1130, column: 13)
!2219 = !DILocation(line: 1131, column: 21, scope: !2218)
!2220 = !DILocation(line: 1133, column: 48, scope: !2221)
!2221 = distinct !DILexicalBlock(scope: !2217, file: !32, line: 1132, column: 17)
!2222 = !DILocation(line: 1133, column: 21, scope: !2221)
!2223 = !DILocation(line: 1133, column: 38, scope: !2221)
!2224 = !DILocation(line: 1133, column: 46, scope: !2221)
!2225 = !DILocation(line: 1134, column: 25, scope: !2226)
!2226 = distinct !DILexicalBlock(scope: !2221, file: !32, line: 1134, column: 25)
!2227 = !DILocation(line: 1134, column: 25, scope: !2221)
!2228 = !DILocalVariable(name: "old", scope: !2229, file: !32, line: 1136, type: !47)
!2229 = distinct !DILexicalBlock(scope: !2226, file: !32, line: 1135, column: 21)
!2230 = !DILocation(line: 1136, column: 35, scope: !2229)
!2231 = !DILocation(line: 1136, column: 41, scope: !2229)
!2232 = !DILocation(line: 1137, column: 49, scope: !2229)
!2233 = !DILocation(line: 1137, column: 25, scope: !2229)
!2234 = !DILocation(line: 1137, column: 42, scope: !2229)
!2235 = !DILocation(line: 1137, column: 47, scope: !2229)
!2236 = !DILocation(line: 1138, column: 37, scope: !2229)
!2237 = !DILocation(line: 1138, column: 25, scope: !2229)
!2238 = !DILocation(line: 1138, column: 30, scope: !2229)
!2239 = !DILocation(line: 1138, column: 35, scope: !2229)
!2240 = !DILocation(line: 1139, column: 37, scope: !2229)
!2241 = !DILocation(line: 1139, column: 35, scope: !2229)
!2242 = !DILocation(line: 1140, column: 21, scope: !2229)
!2243 = !DILocation(line: 1143, column: 37, scope: !2244)
!2244 = distinct !DILexicalBlock(scope: !2226, file: !32, line: 1142, column: 21)
!2245 = !DILocation(line: 1143, column: 35, scope: !2244)
!2246 = !DILocation(line: 1144, column: 37, scope: !2244)
!2247 = !DILocation(line: 1144, column: 35, scope: !2244)
!2248 = !DILocation(line: 1146, column: 17, scope: !2221)
!2249 = !DILocation(line: 1149, column: 25, scope: !2250)
!2250 = distinct !DILexicalBlock(scope: !2217, file: !32, line: 1148, column: 17)
!2251 = !DILocation(line: 1151, column: 13, scope: !2218)
!2252 = !DILocation(line: 1152, column: 30, scope: !2253)
!2253 = distinct !DILexicalBlock(scope: !2214, file: !32, line: 1152, column: 22)
!2254 = !DILocation(line: 1152, column: 44, scope: !2253)
!2255 = !DILocation(line: 1152, column: 22, scope: !2253)
!2256 = !DILocation(line: 1152, column: 79, scope: !2253)
!2257 = !DILocation(line: 1153, column: 29, scope: !2253)
!2258 = !DILocation(line: 1153, column: 22, scope: !2253)
!2259 = !DILocation(line: 1153, column: 40, scope: !2253)
!2260 = !DILocation(line: 1152, column: 22, scope: !2214)
!2261 = !DILocation(line: 1155, column: 21, scope: !2262)
!2262 = distinct !DILexicalBlock(scope: !2263, file: !32, line: 1155, column: 21)
!2263 = distinct !DILexicalBlock(scope: !2253, file: !32, line: 1154, column: 13)
!2264 = !DILocation(line: 1155, column: 21, scope: !2263)
!2265 = !DILocalVariable(name: "parent_node", scope: !2266, file: !32, line: 1157, type: !47)
!2266 = distinct !DILexicalBlock(scope: !2262, file: !32, line: 1156, column: 17)
!2267 = !DILocation(line: 1157, column: 31, scope: !2266)
!2268 = !DILocation(line: 1157, column: 45, scope: !2266)
!2269 = !DILocation(line: 1158, column: 44, scope: !2266)
!2270 = !DILocation(line: 1158, column: 21, scope: !2266)
!2271 = !DILocation(line: 1158, column: 34, scope: !2266)
!2272 = !DILocation(line: 1158, column: 42, scope: !2266)
!2273 = !DILocation(line: 1159, column: 42, scope: !2266)
!2274 = !DILocation(line: 1159, column: 21, scope: !2266)
!2275 = !DILocation(line: 1159, column: 34, scope: !2266)
!2276 = !DILocation(line: 1159, column: 40, scope: !2266)
!2277 = !DILocation(line: 1160, column: 45, scope: !2266)
!2278 = !DILocation(line: 1160, column: 21, scope: !2266)
!2279 = !DILocation(line: 1160, column: 38, scope: !2266)
!2280 = !DILocation(line: 1160, column: 43, scope: !2266)
!2281 = !DILocation(line: 1161, column: 25, scope: !2282)
!2282 = distinct !DILexicalBlock(scope: !2266, file: !32, line: 1161, column: 25)
!2283 = !DILocation(line: 1161, column: 25, scope: !2266)
!2284 = !DILocation(line: 1163, column: 44, scope: !2285)
!2285 = distinct !DILexicalBlock(scope: !2282, file: !32, line: 1162, column: 21)
!2286 = !DILocation(line: 1163, column: 25, scope: !2285)
!2287 = !DILocation(line: 1163, column: 37, scope: !2285)
!2288 = !DILocation(line: 1163, column: 42, scope: !2285)
!2289 = !DILocation(line: 1164, column: 21, scope: !2285)
!2290 = !DILocation(line: 1165, column: 47, scope: !2266)
!2291 = !DILocation(line: 1165, column: 21, scope: !2266)
!2292 = !DILocation(line: 1165, column: 38, scope: !2266)
!2293 = !DILocation(line: 1165, column: 45, scope: !2266)
!2294 = !DILocation(line: 1166, column: 25, scope: !2295)
!2295 = distinct !DILexicalBlock(scope: !2266, file: !32, line: 1166, column: 25)
!2296 = !DILocation(line: 1166, column: 25, scope: !2266)
!2297 = !DILocation(line: 1168, column: 46, scope: !2298)
!2298 = distinct !DILexicalBlock(scope: !2295, file: !32, line: 1167, column: 21)
!2299 = !DILocation(line: 1168, column: 25, scope: !2298)
!2300 = !DILocation(line: 1168, column: 37, scope: !2298)
!2301 = !DILocation(line: 1168, column: 44, scope: !2298)
!2302 = !DILocation(line: 1169, column: 21, scope: !2298)
!2303 = !DILocation(line: 1170, column: 45, scope: !2266)
!2304 = !DILocation(line: 1170, column: 43, scope: !2266)
!2305 = !DILocation(line: 1170, column: 31, scope: !2266)
!2306 = !DILocation(line: 1171, column: 25, scope: !2266)
!2307 = !DILocation(line: 1172, column: 17, scope: !2266)
!2308 = !DILocation(line: 1175, column: 25, scope: !2309)
!2309 = distinct !DILexicalBlock(scope: !2262, file: !32, line: 1174, column: 17)
!2310 = !DILocation(line: 1177, column: 13, scope: !2263)
!2311 = !DILocation(line: 1178, column: 18, scope: !2312)
!2312 = distinct !DILexicalBlock(scope: !2121, file: !32, line: 1178, column: 17)
!2313 = !DILocation(line: 1178, column: 17, scope: !2121)
!2314 = !DILocation(line: 1180, column: 21, scope: !2315)
!2315 = distinct !DILexicalBlock(scope: !2316, file: !32, line: 1180, column: 21)
!2316 = distinct !DILexicalBlock(scope: !2312, file: !32, line: 1179, column: 13)
!2317 = !DILocation(line: 1180, column: 21, scope: !2316)
!2318 = !DILocation(line: 1183, column: 21, scope: !2319)
!2319 = distinct !DILexicalBlock(scope: !2315, file: !32, line: 1181, column: 17)
!2320 = !DILocation(line: 1183, column: 24, scope: !2319)
!2321 = !DILocation(line: 1183, column: 31, scope: !2319)
!2322 = !DILocation(line: 1183, column: 36, scope: !2319)
!2323 = !DILocation(line: 1184, column: 17, scope: !2319)
!2324 = !DILocation(line: 1185, column: 26, scope: !2316)
!2325 = !DILocation(line: 1186, column: 21, scope: !2316)
!2326 = !DILocation(line: 1186, column: 36, scope: !2316)
!2327 = !DILocation(line: 1185, column: 17, scope: !2316)
!2328 = !DILocation(line: 1189, column: 21, scope: !2316)
!2329 = !DILocation(line: 1189, column: 30, scope: !2316)
!2330 = !DILocation(line: 1188, column: 23, scope: !2316)
!2331 = !DILocation(line: 1188, column: 21, scope: !2316)
!2332 = !DILocation(line: 1190, column: 13, scope: !2316)
!2333 = !DILocation(line: 1191, column: 18, scope: !2334)
!2334 = distinct !DILexicalBlock(scope: !2121, file: !32, line: 1191, column: 17)
!2335 = !DILocation(line: 1191, column: 17, scope: !2121)
!2336 = !DILocation(line: 1193, column: 17, scope: !2337)
!2337 = distinct !DILexicalBlock(scope: !2334, file: !32, line: 1192, column: 13)
!2338 = !DILocation(line: 1195, column: 9, scope: !2121)
!2339 = !DILocation(line: 1197, column: 22, scope: !2340)
!2340 = distinct !DILexicalBlock(scope: !2118, file: !32, line: 1196, column: 18)
!2341 = !DILocation(line: 1197, column: 31, scope: !2340)
!2342 = !DILocation(line: 1196, column: 19, scope: !2340)
!2343 = !DILocation(line: 1196, column: 18, scope: !2118)
!2344 = !DILocation(line: 1199, column: 13, scope: !2345)
!2345 = distinct !DILexicalBlock(scope: !2340, file: !32, line: 1198, column: 9)
!2346 = !DILocation(line: 1201, column: 5, scope: !1927)
!2347 = !DILocation(line: 971, column: 32, scope: !1922)
!2348 = !DILocation(line: 971, column: 5, scope: !1922)
!2349 = distinct !{!2349, !1925, !2350, !238}
!2350 = !DILocation(line: 1201, column: 5, scope: !1919)
!2351 = !DILocation(line: 1202, column: 5, scope: !1797)
!2352 = !DILocation(line: 1203, column: 1, scope: !1797)
!2353 = distinct !DISubprogram(name: "conv_pat", scope: !32, file: !32, line: 336, type: !2354, scopeLine: 337, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2354 = !DISubroutineType(types: !2355)
!2355 = !{!15, !7}
!2356 = !DILocalVariable(name: "orig_str", arg: 1, scope: !2353, file: !32, line: 336, type: !7)
!2357 = !DILocation(line: 336, column: 41, scope: !2353)
!2358 = !DILocalVariable(name: "sp", scope: !2353, file: !32, line: 338, type: !8)
!2359 = !DILocation(line: 338, column: 17, scope: !2353)
!2360 = !DILocalVariable(name: "new_buf", scope: !2353, file: !32, line: 339, type: !15)
!2361 = !DILocation(line: 339, column: 11, scope: !2353)
!2362 = !DILocalVariable(name: "cnt", scope: !2353, file: !32, line: 341, type: !25)
!2363 = !DILocation(line: 341, column: 12, scope: !2353)
!2364 = !DILocation(line: 342, column: 15, scope: !2365)
!2365 = distinct !DILexicalBlock(scope: !2353, file: !32, line: 342, column: 5)
!2366 = !DILocation(line: 342, column: 13, scope: !2365)
!2367 = !DILocation(line: 342, column: 10, scope: !2365)
!2368 = !DILocation(line: 342, column: 26, scope: !2369)
!2369 = distinct !DILexicalBlock(scope: !2365, file: !32, line: 342, column: 5)
!2370 = !DILocation(line: 342, column: 25, scope: !2369)
!2371 = !DILocation(line: 342, column: 29, scope: !2369)
!2372 = !DILocation(line: 342, column: 5, scope: !2365)
!2373 = !DILocalVariable(name: "prev_cnt", scope: !2374, file: !32, line: 344, type: !1561)
!2374 = distinct !DILexicalBlock(scope: !2369, file: !32, line: 343, column: 5)
!2375 = !DILocation(line: 344, column: 22, scope: !2374)
!2376 = !DILocation(line: 344, column: 33, scope: !2374)
!2377 = !DILocation(line: 345, column: 13, scope: !2374)
!2378 = !DILocation(line: 345, column: 13, scope: !2379)
!2379 = distinct !DILexicalBlock(scope: !2374, file: !32, line: 345, column: 13)
!2380 = !DILocation(line: 347, column: 17, scope: !2381)
!2381 = distinct !DILexicalBlock(scope: !2379, file: !32, line: 346, column: 9)
!2382 = !DILocation(line: 348, column: 9, scope: !2381)
!2383 = !DILocation(line: 351, column: 13, scope: !2384)
!2384 = distinct !DILexicalBlock(scope: !2379, file: !32, line: 350, column: 9)
!2385 = !DILocation(line: 353, column: 13, scope: !2386)
!2386 = distinct !DILexicalBlock(scope: !2374, file: !32, line: 353, column: 13)
!2387 = !DILocation(line: 353, column: 25, scope: !2386)
!2388 = !DILocation(line: 353, column: 22, scope: !2386)
!2389 = !DILocation(line: 353, column: 13, scope: !2374)
!2390 = !DILocation(line: 355, column: 21, scope: !2391)
!2391 = distinct !DILexicalBlock(scope: !2386, file: !32, line: 354, column: 9)
!2392 = !DILocation(line: 355, column: 13, scope: !2391)
!2393 = !DILocation(line: 357, column: 13, scope: !2391)
!2394 = !DILocation(line: 359, column: 5, scope: !2374)
!2395 = !DILocation(line: 342, column: 38, scope: !2369)
!2396 = !DILocation(line: 342, column: 5, scope: !2369)
!2397 = distinct !{!2397, !2372, !2398, !238}
!2398 = !DILocation(line: 359, column: 5, scope: !2365)
!2399 = !DILocation(line: 360, column: 28, scope: !2400)
!2400 = distinct !DILexicalBlock(scope: !2353, file: !32, line: 360, column: 9)
!2401 = !DILocation(line: 360, column: 21, scope: !2400)
!2402 = !DILocation(line: 360, column: 19, scope: !2400)
!2403 = !DILocation(line: 360, column: 9, scope: !2353)
!2404 = !DILocation(line: 362, column: 17, scope: !2405)
!2405 = distinct !DILexicalBlock(scope: !2400, file: !32, line: 361, column: 5)
!2406 = !DILocation(line: 362, column: 9, scope: !2405)
!2407 = !DILocation(line: 363, column: 9, scope: !2405)
!2408 = !DILocalVariable(name: "dest_ptr", scope: !2353, file: !32, line: 366, type: !15)
!2409 = !DILocation(line: 366, column: 11, scope: !2353)
!2410 = !DILocalVariable(name: "orig", scope: !2353, file: !32, line: 367, type: !8)
!2411 = !DILocation(line: 367, column: 17, scope: !2353)
!2412 = !DILocation(line: 367, column: 24, scope: !2353)
!2413 = !DILocation(line: 368, column: 21, scope: !2414)
!2414 = distinct !DILexicalBlock(scope: !2353, file: !32, line: 368, column: 5)
!2415 = !DILocation(line: 368, column: 19, scope: !2414)
!2416 = !DILocation(line: 368, column: 10, scope: !2414)
!2417 = !DILocation(line: 368, column: 31, scope: !2418)
!2418 = distinct !DILexicalBlock(scope: !2414, file: !32, line: 368, column: 5)
!2419 = !DILocation(line: 368, column: 30, scope: !2418)
!2420 = !DILocation(line: 368, column: 36, scope: !2418)
!2421 = !DILocation(line: 368, column: 5, scope: !2414)
!2422 = !DILocation(line: 370, column: 13, scope: !2423)
!2423 = distinct !DILexicalBlock(scope: !2418, file: !32, line: 369, column: 5)
!2424 = !DILocation(line: 370, column: 13, scope: !2425)
!2425 = distinct !DILexicalBlock(scope: !2423, file: !32, line: 370, column: 13)
!2426 = !DILocation(line: 372, column: 22, scope: !2427)
!2427 = distinct !DILexicalBlock(scope: !2425, file: !32, line: 371, column: 9)
!2428 = !DILocation(line: 372, column: 25, scope: !2427)
!2429 = !DILocation(line: 373, column: 28, scope: !2427)
!2430 = !DILocation(line: 373, column: 27, scope: !2427)
!2431 = !DILocation(line: 373, column: 22, scope: !2427)
!2432 = !DILocation(line: 373, column: 25, scope: !2427)
!2433 = !DILocation(line: 374, column: 42, scope: !2427)
!2434 = !DILocation(line: 374, column: 41, scope: !2427)
!2435 = !DILocation(line: 374, column: 33, scope: !2427)
!2436 = !DILocation(line: 374, column: 27, scope: !2427)
!2437 = !DILocation(line: 374, column: 22, scope: !2427)
!2438 = !DILocation(line: 374, column: 25, scope: !2427)
!2439 = !DILocation(line: 375, column: 22, scope: !2427)
!2440 = !DILocation(line: 375, column: 25, scope: !2427)
!2441 = !DILocation(line: 376, column: 9, scope: !2427)
!2442 = !DILocation(line: 377, column: 18, scope: !2425)
!2443 = !DILocation(line: 377, column: 18, scope: !2444)
!2444 = distinct !DILexicalBlock(scope: !2425, file: !32, line: 377, column: 18)
!2445 = !DILocation(line: 379, column: 22, scope: !2446)
!2446 = distinct !DILexicalBlock(scope: !2444, file: !32, line: 378, column: 9)
!2447 = !DILocation(line: 379, column: 25, scope: !2446)
!2448 = !DILocation(line: 380, column: 28, scope: !2446)
!2449 = !DILocation(line: 380, column: 27, scope: !2446)
!2450 = !DILocation(line: 380, column: 22, scope: !2446)
!2451 = !DILocation(line: 380, column: 25, scope: !2446)
!2452 = !DILocation(line: 381, column: 42, scope: !2446)
!2453 = !DILocation(line: 381, column: 41, scope: !2446)
!2454 = !DILocation(line: 381, column: 33, scope: !2446)
!2455 = !DILocation(line: 381, column: 27, scope: !2446)
!2456 = !DILocation(line: 381, column: 22, scope: !2446)
!2457 = !DILocation(line: 381, column: 25, scope: !2446)
!2458 = !DILocation(line: 382, column: 22, scope: !2446)
!2459 = !DILocation(line: 382, column: 25, scope: !2446)
!2460 = !DILocation(line: 383, column: 9, scope: !2446)
!2461 = !DILocation(line: 386, column: 28, scope: !2462)
!2462 = distinct !DILexicalBlock(scope: !2444, file: !32, line: 385, column: 9)
!2463 = !DILocation(line: 386, column: 27, scope: !2462)
!2464 = !DILocation(line: 386, column: 22, scope: !2462)
!2465 = !DILocation(line: 386, column: 25, scope: !2462)
!2466 = !DILocation(line: 388, column: 5, scope: !2423)
!2467 = !DILocation(line: 368, column: 45, scope: !2418)
!2468 = !DILocation(line: 368, column: 5, scope: !2418)
!2469 = distinct !{!2469, !2421, !2470, !238}
!2470 = !DILocation(line: 388, column: 5, scope: !2414)
!2471 = !DILocation(line: 389, column: 6, scope: !2353)
!2472 = !DILocation(line: 389, column: 15, scope: !2353)
!2473 = !DILocation(line: 390, column: 12, scope: !2353)
!2474 = !DILocation(line: 390, column: 5, scope: !2353)
!2475 = distinct !DISubprogram(name: "top_level__add_file", scope: !32, file: !32, line: 861, type: !2476, scopeLine: 862, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2476 = !DISubroutineType(types: !2477)
!2477 = !{!13, !8}
!2478 = !DILocalVariable(name: "dirpath", arg: 1, scope: !2475, file: !32, line: 861, type: !8)
!2479 = !DILocation(line: 861, column: 44, scope: !2475)
!2480 = !DILocation(line: 863, column: 30, scope: !2475)
!2481 = !DILocation(line: 863, column: 12, scope: !2475)
!2482 = !DILocation(line: 863, column: 5, scope: !2475)
!2483 = distinct !DISubprogram(name: "cond_top_level__FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR", scope: !32, file: !32, line: 876, type: !800, scopeLine: 877, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2484 = !DILocation(line: 878, column: 12, scope: !2483)
!2485 = !DILocation(line: 878, column: 5, scope: !2483)
!2486 = distinct !DISubprogram(name: "cond_top_level__FORTUNE_SYSTEM_OFFENSIVE_FORTUNES_DIR", scope: !32, file: !32, line: 882, type: !800, scopeLine: 883, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2487 = !DILocation(line: 884, column: 12, scope: !2486)
!2488 = !DILocation(line: 884, column: 5, scope: !2486)
!2489 = distinct !DISubprogram(name: "add_file", scope: !32, file: !32, line: 587, type: !2490, scopeLine: 589, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2490 = !DISubroutineType(types: !2491)
!2491 = !{!13, !13, !8, !8, !2492, !2492, !47}
!2492 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !47, size: 32)
!2493 = !DILocalVariable(name: "percent", arg: 1, scope: !2489, file: !32, line: 587, type: !13)
!2494 = !DILocation(line: 587, column: 25, scope: !2489)
!2495 = !DILocalVariable(name: "file", arg: 2, scope: !2489, file: !32, line: 587, type: !8)
!2496 = !DILocation(line: 587, column: 46, scope: !2489)
!2497 = !DILocalVariable(name: "dir", arg: 3, scope: !2489, file: !32, line: 587, type: !8)
!2498 = !DILocation(line: 587, column: 64, scope: !2489)
!2499 = !DILocalVariable(name: "head", arg: 4, scope: !2489, file: !32, line: 588, type: !2492)
!2500 = !DILocation(line: 588, column: 16, scope: !2489)
!2501 = !DILocalVariable(name: "tail", arg: 5, scope: !2489, file: !32, line: 588, type: !2492)
!2502 = !DILocation(line: 588, column: 33, scope: !2489)
!2503 = !DILocalVariable(name: "parent", arg: 6, scope: !2489, file: !32, line: 588, type: !47)
!2504 = !DILocation(line: 588, column: 49, scope: !2489)
!2505 = !DILocalVariable(name: "fp", scope: !2489, file: !32, line: 590, type: !47)
!2506 = !DILocation(line: 590, column: 15, scope: !2489)
!2507 = !DILocalVariable(name: "fd", scope: !2489, file: !32, line: 591, type: !13)
!2508 = !DILocation(line: 591, column: 9, scope: !2489)
!2509 = !DILocalVariable(name: "path", scope: !2489, file: !32, line: 592, type: !15)
!2510 = !DILocation(line: 592, column: 11, scope: !2489)
!2511 = !DILocalVariable(name: "statbuf", scope: !2489, file: !32, line: 593, type: !2512)
!2512 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "stat", file: !2513, line: 15, size: 1152, elements: !2514)
!2513 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__struct_stat.h", directory: "")
!2514 = !{!2515, !2518, !2521, !2524, !2527, !2530, !2533, !2534, !2535, !2536, !2539, !2542, !2548, !2549, !2550}
!2515 = !DIDerivedType(tag: DW_TAG_member, name: "st_dev", scope: !2512, file: !2513, line: 16, baseType: !2516, size: 64)
!2516 = !DIDerivedType(tag: DW_TAG_typedef, name: "dev_t", file: !2517, line: 5, baseType: !1058)
!2517 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_dev_t.h", directory: "")
!2518 = !DIDerivedType(tag: DW_TAG_member, name: "st_ino", scope: !2512, file: !2513, line: 17, baseType: !2519, size: 64, offset: 64)
!2519 = !DIDerivedType(tag: DW_TAG_typedef, name: "ino_t", file: !2520, line: 5, baseType: !1058)
!2520 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_ino_t.h", directory: "")
!2521 = !DIDerivedType(tag: DW_TAG_member, name: "st_nlink", scope: !2512, file: !2513, line: 18, baseType: !2522, size: 64, offset: 128)
!2522 = !DIDerivedType(tag: DW_TAG_typedef, name: "nlink_t", file: !2523, line: 5, baseType: !1058)
!2523 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_nlink_t.h", directory: "")
!2524 = !DIDerivedType(tag: DW_TAG_member, name: "st_mode", scope: !2512, file: !2513, line: 20, baseType: !2525, size: 32, offset: 192)
!2525 = !DIDerivedType(tag: DW_TAG_typedef, name: "mode_t", file: !2526, line: 4, baseType: !5)
!2526 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_mode_t.h", directory: "")
!2527 = !DIDerivedType(tag: DW_TAG_member, name: "st_uid", scope: !2512, file: !2513, line: 21, baseType: !2528, size: 32, offset: 224)
!2528 = !DIDerivedType(tag: DW_TAG_typedef, name: "uid_t", file: !2529, line: 4, baseType: !5)
!2529 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_uid_t.h", directory: "")
!2530 = !DIDerivedType(tag: DW_TAG_member, name: "st_gid", scope: !2512, file: !2513, line: 22, baseType: !2531, size: 32, offset: 256)
!2531 = !DIDerivedType(tag: DW_TAG_typedef, name: "gid_t", file: !2532, line: 4, baseType: !5)
!2532 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_gid_t.h", directory: "")
!2533 = !DIDerivedType(tag: DW_TAG_member, name: "__pad0", scope: !2512, file: !2513, line: 23, baseType: !5, size: 32, offset: 288)
!2534 = !DIDerivedType(tag: DW_TAG_member, name: "st_rdev", scope: !2512, file: !2513, line: 24, baseType: !2516, size: 64, offset: 320)
!2535 = !DIDerivedType(tag: DW_TAG_member, name: "st_size", scope: !2512, file: !2513, line: 25, baseType: !23, size: 64, offset: 384)
!2536 = !DIDerivedType(tag: DW_TAG_member, name: "st_blksize", scope: !2512, file: !2513, line: 26, baseType: !2537, size: 32, offset: 448)
!2537 = !DIDerivedType(tag: DW_TAG_typedef, name: "blksize_t", file: !2538, line: 4, baseType: !28)
!2538 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_blksize_t.h", directory: "")
!2539 = !DIDerivedType(tag: DW_TAG_member, name: "st_blocks", scope: !2512, file: !2513, line: 27, baseType: !2540, size: 64, offset: 512)
!2540 = !DIDerivedType(tag: DW_TAG_typedef, name: "blkcnt_t", file: !2541, line: 5, baseType: !21)
!2541 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_blkcnt_t.h", directory: "")
!2542 = !DIDerivedType(tag: DW_TAG_member, name: "st_atim", scope: !2512, file: !2513, line: 29, baseType: !2543, size: 128, offset: 576)
!2543 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "timespec", file: !2544, line: 7, size: 128, elements: !2545)
!2544 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__struct_timespec.h", directory: "")
!2545 = !{!2546, !2547}
!2546 = !DIDerivedType(tag: DW_TAG_member, name: "tv_sec", scope: !2543, file: !2544, line: 8, baseType: !19, size: 64)
!2547 = !DIDerivedType(tag: DW_TAG_member, name: "tv_nsec", scope: !2543, file: !2544, line: 9, baseType: !28, size: 32, offset: 64)
!2548 = !DIDerivedType(tag: DW_TAG_member, name: "st_mtim", scope: !2512, file: !2513, line: 30, baseType: !2543, size: 128, offset: 704)
!2549 = !DIDerivedType(tag: DW_TAG_member, name: "st_ctim", scope: !2512, file: !2513, line: 31, baseType: !2543, size: 128, offset: 832)
!2550 = !DIDerivedType(tag: DW_TAG_member, name: "__reserved", scope: !2512, file: !2513, line: 32, baseType: !2551, size: 192, offset: 960)
!2551 = !DICompositeType(tag: DW_TAG_array_type, baseType: !21, size: 192, elements: !1891)
!2552 = !DILocation(line: 593, column: 17, scope: !2489)
!2553 = !DILocation(line: 595, column: 10, scope: !2554)
!2554 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 595, column: 9)
!2555 = !DILocation(line: 595, column: 9, scope: !2489)
!2556 = !DILocation(line: 597, column: 23, scope: !2557)
!2557 = distinct !DILexicalBlock(scope: !2554, file: !32, line: 596, column: 5)
!2558 = !DILocation(line: 597, column: 16, scope: !2557)
!2559 = !DILocation(line: 597, column: 14, scope: !2557)
!2560 = !DILocation(line: 598, column: 5, scope: !2557)
!2561 = !DILocalVariable(name: "do_len", scope: !2562, file: !32, line: 601, type: !1561)
!2562 = distinct !DILexicalBlock(scope: !2554, file: !32, line: 600, column: 5)
!2563 = !DILocation(line: 601, column: 22, scope: !2562)
!2564 = !DILocation(line: 602, column: 21, scope: !2562)
!2565 = !DILocation(line: 602, column: 14, scope: !2562)
!2566 = !DILocation(line: 602, column: 35, scope: !2562)
!2567 = !DILocation(line: 602, column: 28, scope: !2562)
!2568 = !DILocation(line: 602, column: 26, scope: !2562)
!2569 = !DILocation(line: 602, column: 41, scope: !2562)
!2570 = !DILocation(line: 603, column: 26, scope: !2562)
!2571 = !DILocation(line: 603, column: 33, scope: !2562)
!2572 = !DILocation(line: 603, column: 16, scope: !2562)
!2573 = !DILocation(line: 603, column: 14, scope: !2562)
!2574 = !DILocation(line: 604, column: 18, scope: !2562)
!2575 = !DILocation(line: 604, column: 24, scope: !2562)
!2576 = !DILocation(line: 604, column: 41, scope: !2562)
!2577 = !DILocation(line: 604, column: 46, scope: !2562)
!2578 = !DILocation(line: 604, column: 9, scope: !2562)
!2579 = !DILocation(line: 606, column: 10, scope: !2580)
!2580 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 606, column: 9)
!2581 = !DILocation(line: 606, column: 9, scope: !2580)
!2582 = !DILocation(line: 606, column: 15, scope: !2580)
!2583 = !DILocation(line: 606, column: 22, scope: !2580)
!2584 = !DILocation(line: 607, column: 22, scope: !2580)
!2585 = !DILocation(line: 607, column: 10, scope: !2580)
!2586 = !DILocation(line: 606, column: 9, scope: !2489)
!2587 = !DILocation(line: 609, column: 14, scope: !2588)
!2588 = distinct !DILexicalBlock(scope: !2580, file: !32, line: 608, column: 5)
!2589 = !DILocation(line: 609, column: 9, scope: !2588)
!2590 = !DILocation(line: 610, column: 9, scope: !2588)
!2591 = !DILocalVariable(name: "isdir", scope: !2489, file: !32, line: 612, type: !1939)
!2592 = !DILocation(line: 612, column: 15, scope: !2489)
!2593 = !DILocation(line: 612, column: 30, scope: !2489)
!2594 = !DILocation(line: 612, column: 23, scope: !2489)
!2595 = !DILocation(line: 613, column: 10, scope: !2596)
!2596 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 613, column: 9)
!2597 = !DILocation(line: 613, column: 16, scope: !2596)
!2598 = !DILocation(line: 613, column: 20, scope: !2596)
!2599 = !DILocation(line: 613, column: 23, scope: !2596)
!2600 = !DILocation(line: 613, column: 31, scope: !2596)
!2601 = !DILocation(line: 613, column: 35, scope: !2596)
!2602 = !DILocation(line: 613, column: 41, scope: !2596)
!2603 = !DILocation(line: 613, column: 9, scope: !2489)
!2604 = !DILocation(line: 615, column: 14, scope: !2605)
!2605 = distinct !DILexicalBlock(scope: !2596, file: !32, line: 614, column: 5)
!2606 = !DILocation(line: 615, column: 9, scope: !2605)
!2607 = !DILocation(line: 616, column: 9, scope: !2605)
!2608 = !DILocation(line: 624, column: 30, scope: !2609)
!2609 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 620, column: 9)
!2610 = !DILocation(line: 624, column: 20, scope: !2609)
!2611 = !DILocation(line: 624, column: 18, scope: !2609)
!2612 = !DILocation(line: 624, column: 37, scope: !2609)
!2613 = !DILocation(line: 624, column: 43, scope: !2609)
!2614 = !DILocation(line: 625, column: 27, scope: !2609)
!2615 = !DILocation(line: 625, column: 10, scope: !2609)
!2616 = !DILocation(line: 620, column: 9, scope: !2489)
!2617 = !DILocalVariable(name: "found", scope: !2618, file: !32, line: 629, type: !64)
!2618 = distinct !DILexicalBlock(scope: !2609, file: !32, line: 626, column: 5)
!2619 = !DILocation(line: 629, column: 14, scope: !2618)
!2620 = !DILocation(line: 630, column: 14, scope: !2621)
!2621 = distinct !DILexicalBlock(scope: !2618, file: !32, line: 630, column: 13)
!2622 = !DILocation(line: 630, column: 20, scope: !2621)
!2623 = !DILocation(line: 630, column: 24, scope: !2621)
!2624 = !DILocation(line: 630, column: 31, scope: !2621)
!2625 = !DILocation(line: 630, column: 35, scope: !2621)
!2626 = !DILocation(line: 630, column: 13, scope: !2618)
!2627 = !DILocation(line: 632, column: 17, scope: !2628)
!2628 = distinct !DILexicalBlock(scope: !2629, file: !32, line: 632, column: 17)
!2629 = distinct !DILexicalBlock(scope: !2621, file: !32, line: 631, column: 9)
!2630 = !DILocation(line: 632, column: 17, scope: !2629)
!2631 = !DILocalVariable(name: "llang", scope: !2632, file: !32, line: 634, type: !1813)
!2632 = distinct !DILexicalBlock(scope: !2628, file: !32, line: 633, column: 13)
!2633 = !DILocation(line: 634, column: 22, scope: !2632)
!2634 = !DILocalVariable(name: "langdir", scope: !2632, file: !32, line: 635, type: !98)
!2635 = !DILocation(line: 635, column: 22, scope: !2632)
!2636 = !DILocalVariable(name: "ret", scope: !2632, file: !32, line: 636, type: !13)
!2637 = !DILocation(line: 636, column: 21, scope: !2632)
!2638 = !DILocation(line: 638, column: 25, scope: !2632)
!2639 = !DILocation(line: 638, column: 32, scope: !2632)
!2640 = !DILocation(line: 638, column: 17, scope: !2632)
!2641 = !DILocation(line: 639, column: 17, scope: !2632)
!2642 = !DILocation(line: 639, column: 42, scope: !2632)
!2643 = !DILocalVariable(name: "lang", scope: !2632, file: !32, line: 640, type: !15)
!2644 = !DILocation(line: 640, column: 23, scope: !2632)
!2645 = !DILocation(line: 640, column: 30, scope: !2632)
!2646 = !DILocation(line: 643, column: 17, scope: !2632)
!2647 = !DILocation(line: 643, column: 25, scope: !2632)
!2648 = !DILocation(line: 643, column: 29, scope: !2632)
!2649 = !DILocation(line: 643, column: 32, scope: !2632)
!2650 = !DILocation(line: 643, column: 37, scope: !2632)
!2651 = !DILocation(line: 643, column: 42, scope: !2632)
!2652 = !DILocation(line: 643, column: 41, scope: !2632)
!2653 = !DILocation(line: 643, column: 40, scope: !2632)
!2654 = !DILocation(line: 0, scope: !2632)
!2655 = !DILocalVariable(name: "p", scope: !2656, file: !32, line: 645, type: !15)
!2656 = distinct !DILexicalBlock(scope: !2632, file: !32, line: 644, column: 17)
!2657 = !DILocation(line: 645, column: 27, scope: !2656)
!2658 = !DILocation(line: 645, column: 38, scope: !2656)
!2659 = !DILocation(line: 645, column: 31, scope: !2656)
!2660 = !DILocation(line: 646, column: 25, scope: !2661)
!2661 = distinct !DILexicalBlock(scope: !2656, file: !32, line: 646, column: 25)
!2662 = !DILocation(line: 646, column: 25, scope: !2656)
!2663 = !DILocation(line: 648, column: 27, scope: !2664)
!2664 = distinct !DILexicalBlock(scope: !2661, file: !32, line: 647, column: 21)
!2665 = !DILocation(line: 648, column: 30, scope: !2664)
!2666 = !DILocation(line: 649, column: 21, scope: !2664)
!2667 = !DILocation(line: 650, column: 30, scope: !2656)
!2668 = !DILocation(line: 651, column: 66, scope: !2656)
!2669 = !DILocation(line: 650, column: 21, scope: !2656)
!2670 = !DILocation(line: 653, column: 33, scope: !2671)
!2671 = distinct !DILexicalBlock(scope: !2656, file: !32, line: 653, column: 25)
!2672 = !DILocation(line: 653, column: 39, scope: !2671)
!2673 = !DILocation(line: 653, column: 25, scope: !2671)
!2674 = !DILocation(line: 653, column: 48, scope: !2671)
!2675 = !DILocation(line: 653, column: 25, scope: !2656)
!2676 = !DILocation(line: 655, column: 29, scope: !2677)
!2677 = distinct !DILexicalBlock(scope: !2671, file: !32, line: 654, column: 21)
!2678 = !DILocation(line: 656, column: 21, scope: !2677)
!2679 = !DILocation(line: 657, column: 38, scope: !2680)
!2680 = distinct !DILexicalBlock(scope: !2671, file: !32, line: 657, column: 30)
!2681 = !DILocation(line: 657, column: 44, scope: !2680)
!2682 = !DILocation(line: 658, column: 34, scope: !2680)
!2683 = !DILocation(line: 659, column: 79, scope: !2680)
!2684 = !DILocation(line: 657, column: 30, scope: !2680)
!2685 = !DILocation(line: 660, column: 41, scope: !2680)
!2686 = !DILocation(line: 657, column: 30, scope: !2671)
!2687 = !DILocation(line: 662, column: 29, scope: !2688)
!2688 = distinct !DILexicalBlock(scope: !2680, file: !32, line: 661, column: 21)
!2689 = !DILocation(line: 663, column: 21, scope: !2688)
!2690 = !DILocation(line: 664, column: 28, scope: !2656)
!2691 = !DILocation(line: 664, column: 26, scope: !2656)
!2692 = distinct !{!2692, !2646, !2693, !238}
!2693 = !DILocation(line: 665, column: 17, scope: !2632)
!2694 = !DILocation(line: 666, column: 22, scope: !2695)
!2695 = distinct !DILexicalBlock(scope: !2632, file: !32, line: 666, column: 21)
!2696 = !DILocation(line: 666, column: 21, scope: !2632)
!2697 = !DILocation(line: 668, column: 28, scope: !2698)
!2698 = distinct !DILexicalBlock(scope: !2695, file: !32, line: 667, column: 17)
!2699 = !DILocation(line: 668, column: 21, scope: !2698)
!2700 = !DILocation(line: 669, column: 17, scope: !2698)
!2701 = !DILocation(line: 670, column: 13, scope: !2632)
!2702 = !DILocation(line: 673, column: 24, scope: !2703)
!2703 = distinct !DILexicalBlock(scope: !2628, file: !32, line: 672, column: 13)
!2704 = !DILocation(line: 673, column: 17, scope: !2703)
!2705 = !DILocation(line: 675, column: 9, scope: !2629)
!2706 = !DILocation(line: 677, column: 14, scope: !2618)
!2707 = !DILocation(line: 677, column: 9, scope: !2618)
!2708 = !DILocation(line: 678, column: 14, scope: !2618)
!2709 = !DILocation(line: 679, column: 16, scope: !2618)
!2710 = !DILocation(line: 679, column: 9, scope: !2618)
!2711 = !DILocation(line: 684, column: 10, scope: !2489)
!2712 = !DILocation(line: 684, column: 8, scope: !2489)
!2713 = !DILocation(line: 685, column: 14, scope: !2489)
!2714 = !DILocation(line: 685, column: 5, scope: !2489)
!2715 = !DILocation(line: 685, column: 9, scope: !2489)
!2716 = !DILocation(line: 685, column: 12, scope: !2489)
!2717 = !DILocation(line: 686, column: 19, scope: !2489)
!2718 = !DILocation(line: 686, column: 5, scope: !2489)
!2719 = !DILocation(line: 686, column: 9, scope: !2489)
!2720 = !DILocation(line: 686, column: 17, scope: !2489)
!2721 = !DILocation(line: 688, column: 23, scope: !2489)
!2722 = !DILocation(line: 688, column: 16, scope: !2489)
!2723 = !DILocation(line: 688, column: 5, scope: !2489)
!2724 = !DILocation(line: 688, column: 9, scope: !2489)
!2725 = !DILocation(line: 688, column: 14, scope: !2489)
!2726 = !DILocation(line: 689, column: 23, scope: !2489)
!2727 = !DILocation(line: 689, column: 16, scope: !2489)
!2728 = !DILocation(line: 689, column: 5, scope: !2489)
!2729 = !DILocation(line: 689, column: 9, scope: !2489)
!2730 = !DILocation(line: 689, column: 14, scope: !2489)
!2731 = !DILocation(line: 692, column: 5, scope: !2489)
!2732 = !DILocation(line: 692, column: 9, scope: !2489)
!2733 = !DILocation(line: 692, column: 22, scope: !2489)
!2734 = !DILocalVariable(name: "do_len", scope: !2489, file: !32, line: 693, type: !1561)
!2735 = !DILocation(line: 693, column: 18, scope: !2489)
!2736 = !DILocation(line: 693, column: 35, scope: !2489)
!2737 = !DILocation(line: 693, column: 28, scope: !2489)
!2738 = !DILocation(line: 693, column: 41, scope: !2489)
!2739 = !DILocalVariable(name: "testpath", scope: !2489, file: !32, line: 694, type: !15)
!2740 = !DILocation(line: 694, column: 11, scope: !2489)
!2741 = !DILocation(line: 694, column: 32, scope: !2489)
!2742 = !DILocation(line: 694, column: 39, scope: !2489)
!2743 = !DILocation(line: 694, column: 22, scope: !2489)
!2744 = !DILocation(line: 695, column: 14, scope: !2489)
!2745 = !DILocation(line: 695, column: 24, scope: !2489)
!2746 = !DILocation(line: 695, column: 41, scope: !2489)
!2747 = !DILocation(line: 695, column: 5, scope: !2489)
!2748 = !DILocation(line: 697, column: 14, scope: !2749)
!2749 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 697, column: 9)
!2750 = !DILocation(line: 697, column: 9, scope: !2749)
!2751 = !DILocation(line: 697, column: 34, scope: !2749)
!2752 = !DILocation(line: 697, column: 9, scope: !2489)
!2753 = !DILocation(line: 699, column: 9, scope: !2754)
!2754 = distinct !DILexicalBlock(scope: !2749, file: !32, line: 698, column: 5)
!2755 = !DILocation(line: 699, column: 13, scope: !2754)
!2756 = !DILocation(line: 699, column: 26, scope: !2754)
!2757 = !DILocation(line: 700, column: 5, scope: !2754)
!2758 = !DILocation(line: 702, column: 10, scope: !2489)
!2759 = !DILocation(line: 702, column: 5, scope: !2489)
!2760 = !DILocation(line: 703, column: 14, scope: !2489)
!2761 = !DILocation(line: 706, column: 18, scope: !2489)
!2762 = !DILocation(line: 706, column: 5, scope: !2489)
!2763 = !DILocation(line: 706, column: 9, scope: !2489)
!2764 = !DILocation(line: 706, column: 16, scope: !2489)
!2765 = !DILocation(line: 708, column: 10, scope: !2766)
!2766 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 708, column: 9)
!2767 = !DILocation(line: 708, column: 16, scope: !2766)
!2768 = !DILocation(line: 708, column: 28, scope: !2766)
!2769 = !DILocation(line: 708, column: 20, scope: !2766)
!2770 = !DILocation(line: 708, column: 33, scope: !2766)
!2771 = !DILocation(line: 708, column: 38, scope: !2766)
!2772 = !DILocation(line: 708, column: 44, scope: !2766)
!2773 = !DILocation(line: 708, column: 60, scope: !2766)
!2774 = !DILocation(line: 708, column: 67, scope: !2766)
!2775 = !DILocation(line: 708, column: 71, scope: !2766)
!2776 = !DILocation(line: 708, column: 48, scope: !2766)
!2777 = !DILocation(line: 708, column: 9, scope: !2489)
!2778 = !DILocation(line: 710, column: 14, scope: !2779)
!2779 = distinct !DILexicalBlock(scope: !2780, file: !32, line: 710, column: 13)
!2780 = distinct !DILexicalBlock(scope: !2766, file: !32, line: 709, column: 5)
!2781 = !DILocation(line: 710, column: 13, scope: !2780)
!2782 = !DILocation(line: 713, column: 17, scope: !2783)
!2783 = distinct !DILexicalBlock(scope: !2779, file: !32, line: 711, column: 9)
!2784 = !DILocation(line: 713, column: 73, scope: !2783)
!2785 = !DILocation(line: 712, column: 13, scope: !2783)
!2786 = !DILocation(line: 714, column: 9, scope: !2783)
!2787 = !DILocation(line: 715, column: 14, scope: !2780)
!2788 = !DILocation(line: 715, column: 9, scope: !2780)
!2789 = !DILocation(line: 716, column: 14, scope: !2780)
!2790 = !DILocation(line: 717, column: 14, scope: !2780)
!2791 = !DILocation(line: 717, column: 18, scope: !2780)
!2792 = !DILocation(line: 717, column: 9, scope: !2780)
!2793 = !DILocation(line: 718, column: 14, scope: !2780)
!2794 = !DILocation(line: 718, column: 18, scope: !2780)
!2795 = !DILocation(line: 718, column: 9, scope: !2780)
!2796 = !DILocation(line: 719, column: 14, scope: !2780)
!2797 = !DILocation(line: 719, column: 18, scope: !2780)
!2798 = !DILocation(line: 719, column: 9, scope: !2780)
!2799 = !DILocation(line: 720, column: 14, scope: !2780)
!2800 = !DILocation(line: 720, column: 18, scope: !2780)
!2801 = !DILocation(line: 720, column: 9, scope: !2780)
!2802 = !DILocation(line: 721, column: 13, scope: !2803)
!2803 = distinct !DILexicalBlock(scope: !2780, file: !32, line: 721, column: 13)
!2804 = !DILocation(line: 721, column: 17, scope: !2803)
!2805 = !DILocation(line: 721, column: 20, scope: !2803)
!2806 = !DILocation(line: 721, column: 13, scope: !2780)
!2807 = !DILocation(line: 723, column: 19, scope: !2808)
!2808 = distinct !DILexicalBlock(scope: !2803, file: !32, line: 722, column: 9)
!2809 = !DILocation(line: 723, column: 23, scope: !2808)
!2810 = !DILocation(line: 723, column: 13, scope: !2808)
!2811 = !DILocation(line: 724, column: 9, scope: !2808)
!2812 = !DILocation(line: 725, column: 14, scope: !2780)
!2813 = !DILocation(line: 725, column: 9, scope: !2780)
!2814 = !DILocation(line: 726, column: 9, scope: !2780)
!2815 = !DILocation(line: 732, column: 9, scope: !2816)
!2816 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 732, column: 9)
!2817 = !DILocation(line: 732, column: 15, scope: !2816)
!2818 = !DILocation(line: 732, column: 18, scope: !2816)
!2819 = !DILocation(line: 732, column: 22, scope: !2816)
!2820 = !DILocation(line: 732, column: 35, scope: !2816)
!2821 = !DILocation(line: 732, column: 9, scope: !2489)
!2822 = !DILocation(line: 734, column: 14, scope: !2823)
!2823 = distinct !DILexicalBlock(scope: !2816, file: !32, line: 733, column: 5)
!2824 = !DILocation(line: 734, column: 9, scope: !2823)
!2825 = !DILocation(line: 735, column: 14, scope: !2823)
!2826 = !DILocation(line: 736, column: 14, scope: !2823)
!2827 = !DILocation(line: 736, column: 18, scope: !2823)
!2828 = !DILocation(line: 736, column: 9, scope: !2823)
!2829 = !DILocation(line: 737, column: 14, scope: !2823)
!2830 = !DILocation(line: 737, column: 18, scope: !2823)
!2831 = !DILocation(line: 737, column: 9, scope: !2823)
!2832 = !DILocation(line: 738, column: 14, scope: !2823)
!2833 = !DILocation(line: 738, column: 18, scope: !2823)
!2834 = !DILocation(line: 738, column: 9, scope: !2823)
!2835 = !DILocation(line: 739, column: 14, scope: !2823)
!2836 = !DILocation(line: 739, column: 18, scope: !2823)
!2837 = !DILocation(line: 739, column: 9, scope: !2823)
!2838 = !DILocation(line: 740, column: 13, scope: !2839)
!2839 = distinct !DILexicalBlock(scope: !2823, file: !32, line: 740, column: 13)
!2840 = !DILocation(line: 740, column: 17, scope: !2839)
!2841 = !DILocation(line: 740, column: 20, scope: !2839)
!2842 = !DILocation(line: 740, column: 13, scope: !2823)
!2843 = !DILocation(line: 742, column: 19, scope: !2844)
!2844 = distinct !DILexicalBlock(scope: !2839, file: !32, line: 741, column: 9)
!2845 = !DILocation(line: 742, column: 23, scope: !2844)
!2846 = !DILocation(line: 742, column: 13, scope: !2844)
!2847 = !DILocation(line: 743, column: 9, scope: !2844)
!2848 = !DILocation(line: 744, column: 14, scope: !2823)
!2849 = !DILocation(line: 744, column: 9, scope: !2823)
!2850 = !DILocation(line: 745, column: 9, scope: !2823)
!2851 = !DILocation(line: 749, column: 12, scope: !2852)
!2852 = distinct !DILexicalBlock(scope: !2489, file: !32, line: 749, column: 9)
!2853 = !DILocation(line: 749, column: 11, scope: !2852)
!2854 = !DILocation(line: 749, column: 10, scope: !2852)
!2855 = !DILocation(line: 749, column: 9, scope: !2489)
!2856 = !DILocation(line: 751, column: 25, scope: !2857)
!2857 = distinct !DILexicalBlock(scope: !2852, file: !32, line: 750, column: 5)
!2858 = !DILocation(line: 751, column: 18, scope: !2857)
!2859 = !DILocation(line: 751, column: 23, scope: !2857)
!2860 = !DILocation(line: 751, column: 10, scope: !2857)
!2861 = !DILocation(line: 751, column: 15, scope: !2857)
!2862 = !DILocation(line: 752, column: 5, scope: !2857)
!2863 = !DILocation(line: 753, column: 14, scope: !2864)
!2864 = distinct !DILexicalBlock(scope: !2852, file: !32, line: 753, column: 14)
!2865 = !DILocation(line: 753, column: 18, scope: !2864)
!2866 = !DILocation(line: 753, column: 26, scope: !2864)
!2867 = !DILocation(line: 753, column: 14, scope: !2852)
!2868 = !DILocation(line: 755, column: 25, scope: !2869)
!2869 = distinct !DILexicalBlock(scope: !2864, file: !32, line: 754, column: 5)
!2870 = !DILocation(line: 755, column: 11, scope: !2869)
!2871 = !DILocation(line: 755, column: 10, scope: !2869)
!2872 = !DILocation(line: 755, column: 18, scope: !2869)
!2873 = !DILocation(line: 755, column: 23, scope: !2869)
!2874 = !DILocation(line: 756, column: 21, scope: !2869)
!2875 = !DILocation(line: 756, column: 20, scope: !2869)
!2876 = !DILocation(line: 756, column: 9, scope: !2869)
!2877 = !DILocation(line: 756, column: 13, scope: !2869)
!2878 = !DILocation(line: 756, column: 18, scope: !2869)
!2879 = !DILocation(line: 757, column: 17, scope: !2869)
!2880 = !DILocation(line: 757, column: 10, scope: !2869)
!2881 = !DILocation(line: 757, column: 15, scope: !2869)
!2882 = !DILocation(line: 758, column: 5, scope: !2869)
!2883 = !DILocation(line: 761, column: 25, scope: !2884)
!2884 = distinct !DILexicalBlock(scope: !2864, file: !32, line: 760, column: 5)
!2885 = !DILocation(line: 761, column: 11, scope: !2884)
!2886 = !DILocation(line: 761, column: 10, scope: !2884)
!2887 = !DILocation(line: 761, column: 18, scope: !2884)
!2888 = !DILocation(line: 761, column: 23, scope: !2884)
!2889 = !DILocation(line: 762, column: 21, scope: !2884)
!2890 = !DILocation(line: 762, column: 20, scope: !2884)
!2891 = !DILocation(line: 762, column: 9, scope: !2884)
!2892 = !DILocation(line: 762, column: 13, scope: !2884)
!2893 = !DILocation(line: 762, column: 18, scope: !2884)
!2894 = !DILocation(line: 763, column: 17, scope: !2884)
!2895 = !DILocation(line: 763, column: 10, scope: !2884)
!2896 = !DILocation(line: 763, column: 15, scope: !2884)
!2897 = !DILocation(line: 766, column: 10, scope: !2489)
!2898 = !DILocation(line: 766, column: 5, scope: !2489)
!2899 = !DILocation(line: 767, column: 10, scope: !2489)
!2900 = !DILocation(line: 769, column: 5, scope: !2489)
!2901 = !DILocation(line: 770, column: 1, scope: !2489)
!2902 = distinct !DISubprogram(name: "top_level_FORTUNEMOD_LOCAL_INOFFENSIVE_FORTUNES_DIR", scope: !32, file: !32, line: 888, type: !800, scopeLine: 889, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2903 = !DILocation(line: 890, column: 13, scope: !2902)
!2904 = !DILocation(line: 891, column: 13, scope: !2902)
!2905 = !DILocation(line: 890, column: 73, scope: !2902)
!2906 = !DILocation(line: 890, column: 5, scope: !2902)
!2907 = distinct !DISubprogram(name: "new_fp", scope: !32, file: !32, line: 414, type: !2908, scopeLine: 415, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2908 = !DISubroutineType(types: !2909)
!2909 = !{!47}
!2910 = !DILocalVariable(name: "fp", scope: !2907, file: !32, line: 416, type: !1012)
!2911 = !DILocation(line: 416, column: 21, scope: !2907)
!2912 = !DILocation(line: 416, column: 26, scope: !2907)
!2913 = !DILocation(line: 417, column: 5, scope: !2907)
!2914 = !DILocation(line: 417, column: 9, scope: !2907)
!2915 = !DILocation(line: 417, column: 15, scope: !2907)
!2916 = !DILocation(line: 418, column: 5, scope: !2907)
!2917 = !DILocation(line: 418, column: 9, scope: !2907)
!2918 = !DILocation(line: 418, column: 13, scope: !2907)
!2919 = !DILocation(line: 419, column: 5, scope: !2907)
!2920 = !DILocation(line: 419, column: 9, scope: !2907)
!2921 = !DILocation(line: 419, column: 13, scope: !2907)
!2922 = !DILocation(line: 420, column: 5, scope: !2907)
!2923 = !DILocation(line: 420, column: 9, scope: !2907)
!2924 = !DILocation(line: 420, column: 12, scope: !2907)
!2925 = !DILocation(line: 421, column: 5, scope: !2907)
!2926 = !DILocation(line: 421, column: 9, scope: !2907)
!2927 = !DILocation(line: 421, column: 17, scope: !2907)
!2928 = !DILocation(line: 422, column: 5, scope: !2907)
!2929 = !DILocation(line: 422, column: 9, scope: !2907)
!2930 = !DILocation(line: 422, column: 18, scope: !2907)
!2931 = !DILocation(line: 423, column: 5, scope: !2907)
!2932 = !DILocation(line: 423, column: 9, scope: !2907)
!2933 = !DILocation(line: 423, column: 13, scope: !2907)
!2934 = !DILocation(line: 423, column: 25, scope: !2907)
!2935 = !DILocation(line: 424, column: 5, scope: !2907)
!2936 = !DILocation(line: 424, column: 9, scope: !2907)
!2937 = !DILocation(line: 424, column: 13, scope: !2907)
!2938 = !DILocation(line: 424, column: 24, scope: !2907)
!2939 = !DILocation(line: 425, column: 5, scope: !2907)
!2940 = !DILocation(line: 425, column: 9, scope: !2907)
!2941 = !DILocation(line: 425, column: 13, scope: !2907)
!2942 = !DILocation(line: 425, column: 25, scope: !2907)
!2943 = !DILocation(line: 426, column: 5, scope: !2907)
!2944 = !DILocation(line: 426, column: 9, scope: !2907)
!2945 = !DILocation(line: 426, column: 13, scope: !2907)
!2946 = !DILocation(line: 426, column: 26, scope: !2907)
!2947 = !DILocation(line: 427, column: 5, scope: !2907)
!2948 = !DILocation(line: 427, column: 9, scope: !2907)
!2949 = !DILocation(line: 427, column: 13, scope: !2907)
!2950 = !DILocation(line: 427, column: 23, scope: !2907)
!2951 = !DILocation(line: 428, column: 5, scope: !2907)
!2952 = !DILocation(line: 428, column: 9, scope: !2907)
!2953 = !DILocation(line: 428, column: 13, scope: !2907)
!2954 = !DILocation(line: 428, column: 22, scope: !2907)
!2955 = !DILocation(line: 429, column: 5, scope: !2907)
!2956 = !DILocation(line: 429, column: 9, scope: !2907)
!2957 = !DILocation(line: 429, column: 13, scope: !2907)
!2958 = !DILocation(line: 429, column: 22, scope: !2907)
!2959 = !DILocation(line: 430, column: 5, scope: !2907)
!2960 = !DILocation(line: 430, column: 9, scope: !2907)
!2961 = !DILocation(line: 430, column: 13, scope: !2907)
!2962 = !DILocation(line: 430, column: 22, scope: !2907)
!2963 = !DILocation(line: 431, column: 5, scope: !2907)
!2964 = !DILocation(line: 431, column: 9, scope: !2907)
!2965 = !DILocation(line: 431, column: 13, scope: !2907)
!2966 = !DILocation(line: 431, column: 22, scope: !2907)
!2967 = !DILocation(line: 432, column: 5, scope: !2907)
!2968 = !DILocation(line: 432, column: 9, scope: !2907)
!2969 = !DILocation(line: 432, column: 14, scope: !2907)
!2970 = !DILocation(line: 433, column: 5, scope: !2907)
!2971 = !DILocation(line: 433, column: 9, scope: !2907)
!2972 = !DILocation(line: 433, column: 14, scope: !2907)
!2973 = !DILocation(line: 434, column: 5, scope: !2907)
!2974 = !DILocation(line: 434, column: 9, scope: !2907)
!2975 = !DILocation(line: 434, column: 14, scope: !2907)
!2976 = !DILocation(line: 435, column: 5, scope: !2907)
!2977 = !DILocation(line: 435, column: 9, scope: !2907)
!2978 = !DILocation(line: 435, column: 14, scope: !2907)
!2979 = !DILocation(line: 436, column: 5, scope: !2907)
!2980 = !DILocation(line: 436, column: 9, scope: !2907)
!2981 = !DILocation(line: 436, column: 15, scope: !2907)
!2982 = !DILocation(line: 437, column: 5, scope: !2907)
!2983 = !DILocation(line: 437, column: 9, scope: !2907)
!2984 = !DILocation(line: 437, column: 16, scope: !2907)
!2985 = !DILocation(line: 438, column: 5, scope: !2907)
!2986 = !DILocation(line: 438, column: 9, scope: !2907)
!2987 = !DILocation(line: 438, column: 17, scope: !2907)
!2988 = !DILocation(line: 439, column: 5, scope: !2907)
!2989 = !DILocation(line: 439, column: 9, scope: !2907)
!2990 = !DILocation(line: 439, column: 17, scope: !2907)
!2991 = !DILocation(line: 440, column: 12, scope: !2907)
!2992 = !DILocation(line: 440, column: 5, scope: !2907)
!2993 = distinct !DISubprogram(name: "is_existant", scope: !32, file: !32, line: 480, type: !2994, scopeLine: 481, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!2994 = !DISubroutineType(types: !2995)
!2995 = !{!13, !15}
!2996 = !DILocalVariable(name: "file", arg: 1, scope: !2993, file: !32, line: 480, type: !15)
!2997 = !DILocation(line: 480, column: 30, scope: !2993)
!2998 = !DILocalVariable(name: "staat", scope: !2993, file: !32, line: 482, type: !2512)
!2999 = !DILocation(line: 482, column: 17, scope: !2993)
!3000 = !DILocation(line: 484, column: 14, scope: !3001)
!3001 = distinct !DILexicalBlock(scope: !2993, file: !32, line: 484, column: 9)
!3002 = !DILocation(line: 484, column: 9, scope: !3001)
!3003 = !DILocation(line: 484, column: 28, scope: !3001)
!3004 = !DILocation(line: 484, column: 9, scope: !2993)
!3005 = !DILocation(line: 486, column: 9, scope: !3006)
!3006 = distinct !DILexicalBlock(scope: !3001, file: !32, line: 485, column: 5)
!3007 = !DILocation(line: 488, column: 13, scope: !2993)
!3008 = !DILocation(line: 488, column: 5, scope: !2993)
!3009 = !DILocation(line: 492, column: 9, scope: !3010)
!3010 = distinct !DILexicalBlock(scope: !2993, file: !32, line: 489, column: 5)
!3011 = !DILocation(line: 494, column: 9, scope: !3010)
!3012 = !DILocation(line: 495, column: 9, scope: !3010)
!3013 = !DILocation(line: 497, column: 1, scope: !2993)
!3014 = distinct !DISubprogram(name: "is_dir", scope: !32, file: !32, line: 462, type: !1432, scopeLine: 463, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!3015 = !DILocalVariable(name: "file", arg: 1, scope: !3014, file: !32, line: 462, type: !7)
!3016 = !DILocation(line: 462, column: 37, scope: !3014)
!3017 = !DILocalVariable(name: "sbuf", scope: !3014, file: !32, line: 464, type: !2512)
!3018 = !DILocation(line: 464, column: 17, scope: !3014)
!3019 = !DILocation(line: 466, column: 14, scope: !3020)
!3020 = distinct !DILexicalBlock(scope: !3014, file: !32, line: 466, column: 9)
!3021 = !DILocation(line: 466, column: 9, scope: !3020)
!3022 = !DILocation(line: 466, column: 27, scope: !3020)
!3023 = !DILocation(line: 466, column: 9, scope: !3014)
!3024 = !DILocation(line: 469, column: 9, scope: !3025)
!3025 = distinct !DILexicalBlock(scope: !3020, file: !32, line: 467, column: 5)
!3026 = !DILocalVariable(name: "ret", scope: !3014, file: !32, line: 471, type: !3027)
!3027 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !64)
!3028 = !DILocation(line: 471, column: 16, scope: !3014)
!3029 = !DILocation(line: 471, column: 23, scope: !3014)
!3030 = !DILocation(line: 471, column: 22, scope: !3014)
!3031 = !DILocation(line: 473, column: 12, scope: !3014)
!3032 = !DILocation(line: 473, column: 5, scope: !3014)
!3033 = !DILocation(line: 474, column: 1, scope: !3014)
!3034 = distinct !DISubprogram(name: "path_is_absolute", scope: !32, file: !32, line: 562, type: !3035, scopeLine: 563, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!3035 = !DISubroutineType(types: !3036)
!3036 = !{!64, !7}
!3037 = !DILocalVariable(name: "path", arg: 1, scope: !3034, file: !32, line: 562, type: !7)
!3038 = !DILocation(line: 562, column: 48, scope: !3034)
!3039 = !DILocation(line: 564, column: 9, scope: !3040)
!3040 = distinct !DILexicalBlock(scope: !3034, file: !32, line: 564, column: 9)
!3041 = !DILocation(line: 564, column: 17, scope: !3040)
!3042 = !DILocation(line: 564, column: 9, scope: !3034)
!3043 = !DILocation(line: 566, column: 9, scope: !3044)
!3044 = distinct !DILexicalBlock(scope: !3040, file: !32, line: 565, column: 5)
!3045 = !DILocation(line: 574, column: 5, scope: !3034)
!3046 = !DILocation(line: 575, column: 1, scope: !3034)
!3047 = distinct !DISubprogram(name: "add_dir", scope: !32, file: !32, line: 780, type: !3048, scopeLine: 781, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!3048 = !DISubroutineType(types: !3049)
!3049 = !{!13, !1012}
!3050 = !DILocalVariable(name: "fp", arg: 1, scope: !3047, file: !32, line: 780, type: !1012)
!3051 = !DILocation(line: 780, column: 36, scope: !3047)
!3052 = !DILocalVariable(name: "dir", scope: !3047, file: !32, line: 782, type: !3053)
!3053 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !3054, size: 32)
!3054 = !DIDerivedType(tag: DW_TAG_typedef, name: "DIR", file: !3055, line: 4, baseType: !3056)
!3055 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__typedef_DIR.h", directory: "")
!3056 = !DICompositeType(tag: DW_TAG_structure_type, name: "_DIR", file: !3055, line: 4, flags: DIFlagFwdDecl)
!3057 = !DILocation(line: 782, column: 10, scope: !3047)
!3058 = !DILocalVariable(name: "dirent", scope: !3047, file: !32, line: 783, type: !3059)
!3059 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !3060, size: 32)
!3060 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "dirent", file: !3061, line: 8, size: 128, elements: !3062)
!3061 = !DIFile(filename: "/opt/wasi-sdk-16.0/bin/../share/wasi-sysroot/include/__struct_dirent.h", directory: "")
!3062 = !{!3063, !3064, !3065}
!3063 = !DIDerivedType(tag: DW_TAG_member, name: "d_ino", scope: !3060, file: !3061, line: 9, baseType: !2519, size: 64)
!3064 = !DIDerivedType(tag: DW_TAG_member, name: "d_type", scope: !3060, file: !3061, line: 10, baseType: !17, size: 8, offset: 64)
!3065 = !DIDerivedType(tag: DW_TAG_member, name: "d_name", scope: !3060, file: !3061, line: 11, baseType: !3066, offset: 72)
!3066 = !DICompositeType(tag: DW_TAG_array_type, baseType: !10, elements: !3067)
!3067 = !{!3068}
!3068 = !DISubrange(count: -1)
!3069 = !DILocation(line: 783, column: 20, scope: !3047)
!3070 = !DILocalVariable(name: "names", scope: !3047, file: !32, line: 784, type: !108)
!3071 = !DILocation(line: 784, column: 12, scope: !3047)
!3072 = !DILocalVariable(name: "i", scope: !3047, file: !32, line: 785, type: !25)
!3073 = !DILocation(line: 785, column: 12, scope: !3047)
!3074 = !DILocalVariable(name: "count_names", scope: !3047, file: !32, line: 785, type: !25)
!3075 = !DILocation(line: 785, column: 15, scope: !3047)
!3076 = !DILocalVariable(name: "max_count_names", scope: !3047, file: !32, line: 785, type: !25)
!3077 = !DILocation(line: 785, column: 28, scope: !3047)
!3078 = !DILocation(line: 787, column: 11, scope: !3047)
!3079 = !DILocation(line: 787, column: 15, scope: !3047)
!3080 = !DILocation(line: 787, column: 5, scope: !3047)
!3081 = !DILocation(line: 788, column: 5, scope: !3047)
!3082 = !DILocation(line: 788, column: 9, scope: !3047)
!3083 = !DILocation(line: 788, column: 12, scope: !3047)
!3084 = !DILocation(line: 789, column: 25, scope: !3085)
!3085 = distinct !DILexicalBlock(scope: !3047, file: !32, line: 789, column: 9)
!3086 = !DILocation(line: 789, column: 29, scope: !3085)
!3087 = !DILocation(line: 789, column: 17, scope: !3085)
!3088 = !DILocation(line: 789, column: 15, scope: !3085)
!3089 = !DILocation(line: 789, column: 9, scope: !3047)
!3090 = !DILocation(line: 791, column: 16, scope: !3091)
!3091 = distinct !DILexicalBlock(scope: !3085, file: !32, line: 790, column: 5)
!3092 = !DILocation(line: 791, column: 20, scope: !3091)
!3093 = !DILocation(line: 791, column: 9, scope: !3091)
!3094 = !DILocation(line: 792, column: 9, scope: !3091)
!3095 = !DILocalVariable(name: "tailp", scope: !3047, file: !32, line: 794, type: !47)
!3096 = !DILocation(line: 794, column: 15, scope: !3047)
!3097 = !DILocation(line: 796, column: 5, scope: !3047)
!3098 = !DILocation(line: 796, column: 9, scope: !3047)
!3099 = !DILocation(line: 796, column: 22, scope: !3047)
!3100 = !DILocation(line: 797, column: 21, scope: !3047)
!3101 = !DILocation(line: 798, column: 17, scope: !3047)
!3102 = !DILocation(line: 799, column: 39, scope: !3047)
!3103 = !DILocation(line: 799, column: 37, scope: !3047)
!3104 = !DILocation(line: 799, column: 13, scope: !3047)
!3105 = !DILocation(line: 799, column: 11, scope: !3047)
!3106 = !DILocation(line: 800, column: 10, scope: !3107)
!3107 = distinct !DILexicalBlock(scope: !3047, file: !32, line: 800, column: 9)
!3108 = !DILocation(line: 800, column: 9, scope: !3047)
!3109 = !DILocation(line: 802, column: 9, scope: !3110)
!3110 = distinct !DILexicalBlock(scope: !3107, file: !32, line: 801, column: 5)
!3111 = !DILocation(line: 803, column: 9, scope: !3110)
!3112 = !DILocation(line: 805, column: 5, scope: !3047)
!3113 = !DILocation(line: 805, column: 30, scope: !3047)
!3114 = !DILocation(line: 805, column: 22, scope: !3047)
!3115 = !DILocation(line: 805, column: 20, scope: !3047)
!3116 = !DILocation(line: 807, column: 13, scope: !3117)
!3117 = distinct !DILexicalBlock(scope: !3118, file: !32, line: 807, column: 13)
!3118 = distinct !DILexicalBlock(scope: !3047, file: !32, line: 806, column: 5)
!3119 = !DILocation(line: 807, column: 21, scope: !3117)
!3120 = !DILocation(line: 807, column: 31, scope: !3117)
!3121 = !DILocation(line: 807, column: 13, scope: !3118)
!3122 = !DILocation(line: 809, column: 13, scope: !3123)
!3123 = distinct !DILexicalBlock(scope: !3117, file: !32, line: 808, column: 9)
!3124 = distinct !{!3124, !3112, !3125, !238}
!3125 = !DILocation(line: 823, column: 5, scope: !3047)
!3126 = !DILocalVariable(name: "name", scope: !3118, file: !32, line: 811, type: !15)
!3127 = !DILocation(line: 811, column: 15, scope: !3118)
!3128 = !DILocation(line: 811, column: 29, scope: !3118)
!3129 = !DILocation(line: 811, column: 37, scope: !3118)
!3130 = !DILocation(line: 811, column: 22, scope: !3118)
!3131 = !DILocation(line: 812, column: 13, scope: !3132)
!3132 = distinct !DILexicalBlock(scope: !3118, file: !32, line: 812, column: 13)
!3133 = !DILocation(line: 812, column: 28, scope: !3132)
!3134 = !DILocation(line: 812, column: 25, scope: !3132)
!3135 = !DILocation(line: 812, column: 13, scope: !3118)
!3136 = !DILocation(line: 814, column: 29, scope: !3137)
!3137 = distinct !DILexicalBlock(scope: !3132, file: !32, line: 813, column: 9)
!3138 = !DILocation(line: 815, column: 29, scope: !3137)
!3139 = !DILocation(line: 815, column: 55, scope: !3137)
!3140 = !DILocation(line: 815, column: 53, scope: !3137)
!3141 = !DILocation(line: 815, column: 21, scope: !3137)
!3142 = !DILocation(line: 815, column: 19, scope: !3137)
!3143 = !DILocation(line: 816, column: 18, scope: !3144)
!3144 = distinct !DILexicalBlock(scope: !3137, file: !32, line: 816, column: 17)
!3145 = !DILocation(line: 816, column: 17, scope: !3137)
!3146 = !DILocation(line: 818, column: 17, scope: !3147)
!3147 = distinct !DILexicalBlock(scope: !3144, file: !32, line: 817, column: 13)
!3148 = !DILocation(line: 819, column: 17, scope: !3147)
!3149 = !DILocation(line: 821, column: 9, scope: !3137)
!3150 = !DILocation(line: 822, column: 32, scope: !3118)
!3151 = !DILocation(line: 822, column: 9, scope: !3118)
!3152 = !DILocation(line: 822, column: 26, scope: !3118)
!3153 = !DILocation(line: 822, column: 30, scope: !3118)
!3154 = !DILocation(line: 824, column: 14, scope: !3047)
!3155 = !DILocation(line: 824, column: 5, scope: !3047)
!3156 = !DILocation(line: 825, column: 11, scope: !3047)
!3157 = !DILocation(line: 825, column: 18, scope: !3047)
!3158 = !DILocation(line: 825, column: 5, scope: !3047)
!3159 = !DILocation(line: 827, column: 12, scope: !3160)
!3160 = distinct !DILexicalBlock(scope: !3047, file: !32, line: 827, column: 5)
!3161 = !DILocation(line: 827, column: 10, scope: !3160)
!3162 = !DILocation(line: 827, column: 17, scope: !3163)
!3163 = distinct !DILexicalBlock(scope: !3160, file: !32, line: 827, column: 5)
!3164 = !DILocation(line: 827, column: 21, scope: !3163)
!3165 = !DILocation(line: 827, column: 19, scope: !3163)
!3166 = !DILocation(line: 827, column: 5, scope: !3160)
!3167 = !DILocation(line: 829, column: 31, scope: !3168)
!3168 = distinct !DILexicalBlock(scope: !3169, file: !32, line: 829, column: 13)
!3169 = distinct !DILexicalBlock(scope: !3163, file: !32, line: 828, column: 5)
!3170 = !DILocation(line: 829, column: 37, scope: !3168)
!3171 = !DILocation(line: 829, column: 41, scope: !3168)
!3172 = !DILocation(line: 829, column: 45, scope: !3168)
!3173 = !DILocation(line: 829, column: 52, scope: !3168)
!3174 = !DILocation(line: 829, column: 56, scope: !3168)
!3175 = !DILocation(line: 829, column: 71, scope: !3168)
!3176 = !DILocation(line: 829, column: 13, scope: !3168)
!3177 = !DILocation(line: 829, column: 13, scope: !3169)
!3178 = !DILocation(line: 831, column: 13, scope: !3179)
!3179 = distinct !DILexicalBlock(scope: !3168, file: !32, line: 830, column: 9)
!3180 = !DILocation(line: 831, column: 17, scope: !3179)
!3181 = !DILocation(line: 831, column: 29, scope: !3179)
!3182 = !DILocation(line: 832, column: 9, scope: !3179)
!3183 = !DILocation(line: 833, column: 14, scope: !3169)
!3184 = !DILocation(line: 833, column: 20, scope: !3169)
!3185 = !DILocation(line: 833, column: 9, scope: !3169)
!3186 = !DILocation(line: 834, column: 5, scope: !3169)
!3187 = !DILocation(line: 827, column: 34, scope: !3163)
!3188 = !DILocation(line: 827, column: 5, scope: !3163)
!3189 = distinct !{!3189, !3166, !3190, !238}
!3190 = !DILocation(line: 834, column: 5, scope: !3160)
!3191 = !DILocation(line: 835, column: 10, scope: !3047)
!3192 = !DILocation(line: 835, column: 5, scope: !3047)
!3193 = !DILocation(line: 836, column: 9, scope: !3047)
!3194 = !DILocation(line: 837, column: 9, scope: !3195)
!3195 = distinct !DILexicalBlock(scope: !3047, file: !32, line: 837, column: 9)
!3196 = !DILocation(line: 837, column: 13, scope: !3195)
!3197 = !DILocation(line: 837, column: 26, scope: !3195)
!3198 = !DILocation(line: 837, column: 9, scope: !3047)
!3199 = !DILocation(line: 844, column: 60, scope: !3200)
!3200 = distinct !DILexicalBlock(scope: !3201, file: !32, line: 844, column: 13)
!3201 = distinct !DILexicalBlock(scope: !3195, file: !32, line: 838, column: 5)
!3202 = !DILocation(line: 844, column: 64, scope: !3200)
!3203 = !DILocation(line: 844, column: 13, scope: !3200)
!3204 = !DILocation(line: 844, column: 70, scope: !3200)
!3205 = !DILocation(line: 844, column: 75, scope: !3200)
!3206 = !DILocation(line: 845, column: 58, scope: !3200)
!3207 = !DILocation(line: 845, column: 62, scope: !3200)
!3208 = !DILocation(line: 845, column: 13, scope: !3200)
!3209 = !DILocation(line: 845, column: 68, scope: !3200)
!3210 = !DILocation(line: 844, column: 13, scope: !3201)
!3211 = !DILocation(line: 847, column: 13, scope: !3212)
!3212 = distinct !DILexicalBlock(scope: !3200, file: !32, line: 846, column: 9)
!3213 = !DILocation(line: 850, column: 13, scope: !3201)
!3214 = !DILocation(line: 850, column: 70, scope: !3201)
!3215 = !DILocation(line: 850, column: 74, scope: !3201)
!3216 = !DILocation(line: 849, column: 9, scope: !3201)
!3217 = !DILocation(line: 851, column: 9, scope: !3201)
!3218 = !DILocation(line: 853, column: 5, scope: !3047)
!3219 = !DILocation(line: 854, column: 1, scope: !3047)
!3220 = !DILocalVariable(name: "file", arg: 1, scope: !105, file: !32, line: 506, type: !7)
!3221 = !DILocation(line: 506, column: 42, scope: !105)
!3222 = !DILocalVariable(name: "datp", arg: 2, scope: !105, file: !32, line: 506, type: !108)
!3223 = !DILocation(line: 506, column: 55, scope: !105)
!3224 = !DILocalVariable(name: "sp", scope: !105, file: !32, line: 508, type: !8)
!3225 = !DILocation(line: 508, column: 17, scope: !105)
!3226 = !DILocation(line: 508, column: 30, scope: !105)
!3227 = !DILocation(line: 508, column: 22, scope: !105)
!3228 = !DILocation(line: 515, column: 10, scope: !3229)
!3229 = distinct !DILexicalBlock(scope: !105, file: !32, line: 515, column: 9)
!3230 = !DILocation(line: 515, column: 9, scope: !105)
!3231 = !DILocation(line: 517, column: 14, scope: !3232)
!3232 = distinct !DILexicalBlock(scope: !3229, file: !32, line: 516, column: 5)
!3233 = !DILocation(line: 517, column: 12, scope: !3232)
!3234 = !DILocation(line: 518, column: 5, scope: !3232)
!3235 = !DILocation(line: 521, column: 11, scope: !3236)
!3236 = distinct !DILexicalBlock(scope: !3229, file: !32, line: 520, column: 5)
!3237 = !DILocation(line: 523, column: 10, scope: !3238)
!3238 = distinct !DILexicalBlock(scope: !105, file: !32, line: 523, column: 9)
!3239 = !DILocation(line: 523, column: 9, scope: !3238)
!3240 = !DILocation(line: 523, column: 13, scope: !3238)
!3241 = !DILocation(line: 523, column: 9, scope: !105)
!3242 = !DILocation(line: 526, column: 9, scope: !3243)
!3243 = distinct !DILexicalBlock(scope: !3238, file: !32, line: 524, column: 5)
!3244 = !DILocation(line: 528, column: 23, scope: !3245)
!3245 = distinct !DILexicalBlock(scope: !105, file: !32, line: 528, column: 9)
!3246 = !DILocation(line: 528, column: 15, scope: !3245)
!3247 = !DILocation(line: 528, column: 13, scope: !3245)
!3248 = !DILocation(line: 528, column: 9, scope: !105)
!3249 = !DILocation(line: 530, column: 11, scope: !3250)
!3250 = distinct !DILexicalBlock(scope: !3245, file: !32, line: 529, column: 5)
!3251 = !DILocalVariable(name: "i", scope: !3252, file: !32, line: 531, type: !13)
!3252 = distinct !DILexicalBlock(scope: !3250, file: !32, line: 531, column: 9)
!3253 = !DILocation(line: 531, column: 18, scope: !3252)
!3254 = !DILocation(line: 531, column: 14, scope: !3252)
!3255 = !DILocation(line: 531, column: 33, scope: !3256)
!3256 = distinct !DILexicalBlock(scope: !3252, file: !32, line: 531, column: 9)
!3257 = !DILocation(line: 531, column: 25, scope: !3256)
!3258 = !DILocation(line: 531, column: 9, scope: !3252)
!3259 = !DILocation(line: 533, column: 24, scope: !3260)
!3260 = distinct !DILexicalBlock(scope: !3261, file: !32, line: 533, column: 17)
!3261 = distinct !DILexicalBlock(scope: !3256, file: !32, line: 532, column: 9)
!3262 = !DILocation(line: 533, column: 36, scope: !3260)
!3263 = !DILocation(line: 533, column: 28, scope: !3260)
!3264 = !DILocation(line: 533, column: 17, scope: !3260)
!3265 = !DILocation(line: 533, column: 40, scope: !3260)
!3266 = !DILocation(line: 533, column: 17, scope: !3261)
!3267 = !DILocation(line: 536, column: 17, scope: !3268)
!3268 = distinct !DILexicalBlock(scope: !3260, file: !32, line: 534, column: 13)
!3269 = !DILocation(line: 538, column: 9, scope: !3261)
!3270 = !DILocation(line: 531, column: 37, scope: !3256)
!3271 = !DILocation(line: 531, column: 9, scope: !3256)
!3272 = distinct !{!3272, !3258, !3273, !238}
!3273 = !DILocation(line: 538, column: 9, scope: !3252)
!3274 = !DILocation(line: 539, column: 5, scope: !3250)
!3275 = !DILocalVariable(name: "do_len", scope: !105, file: !32, line: 541, type: !1561)
!3276 = !DILocation(line: 541, column: 18, scope: !105)
!3277 = !DILocation(line: 541, column: 35, scope: !105)
!3278 = !DILocation(line: 541, column: 28, scope: !105)
!3279 = !DILocation(line: 541, column: 41, scope: !105)
!3280 = !DILocalVariable(name: "datfile", scope: !105, file: !32, line: 542, type: !3281)
!3281 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !15)
!3282 = !DILocation(line: 542, column: 17, scope: !105)
!3283 = !DILocation(line: 542, column: 37, scope: !105)
!3284 = !DILocation(line: 542, column: 44, scope: !105)
!3285 = !DILocation(line: 542, column: 27, scope: !105)
!3286 = !DILocation(line: 543, column: 14, scope: !105)
!3287 = !DILocation(line: 543, column: 23, scope: !105)
!3288 = !DILocation(line: 543, column: 41, scope: !105)
!3289 = !DILocation(line: 543, column: 5, scope: !105)
!3290 = !DILocation(line: 544, column: 16, scope: !3291)
!3291 = distinct !DILexicalBlock(scope: !105, file: !32, line: 544, column: 9)
!3292 = !DILocation(line: 544, column: 9, scope: !3291)
!3293 = !DILocation(line: 544, column: 31, scope: !3291)
!3294 = !DILocation(line: 544, column: 9, scope: !105)
!3295 = !DILocation(line: 546, column: 14, scope: !3296)
!3296 = distinct !DILexicalBlock(scope: !3291, file: !32, line: 545, column: 5)
!3297 = !DILocation(line: 546, column: 9, scope: !3296)
!3298 = !DILocation(line: 548, column: 9, scope: !3296)
!3299 = !DILocation(line: 550, column: 9, scope: !3300)
!3300 = distinct !DILexicalBlock(scope: !105, file: !32, line: 550, column: 9)
!3301 = !DILocation(line: 550, column: 9, scope: !105)
!3302 = !DILocation(line: 552, column: 17, scope: !3303)
!3303 = distinct !DILexicalBlock(scope: !3300, file: !32, line: 551, column: 5)
!3304 = !DILocation(line: 552, column: 10, scope: !3303)
!3305 = !DILocation(line: 552, column: 15, scope: !3303)
!3306 = !DILocation(line: 553, column: 5, scope: !3303)
!3307 = !DILocation(line: 556, column: 14, scope: !3308)
!3308 = distinct !DILexicalBlock(scope: !3300, file: !32, line: 555, column: 5)
!3309 = !DILocation(line: 556, column: 9, scope: !3308)
!3310 = !DILocation(line: 559, column: 5, scope: !105)
!3311 = !DILocation(line: 560, column: 1, scope: !105)
!3312 = distinct !DISubprogram(name: "names_compare", scope: !32, file: !32, line: 772, type: !3313, scopeLine: 773, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!3313 = !DISubroutineType(types: !3314)
!3314 = !{!13, !3315, !3315}
!3315 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !3316, size: 32)
!3316 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!3317 = !DILocalVariable(name: "a", arg: 1, scope: !3312, file: !32, line: 772, type: !3315)
!3318 = !DILocation(line: 772, column: 38, scope: !3312)
!3319 = !DILocalVariable(name: "b", arg: 2, scope: !3312, file: !32, line: 772, type: !3315)
!3320 = !DILocation(line: 772, column: 53, scope: !3312)
!3321 = !DILocation(line: 774, column: 41, scope: !3312)
!3322 = !DILocation(line: 774, column: 20, scope: !3312)
!3323 = !DILocation(line: 774, column: 19, scope: !3312)
!3324 = !DILocation(line: 774, column: 66, scope: !3312)
!3325 = !DILocation(line: 774, column: 45, scope: !3312)
!3326 = !DILocation(line: 774, column: 44, scope: !3312)
!3327 = !DILocation(line: 774, column: 12, scope: !3312)
!3328 = !DILocation(line: 774, column: 5, scope: !3312)
!3329 = distinct !DISubprogram(name: "cond_top_level__add_file", scope: !32, file: !32, line: 866, type: !3330, scopeLine: 868, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, retainedNodes: !97)
!3330 = !DISubroutineType(types: !3331)
!3331 = !{!13, !8, !8}
!3332 = !DILocalVariable(name: "dirpath", arg: 1, scope: !3329, file: !32, line: 867, type: !8)
!3333 = !DILocation(line: 867, column: 17, scope: !3329)
!3334 = !DILocalVariable(name: "possible_dup", arg: 2, scope: !3329, file: !32, line: 867, type: !8)
!3335 = !DILocation(line: 867, column: 38, scope: !3329)
!3336 = !DILocation(line: 869, column: 17, scope: !3337)
!3337 = distinct !DILexicalBlock(scope: !3329, file: !32, line: 869, column: 9)
!3338 = !DILocation(line: 869, column: 26, scope: !3337)
!3339 = !DILocation(line: 869, column: 10, scope: !3337)
!3340 = !DILocation(line: 869, column: 9, scope: !3329)
!3341 = !DILocation(line: 871, column: 9, scope: !3342)
!3342 = distinct !DILexicalBlock(scope: !3337, file: !32, line: 870, column: 5)
!3343 = !DILocation(line: 873, column: 32, scope: !3329)
!3344 = !DILocation(line: 873, column: 12, scope: !3329)
!3345 = !DILocation(line: 873, column: 5, scope: !3329)
!3346 = !DILocation(line: 874, column: 1, scope: !3329)
