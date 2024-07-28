#ifndef _NOTDEC_RETYPD_UNIFY_H_
#define _NOTDEC_RETYPD_UNIFY_H_

#include <map>
#include <memory>
#include <set>
#include <variant>

#include <llvm/IR/Value.h>

namespace notdec::retypd {

struct Unknown {};
struct Primitive {};
struct Pointer {};
using UniTy = std::variant<Unknown, Primitive, Pointer>;

struct SSGNode {
  UniTy Ty;
};

struct StorageShapeGraph {
  std::string FuncName;
  std::set<SSGNode> Nodes;
};

} // namespace notdec::retypd

#endif
