#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/ConstraintGraph.h"
#include "optimizers/ConstraintGenerator.h"
#include <cassert>
#include <llvm/IR/InstrTypes.h>

namespace notdec::retypd {

/// Visit Add/Mul/shl chain, add the results to OffsetRange.
OffsetRange matchOffsetRange(llvm::Value *I) {
  using namespace llvm;
  assert(I->getType()->isIntegerTy());
  if (auto *CI = dyn_cast<llvm::ConstantInt>(I)) {
    return OffsetRange{.offset = CI->getSExtValue()};
  }
  // unknown value = 1*x
  if (!isa<llvm::BinaryOperator>(I)) {
    return OffsetRange{.offset = 0, .access = {{1, 0}}};
  } else {
    auto Opcode = cast<BinaryOperator>(I)->getOpcode();
    if (Opcode != Instruction::Add && Opcode != Instruction::Mul &&
        Opcode != Instruction::Shl) {
      return OffsetRange{.offset = 0, .access = {{1, 0}}};
    }
  }
  // is Add, Mul, Shl
  auto *BinOp = cast<llvm::BinaryOperator>(I);
  auto *Src1 = BinOp->getOperand(0);
  auto *Src2 = BinOp->getOperand(1);
  if (isa<ConstantInt>(Src1) && isa<ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (isa<ConstantInt>(Src1) && !isa<ConstantInt>(Src2)) {
    // because of InstCombine canonical form, this should not happen?
    assert(false &&
           "Constant cannot be at the left side. Run InstCombine first.");
    std::swap(Src1, Src2);
  }
  // check if add or mul
  if (BinOp->getOpcode() == llvm::Instruction::Add) {
    return matchOffsetRange(Src1) + matchOffsetRange(Src2);
  } else if (BinOp->getOpcode() == llvm::Instruction::Mul) {
    return matchOffsetRange(Src1) * matchOffsetRange(Src2);
  } else if (BinOp->getOpcode() == llvm::Instruction::Shl &&
             llvm::isa<ConstantInt>(Src2)) {
    return matchOffsetRange(Src1) *
           (1 << llvm::cast<ConstantInt>(Src2)->getSExtValue());
  } else {
    return OffsetRange{.offset = 0, .access = {{1, 0}}};
  }
}

bool PNIGraph::solve() {
  bool AnyChanged = false;
  while (!Worklist.empty()) {
    ConsNode *C = *Worklist.begin();
    Worklist.erase(C);
    auto Changed = C->solve();
    bool isFullySolved = C->isFullySolved();
    // add according to changed.
    for (auto *N : Changed) {
      AnyChanged = true;
      markChanged(N, C);
    }
    // remove the constraint if fully solved.
    if (isFullySolved) {
      eraseConstraint(C);
      C = nullptr;
    }
  }
  return AnyChanged;
}

void PNIGraph::eraseConstraint(ConsNode *Cons) {
  // Check if the constraint should be converted
  if (Cons->isAdd()) {
    auto [Left, Right, Result] = Cons->getNodes();
    auto BinOp = const_cast<llvm::BinaryOperator *>(Cons->getInst());
    auto *LeftVal = BinOp->getOperand(0);
    auto *RightVal = BinOp->getOperand(1);
    OffsetRange Off;
    if (Left->getPNIVar()->getPtrOrNum() == retypd::NonPtr &&
        Right->getPNIVar()->getPtrOrNum() == retypd::Pointer) {
      Off = matchOffsetRange(LeftVal);
      Result->setAsPtrAdd(Right, Off);
    } else if (Left->getPNIVar()->getPtrOrNum() == retypd::Pointer &&
               Right->getPNIVar()->getPtrOrNum() == retypd::NonPtr) {
      Off = matchOffsetRange(RightVal);
      Result->setAsPtrAdd(Left, Off);
    }
  }
  for (auto *N : Cons->getNodes()) {
    assert(N != nullptr);
    NodeToCons[N].erase(Cons);
  }

  if (Worklist.count(Cons)) {
    Worklist.erase(Cons);
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

bool AddNodeCons::isFullySolved() {
  PNINode *Left = this->LeftNode->getPNIVar();
  PNINode *Right = this->RightNode->getPNIVar();
  PNINode *Result = this->ResultNode->getPNIVar();
  return !Left->isUnknown() && !Right->isUnknown() && !Result->isUnknown();
}

bool SubNodeCons::isFullySolved() {
  PNINode *Left = this->LeftNode->getPNIVar();
  PNINode *Right = this->RightNode->getPNIVar();
  PNINode *Result = this->ResultNode->getPNIVar();
  return !Left->isUnknown() && !Right->isUnknown() && !Result->isUnknown();
}

llvm::SmallVector<PNINode *, 3> AddNodeCons::solve() {
  PNINode *Left = this->LeftNode->getPNIVar();
  PNINode *Right = this->RightNode->getPNIVar();
  PNINode *Result = this->ResultNode->getPNIVar();
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
        PtrOrNum PTy = fromIPChar(Rule[i]);
        bool IsChanged = Arr[i]->setPtrOrNum(PTy);
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
        Changed.push_back(Merged);
      } else if (Right->isNonPtr()) {
        // Unify Left and Result
        auto *Merged = Left->unifyPN(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
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
  PNINode *Left = this->LeftNode->getPNIVar();
  PNINode *Right = this->RightNode->getPNIVar();
  PNINode *Result = this->ResultNode->getPNIVar();
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
        Changed.push_back(Merged);
      } else if (Result->isNonPtr()) {
        // Unify Left and Right
        auto *Merged = Left->unifyPN(*Right);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
      } else if (Left->isPointer()) {
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
  return node_with_erase<PNINode, PNIGraph>::eraseFromParent();
}

// Notify CGNode that it becomes a pointer.
void PNIGraph::onUpdatePNType(PNINode *N) {
  if (PNIToNode.count(N) > 0) {
    for (auto *Node : PNIToNode[N]) {
      Node->onUpdatePNType();
    }
  }
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
      Parent->onUpdatePNType(this);
      return true;
    } else {
      return false;
    }
  }
  assert(Ty == Unknown);
  Ty = NewTy;
  Parent->onUpdatePNType(this);
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
  AddNodeCons C = {
      .LeftNode = Left, .RightNode = Right, .ResultNode = Result, .Inst = Inst};
  auto *Node = new ConsNode(*this, C);
  NodeToCons[Left].insert(Node);
  NodeToCons[Right].insert(Node);
  NodeToCons[Result].insert(Node);
  Constraints.push_back(Node);
  Worklist.insert(Node);
}

void PNIGraph::addSubCons(CGNode *Left, CGNode *Right, CGNode *Result,
                          llvm::BinaryOperator *Inst) {
  SubNodeCons C = {
      .LeftNode = Left, .RightNode = Right, .ResultNode = Result, .Inst = Inst};
  auto *Node = new ConsNode(*this, C);
  NodeToCons[Left].insert(Node);
  NodeToCons[Right].insert(Node);
  NodeToCons[Result].insert(Node);
  Constraints.push_back(Node);
  Worklist.insert(Node);
}

void PNIGraph::markChanged(PNINode *N, ConsNode *Except) {
  // worklist algorithm.
  // When a var changed, add all constraints that use this var.
  for (auto *N2 : PNIToNode[N]) {
    if (NodeToCons.count(N2)) {
      for (auto *C2 : NodeToCons[N2]) {
        if (C2 == Except) {
          continue;
        }
        Worklist.insert(C2);
      }
    }
  }
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
  Var->eraseFromParent();
  // Target is Changed, add related cons to worklist
  markChanged(Target);
}
void PNINode::addUser(CGNode *Node) { Parent->PNIToNode[this].insert(Node); }

PNINode::PNINode(PNIGraph &SSG)
    : node_with_erase(SSG), Id(ValueNamer::getId()) {}

void PNINode::cloneFrom(const PNINode &N) {
  Ty = N.Ty;
  hasConflict = N.hasConflict;
  Id = ValueNamer::getId();
}

void PNIGraph::cloneFrom(const PNIGraph &G,
                         std::map<const CGNode *, CGNode *> Old2New) {
  assert(PNINodes.size() == 0);
  assert(Constraints.size() == 0);
  // clone PNINodes
  for (auto &N : G.PNINodes) {
    auto *NewNode = new PNINode(*this);
    NewNode->cloneFrom(N);
    PNINodes.push_back(NewNode);
    // maintain PNIToNode
    for (const auto *Node : G.PNIToNode.at(const_cast<PNINode *>(&N))) {
      Old2New[Node]->setPNIVar(NewNode);
      PNIToNode[NewNode].insert(Old2New[Node]);
    }
  }
  // clone Constraints
  for (auto &C : Constraints) {
    auto *NewNode = new ConsNode(*this, AddNodeCons{});
    NewNode->cloneFrom(C, Old2New);
    Constraints.push_back(NewNode);
    NodeToCons[NewNode->getNodes()[0]].insert(NewNode);
    NodeToCons[NewNode->getNodes()[1]].insert(NewNode);
    NodeToCons[NewNode->getNodes()[2]].insert(NewNode);
    if (G.Worklist.count(&C)) {
      Worklist.insert(NewNode);
    }
  }
}

} // namespace notdec::retypd
