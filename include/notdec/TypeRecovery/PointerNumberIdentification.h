#ifndef _NOTDEC_RETYPD_PNI_H_
#define _NOTDEC_RETYPD_PNI_H_

#include <array>
#include <cassert>
#include <cstdint>
#include <list>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/IR/InstrTypes.h>

#include "LowTy.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "Utils/Utils.h"

namespace notdec::retypd {

struct ConstraintGraph;

// Forward Declaration
struct PNIGraph;
struct CGNode;

OffsetRange matchOffsetRange(llvm::Value *I);
bool isUnknown(const CGNode *N);

// PNINode stores low level LLVM type. If the LowTy is pointer or
// pointer-sized int, we use PtrOrNum to further distinguish.
struct PNINode : public node_with_erase<PNINode, PNIGraph> {
protected:
  friend struct PNIGraph;
  PNINode(PNIGraph &SSG, llvm::Type *LowTy);
  // clone constructor for PNIGraph::cloneFrom
  PNINode(PNIGraph &SSG, const PNINode &OtherGraphNode);
  PNINode(PNIGraph &SSG, std::string SerializedTy);
  unsigned long Id = 0;
  LowTy Ty;

public:
  llvm::iplist<PNINode>::iterator eraseFromParent();

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
  void addUser(CGNode *Node);

  LowTy &getLatticeTy() { return Ty; }
  const LowTy &getLatticeTy() const { return Ty; }
  void merge(LowTy &Other) { Ty.merge(Other); }

  bool tyEqual(const PNINode &Other) const { return Ty == Other.Ty; }

  std::string str() const { return Ty.str(); }

  std::string serialize() const { return Ty.str() + " #" + std::to_string(Id); }
};

struct AddNodeCons {
  CGNode *LeftNode = nullptr;
  CGNode *RightNode = nullptr;
  CGNode *ResultNode = nullptr;
  llvm::BinaryOperator *Inst;

  static const char Rules[][3];
  // return a list of changed nodes and whether the constraint is fully
  // solved.
  llvm::SmallVector<PNINode *, 3> solve();
  bool isFullySolved();
};

struct SubNodeCons {
  CGNode *LeftNode = nullptr;
  CGNode *RightNode = nullptr;
  CGNode *ResultNode = nullptr;
  llvm::BinaryOperator *Inst;

  static const char Rules[][3];
  llvm::SmallVector<PNINode *, 3> solve();
  bool isFullySolved();
};

using NodeCons = std::variant<AddNodeCons, SubNodeCons>;

struct ConsNode : node_with_erase<ConsNode, PNIGraph> {
  ConsNode(PNIGraph &SSG, NodeCons C) : node_with_erase(SSG), C(C) {}
  NodeCons C;
  llvm::SmallVector<PNINode *, 3> solve() {
    // call solve according to the variant
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->solve();
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->solve();
    }
    assert(false && "PNIConsNode::solve: unhandled variant");
  }
  bool isFullySolved() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->isFullySolved();
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->isFullySolved();
    }
    assert(false && "PNIConsNode::isFullySolved: unhandled variant");
  }
  std::array<const CGNode *, 3> getNodes() const {
    auto ret = const_cast<ConsNode *>(this)->getNodes();
    return {ret[0], ret[1], ret[2]};
  }
  std::array<CGNode *, 3> getNodes() {
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
  void cloneFrom(const ConsNode &N, std::map<const CGNode *, CGNode *> Old2New) {
    if (auto *Add = std::get_if<AddNodeCons>(&N.C)) {
      auto *NewLeft = Old2New[Add->LeftNode];
      auto *NewRight = Old2New[Add->RightNode];
      auto *NewResult = Old2New[Add->ResultNode];
      auto *NewInst = Add->Inst;
      C = AddNodeCons{NewLeft, NewRight, NewResult, NewInst};
      return;
    } else if (auto *Sub = std::get_if<SubNodeCons>(&N.C)) {
      auto *NewLeft = Old2New[Sub->LeftNode];
      auto *NewRight = Old2New[Sub->RightNode];
      auto *NewResult = Old2New[Sub->ResultNode];
      auto *NewInst = Sub->Inst;
      C = SubNodeCons{NewLeft, NewRight, NewResult, NewInst};
      return;
    }
    assert(false && "PNIConsNode::cloneFrom: unhandled variant");
  }
};

struct PNIGraph {
  ConstraintGraph &CG;
  llvm::FunctionType *FuncTy = nullptr;
  std::string Name;
  std::set<ConsNode *> Worklist;
  long PointerSize = 0;

  // list for ConstraintNode
  using ConstraintsType = llvm::ilist<ConsNode>;
  ConstraintsType Constraints;
  static ConstraintsType PNIGraph::*getSublistAccess(ConsNode *) {
    return &PNIGraph::Constraints;
  }
  std::map<CGNode *, std::set<ConsNode *>> NodeToCons;

  // list for PNINode
  using PNINodesType = llvm::ilist<PNINode>;
  PNINodesType PNINodes;
  static PNINodesType PNIGraph::*getSublistAccess(PNINode *) {
    return &PNIGraph::PNINodes;
  }
  std::map<PNINode *, std::set<CGNode *>> PNIToNode;
  const std::set<CGNode *> &getNodeSet(PNINode *Cons) {
    return PNIToNode[Cons];
  }
  PNINode *createPNINode(std::string SerializedTy) {
    auto *N = new PNINode(*this, SerializedTy);
    PNINodes.push_back(N);
    return N;
  }
  PNINode *createPNINode(llvm::Type *LowTy) {
    auto *N = new PNINode(*this, LowTy);
    PNINodes.push_back(N);
    return N;
  }
  PNINode *clonePNINode(const PNINode &OGN) {
    auto *N = new PNINode(*this, OGN);
    PNINodes.push_back(N);
    return N;
  }
  static void addPNINodeTarget(CGNode &To, PNINode &N);
  void markRemoved(CGNode &N);

  PNIGraph(ConstraintGraph &CG, std::string Name, long PointerSize)
      : CG(CG), Name(Name), PointerSize(PointerSize) {}
  void cloneFrom(const PNIGraph &G, std::map<const CGNode *, CGNode *> Old2New);

  void addAddCons(CGNode *Left, CGNode *Right, CGNode *Result,
                  llvm::BinaryOperator *Inst);

  void addSubCons(CGNode *Left, CGNode *Right, CGNode *Result,
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
  void markChanged(PNINode *N, ConsNode *Except = nullptr);
  void mergePNVarTo(PNINode *Var, PNINode *Target);
};

} // namespace notdec::retypd

#endif
