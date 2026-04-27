; This small loop is meant to stress replace-mode PointerAnalysis.
; The phi copies %next back into %p, while %next is %p plus one byte.
; In PA terms this forms p <- field(p), which can keep extending MemoryLocKey
; paths if the solver does not cap or merge recursive field paths.
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal i32 @main(i32 %limit) {
entry:
  %buf = alloca [16 x i8], align 1
  %base = ptrtoint [16 x i8]* %buf to i32
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %p = phi i32 [ %base, %entry ], [ %next, %loop ]
  %next = add i32 %p, 1
  %p.ptr = inttoptr i32 %p to i8*
  %next.ptr = inttoptr i32 %next to i8*
  %v = load i8, i8* %p.ptr, align 1
  store i8 %v, i8* %next.ptr, align 1
  %i.next = add i32 %i, 1
  %done = icmp uge i32 %i.next, %limit
  br i1 %done, label %exit, label %loop

exit:
  ret i32 0
}
