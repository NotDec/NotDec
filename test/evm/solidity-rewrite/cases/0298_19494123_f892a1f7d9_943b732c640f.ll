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
  %evm.caller = call i256 @evm_caller(ptr %env), !notdec.evm !0
  %evm.eq = icmp eq i256 648340220550266816263448094761317005983842894690, %evm.caller, !notdec.evm !1
  %evm.bool = zext i1 %evm.eq to i256, !notdec.evm !1
  %evm.branch.cond = icmp ne i256 %evm.bool, 0, !notdec.evm !2
  br i1 %evm.branch.cond, label %bb._0x39, label %bb._0x1b, !notdec.evm !2

bb._0x39:                                         ; preds = %bb._0x0
  call void @evm_mstore(ptr %mem, i256 0, i256 151187884), !notdec.evm !3
  %evm.gas = call i256 @evm_gas(ptr %env), !notdec.evm !4
  %evm.staticcall = call i256 @evm_staticcall(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas, i256 1128921706123971286396643566168932927452814570508, i256 0, i256 4, i256 0, i256 64), !notdec.evm !5
  %evm.shr = call i256 @evm_shr(i256 224, i256 36441503), !notdec.evm !6
  call void @evm_mstore(ptr %mem, i256 0, i256 %evm.shr), !notdec.evm !7
  %evm.address = call i256 @evm_address(ptr %env), !notdec.evm !8
  call void @evm_mstore(ptr %mem, i256 96, i256 %evm.address), !notdec.evm !9
  call void @evm_mstore(ptr %mem, i256 128, i256 128), !notdec.evm !10
  call void @evm_mstore(ptr %mem, i256 160, i256 32), !notdec.evm !11
  %evm.mload = call i256 @evm_mload(ptr %mem, i256 32), !notdec.evm !12
  call void @evm_mstore(ptr %mem, i256 192, i256 %evm.mload), !notdec.evm !13
  %evm.gas1 = call i256 @evm_gas(ptr %env), !notdec.evm !14
  %evm.call = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas1, i256 1128921706123971286396643566168932927452814570508, i256 0, i256 28, i256 164, i256 0, i256 0), !notdec.evm !15
  call void @evm_mstore(ptr %mem, i256 0, i256 2745690099), !notdec.evm !16
  call void @evm_mstore(ptr %mem, i256 4, i256 648340220550266816263448094761317005983842894690), !notdec.evm !17
  %evm.gas2 = call i256 @evm_gas(ptr %env), !notdec.evm !18
  %evm.call3 = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas2, i256 1212706757719390328488828735575087460932363329889, i256 0, i256 0, i256 36, i256 0, i256 0), !notdec.evm !19
  ret void, !notdec.evm !20

bb._0x1b:                                         ; preds = %bb._0x0
  %evm.origin = call i256 @evm_origin(ptr %env), !notdec.evm !21
  %evm.eq4 = icmp eq i256 648340220550266816263448094761317005983842894690, %evm.origin, !notdec.evm !22
  %evm.bool5 = zext i1 %evm.eq4 to i256, !notdec.evm !22
  %evm.branch.cond6 = icmp ne i256 %evm.bool5, 0, !notdec.evm !23
  br i1 %evm.branch.cond6, label %bb._0xd9, label %bb._0x36, !notdec.evm !23

bb._0xd9:                                         ; preds = %bb._0x1b
  call void @evm_mstore(ptr %mem, i256 0, i256 2937324844), !notdec.evm !24
  %evm.gas7 = call i256 @evm_gas(ptr %env), !notdec.evm !25
  %evm.call8 = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas7, i256 637623786016002795638372688177690688039170113148, i256 0, i256 0, i256 4, i256 0, i256 0), !notdec.evm !26
  call void @evm_mstore(ptr %mem, i256 0, i256 2835717307), !notdec.evm !27
  call void @evm_mstore(ptr %mem, i256 4, i256 1128921706123971286396643566168932927452814570508), !notdec.evm !28
  %evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 68), !notdec.evm !29
  %evm.mul = mul i256 3, %evm.calldataload, !notdec.evm !30
  %evm.div = call i256 @evm_div(i256 %evm.mul, i256 997), !notdec.evm !31
  %evm.add = add i256 1, %evm.div, !notdec.evm !32
  %evm.add9 = add i256 %evm.add, %evm.calldataload, !notdec.evm !33
  call void @evm_mstore(ptr %mem, i256 36, i256 %evm.add9), !notdec.evm !34
  %evm.gas10 = call i256 @evm_gas(ptr %env), !notdec.evm !35
  %evm.call11 = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 %evm.gas10, i256 1212706757719390328488828735575087460932363329889, i256 0, i256 0, i256 68, i256 0, i256 0), !notdec.evm !36
  ret void, !notdec.evm !37

bb._0x36:                                         ; preds = %bb._0x1b
  call void @evm_revert(ptr %mem, i256 0, i256 0), !notdec.evm !38
  unreachable, !notdec.evm !38
}

!0 = !{!"tac=0x0", !"op=CALLER", !"evm.pc=0x0"}
!1 = !{!"tac=0x16", !"op=EQ", !"evm.pc=0x16"}
!2 = !{!"tac=0x1a", !"op=JUMPI", !"evm.pc=0x1a"}
!3 = !{!"tac=0x40", !"op=MSTORE", !"evm.pc=0x40"}
!4 = !{!"tac=0x5c", !"op=GAS", !"evm.pc=0x5c"}
!5 = !{!"tac=0x5d", !"op=STATICCALL", !"evm.pc=0x5d"}
!6 = !{!"tac=0x66", !"op=SHR", !"evm.pc=0x66"}
!7 = !{!"tac=0x68", !"op=MSTORE", !"evm.pc=0x68"}
!8 = !{!"tac=0x69", !"op=ADDRESS", !"evm.pc=0x69"}
!9 = !{!"tac=0x6c", !"op=MSTORE", !"evm.pc=0x6c"}
!10 = !{!"tac=0x71", !"op=MSTORE", !"evm.pc=0x71"}
!11 = !{!"tac=0x76", !"op=MSTORE", !"evm.pc=0x76"}
!12 = !{!"tac=0x79", !"op=MLOAD", !"evm.pc=0x79"}
!13 = !{!"tac=0x7c", !"op=MSTORE", !"evm.pc=0x7c"}
!14 = !{!"tac=0x99", !"op=GAS", !"evm.pc=0x99"}
!15 = !{!"tac=0x9a", !"op=CALL", !"evm.pc=0x9a"}
!16 = !{!"tac=0xa2", !"op=MSTORE", !"evm.pc=0xa2"}
!17 = !{!"tac=0xba", !"op=MSTORE", !"evm.pc=0xba"}
!18 = !{!"tac=0xd6", !"op=GAS", !"evm.pc=0xd6"}
!19 = !{!"tac=0xd7", !"op=CALL", !"evm.pc=0xd7"}
!20 = !{!"tac=0xd8", !"op=STOP", !"evm.pc=0xd8"}
!21 = !{!"tac=0x1b", !"op=ORIGIN", !"evm.pc=0x1b"}
!22 = !{!"tac=0x31", !"op=EQ", !"evm.pc=0x31"}
!23 = !{!"tac=0x35", !"op=JUMPI", !"evm.pc=0x35"}
!24 = !{!"tac=0xe0", !"op=MSTORE", !"evm.pc=0xe0"}
!25 = !{!"tac=0xfc", !"op=GAS", !"evm.pc=0xfc"}
!26 = !{!"tac=0xfd", !"op=CALL", !"evm.pc=0xfd"}
!27 = !{!"tac=0x105", !"op=MSTORE", !"evm.pc=0x105"}
!28 = !{!"tac=0x11d", !"op=MSTORE", !"evm.pc=0x11d"}
!29 = !{!"tac=0x120", !"op=CALLDATALOAD", !"evm.pc=0x120"}
!30 = !{!"tac=0x124", !"op=MUL", !"evm.pc=0x124"}
!31 = !{!"tac=0x129", !"op=DIV", !"evm.pc=0x129"}
!32 = !{!"tac=0x12c", !"op=ADD", !"evm.pc=0x12c"}
!33 = !{!"tac=0x12d", !"op=ADD", !"evm.pc=0x12d"}
!34 = !{!"tac=0x130", !"op=MSTORE", !"evm.pc=0x130"}
!35 = !{!"tac=0x14c", !"op=GAS", !"evm.pc=0x14c"}
!36 = !{!"tac=0x14d", !"op=CALL", !"evm.pc=0x14d"}
!37 = !{!"tac=0x14e", !"op=STOP", !"evm.pc=0x14e"}
!38 = !{!"tac=0x38", !"op=REVERT", !"evm.pc=0x38"}
