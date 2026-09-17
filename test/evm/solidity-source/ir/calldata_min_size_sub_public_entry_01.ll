; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare i256 @evm_calldatasize(ptr)
declare void @evm_revert(ptr, i256, i256)
declare void @evm_sstore(i256, i256)

define void @public_calldata_min_size_sub_address_address_uint256__0x20(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %available = sub i256 %size, 4
  %too.short = icmp slt i256 %available, 96
  br i1 %too.short, label %revert, label %body

body:
  %word = call i256 @evm_calldataload(ptr %calldata, i256 68)
  call void @evm_sstore(i256 0, i256 %word)
  ret void

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable
}
