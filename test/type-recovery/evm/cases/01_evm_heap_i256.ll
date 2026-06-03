; ModuleID = 'notdec.evm.type-recovery'
source_filename = "notdec.evm.type-recovery"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare noalias ptr @calloc(i256, i256)
declare noalias ptr @calloc_unbounded()
declare void @notdec_evm_finalize_alloc(i256, i256)
declare void @evm_return(ptr, i256, i256)

define void @main(ptr %ctx) #0 {
entry:
  %buf = call ptr @calloc(i256 1, i256 64)
  %addr = ptrtoint ptr %buf to i256
  %buf.again = inttoptr i256 %addr to ptr
  store i256 42, ptr %buf.again, align 32
  %word = load i256, ptr %buf.again, align 32
  %scratch = call ptr @calloc_unbounded()
  %scratch.addr = ptrtoint ptr %scratch to i256
  call void @notdec_evm_finalize_alloc(i256 %scratch.addr, i256 96)
  call void @evm_return(ptr %ctx, i256 %addr, i256 32)
  ret void
}

attributes #0 = { null_pointer_is_valid }
