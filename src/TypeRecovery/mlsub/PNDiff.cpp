#include "TypeRecovery/mlsub/PNDiff.h"
// #include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "TypeRecovery/mlsub/MLsubGraph.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include <cassert>
#include <iostream>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Type.h>
#include <string>
#include <utility>
#include <vector>

namespace notdec::mlsub {

using retypd::fromIPChar;

// remove and ignore all negative access.
OffsetRange matchOffsetRangeNoNegativeAccess(llvm::Value *I) {
  auto R = matchOffsetRange(I);
  R.access.erase(std::remove_if(R.access.begin(), R.access.end(),
                                [](ArrayOffset Val) { return Val.Size < 0; }),
                 R.access.end());
  return R;
}

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
  assert(false && "TODO");
  // CG.applyPNIPolicy();

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
    if (getPNIVar(Left).getPtrOrNum() == retypd::Number &&
        getPNIVar(Right).getPtrOrNum() == retypd::Pointer) {
      Off = matchOffsetRangeNoNegativeAccess(LeftVal);
      assert(false && "TODO");
      // Result->setAsPtrAdd(*Right, Off);
    } else if (getPNIVar(Left).getPtrOrNum() == retypd::Pointer &&
               getPNIVar(Right).getPtrOrNum() == retypd::Number) {
      Off = matchOffsetRangeNoNegativeAccess(RightVal);
      assert(false && "TODO");
      // Result->setAsPtrAdd(*Left, Off);
    }
  }
  for (auto N : Cons->getNodes()) {
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

bool AddNodeCons::isFullySolved(PNIGraph &G) {
  PNINode *Left = &G.getPNIVar(LeftNode);
  PNINode *Right = &G.getPNIVar(RightNode);
  PNINode *Result = &G.getPNIVar(ResultNode);
  return !Left->isUnknown() && !Right->isUnknown() && !Result->isUnknown();
}

bool SubNodeCons::isFullySolved(PNIGraph &G) {
  PNINode *Left = &G.getPNIVar(LeftNode);
  PNINode *Right = &G.getPNIVar(RightNode);
  PNINode *Result = &G.getPNIVar(ResultNode);
  return !Left->isUnknown() && !Right->isUnknown() && !Result->isUnknown();
}

llvm::SmallVector<PNINode *, 3> AddNodeCons::solve(PNIGraph &G) {
  PNINode *Left = &G.getPNIVar(LeftNode);
  PNINode *Right = &G.getPNIVar(RightNode);
  PNINode *Result = &G.getPNIVar(ResultNode);
  assert(Left->isPNRelated());
  assert(Right->isPNRelated());
  assert(Result->isPNRelated());
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
    bool IsChanged = Left->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Left);
    }
    IsChanged = Right->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Right);
    }
    IsChanged = Result->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  }
  // Left != Right.
  else if (Left == Result) {
    // If Left == Result, Right must be number
    bool IsChanged = Right->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else if (Right == Result) {
    // same as above
    bool IsChanged = Left->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Left);
    }
  } else {
    // no alias at all. try to check merge rules.
    if (unknownCount < 3) {
      assert(unknownCount == 2);
      // 1. Unknown + Number = SameUnknown
      //    or Number + Unknown = SameUnknown
      if (Left->isNumber()) {
        // Unify Right and Result
        auto *Merged = Right->unify(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
      } else if (Right->isNumber()) {
        // Unify Left and Result
        auto *Merged = Left->unify(*Result);
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

llvm::SmallVector<PNINode *, 3> SubNodeCons::solve(PNIGraph &G) {
  PNINode *Left = &G.getPNIVar(LeftNode);
  PNINode *Right = &G.getPNIVar(RightNode);
  PNINode *Result = &G.getPNIVar(ResultNode);
  assert(Left->isPNRelated());
  assert(Right->isPNRelated());
  assert(Result->isPNRelated());
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
    bool IsChanged = Left->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Left);
    }
    IsChanged = Right->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Right);
    }
    IsChanged = Result->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  } else if (Left == Right) {
    // If Left == Right, Result must be number
    // Must be unknown because there are at least 2 unknowns.
    assert(Left->isUnknown());
    bool IsChanged = Result->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  } else if (Left == Result) {
    // same as above
    bool IsChanged = Right->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else {
    // no alias at all. try to check merge rules.
    if (unknownCount < 3) {
      assert(unknownCount == 2);
      // 1. Unknown - Number = SameUnknown
      //    or Unknown - SameUnknown = Number
      if (Right->isNumber()) {
        // Unify Left and Result
        auto *Merged = Left->unify(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
      } else if (Result->isNumber()) {
        // Unify Left and Right
        auto *Merged = Left->unify(*Right);
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

ConsNode::iteratorTy ConsNode::eraseFromParent() {
  return Parent.Constraints.erase(getIterator());
}

// overload with check
PNINode::iteratorTy PNINode::eraseFromParent() {
  // Check links before destruction. Should be replaced beforehand.
  assert(Parent.PNIMap.count(this) == 0);
  if (TraceIds.count(getId())) {
    llvm::errs() << "TraceID=" << getId() << " PNINode=" << str()
                 << ": PNINode::eraseFromParent: Erasing node\n";
  }
  return Parent.PNINodes.erase(getIterator());
}

// Notify Node that it becomes a pointer.
void PNIGraph::onUpdatePNType(PNINode *N) {
  if (PNIMap.count(N) > 0) {
    for (auto Node : PNIMap.rev().at(N)) {
      assert(false && "TODO");
      // Node->onUpdatePNType();
    }
  }
}

llvm::Type *PNINode::mergeLowTy(llvm::Type *T, llvm::Type *O) {
  if (T == nullptr) {
    return O;
  } else if (O == nullptr) {
    return T;
  } else {
    assert(T == O);
    return T;
  }
}

PNINode *PNINode::unify(PNINode &other) {
  assert(&Parent == &other.Parent);
  if (this == &other) {
    return this;
  }
  auto *Node = Parent.mergePNINodes(this, &other);
  return Node;
}

void PNIGraph::addAddCons(ExtValuePtr Left, ExtValuePtr Right, ExtValuePtr Result,
                          llvm::BinaryOperator *Inst) {
  assert(getPNIVar(Left).isPNRelated());
  assert(getPNIVar(Right).isPNRelated());
  assert(getPNIVar(Result).isPNRelated());
  AddNodeCons C = {
      .LeftNode = Left, .RightNode = Right, .ResultNode = Result, .Inst = Inst};
  auto &Node = Constraints.emplace_back(*this, C);
  NodeToCons[Left].insert(&Node);
  NodeToCons[Right].insert(&Node);
  NodeToCons[Result].insert(&Node);
  Worklist.insert(&Node);
}

void PNIGraph::addSubCons(ExtValuePtr Left, ExtValuePtr Right, ExtValuePtr Result,
                          llvm::BinaryOperator *Inst) {
  assert(getPNIVar(Left).isPNRelated());
  assert(getPNIVar(Right).isPNRelated());
  assert(getPNIVar(Result).isPNRelated());
  SubNodeCons C = {
      .LeftNode = Left, .RightNode = Right, .ResultNode = Result, .Inst = Inst};
  auto &Node = Constraints.emplace_back(*this, C);
  NodeToCons[Left].insert(&Node);
  NodeToCons[Right].insert(&Node);
  NodeToCons[Result].insert(&Node);
  Worklist.insert(&Node);
}

void PNIGraph::markChanged(PNINode *N, ConsNode *Except) {
  // worklist algorithm.
  // When a var changed, add all constraints that use this var.
  for (auto N2 : PNIMap.rev().at(N)) {
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
  assert(Var->getSize() == Target->getSize());
  assert(&Var->Parent == &Target->Parent);
  if (Var == Target) {
    return;
  }
  // maintain PNIMap
  PNIMap.merge(Var, Target);
  Var->eraseFromParent();
  // Target is Changed, add related cons to worklist
  markChanged(Target);
}

// void PNINode::addUser(ExtValuePtr Node) {
//   Parent.PNIMap.insert(Node, this);
// }

bool PNINode::setPtrOrNum(PtrOrNum NewTy) {
  bool Updated = Ty.setPtrOrNum(NewTy);
  if (Updated) {
    Parent.onUpdatePNType(this);
  }
  return Updated;
}

// When LowTy is pointer-sized int, we initialize Ty as Unknown.
PNINode::PNINode(PNIGraph &SSG, llvm::Type *LowTy)
    : Parent(SSG), Id(ValueNamer::getId()), Ty(LowTy, SSG.PointerSize) {
  if (TraceIds.count(Id)) {
    std::cerr << "PNINode::PNINode(" << Id << "): " << str() << "\n";
  }
}

PNINode::PNINode(PNIGraph &SSG, const PNINode &OtherGraphNode)
    : Parent(SSG), Id(ValueNamer::getId()), Ty(OtherGraphNode.Ty) {}

PNINode::PNINode(PNIGraph &SSG, std::string SerializedTy)
    : Parent(SSG), Id(ValueNamer::getId()),
      Ty(SerializedTy.substr(0, SerializedTy.find(" ")), ({
           auto Pos = SerializedTy.find(" ");
           unsigned long Size;
           if (SerializedTy.substr(Pos + 1) == "p") {
             Size = SSG.PointerSize;
           } else {
             Size = std::stoul(SerializedTy.substr(Pos + 1));
           }
           Size;
         })) {}

} // namespace notdec::mlsub
