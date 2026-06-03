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
  br i1 %evm.branch.cond, label %bb._0xf, label %bb._0xb, !notdec.evm !3

bb._0xf:                                          ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.lt to i256, !notdec.evm !5
  %evm.branch.cond2 = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond2, label %bb._0x1556, label %bb._0x18, !notdec.evm !6

bb._0x18:                                         ; preds = %bb._0xf
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 720013615, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x1f56, label %bb._0x1556, !notdec.evm !10

bb._0x1f56:                                       ; preds = %bb._0x18
  call void @public__0x2aea892f_0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x1556:                                       ; preds = %bb._0x18, %bb._0xf
  call void @public_fallback___0x28(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !13
  unreachable, !notdec.evm !13
}

define void @public_fallback___0x28(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x28:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !14
  unreachable, !notdec.evm !14
}

define void @public__0x2aea892f_0x2d(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x2d:
  br label %bb._0x35, !notdec.evm !15

bb._0x35:                                         ; preds = %bb._0x2d
  %evm.sload = call i256 @evm_sload(i256 0), !notdec.evm !16
  %evm.branch.cond = icmp ne i256 %evm.sload, 0, !notdec.evm !17
  br i1 %evm.branch.cond, label %bb._0x43, label %bb._0x3e, !notdec.evm !17

bb._0x43:                                         ; preds = %bb._0x35
  %evm.sub = sub i256 %evm.sload, 1, !notdec.evm !18
  %notdec.evm.mem.ptr.1 = inttoptr i256 0 to ptr
  store i256 0, ptr %notdec.evm.mem.ptr.1, align 1, !notdec.evm !19
  %evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 0, i256 32), !notdec.evm !20
  %evm.add = add i256 %evm.sha3, %evm.sub, !notdec.evm !21
  call void @evm_sstore(i256 %evm.add, i256 0), !notdec.evm !22
  call void @evm_sstore(i256 0, i256 %evm.sub), !notdec.evm !23
  br label %bb._0x33, !notdec.evm !24

bb._0x33:                                         ; preds = %bb._0x43
  ret void, !notdec.evm !25

bb._0x3e:                                         ; preds = %bb._0x35
  br label %bb._0x5b, !notdec.evm !26

bb._0x5b:                                         ; preds = %bb._0x3e
  %evm.shl = call i256 @evm_shl(i256 224, i256 1313373041), !notdec.evm !27
  %notdec.evm.mem.ptr.2 = inttoptr i256 0 to ptr
  store i256 %evm.shl, ptr %notdec.evm.mem.ptr.2, align 1, !notdec.evm !28
  %notdec.evm.mem.ptr.3 = inttoptr i256 4 to ptr
  store i256 49, ptr %notdec.evm.mem.ptr.3, align 1, !notdec.evm !29
  call void @evm_revert(ptr %mem, i256 0, i256 36), !notdec.evm !30
  unreachable, !notdec.evm !30
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x13", !"op=CALLDATASIZE", !"evm.pc=0x13"}
!5 = !{!"tac=0x14", !"op=LT", !"evm.pc=0x14"}
!6 = !{!"tac=0x176", !"op=JUMPI", !"evm.pc=0x15"}
!7 = !{!"tac=0x1a", !"op=CALLDATALOAD", !"evm.pc=0x1a"}
!8 = !{!"tac=0x1d", !"op=SHR", !"evm.pc=0x1d"}
!9 = !{!"tac=0x24", !"op=EQ", !"evm.pc=0x24"}
!10 = !{!"tac=0xb76", !"op=JUMPI", !"evm.pc=0x25"}
!11 = !{!"tac=0x1f96", !"op=CALLPRIVATE", !"evm.pc=0x2d"}
!12 = !{!"tac=0x1596", !"op=CALLPRIVATE", !"evm.pc=0x28"}
!13 = !{!"tac=0xe", !"op=REVERT", !"evm.pc=0xe"}
!14 = !{!"tac=0x2c", !"op=REVERT", !"evm.pc=0x2c"}
!15 = !{!"tac=0x32", !"op=JUMP", !"evm.pc=0x32"}
!16 = !{!"tac=0x39", !"op=SLOAD", !"evm.pc=0x39"}
!17 = !{!"tac=0x3d", !"op=JUMPI", !"evm.pc=0x3d"}
!18 = !{!"tac=0x47", !"op=SUB", !"evm.pc=0x47"}
!19 = !{!"tac=0x4d", !"op=MSTORE", !"evm.pc=0x4d"}
!20 = !{!"tac=0x52", !"op=SHA3", !"evm.pc=0x52"}
!21 = !{!"tac=0x53", !"op=ADD", !"evm.pc=0x53"}
!22 = !{!"tac=0x57", !"op=SSTORE", !"evm.pc=0x57"}
!23 = !{!"tac=0x59", !"op=SSTORE", !"evm.pc=0x59"}
!24 = !{!"tac=0x5a", !"op=JUMP", !"evm.pc=0x5a"}
!25 = !{!"tac=0x34", !"op=STOP", !"evm.pc=0x34"}
!26 = !{!"tac=0x42", !"op=JUMP", !"evm.pc=0x42"}
!27 = !{!"tac=0x63", !"op=SHL", !"evm.pc=0x63"}
!28 = !{!"tac=0x66", !"op=MSTORE", !"evm.pc=0x66"}
!29 = !{!"tac=0x6b", !"op=MSTORE", !"evm.pc=0x6b"}
!30 = !{!"tac=0x70", !"op=REVERT", !"evm.pc=0x70"}

attributes #0 = { null_pointer_is_valid }
