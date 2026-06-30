; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

define void @public___function_selector___0x0(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) #0 {
bb._0x0:
  ret void, !notdec.evm !0
}

attributes #0 = { null_pointer_is_valid }

!0 = !{!"tac=0x0", !"op=STOP", !"evm.pc=0x0"}
