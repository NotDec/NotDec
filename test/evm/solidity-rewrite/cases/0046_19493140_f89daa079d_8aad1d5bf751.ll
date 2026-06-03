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
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !1
  %evm.iszero = icmp eq i256 %evm.calldatasize, 0, !notdec.evm !2
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !2
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !3
  br i1 %evm.branch.cond, label %bb._0x16, label %bb._0xc, !notdec.evm !3

bb._0x16:                                         ; preds = %bb._0x0
  %evm.shl = call i256 @evm_shl(i256 224, i256 1549851163), !notdec.evm !4
  %notdec.evm.mem.ptr.1 = inttoptr i256 128 to ptr
  store i256 %evm.shl, ptr %notdec.evm.mem.ptr.1, align 1, !notdec.evm !5
  %evm.shl1 = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !6
  %evm.sub = sub i256 %evm.shl1, 1, !notdec.evm !7
  %evm.and = and i256 %evm.sub, 203756526994938756795271729070071115146827865755, !notdec.evm !8
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !9
  %evm.staticcall = call i256 @evm_staticcall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 %evm.and, i256 128, i256 4, i256 128, i256 32), !notdec.evm !10
  %evm.iszero2 = icmp eq i256 %evm.staticcall, 0, !notdec.evm !11
  %evm.bool3 = zext i1 %evm.iszero2 to i256, !notdec.evm !11
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !12
  br i1 %evm.branch.cond4, label %bb._0x452, label %bb._0x5a, !notdec.evm !12

bb._0x452:                                        ; preds = %bb._0x16
  %notdec.evm.mem.ptr.2 = inttoptr i256 64 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.2, align 1, !notdec.evm !13
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !14
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 %evm.mload, i256 0, i256 %evm.returndatasize), !notdec.evm !15
  %evm.returndatasize5 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !16
  call void @evm_revert(ptr %mem, i256 %evm.mload, i256 %evm.returndatasize5), !notdec.evm !17
  unreachable, !notdec.evm !17

bb._0x5a:                                         ; preds = %bb._0x16
  %evm.branch.cond6 = icmp ne i256 %evm.staticcall, 0, !notdec.evm !18
  br i1 %evm.branch.cond6, label %bb._0x71, label %bb._0x61, !notdec.evm !18

bb._0x71:                                         ; preds = %bb._0x5a
  %evm.returndatasize7 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !19
  %evm.gt = icmp ugt i256 32, %evm.returndatasize7, !notdec.evm !20
  %evm.bool8 = zext i1 %evm.gt to i256, !notdec.evm !20
  %evm.branch.cond9 = icmp ne i256 %evm.bool8, 0, !notdec.evm !21
  br i1 %evm.branch.cond9, label %bb._0xc6, label %bb._0x7c, !notdec.evm !21

bb._0xc6:                                         ; preds = %bb._0x71
  %evm.returndatasize10 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !22
  br label %bb._0x7c, !notdec.evm !23

bb._0x7c:                                         ; preds = %bb._0xc6, %bb._0x71
  %_0x7c_0x2 = phi i256 [ 32, %bb._0x71 ], [ %evm.returndatasize10, %bb._0xc6 ], !notdec.evm !24
  %evm.add = add i256 %_0x7c_0x2, 31, !notdec.evm !25
  %evm.and11 = and i256 -32, %evm.add, !notdec.evm !26
  %evm.add12 = add i256 128, %evm.and11, !notdec.evm !27
  %evm.gt13 = icmp ugt i256 %evm.add12, 18446744073709551615, !notdec.evm !28
  %evm.bool14 = zext i1 %evm.gt13 to i256, !notdec.evm !28
  %evm.lt = icmp ult i256 %evm.add12, 128, !notdec.evm !29
  %evm.bool15 = zext i1 %evm.lt to i256, !notdec.evm !29
  %evm.or = or i256 %evm.bool15, %evm.bool14, !notdec.evm !30
  %evm.branch.cond16 = icmp ne i256 %evm.or, 0, !notdec.evm !31
  br i1 %evm.branch.cond16, label %bb._0x47d, label %bb._0x9b, !notdec.evm !31

bb._0x47d:                                        ; preds = %bb._0x7c
  %_0x47d_0x3 = phi i256 [ %_0x7c_0x2, %bb._0x7c ], !notdec.evm !32
  %evm.shl17 = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !33
  %notdec.evm.mem.ptr.3 = inttoptr i256 0 to ptr
  store i256 %evm.shl17, ptr %notdec.evm.mem.ptr.3, align 1, !notdec.evm !34
  %notdec.evm.mem.ptr.4 = inttoptr i256 4 to ptr
  store i256 65, ptr %notdec.evm.mem.ptr.4, align 1, !notdec.evm !35
  call void @evm_revert(ptr %mem, i256 0, i256 36), !notdec.evm !36
  unreachable, !notdec.evm !36

bb._0x9b:                                         ; preds = %bb._0x7c
  %_0x9b_0x3 = phi i256 [ %_0x7c_0x2, %bb._0x7c ], !notdec.evm !37
  %notdec.evm.mem.ptr.5 = inttoptr i256 64 to ptr
  store i256 %evm.add12, ptr %notdec.evm.mem.ptr.5, align 1, !notdec.evm !38
  %evm.add18 = add i256 128, %_0x9b_0x3, !notdec.evm !39
  %private.call = call i256 @private__0x19e_0x19e(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.add18, i256 172), !notdec.evm !40
  br label %bb._0xac

bb._0xac:                                         ; preds = %bb._0x9b
  br label %bb._0x61, !notdec.evm !41

bb._0x61:                                         ; preds = %bb._0xac, %bb._0x5a
  %_0x61_0x0 = phi i256 [ 128, %bb._0x5a ], [ 2881, %bb._0xac ], !notdec.evm !42
  %_0x61_0x1 = phi i256 [ 0, %bb._0x5a ], [ %private.call, %bb._0xac ], !notdec.evm !43
  br label %bb._0x17d, !notdec.evm !44

bb._0xc:                                          ; preds = %bb._0x0
  br label %bb._0xda, !notdec.evm !45

bb._0xda:                                         ; preds = %bb._0xc
  %notdec.evm.mem.ptr.6 = inttoptr i256 64 to ptr
  %evm.mload19 = load i256, ptr %notdec.evm.mem.ptr.6, align 1, !notdec.evm !46
  %evm.shl20 = call i256 @evm_shl(i256 224, i256 1549851163), !notdec.evm !47
  %notdec.evm.mem.ptr.7 = inttoptr i256 %evm.mload19 to ptr
  store i256 %evm.shl20, ptr %notdec.evm.mem.ptr.7, align 1, !notdec.evm !48
  %evm.shl21 = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !49
  %evm.sub22 = sub i256 %evm.shl21, 1, !notdec.evm !50
  %evm.and23 = and i256 %evm.sub22, 203756526994938756795271729070071115146827865755, !notdec.evm !51
  %evm.gas24 = call i256 @evm_gas(ptr %env), !notdec.evm !52
  %evm.staticcall25 = call i256 @evm_staticcall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas24, i256 %evm.and23, i256 %evm.mload19, i256 4, i256 %evm.mload19, i256 32), !notdec.evm !53
  %evm.iszero26 = icmp eq i256 %evm.staticcall25, 0, !notdec.evm !54
  %evm.bool27 = zext i1 %evm.iszero26 to i256, !notdec.evm !54
  %evm.branch.cond28 = icmp ne i256 %evm.bool27, 0, !notdec.evm !55
  br i1 %evm.branch.cond28, label %bb._0x4b0, label %bb._0x120, !notdec.evm !55

bb._0x4b0:                                        ; preds = %bb._0xda
  %notdec.evm.mem.ptr.8 = inttoptr i256 64 to ptr
  %evm.mload29 = load i256, ptr %notdec.evm.mem.ptr.8, align 1, !notdec.evm !56
  %evm.returndatasize30 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !57
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 %evm.mload29, i256 0, i256 %evm.returndatasize30), !notdec.evm !58
  %evm.returndatasize31 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !59
  call void @evm_revert(ptr %mem, i256 %evm.mload29, i256 %evm.returndatasize31), !notdec.evm !60
  unreachable, !notdec.evm !60

bb._0x120:                                        ; preds = %bb._0xda
  %evm.branch.cond32 = icmp ne i256 %evm.staticcall25, 0, !notdec.evm !61
  br i1 %evm.branch.cond32, label %bb._0x133, label %bb._0x127, !notdec.evm !61

bb._0x133:                                        ; preds = %bb._0x120
  %evm.returndatasize33 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !62
  %evm.gt34 = icmp ugt i256 32, %evm.returndatasize33, !notdec.evm !63
  %evm.bool35 = zext i1 %evm.gt34 to i256, !notdec.evm !63
  %evm.branch.cond36 = icmp ne i256 %evm.bool35, 0, !notdec.evm !64
  br i1 %evm.branch.cond36, label %bb._0x175, label %bb._0x13e, !notdec.evm !64

bb._0x175:                                        ; preds = %bb._0x133
  %evm.returndatasize37 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !65
  br label %bb._0x13e, !notdec.evm !66

bb._0x13e:                                        ; preds = %bb._0x175, %bb._0x133
  %_0x13e_0x1 = phi i256 [ 32, %bb._0x133 ], [ %evm.returndatasize37, %bb._0x175 ], !notdec.evm !67
  %evm.add38 = add i256 %_0x13e_0x1, 31, !notdec.evm !68
  %evm.and39 = and i256 -32, %evm.add38, !notdec.evm !69
  %evm.add40 = add i256 %evm.mload19, %evm.and39, !notdec.evm !70
  %evm.gt41 = icmp ugt i256 %evm.add40, 18446744073709551615, !notdec.evm !71
  %evm.bool42 = zext i1 %evm.gt41 to i256, !notdec.evm !71
  %evm.lt43 = icmp ult i256 %evm.add40, %evm.mload19, !notdec.evm !72
  %evm.bool44 = zext i1 %evm.lt43 to i256, !notdec.evm !72
  %evm.or45 = or i256 %evm.bool44, %evm.bool42, !notdec.evm !73
  %evm.branch.cond46 = icmp ne i256 %evm.or45, 0, !notdec.evm !74
  br i1 %evm.branch.cond46, label %bb._0x4db, label %bb._0x15d, !notdec.evm !74

bb._0x4db:                                        ; preds = %bb._0x13e
  %_0x4db_0x2 = phi i256 [ %_0x13e_0x1, %bb._0x13e ], !notdec.evm !75
  %evm.shl47 = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !76
  %notdec.evm.mem.ptr.9 = inttoptr i256 0 to ptr
  store i256 %evm.shl47, ptr %notdec.evm.mem.ptr.9, align 1, !notdec.evm !77
  %notdec.evm.mem.ptr.10 = inttoptr i256 4 to ptr
  store i256 65, ptr %notdec.evm.mem.ptr.10, align 1, !notdec.evm !78
  call void @evm_revert(ptr %mem, i256 0, i256 36), !notdec.evm !79
  unreachable, !notdec.evm !79

bb._0x15d:                                        ; preds = %bb._0x13e
  %_0x15d_0x2 = phi i256 [ %_0x13e_0x1, %bb._0x13e ], !notdec.evm !80
  %notdec.evm.mem.ptr.11 = inttoptr i256 64 to ptr
  store i256 %evm.add40, ptr %notdec.evm.mem.ptr.11, align 1, !notdec.evm !81
  %evm.add48 = add i256 %evm.mload19, %_0x15d_0x2, !notdec.evm !82
  %private.call49 = call i256 @private__0x1c5_0x1c5(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.mload19, i256 %evm.add48, i256 368), !notdec.evm !83
  br label %bb._0x170

bb._0x170:                                        ; preds = %bb._0x15d
  br label %bb._0x127, !notdec.evm !84

bb._0x127:                                        ; preds = %bb._0x170, %bb._0x120
  %_0x127_0x0 = phi i256 [ 0, %bb._0x120 ], [ %private.call49, %bb._0x170 ], !notdec.evm !85
  %_0x127_0x1 = phi i256 [ %evm.mload19, %bb._0x120 ], [ 2951, %bb._0x170 ], !notdec.evm !86
  br label %bb._0x17d, !notdec.evm !87

bb._0x17d:                                        ; preds = %bb._0x127, %bb._0x61
  %_0x17d_0x0 = phi i256 [ %_0x127_0x0, %bb._0x127 ], [ %_0x61_0x1, %bb._0x61 ], !notdec.evm !88
  %_0x17d_0x1 = phi i256 [ 2918, %bb._0x127 ], [ 2844, %bb._0x61 ], !notdec.evm !89
  %evm.calldatasize50 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !90
  call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 0, i256 0, i256 %evm.calldatasize50), !notdec.evm !91
  %evm.calldatasize51 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !92
  %evm.gas52 = call i256 @evm_gas(ptr %env), !notdec.evm !93
  %evm.delegatecall = call i256 @evm_delegatecall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas52, i256 %_0x17d_0x0, i256 0, i256 %evm.calldatasize51, i256 0, i256 0), !notdec.evm !94
  %evm.returndatasize53 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !95
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize53), !notdec.evm !96
  %evm.iszero54 = icmp eq i256 %evm.delegatecall, 0, !notdec.evm !97
  %evm.bool55 = zext i1 %evm.iszero54 to i256, !notdec.evm !97
  %evm.branch.cond56 = icmp ne i256 %evm.bool55, 0, !notdec.evm !98
  br i1 %evm.branch.cond56, label %bb._0x19a, label %bb._0x197, !notdec.evm !98

bb._0x19a:                                        ; preds = %bb._0x17d
  %evm.returndatasize57 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !99
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize57), !notdec.evm !100
  unreachable, !notdec.evm !100

bb._0x197:                                        ; preds = %bb._0x17d
  %evm.returndatasize58 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !101
  call void @evm_return(ptr %mem, i256 0, i256 %evm.returndatasize58), !notdec.evm !102
  ret void, !notdec.evm !102
}

define i256 @private__0x19e_0x19e(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x19earg0x0, i256 %_0x19earg0x1) #0 {
bb._0x19e:
  %evm.add = add i256 -128, %_0x19earg0x0, !notdec.evm !103
  %evm.slt = icmp slt i256 %evm.add, 32, !notdec.evm !104
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !104
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !105
  br i1 %evm.branch.cond, label %bb._0x50e, label %bb._0x1ab, !notdec.evm !105

bb._0x50e:                                        ; preds = %bb._0x19e
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !106
  unreachable, !notdec.evm !106

bb._0x1ab:                                        ; preds = %bb._0x19e
  %notdec.evm.mem.ptr.12 = inttoptr i256 128 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.12, align 1, !notdec.evm !107
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !108
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !109
  %evm.and = and i256 %evm.mload, %evm.sub, !notdec.evm !110
  %evm.sub1 = sub i256 %evm.mload, %evm.and, !notdec.evm !111
  %evm.branch.cond2 = icmp ne i256 %evm.sub1, 0, !notdec.evm !112
  br i1 %evm.branch.cond2, label %bb._0x532, label %bb._0x1be, !notdec.evm !112

bb._0x532:                                        ; preds = %bb._0x1ab
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !113
  unreachable, !notdec.evm !113

bb._0x1be:                                        ; preds = %bb._0x1ab
  ret i256 %evm.mload, !notdec.evm !114
}

define i256 @private__0x1c5_0x1c5(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x1c5arg0x0, i256 %_0x1c5arg0x1, i256 %_0x1c5arg0x2) #0 {
bb._0x1c5:
  %evm.sub = sub i256 %_0x1c5arg0x1, %_0x1c5arg0x0, !notdec.evm !115
  %evm.slt = icmp slt i256 %evm.sub, 32, !notdec.evm !116
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !116
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !117
  br i1 %evm.branch.cond, label %bb._0x556, label %bb._0x1d1, !notdec.evm !117

bb._0x556:                                        ; preds = %bb._0x1c5
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !118
  unreachable, !notdec.evm !118

bb._0x1d1:                                        ; preds = %bb._0x1c5
  %notdec.evm.mem.ptr.13 = inttoptr i256 %_0x1c5arg0x0 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.13, align 1, !notdec.evm !119
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !120
  %evm.sub1 = sub i256 %evm.shl, 1, !notdec.evm !121
  %evm.and = and i256 %evm.mload, %evm.sub1, !notdec.evm !122
  %evm.sub2 = sub i256 %evm.mload, %evm.and, !notdec.evm !123
  %evm.branch.cond3 = icmp ne i256 %evm.sub2, 0, !notdec.evm !124
  br i1 %evm.branch.cond3, label %bb._0x57a, label %bb._0x1e2, !notdec.evm !124

bb._0x57a:                                        ; preds = %bb._0x1d1
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !125
  unreachable, !notdec.evm !125

bb._0x1e2:                                        ; preds = %bb._0x1d1
  ret i256 %evm.mload, !notdec.evm !126
}

!0 = !{!"tac=0x5", !"op=MSTORE", !"evm.pc=0x5"}
!1 = !{!"tac=0x6", !"op=CALLDATASIZE", !"evm.pc=0x6"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xb", !"op=JUMPI", !"evm.pc=0xb"}
!4 = !{!"tac=0x1e", !"op=SHL", !"evm.pc=0x1e"}
!5 = !{!"tac=0x20", !"op=MSTORE", !"evm.pc=0x20"}
!6 = !{!"tac=0x4e", !"op=SHL", !"evm.pc=0x4e"}
!7 = !{!"tac=0x4f", !"op=SUB", !"evm.pc=0x4f"}
!8 = !{!"tac=0x50", !"op=AND", !"evm.pc=0x50"}
!9 = !{!"tac=0x51", !"op=GAS", !"evm.pc=0x51"}
!10 = !{!"tac=0x52", !"op=STATICCALL", !"evm.pc=0x52"}
!11 = !{!"tac=0x55", !"op=ISZERO", !"evm.pc=0x55"}
!12 = !{!"tac=0x59", !"op=JUMPI", !"evm.pc=0x59"}
!13 = !{!"tac=0x455", !"op=MLOAD", !"evm.pc=0xd1"}
!14 = !{!"tac=0x456", !"op=RETURNDATASIZE", !"evm.pc=0xd2"}
!15 = !{!"tac=0x45a", !"op=RETURNDATACOPY", !"evm.pc=0xd6"}
!16 = !{!"tac=0x45b", !"op=RETURNDATASIZE", !"evm.pc=0xd7"}
!17 = !{!"tac=0x45d", !"op=REVERT", !"evm.pc=0xd9"}
!18 = !{!"tac=0x60", !"op=JUMPI", !"evm.pc=0x60"}
!19 = !{!"tac=0x75", !"op=RETURNDATASIZE", !"evm.pc=0x75"}
!20 = !{!"tac=0x77", !"op=GT", !"evm.pc=0x77"}
!21 = !{!"tac=0x7b", !"op=JUMPI", !"evm.pc=0x7b"}
!22 = !{!"tac=0xc7", !"op=RETURNDATASIZE", !"evm.pc=0xc7"}
!23 = !{!"tac=0xcd", !"op=JUMP", !"evm.pc=0xcd"}
!24 = !{!"tac=0x7c_0x2", !"op=PHI"}
!25 = !{!"tac=0x80", !"op=ADD", !"evm.pc=0x80"}
!26 = !{!"tac=0x84", !"op=AND", !"evm.pc=0x84"}
!27 = !{!"tac=0x86", !"op=ADD", !"evm.pc=0x86"}
!28 = !{!"tac=0x92", !"op=GT", !"evm.pc=0x92"}
!29 = !{!"tac=0x95", !"op=LT", !"evm.pc=0x95"}
!30 = !{!"tac=0x96", !"op=OR", !"evm.pc=0x96"}
!31 = !{!"tac=0x9a", !"op=JUMPI", !"evm.pc=0x9a"}
!32 = !{!"tac=0x47d_0x3", !"op=PHI"}
!33 = !{!"tac=0x485", !"op=SHL", !"evm.pc=0xba"}
!34 = !{!"tac=0x487", !"op=MSTORE", !"evm.pc=0xbc"}
!35 = !{!"tac=0x48c", !"op=MSTORE", !"evm.pc=0xc1"}
!36 = !{!"tac=0x490", !"op=REVERT", !"evm.pc=0xc5"}
!37 = !{!"tac=0x9b_0x3", !"op=PHI"}
!38 = !{!"tac=0x9e", !"op=MSTORE", !"evm.pc=0x9e"}
!39 = !{!"tac=0xa7", !"op=ADD", !"evm.pc=0xa7"}
!40 = !{!"tac=0xab", !"op=CALLPRIVATE", !"evm.pc=0xab"}
!41 = !{!"tac=0xb1", !"op=JUMP", !"evm.pc=0xb1"}
!42 = !{!"tac=0x61_0x0", !"op=PHI"}
!43 = !{!"tac=0x61_0x1", !"op=PHI"}
!44 = !{!"tac=0x6a", !"op=JUMP", !"evm.pc=0x6a"}
!45 = !{!"tac=0x13", !"op=JUMP", !"evm.pc=0x13"}
!46 = !{!"tac=0xdd", !"op=MLOAD", !"evm.pc=0xdd"}
!47 = !{!"tac=0xe5", !"op=SHL", !"evm.pc=0xe5"}
!48 = !{!"tac=0xe7", !"op=MSTORE", !"evm.pc=0xe7"}
!49 = !{!"tac=0x115", !"op=SHL", !"evm.pc=0x115"}
!50 = !{!"tac=0x116", !"op=SUB", !"evm.pc=0x116"}
!51 = !{!"tac=0x117", !"op=AND", !"evm.pc=0x117"}
!52 = !{!"tac=0x118", !"op=GAS", !"evm.pc=0x118"}
!53 = !{!"tac=0x119", !"op=STATICCALL", !"evm.pc=0x119"}
!54 = !{!"tac=0x11b", !"op=ISZERO", !"evm.pc=0x11b"}
!55 = !{!"tac=0x11f", !"op=JUMPI", !"evm.pc=0x11f"}
!56 = !{!"tac=0x4b3", !"op=MLOAD", !"evm.pc=0xd1"}
!57 = !{!"tac=0x4b4", !"op=RETURNDATASIZE", !"evm.pc=0xd2"}
!58 = !{!"tac=0x4b8", !"op=RETURNDATACOPY", !"evm.pc=0xd6"}
!59 = !{!"tac=0x4b9", !"op=RETURNDATASIZE", !"evm.pc=0xd7"}
!60 = !{!"tac=0x4bb", !"op=REVERT", !"evm.pc=0xd9"}
!61 = !{!"tac=0x126", !"op=JUMPI", !"evm.pc=0x126"}
!62 = !{!"tac=0x137", !"op=RETURNDATASIZE", !"evm.pc=0x137"}
!63 = !{!"tac=0x139", !"op=GT", !"evm.pc=0x139"}
!64 = !{!"tac=0x13d", !"op=JUMPI", !"evm.pc=0x13d"}
!65 = !{!"tac=0x176", !"op=RETURNDATASIZE", !"evm.pc=0x176"}
!66 = !{!"tac=0x17c", !"op=JUMP", !"evm.pc=0x17c"}
!67 = !{!"tac=0x13e_0x1", !"op=PHI"}
!68 = !{!"tac=0x142", !"op=ADD", !"evm.pc=0x142"}
!69 = !{!"tac=0x146", !"op=AND", !"evm.pc=0x146"}
!70 = !{!"tac=0x148", !"op=ADD", !"evm.pc=0x148"}
!71 = !{!"tac=0x154", !"op=GT", !"evm.pc=0x154"}
!72 = !{!"tac=0x157", !"op=LT", !"evm.pc=0x157"}
!73 = !{!"tac=0x158", !"op=OR", !"evm.pc=0x158"}
!74 = !{!"tac=0x15c", !"op=JUMPI", !"evm.pc=0x15c"}
!75 = !{!"tac=0x4db_0x2", !"op=PHI"}
!76 = !{!"tac=0x4e3", !"op=SHL", !"evm.pc=0xba"}
!77 = !{!"tac=0x4e5", !"op=MSTORE", !"evm.pc=0xbc"}
!78 = !{!"tac=0x4ea", !"op=MSTORE", !"evm.pc=0xc1"}
!79 = !{!"tac=0x4ee", !"op=REVERT", !"evm.pc=0xc5"}
!80 = !{!"tac=0x15d_0x2", !"op=PHI"}
!81 = !{!"tac=0x160", !"op=MSTORE", !"evm.pc=0x160"}
!82 = !{!"tac=0x16a", !"op=ADD", !"evm.pc=0x16a"}
!83 = !{!"tac=0x16f", !"op=CALLPRIVATE", !"evm.pc=0x16f"}
!84 = !{!"tac=0x174", !"op=JUMP", !"evm.pc=0x174"}
!85 = !{!"tac=0x127_0x0", !"op=PHI"}
!86 = !{!"tac=0x127_0x1", !"op=PHI"}
!87 = !{!"tac=0x130", !"op=JUMP", !"evm.pc=0x130"}
!88 = !{!"tac=0x17d_0x0", !"op=PHI"}
!89 = !{!"tac=0x17d_0x1", !"op=PHI"}
!90 = !{!"tac=0x185", !"op=CALLDATASIZE", !"evm.pc=0x185"}
!91 = !{!"tac=0x188", !"op=CALLDATACOPY", !"evm.pc=0x188"}
!92 = !{!"tac=0x18a", !"op=CALLDATASIZE", !"evm.pc=0x18a"}
!93 = !{!"tac=0x18c", !"op=GAS", !"evm.pc=0x18c"}
!94 = !{!"tac=0x18d", !"op=DELEGATECALL", !"evm.pc=0x18d"}
!95 = !{!"tac=0x18e", !"op=RETURNDATASIZE", !"evm.pc=0x18e"}
!96 = !{!"tac=0x191", !"op=RETURNDATACOPY", !"evm.pc=0x191"}
!97 = !{!"tac=0x192", !"op=ISZERO", !"evm.pc=0x192"}
!98 = !{!"tac=0x196", !"op=JUMPI", !"evm.pc=0x196"}
!99 = !{!"tac=0x19b", !"op=RETURNDATASIZE", !"evm.pc=0x19b"}
!100 = !{!"tac=0x19d", !"op=REVERT", !"evm.pc=0x19d"}
!101 = !{!"tac=0x197", !"op=RETURNDATASIZE", !"evm.pc=0x197"}
!102 = !{!"tac=0x199", !"op=RETURN", !"evm.pc=0x199"}
!103 = !{!"tac=0x1a5", !"op=ADD", !"evm.pc=0x1a5"}
!104 = !{!"tac=0x1a6", !"op=SLT", !"evm.pc=0x1a6"}
!105 = !{!"tac=0x1aa", !"op=JUMPI", !"evm.pc=0x1aa"}
!106 = !{!"tac=0x512", !"op=REVERT", !"evm.pc=0x1c4"}
!107 = !{!"tac=0x1ad", !"op=MLOAD", !"evm.pc=0x1ad"}
!108 = !{!"tac=0x1b4", !"op=SHL", !"evm.pc=0x1b4"}
!109 = !{!"tac=0x1b5", !"op=SUB", !"evm.pc=0x1b5"}
!110 = !{!"tac=0x1b7", !"op=AND", !"evm.pc=0x1b7"}
!111 = !{!"tac=0x1b9", !"op=SUB", !"evm.pc=0x1b9"}
!112 = !{!"tac=0x1bd", !"op=JUMPI", !"evm.pc=0x1bd"}
!113 = !{!"tac=0x536", !"op=REVERT", !"evm.pc=0x1c4"}
!114 = !{!"tac=0x1bf", !"op=RETURNPRIVATE", !"evm.pc=0x1bf"}
!115 = !{!"tac=0x1cb", !"op=SUB", !"evm.pc=0x1cb"}
!116 = !{!"tac=0x1cc", !"op=SLT", !"evm.pc=0x1cc"}
!117 = !{!"tac=0x1d0", !"op=JUMPI", !"evm.pc=0x1d0"}
!118 = !{!"tac=0x55a", !"op=REVERT", !"evm.pc=0x1c4"}
!119 = !{!"tac=0x1d1", !"op=MLOAD", !"evm.pc=0x1d1"}
!120 = !{!"tac=0x1d8", !"op=SHL", !"evm.pc=0x1d8"}
!121 = !{!"tac=0x1d9", !"op=SUB", !"evm.pc=0x1d9"}
!122 = !{!"tac=0x1db", !"op=AND", !"evm.pc=0x1db"}
!123 = !{!"tac=0x1dd", !"op=SUB", !"evm.pc=0x1dd"}
!124 = !{!"tac=0x1e1", !"op=JUMPI", !"evm.pc=0x1e1"}
!125 = !{!"tac=0x57e", !"op=REVERT", !"evm.pc=0x1c4"}
!126 = !{!"tac=0x1e3", !"op=RETURNPRIVATE", !"evm.pc=0x1e3"}

attributes #0 = { null_pointer_is_valid }
