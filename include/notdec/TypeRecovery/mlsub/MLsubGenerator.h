#ifndef _BINARYSUB_MLSUBGENERATOR_H_
#define _BINARYSUB_MLSUBGENERATOR_H_

#include <cassert>
#include <clang/AST/Type.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/FormattedStream.h>

#include "TypeRecovery/DotSummaryParser.h"
#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "TypeRecovery/mlsub/PNDiff.h"
#include "TypeRecovery/retypd/Schema.h"
#include "Utils/DSUMap.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#endif

namespace notdec::mlsub {

struct ConstraintsGenerator;

struct ConstraintsGenerator {
  DSUMap<ExtValuePtr, CGNode *> V2N;
  DSUMap<ExtValuePtr, CGNode *> V2NContra;
  void removeNode(CGNode &N);

  ConstraintGraph CG;
  PNIGraph *PG;
  std::set<llvm::Function *> SCCs;
  // std::map<llvm::CallBase *, std::pair<CGNode *, CGNode *>> CallToInstance;
  // std::map<llvm::CallBase *, std::pair<CGNode *, CGNode *>> UnhandledCalls;

  DSUMap<std::pair<std::string, llvm::Type *>, CGNode *> PrimMap;

  void addMergeNode(CGNode &From, CGNode &To) { V2N.merge(&From, &To); }

  void instantiateSummary(llvm::CallBase *Inst, llvm::Function *Target,
                          const ConstraintsGenerator &Summary);

  // void dumpV2N();
  ConstraintsGenerator(std::string Name, unsigned int pointer_size,
                       const std::set<llvm::Function *> &SCCs = {})
      : CG(pointer_size, Name, false), PG(CG.PG.get()), SCCs(SCCs) {}

  void run() {
    for (llvm::Function *Func : SCCs) {
      // create function nodes
      auto [F, FC] = createNode(Func, nullptr, -1);
      for (int i = 0; i < Func->arg_size(); i++) {
        auto [Arg, ArgC] = createNode(Func->getArg(i), nullptr, i);
        // Contra-variant.
        addEdge(F, ArgC, {InLabel{.name = std::to_string(i)}});
        addEdge(FC, Arg, {InLabel{.name = std::to_string(i)}});
      }
      if (!Func->getReturnType()->isVoidTy()) {
        auto [Ret, RetC] = createNode(ReturnValue{.Func = Func}, nullptr, -1);
        addEdge(F, Ret, {OutLabel{}});
        addEdge(FC, RetC, {OutLabel{}});
      }
    }
    for (llvm::Function *Func : SCCs) {
      MLsubVisitor Visitor(*this);
      Visitor.visit(Func);
      Visitor.handlePHINodes();
    }
  }
  void cloneTo(ConstraintsGenerator &Target,
               std::map<const CGNode *, CGNode *> &Old2New);
  std::shared_ptr<ConstraintsGenerator>
  cloneShared(std::map<const CGNode *, CGNode *> &Old2New);

public:
  const CGEdge *addEdge(CGNode &From, CGNode &To, EdgeLabel Label) {
    return CG.addEdge(From, To, Label);
  }

  // Create Node of both variance
  std::pair<CGNode &, CGNode &> createNode(ExtValuePtr Val, llvm::User *User,
                                           long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);
    auto [N, NContra] = CG.createNodePair(getType(Val));
    auto It = V2N.insert(Val, &N);
    if (!It.second) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "setTypeVar: Value already mapped to "
                   << It.first->second->str() << ", but now set to "
                   << toString(Val) << "\n";
      std::abort();
    }
    auto It2 = V2NContra.insert(Val, &NContra);
    if (!It2.second) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "setTypeVar: Value already mapped to "
                   << It2.first->second->str() << ", but now set to "
                   << toString(Val) << "\n";
      std::abort();
    }
    // if the value is constant addr, we set ptr and link to memory
    if (auto CA = std::get_if<ConstantAddr>(&Val)) {
      assert(false && "TODO");
    }
    return {N, NContra};
  }

  CGNode *getNodeOrNull(ExtValuePtr Val, llvm::User *User, long OpInd,
                        Variance V) {
    llvmValue2ExtVal(Val, User, OpInd);

    if (V == Covariant) {
      if (V2N.count(Val)) {
        return V2N.at(Val);
      }
      return nullptr;
    } else {
      if (V2NContra.count(Val)) {
        return V2NContra.at(Val);
      }
      return nullptr;
    }
  }

  unsigned getPointerElemSize(llvm::Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<llvm::AddrSpaceCastInst, llvm::BitCastInst,
                     llvm::PtrToIntInst, llvm::IntToPtrInst>(Val);
  }

  void addAddConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addSubConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addCmpConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::ICmpInst *I);

  void setPointer(CGNode &Node) { CG.setPointer(Node); }

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
    bool addRetConstraint(llvm::Instruction *I, ConstraintsGenerator &cg) const;
    bool addOpConstraint(unsigned Index, llvm::Instruction *I,
                         ConstraintsGenerator &cg) const;
  };

  static const std::map<unsigned, PcodeOpType> opTypes;

protected:
  // visitor class
  // Visit each basic block in topo order. Then handle dataflow of Phi nodes.
  // After visiting each instruction, it must be assigned a type variable.
  // Often visitor will immediately add a subtype constraint. If the primitive
  // type is final, then it will directly map as the known type.
  class MLsubVisitor
      : public llvm::InstVisitor<MLsubVisitor> {
    ConstraintsGenerator &cg;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    MLsubVisitor(ConstraintsGenerator &cg) : cg(cg) {}

    // bool handleIntrinsicCall(llvm::CallBase &I);
    // overloaded visit functions
    void visitExtractValueInst(llvm::ExtractValueInst &I);
    void visitCastInst(llvm::CastInst &I);
    void visitCallBase(llvm::CallBase &I);
    void visitReturnInst(llvm::ReturnInst &I) {
      assert(false && "TODO");
    }
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

    void handlePHINodes() {
      assert(false && "TODO");
    }
    // use opTypes to handle other insts.
    void visitInstruction(llvm::Instruction &I);
  };
};

class MLsubRecovery {
  const llvm::Module &Mod;
  llvm::ModuleAnalysisManager &MAM;
  MLsubRecovery(llvm::Module &Mod, llvm::ModuleAnalysisManager &MAM)
      : Mod(Mod), MAM(MAM) {}

  unsigned int PointerSize = Mod.getDataLayout().getPointerSizeInBits();
  std::shared_ptr<ConstraintsGenerator> Main;

public:
  void run() {
    // TODO: prepareSCC, mark levels
    // 遍历调用图，然后根据用户提供的多台函数标记，将SCC标记为高一个level。
    // 处理SCC的call的时候，直接看目标函数的level是不是更高，是则多态实例化。
    // 调用图上可达的相同level的函数，可以放到同一个Generator？尤其是顶层的要尽量大，不然不好弄。

    // 当前简化情况：先假设所有函数都是同一个generator
    // 把所有函数加入SCC，然后构造Generator。
    std::set<llvm::Function *> SCCs(Mod.getFunctionList().begin(),
                                    Mod.getFunctionList().end());
    Main = std::make_shared<ConstraintsGenerator>("Main", PointerSize, SCCs);
    Main->run();
  }
};

struct MLsubRecoveryMain : llvm::PassInfoMixin<MLsubRecoveryMain> {

  MLsubRecovery &TR;
  MLsubRecoveryMain(MLsubRecovery &TR) : TR(TR) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    TR.run();
    return llvm::PreservedAnalyses::all();
  }
};

} // namespace notdec::mlsub

#endif
