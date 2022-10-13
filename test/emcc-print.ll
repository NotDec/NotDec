; ModuleID = './test/emcc-print.wasm'
source_filename = "./test/emcc-print.wasm"

@"$__stack_pointer" = internal global i32 5247184
@"$__stack_end" = internal global i32 0
@"$__stack_base" = internal global i32 0
@mem0 = external dso_local global [16777216 x i8]

declare i32 @"$__wasi_fd_seek"(i32, i64, i32, i32)

declare i32 @"$__wasi_fd_read"(i32, i32, i32, i32)

declare i32 @"$__wasi_fd_close"(i32)

declare void @"$__wasi_proc_exit"(i32)

declare i32 @"$__wasi_fd_write"(i32, i32, i32, i32)

define internal void @"$__wasm_call_ctors"() {
entry:
}

define internal i32 @"$__original_main"() {
entry:
  %_local_0 = alloca i32, align 4
  store i32 0, i32* %_local_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
  %_local_6 = alloca i32, align 4
  store i32 0, i32* %_local_6, align 4
  %_local_7 = alloca i32, align 4
  store i32 0, i32* %_local_7, align 4
  %_local_8 = alloca i32, align 4
  store i32 0, i32* %_local_8, align 4
  %_local_9 = alloca i32, align 4
  store i32 0, i32* %_local_9, align 4
  %_local_10 = alloca i32, align 4
  store i32 0, i32* %_local_10, align 4
  %_local_11 = alloca i32, align 4
  store i32 0, i32* %_local_11, align 4
  %_local_12 = alloca i32, align 4
  store i32 0, i32* %_local_12, align 4
  %_local_13 = alloca i32, align 4
  store i32 0, i32* %_local_13, align 4
  %_local_14 = alloca i32, align 4
  store i32 0, i32* %_local_14, align 4
  %_local_15 = alloca i32, align 4
  store i32 0, i32* %_local_15, align 4
  %_local_16 = alloca i32, align 4
  store i32 0, i32* %_local_16, align 4
  %_local_17 = alloca i32, align 4
  store i32 0, i32* %_local_17, align 4
  %_local_18 = alloca i32, align 4
  store i32 0, i32* %_local_18, align 4
  %_local_19 = alloca i32, align 4
  store i32 0, i32* %_local_19, align 4
}

define void @"$_start"() {
entry:
}

define internal i32 @"$getchar"() {
entry:
}

define internal i64 @"$__stdio_seek"(i32 %i32, i64 %i64, i32 %i321) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i64, align 8
  store i64 %i64, i64* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i321, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
}

define internal i32 @"$__stdio_read"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
  %_local_6 = alloca i32, align 4
  store i32 0, i32* %_local_6, align 4
}

define internal i32 @"$dummy"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal i32 @"$__stdio_close"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal i32 @"$__ofl_lock"() {
entry:
}

define internal void @"$__stdio_exit"() {
entry:
  %_local_0 = alloca i32, align 4
  store i32 0, i32* %_local_0, align 4
}

define internal void @"$close_file"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
}

define internal i32 @"$__toread"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
}

define internal i32 @"$__uflow"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
}

define internal i32 @"$fgetc"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
}

define internal void @"$__lock"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define i32 @"$__errno_location"() {
entry:
}

define internal i32 @"$__wasi_syscall_ret"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal void @"$_Exit"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal void @"$libc_exit_fini"() {
entry:
  %_local_0 = alloca i32, align 4
  store i32 0, i32* %_local_0, align 4
}

define internal void @"$exit"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal void @"$_fini"() {
entry:
}

define internal i32 @"$__stdio_write"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
  %_local_6 = alloca i32, align 4
  store i32 0, i32* %_local_6, align 4
  %_local_7 = alloca i32, align 4
  store i32 0, i32* %_local_7, align 4
  %_local_8 = alloca i32, align 4
  store i32 0, i32* %_local_8, align 4
  %_local_9 = alloca i32, align 4
  store i32 0, i32* %_local_9, align 4
}

define internal i32 @"$__emscripten_stdout_close"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal i64 @"$__emscripten_stdout_seek"(i32 %i32, i64 %i64, i32 %i321) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i64, align 8
  store i64 %i64, i64* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i321, i32* %_param_2, align 4
}

define internal i32 @"$memset"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
  %_local_6 = alloca i64, align 8
  store i64 0, i64* %_local_6, align 4
}

define internal i32 @"$isdigit"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal i32 @"$memchr"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
}

define internal i32 @"$__memcpy"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
}

define void @"$emscripten_stack_init"() {
entry:
}

define i32 @"$emscripten_stack_get_free"() {
entry:
}

define i32 @"$emscripten_stack_get_end"() {
entry:
}

define internal i32 @"$__pthread_self"() {
entry:
}

define internal i32 @"$wcrtomb"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
}

define internal i32 @"$wctomb"(i32 %i32, i32 %i321) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
}

define internal double @"$frexp"(double %f64, i32 %i32) {
entry:
  %_param_0 = alloca double, align 8
  store double %f64, double* %_param_0, align 8
  %_param_1 = alloca i32, align 4
  store i32 %i32, i32* %_param_1, align 4
  %_local_2 = alloca i64, align 8
  store i64 0, i64* %_local_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
}

define internal void @"$__ashlti3"(i32 %i32, i64 %i64, i64 %i641, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i64, align 8
  store i64 %i64, i64* %_param_1, align 4
  %_param_2 = alloca i64, align 8
  store i64 %i641, i64* %_param_2, align 4
  %_param_3 = alloca i32, align 4
  store i32 %i322, i32* %_param_3, align 4
  %_local_4 = alloca i64, align 8
  store i64 0, i64* %_local_4, align 4
}

define internal void @"$__lshrti3"(i32 %i32, i64 %i64, i64 %i641, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i64, align 8
  store i64 %i64, i64* %_param_1, align 4
  %_param_2 = alloca i64, align 8
  store i64 %i641, i64* %_param_2, align 4
  %_param_3 = alloca i32, align 4
  store i32 %i322, i32* %_param_3, align 4
  %_local_4 = alloca i64, align 8
  store i64 0, i64* %_local_4, align 4
}

define internal double @"$__trunctfdf2"(i64 %i64, i64 %i641) {
entry:
  %_param_0 = alloca i64, align 8
  store i64 %i64, i64* %_param_0, align 4
  %_param_1 = alloca i64, align 8
  store i64 %i641, i64* %_param_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
  %_local_3 = alloca i64, align 8
  store i64 0, i64* %_local_3, align 4
  %_local_4 = alloca i64, align 8
  store i64 0, i64* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
}

define internal i32 @"$__towrite"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
}

define internal i32 @"$__fwritex"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
}

define internal i32 @"$__vfprintf_internal"(i32 %i32, i32 %i321, i32 %i322, i32 %i323, i32 %i324) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_param_3 = alloca i32, align 4
  store i32 %i323, i32* %_param_3, align 4
  %_param_4 = alloca i32, align 4
  store i32 %i324, i32* %_param_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
  %_local_6 = alloca i32, align 4
  store i32 0, i32* %_local_6, align 4
  %_local_7 = alloca i32, align 4
  store i32 0, i32* %_local_7, align 4
}

define internal i32 @"$printf_core"(i32 %i32, i32 %i321, i32 %i322, i32 %i323, i32 %i324, i32 %i325, i32 %i326) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_param_3 = alloca i32, align 4
  store i32 %i323, i32* %_param_3, align 4
  %_param_4 = alloca i32, align 4
  store i32 %i324, i32* %_param_4, align 4
  %_param_5 = alloca i32, align 4
  store i32 %i325, i32* %_param_5, align 4
  %_param_6 = alloca i32, align 4
  store i32 %i326, i32* %_param_6, align 4
  %_local_7 = alloca i32, align 4
  store i32 0, i32* %_local_7, align 4
  %_local_8 = alloca i32, align 4
  store i32 0, i32* %_local_8, align 4
  %_local_9 = alloca i32, align 4
  store i32 0, i32* %_local_9, align 4
  %_local_10 = alloca i32, align 4
  store i32 0, i32* %_local_10, align 4
  %_local_11 = alloca i32, align 4
  store i32 0, i32* %_local_11, align 4
  %_local_12 = alloca i32, align 4
  store i32 0, i32* %_local_12, align 4
  %_local_13 = alloca i32, align 4
  store i32 0, i32* %_local_13, align 4
  %_local_14 = alloca i32, align 4
  store i32 0, i32* %_local_14, align 4
  %_local_15 = alloca i32, align 4
  store i32 0, i32* %_local_15, align 4
  %_local_16 = alloca i32, align 4
  store i32 0, i32* %_local_16, align 4
  %_local_17 = alloca i32, align 4
  store i32 0, i32* %_local_17, align 4
  %_local_18 = alloca i32, align 4
  store i32 0, i32* %_local_18, align 4
  %_local_19 = alloca i32, align 4
  store i32 0, i32* %_local_19, align 4
  %_local_20 = alloca i32, align 4
  store i32 0, i32* %_local_20, align 4
  %_local_21 = alloca i32, align 4
  store i32 0, i32* %_local_21, align 4
  %_local_22 = alloca i64, align 8
  store i64 0, i64* %_local_22, align 4
}

define internal void @"$out"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
}

define internal i32 @"$getint"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
}

define internal void @"$pop_arg"(i32 %i32, i32 %i321, i32 %i322, i32 %i323) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_param_3 = alloca i32, align 4
  store i32 %i323, i32* %_param_3, align 4
}

define internal i32 @"$fmt_x"(i64 %i64, i32 %i32, i32 %i321) {
entry:
  %_param_0 = alloca i64, align 8
  store i64 %i64, i64* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i32, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i321, i32* %_param_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
}

define internal i32 @"$fmt_o"(i64 %i64, i32 %i32) {
entry:
  %_param_0 = alloca i64, align 8
  store i64 %i64, i64* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i32, i32* %_param_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
}

define internal i32 @"$fmt_u"(i64 %i64, i32 %i32) {
entry:
  %_param_0 = alloca i64, align 8
  store i64 %i64, i64* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i32, i32* %_param_1, align 4
  %_local_2 = alloca i64, align 8
  store i64 0, i64* %_local_2, align 4
  %_local_3 = alloca i32, align 4
  store i32 0, i32* %_local_3, align 4
  %_local_4 = alloca i32, align 4
  store i32 0, i32* %_local_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
}

define internal void @"$pad"(i32 %i32, i32 %i321, i32 %i322, i32 %i323, i32 %i324) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
  %_param_3 = alloca i32, align 4
  store i32 %i323, i32* %_param_3, align 4
  %_param_4 = alloca i32, align 4
  store i32 %i324, i32* %_param_4, align 4
  %_local_5 = alloca i32, align 4
  store i32 0, i32* %_local_5, align 4
}

define internal i32 @"$vfprintf"(i32 %i32, i32 %i321, i32 %i322) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_param_2 = alloca i32, align 4
  store i32 %i322, i32* %_param_2, align 4
}

define internal i32 @"$fmt_fp"(i32 %i32, double %f64, i32 %i321, i32 %i322, i32 %i323, i32 %i324) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca double, align 8
  store double %f64, double* %_param_1, align 8
  %_param_2 = alloca i32, align 4
  store i32 %i321, i32* %_param_2, align 4
  %_param_3 = alloca i32, align 4
  store i32 %i322, i32* %_param_3, align 4
  %_param_4 = alloca i32, align 4
  store i32 %i323, i32* %_param_4, align 4
  %_param_5 = alloca i32, align 4
  store i32 %i324, i32* %_param_5, align 4
  %_local_6 = alloca i32, align 4
  store i32 0, i32* %_local_6, align 4
  %_local_7 = alloca i32, align 4
  store i32 0, i32* %_local_7, align 4
  %_local_8 = alloca i64, align 8
  store i64 0, i64* %_local_8, align 4
  %_local_9 = alloca i32, align 4
  store i32 0, i32* %_local_9, align 4
  %_local_10 = alloca i32, align 4
  store i32 0, i32* %_local_10, align 4
  %_local_11 = alloca i32, align 4
  store i32 0, i32* %_local_11, align 4
  %_local_12 = alloca i32, align 4
  store i32 0, i32* %_local_12, align 4
  %_local_13 = alloca i32, align 4
  store i32 0, i32* %_local_13, align 4
  %_local_14 = alloca i32, align 4
  store i32 0, i32* %_local_14, align 4
  %_local_15 = alloca i32, align 4
  store i32 0, i32* %_local_15, align 4
  %_local_16 = alloca i32, align 4
  store i32 0, i32* %_local_16, align 4
  %_local_17 = alloca i32, align 4
  store i32 0, i32* %_local_17, align 4
  %_local_18 = alloca i32, align 4
  store i32 0, i32* %_local_18, align 4
  %_local_19 = alloca i32, align 4
  store i32 0, i32* %_local_19, align 4
  %_local_20 = alloca i64, align 8
  store i64 0, i64* %_local_20, align 4
  %_local_21 = alloca i32, align 4
  store i32 0, i32* %_local_21, align 4
  %_local_22 = alloca i32, align 4
  store i32 0, i32* %_local_22, align 4
  %_local_23 = alloca i32, align 4
  store i32 0, i32* %_local_23, align 4
  %_local_24 = alloca i32, align 4
  store i32 0, i32* %_local_24, align 4
  %_local_25 = alloca i32, align 4
  store i32 0, i32* %_local_25, align 4
  %_local_26 = alloca double, align 8
  store double 0.000000e+00, double* %_local_26, align 8
}

define internal void @"$pop_arg_long_double"(i32 %i32, i32 %i321) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
}

define internal i64 @"$__DOUBLE_BITS"(double %f64) {
entry:
  %_param_0 = alloca double, align 8
  store double %f64, double* %_param_0, align 8
}

define internal i32 @"$printf"(i32 %i32, i32 %i321) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_param_1 = alloca i32, align 4
  store i32 %i321, i32* %_param_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
}

define internal i32 @"$__lockfile"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define internal void @"$__unlockfile"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define i32 @"$stackSave"() {
entry:
}

define void @"$stackRestore"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
}

define i32 @"$stackAlloc"(i32 %i32) {
entry:
  %_param_0 = alloca i32, align 4
  store i32 %i32, i32* %_param_0, align 4
  %_local_1 = alloca i32, align 4
  store i32 0, i32* %_local_1, align 4
  %_local_2 = alloca i32, align 4
  store i32 0, i32* %_local_2, align 4
}
