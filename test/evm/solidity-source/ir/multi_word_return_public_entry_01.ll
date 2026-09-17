; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare ptr @calloc_unbounded()
declare void @evm_return(ptr, i256, i256)

define void @public_two_returns_uint256_uint256__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %arg0 = call i256 @evm_calldataload(ptr %calldata, i256 4)
  %arg1 = call i256 @evm_calldataload(ptr %calldata, i256 36)
  %buffer = call ptr @calloc_unbounded()
  %base = ptrtoint ptr %buffer to i256
  store i256 %arg0, ptr %buffer, align 1
  %second.addr = add i256 %base, 32
  %second.ptr = inttoptr i256 %second.addr to ptr
  store i256 %arg1, ptr %second.ptr, align 1
  call void @evm_return(ptr %mem, i256 %base, i256 64)
  ret void
}
