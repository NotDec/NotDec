; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare i256 @evm_calldatasize(ptr)
declare void @evm_revert(ptr, i256, i256)
declare void @evm_sstore(i256, i256)

; evm2llvm outlines shared code into private__* helpers.  %sum has two uses in
; the same block, so the backend caches it in a local (the llvm2c C backend
; rule: fold only a single same-block use); %word is used once and is folded.
define i256 @private__sum_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x100arg0x0, i256 %_0x100arg0x1) {
bb._0x100:
  %evm.add = add i256 %_0x100arg0x0, %_0x100arg0x1
  ret i256 %evm.add
}

; A void helper keeps its side effect as a call statement.
define void @private__record_0x200(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x200arg0x0) {
bb._0x200:
  call void @evm_sstore(i256 2, i256 %_0x200arg0x0)
  ret void
}

define void @public_two_stores_uint256__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %too.short = icmp ult i256 %size, 36
  br i1 %too.short, label %revert, label %body

body:
  %word = call i256 @evm_calldataload(ptr %calldata, i256 4)
  %sum = call i256 @private__sum_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 5, i256 %word)
  call void @private__record_0x200(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %sum)
  call void @evm_sstore(i256 0, i256 %sum)
  call void @evm_sstore(i256 1, i256 %sum)
  ret void

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable
}

; A helper result with a single use in its own block is folded into that use
; instead of being cached (hasOneUseIgnoreCast + onlyUsedInCurrentBlock).
define void @public_one_store_uint256__0x30(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %too.short = icmp ult i256 %size, 36
  br i1 %too.short, label %revert, label %body

body:
  %word = call i256 @evm_calldataload(ptr %calldata, i256 4)
  %sum = call i256 @private__sum_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 1, i256 %word)
  call void @evm_sstore(i256 3, i256 %sum)
  ret void

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable
}
