#ifndef _NOTDEC_RETYPD_REXP_H_
#define _NOTDEC_RETYPD_REXP_H_

#include "Retypd/Schema.h"
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
struct Empty {};
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

std::vector<std::tuple<CGNode *, CGNode *, PRExp>>
eliminate(const ConstraintGraph &CG, std::set<CGNode *> &SCCNodes);

} // namespace notdec::retypd::rexp

#endif
