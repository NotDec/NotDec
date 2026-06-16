target triple = "evm"

declare i256 @evm_sload(i256)

define i256 @bytes_short_data() {
entry:
  %word = call i256 @evm_sload(i256 0)
  %low = and i256 %word, 1
  %is_short = icmp eq i256 %low, 0
  %full_len = lshr i256 %word, 1
  %short_len = and i256 %full_len, 127
  %len = select i1 %is_short, i256 %short_len, i256 %full_len
  %short = and i256 %word, -256
  %ret = add i256 %len, %short
  ret i256 %ret
}
