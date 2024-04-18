%union.anon = type { i64, [24 x i8] }
%struct.anon.0 = type { i64, i64, %union.anon, i64, i64 }
%struct.anon = type { i64, %struct.anon.0, i64 }
%struct.foo = type { %struct.anon }

@chunky = common dso_local global %struct.foo zeroinitializer, align 8

define i8 @take_field() #0 {
  %1 = load i8, i8* getelementptr inbounds (%struct.foo, %struct.foo* @chunky, i64 0, i32 0, i32 1, i32 2, i32 1, i64 9), align 1
  ret i8 %1
}
