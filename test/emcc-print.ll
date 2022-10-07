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
}

define void @"$_start"() {
entry:
}

define internal i32 @"$getchar"() {
entry:
}

define internal i64 @"$__stdio_seek"(i32 %i32, i64 %i64, i32 %i321) {
entry:
}

define internal i32 @"$__stdio_read"(i32 %i32, i32 %i321, i32 %i322) {
entry:
}

define internal i32 @"$dummy"(i32 %i32) {
entry:
}

define internal i32 @"$__stdio_close"(i32 %i32) {
entry:
}

define internal i32 @"$__ofl_lock"() {
entry:
}

define internal void @"$__stdio_exit"() {
entry:
}

define internal void @"$close_file"(i32 %i32) {
entry:
}

define internal i32 @"$__toread"(i32 %i32) {
entry:
}

define internal i32 @"$__uflow"(i32 %i32) {
entry:
}

define internal i32 @"$fgetc"(i32 %i32) {
entry:
}

define internal void @"$__lock"(i32 %i32) {
entry:
}

define i32 @"$__errno_location"() {
entry:
}

define internal i32 @"$__wasi_syscall_ret"(i32 %i32) {
entry:
}

define internal void @"$_Exit"(i32 %i32) {
entry:
}

define internal void @"$libc_exit_fini"() {
entry:
}

define internal void @"$exit"(i32 %i32) {
entry:
}

define internal void @"$_fini"() {
entry:
}

define internal i32 @"$__stdio_write"(i32 %i32, i32 %i321, i32 %i322) {
entry:
}

define internal i32 @"$__emscripten_stdout_close"(i32 %i32) {
entry:
}

define internal i64 @"$__emscripten_stdout_seek"(i32 %i32, i64 %i64, i32 %i321) {
entry:
}

define internal i32 @"$memset"(i32 %i32, i32 %i321, i32 %i322) {
entry:
}

define internal i32 @"$isdigit"(i32 %i32) {
entry:
}

define internal i32 @"$memchr"(i32 %i32, i32 %i321, i32 %i322) {
entry:
}

define internal i32 @"$__memcpy"(i32 %i32, i32 %i321, i32 %i322) {
entry:
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
}

define internal i32 @"$wctomb"(i32 %i32, i32 %i321) {
entry:
}

define internal double @"$frexp"(double %f64, i32 %i32) {
entry:
}

define internal void @"$__ashlti3"(i32 %i32, i64 %i64, i64 %i641, i32 %i322) {
entry:
}

define internal void @"$__lshrti3"(i32 %i32, i64 %i64, i64 %i641, i32 %i322) {
entry:
}

define internal double @"$__trunctfdf2"(i64 %i64, i64 %i641) {
entry:
}

define internal i32 @"$__towrite"(i32 %i32) {
entry:
}

define internal i32 @"$__fwritex"(i32 %i32, i32 %i321, i32 %i322) {
entry:
}

define internal i32 @"$__vfprintf_internal"(i32 %i32, i32 %i321, i32 %i322, i32 %i323, i32 %i324) {
entry:
}

define internal i32 @"$printf_core"(i32 %i32, i32 %i321, i32 %i322, i32 %i323, i32 %i324, i32 %i325, i32 %i326) {
entry:
}

define internal void @"$out"(i32 %i32, i32 %i321, i32 %i322) {
entry:
}

define internal i32 @"$getint"(i32 %i32) {
entry:
}

define internal void @"$pop_arg"(i32 %i32, i32 %i321, i32 %i322, i32 %i323) {
entry:
}

define internal i32 @"$fmt_x"(i64 %i64, i32 %i32, i32 %i321) {
entry:
}

define internal i32 @"$fmt_o"(i64 %i64, i32 %i32) {
entry:
}

define internal i32 @"$fmt_u"(i64 %i64, i32 %i32) {
entry:
}

define internal void @"$pad"(i32 %i32, i32 %i321, i32 %i322, i32 %i323, i32 %i324) {
entry:
}

define internal i32 @"$vfprintf"(i32 %i32, i32 %i321, i32 %i322) {
entry:
}

define internal i32 @"$fmt_fp"(i32 %i32, double %f64, i32 %i321, i32 %i322, i32 %i323, i32 %i324) {
entry:
}

define internal void @"$pop_arg_long_double"(i32 %i32, i32 %i321) {
entry:
}

define internal i64 @"$__DOUBLE_BITS"(double %f64) {
entry:
}

define internal i32 @"$printf"(i32 %i32, i32 %i321) {
entry:
}

define internal i32 @"$__lockfile"(i32 %i32) {
entry:
}

define internal void @"$__unlockfile"(i32 %i32) {
entry:
}

define i32 @"$stackSave"() {
entry:
}

define void @"$stackRestore"(i32 %i32) {
entry:
}

define i32 @"$stackAlloc"(i32 %i32) {
entry:
}
