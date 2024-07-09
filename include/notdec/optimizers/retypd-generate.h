#ifndef _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_
#define _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_

#include "Retypd/Schema.h"
#include <cassert>
#include <deque>
#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>

namespace notdec {

using namespace llvm;

struct Retypd : PassInfoMixin<Retypd> {
  PreservedAnalyses run(Module &F, ModuleAnalysisManager &);
};

struct SimpleNumericalDomain {
  using offset_num_t = int64_t;
  using offset_t = std::variant<offset_num_t, std::string>;
  offset_num_t offset = 0;
  retypd::Bound bound = retypd::None{};
  bool isZero() const {
    return offset == 0 && std::holds_alternative<retypd::None>(bound);
  }
};

struct VariableWithOffset {
  retypd::DerivedTypeVariable dtv;
  SimpleNumericalDomain offset;
  bool isPrimitive = false;

  VariableWithOffset(retypd::DerivedTypeVariable dtv,
                     SimpleNumericalDomain offset = {0})
      : dtv(dtv), offset(offset){};

  std::string str() const { return toString(dtv); }
};

inline retypd::SubTypeConstraint makeCons(const VariableWithOffset &sub,
                                          const VariableWithOffset &sup) {
  if (!sub.offset.isZero() || !sup.offset.isZero()) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "makeCons: offset not zero?\n";
  }
  return retypd::SubTypeConstraint{sub.dtv, sup.dtv};
}

struct RetypdGenerator {
  std::map<Value *, VariableWithOffset> Val2Dtv;
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

  VariableWithOffset &setTypeVar(Value *val, const VariableWithOffset &dtv) {
    auto ref = Val2Dtv.emplace(val, dtv);
    assert(ref.second && "setTypeVar: Value already exists");
    return ref.first->second;
  }
  void addConstraint(const VariableWithOffset &sub,
                     const VariableWithOffset &sup) {
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
  VariableWithOffset &getTypeVar(Value *val);
  VariableWithOffset getTypeVarNoCache(Value *Val);
  // get a new type_var name from type_val_id
  std::string getNewName(Value &Val, const char *prefix = "v_");
  std::string getFuncName(Function &F);

  VariableWithOffset deref(Value *val, long BitSize, bool isLoad);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<AddrSpaceCastInst, BitCastInst, PtrToIntInst,
                     IntToPtrInst>(Val);
  }
  static retypd::DerefLabel make_deref(uint32_t BitSize,
                                       SimpleNumericalDomain Offset);
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
    // void visitAllocaInst(AllocaInst &I);
    void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitICmpInst(ICmpInst &I);
    void visitFCmpInst(FCmpInst &I);
    void visitSelectInst(SelectInst &I);
    // use opTypes to handle other insts.
    void visitInstruction(Instruction &I);
    void visitUnreachableInst(UnreachableInst &I) {}
    void visitBranchInst(BranchInst &I) {}
    void handlePHINodes();
  };
};

} // namespace notdec

#endif
