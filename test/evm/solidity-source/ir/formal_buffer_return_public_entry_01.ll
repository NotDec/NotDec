; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare void @evm_return(ptr, i256, i256)

define void @public_word_return_formal_buffer_uint256__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %buffer, i256 %size, i256 %cont) {
entry:
  %arg0 = call i256 @evm_calldataload(ptr %calldata, i256 4)
  %buffer.ptr = inttoptr i256 %buffer to ptr
  store i256 %arg0, ptr %buffer.ptr, align 1
  call void @evm_return(ptr %mem, i256 %buffer, i256 32)
  ret void
}
