#ifndef _NOTDEC_RETYPD_UNIFY_H_
#define _NOTDEC_RETYPD_UNIFY_H_

#include <array>
#include <cassert>
#include <cstdint>
#include <list>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <llvm/IR/Value.h>

#include "TypeRecovery/Schema.h"
#include "Utils/Range.h"
#include "utils.h"

namespace notdec {
struct ConstraintsGenerator;
}

namespace notdec::retypd {

struct StorageShapeGraph;
struct SSGNode;
struct PNINode;

enum PtrOrNum {
  Unknown = 0,
  NonPtr = 1,
  Pointer = 2,
};

bool isUnknown(const SSGNode *N);
bool isUnknown(const PNINode *N);
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

struct PNINode : public node_with_erase<PNINode, StorageShapeGraph> {
protected:
  friend struct StorageShapeGraph;
  PNINode(StorageShapeGraph &SSG);
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
  char getPNChar() const {
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

  std::string str() const {
    return "PNI_" + std::to_string(Id) + "(" + getPNChar() +
           (hasConflict ? "C" : "") + ")";
  }
};

struct SSGValue {
  /// Maintain the reverse map Parent->PNVarToNode
  PNINode *PNIVar = nullptr;
  // pointes to edge. Nullptr for empty struct with no relationship.
  SSGNode *PointsTo = nullptr;
};

struct SSGAggregate {
  using IndexTy = int64_t;
  // using RangeTy = std::pair<IndexTy, unsigned>;
  // using EntryTy = std::pair<RangeTy, SSGNode *>;
  struct EntryTy {
    IndexTy Start;
    unsigned Size;
    SSGNode *Target;
  };
  // Ensure range will not overlap
  using iterator = std::vector<EntryTy>::iterator;
  std::vector<EntryTy> Values;

  EntryTy *find(IndexTy Start, unsigned Size) {
    for (auto &Entry : Values) {
      unsigned End = Entry.Start + Entry.Size;
      if (Entry.Start <= Start && Entry.Size >= Size) {
        // Contains or equal
        return &Entry;
      } else if (Entry.Start >= Start + Size) {
        // Goes over
        return nullptr;
      } else if (End < Start) {
        continue;
      } else {
        assert(false && "SSGAggregate::find: overlap?");
      }
    }
    return nullptr;
  }
};

struct SSGNode
    : public llvm::ilist_node_with_parent<SSGNode, StorageShapeGraph> {
  StorageShapeGraph *Parent = nullptr;
  std::string Name;
  unsigned long Id = 0;
  unsigned Size = 0;
  inline StorageShapeGraph *getParent() { return Parent; }
  llvm::iplist<SSGNode>::iterator eraseFromParent();

protected:
  friend struct StorageShapeGraph;
  SSGNode(StorageShapeGraph &SSG, unsigned size, bool isAgg = false);
  /// Move constructor for makeAggregate
  // SSGNode(SSGNode &&Other);
  void setPNVar(PNINode *PN) {
    assert(std::holds_alternative<SSGValue>(Ty));
    getAsValue().PNIVar = PN;
  }

public:
  void setName(const std::string N) { Name = N; }
  std::string getName() const { return Name; }

  using SSGTy = std::variant<SSGValue, SSGAggregate>;
  SSGTy Ty;
  // reverse link to the users.
  std::set<SSGNode *> AggregateUsers;
  bool isAggregate() const { return std::holds_alternative<SSGAggregate>(Ty); }
  SSGAggregate &getAggregate() {
    assert(isAggregate());
    return std::get<SSGAggregate>(Ty);
  }
  const SSGAggregate &getAggregate() const {
    assert(isAggregate());
    return std::get<SSGAggregate>(Ty);
  }
  // Allow implicit conversion to SSGValue by add zero offset
  notdec::retypd::SSGValue &getAsValue() {
    if (!isAggregate()) {
      return std::get<SSGValue>(Ty);
    } else {
      auto &Agg = getAggregate();
      assert(!Agg.Values.empty());
      assert(Agg.Values.front().Start == 0);
      return Agg.Values.front().Target->getAsValue();
    }
  }
  const notdec::retypd::SSGValue &getAsValue() const {
    return const_cast<SSGNode *>(this)->getAsValue();
  }

  SSGAggregate &getPointsToAgg();

  // #region Ptr/Num handling
  const PNINode *getPNVar() const { return getAsValue().PNIVar; }
  PNINode *getPNVar() { return getAsValue().PNIVar; }
  SSGNode *getPointsTo() { return getAsValue().PointsTo; }
  void setPointsTo(SSGNode *N) { getAsValue().PointsTo = N; }
  PtrOrNum getPNTy() const { return getPNVar()->getPtrOrNum(); }
  bool isNonPtr() const { return getPNTy() == NonPtr; }
  bool isPointer() const { return getPNTy() == Pointer; }
  bool isUnknown() const { return getPNTy() == Unknown; }

  bool setPtrOrNum(PtrOrNum Ty) { return getPNVar()->setPtrOrNum(Ty); }
  /// Unify the PNVarTy of the node to other type or unknown var.
  SSGNode *unifyPN(SSGNode &Other);
  // #endregion Ptr/Num handling

  llvm::ConstantInt *IntC = nullptr;
  bool isIntConstant() const { return IntC != nullptr; }
  void setIntConstant(llvm::ConstantInt *Constant) { this->IntC = Constant; }

  std::set<SSGNode *> strictParent;

  unsigned getSize() const {
    if (!isAggregate()) {
      return Size;
    } else {
      auto &Values = getAggregate().Values;
      if (Values.empty()) {
        return 0;
      } else {
        // no recursive update
        return Values.back().Start + Values.back().Size;
      }
    }
  }

  SSGNode *cloneNonAgg() const;
  // void makeAggregate();

  void replaceUseOfWith(SSGNode *Old, SSGNode *New) {
    if (!isAggregate()) {
      if (isPointer() && getPointsTo() == Old) {
        setPointsTo(New);
      }
    } else {
      auto &Agg = getAggregate();
      for (auto &Entry : Agg.Values) {
        if (Entry.Target == Old) {
          Entry.Target = New;
        }
      }
    }
  }

  /// Merge the other SSGNode into one.
  /// the other SSGNode is expected to be removed.
  void mergeTo(SSGNode &OtherNode, OffsetRange Offset = {.offset = 0});
  void mergeTo(SSGNode &OtherNode, int64_t Offset) {
    mergeTo(OtherNode, {.offset = Offset});
  }

  /// Propagate the size change (capabilities) upward.
  /// May invalidate all index/iterator after the entry.
  void updateEntSize(std::size_t Index);
  void updateEntSize(SSGAggregate::iterator Index) {
    updateEntSize(std::distance(getAggregate().Values.begin(), Index));
  }

  /// delegate a node to a offset
  void delegateToOffset(SSGNode &Target, int64_t Offset);

  void tryRemoveUse(SSGNode *Target) {
    assert(isAggregate());
    bool stillUse = false;
    for (auto &Ent : getAggregate().Values) {
      if (Ent.Target == Target) {
        stillUse = true;
      }
    }
    if (!stillUse) {
      Target->AggregateUsers.erase(this);
    }
  }
  void eraseEntry(std::size_t Index) {
    assert(isAggregate());
    auto &Agg = getAggregate();
    auto *Target = Agg.Values[Index].Target;
    Agg.Values.erase(Agg.Values.begin() + Index);
    tryRemoveUse(Target);
  }

  std::string str() const {
    return "SSG_" + std::to_string(Id) + "-" + getPNVar()->str();
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
};

struct AddNodeCons {
  PNINode *Left = nullptr;
  PNINode *Right = nullptr;
  PNINode *Result = nullptr;
  llvm::BinaryOperator *Inst;

  static const char Rules[][3];
  // return a list of changed nodes and whether the constraint is fully solved.
  llvm::SmallVector<PNINode *, 3> solve();
  bool isFullySolved() {
    return !isUnknown(Left) && !isUnknown(Right) && !isUnknown(Result);
  }
};

struct SubNodeCons {
  PNINode *Left = nullptr;
  PNINode *Right = nullptr;
  PNINode *Result = nullptr;
  llvm::BinaryOperator *Inst;

  static const char Rules[][3];
  llvm::SmallVector<PNINode *, 3> solve();
  bool isFullySolved() {
    return !isUnknown(Left) && !isUnknown(Right) && !isUnknown(Result);
  }
};

/// Sub = Sup + Offset
/// When adding SubTypeNodeCons, also add to PNI2Cons map.
struct SubTypeNodeCons {
  SSGNode *Sub = nullptr;
  SSGNode *Sup = nullptr;
  OffsetRange Offset;
  void solve();
  bool isFullySolved() { return !isUnknown(Sub) && !isUnknown(Sup); }
};

using NodeCons = std::variant<AddNodeCons, SubNodeCons, SubTypeNodeCons>;

struct ConsNode : node_with_erase<ConsNode, StorageShapeGraph> {
  ConsNode(StorageShapeGraph &SSG, NodeCons C) : node_with_erase(SSG), C(C) {}
  NodeCons C;
  llvm::SmallVector<PNINode *, 3> solve() {
    // call solve according to the variant
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->solve();
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->solve();
    } else if (auto *SubType = std::get_if<SubTypeNodeCons>(&C)) {
      return {};
    }
    assert(false && "PNIConsNode::solve: unhandled variant");
  }
  bool isFullySolved() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return Add->isFullySolved();
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return Sub->isFullySolved();
    } else if (auto *SubType = std::get_if<SubTypeNodeCons>(&C)) {
      return SubType->isFullySolved();
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
      return;
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
      return;
    } else if (auto *SubType = std::get_if<SubTypeNodeCons>(&C)) {
      return;
    }
    assert(false && "PNIConsNode::replaceUseOfWith: unhandled variant");
  }
  bool isOfPN() const { return !std::holds_alternative<SubTypeNodeCons>(C); }
  std::array<const PNINode *, 3> getNodes() const {
    auto ret = const_cast<ConsNode *>(this)->getNodes();
    return {ret[0], ret[1], ret[2]};
  }
  std::array<PNINode *, 3> getNodes() {
    if (auto *Add = std::get_if<AddNodeCons>(&C)) {
      return {Add->Left, Add->Right, Add->Result};
    } else if (auto *Sub = std::get_if<SubNodeCons>(&C)) {
      return {Sub->Left, Sub->Right, Sub->Result};
    } else if (auto *SubType = std::get_if<SubTypeNodeCons>(&C)) {
      assert(
          false &&
          "PNIConsNode::getNodes: cannot get PNINodes for SubTypeConstraint");
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
    } else if (auto *SubType = std::get_if<SubTypeNodeCons>(&C)) {
      assert(false &&
             "PNIConsNode::getInst: cannot call getInst for SubTypeConstraint");
    }
    assert(false && "PNIConsNode::getInst: unhandled variant");
  }
};

struct SignednessNode : node_with_erase<SignednessNode, StorageShapeGraph> {
  SignednessNode(StorageShapeGraph &SSG) : node_with_erase(SSG) {}
  SSGNode *Node;
  bool IsSigned;
  const char *Reason;
};

struct StorageShapeGraph {
  std::function<void(const retypd::ConsNode *)> onEraseConstraint;
  std::string Name;
  SSGNode *Memory = nullptr;

  // region linked list definitions
  // list for SSGNode
  using NodesType = llvm::ilist<SSGNode>;
  NodesType Nodes;
  static NodesType StorageShapeGraph::*getSublistAccess(SSGNode *) {
    return &StorageShapeGraph::Nodes;
  }

  // list for ConstraintNode
  using ConstraintsType = llvm::ilist<ConsNode>;
  ConstraintsType Constraints;
  static ConstraintsType StorageShapeGraph::*getSublistAccess(ConsNode *) {
    return &StorageShapeGraph::Constraints;
  }
  // Map for worklist algorithm.
  std::map<PNINode *, std::set<ConsNode *>> PNIToCons;

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
    auto *N = new PNINode(*this);
    PNINodes.push_back(N);
    return N;
  }
  // endregion linked list definitions

  StorageShapeGraph(
      std::function<void(const retypd::ConsNode *)> onEraseConstraint,
      std::string Name)
      : onEraseConstraint(onEraseConstraint), Name(Name) {
    initMemory();
  }

  void solve();

  SSGNode *initMemory() {
    Memory = createUnknown(0);
    return Memory;
  }

  void addAddCons(SSGNode *Left, SSGNode *Right, SSGNode *Result,
                  llvm::BinaryOperator *Inst) {
    AddNodeCons C = {.Left = Left->getPNVar(),
                     .Right = Right->getPNVar(),
                     .Result = Result->getPNVar(),
                     .Inst = Inst};
    auto *Node = new ConsNode(*this, C);
    PNIToCons[Left->getPNVar()].insert(Node);
    PNIToCons[Right->getPNVar()].insert(Node);
    PNIToCons[Result->getPNVar()].insert(Node);
    Constraints.push_back(Node);
  }

  void addSubCons(SSGNode *Left, SSGNode *Right, SSGNode *Result,
                  llvm::BinaryOperator *Inst) {
    SubNodeCons C = {.Left = Left->getPNVar(),
                     .Right = Right->getPNVar(),
                     .Result = Result->getPNVar(),
                     .Inst = Inst};
    auto *Node = new ConsNode(*this, C);
    PNIToCons[Left->getPNVar()].insert(Node);
    PNIToCons[Right->getPNVar()].insert(Node);
    PNIToCons[Result->getPNVar()].insert(Node);
    Constraints.push_back(Node);
  }

  void addSubTypeCons(SSGNode *Sub, SSGNode *Sup, OffsetRange Offset) {
    if (Sub == Sup) {
      return;
    }
    if (Sub->isUnknown() || Sup->isUnknown()) {
      Sub->unifyPN(*Sup);
    }
    SubTypeNodeCons C = {.Sub = Sub, .Sup = Sup, .Offset = Offset};
    auto *Node = new ConsNode(*this, C);
    Constraints.push_back(Node);
    // make the constraint depend on the PNINode
    PNIToCons[Sub->getPNVar()].insert(Node);
    PNIToCons[Sup->getPNVar()].insert(Node);
  }

  friend struct SSGNode;

protected:
  static unsigned long IdCounter;

public:
  unsigned long getNextId() { return IdCounter++; }
  // SSGNode *moveNonAgg(SSGNode &N) {
  //   // target to the same PNINode. Because struct as a value represents the
  //   // value at offset zero. (implicit add offset zero.)
  //   assert(!N.isAggregate());
  //   SSGNode *NewN = new SSGNode(std::move(N));
  //   Nodes.push_back(NewN);
  //   return NewN;
  // }
  SSGNode *createAgg() {
    SSGNode *N = new SSGNode(*this, 0, true);
    Nodes.push_back(N);
    return N;
  }
  SSGNode *createUnknown(unsigned int Size) {
    SSGNode *N = new SSGNode(*this, Size);
    Nodes.push_back(N);
    return N;
  }
  SSGNode *createNonPtr(std::string Name, unsigned int Size) {
    // TODO handle Name
    SSGNode *N = new SSGNode(*this, Size);
    N->getPNVar()->setPtrOrNum(NonPtr);
    Nodes.push_back(N);
    return N;
  }

  void mergePNVarTo(PNINode *Var, PNINode *Target) {
    // maintain PNIToNode
    if (PNIToNode.count(Var)) {
      for (auto *N : PNIToNode[Var]) {
        N->setPNVar(Target);
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

  SSGNode *replaceSSGNodes(SSGNode *Discard, SSGNode *Keep) {
    for (auto *User : Discard->AggregateUsers) {
      User->replaceUseOfWith(Discard, Keep);
    }
    Discard->eraseFromParent();
    assert(false && "TODO");
  }

  PNINode *mergePNINodes(PNINode *Left, PNINode *Right) {
    if (Left == Right ||
        (!isUnknown(Left) && Left->getPtrOrNum() == Right->getPtrOrNum())) {
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
};

} // namespace notdec::retypd

#endif
