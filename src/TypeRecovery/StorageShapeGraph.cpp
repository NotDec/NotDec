#include "TypeRecovery/StorageShapeGraph.h"
#include <cassert>
#include <cctype>
#include <queue>

namespace notdec::retypd {

unsigned long StorageShapeGraph::PNVarCounter = 1;

void StorageShapeGraph::solve() {
  // write a worklist algorithm.
  // When a unknown var changed to known, add all constraints that use this var.
  // (maintain a map from var to constraints, maintain when two unknown vars
  // merge)
  // When two unknown vars merge, add all constraints that use these two vars.
  std::set<PNIConsNode *> Worklist;
  for (auto &C : Constraints) {
    Worklist.insert(&C);
  }
  while (!Worklist.empty()) {
    PNIConsNode *C = *Worklist.begin();
    Worklist.erase(C);
    auto Changed = C->solve();
    bool isFullySolved = C->isFullySolved();
    // add according to changed.
    if (isFullySolved) {
      C->eraseFromParent();
    }
  }
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

llvm::SmallVector<SSGNode *, 3> NeNodeCons::solve() {
  if (isUnknown(Left) != isUnknown(Right)) {
    if (!isUnknown(Left)) {
      bool IsChanged = Right->setPtrOrNum(inverse(Left->getPNTy()));
      if (IsChanged) {
        return {Right};
      }
    } else {
      assert(!isUnknown(Right));
      bool IsChanged = Left->setPtrOrNum(inverse(Right->getPNTy()));
      if (IsChanged) {
        return {Left};
      }
    }
  }
  return {};
}

llvm::SmallVector<SSGNode *, 3> AddNodeCons::solve() {
  llvm::SmallVector<SSGNode *, 3> Changed;

  // 1. solving using add rules.
  for (const char *Rule : Rules) {
    bool NotMatch = false;
    SSGNode *Arr[3] = {Left, Right, Result};
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
  for (SSGNode *N : {Left, Right, Result}) {
    if (isUnknown(N)) {
      unknownCount++;
    }
  }
  assert(unknownCount >= 2);

  // 2. check using alias relation
  // Left alias right: Must be number
  // this includes the Left == Right == Result case.
  if (Left->getPNVar() == Right->getPNVar()) {
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
  else if (Left->getPNVar() == Result->getPNVar()) {
    // If Left == Result, Right must be number
    bool IsChanged = Right->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else if (Right->getPNVar() == Result->getPNVar()) {
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

llvm::SmallVector<SSGNode *, 3> SubNodeCons::solve() {
  llvm::SmallVector<SSGNode *, 3> Changed;

  // 1. solving using add rules.
  for (const char *Rule : Rules) {
    bool NotMatch = false;
    SSGNode *Arr[3] = {Left, Right, Result};
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
  for (SSGNode *N : {Left, Right, Result}) {
    if (isUnknown(N)) {
      unknownCount++;
    }
  }
  assert(unknownCount >= 2);

  // 2. check using alias relation
  // Right alias Result: Must be number
  // this includes the Left == Right == Result case.
  if (Result->getPNVar() == Right->getPNVar()) {
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
  } else if (Left->getPNVar() == Right->getPNVar()) {
    // If Left == Right, Result must be number
    // Must be unknown because there are at least 2 unknowns.
    assert(isUnknown(Left));
    bool IsChanged = Result->setPtrOrNum(NonPtr);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  } else if (Left->getPNVar() == Result->getPNVar()) {
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

llvm::iplist<SSGNode>::iterator SSGNode::eraseFromParent() {
  // remove self from PNVarToNode map
  Parent->PNIToNode[PNIVar].erase(this);
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
  assert(NewTy == Unknown);
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

SSGNode *SSGNode::unifyPN(SSGNode &other) {
  auto *Node = Parent->mergePNINodes(this->getPNVar(), other.getPNVar());
  if (Node == nullptr) {
    // no need to merge
    return nullptr;
  } else if (Node == this->getPNVar()) {
    return this;
  } else {
    assert(Node == other.getPNVar());
    return &other;
  }
}

SSGNode::SSGNode(StorageShapeGraph &SSG) : Parent(&SSG) {
  PNIVar = SSG.createPNINode();
  Parent->PNIToNode[PNIVar].insert(this);
}

bool isUnknown(SSGNode *N) { return N->getPNVar()->getPtrOrNum() == Unknown; }

} // namespace notdec::retypd
