#ifndef BINARYSUB_SIMPLESUB_H
#define BINARYSUB_SIMPLESUB_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace notdec::simplesub {

// Forward declaration for error handling
template <typename E> class unexpected {
public:
  unexpected(E &&error) : error_(std::move(error)) {}
  unexpected(const E &error) : error_(error) {}
  const E &error() const { return error_; }

private:
  E error_;
};

template <typename E> unexpected<E> make_unexpected(E &&error) {
  return unexpected<E>(std::forward<E>(error));
}

template <typename T, typename E> class expected {
public:
  expected() : has_value_(true) {}
  expected(const T &value) : has_value_(true), value_(value) {}

  template <typename U>
  expected(const unexpected<U> &error)
      : has_value_(false), error_(error.error()) {}

  bool has_value() const { return has_value_; }
  operator bool() const { return has_value_; }
  bool operator!() const { return !has_value_; }

  const T &value() const { return value_; }
  T &value() { return value_; }

  const E &error() const { return error_; }

private:
  bool has_value_;
  union {
    T value_;
    E error_;
  };
};

// Specialization for void
template <typename E> class expected<void, E> {
public:
  expected() : has_value_(true) {}

  template <typename U>
  expected(const unexpected<U> &error)
      : has_value_(false), error_(error.error()) {}

  bool has_value() const { return has_value_; }
  operator bool() const { return has_value_; }
  bool operator!() const { return !has_value_; }

  const E &error() const { return error_; }

private:
  bool has_value_;
  E error_;
};

// ======================= Solver cache & error ==============================
struct Error {
  std::string msg;
  static Error make(std::string m);
};
inline Error Error::make(std::string m) { return {std::move(m)}; }

// ======================= Fresh supply & scope levels =======================
struct VarSupply {
  std::uint32_t next = 0;
  std::uint32_t fresh_id();
};

struct Scope {
  int level = 0;
  void enter();
  void leave();
};

// ======================= SimpleType =============
struct TypeNode;
using SimpleType = std::shared_ptr<TypeNode>;

struct VariableState {
  std::vector<SimpleType> lowerBounds;
  std::vector<SimpleType> upperBounds;
  std::uint32_t id;
  int level;
  VariableState(std::uint32_t i, int lvl);
};

struct TPrimitive {
  std::string name;
};

struct TFunction {
  SimpleType lhs, rhs;
};

struct TRecord {
  std::vector<std::pair<std::string, SimpleType>> fields;
};

struct TypeNode {
  std::variant<TPrimitive, VariableState, TFunction, TRecord> v;

  explicit TypeNode(TPrimitive p);
  explicit TypeNode(VariableState vs);
  explicit TypeNode(TFunction f);
  explicit TypeNode(TRecord r);

  TPrimitive *getAsTPrimitive();
  const TPrimitive *getAsTPrimitive() const;

  VariableState *getAsVariableState();
  const VariableState *getAsVariableState() const;

  TFunction *getAsTFunction();
  const TFunction *getAsTFunction() const;

  TRecord *getAsTRecord();
  const TRecord *getAsTRecord() const;

  TFunction &getAsTFunctionRef();
  const TFunction &getAsTFunctionRef() const;

  bool isTPrimitive() const;
  bool isVariableState() const;
  bool isTFunction() const;
  bool isTRecord() const;
};

// Helper functions for type checking variant types directly
template <typename T> constexpr bool isTPrimitiveType();

template <typename T> constexpr bool isVariableStateType();

template <typename T> constexpr bool isTFunctionType();

template <typename T> constexpr bool isTRecordType();

// Type creation functions
SimpleType make_primitive(std::string name);
SimpleType make_variable(std::uint32_t id, int lvl);
SimpleType fresh_variable(VarSupply &vs, int lvl);
SimpleType make_function(SimpleType a, SimpleType b);
SimpleType make_record(std::vector<std::pair<std::string, SimpleType>> fields);

// Utility functions
int level_of(const SimpleType &st);
VariableState *extractVariableState(const SimpleType &st);

using Cache = std::set<std::pair<const TypeNode *, const TypeNode *>>;

// ======================= Extrusion (level-fixing copy) =====================
struct PolarVar {
  SimpleType var;
  bool pos;
  bool operator<(const PolarVar &other) const {
    auto var_ptr = extractVariableState(var);
    auto other_var_ptr = extractVariableState(other.var);
    if (var_ptr != other_var_ptr)
      return var_ptr < other_var_ptr;
    return pos < other.pos;
  }
};

SimpleType extrude(const SimpleType &ty, bool pol, int lvl,
                   std::map<PolarVar, std::shared_ptr<VariableState>> &cache,
                   VarSupply &supply);

// ======================= Subtype constraint solver with levels =============
expected<void, Error> constrain(const SimpleType &lhs, const SimpleType &rhs,
                                Cache &cache, VarSupply &supply);

expected<void, Error> constrain_impl(const SimpleType &lhs,
                                     const SimpleType &rhs, Cache &cache,
                                     VarSupply &supply);

// ======================= User-facing types ========================

struct UTop {};
struct UBot {};
struct UTypeVariable {
  std::string name;
};
struct UPrimitiveType {
  std::string name;
};

struct UType;
using UTypePtr = std::shared_ptr<UType>;

struct UUnion {
  UTypePtr lhs, rhs;
};
struct UInter {
  UTypePtr lhs, rhs;
};
struct UFunctionType {
  UTypePtr lhs, rhs;
};
struct URecordType {
  std::vector<std::pair<std::string, UTypePtr>> fields;
};
struct URecursiveType {
  std::string name;
  UTypePtr body;
};

struct UType {
  std::variant<UTop, UBot, UUnion, UInter, UFunctionType, URecordType,
               URecursiveType, UTypeVariable, UPrimitiveType>
      v;

  explicit UType(UTop t) : v(std::move(t)) {}
  explicit UType(UBot b) : v(std::move(b)) {}
  explicit UType(UUnion u) : v(std::move(u)) {}
  explicit UType(UInter i) : v(std::move(i)) {}
  explicit UType(UFunctionType f) : v(std::move(f)) {}
  explicit UType(URecordType r) : v(std::move(r)) {}
  explicit UType(URecursiveType rt) : v(std::move(rt)) {}
  explicit UType(UTypeVariable tv) : v(std::move(tv)) {}
  explicit UType(UPrimitiveType pt) : v(std::move(pt)) {}
};

// Helper functions to create UType instances
inline UTypePtr make_utop() { return std::make_shared<UType>(UTop{}); }

inline UTypePtr make_ubot() { return std::make_shared<UType>(UBot{}); }

inline UTypePtr make_uunion(UTypePtr lhs, UTypePtr rhs) {
  return std::make_shared<UType>(UUnion{std::move(lhs), std::move(rhs)});
}

inline UTypePtr make_uinter(UTypePtr lhs, UTypePtr rhs) {
  return std::make_shared<UType>(UInter{std::move(lhs), std::move(rhs)});
}

inline UTypePtr make_ufunctiontype(UTypePtr lhs, UTypePtr rhs) {
  return std::make_shared<UType>(UFunctionType{std::move(lhs), std::move(rhs)});
}

inline UTypePtr
make_urecordtype(std::vector<std::pair<std::string, UTypePtr>> fields) {
  return std::make_shared<UType>(URecordType{std::move(fields)});
}

inline UTypePtr make_urecursivetype(std::string name, UTypePtr body) {
  return std::make_shared<UType>(
      URecursiveType{std::move(name), std::move(body)});
}

inline UTypePtr make_utypevariable(std::string name) {
  return std::make_shared<UType>(UTypeVariable{std::move(name)});
}

inline UTypePtr make_uprimitivetype(std::string name) {
  return std::make_shared<UType>(UPrimitiveType{std::move(name)});
}

// Pretty printing
std::string printType(const UTypePtr &ty);
void printTypeImpl(const UTypePtr &ty, std::ostream &os, int precedence = 0);

// =================== Type Simplification ===========================

// Intermediate representation for simplification (Section 4.4)
struct CompactType {
  std::set<SimpleType> vars;  // type variables
  std::set<SimpleType> prims; // primitive types
  std::optional<std::map<std::string, std::shared_ptr<CompactType>>>
      record; // record fields
  std::optional<
      std::pair<std::shared_ptr<CompactType>, std::shared_ptr<CompactType>>>
      function; // function type
};

struct CompactTypeScheme {
  std::shared_ptr<CompactType> cty;
  std::map<SimpleType, std::shared_ptr<CompactType>>
      recVars; // recursive variable bounds
};

// CompactType helper functions
inline std::shared_ptr<CompactType> make_empty_compact_type() {
  return std::make_shared<CompactType>();
}
std::shared_ptr<CompactType>
merge_compact_types(bool pol, const std::shared_ptr<CompactType> &lhs,
                    const std::shared_ptr<CompactType> &rhs);
std::string toString(const CompactType &ct);

// Co-occurrence analysis data structures
struct OccurrenceData {
  std::set<SimpleType> variables;  // Only variable types
  std::set<SimpleType> primitives; // Only primitive types
};
using OccurrenceMap = std::map<PolarVar, OccurrenceData>;
OccurrenceMap analyzeOccurrences(const CompactTypeScheme &ty);
std::string toString(const OccurrenceMap &om);

// Coalesce SimpleType to UType for display purposes
UTypePtr coalesceType(const SimpleType &st);

// Simplification functions
CompactTypeScheme compactType(const SimpleType &st);
CompactTypeScheme canonicalizeType(const SimpleType &st);
CompactTypeScheme simplifyType(const CompactTypeScheme &ty);
// Coalesces a CompactTypeScheme into a Type while performing hash-consing
UTypePtr coalesceCompactType(const CompactTypeScheme &st);

// Simplification transformations
CompactTypeScheme removePolarVariables(const CompactTypeScheme &ty,
                                       const OccurrenceMap &occMap);

// ============= Type schemes (let-polymorphism without AST) =================
struct MonoScheme {
  SimpleType body;
};

struct PolyScheme {
  int generalized_above;
  SimpleType body;
};

using TypeScheme = std::variant<MonoScheme, PolyScheme>;

SimpleType freshen_above_rec(const SimpleType &t, int cutoff, int at_level,
                             std::map<SimpleType, SimpleType> &memo,
                             VarSupply &supply);

SimpleType instantiate(const TypeScheme &sch, int at_level, VarSupply &supply);

TypeScheme generalize(const SimpleType &rhs, int env_level);

} // namespace binarysub

#endif // BINARYSUB_SIMPLESUB_H
