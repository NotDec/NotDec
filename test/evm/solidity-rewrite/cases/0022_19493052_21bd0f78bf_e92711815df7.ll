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
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !1
  %evm.branch.cond = icmp ne i256 %evm.calldatasize, 0, !notdec.evm !2
  br i1 %evm.branch.cond, label %bb._0xa, label %bb._0x9, !notdec.evm !2

bb._0xa:                                          ; preds = %bb._0x0
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !3
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !4
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !4
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !5
  br i1 %evm.branch.cond1, label %bb._0x15, label %bb._0x11, !notdec.evm !5

bb._0x15:                                         ; preds = %bb._0xa
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !6
  %evm.calldatasize2 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !7
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 %evm.mload, i256 0, i256 %evm.calldatasize2), !notdec.evm !8
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !9
  %evm.delegatecall = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 735410655409923118487040654235217855302685300554, i256 %evm.mload, i256 %evm.calldatasize2, i256 0, i256 0), !notdec.evm !10
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !11
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 %evm.mload, i256 0, i256 %evm.returndatasize), !notdec.evm !12
  %evm.iszero3 = icmp eq i256 %evm.delegatecall, 0, !notdec.evm !13
  %evm.bool4 = zext i1 %evm.iszero3 to i256, !notdec.evm !13
  %evm.branch.cond5 = icmp ne i256 %evm.bool4, 0, !notdec.evm !14
  br i1 %evm.branch.cond5, label %bb._0x5b, label %bb._0x58, !notdec.evm !14

bb._0x5b:                                         ; preds = %bb._0x15
  call void @evm_revert(ptr %mem, i256 %evm.mload, i256 %evm.returndatasize), !notdec.evm !15
  unreachable, !notdec.evm !15

bb._0x58:                                         ; preds = %bb._0x15
  call void @evm_return(ptr %mem, i256 %evm.mload, i256 %evm.returndatasize), !notdec.evm !16
  ret void, !notdec.evm !16

bb._0x11:                                         ; preds = %bb._0xa
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !17
  unreachable, !notdec.evm !17

bb._0x9:                                          ; preds = %bb._0x0
  ret void, !notdec.evm !18
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLDATASIZE", !"evm.pc=0x5"}
!2 = !{!"tac=0x8", !"op=JUMPI", !"evm.pc=0x8"}
!3 = !{!"tac=0xb", !"op=CALLVALUE", !"evm.pc=0xb"}
!4 = !{!"tac=0xd", !"op=ISZERO", !"evm.pc=0xd"}
!5 = !{!"tac=0x10", !"op=JUMPI", !"evm.pc=0x10"}
!6 = !{!"tac=0x19", !"op=MLOAD", !"evm.pc=0x19"}
!7 = !{!"tac=0x3c", !"op=CALLDATASIZE", !"evm.pc=0x3c"}
!8 = !{!"tac=0x41", !"op=CALLDATACOPY", !"evm.pc=0x41"}
!9 = !{!"tac=0x48", !"op=GAS", !"evm.pc=0x48"}
!10 = !{!"tac=0x49", !"op=DELEGATECALL", !"evm.pc=0x49"}
!11 = !{!"tac=0x4a", !"op=RETURNDATASIZE", !"evm.pc=0x4a"}
!12 = !{!"tac=0x51", !"op=RETURNDATACOPY", !"evm.pc=0x51"}
!13 = !{!"tac=0x54", !"op=ISZERO", !"evm.pc=0x54"}
!14 = !{!"tac=0x57", !"op=JUMPI", !"evm.pc=0x57"}
!15 = !{!"tac=0x5e", !"op=REVERT", !"evm.pc=0x5e"}
!16 = !{!"tac=0x5a", !"op=RETURN", !"evm.pc=0x5a"}
!17 = !{!"tac=0x14", !"op=REVERT", !"evm.pc=0x14"}
!18 = !{!"tac=0x9", !"op=STOP", !"evm.pc=0x9"}
