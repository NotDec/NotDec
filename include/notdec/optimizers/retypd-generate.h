#ifndef _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_
#define _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_

#include <cassert>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>

#include "Retypd/Schema.h"
#include "Utils/Range.h"

namespace notdec {

using namespace llvm;
using retypd::DerivedTypeVariable;

struct Retypd : PassInfoMixin<Retypd> {
  PreservedAnalyses run(Module &F, ModuleAnalysisManager &);
};

inline retypd::SubTypeConstraint makeCons(const DerivedTypeVariable &sub,
                                          const DerivedTypeVariable &sup) {
  return retypd::SubTypeConstraint{sub, sup};
}

struct RetypdGenerator {
  std::map<Value *, DerivedTypeVariable> Val2Dtv;
  std::map<Function *, std::vector<retypd::Constraint>> func_constrains;
  std::string data_layout;
  std::vector<retypd::Constraint> *current;
  llvm::Value *StackPointer;
  unsigned pointer_size = 0;

  void run(Module &M, llvm::Value *StackPointer);

protected:
  std::map<std::string, long> callInstanceId;
  void run(Function &M);

public:
  static const char *Stack;
  static const char *Memory;

  void gen_json(std::string OutputFilename);

  DerivedTypeVariable &setTypeVar(Value *val, const DerivedTypeVariable &dtv) {
    auto ref = Val2Dtv.emplace(val, dtv);
    assert(ref.second && "setTypeVar: Value already exists");
    return ref.first->second;
  }
  void addConstraint(const DerivedTypeVariable &sub,
                     const DerivedTypeVariable &sup) {
    addConstraint(makeCons(sub, sup));
  }
  void addConstraint(retypd::Constraint Cons) {
    if (std::holds_alternative<retypd::SubTypeConstraint>(Cons)) {
      auto &SCons = std::get<retypd::SubTypeConstraint>(Cons);
      if (SCons.sub.isPrimitive() && SCons.sup.isPrimitive()) {
        assert(SCons.sub.name == SCons.sup.name &&
               "addConstraint: different primitive types !?");
        return;
      }
    }

    current->push_back(Cons);
  }
  DerivedTypeVariable &getTypeVar(Value *val);
  DerivedTypeVariable getTypeVarNoCache(Value *Val);
  // get a new type_var name from type_val_id
  std::string getNewName(Value &Val, const char *prefix = "v_");
  std::string getFuncName(Function &F);

  void setOffset(DerivedTypeVariable &dtv, OffsetRange Offset);
  DerivedTypeVariable deref(Value *Val, long BitSize, bool isLoad);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<AddrSpaceCastInst, BitCastInst, PtrToIntInst,
                     IntToPtrInst>(Val);
  }
  static std::string offset(APInt Offset, int Count = 0);
  static std::string sanitize_name(std::string s);

protected:
  size_t typeValId = 0;

  struct PcodeOpType {
    const char *output;
    // allow for uniform initializations
    const char **inputs;
    int size;

  public:
    PcodeOpType(const char *output, const char *inputs[], int size)
        : output(output), inputs(inputs), size(size) {}
    void addConstrains(Instruction *I, RetypdGenerator &cg) const;
  };

  static const std::map<std::string_view, std::map<int, std::string_view>>
      typeSize;
  static const std::map<unsigned, PcodeOpType> opTypes;

  // visitor class
  class RetypdGeneratorVisitor
      : public llvm::InstVisitor<RetypdGeneratorVisitor> {
    RetypdGenerator &cg;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    RetypdGeneratorVisitor(RetypdGenerator &cg,
                           std::vector<retypd::Constraint> &constrains)
        : cg(cg) {}
    // overloaded visit functions
    void visitCastInst(CastInst &I);
    void visitCallBase(CallBase &I);
    void visitReturnInst(ReturnInst &I);
    void visitPHINode(PHINode &I);
    void visitLoadInst(LoadInst &I);
    void visitStoreInst(StoreInst &I);
    void visitAllocaInst(AllocaInst &I);
    void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitICmpInst(ICmpInst &I);
    void visitFCmpInst(FCmpInst &I);
    void visitSelectInst(SelectInst &I);

    void visitAdd(BinaryOperator &I);
    void visitSub(BinaryOperator &I);
    void visitMul(BinaryOperator &I);

    void visitShl(BinaryOperator &I);
    void visitLShr(BinaryOperator &I);
    void visitAShr(BinaryOperator &I);
    // handle sth like
    // 1. Alignment/ Use lowest bits in pointer: And %x, 0xffffff00.
    // 2. set lowest bits in the pointer: Or %x, 0x7
    void visitAnd(BinaryOperator &I);
    void visitOr(BinaryOperator &I);

    // ignore control flow related instructions
    void visitUnreachableInst(UnreachableInst &I) {}
    void visitBranchInst(BranchInst &I) {}
    void handlePHINodes();
    // use opTypes to handle other insts.
    void visitInstruction(Instruction &I);
  };
};

} // namespace notdec

#endif
