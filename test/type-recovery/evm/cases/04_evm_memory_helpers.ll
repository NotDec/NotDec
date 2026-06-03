; ModuleID = 'notdec.evm.type-recovery.memory-helpers'
source_filename = "notdec.evm.type-recovery.memory-helpers"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_mload(ptr, i256)
declare void @evm_mstore(ptr, i256, i256)

define void @main(ptr %mem) {
entry:
  call void @evm_mstore(ptr %mem, i256 0, i256 11)
  %word = call i256 @evm_mload(ptr %mem, i256 32)
  call void @evm_mstore(ptr %mem, i256 64, i256 %word)
  ret void
}
