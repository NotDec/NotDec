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
#include <llvm/IR/Constant.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
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
#include "Utils/DSUMap.h"
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
  int32_t Index;
  bool operator<(const CallArg &rhs) const {
    return std::tie(Call, Index) < std::tie(rhs.Call, rhs.Index);
  }
  bool operator==(const CallArg &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
struct CallRet {
  llvm::CallBase *Call;
  bool operator<(const CallRet &rhs) const {
    return std::tie(Call) < std::tie(rhs.Call);
  }
  bool operator==(const CallRet &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};
/// Differentiate pointer-sized int constant. It can be pointer or int under
/// different context.
struct UConstant {
  llvm::Constant *Val;
  llvm::User *User;
  long OpInd = -1;
  bool operator<(const UConstant &rhs) const {
    return std::tie(Val, User, OpInd) < std::tie(rhs.Val, rhs.User, rhs.OpInd);
  }
  bool operator==(const UConstant &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

// Global variable address / constant address pointer
// To merge different constant expr like (inttoptr i32 1064)
struct ConstantAddr {
  llvm::ConstantInt *Val;
  bool operator<(const ConstantAddr &rhs) const {
    return std::tie(Val) < std::tie(rhs.Val);
  }
  bool operator==(const ConstantAddr &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

// We cannot directly map llvm::Value* to Node, because we need to
// differentiate/merge different constants. Extend Value* with some special
// values.
// 1. Differentiate Constants by Users.
// 2. For some constant expr like (inttoptr i32 1064), the address of the expr
// is different, but we need to merge them.
using ExtValuePtr = std::variant<llvm::Value *, ReturnValue, CallArg, CallRet,
                                 UConstant, ConstantAddr>;

ExtValuePtr getExtValuePtr(llvm::Value *Val, User *User, long OpInd = -1);
std::string getName(const ExtValuePtr &Val);
std::string toString(const ExtValuePtr &Val);
void dump(const ExtValuePtr &Val);
llvm::Type *getType(const ExtValuePtr &Val);
unsigned int getSize(llvm::Type *Ty, unsigned int pointer_size);
unsigned int getSize(const ExtValuePtr &Val, unsigned int pointer_size);
inline void llvmValue2ExtVal(ExtValuePtr &Val, User *User, long OpInd) {
  // Differentiate int32/int64 by User.
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    if (isa<GlobalValue>(*V)) {
      return;
    }
    if (auto CI = dyn_cast<Constant>(*V)) {
      // Convert inttoptr constant int to ConstantAddr
      if (auto CExpr = dyn_cast<ConstantExpr>(CI)) {
        if (CExpr->isCast() && CExpr->getOpcode() == Instruction::IntToPtr) {
          if (auto CI1 = dyn_cast<ConstantInt>(CExpr->getOperand(0))) {
            V = nullptr;
            Val = ConstantAddr{.Val = CI1};
            return;
          }
        }
      }
      assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
      assert(hasUser(*V, User) &&
             "convertTypeVarVal: constant not used by user");
      Val = UConstant{.Val = cast<Constant>(*V), .User = User, .OpInd = OpInd};
    }
  }
}

// Check if differentiated constants (especially int32/int64 constants) by User.
inline bool checkWrapped(ExtValuePtr &Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    if (!isa<GlobalValue>(*V)) {
      if (auto CI = dyn_cast<Constant>(*V)) {
        return false;
        // assert(false && "Should already be converted to UConstant");
      }
    }
  }
  return true;
}

struct TypeRecovery : public AnalysisInfoMixin<TypeRecovery> {
  // Provide a unique key, i.e., memory address to be used by the LLVM's pass
  // infrastructure.
  static inline llvm::AnalysisKey Key; // NOLINT
  friend llvm::AnalysisInfoMixin<TypeRecovery>;

  // Specify the result type of this analysis pass.
  using Result = ::notdec::llvm2c::HighTypes;

  TypeRecovery(std::shared_ptr<retypd::TRContext> TRCtx) : TRCtx(TRCtx) {}

  std::shared_ptr<retypd::TRContext> TRCtx;
  llvm::Value *StackPointer;
  std::string data_layout;
  std::shared_ptr<ConstraintsGenerator> Global;
  // Map from SCC to initial constraint graph.
  std::map<llvm::Function *, std::shared_ptr<ConstraintsGenerator>> FuncCtxs;
  std::map<llvm::Function *, std::shared_ptr<ConstraintsGenerator>>
      FuncSummaries;
  unsigned pointer_size = 0;

  Result run(Module &M, ModuleAnalysisManager &);
  void gen_json(std::string OutputFilename);

public:
  void print(Module &M, std::string path);
  void printAnnotatedModule(Module &M, std::string path);
};

/// The ConstraintsGenerator class is responsible for generating constraints.
struct ConstraintsGenerator {
  TypeRecovery &Ctx;
  // todo refactor to save the target module.
  LLVMContext &LLCtx;

  // LLVM值到那边节点的映射关系由转TypeVar的函数决定。这里其实只需要保证不重复，其次是保留名字的意义？
  // 因为转换过程有临时变量参与，所以这里到Key的映射要缓存。这里使用的就是Val2Node做缓存
  // std::map<ExtValuePtr, retypd::NodeKey> Val2Node;
  DSUMap<ExtValuePtr, retypd::NodeKey> V2N;
  DSUMap<ExtValuePtr, retypd::NodeKey> V2NContra;

  retypd::ConstraintGraph CG;
  retypd::PNIGraph *PG;
  std::set<llvm::Function *> SCCs;
  std::map<CallBase *, std::pair<CGNode *, CGNode *>> CallToInstance;
  // unhandled due to not having function body.
  // TODO: If reachable from function node, then it makes summary incorrect.
  std::map<CallBase *, std::pair<CGNode *, CGNode *>> UnhandledCalls;
  std::string DebugDir;

  retypd::CGNode &getNode(const retypd::NodeKey &Key) {
    auto *N = getNodeOrNull(Key);
    assert(N != nullptr && "getNode: Node not found");
    return *N;
  }

  retypd::CGNode *getNodeOrNull(const retypd::NodeKey &Key) {
    auto It2 = CG.Nodes.find(Key);
    if (It2 != CG.Nodes.end()) {
      return &It2->second;
    }
    return nullptr;
  }
  void addMergeNode(const retypd::NodeKey &From, const retypd::NodeKey &To) {
    V2N.merge(From, To);
    V2NContra.merge(From, To);
  }

  std::shared_ptr<ConstraintsGenerator> genSummary();
  void instantiateSummary(llvm::CallBase *Inst, llvm::Function *Target,
                          const ConstraintsGenerator &Summary);
  // std::shared_ptr<retypd::Sketch> solveType(const TypeVariable &Node);
  // void instantiateSketchAsSub(ExtValuePtr Val,
  //                             std::shared_ptr<retypd::Sketch> Sk);
  // void instantiateSketchAsSup(ExtValuePtr Val,
  //                             std::shared_ptr<retypd::Sketch> Sk);

  // for determinization: extended powerset construction
  std::map<std::set<CGNode *>, CGNode *> DTrans;
  // void determinizeStructEqual();
  void eliminateCycle();
  void mergeOnlySubtype();
  void mergeAfterDeterminize();
  void determinize();
  // remove nodes that is unreachable from nodes in Val2Node map.
  void removeUnreachable();
  void linkContraToCovariant();

  bool checkSymmetry();
  void makeSymmetry();

  using IndexTy = OffsetRange;
  struct FieldEntry {
    IndexTy Start;
    long Size;
    CGEdge *OutEdge;
  };
  struct FieldInfo {
    std::vector<FieldEntry> Fields;
    // long TotalSize;
    long getMaxOffset() {
      long Max = 0;
      for (auto &Ent : Fields) {
        Max = std::max(Max, Ent.Start.offset + Ent.Size);
      }
      return Max;
    }
  };
  // Map from (Node, Range) to Edge, FieldDecl*
  std::map<CGNode *, FieldInfo> FieldInfoCache;
  // struct field range analysis.
  void analyzeFieldRange();
  static FieldInfo getFieldInfo(const CGNode &Node);

  void run();
  // clone CG and maintain value map.
  ConstraintsGenerator clone(std::map<const CGNode *, CGNode *> &Old2New);
  void cloneTo(ConstraintsGenerator &Target,
               std::map<const CGNode *, CGNode *> &Old2New);
  ConstraintsGenerator(TypeRecovery &Ctx, LLVMContext &LLCtx, std::string Name,
                       std::set<llvm::Function *> SCCs = {})
      : Ctx(Ctx), LLCtx(LLCtx),
        CG(Ctx.TRCtx, &LLCtx, Ctx.pointer_size, Name, false), PG(CG.PG.get()),
        SCCs(SCCs) {}

public:
  CGNode &addVarSubtype(llvm::Value *Val, CGNode &dtv) {
    auto &Node = getOrInsertNode(Val, nullptr, -1);
    addSubtype(dtv, Node);
    return Node;
  }
  void addSubtype(CGNode &SubNode, CGNode &SupNode) {
    auto &Sub = SubNode.key.Base;
    auto &Sup = SupNode.key.Base;
    if (Sub.isPrimitive() && Sup.isPrimitive()) {
      // TODO check if this is correct
      if (Sub.getPrimitiveName() != Sup.getPrimitiveName()) {
        std::cerr << "addSubtype: relation between primitive types: "
                  << toString(Sub) << " <= " << toString(Sup) << "\n";
        // assert(sub.getPrimitiveName() == sup.getPrimitiveName() &&
        //        "addConstraint: different primitive types !?");
      }
      return;
    }
    CG.addConstraint(SubNode, SupNode);
  }
  void mergeNodeTo(CGNode &From, CGNode &To, bool NoSelfLoop = false);

  void setPointer(CGNode &Node) { CG.setPointer(Node); }

  retypd::CGNode &getNode(ExtValuePtr Val, User *User, long OpInd,
                          retypd::Variance V);
  const retypd::CGNode &getNode(ExtValuePtr Val, User *User, long OpInd,
                                retypd::Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNode(Val, User, OpInd,
                                                             V);
  }
  retypd::CGNode *getNodeOrNull(ExtValuePtr Val, User *User, long OpInd,
                                retypd::Variance V);
  const retypd::CGNode *getNodeOrNull(ExtValuePtr Val, User *User, long OpInd,
                                      retypd::Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNodeOrNull(Val, User,
                                                                   OpInd, V);
  }
  // Create Node of both variance
  std::pair<retypd::CGNode &, retypd::CGNode &>
  createNode(ExtValuePtr Val, User *User, long OpInd);
  retypd::CGNode &createNodeCovariant(ExtValuePtr Val, User *User, long OpInd) {
    auto [N, NC] = createNode(Val, User, OpInd);
    return N;
  }
  retypd::CGNode &getOrInsertNode(ExtValuePtr Val, User *User, long OpInd,
                                  retypd::Variance V = retypd::Covariant);

  const TypeVariable &getTypeVar(ExtValuePtr val, User *User, long OpInd);
  // convert the value to a type variable.
  TypeVariable convertTypeVar(ExtValuePtr Val, User *User = nullptr,
                              long OpInd = -1);
  TypeVariable convertTypeVarVal(Value *Val, User *User = nullptr,
                                 long OpInd = -1);
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
  TypeVariable deref(Value *Val, User *User, long OpInd, unsigned BitSize,
                     bool isLoad);
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
    void visitSwitchInst(SwitchInst &I) {}

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

inline TypeVariable getCallArgTV(CGNode &Target, int32_t Index) {
  TypeVariable TV = Target.key.Base;
  return TV.pushLabel(retypd::InLabel{std::to_string(Index)});
}

inline TypeVariable getCallRetTV(CGNode &Target) {
  TypeVariable TV = Target.key.Base;
  return TV.pushLabel(retypd::OutLabel{});
}

} // namespace notdec

#endif
