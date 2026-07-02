#include "TypeRecovery/mlsub/PNDiff.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace notdec::mlsub {

using retypd::fromIPChar;

namespace {

std::string formatPNINodeForTrace(const PNINode &Node) {
  return Node.serialize();
}

char formatPtrOrNumChar(PtrOrNum Ty) {
  switch (Ty) {
  case Pointer:
    return 'p';
  case Number:
    return 'i';
  case Null:
    return '0';
  case NotPN:
    return 'x';
  case Unknown:
    return '?';
  }
  return '!';
}

std::string formatConstraintForTrace(PNIGraph &G, const char *Kind,
                                     PNIValue Left, PNIValue Right,
                                     PNIValue Result,
                                     PNIInstruction Inst) {
  std::ostringstream OS;
  OS << "kind=" << Kind << " inst=\"" << G.formatInstruction(Inst) << "\""
     << " op0=" << G.formatValue(Left) << "["
     << formatPNINodeForTrace(G.getPNIVar(Left)) << "]"
     << " op1=" << G.formatValue(Right) << "["
     << formatPNINodeForTrace(G.getPNIVar(Right)) << "]"
     << " result=" << G.formatValue(Result) << "["
     << formatPNINodeForTrace(G.getPNIVar(Result)) << "]";
  return OS.str();
}

std::string formatConstraintForTrace(PNIGraph &G, const ConsNode &Cons) {
  auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
  return formatConstraintForTrace(G, Cons.isAdd() ? "add" : "sub", Nodes[0],
                                  Nodes[1], Nodes[2], Cons.getInst());
}

void traceConstraintEvent(PNIGraph &G, const std::string &Event,
                          const std::string &Detail) {
  if (!G.traceEnabled()) {
    return;
  }
  G.trace("[pndiff:" + Event + "] " + Detail);
}

} // namespace

std::string PNIGraph::formatValue(PNIValue Val) const {
  if (FormatValue) {
    return FormatValue(Val);
  }
  std::ostringstream OS;
  OS << Val;
  return OS.str();
}

std::string PNIGraph::formatInstruction(PNIInstruction Inst) const {
  if (FormatInstruction) {
    return FormatInstruction(Inst);
  }
  if (Inst == nullptr) {
    return "<null-inst>";
  }
  std::ostringstream OS;
  OS << Inst;
  return OS.str();
}

bool PNIGraph::solve() {
  trace("[pndiff:solve-begin] scc=" + Name +
        " worklist=" + std::to_string(Worklist.size()) +
        " constraints=" + std::to_string(Constraints.size()));
  bool AnyChanged = applyPNIPolicy();
  while (!Worklist.empty()) {
    ConsNode *C = *Worklist.begin();
    Worklist.erase(C);
    traceConstraintEvent(*this, "worklist-pop",
                         formatConstraintForTrace(*this, *C));
    auto Changed = C->solve();
    bool isFullySolved = C->isFullySolved();
    for (auto *N : Changed) {
      AnyChanged = true;
      markChanged(N, C);
    }
    if (isFullySolved) {
      eraseConstraint(C);
      C = nullptr;
    }
  }
  trace("[pndiff:solve-end] scc=" + Name +
        " changed=" + std::string(AnyChanged ? "true" : "false") +
        " remaining_constraints=" + std::to_string(Constraints.size()));
  return AnyChanged;
}

bool PNIGraph::applyPNIPolicy() {
  if (Policy.nonPointerAbsLt <= 0 || !GetIntConstant) {
    return false;
  }

  bool AnyChanged = false;
  for (const auto &Ent : PNIMap.rev()) {
    auto *Node = Ent.first;
    if (!Node->isUnknown()) {
      continue;
    }
    for (const auto &Val : Ent.second) {
      auto ConstantValue = GetIntConstant(Val);
      if (!ConstantValue) {
        continue;
      }
      if (Policy.excludeZero && *ConstantValue == 0) {
        continue;
      }
      if (std::llabs(*ConstantValue) < Policy.nonPointerAbsLt) {
        bool Changed = Node->setNonPtr();
        AnyChanged |= Changed;
        if (Changed) {
          trace("[pndiff:policy:set-nonptr] node=" +
                formatPNINodeForTrace(*Node) + " value=" + formatValue(Val) +
                " constant=" + std::to_string(*ConstantValue));
        }
        break;
      }
    }
  }
  return AnyChanged;
}

void PNIGraph::eraseConstraint(ConsNode *Cons) {
  traceConstraintEvent(*this, "constraint-solved",
                       formatConstraintForTrace(*this, *Cons));
  if (Cons->isAdd() && MatchOffsetRange && OnPtrAdd) {
    auto [Left, Right, Result] = Cons->getNodes();
    if (getPNIVar(Left).getPtrOrNum() == retypd::Number &&
        getPNIVar(Right).getPtrOrNum() == retypd::Pointer) {
      auto Off = MatchOffsetRange(Left);
      if (Off) {
        trace("[pndiff:ptradd-reify] base=" + formatValue(Right) +
              " result=" + formatValue(Result) + " offset=" + Off->str());
        OnPtrAdd(Right, Result, *Off);
      }
    } else if (getPNIVar(Left).getPtrOrNum() == retypd::Pointer &&
               getPNIVar(Right).getPtrOrNum() == retypd::Number) {
      auto Off = MatchOffsetRange(Right);
      if (Off) {
        trace("[pndiff:ptradd-reify] base=" + formatValue(Left) +
              " result=" + formatValue(Result) + " offset=" + Off->str());
        OnPtrAdd(Left, Result, *Off);
      }
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
    {'p', 'P', 'i'}, {'p', 'i', 'P'}, {'p', 'I', 'p'}, {'P', 'I', 'p'}};

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

PNChangedNodes AddNodeCons::solve(PNIGraph &G) {
  PNINode *Left = &G.getPNIVar(LeftNode);
  PNINode *Right = &G.getPNIVar(RightNode);
  PNINode *Result = &G.getPNIVar(ResultNode);
  assert(Left->isPNRelated());
  assert(Right->isPNRelated());
  assert(Result->isPNRelated());
  PNChangedNodes Changed;

  // 1. solving using add rules.
  for (const char *Rule : Rules) {
    bool NotMatch = false;
    PNINode *Arr[3] = {Left, Right, Result};
    for (unsigned i = 0; i < 3; i++) {
      if (std::islower(Rule[i]) && Rule[i] != Arr[i]->getPNChar()) {
        NotMatch = true;
        break;
      }
    }
    if (NotMatch) {
      continue;
    }
    traceConstraintEvent(
        G, "add:rule",
        formatConstraintForTrace(G, "add", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " rule=" + std::string(Rule, Rule + 3));
    for (unsigned i = 0; i < 3; i++) {
      if (std::isupper(Rule[i])) {
        PtrOrNum PTy = fromIPChar(Rule[i]);
        bool IsChanged = Arr[i]->setPtrOrNum(PTy);
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

  // 2. check using alias relation.
  if (Left == Right) {
    traceConstraintEvent(
        G, "add:alias",
        formatConstraintForTrace(G, "add", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " reason=left-equals-right");
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
  } else if (Left == Result) {
    traceConstraintEvent(
        G, "add:alias",
        formatConstraintForTrace(G, "add", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " reason=left-equals-result");
    bool IsChanged = Right->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else if (Right == Result) {
    traceConstraintEvent(
        G, "add:alias",
        formatConstraintForTrace(G, "add", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " reason=right-equals-result");
    bool IsChanged = Left->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Left);
    }
  } else {
    if (unknownCount < 3) {
      assert(unknownCount == 2);
      if (Left->isNumber()) {
        traceConstraintEvent(
            G, "add:unify",
            formatConstraintForTrace(G, "add", LeftNode, RightNode, ResultNode,
                                     Inst) +
                " reason=left-is-number unify=op1,result");
        auto *Merged = Right->unify(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
      } else if (Right->isNumber()) {
        traceConstraintEvent(
            G, "add:unify",
            formatConstraintForTrace(G, "add", LeftNode, RightNode, ResultNode,
                                     Inst) +
                " reason=right-is-number unify=op0,result");
        auto *Merged = Left->unify(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
      } else if (Result->isPointer()) {
        // Unknown + Unknown = Pointer. Nothing useful to add here.
      } else {
        assert(false && "Should not reach here");
      }
    }
  }
  return Changed;
}

PNChangedNodes SubNodeCons::solve(PNIGraph &G) {
  PNINode *Left = &G.getPNIVar(LeftNode);
  PNINode *Right = &G.getPNIVar(RightNode);
  PNINode *Result = &G.getPNIVar(ResultNode);
  assert(Left->isPNRelated());
  assert(Right->isPNRelated());
  assert(Result->isPNRelated());
  PNChangedNodes Changed;

  // 1. solving using sub rules.
  for (const char *Rule : Rules) {
    bool NotMatch = false;
    PNINode *Arr[3] = {Left, Right, Result};
    for (unsigned i = 0; i < 3; i++) {
      if (std::islower(Rule[i]) && Rule[i] != Arr[i]->getPNChar()) {
        NotMatch = true;
        break;
      }
    }
    if (NotMatch) {
      continue;
    }
    traceConstraintEvent(
        G, "sub:rule",
        formatConstraintForTrace(G, "sub", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " rule=" + std::string(Rule, Rule + 3));
    for (unsigned i = 0; i < 3; i++) {
      if (std::isupper(Rule[i])) {
        PtrOrNum ToUnify = fromIPChar(Rule[i]);
        bool IsChanged = Arr[i]->setPtrOrNum(ToUnify);
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

  if (Result == Right) {
    traceConstraintEvent(
        G, "sub:alias",
        formatConstraintForTrace(G, "sub", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " reason=result-equals-right");
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
    assert(Left->isUnknown());
    traceConstraintEvent(
        G, "sub:alias",
        formatConstraintForTrace(G, "sub", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " reason=left-equals-right");
    bool IsChanged = Result->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Result);
    }
  } else if (Left == Result) {
    traceConstraintEvent(
        G, "sub:alias",
        formatConstraintForTrace(G, "sub", LeftNode, RightNode, ResultNode,
                                 Inst) +
            " reason=left-equals-result");
    bool IsChanged = Right->setPtrOrNum(Number);
    if (IsChanged) {
      Changed.push_back(Right);
    }
  } else {
    if (unknownCount < 3) {
      assert(unknownCount == 2);
      if (Right->isNumber()) {
        traceConstraintEvent(
            G, "sub:unify",
            formatConstraintForTrace(G, "sub", LeftNode, RightNode, ResultNode,
                                     Inst) +
                " reason=right-is-number unify=op0,result");
        auto *Merged = Left->unify(*Result);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
      } else if (Result->isNumber()) {
        traceConstraintEvent(
            G, "sub:unify",
            formatConstraintForTrace(G, "sub", LeftNode, RightNode, ResultNode,
                                     Inst) +
                " reason=result-is-number unify=op0,op1");
        auto *Merged = Left->unify(*Right);
        assert(Merged != nullptr);
        Changed.push_back(Merged);
      } else if (Left->isPointer()) {
        // Pointer - Unknown = Unknown. Nothing useful to add here.
      } else {
        assert(false && "Should not reach here");
      }
    }
  }
  return Changed;
}

ConsNode::iteratorTy ConsNode::eraseFromParent() {
  return Parent.Constraints.erase(getIterator());
}

void PNIGraph::trace(const std::string &message) {
  if (TraceStream == nullptr) {
    return;
  }
  *TraceStream << message << "\n";
  TraceStream->flush();
}

PNINode::iteratorTy PNINode::eraseFromParent() {
  assert(Parent.PNIMap.count(this) == 0);
  Parent.trace("[pndiff:node-erase] node=" + formatPNINodeForTrace(*this));
  return Parent.PNINodes.erase(getIterator());
}

void PNIGraph::onUpdatePNType(PNINode *N) {
  if (!OnUpdatePNType || PNIMap.count(N) == 0) {
    return;
  }
  for (auto Node : PNIMap.rev().at(N)) {
    OnUpdatePNType(Node);
  }
}

PNINode *PNINode::unify(PNINode &other) {
  assert(&Parent == &other.Parent);
  if (this == &other) {
    return this;
  }
  Parent.trace("[pndiff:unify] lhs=" + formatPNINodeForTrace(*this) +
               " rhs=" + formatPNINodeForTrace(other));
  auto *Node = Parent.mergePNINodes(this, &other);
  Parent.trace("[pndiff:unify-result] merged=" + formatPNINodeForTrace(*Node));
  return Node;
}

void PNIGraph::addAddCons(PNIValue Left, PNIValue Right, PNIValue Result,
                          PNIInstruction Inst) {
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
  traceConstraintEvent(*this, "constraint-add",
                       formatConstraintForTrace(*this, Node));
}

void PNIGraph::addSubCons(PNIValue Left, PNIValue Right, PNIValue Result,
                          PNIInstruction Inst) {
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
  traceConstraintEvent(*this, "constraint-add",
                       formatConstraintForTrace(*this, Node));
}

void PNIGraph::markChanged(PNINode *N, ConsNode *Except) {
  std::size_t Scheduled = 0;
  for (auto N2 : PNIMap.rev().at(N)) {
    if (NodeToCons.count(N2)) {
      for (auto *C2 : NodeToCons[N2]) {
        if (C2 == Except) {
          continue;
        }
        auto [_, Inserted] = Worklist.insert(C2);
        if (Inserted) {
          ++Scheduled;
        }
      }
    }
  }
  if (Scheduled != 0) {
    trace("[pndiff:mark-changed] node=" + formatPNINodeForTrace(*N) +
          " scheduled=" + std::to_string(Scheduled));
  }
}

void PNIGraph::mergePNVarTo(PNINode *Var, PNINode *Target) {
  assert(Var->getSize() == Target->getSize());
  assert(&Var->Parent == &Target->Parent);
  if (Var == Target) {
    return;
  }
  trace("[pndiff:merge-node] from=" + formatPNINodeForTrace(*Var) +
        " to=" + formatPNINodeForTrace(*Target));
  PNIMap.merge(Var, Target);
  Var->eraseFromParent();
  markChanged(Target);
}

bool PNINode::setPtrOrNum(PtrOrNum NewTy) {
  auto OldTy = Ty.getPtrOrNum();
  bool Updated = Ty.setPtrOrNum(NewTy);
  if (Updated) {
    Parent.trace("[pndiff:set-type] node=" + formatPNINodeForTrace(*this) +
                 " from=" + std::string(1, formatPtrOrNumChar(OldTy)) +
                 " to=" + std::string(1, formatPtrOrNumChar(NewTy)));
    Parent.onUpdatePNType(this);
  }
  return Updated;
}

PNINode::PNINode(PNIGraph &SSG, PNTy LowTy)
    : Parent(SSG), Id(SSG.allocateNodeId()), Ty(std::move(LowTy)) {
  Parent.trace("[pndiff:node-create] node=" + formatPNINodeForTrace(*this));
}

PNINode::PNINode(PNIGraph &SSG, const PNINode &OtherGraphNode)
    : Parent(SSG), Id(SSG.allocateNodeId()), Ty(OtherGraphNode.Ty) {
  Parent.trace("[pndiff:node-clone] node=" + formatPNINodeForTrace(*this));
}

PNINode::PNINode(PNIGraph &SSG, std::string SerializedTy)
    : Parent(SSG), Id(SSG.allocateNodeId()),
      Ty(SerializedTy.substr(0, SerializedTy.find(" ")), ({
           auto Pos = SerializedTy.find(" ");
           unsigned long Size;
           if (SerializedTy.substr(Pos + 1) == "p") {
             Size = SSG.PointerSize;
           } else {
             Size = std::stoul(SerializedTy.substr(Pos + 1));
           }
           Size;
         })) {
  Parent.trace("[pndiff:node-create] node=" + formatPNINodeForTrace(*this));
}

} // namespace notdec::mlsub
