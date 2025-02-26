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
#include "notdec-llvm2c/Interface/Range.h"
#include "Utils/Utils.h"

namespace notdec {
struct ConstraintsGenerator;
}

namespace notdec::retypd {

struct StorageShapeGraph;
struct SSGNode;

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

struct SignednessNode : node_with_erase<SignednessNode, StorageShapeGraph> {
  SignednessNode(StorageShapeGraph &SSG) : node_with_erase(SSG) {}
  SSGNode *Node;
  bool IsSigned;
  const char *Reason;
};

struct StorageShapeGraph {
  std::string Name;
  SSGNode *Memory = nullptr;
  PNIGraph PG;

  // region linked list definitions
  // list for SSGNode
  using NodesType = llvm::ilist<SSGNode>;
  NodesType Nodes;
  static NodesType StorageShapeGraph::*getSublistAccess(SSGNode *) {
    return &StorageShapeGraph::Nodes;
  }

  // list for SignednessNode
  using SignednessType = llvm::ilist<SignednessNode>;
  SignednessType Signedness;
  static SignednessType StorageShapeGraph::*getSublistAccess(SignednessNode *) {
    return &StorageShapeGraph::Signedness;
  }
  std::map<SSGNode *, std::set<SignednessNode *>> NodeToSignedness;

  // endregion linked list definitions

  StorageShapeGraph(
      std::function<void(const retypd::ConsNode *)> onEraseConstraint,
      std::string Name)
      : Name(Name), PG(onEraseConstraint, Name) {
    initMemory();
  }

  void solve();

  SSGNode *initMemory() {
    Memory = createUnknown(0);
    return Memory;
  }

  void addSubTypeCons(SSGNode *Sub, SSGNode *Sup, OffsetRange Offset) {
    // if (Sub == Sup) {
    //   return;
    // }
    // if (Sub->isUnknown() || Sup->isUnknown()) {
    //   Sub->unifyPN(*Sup);
    // }
    // SubTypeNodeCons C = {.Sub = Sub, .Sup = Sup, .Offset = Offset};
    // auto *Node = new ConsNode(*this, C);
    // Constraints.push_back(Node);
    // // make the constraint depend on the PNINode
    // PNIToCons[Sub->getPNVar()].insert(Node);
    // PNIToCons[Sup->getPNVar()].insert(Node);
  }

  friend struct SSGNode;

protected:
  static unsigned long IdCounter;

public:
  unsigned long getNextId() { return IdCounter++; }
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

  SSGNode *replaceSSGNodes(SSGNode *Discard, SSGNode *Keep) {
    for (auto *User : Discard->AggregateUsers) {
      User->replaceUseOfWith(Discard, Keep);
    }
    Discard->eraseFromParent();
    assert(false && "TODO");
  }
};

} // namespace notdec::retypd

#endif
