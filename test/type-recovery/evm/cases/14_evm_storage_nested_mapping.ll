target triple = "evm"

declare i256 @evm_sha3(i256, i256, i256)
declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)

define i256 @nested_mapping(i256 %owner, i256 %spender, i256 %v) {
entry:
  store i256 %owner, ptr inttoptr (i256 0 to ptr)
  store i256 1, ptr inttoptr (i256 32 to ptr)
  %slot1.store = call i256 @evm_sha3(i256 0, i256 0, i256 64)
  store i256 %spender, ptr inttoptr (i256 0 to ptr)
  store i256 %slot1.store, ptr inttoptr (i256 32 to ptr)
  %slot2.store = call i256 @evm_sha3(i256 0, i256 0, i256 64)
  call void @evm_sstore(i256 %slot2.store, i256 %v)

  store i256 %owner, ptr inttoptr (i256 0 to ptr)
  store i256 1, ptr inttoptr (i256 32 to ptr)
  %slot1.load = call i256 @evm_sha3(i256 0, i256 0, i256 64)
  store i256 %spender, ptr inttoptr (i256 0 to ptr)
  store i256 %slot1.load, ptr inttoptr (i256 32 to ptr)
  %slot2.load = call i256 @evm_sha3(i256 0, i256 0, i256 64)
  %y = call i256 @evm_sload(i256 %slot2.load)
  ret i256 %y
}
