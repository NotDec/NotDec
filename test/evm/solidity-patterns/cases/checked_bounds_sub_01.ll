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
  br i1 %evm.branch.cond, label %bb._0xf, label %bb._0xb, !notdec.evm !3

bb._0xf:                                          ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.lt to i256, !notdec.evm !5
  %evm.branch.cond2 = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond2, label %bb._0x1c04, label %bb._0x18, !notdec.evm !6

bb._0x18:                                         ; preds = %bb._0xf
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 3061675973, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x2604, label %bb._0x1c04, !notdec.evm !10

bb._0x2604:                                       ; preds = %bb._0x18
  call void @public_sub_uint256_uint256__0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x1c04:                                       ; preds = %bb._0x18, %bb._0xf
  call void @public_fallback___0x28(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !13
  unreachable, !notdec.evm !13
}

define void @public_fallback___0x28(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x28:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !14
  unreachable, !notdec.evm !14
}

define void @public_sub_uint256_uint256__0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x2d:
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !15
  br label %bb._0x61, !notdec.evm !16

bb._0x61:                                         ; preds = %bb._0x2d
  %evm.sub = sub i256 %evm.calldatasize, 4, !notdec.evm !17
  %evm.slt = icmp slt i256 %evm.sub, 64, !notdec.evm !18
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !18
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !19
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !19
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !20
  br i1 %evm.branch.cond, label %bb._0x73, label %bb._0x6f, !notdec.evm !20

bb._0x73:                                         ; preds = %bb._0x61
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 4), !notdec.evm !21
  %evm.calldataload2 = call i256 @evm_calldataload(ptr %calldata, i256 36), !notdec.evm !22
  br label %bb._0x38, !notdec.evm !23

bb._0x38:                                         ; preds = %bb._0x73
  br label %bb._0x4eB0x38, !notdec.evm !24

bb._0x4eB0x38:                                    ; preds = %bb._0x38
  br label %bb._0x82B0x4eB0x38, !notdec.evm !25

bb._0x82B0x4eB0x38:                               ; preds = %bb._0x4eB0x38
  %evm.sub3 = sub i256 %evm.calldataload, %evm.calldataload2, !notdec.evm !26
  %evm.gt = icmp ugt i256 %evm.sub3, %evm.calldataload, !notdec.evm !27
  %evm.bool4 = zext i1 %evm.gt to i256, !notdec.evm !27
  %evm.iszero5 = icmp eq i256 %evm.bool4, 0, !notdec.evm !28
  %evm.bool6 = zext i1 %evm.iszero5 to i256, !notdec.evm !28
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !29
  br i1 %evm.branch.cond7, label %bb._0x3c8B0x4eB0x38, label %bb._0x8dB0x4eB0x38, !notdec.evm !29

bb._0x3c8B0x4eB0x38:                              ; preds = %bb._0x82B0x4eB0x38
  br label %bb._0x58B0x38, !notdec.evm !30

bb._0x58B0x38:                                    ; preds = %bb._0x3c8B0x4eB0x38
  br label %bb._0x3edB0x38, !notdec.evm !31

bb._0x3edB0x38:                                   ; preds = %bb._0x58B0x38
  br label %bb._0x3c, !notdec.evm !32

bb._0x3c:                                         ; preds = %bb._0x3edB0x38
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !33
  call void @evm_mstore(ptr %mem, i256 %evm.mload, i256 %evm.sub3), !notdec.evm !34
  %evm.add = add i256 32, %evm.mload, !notdec.evm !35
  %evm.mload8 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !36
  %evm.sub9 = sub i256 %evm.add, %evm.mload8, !notdec.evm !37
  call void @evm_return(ptr %mem, i256 %evm.mload8, i256 %evm.sub9), !notdec.evm !38
  ret void, !notdec.evm !38

bb._0x8dB0x4eB0x38:                               ; preds = %bb._0x82B0x4eB0x38
  %evm.shl = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !39
  call void @evm_mstore(ptr %mem, i256 0, i256 %evm.shl), !notdec.evm !40
  call void @evm_mstore(ptr %mem, i256 4, i256 17), !notdec.evm !41
  call void @evm_revert(ptr %mem, i256 0, i256 36), !notdec.evm !42
  unreachable, !notdec.evm !42

bb._0x6f:                                         ; preds = %bb._0x61
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !43
  unreachable, !notdec.evm !43
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x13", !"op=CALLDATASIZE", !"evm.pc=0x13"}
!5 = !{!"tac=0x14", !"op=LT", !"evm.pc=0x14"}
!6 = !{!"tac=0x824", !"op=JUMPI", !"evm.pc=0x15"}
!7 = !{!"tac=0x1a", !"op=CALLDATALOAD", !"evm.pc=0x1a"}
!8 = !{!"tac=0x1d", !"op=SHR", !"evm.pc=0x1d"}
!9 = !{!"tac=0x24", !"op=EQ", !"evm.pc=0x24"}
!10 = !{!"tac=0x1224", !"op=JUMPI", !"evm.pc=0x25"}
!11 = !{!"tac=0x2644", !"op=CALLPRIVATE", !"evm.pc=0x2d"}
!12 = !{!"tac=0x1c44", !"op=CALLPRIVATE", !"evm.pc=0x28"}
!13 = !{!"tac=0xe", !"op=REVERT", !"evm.pc=0xe"}
!14 = !{!"tac=0x2c", !"op=REVERT", !"evm.pc=0x2c"}
!15 = !{!"tac=0x32", !"op=CALLDATASIZE", !"evm.pc=0x32"}
!16 = !{!"tac=0x37", !"op=JUMP", !"evm.pc=0x37"}
!17 = !{!"tac=0x69", !"op=SUB", !"evm.pc=0x69"}
!18 = !{!"tac=0x6a", !"op=SLT", !"evm.pc=0x6a"}
!19 = !{!"tac=0x6b", !"op=ISZERO", !"evm.pc=0x6b"}
!20 = !{!"tac=0x6e", !"op=JUMPI", !"evm.pc=0x6e"}
!21 = !{!"tac=0x77", !"op=CALLDATALOAD", !"evm.pc=0x77"}
!22 = !{!"tac=0x7e", !"op=CALLDATALOAD", !"evm.pc=0x7e"}
!23 = !{!"tac=0x81", !"op=JUMP", !"evm.pc=0x81"}
!24 = !{!"tac=0x3b", !"op=JUMP", !"evm.pc=0x3b"}
!25 = !{!"tac=0x57S0x38", !"op=JUMP", !"evm.pc=0x3b,0x57", !"inline=[0x4e, nil]"}
!26 = !{!"tac=0x85S0x4eS0x38", !"op=SUB", !"evm.pc=0x3b,0x57,0x85", !"inline=[0x4e, [0x82, nil]]"}
!27 = !{!"tac=0x88S0x4eS0x38", !"op=GT", !"evm.pc=0x3b,0x57,0x88", !"inline=[0x4e, [0x82, nil]]"}
!28 = !{!"tac=0x89S0x4eS0x38", !"op=ISZERO", !"evm.pc=0x3b,0x57,0x89", !"inline=[0x4e, [0x82, nil]]"}
!29 = !{!"tac=0x8cS0x4eS0x38", !"op=JUMPI", !"evm.pc=0x3b,0x57,0x8c", !"inline=[0x4e, [0x82, nil]]"}
!30 = !{!"tac=0x3cdS0x4eS0x38", !"op=JUMP", !"evm.pc=0x3b,0x57,0x60", !"inline=[0x4e, [0x82, nil]]"}
!31 = !{!"tac=0x1d4S0x38", !"op=JUMP", !"evm.pc=0x3b,0x5b", !"inline=[0x4e, nil]"}
!32 = !{!"tac=0x3f2S0x38", !"op=JUMP", !"evm.pc=0x3b,0x60", !"inline=[0x4e, nil]"}
!33 = !{!"tac=0x3f", !"op=MLOAD", !"evm.pc=0x3f"}
!34 = !{!"tac=0x42", !"op=MSTORE", !"evm.pc=0x42"}
!35 = !{!"tac=0x45", !"op=ADD", !"evm.pc=0x45"}
!36 = !{!"tac=0x48", !"op=MLOAD", !"evm.pc=0x48"}
!37 = !{!"tac=0x4b", !"op=SUB", !"evm.pc=0x4b"}
!38 = !{!"tac=0x4d", !"op=RETURN", !"evm.pc=0x4d"}
!39 = !{!"tac=0x94S0x4eS0x38", !"op=SHL", !"evm.pc=0x3b,0x57,0x94", !"inline=[0x4e, [0x82, nil]]"}
!40 = !{!"tac=0x97S0x4eS0x38", !"op=MSTORE", !"evm.pc=0x3b,0x57,0x97", !"inline=[0x4e, [0x82, nil]]"}
!41 = !{!"tac=0x9cS0x4eS0x38", !"op=MSTORE", !"evm.pc=0x3b,0x57,0x9c", !"inline=[0x4e, [0x82, nil]]"}
!42 = !{!"tac=0xa1S0x4eS0x38", !"op=REVERT", !"evm.pc=0x3b,0x57,0xa1", !"inline=[0x4e, [0x82, nil]]"}
!43 = !{!"tac=0x72", !"op=REVERT", !"evm.pc=0x72"}
