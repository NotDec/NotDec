; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare i256 @evm_calldatasize(ptr)
declare void @evm_revert(ptr, i256, i256)
declare void @evm_sstore(i256, i256)

define { i256, i256 } @private__0x100_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %base, i256 %size, i256 %cont) {
entry:
  %available = sub i256 %size, %base
  %too.short = icmp sgt i256 %available, 63
  br i1 %too.short, label %decode, label %revert

decode:
  %w0 = call i256 @evm_calldataload(ptr %calldata, i256 %base)
  %second = add i256 %base, 32
  %w1 = call i256 @evm_calldataload(ptr %calldata, i256 %second)
  %first = insertvalue { i256, i256 } poison, i256 %w1, 0
  %result = insertvalue { i256, i256 } %first, i256 %w0, 1
  ret { i256, i256 } %result

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable
}

define void @public_two_args_uint256_uint256__0x200(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %args = call { i256, i256 } @private__0x100_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4, i256 %size, i256 999)
  %arg1 = extractvalue { i256, i256 } %args, 0
  %arg0 = extractvalue { i256, i256 } %args, 1
  call void @evm_sstore(i256 0, i256 %arg1)
  call void @evm_sstore(i256 1, i256 %arg0)
  ret void
}
