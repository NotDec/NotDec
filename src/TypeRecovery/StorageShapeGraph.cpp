#include "TypeRecovery/StorageShapeGraph.h"

namespace notdec::retypd {

void StorageShapeGraph::solve() {}

llvm::iplist<SSGNode>::iterator SSGNode::eraseFromParent() {
  auto &List =
      getParent()->*(StorageShapeGraph::getSublistAccess((SSGNode *)nullptr));
  return List.erase(getIterator());
}

llvm::iplist<SSGLink>::iterator SSGLink::eraseFromParent() {
  auto &List =
      getParent()->*(StorageShapeGraph::getSublistAccess((SSGLink *)nullptr));
  return List.erase(getIterator());
}

bool unifyPrimitive(Primitive &Left, Primitive &Right) {
  assert(Left == Right && "unifyPrimitive: Left and Right are different");
  return true;
}

bool unifyPointer(Pointer &Left, Pointer &Right) {
  // TODO
  return true;
}

bool unify(StorageShapeTy &Left, StorageShapeTy &Right) {
  StorageShapeTy ret = Left;
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

} // namespace notdec::retypd
