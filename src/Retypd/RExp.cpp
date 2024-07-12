#include "Retypd/RExp.h"

namespace notdec::retypd::rexp {

std::string toString(const PRExp &rexp) {
  if (std::holds_alternative<Null>(*rexp)) {
    return "∅";
  } else if (std::holds_alternative<Empty>(*rexp)) {
    return "ε";
  } else if (std::holds_alternative<Star>(*rexp)) {
    return "Star";
  } else if (std::holds_alternative<Or>(*rexp)) {
    std::string ret = "(";
    for (const auto &e : std::get<Or>(*rexp).E) {
      if (ret.size() > 1) {
        ret += " . ";
      }
      ret += toString(e);
    }
    ret += ")";
    return ret;
  } else if (std::holds_alternative<And>(*rexp)) {
    std::string ret = "(";
    for (const auto &e : std::get<And>(*rexp).E) {
      if (ret.size() > 1) {
        ret += " U ";
      }
      ret += toString(e);
    }
    ret += ")";
    return ret;
  } else if (std::holds_alternative<Node>(*rexp)) {
    return toString(std::get<Node>(*rexp).E);
  }
  std::abort();
}

/// \brief Simplify the outer most layer of RExp tree.
///
/// 1. For Or / And, flatten nested Or / And.
/// 2. For Star, remove directly nested star.
/// 3. Handle cases involving Empty or Null.
PRExp simplifyOnce(const PRExp &Original) {
  // For Or / And, flatten nested Or / And.
  if (std::holds_alternative<Or>(*Original)) {
    auto &E = std::get<Or>(*Original).E;
    if (E.size() == 0) {
      return std::make_shared<RExp>(Null{});
    } else if (E.size() == 1) {
      return *E.begin();
    }
    auto Ret = std::make_shared<RExp>(Or{});
    auto &RetE = std::get<Or>(*Ret).E;
    for (auto it = E.begin(); it != E.end(); ++it) {
      const PRExp &E = *it;
      // flatten nested OR
      if (std::holds_alternative<Or>(*E)) {
        auto &orE2 = std::get<Or>(*E).E;
        RetE.insert(orE2.begin(), orE2.end());
      } else if (std::holds_alternative<Null>(*E)) {
        // do nothing, skip null.
      } else {
        RetE.insert(E);
      }
    }
    return Ret;
  } else if (std::holds_alternative<And>(*Original)) {
    auto &E = std::get<And>(*Original).E;
    if (E.size() == 0) {
      return std::make_shared<RExp>(Empty{});
    } else if (E.size() == 1) {
      return *E.begin();
    }
    auto Ret = std::make_shared<RExp>(And{});
    auto &RetE = std::get<And>(*Ret).E;
    for (auto it = E.begin(); it != E.end(); ++it) {
      const PRExp &E = *it;
      if (std::holds_alternative<And>(*E)) {
        auto &andE2 = std::get<And>(*E).E;
        RetE.insert(RetE.end(), andE2.begin(), andE2.end());
      } else if (std::holds_alternative<Empty>(*E)) {
        // do nothing, skip empty.
      } else {
        RetE.push_back(E);
      }
    }
    return Ret;
  } else if (std::holds_alternative<Star>(*Original)) {
    auto &E = std::get<Star>(*Original).E;
    // remove directly nested star
    if (std::holds_alternative<Star>(*E)) {
      return E;
    } else if (std::holds_alternative<Empty>(*E)) {
      return std::make_shared<RExp>(Empty{});
    } else if (std::holds_alternative<Null>(*E)) {
      return std::make_shared<RExp>(Empty{});
    } else {
      return Original;
    }
  } else {
    return Original;
  }
}

/// \brief Get the first path expression node.
///
/// Used for checking if it is a recall or forget.
std::optional<retypd::EdgeLabel *> first_node(const PRExp &rexp) {
  if (std::holds_alternative<Node>(*rexp)) {
    return &std::get<Node>(*rexp).E;
  } else if (std::holds_alternative<Or>(*rexp)) {
    for (const auto &e : std::get<Or>(*rexp).E) {
      auto ret = first_node(e);
      if (ret) {
        return ret;
      }
    }
    return std::nullopt;
  } else if (std::holds_alternative<And>(*rexp)) {
    for (const auto &e : std::get<And>(*rexp).E) {
      auto ret = first_node(e);
      if (ret) {
        return ret;
      }
    }
    return std::nullopt;
  } else if (std::holds_alternative<Star>(*rexp)) {
    return first_node(std::get<Star>(*rexp).E);
  } else if (std::holds_alternative<Empty>(*rexp) ||
             std::holds_alternative<Null>(*rexp)) {
    return std::nullopt;
  }
  std::abort();
}

/// \brief Get the last path expression node.
///
/// Used for checking if it is a recall or forget.
std::optional<retypd::EdgeLabel *> last_node(const PRExp &rexp) {
  if (std::holds_alternative<Node>(*rexp)) {
    return &std::get<Node>(*rexp).E;
  } else if (std::holds_alternative<Or>(*rexp)) {
    auto &E = std::get<Or>(*rexp).E;
    for (auto it = E.rbegin(); it != E.rend(); ++it) {
      const auto &E2 = *it;
      auto ret = last_node(E2);
      if (ret) {
        return ret;
      }
    }
    return std::nullopt;
  } else if (std::holds_alternative<And>(*rexp)) {
    auto &E = std::get<And>(*rexp).E;
    for (auto it = E.rbegin(); it != E.rend(); ++it) {
      const auto &E2 = *it;
      auto ret = last_node(E2);
      if (ret) {
        return ret;
      }
    }
    return std::nullopt;
  } else if (std::holds_alternative<Star>(*rexp)) {
    return last_node(std::get<Star>(*rexp).E);
  } else if (std::holds_alternative<Empty>(*rexp) ||
             std::holds_alternative<Null>(*rexp)) {
    return std::nullopt;
  }
  std::abort();
}

/// Combining two path expression with And.
PRExp operator&(const PRExp &A, const PRExp &B) {
  if (std::holds_alternative<And>(*A) && std::holds_alternative<And>(*B)) {
    auto Ret = std::make_shared<RExp>(And{});
    auto &E = std::get<And>(*Ret).E;
    auto &EA = std::get<And>(*A).E;
    auto &EB = std::get<And>(*B).E;
    E.insert(E.end(), EA.begin(), EA.end());
    E.insert(E.end(), EB.begin(), EB.end());
    return Ret;
  } else if (std::holds_alternative<And>(*A)) {
    auto Ret = std::make_shared<RExp>(And{});
    auto &E = std::get<And>(*Ret).E;
    auto &EA = std::get<And>(*A).E;
    E.insert(E.end(), EA.begin(), EA.end());
    E.push_back(B);
    return Ret;
  } else if (std::holds_alternative<And>(*B)) {
    auto Ret = std::make_shared<RExp>(And{});
    auto &E = std::get<And>(*Ret).E;
    auto &EB = std::get<And>(*B).E;
    E.push_back(A);
    E.insert(E.end(), EB.begin(), EB.end());
    return Ret;
  } else {
    auto Ret = std::make_shared<RExp>(And{});
    auto &E = std::get<And>(*Ret).E;
    E.push_back(A);
    E.push_back(B);
    return Ret;
  }
}

/// Combining two path expression with Or.
PRExp operator|(const PRExp &A, const PRExp &B) {
  if (std::holds_alternative<Or>(*A) && std::holds_alternative<Or>(*B)) {
    auto Ret = std::make_shared<RExp>(Or{});
    auto &E = std::get<Or>(*Ret).E;
    auto &EA = std::get<Or>(*A).E;
    auto &EB = std::get<Or>(*B).E;
    E.insert(EA.begin(), EA.end());
    E.insert(EB.begin(), EB.end());
    return Ret;
  } else if (std::holds_alternative<Or>(*A)) {
    auto Ret = std::make_shared<RExp>(Or{});
    auto &E = std::get<Or>(*Ret).E;
    auto &EA = std::get<Or>(*A).E;
    E.insert(EA.begin(), EA.end());
    E.insert(B);
    return Ret;
  } else if (std::holds_alternative<Or>(*B)) {
    auto Ret = std::make_shared<RExp>(Or{});
    auto &E = std::get<Or>(*Ret).E;
    auto &EB = std::get<Or>(*B).E;
    E.insert(A);
    E.insert(EB.begin(), EB.end());
    return Ret;
  } else {
    auto Ret = std::make_shared<RExp>(Or{});
    auto &E = std::get<Or>(*Ret).E;
    E.insert(A);
    E.insert(B);
    return Ret;
  }
}

// TODO: do we need &= or |=

std::vector<std::tuple<CGNode *, CGNode *, rexp::PRExp>>
eliminate(const ConstraintGraph &CG, std::vector<CGNode *> SCCNodes) {}

} // namespace notdec::retypd::rexp
