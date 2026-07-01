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
  br i1 %evm.branch.cond2, label %bb._0x1fd0, label %bb._0x17, !notdec.evm !6

bb._0x17:                                         ; preds = %bb._0xe
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 0), !notdec.evm !7
  %evm.shr = call i256 @evm_shr(i256 224, i256 %evm.calldataload), !notdec.evm !8
  %evm.eq = icmp eq i256 1218665806, %evm.shr, !notdec.evm !9
  %evm.bool3 = zext i1 %evm.eq to i256, !notdec.evm !9
  %evm.branch.cond4 = icmp ne i256 %evm.bool3, 0, !notdec.evm !10
  br i1 %evm.branch.cond4, label %bb._0x29d0, label %bb._0x26, !notdec.evm !10

bb._0x29d0:                                       ; preds = %bb._0x17
  call void @public__0x48a35d4e_0x34(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !11
  ret void

bb._0x26:                                         ; preds = %bb._0x17
  %evm.eq5 = icmp eq i256 1535508120, %evm.shr, !notdec.evm !12
  %evm.bool6 = zext i1 %evm.eq5 to i256, !notdec.evm !12
  %evm.branch.cond7 = icmp ne i256 %evm.bool6, 0, !notdec.evm !13
  br i1 %evm.branch.cond7, label %bb._0x33d0, label %bb._0x1fd0, !notdec.evm !13

bb._0x33d0:                                       ; preds = %bb._0x26
  call void @public__0x5b85fe98_0x4e(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !14
  ret void

bb._0x1fd0:                                       ; preds = %bb._0x26, %bb._0xe
  call void @public_fallback___0x30(ptr %mem, ptr %calldata, ptr %returndata, ptr %env), !notdec.evm !15
  ret void

bb._0xb:                                          ; preds = %bb._0x0
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !16
  unreachable, !notdec.evm !16
}

; Function Attrs: null_pointer_is_valid
define void @public_fallback___0x30(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x30:
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !17
  unreachable, !notdec.evm !17
}

; Function Attrs: null_pointer_is_valid
define void @public__0x48a35d4e_0x34(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x34:
  br label %bb._0x68, !notdec.evm !18

bb._0x68:                                         ; preds = %bb._0x34
  %evm.blobbasefee = call i256 @evm_blobbasefee(ptr %env), !notdec.evm !19
  br label %bb._0x3a, !notdec.evm !20

bb._0x3a:                                         ; preds = %bb._0x68
  %evm.mload = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !21
  br label %bb._0x8cB0x3a, !notdec.evm !22

bb._0x8cB0x3a:                                    ; preds = %bb._0x3a
  %evm.add = add i256 %evm.mload, 32, !notdec.evm !23
  %evm.add1 = add i256 %evm.mload, 0, !notdec.evm !24
  br label %bb._0x7fB0x3a, !notdec.evm !25

bb._0x7fB0x3a:                                    ; preds = %bb._0x8cB0x3a
  br label %bb._0x76B0x3a, !notdec.evm !26

bb._0x76B0x3a:                                    ; preds = %bb._0x7fB0x3a
  br label %bb._0x86B0x3a, !notdec.evm !27

bb._0x86B0x3a:                                    ; preds = %bb._0x76B0x3a
  %evm.mem.ptr = inttoptr i256 %evm.add1 to ptr, !notdec.evm !28
  store i256 %evm.blobbasefee, ptr %evm.mem.ptr, align 1, !notdec.evm !28
  br label %bb._0x9dB0x3a, !notdec.evm !29

bb._0x9dB0x3a:                                    ; preds = %bb._0x86B0x3a
  br label %bb._0x45, !notdec.evm !30

bb._0x45:                                         ; preds = %bb._0x9dB0x3a
  %evm.mload2 = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !31
  %evm.sub = sub i256 %evm.add, %evm.mload2, !notdec.evm !32
  call void @evm_return(ptr %mem, i256 %evm.mload2, i256 %evm.sub), !notdec.evm !33
  ret void, !notdec.evm !33
}

; Function Attrs: null_pointer_is_valid
define void @public__0x5b85fe98_0x4e(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x4e:
  br label %bb._0x6f, !notdec.evm !34

bb._0x6f:                                         ; preds = %bb._0x4e
  %evm.blobbasefee = call i256 @evm_blobbasefee(ptr %env), !notdec.evm !35
  br label %bb._0x54, !notdec.evm !36

bb._0x54:                                         ; preds = %bb._0x6f
  %evm.mload = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !37
  br label %bb._0x8cB0x54, !notdec.evm !38

bb._0x8cB0x54:                                    ; preds = %bb._0x54
  %evm.add = add i256 %evm.mload, 32, !notdec.evm !39
  %evm.add1 = add i256 %evm.mload, 0, !notdec.evm !40
  br label %bb._0x7fB0x54, !notdec.evm !41

bb._0x7fB0x54:                                    ; preds = %bb._0x8cB0x54
  br label %bb._0x76B0x54, !notdec.evm !42

bb._0x76B0x54:                                    ; preds = %bb._0x7fB0x54
  br label %bb._0x86B0x54, !notdec.evm !43

bb._0x86B0x54:                                    ; preds = %bb._0x76B0x54
  %evm.mem.ptr = inttoptr i256 %evm.add1 to ptr, !notdec.evm !44
  store i256 %evm.blobbasefee, ptr %evm.mem.ptr, align 1, !notdec.evm !44
  br label %bb._0x9dB0x54, !notdec.evm !45

bb._0x9dB0x54:                                    ; preds = %bb._0x86B0x54
  br label %bb._0x5f, !notdec.evm !46

bb._0x5f:                                         ; preds = %bb._0x9dB0x54
  %evm.mload2 = load i256, ptr inttoptr (i256 64 to ptr), align 1, !notdec.evm !47
  %evm.sub = sub i256 %evm.add, %evm.mload2, !notdec.evm !48
  call void @evm_return(ptr %mem, i256 %evm.mload2, i256 %evm.sub), !notdec.evm !49
  ret void, !notdec.evm !49
}

attributes #0 = { null_pointer_is_valid }

!0 = !{!"tac=0x4", !"op=MSTORE", !"evm.pc=0x4"}
!1 = !{!"tac=0x5", !"op=CALLVALUE", !"evm.pc=0x5"}
!2 = !{!"tac=0x7", !"op=ISZERO", !"evm.pc=0x7"}
!3 = !{!"tac=0xa", !"op=JUMPI", !"evm.pc=0xa"}
!4 = !{!"tac=0x12", !"op=CALLDATASIZE", !"evm.pc=0x12"}
!5 = !{!"tac=0x13", !"op=LT", !"evm.pc=0x13"}
!6 = !{!"tac=0x1f0", !"op=JUMPI", !"evm.pc=0x14"}
!7 = !{!"tac=0x18", !"op=CALLDATALOAD", !"evm.pc=0x18"}
!8 = !{!"tac=0x1b", !"op=SHR", !"evm.pc=0x1b"}
!9 = !{!"tac=0x22", !"op=EQ", !"evm.pc=0x22"}
!10 = !{!"tac=0xbf0", !"op=JUMPI", !"evm.pc=0x23"}
!11 = !{!"tac=0x2a10", !"op=CALLPRIVATE", !"evm.pc=0x34"}
!12 = !{!"tac=0x2c", !"op=EQ", !"evm.pc=0x2c"}
!13 = !{!"tac=0x15f0", !"op=JUMPI", !"evm.pc=0x2d"}
!14 = !{!"tac=0x3410", !"op=CALLPRIVATE", !"evm.pc=0x4e"}
!15 = !{!"tac=0x2010", !"op=CALLPRIVATE", !"evm.pc=0x30"}
!16 = !{!"tac=0xd", !"op=REVERT", !"evm.pc=0xd"}
!17 = !{!"tac=0x33", !"op=REVERT", !"evm.pc=0x33"}
!18 = !{!"tac=0x39", !"op=JUMP", !"evm.pc=0x39"}
!19 = !{!"tac=0x6a", !"op=BLOBBASEFEE", !"evm.pc=0x6a"}
!20 = !{!"tac=0x6e", !"op=JUMP", !"evm.pc=0x6e"}
!21 = !{!"tac=0x3d", !"op=MLOAD", !"evm.pc=0x3d"}
!22 = !{!"tac=0x44", !"op=JUMP", !"evm.pc=0x44"}
!23 = !{!"tac=0x91S0x3a", !"op=ADD", !"evm.pc=0x44,0x91", !"inline=[0x8c, nil]"}
!24 = !{!"tac=0x98S0x3a", !"op=ADD", !"evm.pc=0x44,0x98", !"inline=[0x8c, nil]"}
!25 = !{!"tac=0x9cS0x3a", !"op=JUMP", !"evm.pc=0x44,0x9c", !"inline=[0x8c, nil]"}
!26 = !{!"tac=0x85S0x3a", !"op=JUMP", !"evm.pc=0x44,0x85", !"inline=[0x8c, nil]"}
!27 = !{!"tac=0x7eS0x3a", !"op=JUMP", !"evm.pc=0x44,0x7e", !"inline=[0x8c, nil]"}
!28 = !{!"tac=0x88S0x3a", !"op=MSTORE", !"evm.pc=0x44,0x88", !"inline=[0x8c, nil]"}
!29 = !{!"tac=0x8bS0x3a", !"op=JUMP", !"evm.pc=0x44,0x8b", !"inline=[0x8c, nil]"}
!30 = !{!"tac=0xa2S0x3a", !"op=JUMP", !"evm.pc=0x44,0xa2", !"inline=[0x8c, nil]"}
!31 = !{!"tac=0x48", !"op=MLOAD", !"evm.pc=0x48"}
!32 = !{!"tac=0x4b", !"op=SUB", !"evm.pc=0x4b"}
!33 = !{!"tac=0x4d", !"op=RETURN", !"evm.pc=0x4d"}
!34 = !{!"tac=0x53", !"op=JUMP", !"evm.pc=0x53"}
!35 = !{!"tac=0x71", !"op=BLOBBASEFEE", !"evm.pc=0x71"}
!36 = !{!"tac=0x75", !"op=JUMP", !"evm.pc=0x75"}
!37 = !{!"tac=0x57", !"op=MLOAD", !"evm.pc=0x57"}
!38 = !{!"tac=0x5e", !"op=JUMP", !"evm.pc=0x5e"}
!39 = !{!"tac=0x91S0x54", !"op=ADD", !"evm.pc=0x5e,0x91", !"inline=[0x8c, nil]"}
!40 = !{!"tac=0x98S0x54", !"op=ADD", !"evm.pc=0x5e,0x98", !"inline=[0x8c, nil]"}
!41 = !{!"tac=0x9cS0x54", !"op=JUMP", !"evm.pc=0x5e,0x9c", !"inline=[0x8c, nil]"}
!42 = !{!"tac=0x85S0x54", !"op=JUMP", !"evm.pc=0x5e,0x85", !"inline=[0x8c, nil]"}
!43 = !{!"tac=0x7eS0x54", !"op=JUMP", !"evm.pc=0x5e,0x7e", !"inline=[0x8c, nil]"}
!44 = !{!"tac=0x88S0x54", !"op=MSTORE", !"evm.pc=0x5e,0x88", !"inline=[0x8c, nil]"}
!45 = !{!"tac=0x8bS0x54", !"op=JUMP", !"evm.pc=0x5e,0x8b", !"inline=[0x8c, nil]"}
!46 = !{!"tac=0xa2S0x54", !"op=JUMP", !"evm.pc=0x5e,0xa2", !"inline=[0x8c, nil]"}
!47 = !{!"tac=0x62", !"op=MLOAD", !"evm.pc=0x62"}
!48 = !{!"tac=0x65", !"op=SUB", !"evm.pc=0x65"}
!49 = !{!"tac=0x67", !"op=RETURN", !"evm.pc=0x67"}
