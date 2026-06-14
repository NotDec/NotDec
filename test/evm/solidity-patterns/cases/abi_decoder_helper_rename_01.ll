; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_calldataload(ptr, i256)
declare i256 @evm_calldatasize(ptr)
declare void @evm_revert(ptr, i256, i256)
declare void @evm_sstore(i256, i256)

define void @public_use_abi_decoder_helpers__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %word = call i256 @private__0x100_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4)
  %array = call { i256, i256 } @private__0x200_0x200(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 36)
  %array.base = extractvalue { i256, i256 } %array, 0
  %array.len = extractvalue { i256, i256 } %array, 1
  %elem = call i256 @private__0x300_0x300(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %array.base, i256 %array.len, i256 0)
  %sum = add i256 %word, %elem
  call void @evm_sstore(i256 0, i256 %sum)
  ret void
}

define i256 @private__0x100_0x100(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %offset) {
entry:
  %word = call i256 @evm_calldataload(ptr %calldata, i256 %offset)
  ret i256 %word
}

define { i256, i256 } @private__0x200_0x200(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %offset) {
entry:
  %len = call i256 @evm_calldataload(ptr %calldata, i256 %offset)
  %data = add i256 %offset, 32
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %bytes = mul i256 %len, 32
  %end = add i256 %data, %bytes
  %too.short = icmp ugt i256 %end, %size
  br i1 %too.short, label %revert, label %ok

ok:
  %pair0 = insertvalue { i256, i256 } poison, i256 %data, 0
  %pair1 = insertvalue { i256, i256 } %pair0, i256 %len, 1
  ret { i256, i256 } %pair1

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable
}

define i256 @private__0x300_0x300(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %base, i256 %len, i256 %index) {
entry:
  %out.of.bounds = icmp uge i256 %index, %len
  br i1 %out.of.bounds, label %revert, label %ok

ok:
  %scaled = mul i256 %index, 32
  %addr = add i256 %base, %scaled
  ret i256 %addr

revert:
  call void @evm_revert(ptr %mem, i256 0, i256 0)
  unreachable
}
