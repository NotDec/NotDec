; ModuleID = 'notdec.evm.type-recovery.markers'
source_filename = "notdec.evm.type-recovery.markers"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare noalias ptr @calloc(i256, i256)
declare void @notdec_solidity_memory_allocation(i256, i256)
declare void @notdec_solidity_memory_write(i256, i256, i256)
declare void @notdec_solidity_memory_read(i256, i256, i256)
declare void @notdec_solidity_memory_consumer(i256, i256, i256)
declare void @notdec_solidity_abi_return_data_word_write(i256, i256, i256, i256)
declare void @notdec_solidity_event_data_word_write(i256, i256, i256, i256)
declare void @notdec_solidity_external_call_input_word_write(i256, i256, i256, i256)
declare void @notdec_solidity_external_call_output_word_read(i256, i256, i256, i256)

define void @main() {
entry:
  %buf = call ptr @calloc(i256 1, i256 96)
  %addr = ptrtoint ptr %buf to i256
  call void @notdec_solidity_memory_allocation(i256 %addr, i256 96)
  call void @notdec_solidity_memory_write(i256 %addr, i256 0, i256 1)
  call void @notdec_solidity_memory_consumer(i256 %addr, i256 32, i256 1)
  call void @notdec_solidity_abi_return_data_word_write(i256 %addr, i256 0, i256 1, i256 1)
  call void @notdec_solidity_event_data_word_write(i256 %addr, i256 32, i256 2, i256 1)
  call void @notdec_solidity_external_call_input_word_write(i256 %addr, i256 64, i256 3, i256 1)
  call void @notdec_solidity_memory_read(i256 %addr, i256 32, i256 4)
  call void @notdec_solidity_external_call_output_word_read(i256 %addr, i256 64, i256 5, i256 1)
  ret void
}
