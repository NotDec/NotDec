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
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/TRContext.h"
#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "TypeRecovery/mlsub/PNDiff.h"
#include "Utils/DSUMap.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"

#ifdef NOTDEC_ENABLE_LLVM2C
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#endif

namespace notdec::mlsub {

using retypd::DerivedTypeVariable;
using retypd::DotGraph;
using retypd::FieldLabel;
using retypd::InLabel;
using retypd::One;
using retypd::OutLabel;
using retypd::TRContext;
using retypd::TypeVariable;

struct ConstraintsGenerator;

/// The ConstraintsGenerator class is responsible for generating constraints.
/// It represents a constraint graph and mappings from LLVM values to nodes in
/// the graph.
struct ConstraintsGenerator {
  // TypeRecovery &Ctx;

  // LLVM值到那边节点的映射关系由转TypeVar的函数决定。这里其实只需要保证不重复，其次是保留名字的意义？
  // 因为转换过程有临时变量参与，所以这里到Key的映射要缓存。这里使用的就是Val2Node做缓存
  // std::map<ExtValuePtr, NodeKey> Val2Node;
  // TODO
  // 使用getPreferredVariance合并两个map。key是ExtValuePtr带上variance的pair。
  DSUMap<ExtValuePtr, CGNode *> V2N;
  void removeNode(CGNode &N);

  ConstraintGraph CG;
  PNIGraph *PG;
  std::set<llvm::Function *> SCCs;
  std::map<llvm::CallBase *, std::pair<CGNode *, CGNode *>> CallToInstance;
  // unhandled due to not having function body.
  // TODO: If reachable from function node, then it makes summary incorrect.
  std::map<llvm::CallBase *, std::pair<CGNode *, CGNode *>> UnhandledCalls;

  DSUMap<std::pair<std::string, llvm::Type *>, CGNode *> PrimMap;

  CGNode &getOrCreatePrim(std::string Name, llvm::Type *LowType) {
    assert(false && "TODO");
  }

  bool isPrimitive(const CGNode &N) const { assert(false && "TODO"); }

  void addMergeNode(CGNode &From, CGNode &To) { V2N.merge(&From, &To); }

  void instantiateSummary(llvm::CallBase *Inst, llvm::Function *Target,
                          const ConstraintsGenerator &Summary);

  void dumpV2N();

  void run();
  void
  cloneTo(ConstraintsGenerator &Target,
          std::map<const CGNode *, CGNode *> &Old2New);
  std::shared_ptr<ConstraintsGenerator>
  cloneShared(std::map<const CGNode *, CGNode *> &Old2New);
  ConstraintsGenerator(std::string Name, unsigned int pointer_size,
                       std::set<llvm::Function *> SCCs = {})
      : CG(pointer_size, Name, false), PG(CG.PG.get()),
        SCCs(SCCs) {}

public:

  const CGEdge *addConstraint(CGNode &From, CGNode &To, FieldLabel Label) {
    return CG.addRecallEdge(From, To, Label);
  }

  CGNode &getNode(ExtValuePtr Val, llvm::User *User, long OpInd, Variance V);
  const CGNode &getNode(ExtValuePtr Val, llvm::User *User, long OpInd,
                        Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNode(Val, User, OpInd,
                                                             V);
  }

  CGNode *getNodeOrNull(ExtValuePtr Val, llvm::User *User, long OpInd,
                        Variance V);
  const CGNode *getNodeOrNull(ExtValuePtr Val, llvm::User *User, long OpInd,
                              Variance V) const {
    return const_cast<ConstraintsGenerator *>(this)->getNodeOrNull(Val, User,
                                                                   OpInd, V);
  }

  // Create Node of both variance
  std::pair<CGNode &, CGNode &> createNode(ExtValuePtr Val, llvm::User *User,
                                           long OpInd);
  CGNode &createNodeCovariant(ExtValuePtr Val, llvm::User *User, long OpInd) {
    auto [N, NC] = createNode(Val, User, OpInd);
    return N;
  }

  CGNode &getOrInsertNode(ExtValuePtr Val, llvm::User *User, long OpInd,
                          Variance V = Covariant);

  const TypeVariable &getTypeVar(ExtValuePtr val, llvm::User *User, long OpInd);
  // convert the value to a type variable.
  TypeVariable convertTypeVar(ExtValuePtr Val, llvm::User *User = nullptr,
                              long OpInd = -1);
  TypeVariable convertTypeVarVal(Value *Val, llvm::User *User = nullptr,
                                 long OpInd = -1);
  void addAddConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addSubConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::BinaryOperator *Result);
  void addCmpConstraint(const ExtValuePtr LHS, const ExtValuePtr RHS,
                        llvm::ICmpInst *I);

  TypeVariable addOffset(TypeVariable &dtv, OffsetRange Offset);
  unsigned getPointerElemSize(llvm::Type *ty);
  static inline bool is_cast(Value *Val) {
    return llvm::isa<llvm::AddrSpaceCastInst, llvm::BitCastInst,
                     llvm::PtrToIntInst, llvm::IntToPtrInst>(Val);
  }

  std::map<const CGEdge *, const CGEdge *> mergeNodeTo(CGNode &From, CGNode &To,
                                                       bool NoSelfLoop = false);

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
  class RetypdGeneratorVisitor
      : public llvm::InstVisitor<RetypdGeneratorVisitor> {
    ConstraintsGenerator &cg;
    // defer phi node constraints
    std::vector<llvm::PHINode *> phiNodes;

  public:
    RetypdGeneratorVisitor(ConstraintsGenerator &cg) : cg(cg) {}

    bool handleIntrinsicCall(llvm::CallBase &I);
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
};


} // namespace notdec::mlsub

#endif
