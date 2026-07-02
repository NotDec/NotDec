#ifndef _BINARYSUB_PNDIFF_H_
#define _BINARYSUB_PNDIFF_H_

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "Utils/DSUMap.h"
#include "binarysub/Range.h"
#include "notdec/TypeRecovery/LowTy.h"

namespace notdec::mlsub {

using retypd::NotPN;
using retypd::Null;
using retypd::Number;
using retypd::PNTy;
using retypd::Pointer;
using retypd::PtrOrNum;
using retypd::Unknown;

// PNDiff must not know the source IR object layout.  NotDec owns the real
// source objects and interns them into these opaque handles.
using PNIValue = void *;
using PNIInstruction = void *;

struct PNDiffPolicyConfig {
  std::int64_t nonPointerAbsLt = 900;
  bool excludeZero = true;
};

struct PNINode;
struct PNIGraph;
using PNChangedNodes = std::vector<PNINode *>;

// PNINode stores low level pointer/number state. If the source value is pointer
// or pointer-sized int, we use PtrOrNum to further distinguish it.
struct PNINode {
  PNIGraph &Parent;

  friend struct PNIGraph;
  PNINode(PNIGraph &SSG, PNTy LowTy);
  // clone constructor for PNIGraph::cloneFrom
  PNINode(PNIGraph &SSG, const PNINode &OtherGraphNode);
  PNINode(PNIGraph &SSG, std::string SerializedTy);

protected:
  unsigned long Id = 0;
  PNTy Ty;

public:
  using iteratorTy = std::list<PNINode>::iterator;
  iteratorTy getIterator() {
    size_t iterOffset = (size_t)&(*((iteratorTy)nullptr));
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

  bool isNumber() const { return getPtrOrNum() == Number; }
  bool isPointer() const { return getPtrOrNum() == Pointer; }
  bool isUnknown() const { return getPtrOrNum() == Unknown; }
  bool isNull() const { return getPtrOrNum() == Null; }
  bool isNotPN() const { return getPtrOrNum() == NotPN; }
  bool isPNRelated() const { return Ty.isPNRelated(); }
  char getPNChar() const { return Ty.getPNChar(); }
  /// merge two PNVar into one. Return the unified PNVar.
  PNINode *unify(PNINode &other);

  PNTy &getLatticeTy() { return Ty; }
  const PNTy &getLatticeTy() const { return Ty; }
  void merge(PNTy &Other) { Ty.merge(Other); }

  bool tyEqual(const PNINode &Other) const { return Ty == Other.Ty; }

  std::string str() const { return Ty.str(); }

  std::string serialize() const { return Ty.str() + " #" + std::to_string(Id); }
};

struct AddNodeCons {
  PNIValue LeftNode = nullptr;
  PNIValue RightNode = nullptr;
  PNIValue ResultNode = nullptr;
  PNIInstruction Inst = nullptr;

  static const char Rules[][3];
  // return a list of changed nodes and whether the constraint is fully solved.
  PNChangedNodes solve(PNIGraph &G);
  bool isFullySolved(PNIGraph &G);
};

struct SubNodeCons {
  PNIValue LeftNode = nullptr;
  PNIValue RightNode = nullptr;
  PNIValue ResultNode = nullptr;
  PNIInstruction Inst = nullptr;

  static const char Rules[][3];
  PNChangedNodes solve(PNIGraph &G);
  bool isFullySolved(PNIGraph &G);
};

using NodeCons = std::variant<AddNodeCons, SubNodeCons>;

struct ConsNode {
  PNIGraph &Parent;
  ConsNode(PNIGraph &SSG, NodeCons C) : Parent(SSG), C(C) {}
  NodeCons C;
  PNChangedNodes solve() {
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
  std::array<PNIValue, 3> getNodes() const {
    auto ret = const_cast<ConsNode *>(this)->getNodes();
    return {ret[0], ret[1], ret[2]};
  }
  std::array<PNIValue, 3> getNodes() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return {Add->LeftNode, Add->RightNode, Add->ResultNode};
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return {Sub->LeftNode, Sub->RightNode, Sub->ResultNode};
    }
    assert(false && "PNIConsNode::getNodes: unhandled variant");
  }
  bool isAdd() const { return std::holds_alternative<AddNodeCons>(C); }
  bool isSub() const { return std::holds_alternative<SubNodeCons>(C); }
  PNIInstruction getInst() const {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->Inst;
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->Inst;
    }
    assert(false && "PNIConsNode::getInst: unhandled variant");
  }

  using iteratorTy = std::list<ConsNode>::iterator;
  iteratorTy getIterator() {
    size_t iterOffset = (size_t)&(*((iteratorTy)nullptr));
    iteratorTy iter;
    *(intptr_t *)&iter = (intptr_t)this - iterOffset;
    return iter;
  }
  iteratorTy eraseFromParent();
};

struct PNIGraph {
  std::string Name;
  std::set<ConsNode *> Worklist;
  long PointerSize = 0;

  std::function<std::optional<int64_t>(PNIValue)> GetIntConstant;
  std::function<std::optional<OffsetRange>(PNIValue)> MatchOffsetRange;
  std::function<std::string(PNIValue)> FormatValue;
  std::function<std::string(PNIInstruction)> FormatInstruction;
  std::function<void(PNIValue)> OnUpdatePNType;
  std::function<void(PNIValue, PNIValue, OffsetRange)> OnPtrAdd;
  std::function<unsigned long()> AllocateNodeId;
  PNDiffPolicyConfig Policy;

  // list for ConstraintNode
  using ConstraintsType = std::list<ConsNode>;
  ConstraintsType Constraints;

  std::map<PNIValue, std::set<ConsNode *>> NodeToCons;

  // list for PNINode
  using PNINodesType = std::list<PNINode>;
  PNINodesType PNINodes;

  DSUMap<PNIValue, PNINode *> PNIMap;

  PNINode &createPNINode(PNIValue Val, const PNTy &Ty) {
    auto N = createPNINode(Ty);
    auto It = PNIMap.insert(Val, N);
    if (!It.second) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "createPNINode: Value already mapped to "
                << It.first->second->str() << ", but now set to "
                << formatValue(Val) << "\n";
      std::abort();
    }
    return *N;
  }

  PNINode *getPNIVarOrNull(PNIValue N) {
    auto It = PNIMap.find(N);
    if (It == PNIMap.end()) {
      return nullptr;
    }
    return It->second;
  }
  PNINode &getPNIVar(PNIValue N) {
    auto Ret = getPNIVarOrNull(N);
    assert(Ret != nullptr);
    return *Ret;
  }

  PNINode *createPNINode(std::string SerializedTy) {
    auto &It = PNINodes.emplace_back(*this, SerializedTy);
    return &It;
  }

  PNINode &getOrInsertPNINode(PNIValue Val, const PNTy &Ty) {
    auto N = getPNIVarOrNull(Val);
    if (N != nullptr) {
      return *N;
    }
    return createPNINode(Val, Ty);
  }
  PNINode &remapPNIVar(PNIValue Val, PNIValue Target) {
    auto *TargetNode = getPNIVarOrNull(Target);
    assert(TargetNode != nullptr);
    auto *ValNode = getPNIVarOrNull(Val);
    if (ValNode == TargetNode) {
      return *TargetNode;
    }
    if (ValNode != nullptr) {
      return *TargetNode->unify(*ValNode);
    }
    auto It = PNIMap.insert(Val, TargetNode);
    if (!It.second) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "remapPNIVar: Value already mapped to "
                << It.first->second->str() << ", but now set to "
                << formatValue(Val) << "\n";
      std::abort();
    }
    return *TargetNode;
  }
  void clearConstraints() {
    NodeToCons.clear();
    Constraints.clear();
    Worklist.clear();
  }

  void unifyVar(PNIValue V1, PNIValue V2) {
    getPNIVar(V1).unify(getPNIVar(V2));
  }

  PNIGraph(std::string Name, long PointerSize)
      : Name(std::move(Name)), PointerSize(PointerSize) {}

  bool applyPNIPolicy();
  void addAddCons(PNIValue Left, PNIValue Right, PNIValue Result,
                  PNIInstruction Inst);

  void addSubCons(PNIValue Left, PNIValue Right, PNIValue Result,
                  PNIInstruction Inst);

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
  bool traceEnabled() const { return TraceStream != nullptr; }
  void trace(const std::string &message);
  std::string formatValue(PNIValue Val) const;
  std::string formatInstruction(PNIInstruction Inst) const;
  unsigned long allocateNodeId() {
    if (AllocateNodeId) {
      return AllocateNodeId();
    }
    return NextNodeId++;
  }

protected:
  PNINode *createPNINode(const PNTy &Ty) {
    auto &It = PNINodes.emplace_back(*this, Ty);
    return &It;
  }
  void markChanged(PNINode *N, ConsNode *Except = nullptr);
  void mergePNVarTo(PNINode *Var, PNINode *Target);

public:
  std::ostream *TraceStream = nullptr;

private:
  unsigned long NextNodeId = 1;
};

} // namespace notdec::mlsub

#endif
