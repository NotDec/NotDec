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

STATISTIC(NumEvents, "Number of Solidity event candidates found");
STATISTIC(NumEventNonPointerDataHTypes,
          "Number of event data bases without pointer HType");
STATISTIC(NumEventNonRecordDataHTypes,
          "Number of event data bases without record pointee HType");
STATISTIC(NumEventMissingDataHTypes,
          "Number of event data payloads not recoverable from HType");

namespace {

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
    LLVM_DEBUG(dbgs() << "evm event: data base has no pointer HType: " << *Base
                      << "\n");
    ++NumEventNonPointerDataHTypes;
    return nullptr;
  }

  LLVM_DEBUG(dbgs() << "evm event: data base HType is not record pointer: "
                    << Ty->getAsString() << " for " << *Base << "\n");
  ++NumEventNonRecordDataHTypes;
  return nullptr;
}

bool isEVMLogCall(CallBase &Call) {
  StringRef Name = getCalleeName(&Call);
  return Name.starts_with("evm_log") && Name.size() == 8 &&
         Name.back() >= '0' && Name.back() <= '4' && Call.arg_size() >= 3;
}

} // namespace

PreservedAnalyses EventLogPass::run(Module &M, ModuleAnalysisManager &MAM) {
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
      if (Call == nullptr || !isEVMLogCall(*Call)) {
        continue;
      }

      if (!isConstantIntValue(Call->getArgOperand(2), 0) &&
          getRecordPointeeHType(*HighTypes, Call->getArgOperand(1), *Call, 1) ==
              nullptr) {
        ++NumEventMissingDataHTypes;
      }

      StringRef Name = getCalleeName(Call);
      addStringMetadata(Ctx, I, KIND_SOLIDITY_EVENT,
                        ("topic_count_" + Twine(Name.back())).str());
      ++NumEvents;
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace notdec::passes::evm
