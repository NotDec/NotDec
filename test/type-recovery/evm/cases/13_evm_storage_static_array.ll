target triple = "evm"

declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)

define i256 @static_array(i256 %i, i256 %v) {
entry:
  %slot.store = add i256 0, %i
  call void @evm_sstore(i256 %slot.store, i256 %v)
  %slot.load = add i256 0, %i
  %y = call i256 @evm_sload(i256 %slot.load)
  ret i256 %y
}
