#ifndef _NOTDEC_RETYPD_UNIFY_H_
#define _NOTDEC_RETYPD_UNIFY_H_

#include "TypeRecovery/Schema.h"
#include "utils.h"
#include <cassert>
#include <cstdint>
#include <list>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <variant>

#include <llvm/IR/Value.h>
#include <vector>

namespace notdec::retypd {

struct StorageShapeGraph;
struct SSGNode;

enum PtrOrNum {
  Unknown = 0,
  NonPtr = 1,
  Pointer = 2,
};

using PNVarTy = unsigned long;
const int PNVarStart = 3;
inline bool isUnknown(PNVarTy Ty) { return Ty >= PNVarStart; }
bool isUnknown(SSGNode *N);

// mergeMap[left index][right index] = isPreserveLeft
// column -> right index, row -> left index
const unsigned char UniTyMergeMap[][3] = {
    {2, 0, 0},
    {1, 2, 0},
    {1, 1, 2},
};

PtrOrNum unify(const PtrOrNum &Left, const PtrOrNum &Right);

PtrOrNum fromIPChar(char C);

struct SSGNode
    : public llvm::ilist_node_with_parent<SSGNode, StorageShapeGraph> {
  StorageShapeGraph *Parent = nullptr;
  inline StorageShapeGraph *getParent() { return Parent; }
  llvm::iplist<SSGNode>::iterator eraseFromParent();

  SSGNode(StorageShapeGraph &SSG, unsigned long PNVar);

  // #region Ptr/Num handling
  /// Represent the type of the node 0-2, Or an unknown variable 3+.
  /// Maintain the reverse map Parent->PNVarToNode
  PNVarTy PNVar;
  PNVarTy getPNVar() { return PNVar; }
  static PtrOrNum toPNTy(PNVarTy PNVar) {
    PNVarTy E = PNVar > 2 ? 0 : PNVar;
    return static_cast<PtrOrNum>(E);
  }
  PtrOrNum getPNTy() { return toPNTy(PNVar); }

protected:
  bool setPtrOrNum(PtrOrNum Ty);

public:
  /// Unify the PNVarTy of the node to other type or unknown var.
  bool setPNVar(PNVarTy Ty);

  // void setPtr();
  // void setNonPtr();

  bool isNonPtr() { return getPNTy() == NonPtr; }
  bool isPointer() { return getPNTy() == Pointer; }
  char getPNChar() {
    if (isUnknown(this)) {
      return 'u';
    } else if (isNonPtr()) {
      return 'i';
    } else if (isPointer()) {
      return 'p';
    }
    assert(false && "SSGNode::getPNChar: unhandled type");
  }

  /// merge two PNVar into one. Return the unified PNVar.
  PNVarTy unifyPN(SSGNode &other);
  friend struct AddNodeCons;
  // #endregion Ptr/Num handling

  /// merge two SSGNode into one. Return true if the left one is preserved.
  /// the other one is expected to be removed.
  bool unify(SSGNode &other) {
    unifyPN(other);
    return true;
  }
};

struct SSGLink {
  StorageShapeGraph *Parent = nullptr;
  void setParent(StorageShapeGraph *P) { Parent = P; }
  inline StorageShapeGraph *getParent() { return Parent; }

  SSGLink(StorageShapeGraph &SSG) : Parent(&SSG) {}

  using LinkTy = std::variant<SSGLink *, SSGNode *>;
  LinkTy Link;
  static SSGLink *lookup(SSGLink *Current) {
    assert(Current != nullptr && "SSGLink::lookup: Current is null");
    std::vector<SSGLink *> path;

    while (auto *N = std::get_if<SSGLink *>(&Current->Link)) {
      assert(N != nullptr && "SSGLink::lookup: N is null");
      path.push_back(Current);
      Current = *N;
      assert(Current != nullptr && "SSGLink::lookup: Current is null");
    }
    // SSGNode *Result = std::get<SSGNode *>(Current->Link);
    for (auto N : path) {
      assert(Current != nullptr && "SSGLink::lookup: Current is null");
      N->Link = Current;
    }
    return Current;
  }
  void setForward(SSGLink *N) { Link = N; }
  void setNode(SSGNode *N) { Link = N; }
  SSGNode *lookupNode() { return lookup(this)->getNode(); }
  SSGNode *getNode() {
    if (auto *N = std::get_if<SSGNode *>(&Link)) {
      return *N;
    }
    return nullptr;
  }

  void unify(SSGLink &Other) {
    assert(getParent() == Other.getParent() &&
           "SSGLink::unifyNode: Parents are different");
    SSGLink *LThis = lookup(this);
    SSGLink *LOther = lookup(&Other);
    assert(LThis != nullptr && LOther != nullptr);
    SSGNode *SNThis = LThis->getNode();
    SSGNode *SNOther = LOther->getNode();
    assert(SNThis != nullptr && SNOther != nullptr);
    bool PreserveLeft = SNThis->unify(*SNOther);
    if (PreserveLeft) {
      LOther->Link = LThis;
      SNOther->eraseFromParent();
    } else {
      Link = LOther;
      SNThis->eraseFromParent();
    }
  }
};

struct AddNodeCons {
  SSGNode *Left;
  SSGNode *Right;
  SSGNode *Result;

  static const char Rules[][3];
  // return a list of changed nodes and whether the constraint is fully solved.
  llvm::SmallVector<SSGNode *, 3> solve();
  bool isFullySolved() {
    return !isUnknown(Left) && !isUnknown(Right) && !isUnknown(Result);
  }
};

struct SubNodeCons {
  SSGNode *Left;
  SSGNode *Right;
  SSGNode *Result;
  static const char Rules[][3];
  llvm::SmallVector<SSGNode *, 3> solve();
  bool isFullySolved() {
    return !isUnknown(Left) && !isUnknown(Right) && !isUnknown(Result);
  }
};
struct NeNodeCons {
  SSGNode *Left;
  SSGNode *Right;
  llvm::SmallVector<SSGNode *, 3> solve();
  bool isFullySolved() {
    assert(isUnknown(Left) == isUnknown(Right));
    return !isUnknown(Left) && !isUnknown(Right);
  }
};
using NodeCons = std::variant<AddNodeCons, SubNodeCons, NeNodeCons>;

struct PNIConsNode : node_with_erase<PNIConsNode, StorageShapeGraph> {
  PNIConsNode(StorageShapeGraph &SSG, NodeCons C)
      : node_with_erase(SSG), C(C) {}
  NodeCons C;
  llvm::SmallVector<SSGNode *, 3> solve() {
    assert(false && "TODO");
    // call solve according to the variant
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->solve();
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->solve();
    } else if (auto *Cmp = std::get_if<NeNodeCons>(&C)) {
      return Cmp->solve();
    }
    // If fully solved, remove from NodeToConstraint Map. Remove self.
  }
  bool isFullySolved() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->isFullySolved();
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->isFullySolved();
    } else if (auto *Cmp = std::get_if<NeNodeCons>(&C)) {
      return Cmp->isFullySolved();
    }
    assert(false && "PNIConsNode::isFullySolved: unhandled variant");
  }
};

struct SignednessNode : node_with_erase<SignednessNode, StorageShapeGraph> {
  SignednessNode(StorageShapeGraph &SSG) : node_with_erase(SSG) {}
  SSGNode *Node;
  bool IsSigned;
  const char *Reason;
};

struct StorageShapeGraph {
  std::string FuncName;
  SSGNode *Memory = nullptr;

  // region ilist definition
  // list for SSGNode
  using NodesType = llvm::ilist<SSGNode>;
  NodesType Nodes;
  static NodesType StorageShapeGraph::*getSublistAccess(SSGNode *) {
    return &StorageShapeGraph::Nodes;
  }

  // list for ConstraintNode
  using ConstraintsType = llvm::ilist<PNIConsNode>;
  ConstraintsType Constraints;
  static ConstraintsType StorageShapeGraph::*getSublistAccess(PNIConsNode *) {
    return &StorageShapeGraph::Constraints;
  }
  // Map for worklist algorithm.
  std::map<SSGNode *, std::set<PNIConsNode *>> NodeToConstraint;

  // list for SignednessNode
  using SignednessType = llvm::ilist<SignednessNode>;
  SignednessType Signedness;
  static SignednessType StorageShapeGraph::*getSublistAccess(SignednessNode *) {
    return &StorageShapeGraph::Signedness;
  }
  std::map<SSGNode *, std::set<SignednessNode *>> NodeToSignedness;

  // endregion ilist definition

  StorageShapeGraph(std::string FuncName) : FuncName(FuncName) { initMemory(); }

  void solve();

  SSGNode *initMemory() {
    Memory = createUnknown();
    return Memory;
  }

  void addAddCons(SSGNode *Left, SSGNode *Right, SSGNode *Result) {
    AddNodeCons C = {.Left = Left, .Right = Right, .Result = Result};
    auto *Node = new PNIConsNode(*this, C);
    NodeToConstraint[Left].insert(Node);
    NodeToConstraint[Right].insert(Node);
    NodeToConstraint[Result].insert(Node);
    Constraints.push_back(Node);
  }

  void addSubCons(SSGNode *Left, SSGNode *Right, SSGNode *Result) {
    SubNodeCons C = {.Left = Left, .Right = Right, .Result = Result};
    auto *Node = new PNIConsNode(*this, C);
    NodeToConstraint[Left].insert(Node);
    NodeToConstraint[Right].insert(Node);
    NodeToConstraint[Result].insert(Node);
    Constraints.push_back(Node);
  }

  void addNeCons(SSGNode *Left, SSGNode *Right) {
    NeNodeCons C = {.Left = Left, .Right = Right};
    auto *Node = new PNIConsNode(*this, C);
    NodeToConstraint[Left].insert(Node);
    NodeToConstraint[Right].insert(Node);
    Constraints.push_back(Node);
  }

  friend struct SSGNode;
  static unsigned long PNVarCounter;
  std::map<PNVarTy, std::set<SSGNode *>> PNVarToNode;
  SSGNode *createUnknown() {
    SSGNode *N = new SSGNode(*this, ++PNVarCounter);
    Nodes.push_back(N);
    return N;
  }
  SSGNode *createPrimitive(std::string Name) {
    SSGNode *N = new SSGNode(*this, NonPtr);
    Nodes.push_back(N);
    return N;
  }

  void mergePNVar(PNVarTy Var, PNVarTy Target) {
    assert(Var >= PNVarStart && Target != 0);
    for (auto *N : PNVarToNode[Var]) {
      N->PNVar = Target;
      PNVarToNode[Target].insert(N);
    }
    PNVarToNode.erase(Var);
  }
};

} // namespace notdec::retypd

#endif
