#ifndef _NOTDEC_RETYPD_REXP_H_
#define _NOTDEC_RETYPD_REXP_H_

#include "TypeRecovery/Schema.h"
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/iterator_range.h>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

namespace notdec::retypd {

struct CGNode;
struct ConstraintGraph;
} // namespace notdec::retypd

namespace notdec::retypd::rexp {

struct Null;
struct Empty;
struct Star;
struct Or;
struct And;
struct Node;
using notdec::retypd::CGNode;
using notdec::retypd::ConstraintGraph;
using notdec::retypd::EdgeLabel;

using RExp = std::variant<Null, Empty, Star, Or, And, Node>;
using PRExp = std::shared_ptr<std::variant<Null, Empty, Star, Or, And, Node>>;
struct Null {};
extern PRExp NullInstance;
struct Empty {};
extern PRExp EmptyInstance;
struct Star {
  PRExp E;
};
// `U`, `\cup`
struct Or {
  std::set<PRExp> E;
};
// Dot concatenation
struct And {
  std::vector<PRExp> E;
};
struct Node {
  EdgeLabel E;
};

inline bool isNull(const PRExp &rexp) {
  return std::holds_alternative<Null>(*rexp);
}

inline bool isEmpty(const PRExp &rexp) {
  return std::holds_alternative<Empty>(*rexp);
}

PRExp createNull();
PRExp createEmpty();
PRExp createOr();
PRExp createAnd();
PRExp createStar(const PRExp &EL);
PRExp create(const EdgeLabel &EL);

std::string toString(const PRExp &rexp);
PRExp simplifyOnce(const PRExp &Original);
std::optional<retypd::EdgeLabel *> firstNode(const PRExp &rexp);
std::optional<retypd::EdgeLabel *> lastNode(const PRExp &rexp);
PRExp operator&(const PRExp &A, const PRExp &B);
PRExp operator|(const PRExp &A, const PRExp &B);

// std::vector<std::tuple<NodeRef , NodeRef , PRExp>>
// eliminate(std::set<NodeRef > &SCCNodes);

template <class NodeRef>
std::vector<std::tuple<NodeRef, NodeRef, PRExp>>
eliminate(std::set<NodeRef> &SCCNodes) {
  using GT = llvm::GraphTraits<NodeRef>;
  // default to Null path (no path).
  // TODO: std::map<std::pair<unsigned, unsigned>, PRExp> P; ? use index
  // instead of pointer
  std::map<std::pair<NodeRef, NodeRef>, PRExp> P;
  auto getMap = [&](NodeRef N1, NodeRef N2) -> PRExp {
    auto It = P.find({N1, N2});
    if (It == P.end()) {
      return createNull();
    }
    return It->second;
  };

  // have a index for each node.
  std::vector<NodeRef> Nodes;
  // For each edge within SCC, initialize.
  for (auto N : SCCNodes) {
    Nodes.push_back(N);
    for (auto E :
         llvm::make_range(GT::child_edge_begin(N), GT::child_edge_end(N))) {
      auto &Target = *GT::getEdgeTarget(E);
      if (SCCNodes.count(&Target) == 0) {
        continue;
      }
      auto E1 = create(GT::getEdgeLabel(E));
      auto R = P.insert({{N, &Target}, E1});
      // assert(R.second && "Not Inserted?");
      if (!R.second) {
        // not inserted? There is a multi edge.
        std::set<PRExp> OrInner;
        OrInner.insert(R.first->second);
        OrInner.insert(E1);
        auto Or1 = std::make_shared<RExp>(Or{.E = OrInner});
        P.insert_or_assign({N, &Target}, Or1);
      }
    }
  }

  // eliminate
  for (unsigned VInd = 0; VInd < Nodes.size(); VInd++) {
    NodeRef V = Nodes[VInd];
    auto VV = getMap(V, V);
    if (!isNull(VV)) {
      P.insert_or_assign({V, V}, simplifyOnce(createStar(VV)));
    }
    for (unsigned UInd = VInd + 1; UInd < Nodes.size(); UInd++) {
      NodeRef U = Nodes[UInd];
      auto UV = getMap(U, V);
      if (isNull(UV)) {
        continue;
      }

      if (!isNull(VV)) {
        UV = simplifyOnce(UV & VV);
        P.insert_or_assign({U, V}, UV);
      }

      for (unsigned WInd = VInd + 1; WInd < Nodes.size(); WInd++) {
        NodeRef W = Nodes[WInd];
        auto VW = getMap(V, W);
        if (isNull(VW)) {
          continue;
        }

        auto UW = getMap(U, W);
        UW = simplifyOnce(UW | simplifyOnce(UV & VW));
        P.insert_or_assign({U, W}, UW);
      }
    }
  }

  std::map<NodeRef, unsigned> IndexMap;
  for (unsigned i = 0; i < Nodes.size(); i++) {
    IndexMap.insert({Nodes[i], i});
  }
  std::vector<std::tuple<NodeRef, NodeRef, PRExp>> Ascending;
  Ascending.reserve(P.size());
  std::vector<std::tuple<NodeRef, NodeRef, PRExp>> Descending;
  for (auto &Ent : P) {
    if (isNull(Ent.second)) {
      continue;
    }
    if (IndexMap[Ent.first.first] <= IndexMap[Ent.first.second]) {
      Ascending.push_back({Ent.first.first, Ent.first.second, Ent.second});
    } else {
      Descending.push_back({Ent.first.first, Ent.first.second, Ent.second});
    }
  }
  // sort Ascending by the first index
  std::sort(Ascending.begin(), Ascending.end(),
            [&](const auto &A, const auto &B) {
              return IndexMap[std::get<0>(A)] < IndexMap[std::get<0>(B)];
            });
  // sort Descending by the first index
  std::sort(Descending.begin(), Descending.end(),
            [&](const auto &A, const auto &B) {
              return IndexMap[std::get<0>(A)] > IndexMap[std::get<0>(B)];
            });
  Ascending.insert(Ascending.end(), Descending.begin(), Descending.end());
  return Ascending;
}

template <typename NodeRef>
std::map<std::pair<NodeRef, NodeRef>, rexp::PRExp> solve_constraints(
    NodeRef Source,
    std::vector<std::tuple<NodeRef, NodeRef, rexp::PRExp>> &PathSeq) {
  std::map<std::pair<NodeRef, NodeRef>, rexp::PRExp> P;

  auto getPexp = [&](NodeRef From, NodeRef To) -> rexp::PRExp {
    auto Key = std::make_pair(From, To);
    if (P.count(Key)) {
      return P[Key];
    }
    if (From == To) {
      return createEmpty();
    } else {
      return createNull();
    }
  };
  auto assignPExp = [&](NodeRef From, NodeRef To, rexp::PRExp E) {
    if (From == To && rexp::isEmpty(E)) {
      return;
    }
    if (From != To && rexp::isNull(E)) {
      return;
    }
    P[std::make_pair(From, To)] = E;
  };
  auto bitandAssignPExp = [&](NodeRef From, NodeRef To, rexp::PRExp E) {
    auto Old = getPexp(From, To);
    assignPExp(From, To, rexp::simplifyOnce(Old & E));
  };
  auto bitorAssignPExp = [&](NodeRef From, NodeRef To, rexp::PRExp E) {
    auto Old = getPexp(From, To);
    assignPExp(From, To, rexp::simplifyOnce(Old | E));
  };

  // 1.3. solve the pathexpr(start)
  for (auto Ent : PathSeq) {
    auto [From, To, E] = Ent;
    if (From == To) {
      // P[source, from] = P[source, from] & exp
      bitandAssignPExp(Source, From, E);
    } else {
      // P[source, to] = P[source, to] | (P[source, from] & exp)
      auto srcFrom = getPexp(Source, From);
      bitorAssignPExp(Source, To, rexp::simplifyOnce(srcFrom & E));
    }
  }
  return P;
}

} // namespace notdec::retypd::rexp

#endif
