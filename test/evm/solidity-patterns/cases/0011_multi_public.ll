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
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !2
  %evm.bool = zext i1 %evm.lt to i256, !notdec.evm !2
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !3
  br i1 %evm.branch.cond, label %bb._0x59, label %bb._0xd, !notdec.evm !3

bb._0x59:                                         ; preds = %bb._0x0
  %evm.calldatasize1 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.branch.cond2 = icmp ne i256 %evm.calldatasize1, 0, !notdec.evm !5
  br i1 %evm.branch.cond2, label %bb._0x60, label %bb._0x5f2a, !notdec.evm !5

bb._0x5f2a:                                       ; preds = %bb._0x59
  call void @public__0xeeeeeeee_0x5f6a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !6
  ret void

bb._0x60:                                         ; preds = %bb._0x59
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !7
  unreachable, !notdec.evm !7

bb._0xd:                                          ; preds = %bb._0x0
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !8
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !9
  %evm.eq = icmp eq i256 1474988470, %evm.shr, !notdec.evm !10
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !10
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !11
  br i1 %evm.branch.cond4, label %bb._0x692a, label %bb._0x1e, !notdec.evm !11

bb._0x692a:                                       ; preds = %bb._0xd
  call void @public_Withdraw___0x65(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  ret void

bb._0x1e:                                         ; preds = %bb._0xd
  %evm.eq5 = icmp eq i256 2150621658, %evm.shr, !notdec.evm !13
  %evm.bool6 = zext i1 %evm.eq5 to i256, !notdec.evm !13
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !14
  br i1 %evm.branch.cond7, label %bb._0x732a, label %bb._0x29, !notdec.evm !14

bb._0x732a:                                       ; preds = %bb._0x1e
  call void @public__0x802fe1da_0x7c(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !15
  ret void

bb._0x29:                                         ; preds = %bb._0x1e
  %evm.eq8 = icmp eq i256 3202289482, %evm.shr, !notdec.evm !16
  %evm.bool9 = zext i1 %evm.eq8 to i256, !notdec.evm !16
  %evm.branch.cond10 = icmp ne i256 %evm.bool9, 0, !notdec.evm !17
  br i1 %evm.branch.cond10, label %bb._0x7d2a, label %bb._0x34, !notdec.evm !17

bb._0x7d2a:                                       ; preds = %bb._0x29
  call void @public_Stop___0x9c(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !18
  ret void

bb._0x34:                                         ; preds = %bb._0x29
  %evm.eq11 = icmp eq i256 3225759480, %evm.shr, !notdec.evm !19
  %evm.bool12 = zext i1 %evm.eq11 to i256, !notdec.evm !19
  %evm.branch.cond13 = icmp ne i256 %evm.bool12, 0, !notdec.evm !20
  br i1 %evm.branch.cond13, label %bb._0x872a, label %bb._0x3f, !notdec.evm !20

bb._0x872a:                                       ; preds = %bb._0x34
  call void @public__0xc0452ef8_0xb8(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !21
  ret void

bb._0x3f:                                         ; preds = %bb._0x34
  %evm.eq14 = icmp eq i256 3942021817, %evm.shr, !notdec.evm !22
  %evm.bool15 = zext i1 %evm.eq14 to i256, !notdec.evm !22
  %evm.branch.cond16 = icmp ne i256 %evm.bool15, 0, !notdec.evm !23
  br i1 %evm.branch.cond16, label %bb._0x912a, label %bb._0x4a, !notdec.evm !23

bb._0x912a:                                       ; preds = %bb._0x3f
  call void @public__0xeaf67ab9_0xd8(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !24
  ret void

bb._0x4a:                                         ; preds = %bb._0x3f
  %evm.eq17 = icmp eq i256 4087188581, %evm.shr, !notdec.evm !25
  %evm.bool18 = zext i1 %evm.eq17 to i256, !notdec.evm !25
  %evm.branch.cond19 = icmp ne i256 %evm.bool18, 0, !notdec.evm !26
  br i1 %evm.branch.cond19, label %bb._0x192a, label %bb._0x55, !notdec.evm !26

bb._0x192a:                                       ; preds = %bb._0x4a
  call void @public__0xf39d8c65_0xe0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !27
  ret void

bb._0x55:                                         ; preds = %bb._0x4a
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !28
  unreachable, !notdec.evm !28
}

define void @private__0x107_0x107(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x107arg0x0) {
bb._0x107:
  %evm.sload = call i256 @evm_sload(i256 0), !notdec.evm !29
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !30
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !31
  %evm.and = and i256 %evm.sub, %evm.sload, !notdec.evm !32
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !33
  %evm.eq = icmp eq i256 %evm.caller, %evm.and, !notdec.evm !34
  %evm.bool = zext i1 %evm.eq to i256, !notdec.evm !34
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !35
  br i1 %evm.branch.cond, label %bb._0x166, label %bb._0x11a, !notdec.evm !35

bb._0x166:                                        ; preds = %bb._0x107
  call void @private__0x1dc_0x1dc(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 366), !notdec.evm !36
  br label %bb._0x16e

bb._0x16e:                                        ; preds = %bb._0x166
  ret void, !notdec.evm !37

bb._0x11a:                                        ; preds = %bb._0x107
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !38
  %evm.shl1 = call i256 @evm_shl(i256 229, i256 4594637), !notdec.evm !39
  call void @evm_mstore(ptr %mem, i256 %evm.mload, i256 %evm.shl1), !notdec.evm !40
  %evm.add = add i256 %evm.mload, 4, !notdec.evm !41
  call void @evm_mstore(ptr %mem, i256 %evm.add, i256 32), !notdec.evm !42
  %evm.add2 = add i256 %evm.mload, 36, !notdec.evm !43
  call void @evm_mstore(ptr %mem, i256 %evm.add2, i256 32), !notdec.evm !44
  %evm.add3 = add i256 %evm.mload, 68, !notdec.evm !45
  call void @evm_mstore(ptr %mem, i256 %evm.add3, i256 35943731656364841964516503116990081338611484598491072354577564874054038349170), !notdec.evm !46
  %evm.add4 = add i256 100, %evm.mload, !notdec.evm !47
  br label %bb._0x15d0x107, !notdec.evm !48

bb._0x15d0x107:                                   ; preds = %bb._0x11a
  %evm.mload5 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !49
  %evm.sub6 = sub i256 %evm.add4, %evm.mload5, !notdec.evm !50
  call void @evm_revert(ptr %mem, i256 %evm.mload5, i256 %evm.sub6), !notdec.evm !51
  unreachable, !notdec.evm !51
}

define void @private__0x170_0x170(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x170arg0x0) {
bb._0x170:
  %evm.sload = call i256 @evm_sload(i256 8), !notdec.evm !52
  br label %bb._0x17b, !notdec.evm !53

bb._0x17b:                                        ; preds = %bb._0x170
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !54
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !55
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !56
  %evm.and = and i256 %evm.sload, %evm.sub, !notdec.evm !57
  %evm.selfbalance = call i256 @evm_selfbalance(ptr %env), !notdec.evm !58
  %evm.iszero = icmp eq i256 %evm.selfbalance, 0, !notdec.evm !59
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !59
  %evm.mul = mul i256 2300, %evm.bool, !notdec.evm !60
  %evm.call = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.mul, i256 %evm.and, i256 %evm.selfbalance, i256 %evm.mload, i256 0, i256 %evm.mload, i256 0), !notdec.evm !61
  %evm.iszero1 = icmp eq i256 %evm.call, 0, !notdec.evm !62
  %evm.bool2 = zext i1 %evm.iszero1 to i256, !notdec.evm !62
  %evm.iszero3 = icmp eq i256 %evm.bool2, 0, !notdec.evm !63
  %evm.bool4 = zext i1 %evm.iszero3 to i256, !notdec.evm !63
  %evm.branch.cond = icmp ne i256 %evm.bool4, 0, !notdec.evm !64
  br i1 %evm.branch.cond, label %bb._0x1b3, label %bb._0x1aa, !notdec.evm !64

bb._0x1b3:                                        ; preds = %bb._0x17b
  ret void, !notdec.evm !65

bb._0x1aa:                                        ; preds = %bb._0x17b
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !66
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize), !notdec.evm !67
  %evm.returndatasize5 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !68
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize5), !notdec.evm !69
  unreachable, !notdec.evm !69
}

define i256 @private__0x1b7_0x1b7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x1b7arg0x0) {
bb._0x1b7:
  %evm.sload = call i256 @evm_sload(i256 3), !notdec.evm !70
  %evm.sload1 = call i256 @evm_sload(i256 0), !notdec.evm !71
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !72
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !73
  %evm.and = and i256 %evm.sub, %evm.sload1, !notdec.evm !74
  %evm.balance = call i256 @evm_balance(ptr %env, i256 %evm.and), !notdec.evm !75
  br label %bb._0x27e, !notdec.evm !76

bb._0x27e:                                        ; preds = %bb._0x1b7
  %evm.lt = icmp ult i256 %evm.balance, %evm.sload, !notdec.evm !77
  %evm.bool = zext i1 %evm.lt to i256, !notdec.evm !77
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !78
  %evm.bool2 = zext i1 %evm.iszero to i256, !notdec.evm !78
  %evm.branch.cond = icmp ne i256 %evm.bool2, 0, !notdec.evm !79
  br i1 %evm.branch.cond, label %bb._0x29e, label %bb._0x289, !notdec.evm !79

bb._0x29e:                                        ; preds = %bb._0x27e
  %evm.sub3 = sub i256 %evm.balance, %evm.sload, !notdec.evm !80
  br label %bb._0x1d6, !notdec.evm !81

bb._0x1d6:                                        ; preds = %bb._0x29e
  ret i256 %evm.sub3, !notdec.evm !82

bb._0x289:                                        ; preds = %bb._0x27e
  %evm.shl4 = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !83
  call void @evm_mstore(ptr %mem, i256 0, i256 %evm.shl4), !notdec.evm !84
  call void @evm_mstore(ptr %mem, i256 4, i256 17), !notdec.evm !85
  call void @evm_revert(ptr %mem, i256 0, i256 36), !notdec.evm !86
  unreachable, !notdec.evm !86
}

define void @private__0x1dc_0x1dc(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x1dcarg0x0) {
bb._0x1dc:
  %evm.sload = call i256 @evm_sload(i256 0), !notdec.evm !87
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !88
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !89
  %evm.and = and i256 %evm.sub, %evm.sload, !notdec.evm !90
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !91
  %evm.eq = icmp eq i256 %evm.caller, %evm.and, !notdec.evm !92
  %evm.bool = zext i1 %evm.eq to i256, !notdec.evm !92
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !93
  br i1 %evm.branch.cond, label %bb._0x236, label %bb._0x1ef, !notdec.evm !93

bb._0x236:                                        ; preds = %bb._0x1dc
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !94
  %evm.caller1 = call i256 @evm_caller(ptr %env), !notdec.evm !95
  %evm.selfbalance = call i256 @evm_selfbalance(ptr %env), !notdec.evm !96
  %evm.iszero = icmp eq i256 %evm.selfbalance, 0, !notdec.evm !97
  %evm.bool2 = zext i1 %evm.iszero to i256, !notdec.evm !97
  %evm.mul = mul i256 2300, %evm.bool2, !notdec.evm !98
  %evm.call = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.mul, i256 %evm.caller1, i256 %evm.selfbalance, i256 %evm.mload, i256 0, i256 %evm.mload, i256 0), !notdec.evm !99
  %evm.iszero3 = icmp eq i256 %evm.call, 0, !notdec.evm !100
  %evm.bool4 = zext i1 %evm.iszero3 to i256, !notdec.evm !100
  %evm.iszero5 = icmp eq i256 %evm.bool4, 0, !notdec.evm !101
  %evm.bool6 = zext i1 %evm.iszero5 to i256, !notdec.evm !101
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !102
  br i1 %evm.branch.cond7, label %bb._0x262, label %bb._0x259, !notdec.evm !102

bb._0x262:                                        ; preds = %bb._0x236
  ret void, !notdec.evm !103

bb._0x259:                                        ; preds = %bb._0x236
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !104
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize), !notdec.evm !105
  %evm.returndatasize8 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !106
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize8), !notdec.evm !107
  unreachable, !notdec.evm !107

bb._0x1ef:                                        ; preds = %bb._0x1dc
  %evm.mload9 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !108
  %evm.shl10 = call i256 @evm_shl(i256 229, i256 4594637), !notdec.evm !109
  call void @evm_mstore(ptr %mem, i256 %evm.mload9, i256 %evm.shl10), !notdec.evm !110
  %evm.add = add i256 %evm.mload9, 4, !notdec.evm !111
  call void @evm_mstore(ptr %mem, i256 %evm.add, i256 32), !notdec.evm !112
  %evm.add11 = add i256 %evm.mload9, 36, !notdec.evm !113
  call void @evm_mstore(ptr %mem, i256 %evm.add11, i256 32), !notdec.evm !114
  %evm.add12 = add i256 %evm.mload9, 68, !notdec.evm !115
  call void @evm_mstore(ptr %mem, i256 %evm.add12, i256 35943731656364841964516503116990081338611484598491072354577564874054038349170), !notdec.evm !116
  %evm.add13 = add i256 100, %evm.mload9, !notdec.evm !117
  br label %bb._0x15d0x1dc, !notdec.evm !118

bb._0x15d0x1dc:                                   ; preds = %bb._0x1ef
  %evm.mload14 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !119
  %evm.sub15 = sub i256 %evm.add13, %evm.mload14, !notdec.evm !120
  call void @evm_revert(ptr %mem, i256 %evm.mload14, i256 %evm.sub15), !notdec.evm !121
  unreachable, !notdec.evm !121
}

define i256 @private__0x265_0x265(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x265arg0x0, i256 %_0x265arg0x1, i256 %_0x265arg0x2) {
bb._0x265:
  %evm.sub = sub i256 %_0x265arg0x1, %_0x265arg0x0, !notdec.evm !122
  %evm.slt = icmp slt i256 %evm.sub, 32, !notdec.evm !123
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !123
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !124
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !124
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !125
  br i1 %evm.branch.cond, label %bb._0x277, label %bb._0x273, !notdec.evm !125

bb._0x277:                                        ; preds = %bb._0x265
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 %_0x265arg0x0), !notdec.evm !126
  ret i256 %evm.calldataload, !notdec.evm !127

bb._0x273:                                        ; preds = %bb._0x265
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !128
  unreachable, !notdec.evm !128
}

define void @public__0xeeeeeeee_0x5f6a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x5f6a:
  ret void, !notdec.evm !129
}

define void @public_Withdraw___0x65(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x65:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !130
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !131
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !131
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !132
  br i1 %evm.branch.cond, label %bb._0x71, label %bb._0x6d, !notdec.evm !132

bb._0x71:                                         ; preds = %bb._0x65
  call void @private__0x107_0x107(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 3072), !notdec.evm !133
  br label %bb._0xc00

bb._0xc00:                                        ; preds = %bb._0x71
  ret void, !notdec.evm !134

bb._0x6d:                                         ; preds = %bb._0x65
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !135
  unreachable, !notdec.evm !135
}

define void @public__0x802fe1da_0x7c(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x7c:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !136
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !137
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !137
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !138
  br i1 %evm.branch.cond, label %bb._0x88, label %bb._0x84, !notdec.evm !138

bb._0x88:                                         ; preds = %bb._0x7c
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !139
  %private.call = call i256 @private__0x265_0x265(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4, i256 %evm.calldatasize, i256 151), !notdec.evm !140
  br label %bb._0x97

bb._0x97:                                         ; preds = %bb._0x88
  call void @evm_sstore(i256 5, i256 %private.call), !notdec.evm !141
  br label %bb._0xc21, !notdec.evm !142

bb._0xc21:                                        ; preds = %bb._0x97
  ret void, !notdec.evm !143

bb._0x84:                                         ; preds = %bb._0x7c
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !144
  unreachable, !notdec.evm !144
}

define void @public_Stop___0x9c(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0x9c:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !145
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !146
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !146
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !147
  br i1 %evm.branch.cond, label %bb._0xa8, label %bb._0xa4, !notdec.evm !147

bb._0xa8:                                         ; preds = %bb._0x9c
  %evm.sload = call i256 @evm_sload(i256 4), !notdec.evm !148
  %evm.and = and i256 -256, %evm.sload, !notdec.evm !149
  call void @evm_sstore(i256 4, i256 %evm.and), !notdec.evm !150
  br label %bb._0xc42, !notdec.evm !151

bb._0xc42:                                        ; preds = %bb._0xa8
  ret void, !notdec.evm !152

bb._0xa4:                                         ; preds = %bb._0x9c
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !153
  unreachable, !notdec.evm !153
}

define void @public__0xc0452ef8_0xb8(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0xb8:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !154
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !155
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !155
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !156
  br i1 %evm.branch.cond, label %bb._0xc4, label %bb._0xc0, !notdec.evm !156

bb._0xc4:                                         ; preds = %bb._0xb8
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !157
  %private.call = call i256 @private__0x265_0x265(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4, i256 %evm.calldatasize, i256 211), !notdec.evm !158
  br label %bb._0xd3

bb._0xd3:                                         ; preds = %bb._0xc4
  call void @evm_sstore(i256 6, i256 %private.call), !notdec.evm !159
  br label %bb._0xc63, !notdec.evm !160

bb._0xc63:                                        ; preds = %bb._0xd3
  ret void, !notdec.evm !161

bb._0xc0:                                         ; preds = %bb._0xb8
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !162
  unreachable, !notdec.evm !162
}

define void @public__0xeaf67ab9_0xd8(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0xd8:
  call void @private__0x170_0x170(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 3204), !notdec.evm !163
  br label %bb._0xc84

bb._0xc84:                                        ; preds = %bb._0xd8
  ret void, !notdec.evm !164
}

define void @public__0xf39d8c65_0xe0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
bb._0xe0:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !165
  %evm.iszero = icmp eq i256 %evm.callvalue, 0, !notdec.evm !166
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !166
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !167
  br i1 %evm.branch.cond, label %bb._0xec, label %bb._0xe8, !notdec.evm !167

bb._0xec:                                         ; preds = %bb._0xe0
  %private.call = call i256 @private__0x1b7_0x1b7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 245), !notdec.evm !168
  br label %bb._0xf5

bb._0xf5:                                         ; preds = %bb._0xec
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !169
  call void @evm_mstore(ptr %mem, i256 %evm.mload, i256 %private.call), !notdec.evm !170
  %evm.add = add i256 32, %evm.mload, !notdec.evm !171
  %evm.mload1 = call i256 @evm_mload(ptr %mem, i256 64), !notdec.evm !172
  %evm.sub = sub i256 %evm.add, %evm.mload1, !notdec.evm !173
  call void @evm_return(ptr %mem, i256 %evm.mload1, i256 %evm.sub), !notdec.evm !174
  ret void, !notdec.evm !174

bb._0xe8:                                         ; preds = %bb._0xe0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !175
  unreachable, !notdec.evm !175
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x7", !"op=CALLDATASIZE", !"evm.pc=0x7"}
!2 = !{!"tac=0x8", !"op=LT", !"evm.pc=0x8"}
!3 = !{!"tac=0xc", !"op=JUMPI", !"evm.pc=0xc"}
!4 = !{!"tac=0x5a", !"op=CALLDATASIZE", !"evm.pc=0x5a"}
!5 = !{!"tac=0x5e", !"op=JUMPI", !"evm.pc=0x5e"}
!6 = !{!"tac=0x5f4a", !"op=CALLPRIVATE", !"evm.pc=0x5f"}
!7 = !{!"tac=0x64", !"op=REVERT", !"evm.pc=0x64"}
!8 = !{!"tac=0xf", !"op=CALLDATALOAD", !"evm.pc=0xf"}
!9 = !{!"tac=0x12", !"op=SHR", !"evm.pc=0x12"}
!10 = !{!"tac=0x19", !"op=EQ", !"evm.pc=0x19"}
!11 = !{!"tac=0x234a", !"op=JUMPI", !"evm.pc=0x1a"}
!12 = !{!"tac=0x696a", !"op=CALLPRIVATE", !"evm.pc=0x65"}
!13 = !{!"tac=0x24", !"op=EQ", !"evm.pc=0x24"}
!14 = !{!"tac=0x2d4a", !"op=JUMPI", !"evm.pc=0x25"}
!15 = !{!"tac=0x736a", !"op=CALLPRIVATE", !"evm.pc=0x7c"}
!16 = !{!"tac=0x2f", !"op=EQ", !"evm.pc=0x2f"}
!17 = !{!"tac=0x374a", !"op=JUMPI", !"evm.pc=0x30"}
!18 = !{!"tac=0x7d6a", !"op=CALLPRIVATE", !"evm.pc=0x9c"}
!19 = !{!"tac=0x3a", !"op=EQ", !"evm.pc=0x3a"}
!20 = !{!"tac=0x414a", !"op=JUMPI", !"evm.pc=0x3b"}
!21 = !{!"tac=0x876a", !"op=CALLPRIVATE", !"evm.pc=0xb8"}
!22 = !{!"tac=0x45", !"op=EQ", !"evm.pc=0x45"}
!23 = !{!"tac=0x4b4a", !"op=JUMPI", !"evm.pc=0x46"}
!24 = !{!"tac=0x916a", !"op=CALLPRIVATE", !"evm.pc=0xd8"}
!25 = !{!"tac=0x50", !"op=EQ", !"evm.pc=0x50"}
!26 = !{!"tac=0x554a", !"op=JUMPI", !"evm.pc=0x51"}
!27 = !{!"tac=0x196a", !"op=CALLPRIVATE", !"evm.pc=0xe0"}
!28 = !{!"tac=0x58", !"op=REVERT", !"evm.pc=0x58"}
!29 = !{!"tac=0x10a", !"op=SLOAD", !"evm.pc=0x10a"}
!30 = !{!"tac=0x111", !"op=SHL", !"evm.pc=0x111"}
!31 = !{!"tac=0x112", !"op=SUB", !"evm.pc=0x112"}
!32 = !{!"tac=0x113", !"op=AND", !"evm.pc=0x113"}
!33 = !{!"tac=0x114", !"op=CALLER", !"evm.pc=0x114"}
!34 = !{!"tac=0x115", !"op=EQ", !"evm.pc=0x115"}
!35 = !{!"tac=0x119", !"op=JUMPI", !"evm.pc=0x119"}
!36 = !{!"tac=0x16d", !"op=CALLPRIVATE", !"evm.pc=0x16d"}
!37 = !{!"tac=0x16f", !"op=RETURNPRIVATE", !"evm.pc=0x16f"}
!38 = !{!"tac=0x11c", !"op=MLOAD", !"evm.pc=0x11c"}
!39 = !{!"tac=0x123", !"op=SHL", !"evm.pc=0x123"}
!40 = !{!"tac=0x125", !"op=MSTORE", !"evm.pc=0x125"}
!41 = !{!"tac=0x12b", !"op=ADD", !"evm.pc=0x12b"}
!42 = !{!"tac=0x12e", !"op=MSTORE", !"evm.pc=0x12e"}
!43 = !{!"tac=0x132", !"op=ADD", !"evm.pc=0x132"}
!44 = !{!"tac=0x133", !"op=MSTORE", !"evm.pc=0x133"}
!45 = !{!"tac=0x158", !"op=ADD", !"evm.pc=0x158"}
!46 = !{!"tac=0x159", !"op=MSTORE", !"evm.pc=0x159"}
!47 = !{!"tac=0x15c", !"op=ADD", !"evm.pc=0x15c"}
!48 = !{!"tac=0x5f0", !"op=JUMP", !"evm.pc=0x15d"}
!49 = !{!"tac=0x1600x107", !"op=MLOAD", !"evm.pc=0x160"}
!50 = !{!"tac=0x1630x107", !"op=SUB", !"evm.pc=0x163"}
!51 = !{!"tac=0x1650x107", !"op=REVERT", !"evm.pc=0x165"}
!52 = !{!"tac=0x178", !"op=SLOAD", !"evm.pc=0x178"}
!53 = !{!"tac=0x17a", !"op=JUMP", !"evm.pc=0x17a"}
!54 = !{!"tac=0x17e", !"op=MLOAD", !"evm.pc=0x17e"}
!55 = !{!"tac=0x188", !"op=SHL", !"evm.pc=0x188"}
!56 = !{!"tac=0x189", !"op=SUB", !"evm.pc=0x189"}
!57 = !{!"tac=0x18b", !"op=AND", !"evm.pc=0x18b"}
!58 = !{!"tac=0x18d", !"op=SELFBALANCE", !"evm.pc=0x18d"}
!59 = !{!"tac=0x18f", !"op=ISZERO", !"evm.pc=0x18f"}
!60 = !{!"tac=0x193", !"op=MUL", !"evm.pc=0x193"}
!61 = !{!"tac=0x19d", !"op=CALL", !"evm.pc=0x19d"}
!62 = !{!"tac=0x1a3", !"op=ISZERO", !"evm.pc=0x1a3"}
!63 = !{!"tac=0x1a5", !"op=ISZERO", !"evm.pc=0x1a5"}
!64 = !{!"tac=0x1a9", !"op=JUMPI", !"evm.pc=0x1a9"}
!65 = !{!"tac=0x1b6", !"op=RETURNPRIVATE", !"evm.pc=0x1b6"}
!66 = !{!"tac=0x1aa", !"op=RETURNDATASIZE", !"evm.pc=0x1aa"}
!67 = !{!"tac=0x1ae", !"op=RETURNDATACOPY", !"evm.pc=0x1ae"}
!68 = !{!"tac=0x1af", !"op=RETURNDATASIZE", !"evm.pc=0x1af"}
!69 = !{!"tac=0x1b2", !"op=REVERT", !"evm.pc=0x1b2"}
!70 = !{!"tac=0x1ba", !"op=SLOAD", !"evm.pc=0x1ba"}
!71 = !{!"tac=0x1be", !"op=SLOAD", !"evm.pc=0x1be"}
!72 = !{!"tac=0x1ce", !"op=SHL", !"evm.pc=0x1ce"}
!73 = !{!"tac=0x1cf", !"op=SUB", !"evm.pc=0x1cf"}
!74 = !{!"tac=0x1d0", !"op=AND", !"evm.pc=0x1d0"}
!75 = !{!"tac=0x1d1", !"op=BALANCE", !"evm.pc=0x1d1"}
!76 = !{!"tac=0x1d5", !"op=JUMP", !"evm.pc=0x1d5"}
!77 = !{!"tac=0x283", !"op=LT", !"evm.pc=0x283"}
!78 = !{!"tac=0x284", !"op=ISZERO", !"evm.pc=0x284"}
!79 = !{!"tac=0x288", !"op=JUMPI", !"evm.pc=0x288"}
!80 = !{!"tac=0x2a0", !"op=SUB", !"evm.pc=0x2a0"}
!81 = !{!"tac=0x2a2", !"op=JUMP", !"evm.pc=0x2a2"}
!82 = !{!"tac=0x1db", !"op=RETURNPRIVATE", !"evm.pc=0x1db"}
!83 = !{!"tac=0x290", !"op=SHL", !"evm.pc=0x290"}
!84 = !{!"tac=0x293", !"op=MSTORE", !"evm.pc=0x293"}
!85 = !{!"tac=0x298", !"op=MSTORE", !"evm.pc=0x298"}
!86 = !{!"tac=0x29d", !"op=REVERT", !"evm.pc=0x29d"}
!87 = !{!"tac=0x1df", !"op=SLOAD", !"evm.pc=0x1df"}
!88 = !{!"tac=0x1e6", !"op=SHL", !"evm.pc=0x1e6"}
!89 = !{!"tac=0x1e7", !"op=SUB", !"evm.pc=0x1e7"}
!90 = !{!"tac=0x1e8", !"op=AND", !"evm.pc=0x1e8"}
!91 = !{!"tac=0x1e9", !"op=CALLER", !"evm.pc=0x1e9"}
!92 = !{!"tac=0x1ea", !"op=EQ", !"evm.pc=0x1ea"}
!93 = !{!"tac=0x1ee", !"op=JUMPI", !"evm.pc=0x1ee"}
!94 = !{!"tac=0x239", !"op=MLOAD", !"evm.pc=0x239"}
!95 = !{!"tac=0x23a", !"op=CALLER", !"evm.pc=0x23a"}
!96 = !{!"tac=0x23c", !"op=SELFBALANCE", !"evm.pc=0x23c"}
!97 = !{!"tac=0x23e", !"op=ISZERO", !"evm.pc=0x23e"}
!98 = !{!"tac=0x242", !"op=MUL", !"evm.pc=0x242"}
!99 = !{!"tac=0x24c", !"op=CALL", !"evm.pc=0x24c"}
!100 = !{!"tac=0x252", !"op=ISZERO", !"evm.pc=0x252"}
!101 = !{!"tac=0x254", !"op=ISZERO", !"evm.pc=0x254"}
!102 = !{!"tac=0x258", !"op=JUMPI", !"evm.pc=0x258"}
!103 = !{!"tac=0x264", !"op=RETURNPRIVATE", !"evm.pc=0x264"}
!104 = !{!"tac=0x259", !"op=RETURNDATASIZE", !"evm.pc=0x259"}
!105 = !{!"tac=0x25d", !"op=RETURNDATACOPY", !"evm.pc=0x25d"}
!106 = !{!"tac=0x25e", !"op=RETURNDATASIZE", !"evm.pc=0x25e"}
!107 = !{!"tac=0x261", !"op=REVERT", !"evm.pc=0x261"}
!108 = !{!"tac=0x1f1", !"op=MLOAD", !"evm.pc=0x1f1"}
!109 = !{!"tac=0x1f8", !"op=SHL", !"evm.pc=0x1f8"}
!110 = !{!"tac=0x1fa", !"op=MSTORE", !"evm.pc=0x1fa"}
!111 = !{!"tac=0x200", !"op=ADD", !"evm.pc=0x200"}
!112 = !{!"tac=0x203", !"op=MSTORE", !"evm.pc=0x203"}
!113 = !{!"tac=0x207", !"op=ADD", !"evm.pc=0x207"}
!114 = !{!"tac=0x208", !"op=MSTORE", !"evm.pc=0x208"}
!115 = !{!"tac=0x22d", !"op=ADD", !"evm.pc=0x22d"}
!116 = !{!"tac=0x22e", !"op=MSTORE", !"evm.pc=0x22e"}
!117 = !{!"tac=0x231", !"op=ADD", !"evm.pc=0x231"}
!118 = !{!"tac=0x235", !"op=JUMP", !"evm.pc=0x235"}
!119 = !{!"tac=0x1600x1dc", !"op=MLOAD", !"evm.pc=0x160"}
!120 = !{!"tac=0x1630x1dc", !"op=SUB", !"evm.pc=0x163"}
!121 = !{!"tac=0x1650x1dc", !"op=REVERT", !"evm.pc=0x165"}
!122 = !{!"tac=0x26c", !"op=SUB", !"evm.pc=0x26c"}
!123 = !{!"tac=0x26d", !"op=SLT", !"evm.pc=0x26d"}
!124 = !{!"tac=0x26e", !"op=ISZERO", !"evm.pc=0x26e"}
!125 = !{!"tac=0x272", !"op=JUMPI", !"evm.pc=0x272"}
!126 = !{!"tac=0x279", !"op=CALLDATALOAD", !"evm.pc=0x279"}
!127 = !{!"tac=0x27d", !"op=RETURNPRIVATE", !"evm.pc=0x27d"}
!128 = !{!"tac=0x276", !"op=REVERT", !"evm.pc=0x276"}
!129 = !{!"tac=0x5f", !"op=STOP", !"evm.pc=0x5f"}
!130 = !{!"tac=0x66", !"op=CALLVALUE", !"evm.pc=0x66"}
!131 = !{!"tac=0x68", !"op=ISZERO", !"evm.pc=0x68"}
!132 = !{!"tac=0x6c", !"op=JUMPI", !"evm.pc=0x6c"}
!133 = !{!"tac=0x79", !"op=CALLPRIVATE", !"evm.pc=0x79"}
!134 = !{!"tac=0xc01", !"op=STOP", !"evm.pc=0x7b"}
!135 = !{!"tac=0x70", !"op=REVERT", !"evm.pc=0x70"}
!136 = !{!"tac=0x7d", !"op=CALLVALUE", !"evm.pc=0x7d"}
!137 = !{!"tac=0x7f", !"op=ISZERO", !"evm.pc=0x7f"}
!138 = !{!"tac=0x83", !"op=JUMPI", !"evm.pc=0x83"}
!139 = !{!"tac=0x90", !"op=CALLDATASIZE", !"evm.pc=0x90"}
!140 = !{!"tac=0x96", !"op=CALLPRIVATE", !"evm.pc=0x96"}
!141 = !{!"tac=0x9a", !"op=SSTORE", !"evm.pc=0x9a"}
!142 = !{!"tac=0x9b", !"op=JUMP", !"evm.pc=0x9b"}
!143 = !{!"tac=0xc22", !"op=STOP", !"evm.pc=0x7b"}
!144 = !{!"tac=0x87", !"op=REVERT", !"evm.pc=0x87"}
!145 = !{!"tac=0x9d", !"op=CALLVALUE", !"evm.pc=0x9d"}
!146 = !{!"tac=0x9f", !"op=ISZERO", !"evm.pc=0x9f"}
!147 = !{!"tac=0xa3", !"op=JUMPI", !"evm.pc=0xa3"}
!148 = !{!"tac=0xb0", !"op=SLOAD", !"evm.pc=0xb0"}
!149 = !{!"tac=0xb4", !"op=AND", !"evm.pc=0xb4"}
!150 = !{!"tac=0xb6", !"op=SSTORE", !"evm.pc=0xb6"}
!151 = !{!"tac=0xb7", !"op=JUMP", !"evm.pc=0xb7"}
!152 = !{!"tac=0xc43", !"op=STOP", !"evm.pc=0x7b"}
!153 = !{!"tac=0xa7", !"op=REVERT", !"evm.pc=0xa7"}
!154 = !{!"tac=0xb9", !"op=CALLVALUE", !"evm.pc=0xb9"}
!155 = !{!"tac=0xbb", !"op=ISZERO", !"evm.pc=0xbb"}
!156 = !{!"tac=0xbf", !"op=JUMPI", !"evm.pc=0xbf"}
!157 = !{!"tac=0xcc", !"op=CALLDATASIZE", !"evm.pc=0xcc"}
!158 = !{!"tac=0xd2", !"op=CALLPRIVATE", !"evm.pc=0xd2"}
!159 = !{!"tac=0xd6", !"op=SSTORE", !"evm.pc=0xd6"}
!160 = !{!"tac=0xd7", !"op=JUMP", !"evm.pc=0xd7"}
!161 = !{!"tac=0xc64", !"op=STOP", !"evm.pc=0x7b"}
!162 = !{!"tac=0xc3", !"op=REVERT", !"evm.pc=0xc3"}
!163 = !{!"tac=0xdf", !"op=CALLPRIVATE", !"evm.pc=0xdf"}
!164 = !{!"tac=0xc85", !"op=STOP", !"evm.pc=0x7b"}
!165 = !{!"tac=0xe1", !"op=CALLVALUE", !"evm.pc=0xe1"}
!166 = !{!"tac=0xe3", !"op=ISZERO", !"evm.pc=0xe3"}
!167 = !{!"tac=0xe7", !"op=JUMPI", !"evm.pc=0xe7"}
!168 = !{!"tac=0xf4", !"op=CALLPRIVATE", !"evm.pc=0xf4"}
!169 = !{!"tac=0xf8", !"op=MLOAD", !"evm.pc=0xf8"}
!170 = !{!"tac=0xfb", !"op=MSTORE", !"evm.pc=0xfb"}
!171 = !{!"tac=0xfe", !"op=ADD", !"evm.pc=0xfe"}
!172 = !{!"tac=0x101", !"op=MLOAD", !"evm.pc=0x101"}
!173 = !{!"tac=0x104", !"op=SUB", !"evm.pc=0x104"}
!174 = !{!"tac=0x106", !"op=RETURN", !"evm.pc=0x106"}
!175 = !{!"tac=0xeb", !"op=REVERT", !"evm.pc=0xeb"}
