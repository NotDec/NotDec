; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare void @evm_mstore(ptr, i256, i256)
declare void @evm_revert(ptr, i256, i256)
declare i256 @evm_shl(i256, i256)

define void @revert_custom_error_06(ptr %mem) {
entry:
  %selector = call i256 @evm_shl(i256 224, i256 4277009102)
  call void @evm_mstore(ptr %mem, i256 0, i256 %selector)
  call void @evm_revert(ptr %mem, i256 0, i256 36)
  unreachable
}
