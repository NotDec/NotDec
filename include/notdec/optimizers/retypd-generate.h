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

#include "Retypd/Graph.h"
#include "Retypd/RExp.h"
#include "Retypd/Schema.h"
#include "Retypd/Unify.h"
#include "Utils/Range.h"

namespace notdec {

using namespace llvm;
using retypd::DerivedTypeVariable;

struct RetypdGenerator;
struct Retypd : PassInfoMixin<Retypd> {
  std::map<Function *, RetypdGenerator> func_ctxs;
  llvm::Value *StackPointer;
  std::string data_layout;
  unsigned pointer_size = 0;

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
  void gen_json(std::string OutputFilename);

  static std::string sanitize_name(std::string s);

protected:
  size_t typeValId = 0;

public:
  // get a new type_var name from type_val_id
  std::string getName(Value &Val, const char *prefix = "v_");
};

inline retypd::SubTypeConstraint makeCons(const DerivedTypeVariable &sub,
                                          const DerivedTypeVariable &sup) {
  return retypd::SubTypeConstraint{sub, sup};
}

struct RetypdGenerator {
  Retypd &Ctx;
  std::map<Value *, retypd::CGNode *> Val2Dtv;
  retypd::ConstraintGraph CG;
  retypd::StorageShapeGraph SSG;

  void run(Function &M);
  RetypdGenerator(Retypd &Ctx) : Ctx(Ctx) {}

protected:
  std::map<std::string, long> callInstanceId;

public:
  static const char *Stack;
  static const char *Memory;

  const DerivedTypeVariable &setTypeVar(Value *val,
                                        const DerivedTypeVariable &dtv) {
    auto ref = Val2Dtv.emplace(val, &CG.getOrInsertNode(dtv));
    ref.first->second->Link.setNode(SSG.createUnknown());
    assert(ref.second && "setTypeVar: Value already exists");
    return ref.first->second->key.Base;
  }
  void addConstraint(const DerivedTypeVariable &sub,
                     const DerivedTypeVariable &sup) {
    if (sub.isPrimitive() && sup.isPrimitive()) {
      assert(sub.name == sup.name &&
             "addConstraint: different primitive types !?");
      return;
    }
    CG.addConstraint(sub, sup);
    CG.getOrInsertNode(sub).Link.unify(CG.getOrInsertNode(sup).Link);
  }
  const DerivedTypeVariable &getTypeVar(Value *val);
  DerivedTypeVariable getTypeVarNoCache(Value *Val);

  void setOffset(DerivedTypeVariable &dtv, OffsetRange Offset);
  DerivedTypeVariable deref(Value *Val, long BitSize, bool isLoad);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<AddrSpaceCastInst, BitCastInst, PtrToIntInst,
                     IntToPtrInst>(Val);
  }
  static std::string offset(APInt Offset, int Count = 0);

protected:
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
    RetypdGeneratorVisitor(RetypdGenerator &cg) : cg(cg) {}
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
