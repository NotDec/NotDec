; This case extracts the shape behind fortune's hot malloc loop.
; The loop-carried pointer %p feeds three backedge candidates:
; - one +1 step
; - two distinct +4 steps from different branch sites
; Current PA path dedup keeps ptradd site tags, so the two +4 edges are
; different atoms even though they print as the same "@4" offset in traces.
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

declare i32 @malloc(i32)

define internal i32 @main(i32 %limit, i1 %take_short, i1 %pick_left) {
entry:
  %base = call i32 @malloc(i32 64)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %merge ]
  %p = phi i32 [ %base, %entry ], [ %next, %merge ]
  br i1 %take_short, label %short_step, label %wide_step

short_step:
  %short.next = add i32 %p, 1
  %short.ptr = inttoptr i32 %short.next to i8*
  store i8 1, i8* %short.ptr, align 1
  br label %merge

wide_step:
  br i1 %pick_left, label %wide_left, label %wide_right

wide_left:
  %wide.left.next = add i32 %p, 4
  %wide.left.ptr = inttoptr i32 %wide.left.next to i8*
  store i8 4, i8* %wide.left.ptr, align 1
  br label %merge

wide_right:
  %wide.right.next = add i32 %p, 4
  %wide.right.ptr = inttoptr i32 %wide.right.next to i8*
  store i8 5, i8* %wide.right.ptr, align 1
  br label %merge

merge:
  %next = phi i32 [ %short.next, %short_step ], [ %wide.left.next, %wide_left ], [ %wide.right.next, %wide_right ]
  %next.ptr = inttoptr i32 %next to i8*
  %loaded = load i8, i8* %next.ptr, align 1
  %echo.ptr = inttoptr i32 %p to i8*
  store i8 %loaded, i8* %echo.ptr, align 1
  %i.next = add i32 %i, 1
  %done = icmp uge i32 %i.next, %limit
  br i1 %done, label %exit, label %loop

exit:
  ret i32 0
}
