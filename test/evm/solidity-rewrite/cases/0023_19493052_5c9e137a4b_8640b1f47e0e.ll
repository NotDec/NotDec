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
  %evm.branch.cond = icmp ne i256 %evm.calldatasize, 0, !notdec.evm !1
  br i1 %evm.branch.cond, label %bb._0x2c, label %bb._0x4, !notdec.evm !1

bb._0x2c:                                         ; preds = %bb._0x0
  %evm.calldatasize1 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !2
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !3
  %evm.returndatasize2 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !4
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 %evm.returndatasize2, i256 %evm.returndatasize, i256 %evm.calldatasize1), !notdec.evm !5
  %evm.returndatasize3 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !6
  %evm.returndatasize4 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !7
  %evm.returndatasize5 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !8
  %evm.returndatasize6 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !9
  %evm.calldatasize7 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !10
  call void @evm_codecopy(ptr %mem, ptr %env, i256 %evm.calldatasize7, i256 98, i256 49), !notdec.evm !11
  %evm.calldatasize8 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !12
  %evm.add = add i256 %evm.calldatasize8, 49, !notdec.evm !13
  %evm.returndatasize9 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !14
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !15
  %evm.delegatecall = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 386786030133392365705216726071433135171688965203, i256 %evm.returndatasize9, i256 %evm.add, i256 %evm.returndatasize6, i256 %evm.returndatasize5), !notdec.evm !16
  %evm.returndatasize10 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !17
  %evm.returndatasize11 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !18
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 %evm.returndatasize3, i256 %evm.returndatasize3, i256 %evm.returndatasize10), !notdec.evm !19
  %evm.branch.cond12 = icmp ne i256 %evm.delegatecall, 0, !notdec.evm !20
  br i1 %evm.branch.cond12, label %bb._0x60, label %bb._0x5f, !notdec.evm !20

bb._0x60:                                         ; preds = %bb._0x2c
  call void @evm_return(ptr %mem, i256 %evm.returndatasize4, i256 %evm.returndatasize11), !notdec.evm !21
  ret void, !notdec.evm !21

bb._0x5f:                                         ; preds = %bb._0x2c
  call void @evm_revert(ptr %mem, i256 %evm.returndatasize4, i256 %evm.returndatasize11), !notdec.evm !22
  unreachable, !notdec.evm !22

bb._0x4:                                          ; preds = %bb._0x0
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !23
  %evm.returndatasize13 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !24
  %notdec.evm.mem.ptr.0 = inttoptr i256 %evm.returndatasize13 to ptr
  store i256 %evm.callvalue, ptr %notdec.evm.mem.ptr.0, align 1, !notdec.evm !25
  %evm.msize = call i256 @evm_msize(ptr %mem), !notdec.evm !26
  %evm.returndatasize14 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !27
  call void @evm_log1(ptr %mem, i256 %evm.returndatasize14, i256 %evm.msize, i256 -44194564504436295181980612648397100322757857567031707332955818750814971871745), !notdec.evm !28
  ret void, !notdec.evm !29
}

!0 = !{!"tac=0x0", !"op=CALLDATASIZE", !"evm.pc=0x0"}
!1 = !{!"tac=0x3", !"op=JUMPI", !"evm.pc=0x3"}
!2 = !{!"tac=0x2d", !"op=CALLDATASIZE", !"evm.pc=0x2d"}
!3 = !{!"tac=0x2e", !"op=RETURNDATASIZE", !"evm.pc=0x2e"}
!4 = !{!"tac=0x2f", !"op=RETURNDATASIZE", !"evm.pc=0x2f"}
!5 = !{!"tac=0x30", !"op=CALLDATACOPY", !"evm.pc=0x30"}
!6 = !{!"tac=0x31", !"op=RETURNDATASIZE", !"evm.pc=0x31"}
!7 = !{!"tac=0x32", !"op=RETURNDATASIZE", !"evm.pc=0x32"}
!8 = !{!"tac=0x33", !"op=RETURNDATASIZE", !"evm.pc=0x33"}
!9 = !{!"tac=0x34", !"op=RETURNDATASIZE", !"evm.pc=0x34"}
!10 = !{!"tac=0x3b", !"op=CALLDATASIZE", !"evm.pc=0x3b"}
!11 = !{!"tac=0x3c", !"op=CODECOPY", !"evm.pc=0x3c"}
!12 = !{!"tac=0x3d", !"op=CALLDATASIZE", !"evm.pc=0x3d"}
!13 = !{!"tac=0x3e", !"op=ADD", !"evm.pc=0x3e"}
!14 = !{!"tac=0x3f", !"op=RETURNDATASIZE", !"evm.pc=0x3f"}
!15 = !{!"tac=0x55", !"op=GAS", !"evm.pc=0x55"}
!16 = !{!"tac=0x56", !"op=DELEGATECALL", !"evm.pc=0x56"}
!17 = !{!"tac=0x57", !"op=RETURNDATASIZE", !"evm.pc=0x57"}
!18 = !{!"tac=0x58", !"op=RETURNDATASIZE", !"evm.pc=0x58"}
!19 = !{!"tac=0x5b", !"op=RETURNDATACOPY", !"evm.pc=0x5b"}
!20 = !{!"tac=0x5e", !"op=JUMPI", !"evm.pc=0x5e"}
!21 = !{!"tac=0x61", !"op=RETURN", !"evm.pc=0x61"}
!22 = !{!"tac=0x5f", !"op=REVERT", !"evm.pc=0x5f"}
!23 = !{!"tac=0x4", !"op=CALLVALUE", !"evm.pc=0x4"}
!24 = !{!"tac=0x5", !"op=RETURNDATASIZE", !"evm.pc=0x5"}
!25 = !{!"tac=0x6", !"op=MSTORE", !"evm.pc=0x6"}
!26 = !{!"tac=0x28", !"op=MSIZE", !"evm.pc=0x28"}
!27 = !{!"tac=0x29", !"op=RETURNDATASIZE", !"evm.pc=0x29"}
!28 = !{!"tac=0x2a", !"op=LOG1", !"evm.pc=0x2a"}
!29 = !{!"tac=0x2b", !"op=STOP", !"evm.pc=0x2b"}

attributes #0 = { null_pointer_is_valid }
