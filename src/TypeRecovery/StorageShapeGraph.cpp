#include "TypeRecovery/StorageShapeGraph.h"
#include <cassert>
#include <cctype>
#include <queue>

namespace notdec::retypd {

unsigned long StorageShapeGraph::PNVarCounter = 3;

void StorageShapeGraph::solve() {
  // write a worklist algorithm.
  // When a unknown var changed to known, add all constraints that use this var.
  // (maintain a map from var to constraints, maintain when two unknown vars
  // merge)
  // When two unknown vars merge, add all constraints that use these two vars.
  std::queue<PNIConsNode *> Worklist;
  for (auto &C : Constraints) {
    Worklist.push(&C);
  }
  while (!Worklist.empty()) {
    PNIConsNode *C = Worklist.front();
    Worklist.pop();
    auto Changed = C->solve();
    if (C->isFullySolved()) {
      continue;
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
      bool IsChanged = Right->setPNVar(NonPtr);
      if (IsChanged) {
        return {Right};
      }

    } else {
      bool IsChanged = Left->setPNVar(NonPtr);
      if (IsChanged) {
        return {Left};
      }
    }
  }
}
llvm::SmallVector<SSGNode *, 3> SubNodeCons::solve() {}

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
        bool IsChanged = Arr[i]->setPNVar(ToUnify);
        // Update changed list accordingly.
        if (IsChanged) {
          Changed.push_back(Arr[i]);
        }
      }
    }
    return Changed;
  }

  // 2. check using alias relation
  // Left alias right: Must be number
  // this includes the Left == Right == Result case.
  unsigned unknownCount = 0;
  for (SSGNode *N : {Left, Right, Result}) {
    if (isUnknown(N)) {
      unknownCount++;
    }
  }
  assert(unknownCount >= 2);
  if (Left->getPNVar() == Right->getPNVar()) {
    // Must be unknown because there are at least 2 unknowns.
    assert(isUnknown(Left));
    bool IsChanged = Left->setPNVar(NonPtr);
    if (IsChanged) {
      Changed.push_back(Left);
    }
    IsChanged = Right->setPNVar(NonPtr);
    if (IsChanged) {
      Changed.push_back(Right);
    }
    IsChanged = Result->setPNVar(NonPtr);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  }
  // Left != Right.
  else if (Left->getPNVar() == Result->getPNVar()) {
    // If Left == Result, Right must be number
    bool IsChanged = Right->setPNVar(NonPtr);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else if (Right == Result) {
    // same as above
    bool IsChanged = Left->setPNVar(NonPtr);
    if (IsChanged) {
      Changed.push_back(Left);
    }
  } else {
    // no alias at all. try to check merge rules.
    if (unknownCount < 3) {
      assert(unknownCount == 2);
      // 1. Unknown + Number = Unknown
      //    or Number + Unknown = Unknown
      if (Left->isNonPtr()) {
        // Unify Right and Result
      } else if (Right->isNonPtr()) {
        // Unify Left and Result
      } else if (Result->isPointer()) {
        // 2. Unknown + Unknown = Pointer
        // add Left != Right constraint
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
  Parent->PNVarToNode[PNVar].erase(this);
  auto &List =
      getParent()->*(StorageShapeGraph::getSublistAccess((SSGNode *)nullptr));
  return List.erase(getIterator());
}

// llvm::iplist<SSGLink>::iterator SSGLink::eraseFromParent() {
//   auto &List =
//       getParent()->*(StorageShapeGraph::getSublistAccess((SSGLink
//       *)nullptr));
//   return List.erase(getIterator());
// }

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

bool SSGNode::setPNVar(PNVarTy Ty) {
  assert(Ty != Unknown);
  if (PNVar == Ty) {
    return false;
  }
  if ((getPNTy() == NonPtr && Ty == Pointer) ||
      (getPNTy() == Pointer && Ty == NonPtr)) {
    std::cerr << "Warning: SSGNode::unifyPNVar: Pointer and NonPtr merge\n";
    if (getPNTy() == NonPtr && Ty == Pointer) {
      PNVar = Pointer;
      return true;
    } else {
      return false;
    }
  }

  if (isUnknown(this)) {
    Parent->mergePNVar(PNVar, Ty);
    // should be set by mergePNVar
    assert(PNVar == Ty);
    return true;
  } else if (isUnknown(Ty)) {
    Parent->mergePNVar(Ty, PNVar);
    return true;
  } else {
    assert(false && "SSGNode::setPNVar: unreachable case");
  }
}

PNVarTy SSGNode::unifyPN(SSGNode &other) {
  // fully merge PNVar
  setPNVar(other.PNVar);
  other.setPNVar(this->PNVar);
  assert(PNVar == other.PNVar);
  return PNVar;
}

SSGNode::SSGNode(StorageShapeGraph &SSG, unsigned long PNVar)
    : Parent(&SSG), PNVar(PNVar) {
  assert(PNVar != 0);
  if (isUnknown(PNVar)) {
    Parent->PNVarToNode[PNVar].insert(this);
  }
}

bool isUnknown(SSGNode *N) {
  assert(N->getPNVar() != 0);
  return isUnknown(N->getPNVar());
}

} // namespace notdec::retypd
