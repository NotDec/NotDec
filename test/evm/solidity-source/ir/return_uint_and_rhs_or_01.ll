; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare void @evm_return(ptr, i256, i256)

define void @public_andrhsor_uint256_uint256_uint256__0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %arg0, i256 %arg1, i256 %arg2) #0 {
bb._0x2a:
  %rhs = or i256 %arg1, %arg2, !notdec.evm !0
  %masked = and i256 %arg0, %rhs, !notdec.evm !1
  %ret.ptr = inttoptr i256 128 to ptr
  store i256 %masked, ptr %ret.ptr, align 1, !notdec.evm !2
  call void @evm_return(ptr %mem, i256 128, i256 32), !notdec.evm !3
  unreachable, !notdec.evm !3
}

attributes #0 = { null_pointer_is_valid }

!0 = !{!"tac=0x2a", !"op=OR", !"evm.pc=0x2a"}
!1 = !{!"tac=0x2b", !"op=AND", !"evm.pc=0x2b"}
!2 = !{!"tac=0x2c", !"op=MSTORE", !"evm.pc=0x2c"}
!3 = !{!"tac=0x2d", !"op=RETURN", !"evm.pc=0x2d"}
