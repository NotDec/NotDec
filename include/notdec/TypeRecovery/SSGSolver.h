#ifndef _NOTDEC_SSGSOLVER_H_
#define _NOTDEC_SSGSOLVER_H_

#include "TypeRecovery/StorageShapeGraph.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "Passes/ConstraintGenerator.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>
#include <map>

namespace notdec::retypd {

using namespace llvm;

struct SSGTypeRec : llvm::PassInfoMixin<SSGTypeRec> {
  StorageShapeGraph SSG;
  std::map<ExtValPtr, retypd::SSGLink> Val2Node;
  unsigned pointer_size = 0;

  SSGTypeRec() : SSG([](const retypd::ConsNode *Node) {}, "") {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &);

  SSGNode &getOrInsertNode(llvm::Value *Val, User *User = nullptr);
  SSGNode &getOrInsertNode(ExtValPtr Key, unsigned int Size,
                           User *User = nullptr);
  void setAsPtr(SSGNode &Key);
  void setAsNonPtr(SSGNode &Key);
  void addSubTypeCons(SSGNode &DestNode, SSGNode &SrcNode,
                      OffsetRange Offset = {0});

public:
  void print(Module &M, std::string path);
};

struct SSGVisitor : public llvm::InstVisitor<SSGVisitor> {
  SSGTypeRec &Ctx;
  // defer phi node constraints
  std::vector<llvm::PHINode *> phiNodes;

public:
  SSGVisitor(SSGTypeRec &Ctx) : Ctx(Ctx) {}
  // overloaded visit functions
  void visitCastInst(CastInst &I);
  void visitCallBase(CallBase &I);
  // void visitReturnInst(ReturnInst &I);
  // void visitPHINode(PHINode &I);
  void visitLoadInst(LoadInst &I);
  void visitStoreInst(StoreInst &I);
  void visitAllocaInst(AllocaInst &I);
  // void visitGetElementPtrInst(GetElementPtrInst &I);
  // void visitICmpInst(ICmpInst &I);
  // void visitSelectInst(SelectInst &I);

  void visitAdd(BinaryOperator &I);
  void visitSub(BinaryOperator &I);

  // handle sth like
  // 1. Alignment/ Use lowest bits in pointer: And %x, 0xfffffff0.
  // 2. set lowest bits in the pointer: Or %x, 0x7
  // void visitAnd(BinaryOperator &I);
  // void visitOr(BinaryOperator &I);

  // ignore control flow related instructions
  void visitUnreachableInst(UnreachableInst &I) {}
  void visitBranchInst(BranchInst &I) {}
  // void handlePHINodes();
  // use opTypes to handle other insts.
  void visitInstruction(Instruction &I);
};

} // namespace notdec::retypd

#endif
