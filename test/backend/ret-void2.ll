%mytype = type { %mytype*, i32 }

@T1 = global [1 x void ()*] [void ()* @dummy]
@T0 = global [1 x void ()*] [void ()* null]
@aaa = global i32* null
@b = global %mytype { %mytype* null, i32 0 }

define void @test() {
  call void @dummy()
  ret void
}

define internal void @dummy() {
allocator:
  %a0 = sub i32 1, 12
  %a1 = fcmp olt float 0.0, 0.0
  call void @dummy()
  ;%a8 = getelementptr [1 x void ()*], [1 x void ()*]* @T0, i32 0, i32 0
  ;%callind_funcptr22 = load void ()*, void ()** %a8
  ;call void %callind_funcptr22()
  br label %return

return:                                           ; preds = %allocator
  ret void
}
