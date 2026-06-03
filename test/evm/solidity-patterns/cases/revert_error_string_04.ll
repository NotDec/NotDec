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
  %notdec.evm.mem.ptr.0 = inttoptr i256 64 to ptr
  store i256 128, ptr %notdec.evm.mem.ptr.0, align 1, !notdec.evm !0
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !1
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !2
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !2
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !3
  br i1 %evm.branch.cond, label %bb._0xf, label %bb._0xc, !notdec.evm !3

bb._0xf:                                          ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.lt to i256, !notdec.evm !5
  %evm.branch.cond2 = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond2, label %bb._0x16aa, label %bb._0x19, !notdec.evm !6

bb._0x19:                                         ; preds = %bb._0xf
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 975660323, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x20aa, label %bb._0x16aa, !notdec.evm !10

bb._0x20aa:                                       ; preds = %bb._0x19
  call void @public__0x3a276523_0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x16aa:                                       ; preds = %bb._0x19, %bb._0xf
  call void @public_fallback___0x29(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  ret void

bb._0xc:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !13
  unreachable, !notdec.evm !13
}

define void @public_fallback___0x29(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x29:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !14
  unreachable, !notdec.evm !14
}

define void @public__0x3a276523_0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x2d:
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !15
  br label %bb._0xa2, !notdec.evm !16

bb._0xa2:                                         ; preds = %bb._0x2d
  %evm.sub = sub i256 %evm.calldatasize, 4, !notdec.evm !17
  %evm.slt = icmp slt i256 %evm.sub, 32, !notdec.evm !18
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !18
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !19
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !19
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !20
  br i1 %evm.branch.cond, label %bb._0xb3, label %bb._0xb0, !notdec.evm !20

bb._0xb3:                                         ; preds = %bb._0xa2
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 4), !notdec.evm !21
  %evm.gt = icmp ugt i256 %evm.calldataload, 18446744073709551615, !notdec.evm !22
  %evm.bool2 = zext i1 %evm.gt to i256, !notdec.evm !22
  %evm.iszero3 = icmp eq i256 %evm.bool2, 0, !notdec.evm !23
  %evm.bool4 = zext i1 %evm.iszero3 to i256, !notdec.evm !23
  %evm.branch.cond5 = icmp ne i256 %evm.bool4, 0, !notdec.evm !24
  br i1 %evm.branch.cond5, label %bb._0xc9, label %bb._0xc6, !notdec.evm !24

bb._0xc9:                                         ; preds = %bb._0xb3
  %evm.add = add i256 4, %evm.calldataload, !notdec.evm !25
  %evm.add6 = add i256 %evm.add, 31, !notdec.evm !26
  %evm.sgt = icmp sgt i256 %evm.calldatasize, %evm.add6, !notdec.evm !27
  %evm.bool7 = zext i1 %evm.sgt to i256, !notdec.evm !27
  %evm.branch.cond8 = icmp ne i256 %evm.bool7, 0, !notdec.evm !28
  br i1 %evm.branch.cond8, label %bb._0xd9, label %bb._0xd6, !notdec.evm !28

bb._0xd9:                                         ; preds = %bb._0xc9
  %evm.calldataload9 = call i256 @evm_calldataload(ptr %calldata, i256 %evm.add), !notdec.evm !29
  %evm.gt10 = icmp ugt i256 %evm.calldataload9, 18446744073709551615, !notdec.evm !30
  %evm.bool11 = zext i1 %evm.gt10 to i256, !notdec.evm !30
  %evm.iszero12 = icmp eq i256 %evm.bool11, 0, !notdec.evm !31
  %evm.bool13 = zext i1 %evm.iszero12 to i256, !notdec.evm !31
  %evm.branch.cond14 = icmp ne i256 %evm.bool13, 0, !notdec.evm !32
  br i1 %evm.branch.cond14, label %bb._0xef, label %bb._0xec, !notdec.evm !32

bb._0xef:                                         ; preds = %bb._0xd9
  %evm.add15 = add i256 %evm.add, %evm.calldataload9, !notdec.evm !33
  %evm.add16 = add i256 %evm.add15, 32, !notdec.evm !34
  %evm.gt17 = icmp ugt i256 %evm.add16, %evm.calldatasize, !notdec.evm !35
  %evm.bool18 = zext i1 %evm.gt17 to i256, !notdec.evm !35
  %evm.iszero19 = icmp eq i256 %evm.bool18, 0, !notdec.evm !36
  %evm.bool20 = zext i1 %evm.iszero19 to i256, !notdec.evm !36
  %evm.branch.cond21 = icmp ne i256 %evm.bool20, 0, !notdec.evm !37
  br i1 %evm.branch.cond21, label %bb._0x100, label %bb._0xfd, !notdec.evm !37

bb._0x100:                                        ; preds = %bb._0xef
  %evm.add22 = add i256 32, %evm.add, !notdec.evm !38
  br label %bb._0x3b, !notdec.evm !39

bb._0x3b:                                         ; preds = %bb._0x100
  br label %bb._0x52B0x3b, !notdec.evm !40

bb._0x52B0x3b:                                    ; preds = %bb._0x3b
  %evm.gt23 = icmp ugt i256 %evm.calldataload9, 3, !notdec.evm !41
  %evm.bool24 = zext i1 %evm.gt23 to i256, !notdec.evm !41
  %evm.branch.cond25 = icmp ne i256 %evm.bool24, 0, !notdec.evm !42
  br i1 %evm.branch.cond25, label %bb._0x9cB0x3b, label %bb._0x5cB0x3b, !notdec.evm !42

bb._0x9cB0x3b:                                    ; preds = %bb._0x52B0x3b
  br label %bb._0x40, !notdec.evm !43

bb._0x40:                                         ; preds = %bb._0x9cB0x3b
  %notdec.evm.mem.ptr.1 = inttoptr i256 64 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.1, align 1, !notdec.evm !44
  %notdec.evm.mem.ptr.2 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.calldataload9, ptr %notdec.evm.mem.ptr.2, align 1, !notdec.evm !45
  %evm.add26 = add i256 32, %evm.mload, !notdec.evm !46
  %notdec.evm.mem.ptr.3 = inttoptr i256 64 to ptr
  %evm.mload27 = load i256, ptr %notdec.evm.mem.ptr.3, align 1, !notdec.evm !47
  %evm.sub28 = sub i256 %evm.add26, %evm.mload27, !notdec.evm !48
  call void @evm_return(ptr %mem, i256 %evm.mload27, i256 %evm.sub28), !notdec.evm !49
  ret void, !notdec.evm !49

bb._0x5cB0x3b:                                    ; preds = %bb._0x52B0x3b
  %notdec.evm.mem.ptr.4 = inttoptr i256 64 to ptr
  %evm.mload29 = load i256, ptr %notdec.evm.mem.ptr.4, align 1, !notdec.evm !50
  %evm.shl = call i256 @evm_shl(i256 229, i256 4594637), !notdec.evm !51
  %notdec.evm.mem.ptr.5 = inttoptr i256 %evm.mload29 to ptr
  store i256 %evm.shl, ptr %notdec.evm.mem.ptr.5, align 1, !notdec.evm !52
  %evm.add30 = add i256 %evm.mload29, 4, !notdec.evm !53
  %notdec.evm.mem.ptr.6 = inttoptr i256 %evm.add30 to ptr
  store i256 32, ptr %notdec.evm.mem.ptr.6, align 1, !notdec.evm !54
  %evm.add31 = add i256 %evm.mload29, 36, !notdec.evm !55
  %notdec.evm.mem.ptr.7 = inttoptr i256 %evm.add31 to ptr
  store i256 18, ptr %notdec.evm.mem.ptr.7, align 1, !notdec.evm !56
  %evm.shl32 = call i256 @evm_shl(i256 114, i256 2164316954065790762864157230338187359673501), !notdec.evm !57
  %evm.add33 = add i256 %evm.mload29, 68, !notdec.evm !58
  %notdec.evm.mem.ptr.8 = inttoptr i256 %evm.add33 to ptr
  store i256 %evm.shl32, ptr %notdec.evm.mem.ptr.8, align 1, !notdec.evm !59
  %evm.add34 = add i256 100, %evm.mload29, !notdec.evm !60
  %notdec.evm.mem.ptr.9 = inttoptr i256 64 to ptr
  %evm.mload35 = load i256, ptr %notdec.evm.mem.ptr.9, align 1, !notdec.evm !61
  %evm.sub36 = sub i256 %evm.add34, %evm.mload35, !notdec.evm !62
  call void @evm_revert(ptr %mem, i256 %evm.mload35, i256 %evm.sub36), !notdec.evm !63
  unreachable, !notdec.evm !63

bb._0xfd:                                         ; preds = %bb._0xef
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !64
  unreachable, !notdec.evm !64

bb._0xec:                                         ; preds = %bb._0xd9
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !65
  unreachable, !notdec.evm !65

bb._0xd6:                                         ; preds = %bb._0xc9
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !66
  unreachable, !notdec.evm !66

bb._0xc6:                                         ; preds = %bb._0xb3
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !67
  unreachable, !notdec.evm !67

bb._0xb0:                                         ; preds = %bb._0xa2
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !68
  unreachable, !notdec.evm !68
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xb", !"op=JUMPI", !"evm.pc=0xb"}
!4 = !{!"tac=0x13", !"op=CALLDATASIZE", !"evm.pc=0x13"}
!5 = !{!"tac=0x14", !"op=LT", !"evm.pc=0x14"}
!6 = !{!"tac=0x2ca", !"op=JUMPI", !"evm.pc=0x15"}
!7 = !{!"tac=0x1a", !"op=CALLDATALOAD", !"evm.pc=0x1a"}
!8 = !{!"tac=0x1d", !"op=SHR", !"evm.pc=0x1d"}
!9 = !{!"tac=0x24", !"op=EQ", !"evm.pc=0x24"}
!10 = !{!"tac=0xcca", !"op=JUMPI", !"evm.pc=0x25"}
!11 = !{!"tac=0x20ea", !"op=CALLPRIVATE", !"evm.pc=0x2d"}
!12 = !{!"tac=0x16ea", !"op=CALLPRIVATE", !"evm.pc=0x29"}
!13 = !{!"tac=0xe", !"op=REVERT", !"evm.pc=0xe"}
!14 = !{!"tac=0x2c", !"op=REVERT", !"evm.pc=0x2c"}
!15 = !{!"tac=0x34", !"op=CALLDATASIZE", !"evm.pc=0x34"}
!16 = !{!"tac=0x3a", !"op=JUMP", !"evm.pc=0x3a"}
!17 = !{!"tac=0xa9", !"op=SUB", !"evm.pc=0xa9"}
!18 = !{!"tac=0xaa", !"op=SLT", !"evm.pc=0xaa"}
!19 = !{!"tac=0xab", !"op=ISZERO", !"evm.pc=0xab"}
!20 = !{!"tac=0xaf", !"op=JUMPI", !"evm.pc=0xaf"}
!21 = !{!"tac=0xb5", !"op=CALLDATALOAD", !"evm.pc=0xb5"}
!22 = !{!"tac=0xc0", !"op=GT", !"evm.pc=0xc0"}
!23 = !{!"tac=0xc1", !"op=ISZERO", !"evm.pc=0xc1"}
!24 = !{!"tac=0xc5", !"op=JUMPI", !"evm.pc=0xc5"}
!25 = !{!"tac=0xcb", !"op=ADD", !"evm.pc=0xcb"}
!26 = !{!"tac=0xcf", !"op=ADD", !"evm.pc=0xcf"}
!27 = !{!"tac=0xd1", !"op=SGT", !"evm.pc=0xd1"}
!28 = !{!"tac=0xd5", !"op=JUMPI", !"evm.pc=0xd5"}
!29 = !{!"tac=0xdb", !"op=CALLDATALOAD", !"evm.pc=0xdb"}
!30 = !{!"tac=0xe6", !"op=GT", !"evm.pc=0xe6"}
!31 = !{!"tac=0xe7", !"op=ISZERO", !"evm.pc=0xe7"}
!32 = !{!"tac=0xeb", !"op=JUMPI", !"evm.pc=0xeb"}
!33 = !{!"tac=0xf5", !"op=ADD", !"evm.pc=0xf5"}
!34 = !{!"tac=0xf6", !"op=ADD", !"evm.pc=0xf6"}
!35 = !{!"tac=0xf7", !"op=GT", !"evm.pc=0xf7"}
!36 = !{!"tac=0xf8", !"op=ISZERO", !"evm.pc=0xf8"}
!37 = !{!"tac=0xfc", !"op=JUMPI", !"evm.pc=0xfc"}
!38 = !{!"tac=0x106", !"op=ADD", !"evm.pc=0x106"}
!39 = !{!"tac=0x10f", !"op=JUMP", !"evm.pc=0x10f"}
!40 = !{!"tac=0x3f", !"op=JUMP", !"evm.pc=0x3f"}
!41 = !{!"tac=0x57S0x3b", !"op=GT", !"evm.pc=0x3f,0x57", !"inline=[0x52, nil]"}
!42 = !{!"tac=0x5bS0x3b", !"op=JUMPI", !"evm.pc=0x3f,0x5b", !"inline=[0x52, nil]"}
!43 = !{!"tac=0xa1S0x3b", !"op=JUMP", !"evm.pc=0x3f,0xa1", !"inline=[0x52, nil]"}
!44 = !{!"tac=0x43", !"op=MLOAD", !"evm.pc=0x43"}
!45 = !{!"tac=0x46", !"op=MSTORE", !"evm.pc=0x46"}
!46 = !{!"tac=0x49", !"op=ADD", !"evm.pc=0x49"}
!47 = !{!"tac=0x4c", !"op=MLOAD", !"evm.pc=0x4c"}
!48 = !{!"tac=0x4f", !"op=SUB", !"evm.pc=0x4f"}
!49 = !{!"tac=0x51", !"op=RETURN", !"evm.pc=0x51"}
!50 = !{!"tac=0x5eS0x3b", !"op=MLOAD", !"evm.pc=0x3f,0x5e", !"inline=[0x52, nil]"}
!51 = !{!"tac=0x65S0x3b", !"op=SHL", !"evm.pc=0x3f,0x65", !"inline=[0x52, nil]"}
!52 = !{!"tac=0x67S0x3b", !"op=MSTORE", !"evm.pc=0x3f,0x67", !"inline=[0x52, nil]"}
!53 = !{!"tac=0x6dS0x3b", !"op=ADD", !"evm.pc=0x3f,0x6d", !"inline=[0x52, nil]"}
!54 = !{!"tac=0x6eS0x3b", !"op=MSTORE", !"evm.pc=0x3f,0x6e", !"inline=[0x52, nil]"}
!55 = !{!"tac=0x74S0x3b", !"op=ADD", !"evm.pc=0x3f,0x74", !"inline=[0x52, nil]"}
!56 = !{!"tac=0x75S0x3b", !"op=MSTORE", !"evm.pc=0x3f,0x75", !"inline=[0x52, nil]"}
!57 = !{!"tac=0x8bS0x3b", !"op=SHL", !"evm.pc=0x3f,0x8b", !"inline=[0x52, nil]"}
!58 = !{!"tac=0x8fS0x3b", !"op=ADD", !"evm.pc=0x3f,0x8f", !"inline=[0x52, nil]"}
!59 = !{!"tac=0x90S0x3b", !"op=MSTORE", !"evm.pc=0x3f,0x90", !"inline=[0x52, nil]"}
!60 = !{!"tac=0x93S0x3b", !"op=ADD", !"evm.pc=0x3f,0x93", !"inline=[0x52, nil]"}
!61 = !{!"tac=0x96S0x3b", !"op=MLOAD", !"evm.pc=0x3f,0x96", !"inline=[0x52, nil]"}
!62 = !{!"tac=0x99S0x3b", !"op=SUB", !"evm.pc=0x3f,0x99", !"inline=[0x52, nil]"}
!63 = !{!"tac=0x9bS0x3b", !"op=REVERT", !"evm.pc=0x3f,0x9b", !"inline=[0x52, nil]"}
!64 = !{!"tac=0xff", !"op=REVERT", !"evm.pc=0xff"}
!65 = !{!"tac=0xee", !"op=REVERT", !"evm.pc=0xee"}
!66 = !{!"tac=0xd8", !"op=REVERT", !"evm.pc=0xd8"}
!67 = !{!"tac=0xc8", !"op=REVERT", !"evm.pc=0xc8"}
!68 = !{!"tac=0xb2", !"op=REVERT", !"evm.pc=0xb2"}

attributes #0 = { null_pointer_is_valid }
