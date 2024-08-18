#include "TypeRecovery/StorageShapeGraph.h"
#include "Utils/Range.h"
#include "optimizers/ConstraintGenerator.h"
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <queue>

namespace notdec::retypd {

unsigned long StorageShapeGraph::IdCounter = 1;

void StorageShapeGraph::solve() {
  // write a worklist algorithm.
  // When a unknown var changed to known, add all constraints that use this var.
  // (maintain a map from var to constraints, maintain when two unknown vars
  // merge)
  // When two unknown vars merge, add all constraints that use these two vars.
  std::set<ConsNode *> Worklist;
  for (auto &C : Constraints) {
    Worklist.insert(&C);
  }
  while (!Worklist.empty()) {
    ConsNode *C = *Worklist.begin();
    Worklist.erase(C);
    auto Changed = C->solve();
    bool isFullySolved = C->isFullySolved();
    // add according to changed.
    for (auto *N : Changed) {
      for (auto *C2 : PNIToCons[N]) {
        if (C2 == C) {
          continue;
        }
        Worklist.insert(C2);
      }
    }
    // remove the constraint if fully solved.
    if (isFullySolved) {
      eraseConstraint(C);
      C = nullptr;
    }
  }
}

void StorageShapeGraph::eraseConstraint(ConsNode *Cons) {
  if (CG != nullptr) {
    CG->onEraseConstraint(Cons);
  }
  if (Cons->isOfPN()) {
    for (auto *N : Cons->getNodes()) {
      assert(N != nullptr);
      PNIToCons[N].erase(Cons);
    }
  }

  Cons->eraseFromParent();
}

// retypd. Figure 13.
// Lower case letters denote known integer or pointer types. Upper case letters
// denote inferred types. For example, the first column says that if X and Y are
// integral types in an ADD(X, Y ; Z) constraint, then Z is integral as well.
const char AddNodeCons::Rules[][3] = {
    {'i', 'i', 'I'}, {'I', 'I', 'i'}, {'p', 'I', 'P'},
    {'P', 'i', 'p'}, {'I', 'p', 'P'}, {'i', 'P', 'p'},
};

const char SubNodeCons::Rules[][3] = {
    {'i', 'I', 'I'}, {'I', 'i', 'i'}, {'P', 'i', 'p'}, {'P', 'p', 'I'},
    {'p', 'P', 'i'}, {'p', 'i', 'P'}, {'p', 'I', 'p'}};

llvm::SmallVector<PNINode *, 3> AddNodeCons::solve() {
  llvm::SmallVector<PNINode *, 3> Changed;

  // 1. solving using add rules.
  for (const char *Rule : Rules) {
    bool NotMatch = false;
    PNINode *Arr[3] = {Left, Right, Result};
    // first scan lower case letters
    for (unsigned i = 0; i < 3; i++) {
      if (std::islower(Rule[i]) && Rule[i] != Arr[i]->getPNChar()) {
        NotMatch = true;
        break;
      }
    }
    // if not match, continue.
    if (NotMatch) {
      continue;
    }
    // this rule match, apply upper case letter constraints.
    for (unsigned i = 0; i < 3; i++) {
      if (std::isupper(Rule[i])) {
        PtrOrNum ToUnify = fromIPChar(Rule[i]);
        bool IsChanged = Arr[i]->setPtrOrNum(ToUnify);
        // Update changed list accordingly.
        if (IsChanged) {
          Changed.push_back(Arr[i]);
        }
      }
    }
    return Changed;
  }

  unsigned unknownCount = 0;
  for (PNINode *N : {Left, Right, Result}) {
    if (isUnknown(N)) {
      unknownCount++;
    }
  }
  assert(unknownCount >= 2);

  // 2. check using alias relation
  // Left alias right: Must be number
  // this includes the Left == Right == Result case.
  if (Left == Right) {
    bool IsChanged = Left->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Left);
    }
    IsChanged = Right->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Right);
    }
    IsChanged = Result->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  }
  // Left != Right.
  else if (Left == Result) {
    // If Left == Result, Right must be number
    bool IsChanged = Right->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else if (Right == Result) {
    // same as above
    bool IsChanged = Left->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Left);
    }
  } else {
    // no alias at all. try to check merge rules.
    if (unknownCount < 3) {
      assert(unknownCount == 2);
      // 1. Unknown + Number = SameUnknown
      //    or Number + Unknown = SameUnknown
      if (Left->isNonPtr()) {
        // Unify Right and Result
        auto *Merged = Right->unifyPN(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Right);
        Changed.push_back(Result);
      } else if (Right->isNonPtr()) {
        // Unify Left and Result
        auto *Merged = Left->unifyPN(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Left);
        Changed.push_back(Result);
      } else if (Result->isPointer()) {
        // 2. Unknown + Unknown = Pointer
        // degrade to Left != Right constraint? Not very useful
      } else {
        assert(false && "Should not reach here");
      }
    } else {
      // all unknown, nothing we can do now.
    }
  }
  return Changed;
}

llvm::SmallVector<PNINode *, 3> SubNodeCons::solve() {
  llvm::SmallVector<PNINode *, 3> Changed;

  // 1. solving using add rules.
  for (const char *Rule : Rules) {
    bool NotMatch = false;
    PNINode *Arr[3] = {Left, Right, Result};
    // first scan lower case letters
    for (unsigned i = 0; i < 3; i++) {
      if (std::islower(Rule[i]) && Rule[i] != Arr[i]->getPNChar()) {
        NotMatch = true;
        break;
      }
    }
    // if not match, continue.
    if (NotMatch) {
      continue;
    }
    // this rule match, apply upper case letter constraints.
    for (unsigned i = 0; i < 3; i++) {
      if (std::isupper(Rule[i])) {
        PtrOrNum ToUnify = fromIPChar(Rule[i]);
        bool IsChanged = Arr[i]->setPtrOrNum(ToUnify);
        // Update changed list accordingly.
        if (IsChanged) {
          Changed.push_back(Arr[i]);
        }
      }
    }
    return Changed;
  }

  unsigned unknownCount = 0;
  for (PNINode *N : {Left, Right, Result}) {
    if (isUnknown(N)) {
      unknownCount++;
    }
  }
  assert(unknownCount >= 2);

  // 2. check using alias relation
  // Right alias Result: Must be number
  // this includes the Left == Right == Result case.
  if (Result == Right) {
    bool IsChanged = Left->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Left);
    }
    IsChanged = Right->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Right);
    }
    IsChanged = Result->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  } else if (Left == Right) {
    // If Left == Right, Result must be number
    // Must be unknown because there are at least 2 unknowns.
    assert(isUnknown(Left));
    bool IsChanged = Result->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  } else if (Left == Result) {
    // same as above
    bool IsChanged = Right->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else {
    // no alias at all. try to check merge rules.
    if (unknownCount < 3) {
      assert(unknownCount == 2);
      // 1. Unknown - Number = SameUnknown
      //    or Unknown - SameUnknown = Number
      if (Right->isNonPtr()) {
        // Unify Left and Result
        auto *Merged = Left->unifyPN(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Left);
        Changed.push_back(Result);
      } else if (Result->isNonPtr()) {
        // Unify Left and Right
        auto *Merged = Left->unifyPN(*Right);
        assert(Merged != nullptr);
        Changed.push_back(Left);
        Changed.push_back(Right);
      } else if (Result->isPointer()) {
        // 2. Pointer - Unknown = Unknown
        // degrade to Result != Right constraint? Not very useful
      } else {
        assert(false && "Should not reach here");
      }
    } else {
      // all unknown, nothing we can do now.
    }
  }
  return Changed;
}

void SubTypeNodeCons::solve() { Sup->unify(*Sub, Offset); }

PtrOrNum fromIPChar(char C) {
  switch (C) {
  case 'I':
    return NonPtr;
  case 'P':
    return Pointer;
  default:
    assert(false && "fromIPChar: unhandled char");
  }
}

llvm::iplist<PNINode>::iterator PNINode::eraseFromParent() {
  // Check links before destruction. Should be replaced beforehand.
  assert(Parent->PNIToNode.count(this) == 0);
  assert(Parent->PNIToCons.count(this) == 0);
  return node_with_erase<PNINode, StorageShapeGraph>::eraseFromParent();
}

llvm::iplist<SSGNode>::iterator SSGNode::eraseFromParent() {
  /// maintain links before destruction
  if (!isAggregate()) {
    // remove self from PNVarToNode map
    Parent->PNIToNode[getPNVar()].erase(this);
  }
  auto &List =
      getParent()->*(StorageShapeGraph::getSublistAccess((SSGNode *)nullptr));
  return List.erase(getIterator());
}

bool PNINode::setPtrOrNum(PtrOrNum NewTy) {
  assert(NewTy != Unknown);
  if (Ty == NewTy) {
    return false;
  }
  if ((Ty == NonPtr && NewTy == Pointer) ||
      (Ty == Pointer && NewTy == NonPtr)) {
    std::cerr << "Warning: PNINode::setPtrOrNum: Pointer and NonPtr merge\n";
    hasConflict = true;
    if (Ty == NonPtr && NewTy == Pointer) {
      Ty = Pointer;
      return true;
    } else {
      return false;
    }
  }
  assert(Ty == Unknown);
  Ty = NewTy;
  return true;
}

// Like a operator
PtrOrNum unify(const PtrOrNum &Left, const PtrOrNum &Right) {
  if (Left == Right) { // same inner type
    return Left;
  } else {
    unsigned char Val = UniTyMergeMap[Left][Right];
    assert(Val != 2);
    if (Val != 0) {
      return Left;
    } else {
      return Right;
    }
  }
}

PNINode *PNINode::unifyPN(PNINode &other) {
  auto *Node = Parent->mergePNINodes(this, &other);
  return Node;
}

SSGNode *SSGNode::unifyPN(SSGNode &Other) {
  auto *Node = getPNVar()->unifyPN(*Other.getPNVar());
  if (Node == nullptr) {
    // no need to merge
    return nullptr;
  } else if (Node == this->getPNVar()) {
    return this;
  } else {
    assert(Node == Other.getPNVar());
    return &Other;
  }
}

/// Unify a parent type node's capability into this node.
/// This node is the parent type of OtherNode. This node can have more
/// capabilities.
/// Example: For `q = p + 4`, run p.unify(q, 4).
void SSGNode::unify(SSGNode &OtherNode, OffsetRange OffR) {
  if (getPNTy() != OtherNode.getPNTy()) {
    assert(false && "SSGNode::unify: PN type mismatch");
  }
  if (isNonPtr()) {
    if (Size != OtherNode.Size) {
      std::cerr << "Warning: SSGNode::unify: size mismatch\n";
    }
    // TODO unify primitive type
    return;
  }
  // handle array later
  assert(OffR.access.size() == 0);
  int64_t Offset = OffR.offset;
  // 1. If offset is not zero, try to find or delegate the subrange for it.
  // Because this and other should have its own type (represented by a
  // SSGNode). Make offset zero first.
  if (Offset != 0) {
    if (!isAggregate()) {
      makeAggregate();
    }
    // try to find a range that contains OtherStart.
    auto OtherStart = Offset;
    bool Found = false;
    auto It = getAggregate().Values.begin();
    for (; It != getAggregate().Values.end(); It++) {
      auto &Entry = *It;
      unsigned EntryEnd = Entry.Start + Entry.Size;
      // Check according to the location of OtherStart
      // When Entry.Size == 0, the range is considered empty and not include
      // Entry.Start
      if (EntryEnd <= OtherStart) {
        // Case1: Entry.Start < EntryEnd <= OtherStart
        // Entry.Start == EntryEnd == OtherStart is considered as non-intersect
        // Keep iterating
      } else if (Entry.Start <= OtherStart && OtherStart < EntryEnd) {
        // Case2: OtherStart in a range: Entry.Start <= OtherStart < EntryEnd
        // Delegate unify here.
        assert(Entry.Size > 0);
        Found = true;
        Entry.Target->unify(OtherNode, OtherStart - Entry.Start);
        break;
      } else {
        // Case3: Out of the range, stop iterating.
        break;
      }
    }
    if (!Found) {
      // No range exists, delegate the range to OtherNode.
      // insert Entry2 into self
      auto NewEnt =
          getAggregate().Values.insert(It, {OtherStart, 0, &OtherNode});
      // maintain the reverse link
      OtherNode.AggregateUsers.insert(this);
      updateEntSize(NewEnt);
    }
    return;
  }
  // 2. If offset is zero, do the real merge.
  assert(Offset == 0);
  bool hasAgg = isAggregate() || OtherNode.isAggregate();
  if (!hasAgg) {
    unifyPN(OtherNode);
    assert(getPNTy() == OtherNode.getPNTy());
    if (isNonPtr()) {
      assert(Size == OtherNode.Size);
    } else {
      if (getPointsTo() == nullptr) {
        setPointsTo(OtherNode.getPointsTo());
        // 2. Update the size/replace the node upward.
      } else if (OtherNode.getPointsTo() == nullptr) {
        OtherNode.setPointsTo(this->getPointsTo());
        // 2. Update the size/replace the node upward.
      } else {
        getPointsTo()->unify(*OtherNode.getPointsTo());
      }
    }
  } else {
    if (!isAggregate()) {
      makeAggregate();
    }
    if (!OtherNode.isAggregate()) {
      OtherNode.makeAggregate();
    }
    auto &Agg = std::get<SSGAggregate>(Ty);
    auto &OtherAgg = std::get<SSGAggregate>(OtherNode.Ty);
    // merge Other into this. Assume that smaller offset range contains larger
    // offset range.
    unsigned Ind1 = 0;
    unsigned Ind2 = 0;
    while (Ind1 < Agg.Values.size() && Ind2 < OtherAgg.Values.size()) {
      auto &Entry = Agg.Values[Ind1];
      auto &Other = OtherAgg.Values[Ind2];
      auto *EntryNode = Entry.Target;
      auto EntryEnd = Entry.Start + Entry.Size;
      auto *OtherNode = Other.Target;
      auto OtherEnd = Other.Start + Other.Size;

      // Check according to the location of Start index
      if (EntryEnd <= Other.Start) {
        // Case1: EntryStart < EntryEnd <= OtherStart
        // OtherStart out of range of Entry
        // Entry is at the left of Other.
        Ind1++;
        continue;
      } else if (OtherEnd <= Entry.Start) {
        // Case2: OtherStart < OtherEnd <= EntryStart
        // Other is at the left of Entry.
        Agg.Values.insert(Agg.Values.begin() + Ind1, Other);
        OtherNode->AggregateUsers.insert(this);
        Ind1++;
        Ind2++;
        continue;
      } else if (Entry.Start <= Other.Start && Other.Start < EntryEnd) {
        // Case3: OtherStart in Entry
        EntryNode->unify(*OtherNode, Other.Start - Entry.Start);
        Ind2++;
      } else if (Other.Start <= Entry.Start && Entry.Start < OtherEnd) {
        // Case4: EntryStart in Other
        // merge into other
        OtherNode->unify(*EntryNode, Entry.Start - Other.Start);
        Agg.Values.at(Ind1) = {Other.Start, OtherNode->getSize(), OtherNode};
        Ind2++;
      } else {
        assert(false && "SSGNode::unify: should be unreachable");
      }
    }
  }
  Parent->replaceSSGNodes(&OtherNode, this);
}

// TODO detect infinite loop in struct delegate!
void SSGNode::updateEntSize(std::size_t Index) {
  assert(isAggregate());
  auto &Agg = getAggregate();
  assert(Index < Agg.Values.size());
  auto &Ent = Agg.Values.at(Index);
  auto NewSize = Ent.Target->getSize();
  if (Ent.Size == NewSize) {
    return;
  }
  Ent.Size = NewSize;
  // the size increased, so it must be > 0
  assert(Ent.Size > 0);
  // 2. Update the size of the entry that contains the index.
  for (auto I = Index + 1; I < Agg.Values.size();) {
    auto &Next = Agg.Values.at(I);
    if (Next.Start < Ent.Start + Ent.Size) {
      // in the range
      Ent.Target->unify(*Next.Target, Next.Start - Ent.Start);
      eraseEntry(I);
    } else {
      break;
    }
  }
}

PNINode::PNINode(StorageShapeGraph &SSG)
    : node_with_erase(SSG), Id(Parent->getNextId()) {}

SSGNode::SSGNode(StorageShapeGraph &SSG, unsigned int size)
    : Parent(&SSG), Id(Parent->getNextId()), Size(size) {
  auto PNIVar = SSG.createPNINode();
  Ty = SSGValue{.PNIVar = PNIVar};
  Parent->PNIToNode[PNIVar].insert(this);
}

/// Move constructor for makeAggregate
SSGNode::SSGNode(SSGNode &&Other)
    : Parent(Other.Parent), Id(Parent->getNextId()), Size(Other.Size),
      Ty(Other.Ty) {
  if (!isAggregate()) {
    auto Num = Parent->PNIToNode[getPNVar()].erase(&Other);
    assert(Num == 1);
    Parent->PNIToNode[getPNVar()].insert(this);
    Other.Ty = SSGAggregate();
    Other.Size = 0;
  }
}

void SSGNode::makeAggregate() {
  assert(!isAggregate());
  auto Size = this->Size;
  auto Target = Parent->moveNonAgg(*this);
  assert(isAggregate());
  auto &Agg = getAggregate();
  assert(Target->getSize() == Size);
  // TODO wrap the following to a function?
  Agg.Values.push_back({0, Size, Target});
  this->Size = Size;
  // maintain the reverse link
  Target->AggregateUsers.insert(this);
}

bool isUnknown(const PNINode *N) { return N->getPtrOrNum() == Unknown; }
bool isUnknown(const SSGNode *N) {
  return N->getPNVar()->getPtrOrNum() == Unknown;
}

} // namespace notdec::retypd
