#ifndef _NOTDEC_RETYPD_GRAPH_H_
#define _NOTDEC_RETYPD_GRAPH_H_

#include "Retypd/Schema.h"
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/simple_ilist.h>
#include <set>
#include <variant>

namespace notdec::retypd {

struct One {};
struct ForgetLabel {
  FieldLabel label;
};
struct ForgetBase {
  std::string base;
};
struct RecallLabel {
  FieldLabel label;
};
struct RecallBase {
  std::string base;
};
using EdgeLabel =
    std::variant<One, ForgetLabel, ForgetBase, RecallLabel, RecallBase>;

std::string toString(EdgeLabel label);
inline bool isBase(EdgeLabel label) {
  return std::holds_alternative<ForgetBase>(label) ||
         std::holds_alternative<RecallBase>(label);
}
inline bool isLabel(EdgeLabel label) {
  return std::holds_alternative<ForgetLabel>(label) ||
         std::holds_alternative<RecallLabel>(label);
}

struct Node;
struct ConstraintGraph;
struct Edge {
  EdgeLabel label;
  Node *target;
};

struct Node : public llvm::ilist_node_with_parent<Node, ConstraintGraph> {
  DerivedTypeVariable Base;
  Variance SuffixVariance;
  bool IsNewLayer = false;

  ConstraintGraph *Parent;
  std::vector<Edge> Edges;
  std::vector<Node *> Preds;
};

struct ConstraintGraph {
  using NodeListTy = llvm::simple_ilist<Node>;
  std::set<Node *> StartNodes;
  std::set<Node *> EndNodes;
  // pub path_seq: Vec<(NodeIndex, NodeIndex, RExp)>,
  Node *Start;
  Node *End;
};

} // namespace notdec::retypd

#endif
