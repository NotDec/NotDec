#define SOUFFLE_GENERATOR_VERSION "2.4"
#include "souffle/CompiledSouffle.h"
#include "souffle/SignalHandler.h"
#include "souffle/SouffleInterface.h"
#include "souffle/datastructure/BTree.h"
#include "souffle/io/IOSystem.h"
#include <any>
namespace functors {
extern "C" {}
} // namespace functors
namespace souffle::t_btree_uiiu__1_0_2_3__1111__0100 {
using namespace souffle;
struct Type {
  static constexpr Relation::arity_type Arity = 4;
  using t_tuple = Tuple<RamDomain, 4>;
  struct t_comparator_0 {
    int operator()(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1])) ? -1
             : (ramBitCast<RamSigned>(a[1]) > ramBitCast<RamSigned>(b[1]))
                 ? 1
                 : ((ramBitCast<RamUnsigned>(a[0]) <
                     ramBitCast<RamUnsigned>(b[0]))
                        ? -1
                    : (ramBitCast<RamUnsigned>(a[0]) >
                       ramBitCast<RamUnsigned>(b[0]))
                        ? 1
                        : ((ramBitCast<RamSigned>(a[2]) <
                            ramBitCast<RamSigned>(b[2]))
                               ? -1
                           : (ramBitCast<RamSigned>(a[2]) >
                              ramBitCast<RamSigned>(b[2]))
                               ? 1
                               : ((ramBitCast<RamUnsigned>(a[3]) <
                                   ramBitCast<RamUnsigned>(b[3]))
                                      ? -1
                                  : (ramBitCast<RamUnsigned>(a[3]) >
                                     ramBitCast<RamUnsigned>(b[3]))
                                      ? 1
                                      : (0))));
    }
    bool less(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamSigned>(a[1]) < ramBitCast<RamSigned>(b[1])) ||
             ((ramBitCast<RamSigned>(a[1]) == ramBitCast<RamSigned>(b[1])) &&
              ((ramBitCast<RamUnsigned>(a[0]) <
                ramBitCast<RamUnsigned>(b[0])) ||
               ((ramBitCast<RamUnsigned>(a[0]) ==
                 ramBitCast<RamUnsigned>(b[0])) &&
                ((ramBitCast<RamSigned>(a[2]) < ramBitCast<RamSigned>(b[2])) ||
                 ((ramBitCast<RamSigned>(a[2]) ==
                   ramBitCast<RamSigned>(b[2])) &&
                  ((ramBitCast<RamUnsigned>(a[3]) <
                    ramBitCast<RamUnsigned>(b[3]))))))));
    }
    bool equal(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamSigned>(a[1]) == ramBitCast<RamSigned>(b[1])) &&
             (ramBitCast<RamUnsigned>(a[0]) == ramBitCast<RamUnsigned>(b[0])) &&
             (ramBitCast<RamSigned>(a[2]) == ramBitCast<RamSigned>(b[2])) &&
             (ramBitCast<RamUnsigned>(a[3]) == ramBitCast<RamUnsigned>(b[3]));
    }
  };
  using t_ind_0 = btree_set<t_tuple, t_comparator_0>;
  t_ind_0 ind_0;
  using iterator = t_ind_0::iterator;
  struct context {
    t_ind_0::operation_hints hints_0_lower;
    t_ind_0::operation_hints hints_0_upper;
  };
  context createContext() { return context(); }
  bool insert(const t_tuple &t);
  bool insert(const t_tuple &t, context &h);
  bool insert(const RamDomain *ramDomain);
  bool insert(RamDomain a0, RamDomain a1, RamDomain a2, RamDomain a3);
  bool contains(const t_tuple &t, context &h) const;
  bool contains(const t_tuple &t) const;
  std::size_t size() const;
  iterator find(const t_tuple &t, context &h) const;
  iterator find(const t_tuple &t) const;
  range<iterator> lowerUpperRange_0000(const t_tuple & /* lower */,
                                       const t_tuple & /* upper */,
                                       context & /* h */) const;
  range<iterator> lowerUpperRange_0000(const t_tuple & /* lower */,
                                       const t_tuple & /* upper */) const;
  range<t_ind_0::iterator> lowerUpperRange_1111(const t_tuple &lower,
                                                const t_tuple &upper,
                                                context &h) const;
  range<t_ind_0::iterator> lowerUpperRange_1111(const t_tuple &lower,
                                                const t_tuple &upper) const;
  range<t_ind_0::iterator> lowerUpperRange_0100(const t_tuple &lower,
                                                const t_tuple &upper,
                                                context &h) const;
  range<t_ind_0::iterator> lowerUpperRange_0100(const t_tuple &lower,
                                                const t_tuple &upper) const;
  bool empty() const;
  std::vector<range<iterator>> partition() const;
  void purge();
  iterator begin() const;
  iterator end() const;
  void printStatistics(std::ostream &o) const;
};
} // namespace souffle::t_btree_uiiu__1_0_2_3__1111__0100
namespace souffle::t_btree_uiiu__1_0_2_3__1111__0100 {
using namespace souffle;
using t_ind_0 = Type::t_ind_0;
using iterator = Type::iterator;
using context = Type::context;
bool Type::insert(const t_tuple &t) {
  context h;
  return insert(t, h);
}
bool Type::insert(const t_tuple &t, context &h) {
  if (ind_0.insert(t, h.hints_0_lower)) {
    return true;
  } else
    return false;
}
bool Type::insert(const RamDomain *ramDomain) {
  RamDomain data[4];
  std::copy(ramDomain, ramDomain + 4, data);
  const t_tuple &tuple = reinterpret_cast<const t_tuple &>(data);
  context h;
  return insert(tuple, h);
}
bool Type::insert(RamDomain a0, RamDomain a1, RamDomain a2, RamDomain a3) {
  RamDomain data[4] = {a0, a1, a2, a3};
  return insert(data);
}
bool Type::contains(const t_tuple &t, context &h) const {
  return ind_0.contains(t, h.hints_0_lower);
}
bool Type::contains(const t_tuple &t) const {
  context h;
  return contains(t, h);
}
std::size_t Type::size() const { return ind_0.size(); }
iterator Type::find(const t_tuple &t, context &h) const {
  return ind_0.find(t, h.hints_0_lower);
}
iterator Type::find(const t_tuple &t) const {
  context h;
  return find(t, h);
}
range<iterator> Type::lowerUpperRange_0000(const t_tuple & /* lower */,
                                           const t_tuple & /* upper */,
                                           context & /* h */) const {
  return range<iterator>(ind_0.begin(), ind_0.end());
}
range<iterator> Type::lowerUpperRange_0000(const t_tuple & /* lower */,
                                           const t_tuple & /* upper */) const {
  return range<iterator>(ind_0.begin(), ind_0.end());
}
range<t_ind_0::iterator> Type::lowerUpperRange_1111(const t_tuple &lower,
                                                    const t_tuple &upper,
                                                    context &h) const {
  t_comparator_0 comparator;
  int cmp = comparator(lower, upper);
  if (cmp == 0) {
    auto pos = ind_0.find(lower, h.hints_0_lower);
    auto fin = ind_0.end();
    if (pos != fin) {
      fin = pos;
      ++fin;
    }
    return make_range(pos, fin);
  }
  if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
  }
  return make_range(ind_0.lower_bound(lower, h.hints_0_lower),
                    ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator>
Type::lowerUpperRange_1111(const t_tuple &lower, const t_tuple &upper) const {
  context h;
  return lowerUpperRange_1111(lower, upper, h);
}
range<t_ind_0::iterator> Type::lowerUpperRange_0100(const t_tuple &lower,
                                                    const t_tuple &upper,
                                                    context &h) const {
  t_comparator_0 comparator;
  int cmp = comparator(lower, upper);
  if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
  }
  return make_range(ind_0.lower_bound(lower, h.hints_0_lower),
                    ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator>
Type::lowerUpperRange_0100(const t_tuple &lower, const t_tuple &upper) const {
  context h;
  return lowerUpperRange_0100(lower, upper, h);
}
bool Type::empty() const { return ind_0.empty(); }
std::vector<range<iterator>> Type::partition() const {
  return ind_0.getChunks(400);
}
void Type::purge() { ind_0.clear(); }
iterator Type::begin() const { return ind_0.begin(); }
iterator Type::end() const { return ind_0.end(); }
void Type::printStatistics(std::ostream &o) const {
  o << " arity 4 direct b-tree index 0 lex-order [1,0,2,3]\n";
  ind_0.printStats(o);
}
} // namespace souffle::t_btree_uiiu__1_0_2_3__1111__0100
namespace souffle::t_btree_uuu__0_1_2__110__111 {
using namespace souffle;
struct Type {
  static constexpr Relation::arity_type Arity = 3;
  using t_tuple = Tuple<RamDomain, 3>;
  struct t_comparator_0 {
    int operator()(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamUnsigned>(a[0]) < ramBitCast<RamUnsigned>(b[0]))
                 ? -1
             : (ramBitCast<RamUnsigned>(a[0]) > ramBitCast<RamUnsigned>(b[0]))
                 ? 1
                 : ((ramBitCast<RamUnsigned>(a[1]) <
                     ramBitCast<RamUnsigned>(b[1]))
                        ? -1
                    : (ramBitCast<RamUnsigned>(a[1]) >
                       ramBitCast<RamUnsigned>(b[1]))
                        ? 1
                        : ((ramBitCast<RamUnsigned>(a[2]) <
                            ramBitCast<RamUnsigned>(b[2]))
                               ? -1
                           : (ramBitCast<RamUnsigned>(a[2]) >
                              ramBitCast<RamUnsigned>(b[2]))
                               ? 1
                               : (0)));
    }
    bool less(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamUnsigned>(a[0]) < ramBitCast<RamUnsigned>(b[0])) ||
             ((ramBitCast<RamUnsigned>(a[0]) ==
               ramBitCast<RamUnsigned>(b[0])) &&
              ((ramBitCast<RamUnsigned>(a[1]) <
                ramBitCast<RamUnsigned>(b[1])) ||
               ((ramBitCast<RamUnsigned>(a[1]) ==
                 ramBitCast<RamUnsigned>(b[1])) &&
                ((ramBitCast<RamUnsigned>(a[2]) <
                  ramBitCast<RamUnsigned>(b[2]))))));
    }
    bool equal(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamUnsigned>(a[0]) == ramBitCast<RamUnsigned>(b[0])) &&
             (ramBitCast<RamUnsigned>(a[1]) == ramBitCast<RamUnsigned>(b[1])) &&
             (ramBitCast<RamUnsigned>(a[2]) == ramBitCast<RamUnsigned>(b[2]));
    }
  };
  using t_ind_0 = btree_set<t_tuple, t_comparator_0>;
  t_ind_0 ind_0;
  using iterator = t_ind_0::iterator;
  struct context {
    t_ind_0::operation_hints hints_0_lower;
    t_ind_0::operation_hints hints_0_upper;
  };
  context createContext() { return context(); }
  bool insert(const t_tuple &t);
  bool insert(const t_tuple &t, context &h);
  bool insert(const RamDomain *ramDomain);
  bool insert(RamDomain a0, RamDomain a1, RamDomain a2);
  bool contains(const t_tuple &t, context &h) const;
  bool contains(const t_tuple &t) const;
  std::size_t size() const;
  iterator find(const t_tuple &t, context &h) const;
  iterator find(const t_tuple &t) const;
  range<iterator> lowerUpperRange_000(const t_tuple & /* lower */,
                                      const t_tuple & /* upper */,
                                      context & /* h */) const;
  range<iterator> lowerUpperRange_000(const t_tuple & /* lower */,
                                      const t_tuple & /* upper */) const;
  range<t_ind_0::iterator> lowerUpperRange_110(const t_tuple &lower,
                                               const t_tuple &upper,
                                               context &h) const;
  range<t_ind_0::iterator> lowerUpperRange_110(const t_tuple &lower,
                                               const t_tuple &upper) const;
  range<t_ind_0::iterator> lowerUpperRange_111(const t_tuple &lower,
                                               const t_tuple &upper,
                                               context &h) const;
  range<t_ind_0::iterator> lowerUpperRange_111(const t_tuple &lower,
                                               const t_tuple &upper) const;
  bool empty() const;
  std::vector<range<iterator>> partition() const;
  void purge();
  iterator begin() const;
  iterator end() const;
  void printStatistics(std::ostream &o) const;
};
} // namespace souffle::t_btree_uuu__0_1_2__110__111
namespace souffle::t_btree_uuu__0_1_2__110__111 {
using namespace souffle;
using t_ind_0 = Type::t_ind_0;
using iterator = Type::iterator;
using context = Type::context;
bool Type::insert(const t_tuple &t) {
  context h;
  return insert(t, h);
}
bool Type::insert(const t_tuple &t, context &h) {
  if (ind_0.insert(t, h.hints_0_lower)) {
    return true;
  } else
    return false;
}
bool Type::insert(const RamDomain *ramDomain) {
  RamDomain data[3];
  std::copy(ramDomain, ramDomain + 3, data);
  const t_tuple &tuple = reinterpret_cast<const t_tuple &>(data);
  context h;
  return insert(tuple, h);
}
bool Type::insert(RamDomain a0, RamDomain a1, RamDomain a2) {
  RamDomain data[3] = {a0, a1, a2};
  return insert(data);
}
bool Type::contains(const t_tuple &t, context &h) const {
  return ind_0.contains(t, h.hints_0_lower);
}
bool Type::contains(const t_tuple &t) const {
  context h;
  return contains(t, h);
}
std::size_t Type::size() const { return ind_0.size(); }
iterator Type::find(const t_tuple &t, context &h) const {
  return ind_0.find(t, h.hints_0_lower);
}
iterator Type::find(const t_tuple &t) const {
  context h;
  return find(t, h);
}
range<iterator> Type::lowerUpperRange_000(const t_tuple & /* lower */,
                                          const t_tuple & /* upper */,
                                          context & /* h */) const {
  return range<iterator>(ind_0.begin(), ind_0.end());
}
range<iterator> Type::lowerUpperRange_000(const t_tuple & /* lower */,
                                          const t_tuple & /* upper */) const {
  return range<iterator>(ind_0.begin(), ind_0.end());
}
range<t_ind_0::iterator> Type::lowerUpperRange_110(const t_tuple &lower,
                                                   const t_tuple &upper,
                                                   context &h) const {
  t_comparator_0 comparator;
  int cmp = comparator(lower, upper);
  if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
  }
  return make_range(ind_0.lower_bound(lower, h.hints_0_lower),
                    ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_110(const t_tuple &lower,
                                                   const t_tuple &upper) const {
  context h;
  return lowerUpperRange_110(lower, upper, h);
}
range<t_ind_0::iterator> Type::lowerUpperRange_111(const t_tuple &lower,
                                                   const t_tuple &upper,
                                                   context &h) const {
  t_comparator_0 comparator;
  int cmp = comparator(lower, upper);
  if (cmp == 0) {
    auto pos = ind_0.find(lower, h.hints_0_lower);
    auto fin = ind_0.end();
    if (pos != fin) {
      fin = pos;
      ++fin;
    }
    return make_range(pos, fin);
  }
  if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
  }
  return make_range(ind_0.lower_bound(lower, h.hints_0_lower),
                    ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_111(const t_tuple &lower,
                                                   const t_tuple &upper) const {
  context h;
  return lowerUpperRange_111(lower, upper, h);
}
bool Type::empty() const { return ind_0.empty(); }
std::vector<range<iterator>> Type::partition() const {
  return ind_0.getChunks(400);
}
void Type::purge() { ind_0.clear(); }
iterator Type::begin() const { return ind_0.begin(); }
iterator Type::end() const { return ind_0.end(); }
void Type::printStatistics(std::ostream &o) const {
  o << " arity 3 direct b-tree index 0 lex-order [0,1,2]\n";
  ind_0.printStats(o);
}
} // namespace souffle::t_btree_uuu__0_1_2__110__111
namespace souffle::t_btree_u__0__1 {
using namespace souffle;
struct Type {
  static constexpr Relation::arity_type Arity = 1;
  using t_tuple = Tuple<RamDomain, 1>;
  struct t_comparator_0 {
    int operator()(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamUnsigned>(a[0]) < ramBitCast<RamUnsigned>(b[0]))
                 ? -1
             : (ramBitCast<RamUnsigned>(a[0]) > ramBitCast<RamUnsigned>(b[0]))
                 ? 1
                 : (0);
    }
    bool less(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamUnsigned>(a[0]) < ramBitCast<RamUnsigned>(b[0]));
    }
    bool equal(const t_tuple &a, const t_tuple &b) const {
      return (ramBitCast<RamUnsigned>(a[0]) == ramBitCast<RamUnsigned>(b[0]));
    }
  };
  using t_ind_0 = btree_set<t_tuple, t_comparator_0>;
  t_ind_0 ind_0;
  using iterator = t_ind_0::iterator;
  struct context {
    t_ind_0::operation_hints hints_0_lower;
    t_ind_0::operation_hints hints_0_upper;
  };
  context createContext() { return context(); }
  bool insert(const t_tuple &t);
  bool insert(const t_tuple &t, context &h);
  bool insert(const RamDomain *ramDomain);
  bool insert(RamDomain a0);
  bool contains(const t_tuple &t, context &h) const;
  bool contains(const t_tuple &t) const;
  std::size_t size() const;
  iterator find(const t_tuple &t, context &h) const;
  iterator find(const t_tuple &t) const;
  range<iterator> lowerUpperRange_0(const t_tuple & /* lower */,
                                    const t_tuple & /* upper */,
                                    context & /* h */) const;
  range<iterator> lowerUpperRange_0(const t_tuple & /* lower */,
                                    const t_tuple & /* upper */) const;
  range<t_ind_0::iterator> lowerUpperRange_1(const t_tuple &lower,
                                             const t_tuple &upper,
                                             context &h) const;
  range<t_ind_0::iterator> lowerUpperRange_1(const t_tuple &lower,
                                             const t_tuple &upper) const;
  bool empty() const;
  std::vector<range<iterator>> partition() const;
  void purge();
  iterator begin() const;
  iterator end() const;
  void printStatistics(std::ostream &o) const;
};
} // namespace souffle::t_btree_u__0__1
namespace souffle::t_btree_u__0__1 {
using namespace souffle;
using t_ind_0 = Type::t_ind_0;
using iterator = Type::iterator;
using context = Type::context;
bool Type::insert(const t_tuple &t) {
  context h;
  return insert(t, h);
}
bool Type::insert(const t_tuple &t, context &h) {
  if (ind_0.insert(t, h.hints_0_lower)) {
    return true;
  } else
    return false;
}
bool Type::insert(const RamDomain *ramDomain) {
  RamDomain data[1];
  std::copy(ramDomain, ramDomain + 1, data);
  const t_tuple &tuple = reinterpret_cast<const t_tuple &>(data);
  context h;
  return insert(tuple, h);
}
bool Type::insert(RamDomain a0) {
  RamDomain data[1] = {a0};
  return insert(data);
}
bool Type::contains(const t_tuple &t, context &h) const {
  return ind_0.contains(t, h.hints_0_lower);
}
bool Type::contains(const t_tuple &t) const {
  context h;
  return contains(t, h);
}
std::size_t Type::size() const { return ind_0.size(); }
iterator Type::find(const t_tuple &t, context &h) const {
  return ind_0.find(t, h.hints_0_lower);
}
iterator Type::find(const t_tuple &t) const {
  context h;
  return find(t, h);
}
range<iterator> Type::lowerUpperRange_0(const t_tuple & /* lower */,
                                        const t_tuple & /* upper */,
                                        context & /* h */) const {
  return range<iterator>(ind_0.begin(), ind_0.end());
}
range<iterator> Type::lowerUpperRange_0(const t_tuple & /* lower */,
                                        const t_tuple & /* upper */) const {
  return range<iterator>(ind_0.begin(), ind_0.end());
}
range<t_ind_0::iterator> Type::lowerUpperRange_1(const t_tuple &lower,
                                                 const t_tuple &upper,
                                                 context &h) const {
  t_comparator_0 comparator;
  int cmp = comparator(lower, upper);
  if (cmp == 0) {
    auto pos = ind_0.find(lower, h.hints_0_lower);
    auto fin = ind_0.end();
    if (pos != fin) {
      fin = pos;
      ++fin;
    }
    return make_range(pos, fin);
  }
  if (cmp > 0) {
    return make_range(ind_0.end(), ind_0.end());
  }
  return make_range(ind_0.lower_bound(lower, h.hints_0_lower),
                    ind_0.upper_bound(upper, h.hints_0_upper));
}
range<t_ind_0::iterator> Type::lowerUpperRange_1(const t_tuple &lower,
                                                 const t_tuple &upper) const {
  context h;
  return lowerUpperRange_1(lower, upper, h);
}
bool Type::empty() const { return ind_0.empty(); }
std::vector<range<iterator>> Type::partition() const {
  return ind_0.getChunks(400);
}
void Type::purge() { ind_0.clear(); }
iterator Type::begin() const { return ind_0.begin(); }
iterator Type::end() const { return ind_0.end(); }
void Type::printStatistics(std::ostream &o) const {
  o << " arity 1 direct b-tree index 0 lex-order [0]\n";
  ind_0.printStats(o);
}
} // namespace souffle::t_btree_u__0__1
namespace souffle {
using namespace souffle;
class Stratum_Instruction_10d006adf06ef128 {
public:
  Stratum_Instruction_10d006adf06ef128(
      SymbolTable &symTable, RecordTable &recordTable,
      ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
      bool &performIO, SignalHandler *&signalHandler,
      std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
      std::string &inputDirectory, std::string &outputDirectory,
      t_btree_uiiu__1_0_2_3__1111__0100::Type
          &rel_Instruction_b9eae3fec8604f9a);
  void run([[maybe_unused]] const std::vector<RamDomain> &args,
           [[maybe_unused]] std::vector<RamDomain> &ret);

private:
  SymbolTable &symTable;
  RecordTable &recordTable;
  ConcurrentCache<std::string, std::regex> &regexCache;
  bool &pruneImdtRels;
  bool &performIO;
  SignalHandler *&signalHandler;
  std::atomic<std::size_t> &iter;
  std::atomic<RamDomain> &ctr;
  std::string &inputDirectory;
  std::string &outputDirectory;
  t_btree_uiiu__1_0_2_3__1111__0100::Type *rel_Instruction_b9eae3fec8604f9a;
};
} // namespace  souffle
namespace souffle {
using namespace souffle;
Stratum_Instruction_10d006adf06ef128::Stratum_Instruction_10d006adf06ef128(
    SymbolTable &symTable, RecordTable &recordTable,
    ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
    bool &performIO, SignalHandler *&signalHandler,
    std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
    std::string &inputDirectory, std::string &outputDirectory,
    t_btree_uiiu__1_0_2_3__1111__0100::Type &rel_Instruction_b9eae3fec8604f9a)
    : symTable(symTable), recordTable(recordTable), regexCache(regexCache),
      pruneImdtRels(pruneImdtRels), performIO(performIO),
      signalHandler(signalHandler), iter(iter), ctr(ctr),
      inputDirectory(inputDirectory), outputDirectory(outputDirectory),
      rel_Instruction_b9eae3fec8604f9a(&rel_Instruction_b9eae3fec8604f9a) {}

void Stratum_Instruction_10d006adf06ef128::run(
    [[maybe_unused]] const std::vector<RamDomain> &args,
    [[maybe_unused]] std::vector<RamDomain> &ret) {
  if (performIO) {
    try {
      std::map<std::string, std::string> directiveMap(
          {{"IO", "file"},
           {"attributeNames", "id\topcode\tname\tblock"},
           {"auxArity", "0"},
           {"fact-dir", "."},
           {"name", "Instruction"},
           {"operation", "input"},
           {"params",
            "{\"records\": {}, \"relation\": {\"arity\": 4, \"params\": "
            "[\"id\", \"opcode\", \"name\", \"block\"]}}"},
           {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": "
                     "{\"arity\": 4, \"types\": [\"u:vid\", \"s:symbol\", "
                     "\"s:symbol\", \"u:vid\"]}}"}});
      if (!inputDirectory.empty()) {
        directiveMap["fact-dir"] = inputDirectory;
      }
      IOSystem::getInstance()
          .getReader(directiveMap, symTable, recordTable)
          ->readAll(*rel_Instruction_b9eae3fec8604f9a);
    } catch (std::exception &e) {
      std::cerr << "Error loading Instruction data: " << e.what() << '\n';
      exit(1);
    }
  }
}

} // namespace  souffle

namespace souffle {
using namespace souffle;
class Stratum_Operand_a87ce00b6226edca {
public:
  Stratum_Operand_a87ce00b6226edca(
      SymbolTable &symTable, RecordTable &recordTable,
      ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
      bool &performIO, SignalHandler *&signalHandler,
      std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
      std::string &inputDirectory, std::string &outputDirectory,
      t_btree_uuu__0_1_2__110__111::Type &rel_Operand_f2644e1d53a897f3);
  void run([[maybe_unused]] const std::vector<RamDomain> &args,
           [[maybe_unused]] std::vector<RamDomain> &ret);

private:
  SymbolTable &symTable;
  RecordTable &recordTable;
  ConcurrentCache<std::string, std::regex> &regexCache;
  bool &pruneImdtRels;
  bool &performIO;
  SignalHandler *&signalHandler;
  std::atomic<std::size_t> &iter;
  std::atomic<RamDomain> &ctr;
  std::string &inputDirectory;
  std::string &outputDirectory;
  t_btree_uuu__0_1_2__110__111::Type *rel_Operand_f2644e1d53a897f3;
};
} // namespace  souffle
namespace souffle {
using namespace souffle;
Stratum_Operand_a87ce00b6226edca::Stratum_Operand_a87ce00b6226edca(
    SymbolTable &symTable, RecordTable &recordTable,
    ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
    bool &performIO, SignalHandler *&signalHandler,
    std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
    std::string &inputDirectory, std::string &outputDirectory,
    t_btree_uuu__0_1_2__110__111::Type &rel_Operand_f2644e1d53a897f3)
    : symTable(symTable), recordTable(recordTable), regexCache(regexCache),
      pruneImdtRels(pruneImdtRels), performIO(performIO),
      signalHandler(signalHandler), iter(iter), ctr(ctr),
      inputDirectory(inputDirectory), outputDirectory(outputDirectory),
      rel_Operand_f2644e1d53a897f3(&rel_Operand_f2644e1d53a897f3) {}

void Stratum_Operand_a87ce00b6226edca::run(
    [[maybe_unused]] const std::vector<RamDomain> &args,
    [[maybe_unused]] std::vector<RamDomain> &ret) {
  if (performIO) {
    try {
      std::map<std::string, std::string> directiveMap(
          {{"IO", "file"},
           {"attributeNames", "inst\tidx\tvalue"},
           {"auxArity", "0"},
           {"fact-dir", "."},
           {"name", "Operand"},
           {"operation", "input"},
           {"params", "{\"records\": {}, \"relation\": {\"arity\": 3, "
                      "\"params\": [\"inst\", \"idx\", \"value\"]}}"},
           {"types",
            "{\"ADTs\": {}, \"records\": {}, \"relation\": {\"arity\": 3, "
            "\"types\": [\"u:vid\", \"u:unsigned\", \"u:vid\"]}}"}});
      if (!inputDirectory.empty()) {
        directiveMap["fact-dir"] = inputDirectory;
      }
      IOSystem::getInstance()
          .getReader(directiveMap, symTable, recordTable)
          ->readAll(*rel_Operand_f2644e1d53a897f3);
    } catch (std::exception &e) {
      std::cerr << "Error loading Operand data: " << e.what() << '\n';
      exit(1);
    }
  }
}

} // namespace  souffle

namespace souffle {
using namespace souffle;
class Stratum_isMemory_0821342b07e3d3fa {
public:
  Stratum_isMemory_0821342b07e3d3fa(
      SymbolTable &symTable, RecordTable &recordTable,
      ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
      bool &performIO, SignalHandler *&signalHandler,
      std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
      std::string &inputDirectory, std::string &outputDirectory,
      t_btree_u__0__1::Type &rel_isMemory_a06675b24aefb5ee);
  void run([[maybe_unused]] const std::vector<RamDomain> &args,
           [[maybe_unused]] std::vector<RamDomain> &ret);

private:
  SymbolTable &symTable;
  RecordTable &recordTable;
  ConcurrentCache<std::string, std::regex> &regexCache;
  bool &pruneImdtRels;
  bool &performIO;
  SignalHandler *&signalHandler;
  std::atomic<std::size_t> &iter;
  std::atomic<RamDomain> &ctr;
  std::string &inputDirectory;
  std::string &outputDirectory;
  t_btree_u__0__1::Type *rel_isMemory_a06675b24aefb5ee;
};
} // namespace  souffle
namespace souffle {
using namespace souffle;
Stratum_isMemory_0821342b07e3d3fa::Stratum_isMemory_0821342b07e3d3fa(
    SymbolTable &symTable, RecordTable &recordTable,
    ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
    bool &performIO, SignalHandler *&signalHandler,
    std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
    std::string &inputDirectory, std::string &outputDirectory,
    t_btree_u__0__1::Type &rel_isMemory_a06675b24aefb5ee)
    : symTable(symTable), recordTable(recordTable), regexCache(regexCache),
      pruneImdtRels(pruneImdtRels), performIO(performIO),
      signalHandler(signalHandler), iter(iter), ctr(ctr),
      inputDirectory(inputDirectory), outputDirectory(outputDirectory),
      rel_isMemory_a06675b24aefb5ee(&rel_isMemory_a06675b24aefb5ee) {}

void Stratum_isMemory_0821342b07e3d3fa::run(
    [[maybe_unused]] const std::vector<RamDomain> &args,
    [[maybe_unused]] std::vector<RamDomain> &ret) {
  if (performIO) {
    try {
      std::map<std::string, std::string> directiveMap(
          {{"IO", "file"},
           {"attributeNames", "val"},
           {"auxArity", "0"},
           {"fact-dir", "."},
           {"name", "isMemory"},
           {"operation", "input"},
           {"params", "{\"records\": {}, \"relation\": {\"arity\": 1, "
                      "\"params\": [\"val\"]}}"},
           {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": "
                     "{\"arity\": 1, \"types\": [\"u:vid\"]}}"}});
      if (!inputDirectory.empty()) {
        directiveMap["fact-dir"] = inputDirectory;
      }
      IOSystem::getInstance()
          .getReader(directiveMap, symTable, recordTable)
          ->readAll(*rel_isMemory_a06675b24aefb5ee);
    } catch (std::exception &e) {
      std::cerr << "Error loading isMemory data: " << e.what() << '\n';
      exit(1);
    }
  }
}

} // namespace  souffle

namespace souffle {
using namespace souffle;
class Stratum_isPointer_49fa31835b26f17f {
public:
  Stratum_isPointer_49fa31835b26f17f(
      SymbolTable &symTable, RecordTable &recordTable,
      ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
      bool &performIO, SignalHandler *&signalHandler,
      std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
      std::string &inputDirectory, std::string &outputDirectory,
      t_btree_uiiu__1_0_2_3__1111__0100::Type &rel_Instruction_b9eae3fec8604f9a,
      t_btree_uuu__0_1_2__110__111::Type &rel_Operand_f2644e1d53a897f3,
      t_btree_u__0__1::Type &rel_isMemory_a06675b24aefb5ee,
      t_btree_u__0__1::Type &rel_isPointer_7c4bb1448d21b2e8);
  void run([[maybe_unused]] const std::vector<RamDomain> &args,
           [[maybe_unused]] std::vector<RamDomain> &ret);

private:
  SymbolTable &symTable;
  RecordTable &recordTable;
  ConcurrentCache<std::string, std::regex> &regexCache;
  bool &pruneImdtRels;
  bool &performIO;
  SignalHandler *&signalHandler;
  std::atomic<std::size_t> &iter;
  std::atomic<RamDomain> &ctr;
  std::string &inputDirectory;
  std::string &outputDirectory;
  t_btree_uiiu__1_0_2_3__1111__0100::Type *rel_Instruction_b9eae3fec8604f9a;
  t_btree_uuu__0_1_2__110__111::Type *rel_Operand_f2644e1d53a897f3;
  t_btree_u__0__1::Type *rel_isMemory_a06675b24aefb5ee;
  t_btree_u__0__1::Type *rel_isPointer_7c4bb1448d21b2e8;
};
} // namespace  souffle
namespace souffle {
using namespace souffle;
Stratum_isPointer_49fa31835b26f17f::Stratum_isPointer_49fa31835b26f17f(
    SymbolTable &symTable, RecordTable &recordTable,
    ConcurrentCache<std::string, std::regex> &regexCache, bool &pruneImdtRels,
    bool &performIO, SignalHandler *&signalHandler,
    std::atomic<std::size_t> &iter, std::atomic<RamDomain> &ctr,
    std::string &inputDirectory, std::string &outputDirectory,
    t_btree_uiiu__1_0_2_3__1111__0100::Type &rel_Instruction_b9eae3fec8604f9a,
    t_btree_uuu__0_1_2__110__111::Type &rel_Operand_f2644e1d53a897f3,
    t_btree_u__0__1::Type &rel_isMemory_a06675b24aefb5ee,
    t_btree_u__0__1::Type &rel_isPointer_7c4bb1448d21b2e8)
    : symTable(symTable), recordTable(recordTable), regexCache(regexCache),
      pruneImdtRels(pruneImdtRels), performIO(performIO),
      signalHandler(signalHandler), iter(iter), ctr(ctr),
      inputDirectory(inputDirectory), outputDirectory(outputDirectory),
      rel_Instruction_b9eae3fec8604f9a(&rel_Instruction_b9eae3fec8604f9a),
      rel_Operand_f2644e1d53a897f3(&rel_Operand_f2644e1d53a897f3),
      rel_isMemory_a06675b24aefb5ee(&rel_isMemory_a06675b24aefb5ee),
      rel_isPointer_7c4bb1448d21b2e8(&rel_isPointer_7c4bb1448d21b2e8) {}

void Stratum_isPointer_49fa31835b26f17f::run(
    [[maybe_unused]] const std::vector<RamDomain> &args,
    [[maybe_unused]] std::vector<RamDomain> &ret) {
  if (performIO) {
    try {
      std::map<std::string, std::string> directiveMap(
          {{"IO", "file"},
           {"attributeNames", "val"},
           {"auxArity", "0"},
           {"fact-dir", "."},
           {"name", "isPointer"},
           {"operation", "input"},
           {"params", "{\"records\": {}, \"relation\": {\"arity\": 1, "
                      "\"params\": [\"val\"]}}"},
           {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": "
                     "{\"arity\": 1, \"types\": [\"u:vid\"]}}"}});
      if (!inputDirectory.empty()) {
        directiveMap["fact-dir"] = inputDirectory;
      }
      IOSystem::getInstance()
          .getReader(directiveMap, symTable, recordTable)
          ->readAll(*rel_isPointer_7c4bb1448d21b2e8);
    } catch (std::exception &e) {
      std::cerr << "Error loading isPointer data: " << e.what() << '\n';
      exit(1);
    }
  }
  signalHandler->setMsg(R"_(isPointer(X) :- 
   Instruction(id,"getelementptr",_,_),
   Operand(id,0,memId),
   isMemory(memId),
   Operand(id,1,X).
in file pointer.dl [9:1-13:23])_");
  if (!(rel_isMemory_a06675b24aefb5ee->empty()) &&
      !(rel_Instruction_b9eae3fec8604f9a->empty()) &&
      !(rel_Operand_f2644e1d53a897f3->empty())) {
    [&]() {
      CREATE_OP_CONTEXT(rel_Instruction_b9eae3fec8604f9a_op_ctxt,
                        rel_Instruction_b9eae3fec8604f9a->createContext());
      CREATE_OP_CONTEXT(rel_Operand_f2644e1d53a897f3_op_ctxt,
                        rel_Operand_f2644e1d53a897f3->createContext());
      CREATE_OP_CONTEXT(rel_isMemory_a06675b24aefb5ee_op_ctxt,
                        rel_isMemory_a06675b24aefb5ee->createContext());
      CREATE_OP_CONTEXT(rel_isPointer_7c4bb1448d21b2e8_op_ctxt,
                        rel_isPointer_7c4bb1448d21b2e8->createContext());
      auto range = rel_Instruction_b9eae3fec8604f9a->lowerUpperRange_0100(
          Tuple<RamDomain, 4>{{ramBitCast<RamDomain>(MIN_RAM_UNSIGNED),
                               ramBitCast(RamSigned(0)),
                               ramBitCast<RamDomain>(MIN_RAM_SIGNED),
                               ramBitCast<RamDomain>(MIN_RAM_UNSIGNED)}},
          Tuple<RamDomain, 4>{{ramBitCast<RamDomain>(MAX_RAM_UNSIGNED),
                               ramBitCast(RamSigned(0)),
                               ramBitCast<RamDomain>(MAX_RAM_SIGNED),
                               ramBitCast<RamDomain>(MAX_RAM_UNSIGNED)}},
          READ_OP_CONTEXT(rel_Instruction_b9eae3fec8604f9a_op_ctxt));
      for (const auto &env0 : range) {
        auto range = rel_Operand_f2644e1d53a897f3->lowerUpperRange_110(
            Tuple<RamDomain, 3>{{ramBitCast(env0[0]),
                                 ramBitCast(RamUnsigned(0)),
                                 ramBitCast<RamDomain>(MIN_RAM_UNSIGNED)}},
            Tuple<RamDomain, 3>{{ramBitCast(env0[0]),
                                 ramBitCast(RamUnsigned(0)),
                                 ramBitCast<RamDomain>(MAX_RAM_UNSIGNED)}},
            READ_OP_CONTEXT(rel_Operand_f2644e1d53a897f3_op_ctxt));
        for (const auto &env1 : range) {
          if (rel_isMemory_a06675b24aefb5ee->contains(
                  Tuple<RamDomain, 1>{{ramBitCast(env1[2])}},
                  READ_OP_CONTEXT(rel_isMemory_a06675b24aefb5ee_op_ctxt))) {
            auto range = rel_Operand_f2644e1d53a897f3->lowerUpperRange_110(
                Tuple<RamDomain, 3>{{ramBitCast(env0[0]),
                                     ramBitCast(RamUnsigned(1)),
                                     ramBitCast<RamDomain>(MIN_RAM_UNSIGNED)}},
                Tuple<RamDomain, 3>{{ramBitCast(env0[0]),
                                     ramBitCast(RamUnsigned(1)),
                                     ramBitCast<RamDomain>(MAX_RAM_UNSIGNED)}},
                READ_OP_CONTEXT(rel_Operand_f2644e1d53a897f3_op_ctxt));
            for (const auto &env2 : range) {
              Tuple<RamDomain, 1> tuple{{ramBitCast(env2[2])}};
              rel_isPointer_7c4bb1448d21b2e8->insert(
                  tuple,
                  READ_OP_CONTEXT(rel_isPointer_7c4bb1448d21b2e8_op_ctxt));
            }
            break;
          }
        }
      }
    }();
  }
  if (performIO) {
    try {
      std::map<std::string, std::string> directiveMap(
          {{"IO", "file"},
           {"attributeNames", "val"},
           {"auxArity", "0"},
           {"name", "isPointer"},
           {"operation", "output"},
           {"output-dir", "."},
           {"params", "{\"records\": {}, \"relation\": {\"arity\": 1, "
                      "\"params\": [\"val\"]}}"},
           {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": "
                     "{\"arity\": 1, \"types\": [\"u:vid\"]}}"}});
      if (outputDirectory == "-") {
        directiveMap["IO"] = "stdout";
        directiveMap["headers"] = "true";
      } else if (!outputDirectory.empty()) {
        directiveMap["output-dir"] = outputDirectory;
      }
      IOSystem::getInstance()
          .getWriter(directiveMap, symTable, recordTable)
          ->writeAll(*rel_isPointer_7c4bb1448d21b2e8);
    } catch (std::exception &e) {
      std::cerr << e.what();
      exit(1);
    }
  }
  if (pruneImdtRels)
    rel_Instruction_b9eae3fec8604f9a->purge();
  if (pruneImdtRels)
    rel_Operand_f2644e1d53a897f3->purge();
  if (pruneImdtRels)
    rel_isMemory_a06675b24aefb5ee->purge();
}

} // namespace  souffle

namespace souffle {
using namespace souffle;
class Sf_stack : public SouffleProgram {
public:
  Sf_stack();
  ~Sf_stack();
  void run();
  void runAll(std::string inputDirectoryArg = "",
              std::string outputDirectoryArg = "", bool performIOArg = true,
              bool pruneImdtRelsArg = true);
  void printAll([[maybe_unused]] std::string outputDirectoryArg = "");
  void loadAll([[maybe_unused]] std::string inputDirectoryArg = "");
  void dumpInputs();
  void dumpOutputs();
  SymbolTable &getSymbolTable();
  RecordTable &getRecordTable();
  void setNumThreads(std::size_t numThreadsValue);
  void executeSubroutine(std::string name, const std::vector<RamDomain> &args,
                         std::vector<RamDomain> &ret);

private:
  void runFunction(std::string inputDirectoryArg,
                   std::string outputDirectoryArg, bool performIOArg,
                   bool pruneImdtRelsArg);
  SymbolTableImpl symTable;
  SpecializedRecordTable<0> recordTable;
  ConcurrentCache<std::string, std::regex> regexCache;
  Own<t_btree_uiiu__1_0_2_3__1111__0100::Type> rel_Instruction_b9eae3fec8604f9a;
  souffle::RelationWrapper<t_btree_uiiu__1_0_2_3__1111__0100::Type>
      wrapper_rel_Instruction_b9eae3fec8604f9a;
  Own<t_btree_uuu__0_1_2__110__111::Type> rel_Operand_f2644e1d53a897f3;
  souffle::RelationWrapper<t_btree_uuu__0_1_2__110__111::Type>
      wrapper_rel_Operand_f2644e1d53a897f3;
  Own<t_btree_u__0__1::Type> rel_isMemory_a06675b24aefb5ee;
  souffle::RelationWrapper<t_btree_u__0__1::Type>
      wrapper_rel_isMemory_a06675b24aefb5ee;
  Own<t_btree_u__0__1::Type> rel_isPointer_7c4bb1448d21b2e8;
  souffle::RelationWrapper<t_btree_u__0__1::Type>
      wrapper_rel_isPointer_7c4bb1448d21b2e8;
  Stratum_Instruction_10d006adf06ef128 stratum_Instruction_3566c2a3b844a3b6;
  Stratum_Operand_a87ce00b6226edca stratum_Operand_b6c578eff44e0da7;
  Stratum_isMemory_0821342b07e3d3fa stratum_isMemory_6aec158966be286c;
  Stratum_isPointer_49fa31835b26f17f stratum_isPointer_ca77d06b67edb5d9;
  std::string inputDirectory;
  std::string outputDirectory;
  SignalHandler *signalHandler{SignalHandler::instance()};
  std::atomic<RamDomain> ctr{};
  std::atomic<std::size_t> iter{};
};
} // namespace  souffle
namespace souffle {
using namespace souffle;
Sf_stack::Sf_stack()
    : symTable({
          R"_(getelementptr)_",
      }),
      recordTable(), regexCache(),
      rel_Instruction_b9eae3fec8604f9a(
          mk<t_btree_uiiu__1_0_2_3__1111__0100::Type>()),
      wrapper_rel_Instruction_b9eae3fec8604f9a(
          0, *rel_Instruction_b9eae3fec8604f9a, *this, "Instruction",
          std::array<const char *, 4>{
              {"u:vid", "s:symbol", "s:symbol", "u:vid"}},
          std::array<const char *, 4>{{"id", "opcode", "name", "block"}}, 0),
      rel_Operand_f2644e1d53a897f3(mk<t_btree_uuu__0_1_2__110__111::Type>()),
      wrapper_rel_Operand_f2644e1d53a897f3(
          1, *rel_Operand_f2644e1d53a897f3, *this, "Operand",
          std::array<const char *, 3>{{"u:vid", "u:unsigned", "u:vid"}},
          std::array<const char *, 3>{{"inst", "idx", "value"}}, 0),
      rel_isMemory_a06675b24aefb5ee(mk<t_btree_u__0__1::Type>()),
      wrapper_rel_isMemory_a06675b24aefb5ee(
          2, *rel_isMemory_a06675b24aefb5ee, *this, "isMemory",
          std::array<const char *, 1>{{"u:vid"}},
          std::array<const char *, 1>{{"val"}}, 0),
      rel_isPointer_7c4bb1448d21b2e8(mk<t_btree_u__0__1::Type>()),
      wrapper_rel_isPointer_7c4bb1448d21b2e8(
          3, *rel_isPointer_7c4bb1448d21b2e8, *this, "isPointer",
          std::array<const char *, 1>{{"u:vid"}},
          std::array<const char *, 1>{{"val"}}, 0),
      stratum_Instruction_3566c2a3b844a3b6(
          symTable, recordTable, regexCache, pruneImdtRels, performIO,
          signalHandler, iter, ctr, inputDirectory, outputDirectory,
          *rel_Instruction_b9eae3fec8604f9a),
      stratum_Operand_b6c578eff44e0da7(
          symTable, recordTable, regexCache, pruneImdtRels, performIO,
          signalHandler, iter, ctr, inputDirectory, outputDirectory,
          *rel_Operand_f2644e1d53a897f3),
      stratum_isMemory_6aec158966be286c(
          symTable, recordTable, regexCache, pruneImdtRels, performIO,
          signalHandler, iter, ctr, inputDirectory, outputDirectory,
          *rel_isMemory_a06675b24aefb5ee),
      stratum_isPointer_ca77d06b67edb5d9(
          symTable, recordTable, regexCache, pruneImdtRels, performIO,
          signalHandler, iter, ctr, inputDirectory, outputDirectory,
          *rel_Instruction_b9eae3fec8604f9a, *rel_Operand_f2644e1d53a897f3,
          *rel_isMemory_a06675b24aefb5ee, *rel_isPointer_7c4bb1448d21b2e8) {
  addRelation("Instruction", wrapper_rel_Instruction_b9eae3fec8604f9a, true,
              false);
  addRelation("Operand", wrapper_rel_Operand_f2644e1d53a897f3, true, false);
  addRelation("isMemory", wrapper_rel_isMemory_a06675b24aefb5ee, true, false);
  addRelation("isPointer", wrapper_rel_isPointer_7c4bb1448d21b2e8, true, true);
}

Sf_stack::~Sf_stack() {}

void Sf_stack::runFunction(std::string inputDirectoryArg,
                           std::string outputDirectoryArg, bool performIOArg,
                           bool pruneImdtRelsArg) {

  this->inputDirectory = std::move(inputDirectoryArg);
  this->outputDirectory = std::move(outputDirectoryArg);
  this->performIO = performIOArg;
  this->pruneImdtRels = pruneImdtRelsArg;

  // set default threads (in embedded mode)
  // if this is not set, and omp is used, the default omp setting of number of
  // cores is used.
#if defined(_OPENMP)
  if (0 < getNumThreads()) {
    omp_set_num_threads(static_cast<int>(getNumThreads()));
  }
#endif

  signalHandler->set();
  // -- query evaluation --
  {
    std::vector<RamDomain> args, ret;
    stratum_Instruction_3566c2a3b844a3b6.run(args, ret);
  }
  {
    std::vector<RamDomain> args, ret;
    stratum_Operand_b6c578eff44e0da7.run(args, ret);
  }
  {
    std::vector<RamDomain> args, ret;
    stratum_isMemory_6aec158966be286c.run(args, ret);
  }
  {
    std::vector<RamDomain> args, ret;
    stratum_isPointer_ca77d06b67edb5d9.run(args, ret);
  }

  // -- relation hint statistics --
  signalHandler->reset();
}

void Sf_stack::run() { runFunction("", "", false, false); }

void Sf_stack::runAll(std::string inputDirectoryArg,
                      std::string outputDirectoryArg, bool performIOArg,
                      bool pruneImdtRelsArg) {
  runFunction(inputDirectoryArg, outputDirectoryArg, performIOArg,
              pruneImdtRelsArg);
}

void Sf_stack::printAll([[maybe_unused]] std::string outputDirectoryArg) {
  try {
    std::map<std::string, std::string> directiveMap(
        {{"IO", "file"},
         {"attributeNames", "val"},
         {"auxArity", "0"},
         {"name", "isPointer"},
         {"operation", "output"},
         {"output-dir", "."},
         {"params", "{\"records\": {}, \"relation\": {\"arity\": 1, "
                    "\"params\": [\"val\"]}}"},
         {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": {\"arity\": "
                   "1, \"types\": [\"u:vid\"]}}"}});
    if (!outputDirectoryArg.empty()) {
      directiveMap["output-dir"] = outputDirectoryArg;
    }
    IOSystem::getInstance()
        .getWriter(directiveMap, symTable, recordTable)
        ->writeAll(*rel_isPointer_7c4bb1448d21b2e8);
  } catch (std::exception &e) {
    std::cerr << e.what();
    exit(1);
  }
}

void Sf_stack::loadAll([[maybe_unused]] std::string inputDirectoryArg) {
  try {
    std::map<std::string, std::string> directiveMap(
        {{"IO", "file"},
         {"attributeNames", "val"},
         {"auxArity", "0"},
         {"fact-dir", "."},
         {"name", "isPointer"},
         {"operation", "input"},
         {"params", "{\"records\": {}, \"relation\": {\"arity\": 1, "
                    "\"params\": [\"val\"]}}"},
         {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": {\"arity\": "
                   "1, \"types\": [\"u:vid\"]}}"}});
    if (!inputDirectoryArg.empty()) {
      directiveMap["fact-dir"] = inputDirectoryArg;
    }
    IOSystem::getInstance()
        .getReader(directiveMap, symTable, recordTable)
        ->readAll(*rel_isPointer_7c4bb1448d21b2e8);
  } catch (std::exception &e) {
    std::cerr << "Error loading isPointer data: " << e.what() << '\n';
    exit(1);
  }
  try {
    std::map<std::string, std::string> directiveMap(
        {{"IO", "file"},
         {"attributeNames", "id\topcode\tname\tblock"},
         {"auxArity", "0"},
         {"fact-dir", "."},
         {"name", "Instruction"},
         {"operation", "input"},
         {"params", "{\"records\": {}, \"relation\": {\"arity\": 4, "
                    "\"params\": [\"id\", \"opcode\", \"name\", \"block\"]}}"},
         {"types",
          "{\"ADTs\": {}, \"records\": {}, \"relation\": {\"arity\": 4, "
          "\"types\": [\"u:vid\", \"s:symbol\", \"s:symbol\", \"u:vid\"]}}"}});
    if (!inputDirectoryArg.empty()) {
      directiveMap["fact-dir"] = inputDirectoryArg;
    }
    IOSystem::getInstance()
        .getReader(directiveMap, symTable, recordTable)
        ->readAll(*rel_Instruction_b9eae3fec8604f9a);
  } catch (std::exception &e) {
    std::cerr << "Error loading Instruction data: " << e.what() << '\n';
    exit(1);
  }
  try {
    std::map<std::string, std::string> directiveMap(
        {{"IO", "file"},
         {"attributeNames", "val"},
         {"auxArity", "0"},
         {"fact-dir", "."},
         {"name", "isMemory"},
         {"operation", "input"},
         {"params", "{\"records\": {}, \"relation\": {\"arity\": 1, "
                    "\"params\": [\"val\"]}}"},
         {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": {\"arity\": "
                   "1, \"types\": [\"u:vid\"]}}"}});
    if (!inputDirectoryArg.empty()) {
      directiveMap["fact-dir"] = inputDirectoryArg;
    }
    IOSystem::getInstance()
        .getReader(directiveMap, symTable, recordTable)
        ->readAll(*rel_isMemory_a06675b24aefb5ee);
  } catch (std::exception &e) {
    std::cerr << "Error loading isMemory data: " << e.what() << '\n';
    exit(1);
  }
  try {
    std::map<std::string, std::string> directiveMap(
        {{"IO", "file"},
         {"attributeNames", "inst\tidx\tvalue"},
         {"auxArity", "0"},
         {"fact-dir", "."},
         {"name", "Operand"},
         {"operation", "input"},
         {"params", "{\"records\": {}, \"relation\": {\"arity\": 3, "
                    "\"params\": [\"inst\", \"idx\", \"value\"]}}"},
         {"types", "{\"ADTs\": {}, \"records\": {}, \"relation\": {\"arity\": "
                   "3, \"types\": [\"u:vid\", \"u:unsigned\", \"u:vid\"]}}"}});
    if (!inputDirectoryArg.empty()) {
      directiveMap["fact-dir"] = inputDirectoryArg;
    }
    IOSystem::getInstance()
        .getReader(directiveMap, symTable, recordTable)
        ->readAll(*rel_Operand_f2644e1d53a897f3);
  } catch (std::exception &e) {
    std::cerr << "Error loading Operand data: " << e.what() << '\n';
    exit(1);
  }
}

void Sf_stack::dumpInputs() {
  try {
    std::map<std::string, std::string> rwOperation;
    rwOperation["IO"] = "stdout";
    rwOperation["name"] = "isPointer";
    rwOperation["types"] = "{\"relation\": {\"arity\": 1, \"auxArity\": 0, "
                           "\"types\": [\"u:vid\"]}}";
    IOSystem::getInstance()
        .getWriter(rwOperation, symTable, recordTable)
        ->writeAll(*rel_isPointer_7c4bb1448d21b2e8);
  } catch (std::exception &e) {
    std::cerr << e.what();
    exit(1);
  }
  try {
    std::map<std::string, std::string> rwOperation;
    rwOperation["IO"] = "stdout";
    rwOperation["name"] = "Instruction";
    rwOperation["types"] =
        "{\"relation\": {\"arity\": 4, \"auxArity\": 0, \"types\": [\"u:vid\", "
        "\"s:symbol\", \"s:symbol\", \"u:vid\"]}}";
    IOSystem::getInstance()
        .getWriter(rwOperation, symTable, recordTable)
        ->writeAll(*rel_Instruction_b9eae3fec8604f9a);
  } catch (std::exception &e) {
    std::cerr << e.what();
    exit(1);
  }
  try {
    std::map<std::string, std::string> rwOperation;
    rwOperation["IO"] = "stdout";
    rwOperation["name"] = "isMemory";
    rwOperation["types"] = "{\"relation\": {\"arity\": 1, \"auxArity\": 0, "
                           "\"types\": [\"u:vid\"]}}";
    IOSystem::getInstance()
        .getWriter(rwOperation, symTable, recordTable)
        ->writeAll(*rel_isMemory_a06675b24aefb5ee);
  } catch (std::exception &e) {
    std::cerr << e.what();
    exit(1);
  }
  try {
    std::map<std::string, std::string> rwOperation;
    rwOperation["IO"] = "stdout";
    rwOperation["name"] = "Operand";
    rwOperation["types"] =
        "{\"relation\": {\"arity\": 3, \"auxArity\": 0, \"types\": [\"u:vid\", "
        "\"u:unsigned\", \"u:vid\"]}}";
    IOSystem::getInstance()
        .getWriter(rwOperation, symTable, recordTable)
        ->writeAll(*rel_Operand_f2644e1d53a897f3);
  } catch (std::exception &e) {
    std::cerr << e.what();
    exit(1);
  }
}

void Sf_stack::dumpOutputs() {
  try {
    std::map<std::string, std::string> rwOperation;
    rwOperation["IO"] = "stdout";
    rwOperation["name"] = "isPointer";
    rwOperation["types"] = "{\"relation\": {\"arity\": 1, \"auxArity\": 0, "
                           "\"types\": [\"u:vid\"]}}";
    IOSystem::getInstance()
        .getWriter(rwOperation, symTable, recordTable)
        ->writeAll(*rel_isPointer_7c4bb1448d21b2e8);
  } catch (std::exception &e) {
    std::cerr << e.what();
    exit(1);
  }
}

SymbolTable &Sf_stack::getSymbolTable() { return symTable; }

RecordTable &Sf_stack::getRecordTable() { return recordTable; }

void Sf_stack::setNumThreads(std::size_t numThreadsValue) {
  SouffleProgram::setNumThreads(numThreadsValue);
  symTable.setNumLanes(getNumThreads());
  recordTable.setNumLanes(getNumThreads());
  regexCache.setNumLanes(getNumThreads());
}

void Sf_stack::executeSubroutine(std::string name,
                                 const std::vector<RamDomain> &args,
                                 std::vector<RamDomain> &ret) {
  if (name == "Instruction") {
    stratum_Instruction_3566c2a3b844a3b6.run(args, ret);
    return;
  }
  if (name == "Operand") {
    stratum_Operand_b6c578eff44e0da7.run(args, ret);
    return;
  }
  if (name == "isMemory") {
    stratum_isMemory_6aec158966be286c.run(args, ret);
    return;
  }
  if (name == "isPointer") {
    stratum_isPointer_ca77d06b67edb5d9.run(args, ret);
    return;
  }
  fatal(("unknown subroutine " + name).c_str());
}

} // namespace  souffle
namespace souffle {
SouffleProgram *newInstance_stack() { return new souffle::Sf_stack; }
SymbolTable *getST_stack(SouffleProgram *p) {
  return &reinterpret_cast<souffle::Sf_stack *>(p)->getSymbolTable();
}
} // namespace souffle

#ifndef __EMBEDDED_SOUFFLE__
#include "souffle/CompiledOptions.h"
int main(int argc, char **argv) {
  try {
    souffle::CmdOptions opt(R"(pointer_main.dl)", R"()", R"()", false, R"()",
                            1);
    if (!opt.parse(argc, argv))
      return 1;
    souffle::Sf_stack obj;
#if defined(_OPENMP)
    obj.setNumThreads(opt.getNumJobs());

#endif
    obj.runAll(opt.getInputFileDir(), opt.getOutputFileDir());
    return 0;
  } catch (std::exception &e) {
    souffle::SignalHandler::instance()->error(e.what());
  }
}
#endif

namespace souffle {
using namespace souffle;
class factory_Sf_stack : souffle::ProgramFactory {
public:
  souffle::SouffleProgram *newInstance();
  factory_Sf_stack();

private:
};
} // namespace  souffle
namespace souffle {
using namespace souffle;
souffle::SouffleProgram *factory_Sf_stack::newInstance() {
  return new souffle::Sf_stack();
}

factory_Sf_stack::factory_Sf_stack() : souffle::ProgramFactory("stack") {}

} // namespace  souffle
namespace souffle {

#ifdef __EMBEDDED_SOUFFLE__
extern "C" {
souffle::factory_Sf_stack __factory_Sf_stack_instance;
}
#endif
} // namespace souffle
