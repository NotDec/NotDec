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
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !0
  %evm.returndatasize1 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !1
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !2
  %evm.eq = icmp eq i256 536032175862749073745621066030491056922849008355, %evm.caller, !notdec.evm !3
  %evm.bool = zext i1 %evm.eq to i256, !notdec.evm !3
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !4
  br i1 %evm.branch.cond, label %bb._0x57, label %bb._0x1c, !notdec.evm !4

bb._0x57:                                         ; preds = %bb._0x0
  %evm.returndatasize2 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !5
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 %evm.returndatasize2), !notdec.evm !6
  %evm.calldataload3 = call i256 @evm_calldataload(ptr %calldata, i256 32), !notdec.evm !7
  call void @evm_sstore(i256 %evm.calldataload3, i256 %evm.calldataload), !notdec.evm !8
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !9
  %evm.gt = icmp ugt i256 %evm.calldatasize, 64, !notdec.evm !10
  %evm.bool4 = zext i1 %evm.gt to i256, !notdec.evm !10
  %evm.iszero = icmp eq i256 %evm.bool4, 0, !notdec.evm !11
  %evm.bool5 = zext i1 %evm.iszero to i256, !notdec.evm !11
  %evm.branch.cond6 = icmp ne i256 %evm.bool5, 0, !notdec.evm !12
  br i1 %evm.branch.cond6, label %bb._0x52, label %bb._0x67, !notdec.evm !12

bb._0x67:                                         ; preds = %bb._0x57
  %evm.calldatasize7 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !13
  %evm.sub = sub i256 %evm.calldatasize7, 64, !notdec.evm !14
  %evm.returndatasize8 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !15
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 %evm.returndatasize8, i256 64, i256 %evm.sub), !notdec.evm !16
  %evm.returndatasize9 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !17
  %evm.returndatasize10 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !18
  %evm.calldataload11 = call i256 @evm_calldataload(ptr %calldata, i256 %evm.returndatasize10), !notdec.evm !19
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !20
  %evm.delegatecall = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 %evm.calldataload11, i256 %evm.returndatasize9, i256 %evm.sub, i256 %evm.returndatasize1, i256 %evm.returndatasize), !notdec.evm !21
  %evm.returndatasize12 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !22
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize12), !notdec.evm !23
  %evm.branch.cond13 = icmp ne i256 %evm.delegatecall, 0, !notdec.evm !24
  br i1 %evm.branch.cond13, label %bb._0x52, label %bb._0x7b, !notdec.evm !24

bb._0x7b:                                         ; preds = %bb._0x67
  %evm.returndatasize14 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !25
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize14), !notdec.evm !26
  unreachable, !notdec.evm !26

bb._0x1c:                                         ; preds = %bb._0x0
  %evm.calldatasize15 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !27
  %evm.returndatasize16 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !28
  %evm.returndatasize17 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !29
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 %evm.returndatasize17, i256 %evm.returndatasize16, i256 %evm.calldatasize15), !notdec.evm !30
  %evm.calldatasize18 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !31
  %evm.returndatasize19 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !32
  %evm.sload = call i256 @evm_sload(i256 24440054405305269366569402256811496959409073762505157381672968839269610695612), !notdec.evm !33
  %evm.gas20 = call i256 @evm_gas(ptr %env), !notdec.evm !34
  %evm.delegatecall21 = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas20, i256 %evm.sload, i256 %evm.returndatasize19, i256 %evm.calldatasize18, i256 %evm.returndatasize1, i256 %evm.returndatasize), !notdec.evm !35
  %evm.returndatasize22 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !36
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize22), !notdec.evm !37
  %evm.branch.cond23 = icmp ne i256 %evm.delegatecall21, 0, !notdec.evm !38
  br i1 %evm.branch.cond23, label %bb._0x52, label %bb._0x4e, !notdec.evm !38

bb._0x52:                                         ; preds = %bb._0x1c, %bb._0x67, %bb._0x57
  %evm.returndatasize24 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !39
  call void @evm_return(ptr %mem, i256 0, i256 %evm.returndatasize24), !notdec.evm !40
  ret void, !notdec.evm !40

bb._0x4e:                                         ; preds = %bb._0x1c
  %evm.returndatasize25 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !41
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize25), !notdec.evm !42
  unreachable, !notdec.evm !42
}

!0 = !{!"tac=0x0", !"op=RETURNDATASIZE", !"evm.pc=0x0"}
!1 = !{!"tac=0x1", !"op=RETURNDATASIZE", !"evm.pc=0x1"}
!2 = !{!"tac=0x2", !"op=CALLER", !"evm.pc=0x2"}
!3 = !{!"tac=0x18", !"op=EQ", !"evm.pc=0x18"}
!4 = !{!"tac=0x1b", !"op=JUMPI", !"evm.pc=0x1b"}
!5 = !{!"tac=0x58", !"op=RETURNDATASIZE", !"evm.pc=0x58"}
!6 = !{!"tac=0x59", !"op=CALLDATALOAD", !"evm.pc=0x59"}
!7 = !{!"tac=0x5c", !"op=CALLDATALOAD", !"evm.pc=0x5c"}
!8 = !{!"tac=0x5d", !"op=SSTORE", !"evm.pc=0x5d"}
!9 = !{!"tac=0x61", !"op=CALLDATASIZE", !"evm.pc=0x61"}
!10 = !{!"tac=0x62", !"op=GT", !"evm.pc=0x62"}
!11 = !{!"tac=0x63", !"op=ISZERO", !"evm.pc=0x63"}
!12 = !{!"tac=0x66", !"op=JUMPI", !"evm.pc=0x66"}
!13 = !{!"tac=0x67", !"op=CALLDATASIZE", !"evm.pc=0x67"}
!14 = !{!"tac=0x68", !"op=SUB", !"evm.pc=0x68"}
!15 = !{!"tac=0x6c", !"op=RETURNDATASIZE", !"evm.pc=0x6c"}
!16 = !{!"tac=0x6d", !"op=CALLDATACOPY", !"evm.pc=0x6d"}
!17 = !{!"tac=0x6e", !"op=RETURNDATASIZE", !"evm.pc=0x6e"}
!18 = !{!"tac=0x6f", !"op=RETURNDATASIZE", !"evm.pc=0x6f"}
!19 = !{!"tac=0x70", !"op=CALLDATALOAD", !"evm.pc=0x70"}
!20 = !{!"tac=0x71", !"op=GAS", !"evm.pc=0x71"}
!21 = !{!"tac=0x72", !"op=DELEGATECALL", !"evm.pc=0x72"}
!22 = !{!"tac=0x73", !"op=RETURNDATASIZE", !"evm.pc=0x73"}
!23 = !{!"tac=0x77", !"op=RETURNDATACOPY", !"evm.pc=0x77"}
!24 = !{!"tac=0x7a", !"op=JUMPI", !"evm.pc=0x7a"}
!25 = !{!"tac=0x7b", !"op=RETURNDATASIZE", !"evm.pc=0x7b"}
!26 = !{!"tac=0x7e", !"op=REVERT", !"evm.pc=0x7e"}
!27 = !{!"tac=0x1c", !"op=CALLDATASIZE", !"evm.pc=0x1c"}
!28 = !{!"tac=0x1d", !"op=RETURNDATASIZE", !"evm.pc=0x1d"}
!29 = !{!"tac=0x1e", !"op=RETURNDATASIZE", !"evm.pc=0x1e"}
!30 = !{!"tac=0x1f", !"op=CALLDATACOPY", !"evm.pc=0x1f"}
!31 = !{!"tac=0x20", !"op=CALLDATASIZE", !"evm.pc=0x20"}
!32 = !{!"tac=0x21", !"op=RETURNDATASIZE", !"evm.pc=0x21"}
!33 = !{!"tac=0x43", !"op=SLOAD", !"evm.pc=0x43"}
!34 = !{!"tac=0x44", !"op=GAS", !"evm.pc=0x44"}
!35 = !{!"tac=0x45", !"op=DELEGATECALL", !"evm.pc=0x45"}
!36 = !{!"tac=0x46", !"op=RETURNDATASIZE", !"evm.pc=0x46"}
!37 = !{!"tac=0x4a", !"op=RETURNDATACOPY", !"evm.pc=0x4a"}
!38 = !{!"tac=0x4d", !"op=JUMPI", !"evm.pc=0x4d"}
!39 = !{!"tac=0x53", !"op=RETURNDATASIZE", !"evm.pc=0x53"}
!40 = !{!"tac=0x56", !"op=RETURN", !"evm.pc=0x56"}
!41 = !{!"tac=0x4e", !"op=RETURNDATASIZE", !"evm.pc=0x4e"}
!42 = !{!"tac=0x51", !"op=REVERT", !"evm.pc=0x51"}

attributes #0 = { null_pointer_is_valid }
