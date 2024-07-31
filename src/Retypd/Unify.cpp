#include "Retypd/Unify.h"

namespace notdec::retypd {

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

} // namespace notdec::retypd
