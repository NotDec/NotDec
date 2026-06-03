; ModuleID = 'notdec.evm.type-recovery.memory-helpers'
source_filename = "notdec.evm.type-recovery.memory-helpers"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"


define void @main(ptr %mem) #0 {
entry:
  %notdec.evm.mem.ptr.0 = inttoptr i256 0 to ptr
  store i256 11, ptr %notdec.evm.mem.ptr.0, align 1
  %notdec.evm.mem.ptr.1 = inttoptr i256 32 to ptr
  %word = load i256, ptr %notdec.evm.mem.ptr.1, align 1
  %notdec.evm.mem.ptr.2 = inttoptr i256 64 to ptr
  store i256 %word, ptr %notdec.evm.mem.ptr.2, align 1
  ret void
}

attributes #0 = { null_pointer_is_valid }
