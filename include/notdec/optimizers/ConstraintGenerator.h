#ifndef _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_
#define _NOTDEC_OPTIMIZERS_RETYPD_GENERATE_H_

#include <cassert>
#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <memory>
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
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
#include "TypeRecovery/TRContext.h"
#include "Utils/Range.h"
#include "Utils/ValueNamer.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#endif

namespace notdec {

using namespace llvm;
using retypd::DerivedTypeVariable;
using retypd::TRContext;
using retypd::TypeVariable;

bool hasUser(const Value *Val, const User *User);
bool isFinal(const std::string &Name);
bool mustBePrimitive(const llvm::Type *Ty);

struct ConstraintsGenerator;

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
  size_t InstanceId;
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
  size_t InstanceId;
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

// Extend Value* with some special values.
using ExtValuePtr =
    std::variant<llvm::Value *, ReturnValue, CallArg, CallRet, IntConstant>;

std::string getName(const ExtValuePtr &Val);

struct TypeRecovery : public AnalysisInfoMixin<TypeRecovery> {
  // Provide a unique key, i.e., memory address to be used by the LLVM's pass
  // infrastructure.
  static inline llvm::AnalysisKey Key; // NOLINT
  friend llvm::AnalysisInfoMixin<TypeRecovery>;

  // Specify the result type of this analysis pass.
  using Result = ::notdec::llvm2c::HighTypes;

  TypeRecovery(TRContext &TRCtx) : TRCtx(TRCtx) {}

  TRContext &TRCtx;
  llvm::Value *StackPointer;
  std::string data_layout;
  // Map from SCC to initial constraint graph.
  std::map<llvm::Function *, std::shared_ptr<ConstraintsGenerator>> FuncCtxs;
  std::map<llvm::Function *, std::vector<retypd::SubTypeConstraint>>
      FuncSummaries;
  std::map<llvm::Function *,
           std::pair<std::vector<std::shared_ptr<retypd::Sketch>>,
                     std::shared_ptr<retypd::Sketch>>>
      FuncSketches;
  unsigned pointer_size = 0;

  Result run(Module &M, ModuleAnalysisManager &);
  void gen_json(std::string OutputFilename);

public:
  void print(Module &M, std::string path);
};

/// The ConstraintsGenerator class is responsible for generating constraints.
/// The ConstraintGraph/StorageShapeGraph is expected to be able to print to a
/// readable format. (TODO)
struct ConstraintsGenerator {
  TypeRecovery &Ctx;
  std::map<ExtValuePtr, retypd::CGNode *> Val2Node;
  retypd::ConstraintGraph CG;
  retypd::PNIGraph &PG;
  std::set<llvm::Function *> SCCs;
  std::map<CallBase *, size_t> CallToID;

  std::vector<retypd::SubTypeConstraint> genSummary();
  std::function<const std::vector<retypd::SubTypeConstraint> *(
      llvm::Function *)>
      GetSummary;
  size_t instantiateSummary(llvm::Function *Target);
  std::shared_ptr<retypd::Sketch> solveType(const TypeVariable &Node);
  void instantiateSketchAsSub(ExtValuePtr Val,
                              std::shared_ptr<retypd::Sketch> Sk);
  void instantiateSketchAsSup(ExtValuePtr Val,
                              std::shared_ptr<retypd::Sketch> Sk);

  void run();
  ConstraintsGenerator(
      TypeRecovery &Ctx, std::string Name, std::set<llvm::Function *> SCCs,
      std::function<
          const std::vector<retypd::SubTypeConstraint> *(llvm::Function *)>
          GetSummary)
      : Ctx(Ctx), CG(Ctx.TRCtx, Name, false), PG(*CG.PG), SCCs(SCCs),
        GetSummary(GetSummary) {}

public:
  CGNode &setTypeVar(ExtValuePtr Val, const TypeVariable &dtv, User *User,
                     unsigned int Size) {
    // Differentiate int32/int64 by User.
    if (auto V = std::get_if<llvm::Value *>(&Val)) {
      if (auto CI = dyn_cast<ConstantInt>(*V)) {
        if (CI->getBitWidth() == 32 || CI->getBitWidth() == 64) {
          assert(false && "Should already be converted to IntConstant");
        }
      }
    }
    auto ref = Val2Node.emplace(Val, &CG.getOrInsertNode(dtv, Size));
    if (!ref.second) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "setTypeVar: Value already mapped to "
                   << toString(ref.first->second->key.Base)
                   << ", but now set to " << toString(dtv) << "\n";
      std::abort();
    }
    return *ref.first->second;
  }
  CGNode &addVarSubtype(llvm::Value *Val, const TypeVariable &dtv) {
    auto &Node = getNode(Val, nullptr);
    addSubtype(dtv, Node.key.Base);
    return Node;
  }
  void addSubtype(const TypeVariable &sub, const TypeVariable &sup) {
    if (sub.isPrimitive() && sup.isPrimitive()) {
      // TODO check if this is correct
      if (sub.getPrimitiveName() != sup.getPrimitiveName()) {
        std::cerr << "addSubtype: relation between primitive types: "
                  << toString(sub) << " <= " << toString(sup) << "\n";
        // assert(sub.getPrimitiveName() == sup.getPrimitiveName() &&
        //        "addConstraint: different primitive types !?");
      }
      return;
    }
    CG.addConstraint(sub, sup);
  }

  void setPointer(CGNode &Node) { CG.setPointer(Node); }

  retypd::CGNode &getNode(ExtValuePtr Val, User *User);

  const TypeVariable &getTypeVar(ExtValuePtr val, User *User);
  TypeVariable convertTypeVar(ExtValuePtr Val, User *User = nullptr);
  TypeVariable convertTypeVarVal(Value *Val, User *User = nullptr);
  void addAddConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        BinaryOperator *Result);
  void addSubConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        BinaryOperator *Result);
  void addCmpConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        ICmpInst *I);
  // void onEraseConstraint(const retypd::ConsNode *Cons);
  // void addSubTypeCons(retypd::SSGNode *LHS, retypd::SSGNode *RHS,
  //                     OffsetRange Offset);
  // void addSubTypeCons(llvm::Value *LHS, llvm::BinaryOperator *RHS,
  //                     OffsetRange Offset);

  TypeVariable addOffset(TypeVariable &dtv, OffsetRange Offset);
  TypeVariable deref(Value *Val, User *User, unsigned BitSize, bool isLoad);
  unsigned getPointerElemSize(Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<AddrSpaceCastInst, BitCastInst, PtrToIntInst,
                     IntToPtrInst>(Val);
  }
  static std::string offset(APInt Offset, int Count = 0);

public:
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

protected:
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

std::string inline getFuncTvName(llvm::Function *Func) {
  return ValueNamer::getName(*Func, ValueNamer::FuncPrefix);
}

inline TypeVariable getCallArgTV(retypd::TRContext &Ctx, llvm::Function *Target,
                                 size_t InstanceId, int32_t Index) {
  auto TargetName = getFuncTvName(Target);
  TypeVariable TV = TypeVariable::CreateDtv(Ctx, TargetName, InstanceId);
  return TV.pushLabel(retypd::InLabel{std::to_string(Index)});
}

inline TypeVariable getCallRetTV(retypd::TRContext &Ctx, llvm::Function *Target,
                                 size_t InstanceId) {
  auto TargetName = getFuncTvName(Target);
  TypeVariable TV = TypeVariable::CreateDtv(Ctx, TargetName, InstanceId);
  return TV.pushLabel(retypd::OutLabel{});
}

} // namespace notdec

#endif
