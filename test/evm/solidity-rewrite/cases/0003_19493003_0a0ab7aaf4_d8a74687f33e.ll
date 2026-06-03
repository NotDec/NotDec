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

define void @public___function_selector___0x0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x0:
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !0
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !1
  %evm.returndatasize1 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !2
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 %evm.returndatasize1, i256 %evm.returndatasize, i256 %evm.calldatasize), !notdec.evm !3
  %evm.returndatasize2 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !4
  %evm.returndatasize3 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !5
  %evm.returndatasize4 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !6
  %evm.calldatasize5 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !7
  %evm.returndatasize6 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !8
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !9
  %evm.delegatecall = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 325992658475039433063722119161758467225641092336, i256 %evm.returndatasize6, i256 %evm.calldatasize5, i256 %evm.returndatasize4, i256 %evm.returndatasize3), !notdec.evm !10
  %evm.returndatasize7 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !11
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 %evm.returndatasize2, i256 %evm.returndatasize2, i256 %evm.returndatasize7), !notdec.evm !12
  %evm.returndatasize8 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !13
  %evm.branch.cond = icmp ne i256 %evm.delegatecall, 0, !notdec.evm !14
  br i1 %evm.branch.cond, label %bb._0x2b, label %bb._0x2a, !notdec.evm !14

bb._0x2b:                                         ; preds = %bb._0x0
  call void @evm_return(ptr %mem, i256 %evm.returndatasize2, i256 %evm.returndatasize8), !notdec.evm !15
  ret void, !notdec.evm !15

bb._0x2a:                                         ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 %evm.returndatasize2, i256 %evm.returndatasize8), !notdec.evm !16
  unreachable, !notdec.evm !16
}

!0 = !{!"tac=0x0", !"op=CALLDATASIZE", !"evm.pc=0x0"}
!1 = !{!"tac=0x1", !"op=RETURNDATASIZE", !"evm.pc=0x1"}
!2 = !{!"tac=0x2", !"op=RETURNDATASIZE", !"evm.pc=0x2"}
!3 = !{!"tac=0x3", !"op=CALLDATACOPY", !"evm.pc=0x3"}
!4 = !{!"tac=0x4", !"op=RETURNDATASIZE", !"evm.pc=0x4"}
!5 = !{!"tac=0x5", !"op=RETURNDATASIZE", !"evm.pc=0x5"}
!6 = !{!"tac=0x6", !"op=RETURNDATASIZE", !"evm.pc=0x6"}
!7 = !{!"tac=0x7", !"op=CALLDATASIZE", !"evm.pc=0x7"}
!8 = !{!"tac=0x8", !"op=RETURNDATASIZE", !"evm.pc=0x8"}
!9 = !{!"tac=0x1e", !"op=GAS", !"evm.pc=0x1e"}
!10 = !{!"tac=0x1f", !"op=DELEGATECALL", !"evm.pc=0x1f"}
!11 = !{!"tac=0x20", !"op=RETURNDATASIZE", !"evm.pc=0x20"}
!12 = !{!"tac=0x23", !"op=RETURNDATACOPY", !"evm.pc=0x23"}
!13 = !{!"tac=0x25", !"op=RETURNDATASIZE", !"evm.pc=0x25"}
!14 = !{!"tac=0x29", !"op=JUMPI", !"evm.pc=0x29"}
!15 = !{!"tac=0x2c", !"op=RETURN", !"evm.pc=0x2c"}
!16 = !{!"tac=0x2a", !"op=REVERT", !"evm.pc=0x2a"}

attributes #0 = { null_pointer_is_valid }
