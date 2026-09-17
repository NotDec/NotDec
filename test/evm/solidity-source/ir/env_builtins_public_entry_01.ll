; ModuleID = 'notdec.evm2llvm'
source_filename = "notdec.evm2llvm"
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"

declare i256 @evm_timestamp(ptr)
declare i256 @evm_number(ptr)
declare i256 @evm_gasprice(ptr)
declare i256 @evm_chainid(ptr)
declare i256 @evm_coinbase(ptr)
declare i256 @evm_origin(ptr)
declare i256 @evm_selfbalance(ptr)
declare i256 @evm_address(ptr)
declare void @evm_sstore(i256, i256)

define void @public_env_uint256__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %ts = call i256 @evm_timestamp(ptr %env)
  call void @evm_sstore(i256 0, i256 %ts)
  %num = call i256 @evm_number(ptr %env)
  call void @evm_sstore(i256 1, i256 %num)
  %gp = call i256 @evm_gasprice(ptr %env)
  call void @evm_sstore(i256 2, i256 %gp)
  %cid = call i256 @evm_chainid(ptr %env)
  call void @evm_sstore(i256 3, i256 %cid)
  %sb = call i256 @evm_selfbalance(ptr %env)
  call void @evm_sstore(i256 4, i256 %sb)
  %cb = call i256 @evm_coinbase(ptr %env)
  call void @evm_sstore(i256 5, i256 %cb)
  %org = call i256 @evm_origin(ptr %env)
  call void @evm_sstore(i256 6, i256 %org)
  %addr = call i256 @evm_address(ptr %env)
  call void @evm_sstore(i256 7, i256 %addr)
  ret void
}
