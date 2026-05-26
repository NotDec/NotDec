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
  %evm.shl = call i256 @evm_shl(i256 176, i256 80595054640975278313745), !notdec.evm !1
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !2
  %evm.add = add i256 32, %evm.mload, !notdec.evm !3
  %private.call = call i256 @private__0x23a_0x23a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.add, i256 %evm.shl, i256 34), !notdec.evm !4
  br label %bb._0x22

bb._0x22:                                         ; preds = %bb._0x0
  %evm.mload1 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !5
  %evm.sub = sub i256 %private.call, %evm.mload1, !notdec.evm !6
  %evm.sub2 = sub i256 %evm.sub, 32, !notdec.evm !7
  call void @evm_mstore(ptr %mem, i256 %evm.mload1, i256 %evm.sub2), !notdec.evm !8
  call void @evm_mstore(ptr %mem, i256 64, i256 %private.call), !notdec.evm !9
  %evm.mload3 = call i256 @evm_mload(ptr %mem, i256 %evm.mload1), !notdec.evm !10
  %evm.add4 = add i256 32, %evm.mload1, !notdec.evm !11
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 %evm.add4, i256 %evm.mload3), !notdec.evm !12
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !13
  %evm.mload5 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !14
  %private.call6 = call i256 @private__0x294_0x294(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.mload5, i256 %evm.calldatasize, i256 0, i256 73), !notdec.evm !15
  br label %bb._0x49

bb._0x49:                                         ; preds = %bb._0x22
  %evm.mload7 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !16
  %evm.sub8 = sub i256 %private.call6, %evm.mload7, !notdec.evm !17
  %evm.sha39 = call i256 @evm_sha3(ptr %mem, i256 %evm.mload7, i256 %evm.sub8), !notdec.evm !18
  %evm.eq = icmp eq i256 %evm.sha39, %evm.sha3, !notdec.evm !19
  %evm.bool = zext i1 %evm.eq to i256, !notdec.evm !19
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !20
  %evm.bool10 = zext i1 %evm.iszero to i256, !notdec.evm !20
  %evm.branch.cond = icmp ne i256 %evm.bool10, 0, !notdec.evm !21
  br i1 %evm.branch.cond, label %bb._0x69, label %bb._0x58, !notdec.evm !21

bb._0x69:                                         ; preds = %bb._0x49
  %evm.calldatasize11 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !22
  %evm.lt = icmp ult i256 %evm.calldatasize11, 10, !notdec.evm !23
  %evm.bool12 = zext i1 %evm.lt to i256, !notdec.evm !23
  %evm.iszero13 = icmp eq i256 %evm.bool12, 0, !notdec.evm !24
  %evm.bool14 = zext i1 %evm.iszero13 to i256, !notdec.evm !24
  %evm.iszero15 = icmp eq i256 %evm.bool14, 0, !notdec.evm !25
  %evm.bool16 = zext i1 %evm.iszero15 to i256, !notdec.evm !25
  %evm.branch.cond17 = icmp ne i256 %evm.bool16, 0, !notdec.evm !26
  br i1 %evm.branch.cond17, label %bb._0xeb, label %bb._0x79, !notdec.evm !26

bb._0x79:                                         ; preds = %bb._0x69
  %evm.shl18 = call i256 @evm_shl(i256 176, i256 161188879499012309324049), !notdec.evm !27
  %evm.mload19 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !28
  %evm.add20 = add i256 32, %evm.mload19, !notdec.evm !29
  %private.call21 = call i256 @private__0x23a_0x23a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.add20, i256 %evm.shl18, i256 151), !notdec.evm !30
  br label %bb._0x97

bb._0x97:                                         ; preds = %bb._0x79
  %evm.mload22 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !31
  %evm.sub23 = sub i256 %private.call21, %evm.mload22, !notdec.evm !32
  %evm.sub24 = sub i256 %evm.sub23, 32, !notdec.evm !33
  call void @evm_mstore(ptr %mem, i256 %evm.mload22, i256 %evm.sub24), !notdec.evm !34
  call void @evm_mstore(ptr %mem, i256 64, i256 %private.call21), !notdec.evm !35
  %evm.mload25 = call i256 @evm_mload(ptr %mem, i256 %evm.mload22), !notdec.evm !36
  %evm.add26 = add i256 32, %evm.mload22, !notdec.evm !37
  %evm.sha327 = call i256 @evm_sha3(ptr %mem, i256 %evm.add26, i256 %evm.mload25), !notdec.evm !38
  %evm.calldatasize28 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !39
  br label %bb._0x2b7, !notdec.evm !40

bb._0x2b7:                                        ; preds = %bb._0x97
  br i1 true, label %bb._0x2cb, label %bb._0x2c3, !notdec.evm !41

bb._0x2cb:                                        ; preds = %bb._0x2b7
  %evm.gt = icmp ugt i256 10, %evm.calldatasize28, !notdec.evm !42
  %evm.bool29 = zext i1 %evm.gt to i256, !notdec.evm !42
  %evm.iszero30 = icmp eq i256 %evm.bool29, 0, !notdec.evm !43
  %evm.bool31 = zext i1 %evm.iszero30 to i256, !notdec.evm !43
  %evm.branch.cond32 = icmp ne i256 %evm.bool31, 0, !notdec.evm !44
  br i1 %evm.branch.cond32, label %bb._0x2dc, label %bb._0x2d4, !notdec.evm !44

bb._0x2dc:                                        ; preds = %bb._0x2cb
  br label %bb._0xc2, !notdec.evm !45

bb._0xc2:                                         ; preds = %bb._0x2dc
  %evm.mload33 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !46
  %evm.add34 = add i256 32, %evm.mload33, !notdec.evm !47
  %private.call35 = call i256 @private__0x2f2_0x2f2(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.add34, i256 10, i256 0, i256 211), !notdec.evm !48
  br label %bb._0xd3

bb._0xd3:                                         ; preds = %bb._0xc2
  %evm.mload36 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !49
  %evm.sub37 = sub i256 %private.call35, %evm.mload36, !notdec.evm !50
  %evm.sub38 = sub i256 %evm.sub37, 32, !notdec.evm !51
  call void @evm_mstore(ptr %mem, i256 %evm.mload36, i256 %evm.sub38), !notdec.evm !52
  call void @evm_mstore(ptr %mem, i256 64, i256 %private.call35), !notdec.evm !53
  %evm.mload39 = call i256 @evm_mload(ptr %mem, i256 %evm.mload36), !notdec.evm !54
  %evm.add40 = add i256 32, %evm.mload36, !notdec.evm !55
  %evm.sha341 = call i256 @evm_sha3(ptr %mem, i256 %evm.add40, i256 %evm.mload39), !notdec.evm !56
  %evm.eq42 = icmp eq i256 %evm.sha341, %evm.sha327, !notdec.evm !57
  %evm.bool43 = zext i1 %evm.eq42 to i256, !notdec.evm !57
  br label %bb._0xeb, !notdec.evm !58

bb._0xeb:                                         ; preds = %bb._0xd3, %bb._0x69
  %_0xeb_0x0 = phi i256 [ %evm.bool14, %bb._0x69 ], [ %evm.bool43, %bb._0xd3 ], !notdec.evm !59
  %evm.iszero44 = icmp eq i256 %_0xeb_0x0, 0, !notdec.evm !60
  %evm.bool45 = zext i1 %evm.iszero44 to i256, !notdec.evm !60
  %evm.branch.cond46 = icmp ne i256 %evm.bool45, 0, !notdec.evm !61
  br i1 %evm.branch.cond46, label %bb._0x1b8, label %bb._0xf1, !notdec.evm !61

bb._0x1b8:                                        ; preds = %bb._0xeb
  %evm.sload = call i256 @evm_sload(i256 0), !notdec.evm !62
  call void @evm_mstore(ptr %mem, i256 0, i256 4803839602528529), !notdec.evm !63
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !64
  %evm.delegatecall = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 %evm.sload, i256 0, i256 7, i256 0, i256 0), !notdec.evm !65
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !66
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize), !notdec.evm !67
  %evm.eq47 = icmp eq i256 %evm.delegatecall, 0, !notdec.evm !68
  %evm.bool48 = zext i1 %evm.eq47 to i256, !notdec.evm !68
  %evm.branch.cond49 = icmp ne i256 %evm.bool48, 0, !notdec.evm !69
  br i1 %evm.branch.cond49, label %bb._0x1e6, label %bb._0x1e2, !notdec.evm !69

bb._0x1e6:                                        ; preds = %bb._0x1b8
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize), !notdec.evm !70
  unreachable, !notdec.evm !70

bb._0x1e2:                                        ; preds = %bb._0x1b8
  call void @evm_return(ptr %mem, i256 0, i256 %evm.returndatasize), !notdec.evm !71
  ret void, !notdec.evm !71

bb._0xf1:                                         ; preds = %bb._0xeb
  %evm.sload50 = call i256 @evm_sload(i256 0), !notdec.evm !72
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !73
  %evm.eq51 = icmp eq i256 %evm.caller, 1131328067461702119379948382679143011402029489796, !notdec.evm !74
  %evm.bool52 = zext i1 %evm.eq51 to i256, !notdec.evm !74
  %evm.branch.cond53 = icmp ne i256 %evm.bool52, 0, !notdec.evm !75
  br i1 %evm.branch.cond53, label %bb._0x129, label %bb._0x125, !notdec.evm !75

bb._0x129:                                        ; preds = %bb._0xf1
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !76
  %evm.gt54 = icmp ugt i256 %evm.callvalue, 0, !notdec.evm !77
  %evm.bool55 = zext i1 %evm.gt54 to i256, !notdec.evm !77
  %evm.iszero56 = icmp eq i256 %evm.bool55, 0, !notdec.evm !78
  %evm.bool57 = zext i1 %evm.iszero56 to i256, !notdec.evm !78
  %evm.branch.cond58 = icmp ne i256 %evm.bool57, 0, !notdec.evm !79
  br i1 %evm.branch.cond58, label %bb._0x181, label %bb._0x133, !notdec.evm !79

bb._0x181:                                        ; preds = %bb._0x129
  call void @evm_sstore(i256 0, i256 1153611788939004710816649274463142904956676655633), !notdec.evm !80
  %evm.sload59 = call i256 @evm_sload(i256 0), !notdec.evm !81
  %evm.calldatasize60 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !82
  %evm.sub61 = sub i256 %evm.calldatasize60, 10, !notdec.evm !83
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 0, i256 10, i256 %evm.sub61), !notdec.evm !84
  %evm.gas62 = call i256 @evm_gas(ptr %env), !notdec.evm !85
  %evm.delegatecall63 = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas62, i256 %evm.sload59, i256 0, i256 %evm.sub61, i256 0, i256 0), !notdec.evm !86
  %evm.returndatasize64 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !87
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize64), !notdec.evm !88
  call void @evm_sstore(i256 0, i256 %evm.sload50), !notdec.evm !89
  %evm.eq65 = icmp eq i256 %evm.delegatecall63, 0, !notdec.evm !90
  %evm.bool66 = zext i1 %evm.eq65 to i256, !notdec.evm !90
  %evm.branch.cond67 = icmp ne i256 %evm.bool66, 0, !notdec.evm !91
  br i1 %evm.branch.cond67, label %bb._0x1b3, label %bb._0x1af, !notdec.evm !91

bb._0x1b3:                                        ; preds = %bb._0x181
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize64), !notdec.evm !92
  unreachable, !notdec.evm !92

bb._0x1af:                                        ; preds = %bb._0x181
  call void @evm_return(ptr %mem, i256 0, i256 %evm.returndatasize64), !notdec.evm !93
  ret void, !notdec.evm !93

bb._0x133:                                        ; preds = %bb._0x129
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 10), !notdec.evm !94
  %evm.calldataload68 = call i256 @evm_calldataload(ptr %calldata, i256 42), !notdec.evm !95
  %evm.calldataload69 = call i256 @evm_calldataload(ptr %calldata, i256 74), !notdec.evm !96
  %evm.calldataload70 = call i256 @evm_calldataload(ptr %calldata, i256 106), !notdec.evm !97
  %evm.shr = call i256 @evm_shr(i256 96, i256 %evm.calldataload), !notdec.evm !98
  %evm.shr71 = call i256 @evm_shr(i256 96, i256 %evm.calldataload69), !notdec.evm !99
  %evm.gas72 = call i256 @evm_gas(ptr %env), !notdec.evm !100
  %evm.call = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas72, i256 %evm.shr, i256 %evm.calldataload68, i256 0, i256 0, i256 0, i256 0), !notdec.evm !101
  %evm.branch.cond73 = icmp ne i256 %evm.call, 0, !notdec.evm !102
  br i1 %evm.branch.cond73, label %bb._0x168, label %bb._0x164, !notdec.evm !102

bb._0x168:                                        ; preds = %bb._0x133
  %evm.gas74 = call i256 @evm_gas(ptr %env), !notdec.evm !103
  %evm.call75 = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas74, i256 %evm.shr71, i256 %evm.calldataload70, i256 0, i256 0, i256 0, i256 0), !notdec.evm !104
  %evm.branch.cond76 = icmp ne i256 %evm.call75, 0, !notdec.evm !105
  br i1 %evm.branch.cond76, label %bb._0x17c, label %bb._0x178, !notdec.evm !105

bb._0x17c:                                        ; preds = %bb._0x168
  call void @evm_return(ptr %mem, i256 0, i256 0), !notdec.evm !106
  ret void, !notdec.evm !106

bb._0x178:                                        ; preds = %bb._0x168
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !107
  unreachable, !notdec.evm !107

bb._0x164:                                        ; preds = %bb._0x133
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !108
  unreachable, !notdec.evm !108

bb._0x125:                                        ; preds = %bb._0xf1
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !109
  unreachable, !notdec.evm !109

bb._0x2d4:                                        ; preds = %bb._0x2cb
  br label %bb._0x2b2, !notdec.evm !110

bb._0x2b2:                                        ; preds = %bb._0x2d4
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !111
  unreachable, !notdec.evm !111

bb._0x2c3:                                        ; preds = %bb._0x2b7
  br label %bb._0x2ad, !notdec.evm !112

bb._0x2ad:                                        ; preds = %bb._0x2c3
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !113
  unreachable, !notdec.evm !113

bb._0x58:                                         ; preds = %bb._0x60, %bb._0x49
  br i1 false, label %bb._0x64, label %bb._0x60, !notdec.evm !114

bb._0x64:                                         ; preds = %bb._0x58
  br label %bb._0x1eb, !notdec.evm !115

bb._0x1eb:                                        ; preds = %bb._0x64
  ret void, !notdec.evm !116

bb._0x60:                                         ; preds = %bb._0x58
  br label %bb._0x58, !notdec.evm !117
}

define i256 @private__0x23a_0x23a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x23aarg0x0, i256 %_0x23aarg0x1, i256 %_0x23aarg0x2) {
bb._0x23a:
  br label %bb._0x223, !notdec.evm !118

bb._0x223:                                        ; preds = %bb._0x23a
  br label %bb._0x1ed, !notdec.evm !119

bb._0x1ed:                                        ; preds = %bb._0x223
  %evm.and = and i256 %_0x23aarg0x1, -95780971304118053647396689196894323976171195136475136, !notdec.evm !120
  br label %bb._0x22f, !notdec.evm !121

bb._0x22f:                                        ; preds = %bb._0x1ed
  br label %bb._0x219, !notdec.evm !122

bb._0x219:                                        ; preds = %bb._0x22f
  br label %bb._0x234, !notdec.evm !123

bb._0x234:                                        ; preds = %bb._0x219
  call void @evm_mstore(ptr %mem, i256 %_0x23aarg0x0, i256 %evm.and), !notdec.evm !124
  br label %bb._0x246, !notdec.evm !125

bb._0x246:                                        ; preds = %bb._0x234
  %evm.add = add i256 %_0x23aarg0x0, 10, !notdec.evm !126
  ret i256 %evm.add, !notdec.evm !127
}

define i256 @private__0x26f_0x26f(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x26farg0x0, i256 %_0x26farg0x1, i256 %_0x26farg0x2, i256 %_0x26farg0x3) {
bb._0x26f:
  br label %bb._0x255, !notdec.evm !128

bb._0x255:                                        ; preds = %bb._0x26f
  br label %bb._0x27b, !notdec.evm !129

bb._0x27b:                                        ; preds = %bb._0x255
  br label %bb._0x260, !notdec.evm !130

bb._0x260:                                        ; preds = %bb._0x27b
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 %_0x26farg0x2, i256 %_0x26farg0x0, i256 %_0x26farg0x1), !notdec.evm !131
  %evm.add = add i256 %_0x26farg0x2, %_0x26farg0x1, !notdec.evm !132
  call void @evm_mstore(ptr %mem, i256 %evm.add, i256 0), !notdec.evm !133
  br label %bb._0x288, !notdec.evm !134

bb._0x288:                                        ; preds = %bb._0x260
  %evm.add1 = add i256 %_0x26farg0x2, %_0x26farg0x1, !notdec.evm !135
  ret i256 %evm.add1, !notdec.evm !136
}

define i256 @private__0x294_0x294(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x294arg0x0, i256 %_0x294arg0x1, i256 %_0x294arg0x2, i256 %_0x294arg0x3) {
bb._0x294:
  %private.call = call i256 @private__0x26f_0x26f(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x294arg0x2, i256 %_0x294arg0x1, i256 %_0x294arg0x0, i256 673), !notdec.evm !137
  br label %bb._0x2a1

bb._0x2a1:                                        ; preds = %bb._0x294
  ret i256 %private.call, !notdec.evm !138
}

define i256 @private__0x2f2_0x2f2(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x2f2arg0x0, i256 %_0x2f2arg0x1, i256 %_0x2f2arg0x2, i256 %_0x2f2arg0x3) {
bb._0x2f2:
  %private.call = call i256 @private__0x26f_0x26f(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x2f2arg0x2, i256 %_0x2f2arg0x1, i256 %_0x2f2arg0x0, i256 767), !notdec.evm !139
  br label %bb._0x2ff

bb._0x2ff:                                        ; preds = %bb._0x2f2
  ret i256 %private.call, !notdec.evm !140
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x12", !"op=SHL", !"evm.pc=0x12"}
!2 = !{!"tac=0x15", !"op=MLOAD", !"evm.pc=0x15"}
!3 = !{!"tac=0x18", !"op=ADD", !"evm.pc=0x18"}
!4 = !{!"tac=0x21", !"op=CALLPRIVATE", !"evm.pc=0x21"}
!5 = !{!"tac=0x25", !"op=MLOAD", !"evm.pc=0x25"}
!6 = !{!"tac=0x2a", !"op=SUB", !"evm.pc=0x2a"}
!7 = !{!"tac=0x2b", !"op=SUB", !"evm.pc=0x2b"}
!8 = !{!"tac=0x2d", !"op=MSTORE", !"evm.pc=0x2d"}
!9 = !{!"tac=0x31", !"op=MSTORE", !"evm.pc=0x31"}
!10 = !{!"tac=0x33", !"op=MLOAD", !"evm.pc=0x33"}
!11 = !{!"tac=0x37", !"op=ADD", !"evm.pc=0x37"}
!12 = !{!"tac=0x38", !"op=SHA3", !"evm.pc=0x38"}
!13 = !{!"tac=0x3b", !"op=CALLDATASIZE", !"evm.pc=0x3b"}
!14 = !{!"tac=0x3e", !"op=MLOAD", !"evm.pc=0x3e"}
!15 = !{!"tac=0x48", !"op=CALLPRIVATE", !"evm.pc=0x48"}
!16 = !{!"tac=0x4c", !"op=MLOAD", !"evm.pc=0x4c"}
!17 = !{!"tac=0x4f", !"op=SUB", !"evm.pc=0x4f"}
!18 = !{!"tac=0x51", !"op=SHA3", !"evm.pc=0x51"}
!19 = !{!"tac=0x52", !"op=EQ", !"evm.pc=0x52"}
!20 = !{!"tac=0x53", !"op=ISZERO", !"evm.pc=0x53"}
!21 = !{!"tac=0x57", !"op=JUMPI", !"evm.pc=0x57"}
!22 = !{!"tac=0x6e", !"op=CALLDATASIZE", !"evm.pc=0x6e"}
!23 = !{!"tac=0x71", !"op=LT", !"evm.pc=0x71"}
!24 = !{!"tac=0x72", !"op=ISZERO", !"evm.pc=0x72"}
!25 = !{!"tac=0x74", !"op=ISZERO", !"evm.pc=0x74"}
!26 = !{!"tac=0x78", !"op=JUMPI", !"evm.pc=0x78"}
!27 = !{!"tac=0x87", !"op=SHL", !"evm.pc=0x87"}
!28 = !{!"tac=0x8a", !"op=MLOAD", !"evm.pc=0x8a"}
!29 = !{!"tac=0x8d", !"op=ADD", !"evm.pc=0x8d"}
!30 = !{!"tac=0x96", !"op=CALLPRIVATE", !"evm.pc=0x96"}
!31 = !{!"tac=0x9a", !"op=MLOAD", !"evm.pc=0x9a"}
!32 = !{!"tac=0x9f", !"op=SUB", !"evm.pc=0x9f"}
!33 = !{!"tac=0xa0", !"op=SUB", !"evm.pc=0xa0"}
!34 = !{!"tac=0xa2", !"op=MSTORE", !"evm.pc=0xa2"}
!35 = !{!"tac=0xa6", !"op=MSTORE", !"evm.pc=0xa6"}
!36 = !{!"tac=0xa8", !"op=MLOAD", !"evm.pc=0xa8"}
!37 = !{!"tac=0xac", !"op=ADD", !"evm.pc=0xac"}
!38 = !{!"tac=0xad", !"op=SHA3", !"evm.pc=0xad"}
!39 = !{!"tac=0xb0", !"op=CALLDATASIZE", !"evm.pc=0xb0"}
!40 = !{!"tac=0xc1", !"op=JUMP", !"evm.pc=0xc1"}
!41 = !{!"tac=0x2c2", !"op=JUMPI", !"evm.pc=0x2c2"}
!42 = !{!"tac=0x2ce", !"op=GT", !"evm.pc=0x2ce"}
!43 = !{!"tac=0x2cf", !"op=ISZERO", !"evm.pc=0x2cf"}
!44 = !{!"tac=0x2d3", !"op=JUMPI", !"evm.pc=0x2d3"}
!45 = !{!"tac=0x2f1", !"op=JUMP", !"evm.pc=0x2f1"}
!46 = !{!"tac=0xc5", !"op=MLOAD", !"evm.pc=0xc5"}
!47 = !{!"tac=0xc8", !"op=ADD", !"evm.pc=0xc8"}
!48 = !{!"tac=0xd2", !"op=CALLPRIVATE", !"evm.pc=0xd2"}
!49 = !{!"tac=0xd6", !"op=MLOAD", !"evm.pc=0xd6"}
!50 = !{!"tac=0xdb", !"op=SUB", !"evm.pc=0xdb"}
!51 = !{!"tac=0xdc", !"op=SUB", !"evm.pc=0xdc"}
!52 = !{!"tac=0xde", !"op=MSTORE", !"evm.pc=0xde"}
!53 = !{!"tac=0xe2", !"op=MSTORE", !"evm.pc=0xe2"}
!54 = !{!"tac=0xe4", !"op=MLOAD", !"evm.pc=0xe4"}
!55 = !{!"tac=0xe8", !"op=ADD", !"evm.pc=0xe8"}
!56 = !{!"tac=0xe9", !"op=SHA3", !"evm.pc=0xe9"}
!57 = !{!"tac=0xea", !"op=EQ", !"evm.pc=0xea"}
!58 = !{!"tac=0x6c0", !"op=JUMP", !"evm.pc=0xeb"}
!59 = !{!"tac=0xeb_0x0", !"op=PHI"}
!60 = !{!"tac=0xec", !"op=ISZERO", !"evm.pc=0xec"}
!61 = !{!"tac=0xf0", !"op=JUMPI", !"evm.pc=0xf0"}
!62 = !{!"tac=0x1bb", !"op=SLOAD", !"evm.pc=0x1bb"}
!63 = !{!"tac=0x1c9", !"op=MSTORE", !"evm.pc=0x1c9"}
!64 = !{!"tac=0x1d1", !"op=GAS", !"evm.pc=0x1d1"}
!65 = !{!"tac=0x1d2", !"op=DELEGATECALL", !"evm.pc=0x1d2"}
!66 = !{!"tac=0x1d3", !"op=RETURNDATASIZE", !"evm.pc=0x1d3"}
!67 = !{!"tac=0x1d8", !"op=RETURNDATACOPY", !"evm.pc=0x1d8"}
!68 = !{!"tac=0x1dd", !"op=EQ", !"evm.pc=0x1dd"}
!69 = !{!"tac=0x1e1", !"op=JUMPI", !"evm.pc=0x1e1"}
!70 = !{!"tac=0x1ea", !"op=REVERT", !"evm.pc=0x1ea"}
!71 = !{!"tac=0x1e5", !"op=RETURN", !"evm.pc=0x1e5"}
!72 = !{!"tac=0x108", !"op=SLOAD", !"evm.pc=0x108"}
!73 = !{!"tac=0x11f", !"op=CALLER", !"evm.pc=0x11f"}
!74 = !{!"tac=0x120", !"op=EQ", !"evm.pc=0x120"}
!75 = !{!"tac=0x124", !"op=JUMPI", !"evm.pc=0x124"}
!76 = !{!"tac=0x12c", !"op=CALLVALUE", !"evm.pc=0x12c"}
!77 = !{!"tac=0x12d", !"op=GT", !"evm.pc=0x12d"}
!78 = !{!"tac=0x12e", !"op=ISZERO", !"evm.pc=0x12e"}
!79 = !{!"tac=0x132", !"op=JUMPI", !"evm.pc=0x132"}
!80 = !{!"tac=0x185", !"op=SSTORE", !"evm.pc=0x185"}
!81 = !{!"tac=0x188", !"op=SLOAD", !"evm.pc=0x188"}
!82 = !{!"tac=0x18b", !"op=CALLDATASIZE", !"evm.pc=0x18b"}
!83 = !{!"tac=0x18c", !"op=SUB", !"evm.pc=0x18c"}
!84 = !{!"tac=0x192", !"op=CALLDATACOPY", !"evm.pc=0x192"}
!85 = !{!"tac=0x19a", !"op=GAS", !"evm.pc=0x19a"}
!86 = !{!"tac=0x19b", !"op=DELEGATECALL", !"evm.pc=0x19b"}
!87 = !{!"tac=0x19c", !"op=RETURNDATASIZE", !"evm.pc=0x19c"}
!88 = !{!"tac=0x1a1", !"op=RETURNDATACOPY", !"evm.pc=0x1a1"}
!89 = !{!"tac=0x1a5", !"op=SSTORE", !"evm.pc=0x1a5"}
!90 = !{!"tac=0x1aa", !"op=EQ", !"evm.pc=0x1aa"}
!91 = !{!"tac=0x1ae", !"op=JUMPI", !"evm.pc=0x1ae"}
!92 = !{!"tac=0x1b7", !"op=REVERT", !"evm.pc=0x1b7"}
!93 = !{!"tac=0x1b2", !"op=RETURN", !"evm.pc=0x1b2"}
!94 = !{!"tac=0x142", !"op=CALLDATALOAD", !"evm.pc=0x142"}
!95 = !{!"tac=0x144", !"op=CALLDATALOAD", !"evm.pc=0x144"}
!96 = !{!"tac=0x146", !"op=CALLDATALOAD", !"evm.pc=0x146"}
!97 = !{!"tac=0x148", !"op=CALLDATALOAD", !"evm.pc=0x148"}
!98 = !{!"tac=0x14c", !"op=SHR", !"evm.pc=0x14c"}
!99 = !{!"tac=0x152", !"op=SHR", !"evm.pc=0x152"}
!100 = !{!"tac=0x15d", !"op=GAS", !"evm.pc=0x15d"}
!101 = !{!"tac=0x15e", !"op=CALL", !"evm.pc=0x15e"}
!102 = !{!"tac=0x163", !"op=JUMPI", !"evm.pc=0x163"}
!103 = !{!"tac=0x171", !"op=GAS", !"evm.pc=0x171"}
!104 = !{!"tac=0x172", !"op=CALL", !"evm.pc=0x172"}
!105 = !{!"tac=0x177", !"op=JUMPI", !"evm.pc=0x177"}
!106 = !{!"tac=0x180", !"op=RETURN", !"evm.pc=0x180"}
!107 = !{!"tac=0x17b", !"op=REVERT", !"evm.pc=0x17b"}
!108 = !{!"tac=0x167", !"op=REVERT", !"evm.pc=0x167"}
!109 = !{!"tac=0x128", !"op=REVERT", !"evm.pc=0x128"}
!110 = !{!"tac=0x2da", !"op=JUMP", !"evm.pc=0x2da"}
!111 = !{!"tac=0x2b6", !"op=REVERT", !"evm.pc=0x2b6"}
!112 = !{!"tac=0x2c9", !"op=JUMP", !"evm.pc=0x2c9"}
!113 = !{!"tac=0x2b1", !"op=REVERT", !"evm.pc=0x2b1"}
!114 = !{!"tac=0x5f", !"op=JUMPI", !"evm.pc=0x5f"}
!115 = !{!"tac=0x68", !"op=JUMP", !"evm.pc=0x68"}
!116 = !{!"tac=0x1ec", !"op=STOP", !"evm.pc=0x1ec"}
!117 = !{!"tac=0x63", !"op=JUMP", !"evm.pc=0x63"}
!118 = !{!"tac=0x245", !"op=JUMP", !"evm.pc=0x245"}
!119 = !{!"tac=0x22e", !"op=JUMP", !"evm.pc=0x22e"}
!120 = !{!"tac=0x212", !"op=AND", !"evm.pc=0x212"}
!121 = !{!"tac=0x218", !"op=JUMP", !"evm.pc=0x218"}
!122 = !{!"tac=0x233", !"op=JUMP", !"evm.pc=0x233"}
!123 = !{!"tac=0x222", !"op=JUMP", !"evm.pc=0x222"}
!124 = !{!"tac=0x236", !"op=MSTORE", !"evm.pc=0x236"}
!125 = !{!"tac=0x239", !"op=JUMP", !"evm.pc=0x239"}
!126 = !{!"tac=0x24a", !"op=ADD", !"evm.pc=0x24a"}
!127 = !{!"tac=0x254", !"op=RETURNPRIVATE", !"evm.pc=0x254"}
!128 = !{!"tac=0x27a", !"op=JUMP", !"evm.pc=0x27a"}
!129 = !{!"tac=0x25f", !"op=JUMP", !"evm.pc=0x25f"}
!130 = !{!"tac=0x287", !"op=JUMP", !"evm.pc=0x287"}
!131 = !{!"tac=0x264", !"op=CALLDATACOPY", !"evm.pc=0x264"}
!132 = !{!"tac=0x269", !"op=ADD", !"evm.pc=0x269"}
!133 = !{!"tac=0x26a", !"op=MSTORE", !"evm.pc=0x26a"}
!134 = !{!"tac=0x26e", !"op=JUMP", !"evm.pc=0x26e"}
!135 = !{!"tac=0x28b", !"op=ADD", !"evm.pc=0x28b"}
!136 = !{!"tac=0x293", !"op=RETURNPRIVATE", !"evm.pc=0x293"}
!137 = !{!"tac=0x2a0", !"op=CALLPRIVATE", !"evm.pc=0x2a0"}
!138 = !{!"tac=0x2ac", !"op=RETURNPRIVATE", !"evm.pc=0x2ac"}
!139 = !{!"tac=0x2fe", !"op=CALLPRIVATE", !"evm.pc=0x2fe"}
!140 = !{!"tac=0x30a", !"op=RETURNPRIVATE", !"evm.pc=0x30a"}
