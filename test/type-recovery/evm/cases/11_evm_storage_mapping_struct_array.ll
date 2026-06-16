target triple = "evm"

declare i256 @evm_sha3(i256, i256, i256)
declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)

define i256 @mapping_struct_array(i256 %key, i256 %i, i256 %v) {
entry:
  store i256 %key, ptr inttoptr (i256 0 to ptr)
  store i256 4, ptr inttoptr (i256 32 to ptr)
  %user = call i256 @evm_sha3(i256 0, i256 0, i256 64)
  %history = add i256 %user, 1
  %len = call i256 @evm_sload(i256 %history)

  store i256 %history, ptr inttoptr (i256 0 to ptr)
  %base.store = call i256 @evm_sha3(i256 0, i256 0, i256 32)
  %slot.store = add i256 %base.store, %i
  call void @evm_sstore(i256 %slot.store, i256 %v)

  store i256 %history, ptr inttoptr (i256 0 to ptr)
  %base.load = call i256 @evm_sha3(i256 0, i256 0, i256 32)
  %slot.load = add i256 %base.load, %i
  %y = call i256 @evm_sload(i256 %slot.load)
  %r = add i256 %y, %len
  ret i256 %r
}
