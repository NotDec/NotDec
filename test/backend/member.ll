
%struct1 = type { i64, i64 }

@chunky = common dso_local global %struct1 zeroinitializer, align 8

define i64 @take_field() #0 {
  %1 = getelementptr inbounds %struct1, %struct1* @chunky, i64 0, i32 1
  %2 = load i64, i64* %1
  ret i64 %2
}


; define i64 @take_field2() #0 {
;   %1 = load i64, i64* getelementptr inbounds (%struct1, %struct1* @chunky, i64 0, i32 1), align 1
;   ret i64 %1
; }
