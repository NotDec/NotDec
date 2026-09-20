; ModuleID = '/tmp/u4_helper_offset.ll'
source_filename = "/tmp/u4_helper_offset.ll"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare void @evm_sstore(i256, i256)

define internal i256 @abi_decode_word_from_calldata__unknown(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %off) !notdec.mlsub.polymorphic_function !0 !notdec.evm.original_private_helper !1 {
entry:
  %b = ptrtoint ptr %calldata to i256
  %a = add i256 %off, %b
  %p = inttoptr i256 %a to ptr
  %v = load i256, ptr %p, align 1
  ret i256 %v
}

define void @public_a__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) !notdec.solidity.abi_param.record !2 !notdec.solidity.abi_param.annotated !3 !notdec.solidity.abi_param.mismatch !4 {
entry:
  %r = call i256 @abi_decode_word_from_calldata__unknown(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4), !notdec.solidity.calldata.index !2
  %0 = call i256 @evm.storage.slot(i256 4)
  call void @evm.storage.store(i256 %0, i256 %r)
  ret void
}

define void @public_b__0x20(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) !notdec.solidity.abi_param.record !2 !notdec.solidity.abi_param.annotated !5 !notdec.solidity.abi_param.mismatch !6 {
entry:
  %r = call i256 @abi_decode_word_from_calldata__unknown(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 36), !notdec.solidity.calldata.index !3
  %0 = call i256 @evm.storage.slot(i256 36)
  call void @evm.storage.store(i256 %0, i256 %r)
  ret void
}

declare i256 @evm.storage.slot(i256)

declare void @evm.storage.store(i256, i256)

!0 = !{!"calldata_buffer"}
!1 = !{!"shared_h"}
!2 = !{!"0"}
!3 = !{!"1"}
!4 = !{!"record=0 annotated=1"}
!5 = !{!"2"}
!6 = !{!"record=0 annotated=2"}
