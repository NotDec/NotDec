//===- CFG.h - Classes for representing and building CFGs -------*- C++ -*-===//
//===----------------------------------------------------------------------===//
//
// This file implements a simplified version of clang's CFG.
// 1. use C++ STL instead of Custom allocator.
// 2. Add utility functions to remove blocks.
//
//===----------------------------------------------------------------------===//

#ifndef _NOTDEC_BACKEND_CFG_H_
#define _NOTDEC_BACKEND_CFG_H_

#include <cassert>
#include <deque>
#include <list>
#include <set>
#include <utility>
#include <vector>

#include <clang/AST/Stmt.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/Support/raw_ostream.h>

namespace notdec::backend {

class CFGElement;
class CFGTerminator;
class CFG;

class CFGElement {
public:
  enum Kind {
    // main kind
    Initializer,
    ScopeBegin,
    ScopeEnd,
    NewAllocator,
    LifetimeEnds,
    LoopExit,
    // stmt kind
    Statement,
    Constructor,
    CXXRecordTypedCall,
    STMT_BEGIN = Statement,
    STMT_END = CXXRecordTypedCall,
    // dtor kind
    AutomaticObjectDtor,
    DeleteDtor,
    BaseDtor,
    MemberDtor,
    TemporaryDtor,
    DTOR_BEGIN = AutomaticObjectDtor,
    DTOR_END = TemporaryDtor
  };

protected:
  // The int bits are used to mark the kind.
  llvm::PointerIntPair<void *, 2> Data1;
  llvm::PointerIntPair<void *, 2> Data2;

  CFGElement(Kind kind, const void *Ptr1, const void *Ptr2 = nullptr)
      : Data1(const_cast<void *>(Ptr1), ((unsigned)kind) & 0x3),
        Data2(const_cast<void *>(Ptr2), (((unsigned)kind) >> 2) & 0x3) {
    assert(getKind() == kind);
  }

  CFGElement() = default;

public:
  /// Convert to the specified CFGElement type, asserting that this
  /// CFGElement is of the desired type.
  template <typename T> T castAs() const {
    assert(T::isKind(*this));
    T t;
    CFGElement &e = t;
    e = *this;
    return t;
  }

  /// Convert to the specified CFGElement type, returning None if this
  /// CFGElement is not of the desired type.
  template <typename T> clang::Optional<T> getAs() const {
    if (!T::isKind(*this))
      return clang::None;
    T t;
    CFGElement &e = t;
    e = *this;
    return t;
  }

  Kind getKind() const {
    unsigned x = Data2.getInt();
    x <<= 2;
    x |= Data1.getInt();
    return (Kind)x;
  }

  void dumpToStream(llvm::raw_ostream &OS) const;

  void dump() const { dumpToStream(llvm::errs()); }
};

class CFGStmt : public CFGElement {
  using Stmt = clang::Stmt;

public:
  explicit CFGStmt(Stmt *S, Kind K = Statement) : CFGElement(K, S) {
    assert(isKind(*this));
  }

  const Stmt *getStmt() const {
    return static_cast<const Stmt *>(Data1.getPointer());
  }

private:
  friend class CFGElement;

  static bool isKind(const CFGElement &E) {
    return E.getKind() >= STMT_BEGIN && E.getKind() <= STMT_END;
  }

protected:
  CFGStmt() = default;
};

class CFGTerminator {
private:
  using Stmt = clang::Stmt;
  Stmt *Data = nullptr;

public:
  CFGTerminator() {}
  CFGTerminator(Stmt *S) : Data(S) {}
  bool isValid() const { return Data != nullptr; }
  Stmt *getStmt() { return Data; }
  const Stmt *getStmt() const { return Data; }
};

/// Represents a single basic block in a source-level CFG.
///  It consists of:
///
///  (1) A set of statements/expressions (which may contain subexpressions).
///  (2) A "terminator" statement (not in the set of statements).
///      (2.1) for conditional block, it contains the condition expression.
///      (2.2) for unconditional block (succ = 1/0), it is null.
///      (2.3) for switch (succ > 2), it contains the condition expression.
///  (3) A list of successors and predecessors.
///
/// Terminator: The terminator represents the type of control-flow that occurs
/// at the end of the basic block.  The terminator is a Stmt* referring to an
/// AST node that has control-flow: if-statements, breaks, loops, etc.
/// If the control-flow is conditional, the condition expression will appear
/// within the set of statements in the block (usually the last statement).
///
/// Predecessors: the order in the set of predecessors is arbitrary.
///
/// Successors orderings of original Clang CFG:
///
///     Terminator     |   Successor Ordering
///  ------------------|------------------------------------
///       if           |  Then Block;  Else Block
///     ? operator     |  LHS expression;  RHS expression
///     logical and/or |  expression that consumes the op, RHS
///     vbase inits    |  already handled by the most derived class; not yet
///
class CFGBlock {
public:
  using ElementListTy = std::vector<CFGElement>;
  using Stmt = clang::Stmt;

  CFGBlock(CFG *parent) : Parent(parent) { HasNoReturnElement = false; };
  CFGBlock(CFG *parent, unsigned ID) : BlockID(ID), Parent(parent) {
    HasNoReturnElement = false;
  }

  ElementListTy Elements;

  /// An (optional) label that prefixes the executable statements in the block.
  /// When this variable is non-NULL, it is either an instance of LabelStmt,
  /// SwitchCase or CXXCatchStmt.
  Stmt *Label = nullptr;

  /// The terminator for a basic block that indicates the type of control-flow
  /// that occurs between a block and its successors.
  CFGTerminator Terminator;

  /// A numerical ID assigned to a CFGBlock during construction of the CFG.
  unsigned BlockID;

public:
  class AdjacentBlock {
    CFGBlock *Block;

  public:
    bool operator<(const AdjacentBlock &rhs) const { return Block < rhs.Block; }
    AdjacentBlock(CFGBlock *B) : Block(B) {}
    CFGBlock *getBlock() const { return Block; }
    void setBlock(CFGBlock *B) { Block = B; }
    /// Provide an implicit conversion to CFGBlock* so that
    /// AdjacentBlock can be substituted for CFGBlock*.
    operator CFGBlock *() const { return Block; }

    CFGBlock &operator*() const { return *Block; }

    CFGBlock *operator->() const { return Block; }
    bool isReachable() const { return true; }
    CFGBlock *getPossiblyUnreachableBlock() const { return nullptr; }
  };

private:
  using AdjacentBlockSet = std::set<AdjacentBlock>;
  using AdjacentBlocks = std::vector<AdjacentBlock>;
  AdjacentBlockSet Preds;
  AdjacentBlocks Succs;

  /// This bit is set when the basic block contains a function call
  /// or implicit destructor that is attributed as 'noreturn'. In that case,
  /// control cannot technically ever proceed past this block. All such blocks
  /// will have a single immediate successor: the exit block. This allows them
  /// to be easily reached from the exit block and using this bit quickly
  /// recognized without scanning the contents of the block.
  ///
  /// Optimization Note: This bit could be profitably folded with Terminator's
  /// storage if the memory usage of CFGBlock becomes an issue.
  unsigned HasNoReturnElement : 1;

  /// The parent CFG that owns this CFGBlock.
  CFG *Parent;

public:
  std::pair<CFGBlock *, CFGBlock *> getTwoSuccs() {
    assert(succ_size() == 2);
    return std::make_pair(Succs[0], Succs[1]);
  }
  bool isTrueBrSucc(CFGBlock *To) {
    assert(succ_size() == 2);
    return Succs[0] == To;
  }
  bool isFalseBrSucc(CFGBlock *To) {
    assert(succ_size() == 2);
    return Succs[1] == To;
  }
  void replaceSucc(CFGBlock *From, CFGBlock *To) {
    for (auto &succ : Succs) {
      if (succ == From) {
        succ.setBlock(To);
        return;
      }
    }
    assert(false && "replaceSucc failed!");
  }
  void replacePred(CFGBlock *From, CFGBlock *To) {
    assert(Preds.erase(AdjacentBlock(From)) == 1);
    Preds.insert(AdjacentBlock(To));
  }

  using iterator = ElementListTy::iterator;
  using const_iterator = ElementListTy::const_iterator;
  using reverse_iterator = ElementListTy::reverse_iterator;
  using const_reverse_iterator = ElementListTy::const_reverse_iterator;
  CFGElement front() const { return Elements.front(); }
  CFGElement back() const { return Elements.back(); }

  iterator begin() { return Elements.begin(); }
  iterator end() { return Elements.end(); }
  const_iterator begin() const { return Elements.begin(); }
  const_iterator end() const { return Elements.end(); }
  iterator insert(iterator it, CFGElement e) { return Elements.insert(it, e); }
  iterator erase(iterator it) { return Elements.erase(it); }
  void clear() { Elements.clear(); }

  reverse_iterator rbegin() { return Elements.rbegin(); }
  reverse_iterator rend() { return Elements.rend(); }
  const_reverse_iterator rbegin() const { return Elements.rbegin(); }
  const_reverse_iterator rend() const { return Elements.rend(); }

  unsigned long size() const { return Elements.size(); }

  // CFG iterators
  using pred_iterator = AdjacentBlockSet::iterator;
  using const_pred_iterator = AdjacentBlockSet::const_iterator;
  using pred_reverse_iterator = AdjacentBlockSet::reverse_iterator;
  using const_pred_reverse_iterator = AdjacentBlockSet::const_reverse_iterator;
  using pred_range = llvm::iterator_range<pred_iterator>;
  using pred_const_range = llvm::iterator_range<const_pred_iterator>;

  using succ_iterator = AdjacentBlocks::iterator;
  using const_succ_iterator = AdjacentBlocks::const_iterator;
  using succ_reverse_iterator = AdjacentBlocks::reverse_iterator;
  using const_succ_reverse_iterator = AdjacentBlocks::const_reverse_iterator;
  using succ_range = llvm::iterator_range<succ_iterator>;
  using succ_const_range = llvm::iterator_range<const_succ_iterator>;

  pred_iterator pred_begin() { return Preds.begin(); }
  pred_iterator pred_end() { return Preds.end(); }
  const_pred_iterator pred_begin() const { return Preds.begin(); }
  const_pred_iterator pred_end() const { return Preds.end(); }
  void pred_clear() { return Preds.clear(); }

  pred_reverse_iterator pred_rbegin() { return Preds.rbegin(); }
  pred_reverse_iterator pred_rend() { return Preds.rend(); }
  const_pred_reverse_iterator pred_rbegin() const { return Preds.rbegin(); }
  const_pred_reverse_iterator pred_rend() const { return Preds.rend(); }

  pred_range preds() { return pred_range(pred_begin(), pred_end()); }

  pred_const_range preds() const {
    return pred_const_range(pred_begin(), pred_end());
  }

  succ_iterator succ_begin() { return Succs.begin(); }
  succ_iterator succ_end() { return Succs.end(); }
  const_succ_iterator succ_begin() const { return Succs.begin(); }
  const_succ_iterator succ_end() const { return Succs.end(); }
  void succ_clear() { return Succs.clear(); }

  succ_reverse_iterator succ_rbegin() { return Succs.rbegin(); }
  succ_reverse_iterator succ_rend() { return Succs.rend(); }
  const_succ_reverse_iterator succ_rbegin() const { return Succs.rbegin(); }
  const_succ_reverse_iterator succ_rend() const { return Succs.rend(); }

  succ_range succs() { return succ_range(succ_begin(), succ_end()); }

  succ_const_range succs() const {
    return succ_const_range(succ_begin(), succ_end());
  }

  unsigned succ_size() const { return Succs.size(); }
  bool succ_empty() const { return Succs.empty(); }

  unsigned pred_size() const { return Preds.size(); }
  bool pred_empty() const { return Preds.empty(); }

  // Manipulation of block contents
  void setTerminator(CFGTerminator Term) { Terminator = Term; }
  void setLabel(Stmt *Statement) { Label = Statement; }
  void setHasNoReturnElement() { HasNoReturnElement = true; }
  CFGTerminator getTerminator() const { return Terminator; }
  void removeSucc(CFGBlock *B) {
    Succs.erase(
        std::remove_if(Succs.begin(), Succs.end(),
                       [B](AdjacentBlock &AB) { return AB.getBlock() == B; }),
        Succs.end());
  }
  void removePred(CFGBlock *B) { Preds.erase(AdjacentBlock(B)); }

  Stmt *getTerminatorStmt() { return Terminator.getStmt(); }
  const Stmt *getTerminatorStmt() const { return Terminator.getStmt(); }

  Stmt *getLabel() { return Label; }
  const Stmt *getLabel() const { return Label; }

  bool hasNoReturnElement() const { return HasNoReturnElement; }

  unsigned getBlockID() const { return BlockID; }

  CFG *getParent() const { return Parent; }

  using LangOptions = clang::LangOptions;
  using raw_ostream = llvm::raw_ostream;
  void dump() const;
  void dump(const CFG *cfg, const LangOptions &LO,
            bool ShowColors = false) const;
  void print(raw_ostream &OS, const CFG *cfg, const LangOptions &LO,
             bool ShowColors) const;

  void printTerminator(raw_ostream &OS, const LangOptions &LO) const;

  void printAsOperand(raw_ostream &OS, bool /*PrintType*/) {
    OS << "BB#" << getBlockID();
  }

  /// Adds a (potentially unreachable) successor block to the current block.
  void addSuccessor(AdjacentBlock Succ);
  void addOnlyPredecessor(AdjacentBlock Pred) { Preds.insert(Pred); }

  void prependStmt(Stmt *statement) {
    Elements.insert(Elements.begin(), CFGStmt(statement));
  }
  void appendStmt(Stmt *statement) { Elements.push_back(CFGStmt(statement)); }
  void appendElement(CFGElement statement) { Elements.push_back(statement); }
};

class CFG {
  using CFGBlockListTy = std::vector<CFGBlock *>;

public:
  void setEntry(CFGBlock *B) { Entry = B; }
  void setExit(CFGBlock *B) { Exit = B; }

  using iterator = CFGBlockListTy::iterator;
  using const_iterator = CFGBlockListTy::const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  CFGBlock &front() { return *Blocks.front(); }
  CFGBlock &back() { return *Blocks.back(); }

  CFGBlock **data() { return Blocks.data(); }

  bool contains(CFGBlock *B) {
    return std::find(Blocks.begin(), Blocks.end(), B) != Blocks.end();
  }
  iterator begin() { return Blocks.begin(); }
  iterator end() { return Blocks.end(); }
  const_iterator begin() const { return Blocks.begin(); }
  const_iterator end() const { return Blocks.end(); }

  iterator nodes_begin() { return iterator(Blocks.begin()); }
  iterator nodes_end() { return iterator(Blocks.end()); }

  llvm::iterator_range<iterator> nodes() { return {begin(), end()}; }
  llvm::iterator_range<const_iterator> const_nodes() const {
    return {begin(), end()};
  }

  const_iterator nodes_begin() const { return const_iterator(Blocks.begin()); }
  const_iterator nodes_end() const { return const_iterator(Blocks.end()); }

  reverse_iterator rbegin() { return Blocks.rbegin(); }
  reverse_iterator rend() { return Blocks.rend(); }
  const_reverse_iterator rbegin() const { return Blocks.rbegin(); }
  const_reverse_iterator rend() const { return Blocks.rend(); }

  void remove(CFGBlock *I) {
    Blocks.erase(std::remove(Blocks.begin(), Blocks.end(), I), Blocks.end());
    delete I;
  }

  CFGBlock &getEntry() { return *Entry; }
  const CFGBlock &getEntry() const { return *Entry; }
  CFGBlock &getExit() { return *Exit; }
  const CFGBlock &getExit() const { return *Exit; }

  //===--------------------------------------------------------------------===//
  // CFG Introspection.
  //===--------------------------------------------------------------------===//

  /// Returns the total number of BlockIDs allocated (which start at 0).
  unsigned getNumBlockIDs() const { return NumBlockIDs; }

  /// Return the total number of CFGBlocks within the CFG. This is necessary
  /// because the dominator implementation needs such an interface.
  unsigned size() const { return Blocks.size(); }

  /// Returns true if the CFG has no branches. Usually it boils down to the CFG
  /// having exactly three blocks (entry, the actual code, exit), but sometimes
  /// more blocks appear due to having control flow that can be fully
  /// resolved in compile time.
  bool isLinear() const;

  //===--------------------------------------------------------------------===//
  // CFG Debugging: Pretty-Printing and Visualization.
  //===--------------------------------------------------------------------===//
  using LangOptions = clang::LangOptions;
  using raw_ostream = llvm::raw_ostream;

  void viewCFG(const LangOptions &LO) const;
  void print(raw_ostream &OS, const LangOptions &LO, bool ShowColors) const;
  void dump(const LangOptions &LO, bool ShowColors) const;

  iterator createBlock();

  int sanityCheck();

private:
  CFGBlock *Entry = nullptr;
  CFGBlock *Exit = nullptr;

  unsigned NumBlockIDs = 0;
  CFGBlockListTy Blocks;
};
} // namespace notdec::backend

//===----------------------------------------------------------------------===//
// GraphTraits specializations for CFG basic block graphs (source-level CFGs)
//===----------------------------------------------------------------------===//

namespace llvm {

/// Implement simplify_type for CFGTerminator, so that we can dyn_cast from
/// CFGTerminator to a specific Stmt class.
template <> struct simplify_type<::notdec::backend::CFGTerminator> {
  using SimpleType = ::clang::Stmt *;

  static SimpleType getSimplifiedValue(::notdec::backend::CFGTerminator Val) {
    return Val.getStmt();
  }
};

// Traits for: CFGBlock

template <> struct ::llvm::GraphTraits<::notdec::backend::CFGBlock *> {
  using NodeRef = ::notdec::backend::CFGBlock *;
  using ChildIteratorType = ::notdec::backend::CFGBlock::succ_iterator;

  static NodeRef getEntryNode(::notdec::backend::CFGBlock *BB) { return BB; }
  static ChildIteratorType child_begin(NodeRef N) { return N->succ_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->succ_end(); }
};

template <> struct ::llvm::GraphTraits<const ::notdec::backend::CFGBlock *> {
  using NodeRef = const ::notdec::backend::CFGBlock *;
  using ChildIteratorType = ::notdec::backend::CFGBlock::const_succ_iterator;

  static NodeRef getEntryNode(const ::notdec::backend::CFGBlock *BB) {
    return BB;
  }
  static ChildIteratorType child_begin(NodeRef N) { return N->succ_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->succ_end(); }
};

template <> struct GraphTraits<Inverse<::notdec::backend::CFGBlock *>> {
  using NodeRef = ::notdec::backend::CFGBlock *;
  using ChildIteratorType = ::notdec::backend::CFGBlock::const_pred_iterator;

  static NodeRef getEntryNode(Inverse<::notdec::backend::CFGBlock *> G) {
    return G.Graph;
  }

  static ChildIteratorType child_begin(NodeRef N) { return N->pred_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->pred_end(); }
};

template <> struct GraphTraits<Inverse<const ::notdec::backend::CFGBlock *>> {
  using NodeRef = const ::notdec::backend::CFGBlock *;
  using ChildIteratorType = ::notdec::backend::CFGBlock::const_pred_iterator;

  static NodeRef getEntryNode(Inverse<const ::notdec::backend::CFGBlock *> G) {
    return G.Graph;
  }

  static ChildIteratorType child_begin(NodeRef N) { return N->pred_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->pred_end(); }
};

// Traits for: CFG

template <>
struct GraphTraits<::notdec::backend::CFG *>
    : public GraphTraits<::notdec::backend::CFGBlock *> {
  using nodes_iterator = ::notdec::backend::CFGBlock **;

  static NodeRef getEntryNode(::notdec::backend::CFG *F) {
    return &F->getEntry();
  }
  static nodes_iterator nodes_begin(::notdec::backend::CFG *F) {
    return F->data();
  }
  static nodes_iterator nodes_end(::notdec::backend::CFG *F) {
    return F->data() + F->size();
  }
  static unsigned size(::notdec::backend::CFG *F) { return F->size(); }
};

template <>
struct GraphTraits<const ::notdec::backend::CFG *>
    : public GraphTraits<const ::notdec::backend::CFGBlock *> {
  using nodes_iterator = ::notdec::backend::CFGBlock *const *;

  static NodeRef getEntryNode(const ::notdec::backend::CFG *F) {
    return &F->getEntry();
  }

  static nodes_iterator nodes_begin(const ::notdec::backend::CFG *F) {
    return const_cast<::notdec::backend::CFG *>(F)->data();
  }

  static nodes_iterator nodes_end(const ::notdec::backend::CFG *F) {
    return const_cast<::notdec::backend::CFG *>(F)->data() + F->size();
  }

  static unsigned size(const ::notdec::backend::CFG *F) { return F->size(); }
};

template <>
struct GraphTraits<Inverse<::notdec::backend::CFG *>>
    : public GraphTraits<Inverse<::notdec::backend::CFGBlock *>> {
  using nodes_iterator = ::notdec::backend::CFGBlock **;

  static NodeRef getEntryNode(::notdec::backend::CFG *F) {
    return &F->getExit();
  }
  static nodes_iterator nodes_begin(::notdec::backend::CFG *F) {
    return F->data();
  }
  static nodes_iterator nodes_end(::notdec::backend::CFG *F) {
    return F->data() + F->size();
  }
};

template <>
struct GraphTraits<Inverse<const ::notdec::backend::CFG *>>
    : public GraphTraits<Inverse<const ::notdec::backend::CFGBlock *>> {
  using nodes_iterator = ::notdec::backend::CFGBlock *const *;

  static NodeRef getEntryNode(const ::notdec::backend::CFG *F) {
    return &F->getExit();
  }

  static nodes_iterator nodes_begin(const ::notdec::backend::CFG *F) {
    return const_cast<::notdec::backend::CFG *>(F)->data();
  }

  static nodes_iterator nodes_end(const ::notdec::backend::CFG *F) {
    return const_cast<::notdec::backend::CFG *>(F)->data() + F->size();
  }
};
} // namespace llvm

#endif
