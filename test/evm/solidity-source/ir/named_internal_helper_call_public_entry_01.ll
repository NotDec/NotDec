; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare i256 @evm_calldatasize(ptr)
declare void @evm_revert(ptr, i256, i256)
declare void @evm_sstore(i256, i256)

; evm2llvm names an outlined helper from the recovered high-level name:
; "private_" + sanitize(<name> + "_" + <fact id>), so it is private_<name>_<id>
; and NOT private__<id>_<id>.  It is still an internal helper and must be
; classified as one.
define internal i256 @private_sum_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x100arg0x0, i256 %_0x100arg0x1) {
bb._0x100:
  %evm.add = add i256 %_0x100arg0x0, %_0x100arg0x1
  ret i256 %evm.add
}

; Since 2026-09-18 evm2llvm also emits helpers with internal linkage, which is
; the authoritative "not part of the ABI surface" signal.
define internal i256 @internal_helper_0x200(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %_0x200arg0x0) {
bb._0x200:
  %evm.mul = mul i256 %_0x200arg0x0, 3
  ret i256 %evm.mul
}

define void @public_calls_helpers_uint256__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %too.short = icmp ult i256 %size, 36
  br i1 %too.short, label %revert, label %body

body:
  %word = call i256 @evm_calldataload(ptr %calldata, i256 4)
  %sum = call i256 @private_sum_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 5, i256 %word)
  %triple = call i256 @internal_helper_0x200(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %sum)
  call void @evm_sstore(i256 0, i256 %triple)
  ret void

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable
}
