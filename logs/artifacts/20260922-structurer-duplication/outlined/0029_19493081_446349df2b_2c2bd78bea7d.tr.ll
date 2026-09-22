; ModuleID = 'test/evm/solidity-patterns/cases/0029_19493081_446349df2b_2c2bd78bea7d.ll'
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

; Function Attrs: null_pointer_is_valid
define void @public___function_selector___0x0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x0:
  store i256 128, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !1
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !2
  br i1 %evm.lt, label %bb._0x43, label %bb._0xd, !notdec.evm !3

bb._0x43:                                         ; preds = %bb._0x0
  %evm.calldatasize1 = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.branch.cond2.not = icmp eq i256 %evm.calldatasize1, 0, !notdec.evm !5
  br i1 %evm.branch.cond2.not, label %bb._0x6125c, label %bb._0x1c78, !notdec.evm !5

common.ret:                                       ; preds = %bb._0x63a5c, %bb._0x6305c, %bb._0x6265c, %bb._0x61c5c, %bb._0x6125c
  ret void

bb._0x6125c:                                      ; preds = %bb._0x43
  call void @public__0xeeeeeeee_0x6129c(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !6
  br label %common.ret

bb._0x1c78:                                       ; preds = %bb._0x43
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !7
  unreachable, !notdec.evm !7

bb._0xd:                                          ; preds = %bb._0x0
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !8
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !9
  switch i256 %evm.shr, label %bb._0x1c54 [
    i256 1812113713, label %bb._0x61c5c
    i256 2070004512, label %bb._0x6265c
    i256 3197789525, label %bb._0x6305c
    i256 3572052626, label %bb._0x63a5c
  ]

bb._0x61c5c:                                      ; preds = %bb._0xd
  call void @public_tokenName___0x4f(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !10
  br label %common.ret

bb._0x6265c:                                      ; preds = %bb._0xd
  call void @public_tokenSymbol___0xdf(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  br label %common.ret

bb._0x6305c:                                      ; preds = %bb._0xd
  call void @public_start___0x16f(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  br label %common.ret

bb._0x63a5c:                                      ; preds = %bb._0xd
  call void @public_withdrawal___0x179(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !13
  br label %common.ret

bb._0x1c54:                                       ; preds = %bb._0xd
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !14
  unreachable, !notdec.evm !14
}

; Function Attrs: null_pointer_is_valid
define void @public_start___0x16f(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x16f:
  %notdec.calldata.view = call ptr @notdec_evm_calldata_view(ptr %calldata)
  call void @private__0x2bf_0x2bf(ptr %mem, ptr %notdec.calldata.view, ptr %returndata, ptr %env, i256 375), !notdec.evm !15
  ret void, !notdec.evm !16
}

; Function Attrs: null_pointer_is_valid
define void @public_withdrawal___0x179(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x179:
  %notdec.calldata.view = call ptr @notdec_evm_calldata_view(ptr %calldata)
  call void @private__0x3bb_0x3bb(ptr %mem, ptr %notdec.calldata.view, ptr %returndata, ptr %env, i256 385), !notdec.evm !17
  ret void, !notdec.evm !18
}

; Function Attrs: null_pointer_is_valid
define internal { i256, i256 } @private__0x183_0x183(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x183arg0x0) #0 !notdec.evm.yul_function_summary !19 !notdec.mlsub.polymorphic_function !20 {
bb._0x183:
  %evm.sload = call i256 @evm_sload(i256 0), !notdec.evm !21
  %evm.and = shl i256 %evm.sload, 8
  %evm.bool = and i256 %evm.and, 256
  %evm.mul = xor i256 %evm.bool, 256
  %evm.sub = add nsw i256 %evm.mul, -1
  %evm.and1 = and i256 %evm.sub, %evm.sload, !notdec.evm !22
  %evm.div = call i256 @evm_div(i256 %evm.and1, i256 2), !notdec.evm !23
  %evm.add = add i256 %evm.div, 31, !notdec.evm !24
  %evm.div2 = call i256 @evm_div(i256 %evm.add, i256 32), !notdec.evm !25
  %evm.mul3 = shl i256 %evm.div2, 5
  %evm.add4 = add i256 %evm.mul3, 32, !notdec.evm !26
  %0 = call ptr @calloc(i256 1, i256 %evm.add4)
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr, i256 %evm.add4)
  store i256 %evm.div, ptr %0, align 1, !notdec.evm !27
  %evm.add6 = add i256 %evm.alloc.addr, 32, !notdec.evm !28
  %evm.sload7 = call i256 @evm_sload(i256 0), !notdec.evm !29
  %evm.and8 = shl i256 %evm.sload7, 8
  %evm.bool10 = and i256 %evm.and8, 256
  %evm.mul11 = xor i256 %evm.bool10, 256
  %evm.sub12 = add nsw i256 %evm.mul11, -1
  %evm.and13 = and i256 %evm.sub12, %evm.sload7, !notdec.evm !30
  %evm.div14 = call i256 @evm_div(i256 %evm.and13, i256 2), !notdec.evm !31
  %evm.iszero15 = icmp eq i256 %evm.div14, 0, !notdec.evm !32
  br i1 %evm.iszero15, label %common.ret, label %bb._0x1d3, !notdec.evm !33

common.ret:                                       ; preds = %bb._0x1fc, %bb._0x183, %bb._0x1db
  %ret.insert.pn = insertvalue { i256, i256 } poison, i256 %evm.alloc.addr, 0
  %common.ret.op = insertvalue { i256, i256 } %ret.insert.pn, i256 %_0x183arg0x0, 1
  ret { i256, i256 } %common.ret.op, !notdec.evm !34

bb._0x1d3:                                        ; preds = %bb._0x183
  %evm.lt = icmp ugt i256 %evm.div14, 31, !notdec.evm !35
  br i1 %evm.lt, label %bb._0x1ee, label %bb._0x1db, !notdec.evm !36

bb._0x1ee:                                        ; preds = %bb._0x1d3
  %evm.add20 = add i256 %evm.add6, %evm.div14, !notdec.evm !37
  store i256 0, ptr null, align 1, !notdec.evm !38
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 0, i256 32), !notdec.evm !39
  br label %bb._0x1fc, !notdec.evm !40

bb._0x1fc:                                        ; preds = %bb._0x1fc, %bb._0x1ee
  %_0x1fc_0x0 = phi i256 [ %evm.add6, %bb._0x1ee ], [ %evm.add23, %bb._0x1fc ], !notdec.evm !41
  %_0x1fc_0x1 = phi i256 [ %evm.sha3, %bb._0x1ee ], [ %evm.add22, %bb._0x1fc ], !notdec.evm !42
  %evm.sload21 = call i256 @evm_sload(i256 %_0x1fc_0x1), !notdec.evm !43
  %notdec.evm.mem.ptr.5 = inttoptr i256 %_0x1fc_0x0 to ptr
  store i256 %evm.sload21, ptr %notdec.evm.mem.ptr.5, align 1, !notdec.evm !44
  %evm.add22 = add i256 %_0x1fc_0x1, 1, !notdec.evm !45
  %evm.add23 = add i256 %_0x1fc_0x0, 32, !notdec.evm !46
  %evm.gt = icmp ugt i256 %evm.add20, %evm.add23, !notdec.evm !47
  br i1 %evm.gt, label %bb._0x1fc, label %common.ret, !notdec.evm !48

bb._0x1db:                                        ; preds = %bb._0x1d3
  %evm.sload31 = call i256 @evm_sload(i256 0), !notdec.evm !49
  %evm.div32 = call i256 @evm_div(i256 %evm.sload31, i256 256), !notdec.evm !50
  %evm.mul33 = shl i256 %evm.div32, 8
  %notdec.evm.mem.ptr.6 = inttoptr i256 %evm.add6 to ptr
  store i256 %evm.mul33, ptr %notdec.evm.mem.ptr.6, align 1, !notdec.evm !51
  br label %common.ret
}

; Function Attrs: null_pointer_is_valid
define internal { i256, i256 } @private__0x221_0x221(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x221arg0x0) #0 !notdec.evm.yul_function_summary !19 !notdec.mlsub.polymorphic_function !20 {
bb._0x221:
  %evm.sload = call i256 @evm_sload(i256 1), !notdec.evm !52
  %evm.and = shl i256 %evm.sload, 8
  %evm.bool = and i256 %evm.and, 256
  %evm.mul = xor i256 %evm.bool, 256
  %evm.sub = add nsw i256 %evm.mul, -1
  %evm.and1 = and i256 %evm.sub, %evm.sload, !notdec.evm !53
  %evm.div = call i256 @evm_div(i256 %evm.and1, i256 2), !notdec.evm !54
  %evm.add = add i256 %evm.div, 31, !notdec.evm !55
  %evm.div2 = call i256 @evm_div(i256 %evm.add, i256 32), !notdec.evm !56
  %evm.mul3 = shl i256 %evm.div2, 5
  %evm.add4 = add i256 %evm.mul3, 32, !notdec.evm !57
  %0 = call ptr @calloc(i256 1, i256 %evm.add4)
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr, i256 %evm.add4)
  store i256 %evm.div, ptr %0, align 1, !notdec.evm !58
  %evm.add6 = add i256 %evm.alloc.addr, 32, !notdec.evm !59
  %evm.sload7 = call i256 @evm_sload(i256 1), !notdec.evm !60
  %evm.and8 = shl i256 %evm.sload7, 8
  %evm.bool10 = and i256 %evm.and8, 256
  %evm.mul11 = xor i256 %evm.bool10, 256
  %evm.sub12 = add nsw i256 %evm.mul11, -1
  %evm.and13 = and i256 %evm.sub12, %evm.sload7, !notdec.evm !61
  %evm.div14 = call i256 @evm_div(i256 %evm.and13, i256 2), !notdec.evm !62
  %evm.iszero15 = icmp eq i256 %evm.div14, 0, !notdec.evm !63
  br i1 %evm.iszero15, label %common.ret, label %bb._0x271, !notdec.evm !64

common.ret:                                       ; preds = %bb._0x29a, %bb._0x221, %bb._0x279
  %ret.insert.pn = insertvalue { i256, i256 } poison, i256 %evm.alloc.addr, 0
  %common.ret.op = insertvalue { i256, i256 } %ret.insert.pn, i256 %_0x221arg0x0, 1
  ret { i256, i256 } %common.ret.op, !notdec.evm !65

bb._0x271:                                        ; preds = %bb._0x221
  %evm.lt = icmp ugt i256 %evm.div14, 31, !notdec.evm !66
  br i1 %evm.lt, label %bb._0x28c, label %bb._0x279, !notdec.evm !67

bb._0x28c:                                        ; preds = %bb._0x271
  %evm.add20 = add i256 %evm.add6, %evm.div14, !notdec.evm !68
  store i256 1, ptr null, align 1, !notdec.evm !69
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 0, i256 32), !notdec.evm !70
  br label %bb._0x29a, !notdec.evm !71

bb._0x29a:                                        ; preds = %bb._0x29a, %bb._0x28c
  %_0x29a_0x0 = phi i256 [ %evm.add6, %bb._0x28c ], [ %evm.add23, %bb._0x29a ], !notdec.evm !72
  %_0x29a_0x1 = phi i256 [ %evm.sha3, %bb._0x28c ], [ %evm.add22, %bb._0x29a ], !notdec.evm !73
  %evm.sload21 = call i256 @evm_sload(i256 %_0x29a_0x1), !notdec.evm !74
  %notdec.evm.mem.ptr.11 = inttoptr i256 %_0x29a_0x0 to ptr
  store i256 %evm.sload21, ptr %notdec.evm.mem.ptr.11, align 1, !notdec.evm !75
  %evm.add22 = add i256 %_0x29a_0x1, 1, !notdec.evm !76
  %evm.add23 = add i256 %_0x29a_0x0, 32, !notdec.evm !77
  %evm.gt = icmp ugt i256 %evm.add20, %evm.add23, !notdec.evm !78
  br i1 %evm.gt, label %bb._0x29a, label %common.ret, !notdec.evm !79

bb._0x279:                                        ; preds = %bb._0x271
  %evm.sload31 = call i256 @evm_sload(i256 1), !notdec.evm !80
  %evm.div32 = call i256 @evm_div(i256 %evm.sload31, i256 256), !notdec.evm !81
  %evm.mul33 = shl i256 %evm.div32, 8
  %notdec.evm.mem.ptr.12 = inttoptr i256 %evm.add6 to ptr
  store i256 %evm.mul33, ptr %notdec.evm.mem.ptr.12, align 1, !notdec.evm !82
  br label %common.ret
}

; Function Attrs: null_pointer_is_valid
define internal void @private__0x2bf_0x2bf(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x2bfarg0x0) #0 !notdec.evm.yul_function_summary !83 !notdec.mlsub.polymorphic_function !20 {
bb._0x2bf:
  %0 = call ptr @calloc_unbounded()
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  %evm.add = add i256 %evm.alloc.addr, 32, !notdec.evm !84
  store i256 32, ptr %0, align 1, !notdec.evm !85
  %notdec.evm.mem.ptr.15 = inttoptr i256 %evm.add to ptr
  store i256 56, ptr %notdec.evm.mem.ptr.15, align 1, !notdec.evm !86
  %evm.add1 = add i256 %evm.alloc.addr, 64, !notdec.evm !87
  call void @evm_codecopy(ptr %mem, ptr %env, i256 %evm.add1, i256 3502, i256 56), !notdec.evm !88
  call void @evm_log1(ptr %mem, i256 %evm.alloc.addr, i256 128, i256 -22069801765230701749439377517111017042771441076549128070277515231530640000085), !notdec.evm !89
  %private.call = call i256 @private__0x4b7_0x4b7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 786), !notdec.evm !90
  %evm.iszero = icmp eq i256 %private.call, 0, !notdec.evm !91
  br i1 %evm.iszero, label %bb._0x36a, label %bb._0x318, !notdec.evm !92

bb._0x36a:                                        ; preds = %bb._0x2bf
  %private.call5 = call i256 @private__0x4d4_0x4d4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 882), !notdec.evm !93
  %evm.and = and i256 %private.call5, 1461501637330902918203684832716283019655932542975, !notdec.evm !94
  %evm.selfbalance = call i256 @evm_selfbalance(ptr %env), !notdec.evm !95
  %evm.iszero6 = icmp eq i256 %evm.selfbalance, 0, !notdec.evm !96
  %evm.mul = select i1 %evm.iszero6, i256 2300, i256 0
  %evm.call = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.mul, i256 %evm.and, i256 %evm.selfbalance, i256 %evm.alloc.addr, i256 0, i256 %evm.alloc.addr, i256 0), !notdec.evm !97
  %evm.iszero11.not = icmp eq i256 %evm.call, 0, !notdec.evm !98
  br i1 %evm.iszero11.not, label %bb._0x3ae, label %common.ret, !notdec.evm !99

common.ret:                                       ; preds = %bb._0x318, %bb._0x36a
  ret void, !notdec.evm !100

bb._0x3ae:                                        ; preds = %bb._0x36a
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !101
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize), !notdec.evm !102
  %evm.returndatasize16 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !103
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize16), !notdec.evm !104
  unreachable, !notdec.evm !104

bb._0x318:                                        ; preds = %bb._0x2bf
  %private.call17 = call i256 @private__0x4d4_0x4d4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 799), !notdec.evm !105
  %evm.and18 = and i256 %private.call17, 1461501637330902918203684832716283019655932542975, !notdec.evm !106
  %evm.selfbalance19 = call i256 @evm_selfbalance(ptr %env), !notdec.evm !107
  %evm.iszero20 = icmp eq i256 %evm.selfbalance19, 0, !notdec.evm !108
  %evm.mul22 = select i1 %evm.iszero20, i256 2300, i256 0
  %evm.call26 = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.mul22, i256 %evm.and18, i256 %evm.selfbalance19, i256 %evm.alloc.addr, i256 0, i256 %evm.alloc.addr, i256 0), !notdec.evm !109
  %evm.iszero27.not = icmp eq i256 %evm.call26, 0, !notdec.evm !110
  br i1 %evm.iszero27.not, label %bb._0x35b, label %common.ret, !notdec.evm !111

bb._0x35b:                                        ; preds = %bb._0x318
  %evm.returndatasize32 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !112
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize32), !notdec.evm !113
  %evm.returndatasize33 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !114
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize33), !notdec.evm !115
  unreachable, !notdec.evm !115
}

; Function Attrs: null_pointer_is_valid
define internal void @private__0x3bb_0x3bb(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x3bbarg0x0) #0 !notdec.evm.yul_function_summary !83 !notdec.mlsub.polymorphic_function !20 {
bb._0x3bb:
  %0 = call ptr @calloc_unbounded()
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  %evm.add = add i256 %evm.alloc.addr, 32, !notdec.evm !116
  store i256 32, ptr %0, align 1, !notdec.evm !117
  %notdec.evm.mem.ptr.23 = inttoptr i256 %evm.add to ptr
  store i256 51, ptr %notdec.evm.mem.ptr.23, align 1, !notdec.evm !118
  %evm.add1 = add i256 %evm.alloc.addr, 64, !notdec.evm !119
  call void @evm_codecopy(ptr %mem, ptr %env, i256 %evm.add1, i256 3451, i256 51), !notdec.evm !120
  call void @evm_log1(ptr %mem, i256 %evm.alloc.addr, i256 128, i256 -22069801765230701749439377517111017042771441076549128070277515231530640000085), !notdec.evm !121
  %private.call = call i256 @private__0x4b7_0x4b7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1038), !notdec.evm !122
  %evm.iszero = icmp eq i256 %private.call, 0, !notdec.evm !123
  br i1 %evm.iszero, label %bb._0x466, label %bb._0x414, !notdec.evm !124

bb._0x466:                                        ; preds = %bb._0x3bb
  %private.call5 = call i256 @private__0x4eb_0x4eb(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1134), !notdec.evm !125
  %evm.and = and i256 %private.call5, 1461501637330902918203684832716283019655932542975, !notdec.evm !126
  %evm.selfbalance = call i256 @evm_selfbalance(ptr %env), !notdec.evm !127
  %evm.iszero6 = icmp eq i256 %evm.selfbalance, 0, !notdec.evm !128
  %evm.mul = select i1 %evm.iszero6, i256 2300, i256 0
  %evm.call = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.mul, i256 %evm.and, i256 %evm.selfbalance, i256 %evm.alloc.addr, i256 0, i256 %evm.alloc.addr, i256 0), !notdec.evm !129
  %evm.iszero11.not = icmp eq i256 %evm.call, 0, !notdec.evm !130
  br i1 %evm.iszero11.not, label %bb._0x4aa, label %common.ret, !notdec.evm !131

common.ret:                                       ; preds = %bb._0x414, %bb._0x466
  ret void, !notdec.evm !132

bb._0x4aa:                                        ; preds = %bb._0x466
  %evm.returndatasize = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !133
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize), !notdec.evm !134
  %evm.returndatasize16 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !135
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize16), !notdec.evm !136
  unreachable, !notdec.evm !136

bb._0x414:                                        ; preds = %bb._0x3bb
  %private.call17 = call i256 @private__0x4eb_0x4eb(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1051), !notdec.evm !137
  %evm.and18 = and i256 %private.call17, 1461501637330902918203684832716283019655932542975, !notdec.evm !138
  %evm.selfbalance19 = call i256 @evm_selfbalance(ptr %env), !notdec.evm !139
  %evm.iszero20 = icmp eq i256 %evm.selfbalance19, 0, !notdec.evm !140
  %evm.mul22 = select i1 %evm.iszero20, i256 2300, i256 0
  %evm.call26 = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.mul22, i256 %evm.and18, i256 %evm.selfbalance19, i256 %evm.alloc.addr, i256 0, i256 %evm.alloc.addr, i256 0), !notdec.evm !141
  %evm.iszero27.not = icmp eq i256 %evm.call26, 0, !notdec.evm !142
  br i1 %evm.iszero27.not, label %bb._0x457, label %common.ret, !notdec.evm !143

bb._0x457:                                        ; preds = %bb._0x414
  %evm.returndatasize32 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !144
  call void @evm_returndatacopy(ptr %mem, ptr %returndata, i256 0, i256 0, i256 %evm.returndatasize32), !notdec.evm !145
  %evm.returndatasize33 = call i256 @evm_returndatasize(ptr %returndata), !notdec.evm !146
  call void @evm_revert(ptr %mem, i256 0, i256 %evm.returndatasize33), !notdec.evm !147
  unreachable, !notdec.evm !147
}

; Function Attrs: null_pointer_is_valid
define internal range(i256 0, 2) i256 @private__0x4b7_0x4b7(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x4b7arg0x0) #0 !notdec.evm.yul_function_summary !148 !notdec.mlsub.polymorphic_function !20 {
bb._0x4b7:
  %evm.sload = call i256 @evm_sload(i256 3), !notdec.evm !149
  %evm.selfbalance = call i256 @evm_selfbalance(ptr %env), !notdec.evm !150
  %evm.gt.not = icmp ugt i256 %evm.selfbalance, %evm.sload, !notdec.evm !151
  %spec.select = zext i1 %evm.gt.not to i256
  ret i256 %spec.select, !notdec.evm !152
}

; Function Attrs: null_pointer_is_valid
define internal i256 @private__0x4d4_0x4d4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x4d4arg0x0) #0 !notdec.evm.yul_function_summary !153 !notdec.mlsub.polymorphic_function !20 {
bb._0x4d4:
  %private.call = call i256 @private__0x502_0x502(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1249), !notdec.evm !154
  %private.call1 = call i256 @private__0x680_0x680(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call, i256 1254), !notdec.evm !155
  ret i256 %private.call1, !notdec.evm !156
}

; Function Attrs: null_pointer_is_valid
define internal i256 @private__0x4eb_0x4eb(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x4ebarg0x0) #0 !notdec.evm.yul_function_summary !153 !notdec.mlsub.polymorphic_function !20 {
bb._0x4eb:
  %private.call = call i256 @private__0x502_0x502(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1272), !notdec.evm !157
  %private.call1 = call i256 @private__0x680_0x680(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call, i256 1277), !notdec.evm !158
  ret i256 %private.call1, !notdec.evm !159
}

; Function Attrs: null_pointer_is_valid
define void @public_tokenName___0x4f(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 !notdec.solidity.nonpayable !160 {
bb._0x4f:
  %notdec.calldata.view = call ptr @notdec_evm_calldata_view(ptr %calldata)
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !161, !notdec.solidity.payability_guard !162, !notdec.solidity.rewrite_hidden.payability_guard !162
  call void @notdec_solidity_rewrite_hidden(i256 9745862514924683837)
  call void @notdec_solidity_rewrite_payability_guard(i256 8598331119976828368)
  call void @notdec_solidity_rewrite_hidden(i256 9745862514924683837)
  call void @notdec_solidity_rewrite_payability_guard(i256 12829813975230984100)
  call void @notdec_solidity_rewrite_payability_guard(i256 7817397773529159771)
  call void @notdec_solidity_rewrite_hidden(i256 9745862514924683837)
  call void @notdec_solidity_cfg_rewrite_payability_guard(i256 1330768661709002677)
  br label %bb._0x5b, !notdec.evm !163, !notdec.solidity.payability_guard !164, !notdec.solidity.rewrite_hidden.payability_guard !164

bb._0x5b:                                         ; preds = %bb._0x4f
  %private.call = call { i256, i256 } @private__0x183_0x183(ptr %mem, ptr %notdec.calldata.view, ptr %returndata, ptr %env, i256 100), !notdec.evm !165
  %private.ret = extractvalue { i256, i256 } %private.call, 0, !notdec.evm !165
  %0 = call ptr @calloc_unbounded()
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  %evm.add = add i256 %evm.alloc.addr, 32, !notdec.evm !166
  store i256 32, ptr %0, align 1, !notdec.evm !167
  %notdec.evm.mem.ptr.31 = inttoptr i256 %private.ret to ptr
  %evm.mload2 = load i256, ptr %notdec.evm.mem.ptr.31, align 1, !notdec.evm !168
  %notdec.evm.mem.ptr.32 = inttoptr i256 %evm.add to ptr
  store i256 %evm.mload2, ptr %notdec.evm.mem.ptr.32, align 1, !notdec.evm !169
  %evm.add3 = add i256 %evm.alloc.addr, 64, !notdec.evm !170
  %evm.mload4 = load i256, ptr %notdec.evm.mem.ptr.31, align 1, !notdec.evm !171
  %evm.add5 = add i256 %private.ret, 32, !notdec.evm !172
  br label %bb._0x89, !notdec.evm !173

bb._0x89:                                         ; preds = %bb._0x92, %bb._0x5b
  %_0x89_0x0 = phi i256 [ 0, %bb._0x5b ], [ %evm.add25, %bb._0x92 ], !notdec.evm !174
  %evm.lt.not = icmp ult i256 %_0x89_0x0, %evm.mload4, !notdec.evm !175
  br i1 %evm.lt.not, label %bb._0x92, label %bb._0xa4, !notdec.evm !176

bb._0xa4:                                         ; preds = %bb._0x89
  %evm.add10 = add i256 %evm.mload4, %evm.add3, !notdec.evm !177
  %evm.and = and i256 %evm.mload4, 31, !notdec.evm !178
  %evm.iszero11 = icmp eq i256 %evm.and, 0, !notdec.evm !179
  br i1 %evm.iszero11, label %bb._0xd1, label %bb._0xb8, !notdec.evm !180

bb._0xb8:                                         ; preds = %bb._0xa4
  %evm.sub14 = sub i256 %evm.add10, %evm.and, !notdec.evm !181
  %notdec.evm.mem.ptr.34 = inttoptr i256 %evm.sub14 to ptr
  %evm.mload15 = load i256, ptr %notdec.evm.mem.ptr.34, align 1, !notdec.evm !182
  %evm.sub16 = sub nuw nsw i256 32, %evm.and, !notdec.evm !183
  %evm.exp = call i256 @evm_exp(i256 256, i256 %evm.sub16), !notdec.evm !184
  %evm.not = sub i256 0, %evm.exp
  %evm.and18 = and i256 %evm.mload15, %evm.not, !notdec.evm !185
  store i256 %evm.and18, ptr %notdec.evm.mem.ptr.34, align 1, !notdec.evm !186
  %evm.add19 = add i256 %evm.sub14, 32, !notdec.evm !187
  br label %bb._0xd1, !notdec.evm !188

bb._0xd1:                                         ; preds = %bb._0xb8, %bb._0xa4
  %_0xd1_0x1 = phi i256 [ %evm.add10, %bb._0xa4 ], [ %evm.add19, %bb._0xb8 ], !notdec.evm !189
  %evm.sub21 = sub i256 %_0xd1_0x1, %evm.alloc.addr, !notdec.evm !190
  call void @evm_return(ptr %mem, i256 %evm.alloc.addr, i256 %evm.sub21), !notdec.evm !191
  ret void, !notdec.evm !191

bb._0x92:                                         ; preds = %bb._0x89
  %evm.add22 = add i256 %evm.add5, %_0x89_0x0, !notdec.evm !192
  %notdec.evm.mem.ptr.37 = inttoptr i256 %evm.add22 to ptr
  %evm.mload23 = load i256, ptr %notdec.evm.mem.ptr.37, align 1, !notdec.evm !193
  %evm.add24 = add i256 %evm.add3, %_0x89_0x0, !notdec.evm !194
  %notdec.evm.mem.ptr.38 = inttoptr i256 %evm.add24 to ptr
  store i256 %evm.mload23, ptr %notdec.evm.mem.ptr.38, align 1, !notdec.evm !195
  %evm.add25 = add i256 %_0x89_0x0, 32, !notdec.evm !196
  br label %bb._0x89, !notdec.evm !197

bb._0x57:                                         ; No predecessors!
  unreachable, !notdec.evm !198, !notdec.solidity.payability_guard !199, !notdec.solidity.rewrite_hidden.payability_guard !199
}

; Function Attrs: null_pointer_is_valid
define internal i256 @private__0x502_0x502(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x502arg0x0) #0 !notdec.evm.yul_function_summary !200 !notdec.mlsub.polymorphic_function !20 {
bb._0x502:
  %0 = call ptr @calloc(i256 1, i256 64)
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr, i256 64)
  store i256 1, ptr %0, align 1, !notdec.evm !201
  %evm.add1 = add i256 %evm.alloc.addr, 32, !notdec.evm !202
  %notdec.evm.mem.ptr.42 = inttoptr i256 %evm.add1 to ptr
  store i256 54277541829991966604798899222822456806220305312019014393495742503709279518720, ptr %notdec.evm.mem.ptr.42, align 1, !notdec.evm !203
  %private.call = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 739426, i256 1358), !notdec.evm !204
  %private.call2 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call, i256 %evm.alloc.addr, i256 1363), !notdec.evm !205
  %private.call3 = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 918950, i256 1448), !notdec.evm !206
  %private.call4 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call3, i256 %private.call2, i256 1453), !notdec.evm !207
  %private.call5 = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 405294, i256 1469), !notdec.evm !208
  %private.call6 = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1021841, i256 1478), !notdec.evm !209
  %private.call7 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call6, i256 %private.call5, i256 1483), !notdec.evm !210
  %private.call8 = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 45060, i256 1499), !notdec.evm !211
  %private.call9 = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 582688, i256 1508), !notdec.evm !212
  %private.call10 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call9, i256 %private.call8, i256 1513), !notdec.evm !213
  %private.call11 = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 521373, i256 1529), !notdec.evm !214
  %private.call12 = call i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 905777, i256 1538), !notdec.evm !215
  %private.call13 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call12, i256 %private.call11, i256 1543), !notdec.evm !216
  %private.call14 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call7, i256 %private.call4, i256 1560), !notdec.evm !217
  %private.call15 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call13, i256 %private.call10, i256 1570), !notdec.evm !218
  %private.call16 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call15, i256 %private.call14, i256 1575), !notdec.evm !219
  %1 = call ptr @calloc(i256 1, i256 64)
  %evm.alloc.addr1 = ptrtoint ptr %1 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr1, i256 64)
  store i256 1, ptr %1, align 1, !notdec.evm !220
  %evm.add19 = add i256 %evm.alloc.addr1, 32, !notdec.evm !221
  %notdec.evm.mem.ptr.46 = inttoptr i256 %evm.add19 to ptr
  store i256 21711016731996786641919559689128982722488122124807605757398297001483711807488, ptr %notdec.evm.mem.ptr.46, align 1, !notdec.evm !222
  %private.call20 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %private.call16, i256 %evm.alloc.addr1, i256 1642), !notdec.evm !223
  ret i256 %private.call20, !notdec.evm !224
}

; Function Attrs: null_pointer_is_valid
define void @public__0xeeeeeeee_0x6129c(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x6129c:
  ret void, !notdec.evm !225
}

; Function Attrs: null_pointer_is_valid
define internal i256 @private__0x680_0x680(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x680arg0x0, i256 %_0x680arg0x1) #0 !notdec.evm.yul_function_summary !226 !notdec.mlsub.polymorphic_function !20 {
bb._0x680:
  br label %bb._0x696, !notdec.evm !227

bb._0x696:                                        ; preds = %bb._0x8b7, %bb._0x680
  %_0x696_0x0 = phi i256 [ 2, %bb._0x680 ], [ %evm.add123, %bb._0x8b7 ], !notdec.evm !228
  %_0x696_0x3 = phi i256 [ 0, %bb._0x680 ], [ %evm.add122, %bb._0x8b7 ], !notdec.evm !229
  %evm.lt = icmp samesign ugt i256 %_0x696_0x0, 41
  br i1 %evm.lt, label %bb._0x8cc, label %bb._0x6a0, !notdec.evm !230

bb._0x8cc:                                        ; preds = %bb._0x696
  ret i256 %_0x696_0x3, !notdec.evm !231

bb._0x6a0:                                        ; preds = %bb._0x696
  %evm.mul = shl i256 %_0x696_0x3, 8
  %notdec.evm.mem.ptr.47 = inttoptr i256 %_0x680arg0x0 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.47, align 1, !notdec.evm !232
  %evm.lt2 = icmp ult i256 %_0x696_0x0, %evm.mload, !notdec.evm !233
  call void @llvm.assume(i1 %evm.lt2)
  %evm.add = add nuw nsw i256 %_0x696_0x0, 32, !notdec.evm !234
  %evm.add5 = add i256 %evm.add, %_0x680arg0x0, !notdec.evm !235
  %notdec.evm.mem.ptr.48 = inttoptr i256 %evm.add5 to ptr
  %evm.mload6 = load i256, ptr %notdec.evm.mem.ptr.48, align 1, !notdec.evm !236
  %evm.shr = call i256 @evm_shr(i256 248, i256 %evm.mload6), !notdec.evm !237
  %evm.shl = call i256 @evm_shl(i256 248, i256 %evm.shr), !notdec.evm !238
  %evm.shr7 = call i256 @evm_shr(i256 248, i256 %evm.shl), !notdec.evm !239
  %evm.and = and i256 %evm.shr7, 255, !notdec.evm !240
  %evm.add8 = or disjoint i256 %_0x696_0x0, 1
  %evm.mload9 = load i256, ptr %notdec.evm.mem.ptr.47, align 1, !notdec.evm !241
  %evm.lt10 = icmp ult i256 %evm.add8, %evm.mload9, !notdec.evm !242
  call void @llvm.assume(i1 %evm.lt10)
  %evm.add13 = add nuw nsw i256 %_0x696_0x0, 33, !notdec.evm !243
  %evm.add14 = add i256 %evm.add13, %_0x680arg0x0, !notdec.evm !244
  %notdec.evm.mem.ptr.50 = inttoptr i256 %evm.add14 to ptr
  %evm.mload15 = load i256, ptr %notdec.evm.mem.ptr.50, align 1, !notdec.evm !245
  %evm.shr16 = call i256 @evm_shr(i256 248, i256 %evm.mload15), !notdec.evm !246
  %evm.shl17 = call i256 @evm_shl(i256 248, i256 %evm.shr16), !notdec.evm !247
  %evm.shr18 = call i256 @evm_shr(i256 248, i256 %evm.shl17), !notdec.evm !248
  %evm.and19 = and i256 %evm.shr18, 255, !notdec.evm !249
  %0 = add nsw i256 %evm.and, -97
  %spec.select = icmp ult i256 %0, 6
  br i1 %spec.select, label %bb._0x72b, label %bb._0x735, !notdec.evm !250

bb._0x735:                                        ; preds = %bb._0x6a0
  %1 = add nsw i256 %evm.and, -65
  %spec.select1 = icmp ult i256 %1, 6
  br i1 %spec.select1, label %bb._0x779, label %bb._0x783, !notdec.evm !251

bb._0x783:                                        ; preds = %bb._0x735
  %2 = add nsw i256 %evm.and, -48
  %spec.select2 = icmp ult i256 %2, 10
  %_0x7cd_0x2 = select i1 %spec.select2, i256 %2, i256 %evm.and
  br label %bb._0x7cf, !notdec.evm !252

bb._0x779:                                        ; preds = %bb._0x735
  %evm.sub67 = add nsw i256 %evm.and, -55
  br label %bb._0x7cf, !notdec.evm !253

bb._0x72b:                                        ; preds = %bb._0x6a0
  %evm.sub68 = add nsw i256 %evm.and, -87
  br label %bb._0x7cf, !notdec.evm !254

bb._0x7cf:                                        ; preds = %bb._0x783, %bb._0x779, %bb._0x72b
  %_0x7cf_0x2 = phi i256 [ %evm.sub68, %bb._0x72b ], [ %evm.sub67, %bb._0x779 ], [ %_0x7cd_0x2, %bb._0x783 ], !notdec.evm !255
  %3 = add nsw i256 %evm.and19, -97
  %spec.select3 = icmp ult i256 %3, 6
  br i1 %spec.select3, label %bb._0x813, label %bb._0x81d, !notdec.evm !256

bb._0x81d:                                        ; preds = %bb._0x7cf
  %4 = add nsw i256 %evm.and19, -65
  %spec.select4 = icmp ult i256 %4, 6
  br i1 %spec.select4, label %bb._0x861, label %bb._0x86b, !notdec.evm !257

bb._0x86b:                                        ; preds = %bb._0x81d
  %5 = add nsw i256 %evm.and19, -48
  %spec.select5 = icmp ult i256 %5, 10
  %_0x8b5_0x1 = select i1 %spec.select5, i256 %5, i256 %evm.and19
  br label %bb._0x8b7, !notdec.evm !258

bb._0x861:                                        ; preds = %bb._0x81d
  %evm.sub118 = add nsw i256 %evm.and19, -55
  br label %bb._0x8b7, !notdec.evm !259

bb._0x813:                                        ; preds = %bb._0x7cf
  %evm.sub119 = add nsw i256 %evm.and19, -87
  br label %bb._0x8b7, !notdec.evm !260

bb._0x8b7:                                        ; preds = %bb._0x86b, %bb._0x861, %bb._0x813
  %_0x8b7_0x1 = phi i256 [ %evm.sub119, %bb._0x813 ], [ %evm.sub118, %bb._0x861 ], [ %_0x8b5_0x1, %bb._0x86b ], !notdec.evm !261
  %evm.mul120 = shl nsw i256 %_0x7cf_0x2, 4
  %evm.add121 = add nsw i256 %evm.mul120, %_0x8b7_0x1, !notdec.evm !262
  %evm.add122 = add i256 %evm.mul, %evm.add121, !notdec.evm !263
  %evm.add123 = add nuw nsw i256 %_0x696_0x0, 2, !notdec.evm !264
  br label %bb._0x696, !notdec.evm !265
}

; Function Attrs: null_pointer_is_valid
define internal i256 @private__0x8e4_0x8e4(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x8e4arg0x0, i256 %_0x8e4arg0x1) #0 !notdec.evm.yul_function_summary !266 !notdec.mlsub.polymorphic_function !20 {
bb._0x8e4:
  br label %bb._0x8f1, !notdec.evm !267

bb._0x8f1:                                        ; preds = %bb._0x90b, %bb._0x8e4
  %_0x8f1_0x0 = phi i256 [ %_0x8e4arg0x0, %bb._0x8e4 ], [ %evm.div62, %bb._0x90b ], !notdec.evm !268
  %_0x8f1_0x1 = phi i256 [ 0, %bb._0x8e4 ], [ %evm.add61, %bb._0x90b ], !notdec.evm !269
  %evm.eq = icmp eq i256 %_0x8f1_0x0, 0, !notdec.evm !270
  br i1 %evm.eq, label %bb._0x913, label %bb._0x90b, !notdec.evm !271

bb._0x913:                                        ; preds = %bb._0x8f1
  %evm.gt = icmp ult i256 %_0x8f1_0x1, 18446744073709551616
  br i1 %evm.gt, label %bb._0x92c, label %bb._0x928, !notdec.evm !272

bb._0x92c:                                        ; preds = %bb._0x913
  %0 = call ptr @calloc_unbounded()
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  store i256 %_0x8f1_0x1, ptr %0, align 1, !notdec.evm !273
  %evm.add = add nuw nsw i256 %_0x8f1_0x1, 31, !notdec.evm !274
  %evm.and = and i256 %evm.add, 36893488147419103200, !notdec.evm !275
  %evm.add4 = add nuw nsw i256 %evm.and, 32, !notdec.evm !276
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr, i256 %evm.add4)
  call void @notdec_evm_finalize_alloc(i256 %evm.alloc.addr, i256 %evm.add4)
  %evm.iszero6 = icmp eq i256 %_0x8f1_0x1, 0, !notdec.evm !277
  br i1 %evm.iszero6, label %bb._0x95f, label %bb._0x94b, !notdec.evm !278

bb._0x94b:                                        ; preds = %bb._0x92c
  %evm.add9 = add i256 %evm.alloc.addr, 32, !notdec.evm !279
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !280
  %calldata.copy.dst = inttoptr i256 %evm.add9 to ptr
  %calldata.copy.src.base = ptrtoint ptr %calldata to i256
  %calldata.copy.src.addr = add i256 %evm.calldatasize, %calldata.copy.src.base
  %calldata.copy.src = inttoptr i256 %calldata.copy.src.addr to ptr
  call void @llvm.memcpy.p0.p0.i256(ptr align 1 %calldata.copy.dst, ptr align 1 %calldata.copy.src, i256 %_0x8f1_0x1, i1 false)
  br label %bb._0x95f, !notdec.evm !281

bb._0x95f:                                        ; preds = %bb._0x94b, %bb._0x92c
  br label %bb._0x968, !notdec.evm !282

bb._0x968:                                        ; preds = %bb._0x97a, %bb._0x95f
  %_0x968_0x0 = phi i256 [ 0, %bb._0x95f ], [ %evm.add60, %bb._0x97a ], !notdec.evm !283
  %_0x968_0x5 = phi i256 [ %_0x8e4arg0x0, %bb._0x95f ], [ %evm.div, %bb._0x97a ], !notdec.evm !284
  %evm.lt.not = icmp ult i256 %_0x968_0x0, %_0x8f1_0x1, !notdec.evm !285
  br i1 %evm.lt.not, label %bb._0x97a, label %bb._0x9de, !notdec.evm !286

bb._0x9de:                                        ; preds = %bb._0x968
  %evm.mload15 = load i256, ptr %0, align 1, !notdec.evm !287
  switch i256 %evm.mload15, label %common.ret [
    i256 4, label %bb._0x9ef
    i256 3, label %bb._0xa48
    i256 2, label %bb._0xaa1
    i256 1, label %bb._0xafa
  ]

common.ret:                                       ; preds = %bb._0x9de, %bb._0x9ef, %bb._0xa48, %bb._0xaa1, %bb._0xafa
  %common.ret.op = phi i256 [ %private.call50, %bb._0x9ef ], [ %private.call, %bb._0xafa ], [ %private.call42, %bb._0xaa1 ], [ %private.call46, %bb._0xa48 ], [ %evm.alloc.addr, %bb._0x9de ]
  ret i256 %common.ret.op, !notdec.evm !288

bb._0xafa:                                        ; preds = %bb._0x9de
  %1 = call ptr @calloc(i256 1, i256 64)
  %evm.alloc.addr1 = ptrtoint ptr %1 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr1, i256 64)
  store i256 4, ptr %1, align 1, !notdec.evm !289
  %evm.add38 = add i256 %evm.alloc.addr1, 32, !notdec.evm !290
  %notdec.evm.mem.ptr.58 = inttoptr i256 %evm.add38 to ptr
  store i256 21796157969008235060032064981462756285411831339671231927890260993281791885312, ptr %notdec.evm.mem.ptr.58, align 1, !notdec.evm !291
  %private.call = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.alloc.addr, i256 %evm.alloc.addr1, i256 2874), !notdec.evm !292
  br label %common.ret

bb._0xaa1:                                        ; preds = %bb._0x9de
  %2 = call ptr @calloc(i256 1, i256 64)
  %evm.alloc.addr2 = ptrtoint ptr %2 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr2, i256 64)
  store i256 3, ptr %2, align 1, !notdec.evm !293
  %evm.add41 = add i256 %evm.alloc.addr2, 32, !notdec.evm !294
  %notdec.evm.mem.ptr.62 = inttoptr i256 %evm.add41 to ptr
  store i256 21796156674930795036801354837446032108469559005088299645942781900308499922944, ptr %notdec.evm.mem.ptr.62, align 1, !notdec.evm !295
  %private.call42 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.alloc.addr, i256 %evm.alloc.addr2, i256 2785), !notdec.evm !296
  br label %common.ret

bb._0xa48:                                        ; preds = %bb._0x9de
  %3 = call ptr @calloc(i256 1, i256 64)
  %evm.alloc.addr3 = ptrtoint ptr %3 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr3, i256 64)
  store i256 1, ptr %3, align 1, !notdec.evm !297
  %evm.add45 = add i256 %evm.alloc.addr3, 32, !notdec.evm !298
  %notdec.evm.mem.ptr.66 = inttoptr i256 %evm.add45 to ptr
  store i256 21711016731996786641919559689128982722488122124807605757398297001483711807488, ptr %notdec.evm.mem.ptr.66, align 1, !notdec.evm !299
  %private.call46 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.alloc.addr, i256 %evm.alloc.addr3, i256 2696), !notdec.evm !300
  br label %common.ret

bb._0x9ef:                                        ; preds = %bb._0x9de
  %4 = call ptr @calloc(i256 1, i256 64)
  %evm.alloc.addr4 = ptrtoint ptr %4 to i256
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr4, i256 64)
  store i256 1, ptr %4, align 1, !notdec.evm !301
  %evm.add49 = add i256 %evm.alloc.addr4, 32, !notdec.evm !302
  %notdec.evm.mem.ptr.70 = inttoptr i256 %evm.add49 to ptr
  store i256 21711016731996786641919559689128982722488122124807605757398297001483711807488, ptr %notdec.evm.mem.ptr.70, align 1, !notdec.evm !303
  %private.call50 = call i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.alloc.addr, i256 %evm.alloc.addr4, i256 2607), !notdec.evm !304
  br label %common.ret

bb._0x97a:                                        ; preds = %bb._0x968
  %evm.mod = call i256 @evm_mod(i256 %_0x968_0x5, i256 16), !notdec.evm !305
  %private.call51 = call i256 @private__0xcd1_0xcd1(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %evm.mod, i256 2438), !notdec.evm !306
  %5 = xor i256 %_0x968_0x0, -1
  %evm.sub52 = add nsw i256 %_0x8f1_0x1, %5
  %evm.mload53 = load i256, ptr %0, align 1, !notdec.evm !307
  %evm.lt54 = icmp ult i256 %evm.sub52, %evm.mload53, !notdec.evm !308
  call void @llvm.assume(i1 %evm.lt54)
  %evm.add57 = add nsw i256 %evm.sub52, 32, !notdec.evm !309
  %evm.add58 = add i256 %evm.add57, %evm.alloc.addr, !notdec.evm !310
  %evm.and59 = and i256 %private.call51, -452312848583266388373324160190187140051835877600158453279131187530910662656, !notdec.evm !311
  %evm.byte = call i256 @evm_byte(i256 0, i256 %evm.and59), !notdec.evm !312
  %notdec.evm.mem.ptr.72 = inttoptr i256 %evm.add58 to ptr
  %notdec.evm.mem.byte.73 = trunc i256 %evm.byte to i8
  %6 = and i256 %evm.byte, 255
  call void @notdec_solidity_memory_array_byte_write(i256 %evm.alloc.addr, i256 %evm.sub52, i256 %6)
  store i8 %notdec.evm.mem.byte.73, ptr %notdec.evm.mem.ptr.72, align 1, !notdec.evm !313
  %evm.div = call i256 @evm_div(i256 %_0x968_0x5, i256 16), !notdec.evm !314
  %evm.add60 = add nuw nsw i256 %_0x968_0x0, 1, !notdec.evm !315
  br label %bb._0x968, !notdec.evm !316

bb._0x928:                                        ; preds = %bb._0x913
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !317
  unreachable, !notdec.evm !317

bb._0x90b:                                        ; preds = %bb._0x8f1
  %evm.add61 = add i256 %_0x8f1_0x1, 1, !notdec.evm !318
  %evm.div62 = call i256 @evm_div(i256 %_0x8f1_0x0, i256 16), !notdec.evm !319
  br label %bb._0x8f1, !notdec.evm !320
}

; Function Attrs: null_pointer_is_valid
define internal i256 @private__0xb56_0xb56(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0xb56arg0x0, i256 %_0xb56arg0x1, i256 %_0xb56arg0x2) #0 !notdec.evm.yul_function_summary !321 !notdec.mlsub.polymorphic_function !20 {
bb._0xb56:
  %notdec.evm.mem.ptr.74 = inttoptr i256 %_0xb56arg0x0 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.74, align 1, !notdec.evm !322
  %notdec.evm.mem.ptr.75 = inttoptr i256 %_0xb56arg0x1 to ptr
  %evm.mload1 = load i256, ptr %notdec.evm.mem.ptr.75, align 1, !notdec.evm !323
  %evm.add = add i256 %evm.mload1, %evm.mload, !notdec.evm !324
  %evm.gt = icmp ult i256 %evm.add, 18446744073709551616
  br i1 %evm.gt, label %bb._0xb7e, label %bb._0xb7a, !notdec.evm !325

bb._0xb7e:                                        ; preds = %bb._0xb56
  %0 = call ptr @calloc_unbounded()
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  store i256 %evm.add, ptr %0, align 1, !notdec.evm !326
  %evm.add4 = add nuw nsw i256 %evm.add, 31, !notdec.evm !327
  %evm.and = and i256 %evm.add4, 36893488147419103200, !notdec.evm !328
  %evm.add5 = add nuw nsw i256 %evm.and, 32, !notdec.evm !329
  call void @notdec_solidity_memory_allocation(i256 %evm.alloc.addr, i256 %evm.add5)
  call void @notdec_evm_finalize_alloc(i256 %evm.alloc.addr, i256 %evm.add5)
  %evm.iszero7 = icmp eq i256 %evm.add, 0, !notdec.evm !330
  br i1 %evm.iszero7, label %bb._0xbb1, label %bb._0xb9d, !notdec.evm !331

bb._0xb9d:                                        ; preds = %bb._0xb7e
  %evm.add10 = add i256 %evm.alloc.addr, 32, !notdec.evm !332
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !333
  %calldata.copy.dst = inttoptr i256 %evm.add10 to ptr
  %calldata.copy.src.base = ptrtoint ptr %calldata to i256
  %calldata.copy.src.addr = add i256 %evm.calldatasize, %calldata.copy.src.base
  %calldata.copy.src = inttoptr i256 %calldata.copy.src.addr to ptr
  call void @llvm.memcpy.p0.p0.i256(ptr align 1 %calldata.copy.dst, ptr align 1 %calldata.copy.src, i256 %evm.add, i1 false)
  br label %bb._0xbb1, !notdec.evm !334

bb._0xbb1:                                        ; preds = %bb._0xb9d, %bb._0xb7e
  br label %bb._0xbc1, !notdec.evm !335

bb._0xbc1:                                        ; preds = %bb._0xbcb, %bb._0xbb1
  %_0xbc1_0x0 = phi i256 [ 0, %bb._0xbb1 ], [ %evm.add48, %bb._0xbcb ], !notdec.evm !336
  %_0xbc1_0x1 = phi i256 [ 0, %bb._0xbb1 ], [ %evm.add57, %bb._0xbcb ], !notdec.evm !337
  %evm.mload12 = load i256, ptr %notdec.evm.mem.ptr.75, align 1
  %evm.lt.not = icmp ult i256 %_0xbc1_0x1, %evm.mload12, !notdec.evm !338
  br i1 %evm.lt.not, label %bb._0xbcb, label %bb._0xc34, !notdec.evm !339

bb._0xc34:                                        ; preds = %bb._0xbc1, %bb._0xc3e
  %_0xc34_0x0 = phi i256 [ %evm.add30, %bb._0xc3e ], [ %_0xbc1_0x0, %bb._0xbc1 ], !notdec.evm !340
  %_0xc34_0x1 = phi i256 [ %evm.add38, %bb._0xc3e ], [ 0, %bb._0xbc1 ], !notdec.evm !341
  %evm.mload17 = load i256, ptr %notdec.evm.mem.ptr.74, align 1
  %evm.lt18.not = icmp ult i256 %_0xc34_0x1, %evm.mload17, !notdec.evm !342
  br i1 %evm.lt18.not, label %bb._0xc3e, label %bb._0xca2, !notdec.evm !343

bb._0xca2:                                        ; preds = %bb._0xc34
  ret i256 %evm.alloc.addr, !notdec.evm !344

bb._0xc3e:                                        ; preds = %bb._0xc34
  %evm.add27 = add i256 %_0xc34_0x1, 32, !notdec.evm !345
  %evm.add28 = add i256 %evm.add27, %_0xb56arg0x0, !notdec.evm !346
  %notdec.evm.mem.ptr.82 = inttoptr i256 %evm.add28 to ptr
  %evm.mload29 = load i256, ptr %notdec.evm.mem.ptr.82, align 1, !notdec.evm !347
  %evm.shr = call i256 @evm_shr(i256 248, i256 %evm.mload29), !notdec.evm !348
  %evm.shl = call i256 @evm_shl(i256 248, i256 %evm.shr), !notdec.evm !349
  %evm.mload31 = load i256, ptr %0, align 1, !notdec.evm !350
  %evm.lt32 = icmp ult i256 %_0xc34_0x0, %evm.mload31, !notdec.evm !351
  call void @llvm.assume(i1 %evm.lt32)
  %evm.add30 = add i256 %_0xc34_0x0, 1, !notdec.evm !352
  %evm.add35 = add i256 %_0xc34_0x0, 32, !notdec.evm !353
  %evm.add36 = add i256 %evm.add35, %evm.alloc.addr, !notdec.evm !354
  %evm.and37 = and i256 %evm.shl, -452312848583266388373324160190187140051835877600158453279131187530910662656, !notdec.evm !355
  %evm.byte = call i256 @evm_byte(i256 0, i256 %evm.and37), !notdec.evm !356
  %notdec.evm.mem.ptr.84 = inttoptr i256 %evm.add36 to ptr
  %notdec.evm.mem.byte.85 = trunc i256 %evm.byte to i8
  %1 = and i256 %evm.byte, 255
  call void @notdec_solidity_memory_array_byte_write(i256 %evm.alloc.addr, i256 %_0xc34_0x0, i256 %1)
  store i8 %notdec.evm.mem.byte.85, ptr %notdec.evm.mem.ptr.84, align 1, !notdec.evm !357
  %evm.add38 = add i256 %_0xc34_0x1, 1, !notdec.evm !358
  br label %bb._0xc34, !notdec.evm !359

bb._0xbcb:                                        ; preds = %bb._0xbc1
  %evm.add43 = add i256 %_0xbc1_0x1, 32, !notdec.evm !360
  %evm.add44 = add i256 %evm.add43, %_0xb56arg0x1, !notdec.evm !361
  %notdec.evm.mem.ptr.87 = inttoptr i256 %evm.add44 to ptr
  %evm.mload45 = load i256, ptr %notdec.evm.mem.ptr.87, align 1, !notdec.evm !362
  %evm.shr46 = call i256 @evm_shr(i256 248, i256 %evm.mload45), !notdec.evm !363
  %evm.shl47 = call i256 @evm_shl(i256 248, i256 %evm.shr46), !notdec.evm !364
  %evm.mload49 = load i256, ptr %0, align 1, !notdec.evm !365
  %evm.lt50 = icmp ult i256 %_0xbc1_0x0, %evm.mload49, !notdec.evm !366
  call void @llvm.assume(i1 %evm.lt50)
  %evm.add48 = add i256 %_0xbc1_0x0, 1, !notdec.evm !367
  %evm.add53 = add i256 %_0xbc1_0x0, 32, !notdec.evm !368
  %evm.add54 = add i256 %evm.add53, %evm.alloc.addr, !notdec.evm !369
  %evm.and55 = and i256 %evm.shl47, -452312848583266388373324160190187140051835877600158453279131187530910662656, !notdec.evm !370
  %evm.byte56 = call i256 @evm_byte(i256 0, i256 %evm.and55), !notdec.evm !371
  %notdec.evm.mem.ptr.89 = inttoptr i256 %evm.add54 to ptr
  %notdec.evm.mem.byte.90 = trunc i256 %evm.byte56 to i8
  %2 = and i256 %evm.byte56, 255
  call void @notdec_solidity_memory_array_byte_write(i256 %evm.alloc.addr, i256 %_0xbc1_0x0, i256 %2)
  store i8 %notdec.evm.mem.byte.90, ptr %notdec.evm.mem.ptr.89, align 1, !notdec.evm !372
  %evm.add57 = add i256 %_0xbc1_0x1, 1, !notdec.evm !373
  br label %bb._0xbc1, !notdec.evm !374

bb._0xb7a:                                        ; preds = %bb._0xb56
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !375
  unreachable, !notdec.evm !375
}

; Function Attrs: null_pointer_is_valid
define internal i256 @private__0xcd1_0xcd1(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0xcd1arg0x0, i256 %_0xcd1arg0x1) #0 !notdec.evm.yul_function_summary !376 !notdec.mlsub.polymorphic_function !20 {
bb._0xcd1:
  %evm.and4 = and i256 %_0xcd1arg0x0, 254
  %evm.gt5 = icmp samesign ugt i256 %evm.and4, 9
  br i1 %evm.gt5, label %bb._0xd20, label %bb._0xcf1, !notdec.evm !377

bb._0xd20:                                        ; preds = %bb._0xcd1
  %evm.and20 = and i256 %_0xcd1arg0x0, 240
  %evm.gt21.not = icmp eq i256 %evm.and20, 0
  br i1 %evm.gt21.not, label %bb._0xd3e, label %bb._0xd70, !notdec.evm !378

bb._0xd70:                                        ; preds = %bb._0xd20
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !379
  unreachable, !notdec.evm !379

bb._0xd3e:                                        ; preds = %bb._0xd20
  %evm.shr = call i256 @evm_shr(i256 248, i256 43874346312576839672212443538448152585028080127215369968075725190498334277632), !notdec.evm !380
  %evm.add = add i256 %evm.shr, %_0xcd1arg0x0, !notdec.evm !381
  %evm.sub = add i256 %evm.add, -10
  %evm.shl = call i256 @evm_shl(i256 248, i256 %evm.sub), !notdec.evm !382
  br label %common.ret

common.ret:                                       ; preds = %bb._0xcf1, %bb._0xd3e
  %common.ret.op = phi i256 [ %evm.shl, %bb._0xd3e ], [ %evm.shl30, %bb._0xcf1 ]
  ret i256 %common.ret.op, !notdec.evm !383

bb._0xcf1:                                        ; preds = %bb._0xcd1
  %evm.shr28 = call i256 @evm_shr(i256 248, i256 21711016731996786641919559689128982722488122124807605757398297001483711807488), !notdec.evm !384
  %evm.add29 = add i256 %evm.shr28, %_0xcd1arg0x0, !notdec.evm !385
  %evm.shl30 = call i256 @evm_shl(i256 248, i256 %evm.add29), !notdec.evm !386
  br label %common.ret
}

; Function Attrs: null_pointer_is_valid
define void @public_tokenSymbol___0xdf(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 !notdec.solidity.nonpayable !160 {
bb._0xdf:
  %notdec.calldata.view = call ptr @notdec_evm_calldata_view(ptr %calldata)
  %evm.callvalue = call i256 @evm_callvalue(ptr %env), !notdec.evm !387, !notdec.solidity.payability_guard !162, !notdec.solidity.rewrite_hidden.payability_guard !162
  call void @notdec_solidity_rewrite_hidden(i256 9745862514924683837)
  call void @notdec_solidity_rewrite_payability_guard(i256 8598331119976828368)
  call void @notdec_solidity_rewrite_hidden(i256 9745862514924683837)
  call void @notdec_solidity_rewrite_payability_guard(i256 12829813975230984100)
  call void @notdec_solidity_rewrite_payability_guard(i256 7817397773529159771)
  call void @notdec_solidity_rewrite_hidden(i256 9745862514924683837)
  call void @notdec_solidity_cfg_rewrite_payability_guard(i256 1330768661709002677)
  br label %bb._0xeb, !notdec.evm !388, !notdec.solidity.payability_guard !164, !notdec.solidity.rewrite_hidden.payability_guard !164

bb._0xeb:                                         ; preds = %bb._0xdf
  %private.call = call { i256, i256 } @private__0x221_0x221(ptr %mem, ptr %notdec.calldata.view, ptr %returndata, ptr %env, i256 244), !notdec.evm !389
  %private.ret = extractvalue { i256, i256 } %private.call, 0, !notdec.evm !389
  %0 = call ptr @calloc_unbounded()
  %evm.alloc.addr = ptrtoint ptr %0 to i256
  %evm.add = add i256 %evm.alloc.addr, 32, !notdec.evm !390
  store i256 32, ptr %0, align 1, !notdec.evm !391
  %notdec.evm.mem.ptr.93 = inttoptr i256 %private.ret to ptr
  %evm.mload2 = load i256, ptr %notdec.evm.mem.ptr.93, align 1, !notdec.evm !392
  %notdec.evm.mem.ptr.94 = inttoptr i256 %evm.add to ptr
  store i256 %evm.mload2, ptr %notdec.evm.mem.ptr.94, align 1, !notdec.evm !393
  %evm.add3 = add i256 %evm.alloc.addr, 64, !notdec.evm !394
  %evm.mload4 = load i256, ptr %notdec.evm.mem.ptr.93, align 1, !notdec.evm !395
  %evm.add5 = add i256 %private.ret, 32, !notdec.evm !396
  br label %bb._0x119, !notdec.evm !397

bb._0x119:                                        ; preds = %bb._0x122, %bb._0xeb
  %_0x119_0x0 = phi i256 [ %evm.add25, %bb._0x122 ], [ 0, %bb._0xeb ], !notdec.evm !398
  %evm.lt.not = icmp ult i256 %_0x119_0x0, %evm.mload4, !notdec.evm !399
  br i1 %evm.lt.not, label %bb._0x122, label %bb._0x134, !notdec.evm !400

bb._0x134:                                        ; preds = %bb._0x119
  %evm.add10 = add i256 %evm.mload4, %evm.add3, !notdec.evm !401
  %evm.and = and i256 %evm.mload4, 31, !notdec.evm !402
  %evm.iszero11 = icmp eq i256 %evm.and, 0, !notdec.evm !403
  br i1 %evm.iszero11, label %bb._0x161, label %bb._0x148, !notdec.evm !404

bb._0x148:                                        ; preds = %bb._0x134
  %evm.sub14 = sub i256 %evm.add10, %evm.and, !notdec.evm !405
  %notdec.evm.mem.ptr.96 = inttoptr i256 %evm.sub14 to ptr
  %evm.mload15 = load i256, ptr %notdec.evm.mem.ptr.96, align 1, !notdec.evm !406
  %evm.sub16 = sub nuw nsw i256 32, %evm.and, !notdec.evm !407
  %evm.exp = call i256 @evm_exp(i256 256, i256 %evm.sub16), !notdec.evm !408
  %evm.not = sub i256 0, %evm.exp
  %evm.and18 = and i256 %evm.mload15, %evm.not, !notdec.evm !409
  store i256 %evm.and18, ptr %notdec.evm.mem.ptr.96, align 1, !notdec.evm !410
  %evm.add19 = add i256 %evm.sub14, 32, !notdec.evm !411
  br label %bb._0x161, !notdec.evm !412

bb._0x161:                                        ; preds = %bb._0x148, %bb._0x134
  %_0x161_0x1 = phi i256 [ %evm.add10, %bb._0x134 ], [ %evm.add19, %bb._0x148 ], !notdec.evm !413
  %evm.sub21 = sub i256 %_0x161_0x1, %evm.alloc.addr, !notdec.evm !414
  call void @evm_return(ptr %mem, i256 %evm.alloc.addr, i256 %evm.sub21), !notdec.evm !415
  ret void, !notdec.evm !415

bb._0x122:                                        ; preds = %bb._0x119
  %evm.add22 = add i256 %evm.add5, %_0x119_0x0, !notdec.evm !416
  %notdec.evm.mem.ptr.99 = inttoptr i256 %evm.add22 to ptr
  %evm.mload23 = load i256, ptr %notdec.evm.mem.ptr.99, align 1, !notdec.evm !417
  %evm.add24 = add i256 %evm.add3, %_0x119_0x0, !notdec.evm !418
  %notdec.evm.mem.ptr.100 = inttoptr i256 %evm.add24 to ptr
  store i256 %evm.mload23, ptr %notdec.evm.mem.ptr.100, align 1, !notdec.evm !419
  %evm.add25 = add i256 %_0x119_0x0, 32, !notdec.evm !420
  br label %bb._0x119, !notdec.evm !421

bb._0xe7:                                         ; No predecessors!
  unreachable, !notdec.evm !422, !notdec.solidity.payability_guard !199, !notdec.solidity.rewrite_hidden.payability_guard !199
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #1

declare void @notdec_solidity_rewrite_payability_guard(i256)

declare void @notdec_solidity_rewrite_hidden(i256)

declare void @notdec_solidity_cfg_rewrite_payability_guard(i256)

declare void @notdec_solidity_memory_allocation(i256, i256)

declare noalias ptr @calloc(i256, i256)

declare noalias ptr @calloc_unbounded()

declare void @notdec_solidity_memory_array_byte_write(i256, i256, i256)

declare void @notdec_evm_finalize_alloc(i256, i256)

declare !notdec.mlsub.polymorphic_function !423 ptr @notdec_evm_calldata_view(ptr)

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i256(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i256, i1 immarg) #2

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #2 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x7", !"op=CALLDATASIZE", !"evm.pc=0x7"}
!2 = !{!"tac=0x8", !"op=LT", !"evm.pc=0x8"}
!3 = !{!"tac=0xc", !"op=JUMPI", !"evm.pc=0xc"}
!4 = !{!"tac=0x44", !"op=CALLDATASIZE", !"evm.pc=0x44"}
!5 = !{!"tac=0x48", !"op=JUMPI", !"evm.pc=0x48"}
!6 = !{!"tac=0x6127c", !"op=CALLPRIVATE", !"evm.pc=0x49"}
!7 = !{!"tac=0x1c7c", !"op=REVERT", !"evm.pc=0x4e"}
!8 = !{!"tac=0xf", !"op=CALLDATALOAD", !"evm.pc=0xf"}
!9 = !{!"tac=0x12", !"op=SHR", !"evm.pc=0x12"}
!10 = !{!"tac=0x61c9c", !"op=CALLPRIVATE", !"evm.pc=0x4f"}
!11 = !{!"tac=0x6269c", !"op=CALLPRIVATE", !"evm.pc=0xdf"}
!12 = !{!"tac=0x6309c", !"op=CALLPRIVATE", !"evm.pc=0x16f"}
!13 = !{!"tac=0x63a9c", !"op=CALLPRIVATE", !"evm.pc=0x179"}
!14 = !{!"tac=0x1c58", !"op=REVERT", !"evm.pc=0x4e"}
!15 = !{!"tac=0x176", !"op=CALLPRIVATE", !"evm.pc=0x176"}
!16 = !{!"tac=0x178", !"op=STOP", !"evm.pc=0x178"}
!17 = !{!"tac=0x180", !"op=CALLPRIVATE", !"evm.pc=0x180"}
!18 = !{!"tac=0x182", !"op=STOP", !"evm.pc=0x182"}
!19 = !{!"v1;params=1;ret=2;label=-1;loads=0;unresolved=0;yulcalls=0;labelconst=0/0;calldata="}
!20 = !{!"calldata_buffer"}
!21 = !{!"tac=0x187", !"op=SLOAD", !"evm.pc=0x187"}
!22 = !{!"tac=0x194", !"op=AND", !"evm.pc=0x194"}
!23 = !{!"tac=0x198", !"op=DIV", !"evm.pc=0x198"}
!24 = !{!"tac=0x19c", !"op=ADD", !"evm.pc=0x19c"}
!25 = !{!"tac=0x1a1", !"op=DIV", !"evm.pc=0x1a1"}
!26 = !{!"tac=0x1a5", !"op=ADD", !"evm.pc=0x1a5"}
!27 = !{!"tac=0x1b5", !"op=MSTORE", !"evm.pc=0x1b5"}
!28 = !{!"tac=0x1b8", !"op=ADD", !"evm.pc=0x1b8"}
!29 = !{!"tac=0x1bb", !"op=SLOAD", !"evm.pc=0x1bb"}
!30 = !{!"tac=0x1c8", !"op=AND", !"evm.pc=0x1c8"}
!31 = !{!"tac=0x1cc", !"op=DIV", !"evm.pc=0x1cc"}
!32 = !{!"tac=0x1ce", !"op=ISZERO", !"evm.pc=0x1ce"}
!33 = !{!"tac=0x1d2", !"op=JUMPI", !"evm.pc=0x1d2"}
!34 = !{!"tac=0x2f297", !"op=RETURNPRIVATE", !"evm.pc=0x220"}
!35 = !{!"tac=0x1d6", !"op=LT", !"evm.pc=0x1d6"}
!36 = !{!"tac=0x1da", !"op=JUMPI", !"evm.pc=0x1da"}
!37 = !{!"tac=0x1f0", !"op=ADD", !"evm.pc=0x1f0"}
!38 = !{!"tac=0x1f5", !"op=MSTORE", !"evm.pc=0x1f5"}
!39 = !{!"tac=0x1fa", !"op=SHA3", !"evm.pc=0x1fa"}
!40 = !{!"tac=0x6138", !"op=JUMP", !"evm.pc=0x1fc"}
!41 = !{!"tac=0x1fc_0x0", !"op=PHI"}
!42 = !{!"tac=0x1fc_0x1", !"op=PHI"}
!43 = !{!"tac=0x1fe", !"op=SLOAD", !"evm.pc=0x1fe"}
!44 = !{!"tac=0x200", !"op=MSTORE", !"evm.pc=0x200"}
!45 = !{!"tac=0x204", !"op=ADD", !"evm.pc=0x204"}
!46 = !{!"tac=0x208", !"op=ADD", !"evm.pc=0x208"}
!47 = !{!"tac=0x20b", !"op=GT", !"evm.pc=0x20b"}
!48 = !{!"tac=0x20f", !"op=JUMPI", !"evm.pc=0x20f"}
!49 = !{!"tac=0x1e0", !"op=SLOAD", !"evm.pc=0x1e0"}
!50 = !{!"tac=0x1e1", !"op=DIV", !"evm.pc=0x1e1"}
!51 = !{!"tac=0x1e4", !"op=MSTORE", !"evm.pc=0x1e4"}
!52 = !{!"tac=0x225", !"op=SLOAD", !"evm.pc=0x225"}
!53 = !{!"tac=0x232", !"op=AND", !"evm.pc=0x232"}
!54 = !{!"tac=0x236", !"op=DIV", !"evm.pc=0x236"}
!55 = !{!"tac=0x23a", !"op=ADD", !"evm.pc=0x23a"}
!56 = !{!"tac=0x23f", !"op=DIV", !"evm.pc=0x23f"}
!57 = !{!"tac=0x243", !"op=ADD", !"evm.pc=0x243"}
!58 = !{!"tac=0x253", !"op=MSTORE", !"evm.pc=0x253"}
!59 = !{!"tac=0x256", !"op=ADD", !"evm.pc=0x256"}
!60 = !{!"tac=0x259", !"op=SLOAD", !"evm.pc=0x259"}
!61 = !{!"tac=0x266", !"op=AND", !"evm.pc=0x266"}
!62 = !{!"tac=0x26a", !"op=DIV", !"evm.pc=0x26a"}
!63 = !{!"tac=0x26c", !"op=ISZERO", !"evm.pc=0x26c"}
!64 = !{!"tac=0x270", !"op=JUMPI", !"evm.pc=0x270"}
!65 = !{!"tac=0x2f2e5", !"op=RETURNPRIVATE", !"evm.pc=0x2be"}
!66 = !{!"tac=0x274", !"op=LT", !"evm.pc=0x274"}
!67 = !{!"tac=0x278", !"op=JUMPI", !"evm.pc=0x278"}
!68 = !{!"tac=0x28e", !"op=ADD", !"evm.pc=0x28e"}
!69 = !{!"tac=0x293", !"op=MSTORE", !"evm.pc=0x293"}
!70 = !{!"tac=0x298", !"op=SHA3", !"evm.pc=0x298"}
!71 = !{!"tac=0x7538", !"op=JUMP", !"evm.pc=0x29a"}
!72 = !{!"tac=0x29a_0x0", !"op=PHI"}
!73 = !{!"tac=0x29a_0x1", !"op=PHI"}
!74 = !{!"tac=0x29c", !"op=SLOAD", !"evm.pc=0x29c"}
!75 = !{!"tac=0x29e", !"op=MSTORE", !"evm.pc=0x29e"}
!76 = !{!"tac=0x2a2", !"op=ADD", !"evm.pc=0x2a2"}
!77 = !{!"tac=0x2a6", !"op=ADD", !"evm.pc=0x2a6"}
!78 = !{!"tac=0x2a9", !"op=GT", !"evm.pc=0x2a9"}
!79 = !{!"tac=0x2ad", !"op=JUMPI", !"evm.pc=0x2ad"}
!80 = !{!"tac=0x27e", !"op=SLOAD", !"evm.pc=0x27e"}
!81 = !{!"tac=0x27f", !"op=DIV", !"evm.pc=0x27f"}
!82 = !{!"tac=0x282", !"op=MSTORE", !"evm.pc=0x282"}
!83 = !{!"v1;params=0;ret=0;label=4;loads=0;unresolved=0;yulcalls=3;labelconst=1/1;calldata="}
!84 = !{!"tac=0x2e8", !"op=ADD", !"evm.pc=0x2e8"}
!85 = !{!"tac=0x2ed", !"op=MSTORE", !"evm.pc=0x2ed"}
!86 = !{!"tac=0x2f1", !"op=MSTORE", !"evm.pc=0x2f1"}
!87 = !{!"tac=0x2f4", !"op=ADD", !"evm.pc=0x2f4"}
!88 = !{!"tac=0x2fc", !"op=CODECOPY", !"evm.pc=0x2fc"}
!89 = !{!"tac=0x30a", !"op=LOG1", !"evm.pc=0x30a"}
!90 = !{!"tac=0x311", !"op=CALLPRIVATE", !"evm.pc=0x311"}
!91 = !{!"tac=0x313", !"op=ISZERO", !"evm.pc=0x313"}
!92 = !{!"tac=0x317", !"op=JUMPI", !"evm.pc=0x317"}
!93 = !{!"tac=0x371", !"op=CALLPRIVATE", !"evm.pc=0x371"}
!94 = !{!"tac=0x388", !"op=AND", !"evm.pc=0x388"}
!95 = !{!"tac=0x38c", !"op=SELFBALANCE", !"evm.pc=0x38c"}
!96 = !{!"tac=0x38f", !"op=ISZERO", !"evm.pc=0x38f"}
!97 = !{!"tac=0x3a1", !"op=CALL", !"evm.pc=0x3a1"}
!98 = !{!"tac=0x3a7", !"op=ISZERO", !"evm.pc=0x3a7"}
!99 = !{!"tac=0x3ad", !"op=JUMPI", !"evm.pc=0x3ad"}
!100 = !{!"tac=0x2f4b7", !"op=RETURNPRIVATE", !"evm.pc=0x3ba"}
!101 = !{!"tac=0x3ae", !"op=RETURNDATASIZE", !"evm.pc=0x3ae"}
!102 = !{!"tac=0x3b2", !"op=RETURNDATACOPY", !"evm.pc=0x3b2"}
!103 = !{!"tac=0x3b3", !"op=RETURNDATASIZE", !"evm.pc=0x3b3"}
!104 = !{!"tac=0x3b6", !"op=REVERT", !"evm.pc=0x3b6"}
!105 = !{!"tac=0x31e", !"op=CALLPRIVATE", !"evm.pc=0x31e"}
!106 = !{!"tac=0x335", !"op=AND", !"evm.pc=0x335"}
!107 = !{!"tac=0x339", !"op=SELFBALANCE", !"evm.pc=0x339"}
!108 = !{!"tac=0x33c", !"op=ISZERO", !"evm.pc=0x33c"}
!109 = !{!"tac=0x34e", !"op=CALL", !"evm.pc=0x34e"}
!110 = !{!"tac=0x354", !"op=ISZERO", !"evm.pc=0x354"}
!111 = !{!"tac=0x35a", !"op=JUMPI", !"evm.pc=0x35a"}
!112 = !{!"tac=0x35b", !"op=RETURNDATASIZE", !"evm.pc=0x35b"}
!113 = !{!"tac=0x35f", !"op=RETURNDATACOPY", !"evm.pc=0x35f"}
!114 = !{!"tac=0x360", !"op=RETURNDATASIZE", !"evm.pc=0x360"}
!115 = !{!"tac=0x363", !"op=REVERT", !"evm.pc=0x363"}
!116 = !{!"tac=0x3e4", !"op=ADD", !"evm.pc=0x3e4"}
!117 = !{!"tac=0x3e9", !"op=MSTORE", !"evm.pc=0x3e9"}
!118 = !{!"tac=0x3ed", !"op=MSTORE", !"evm.pc=0x3ed"}
!119 = !{!"tac=0x3f0", !"op=ADD", !"evm.pc=0x3f0"}
!120 = !{!"tac=0x3f8", !"op=CODECOPY", !"evm.pc=0x3f8"}
!121 = !{!"tac=0x406", !"op=LOG1", !"evm.pc=0x406"}
!122 = !{!"tac=0x40d", !"op=CALLPRIVATE", !"evm.pc=0x40d"}
!123 = !{!"tac=0x40f", !"op=ISZERO", !"evm.pc=0x40f"}
!124 = !{!"tac=0x413", !"op=JUMPI", !"evm.pc=0x413"}
!125 = !{!"tac=0x46d", !"op=CALLPRIVATE", !"evm.pc=0x46d"}
!126 = !{!"tac=0x484", !"op=AND", !"evm.pc=0x484"}
!127 = !{!"tac=0x488", !"op=SELFBALANCE", !"evm.pc=0x488"}
!128 = !{!"tac=0x48b", !"op=ISZERO", !"evm.pc=0x48b"}
!129 = !{!"tac=0x49d", !"op=CALL", !"evm.pc=0x49d"}
!130 = !{!"tac=0x4a3", !"op=ISZERO", !"evm.pc=0x4a3"}
!131 = !{!"tac=0x4a9", !"op=JUMPI", !"evm.pc=0x4a9"}
!132 = !{!"tac=0x2f4d8", !"op=RETURNPRIVATE", !"evm.pc=0x4b6"}
!133 = !{!"tac=0x4aa", !"op=RETURNDATASIZE", !"evm.pc=0x4aa"}
!134 = !{!"tac=0x4ae", !"op=RETURNDATACOPY", !"evm.pc=0x4ae"}
!135 = !{!"tac=0x4af", !"op=RETURNDATASIZE", !"evm.pc=0x4af"}
!136 = !{!"tac=0x4b2", !"op=REVERT", !"evm.pc=0x4b2"}
!137 = !{!"tac=0x41a", !"op=CALLPRIVATE", !"evm.pc=0x41a"}
!138 = !{!"tac=0x431", !"op=AND", !"evm.pc=0x431"}
!139 = !{!"tac=0x435", !"op=SELFBALANCE", !"evm.pc=0x435"}
!140 = !{!"tac=0x438", !"op=ISZERO", !"evm.pc=0x438"}
!141 = !{!"tac=0x44a", !"op=CALL", !"evm.pc=0x44a"}
!142 = !{!"tac=0x450", !"op=ISZERO", !"evm.pc=0x450"}
!143 = !{!"tac=0x456", !"op=JUMPI", !"evm.pc=0x456"}
!144 = !{!"tac=0x457", !"op=RETURNDATASIZE", !"evm.pc=0x457"}
!145 = !{!"tac=0x45b", !"op=RETURNDATACOPY", !"evm.pc=0x45b"}
!146 = !{!"tac=0x45c", !"op=RETURNDATASIZE", !"evm.pc=0x45c"}
!147 = !{!"tac=0x45f", !"op=REVERT", !"evm.pc=0x45f"}
!148 = !{!"v1;params=0;ret=1;label=4;loads=0;unresolved=0;yulcalls=0;labelconst=2/2;calldata="}
!149 = !{!"tac=0x4bc", !"op=SLOAD", !"evm.pc=0x4bc"}
!150 = !{!"tac=0x4bd", !"op=SELFBALANCE", !"evm.pc=0x4bd"}
!151 = !{!"tac=0x4be", !"op=GT", !"evm.pc=0x4be"}
!152 = !{!"tac=0x2f4fa", !"op=RETURNPRIVATE", !"evm.pc=0x4d3"}
!153 = !{!"v1;params=0;ret=1;label=4;loads=0;unresolved=0;yulcalls=2;labelconst=2/2;calldata="}
!154 = !{!"tac=0x4e0", !"op=CALLPRIVATE", !"evm.pc=0x4e0"}
!155 = !{!"tac=0x4e5", !"op=CALLPRIVATE", !"evm.pc=0x4e5"}
!156 = !{!"tac=0x4ea", !"op=RETURNPRIVATE", !"evm.pc=0x4ea"}
!157 = !{!"tac=0x4f7", !"op=CALLPRIVATE", !"evm.pc=0x4f7"}
!158 = !{!"tac=0x4fc", !"op=CALLPRIVATE", !"evm.pc=0x4fc"}
!159 = !{!"tac=0x501", !"op=RETURNPRIVATE", !"evm.pc=0x501"}
!160 = !{!"true"}
!161 = !{!"tac=0x50", !"op=CALLVALUE", !"evm.pc=0x50"}
!162 = !{!"callvalue"}
!163 = !{!"tac=0x56", !"op=JUMPI", !"evm.pc=0x56"}
!164 = !{!"branch"}
!165 = !{!"tac=0x63", !"op=CALLPRIVATE", !"evm.pc=0x63"}
!166 = !{!"tac=0x6c", !"op=ADD", !"evm.pc=0x6c"}
!167 = !{!"tac=0x71", !"op=MSTORE", !"evm.pc=0x71"}
!168 = !{!"tac=0x75", !"op=MLOAD", !"evm.pc=0x75"}
!169 = !{!"tac=0x77", !"op=MSTORE", !"evm.pc=0x77"}
!170 = !{!"tac=0x7a", !"op=ADD", !"evm.pc=0x7a"}
!171 = !{!"tac=0x7e", !"op=MLOAD", !"evm.pc=0x7e"}
!172 = !{!"tac=0x82", !"op=ADD", !"evm.pc=0x82"}
!173 = !{!"tac=0x3938", !"op=JUMP", !"evm.pc=0x89"}
!174 = !{!"tac=0x89_0x0", !"op=PHI"}
!175 = !{!"tac=0x8c", !"op=LT", !"evm.pc=0x8c"}
!176 = !{!"tac=0x91", !"op=JUMPI", !"evm.pc=0x91"}
!177 = !{!"tac=0xad", !"op=ADD", !"evm.pc=0xad"}
!178 = !{!"tac=0xb1", !"op=AND", !"evm.pc=0xb1"}
!179 = !{!"tac=0xb3", !"op=ISZERO", !"evm.pc=0xb3"}
!180 = !{!"tac=0xb7", !"op=JUMPI", !"evm.pc=0xb7"}
!181 = !{!"tac=0xba", !"op=SUB", !"evm.pc=0xba"}
!182 = !{!"tac=0xbc", !"op=MLOAD", !"evm.pc=0xbc"}
!183 = !{!"tac=0xc2", !"op=SUB", !"evm.pc=0xc2"}
!184 = !{!"tac=0xc6", !"op=EXP", !"evm.pc=0xc6"}
!185 = !{!"tac=0xc9", !"op=AND", !"evm.pc=0xc9"}
!186 = !{!"tac=0xcb", !"op=MSTORE", !"evm.pc=0xcb"}
!187 = !{!"tac=0xce", !"op=ADD", !"evm.pc=0xce"}
!188 = !{!"tac=0x4338", !"op=JUMP", !"evm.pc=0xd1"}
!189 = !{!"tac=0xd1_0x1", !"op=PHI"}
!190 = !{!"tac=0xdc", !"op=SUB", !"evm.pc=0xdc"}
!191 = !{!"tac=0xde", !"op=RETURN", !"evm.pc=0xde"}
!192 = !{!"tac=0x94", !"op=ADD", !"evm.pc=0x94"}
!193 = !{!"tac=0x95", !"op=MLOAD", !"evm.pc=0x95"}
!194 = !{!"tac=0x98", !"op=ADD", !"evm.pc=0x98"}
!195 = !{!"tac=0x99", !"op=MSTORE", !"evm.pc=0x99"}
!196 = !{!"tac=0x9d", !"op=ADD", !"evm.pc=0x9d"}
!197 = !{!"tac=0xa3", !"op=JUMP", !"evm.pc=0xa3"}
!198 = !{!"tac=0x5a", !"op=REVERT", !"evm.pc=0x5a"}
!199 = !{!"revert"}
!200 = !{!"v1;params=0;ret=1;label=4;loads=0;unresolved=0;yulcalls=17;labelconst=2/2;calldata="}
!201 = !{!"tac=0x517", !"op=MSTORE", !"evm.pc=0x517"}
!202 = !{!"tac=0x51a", !"op=ADD", !"evm.pc=0x51a"}
!203 = !{!"tac=0x53d", !"op=MSTORE", !"evm.pc=0x53d"}
!204 = !{!"tac=0x54d", !"op=CALLPRIVATE", !"evm.pc=0x54d"}
!205 = !{!"tac=0x552", !"op=CALLPRIVATE", !"evm.pc=0x552"}
!206 = !{!"tac=0x5a7", !"op=CALLPRIVATE", !"evm.pc=0x5a7"}
!207 = !{!"tac=0x5ac", !"op=CALLPRIVATE", !"evm.pc=0x5ac"}
!208 = !{!"tac=0x5bc", !"op=CALLPRIVATE", !"evm.pc=0x5bc"}
!209 = !{!"tac=0x5c5", !"op=CALLPRIVATE", !"evm.pc=0x5c5"}
!210 = !{!"tac=0x5ca", !"op=CALLPRIVATE", !"evm.pc=0x5ca"}
!211 = !{!"tac=0x5da", !"op=CALLPRIVATE", !"evm.pc=0x5da"}
!212 = !{!"tac=0x5e3", !"op=CALLPRIVATE", !"evm.pc=0x5e3"}
!213 = !{!"tac=0x5e8", !"op=CALLPRIVATE", !"evm.pc=0x5e8"}
!214 = !{!"tac=0x5f8", !"op=CALLPRIVATE", !"evm.pc=0x5f8"}
!215 = !{!"tac=0x601", !"op=CALLPRIVATE", !"evm.pc=0x601"}
!216 = !{!"tac=0x606", !"op=CALLPRIVATE", !"evm.pc=0x606"}
!217 = !{!"tac=0x617", !"op=CALLPRIVATE", !"evm.pc=0x617"}
!218 = !{!"tac=0x621", !"op=CALLPRIVATE", !"evm.pc=0x621"}
!219 = !{!"tac=0x626", !"op=CALLPRIVATE", !"evm.pc=0x626"}
!220 = !{!"tac=0x63d", !"op=MSTORE", !"evm.pc=0x63d"}
!221 = !{!"tac=0x640", !"op=ADD", !"evm.pc=0x640"}
!222 = !{!"tac=0x663", !"op=MSTORE", !"evm.pc=0x663"}
!223 = !{!"tac=0x669", !"op=CALLPRIVATE", !"evm.pc=0x669"}
!224 = !{!"tac=0x67f", !"op=RETURNPRIVATE", !"evm.pc=0x67f"}
!225 = !{!"tac=0x49", !"op=STOP", !"evm.pc=0x49"}
!226 = !{!"v1;params=1;ret=1;label=5;loads=0;unresolved=0;yulcalls=0;labelconst=2/2;calldata="}
!227 = !{!"tac=0xa738", !"op=JUMP", !"evm.pc=0x696"}
!228 = !{!"tac=0x696_0x0", !"op=PHI"}
!229 = !{!"tac=0x696_0x3", !"op=PHI"}
!230 = !{!"tac=0x69f", !"op=JUMPI", !"evm.pc=0x69f"}
!231 = !{!"tac=0x8d8", !"op=RETURNPRIVATE", !"evm.pc=0x8d8"}
!232 = !{!"tac=0x6aa", !"op=MLOAD", !"evm.pc=0x6aa"}
!233 = !{!"tac=0x6ac", !"op=LT", !"evm.pc=0x6ac"}
!234 = !{!"tac=0x6b5", !"op=ADD", !"evm.pc=0x6b5"}
!235 = !{!"tac=0x6b6", !"op=ADD", !"evm.pc=0x6b6"}
!236 = !{!"tac=0x6b7", !"op=MLOAD", !"evm.pc=0x6b7"}
!237 = !{!"tac=0x6ba", !"op=SHR", !"evm.pc=0x6ba"}
!238 = !{!"tac=0x6bd", !"op=SHL", !"evm.pc=0x6bd"}
!239 = !{!"tac=0x6c0", !"op=SHR", !"evm.pc=0x6c0"}
!240 = !{!"tac=0x6c3", !"op=AND", !"evm.pc=0x6c3"}
!241 = !{!"tac=0x6cc", !"op=MLOAD", !"evm.pc=0x6cc"}
!242 = !{!"tac=0x6ce", !"op=LT", !"evm.pc=0x6ce"}
!243 = !{!"tac=0x6d7", !"op=ADD", !"evm.pc=0x6d7"}
!244 = !{!"tac=0x6d8", !"op=ADD", !"evm.pc=0x6d8"}
!245 = !{!"tac=0x6d9", !"op=MLOAD", !"evm.pc=0x6d9"}
!246 = !{!"tac=0x6dc", !"op=SHR", !"evm.pc=0x6dc"}
!247 = !{!"tac=0x6df", !"op=SHL", !"evm.pc=0x6df"}
!248 = !{!"tac=0x6e2", !"op=SHR", !"evm.pc=0x6e2"}
!249 = !{!"tac=0x6e5", !"op=AND", !"evm.pc=0x6e5"}
!250 = !{!"tac=0x72a", !"op=JUMPI", !"evm.pc=0x72a"}
!251 = !{!"tac=0x778", !"op=JUMPI", !"evm.pc=0x778"}
!252 = !{!"tac=0xd938", !"op=JUMP", !"evm.pc=0x7ce"}
!253 = !{!"tac=0x782", !"op=JUMP", !"evm.pc=0x782"}
!254 = !{!"tac=0x734", !"op=JUMP", !"evm.pc=0x734"}
!255 = !{!"tac=0x7cf_0x2", !"op=PHI"}
!256 = !{!"tac=0x812", !"op=JUMPI", !"evm.pc=0x812"}
!257 = !{!"tac=0x860", !"op=JUMPI", !"evm.pc=0x860"}
!258 = !{!"tac=0x11538", !"op=JUMP", !"evm.pc=0x8b6"}
!259 = !{!"tac=0x86a", !"op=JUMP", !"evm.pc=0x86a"}
!260 = !{!"tac=0x81c", !"op=JUMP", !"evm.pc=0x81c"}
!261 = !{!"tac=0x8b7_0x1", !"op=PHI"}
!262 = !{!"tac=0x8bd", !"op=ADD", !"evm.pc=0x8bd"}
!263 = !{!"tac=0x8bf", !"op=ADD", !"evm.pc=0x8bf"}
!264 = !{!"tac=0x8c5", !"op=ADD", !"evm.pc=0x8c5"}
!265 = !{!"tac=0x8cb", !"op=JUMP", !"evm.pc=0x8cb"}
!266 = !{!"v1;params=1;ret=1;label=5;loads=0;unresolved=0;yulcalls=5;labelconst=8/8;calldata="}
!267 = !{!"tac=0x12938", !"op=JUMP", !"evm.pc=0x8f1"}
!268 = !{!"tac=0x8f1_0x0", !"op=PHI"}
!269 = !{!"tac=0x8f1_0x1", !"op=PHI"}
!270 = !{!"tac=0x8f5", !"op=EQ", !"evm.pc=0x8f5"}
!271 = !{!"tac=0x8f9", !"op=JUMPI", !"evm.pc=0x8f9"}
!272 = !{!"tac=0x927", !"op=JUMPI", !"evm.pc=0x927"}
!273 = !{!"tac=0x934", !"op=MSTORE", !"evm.pc=0x934"}
!274 = !{!"tac=0x938", !"op=ADD", !"evm.pc=0x938"}
!275 = !{!"tac=0x93c", !"op=AND", !"evm.pc=0x93c"}
!276 = !{!"tac=0x93f", !"op=ADD", !"evm.pc=0x93f"}
!277 = !{!"tac=0x946", !"op=ISZERO", !"evm.pc=0x946"}
!278 = !{!"tac=0x94a", !"op=JUMPI", !"evm.pc=0x94a"}
!279 = !{!"tac=0x94e", !"op=ADD", !"evm.pc=0x94e"}
!280 = !{!"tac=0x954", !"op=CALLDATASIZE", !"evm.pc=0x954"}
!281 = !{!"tac=0x13338", !"op=JUMP", !"evm.pc=0x95f"}
!282 = !{!"tac=0x13d38", !"op=JUMP", !"evm.pc=0x968"}
!283 = !{!"tac=0x968_0x0", !"op=PHI"}
!284 = !{!"tac=0x968_0x5", !"op=PHI"}
!285 = !{!"tac=0x96b", !"op=LT", !"evm.pc=0x96b"}
!286 = !{!"tac=0x970", !"op=JUMPI", !"evm.pc=0x970"}
!287 = !{!"tac=0x9e3", !"op=MLOAD", !"evm.pc=0x9e3"}
!288 = !{!"tac=0x2f51e", !"op=RETURNPRIVATE", !"evm.pc=0xb55"}
!289 = !{!"tac=0xb0d", !"op=MSTORE", !"evm.pc=0xb0d"}
!290 = !{!"tac=0xb10", !"op=ADD", !"evm.pc=0xb10"}
!291 = !{!"tac=0xb33", !"op=MSTORE", !"evm.pc=0xb33"}
!292 = !{!"tac=0xb39", !"op=CALLPRIVATE", !"evm.pc=0xb39"}
!293 = !{!"tac=0xab4", !"op=MSTORE", !"evm.pc=0xab4"}
!294 = !{!"tac=0xab7", !"op=ADD", !"evm.pc=0xab7"}
!295 = !{!"tac=0xada", !"op=MSTORE", !"evm.pc=0xada"}
!296 = !{!"tac=0xae0", !"op=CALLPRIVATE", !"evm.pc=0xae0"}
!297 = !{!"tac=0xa5b", !"op=MSTORE", !"evm.pc=0xa5b"}
!298 = !{!"tac=0xa5e", !"op=ADD", !"evm.pc=0xa5e"}
!299 = !{!"tac=0xa81", !"op=MSTORE", !"evm.pc=0xa81"}
!300 = !{!"tac=0xa87", !"op=CALLPRIVATE", !"evm.pc=0xa87"}
!301 = !{!"tac=0xa02", !"op=MSTORE", !"evm.pc=0xa02"}
!302 = !{!"tac=0xa05", !"op=ADD", !"evm.pc=0xa05"}
!303 = !{!"tac=0xa28", !"op=MSTORE", !"evm.pc=0xa28"}
!304 = !{!"tac=0xa2e", !"op=CALLPRIVATE", !"evm.pc=0xa2e"}
!305 = !{!"tac=0x97b", !"op=MOD", !"evm.pc=0x97b"}
!306 = !{!"tac=0x985", !"op=CALLPRIVATE", !"evm.pc=0x985"}
!307 = !{!"tac=0x98f", !"op=MLOAD", !"evm.pc=0x98f"}
!308 = !{!"tac=0x991", !"op=LT", !"evm.pc=0x991"}
!309 = !{!"tac=0x99a", !"op=ADD", !"evm.pc=0x99a"}
!310 = !{!"tac=0x99b", !"op=ADD", !"evm.pc=0x99b"}
!311 = !{!"tac=0x9be", !"op=AND", !"evm.pc=0x9be"}
!312 = !{!"tac=0x9c3", !"op=BYTE", !"evm.pc=0x9c3"}
!313 = !{!"tac=0x9c5", !"op=MSTORE8", !"evm.pc=0x9c5"}
!314 = !{!"tac=0x9d1", !"op=DIV", !"evm.pc=0x9d1"}
!315 = !{!"tac=0x9d7", !"op=ADD", !"evm.pc=0x9d7"}
!316 = !{!"tac=0x9dd", !"op=JUMP", !"evm.pc=0x9dd"}
!317 = !{!"tac=0x92b", !"op=REVERT", !"evm.pc=0x92b"}
!318 = !{!"tac=0x8fe", !"op=ADD", !"evm.pc=0x8fe"}
!319 = !{!"tac=0x90c", !"op=DIV", !"evm.pc=0x90c"}
!320 = !{!"tac=0x912", !"op=JUMP", !"evm.pc=0x912"}
!321 = !{!"v1;params=2;ret=1;label=6;loads=0;unresolved=0;yulcalls=0;labelconst=13/13;calldata="}
!322 = !{!"tac=0xb65", !"op=MLOAD", !"evm.pc=0xb65"}
!323 = !{!"tac=0xb67", !"op=MLOAD", !"evm.pc=0xb67"}
!324 = !{!"tac=0xb68", !"op=ADD", !"evm.pc=0xb68"}
!325 = !{!"tac=0xb79", !"op=JUMPI", !"evm.pc=0xb79"}
!326 = !{!"tac=0xb86", !"op=MSTORE", !"evm.pc=0xb86"}
!327 = !{!"tac=0xb8a", !"op=ADD", !"evm.pc=0xb8a"}
!328 = !{!"tac=0xb8e", !"op=AND", !"evm.pc=0xb8e"}
!329 = !{!"tac=0xb91", !"op=ADD", !"evm.pc=0xb91"}
!330 = !{!"tac=0xb98", !"op=ISZERO", !"evm.pc=0xb98"}
!331 = !{!"tac=0xb9c", !"op=JUMPI", !"evm.pc=0xb9c"}
!332 = !{!"tac=0xba0", !"op=ADD", !"evm.pc=0xba0"}
!333 = !{!"tac=0xba6", !"op=CALLDATASIZE", !"evm.pc=0xba6"}
!334 = !{!"tac=0x15138", !"op=JUMP", !"evm.pc=0xbb1"}
!335 = !{!"tac=0x15b38", !"op=JUMP", !"evm.pc=0xbc1"}
!336 = !{!"tac=0xbc1_0x0", !"op=PHI"}
!337 = !{!"tac=0xbc1_0x1", !"op=PHI"}
!338 = !{!"tac=0xbc5", !"op=LT", !"evm.pc=0xbc5"}
!339 = !{!"tac=0xbca", !"op=JUMPI", !"evm.pc=0xbca"}
!340 = !{!"tac=0xc34_0x0", !"op=PHI"}
!341 = !{!"tac=0xc34_0x1", !"op=PHI"}
!342 = !{!"tac=0xc38", !"op=LT", !"evm.pc=0xc38"}
!343 = !{!"tac=0xc3d", !"op=JUMPI", !"evm.pc=0xc3d"}
!344 = !{!"tac=0xcb0", !"op=RETURNPRIVATE", !"evm.pc=0xcb0"}
!345 = !{!"tac=0xc4c", !"op=ADD", !"evm.pc=0xc4c"}
!346 = !{!"tac=0xc4d", !"op=ADD", !"evm.pc=0xc4d"}
!347 = !{!"tac=0xc4e", !"op=MLOAD", !"evm.pc=0xc4e"}
!348 = !{!"tac=0xc51", !"op=SHR", !"evm.pc=0xc51"}
!349 = !{!"tac=0xc54", !"op=SHL", !"evm.pc=0xc54"}
!350 = !{!"tac=0xc5e", !"op=MLOAD", !"evm.pc=0xc5e"}
!351 = !{!"tac=0xc60", !"op=LT", !"evm.pc=0xc60"}
!352 = !{!"tac=0xc5a", !"op=ADD", !"evm.pc=0xc5a"}
!353 = !{!"tac=0xc69", !"op=ADD", !"evm.pc=0xc69"}
!354 = !{!"tac=0xc6a", !"op=ADD", !"evm.pc=0xc6a"}
!355 = !{!"tac=0xc8d", !"op=AND", !"evm.pc=0xc8d"}
!356 = !{!"tac=0xc92", !"op=BYTE", !"evm.pc=0xc92"}
!357 = !{!"tac=0xc94", !"op=MSTORE8", !"evm.pc=0xc94"}
!358 = !{!"tac=0xc9a", !"op=ADD", !"evm.pc=0xc9a"}
!359 = !{!"tac=0xca1", !"op=JUMP", !"evm.pc=0xca1"}
!360 = !{!"tac=0xbd9", !"op=ADD", !"evm.pc=0xbd9"}
!361 = !{!"tac=0xbda", !"op=ADD", !"evm.pc=0xbda"}
!362 = !{!"tac=0xbdb", !"op=MLOAD", !"evm.pc=0xbdb"}
!363 = !{!"tac=0xbde", !"op=SHR", !"evm.pc=0xbde"}
!364 = !{!"tac=0xbe1", !"op=SHL", !"evm.pc=0xbe1"}
!365 = !{!"tac=0xbeb", !"op=MLOAD", !"evm.pc=0xbeb"}
!366 = !{!"tac=0xbed", !"op=LT", !"evm.pc=0xbed"}
!367 = !{!"tac=0xbe7", !"op=ADD", !"evm.pc=0xbe7"}
!368 = !{!"tac=0xbf6", !"op=ADD", !"evm.pc=0xbf6"}
!369 = !{!"tac=0xbf7", !"op=ADD", !"evm.pc=0xbf7"}
!370 = !{!"tac=0xc1a", !"op=AND", !"evm.pc=0xc1a"}
!371 = !{!"tac=0xc1f", !"op=BYTE", !"evm.pc=0xc1f"}
!372 = !{!"tac=0xc21", !"op=MSTORE8", !"evm.pc=0xc21"}
!373 = !{!"tac=0xc27", !"op=ADD", !"evm.pc=0xc27"}
!374 = !{!"tac=0xc2e", !"op=JUMP", !"evm.pc=0xc2e"}
!375 = !{!"tac=0xb7d", !"op=REVERT", !"evm.pc=0xb7d"}
!376 = !{!"v1;params=1;ret=1;label=5;loads=0;unresolved=0;yulcalls=0;labelconst=1/1;calldata="}
!377 = !{!"tac=0xcf0", !"op=JUMPI", !"evm.pc=0xcf0"}
!378 = !{!"tac=0xd3d", !"op=JUMPI", !"evm.pc=0xd3d"}
!379 = !{!"tac=0xd74", !"op=REVERT", !"evm.pc=0xd74"}
!380 = !{!"tac=0xd64", !"op=SHR", !"evm.pc=0xd64"}
!381 = !{!"tac=0xd65", !"op=ADD", !"evm.pc=0xd65"}
!382 = !{!"tac=0xd69", !"op=SHL", !"evm.pc=0xd69"}
!383 = !{!"tac=0x2f448", !"op=RETURNPRIVATE", !"evm.pc=0xd79"}
!384 = !{!"tac=0xd15", !"op=SHR", !"evm.pc=0xd15"}
!385 = !{!"tac=0xd16", !"op=ADD", !"evm.pc=0xd16"}
!386 = !{!"tac=0xd19", !"op=SHL", !"evm.pc=0xd19"}
!387 = !{!"tac=0xe0", !"op=CALLVALUE", !"evm.pc=0xe0"}
!388 = !{!"tac=0xe6", !"op=JUMPI", !"evm.pc=0xe6"}
!389 = !{!"tac=0xf3", !"op=CALLPRIVATE", !"evm.pc=0xf3"}
!390 = !{!"tac=0xfc", !"op=ADD", !"evm.pc=0xfc"}
!391 = !{!"tac=0x101", !"op=MSTORE", !"evm.pc=0x101"}
!392 = !{!"tac=0x105", !"op=MLOAD", !"evm.pc=0x105"}
!393 = !{!"tac=0x107", !"op=MSTORE", !"evm.pc=0x107"}
!394 = !{!"tac=0x10a", !"op=ADD", !"evm.pc=0x10a"}
!395 = !{!"tac=0x10e", !"op=MLOAD", !"evm.pc=0x10e"}
!396 = !{!"tac=0x112", !"op=ADD", !"evm.pc=0x112"}
!397 = !{!"tac=0x4d38", !"op=JUMP", !"evm.pc=0x119"}
!398 = !{!"tac=0x119_0x0", !"op=PHI"}
!399 = !{!"tac=0x11c", !"op=LT", !"evm.pc=0x11c"}
!400 = !{!"tac=0x121", !"op=JUMPI", !"evm.pc=0x121"}
!401 = !{!"tac=0x13d", !"op=ADD", !"evm.pc=0x13d"}
!402 = !{!"tac=0x141", !"op=AND", !"evm.pc=0x141"}
!403 = !{!"tac=0x143", !"op=ISZERO", !"evm.pc=0x143"}
!404 = !{!"tac=0x147", !"op=JUMPI", !"evm.pc=0x147"}
!405 = !{!"tac=0x14a", !"op=SUB", !"evm.pc=0x14a"}
!406 = !{!"tac=0x14c", !"op=MLOAD", !"evm.pc=0x14c"}
!407 = !{!"tac=0x152", !"op=SUB", !"evm.pc=0x152"}
!408 = !{!"tac=0x156", !"op=EXP", !"evm.pc=0x156"}
!409 = !{!"tac=0x159", !"op=AND", !"evm.pc=0x159"}
!410 = !{!"tac=0x15b", !"op=MSTORE", !"evm.pc=0x15b"}
!411 = !{!"tac=0x15e", !"op=ADD", !"evm.pc=0x15e"}
!412 = !{!"tac=0x5738", !"op=JUMP", !"evm.pc=0x161"}
!413 = !{!"tac=0x161_0x1", !"op=PHI"}
!414 = !{!"tac=0x16c", !"op=SUB", !"evm.pc=0x16c"}
!415 = !{!"tac=0x16e", !"op=RETURN", !"evm.pc=0x16e"}
!416 = !{!"tac=0x124", !"op=ADD", !"evm.pc=0x124"}
!417 = !{!"tac=0x125", !"op=MLOAD", !"evm.pc=0x125"}
!418 = !{!"tac=0x128", !"op=ADD", !"evm.pc=0x128"}
!419 = !{!"tac=0x129", !"op=MSTORE", !"evm.pc=0x129"}
!420 = !{!"tac=0x12d", !"op=ADD", !"evm.pc=0x12d"}
!421 = !{!"tac=0x133", !"op=JUMP", !"evm.pc=0x133"}
!422 = !{!"tac=0xea", !"op=REVERT", !"evm.pc=0xea"}
!423 = !{!"calldata_view"}
