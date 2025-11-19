#ifndef _NOTDEC_SIMPLESUB_RECOVERY_H
#define _NOTDEC_SIMPLESUB_RECOVERY_H

#include <cassert>
#include <iostream>
#include <string>

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>

namespace notdec::simplesub {

class SimpleSubRecovery {
  const llvm::Module &Mod;
  llvm::ModuleAnalysisManager &MAM;
  SimpleSubRecovery(llvm::Module &Mod, llvm::ModuleAnalysisManager &MAM)
      : Mod(Mod), MAM(MAM) {}

  class SimpleSubVisitor : public llvm::InstVisitor<SimpleSubVisitor> {
    SimpleSubRecovery &tr;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    SimpleSubVisitor(SimpleSubRecovery &tr) : tr(tr) {}

    // overloaded visit functions
    void visitExtractValueInst(llvm::ExtractValueInst &I);
    void visitCastInst(llvm::CastInst &I);
    void visitCallBase(llvm::CallBase &I);
    void visitReturnInst(llvm::ReturnInst &I);
    void visitPHINode(llvm::PHINode &I);
    void visitLoadInst(llvm::LoadInst &I);
    void visitStoreInst(llvm::StoreInst &I);
    void visitAllocaInst(llvm::AllocaInst &I);
    void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
    void visitICmpInst(llvm::ICmpInst &I);
    void visitSelectInst(llvm::SelectInst &I);

    void visitAdd(llvm::BinaryOperator &I);
    void visitSub(llvm::BinaryOperator &I);

    // handle sth like
    // 1. Alignment/ Use lowest bits in pointer: And %x, 0xfffffff0.
    // 2. set lowest bits in the pointer: Or %x, 0x7
    void visitAnd(llvm::BinaryOperator &I);
    void visitOr(llvm::BinaryOperator &I);

    // ignore control flow related instructions
    void visitUnreachableInst(llvm::UnreachableInst &I) {}
    void visitBranchInst(llvm::BranchInst &I) {}
    void visitSwitchInst(llvm::SwitchInst &I) {}

    void handlePHINodes();
    // use opTypes to handle other insts.
    void visitInstruction(llvm::Instruction &I);
  };

public:
  void run();
};

struct SimpleSubRecoveryMain : llvm::PassInfoMixin<SimpleSubRecoveryMain> {

  SimpleSubRecovery &TR;
  SimpleSubRecoveryMain(SimpleSubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    TR.run();
    return llvm::PreservedAnalyses::all();
  }
};

} // namespace notdec::simplesub
#endif
