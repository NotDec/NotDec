#ifndef _NOTDEC_RETYPD_UNIFY_H_
#define _NOTDEC_RETYPD_UNIFY_H_

#include <array>
#include <cassert>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <llvm/IR/Value.h>

#include "TypeRecovery/Schema.h"
#include "utils.h"

namespace notdec::retypd {

struct StorageShapeGraph;
struct SSGNode;
struct PNINode;

enum PtrOrNum {
  Unknown = 0,
  NonPtr = 1,
  Pointer = 2,
};

using PNIVarTy = unsigned long;
bool isUnknown(SSGNode *N);
bool isUnknown(PNINode *N);
inline PtrOrNum inverse(PtrOrNum Ty) {
  if (Ty == NonPtr) {
    return Pointer;
  } else if (Ty == Pointer) {
    return NonPtr;
  } else {
    assert(false && "inverse: Unknown type");
  }
}

// mergeMap[left index][right index] = isPreserveLeft
// column -> right index, row -> left index
const unsigned char UniTyMergeMap[][3] = {
    {2, 0, 0},
    {1, 2, 0},
    {1, 1, 2},
};

PtrOrNum unify(const PtrOrNum &Left, const PtrOrNum &Right);

PtrOrNum fromIPChar(char C);

struct PNINode : node_with_erase<PNINode, StorageShapeGraph> {
protected:
  friend struct StorageShapeGraph;
  PNINode(StorageShapeGraph &SSG, PNIVarTy Var)
      : node_with_erase(SSG), Var(Var) {}
  PNIVarTy Var = 0;
  PtrOrNum Ty = Unknown;
  bool hasConflict = false;

public:
  /// Convenient method to set the type of the PNVar.
  bool setPtrOrNum(PtrOrNum NewTy);
  PtrOrNum getPtrOrNum() { return Ty; }
  void setConflict() { hasConflict = true; }

  bool isNonPtr() { return getPtrOrNum() == NonPtr; }
  bool isPointer() { return getPtrOrNum() == Pointer; }
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
  PNINode *unifyPN(PNINode &other);
};

struct SSGNode
    : public llvm::ilist_node_with_parent<SSGNode, StorageShapeGraph> {
  StorageShapeGraph *Parent = nullptr;
  inline StorageShapeGraph *getParent() { return Parent; }
  llvm::iplist<SSGNode>::iterator eraseFromParent();

protected:
  friend struct StorageShapeGraph;
  SSGNode(StorageShapeGraph &SSG);

public:
  // #region Ptr/Num handling
  /// Represent the type of the node 0-2, Or an unknown variable 3+.
  /// Maintain the reverse map Parent->PNVarToNode
  PNINode *PNIVar;
  PNINode *getPNVar() { return PNIVar; }
  PtrOrNum getPNTy() { return PNIVar->getPtrOrNum(); }

  bool setPtrOrNum(PtrOrNum Ty) { return PNIVar->setPtrOrNum(Ty); }
  /// Unify the PNVarTy of the node to other type or unknown var.
  SSGNode *unifyPN(SSGNode &Other);

  // #endregion Ptr/Num handling

  /// merge two SSGNode into one. Return true if the left one is preserved.
  /// the other one is expected to be removed.
  bool unify(SSGNode &other) {
    PNIVar->unifyPN(*other.PNIVar);
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
  PNINode *Left;
  PNINode *Right;
  PNINode *Result;

  static const char Rules[][3];
  // return a list of changed nodes and whether the constraint is fully solved.
  llvm::SmallVector<PNINode *, 3> solve();
  bool isFullySolved() {
    return !isUnknown(Left) && !isUnknown(Right) && !isUnknown(Result);
  }
};

struct SubNodeCons {
  PNINode *Left;
  PNINode *Right;
  PNINode *Result;
  static const char Rules[][3];
  llvm::SmallVector<PNINode *, 3> solve();
  bool isFullySolved() {
    return !isUnknown(Left) && !isUnknown(Right) && !isUnknown(Result);
  }
};
using NodeCons = std::variant<AddNodeCons, SubNodeCons>;

struct PNIConsNode : node_with_erase<PNIConsNode, StorageShapeGraph> {
  PNIConsNode(StorageShapeGraph &SSG, NodeCons C)
      : node_with_erase(SSG), C(C) {}
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
  void replaceUseOfWith(PNINode *Old, PNINode *New) {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      if (Add->Left == Old) {
        Add->Left = New;
      }
      if (Add->Right == Old) {
        Add->Right = New;
      }
      if (Add->Result == Old) {
        Add->Result = New;
      }
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      if (Sub->Left == Old) {
        Sub->Left = New;
      }
      if (Sub->Right == Old) {
        Sub->Right = New;
      }
      if (Sub->Result == Old) {
        Sub->Result = New;
      }
    }
    assert(false && "PNIConsNode::replaceUseOfWith: unhandled variant");
  }
  std::array<PNINode *, 3> getNodes() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return {Add->Left, Add->Right, Add->Result};
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return {Sub->Left, Sub->Right, Sub->Result};
    }
    assert(false && "PNIConsNode::getNodes: unhandled variant");
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

  // region linked list definitions
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
  std::map<PNINode *, std::set<PNIConsNode *>> PNIToCons;

  // list for SignednessNode
  using SignednessType = llvm::ilist<SignednessNode>;
  SignednessType Signedness;
  static SignednessType StorageShapeGraph::*getSublistAccess(SignednessNode *) {
    return &StorageShapeGraph::Signedness;
  }
  std::map<SSGNode *, std::set<SignednessNode *>> NodeToSignedness;

  // list for PNINode
  using PNINodesType = llvm::ilist<PNINode>;
  PNINodesType PNINodes;
  static PNINodesType StorageShapeGraph::*getSublistAccess(PNINode *) {
    return &StorageShapeGraph::PNINodes;
  }
  std::map<PNINode *, std::set<SSGNode *>> PNIToNode;
  PNINode *createPNINode() {
    auto *N = new PNINode(*this, PNVarCounter++);
    PNINodes.push_back(N);
    return N;
  }
  // endregion linked list definitions

  StorageShapeGraph(std::string FuncName) : FuncName(FuncName) { initMemory(); }

  void solve();

  SSGNode *initMemory() {
    Memory = createUnknown();
    return Memory;
  }

  void addAddCons(SSGNode *Left, SSGNode *Right, SSGNode *Result) {
    AddNodeCons C = {.Left = Left->getPNVar(),
                     .Right = Right->getPNVar(),
                     .Result = Result->getPNVar()};
    auto *Node = new PNIConsNode(*this, C);
    PNIToCons[Left->getPNVar()].insert(Node);
    PNIToCons[Right->getPNVar()].insert(Node);
    PNIToCons[Result->getPNVar()].insert(Node);
    Constraints.push_back(Node);
  }

  void addSubCons(SSGNode *Left, SSGNode *Right, SSGNode *Result) {
    SubNodeCons C = {.Left = Left->getPNVar(),
                     .Right = Right->getPNVar(),
                     .Result = Result->getPNVar()};
    auto *Node = new PNIConsNode(*this, C);
    PNIToCons[Left->getPNVar()].insert(Node);
    PNIToCons[Right->getPNVar()].insert(Node);
    PNIToCons[Result->getPNVar()].insert(Node);
    Constraints.push_back(Node);
  }

  // void addNeCons(SSGNode *Left, SSGNode *Right) {
  //   NeNodeCons C = {.Left = Left, .Right = Right};
  //   auto *Node = new PNIConsNode(*this, C);
  //   NodeToConstraint[Left].insert(Node);
  //   NodeToConstraint[Right].insert(Node);
  //   Constraints.push_back(Node);
  // }

  friend struct SSGNode;
  static unsigned long PNVarCounter;
  SSGNode *createUnknown() {
    SSGNode *N = new SSGNode(*this);
    Nodes.push_back(N);
    return N;
  }
  SSGNode *createPrimitive(std::string Name) {
    // TODO handle Name
    SSGNode *N = new SSGNode(*this);
    N->PNIVar->setPtrOrNum(NonPtr);
    Nodes.push_back(N);
    return N;
  }

  void mergePNVarTo(PNINode *Var, PNINode *Target) {
    // maintain PNIToNode
    if (PNIToNode.count(Var)) {
      for (auto *N : PNIToNode[Var]) {
        N->PNIVar = Target;
        PNIToNode[Target].insert(N);
      }
      PNIToNode.erase(Var);
    }
    // maintain NodeToConstraint
    if (PNIToCons.count(Var)) {
      for (auto *Cons : PNIToCons[Var]) {
        Cons->replaceUseOfWith(Var, Target);
      }
      PNIToCons.erase(Var);
    }
    Var->eraseFromParent();
  }

  PNINode *mergePNINodes(PNINode *Left, PNINode *Right) {
    if (Left == Right || Left->getPtrOrNum() == Right->getPtrOrNum()) {
      // no need to merge
      return nullptr;
    }
    if (Left->getPtrOrNum() == Unknown) {
      mergePNVarTo(Left, Right);
      return Right;
    } else if (Right->getPtrOrNum() == Unknown) {
      mergePNVarTo(Right, Left);
      return Left;
    } else {
      if (Left->getPtrOrNum() == NonPtr && Right->getPtrOrNum() == Pointer) {
        std::cerr << "Warning: mergePNINodes: Pointer and NonPtr merge\n";
        mergePNVarTo(Left, Right);
        Right->setConflict();
        return Right;
      } else if ((Left->getPtrOrNum() == Pointer &&
                  Right->getPtrOrNum() == NonPtr)) {
        std::cerr << "Warning: mergePNINodes: Pointer and NonPtr merge\n";
        mergePNVarTo(Right, Left);
        Left->setConflict();
        return Left;
      } else {
        assert(false && "should be unreachable");
      }
    }
  }
  void eraseConstraint(PNIConsNode *Cons) {
    for (auto *N : Cons->getNodes()) {
      assert(N != nullptr);
      PNIToCons[N].erase(Cons);
    }
    Cons->eraseFromParent();
  }
};

} // namespace notdec::retypd

#endif
