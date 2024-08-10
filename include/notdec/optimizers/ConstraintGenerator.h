#ifndef _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_
#define _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_

#include <cassert>
#include <deque>
#include <iostream>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/FormattedStream.h>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/StorageShapeGraph.h"
#include "Utils/Range.h"

namespace notdec {

using namespace llvm;
using retypd::TypeVariable;

bool hasUser(const Value *Val, const User *User);
bool isFinal(const std::string &Name);

struct ConstraintsGenerator;
struct TypeRecovery : PassInfoMixin<TypeRecovery> {
  std::map<Function *, ConstraintsGenerator> func_ctxs;
  llvm::Value *StackPointer;
  std::string data_layout;
  unsigned pointer_size = 0;

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
  void gen_json(std::string OutputFilename);

  static std::string sanitize_name(std::string s);

protected:
  size_t typeValId = 0;

public:
  void print(Module &M, std::string path);
  // get a new type_var name from type_val_id
  std::string getName(Value &Val,
                      const char *prefix = TypeRecovery::DefaultPrefix);
  static const char *DefaultPrefix;
  static const char *FuncPrefix;
  static const char *PhiPrefix;
  static const char *SelectPrefix;
  static const char *NewPrefix;
  static const char *AddPrefix;
  static const char *SubPrefix;
};

inline retypd::SubTypeConstraint makeCons(const TypeVariable &sub,
                                          const TypeVariable &sup) {
  return retypd::SubTypeConstraint{sub, sup};
}

// When solving inter-procedurally, link CallArg (caller) with Argument*
// (callee) link CallRet (caller) with ReturnValue (callee) Separate
// representation because you need to handle instance id anyway.

struct ReturnValue {
  llvm::Function *Func;
  int32_t Index = 0;
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
  llvm::ConstantInt *Val;
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

/// The ConstraintsGenerator class is responsible for generating constraints.
/// The ConstraintGraph/StorageShapeGraph is expected to be able to print to a
/// readable format. (TODO)
struct ConstraintsGenerator {
  TypeRecovery &Ctx;
  Function &Func;
  std::map<ValMapKey, retypd::CGNode *> Val2Node;
  retypd::ConstraintGraph CG;
  retypd::StorageShapeGraph &SSG;

  void solve();
  void generate();
  ConstraintsGenerator(TypeRecovery &Ctx, Function &F)
      : Ctx(Ctx), Func(F), CG(Ctx.getName(F, TypeRecovery::FuncPrefix)),
        SSG(CG.SSG) {}

protected:
  std::map<std::string, uint32_t> callInstanceId;
  std::vector<IntConstant> intConstantIds;

public:
  CGNode &setTypeVar(ValMapKey Val, const TypeVariable &dtv, User *User,
                     unsigned int Size) {
    // Differentiate int32/int64 by User.
    if (auto V = std::get_if<llvm::Value *>(&Val)) {
      if (auto CI = dyn_cast<ConstantInt>(*V)) {
        if (CI->getBitWidth() == 32 || CI->getBitWidth() == 64) {
          assert(User != nullptr &&
                 "RetypdGenerator::getTypeVar: User is Null!");
          assert(hasUser(*V, User) &&
                 "convertTypeVarVal: constant not used by user");
          Val = IntConstant{.Val = cast<ConstantInt>(*V), .User = User};
        }
      }
    }
    auto ref = Val2Node.emplace(Val, &CG.getOrInsertNode(dtv, Size));
    if (!ref.second) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "setTypeVar: Value already mapped to "
                   << toString(ref.first->second->key.Base)
                   << ", but now set to" << toString(dtv) << "\n";
      std::abort();
    }
    return *ref.first->second;
  }
  CGNode &newVarSubtype(llvm::Value *Val, const TypeVariable &dtv,
                        const char *prefix = TypeRecovery::DefaultPrefix) {
    assert(Val2Node.find(Val) == Val2Node.end() &&
           "newVarSubtype: Value already mapped!");
    auto Size = Val->getType()->getScalarSizeInBits();
    if (dtv.isPrimitive() && isFinal(dtv.getBaseName())) {
      return setTypeVar(Val, dtv, nullptr, Size);
    }
    auto &Node = setTypeVar(
        Val, TypeVariable::CreateDtv(Ctx.getName(*Val, prefix)), nullptr, Size);
    addSubtype(Node.key.Base, dtv);
    return Node;
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

  void setPointer(CGNode &Node) {
    Node.Link.lookupNode()->getPNVar()->setPtrOrNum(retypd::Pointer);
  }

  retypd::CGNode &getNode(ValMapKey Val, User *User);
  retypd::SSGNode *getSSGNode(ValMapKey Val, User *User);

  const TypeVariable &getTypeVar(ValMapKey val, User *User);
  TypeVariable convertTypeVar(ValMapKey Val, User *User = nullptr);
  TypeVariable convertTypeVarVal(Value *Val, User *User = nullptr);
  void addAddConstraint(const ValMapKey LHS, const ValMapKey RHS,
                        BinaryOperator *Result);
  void addSubConstraint(const ValMapKey LHS, const ValMapKey RHS,
                        BinaryOperator *Result);
  void addCmpConstraint(const ValMapKey LHS, const ValMapKey RHS, ICmpInst *I);

  void setOffset(TypeVariable &dtv, OffsetRange Offset);
  TypeVariable deref(Value *Val, User *User, long BitSize, bool isLoad);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<AddrSpaceCastInst, BitCastInst, PtrToIntInst,
                     IntToPtrInst>(Val);
  }
  static std::string offset(APInt Offset, int Count = 0);

protected:
  struct PcodeOpType {
    // We only care about number or non-number, and signedness.
    // Size is not included.
    // so nullptr / int / sint / uint
    const char *output;
    // allow for uniform initializations
    int size;
    const char **inputs;

  public:
    PcodeOpType(const char *output, int size, const char *inputs[])
        : output(output), size(size), inputs(inputs) {}
    bool addRetConstraint(Instruction *I, ConstraintsGenerator &cg) const;
    bool addOpConstraint(unsigned Index, Instruction *I,
                         ConstraintsGenerator &cg) const;
  };

  static const std::map<unsigned, PcodeOpType> opTypes;

  // visitor class
  // Visit each basic block in topo order. Then handle dataflow of Phi nodes.
  // After visiting each instruction, it must be assigned a type variable.
  // Often visitor will immediately add a subtype constraint. If the primitive
  // type is final, then it will directly map as the known type.
  class RetypdGeneratorVisitor
      : public llvm::InstVisitor<RetypdGeneratorVisitor> {
    ConstraintsGenerator &cg;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    RetypdGeneratorVisitor(ConstraintsGenerator &cg) : cg(cg) {}
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
    void visitSelectInst(SelectInst &I);

    void visitAdd(BinaryOperator &I);
    void visitSub(BinaryOperator &I);

    // handle sth like
    // 1. Alignment/ Use lowest bits in pointer: And %x, 0xfffffff0.
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

void inline ensureSequence(Value *&Src1, Value *&Src2) {
  if (isa<ConstantInt>(Src1) && isa<ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (isa<ConstantInt>(Src1) && !isa<ConstantInt>(Src2)) {
    // because of InstCombine canonical form, this should not happen?
    assert(false &&
           "Constant cannot be at the left side. Run InstCombine first.");
    std::swap(Src1, Src2);
  }
}

} // namespace notdec

#endif
