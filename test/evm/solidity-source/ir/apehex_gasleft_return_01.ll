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
  br i1 %evm.branch.cond, label %bb._0xf, label %bb._0xb, !notdec.evm !3

bb._0xf:                                          ; preds = %bb._0x0
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata), !notdec.evm !4
  %evm.lt = icmp ult i256 %evm.calldatasize, 4, !notdec.evm !5
  %evm.bool1 = zext i1 %evm.lt to i256, !notdec.evm !5
  %evm.branch.cond2 = icmp ne i256 %evm.bool1, 0, !notdec.evm !6
  br i1 %evm.branch.cond2, label %bb._0x1fe8, label %bb._0x18, !notdec.evm !6

bb._0x18:                                         ; preds = %bb._0xf
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 1103159733, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x29e8, label %bb._0x28, !notdec.evm !10

bb._0x29e8:                                       ; preds = %bb._0x18
  call void @public_kill___0x34(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x28:                                         ; preds = %bb._0x18
  %evm.eq5 = icmp eq i256 3635611908, %evm.shr, !notdec.evm !12
  %evm.bool6 = zext i1 %evm.eq5 to i256, !notdec.evm !12
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !13
  br i1 %evm.branch.cond7, label %bb._0x33e8, label %bb._0x1fe8, !notdec.evm !13

bb._0x33e8:                                       ; preds = %bb._0x28
  call void @public__0xd8b30904_0x3a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !14
  ret void

bb._0x1fe8:                                       ; preds = %bb._0x28, %bb._0xf
  call void @public_fallback___0x32(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !15
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !16
  unreachable, !notdec.evm !16
}

; Function Attrs: null_pointer_is_valid
define void @public_fallback___0x32(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x32:
  ret void, !notdec.evm !17
}

; Function Attrs: null_pointer_is_valid
define void @public_kill___0x34(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x34:
  br label %bb._0x52, !notdec.evm !18

bb._0x52:                                         ; preds = %bb._0x34
  %evm.sload = call i256 @evm_sload(i256 0), !notdec.evm !19
  %evm.shl = call i256 @evm_shl(i256 160, i256 1), !notdec.evm !20
  %evm.sub = sub i256 %evm.shl, 1, !notdec.evm !21
  %evm.and = and i256 %evm.sub, %evm.sload, !notdec.evm !22
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !23
  %evm.eq = icmp eq i256 %evm.caller, %evm.and, !notdec.evm !24
  %evm.bool = zext i1 %evm.eq to i256, !notdec.evm !24
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !25
  br i1 %evm.branch.cond, label %bb._0xaf, label %bb._0x64, !notdec.evm !25

bb._0xaf:                                         ; preds = %bb._0x52
  %evm.origin = call i256 @evm_origin(ptr %env), !notdec.evm !26
  call void @evm_selfdestruct(ptr %env, i256 %evm.origin), !notdec.evm !27
  ret void

bb._0x64:                                         ; preds = %bb._0x52
  %evm.mload = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !28
  %evm.shl1 = call i256 @evm_shl(i256 229, i256 4594637), !notdec.evm !29
  %evm.mem.ptr = inttoptr i256 %evm.mload to ptr, !notdec.evm !30
  store i256 %evm.shl1, ptr %evm.mem.ptr, align 1, !notdec.evm !30
  %evm.add = add i256 %evm.mload, 4, !notdec.evm !31
  %evm.mem.ptr2 = inttoptr i256 %evm.add to ptr, !notdec.evm !32
  store i256 32, ptr %evm.mem.ptr2, align 1, !notdec.evm !32
  %evm.add3 = add i256 %evm.mload, 36, !notdec.evm !33
  %evm.mem.ptr4 = inttoptr i256 %evm.add3 to ptr, !notdec.evm !34
  store i256 32, ptr %evm.mem.ptr4, align 1, !notdec.evm !34
  %evm.add5 = add i256 %evm.mload, 68, !notdec.evm !35
  %evm.mem.ptr6 = inttoptr i256 %evm.add5 to ptr, !notdec.evm !36
  store i256 35943731656364841964516503116990081338611484598491072354577564874054038349170, ptr %evm.mem.ptr6, align 1, !notdec.evm !36
  %evm.add7 = add i256 100, %evm.mload, !notdec.evm !37
  %evm.mload8 = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !38
  %evm.sub9 = sub i256 %evm.add7, %evm.mload8, !notdec.evm !39
  call void @evm_revert(ptr %mem, i256 %evm.mload8, i256 %evm.sub9), !notdec.evm !40
  unreachable, !notdec.evm !40
}

; Function Attrs: null_pointer_is_valid
define void @public__0xd8b30904_0x3a(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x3a:
  br label %bb._0xb2, !notdec.evm !41

bb._0xb2:                                         ; preds = %bb._0x3a
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !42
  br label %bb._0x40, !notdec.evm !43

bb._0x40:                                         ; preds = %bb._0xb2
  %evm.mload = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !44
  %evm.mem.ptr = inttoptr i256 %evm.mload to ptr, !notdec.evm !45
  store i256 %evm.gas, ptr %evm.mem.ptr, align 1, !notdec.evm !45
  %evm.add = add i256 32, %evm.mload, !notdec.evm !46
  %evm.mload1 = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !47
  %evm.sub = sub i256 %evm.add, %evm.mload1, !notdec.evm !48
  call void @evm_return(ptr %mem, i256 %evm.mload1, i256 %evm.sub), !notdec.evm !49
  ret void, !notdec.evm !49
}

attributes #0 = { null_pointer_is_valid }

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x13", !"op=CALLDATASIZE", !"evm.pc=0x13"}
!5 = !{!"tac=0x14", !"op=LT", !"evm.pc=0x14"}
!6 = !{!"tac=0x208", !"op=JUMPI", !"evm.pc=0x15"}
!7 = !{!"tac=0x1a", !"op=CALLDATALOAD", !"evm.pc=0x1a"}
!8 = !{!"tac=0x1d", !"op=SHR", !"evm.pc=0x1d"}
!9 = !{!"tac=0x24", !"op=EQ", !"evm.pc=0x24"}
!10 = !{!"tac=0xc08", !"op=JUMPI", !"evm.pc=0x25"}
!11 = !{!"tac=0x2a28", !"op=CALLPRIVATE", !"evm.pc=0x34"}
!12 = !{!"tac=0x2e", !"op=EQ", !"evm.pc=0x2e"}
!13 = !{!"tac=0x1608", !"op=JUMPI", !"evm.pc=0x2f"}
!14 = !{!"tac=0x3428", !"op=CALLPRIVATE", !"evm.pc=0x3a"}
!15 = !{!"tac=0x2028", !"op=CALLPRIVATE", !"evm.pc=0x32"}
!16 = !{!"tac=0xe", !"op=REVERT", !"evm.pc=0xe"}
!17 = !{!"tac=0x33", !"op=STOP", !"evm.pc=0x33"}
!18 = !{!"tac=0x39", !"op=JUMP", !"evm.pc=0x39"}
!19 = !{!"tac=0x55", !"op=SLOAD", !"evm.pc=0x55"}
!20 = !{!"tac=0x5c", !"op=SHL", !"evm.pc=0x5c"}
!21 = !{!"tac=0x5d", !"op=SUB", !"evm.pc=0x5d"}
!22 = !{!"tac=0x5e", !"op=AND", !"evm.pc=0x5e"}
!23 = !{!"tac=0x5f", !"op=CALLER", !"evm.pc=0x5f"}
!24 = !{!"tac=0x60", !"op=EQ", !"evm.pc=0x60"}
!25 = !{!"tac=0x63", !"op=JUMPI", !"evm.pc=0x63"}
!26 = !{!"tac=0xb0", !"op=ORIGIN", !"evm.pc=0xb0"}
!27 = !{!"tac=0xb1", !"op=SELFDESTRUCT", !"evm.pc=0xb1"}
!28 = !{!"tac=0x66", !"op=MLOAD", !"evm.pc=0x66"}
!29 = !{!"tac=0x6d", !"op=SHL", !"evm.pc=0x6d"}
!30 = !{!"tac=0x6f", !"op=MSTORE", !"evm.pc=0x6f"}
!31 = !{!"tac=0x75", !"op=ADD", !"evm.pc=0x75"}
!32 = !{!"tac=0x78", !"op=MSTORE", !"evm.pc=0x78"}
!33 = !{!"tac=0x7c", !"op=ADD", !"evm.pc=0x7c"}
!34 = !{!"tac=0x7d", !"op=MSTORE", !"evm.pc=0x7d"}
!35 = !{!"tac=0xa2", !"op=ADD", !"evm.pc=0xa2"}
!36 = !{!"tac=0xa3", !"op=MSTORE", !"evm.pc=0xa3"}
!37 = !{!"tac=0xa6", !"op=ADD", !"evm.pc=0xa6"}
!38 = !{!"tac=0xa9", !"op=MLOAD", !"evm.pc=0xa9"}
!39 = !{!"tac=0xac", !"op=SUB", !"evm.pc=0xac"}
!40 = !{!"tac=0xae", !"op=REVERT", !"evm.pc=0xae"}
!41 = !{!"tac=0x3f", !"op=JUMP", !"evm.pc=0x3f"}
!42 = !{!"tac=0xb6", !"op=GAS", !"evm.pc=0xb6"}
!43 = !{!"tac=0xbb", !"op=JUMP", !"evm.pc=0xbb"}
!44 = !{!"tac=0x43", !"op=MLOAD", !"evm.pc=0x43"}
!45 = !{!"tac=0x46", !"op=MSTORE", !"evm.pc=0x46"}
!46 = !{!"tac=0x49", !"op=ADD", !"evm.pc=0x49"}
!47 = !{!"tac=0x4c", !"op=MLOAD", !"evm.pc=0x4c"}
!48 = !{!"tac=0x4f", !"op=SUB", !"evm.pc=0x4f"}
!49 = !{!"tac=0x51", !"op=RETURN", !"evm.pc=0x51"}
