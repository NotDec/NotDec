
@a = constant i32 0


define hidden i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %2 = load i32, i32* %1, align 4
  ret i32 %2
}

define i32 @GEPTest() #0 {
  %1 = alloca i32, align 4
  %2 = getelementptr inbounds i32, i32* %1, i64 1
  store i32 0, i32* %2, align 4
  ret i32 0
}



; define i32 @GEPTest2() #0 {
;   %1 = alloca [3 x i32], align 4
;   %2 = getelementptr inbounds [3 x i32], [3 x i32]* %1, i64 1
;   store [3 x i32] zeroinitializer, [3 x i32]* %2, align 4
;   ret i32 0
; }

