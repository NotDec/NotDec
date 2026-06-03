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
  br i1 %evm.branch.cond, label %bb._0xe, label %bb._0xb, !notdec.evm !3

bb._0xe:                                          ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.lt to i256, !notdec.evm !5
  %evm.branch.cond2 = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond2, label %bb._0x1550, label %bb._0x17, !notdec.evm !6

bb._0x17:                                         ; preds = %bb._0xe
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 2755982825, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x1f50, label %bb._0x1550, !notdec.evm !10

bb._0x1f50:                                       ; preds = %bb._0x17
  call void @public_run_uint256__0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x1550:                                       ; preds = %bb._0x17, %bb._0xe
  call void @public_fallback___0x26(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !13
  unreachable, !notdec.evm !13
}

define void @public_fallback___0x26(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x26:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !14
  unreachable, !notdec.evm !14
}

define void @public_run_uint256__0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x2a:
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !15
  br label %bb._0x61, !notdec.evm !16

bb._0x61:                                         ; preds = %bb._0x2a
  %evm.sub = sub i256 %evm.calldatasize, 4, !notdec.evm !17
  %evm.slt = icmp slt i256 %evm.sub, 32, !notdec.evm !18
  %evm.bool = zext i1 %evm.slt to i256, !notdec.evm !18
  %evm.iszero = icmp eq i256 %evm.bool, 0, !notdec.evm !19
  %evm.bool1 = zext i1 %evm.iszero to i256, !notdec.evm !19
  %evm.branch.cond = icmp ne i256 %evm.bool1, 0, !notdec.evm !20
  br i1 %evm.branch.cond, label %bb._0x70, label %bb._0x6d, !notdec.evm !20

bb._0x70:                                         ; preds = %bb._0x61
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 4), !notdec.evm !21
  br label %bb._0x35, !notdec.evm !22

bb._0x35:                                         ; preds = %bb._0x70
  br label %bb._0x3b, !notdec.evm !23

bb._0x3b:                                         ; preds = %bb._0x35
  %notdec.evm.mem.ptr.1 = inttoptr i256 64 to ptr
  %evm.mload = load i256, ptr %notdec.evm.mem.ptr.1, align 1, !notdec.evm !24
  %evm.shl = call i256 @evm_shl(i256 224, i256 256322039), !notdec.evm !25
  %notdec.evm.mem.ptr.2 = inttoptr i256 %evm.mload to ptr
  store i256 %evm.shl, ptr %notdec.evm.mem.ptr.2, align 1, !notdec.evm !26
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !27
  %evm.add = add i256 %evm.mload, 4, !notdec.evm !28
  %notdec.evm.mem.ptr.3 = inttoptr i256 %evm.add to ptr
  store i256 %evm.caller, ptr %notdec.evm.mem.ptr.3, align 1, !notdec.evm !29
  %evm.add2 = add i256 %evm.mload, 36, !notdec.evm !30
  %notdec.evm.mem.ptr.4 = inttoptr i256 %evm.add2 to ptr
  store i256 %evm.calldataload, ptr %notdec.evm.mem.ptr.4, align 1, !notdec.evm !31
  %evm.add3 = add i256 68, %evm.mload, !notdec.evm !32
  %notdec.evm.mem.ptr.5 = inttoptr i256 64 to ptr
  %evm.mload4 = load i256, ptr %notdec.evm.mem.ptr.5, align 1, !notdec.evm !33
  %evm.sub5 = sub i256 %evm.add3, %evm.mload4, !notdec.evm !34
  call void @evm_revert(ptr %mem, i256 %evm.mload4, i256 %evm.sub5), !notdec.evm !35
  unreachable, !notdec.evm !35

bb._0x6d:                                         ; preds = %bb._0x61
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !36
  unreachable, !notdec.evm !36
}

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x12", !"op=CALLDATASIZE", !"evm.pc=0x12"}
!5 = !{!"tac=0x13", !"op=LT", !"evm.pc=0x13"}
!6 = !{!"tac=0x170", !"op=JUMPI", !"evm.pc=0x14"}
!7 = !{!"tac=0x18", !"op=CALLDATALOAD", !"evm.pc=0x18"}
!8 = !{!"tac=0x1b", !"op=SHR", !"evm.pc=0x1b"}
!9 = !{!"tac=0x22", !"op=EQ", !"evm.pc=0x22"}
!10 = !{!"tac=0xb70", !"op=JUMPI", !"evm.pc=0x23"}
!11 = !{!"tac=0x1f90", !"op=CALLPRIVATE", !"evm.pc=0x2a"}
!12 = !{!"tac=0x1590", !"op=CALLPRIVATE", !"evm.pc=0x26"}
!13 = !{!"tac=0xd", !"op=REVERT", !"evm.pc=0xd"}
!14 = !{!"tac=0x29", !"op=REVERT", !"evm.pc=0x29"}
!15 = !{!"tac=0x2f", !"op=CALLDATASIZE", !"evm.pc=0x2f"}
!16 = !{!"tac=0x34", !"op=JUMP", !"evm.pc=0x34"}
!17 = !{!"tac=0x67", !"op=SUB", !"evm.pc=0x67"}
!18 = !{!"tac=0x68", !"op=SLT", !"evm.pc=0x68"}
!19 = !{!"tac=0x69", !"op=ISZERO", !"evm.pc=0x69"}
!20 = !{!"tac=0x6c", !"op=JUMPI", !"evm.pc=0x6c"}
!21 = !{!"tac=0x72", !"op=CALLDATALOAD", !"evm.pc=0x72"}
!22 = !{!"tac=0x76", !"op=JUMP", !"evm.pc=0x76"}
!23 = !{!"tac=0x38", !"op=JUMP", !"evm.pc=0x38"}
!24 = !{!"tac=0x3e", !"op=MLOAD", !"evm.pc=0x3e"}
!25 = !{!"tac=0x46", !"op=SHL", !"evm.pc=0x46"}
!26 = !{!"tac=0x48", !"op=MSTORE", !"evm.pc=0x48"}
!27 = !{!"tac=0x49", !"op=CALLER", !"evm.pc=0x49"}
!28 = !{!"tac=0x4d", !"op=ADD", !"evm.pc=0x4d"}
!29 = !{!"tac=0x4e", !"op=MSTORE", !"evm.pc=0x4e"}
!30 = !{!"tac=0x52", !"op=ADD", !"evm.pc=0x52"}
!31 = !{!"tac=0x55", !"op=MSTORE", !"evm.pc=0x55"}
!32 = !{!"tac=0x58", !"op=ADD", !"evm.pc=0x58"}
!33 = !{!"tac=0x5b", !"op=MLOAD", !"evm.pc=0x5b"}
!34 = !{!"tac=0x5e", !"op=SUB", !"evm.pc=0x5e"}
!35 = !{!"tac=0x60", !"op=REVERT", !"evm.pc=0x60"}
!36 = !{!"tac=0x6f", !"op=REVERT", !"evm.pc=0x6f"}

attributes #0 = { null_pointer_is_valid }
