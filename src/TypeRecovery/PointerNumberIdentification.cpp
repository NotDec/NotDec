#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/ConstraintGraph.h"

namespace notdec::retypd {

void PNIGraph::eraseConstraint(ConsNode *Cons) {
  onEraseConstraint(Cons);
  for (auto *N : Cons->getNodes()) {
    assert(N != nullptr);
    PNIToCons[N].erase(Cons);
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
    if (N->isUnknown()) {
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
    if (N->isUnknown()) {
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
    assert(Left->isUnknown());
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

// overload with check
llvm::iplist<PNINode>::iterator PNINode::eraseFromParent() {
  // Check links before destruction. Should be replaced beforehand.
  assert(Parent->PNIToNode.count(this) == 0);
  assert(Parent->PNIToCons.count(this) == 0);
  return node_with_erase<PNINode, PNIGraph>::eraseFromParent();
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

void PNIGraph::addAddCons(CGNode *Left, CGNode *Right, CGNode *Result,
                          llvm::BinaryOperator *Inst) {
  AddNodeCons C = {.Left = Left->getPNIVar(),
                   .Right = Right->getPNIVar(),
                   .Result = Result->getPNIVar(),
                   .Inst = Inst};
  auto *Node = new ConsNode(*this, C);
  PNIToCons[Left->getPNIVar()].insert(Node);
  PNIToCons[Right->getPNIVar()].insert(Node);
  PNIToCons[Result->getPNIVar()].insert(Node);
  Constraints.push_back(Node);
}

void PNIGraph::addSubCons(CGNode *Left, CGNode *Right, CGNode *Result,
                          llvm::BinaryOperator *Inst) {
  SubNodeCons C = {.Left = Left->getPNIVar(),
                   .Right = Right->getPNIVar(),
                   .Result = Result->getPNIVar(),
                   .Inst = Inst};
  auto *Node = new ConsNode(*this, C);
  PNIToCons[Left->getPNIVar()].insert(Node);
  PNIToCons[Right->getPNIVar()].insert(Node);
  PNIToCons[Result->getPNIVar()].insert(Node);
  Constraints.push_back(Node);
}

void PNIGraph::mergePNVarTo(PNINode *Var, PNINode *Target) {
  // maintain PNIToNode
  if (PNIToNode.count(Var)) {
    for (auto *N : PNIToNode[Var]) {
      N->setPNIVar(Target);
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

} // namespace notdec::retypd
