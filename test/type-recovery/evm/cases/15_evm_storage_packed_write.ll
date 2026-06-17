target triple = "evm"

declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)

define void @packed_write(i256 %v) {
entry:
  %old = call i256 @evm_sload(i256 0)
  %cleared = and i256 %old, 1461501637330902918203684832716283019655932542975
  %shifted = shl i256 %v, 160
  %merged = or i256 %cleared, %shifted
  call void @evm_sstore(i256 0, i256 %merged)
  ret void
}
