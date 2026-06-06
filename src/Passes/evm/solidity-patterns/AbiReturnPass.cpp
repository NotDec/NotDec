#include "Passes/evm/SolidityPatternUtils.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <notdec-llvm2c/Interface.h>

using namespace llvm;

#define DEBUG_TYPE "evm-solidity-patterns"

namespace notdec::passes::evm {
using namespace detail;

STATISTIC(NumAbiReturns, "Number of Solidity ABI return sites found");
STATISTIC(NumAbiReturnNonPointerBaseHTypes,
          "Number of ABI return bases without pointer HType");
STATISTIC(NumAbiReturnNonRecordBaseHTypes,
          "Number of ABI return bases without record pointee HType");
STATISTIC(NumAbiReturnMissingPayloadHTypes,
          "Number of ABI return payloads not recoverable from HType");

namespace {

StringRef classifyAbiReturnSize(Value *Size) {
  if (isConstantIntValue(Size, 32)) {
    return "static_1_word";
  }
  if (isReturndataSize(Size)) {
    return "returndata_forward";
  }
  return "candidate";
}

ast::RecordDecl *getRecordPointerPointee(ast::HType *Ty) {
  if (Ty == nullptr) {
    return nullptr;
  }
  if (Ty->isPointerType()) {
    ast::HType *Pointee = Ty->getPointeeType();
    return Pointee == nullptr ? nullptr : Pointee->getAsRecordDecl();
  }
  if (auto *Inter = dyn_cast<ast::SetInterType>(Ty)) {
    for (ast::HType *Term : Inter->getTypes()) {
      if (ast::RecordDecl *Record = getRecordPointerPointee(Term)) {
        return Record;
      }
    }
  }
  if (auto *Union = dyn_cast<ast::SetUnionType>(Ty)) {
    for (ast::HType *Term : Union->getTypes()) {
      if (ast::RecordDecl *Record = getRecordPointerPointee(Term)) {
        return Record;
      }
    }
  }
  return nullptr;
}

bool containsPointerType(ast::HType *Ty) {
  if (Ty == nullptr) {
    return false;
  }
  if (Ty->isPointerType()) {
    return true;
  }
  if (auto *Inter = dyn_cast<ast::SetInterType>(Ty)) {
    return llvm::any_of(Inter->getTypes(), containsPointerType);
  }
  if (auto *Union = dyn_cast<ast::SetUnionType>(Ty)) {
    return llvm::any_of(Union->getTypes(), containsPointerType);
  }
  return false;
}

ast::RecordDecl *getRecordPointeeHType(llvm2c::HTypeResult &HTypes,
                                       Value *Base, CallBase &Use,
                                       unsigned ArgIndex) {
  ast::HType *Ty =
      HTypes.getDefaultValueType(getExtValuePtr(Base, &Use, ArgIndex));
  ast::RecordDecl *Record = getRecordPointerPointee(Ty);
  if (Record != nullptr) {
    return Record;
  }

  if (Ty == nullptr || !containsPointerType(Ty)) {
    LLVM_DEBUG(dbgs() << "evm abi return: base has no pointer HType: "
                      << *Base << "\n");
    ++NumAbiReturnNonPointerBaseHTypes;
    return nullptr;
  }

  LLVM_DEBUG(dbgs() << "evm abi return: base HType is not record pointer: "
                    << Ty->getAsString() << " for " << *Base << "\n");
  ++NumAbiReturnNonRecordBaseHTypes;
  return nullptr;
}

} // namespace

PreservedAnalyses AbiReturnPass::run(Module &M, ModuleAnalysisManager &MAM) {
  std::unique_ptr<mlsub::MLsubRecovery::Result> &HighTypes =
      TR.getResult(M, MAM);
  if (HighTypes == nullptr) {
    return PreservedAnalyses::all();
  }

  bool Changed = false;

  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }

    LLVMContext &Ctx = F.getContext();
    for (Instruction &I : instructions(F)) {
      auto *Call = dyn_cast<CallBase>(&I);
      if (Call == nullptr || !isCallTo(Call, "evm_return") ||
          Call->arg_size() != 3) {
        continue;
      }

      StringRef Kind = classifyAbiReturnSize(Call->getArgOperand(2));
      Value *ReturnBase = Call->getArgOperand(1);
      Value *ReturnSize = Call->getArgOperand(2);
      if (!isConstantIntValue(ReturnSize, 0)) {
        if (getRecordPointeeHType(*HighTypes, ReturnBase, *Call, 1) ==
            nullptr) {
          ++NumAbiReturnMissingPayloadHTypes;
          continue;
        }
      }
      addStringMetadata(Ctx, I, KIND_SOLIDITY_ABI_RETURN, Kind);
      addStringMetadata(Ctx, F, KIND_SOLIDITY_ABI_RETURN, "true");
      ++NumAbiReturns;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
