target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"
declare void @evm_sstore(i256, i256)
define void @public_a__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %b = ptrtoint ptr %calldata to i256
  %a = add i256 %b, 4
  %p = inttoptr i256 %a to ptr
  %v = load i256, ptr %p, align 1
  call void @evm_sstore(i256 4, i256 %v)
  ret void
}
define void @public_b__0x20(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %b = ptrtoint ptr %calldata to i256
  %a = add i256 %b, 36
  %p = inttoptr i256 %a to ptr
  %v = load i256, ptr %p, align 1
  call void @evm_sstore(i256 36, i256 %v)
  ret void
}
