target triple = "evm"

declare i256 @evm_sload(i256)
declare i256 @evm_shr(i256, i256)

define i256 @packed_read() {
entry:
  %word = call i256 @evm_sload(i256 0)
  %shifted = call i256 @evm_shr(i256 16, i256 %word)
  %owner = and i256 %shifted, 1461501637330902918203684832716283019655932542975
  ret i256 %owner
}
