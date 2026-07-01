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

; Function Attrs: null_pointer_is_valid
define void @public___function_selector___0x0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x0:
  store i256 128, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !0
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
  br i1 %evm.branch.cond2, label %bb._0x14e6, label %bb._0x17, !notdec.evm !6

bb._0x17:                                         ; preds = %bb._0xe
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 2741362607, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x1ee6, label %bb._0x14e6, !notdec.evm !10

bb._0x1ee6:                                       ; preds = %bb._0x17
  call void @public__0xa365dfaf_0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x14e6:                                       ; preds = %bb._0x17, %bb._0xe
  call void @public_fallback___0x26(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !12
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !13
  unreachable, !notdec.evm !13
}

; Function Attrs: null_pointer_is_valid
define void @public_fallback___0x26(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x26:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !14
  unreachable, !notdec.evm !14
}

; Function Attrs: null_pointer_is_valid
define void @public__0xa365dfaf_0x2a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x2a:
  %evm.basefee = call i256 @evm_basefee(ptr %env), !notdec.evm !15
  %evm.mload = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !16
  %evm.mem.ptr = inttoptr i256 %evm.mload to ptr, !notdec.evm !17
  store i256 %evm.basefee, ptr %evm.mem.ptr, align 1, !notdec.evm !17
  %evm.add = add i256 32, %evm.mload, !notdec.evm !18
  %evm.mload1 = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !19
  %evm.sub = sub i256 %evm.add, %evm.mload1, !notdec.evm !20
  call void @evm_return(ptr %mem, i256 %evm.mload1, i256 %evm.sub), !notdec.evm !21
  ret void, !notdec.evm !21
}

attributes #0 = { null_pointer_is_valid }

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x12", !"op=CALLDATASIZE", !"evm.pc=0x12"}
!5 = !{!"tac=0x13", !"op=LT", !"evm.pc=0x13"}
!6 = !{!"tac=0x106", !"op=JUMPI", !"evm.pc=0x14"}
!7 = !{!"tac=0x18", !"op=CALLDATALOAD", !"evm.pc=0x18"}
!8 = !{!"tac=0x1b", !"op=SHR", !"evm.pc=0x1b"}
!9 = !{!"tac=0x22", !"op=EQ", !"evm.pc=0x22"}
!10 = !{!"tac=0xb06", !"op=JUMPI", !"evm.pc=0x23"}
!11 = !{!"tac=0x1f26", !"op=CALLPRIVATE", !"evm.pc=0x2a"}
!12 = !{!"tac=0x1526", !"op=CALLPRIVATE", !"evm.pc=0x26"}
!13 = !{!"tac=0xd", !"op=REVERT", !"evm.pc=0xd"}
!14 = !{!"tac=0x29", !"op=REVERT", !"evm.pc=0x29"}
!15 = !{!"tac=0x2b", !"op=BASEFEE", !"evm.pc=0x2b"}
!16 = !{!"tac=0x2e", !"op=MLOAD", !"evm.pc=0x2e"}
!17 = !{!"tac=0x31", !"op=MSTORE", !"evm.pc=0x31"}
!18 = !{!"tac=0x34", !"op=ADD", !"evm.pc=0x34"}
!19 = !{!"tac=0x37", !"op=MLOAD", !"evm.pc=0x37"}
!20 = !{!"tac=0x3a", !"op=SUB", !"evm.pc=0x3a"}
!21 = !{!"tac=0x3c", !"op=RETURN", !"evm.pc=0x3c"}
