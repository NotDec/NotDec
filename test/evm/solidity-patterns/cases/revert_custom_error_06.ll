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
  br i1 %evm.branch.cond2, label %bb._0x159a, label %bb._0x17, !notdec.evm !6

bb._0x17:                                         ; preds = %bb._0xe
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 376056533, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x1f9a, label %bb._0x159a, !notdec.evm !10

bb._0x1f9a:                                       ; preds = %bb._0x17
  call void @public__0x166a2ad5_0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x159a:                                       ; preds = %bb._0x17, %bb._0xe
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

define void @public__0x166a2ad5_0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x2a:
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !15
  br label %bb._0x64B0x2a, !notdec.evm !16

bb._0x64B0x2a:                                    ; preds = %bb._0x2a
  %evm.sub = sub i256 %evm.calldatasize, 4, !notdec.evm !17
  %evm.slt = icmp slt i256 %evm.sub, 32, !notdec.evm !18
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !18
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !19
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !19
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !20
  br i1 %evm.branch.cond, label %bb._0x73B0x2a, label %bb._0x70B0x2a, !notdec.evm !20

bb._0x73B0x2a:                                    ; preds = %bb._0x64B0x2a
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 4), !notdec.evm !21
  %evm.iszero2 = icmp eq i256 %evm.calldataload, 0, !notdec.evm !22
  %evm.bool3 = zext i1 %evm.iszero2 to i256, !notdec.evm !22
  %evm.iszero4 = icmp eq i256 %evm.bool3, 0, !notdec.evm !23
  %evm.bool5 = zext i1 %evm.iszero4 to i256, !notdec.evm !23
  %evm.eq = icmp eq i256 %evm.calldataload, %evm.bool5, !notdec.evm !24
  %evm.bool6 = zext i1 %evm.eq to i256, !notdec.evm !24
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !25
  br i1 %evm.branch.cond7, label %bb._0x81B0x2a, label %bb._0x7eB0x2a, !notdec.evm !25

bb._0x81B0x2a:                                    ; preds = %bb._0x73B0x2a
  br label %bb._0x35, !notdec.evm !26

bb._0x35:                                         ; preds = %bb._0x81B0x2a
  br label %bb._0x3bB0x35, !notdec.evm !27

bb._0x3bB0x35:                                    ; preds = %bb._0x35
  %evm.iszero8 = icmp eq i256 %evm.calldataload, 0, !notdec.evm !28
  %evm.bool9 = zext i1 %evm.iszero8 to i256, !notdec.evm !28
  %evm.branch.cond10 = icmp ne i256 %evm.bool9, 0, !notdec.evm !29
  br i1 %evm.branch.cond10, label %bb._0x61B0x35, label %bb._0x41B0x35, !notdec.evm !29

bb._0x61B0x35:                                    ; preds = %bb._0x3bB0x35
  br label %bb._0x39, !notdec.evm !30

bb._0x39:                                         ; preds = %bb._0x61B0x35
  ret void, !notdec.evm !31

bb._0x41B0x35:                                    ; preds = %bb._0x3bB0x35
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !32
  %evm.shl = call i256 @evm_shl(i256 224, i256 1076066811), !notdec.evm !33
  call void @evm_mstore(ptr %mem, i256 %evm.mload, i256 %evm.shl), !notdec.evm !34
  %evm.iszero11 = icmp eq i256 %evm.calldataload, 0, !notdec.evm !35
  %evm.bool12 = zext i1 %evm.iszero11 to i256, !notdec.evm !35
  %evm.iszero13 = icmp eq i256 %evm.bool12, 0, !notdec.evm !36
  %evm.bool14 = zext i1 %evm.iszero13 to i256, !notdec.evm !36
  %evm.add = add i256 %evm.mload, 4, !notdec.evm !37
  call void @evm_mstore(ptr %mem, i256 %evm.add, i256 %evm.bool14), !notdec.evm !38
  %evm.add15 = add i256 36, %evm.mload, !notdec.evm !39
  %evm.mload16 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !40
  %evm.sub17 = sub i256 %evm.add15, %evm.mload16, !notdec.evm !41
  call void @evm_revert(ptr %mem, i256 %evm.mload16, i256 %evm.sub17), !notdec.evm !42
  unreachable, !notdec.evm !42

bb._0x7eB0x2a:                                    ; preds = %bb._0x73B0x2a
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !43
  unreachable, !notdec.evm !43

bb._0x70B0x2a:                                    ; preds = %bb._0x64B0x2a
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !44
  unreachable, !notdec.evm !44
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x12", !"op=CALLDATASIZE", !"evm.pc=0x12"}
!5 = !{!"tac=0x13", !"op=LT", !"evm.pc=0x13"}
!6 = !{!"tac=0x1ba", !"op=JUMPI", !"evm.pc=0x14"}
!7 = !{!"tac=0x18", !"op=CALLDATALOAD", !"evm.pc=0x18"}
!8 = !{!"tac=0x1b", !"op=SHR", !"evm.pc=0x1b"}
!9 = !{!"tac=0x22", !"op=EQ", !"evm.pc=0x22"}
!10 = !{!"tac=0xbba", !"op=JUMPI", !"evm.pc=0x23"}
!11 = !{!"tac=0x1fda", !"op=CALLPRIVATE", !"evm.pc=0x2a"}
!12 = !{!"tac=0x15da", !"op=CALLPRIVATE", !"evm.pc=0x26"}
!13 = !{!"tac=0xd", !"op=REVERT", !"evm.pc=0xd"}
!14 = !{!"tac=0x29", !"op=REVERT", !"evm.pc=0x29"}
!15 = !{!"tac=0x2f", !"op=CALLDATASIZE", !"evm.pc=0x2f"}
!16 = !{!"tac=0x34", !"op=JUMP", !"evm.pc=0x34"}
!17 = !{!"tac=0x6aS0x2a", !"op=SUB", !"evm.pc=0x34,0x6a", !"inline=[0x64, nil]"}
!18 = !{!"tac=0x6bS0x2a", !"op=SLT", !"evm.pc=0x34,0x6b", !"inline=[0x64, nil]"}
!19 = !{!"tac=0x6cS0x2a", !"op=ISZERO", !"evm.pc=0x34,0x6c", !"inline=[0x64, nil]"}
!20 = !{!"tac=0x6fS0x2a", !"op=JUMPI", !"evm.pc=0x34,0x6f", !"inline=[0x64, nil]"}
!21 = !{!"tac=0x75S0x2a", !"op=CALLDATALOAD", !"evm.pc=0x34,0x75", !"inline=[0x64, nil]"}
!22 = !{!"tac=0x77S0x2a", !"op=ISZERO", !"evm.pc=0x34,0x77", !"inline=[0x64, nil]"}
!23 = !{!"tac=0x78S0x2a", !"op=ISZERO", !"evm.pc=0x34,0x78", !"inline=[0x64, nil]"}
!24 = !{!"tac=0x7aS0x2a", !"op=EQ", !"evm.pc=0x34,0x7a", !"inline=[0x64, nil]"}
!25 = !{!"tac=0x7dS0x2a", !"op=JUMPI", !"evm.pc=0x34,0x7d", !"inline=[0x64, nil]"}
!26 = !{!"tac=0x87S0x2a", !"op=JUMP", !"evm.pc=0x34,0x87", !"inline=[0x64, nil]"}
!27 = !{!"tac=0x38", !"op=JUMP", !"evm.pc=0x38"}
!28 = !{!"tac=0x3dS0x35", !"op=ISZERO", !"evm.pc=0x38,0x3d", !"inline=[0x3b, nil]"}
!29 = !{!"tac=0x40S0x35", !"op=JUMPI", !"evm.pc=0x38,0x40", !"inline=[0x3b, nil]"}
!30 = !{!"tac=0x63S0x35", !"op=JUMP", !"evm.pc=0x38,0x63", !"inline=[0x3b, nil]"}
!31 = !{!"tac=0x3a", !"op=STOP", !"evm.pc=0x3a"}
!32 = !{!"tac=0x43S0x35", !"op=MLOAD", !"evm.pc=0x38,0x43", !"inline=[0x3b, nil]"}
!33 = !{!"tac=0x4bS0x35", !"op=SHL", !"evm.pc=0x38,0x4b", !"inline=[0x3b, nil]"}
!34 = !{!"tac=0x4dS0x35", !"op=MSTORE", !"evm.pc=0x38,0x4d", !"inline=[0x3b, nil]"}
!35 = !{!"tac=0x4fS0x35", !"op=ISZERO", !"evm.pc=0x38,0x4f", !"inline=[0x3b, nil]"}
!36 = !{!"tac=0x50S0x35", !"op=ISZERO", !"evm.pc=0x38,0x50", !"inline=[0x3b, nil]"}
!37 = !{!"tac=0x54S0x35", !"op=ADD", !"evm.pc=0x38,0x54", !"inline=[0x3b, nil]"}
!38 = !{!"tac=0x55S0x35", !"op=MSTORE", !"evm.pc=0x38,0x55", !"inline=[0x3b, nil]"}
!39 = !{!"tac=0x58S0x35", !"op=ADD", !"evm.pc=0x38,0x58", !"inline=[0x3b, nil]"}
!40 = !{!"tac=0x5bS0x35", !"op=MLOAD", !"evm.pc=0x38,0x5b", !"inline=[0x3b, nil]"}
!41 = !{!"tac=0x5eS0x35", !"op=SUB", !"evm.pc=0x38,0x5e", !"inline=[0x3b, nil]"}
!42 = !{!"tac=0x60S0x35", !"op=REVERT", !"evm.pc=0x38,0x60", !"inline=[0x3b, nil]"}
!43 = !{!"tac=0x80S0x2a", !"op=REVERT", !"evm.pc=0x34,0x80", !"inline=[0x64, nil]"}
!44 = !{!"tac=0x72S0x2a", !"op=REVERT", !"evm.pc=0x34,0x72", !"inline=[0x64, nil]"}
