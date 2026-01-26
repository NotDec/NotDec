#ifndef _BINARYSUB_PNDIFF_H_
#define _BINARYSUB_PNDIFF_H_

#include <array>
#include <cassert>
#include <list>
#include <map>
#include <set>
#include <string>
#include <variant>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

#include "Passes/ConstraintGenerator.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec/TypeRecovery/LowTy.h"

namespace notdec::mlsub {

using retypd::NotPN;
using retypd::Null;
using retypd::Number;
using retypd::PNTy;
using retypd::Pointer;
using retypd::PtrOrNum;
using retypd::Unknown;

// Forward Declaration
struct PNIGraph;

OffsetRange matchOffsetRangeNoNegativeAccess(llvm::Value *I);
OffsetRange matchOffsetRange(llvm::Value *I);

// PNINode stores low level LLVM type. If the LowTy is pointer or
// pointer-sized int, we use PtrOrNum to further distinguish.
struct PNINode {
  PNIGraph &Parent;

  friend struct PNIGraph;
  PNINode(PNIGraph &SSG, llvm::Type *LowTy);
  // clone constructor for PNIGraph::cloneFrom
  PNINode(PNIGraph &SSG, const PNINode &OtherGraphNode);
  PNINode(PNIGraph &SSG, std::string SerializedTy);

protected:
  unsigned long Id = 0;
  PNTy Ty;

public:
  using iteratorTy = std::list<PNINode>::iterator;
  iteratorTy getIterator() {
    size_t iterOffset = (size_t)&(*((iteratorTy) nullptr));
    iteratorTy iter;
    *(intptr_t *)&iter = (intptr_t)this - iterOffset;
    return iter;
  }
  iteratorTy eraseFromParent();
  PNIGraph &getParent() { return Parent; }

  unsigned getSize() const { return Ty.getSize(); }
  /// Convenient method to set the type of the PNVar.
  bool setPtr() { return setPtrOrNum(Pointer); }
  bool setNonPtr() { return setPtrOrNum(Number); }
  bool setPtrOrNum(PtrOrNum NewTy);
  bool setNonPtrIfRelated() {
    if (isPNRelated()) {
      return setPtrOrNum(Number);
    }
    return false;
  }

  unsigned long getId() const { return Id; }
  PtrOrNum getPtrOrNum() const { return Ty.getPtrOrNum(); }
  bool isConflict() const { return Ty.isConflict(); }
  void setConflict() { Ty.setConflict(); }
  std::string getLowTy() const { return Ty.str(); }
  // bool updateLowTy(llvm::Type *T);

  bool isNumber() const { return getPtrOrNum() == Number; }
  bool isPointer() const { return getPtrOrNum() == Pointer; }
  bool isUnknown() const { return getPtrOrNum() == Unknown; }
  bool isNull() const { return getPtrOrNum() == Null; }
  bool isNotPN() const { return getPtrOrNum() == NotPN; }
  bool isPNRelated() const { return Ty.isPNRelated(); }
  char getPNChar() const { return Ty.getPNChar(); }
  /// merge two PNVar into one. Return the unified PNVar.
  PNINode *unify(PNINode &other);
  static llvm::Type *mergeLowTy(llvm::Type *T, llvm::Type *O);
  // void addUser(ExtValuePtr Node);

  PNTy &getLatticeTy() { return Ty; }
  const PNTy &getLatticeTy() const { return Ty; }
  void merge(PNTy &Other) { Ty.merge(Other); }

  bool tyEqual(const PNINode &Other) const { return Ty == Other.Ty; }

  std::string str() const { return Ty.str(); }

  std::string serialize() const { return Ty.str() + " #" + std::to_string(Id); }
};

struct AddNodeCons {
  ExtValuePtr LeftNode = nullptr;
  ExtValuePtr RightNode = nullptr;
  ExtValuePtr ResultNode = nullptr;
  llvm::BinaryOperator *Inst;

  static const char Rules[][3];
  // return a list of changed nodes and whether the constraint is fully
  // solved.
  llvm::SmallVector<PNINode *, 3> solve(PNIGraph &G);
  bool isFullySolved(PNIGraph &G);
};

struct SubNodeCons {
  ExtValuePtr LeftNode = nullptr;
  ExtValuePtr RightNode = nullptr;
  ExtValuePtr ResultNode = nullptr;
  llvm::BinaryOperator *Inst;

  static const char Rules[][3];
  llvm::SmallVector<PNINode *, 3> solve(PNIGraph &G);
  bool isFullySolved(PNIGraph &G);
};

using NodeCons = std::variant<AddNodeCons, SubNodeCons>;

struct ConsNode {
  PNIGraph &Parent;
  ConsNode(PNIGraph &SSG, NodeCons C) : Parent(SSG), C(C) {}
  NodeCons C;
  llvm::SmallVector<PNINode *, 3> solve() {
    // call solve according to the variant
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->solve(Parent);
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->solve(Parent);
    }
    assert(false && "PNIConsNode::solve: unhandled variant");
  }
  bool isFullySolved() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->isFullySolved(Parent);
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->isFullySolved(Parent);
    }
    assert(false && "PNIConsNode::isFullySolved: unhandled variant");
  }
  std::array<ExtValuePtr, 3> getNodes() const {
    auto ret = const_cast<ConsNode *>(this)->getNodes();
    return {ret[0], ret[1], ret[2]};
  }
  std::array<ExtValuePtr, 3> getNodes() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return {Add->LeftNode, Add->RightNode, Add->ResultNode};
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return {Sub->LeftNode, Sub->RightNode, Sub->ResultNode};
    }
    assert(false && "PNIConsNode::getNodes: unhandled variant");
  }
  bool isAdd() const { return std::holds_alternative<AddNodeCons>(C); }
  bool isSub() const { return std::holds_alternative<SubNodeCons>(C); }
  const llvm::BinaryOperator *getInst() const {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->Inst;
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->Inst;
    }
    assert(false && "PNIConsNode::getInst: unhandled variant");
  }

  using iteratorTy = std::list<ConsNode>::iterator;
  iteratorTy getIterator() {
    size_t iterOffset = (size_t)&(*((iteratorTy) nullptr));
    iteratorTy iter;
    *(intptr_t *)&iter = (intptr_t)this - iterOffset;
    return iter;
  }
  iteratorTy eraseFromParent();
};

struct PNIGraph {
  llvm::FunctionType *FuncTy = nullptr;
  std::string Name;
  std::set<ConsNode *> Worklist;
  long PointerSize = 0;

  // list for ConstraintNode
  using ConstraintsType = std::list<ConsNode>;
  ConstraintsType Constraints;

  std::map<ExtValuePtr, std::set<ConsNode *>> NodeToCons;

  // list for PNINode
  using PNINodesType = std::list<PNINode>;
  PNINodesType PNINodes;

  DSUMap<ExtValuePtr, PNINode *> PNIMap;
  // std::map<PNINode *, std::set<ExtValuePtr>> PNIToNode;

  PNINode &createPNINode(ExtValuePtr Val, llvm::User *User, long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);
    auto N = createPNINode(getType(Val));
    auto It = PNIMap.insert(Val, N);
    if (!It.second) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "createPNINode: Value already mapped to "
                   << It.first->second->str() << ", but now set to "
                   << toString(Val) << "\n";
      std::abort();
    }
    return *N;
  }


  PNINode *getPNIVarOrNull(ExtValuePtr N) {
    auto It = PNIMap.find(N);
    if (It == PNIMap.end()) {
      return nullptr;
    }
    return It->second;
  }
  PNINode &getPNIVar(ExtValuePtr N) {
    auto Ret = getPNIVarOrNull(N);
    assert(Ret != nullptr);
    return *Ret;
  }

  PNINode *createPNINode(std::string SerializedTy) {
    auto &It = PNINodes.emplace_back(*this, SerializedTy);
    return &It;
  }

  PNINode& getOrInsertPNINode(ExtValuePtr Val, llvm::User *User, long OpInd) {
    llvmValue2ExtVal(Val, User, OpInd);
    auto N = getPNIVarOrNull(Val);
    if (N != nullptr) {
      return *N;
    }
    return createPNINode(Val, User, OpInd);
  }
  void clearConstraints() {
    NodeToCons.clear();
    Constraints.clear();
    Worklist.clear();
  }

  void unifyVar(ExtValuePtr V1, ExtValuePtr V2) {
    getPNIVar(V1).unify(getPNIVar(V2));
  }

  PNIGraph(std::string Name, long PointerSize)
      : Name(Name), PointerSize(PointerSize) {}

  void addAddCons(ExtValuePtr Left, ExtValuePtr Right, ExtValuePtr Result,
                  llvm::BinaryOperator *Inst);

  void addSubCons(ExtValuePtr Left, ExtValuePtr Right, ExtValuePtr Result,
                  llvm::BinaryOperator *Inst);

  PNINode *mergePNINodes(PNINode *To, PNINode *From) {
    if (To == From) {
      return To;
    }
    To->Ty.merge(From->Ty);
    mergePNVarTo(From, To);
    return To;
  }
  void eraseConstraint(ConsNode *Cons);
  bool solve();
  void onUpdatePNType(PNINode *N);

protected:
  PNINode *createPNINode(llvm::Type *LowTy) {
    auto &It = PNINodes.emplace_back(*this, LowTy);
    return &It;
  }
  void markChanged(PNINode *N, ConsNode *Except = nullptr);
  void mergePNVarTo(PNINode *Var, PNINode *Target);
};

} // namespace notdec::mlsub

#endif
