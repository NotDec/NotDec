; ModuleID = 'notdec.evm.type-recovery.aggregate-return-pndiff-sub'
source_filename = "notdec.evm.type-recovery.aggregate-return-pndiff-sub"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

define { i256, i256 } @private_pair(i256 %base, i256 %delta) #0 {
entry:
  %addr = sub i256 %base, %delta
  %ptr = inttoptr i256 %addr to ptr
  store i256 7, ptr %ptr, align 1
  %ret0 = insertvalue { i256, i256 } poison, i256 %addr, 0
  %ret1 = insertvalue { i256, i256 } %ret0, i256 %delta, 1
  ret { i256, i256 } %ret1
}

define void @main(i256 %base, i256 %delta) #0 {
entry:
  %pair = call { i256, i256 } @private_pair(i256 %base, i256 %delta)
  %addr = extractvalue { i256, i256 } %pair, 0
  %amount = extractvalue { i256, i256 } %pair, 1
  %ptr = inttoptr i256 %addr to ptr
  %word = load i256, ptr %ptr, align 1
  %sum = add i256 %word, %amount
  %out = inttoptr i256 96 to ptr
  store i256 %sum, ptr %out, align 1
  ret void
}

attributes #0 = { null_pointer_is_valid }
