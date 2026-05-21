; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_mload(ptr, i256)

declare void @evm_mstore(ptr, i256, i256)

declare void @evm_mstore8(ptr, i256, i256)

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

define void @public___function_selector___0x0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x0:
  call void @evm_mstore(ptr %mem, i256 64, i256 128), !notdec.evm !0
  %evm.sload = call i256 @evm_sload(i256 0), !notdec.evm !1
  %evm.and = and i256 %evm.sload, 1461501637330902918203684832716283019655932542975, !notdec.evm !2
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !3
  %evm.eq = icmp eq i256 %evm.calldataload, -40663485304543413038196519263804526123777591306660487573873822099608403181568, !notdec.evm !4
  %evm.bool = zext i1 %evm.eq to i256, !notdec.evm !4
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !5
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond, label %bb._0x50, label %bb._0x47, !notdec.evm !6

bb._0x50:                                         ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !7
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 0, i256 0, i256 %evm.calldatasize), !notdec.evm !8
  %evm.calldatasize2 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !9
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !10
  %evm.delegatecall = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 %evm.and, i256 0, i256 %evm.calldatasize2, i256 0, i256 0), !notdec.evm !11
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !12
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize), !notdec.evm !13
  %evm.eq3 = icmp eq i256 %evm.delegatecall, 0, !notdec.evm !14
  %evm.bool4 = zext i1 %evm.eq3 to i256, !notdec.evm !14
  %evm.iszero5 = icmp eq i256 %evm.bool4, 0, !notdec.evm !15
  %evm.bool6 = zext i1 %evm.iszero5 to i256, !notdec.evm !15
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !16
  br i1 %evm.branch.cond7, label %bb._0x70, label %bb._0x6c, !notdec.evm !16

bb._0x70:                                         ; preds = %bb._0x50
  %evm.returndatasize8 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !17
  call void @evm_return(ptr %mem, i256 0, i256 %evm.returndatasize8), !notdec.evm !18
  ret void, !notdec.evm !18

bb._0x6c:                                         ; preds = %bb._0x50
  %evm.returndatasize9 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !19
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize9), !notdec.evm !20
  unreachable, !notdec.evm !20

bb._0x47:                                         ; preds = %bb._0x0
  call void @evm_mstore(ptr %mem, i256 0, i256 %evm.and), !notdec.evm !21
  call void @evm_return(ptr %mem, i256 0, i256 32), !notdec.evm !22
  ret void, !notdec.evm !22
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x1c", !"op=SLOAD", !"evm.pc=0x1c"}
!2 = !{!"tac=0x1d", !"op=AND", !"evm.pc=0x1d"}
!3 = !{!"tac=0x41", !"op=CALLDATALOAD", !"evm.pc=0x41"}
!4 = !{!"tac=0x42", !"op=EQ", !"evm.pc=0x42"}
!5 = !{!"tac=0x43", !"op=ISZERO", !"evm.pc=0x43"}
!6 = !{!"tac=0x46", !"op=JUMPI", !"evm.pc=0x46"}
!7 = !{!"tac=0x51", !"op=CALLDATASIZE", !"evm.pc=0x51"}
!8 = !{!"tac=0x55", !"op=CALLDATACOPY", !"evm.pc=0x55"}
!9 = !{!"tac=0x59", !"op=CALLDATASIZE", !"evm.pc=0x59"}
!10 = !{!"tac=0x5d", !"op=GAS", !"evm.pc=0x5d"}
!11 = !{!"tac=0x5e", !"op=DELEGATECALL", !"evm.pc=0x5e"}
!12 = !{!"tac=0x5f", !"op=RETURNDATASIZE", !"evm.pc=0x5f"}
!13 = !{!"tac=0x63", !"op=RETURNDATACOPY", !"evm.pc=0x63"}
!14 = !{!"tac=0x67", !"op=EQ", !"evm.pc=0x67"}
!15 = !{!"tac=0x68", !"op=ISZERO", !"evm.pc=0x68"}
!16 = !{!"tac=0x6b", !"op=JUMPI", !"evm.pc=0x6b"}
!17 = !{!"tac=0x71", !"op=RETURNDATASIZE", !"evm.pc=0x71"}
!18 = !{!"tac=0x74", !"op=RETURN", !"evm.pc=0x74"}
!19 = !{!"tac=0x6c", !"op=RETURNDATASIZE", !"evm.pc=0x6c"}
!20 = !{!"tac=0x6f", !"op=REVERT", !"evm.pc=0x6f"}
!21 = !{!"tac=0x4a", !"op=MSTORE", !"evm.pc=0x4a"}
!22 = !{!"tac=0x4f", !"op=RETURN", !"evm.pc=0x4f"}
