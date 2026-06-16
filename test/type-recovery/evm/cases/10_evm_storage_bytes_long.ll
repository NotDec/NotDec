target triple = "evm"

declare i256 @evm_sload(i256)
declare i256 @evm_sha3(i256, i256, i256)

define i256 @bytes_long_elem(i256 %i) {
entry:
  %word = call i256 @evm_sload(i256 0)
  %low = and i256 %word, 1
  %is_short = icmp eq i256 %low, 0
  %full_len = lshr i256 %word, 1
  %short_len = and i256 %full_len, 127
  %len = select i1 %is_short, i256 %short_len, i256 %full_len
  store i256 0, ptr inttoptr (i256 0 to ptr)
  %base = call i256 @evm_sha3(i256 0, i256 0, i256 32)
  %elem_slot = add i256 %base, %i
  %chunk = call i256 @evm_sload(i256 %elem_slot)
  %ret = add i256 %len, %chunk
  ret i256 %ret
}
