; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare void @evm_mcopy(ptr, i256, i256, i256)

declare i256 @evm_msize(ptr)

declare i256 @evm_sload(i256)

declare void @evm_sstore(i256, i256)

declare i256 @evm_tload(i256)

declare void @evm_tstore(i256, i256)

declare i256 @evm_balance(ptr, i256)

declare i256 @evm_calldataload(ptr, i256)

declare i256 @evm_calldatasize(ptr)

declare void @evm_calldatacopy(ptr, ptr, i256, i256, i256)

declare i256 @evm_codesize(ptr)

declare void @evm_codecopy(ptr, ptr, i256, i256, i256)

declare void @evm_extcodecopy(ptr, ptr, i256, i256, i256, i256)

declare i256 @evm_returndatasize(ptr)

declare void @evm_returndatacopy(ptr, ptr, i256, i256, i256)

declare i256 @evm_sha3(ptr, i256, i256)

declare void @evm_log0(ptr, i256, i256)

declare void @evm_log1(ptr, i256, i256, i256)

declare void @evm_log2(ptr, i256, i256, i256, i256)

declare void @evm_log3(ptr, i256, i256, i256, i256, i256)

declare void @evm_log4(ptr, i256, i256, i256, i256, i256, i256)

declare i256 @evm_call(ptr, ptr, ptr, i256, i256, i256, i256, i256, i256, i256)

declare i256 @evm_delegatecall(ptr, ptr, ptr, i256, i256, i256, i256, i256, i256)

declare i256 @evm_staticcall(ptr, ptr, ptr, i256, i256, i256, i256, i256, i256)

declare i256 @evm_callvalue(ptr)

declare i256 @evm_address(ptr)

declare i256 @evm_caller(ptr)

declare i256 @evm_origin(ptr)

declare i256 @evm_extcodesize(ptr, i256)

declare i256 @evm_extcodehash(ptr, i256)

declare i256 @evm_gasprice(ptr)

declare i256 @evm_blockhash(ptr, i256)

declare i256 @evm_coinbase(ptr)

declare i256 @evm_timestamp(ptr)

declare i256 @evm_number(ptr)

declare i256 @evm_prevrandao(ptr)

declare i256 @evm_gaslimit(ptr)

declare i256 @evm_chainid(ptr)

declare i256 @evm_basefee(ptr)

declare i256 @evm_blobhash(ptr, i256)

declare i256 @evm_blobbasefee(ptr)

declare i256 @evm_gas(ptr)

declare i256 @evm_pc(ptr)

declare i256 @evm_selfbalance(ptr)

declare void @evm_selfdestruct(ptr, i256)

declare i256 @evm_create(ptr, ptr, i256, i256, i256)

declare i256 @evm_create2(ptr, ptr, i256, i256, i256, i256)

declare i256 @evm_callcode(ptr, ptr, ptr, i256, i256, i256, i256, i256, i256, i256)

declare i256 @evm_div(i256, i256)

declare i256 @evm_sdiv(i256, i256)

declare i256 @evm_mod(i256, i256)

declare i256 @evm_smod(i256, i256)

declare i256 @evm_addmod(i256, i256, i256)

declare i256 @evm_mulmod(i256, i256, i256)

declare i256 @evm_exp(i256, i256)

declare i256 @evm_signextend(i256, i256)

declare i256 @evm_byte(i256, i256)

declare i256 @evm_shl(i256, i256)

declare i256 @evm_shr(i256, i256)

declare i256 @evm_sar(i256, i256)

declare void @evm_return(ptr, i256, i256)

declare void @evm_revert(ptr, i256, i256)

; Function Attrs: null_pointer_is_valid
define void @public___function_selector___0x0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x0:
  store i256 128, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !0
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !1
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !2
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !2
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !3
  br i1 %evm.branch.cond, label %bb._0xf, label %bb._0xb, !notdec.evm !3

bb._0xf:                                          ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.lt to i256, !notdec.evm !5
  %evm.branch.cond2 = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond2, label %bb._0x4cd0, label %bb._0x18, !notdec.evm !6

bb._0x18:                                         ; preds = %bb._0xf
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 864624567, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x56d0, label %bb._0x28, !notdec.evm !10

bb._0x56d0:                                       ; preds = %bb._0x18
  call void @public__0x33891fb7_0x37(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x28:                                         ; preds = %bb._0x18
  %evm.eq5 = icmp eq i256 1647451790, %evm.shr, !notdec.evm !12
  %evm.bool6 = zext i1 %evm.eq5 to i256, !notdec.evm !12
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !13
  br i1 %evm.branch.cond7, label %bb._0x60d0, label %bb._0x4cd0, !notdec.evm !13

bb._0x60d0:                                       ; preds = %bb._0x28
  call void @public__0x62321e8e_0x57(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !14
  ret void

bb._0x4cd0:                                       ; preds = %bb._0x28, %bb._0xf
  call void @public_fallback___0x32(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !15
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !16
  unreachable, !notdec.evm !16
}

; Function Attrs: null_pointer_is_valid
define void @public_fallback___0x32(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x32:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !17
  unreachable, !notdec.evm !17
}

; Function Attrs: null_pointer_is_valid
define void @public__0x33891fb7_0x37(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x37:
  br label %bb._0x1716, !notdec.evm !18

bb._0x1716:                                       ; preds = %bb._0x37
  %evm.mload = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !19
  %evm.mem.ptr = inttoptr i256 %evm.mload to ptr, !notdec.evm !20
  store i256 1000000000000000000, ptr %evm.mem.ptr, align 1, !notdec.evm !20
  %evm.add = add i256 32, %evm.mload, !notdec.evm !21
  %evm.mload1 = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !22
  %evm.sub = sub i256 %evm.add, %evm.mload1, !notdec.evm !23
  call void @evm_return(ptr %mem, i256 %evm.mload1, i256 %evm.sub), !notdec.evm !24
  ret void, !notdec.evm !24
}

; Function Attrs: null_pointer_is_valid
define void @public__0x62321e8e_0x57(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x57:
  br label %bb._0x1747, !notdec.evm !25

bb._0x1747:                                       ; preds = %bb._0x57
  %evm.mload = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !26
  %evm.mem.ptr = inttoptr i256 %evm.mload to ptr, !notdec.evm !27
  store i256 1000000, ptr %evm.mem.ptr, align 1, !notdec.evm !27
  %evm.add = add i256 32, %evm.mload, !notdec.evm !28
  %evm.mload1 = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !29
  %evm.sub = sub i256 %evm.add, %evm.mload1, !notdec.evm !30
  call void @evm_return(ptr %mem, i256 %evm.mload1, i256 %evm.sub), !notdec.evm !31
  ret void, !notdec.evm !31
}

attributes #0 = { null_pointer_is_valid }

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x13", !"op=CALLDATASIZE", !"evm.pc=0x13"}
!5 = !{!"tac=0x14", !"op=LT", !"evm.pc=0x14"}
!6 = !{!"tac=0x2ef0", !"op=JUMPI", !"evm.pc=0x15"}
!7 = !{!"tac=0x1a", !"op=CALLDATALOAD", !"evm.pc=0x1a"}
!8 = !{!"tac=0x1d", !"op=SHR", !"evm.pc=0x1d"}
!9 = !{!"tac=0x24", !"op=EQ", !"evm.pc=0x24"}
!10 = !{!"tac=0x38f0", !"op=JUMPI", !"evm.pc=0x25"}
!11 = !{!"tac=0x5710", !"op=CALLPRIVATE", !"evm.pc=0x37"}
!12 = !{!"tac=0x2e", !"op=EQ", !"evm.pc=0x2e"}
!13 = !{!"tac=0x42f0", !"op=JUMPI", !"evm.pc=0x2f"}
!14 = !{!"tac=0x6110", !"op=CALLPRIVATE", !"evm.pc=0x57"}
!15 = !{!"tac=0x4d10", !"op=CALLPRIVATE", !"evm.pc=0x32"}
!16 = !{!"tac=0xe", !"op=REVERT", !"evm.pc=0xe"}
!17 = !{!"tac=0x36", !"op=REVERT", !"evm.pc=0x36"}
!18 = !{!"tac=0x44", !"op=JUMP", !"evm.pc=0x44"}
!19 = !{!"tac=0x1719", !"op=MLOAD", !"evm.pc=0x48"}
!20 = !{!"tac=0x171c", !"op=MSTORE", !"evm.pc=0x4b"}
!21 = !{!"tac=0x171f", !"op=ADD", !"evm.pc=0x4e"}
!22 = !{!"tac=0x1722", !"op=MLOAD", !"evm.pc=0x51"}
!23 = !{!"tac=0x1725", !"op=SUB", !"evm.pc=0x54"}
!24 = !{!"tac=0x1727", !"op=RETURN", !"evm.pc=0x56"}
!25 = !{!"tac=0x5f", !"op=JUMP", !"evm.pc=0x5f"}
!26 = !{!"tac=0x174a", !"op=MLOAD", !"evm.pc=0x48"}
!27 = !{!"tac=0x174d", !"op=MSTORE", !"evm.pc=0x4b"}
!28 = !{!"tac=0x1750", !"op=ADD", !"evm.pc=0x4e"}
!29 = !{!"tac=0x1753", !"op=MLOAD", !"evm.pc=0x51"}
!30 = !{!"tac=0x1756", !"op=SUB", !"evm.pc=0x54"}
!31 = !{!"tac=0x1758", !"op=RETURN", !"evm.pc=0x56"}
