; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare void @evm_return(ptr, i256, i256)

define void @public_mixed_uint256_uint256_uint256_uint256_uint256_uint256__0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %arg0, i256 %arg1, i256 %arg2, i256 %arg3, i256 %arg4, i256 %arg5) #0 {
bb._0x2a:
  %lhs = icmp ult i256 %arg0, %arg1, !notdec.evm !0
  %rhs.left = icmp ult i256 %arg2, %arg3, !notdec.evm !1
  %rhs.right = icmp ult i256 %arg4, %arg5, !notdec.evm !2
  %rhs = or i1 %rhs.left, %rhs.right, !notdec.evm !3
  %both = and i1 %lhs, %rhs, !notdec.evm !4
  %ret.word = zext i1 %both to i256, !notdec.evm !4
  %ret.ptr = inttoptr i256 128 to ptr
  store i256 %ret.word, ptr %ret.ptr, align 1, !notdec.evm !5
  call void @evm_return(ptr %mem, i256 128, i256 32), !notdec.evm !6
  unreachable, !notdec.evm !6
}

attributes #0 = { null_pointer_is_valid }

!0 = !{!"tac=0x2a", !"op=LT", !"evm.pc=0x2a"}
!1 = !{!"tac=0x2b", !"op=LT", !"evm.pc=0x2b"}
!2 = !{!"tac=0x2c", !"op=LT", !"evm.pc=0x2c"}
!3 = !{!"tac=0x2d", !"op=OR", !"evm.pc=0x2d"}
!4 = !{!"tac=0x2e", !"op=AND", !"evm.pc=0x2e"}
!5 = !{!"tac=0x2f", !"op=MSTORE", !"evm.pc=0x2f"}
!6 = !{!"tac=0x30", !"op=RETURN", !"evm.pc=0x30"}
