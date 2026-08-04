; This small loop exercises recursive pointer-arithmetic field recovery.
; The phi copies %next back into %p, while %next is %p plus one byte.
; The recursive BinarySub field constraints must converge without losing the
; load/store evidence at the two adjacent addresses.
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

define internal i32 @main(i32 %limit) {
entry:
  %buf = alloca [16 x i8], align 1
  %base = ptrtoint ptr %buf to i32
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %p = phi i32 [ %base, %entry ], [ %next, %loop ]
  %next = add i32 %p, 1
  %p.ptr = inttoptr i32 %p to ptr
  %next.ptr = inttoptr i32 %next to ptr
  %v = load i8, ptr %p.ptr, align 1
  store i8 %v, ptr %next.ptr, align 1
  %i.next = add i32 %i, 1
  %done = icmp uge i32 %i.next, %limit
  br i1 %done, label %exit, label %loop

exit:
  ret i32 0
}
