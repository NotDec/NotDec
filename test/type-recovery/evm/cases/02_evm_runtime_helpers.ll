; ModuleID = 'notdec.evm.type-recovery.helpers'
source_filename = "notdec.evm.type-recovery.helpers"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @notdec_evm_alloc(i256)
declare i256 @notdec_evm_alloc_unbounded()
declare void @notdec_evm_finalize_alloc(i256, i256)
declare void @evm_log1(ptr, i256, i256, i256)
declare i256 @evm_call(ptr, ptr, ptr, i256, i256, i256, i256, i256, i256, i256)

define i256 @main(ptr %mem, ptr %returndata, ptr %env) {
entry:
  %buf.addr = call i256 @notdec_evm_alloc(i256 64)
  %buf.ptr = inttoptr i256 %buf.addr to ptr
  store i256 7, ptr %buf.ptr, align 32
  %scratch.addr = call i256 @notdec_evm_alloc_unbounded()
  call void @notdec_evm_finalize_alloc(i256 %scratch.addr, i256 128)
  call void @evm_log1(ptr %mem, i256 %buf.addr, i256 32, i256 4660)
  %ok = call i256 @evm_call(ptr %mem, ptr %returndata, ptr %env, i256 100000, i256 1, i256 0, i256 %buf.addr, i256 32, i256 %scratch.addr, i256 64)
  ret i256 %ok
}
