; ModuleID = 'notdec.evm.type-recovery.allowed-arch'
source_filename = "notdec.evm.type-recovery.allowed-arch"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_caller()

define i256 @main() #0 {
entry:
  %caller = call i256 @evm_caller()
  ret i256 %caller
}

attributes #0 = { null_pointer_is_valid }
