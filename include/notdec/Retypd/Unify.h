#ifndef _NOTDEC_RETYPD_UNIFY_H_
#define _NOTDEC_RETYPD_UNIFY_H_

#include <cassert>
#include <cstdint>
#include <list>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <map>
#include <memory>
#include <variant>

#include <llvm/IR/Value.h>
#include <vector>

namespace notdec::retypd {

// TODO: optimization: Use nullptr to represent Unknown?
struct Unknown {};
struct Primitive {
  std::string name;
  bool operator==(const Primitive &Other) const { return name == Other.name; }
};
struct Pointer {};
using UniTy = std::variant<Unknown, Primitive, Pointer>;
// mergeMap[left index][right index] = isPreserveLeft
// row -> right index, column -> left index
const unsigned char UniTyMergeMap[][3] = {
    {2, 0, 0},
    {1, 2, 0},
    {1, 1, 2},
};

bool unifyPrimitive(Primitive &Left, Primitive &Right);

bool unifyPointer(Pointer &Left, Pointer &Right);

struct StorageShapeGraph;

struct SSGNode
    : public llvm::ilist_node_with_parent<SSGNode, StorageShapeGraph> {
  StorageShapeGraph *Parent;
  void setParent(StorageShapeGraph *P) { Parent = P; }
  inline StorageShapeGraph *getParent() { return Parent; }
  llvm::iplist<SSGNode>::iterator eraseFromParent();

  UniTy Ty;
  bool unify(SSGNode &other) {
    UniTy &Left = Ty;
    UniTy &Right = other.Ty;
    unsigned char Val = UniTyMergeMap[Left.index()][Right.index()];
    if (Val == 2) { // same inner type
      if (std::holds_alternative<Pointer>(Left)) {
        return unifyPointer(std::get<Pointer>(Left), std::get<Pointer>(Right));
      } else if (std::holds_alternative<Primitive>(Left)) {
        return unifyPrimitive(std::get<Primitive>(Left),
                              std::get<Primitive>(Right));
      } else if (std::holds_alternative<Unknown>(Left)) {
        return true;
      }
      assert(false && "unify: unhandled type");
    } else {
      return Val == 1;
    }
  }
};

struct SSGLink
    : public llvm::ilist_node_with_parent<SSGLink, StorageShapeGraph> {
  StorageShapeGraph *Parent;
  void setParent(StorageShapeGraph *P) { Parent = P; }
  inline StorageShapeGraph *getParent() { return Parent; }
  llvm::iplist<SSGLink>::iterator eraseFromParent();

  // SSGLink(StorageShapeGraph &SSG) : Parent(&SSG) {}

  using LinkTy = std::variant<SSGLink *, SSGNode *>;
  LinkTy Link;
  static SSGLink *lookup(SSGLink *Current) {
    assert(Current != nullptr && "SSGLink::lookup: Current is null");
    std::vector<SSGLink *> path;

    while (auto *N = std::get_if<SSGLink *>(&Current->Link)) {
      assert(N != nullptr && "SSGLink::lookup: N is null");
      path.push_back(Current);
      Current = *N;
      assert(Current != nullptr && "SSGLink::lookup: Current is null");
    }
    // SSGNode *Result = std::get<SSGNode *>(Current->Link);
    for (auto N : path) {
      assert(Current != nullptr && "SSGLink::lookup: Current is null");
      N->Link = Current;
    }
    return Current;
  }
  void setForward(SSGLink *N) { Link = N; }
  void setNode(SSGNode *N) { Link = N; }
  SSGNode *getNode() {
    if (auto *N = std::get_if<SSGNode *>(&Link)) {
      return *N;
    }
    return nullptr;
  }

  void unify(SSGLink &Other) {
    assert(getParent() == Other.getParent() &&
           "SSGLink::unifyNode: Parents are different");
    SSGLink *LThis = lookup(this);
    SSGLink *LOther = lookup(&Other);
    assert(LThis != nullptr && LOther != nullptr);
    SSGNode *SNThis = LThis->getNode();
    SSGNode *SNOther = LOther->getNode();
    assert(SNThis != nullptr && SNOther != nullptr);
    bool PreserveLeft = SNThis->unify(*SNOther);
    if (PreserveLeft) {
      LOther->Link = LThis;
      SNOther->eraseFromParent();
    } else {
      Link = LOther;
      SNThis->eraseFromParent();
    }
  }
};

struct StorageShapeGraph {
  std::string FuncName;
  using NodesType = llvm::ilist<SSGNode>;
  NodesType Nodes;
  static NodesType StorageShapeGraph::*getSublistAccess(SSGNode *) {
    return &StorageShapeGraph::Nodes;
  }
  using LinksType = llvm::ilist<SSGLink>;
  LinksType Links;
  std::map<llvm::Value *, std::shared_ptr<SSGLink>> Val2Node;
  static LinksType StorageShapeGraph::*getSublistAccess(SSGLink *) {
    return &StorageShapeGraph::Links;
  }

  SSGNode *createUnknown() {
    SSGNode *N = new SSGNode();
    N->Ty = Unknown();
    N->setParent(this);
    Nodes.push_back(N);
    return N;
  }
};

} // namespace notdec::retypd

#endif
