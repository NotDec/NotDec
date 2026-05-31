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
  br i1 %evm.branch.cond2, label %bb._0x1682, label %bb._0x18, !notdec.evm !6

bb._0x18:                                         ; preds = %bb._0xf
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 3347130029, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x2082, label %bb._0x1682, !notdec.evm !10

bb._0x2082:                                       ; preds = %bb._0x18
  call void @public__0xc78126ad_0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x1682:                                       ; preds = %bb._0x18, %bb._0xf
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

define void @public__0xc78126ad_0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x2d:
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !15
  br label %bb._0x6f, !notdec.evm !16

bb._0x6f:                                         ; preds = %bb._0x2d
  %evm.sub = sub i256 %evm.calldatasize, 4, !notdec.evm !17
  %evm.slt = icmp slt i256 %evm.sub, 64, !notdec.evm !18
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !18
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !19
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !19
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !20
  br i1 %evm.branch.cond, label %bb._0x83, label %bb._0x7f, !notdec.evm !20

bb._0x83:                                         ; preds = %bb._0x6f
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 4), !notdec.evm !21
  %evm.gt = icmp ugt i256 %evm.calldataload, 18446744073709551615, !notdec.evm !22
  %evm.bool2 = zext i1 %evm.gt to i256, !notdec.evm !22
  %evm.iszero3 = icmp eq i256 %evm.bool2, 0, !notdec.evm !23
  %evm.bool4 = zext i1 %evm.iszero3 to i256, !notdec.evm !23
  %evm.branch.cond5 = icmp ne i256 %evm.bool4, 0, !notdec.evm !24
  br i1 %evm.branch.cond5, label %bb._0x99, label %bb._0x95, !notdec.evm !24

bb._0x99:                                         ; preds = %bb._0x83
  %evm.add = add i256 4, %evm.calldataload, !notdec.evm !25
  %evm.add6 = add i256 %evm.add, 31, !notdec.evm !26
  %evm.sgt = icmp sgt i256 %evm.calldatasize, %evm.add6, !notdec.evm !27
  %evm.bool7 = zext i1 %evm.sgt to i256, !notdec.evm !27
  %evm.branch.cond8 = icmp ne i256 %evm.bool7, 0, !notdec.evm !28
  br i1 %evm.branch.cond8, label %bb._0xa9, label %bb._0xa5, !notdec.evm !28

bb._0xa9:                                         ; preds = %bb._0x99
  %evm.calldataload9 = call i256 @evm_calldataload(ptr %calldata, i256 %evm.add), !notdec.evm !29
  %evm.gt10 = icmp ugt i256 %evm.calldataload9, 18446744073709551615, !notdec.evm !30
  %evm.bool11 = zext i1 %evm.gt10 to i256, !notdec.evm !30
  %evm.iszero12 = icmp eq i256 %evm.bool11, 0, !notdec.evm !31
  %evm.bool13 = zext i1 %evm.iszero12 to i256, !notdec.evm !31
  %evm.branch.cond14 = icmp ne i256 %evm.bool13, 0, !notdec.evm !32
  br i1 %evm.branch.cond14, label %bb._0xbf, label %bb._0xbb, !notdec.evm !32

bb._0xbf:                                         ; preds = %bb._0xa9
  %evm.shl = call i256 @evm_shl(i256 5, i256 %evm.calldataload9), !notdec.evm !33
  %evm.add15 = add i256 %evm.add, %evm.shl, !notdec.evm !34
  %evm.add16 = add i256 %evm.add15, 32, !notdec.evm !35
  %evm.gt17 = icmp ugt i256 %evm.add16, %evm.calldatasize, !notdec.evm !36
  %evm.bool18 = zext i1 %evm.gt17 to i256, !notdec.evm !36
  %evm.iszero19 = icmp eq i256 %evm.bool18, 0, !notdec.evm !37
  %evm.bool20 = zext i1 %evm.iszero19 to i256, !notdec.evm !37
  %evm.branch.cond21 = icmp ne i256 %evm.bool20, 0, !notdec.evm !38
  br i1 %evm.branch.cond21, label %bb._0xd3, label %bb._0xcf, !notdec.evm !38

bb._0xd3:                                         ; preds = %bb._0xbf
  %evm.add22 = add i256 32, %evm.add, !notdec.evm !39
  %evm.calldataload23 = call i256 @evm_calldataload(ptr %calldata, i256 36), !notdec.evm !40
  br label %bb._0x38, !notdec.evm !41

bb._0x38:                                         ; preds = %bb._0xd3
  br label %bb._0x4e, !notdec.evm !42

bb._0x4e:                                         ; preds = %bb._0x38
  %evm.lt = icmp ult i256 %evm.calldataload23, %evm.calldataload9, !notdec.evm !43
  %evm.bool24 = zext i1 %evm.lt to i256, !notdec.evm !43
  %evm.branch.cond25 = icmp ne i256 %evm.bool24, 0, !notdec.evm !44
  br i1 %evm.branch.cond25, label %bb._0x5f, label %bb._0x5a, !notdec.evm !44

bb._0x5f:                                         ; preds = %bb._0x4e
  %evm.mul = mul i256 32, %evm.calldataload23, !notdec.evm !45
  %evm.add26 = add i256 %evm.mul, %evm.add22, !notdec.evm !46
  %evm.calldataload27 = call i256 @evm_calldataload(ptr %calldata, i256 %evm.add26), !notdec.evm !47
  br label %bb._0x3c, !notdec.evm !48

bb._0x3c:                                         ; preds = %bb._0x5f
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !49
  call void @evm_mstore(ptr %mem, i256 %evm.mload, i256 %evm.calldataload27), !notdec.evm !50
  %evm.add28 = add i256 32, %evm.mload, !notdec.evm !51
  %evm.mload29 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !52
  %evm.sub30 = sub i256 %evm.add28, %evm.mload29, !notdec.evm !53
  call void @evm_return(ptr %mem, i256 %evm.mload29, i256 %evm.sub30), !notdec.evm !54
  ret void, !notdec.evm !54

bb._0x5a:                                         ; preds = %bb._0x4e
  br label %bb._0xe6, !notdec.evm !55

bb._0xe6:                                         ; preds = %bb._0x5a
  %evm.shl31 = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !56
  call void @evm_mstore(ptr %mem, i256 0, i256 %evm.shl31), !notdec.evm !57
  call void @evm_mstore(ptr %mem, i256 4, i256 50), !notdec.evm !58
  call void @evm_revert(ptr %mem, i256 0, i256 36), !notdec.evm !59
  unreachable, !notdec.evm !59

bb._0xcf:                                         ; preds = %bb._0xbf
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !60
  unreachable, !notdec.evm !60

bb._0xbb:                                         ; preds = %bb._0xa9
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !61
  unreachable, !notdec.evm !61

bb._0xa5:                                         ; preds = %bb._0x99
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !62
  unreachable, !notdec.evm !62

bb._0x95:                                         ; preds = %bb._0x83
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !63
  unreachable, !notdec.evm !63

bb._0x7f:                                         ; preds = %bb._0x6f
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !64
  unreachable, !notdec.evm !64
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x13", !"op=CALLDATASIZE", !"evm.pc=0x13"}
!5 = !{!"tac=0x14", !"op=LT", !"evm.pc=0x14"}
!6 = !{!"tac=0x2a2", !"op=JUMPI", !"evm.pc=0x15"}
!7 = !{!"tac=0x1a", !"op=CALLDATALOAD", !"evm.pc=0x1a"}
!8 = !{!"tac=0x1d", !"op=SHR", !"evm.pc=0x1d"}
!9 = !{!"tac=0x24", !"op=EQ", !"evm.pc=0x24"}
!10 = !{!"tac=0xca2", !"op=JUMPI", !"evm.pc=0x25"}
!11 = !{!"tac=0x20c2", !"op=CALLPRIVATE", !"evm.pc=0x2d"}
!12 = !{!"tac=0x16c2", !"op=CALLPRIVATE", !"evm.pc=0x28"}
!13 = !{!"tac=0xe", !"op=REVERT", !"evm.pc=0xe"}
!14 = !{!"tac=0x2c", !"op=REVERT", !"evm.pc=0x2c"}
!15 = !{!"tac=0x32", !"op=CALLDATASIZE", !"evm.pc=0x32"}
!16 = !{!"tac=0x37", !"op=JUMP", !"evm.pc=0x37"}
!17 = !{!"tac=0x79", !"op=SUB", !"evm.pc=0x79"}
!18 = !{!"tac=0x7a", !"op=SLT", !"evm.pc=0x7a"}
!19 = !{!"tac=0x7b", !"op=ISZERO", !"evm.pc=0x7b"}
!20 = !{!"tac=0x7e", !"op=JUMPI", !"evm.pc=0x7e"}
!21 = !{!"tac=0x85", !"op=CALLDATALOAD", !"evm.pc=0x85"}
!22 = !{!"tac=0x90", !"op=GT", !"evm.pc=0x90"}
!23 = !{!"tac=0x91", !"op=ISZERO", !"evm.pc=0x91"}
!24 = !{!"tac=0x94", !"op=JUMPI", !"evm.pc=0x94"}
!25 = !{!"tac=0x9b", !"op=ADD", !"evm.pc=0x9b"}
!26 = !{!"tac=0x9f", !"op=ADD", !"evm.pc=0x9f"}
!27 = !{!"tac=0xa1", !"op=SGT", !"evm.pc=0xa1"}
!28 = !{!"tac=0xa4", !"op=JUMPI", !"evm.pc=0xa4"}
!29 = !{!"tac=0xab", !"op=CALLDATALOAD", !"evm.pc=0xab"}
!30 = !{!"tac=0xb6", !"op=GT", !"evm.pc=0xb6"}
!31 = !{!"tac=0xb7", !"op=ISZERO", !"evm.pc=0xb7"}
!32 = !{!"tac=0xba", !"op=JUMPI", !"evm.pc=0xba"}
!33 = !{!"tac=0xc6", !"op=SHL", !"evm.pc=0xc6"}
!34 = !{!"tac=0xc8", !"op=ADD", !"evm.pc=0xc8"}
!35 = !{!"tac=0xc9", !"op=ADD", !"evm.pc=0xc9"}
!36 = !{!"tac=0xca", !"op=GT", !"evm.pc=0xca"}
!37 = !{!"tac=0xcb", !"op=ISZERO", !"evm.pc=0xcb"}
!38 = !{!"tac=0xce", !"op=JUMPI", !"evm.pc=0xce"}
!39 = !{!"tac=0xd8", !"op=ADD", !"evm.pc=0xd8"}
!40 = !{!"tac=0xdf", !"op=CALLDATALOAD", !"evm.pc=0xdf"}
!41 = !{!"tac=0xe5", !"op=JUMP", !"evm.pc=0xe5"}
!42 = !{!"tac=0x3b", !"op=JUMP", !"evm.pc=0x3b"}
!43 = !{!"tac=0x56", !"op=LT", !"evm.pc=0x56"}
!44 = !{!"tac=0x59", !"op=JUMPI", !"evm.pc=0x59"}
!45 = !{!"tac=0x64", !"op=MUL", !"evm.pc=0x64"}
!46 = !{!"tac=0x65", !"op=ADD", !"evm.pc=0x65"}
!47 = !{!"tac=0x66", !"op=CALLDATALOAD", !"evm.pc=0x66"}
!48 = !{!"tac=0x6e", !"op=JUMP", !"evm.pc=0x6e"}
!49 = !{!"tac=0x3f", !"op=MLOAD", !"evm.pc=0x3f"}
!50 = !{!"tac=0x42", !"op=MSTORE", !"evm.pc=0x42"}
!51 = !{!"tac=0x45", !"op=ADD", !"evm.pc=0x45"}
!52 = !{!"tac=0x48", !"op=MLOAD", !"evm.pc=0x48"}
!53 = !{!"tac=0x4b", !"op=SUB", !"evm.pc=0x4b"}
!54 = !{!"tac=0x4d", !"op=RETURN", !"evm.pc=0x4d"}
!55 = !{!"tac=0x5e", !"op=JUMP", !"evm.pc=0x5e"}
!56 = !{!"tac=0xee", !"op=SHL", !"evm.pc=0xee"}
!57 = !{!"tac=0xf1", !"op=MSTORE", !"evm.pc=0xf1"}
!58 = !{!"tac=0xf6", !"op=MSTORE", !"evm.pc=0xf6"}
!59 = !{!"tac=0xfb", !"op=REVERT", !"evm.pc=0xfb"}
!60 = !{!"tac=0xd2", !"op=REVERT", !"evm.pc=0xd2"}
!61 = !{!"tac=0xbe", !"op=REVERT", !"evm.pc=0xbe"}
!62 = !{!"tac=0xa8", !"op=REVERT", !"evm.pc=0xa8"}
!63 = !{!"tac=0x98", !"op=REVERT", !"evm.pc=0x98"}
!64 = !{!"tac=0x82", !"op=REVERT", !"evm.pc=0x82"}
