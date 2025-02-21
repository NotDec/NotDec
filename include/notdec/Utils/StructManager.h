

#ifndef _NOTDEC_UTILS_STRUCTMANAGER_H_
#define _NOTDEC_UTILS_STRUCTMANAGER_H_

#include <clang/AST/Decl.h>
#include <memory>
#include <utility>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>

#include "TypeRecovery/ConstraintGraph.h"
#include "Utils/Range.h"

namespace notdec::retypd {
struct CGNode;
struct CGEdge;
struct ConstraintGraph;
struct ConstraintSummary;
struct TRContext;
} // namespace notdec::retypd

namespace notdec {

using IndexTy = OffsetRange;
using OffsetTy = decltype(IndexTy::offset);

struct Range {
  IndexTy Start;
  OffsetTy Size;

  bool containsOffset(OffsetTy Offset) const {
    // TODO for Array??
    return Start.offset <= Offset && Offset < Start.offset + Size;
  }
};

struct FieldEntry {
  Range R;
  bool isPadding = false;
  retypd::CGEdge *Edge = nullptr;
  retypd::CGNode *Target = nullptr;
};

struct BytesManager {
  std::vector<std::pair<Range, llvm::StringRef>> Bytes;
  static std::shared_ptr<BytesManager> create(llvm::Module &M);
  llvm::StringRef decodeCStr(int64_t Offset);
};

struct StructInfo {
  std::vector<FieldEntry> Fields;
  std::shared_ptr<BytesManager> Bytes;
  clang::RecordDecl *Decl;
  // long TotalSize;
  long getMaxOffset() {
    long Max = 0;
    for (auto &Ent : Fields) {
      Max = std::max(Max, Ent.R.Start.offset + Ent.R.Size);
    }
    return Max;
  }

  void addField(const FieldEntry &Ent) {
    size_t i = 0;
    for (; i < Fields.size(); i++) {
      auto &F = Fields[i];
      if (F.R.Start.offset == Ent.R.Start.offset) {
        if (F.R.Size > Ent.R.Size) {
          // keep the larger field
          break;
        } else if (F.R.Size < Ent.R.Size) {
          // replace the field
          Fields[i] = Ent;
          break;
        } else {
          assert(false && "Field already exists");
        }
      }
      if (F.R.Start.offset > Ent.R.Start.offset) {
        break;
      }
    }
    Fields.insert(Fields.begin() + i, Ent);
  }

  void resolveInitialValue();
  void addPaddings();
  // clang::RecordDecl * createFieldsInDecl();
};

std::shared_ptr<retypd::ConstraintSummary>
buildPrintfSummary(retypd::TRContext &Ctx, uint32_t PointerSize,
                   llvm::StringRef FormatStr);

} // namespace notdec

#endif
