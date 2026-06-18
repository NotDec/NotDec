#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <notdec-llvm2c/Interface.h>

using namespace llvm;

#define DEBUG_TYPE "evm-high-level-type"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumFieldStores, "Number of HType memory field stores rewritten");
STATISTIC(NumFieldLoads, "Number of HType memory field loads rewritten");

namespace {

ast::RecordDecl *getMemoryRecord(llvm2c::HTypeResult &HTypes) {
  if (HTypes.MemoryDecl != nullptr) {
    return HTypes.MemoryDecl;
  }
  ast::HType *Ty = HTypes.MemoryType;
  if (Ty != nullptr && Ty->isPointerType()) {
    Ty = Ty->getPointeeType();
  }
  return Ty == nullptr ? nullptr : Ty->getAsRecordDecl();
}

FunctionCallee getFieldStoreFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.htype.field.store", Type::getVoidTy(Ctx),
                               I256, I256, I256);
}

FunctionCallee getFieldLoadFn(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *I256 = Type::getIntNTy(Ctx, 256);
  return M.getOrInsertFunction("evm.htype.field.load", I256, I256, I256);
}

ConstantInt *getI256(LLVMContext &Ctx, uint64_t Value) {
  return ConstantInt::get(Type::getIntNTy(Ctx, 256), Value);
}

Value *getValueFromExtValue(const ExtValuePtr &Value) {
  if (auto *V = std::get_if<llvm::Value *>(&Value)) {
    return *V;
  }
  if (auto *C = std::get_if<UConstant>(&Value)) {
    return C->Val;
  }
  if (auto *Addr = std::get_if<ConstantAddr>(&Value)) {
    return Addr->Val;
  }
  return nullptr;
}

std::optional<uint64_t> getMemoryOffset(const ExtValuePtr &Addr) {
  Value *V = getValueFromExtValue(Addr);
  if (V == nullptr) {
    return std::nullopt;
  }
  if (isa<ConstantPointerNull>(V)) {
    return 0;
  }
  if (auto *Cast = dyn_cast<IntToPtrInst>(V)) {
    V = Cast->getOperand(0);
  }
  return getUInt64Constant(V);
}

bool rewriteMemoryFieldStore(Module &M, llvm2c::HTypeResult &HTypes,
                             const mlsub::EVMStoreEvidence &Store) {
  ast::RecordDecl *MemoryRecord = getMemoryRecord(HTypes);
  if (Store.Source == nullptr || Store.StoredValue == nullptr ||
      Store.BitSize != 256 || MemoryRecord == nullptr) {
    return false;
  }

  auto *SI = dyn_cast<StoreInst>(Store.Source);
  if (SI == nullptr) {
    return false;
  }

  std::optional<uint64_t> Offset = getMemoryOffset(Store.Addr);
  if (!Offset.has_value()) {
    LLVM_DEBUG(dbgs() << "evm high-level type: skip store without constant "
                         "memory offset: "
                      << *SI << "\n");
    return false;
  }
  if (MemoryRecord->getFieldAt(static_cast<OffsetTy>(*Offset)) == nullptr) {
    LLVM_DEBUG(dbgs() << "evm high-level type: skip store without HType field "
                         "at offset "
                      << *Offset << ": " << *SI << "\n");
    return false;
  }

  IRBuilder<> Builder(SI);
  LLVMContext &Ctx = M.getContext();
  // EVM memory is a single logical object in current HType.  Use base 0 and
  // the recovered field offset rather than inventing a separate object model.
  Builder.CreateCall(getFieldStoreFn(M),
                     {getI256(Ctx, 0), getI256(Ctx, *Offset),
                      Store.StoredValue});
  SI->eraseFromParent();
  ++NumFieldStores;
  return true;
}

bool rewriteMemoryFieldLoad(Module &M, llvm2c::HTypeResult &HTypes,
                            const mlsub::EVMLoadEvidence &Load) {
  ast::RecordDecl *MemoryRecord = getMemoryRecord(HTypes);
  if (Load.Source == nullptr || Load.LoadedValue == nullptr ||
      Load.BitSize != 256 || MemoryRecord == nullptr) {
    return false;
  }

  auto *LI = dyn_cast<LoadInst>(Load.Source);
  if (LI == nullptr) {
    return false;
  }

  std::optional<uint64_t> Offset = getMemoryOffset(Load.Addr);
  if (!Offset.has_value()) {
    LLVM_DEBUG(dbgs() << "evm high-level type: skip load without constant "
                         "memory offset: "
                      << *LI << "\n");
    return false;
  }
  if (MemoryRecord->getFieldAt(static_cast<OffsetTy>(*Offset)) == nullptr) {
    LLVM_DEBUG(dbgs() << "evm high-level type: skip load without HType field "
                         "at offset "
                      << *Offset << ": " << *LI << "\n");
    return false;
  }

  IRBuilder<> Builder(LI);
  LLVMContext &Ctx = M.getContext();
  Value *Call =
      Builder.CreateCall(getFieldLoadFn(M), {getI256(Ctx, 0),
                                             getI256(Ctx, *Offset)});
  LI->replaceAllUsesWith(Call);
  LI->eraseFromParent();
  ++NumFieldLoads;
  return true;
}

} // namespace

PreservedAnalyses EvmHighLevelTypePass::run(Module &M,
                                            ModuleAnalysisManager &MAM) {
  std::unique_ptr<llvm2c::HTypeResult> &HighTypes = TR.getResult(M, MAM);
  if (!HighTypes || getMemoryRecord(*HighTypes) == nullptr) {
    LLVM_DEBUG(dbgs() << "evm high-level type: no memory HType\n");
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  SmallVector<mlsub::EVMLoadEvidence, 16> Loads(TR.getEVMLoadEvidence());
  SmallVector<mlsub::EVMStoreEvidence, 16> Stores(TR.getEVMStoreEvidence());
  LLVM_DEBUG(dbgs() << "evm high-level type: load evidence count "
                    << Loads.size() << "\n");
  LLVM_DEBUG(dbgs() << "evm high-level type: store evidence count "
                    << Stores.size() << "\n");
  for (const mlsub::EVMStoreEvidence &Store : Stores) {
    Changed |= rewriteMemoryFieldStore(M, *HighTypes, Store);
  }
  for (const mlsub::EVMLoadEvidence &Load : Loads) {
    Changed |= rewriteMemoryFieldLoad(M, *HighTypes, Load);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
