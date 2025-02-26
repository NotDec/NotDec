#include "TypeRecovery/StorageShapeGraph.h"
#include "notdec-llvm2c/Range.h"
#include "Passes/ConstraintGenerator.h"
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

void SubTypeNodeCons::solve() { Sup->mergeTo(*Sub, Offset); }

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

/// If not Agg: merge two node.
/// If Agg: Mount a parent type node's capability into this node.
/// This node is the parent type of OtherNode. This node can have more
/// capabilities.
/// Example: For `q = p + 4`, run p.mountTo(q, 4).
void SSGNode::mergeTo(SSGNode &OtherNode, OffsetRange OffR) {
  if (!isAggregate()) {
    // merge the primitive type
    assert(OffR.isZero());
    if (getPNTy() != OtherNode.getPNTy()) {
      assert(false && "SSGNode::unify: PN type mismatch");
    }
    if (isPointer()) {

    } else if (isNonPtr()) {
      if (Size != OtherNode.Size) {
        std::cerr << "Warning: SSGNode::mergeTo: size mismatch\n";
      }
      // TODO unify primitive type
      assert(false); // Need to merge outside by modifying SSGLink
      return;
    }
  }

  // handle array later
  assert(OffR.access.size() == 0);
  int64_t Offset = OffR.offset;
  // 1. If offset is not zero, try to find or delegate the subrange for it.
  // Because this and other should have its own type (represented by a
  // SSGNode). Make offset zero first.
  if (Offset != 0) {
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
        Entry.Target->mergeTo(OtherNode, OtherStart - Entry.Start);
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
  // 2. If offset is zero, just insert.
  assert(Offset == 0);
  auto NewEnt = getAggregate().Values.insert(getAggregate().Values.begin(),
                                             {0, 0, &OtherNode});
  // maintain the reverse link
  OtherNode.AggregateUsers.insert(this);
  updateEntSize(NewEnt);
  return;
  // TODO handle below
  // bool hasAgg = isAggregate() || OtherNode.isAggregate();
  // if (!hasAgg) {
  //   unifyPN(OtherNode);
  //   assert(getPNTy() == OtherNode.getPNTy());
  //   if (isNonPtr()) {
  //     assert(Size == OtherNode.Size);
  //   } else {
  //     if (getPointsTo() == nullptr) {
  //       setPointsTo(OtherNode.getPointsTo());
  //       // 2. Update the size/replace the node upward.
  //     } else if (OtherNode.getPointsTo() == nullptr) {
  //       OtherNode.setPointsTo(this->getPointsTo());
  //       // 2. Update the size/replace the node upward.
  //     } else {
  //       getPointsTo()->unify(*OtherNode.getPointsTo());
  //     }
  //   }
  // } else {
  //   auto &Agg = std::get<SSGAggregate>(Ty);
  //   auto &OtherAgg = std::get<SSGAggregate>(OtherNode.Ty);
  //   // merge Other into this. Assume that smaller offset range contains
  //   larger
  //   // offset range.
  //   unsigned Ind1 = 0;
  //   unsigned Ind2 = 0;
  //   while (Ind1 < Agg.Values.size() && Ind2 < OtherAgg.Values.size()) {
  //     auto &Entry = Agg.Values[Ind1];
  //     auto &Other = OtherAgg.Values[Ind2];
  //     auto *EntryNode = Entry.Target;
  //     auto EntryEnd = Entry.Start + Entry.Size;
  //     auto *OtherNode = Other.Target;
  //     auto OtherEnd = Other.Start + Other.Size;

  //     // Check according to the location of Start index
  //     if (EntryEnd <= Other.Start) {
  //       // Case1: EntryStart < EntryEnd <= OtherStart
  //       // OtherStart out of range of Entry
  //       // Entry is at the left of Other.
  //       Ind1++;
  //       continue;
  //     } else if (OtherEnd <= Entry.Start) {
  //       // Case2: OtherStart < OtherEnd <= EntryStart
  //       // Other is at the left of Entry.
  //       Agg.Values.insert(Agg.Values.begin() + Ind1, Other);
  //       OtherNode->AggregateUsers.insert(this);
  //       Ind1++;
  //       Ind2++;
  //       continue;
  //     } else if (Entry.Start <= Other.Start && Other.Start < EntryEnd) {
  //       // Case3: OtherStart in Entry
  //       EntryNode->mountTo(*OtherNode, Other.Start - Entry.Start);
  //       Ind2++;
  //     } else if (Other.Start <= Entry.Start && Entry.Start < OtherEnd) {
  //       // Case4: EntryStart in Other
  //       // merge into other
  //       OtherNode->mountTo(*EntryNode, Entry.Start - Other.Start);
  //       Agg.Values.at(Ind1) = {Other.Start, OtherNode->getSize(), OtherNode};
  //       Ind2++;
  //     } else {
  //       assert(false && "SSGNode::unify: should be unreachable");
  //     }
  //   }
  // }
  // Parent->replaceSSGNodes(&OtherNode, this);
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
      Ent.Target->mergeTo(*Next.Target, Next.Start - Ent.Start);
      eraseEntry(I);
    } else {
      break;
    }
  }
}

void SSGNode::delegateToOffset(SSGNode &Target, int64_t Offset) {
  auto &Agg = getPointsToAgg();
  auto NewEnt = Agg.Values.insert(Agg.Values.begin(), {0, 0, &Target});
  // maintain the reverse link
  Target.AggregateUsers.insert(this);
  getPointsTo()->updateEntSize(NewEnt);
}

SSGAggregate &SSGNode::getPointsToAgg() {
  assert(isPointer());
  if (getPointsTo() == nullptr) {
    setPointsTo(Parent->createAgg());
    if (isIntConstant()) {
      // mount to memory
    }
  }
  return getPointsTo()->getAggregate();
}

SSGNode::SSGNode(StorageShapeGraph &SSG, unsigned int size, bool isAgg)
    : Parent(&SSG), Id(Parent->getNextId()), Size(size) {
  if (isAgg) {
    Ty = SSGAggregate();
  } else {
    auto PNIVar = SSG.createPNINode();
    Ty = SSGValue{.PNIVar = PNIVar};
    Parent->PNIToNode[PNIVar].insert(this);
  }
}

/// Move constructor for makeAggregate
// SSGNode::SSGNode(SSGNode &&Other)
//     : Parent(Other.Parent), Id(Parent->getNextId()), Size(Other.Size),
//       Ty(Other.Ty) {
//   if (!isAggregate()) {
//     auto Num = Parent->PNIToNode[getPNVar()].erase(&Other);
//     assert(Num == 1);
//     Parent->PNIToNode[getPNVar()].insert(this);
//     Other.Ty = SSGAggregate();
//     Other.Size = 0;
//   }
// }

// void SSGNode::makeAggregate() {
//   assert(!isAggregate());
//   auto Size = this->Size;
//   auto Target = Parent->moveNonAgg(*this);
//   assert(isAggregate());
//   auto &Agg = getAggregate();
//   assert(Target->getSize() == Size);
//   // TODO wrap the following to a function?
//   Agg.Values.push_back({0, Size, Target});
//   this->Size = Size;
//   // maintain the reverse link
//   Target->AggregateUsers.insert(this);
// }
bool isUnknown(const SSGNode *N) {
  return N->getPNVar()->getPtrOrNum() == Unknown;
}
} // namespace notdec::retypd
