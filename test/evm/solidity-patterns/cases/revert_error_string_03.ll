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
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !1
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !2
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !2
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !3
  br i1 %evm.branch.cond, label %bb._0xe, label %bb._0xb, !notdec.evm !3

bb._0xe:                                          ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.lt to i256, !notdec.evm !5
  %evm.branch.cond2 = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond2, label %bb._0x15be, label %bb._0x17, !notdec.evm !6

bb._0x17:                                         ; preds = %bb._0xe
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 2755982825, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x1fbe, label %bb._0x15be, !notdec.evm !10

bb._0x1fbe:                                       ; preds = %bb._0x17
  call void @public_run_uint256__0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x15be:                                       ; preds = %bb._0x17, %bb._0xe
  call void @public_fallback___0x26(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !13
  unreachable, !notdec.evm !13
}

define void @public_fallback___0x26(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x26:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !14
  unreachable, !notdec.evm !14
}

define void @public_run_uint256__0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x2a:
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !15
  br label %bb._0x84, !notdec.evm !16

bb._0x84:                                         ; preds = %bb._0x2a
  %evm.sub = sub i256 %evm.calldatasize, 4, !notdec.evm !17
  %evm.slt = icmp slt i256 %evm.sub, 32, !notdec.evm !18
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !18
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !19
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !19
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !20
  br i1 %evm.branch.cond, label %bb._0x93, label %bb._0x90, !notdec.evm !20

bb._0x93:                                         ; preds = %bb._0x84
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 4), !notdec.evm !21
  br label %bb._0x35, !notdec.evm !22

bb._0x35:                                         ; preds = %bb._0x93
  br label %bb._0x3bB0x35, !notdec.evm !23

bb._0x3bB0x35:                                    ; preds = %bb._0x35
  %evm.iszero2 = icmp eq i256 %evm.calldataload, 0, !notdec.evm !24
  %evm.bool3 = zext i1 %evm.iszero2 to i256, !notdec.evm !24
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !25
  br i1 %evm.branch.cond4, label %bb._0x81B0x35, label %bb._0x41B0x35, !notdec.evm !25

bb._0x81B0x35:                                    ; preds = %bb._0x3bB0x35
  br label %bb._0x39, !notdec.evm !26

bb._0x39:                                         ; preds = %bb._0x81B0x35
  ret void, !notdec.evm !27

bb._0x41B0x35:                                    ; preds = %bb._0x3bB0x35
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !28
  %evm.shl = call i256 @evm_shl(i256 229, i256 4594637), !notdec.evm !29
  call void @evm_mstore(ptr %mem, i256 %evm.mload, i256 %evm.shl), !notdec.evm !30
  %evm.add = add i256 %evm.mload, 4, !notdec.evm !31
  call void @evm_mstore(ptr %mem, i256 %evm.add, i256 32), !notdec.evm !32
  %evm.add5 = add i256 %evm.mload, 36, !notdec.evm !33
  call void @evm_mstore(ptr %mem, i256 %evm.add5, i256 18), !notdec.evm !34
  %evm.shl6 = call i256 @evm_shl(i256 112, i256 10312401295712409901771554521977393277923951), !notdec.evm !35
  %evm.add7 = add i256 %evm.mload, 68, !notdec.evm !36
  call void @evm_mstore(ptr %mem, i256 %evm.add7, i256 %evm.shl6), !notdec.evm !37
  %evm.add8 = add i256 100, %evm.mload, !notdec.evm !38
  %evm.mload9 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !39
  %evm.sub10 = sub i256 %evm.add8, %evm.mload9, !notdec.evm !40
  call void @evm_revert(ptr %mem, i256 %evm.mload9, i256 %evm.sub10), !notdec.evm !41
  unreachable, !notdec.evm !41

bb._0x90:                                         ; preds = %bb._0x84
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !42
  unreachable, !notdec.evm !42
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x12", !"op=CALLDATASIZE", !"evm.pc=0x12"}
!5 = !{!"tac=0x13", !"op=LT", !"evm.pc=0x13"}
!6 = !{!"tac=0x1de", !"op=JUMPI", !"evm.pc=0x14"}
!7 = !{!"tac=0x18", !"op=CALLDATALOAD", !"evm.pc=0x18"}
!8 = !{!"tac=0x1b", !"op=SHR", !"evm.pc=0x1b"}
!9 = !{!"tac=0x22", !"op=EQ", !"evm.pc=0x22"}
!10 = !{!"tac=0xbde", !"op=JUMPI", !"evm.pc=0x23"}
!11 = !{!"tac=0x1ffe", !"op=CALLPRIVATE", !"evm.pc=0x2a"}
!12 = !{!"tac=0x15fe", !"op=CALLPRIVATE", !"evm.pc=0x26"}
!13 = !{!"tac=0xd", !"op=REVERT", !"evm.pc=0xd"}
!14 = !{!"tac=0x29", !"op=REVERT", !"evm.pc=0x29"}
!15 = !{!"tac=0x2f", !"op=CALLDATASIZE", !"evm.pc=0x2f"}
!16 = !{!"tac=0x34", !"op=JUMP", !"evm.pc=0x34"}
!17 = !{!"tac=0x8a", !"op=SUB", !"evm.pc=0x8a"}
!18 = !{!"tac=0x8b", !"op=SLT", !"evm.pc=0x8b"}
!19 = !{!"tac=0x8c", !"op=ISZERO", !"evm.pc=0x8c"}
!20 = !{!"tac=0x8f", !"op=JUMPI", !"evm.pc=0x8f"}
!21 = !{!"tac=0x95", !"op=CALLDATALOAD", !"evm.pc=0x95"}
!22 = !{!"tac=0x99", !"op=JUMP", !"evm.pc=0x99"}
!23 = !{!"tac=0x38", !"op=JUMP", !"evm.pc=0x38"}
!24 = !{!"tac=0x3dS0x35", !"op=ISZERO", !"evm.pc=0x38,0x3d", !"inline=[0x3b, nil]"}
!25 = !{!"tac=0x40S0x35", !"op=JUMPI", !"evm.pc=0x38,0x40", !"inline=[0x3b, nil]"}
!26 = !{!"tac=0x83S0x35", !"op=JUMP", !"evm.pc=0x38,0x83", !"inline=[0x3b, nil]"}
!27 = !{!"tac=0x3a", !"op=STOP", !"evm.pc=0x3a"}
!28 = !{!"tac=0x43S0x35", !"op=MLOAD", !"evm.pc=0x38,0x43", !"inline=[0x3b, nil]"}
!29 = !{!"tac=0x4aS0x35", !"op=SHL", !"evm.pc=0x38,0x4a", !"inline=[0x3b, nil]"}
!30 = !{!"tac=0x4cS0x35", !"op=MSTORE", !"evm.pc=0x38,0x4c", !"inline=[0x3b, nil]"}
!31 = !{!"tac=0x52S0x35", !"op=ADD", !"evm.pc=0x38,0x52", !"inline=[0x3b, nil]"}
!32 = !{!"tac=0x53S0x35", !"op=MSTORE", !"evm.pc=0x38,0x53", !"inline=[0x3b, nil]"}
!33 = !{!"tac=0x59S0x35", !"op=ADD", !"evm.pc=0x38,0x59", !"inline=[0x3b, nil]"}
!34 = !{!"tac=0x5aS0x35", !"op=MSTORE", !"evm.pc=0x38,0x5a", !"inline=[0x3b, nil]"}
!35 = !{!"tac=0x70S0x35", !"op=SHL", !"evm.pc=0x38,0x70", !"inline=[0x3b, nil]"}
!36 = !{!"tac=0x74S0x35", !"op=ADD", !"evm.pc=0x38,0x74", !"inline=[0x3b, nil]"}
!37 = !{!"tac=0x75S0x35", !"op=MSTORE", !"evm.pc=0x38,0x75", !"inline=[0x3b, nil]"}
!38 = !{!"tac=0x78S0x35", !"op=ADD", !"evm.pc=0x38,0x78", !"inline=[0x3b, nil]"}
!39 = !{!"tac=0x7bS0x35", !"op=MLOAD", !"evm.pc=0x38,0x7b", !"inline=[0x3b, nil]"}
!40 = !{!"tac=0x7eS0x35", !"op=SUB", !"evm.pc=0x38,0x7e", !"inline=[0x3b, nil]"}
!41 = !{!"tac=0x80S0x35", !"op=REVERT", !"evm.pc=0x38,0x80", !"inline=[0x3b, nil]"}
!42 = !{!"tac=0x92", !"op=REVERT", !"evm.pc=0x92"}
