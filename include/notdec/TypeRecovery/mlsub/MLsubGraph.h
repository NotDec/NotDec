#ifndef _BINARYSUB_MLSUBGRAPH_H_
#define _BINARYSUB_MLSUBGRAPH_H_

#include "binarysub/binarysub-infer.h"
#include <cassert>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/Type.h>
#include <string>

#include <llvm/ADT/ilist.h>
#include <llvm/ADT/simple_ilist.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/JSON.h>

namespace notdec::mlsub {

struct ConstraintGraph {
  long PointerSize = 0;
  std::string Name;
  std::optional<binarysub::TypeScheme> Ty;

public:
  ConstraintGraph(long PointerSize, std::string Name)
      : PointerSize(PointerSize), Name(Name) {}

  std::string getName() const { return Name; }
};

} // namespace notdec::mlsub

#endif
