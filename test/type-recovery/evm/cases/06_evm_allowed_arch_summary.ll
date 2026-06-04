; ModuleID = 'notdec.evm.type-recovery.allowed-arch'
source_filename = "notdec.evm.type-recovery.allowed-arch"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @notdec_test_semantic_value()

define i256 @main() #0 {
entry:
  %value = call i256 @notdec_test_semantic_value()
  ret i256 %value
}

attributes #0 = { null_pointer_is_valid }
