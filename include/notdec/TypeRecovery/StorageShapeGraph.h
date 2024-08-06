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

// TODO: optimization: Use nullptr to represent Unknown?
struct Unknown {
  bool operator<(const Unknown &rhs) const { return false; }
  bool operator==(const Unknown &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const Unknown &Other) const { return !(*this == Other); }
};

struct Primitive {
  std::string name;
  bool operator<(const Primitive &rhs) const {
    return std::tie(name) < std::tie(name);
  }
  bool operator==(const Primitive &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const Primitive &Other) const { return !(*this == Other); }
};

struct Pointer {
  bool operator<(const Pointer &rhs) const { return false; }
  bool operator==(const Pointer &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
  bool operator!=(const Pointer &Other) const { return !(*this == Other); }
};

using StorageShapeTy = std::variant<Unknown, Primitive, Pointer>;
// mergeMap[left index][right index] = isPreserveLeft
// column -> right index, row -> left index
const unsigned char UniTyMergeMap[][3] = {
    {2, 0, 0},
    {1, 2, 0},
    {1, 1, 2},
};

Primitive unifyPrimitive(const Primitive &Left, const Primitive &Right);

Pointer unifyPointer(const Pointer &Left, const Pointer &Right);

StorageShapeTy unify(const StorageShapeTy &Left, const StorageShapeTy &Right);

StorageShapeTy fromIPChar(char C);

struct StorageShapeGraph;

struct SSGNode
    : public llvm::ilist_node_with_parent<SSGNode, StorageShapeGraph> {
  StorageShapeGraph *Parent = nullptr;
  inline StorageShapeGraph *getParent() { return Parent; }
  llvm::iplist<SSGNode>::iterator eraseFromParent();

  SSGNode(StorageShapeGraph &SSG) : Parent(&SSG) {}

  // protected:
  StorageShapeTy Ty;
  const StorageShapeTy &getTy() { return Ty; }
  bool isUnknown() { return std::holds_alternative<Unknown>(Ty); }
  bool isPrimitive() { return std::holds_alternative<Primitive>(Ty); }
  bool isPointer() { return std::holds_alternative<Pointer>(Ty); }
  char getIPChar() {
    if (isUnknown()) {
      return 'u';
    } else if (isPrimitive()) {
      return 'i';
    } else if (isPointer()) {
      return 'p';
    }
    assert(false && "SSGNode::getip: unhandled type");
  }

  /// Setting current type by reusing the unify logic.
  /// return isChanged (not include signedness). TODO consider signedness later
  bool unifyStorageShape(StorageShapeTy other) {
    StorageShapeTy R = notdec::retypd::unify(Ty, other);
    bool isChanged = R != Ty;
    Ty = R;
    return isChanged;
  }

  /// merge two SSGNode into one. Return true if the left one is preserved.
  /// the other one is expected to be removed.
  bool unify(SSGNode &other) {
    StorageShapeTy R = notdec::retypd::unify(Ty, other.Ty);
    if (R == Ty) {
      return true;
    } else if (R == other.Ty) {
      return false;
    } else {
      Ty = R;
      return true;
    }
  }

protected:
  friend class AddNodeCons;
  // Another Union-find layer for solving pointer/number identification.
  SSGNode *Link = nullptr;
  SSGNode *PNIlookup() {
    std::vector<SSGNode *> path;
    SSGNode *N = this;
    while (N->Link != nullptr) {
      path.push_back(N);
      N = N->Link;
    }
    for (auto *P : path) {
      P->Link = N;
    }
    return N;
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
  std::pair<llvm::SmallVector<SSGNode *, 3>, bool> solve();
};
struct SubNodeCons {
  SSGNode *Left;
  SSGNode *Right;
  SSGNode *Result;
  static const char Rules[][3];
};
struct CmpNodeCons {
  SSGNode *Left;
  SSGNode *Right;
};
using NodeCons = std::variant<AddNodeCons, SubNodeCons, CmpNodeCons>;

struct PNIConsNode : node_with_erase<PNIConsNode, StorageShapeGraph> {
  PNIConsNode(StorageShapeGraph &SSG, NodeCons C)
      : node_with_erase(SSG), C(C) {}
  NodeCons C;
  llvm::SmallVector<SSGNode *, 3> solve() {
    assert(false && "TODO");
    // call solve according to the variant
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      auto [Changed, FullySolved] = Add->solve();
      return Changed;
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
    } else if (auto *Cmp = std::get_if<CmpNodeCons>(&C)) {
    }
    // If fully solved, remove from NodeToConstraint Map. Remove self.
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

  void addCmpCons(SSGNode *Left, SSGNode *Right) {
    CmpNodeCons C = {.Left = Left, .Right = Right};
    auto *Node = new PNIConsNode(*this, C);
    NodeToConstraint[Left].insert(Node);
    NodeToConstraint[Right].insert(Node);
    Constraints.push_back(Node);
  }

  SSGNode *createUnknown() {
    SSGNode *N = new SSGNode(*this);
    N->Ty = Unknown();
    Nodes.push_back(N);
    return N;
  }

  SSGNode *createPrimitive(std::string Name) {
    SSGNode *N = new SSGNode(*this);
    N->Ty = Primitive{.name = Name};
    Nodes.push_back(N);
    return N;
  }
};

} // namespace notdec::retypd

#endif
