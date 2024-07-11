#include <cstring>
#include <variant>

#include <llvm/Support/raw_ostream.h>

#include "Retypd/Graph.h"
#include "Retypd/Schema.h"

namespace notdec::retypd {

std::string toString(EdgeLabel label) {
  if (std::holds_alternative<One>(label)) {
    return "_1_";
  } else if (std::holds_alternative<ForgetLabel>(label)) {
    return "forget " + toString(std::get<ForgetLabel>(label).label);
  } else if (std::holds_alternative<ForgetBase>(label)) {
    return "forget " + std::get<ForgetBase>(label).base;
  } else if (std::holds_alternative<RecallLabel>(label)) {
    return "recall " + toString(std::get<RecallLabel>(label).label);
  } else if (std::holds_alternative<RecallBase>(label)) {
    return "recall " + std::get<RecallBase>(label).base;
  }
  return "unknown";
}

void ConstraintGraph::build(std::vector<Constraint> &Cons) {
  buildInitialGraph(Cons);
  if (const char *path = std::getenv("DEBUG_TRANS_INIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) ||
        (std::strstr(path, FuncName.c_str()))) {
      printGraph("trans_init.dot");
    }
  }
}

/// build the initial graph (Algorithm D.1 Transducer)
void ConstraintGraph::buildInitialGraph(std::vector<Constraint> &Cons) {
  for (Constraint &C : Cons) {
    if (std::holds_alternative<SubTypeConstraint>(C)) {
      auto &SC = std::get<SubTypeConstraint>(C);
      // 1. add two node and 1-labeled edge
      auto &NodeL = GetOrInsertNode(SC.sub);
      auto &NodeR = GetOrInsertNode(SC.sup);
      // add 1-labeled edge between them
      AddEdge(NodeL, NodeR, One{});
      // 2. add each sub var node and edges.
      // 2.1 left
      addRecalls(NodeL);
      // 2.2 right
      addForgets(NodeR);

      // 3-4 the inverse of the above
      // 3. inverse node and 1-labeled edge
      auto &RNodeL = GetOrInsertNode(NodeKey(SC.sub, Contravariant));
      auto &RNodeR = GetOrInsertNode(NodeKey(SC.sup, Contravariant));
      // add 1-labeled edge between them
      AddEdge(RNodeR, RNodeL, One{});
      // 4.1 inverse left
      addRecalls(RNodeL);
      // 4.2 inverse right
      addForgets(RNodeR);
    } else {
      AddConstraints.push_back(C);
    }
  }
}

std::optional<std::pair<FieldLabel, NodeKey>> NodeKey::forgetOnce() const {
  if (Base.Labels.empty()) {
    return std::nullopt;
  }
  NodeKey NewKey(*this);
  FieldLabel Label = NewKey.Base.Labels.back();
  NewKey.Base.Labels.pop_back();
  NewKey.SuffixVariance = combine(this->SuffixVariance, getVariance(Label));
  return std::make_pair(Label, NewKey);
}

void ConstraintGraph::addRecalls(CGNode &N) {
  CGNode *T = &N;
  auto V1 = T->key.forgetOnce();
  while (V1.has_value()) {
    auto [Cap, Next] = V1.value();
    auto &NNext = GetOrInsertNode(Next);
    AddEdge(NNext, *T, RecallLabel{Cap});
    V1 = Next.forgetOnce();
    T = &NNext;
  }
  // We do not really link the node to #Start
  StartNodes.insert(T);
}

void ConstraintGraph::addForgets(CGNode &N) {
  CGNode *T = &N;
  auto V1 = T->key.forgetOnce();
  while (V1.has_value()) {
    auto [Cap, Next] = V1.value();
    auto &NNext = GetOrInsertNode(Next);
    AddEdge(*T, NNext, ForgetLabel{Cap});
    V1 = Next.forgetOnce();
    T = &NNext;
  }
  // We do not really link the node to #Start
  EndNodes.insert(T);
}

CGNode &ConstraintGraph::GetOrInsertNode(const NodeKey &N) {
  auto [it, inserted] = Nodes.try_emplace(N, N);
  if (inserted) {
    assert(addNode(it->second));
  }

  return it->second;
}

void ConstraintGraph::printGraph(const char *DotFile) {
  std::error_code EC;
  llvm::raw_fd_ostream OutStream(DotFile, EC);
  if (EC) {
    llvm::errs() << "Error: " << EC.message() << "\n";
    return;
  }
  llvm::WriteGraph(OutStream, this, false);
  OutStream.flush();
  OutStream.close();
}

std::string toString(NodeKey K) { return K.str(); }

} // namespace notdec::retypd
