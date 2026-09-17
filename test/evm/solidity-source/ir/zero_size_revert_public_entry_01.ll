; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare void @evm_revert(ptr, i256, i256)
declare void @evm_sstore(i256, i256)

define void @public_guard_uint256__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %arg0 = call i256 @evm_calldataload(ptr %calldata, i256 4)
  %is.zero = icmp eq i256 %arg0, 0
  br i1 %is.zero, label %revert, label %body

body:
  call void @evm_sstore(i256 0, i256 %arg0)
  ret void

revert:
  call void @evm_revert(ptr %mem, i256 64, i256 0)
  unreachable
}
