#ifndef _NOTDEC_DSR_LINEAR_POINTER_H_
#define _NOTDEC_DSR_LINEAR_POINTER_H_

namespace notdec::DSR {

struct Node {
  // equivalence class representative: for union find algorithm
  Node *ECR;
  // llvm::Type *Ty;
};

} // namespace notdec::DSR

#endif
