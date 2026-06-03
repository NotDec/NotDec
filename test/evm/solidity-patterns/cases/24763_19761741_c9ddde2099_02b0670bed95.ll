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
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !2
  %evm.bool = zext i1 %evm.lt to i256, !notdec.evm !2
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !3
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !3
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !4
  br i1 %evm.branch.cond, label %bb._0x15, label %bb._0x11, !notdec.evm !4

bb._0x15:                                         ; preds = %bb._0x0
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !5
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !6
  %evm.eq = icmp eq i256 117300739, %evm.shr, !notdec.evm !7
  %evm.bool2 = zext i1 %evm.eq to i256, !notdec.evm !7
  %evm.branch.cond3 = icmp ne i256 %evm.bool2, 0, !notdec.evm !8
  br i1 %evm.branch.cond3, label %bb._0x1dd30, label %bb._0x2a, !notdec.evm !8

bb._0x1dd30:                                      ; preds = %bb._0x15
  call void @public_name___0x570(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 128, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !9
  ret void

bb._0x2a:                                         ; preds = %bb._0x15
  %evm.eq4 = icmp eq i256 157198259, %evm.shr, !notdec.evm !10
  %evm.bool5 = zext i1 %evm.eq4 to i256, !notdec.evm !10
  %evm.branch.cond6 = icmp ne i256 %evm.bool5, 0, !notdec.evm !11
  br i1 %evm.branch.cond6, label %bb._0x1d330, label %bb._0x36, !notdec.evm !11

bb._0x1d330:                                      ; preds = %bb._0x2a
  call void @public_approve_address_uint256__0x4c7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !12
  ret void

bb._0x36:                                         ; preds = %bb._0x2a
  %evm.eq7 = icmp eq i256 404098525, %evm.shr, !notdec.evm !13
  %evm.bool8 = zext i1 %evm.eq7 to i256, !notdec.evm !13
  %evm.branch.cond9 = icmp ne i256 %evm.bool8, 0, !notdec.evm !14
  br i1 %evm.branch.cond9, label %bb._0x1c930, label %bb._0x41, !notdec.evm !14

bb._0x1c930:                                      ; preds = %bb._0x36
  call void @public_totalSupply___0x4a8(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !15
  ret void

bb._0x41:                                         ; preds = %bb._0x36
  %evm.eq10 = icmp eq i256 599290589, %evm.shr, !notdec.evm !16
  %evm.bool11 = zext i1 %evm.eq10 to i256, !notdec.evm !16
  %evm.branch.cond12 = icmp ne i256 %evm.bool11, 0, !notdec.evm !17
  br i1 %evm.branch.cond12, label %bb._0x1bf30, label %bb._0x4c, !notdec.evm !17

bb._0x1bf30:                                      ; preds = %bb._0x41
  call void @public_transferFrom_address_address_uint256__0x3b5(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !18
  ret void

bb._0x4c:                                         ; preds = %bb._0x41
  %evm.eq13 = icmp eq i256 826074471, %evm.shr, !notdec.evm !19
  %evm.bool14 = zext i1 %evm.eq13 to i256, !notdec.evm !19
  %evm.branch.cond15 = icmp ne i256 %evm.bool14, 0, !notdec.evm !20
  br i1 %evm.branch.cond15, label %bb._0x1b530, label %bb._0x57, !notdec.evm !20

bb._0x1b530:                                      ; preds = %bb._0x4c
  call void @public_decimals___0x399(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !21
  ret void

bb._0x57:                                         ; preds = %bb._0x4c
  %evm.eq16 = icmp eq i256 1889567281, %evm.shr, !notdec.evm !22
  %evm.bool17 = zext i1 %evm.eq16 to i256, !notdec.evm !22
  %evm.branch.cond18 = icmp ne i256 %evm.bool17, 0, !notdec.evm !23
  br i1 %evm.branch.cond18, label %bb._0x1ab30, label %bb._0x62, !notdec.evm !23

bb._0x1ab30:                                      ; preds = %bb._0x57
  call void @public_balanceOf_address__0x362(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !24
  ret void

bb._0x62:                                         ; preds = %bb._0x57
  %evm.eq19 = icmp eq i256 1901074598, %evm.shr, !notdec.evm !25
  %evm.bool20 = zext i1 %evm.eq19 to i256, !notdec.evm !25
  %evm.branch.cond21 = icmp ne i256 %evm.bool20, 0, !notdec.evm !26
  br i1 %evm.branch.cond21, label %bb._0x1a130, label %bb._0x6d, !notdec.evm !26

bb._0x1a130:                                      ; preds = %bb._0x62
  call void @public_renounceOwnership___0x302(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !27
  ret void

bb._0x6d:                                         ; preds = %bb._0x62
  %evm.eq22 = icmp eq i256 2376452955, %evm.shr, !notdec.evm !28
  %evm.bool23 = zext i1 %evm.eq22 to i256, !notdec.evm !28
  %evm.branch.cond24 = icmp ne i256 %evm.bool23, 0, !notdec.evm !29
  br i1 %evm.branch.cond24, label %bb._0x19730, label %bb._0x78, !notdec.evm !29

bb._0x19730:                                      ; preds = %bb._0x6d
  call void @public_owner___0x2d9(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !30
  ret void

bb._0x78:                                         ; preds = %bb._0x6d
  %evm.eq25 = icmp eq i256 2514000705, %evm.shr, !notdec.evm !31
  %evm.bool26 = zext i1 %evm.eq25 to i256, !notdec.evm !31
  %evm.branch.cond27 = icmp ne i256 %evm.bool26, 0, !notdec.evm !32
  br i1 %evm.branch.cond27, label %bb._0x18d30, label %bb._0x83, !notdec.evm !32

bb._0x18d30:                                      ; preds = %bb._0x78
  call void @public_symbol___0x1b7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !33
  ret void

bb._0x83:                                         ; preds = %bb._0x78
  %evm.eq28 = icmp eq i256 2835717307, %evm.shr, !notdec.evm !34
  %evm.bool29 = zext i1 %evm.eq28 to i256, !notdec.evm !34
  %evm.branch.cond30 = icmp ne i256 %evm.bool29, 0, !notdec.evm !35
  br i1 %evm.branch.cond30, label %bb._0x18330, label %bb._0x8e, !notdec.evm !35

bb._0x18330:                                      ; preds = %bb._0x83
  call void @public_transfer_address_uint256__0x186(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !36
  ret void

bb._0x8e:                                         ; preds = %bb._0x83
  %evm.eq31 = icmp eq i256 3714247998, %evm.shr, !notdec.evm !37
  %evm.bool32 = zext i1 %evm.eq31 to i256, !notdec.evm !37
  %evm.branch.cond33 = icmp ne i256 %evm.bool32, 0, !notdec.evm !38
  br i1 %evm.branch.cond33, label %bb._0x17930, label %bb._0x99, !notdec.evm !38

bb._0x17930:                                      ; preds = %bb._0x8e
  call void @public_allowance_address_address__0x139(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.shr, i256 4, i256 64, i256 0), !notdec.evm !39
  ret void

bb._0x99:                                         ; preds = %bb._0x8e
  %evm.eq34 = icmp eq i256 4076725131, %evm.shr, !notdec.evm !40
  %evm.bool35 = zext i1 %evm.eq34 to i256, !notdec.evm !40
  %evm.branch.cond36 = icmp ne i256 %evm.bool35, 0, !notdec.evm !41
  br i1 %evm.branch.cond36, label %bb._0x16f30, label %bb._0xa3, !notdec.evm !41

bb._0x16f30:                                      ; preds = %bb._0x99
  call void @public_transferOwnership_address__0xa7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4, i256 64, i256 0), !notdec.evm !42
  ret void

bb._0xa3:                                         ; preds = %bb._0x99
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !43
  unreachable, !notdec.evm !43

bb._0x11:                                         ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !44
  unreachable, !notdec.evm !44
}

define void @public_allowance_address_address__0x139(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x139arg0x0, i256 %_0x139arg0x1, i256 %_0x139arg0x2, i256 %_0x139arg0x3) #0 {
bb._0x139:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !45
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !46
  br i1 %evm.branch.cond, label %bb._0x10a6, label %bb._0x141, !notdec.evm !46

bb._0x10a6:                                       ; preds = %bb._0x139
  call void @evm_revert(ptr %mem, i256 %_0x139arg0x3, i256 %_0x139arg0x3), !notdec.evm !47
  unreachable, !notdec.evm !47

bb._0x141:                                        ; preds = %bb._0x139
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !48
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !49
  %evm.slt = icmp slt i256 %evm.add, %_0x139arg0x2, !notdec.evm !50
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !50
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !51
  br i1 %evm.branch.cond1, label %bb._0x10c9, label %bb._0x14c, !notdec.evm !51

bb._0x10c9:                                       ; preds = %bb._0x141
  call void @evm_revert(ptr %mem, i256 %_0x139arg0x3, i256 %_0x139arg0x3), !notdec.evm !52
  unreachable, !notdec.evm !52

bb._0x14c:                                        ; preds = %bb._0x141
  %private.call = call i256 @private__0x6b0_0x6b0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 343), !notdec.evm !53
  br label %bb._0x157

bb._0x157:                                        ; preds = %bb._0x14c
  %private.call2 = call i256 @private__0x6cb_0x6cb(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 351), !notdec.evm !54
  br label %bb._0x15f

bb._0x15f:                                        ; preds = %bb._0x157
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !55
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !56
  %evm.and = and i256 %evm.sub, %private.call, !notdec.evm !57
  %notdec.evm.mem.ptr.1 = inttoptr i256 %_0x139arg0x3 to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.1, align 1, !notdec.evm !58
  %notdec.evm.mem.ptr.2 = inttoptr i256 32 to ptr
  store i256 1, ptr %notdec.evm.mem.ptr.2, align 1, !notdec.evm !59
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 %_0x139arg0x3, i256 %_0x139arg0x2), !notdec.evm !60
  %evm.and3 = and i256 %evm.sub, %private.call2, !notdec.evm !61
  %notdec.evm.mem.ptr.3 = inttoptr i256 %_0x139arg0x3 to ptr
  store i256 %evm.and3, ptr %notdec.evm.mem.ptr.3, align 1, !notdec.evm !62
  %notdec.evm.mem.ptr.4 = inttoptr i256 32 to ptr
  store i256 %evm.sha3, ptr %notdec.evm.mem.ptr.4, align 1, !notdec.evm !63
  %evm.sha34 = call i256 @evm_sha3(ptr %mem, i256 %_0x139arg0x3, i256 %_0x139arg0x2), !notdec.evm !64
  %evm.sload = call i256 @evm_sload(i256 %evm.sha34), !notdec.evm !65
  %notdec.evm.mem.ptr.5 = inttoptr i256 %_0x139arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.5, align 1, !notdec.evm !66
  %notdec.evm.mem.ptr.6 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.sload, ptr %notdec.evm.mem.ptr.6, align 1, !notdec.evm !67
  call void @evm_return(ptr %mem, i256 %evm.mload, i256 32), !notdec.evm !68
  ret void, !notdec.evm !68
}

define void @public_transfer_address_uint256__0x186(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x186arg0x0, i256 %_0x186arg0x1, i256 %_0x186arg0x2, i256 %_0x186arg0x3) #0 {
bb._0x186:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !69
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !70
  br i1 %evm.branch.cond, label %bb._0x10ec, label %bb._0x18e, !notdec.evm !70

bb._0x10ec:                                       ; preds = %bb._0x186
  call void @evm_revert(ptr %mem, i256 %_0x186arg0x3, i256 %_0x186arg0x3), !notdec.evm !71
  unreachable, !notdec.evm !71

bb._0x18e:                                        ; preds = %bb._0x186
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !72
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !73
  %evm.slt = icmp slt i256 %evm.add, %_0x186arg0x2, !notdec.evm !74
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !74
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !75
  br i1 %evm.branch.cond1, label %bb._0x110f, label %bb._0x199, !notdec.evm !75

bb._0x110f:                                       ; preds = %bb._0x18e
  call void @evm_revert(ptr %mem, i256 %_0x186arg0x3, i256 %_0x186arg0x3), !notdec.evm !76
  unreachable, !notdec.evm !76

bb._0x199:                                        ; preds = %bb._0x18e
  %private.call = call i256 @private__0x6b0_0x6b0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 422), !notdec.evm !77
  br label %bb._0x1a6

bb._0x1a6:                                        ; preds = %bb._0x199
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 36), !notdec.evm !78
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !79
  call void @private__0x6e1_0x6e1(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.caller, i256 %private.call, i256 %evm.calldataload, i256 31472), !notdec.evm !80
  br label %bb._0x7af0

bb._0x7af0:                                       ; preds = %bb._0x1a6
  %notdec.evm.mem.ptr.7 = inttoptr i256 %_0x186arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.7, align 1, !notdec.evm !81
  %notdec.evm.mem.ptr.8 = inttoptr i256 %evm.mload to ptr
  store i256 1, ptr %notdec.evm.mem.ptr.8, align 1, !notdec.evm !82
  call void @evm_return(ptr %mem, i256 %evm.mload, i256 32), !notdec.evm !83
  ret void, !notdec.evm !83
}

define void @public_symbol___0x1b7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x1b7arg0x0, i256 %_0x1b7arg0x1, i256 %_0x1b7arg0x2, i256 %_0x1b7arg0x3) #0 {
bb._0x1b7:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !84
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !85
  br i1 %evm.branch.cond, label %bb._0x1132, label %bb._0x1c0, !notdec.evm !85

bb._0x1132:                                       ; preds = %bb._0x1b7
  call void @evm_revert(ptr %mem, i256 %_0x1b7arg0x3, i256 %_0x1b7arg0x3), !notdec.evm !86
  unreachable, !notdec.evm !86

bb._0x1c0:                                        ; preds = %bb._0x1b7
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !87
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !88
  %evm.slt = icmp slt i256 %evm.add, %_0x1b7arg0x3, !notdec.evm !89
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !89
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !90
  br i1 %evm.branch.cond1, label %bb._0x1155, label %bb._0x1cb, !notdec.evm !90

bb._0x1155:                                       ; preds = %bb._0x1c0
  call void @evm_revert(ptr %mem, i256 %_0x1b7arg0x3, i256 %_0x1b7arg0x3), !notdec.evm !91
  unreachable, !notdec.evm !91

bb._0x1cb:                                        ; preds = %bb._0x1c0
  %notdec.evm.mem.ptr.9 = inttoptr i256 %_0x1b7arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.9, align 1, !notdec.evm !92
  %evm.sload = call i256 @evm_sload(i256 %_0x1b7arg0x1), !notdec.evm !93
  %evm.shr = call i256 @evm_shr(i256 1, i256 %evm.sload), !notdec.evm !94
  %evm.and = and i256 %evm.sload, 1, !notdec.evm !95
  %evm.iszero = icmp eq i256 %evm.and, 0, !notdec.evm !96
  %evm.bool2 = zext i1 %evm.iszero to i256, !notdec.evm !96
  %evm.branch.cond3 = icmp ne i256 %evm.bool2, 0, !notdec.evm !97
  br i1 %evm.branch.cond3, label %bb._0x2cf, label %bb._0x1e3, !notdec.evm !97

bb._0x2cf:                                        ; preds = %bb._0x1cb
  %evm.and4 = and i256 127, %evm.shr, !notdec.evm !98
  br label %bb._0x1e3, !notdec.evm !99

bb._0x1e3:                                        ; preds = %bb._0x2cf, %bb._0x1cb
  %_0x1e3_0x2 = phi i256 [ %evm.shr, %bb._0x1cb ], [ %evm.and4, %bb._0x2cf ], !notdec.evm !100
  %evm.lt = icmp ult i256 %_0x1e3_0x2, 32, !notdec.evm !101
  %evm.bool5 = zext i1 %evm.lt to i256, !notdec.evm !101
  %evm.eq = icmp eq i256 %evm.and, %evm.bool5, !notdec.evm !102
  %evm.bool6 = zext i1 %evm.eq to i256, !notdec.evm !102
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !103
  br i1 %evm.branch.cond7, label %bb._0x1178, label %bb._0x1f0, !notdec.evm !103

bb._0x1178:                                       ; preds = %bb._0x1e3
  %_0x1178_0x3 = phi i256 [ %_0x1e3_0x2, %bb._0x1e3 ], !notdec.evm !104
  %evm.shl = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !105
  %notdec.evm.mem.ptr.10 = inttoptr i256 %_0x1b7arg0x3 to ptr
  store i256 %evm.shl, ptr %notdec.evm.mem.ptr.10, align 1, !notdec.evm !106
  %notdec.evm.mem.ptr.11 = inttoptr i256 %_0x1b7arg0x1 to ptr
  store i256 34, ptr %notdec.evm.mem.ptr.11, align 1, !notdec.evm !107
  call void @evm_revert(ptr %mem, i256 %_0x1b7arg0x3, i256 36), !notdec.evm !108
  unreachable, !notdec.evm !108

bb._0x1f0:                                        ; preds = %bb._0x1e3
  %_0x1f0_0x3 = phi i256 [ %_0x1e3_0x2, %bb._0x1e3 ], !notdec.evm !109
  %notdec.evm.mem.ptr.12 = inttoptr i256 %evm.mload to ptr
  store i256 %_0x1f0_0x3, ptr %notdec.evm.mem.ptr.12, align 1, !notdec.evm !110
  %evm.iszero8 = icmp eq i256 %evm.and, 0, !notdec.evm !111
  %evm.bool9 = zext i1 %evm.iszero8 to i256, !notdec.evm !111
  %evm.branch.cond10 = icmp ne i256 %evm.bool9, 0, !notdec.evm !112
  br i1 %evm.branch.cond10, label %bb._0x2a0, label %bb._0x1fa, !notdec.evm !112

bb._0x2a0:                                        ; preds = %bb._0x1f0
  %_0x2a0_0x3 = phi i256 [ %_0x1f0_0x3, %bb._0x1f0 ], !notdec.evm !113
  %evm.and11 = and i256 -256, %evm.sload, !notdec.evm !114
  %evm.add12 = add i256 32, %evm.mload, !notdec.evm !115
  %notdec.evm.mem.ptr.13 = inttoptr i256 %evm.add12 to ptr
  store i256 %evm.and11, ptr %notdec.evm.mem.ptr.13, align 1, !notdec.evm !116
  %evm.iszero13 = icmp eq i256 %_0x2a0_0x3, 0, !notdec.evm !117
  %evm.bool14 = zext i1 %evm.iszero13 to i256, !notdec.evm !117
  %evm.iszero15 = icmp eq i256 %evm.bool14, 0, !notdec.evm !118
  %evm.bool16 = zext i1 %evm.iszero15 to i256, !notdec.evm !118
  %evm.shl17 = call i256 @evm_shl(i256 5, i256 %evm.bool16), !notdec.evm !119
  %evm.add18 = add i256 %evm.mload, %evm.shl17, !notdec.evm !120
  %evm.add19 = add i256 %evm.add18, 32, !notdec.evm !121
  %evm.codesize = call i256 @evm_codesize(ptr %env), !notdec.evm !122
  br label %bb._0x2020x1b7, !notdec.evm !123

bb._0x1fa:                                        ; preds = %bb._0x1f0
  %_0x1fa_0x3 = phi i256 [ %_0x1f0_0x3, %bb._0x1f0 ], !notdec.evm !124
  %evm.eq20 = icmp eq i256 1, %evm.and, !notdec.evm !125
  %evm.bool21 = zext i1 %evm.eq20 to i256, !notdec.evm !125
  %evm.branch.cond22 = icmp ne i256 %evm.bool21, 0, !notdec.evm !126
  br i1 %evm.branch.cond22, label %bb._0x24b, label %bb._0x2020x1b7, !notdec.evm !126

bb._0x24b:                                        ; preds = %bb._0x1fa
  %_0x24b_0x1 = phi i256 [ %_0x1fa_0x3, %bb._0x1fa ], !notdec.evm !127
  %notdec.evm.mem.ptr.14 = inttoptr i256 %_0x1b7arg0x3 to ptr
  store i256 %_0x1b7arg0x1, ptr %notdec.evm.mem.ptr.14, align 1, !notdec.evm !128
  br label %bb._0x276, !notdec.evm !129

bb._0x276:                                        ; preds = %bb._0x28c, %bb._0x24b
  %_0x276_0x0 = phi i256 [ -53278079350709166316280423202849322519190186591071851114874353210178472783461, %bb._0x24b ], [ %evm.add30, %bb._0x28c ], !notdec.evm !130
  %_0x276_0x3 = phi i256 [ %_0x24b_0x1, %bb._0x24b ], [ %_0x28c_0x3, %bb._0x28c ], !notdec.evm !131
  %_0x276_0x4 = phi i256 [ %_0x1b7arg0x3, %bb._0x24b ], [ %evm.add29, %bb._0x28c ], !notdec.evm !132
  %evm.lt23 = icmp ult i256 %_0x276_0x4, %_0x276_0x3, !notdec.evm !133
  %evm.bool24 = zext i1 %evm.lt23 to i256, !notdec.evm !133
  %evm.branch.cond25 = icmp ne i256 %evm.bool24, 0, !notdec.evm !134
  br i1 %evm.branch.cond25, label %bb._0x28c, label %bb._0x27e, !notdec.evm !134

bb._0x28c:                                        ; preds = %bb._0x276
  %_0x28c_0x0 = phi i256 [ %_0x276_0x0, %bb._0x276 ], !notdec.evm !135
  %_0x28c_0x3 = phi i256 [ %_0x276_0x3, %bb._0x276 ], !notdec.evm !136
  %_0x28c_0x4 = phi i256 [ %_0x276_0x4, %bb._0x276 ], !notdec.evm !137
  %evm.sload26 = call i256 @evm_sload(i256 %_0x28c_0x0), !notdec.evm !138
  %evm.add27 = add i256 %_0x28c_0x4, %evm.mload, !notdec.evm !139
  %evm.add28 = add i256 32, %evm.add27, !notdec.evm !140
  %notdec.evm.mem.ptr.15 = inttoptr i256 %evm.add28 to ptr
  store i256 %evm.sload26, ptr %notdec.evm.mem.ptr.15, align 1, !notdec.evm !141
  %evm.add29 = add i256 %_0x28c_0x4, 32, !notdec.evm !142
  %evm.add30 = add i256 1, %_0x28c_0x0, !notdec.evm !143
  br label %bb._0x276, !notdec.evm !144

bb._0x27e:                                        ; preds = %bb._0x276
  %_0x27e_0x0 = phi i256 [ %_0x276_0x0, %bb._0x276 ], !notdec.evm !145
  %_0x27e_0x3 = phi i256 [ %_0x276_0x3, %bb._0x276 ], !notdec.evm !146
  %_0x27e_0x4 = phi i256 [ %_0x276_0x4, %bb._0x276 ], !notdec.evm !147
  %evm.add31 = add i256 %evm.mload, %_0x27e_0x4, !notdec.evm !148
  %evm.add32 = add i256 %evm.add31, 32, !notdec.evm !149
  %evm.codesize33 = call i256 @evm_codesize(ptr %env), !notdec.evm !150
  br label %bb._0x2020x1b7, !notdec.evm !151

bb._0x2020x1b7:                                   ; preds = %bb._0x27e, %bb._0x1fa, %bb._0x2a0
  %_0x2020x1b7_0x0 = phi i256 [ 1, %bb._0x1fa ], [ %evm.codesize33, %bb._0x27e ], [ %evm.codesize, %bb._0x2a0 ], !notdec.evm !152
  %_0x2020x1b7_0x1 = phi i256 [ %_0x1fa_0x3, %bb._0x1fa ], [ %evm.codesize33, %bb._0x27e ], [ %evm.codesize, %bb._0x2a0 ], !notdec.evm !153
  %_0x2020x1b7_0x2 = phi i256 [ 32, %bb._0x1fa ], [ %evm.codesize33, %bb._0x27e ], [ %evm.codesize, %bb._0x2a0 ], !notdec.evm !154
  %_0x2020x1b7_0x3 = phi i256 [ %_0x1b7arg0x3, %bb._0x1fa ], [ %evm.add32, %bb._0x27e ], [ %evm.add19, %bb._0x2a0 ], !notdec.evm !155
  %evm.sub = sub i256 %_0x2020x1b7_0x3, %evm.mload, !notdec.evm !156
  %evm.add34 = add i256 31, %evm.sub, !notdec.evm !157
  %evm.and35 = and i256 -32, %evm.add34, !notdec.evm !158
  %evm.add36 = add i256 %evm.mload, %evm.and35, !notdec.evm !159
  %evm.gt = icmp ugt i256 %evm.add36, 18446744073709551615, !notdec.evm !160
  %evm.bool37 = zext i1 %evm.gt to i256, !notdec.evm !160
  %evm.lt38 = icmp ult i256 %evm.add36, %evm.mload, !notdec.evm !161
  %evm.bool39 = zext i1 %evm.lt38 to i256, !notdec.evm !161
  %evm.or = or i256 %evm.bool39, %evm.bool37, !notdec.evm !162
  %evm.branch.cond40 = icmp ne i256 %evm.or, 0, !notdec.evm !163
  br i1 %evm.branch.cond40, label %bb._0x11aa0x1b7, label %bb._0x2260x1b7, !notdec.evm !163

bb._0x11aa0x1b7:                                  ; preds = %bb._0x2020x1b7
  %evm.shl41 = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !164
  %notdec.evm.mem.ptr.16 = inttoptr i256 %_0x1b7arg0x3 to ptr
  store i256 %evm.shl41, ptr %notdec.evm.mem.ptr.16, align 1, !notdec.evm !165
  %notdec.evm.mem.ptr.17 = inttoptr i256 %_0x1b7arg0x1 to ptr
  store i256 65, ptr %notdec.evm.mem.ptr.17, align 1, !notdec.evm !166
  call void @evm_revert(ptr %mem, i256 %_0x1b7arg0x3, i256 36), !notdec.evm !167
  unreachable, !notdec.evm !167

bb._0x2260x1b7:                                   ; preds = %bb._0x2020x1b7
  %notdec.evm.mem.ptr.18 = inttoptr i256 %_0x1b7arg0x2 to ptr
  store i256 %evm.add36, ptr %notdec.evm.mem.ptr.18, align 1, !notdec.evm !168
  %private.call = call i256 @private__0x667_0x667(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.add36, i256 %evm.mload, i256 31510), !notdec.evm !169
  br label %bb._0x7b160x1b7

bb._0x7b160x1b7:                                  ; preds = %bb._0x2260x1b7
  %evm.sub42 = sub i256 %private.call, %evm.add36, !notdec.evm !170
  call void @evm_return(ptr %mem, i256 %evm.add36, i256 %evm.sub42), !notdec.evm !171
  ret void, !notdec.evm !171
}

define void @public_owner___0x2d9(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x2d9arg0x0, i256 %_0x2d9arg0x1, i256 %_0x2d9arg0x2, i256 %_0x2d9arg0x3) #0 {
bb._0x2d9:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !172
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !173
  br i1 %evm.branch.cond, label %bb._0x11dc, label %bb._0x2e1, !notdec.evm !173

bb._0x11dc:                                       ; preds = %bb._0x2d9
  call void @evm_revert(ptr %mem, i256 %_0x2d9arg0x3, i256 %_0x2d9arg0x3), !notdec.evm !174
  unreachable, !notdec.evm !174

bb._0x2e1:                                        ; preds = %bb._0x2d9
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !175
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !176
  %evm.slt = icmp slt i256 %evm.add, %_0x2d9arg0x3, !notdec.evm !177
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !177
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !178
  br i1 %evm.branch.cond1, label %bb._0x11ff, label %bb._0x2ec, !notdec.evm !178

bb._0x11ff:                                       ; preds = %bb._0x2e1
  call void @evm_revert(ptr %mem, i256 %_0x2d9arg0x3, i256 %_0x2d9arg0x3), !notdec.evm !179
  unreachable, !notdec.evm !179

bb._0x2ec:                                        ; preds = %bb._0x2e1
  %evm.sload = call i256 @evm_sload(i256 5), !notdec.evm !180
  %notdec.evm.mem.ptr.19 = inttoptr i256 %_0x2d9arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.19, align 1, !notdec.evm !181
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !182
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !183
  %evm.and = and i256 %evm.sload, %evm.sub, !notdec.evm !184
  %notdec.evm.mem.ptr.20 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.20, align 1, !notdec.evm !185
  call void @evm_return(ptr %mem, i256 %evm.mload, i256 32), !notdec.evm !186
  ret void, !notdec.evm !186
}

define void @public_renounceOwnership___0x302(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x302arg0x0, i256 %_0x302arg0x1, i256 %_0x302arg0x2, i256 %_0x302arg0x3) #0 {
bb._0x302:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !187
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !188
  br i1 %evm.branch.cond, label %bb._0x1222, label %bb._0x309, !notdec.evm !188

bb._0x1222:                                       ; preds = %bb._0x302
  call void @evm_revert(ptr %mem, i256 %_0x302arg0x3, i256 %_0x302arg0x3), !notdec.evm !189
  unreachable, !notdec.evm !189

bb._0x309:                                        ; preds = %bb._0x302
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !190
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !191
  %evm.slt = icmp slt i256 %evm.add, %_0x302arg0x3, !notdec.evm !192
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !192
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !193
  br i1 %evm.branch.cond1, label %bb._0x1244, label %bb._0x314, !notdec.evm !193

bb._0x1244:                                       ; preds = %bb._0x309
  call void @evm_revert(ptr %mem, i256 %_0x302arg0x3, i256 %_0x302arg0x3), !notdec.evm !194
  unreachable, !notdec.evm !194

bb._0x314:                                        ; preds = %bb._0x309
  call void @private__0x7bf_0x7bf(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 795), !notdec.evm !195
  br label %bb._0x31b

bb._0x31b:                                        ; preds = %bb._0x314
  %evm.sload = call i256 @evm_sload(i256 5), !notdec.evm !196
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !197
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !198
  %evm.not = xor i256 %evm.sub, -1, !notdec.evm !199
  %evm.and = and i256 %evm.sload, %evm.not, !notdec.evm !200
  call void @evm_sstore(i256 5, i256 %evm.and), !notdec.evm !201
  %evm.shl2 = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !202
  %evm.sub3 = sub i256 %evm.shl2, 1, !notdec.evm !203
  %evm.and4 = and i256 %evm.sub3, %evm.sload, !notdec.evm !204
  call void @evm_log3(ptr %mem, i256 %_0x302arg0x3, i256 %_0x302arg0x3, i256 -52524777015005588113349992707137368332388074750292320778649315033004770043936, i256 %evm.and4, i256 %_0x302arg0x3), !notdec.evm !205
  call void @evm_return(ptr %mem, i256 %_0x302arg0x3, i256 %_0x302arg0x3), !notdec.evm !206
  ret void, !notdec.evm !206
}

define void @public_balanceOf_address__0x362(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x362arg0x0, i256 %_0x362arg0x1, i256 %_0x362arg0x2, i256 %_0x362arg0x3) #0 {
bb._0x362:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !207
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !208
  br i1 %evm.branch.cond, label %bb._0x1266, label %bb._0x36a, !notdec.evm !208

bb._0x1266:                                       ; preds = %bb._0x362
  call void @evm_revert(ptr %mem, i256 %_0x362arg0x3, i256 %_0x362arg0x3), !notdec.evm !209
  unreachable, !notdec.evm !209

bb._0x36a:                                        ; preds = %bb._0x362
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !210
  %evm.add = add i256 -4, %evm.calldatasize, !notdec.evm !211
  %evm.slt = icmp slt i256 %evm.add, 32, !notdec.evm !212
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !212
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !213
  br i1 %evm.branch.cond1, label %bb._0x1289, label %bb._0x376, !notdec.evm !213

bb._0x1289:                                       ; preds = %bb._0x36a
  call void @evm_revert(ptr %mem, i256 %_0x362arg0x3, i256 %_0x362arg0x3), !notdec.evm !214
  unreachable, !notdec.evm !214

bb._0x376:                                        ; preds = %bb._0x36a
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !215
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !216
  %private.call = call i256 @private__0x6b0_0x6b0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 906), !notdec.evm !217
  br label %bb._0x38a

bb._0x38a:                                        ; preds = %bb._0x376
  %evm.and = and i256 %private.call, %evm.sub, !notdec.evm !218
  %notdec.evm.mem.ptr.21 = inttoptr i256 %_0x362arg0x3 to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.21, align 1, !notdec.evm !219
  %notdec.evm.mem.ptr.22 = inttoptr i256 32 to ptr
  store i256 %_0x362arg0x3, ptr %notdec.evm.mem.ptr.22, align 1, !notdec.evm !220
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 %_0x362arg0x3, i256 %_0x362arg0x2), !notdec.evm !221
  %evm.sload = call i256 @evm_sload(i256 %evm.sha3), !notdec.evm !222
  %notdec.evm.mem.ptr.23 = inttoptr i256 %_0x362arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.23, align 1, !notdec.evm !223
  %notdec.evm.mem.ptr.24 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.sload, ptr %notdec.evm.mem.ptr.24, align 1, !notdec.evm !224
  call void @evm_return(ptr %mem, i256 %evm.mload, i256 32), !notdec.evm !225
  ret void, !notdec.evm !225
}

define void @public_decimals___0x399(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x399arg0x0, i256 %_0x399arg0x1, i256 %_0x399arg0x2, i256 %_0x399arg0x3) #0 {
bb._0x399:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !226
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !227
  br i1 %evm.branch.cond, label %bb._0x12ac, label %bb._0x3a1, !notdec.evm !227

bb._0x12ac:                                       ; preds = %bb._0x399
  call void @evm_revert(ptr %mem, i256 %_0x399arg0x3, i256 %_0x399arg0x3), !notdec.evm !228
  unreachable, !notdec.evm !228

bb._0x3a1:                                        ; preds = %bb._0x399
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !229
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !230
  %evm.slt = icmp slt i256 %evm.add, %_0x399arg0x3, !notdec.evm !231
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !231
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !232
  br i1 %evm.branch.cond1, label %bb._0x12cf, label %bb._0x3ac, !notdec.evm !232

bb._0x12cf:                                       ; preds = %bb._0x3a1
  call void @evm_revert(ptr %mem, i256 %_0x399arg0x3, i256 %_0x399arg0x3), !notdec.evm !233
  unreachable, !notdec.evm !233

bb._0x3ac:                                        ; preds = %bb._0x3a1
  %notdec.evm.mem.ptr.25 = inttoptr i256 %_0x399arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.25, align 1, !notdec.evm !234
  %notdec.evm.mem.ptr.26 = inttoptr i256 %evm.mload to ptr
  store i256 18, ptr %notdec.evm.mem.ptr.26, align 1, !notdec.evm !235
  call void @evm_return(ptr %mem, i256 %evm.mload, i256 32), !notdec.evm !236
  ret void, !notdec.evm !236
}

define void @public_transferFrom_address_address_uint256__0x3b5(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x3b5arg0x0, i256 %_0x3b5arg0x1, i256 %_0x3b5arg0x2, i256 %_0x3b5arg0x3) #0 {
bb._0x3b5:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !237
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !238
  br i1 %evm.branch.cond, label %bb._0x12f2, label %bb._0x3bd, !notdec.evm !238

bb._0x12f2:                                       ; preds = %bb._0x3b5
  call void @evm_revert(ptr %mem, i256 %_0x3b5arg0x3, i256 %_0x3b5arg0x3), !notdec.evm !239
  unreachable, !notdec.evm !239

bb._0x3bd:                                        ; preds = %bb._0x3b5
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !240
  %evm.add = add i256 -4, %evm.calldatasize, !notdec.evm !241
  %evm.slt = icmp slt i256 %evm.add, 96, !notdec.evm !242
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !242
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !243
  br i1 %evm.branch.cond1, label %bb._0x1314, label %bb._0x3c9, !notdec.evm !243

bb._0x1314:                                       ; preds = %bb._0x3bd
  call void @evm_revert(ptr %mem, i256 %_0x3b5arg0x3, i256 %_0x3b5arg0x3), !notdec.evm !244
  unreachable, !notdec.evm !244

bb._0x3c9:                                        ; preds = %bb._0x3bd
  %private.call = call i256 @private__0x6b0_0x6b0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 976), !notdec.evm !245
  br label %bb._0x3d0

bb._0x3d0:                                        ; preds = %bb._0x3c9
  %private.call2 = call i256 @private__0x6cb_0x6cb(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 984), !notdec.evm !246
  br label %bb._0x3d8

bb._0x3d8:                                        ; preds = %bb._0x3d0
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 68), !notdec.evm !247
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !248
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !249
  %evm.and = and i256 %private.call, %evm.sub, !notdec.evm !250
  %notdec.evm.mem.ptr.27 = inttoptr i256 %_0x3b5arg0x3 to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.27, align 1, !notdec.evm !251
  %notdec.evm.mem.ptr.28 = inttoptr i256 32 to ptr
  store i256 1, ptr %notdec.evm.mem.ptr.28, align 1, !notdec.evm !252
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 %_0x3b5arg0x3, i256 %_0x3b5arg0x2), !notdec.evm !253
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !254
  %notdec.evm.mem.ptr.29 = inttoptr i256 %_0x3b5arg0x3 to ptr
  store i256 %evm.caller, ptr %notdec.evm.mem.ptr.29, align 1, !notdec.evm !255
  %notdec.evm.mem.ptr.30 = inttoptr i256 32 to ptr
  store i256 %evm.sha3, ptr %notdec.evm.mem.ptr.30, align 1, !notdec.evm !256
  %evm.sha33 = call i256 @evm_sha3(ptr %mem, i256 %_0x3b5arg0x3, i256 %_0x3b5arg0x2), !notdec.evm !257
  %evm.sload = call i256 @evm_sload(i256 %evm.sha33), !notdec.evm !258
  %evm.sub4 = sub i256 %evm.sload, -1, !notdec.evm !259
  %evm.branch.cond5 = icmp ne i256 %evm.sub4, 0, !notdec.evm !260
  br i1 %evm.branch.cond5, label %bb._0x414, label %bb._0x406, !notdec.evm !260

bb._0x414:                                        ; preds = %bb._0x3d8
  %evm.lt = icmp ult i256 %evm.sload, %evm.calldataload, !notdec.evm !261
  %evm.bool6 = zext i1 %evm.lt to i256, !notdec.evm !261
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !262
  br i1 %evm.branch.cond7, label %bb._0x47c, label %bb._0x41c, !notdec.evm !262

bb._0x47c:                                        ; preds = %bb._0x414
  %notdec.evm.mem.ptr.31 = inttoptr i256 %_0x3b5arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.31, align 1, !notdec.evm !263
  %evm.shl8 = call i256 @evm_shl(i256 225, i256 2110234841), !notdec.evm !264
  %notdec.evm.mem.ptr.32 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.shl8, ptr %notdec.evm.mem.ptr.32, align 1, !notdec.evm !265
  %evm.caller9 = call i256 @evm_caller(ptr %env), !notdec.evm !266
  %evm.add10 = add i256 %evm.mload, %_0x3b5arg0x1, !notdec.evm !267
  %notdec.evm.mem.ptr.33 = inttoptr i256 %evm.add10 to ptr
  store i256 %evm.caller9, ptr %notdec.evm.mem.ptr.33, align 1, !notdec.evm !268
  %evm.add11 = add i256 %evm.add10, 32, !notdec.evm !269
  %notdec.evm.mem.ptr.34 = inttoptr i256 %evm.add11 to ptr
  store i256 %evm.sload, ptr %notdec.evm.mem.ptr.34, align 1, !notdec.evm !270
  %evm.add12 = add i256 %evm.add10, 64, !notdec.evm !271
  %notdec.evm.mem.ptr.35 = inttoptr i256 %evm.add12 to ptr
  store i256 %evm.calldataload, ptr %notdec.evm.mem.ptr.35, align 1, !notdec.evm !272
  %evm.add13 = add i256 96, %evm.add10, !notdec.evm !273
  %evm.sub14 = sub i256 %evm.add13, %evm.mload, !notdec.evm !274
  call void @evm_revert(ptr %mem, i256 %evm.mload, i256 %evm.sub14), !notdec.evm !275
  unreachable, !notdec.evm !275

bb._0x41c:                                        ; preds = %bb._0x414
  %evm.iszero = icmp eq i256 %evm.and, 0, !notdec.evm !276
  %evm.bool15 = zext i1 %evm.iszero to i256, !notdec.evm !276
  %evm.branch.cond16 = icmp ne i256 %evm.bool15, 0, !notdec.evm !277
  br i1 %evm.branch.cond16, label %bb._0x465, label %bb._0x422, !notdec.evm !277

bb._0x465:                                        ; preds = %bb._0x41c
  %notdec.evm.mem.ptr.36 = inttoptr i256 %_0x3b5arg0x2 to ptr
  %evm.mload17 = load i256, ptr %notdec.evm.mem.ptr.36, align 1, !notdec.evm !278
  %evm.shl18 = call i256 @evm_shl(i256 224, i256 3858947845), !notdec.evm !279
  %notdec.evm.mem.ptr.37 = inttoptr i256 %evm.mload17 to ptr
  store i256 %evm.shl18, ptr %notdec.evm.mem.ptr.37, align 1, !notdec.evm !280
  %evm.add19 = add i256 %evm.mload17, %_0x3b5arg0x1, !notdec.evm !281
  %notdec.evm.mem.ptr.38 = inttoptr i256 %evm.add19 to ptr
  store i256 %_0x3b5arg0x3, ptr %notdec.evm.mem.ptr.38, align 1, !notdec.evm !282
  call void @evm_revert(ptr %mem, i256 %evm.mload17, i256 36), !notdec.evm !283
  unreachable, !notdec.evm !283

bb._0x422:                                        ; preds = %bb._0x41c
  %evm.caller20 = call i256 @evm_caller(ptr %env), !notdec.evm !284
  %evm.iszero21 = icmp eq i256 %evm.caller20, 0, !notdec.evm !285
  %evm.bool22 = zext i1 %evm.iszero21 to i256, !notdec.evm !285
  %evm.branch.cond23 = icmp ne i256 %evm.bool22, 0, !notdec.evm !286
  br i1 %evm.branch.cond23, label %bb._0x44e, label %bb._0x428, !notdec.evm !286

bb._0x44e:                                        ; preds = %bb._0x422
  %notdec.evm.mem.ptr.39 = inttoptr i256 %_0x3b5arg0x2 to ptr
  %evm.mload24 = load i256, ptr %notdec.evm.mem.ptr.39, align 1, !notdec.evm !287
  %evm.shl25 = call i256 @evm_shl(i256 225, i256 1242826417), !notdec.evm !288
  %notdec.evm.mem.ptr.40 = inttoptr i256 %evm.mload24 to ptr
  store i256 %evm.shl25, ptr %notdec.evm.mem.ptr.40, align 1, !notdec.evm !289
  %evm.add26 = add i256 %evm.mload24, %_0x3b5arg0x1, !notdec.evm !290
  %notdec.evm.mem.ptr.41 = inttoptr i256 %evm.add26 to ptr
  store i256 %_0x3b5arg0x3, ptr %notdec.evm.mem.ptr.41, align 1, !notdec.evm !291
  call void @evm_revert(ptr %mem, i256 %evm.mload24, i256 36), !notdec.evm !292
  unreachable, !notdec.evm !292

bb._0x428:                                        ; preds = %bb._0x422
  %notdec.evm.mem.ptr.42 = inttoptr i256 %_0x3b5arg0x3 to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.42, align 1, !notdec.evm !293
  %notdec.evm.mem.ptr.43 = inttoptr i256 32 to ptr
  store i256 1, ptr %notdec.evm.mem.ptr.43, align 1, !notdec.evm !294
  %evm.sha327 = call i256 @evm_sha3(ptr %mem, i256 %_0x3b5arg0x3, i256 %_0x3b5arg0x2), !notdec.evm !295
  %evm.caller28 = call i256 @evm_caller(ptr %env), !notdec.evm !296
  %notdec.evm.mem.ptr.44 = inttoptr i256 %_0x3b5arg0x3 to ptr
  store i256 %evm.caller28, ptr %notdec.evm.mem.ptr.44, align 1, !notdec.evm !297
  %notdec.evm.mem.ptr.45 = inttoptr i256 32 to ptr
  store i256 %evm.sha327, ptr %notdec.evm.mem.ptr.45, align 1, !notdec.evm !298
  %evm.sha329 = call i256 @evm_sha3(ptr %mem, i256 %_0x3b5arg0x3, i256 %_0x3b5arg0x2), !notdec.evm !299
  %evm.sub30 = sub i256 %evm.sload, %evm.calldataload, !notdec.evm !300
  call void @evm_sstore(i256 %evm.sha329, i256 %evm.sub30), !notdec.evm !301
  br label %bb._0x406, !notdec.evm !302

bb._0x406:                                        ; preds = %bb._0x428, %bb._0x3d8
  %_0x406_0x0 = phi i256 [ %_0x3b5arg0x1, %bb._0x3d8 ], [ %_0x3b5arg0x3, %bb._0x428 ], !notdec.evm !303
  %_0x406_0x1 = phi i256 [ %evm.and, %bb._0x3d8 ], [ 31583, %bb._0x428 ], !notdec.evm !304
  %_0x406_0x2 = phi i256 [ %evm.sload, %bb._0x3d8 ], [ 32, %bb._0x428 ], !notdec.evm !305
  %_0x406_0x3 = phi i256 [ %_0x3b5arg0x3, %bb._0x3d8 ], [ %private.call2, %bb._0x428 ], !notdec.evm !306
  call void @private__0x6e1_0x6e1(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call, i256 %private.call2, i256 %evm.calldataload, i256 31545), !notdec.evm !307
  br label %bb._0x7b39

bb._0x7b39:                                       ; preds = %bb._0x406
  %_0x7b39_0x2 = phi i256 [ %_0x406_0x0, %bb._0x406 ], !notdec.evm !308
  %_0x7b39_0x3 = phi i256 [ %_0x406_0x1, %bb._0x406 ], !notdec.evm !309
  %_0x7b39_0x4 = phi i256 [ %_0x406_0x2, %bb._0x406 ], !notdec.evm !310
  %_0x7b39_0x5 = phi i256 [ %_0x406_0x3, %bb._0x406 ], !notdec.evm !311
  %notdec.evm.mem.ptr.46 = inttoptr i256 %_0x3b5arg0x2 to ptr
  %evm.mload31 = load i256, ptr %notdec.evm.mem.ptr.46, align 1, !notdec.evm !312
  %notdec.evm.mem.ptr.47 = inttoptr i256 %evm.mload31 to ptr
  store i256 1, ptr %notdec.evm.mem.ptr.47, align 1, !notdec.evm !313
  call void @evm_return(ptr %mem, i256 %evm.mload31, i256 32), !notdec.evm !314
  ret void, !notdec.evm !314
}

define void @public_totalSupply___0x4a8(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x4a8arg0x0, i256 %_0x4a8arg0x1, i256 %_0x4a8arg0x2, i256 %_0x4a8arg0x3) #0 {
bb._0x4a8:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !315
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !316
  br i1 %evm.branch.cond, label %bb._0x1336, label %bb._0x4b0, !notdec.evm !316

bb._0x1336:                                       ; preds = %bb._0x4a8
  call void @evm_revert(ptr %mem, i256 %_0x4a8arg0x3, i256 %_0x4a8arg0x3), !notdec.evm !317
  unreachable, !notdec.evm !317

bb._0x4b0:                                        ; preds = %bb._0x4a8
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !318
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !319
  %evm.slt = icmp slt i256 %evm.add, %_0x4a8arg0x3, !notdec.evm !320
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !320
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !321
  br i1 %evm.branch.cond1, label %bb._0x1359, label %bb._0x4bb, !notdec.evm !321

bb._0x1359:                                       ; preds = %bb._0x4b0
  call void @evm_revert(ptr %mem, i256 %_0x4a8arg0x3, i256 %_0x4a8arg0x3), !notdec.evm !322
  unreachable, !notdec.evm !322

bb._0x4bb:                                        ; preds = %bb._0x4b0
  %evm.sload = call i256 @evm_sload(i256 2), !notdec.evm !323
  %notdec.evm.mem.ptr.48 = inttoptr i256 %_0x4a8arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.48, align 1, !notdec.evm !324
  %notdec.evm.mem.ptr.49 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.sload, ptr %notdec.evm.mem.ptr.49, align 1, !notdec.evm !325
  call void @evm_return(ptr %mem, i256 %evm.mload, i256 32), !notdec.evm !326
  ret void, !notdec.evm !326
}

define void @public_approve_address_uint256__0x4c7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x4c7arg0x0, i256 %_0x4c7arg0x1, i256 %_0x4c7arg0x2, i256 %_0x4c7arg0x3) #0 {
bb._0x4c7:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !327
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !328
  br i1 %evm.branch.cond, label %bb._0x137c, label %bb._0x4ce, !notdec.evm !328

bb._0x137c:                                       ; preds = %bb._0x4c7
  call void @evm_revert(ptr %mem, i256 %_0x4c7arg0x3, i256 %_0x4c7arg0x3), !notdec.evm !329
  unreachable, !notdec.evm !329

bb._0x4ce:                                        ; preds = %bb._0x4c7
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !330
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !331
  %evm.slt = icmp slt i256 %evm.add, %_0x4c7arg0x2, !notdec.evm !332
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !332
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !333
  br i1 %evm.branch.cond1, label %bb._0x139f, label %bb._0x4d9, !notdec.evm !333

bb._0x139f:                                       ; preds = %bb._0x4ce
  call void @evm_revert(ptr %mem, i256 %_0x4c7arg0x3, i256 %_0x4c7arg0x3), !notdec.evm !334
  unreachable, !notdec.evm !334

bb._0x4d9:                                        ; preds = %bb._0x4ce
  %private.call = call i256 @private__0x6b0_0x6b0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1248), !notdec.evm !335
  br label %bb._0x4e0

bb._0x4e0:                                        ; preds = %bb._0x4d9
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 36), !notdec.evm !336
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !337
  %evm.iszero = icmp eq i256 %evm.caller, 0, !notdec.evm !338
  %evm.bool2 = zext i1 %evm.iszero to i256, !notdec.evm !338
  %evm.branch.cond3 = icmp ne i256 %evm.bool2, 0, !notdec.evm !339
  br i1 %evm.branch.cond3, label %bb._0x559, label %bb._0x4eb, !notdec.evm !339

bb._0x559:                                        ; preds = %bb._0x4e0
  %notdec.evm.mem.ptr.50 = inttoptr i256 %_0x4c7arg0x2 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.50, align 1, !notdec.evm !340
  %evm.shl = call i256 @evm_shl(i256 224, i256 3858947845), !notdec.evm !341
  %notdec.evm.mem.ptr.51 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.shl, ptr %notdec.evm.mem.ptr.51, align 1, !notdec.evm !342
  %evm.add4 = add i256 %_0x4c7arg0x1, %evm.mload, !notdec.evm !343
  %notdec.evm.mem.ptr.52 = inttoptr i256 %evm.add4 to ptr
  store i256 %_0x4c7arg0x3, ptr %notdec.evm.mem.ptr.52, align 1, !notdec.evm !344
  call void @evm_revert(ptr %mem, i256 %evm.mload, i256 36), !notdec.evm !345
  unreachable, !notdec.evm !345

bb._0x4eb:                                        ; preds = %bb._0x4e0
  %evm.shl5 = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !346
  %evm.sub = sub i256 %evm.shl5, 1, !notdec.evm !347
  %evm.and = and i256 %evm.sub, %private.call, !notdec.evm !348
  %evm.iszero6 = icmp eq i256 %evm.and, 0, !notdec.evm !349
  %evm.bool7 = zext i1 %evm.iszero6 to i256, !notdec.evm !349
  %evm.branch.cond8 = icmp ne i256 %evm.bool7, 0, !notdec.evm !350
  br i1 %evm.branch.cond8, label %bb._0x542, label %bb._0x4fb, !notdec.evm !350

bb._0x542:                                        ; preds = %bb._0x4eb
  %notdec.evm.mem.ptr.53 = inttoptr i256 %_0x4c7arg0x2 to ptr
  %evm.mload9 = load i256, ptr %notdec.evm.mem.ptr.53, align 1, !notdec.evm !351
  %evm.shl10 = call i256 @evm_shl(i256 225, i256 1242826417), !notdec.evm !352
  %notdec.evm.mem.ptr.54 = inttoptr i256 %evm.mload9 to ptr
  store i256 %evm.shl10, ptr %notdec.evm.mem.ptr.54, align 1, !notdec.evm !353
  %evm.add11 = add i256 %evm.mload9, %_0x4c7arg0x1, !notdec.evm !354
  %notdec.evm.mem.ptr.55 = inttoptr i256 %evm.add11 to ptr
  store i256 %_0x4c7arg0x3, ptr %notdec.evm.mem.ptr.55, align 1, !notdec.evm !355
  call void @evm_revert(ptr %mem, i256 %evm.mload9, i256 36), !notdec.evm !356
  unreachable, !notdec.evm !356

bb._0x4fb:                                        ; preds = %bb._0x4eb
  %evm.caller12 = call i256 @evm_caller(ptr %env), !notdec.evm !357
  %notdec.evm.mem.ptr.56 = inttoptr i256 %_0x4c7arg0x3 to ptr
  store i256 %evm.caller12, ptr %notdec.evm.mem.ptr.56, align 1, !notdec.evm !358
  %notdec.evm.mem.ptr.57 = inttoptr i256 32 to ptr
  store i256 1, ptr %notdec.evm.mem.ptr.57, align 1, !notdec.evm !359
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 %_0x4c7arg0x3, i256 %_0x4c7arg0x2), !notdec.evm !360
  %notdec.evm.mem.ptr.58 = inttoptr i256 %_0x4c7arg0x3 to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.58, align 1, !notdec.evm !361
  %notdec.evm.mem.ptr.59 = inttoptr i256 32 to ptr
  store i256 %evm.sha3, ptr %notdec.evm.mem.ptr.59, align 1, !notdec.evm !362
  %evm.sha313 = call i256 @evm_sha3(ptr %mem, i256 %_0x4c7arg0x3, i256 %_0x4c7arg0x2), !notdec.evm !363
  call void @evm_sstore(i256 %evm.sha313, i256 %evm.calldataload), !notdec.evm !364
  %notdec.evm.mem.ptr.60 = inttoptr i256 %_0x4c7arg0x2 to ptr
  %evm.mload14 = load i256, ptr %notdec.evm.mem.ptr.60, align 1, !notdec.evm !365
  %notdec.evm.mem.ptr.61 = inttoptr i256 %evm.mload14 to ptr
  store i256 %evm.calldataload, ptr %notdec.evm.mem.ptr.61, align 1, !notdec.evm !366
  %evm.caller15 = call i256 @evm_caller(ptr %env), !notdec.evm !367
  call void @evm_log3(ptr %mem, i256 %evm.mload14, i256 32, i256 -52305948261162578668367882225327028569797882979485679342215860919519743330011, i256 %evm.caller15, i256 %evm.and), !notdec.evm !368
  %notdec.evm.mem.ptr.62 = inttoptr i256 %_0x4c7arg0x2 to ptr
  %evm.mload16 = load i256, ptr %notdec.evm.mem.ptr.62, align 1, !notdec.evm !369
  %notdec.evm.mem.ptr.63 = inttoptr i256 %evm.mload16 to ptr
  store i256 1, ptr %notdec.evm.mem.ptr.63, align 1, !notdec.evm !370
  call void @evm_return(ptr %mem, i256 %evm.mload16, i256 32), !notdec.evm !371
  ret void, !notdec.evm !371
}

define void @public_name___0x570(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x570arg0x0, i256 %_0x570arg0x1, i256 %_0x570arg0x2, i256 %_0x570arg0x3, i256 %_0x570arg0x4) #0 {
bb._0x570:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !372
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !373
  br i1 %evm.branch.cond, label %bb._0x13c2, label %bb._0x57a, !notdec.evm !373

bb._0x13c2:                                       ; preds = %bb._0x570
  call void @evm_revert(ptr %mem, i256 %_0x570arg0x4, i256 %_0x570arg0x4), !notdec.evm !374
  unreachable, !notdec.evm !374

bb._0x57a:                                        ; preds = %bb._0x570
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !375
  %evm.add = add i256 %evm.calldatasize, -4, !notdec.evm !376
  %evm.slt = icmp slt i256 %evm.add, %_0x570arg0x4, !notdec.evm !377
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !377
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !378
  br i1 %evm.branch.cond1, label %bb._0x13e5, label %bb._0x585, !notdec.evm !378

bb._0x13e5:                                       ; preds = %bb._0x57a
  call void @evm_revert(ptr %mem, i256 %_0x570arg0x4, i256 %_0x570arg0x4), !notdec.evm !379
  unreachable, !notdec.evm !379

bb._0x585:                                        ; preds = %bb._0x57a
  %evm.sload = call i256 @evm_sload(i256 3), !notdec.evm !380
  %evm.shr = call i256 @evm_shr(i256 1, i256 %evm.sload), !notdec.evm !381
  %evm.and = and i256 %evm.sload, 1, !notdec.evm !382
  %evm.iszero = icmp eq i256 %evm.and, 0, !notdec.evm !383
  %evm.bool2 = zext i1 %evm.iszero to i256, !notdec.evm !383
  %evm.branch.cond3 = icmp ne i256 %evm.bool2, 0, !notdec.evm !384
  br i1 %evm.branch.cond3, label %bb._0x65d, label %bb._0x59b, !notdec.evm !384

bb._0x65d:                                        ; preds = %bb._0x585
  %evm.and4 = and i256 127, %evm.shr, !notdec.evm !385
  br label %bb._0x59b, !notdec.evm !386

bb._0x59b:                                        ; preds = %bb._0x65d, %bb._0x585
  %_0x59b_0x2 = phi i256 [ %evm.shr, %bb._0x585 ], [ %evm.and4, %bb._0x65d ], !notdec.evm !387
  %evm.lt = icmp ult i256 %_0x59b_0x2, 32, !notdec.evm !388
  %evm.bool5 = zext i1 %evm.lt to i256, !notdec.evm !388
  %evm.eq = icmp eq i256 %evm.and, %evm.bool5, !notdec.evm !389
  %evm.bool6 = zext i1 %evm.eq to i256, !notdec.evm !389
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !390
  br i1 %evm.branch.cond7, label %bb._0x1408, label %bb._0x5a8, !notdec.evm !390

bb._0x1408:                                       ; preds = %bb._0x59b
  %_0x1408_0x3 = phi i256 [ %_0x59b_0x2, %bb._0x59b ], !notdec.evm !391
  %evm.shl = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !392
  %notdec.evm.mem.ptr.64 = inttoptr i256 %_0x570arg0x4 to ptr
  store i256 %evm.shl, ptr %notdec.evm.mem.ptr.64, align 1, !notdec.evm !393
  %notdec.evm.mem.ptr.65 = inttoptr i256 %_0x570arg0x2 to ptr
  store i256 34, ptr %notdec.evm.mem.ptr.65, align 1, !notdec.evm !394
  call void @evm_revert(ptr %mem, i256 %_0x570arg0x4, i256 36), !notdec.evm !395
  unreachable, !notdec.evm !395

bb._0x5a8:                                        ; preds = %bb._0x59b
  %_0x5a8_0x3 = phi i256 [ %_0x59b_0x2, %bb._0x59b ], !notdec.evm !396
  %notdec.evm.mem.ptr.66 = inttoptr i256 %_0x570arg0x0 to ptr
  store i256 %_0x5a8_0x3, ptr %notdec.evm.mem.ptr.66, align 1, !notdec.evm !397
  %evm.iszero8 = icmp eq i256 %evm.and, 0, !notdec.evm !398
  %evm.bool9 = zext i1 %evm.iszero8 to i256, !notdec.evm !398
  %evm.branch.cond10 = icmp ne i256 %evm.bool9, 0, !notdec.evm !399
  br i1 %evm.branch.cond10, label %bb._0x641, label %bb._0x5b2, !notdec.evm !399

bb._0x641:                                        ; preds = %bb._0x5a8
  %_0x641_0x3 = phi i256 [ %_0x5a8_0x3, %bb._0x5a8 ], !notdec.evm !400
  %evm.and11 = and i256 -256, %evm.sload, !notdec.evm !401
  %evm.add12 = add i256 32, %_0x570arg0x0, !notdec.evm !402
  %notdec.evm.mem.ptr.67 = inttoptr i256 %evm.add12 to ptr
  store i256 %evm.and11, ptr %notdec.evm.mem.ptr.67, align 1, !notdec.evm !403
  %evm.iszero13 = icmp eq i256 %_0x641_0x3, 0, !notdec.evm !404
  %evm.bool14 = zext i1 %evm.iszero13 to i256, !notdec.evm !404
  %evm.iszero15 = icmp eq i256 %evm.bool14, 0, !notdec.evm !405
  %evm.bool16 = zext i1 %evm.iszero15 to i256, !notdec.evm !405
  %evm.shl17 = call i256 @evm_shl(i256 5, i256 %evm.bool16), !notdec.evm !406
  %evm.add18 = add i256 %_0x570arg0x0, %evm.shl17, !notdec.evm !407
  %evm.add19 = add i256 %evm.add18, 32, !notdec.evm !408
  br label %bb._0x2020x570, !notdec.evm !409

bb._0x5b2:                                        ; preds = %bb._0x5a8
  %_0x5b2_0x3 = phi i256 [ %_0x5a8_0x3, %bb._0x5a8 ], !notdec.evm !410
  %evm.eq20 = icmp eq i256 1, %evm.and, !notdec.evm !411
  %evm.bool21 = zext i1 %evm.eq20 to i256, !notdec.evm !411
  %evm.branch.cond22 = icmp ne i256 %evm.bool21, 0, !notdec.evm !412
  br i1 %evm.branch.cond22, label %bb._0x5eb, label %bb._0x5ba, !notdec.evm !412

bb._0x5eb:                                        ; preds = %bb._0x5b2
  %_0x5eb_0x1 = phi i256 [ %_0x5b2_0x3, %bb._0x5b2 ], !notdec.evm !413
  %notdec.evm.mem.ptr.68 = inttoptr i256 %_0x570arg0x4 to ptr
  store i256 3, ptr %notdec.evm.mem.ptr.68, align 1, !notdec.evm !414
  br label %bb._0x617, !notdec.evm !415

bb._0x617:                                        ; preds = %bb._0x62d, %bb._0x5eb
  %_0x617_0x0 = phi i256 [ -27889059366240281169193357100633332908378892778709981755071813198463099602853, %bb._0x5eb ], [ %evm.add30, %bb._0x62d ], !notdec.evm !416
  %_0x617_0x3 = phi i256 [ %_0x5eb_0x1, %bb._0x5eb ], [ %_0x62d_0x3, %bb._0x62d ], !notdec.evm !417
  %_0x617_0x4 = phi i256 [ %_0x570arg0x4, %bb._0x5eb ], [ %evm.add29, %bb._0x62d ], !notdec.evm !418
  %evm.lt23 = icmp ult i256 %_0x617_0x4, %_0x617_0x3, !notdec.evm !419
  %evm.bool24 = zext i1 %evm.lt23 to i256, !notdec.evm !419
  %evm.branch.cond25 = icmp ne i256 %evm.bool24, 0, !notdec.evm !420
  br i1 %evm.branch.cond25, label %bb._0x62d, label %bb._0x61f, !notdec.evm !420

bb._0x62d:                                        ; preds = %bb._0x617
  %_0x62d_0x0 = phi i256 [ %_0x617_0x0, %bb._0x617 ], !notdec.evm !421
  %_0x62d_0x3 = phi i256 [ %_0x617_0x3, %bb._0x617 ], !notdec.evm !422
  %_0x62d_0x4 = phi i256 [ %_0x617_0x4, %bb._0x617 ], !notdec.evm !423
  %evm.sload26 = call i256 @evm_sload(i256 %_0x62d_0x0), !notdec.evm !424
  %evm.add27 = add i256 %_0x62d_0x4, %_0x570arg0x0, !notdec.evm !425
  %evm.add28 = add i256 32, %evm.add27, !notdec.evm !426
  %notdec.evm.mem.ptr.69 = inttoptr i256 %evm.add28 to ptr
  store i256 %evm.sload26, ptr %notdec.evm.mem.ptr.69, align 1, !notdec.evm !427
  %evm.add29 = add i256 %_0x62d_0x4, 32, !notdec.evm !428
  %evm.add30 = add i256 1, %_0x62d_0x0, !notdec.evm !429
  br label %bb._0x617, !notdec.evm !430

bb._0x61f:                                        ; preds = %bb._0x617
  %_0x61f_0x0 = phi i256 [ %_0x617_0x0, %bb._0x617 ], !notdec.evm !431
  %_0x61f_0x3 = phi i256 [ %_0x617_0x3, %bb._0x617 ], !notdec.evm !432
  %_0x61f_0x4 = phi i256 [ %_0x617_0x4, %bb._0x617 ], !notdec.evm !433
  %evm.add31 = add i256 %_0x570arg0x0, %_0x61f_0x4, !notdec.evm !434
  %evm.add32 = add i256 %evm.add31, 32, !notdec.evm !435
  br label %bb._0x2020x570, !notdec.evm !436

bb._0x2020x570:                                   ; preds = %bb._0x61f, %bb._0x641
  %_0x2020x570_0x3 = phi i256 [ %evm.add32, %bb._0x61f ], [ %evm.add19, %bb._0x641 ], !notdec.evm !437
  %evm.sub = sub i256 %_0x2020x570_0x3, %_0x570arg0x0, !notdec.evm !438
  %evm.add33 = add i256 31, %evm.sub, !notdec.evm !439
  %evm.and34 = and i256 -32, %evm.add33, !notdec.evm !440
  %evm.add35 = add i256 %_0x570arg0x0, %evm.and34, !notdec.evm !441
  %evm.gt = icmp ugt i256 %evm.add35, 18446744073709551615, !notdec.evm !442
  %evm.bool36 = zext i1 %evm.gt to i256, !notdec.evm !442
  %evm.lt37 = icmp ult i256 %evm.add35, %_0x570arg0x0, !notdec.evm !443
  %evm.bool38 = zext i1 %evm.lt37 to i256, !notdec.evm !443
  %evm.or = or i256 %evm.bool38, %evm.bool36, !notdec.evm !444
  %evm.branch.cond39 = icmp ne i256 %evm.or, 0, !notdec.evm !445
  br i1 %evm.branch.cond39, label %bb._0x11aa0x570, label %bb._0x2260x570, !notdec.evm !445

bb._0x11aa0x570:                                  ; preds = %bb._0x2020x570
  %evm.shl40 = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !446
  %notdec.evm.mem.ptr.70 = inttoptr i256 %_0x570arg0x4 to ptr
  store i256 %evm.shl40, ptr %notdec.evm.mem.ptr.70, align 1, !notdec.evm !447
  %notdec.evm.mem.ptr.71 = inttoptr i256 %_0x570arg0x2 to ptr
  store i256 65, ptr %notdec.evm.mem.ptr.71, align 1, !notdec.evm !448
  call void @evm_revert(ptr %mem, i256 %_0x570arg0x4, i256 36), !notdec.evm !449
  unreachable, !notdec.evm !449

bb._0x2260x570:                                   ; preds = %bb._0x2020x570
  %notdec.evm.mem.ptr.72 = inttoptr i256 %_0x570arg0x3 to ptr
  store i256 %evm.add35, ptr %notdec.evm.mem.ptr.72, align 1, !notdec.evm !450
  %private.call = call i256 @private__0x667_0x667(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.add35, i256 %_0x570arg0x0, i256 31510), !notdec.evm !451
  br label %bb._0x7b160x570

bb._0x7b160x570:                                  ; preds = %bb._0x2260x570
  %evm.sub41 = sub i256 %private.call, %evm.add35, !notdec.evm !452
  call void @evm_return(ptr %mem, i256 %evm.add35, i256 %evm.sub41), !notdec.evm !453
  ret void, !notdec.evm !453

bb._0x5ba:                                        ; preds = %bb._0x5b2
  %_0x5ba_0x1 = phi i256 [ %_0x5b2_0x3, %bb._0x5b2 ], !notdec.evm !454
  %evm.sub42 = sub i256 %_0x570arg0x4, %_0x570arg0x0, !notdec.evm !455
  %evm.add43 = add i256 31, %evm.sub42, !notdec.evm !456
  %evm.and44 = and i256 -32, %evm.add43, !notdec.evm !457
  %evm.add45 = add i256 %_0x570arg0x0, %evm.and44, !notdec.evm !458
  %evm.gt46 = icmp ugt i256 %evm.add45, 18446744073709551615, !notdec.evm !459
  %evm.bool47 = zext i1 %evm.gt46 to i256, !notdec.evm !459
  %evm.lt48 = icmp ult i256 %evm.add45, %_0x570arg0x0, !notdec.evm !460
  %evm.bool49 = zext i1 %evm.lt48 to i256, !notdec.evm !460
  %evm.or50 = or i256 %evm.bool49, %evm.bool47, !notdec.evm !461
  %evm.branch.cond51 = icmp ne i256 %evm.or50, 0, !notdec.evm !462
  br i1 %evm.branch.cond51, label %bb._0x143a, label %bb._0x5dd, !notdec.evm !462

bb._0x143a:                                       ; preds = %bb._0x5ba
  %evm.shl52 = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !463
  %notdec.evm.mem.ptr.73 = inttoptr i256 %_0x570arg0x4 to ptr
  store i256 %evm.shl52, ptr %notdec.evm.mem.ptr.73, align 1, !notdec.evm !464
  %notdec.evm.mem.ptr.74 = inttoptr i256 %_0x570arg0x2 to ptr
  store i256 65, ptr %notdec.evm.mem.ptr.74, align 1, !notdec.evm !465
  call void @evm_revert(ptr %mem, i256 %_0x570arg0x4, i256 36), !notdec.evm !466
  unreachable, !notdec.evm !466

bb._0x5dd:                                        ; preds = %bb._0x5ba
  %notdec.evm.mem.ptr.75 = inttoptr i256 %_0x570arg0x3 to ptr
  store i256 %evm.add45, ptr %notdec.evm.mem.ptr.75, align 1, !notdec.evm !467
  %private.call53 = call i256 @private__0x667_0x667(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.add45, i256 %_0x570arg0x0, i256 31621), !notdec.evm !468
  br label %bb._0x7b85

bb._0x7b85:                                       ; preds = %bb._0x5dd
  %evm.sub54 = sub i256 %private.call53, %evm.add45, !notdec.evm !469
  call void @evm_return(ptr %mem, i256 %evm.add45, i256 %evm.sub54), !notdec.evm !470
  ret void, !notdec.evm !470
}

define i256 @private__0x667_0x667(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x667arg0x0, i256 %_0x667arg0x1, i256 %_0x667arg0x2) #0 {
bb._0x667:
  %notdec.evm.mem.ptr.76 = inttoptr i256 %_0x667arg0x0 to ptr
  store i256 32, ptr %notdec.evm.mem.ptr.76, align 1, !notdec.evm !471
  %notdec.evm.mem.ptr.77 = inttoptr i256 %_0x667arg0x1 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.77, align 1, !notdec.evm !472
  %evm.add = add i256 %_0x667arg0x0, 32, !notdec.evm !473
  %notdec.evm.mem.ptr.78 = inttoptr i256 %evm.add to ptr
  store i256 %evm.mload, ptr %notdec.evm.mem.ptr.78, align 1, !notdec.evm !474
  br label %bb._0x67a, !notdec.evm !475

bb._0x67a:                                        ; preds = %bb._0x69c, %bb._0x667
  %_0x67a_0x0 = phi i256 [ 0, %bb._0x667 ], [ %evm.add6, %bb._0x69c ], !notdec.evm !476
  %evm.lt = icmp ult i256 %_0x67a_0x0, %evm.mload, !notdec.evm !477
  %evm.bool = zext i1 %evm.lt to i256, !notdec.evm !477
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !478
  br i1 %evm.branch.cond, label %bb._0x69c, label %bb._0x682, !notdec.evm !478

bb._0x69c:                                        ; preds = %bb._0x67a
  %_0x69c_0x0 = phi i256 [ %_0x67a_0x0, %bb._0x67a ], !notdec.evm !479
  %evm.add1 = add i256 %_0x69c_0x0, %_0x667arg0x1, !notdec.evm !480
  %evm.add2 = add i256 32, %evm.add1, !notdec.evm !481
  %notdec.evm.mem.ptr.79 = inttoptr i256 %evm.add2 to ptr
  %evm.mload3 = load i256, ptr %notdec.evm.mem.ptr.79, align 1, !notdec.evm !482
  %evm.add4 = add i256 %_0x69c_0x0, %_0x667arg0x0, !notdec.evm !483
  %evm.add5 = add i256 64, %evm.add4, !notdec.evm !484
  %notdec.evm.mem.ptr.80 = inttoptr i256 %evm.add5 to ptr
  store i256 %evm.mload3, ptr %notdec.evm.mem.ptr.80, align 1, !notdec.evm !485
  %evm.add6 = add i256 32, %_0x69c_0x0, !notdec.evm !486
  br label %bb._0x67a, !notdec.evm !487

bb._0x682:                                        ; preds = %bb._0x67a
  %_0x682_0x0 = phi i256 [ %_0x67a_0x0, %bb._0x67a ], !notdec.evm !488
  %evm.add7 = add i256 %_0x667arg0x0, %evm.mload, !notdec.evm !489
  %evm.add8 = add i256 %evm.add7, 64, !notdec.evm !490
  %notdec.evm.mem.ptr.81 = inttoptr i256 %evm.add8 to ptr
  store i256 0, ptr %notdec.evm.mem.ptr.81, align 1, !notdec.evm !491
  %evm.add9 = add i256 %evm.mload, 31, !notdec.evm !492
  %evm.and = and i256 %evm.add9, -32, !notdec.evm !493
  %evm.add10 = add i256 %evm.and, %_0x667arg0x0, !notdec.evm !494
  %evm.add11 = add i256 %evm.add10, 64, !notdec.evm !495
  ret i256 %evm.add11, !notdec.evm !496
}

define i256 @private__0x6b0_0x6b0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x6b0arg0x0) #0 {
bb._0x6b0:
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 4), !notdec.evm !497
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !498
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !499
  %evm.and = and i256 %evm.calldataload, %evm.sub, !notdec.evm !500
  %evm.sub1 = sub i256 %evm.calldataload, %evm.and, !notdec.evm !501
  %evm.branch.cond = icmp ne i256 %evm.sub1, 0, !notdec.evm !502
  br i1 %evm.branch.cond, label %bb._0x146c, label %bb._0x6c5, !notdec.evm !502

bb._0x146c:                                       ; preds = %bb._0x6b0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !503
  unreachable, !notdec.evm !503

bb._0x6c5:                                        ; preds = %bb._0x6b0
  ret i256 %evm.calldataload, !notdec.evm !504
}

define i256 @private__0x6cb_0x6cb(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x6cbarg0x0) #0 {
bb._0x6cb:
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 36), !notdec.evm !505
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !506
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !507
  %evm.and = and i256 %evm.calldataload, %evm.sub, !notdec.evm !508
  %evm.sub1 = sub i256 %evm.calldataload, %evm.and, !notdec.evm !509
  %evm.branch.cond = icmp ne i256 %evm.sub1, 0, !notdec.evm !510
  br i1 %evm.branch.cond, label %bb._0x1490, label %bb._0x6e0, !notdec.evm !510

bb._0x1490:                                       ; preds = %bb._0x6cb
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !511
  unreachable, !notdec.evm !511

bb._0x6e0:                                        ; preds = %bb._0x6cb
  ret i256 %evm.calldataload, !notdec.evm !512
}

define void @private__0x6e1_0x6e1(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x6e1arg0x0, i256 %_0x6e1arg0x1, i256 %_0x6e1arg0x2, i256 %_0x6e1arg0x3) #0 {
bb._0x6e1:
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !513
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !514
  %evm.and = and i256 %_0x6e1arg0x0, %evm.sub, !notdec.evm !515
  %evm.iszero = icmp eq i256 %evm.and, 0, !notdec.evm !516
  %evm.bool = zext i1 %evm.iszero to i256, !notdec.evm !516
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !517
  br i1 %evm.branch.cond, label %bb._0x7a6, label %bb._0x6f5, !notdec.evm !517

bb._0x7a6:                                        ; preds = %bb._0x6e1
  %notdec.evm.mem.ptr.82 = inttoptr i256 64 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.82, align 1, !notdec.evm !518
  %evm.shl1 = call i256 @evm_shl(i256 225, i256 1264811663), !notdec.evm !519
  %notdec.evm.mem.ptr.83 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.shl1, ptr %notdec.evm.mem.ptr.83, align 1, !notdec.evm !520
  %evm.add = add i256 %evm.mload, 4, !notdec.evm !521
  %notdec.evm.mem.ptr.84 = inttoptr i256 %evm.add to ptr
  store i256 0, ptr %notdec.evm.mem.ptr.84, align 1, !notdec.evm !522
  call void @evm_revert(ptr %mem, i256 %evm.mload, i256 36), !notdec.evm !523
  unreachable, !notdec.evm !523

bb._0x6f5:                                        ; preds = %bb._0x6e1
  %evm.and2 = and i256 %_0x6e1arg0x1, %evm.sub, !notdec.evm !524
  %evm.iszero3 = icmp eq i256 %evm.and2, 0, !notdec.evm !525
  %evm.bool4 = zext i1 %evm.iszero3 to i256, !notdec.evm !525
  %evm.branch.cond5 = icmp ne i256 %evm.bool4, 0, !notdec.evm !526
  br i1 %evm.branch.cond5, label %bb._0x78d, label %bb._0x6fd, !notdec.evm !526

bb._0x78d:                                        ; preds = %bb._0x6f5
  %notdec.evm.mem.ptr.85 = inttoptr i256 64 to ptr
  %evm.mload6 = load i256, ptr %notdec.evm.mem.ptr.85, align 1, !notdec.evm !527
  %evm.shl7 = call i256 @evm_shl(i256 224, i256 3963891461), !notdec.evm !528
  %notdec.evm.mem.ptr.86 = inttoptr i256 %evm.mload6 to ptr
  store i256 %evm.shl7, ptr %notdec.evm.mem.ptr.86, align 1, !notdec.evm !529
  %evm.add8 = add i256 %evm.mload6, 4, !notdec.evm !530
  %notdec.evm.mem.ptr.87 = inttoptr i256 %evm.add8 to ptr
  store i256 0, ptr %notdec.evm.mem.ptr.87, align 1, !notdec.evm !531
  call void @evm_revert(ptr %mem, i256 %evm.mload6, i256 36), !notdec.evm !532
  unreachable, !notdec.evm !532

bb._0x6fd:                                        ; preds = %bb._0x6f5
  %notdec.evm.mem.ptr.88 = inttoptr i256 0 to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.88, align 1, !notdec.evm !533
  %notdec.evm.mem.ptr.89 = inttoptr i256 32 to ptr
  store i256 0, ptr %notdec.evm.mem.ptr.89, align 1, !notdec.evm !534
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 0, i256 64), !notdec.evm !535
  %evm.sload = call i256 @evm_sload(i256 %evm.sha3), !notdec.evm !536
  %evm.lt = icmp ult i256 %evm.sload, %_0x6e1arg0x2, !notdec.evm !537
  %evm.bool9 = zext i1 %evm.lt to i256, !notdec.evm !537
  %evm.branch.cond10 = icmp ne i256 %evm.bool9, 0, !notdec.evm !538
  br i1 %evm.branch.cond10, label %bb._0x75b, label %bb._0x714, !notdec.evm !538

bb._0x75b:                                        ; preds = %bb._0x6fd
  %notdec.evm.mem.ptr.90 = inttoptr i256 64 to ptr
  %evm.mload11 = load i256, ptr %notdec.evm.mem.ptr.90, align 1, !notdec.evm !539
  %evm.shl12 = call i256 @evm_shl(i256 226, i256 957625571), !notdec.evm !540
  %notdec.evm.mem.ptr.91 = inttoptr i256 %evm.mload11 to ptr
  store i256 %evm.shl12, ptr %notdec.evm.mem.ptr.91, align 1, !notdec.evm !541
  %evm.shl13 = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !542
  %evm.sub14 = sub i256 %evm.shl13, 1, !notdec.evm !543
  %evm.and15 = and i256 %evm.sub14, %_0x6e1arg0x0, !notdec.evm !544
  %evm.add16 = add i256 %evm.mload11, 4, !notdec.evm !545
  %notdec.evm.mem.ptr.92 = inttoptr i256 %evm.add16 to ptr
  store i256 %evm.and15, ptr %notdec.evm.mem.ptr.92, align 1, !notdec.evm !546
  %evm.add17 = add i256 %evm.mload11, 36, !notdec.evm !547
  %notdec.evm.mem.ptr.93 = inttoptr i256 %evm.add17 to ptr
  store i256 %evm.sload, ptr %notdec.evm.mem.ptr.93, align 1, !notdec.evm !548
  %evm.add18 = add i256 %evm.mload11, 68, !notdec.evm !549
  %notdec.evm.mem.ptr.94 = inttoptr i256 %evm.add18 to ptr
  store i256 %_0x6e1arg0x2, ptr %notdec.evm.mem.ptr.94, align 1, !notdec.evm !550
  call void @evm_revert(ptr %mem, i256 %evm.mload11, i256 100), !notdec.evm !551
  unreachable, !notdec.evm !551

bb._0x714:                                        ; preds = %bb._0x6fd
  %notdec.evm.mem.ptr.95 = inttoptr i256 0 to ptr
  store i256 %evm.and, ptr %notdec.evm.mem.ptr.95, align 1, !notdec.evm !552
  %notdec.evm.mem.ptr.96 = inttoptr i256 32 to ptr
  store i256 0, ptr %notdec.evm.mem.ptr.96, align 1, !notdec.evm !553
  %evm.sub19 = sub i256 %evm.sload, %_0x6e1arg0x2, !notdec.evm !554
  %evm.sha320 = call i256 @evm_sha3(ptr %mem, i256 0, i256 64), !notdec.evm !555
  call void @evm_sstore(i256 %evm.sha320, i256 %evm.sub19), !notdec.evm !556
  %notdec.evm.mem.ptr.97 = inttoptr i256 0 to ptr
  store i256 %evm.and2, ptr %notdec.evm.mem.ptr.97, align 1, !notdec.evm !557
  %evm.sha321 = call i256 @evm_sha3(ptr %mem, i256 0, i256 64), !notdec.evm !558
  %evm.sload22 = call i256 @evm_sload(i256 %evm.sha321), !notdec.evm !559
  %evm.add23 = add i256 %evm.sload22, %_0x6e1arg0x2, !notdec.evm !560
  call void @evm_sstore(i256 %evm.sha321, i256 %evm.add23), !notdec.evm !561
  %notdec.evm.mem.ptr.98 = inttoptr i256 64 to ptr
  %evm.mload24 = load i256, ptr %notdec.evm.mem.ptr.98, align 1, !notdec.evm !562
  %notdec.evm.mem.ptr.99 = inttoptr i256 %evm.mload24 to ptr
  store i256 %_0x6e1arg0x2, ptr %notdec.evm.mem.ptr.99, align 1, !notdec.evm !563
  call void @evm_log3(ptr %mem, i256 %evm.mload24, i256 32, i256 -15402802100530019096323380498944738953123845089667699673314898783681816316945, i256 %evm.and, i256 %evm.and2), !notdec.evm !564
  ret void, !notdec.evm !565
}

define void @private__0x7bf_0x7bf(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x7bfarg0x0) #0 {
bb._0x7bf:
  %evm.sload = call i256 @evm_sload(i256 5), !notdec.evm !566
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !567
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !568
  %evm.and = and i256 %evm.sub, %evm.sload, !notdec.evm !569
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !570
  %evm.sub1 = sub i256 %evm.caller, %evm.and, !notdec.evm !571
  %evm.branch.cond = icmp ne i256 %evm.sub1, 0, !notdec.evm !572
  br i1 %evm.branch.cond, label %bb._0x7d3, label %bb._0x7d2, !notdec.evm !572

bb._0x7d3:                                        ; preds = %bb._0x7bf
  %notdec.evm.mem.ptr.100 = inttoptr i256 64 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.100, align 1, !notdec.evm !573
  %evm.shl2 = call i256 @evm_shl(i256 224, i256 294443687), !notdec.evm !574
  %notdec.evm.mem.ptr.101 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.shl2, ptr %notdec.evm.mem.ptr.101, align 1, !notdec.evm !575
  %evm.caller3 = call i256 @evm_caller(ptr %env), !notdec.evm !576
  %evm.add = add i256 %evm.mload, 4, !notdec.evm !577
  %notdec.evm.mem.ptr.102 = inttoptr i256 %evm.add to ptr
  store i256 %evm.caller3, ptr %notdec.evm.mem.ptr.102, align 1, !notdec.evm !578
  call void @evm_revert(ptr %mem, i256 %evm.mload, i256 36), !notdec.evm !579
  unreachable, !notdec.evm !579

bb._0x7d2:                                        ; preds = %bb._0x7bf
  ret void, !notdec.evm !580
}

define void @public_transferOwnership_address__0xa7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0xa7arg0x0, i256 %_0xa7arg0x1, i256 %_0xa7arg0x2) #0 {
bb._0xa7:
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !581
  %evm.branch.cond = icmp ne i256 %evm.callvalue, 0, !notdec.evm !582
  br i1 %evm.branch.cond, label %bb._0x1060, label %bb._0xad, !notdec.evm !582

bb._0x1060:                                       ; preds = %bb._0xa7
  call void @evm_revert(ptr %mem, i256 %_0xa7arg0x2, i256 %_0xa7arg0x2), !notdec.evm !583
  unreachable, !notdec.evm !583

bb._0xad:                                         ; preds = %bb._0xa7
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !584
  %evm.add = add i256 -4, %evm.calldatasize, !notdec.evm !585
  %evm.slt = icmp slt i256 %evm.add, 32, !notdec.evm !586
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !586
  %evm.branch.cond1 = icmp ne i256 %evm.bool, 0, !notdec.evm !587
  br i1 %evm.branch.cond1, label %bb._0x1083, label %bb._0xb9, !notdec.evm !587

bb._0x1083:                                       ; preds = %bb._0xad
  call void @evm_revert(ptr %mem, i256 %_0xa7arg0x2, i256 %_0xa7arg0x2), !notdec.evm !588
  unreachable, !notdec.evm !588

bb._0xb9:                                         ; preds = %bb._0xad
  %private.call = call i256 @private__0x6b0_0x6b0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 192), !notdec.evm !589
  br label %bb._0xc0

bb._0xc0:                                         ; preds = %bb._0xb9
  call void @private__0x7bf_0x7bf(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 201), !notdec.evm !590
  br label %bb._0xc9

bb._0xc9:                                         ; preds = %bb._0xc0
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !591
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !592
  %evm.and = and i256 %evm.sub, %private.call, !notdec.evm !593
  %evm.iszero = icmp eq i256 %evm.and, 0, !notdec.evm !594
  %evm.bool2 = zext i1 %evm.iszero to i256, !notdec.evm !594
  %evm.branch.cond3 = icmp ne i256 %evm.bool2, 0, !notdec.evm !595
  br i1 %evm.branch.cond3, label %bb._0x11f, label %bb._0xdc, !notdec.evm !595

bb._0x11f:                                        ; preds = %bb._0xc9
  %notdec.evm.mem.ptr.103 = inttoptr i256 %_0xa7arg0x1 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.103, align 1, !notdec.evm !596
  %evm.shl4 = call i256 @evm_shl(i256 224, i256 508542455), !notdec.evm !597
  %notdec.evm.mem.ptr.104 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.shl4, ptr %notdec.evm.mem.ptr.104, align 1, !notdec.evm !598
  %evm.add5 = add i256 %evm.mload, %_0xa7arg0x0, !notdec.evm !599
  %notdec.evm.mem.ptr.105 = inttoptr i256 %evm.add5 to ptr
  store i256 %_0xa7arg0x2, ptr %notdec.evm.mem.ptr.105, align 1, !notdec.evm !600
  call void @evm_revert(ptr %mem, i256 %evm.mload, i256 36), !notdec.evm !601
  unreachable, !notdec.evm !601

bb._0xdc:                                         ; preds = %bb._0xc9
  %evm.sload = call i256 @evm_sload(i256 5), !notdec.evm !602
  %evm.shl6 = call i256 @evm_shl(i256 160, i256 79228162514264337593543950335), !notdec.evm !603
  %evm.and7 = and i256 %evm.sload, %evm.shl6, !notdec.evm !604
  %evm.or = or i256 %evm.and7, %evm.and, !notdec.evm !605
  call void @evm_sstore(i256 5, i256 %evm.or), !notdec.evm !606
  %evm.and8 = and i256 %evm.sload, %evm.sub, !notdec.evm !607
  call void @evm_log3(ptr %mem, i256 %_0xa7arg0x2, i256 %_0xa7arg0x2, i256 -52524777015005588113349992707137368332388074750292320778649315033004770043936, i256 %evm.and8, i256 %evm.and), !notdec.evm !608
  call void @evm_return(ptr %mem, i256 %_0xa7arg0x2, i256 %_0xa7arg0x2), !notdec.evm !609
  ret void, !notdec.evm !609
}

!0 = !{!"tac=0x6", !"op=MSTORE", !"evm.pc=0x6"}
!1 = !{!"tac=0xa", !"op=CALLDATASIZE", !"evm.pc=0xa"}
!2 = !{!"tac=0xb", !"op=LT", !"evm.pc=0xb"}
!3 = !{!"tac=0xc", !"op=ISZERO", !"evm.pc=0xc"}
!4 = !{!"tac=0x10", !"op=JUMPI", !"evm.pc=0x10"}
!5 = !{!"tac=0x1a", !"op=CALLDATALOAD", !"evm.pc=0x1a"}
!6 = !{!"tac=0x1d", !"op=SHR", !"evm.pc=0x1d"}
!7 = !{!"tac=0x25", !"op=EQ", !"evm.pc=0x25"}
!8 = !{!"tac=0xf750", !"op=JUMPI", !"evm.pc=0x26"}
!9 = !{!"tac=0x1dd70", !"op=CALLPRIVATE", !"evm.pc=0x570"}
!10 = !{!"tac=0x31", !"op=EQ", !"evm.pc=0x31"}
!11 = !{!"tac=0x10150", !"op=JUMPI", !"evm.pc=0x32"}
!12 = !{!"tac=0x1d370", !"op=CALLPRIVATE", !"evm.pc=0x4c7"}
!13 = !{!"tac=0x3c", !"op=EQ", !"evm.pc=0x3c"}
!14 = !{!"tac=0x10b50", !"op=JUMPI", !"evm.pc=0x3d"}
!15 = !{!"tac=0x1c970", !"op=CALLPRIVATE", !"evm.pc=0x4a8"}
!16 = !{!"tac=0x47", !"op=EQ", !"evm.pc=0x47"}
!17 = !{!"tac=0x11550", !"op=JUMPI", !"evm.pc=0x48"}
!18 = !{!"tac=0x1bf70", !"op=CALLPRIVATE", !"evm.pc=0x3b5"}
!19 = !{!"tac=0x52", !"op=EQ", !"evm.pc=0x52"}
!20 = !{!"tac=0x11f50", !"op=JUMPI", !"evm.pc=0x53"}
!21 = !{!"tac=0x1b570", !"op=CALLPRIVATE", !"evm.pc=0x399"}
!22 = !{!"tac=0x5d", !"op=EQ", !"evm.pc=0x5d"}
!23 = !{!"tac=0x12950", !"op=JUMPI", !"evm.pc=0x5e"}
!24 = !{!"tac=0x1ab70", !"op=CALLPRIVATE", !"evm.pc=0x362"}
!25 = !{!"tac=0x68", !"op=EQ", !"evm.pc=0x68"}
!26 = !{!"tac=0x13350", !"op=JUMPI", !"evm.pc=0x69"}
!27 = !{!"tac=0x1a170", !"op=CALLPRIVATE", !"evm.pc=0x302"}
!28 = !{!"tac=0x73", !"op=EQ", !"evm.pc=0x73"}
!29 = !{!"tac=0x13d50", !"op=JUMPI", !"evm.pc=0x74"}
!30 = !{!"tac=0x19770", !"op=CALLPRIVATE", !"evm.pc=0x2d9"}
!31 = !{!"tac=0x7e", !"op=EQ", !"evm.pc=0x7e"}
!32 = !{!"tac=0x14750", !"op=JUMPI", !"evm.pc=0x7f"}
!33 = !{!"tac=0x18d70", !"op=CALLPRIVATE", !"evm.pc=0x1b7"}
!34 = !{!"tac=0x89", !"op=EQ", !"evm.pc=0x89"}
!35 = !{!"tac=0x15150", !"op=JUMPI", !"evm.pc=0x8a"}
!36 = !{!"tac=0x18370", !"op=CALLPRIVATE", !"evm.pc=0x186"}
!37 = !{!"tac=0x94", !"op=EQ", !"evm.pc=0x94"}
!38 = !{!"tac=0x15b50", !"op=JUMPI", !"evm.pc=0x95"}
!39 = !{!"tac=0x17970", !"op=CALLPRIVATE", !"evm.pc=0x139"}
!40 = !{!"tac=0x9e", !"op=EQ", !"evm.pc=0x9e"}
!41 = !{!"tac=0x16550", !"op=JUMPI", !"evm.pc=0x9f"}
!42 = !{!"tac=0x16f70", !"op=CALLPRIVATE", !"evm.pc=0xa7"}
!43 = !{!"tac=0xa6", !"op=REVERT", !"evm.pc=0xa6"}
!44 = !{!"tac=0x14", !"op=REVERT", !"evm.pc=0x14"}
!45 = !{!"tac=0x13c", !"op=CALLVALUE", !"evm.pc=0x13c"}
!46 = !{!"tac=0x140", !"op=JUMPI", !"evm.pc=0x140"}
!47 = !{!"tac=0x10a9", !"op=REVERT", !"evm.pc=0x185"}
!48 = !{!"tac=0x145", !"op=CALLDATASIZE", !"evm.pc=0x145"}
!49 = !{!"tac=0x146", !"op=ADD", !"evm.pc=0x146"}
!50 = !{!"tac=0x147", !"op=SLT", !"evm.pc=0x147"}
!51 = !{!"tac=0x14b", !"op=JUMPI", !"evm.pc=0x14b"}
!52 = !{!"tac=0x10cc", !"op=REVERT", !"evm.pc=0x185"}
!53 = !{!"tac=0x156", !"op=CALLPRIVATE", !"evm.pc=0x156"}
!54 = !{!"tac=0x15e", !"op=CALLPRIVATE", !"evm.pc=0x15e"}
!55 = !{!"tac=0x166", !"op=SHL", !"evm.pc=0x166"}
!56 = !{!"tac=0x167", !"op=SUB", !"evm.pc=0x167"}
!57 = !{!"tac=0x16a", !"op=AND", !"evm.pc=0x16a"}
!58 = !{!"tac=0x16c", !"op=MSTORE", !"evm.pc=0x16c"}
!59 = !{!"tac=0x170", !"op=MSTORE", !"evm.pc=0x170"}
!60 = !{!"tac=0x173", !"op=SHA3", !"evm.pc=0x173"}
!61 = !{!"tac=0x175", !"op=AND", !"evm.pc=0x175"}
!62 = !{!"tac=0x177", !"op=MSTORE", !"evm.pc=0x177"}
!63 = !{!"tac=0x179", !"op=MSTORE", !"evm.pc=0x179"}
!64 = !{!"tac=0x17a", !"op=SHA3", !"evm.pc=0x17a"}
!65 = !{!"tac=0x17b", !"op=SLOAD", !"evm.pc=0x17b"}
!66 = !{!"tac=0x17d", !"op=MLOAD", !"evm.pc=0x17d"}
!67 = !{!"tac=0x180", !"op=MSTORE", !"evm.pc=0x180"}
!68 = !{!"tac=0x181", !"op=RETURN", !"evm.pc=0x181"}
!69 = !{!"tac=0x189", !"op=CALLVALUE", !"evm.pc=0x189"}
!70 = !{!"tac=0x18d", !"op=JUMPI", !"evm.pc=0x18d"}
!71 = !{!"tac=0x10ef", !"op=REVERT", !"evm.pc=0x185"}
!72 = !{!"tac=0x192", !"op=CALLDATASIZE", !"evm.pc=0x192"}
!73 = !{!"tac=0x193", !"op=ADD", !"evm.pc=0x193"}
!74 = !{!"tac=0x194", !"op=SLT", !"evm.pc=0x194"}
!75 = !{!"tac=0x198", !"op=JUMPI", !"evm.pc=0x198"}
!76 = !{!"tac=0x1112", !"op=REVERT", !"evm.pc=0x185"}
!77 = !{!"tac=0x1a5", !"op=CALLPRIVATE", !"evm.pc=0x1a5"}
!78 = !{!"tac=0x1a9", !"op=CALLDATALOAD", !"evm.pc=0x1a9"}
!79 = !{!"tac=0x1ab", !"op=CALLER", !"evm.pc=0x1ab"}
!80 = !{!"tac=0x1af", !"op=CALLPRIVATE", !"evm.pc=0x1af"}
!81 = !{!"tac=0x7af1", !"op=MLOAD", !"evm.pc=0x1b1"}
!82 = !{!"tac=0x7af5", !"op=MSTORE", !"evm.pc=0x1b5"}
!83 = !{!"tac=0x7af6", !"op=RETURN", !"evm.pc=0x1b6"}
!84 = !{!"tac=0x1bb", !"op=CALLVALUE", !"evm.pc=0x1bb"}
!85 = !{!"tac=0x1bf", !"op=JUMPI", !"evm.pc=0x1bf"}
!86 = !{!"tac=0x1135", !"op=REVERT", !"evm.pc=0x185"}
!87 = !{!"tac=0x1c4", !"op=CALLDATASIZE", !"evm.pc=0x1c4"}
!88 = !{!"tac=0x1c5", !"op=ADD", !"evm.pc=0x1c5"}
!89 = !{!"tac=0x1c6", !"op=SLT", !"evm.pc=0x1c6"}
!90 = !{!"tac=0x1ca", !"op=JUMPI", !"evm.pc=0x1ca"}
!91 = !{!"tac=0x1158", !"op=REVERT", !"evm.pc=0x185"}
!92 = !{!"tac=0x1cc", !"op=MLOAD", !"evm.pc=0x1cc"}
!93 = !{!"tac=0x1d0", !"op=SLOAD", !"evm.pc=0x1d0"}
!94 = !{!"tac=0x1d6", !"op=SHR", !"evm.pc=0x1d6"}
!95 = !{!"tac=0x1db", !"op=AND", !"evm.pc=0x1db"}
!96 = !{!"tac=0x1de", !"op=ISZERO", !"evm.pc=0x1de"}
!97 = !{!"tac=0x1e2", !"op=JUMPI", !"evm.pc=0x1e2"}
!98 = !{!"tac=0x2d3", !"op=AND", !"evm.pc=0x2d3"}
!99 = !{!"tac=0x2d8", !"op=JUMP", !"evm.pc=0x2d8"}
!100 = !{!"tac=0x1e3_0x2", !"op=PHI"}
!101 = !{!"tac=0x1e9", !"op=LT", !"evm.pc=0x1e9"}
!102 = !{!"tac=0x1eb", !"op=EQ", !"evm.pc=0x1eb"}
!103 = !{!"tac=0x1ef", !"op=JUMPI", !"evm.pc=0x1ef"}
!104 = !{!"tac=0x1178_0x3", !"op=PHI"}
!105 = !{!"tac=0x1180", !"op=SHL", !"evm.pc=0x2c4"}
!106 = !{!"tac=0x1182", !"op=MSTORE", !"evm.pc=0x2c6"}
!107 = !{!"tac=0x1186", !"op=MSTORE", !"evm.pc=0x2ca"}
!108 = !{!"tac=0x118a", !"op=REVERT", !"evm.pc=0x2ce"}
!109 = !{!"tac=0x1f0_0x3", !"op=PHI"}
!110 = !{!"tac=0x1f2", !"op=MSTORE", !"evm.pc=0x1f2"}
!111 = !{!"tac=0x1f5", !"op=ISZERO", !"evm.pc=0x1f5"}
!112 = !{!"tac=0x1f9", !"op=JUMPI", !"evm.pc=0x1f9"}
!113 = !{!"tac=0x2a0_0x3", !"op=PHI"}
!114 = !{!"tac=0x2a4", !"op=AND", !"evm.pc=0x2a4"}
!115 = !{!"tac=0x2a7", !"op=ADD", !"evm.pc=0x2a7"}
!116 = !{!"tac=0x2a8", !"op=MSTORE", !"evm.pc=0x2a8"}
!117 = !{!"tac=0x2ab", !"op=ISZERO", !"evm.pc=0x2ab"}
!118 = !{!"tac=0x2ac", !"op=ISZERO", !"evm.pc=0x2ac"}
!119 = !{!"tac=0x2af", !"op=SHL", !"evm.pc=0x2af"}
!120 = !{!"tac=0x2b1", !"op=ADD", !"evm.pc=0x2b1"}
!121 = !{!"tac=0x2b2", !"op=ADD", !"evm.pc=0x2b2"}
!122 = !{!"tac=0x2b5", !"op=CODESIZE", !"evm.pc=0x2b5"}
!123 = !{!"tac=0x2bb", !"op=JUMP", !"evm.pc=0x2bb"}
!124 = !{!"tac=0x1fa_0x3", !"op=PHI"}
!125 = !{!"tac=0x1fd", !"op=EQ", !"evm.pc=0x1fd"}
!126 = !{!"tac=0x201", !"op=JUMPI", !"evm.pc=0x201"}
!127 = !{!"tac=0x24b_0x1", !"op=PHI"}
!128 = !{!"tac=0x24e", !"op=MSTORE", !"evm.pc=0x24e"}
!129 = !{!"tac=0x2968", !"op=JUMP", !"evm.pc=0x276"}
!130 = !{!"tac=0x276_0x0", !"op=PHI"}
!131 = !{!"tac=0x276_0x3", !"op=PHI"}
!132 = !{!"tac=0x276_0x4", !"op=PHI"}
!133 = !{!"tac=0x279", !"op=LT", !"evm.pc=0x279"}
!134 = !{!"tac=0x27d", !"op=JUMPI", !"evm.pc=0x27d"}
!135 = !{!"tac=0x28c_0x0", !"op=PHI"}
!136 = !{!"tac=0x28c_0x3", !"op=PHI"}
!137 = !{!"tac=0x28c_0x4", !"op=PHI"}
!138 = !{!"tac=0x28e", !"op=SLOAD", !"evm.pc=0x28e"}
!139 = !{!"tac=0x291", !"op=ADD", !"evm.pc=0x291"}
!140 = !{!"tac=0x293", !"op=ADD", !"evm.pc=0x293"}
!141 = !{!"tac=0x294", !"op=MSTORE", !"evm.pc=0x294"}
!142 = !{!"tac=0x296", !"op=ADD", !"evm.pc=0x296"}
!143 = !{!"tac=0x29b", !"op=ADD", !"evm.pc=0x29b"}
!144 = !{!"tac=0x29f", !"op=JUMP", !"evm.pc=0x29f"}
!145 = !{!"tac=0x27e_0x0", !"op=PHI"}
!146 = !{!"tac=0x27e_0x3", !"op=PHI"}
!147 = !{!"tac=0x27e_0x4", !"op=PHI"}
!148 = !{!"tac=0x283", !"op=ADD", !"evm.pc=0x283"}
!149 = !{!"tac=0x284", !"op=ADD", !"evm.pc=0x284"}
!150 = !{!"tac=0x285", !"op=CODESIZE", !"evm.pc=0x285"}
!151 = !{!"tac=0x28b", !"op=JUMP", !"evm.pc=0x28b"}
!152 = !{!"tac=0x2020x1b7_0x0", !"op=PHI"}
!153 = !{!"tac=0x2020x1b7_0x1", !"op=PHI"}
!154 = !{!"tac=0x2020x1b7_0x2", !"op=PHI"}
!155 = !{!"tac=0x2020x1b7_0x3", !"op=PHI"}
!156 = !{!"tac=0x2080x1b7", !"op=SUB", !"evm.pc=0x208"}
!157 = !{!"tac=0x20b0x1b7", !"op=ADD", !"evm.pc=0x20b"}
!158 = !{!"tac=0x20f0x1b7", !"op=AND", !"evm.pc=0x20f"}
!159 = !{!"tac=0x2110x1b7", !"op=ADD", !"evm.pc=0x211"}
!160 = !{!"tac=0x21d0x1b7", !"op=GT", !"evm.pc=0x21d"}
!161 = !{!"tac=0x2200x1b7", !"op=LT", !"evm.pc=0x220"}
!162 = !{!"tac=0x2210x1b7", !"op=OR", !"evm.pc=0x221"}
!163 = !{!"tac=0x2250x1b7", !"op=JUMPI", !"evm.pc=0x225"}
!164 = !{!"tac=0x11b20x1b7", !"op=SHL", !"evm.pc=0x240"}
!165 = !{!"tac=0x11b40x1b7", !"op=MSTORE", !"evm.pc=0x242"}
!166 = !{!"tac=0x11b80x1b7", !"op=MSTORE", !"evm.pc=0x246"}
!167 = !{!"tac=0x11bc0x1b7", !"op=REVERT", !"evm.pc=0x24a"}
!168 = !{!"tac=0x22e0x1b7", !"op=MSTORE", !"evm.pc=0x22e"}
!169 = !{!"tac=0x2330x1b7", !"op=CALLPRIVATE", !"evm.pc=0x233"}
!170 = !{!"tac=0x7b170x1b7", !"op=SUB", !"evm.pc=0x235"}
!171 = !{!"tac=0x7b190x1b7", !"op=RETURN", !"evm.pc=0x237"}
!172 = !{!"tac=0x2dc", !"op=CALLVALUE", !"evm.pc=0x2dc"}
!173 = !{!"tac=0x2e0", !"op=JUMPI", !"evm.pc=0x2e0"}
!174 = !{!"tac=0x11df", !"op=REVERT", !"evm.pc=0x185"}
!175 = !{!"tac=0x2e5", !"op=CALLDATASIZE", !"evm.pc=0x2e5"}
!176 = !{!"tac=0x2e6", !"op=ADD", !"evm.pc=0x2e6"}
!177 = !{!"tac=0x2e7", !"op=SLT", !"evm.pc=0x2e7"}
!178 = !{!"tac=0x2eb", !"op=JUMPI", !"evm.pc=0x2eb"}
!179 = !{!"tac=0x1202", !"op=REVERT", !"evm.pc=0x185"}
!180 = !{!"tac=0x2ee", !"op=SLOAD", !"evm.pc=0x2ee"}
!181 = !{!"tac=0x2f0", !"op=MLOAD", !"evm.pc=0x2f0"}
!182 = !{!"tac=0x2f7", !"op=SHL", !"evm.pc=0x2f7"}
!183 = !{!"tac=0x2f8", !"op=SUB", !"evm.pc=0x2f8"}
!184 = !{!"tac=0x2fb", !"op=AND", !"evm.pc=0x2fb"}
!185 = !{!"tac=0x2fd", !"op=MSTORE", !"evm.pc=0x2fd"}
!186 = !{!"tac=0x301", !"op=RETURN", !"evm.pc=0x301"}
!187 = !{!"tac=0x304", !"op=CALLVALUE", !"evm.pc=0x304"}
!188 = !{!"tac=0x308", !"op=JUMPI", !"evm.pc=0x308"}
!189 = !{!"tac=0x1224", !"op=REVERT", !"evm.pc=0x361"}
!190 = !{!"tac=0x30d", !"op=CALLDATASIZE", !"evm.pc=0x30d"}
!191 = !{!"tac=0x30e", !"op=ADD", !"evm.pc=0x30e"}
!192 = !{!"tac=0x30f", !"op=SLT", !"evm.pc=0x30f"}
!193 = !{!"tac=0x313", !"op=JUMPI", !"evm.pc=0x313"}
!194 = !{!"tac=0x1246", !"op=REVERT", !"evm.pc=0x361"}
!195 = !{!"tac=0x31a", !"op=CALLPRIVATE", !"evm.pc=0x31a"}
!196 = !{!"tac=0x31f", !"op=SLOAD", !"evm.pc=0x31f"}
!197 = !{!"tac=0x326", !"op=SHL", !"evm.pc=0x326"}
!198 = !{!"tac=0x327", !"op=SUB", !"evm.pc=0x327"}
!199 = !{!"tac=0x328", !"op=NOT", !"evm.pc=0x328"}
!200 = !{!"tac=0x32a", !"op=AND", !"evm.pc=0x32a"}
!201 = !{!"tac=0x32d", !"op=SSTORE", !"evm.pc=0x32d"}
!202 = !{!"tac=0x336", !"op=SHL", !"evm.pc=0x336"}
!203 = !{!"tac=0x337", !"op=SUB", !"evm.pc=0x337"}
!204 = !{!"tac=0x338", !"op=AND", !"evm.pc=0x338"}
!205 = !{!"tac=0x35c", !"op=LOG3", !"evm.pc=0x35c"}
!206 = !{!"tac=0x35e", !"op=RETURN", !"evm.pc=0x35e"}
!207 = !{!"tac=0x365", !"op=CALLVALUE", !"evm.pc=0x365"}
!208 = !{!"tac=0x369", !"op=JUMPI", !"evm.pc=0x369"}
!209 = !{!"tac=0x1269", !"op=REVERT", !"evm.pc=0x185"}
!210 = !{!"tac=0x36c", !"op=CALLDATASIZE", !"evm.pc=0x36c"}
!211 = !{!"tac=0x370", !"op=ADD", !"evm.pc=0x370"}
!212 = !{!"tac=0x371", !"op=SLT", !"evm.pc=0x371"}
!213 = !{!"tac=0x375", !"op=JUMPI", !"evm.pc=0x375"}
!214 = !{!"tac=0x128c", !"op=REVERT", !"evm.pc=0x185"}
!215 = !{!"tac=0x381", !"op=SHL", !"evm.pc=0x381"}
!216 = !{!"tac=0x382", !"op=SUB", !"evm.pc=0x382"}
!217 = !{!"tac=0x389", !"op=CALLPRIVATE", !"evm.pc=0x389"}
!218 = !{!"tac=0x38b", !"op=AND", !"evm.pc=0x38b"}
!219 = !{!"tac=0x38d", !"op=MSTORE", !"evm.pc=0x38d"}
!220 = !{!"tac=0x390", !"op=MSTORE", !"evm.pc=0x390"}
!221 = !{!"tac=0x391", !"op=SHA3", !"evm.pc=0x391"}
!222 = !{!"tac=0x392", !"op=SLOAD", !"evm.pc=0x392"}
!223 = !{!"tac=0x394", !"op=MLOAD", !"evm.pc=0x394"}
!224 = !{!"tac=0x397", !"op=MSTORE", !"evm.pc=0x397"}
!225 = !{!"tac=0x398", !"op=RETURN", !"evm.pc=0x398"}
!226 = !{!"tac=0x39c", !"op=CALLVALUE", !"evm.pc=0x39c"}
!227 = !{!"tac=0x3a0", !"op=JUMPI", !"evm.pc=0x3a0"}
!228 = !{!"tac=0x12af", !"op=REVERT", !"evm.pc=0x185"}
!229 = !{!"tac=0x3a5", !"op=CALLDATASIZE", !"evm.pc=0x3a5"}
!230 = !{!"tac=0x3a6", !"op=ADD", !"evm.pc=0x3a6"}
!231 = !{!"tac=0x3a7", !"op=SLT", !"evm.pc=0x3a7"}
!232 = !{!"tac=0x3ab", !"op=JUMPI", !"evm.pc=0x3ab"}
!233 = !{!"tac=0x12d2", !"op=REVERT", !"evm.pc=0x185"}
!234 = !{!"tac=0x3af", !"op=MLOAD", !"evm.pc=0x3af"}
!235 = !{!"tac=0x3b3", !"op=MSTORE", !"evm.pc=0x3b3"}
!236 = !{!"tac=0x3b4", !"op=RETURN", !"evm.pc=0x3b4"}
!237 = !{!"tac=0x3b8", !"op=CALLVALUE", !"evm.pc=0x3b8"}
!238 = !{!"tac=0x3bc", !"op=JUMPI", !"evm.pc=0x3bc"}
!239 = !{!"tac=0x12f4", !"op=REVERT", !"evm.pc=0x361"}
!240 = !{!"tac=0x3bf", !"op=CALLDATASIZE", !"evm.pc=0x3bf"}
!241 = !{!"tac=0x3c3", !"op=ADD", !"evm.pc=0x3c3"}
!242 = !{!"tac=0x3c4", !"op=SLT", !"evm.pc=0x3c4"}
!243 = !{!"tac=0x3c8", !"op=JUMPI", !"evm.pc=0x3c8"}
!244 = !{!"tac=0x1316", !"op=REVERT", !"evm.pc=0x361"}
!245 = !{!"tac=0x3cf", !"op=CALLPRIVATE", !"evm.pc=0x3cf"}
!246 = !{!"tac=0x3d7", !"op=CALLPRIVATE", !"evm.pc=0x3d7"}
!247 = !{!"tac=0x3dc", !"op=CALLDATALOAD", !"evm.pc=0x3dc"}
!248 = !{!"tac=0x3e3", !"op=SHL", !"evm.pc=0x3e3"}
!249 = !{!"tac=0x3e4", !"op=SUB", !"evm.pc=0x3e4"}
!250 = !{!"tac=0x3e6", !"op=AND", !"evm.pc=0x3e6"}
!251 = !{!"tac=0x3e9", !"op=MSTORE", !"evm.pc=0x3e9"}
!252 = !{!"tac=0x3ee", !"op=MSTORE", !"evm.pc=0x3ee"}
!253 = !{!"tac=0x3f1", !"op=SHA3", !"evm.pc=0x3f1"}
!254 = !{!"tac=0x3f2", !"op=CALLER", !"evm.pc=0x3f2"}
!255 = !{!"tac=0x3f4", !"op=MSTORE", !"evm.pc=0x3f4"}
!256 = !{!"tac=0x3f7", !"op=MSTORE", !"evm.pc=0x3f7"}
!257 = !{!"tac=0x3fa", !"op=SHA3", !"evm.pc=0x3fa"}
!258 = !{!"tac=0x3fb", !"op=SLOAD", !"evm.pc=0x3fb"}
!259 = !{!"tac=0x401", !"op=SUB", !"evm.pc=0x401"}
!260 = !{!"tac=0x405", !"op=JUMPI", !"evm.pc=0x405"}
!261 = !{!"tac=0x417", !"op=LT", !"evm.pc=0x417"}
!262 = !{!"tac=0x41b", !"op=JUMPI", !"evm.pc=0x41b"}
!263 = !{!"tac=0x47e", !"op=MLOAD", !"evm.pc=0x47e"}
!264 = !{!"tac=0x486", !"op=SHL", !"evm.pc=0x486"}
!265 = !{!"tac=0x488", !"op=MSTORE", !"evm.pc=0x488"}
!266 = !{!"tac=0x489", !"op=CALLER", !"evm.pc=0x489"}
!267 = !{!"tac=0x48c", !"op=ADD", !"evm.pc=0x48c"}
!268 = !{!"tac=0x48f", !"op=MSTORE", !"evm.pc=0x48f"}
!269 = !{!"tac=0x493", !"op=ADD", !"evm.pc=0x493"}
!270 = !{!"tac=0x497", !"op=MSTORE", !"evm.pc=0x497"}
!271 = !{!"tac=0x49b", !"op=ADD", !"evm.pc=0x49b"}
!272 = !{!"tac=0x49e", !"op=MSTORE", !"evm.pc=0x49e"}
!273 = !{!"tac=0x4a4", !"op=ADD", !"evm.pc=0x4a4"}
!274 = !{!"tac=0x4a5", !"op=SUB", !"evm.pc=0x4a5"}
!275 = !{!"tac=0x4a7", !"op=REVERT", !"evm.pc=0x4a7"}
!276 = !{!"tac=0x41d", !"op=ISZERO", !"evm.pc=0x41d"}
!277 = !{!"tac=0x421", !"op=JUMPI", !"evm.pc=0x421"}
!278 = !{!"tac=0x467", !"op=MLOAD", !"evm.pc=0x467"}
!279 = !{!"tac=0x46f", !"op=SHL", !"evm.pc=0x46f"}
!280 = !{!"tac=0x471", !"op=MSTORE", !"evm.pc=0x471"}
!281 = !{!"tac=0x474", !"op=ADD", !"evm.pc=0x474"}
!282 = !{!"tac=0x477", !"op=MSTORE", !"evm.pc=0x477"}
!283 = !{!"tac=0x47b", !"op=REVERT", !"evm.pc=0x47b"}
!284 = !{!"tac=0x422", !"op=CALLER", !"evm.pc=0x422"}
!285 = !{!"tac=0x423", !"op=ISZERO", !"evm.pc=0x423"}
!286 = !{!"tac=0x427", !"op=JUMPI", !"evm.pc=0x427"}
!287 = !{!"tac=0x450", !"op=MLOAD", !"evm.pc=0x450"}
!288 = !{!"tac=0x458", !"op=SHL", !"evm.pc=0x458"}
!289 = !{!"tac=0x45a", !"op=MSTORE", !"evm.pc=0x45a"}
!290 = !{!"tac=0x45d", !"op=ADD", !"evm.pc=0x45d"}
!291 = !{!"tac=0x460", !"op=MSTORE", !"evm.pc=0x460"}
!292 = !{!"tac=0x464", !"op=REVERT", !"evm.pc=0x464"}
!293 = !{!"tac=0x42a", !"op=MSTORE", !"evm.pc=0x42a"}
!294 = !{!"tac=0x431", !"op=MSTORE", !"evm.pc=0x431"}
!295 = !{!"tac=0x434", !"op=SHA3", !"evm.pc=0x434"}
!296 = !{!"tac=0x435", !"op=CALLER", !"evm.pc=0x435"}
!297 = !{!"tac=0x437", !"op=MSTORE", !"evm.pc=0x437"}
!298 = !{!"tac=0x439", !"op=MSTORE", !"evm.pc=0x439"}
!299 = !{!"tac=0x43d", !"op=SHA3", !"evm.pc=0x43d"}
!300 = !{!"tac=0x441", !"op=SUB", !"evm.pc=0x441"}
!301 = !{!"tac=0x443", !"op=SSTORE", !"evm.pc=0x443"}
!302 = !{!"tac=0x44d", !"op=JUMP", !"evm.pc=0x44d"}
!303 = !{!"tac=0x406_0x0", !"op=PHI"}
!304 = !{!"tac=0x406_0x1", !"op=PHI"}
!305 = !{!"tac=0x406_0x2", !"op=PHI"}
!306 = !{!"tac=0x406_0x3", !"op=PHI"}
!307 = !{!"tac=0x413", !"op=CALLPRIVATE", !"evm.pc=0x413"}
!308 = !{!"tac=0x7b39_0x2", !"op=PHI"}
!309 = !{!"tac=0x7b39_0x3", !"op=PHI"}
!310 = !{!"tac=0x7b39_0x4", !"op=PHI"}
!311 = !{!"tac=0x7b39_0x5", !"op=PHI"}
!312 = !{!"tac=0x7b3a", !"op=MLOAD", !"evm.pc=0x1b1"}
!313 = !{!"tac=0x7b3e", !"op=MSTORE", !"evm.pc=0x1b5"}
!314 = !{!"tac=0x7b3f", !"op=RETURN", !"evm.pc=0x1b6"}
!315 = !{!"tac=0x4ab", !"op=CALLVALUE", !"evm.pc=0x4ab"}
!316 = !{!"tac=0x4af", !"op=JUMPI", !"evm.pc=0x4af"}
!317 = !{!"tac=0x1339", !"op=REVERT", !"evm.pc=0x185"}
!318 = !{!"tac=0x4b4", !"op=CALLDATASIZE", !"evm.pc=0x4b4"}
!319 = !{!"tac=0x4b5", !"op=ADD", !"evm.pc=0x4b5"}
!320 = !{!"tac=0x4b6", !"op=SLT", !"evm.pc=0x4b6"}
!321 = !{!"tac=0x4ba", !"op=JUMPI", !"evm.pc=0x4ba"}
!322 = !{!"tac=0x135c", !"op=REVERT", !"evm.pc=0x185"}
!323 = !{!"tac=0x4c0", !"op=SLOAD", !"evm.pc=0x4c0"}
!324 = !{!"tac=0x4c2", !"op=MLOAD", !"evm.pc=0x4c2"}
!325 = !{!"tac=0x4c5", !"op=MSTORE", !"evm.pc=0x4c5"}
!326 = !{!"tac=0x4c6", !"op=RETURN", !"evm.pc=0x4c6"}
!327 = !{!"tac=0x4c9", !"op=CALLVALUE", !"evm.pc=0x4c9"}
!328 = !{!"tac=0x4cd", !"op=JUMPI", !"evm.pc=0x4cd"}
!329 = !{!"tac=0x137f", !"op=REVERT", !"evm.pc=0x138"}
!330 = !{!"tac=0x4d2", !"op=CALLDATASIZE", !"evm.pc=0x4d2"}
!331 = !{!"tac=0x4d3", !"op=ADD", !"evm.pc=0x4d3"}
!332 = !{!"tac=0x4d4", !"op=SLT", !"evm.pc=0x4d4"}
!333 = !{!"tac=0x4d8", !"op=JUMPI", !"evm.pc=0x4d8"}
!334 = !{!"tac=0x13a2", !"op=REVERT", !"evm.pc=0x138"}
!335 = !{!"tac=0x4df", !"op=CALLPRIVATE", !"evm.pc=0x4df"}
!336 = !{!"tac=0x4e3", !"op=CALLDATALOAD", !"evm.pc=0x4e3"}
!337 = !{!"tac=0x4e5", !"op=CALLER", !"evm.pc=0x4e5"}
!338 = !{!"tac=0x4e6", !"op=ISZERO", !"evm.pc=0x4e6"}
!339 = !{!"tac=0x4ea", !"op=JUMPI", !"evm.pc=0x4ea"}
!340 = !{!"tac=0x55b", !"op=MLOAD", !"evm.pc=0x55b"}
!341 = !{!"tac=0x563", !"op=SHL", !"evm.pc=0x563"}
!342 = !{!"tac=0x565", !"op=MSTORE", !"evm.pc=0x565"}
!343 = !{!"tac=0x568", !"op=ADD", !"evm.pc=0x568"}
!344 = !{!"tac=0x56b", !"op=MSTORE", !"evm.pc=0x56b"}
!345 = !{!"tac=0x56f", !"op=REVERT", !"evm.pc=0x56f"}
!346 = !{!"tac=0x4f1", !"op=SHL", !"evm.pc=0x4f1"}
!347 = !{!"tac=0x4f2", !"op=SUB", !"evm.pc=0x4f2"}
!348 = !{!"tac=0x4f3", !"op=AND", !"evm.pc=0x4f3"}
!349 = !{!"tac=0x4f6", !"op=ISZERO", !"evm.pc=0x4f6"}
!350 = !{!"tac=0x4fa", !"op=JUMPI", !"evm.pc=0x4fa"}
!351 = !{!"tac=0x544", !"op=MLOAD", !"evm.pc=0x544"}
!352 = !{!"tac=0x54c", !"op=SHL", !"evm.pc=0x54c"}
!353 = !{!"tac=0x54e", !"op=MSTORE", !"evm.pc=0x54e"}
!354 = !{!"tac=0x551", !"op=ADD", !"evm.pc=0x551"}
!355 = !{!"tac=0x554", !"op=MSTORE", !"evm.pc=0x554"}
!356 = !{!"tac=0x558", !"op=REVERT", !"evm.pc=0x558"}
!357 = !{!"tac=0x501", !"op=CALLER", !"evm.pc=0x501"}
!358 = !{!"tac=0x503", !"op=MSTORE", !"evm.pc=0x503"}
!359 = !{!"tac=0x507", !"op=MSTORE", !"evm.pc=0x507"}
!360 = !{!"tac=0x50a", !"op=SHA3", !"evm.pc=0x50a"}
!361 = !{!"tac=0x50d", !"op=MSTORE", !"evm.pc=0x50d"}
!362 = !{!"tac=0x50f", !"op=MSTORE", !"evm.pc=0x50f"}
!363 = !{!"tac=0x510", !"op=SHA3", !"evm.pc=0x510"}
!364 = !{!"tac=0x511", !"op=SSTORE", !"evm.pc=0x511"}
!365 = !{!"tac=0x513", !"op=MLOAD", !"evm.pc=0x513"}
!366 = !{!"tac=0x516", !"op=MSTORE", !"evm.pc=0x516"}
!367 = !{!"tac=0x539", !"op=CALLER", !"evm.pc=0x539"}
!368 = !{!"tac=0x53b", !"op=LOG3", !"evm.pc=0x53b"}
!369 = !{!"tac=0x53c", !"op=MLOAD", !"evm.pc=0x53c"}
!370 = !{!"tac=0x540", !"op=MSTORE", !"evm.pc=0x540"}
!371 = !{!"tac=0x541", !"op=RETURN", !"evm.pc=0x541"}
!372 = !{!"tac=0x575", !"op=CALLVALUE", !"evm.pc=0x575"}
!373 = !{!"tac=0x579", !"op=JUMPI", !"evm.pc=0x579"}
!374 = !{!"tac=0x13c5", !"op=REVERT", !"evm.pc=0x138"}
!375 = !{!"tac=0x57e", !"op=CALLDATASIZE", !"evm.pc=0x57e"}
!376 = !{!"tac=0x57f", !"op=ADD", !"evm.pc=0x57f"}
!377 = !{!"tac=0x580", !"op=SLT", !"evm.pc=0x580"}
!378 = !{!"tac=0x584", !"op=JUMPI", !"evm.pc=0x584"}
!379 = !{!"tac=0x13e8", !"op=REVERT", !"evm.pc=0x138"}
!380 = !{!"tac=0x588", !"op=SLOAD", !"evm.pc=0x588"}
!381 = !{!"tac=0x58e", !"op=SHR", !"evm.pc=0x58e"}
!382 = !{!"tac=0x593", !"op=AND", !"evm.pc=0x593"}
!383 = !{!"tac=0x596", !"op=ISZERO", !"evm.pc=0x596"}
!384 = !{!"tac=0x59a", !"op=JUMPI", !"evm.pc=0x59a"}
!385 = !{!"tac=0x661", !"op=AND", !"evm.pc=0x661"}
!386 = !{!"tac=0x666", !"op=JUMP", !"evm.pc=0x666"}
!387 = !{!"tac=0x59b_0x2", !"op=PHI"}
!388 = !{!"tac=0x5a1", !"op=LT", !"evm.pc=0x5a1"}
!389 = !{!"tac=0x5a3", !"op=EQ", !"evm.pc=0x5a3"}
!390 = !{!"tac=0x5a7", !"op=JUMPI", !"evm.pc=0x5a7"}
!391 = !{!"tac=0x1408_0x3", !"op=PHI"}
!392 = !{!"tac=0x1410", !"op=SHL", !"evm.pc=0x2c4"}
!393 = !{!"tac=0x1412", !"op=MSTORE", !"evm.pc=0x2c6"}
!394 = !{!"tac=0x1416", !"op=MSTORE", !"evm.pc=0x2ca"}
!395 = !{!"tac=0x141a", !"op=REVERT", !"evm.pc=0x2ce"}
!396 = !{!"tac=0x5a8_0x3", !"op=PHI"}
!397 = !{!"tac=0x5aa", !"op=MSTORE", !"evm.pc=0x5aa"}
!398 = !{!"tac=0x5ad", !"op=ISZERO", !"evm.pc=0x5ad"}
!399 = !{!"tac=0x5b1", !"op=JUMPI", !"evm.pc=0x5b1"}
!400 = !{!"tac=0x641_0x3", !"op=PHI"}
!401 = !{!"tac=0x645", !"op=AND", !"evm.pc=0x645"}
!402 = !{!"tac=0x648", !"op=ADD", !"evm.pc=0x648"}
!403 = !{!"tac=0x649", !"op=MSTORE", !"evm.pc=0x649"}
!404 = !{!"tac=0x64c", !"op=ISZERO", !"evm.pc=0x64c"}
!405 = !{!"tac=0x64d", !"op=ISZERO", !"evm.pc=0x64d"}
!406 = !{!"tac=0x650", !"op=SHL", !"evm.pc=0x650"}
!407 = !{!"tac=0x652", !"op=ADD", !"evm.pc=0x652"}
!408 = !{!"tac=0x653", !"op=ADD", !"evm.pc=0x653"}
!409 = !{!"tac=0x65c", !"op=JUMP", !"evm.pc=0x65c"}
!410 = !{!"tac=0x5b2_0x3", !"op=PHI"}
!411 = !{!"tac=0x5b5", !"op=EQ", !"evm.pc=0x5b5"}
!412 = !{!"tac=0x5b9", !"op=JUMPI", !"evm.pc=0x5b9"}
!413 = !{!"tac=0x5eb_0x1", !"op=PHI"}
!414 = !{!"tac=0x5ef", !"op=MSTORE", !"evm.pc=0x5ef"}
!415 = !{!"tac=0x3368", !"op=JUMP", !"evm.pc=0x617"}
!416 = !{!"tac=0x617_0x0", !"op=PHI"}
!417 = !{!"tac=0x617_0x3", !"op=PHI"}
!418 = !{!"tac=0x617_0x4", !"op=PHI"}
!419 = !{!"tac=0x61a", !"op=LT", !"evm.pc=0x61a"}
!420 = !{!"tac=0x61e", !"op=JUMPI", !"evm.pc=0x61e"}
!421 = !{!"tac=0x62d_0x0", !"op=PHI"}
!422 = !{!"tac=0x62d_0x3", !"op=PHI"}
!423 = !{!"tac=0x62d_0x4", !"op=PHI"}
!424 = !{!"tac=0x62f", !"op=SLOAD", !"evm.pc=0x62f"}
!425 = !{!"tac=0x632", !"op=ADD", !"evm.pc=0x632"}
!426 = !{!"tac=0x634", !"op=ADD", !"evm.pc=0x634"}
!427 = !{!"tac=0x635", !"op=MSTORE", !"evm.pc=0x635"}
!428 = !{!"tac=0x637", !"op=ADD", !"evm.pc=0x637"}
!429 = !{!"tac=0x63c", !"op=ADD", !"evm.pc=0x63c"}
!430 = !{!"tac=0x640", !"op=JUMP", !"evm.pc=0x640"}
!431 = !{!"tac=0x61f_0x0", !"op=PHI"}
!432 = !{!"tac=0x61f_0x3", !"op=PHI"}
!433 = !{!"tac=0x61f_0x4", !"op=PHI"}
!434 = !{!"tac=0x624", !"op=ADD", !"evm.pc=0x624"}
!435 = !{!"tac=0x625", !"op=ADD", !"evm.pc=0x625"}
!436 = !{!"tac=0x62c", !"op=JUMP", !"evm.pc=0x62c"}
!437 = !{!"tac=0x2020x570_0x3", !"op=PHI"}
!438 = !{!"tac=0x2080x570", !"op=SUB", !"evm.pc=0x208"}
!439 = !{!"tac=0x20b0x570", !"op=ADD", !"evm.pc=0x20b"}
!440 = !{!"tac=0x20f0x570", !"op=AND", !"evm.pc=0x20f"}
!441 = !{!"tac=0x2110x570", !"op=ADD", !"evm.pc=0x211"}
!442 = !{!"tac=0x21d0x570", !"op=GT", !"evm.pc=0x21d"}
!443 = !{!"tac=0x2200x570", !"op=LT", !"evm.pc=0x220"}
!444 = !{!"tac=0x2210x570", !"op=OR", !"evm.pc=0x221"}
!445 = !{!"tac=0x2250x570", !"op=JUMPI", !"evm.pc=0x225"}
!446 = !{!"tac=0x11b20x570", !"op=SHL", !"evm.pc=0x240"}
!447 = !{!"tac=0x11b40x570", !"op=MSTORE", !"evm.pc=0x242"}
!448 = !{!"tac=0x11b80x570", !"op=MSTORE", !"evm.pc=0x246"}
!449 = !{!"tac=0x11bc0x570", !"op=REVERT", !"evm.pc=0x24a"}
!450 = !{!"tac=0x22e0x570", !"op=MSTORE", !"evm.pc=0x22e"}
!451 = !{!"tac=0x2330x570", !"op=CALLPRIVATE", !"evm.pc=0x233"}
!452 = !{!"tac=0x7b170x570", !"op=SUB", !"evm.pc=0x235"}
!453 = !{!"tac=0x7b190x570", !"op=RETURN", !"evm.pc=0x237"}
!454 = !{!"tac=0x5ba_0x1", !"op=PHI"}
!455 = !{!"tac=0x5bf", !"op=SUB", !"evm.pc=0x5bf"}
!456 = !{!"tac=0x5c2", !"op=ADD", !"evm.pc=0x5c2"}
!457 = !{!"tac=0x5c6", !"op=AND", !"evm.pc=0x5c6"}
!458 = !{!"tac=0x5c8", !"op=ADD", !"evm.pc=0x5c8"}
!459 = !{!"tac=0x5d4", !"op=GT", !"evm.pc=0x5d4"}
!460 = !{!"tac=0x5d7", !"op=LT", !"evm.pc=0x5d7"}
!461 = !{!"tac=0x5d8", !"op=OR", !"evm.pc=0x5d8"}
!462 = !{!"tac=0x5dc", !"op=JUMPI", !"evm.pc=0x5dc"}
!463 = !{!"tac=0x1442", !"op=SHL", !"evm.pc=0x240"}
!464 = !{!"tac=0x1444", !"op=MSTORE", !"evm.pc=0x242"}
!465 = !{!"tac=0x1448", !"op=MSTORE", !"evm.pc=0x246"}
!466 = !{!"tac=0x144c", !"op=REVERT", !"evm.pc=0x24a"}
!467 = !{!"tac=0x5e5", !"op=MSTORE", !"evm.pc=0x5e5"}
!468 = !{!"tac=0x5ea", !"op=CALLPRIVATE", !"evm.pc=0x5ea"}
!469 = !{!"tac=0x7b86", !"op=SUB", !"evm.pc=0x235"}
!470 = !{!"tac=0x7b88", !"op=RETURN", !"evm.pc=0x237"}
!471 = !{!"tac=0x66c", !"op=MSTORE", !"evm.pc=0x66c"}
!472 = !{!"tac=0x66e", !"op=MLOAD", !"evm.pc=0x66e"}
!473 = !{!"tac=0x671", !"op=ADD", !"evm.pc=0x671"}
!474 = !{!"tac=0x674", !"op=MSTORE", !"evm.pc=0x674"}
!475 = !{!"tac=0x3d68", !"op=JUMP", !"evm.pc=0x67a"}
!476 = !{!"tac=0x67a_0x0", !"op=PHI"}
!477 = !{!"tac=0x67d", !"op=LT", !"evm.pc=0x67d"}
!478 = !{!"tac=0x681", !"op=JUMPI", !"evm.pc=0x681"}
!479 = !{!"tac=0x69c_0x0", !"op=PHI"}
!480 = !{!"tac=0x69f", !"op=ADD", !"evm.pc=0x69f"}
!481 = !{!"tac=0x6a1", !"op=ADD", !"evm.pc=0x6a1"}
!482 = !{!"tac=0x6a2", !"op=MLOAD", !"evm.pc=0x6a2"}
!483 = !{!"tac=0x6a5", !"op=ADD", !"evm.pc=0x6a5"}
!484 = !{!"tac=0x6a8", !"op=ADD", !"evm.pc=0x6a8"}
!485 = !{!"tac=0x6a9", !"op=MSTORE", !"evm.pc=0x6a9"}
!486 = !{!"tac=0x6ab", !"op=ADD", !"evm.pc=0x6ab"}
!487 = !{!"tac=0x6af", !"op=JUMP", !"evm.pc=0x6af"}
!488 = !{!"tac=0x682_0x0", !"op=PHI"}
!489 = !{!"tac=0x68e", !"op=ADD", !"evm.pc=0x68e"}
!490 = !{!"tac=0x68f", !"op=ADD", !"evm.pc=0x68f"}
!491 = !{!"tac=0x690", !"op=MSTORE", !"evm.pc=0x690"}
!492 = !{!"tac=0x696", !"op=ADD", !"evm.pc=0x696"}
!493 = !{!"tac=0x697", !"op=AND", !"evm.pc=0x697"}
!494 = !{!"tac=0x698", !"op=ADD", !"evm.pc=0x698"}
!495 = !{!"tac=0x699", !"op=ADD", !"evm.pc=0x699"}
!496 = !{!"tac=0x69b", !"op=RETURNPRIVATE", !"evm.pc=0x69b"}
!497 = !{!"tac=0x6b3", !"op=CALLDATALOAD", !"evm.pc=0x6b3"}
!498 = !{!"tac=0x6bb", !"op=SHL", !"evm.pc=0x6bb"}
!499 = !{!"tac=0x6bc", !"op=SUB", !"evm.pc=0x6bc"}
!500 = !{!"tac=0x6be", !"op=AND", !"evm.pc=0x6be"}
!501 = !{!"tac=0x6c0", !"op=SUB", !"evm.pc=0x6c0"}
!502 = !{!"tac=0x6c4", !"op=JUMPI", !"evm.pc=0x6c4"}
!503 = !{!"tac=0x1470", !"op=REVERT", !"evm.pc=0x6ca"}
!504 = !{!"tac=0x6c5", !"op=RETURNPRIVATE", !"evm.pc=0x6c5"}
!505 = !{!"tac=0x6ce", !"op=CALLDATALOAD", !"evm.pc=0x6ce"}
!506 = !{!"tac=0x6d6", !"op=SHL", !"evm.pc=0x6d6"}
!507 = !{!"tac=0x6d7", !"op=SUB", !"evm.pc=0x6d7"}
!508 = !{!"tac=0x6d9", !"op=AND", !"evm.pc=0x6d9"}
!509 = !{!"tac=0x6db", !"op=SUB", !"evm.pc=0x6db"}
!510 = !{!"tac=0x6df", !"op=JUMPI", !"evm.pc=0x6df"}
!511 = !{!"tac=0x1494", !"op=REVERT", !"evm.pc=0x6ca"}
!512 = !{!"tac=0x6e0", !"op=RETURNPRIVATE", !"evm.pc=0x6e0"}
!513 = !{!"tac=0x6e9", !"op=SHL", !"evm.pc=0x6e9"}
!514 = !{!"tac=0x6ea", !"op=SUB", !"evm.pc=0x6ea"}
!515 = !{!"tac=0x6ed", !"op=AND", !"evm.pc=0x6ed"}
!516 = !{!"tac=0x6f0", !"op=ISZERO", !"evm.pc=0x6f0"}
!517 = !{!"tac=0x6f4", !"op=JUMPI", !"evm.pc=0x6f4"}
!518 = !{!"tac=0x7a9", !"op=MLOAD", !"evm.pc=0x7a9"}
!519 = !{!"tac=0x7b1", !"op=SHL", !"evm.pc=0x7b1"}
!520 = !{!"tac=0x7b3", !"op=MSTORE", !"evm.pc=0x7b3"}
!521 = !{!"tac=0x7b9", !"op=ADD", !"evm.pc=0x7b9"}
!522 = !{!"tac=0x7ba", !"op=MSTORE", !"evm.pc=0x7ba"}
!523 = !{!"tac=0x7be", !"op=REVERT", !"evm.pc=0x7be"}
!524 = !{!"tac=0x6f5", !"op=AND", !"evm.pc=0x6f5"}
!525 = !{!"tac=0x6f8", !"op=ISZERO", !"evm.pc=0x6f8"}
!526 = !{!"tac=0x6fc", !"op=JUMPI", !"evm.pc=0x6fc"}
!527 = !{!"tac=0x790", !"op=MLOAD", !"evm.pc=0x790"}
!528 = !{!"tac=0x798", !"op=SHL", !"evm.pc=0x798"}
!529 = !{!"tac=0x79a", !"op=MSTORE", !"evm.pc=0x79a"}
!530 = !{!"tac=0x7a0", !"op=ADD", !"evm.pc=0x7a0"}
!531 = !{!"tac=0x7a1", !"op=MSTORE", !"evm.pc=0x7a1"}
!532 = !{!"tac=0x7a5", !"op=REVERT", !"evm.pc=0x7a5"}
!533 = !{!"tac=0x702", !"op=MSTORE", !"evm.pc=0x702"}
!534 = !{!"tac=0x706", !"op=MSTORE", !"evm.pc=0x706"}
!535 = !{!"tac=0x70a", !"op=SHA3", !"evm.pc=0x70a"}
!536 = !{!"tac=0x70b", !"op=SLOAD", !"evm.pc=0x70b"}
!537 = !{!"tac=0x70f", !"op=LT", !"evm.pc=0x70f"}
!538 = !{!"tac=0x713", !"op=JUMPI", !"evm.pc=0x713"}
!539 = !{!"tac=0x75e", !"op=MLOAD", !"evm.pc=0x75e"}
!540 = !{!"tac=0x766", !"op=SHL", !"evm.pc=0x766"}
!541 = !{!"tac=0x768", !"op=MSTORE", !"evm.pc=0x768"}
!542 = !{!"tac=0x76f", !"op=SHL", !"evm.pc=0x76f"}
!543 = !{!"tac=0x770", !"op=SUB", !"evm.pc=0x770"}
!544 = !{!"tac=0x774", !"op=AND", !"evm.pc=0x774"}
!545 = !{!"tac=0x778", !"op=ADD", !"evm.pc=0x778"}
!546 = !{!"tac=0x779", !"op=MSTORE", !"evm.pc=0x779"}
!547 = !{!"tac=0x77d", !"op=ADD", !"evm.pc=0x77d"}
!548 = !{!"tac=0x781", !"op=MSTORE", !"evm.pc=0x781"}
!549 = !{!"tac=0x785", !"op=ADD", !"evm.pc=0x785"}
!550 = !{!"tac=0x788", !"op=MSTORE", !"evm.pc=0x788"}
!551 = !{!"tac=0x78c", !"op=REVERT", !"evm.pc=0x78c"}
!552 = !{!"tac=0x740", !"op=MSTORE", !"evm.pc=0x740"}
!553 = !{!"tac=0x743", !"op=MSTORE", !"evm.pc=0x743"}
!554 = !{!"tac=0x744", !"op=SUB", !"evm.pc=0x744"}
!555 = !{!"tac=0x747", !"op=SHA3", !"evm.pc=0x747"}
!556 = !{!"tac=0x748", !"op=SSTORE", !"evm.pc=0x748"}
!557 = !{!"tac=0x74b", !"op=MSTORE", !"evm.pc=0x74b"}
!558 = !{!"tac=0x74c", !"op=SHA3", !"evm.pc=0x74c"}
!559 = !{!"tac=0x74f", !"op=SLOAD", !"evm.pc=0x74f"}
!560 = !{!"tac=0x750", !"op=ADD", !"evm.pc=0x750"}
!561 = !{!"tac=0x752", !"op=SSTORE", !"evm.pc=0x752"}
!562 = !{!"tac=0x755", !"op=MLOAD", !"evm.pc=0x755"}
!563 = !{!"tac=0x758", !"op=MSTORE", !"evm.pc=0x758"}
!564 = !{!"tac=0x759", !"op=LOG3", !"evm.pc=0x759"}
!565 = !{!"tac=0x75a", !"op=RETURNPRIVATE", !"evm.pc=0x75a"}
!566 = !{!"tac=0x7c2", !"op=SLOAD", !"evm.pc=0x7c2"}
!567 = !{!"tac=0x7c9", !"op=SHL", !"evm.pc=0x7c9"}
!568 = !{!"tac=0x7ca", !"op=SUB", !"evm.pc=0x7ca"}
!569 = !{!"tac=0x7cb", !"op=AND", !"evm.pc=0x7cb"}
!570 = !{!"tac=0x7cc", !"op=CALLER", !"evm.pc=0x7cc"}
!571 = !{!"tac=0x7cd", !"op=SUB", !"evm.pc=0x7cd"}
!572 = !{!"tac=0x7d1", !"op=JUMPI", !"evm.pc=0x7d1"}
!573 = !{!"tac=0x7d6", !"op=MLOAD", !"evm.pc=0x7d6"}
!574 = !{!"tac=0x7de", !"op=SHL", !"evm.pc=0x7de"}
!575 = !{!"tac=0x7e0", !"op=MSTORE", !"evm.pc=0x7e0"}
!576 = !{!"tac=0x7e1", !"op=CALLER", !"evm.pc=0x7e1"}
!577 = !{!"tac=0x7e5", !"op=ADD", !"evm.pc=0x7e5"}
!578 = !{!"tac=0x7e6", !"op=MSTORE", !"evm.pc=0x7e6"}
!579 = !{!"tac=0x7ea", !"op=REVERT", !"evm.pc=0x7ea"}
!580 = !{!"tac=0x7d2", !"op=RETURNPRIVATE", !"evm.pc=0x7d2"}
!581 = !{!"tac=0xa8", !"op=CALLVALUE", !"evm.pc=0xa8"}
!582 = !{!"tac=0xac", !"op=JUMPI", !"evm.pc=0xac"}
!583 = !{!"tac=0x1063", !"op=REVERT", !"evm.pc=0x138"}
!584 = !{!"tac=0xaf", !"op=CALLDATASIZE", !"evm.pc=0xaf"}
!585 = !{!"tac=0xb3", !"op=ADD", !"evm.pc=0xb3"}
!586 = !{!"tac=0xb4", !"op=SLT", !"evm.pc=0xb4"}
!587 = !{!"tac=0xb8", !"op=JUMPI", !"evm.pc=0xb8"}
!588 = !{!"tac=0x1086", !"op=REVERT", !"evm.pc=0x138"}
!589 = !{!"tac=0xbf", !"op=CALLPRIVATE", !"evm.pc=0xbf"}
!590 = !{!"tac=0xc8", !"op=CALLPRIVATE", !"evm.pc=0xc8"}
!591 = !{!"tac=0xd0", !"op=SHL", !"evm.pc=0xd0"}
!592 = !{!"tac=0xd1", !"op=SUB", !"evm.pc=0xd1"}
!593 = !{!"tac=0xd4", !"op=AND", !"evm.pc=0xd4"}
!594 = !{!"tac=0xd7", !"op=ISZERO", !"evm.pc=0xd7"}
!595 = !{!"tac=0xdb", !"op=JUMPI", !"evm.pc=0xdb"}
!596 = !{!"tac=0x120", !"op=MLOAD", !"evm.pc=0x120"}
!597 = !{!"tac=0x128", !"op=SHL", !"evm.pc=0x128"}
!598 = !{!"tac=0x12a", !"op=MSTORE", !"evm.pc=0x12a"}
!599 = !{!"tac=0x12d", !"op=ADD", !"evm.pc=0x12d"}
!600 = !{!"tac=0x130", !"op=MSTORE", !"evm.pc=0x130"}
!601 = !{!"tac=0x134", !"op=REVERT", !"evm.pc=0x134"}
!602 = !{!"tac=0xe0", !"op=SLOAD", !"evm.pc=0xe0"}
!603 = !{!"tac=0xf1", !"op=SHL", !"evm.pc=0xf1"}
!604 = !{!"tac=0xf3", !"op=AND", !"evm.pc=0xf3"}
!605 = !{!"tac=0xf4", !"op=OR", !"evm.pc=0xf4"}
!606 = !{!"tac=0xf7", !"op=SSTORE", !"evm.pc=0xf7"}
!607 = !{!"tac=0xf8", !"op=AND", !"evm.pc=0xf8"}
!608 = !{!"tac=0x11c", !"op=LOG3", !"evm.pc=0x11c"}
!609 = !{!"tac=0x11e", !"op=RETURN", !"evm.pc=0x11e"}

attributes #0 = { null_pointer_is_valid }
