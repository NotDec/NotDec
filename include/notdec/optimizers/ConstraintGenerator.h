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

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/StorageShapeGraph.h"
#include "Utils/Range.h"

namespace notdec {

using namespace llvm;
using retypd::TypeVariable;

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

inline retypd::SubTypeConstraint makeCons(const TypeVariable &sub,
                                          const TypeVariable &sup) {
  return retypd::SubTypeConstraint{sub, sup};
}

struct ReturnValue {
  llvm::Function *Func;
  int32_t Index;
  bool operator<(const ReturnValue &rhs) const {
    return std::tie(Func, Index) < std::tie(rhs.Func, rhs.Index);
  }
  bool operator==(const ReturnValue &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
struct CallArg {
  llvm::CallBase *Call;
  uint32_t InstanceId;
  int32_t Index;
  bool operator<(const CallArg &rhs) const {
    return std::tie(Call, InstanceId, Index) <
           std::tie(rhs.Call, rhs.InstanceId, rhs.Index);
  }
  bool operator==(const CallArg &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
struct CallRet {
  llvm::CallBase *Call;
  uint32_t InstanceId;
  bool operator<(const CallRet &rhs) const {
    return std::tie(Call, InstanceId) < std::tie(rhs.Call, InstanceId);
  }
  bool operator==(const CallRet &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
/// Differentiate pointer-sized int constant. It can be pointer or int under
/// different context.
struct IntConstant {
  llvm::Constant *Val;
  llvm::User *User;
  bool operator<(const IntConstant &rhs) const {
    return std::tie(Val, User) < std::tie(rhs.Val, User);
  }
  bool operator==(const IntConstant &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
using ValMapKey =
    std::variant<llvm::Value *, ReturnValue, CallArg, CallRet, IntConstant>;

struct RetypdGenerator {
  Retypd &Ctx;
  std::map<ValMapKey, retypd::CGNode *> Val2Node;
  retypd::ConstraintGraph CG;
  retypd::StorageShapeGraph &SSG;

  void run(Function &F);
  RetypdGenerator(Retypd &Ctx) : Ctx(Ctx), SSG(CG.SSG) {}

protected:
  std::map<std::string, uint32_t> callInstanceId;

public:
  static const char *Stack;
  static const char *Memory;

  CGNode &setTypeVar(ValMapKey val, const TypeVariable &dtv) {
    auto ref = Val2Node.emplace(val, &CG.getOrInsertNode(dtv));
    assert(ref.second && "setTypeVar: Value already exists");
    return *ref.first->second;
  }
  void addSubtype(const TypeVariable &sub, const TypeVariable &sup) {
    if (sub.isPrimitive() && sup.isPrimitive()) {
      assert(sub.getBaseName() == sup.getBaseName() &&
             "addConstraint: different primitive types !?");
      return;
    }
    CG.addConstraint(sub, sup);
    CG.getOrInsertNode(sub).Link.unify(CG.getOrInsertNode(sup).Link);
  }
  const TypeVariable &getTypeVar(ValMapKey val);
  TypeVariable convertTypeVar(ValMapKey Val);
  TypeVariable convertTypeVarVal(Value *Val);

  void setOffset(TypeVariable &dtv, OffsetRange Offset);
  TypeVariable deref(Value *Val, long BitSize, bool isLoad);
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
