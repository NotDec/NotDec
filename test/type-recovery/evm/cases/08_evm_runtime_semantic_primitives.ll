; ModuleID = 'notdec.evm.type-recovery.semantic-primitives'
source_filename = "notdec.evm.type-recovery.semantic-primitives"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_caller()
declare i256 @evm_balance(i256)
declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)
declare i256 @evm_call(ptr, ptr, ptr, i256, i256, i256, i256, i256, i256, i256)

define i256 @main(ptr %mem, ptr %returndata, ptr %env, i256 %slot, i256 %target, i256 %in.base, i256 %out.base) #0 {
entry:
  %caller = call i256 @evm_caller()
  %balance = call i256 @evm_balance(i256 %caller)
  %loaded = call i256 @evm_sload(i256 %slot)
  call void @evm_sstore(i256 %slot, i256 %loaded)
  %ok = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 100000, i256 %target, i256 %balance, i256 %in.base, i256 32, i256 %out.base, i256 64)
  ret i256 %ok
}

attributes #0 = { null_pointer_is_valid }
