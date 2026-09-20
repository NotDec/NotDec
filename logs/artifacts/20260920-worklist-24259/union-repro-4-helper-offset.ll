target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"
declare void @evm_sstore(i256, i256)
define internal i256 @shared_h(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %off) {
entry:
  %b = ptrtoint ptr %calldata to i256
  %a = add i256 %b, %off
  %p = inttoptr i256 %a to ptr
  %v = load i256, ptr %p, align 1
  ret i256 %v
}
define void @public_a__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %r = call i256 @shared_h(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4)
  call void @evm_sstore(i256 4, i256 %r)
  ret void
}
define void @public_b__0x20(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %r = call i256 @shared_h(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 36)
  call void @evm_sstore(i256 36, i256 %r)
  ret void
}
