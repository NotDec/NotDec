#ifndef _NOTDEC_RETYPD_PNI_H_
#define _NOTDEC_RETYPD_PNI_H_

#include <array>
#include <cassert>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/IR/InstrTypes.h>

#include "Utils/Range.h"
#include "utils.h"

namespace notdec::retypd {

// Forward Declaration
struct PNIGraph;
struct CGNode;

OffsetRange matchOffsetRange(llvm::Value *I);

// #region PtrOrNum

enum PtrOrNum {
  Unknown = 0,
  NonPtr = 1,
  Pointer = 2,
};

bool isUnknown(const CGNode *N);
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
// #endregion PtrOrNum

struct PNINode : public node_with_erase<PNINode, PNIGraph> {
protected:
  friend struct PNIGraph;
  PNINode(PNIGraph &SSG);
  unsigned long Id = 0;
  PtrOrNum Ty = Unknown;
  bool hasConflict = false;

public:
  llvm::iplist<PNINode>::iterator eraseFromParent();

  /// Convenient method to set the type of the PNVar.
  bool setPtrOrNum(PtrOrNum NewTy);
  PtrOrNum getPtrOrNum() const { return Ty; }
  void setConflict() { hasConflict = true; }

  bool isNonPtr() const { return getPtrOrNum() == NonPtr; }
  bool isPointer() const { return getPtrOrNum() == Pointer; }
  bool isUnknown() const { return getPtrOrNum() == Unknown; }
  char getPNChar() const {
    if (isUnknown()) {
      return 'u';
    } else if (isNonPtr()) {
      return 'i';
    } else if (isPointer()) {
      return 'p';
    }
    assert(false && "CGNode::getPNChar: unhandled type");
  }
  /// merge two PNVar into one. Return the unified PNVar.
  PNINode *unifyPN(PNINode &other);
  void addUser(CGNode *Node);
  void cloneFrom(const PNINode &N);

  std::string str() const {
    return "PNI_" + std::to_string(Id) + "(" + getPNChar() +
           (hasConflict ? "C" : "") + ")";
  }
};

struct AddNodeCons {
  CGNode *LeftNode = nullptr;
  CGNode *RightNode = nullptr;
  CGNode *ResultNode = nullptr;
  llvm::BinaryOperator *Inst;

  static const char Rules[][3];
  // return a list of changed nodes and whether the constraint is fully solved.
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
  void cloneFrom(ConsNode &N, std::map<const CGNode *, CGNode *> Old2New) {
    if (auto *Add = std::get_if<AddNodeCons>(&N.C)) {
      auto *NewLeft = Old2New[Add->LeftNode];
      auto *NewRight = Old2New[Add->RightNode];
      auto *NewResult = Old2New[Add->ResultNode];
      auto *NewInst = Add->Inst;
      C = AddNodeCons{NewLeft, NewRight, NewResult, NewInst};
    } else if (auto *Sub = std::get_if<SubNodeCons>(&N.C)) {
      auto *NewLeft = Old2New[Sub->LeftNode];
      auto *NewRight = Old2New[Sub->RightNode];
      auto *NewResult = Old2New[Sub->ResultNode];
      auto *NewInst = Sub->Inst;
      C = SubNodeCons{NewLeft, NewRight, NewResult, NewInst};
    }
    assert(false && "PNIConsNode::cloneFrom: unhandled variant");
  }
};

struct PNIGraph {
  std::string Name;
  std::set<ConsNode *> Worklist;

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
  PNINode *createPNINode(CGNode *To) {
    auto *N = new PNINode(*this);
    PNINodes.push_back(N);
    PNIToNode[N].insert(To);
    return N;
  }

  PNIGraph(std::string Name) : Name(Name) {}
  void cloneFrom(const PNIGraph &G, std::map<const CGNode *, CGNode *> Old2New);

  void addAddCons(CGNode *Left, CGNode *Right, CGNode *Result,
                  llvm::BinaryOperator *Inst);

  void addSubCons(CGNode *Left, CGNode *Right, CGNode *Result,
                  llvm::BinaryOperator *Inst);

  PNINode *mergePNINodes(PNINode *Left, PNINode *Right) {
    if (Left == Right ||
        (!Left->isUnknown() && Left->getPtrOrNum() == Right->getPtrOrNum())) {
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
  void eraseConstraint(ConsNode *Cons);
  bool solve();
  void onUpdatePNType(PNINode *N);

protected:
  void markChanged(PNINode *N, ConsNode *Except = nullptr);
  void mergePNVarTo(PNINode *Var, PNINode *Target);
};

} // namespace notdec::retypd

#endif
