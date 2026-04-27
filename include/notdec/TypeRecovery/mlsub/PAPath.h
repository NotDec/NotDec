#ifndef _NOTDEC_MLSUB_PA_PATH_H_
#define _NOTDEC_MLSUB_PA_PATH_H_

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Value.h>

#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "notdec-llvm2c/Interface/Range.h"

namespace notdec::mlsub {

/// PointerAnalysis needs a stable identity for every ptradd-like field step so
/// loop-carried paths can be recognized across solver iterations.
struct PAFieldTag {
  enum class Kind {
    IRPtrAdd,
    VirtualPtrAdd,
  };

  Kind K = Kind::IRPtrAdd;
  ExtValuePtr Value = static_cast<llvm::Value *>(nullptr);
  std::uint64_t ContextId = 0;
  std::uint64_t VirtualId = 0;
};

/// One concrete field step in the path. Offset says "where", Tag says "which
/// ptradd event produced this step".
struct PAPathAtom {
  OffsetRange Offset;
  PAFieldTag Tag;
};

/// A star keeps the exact repeated atom sequence for matching. Summary is only
/// a coarse offset summary and must not replace Body in equality decisions.
struct PAPathStar {
  std::vector<PAPathAtom> Body;
  OffsetRange Summary;
};

using PAPathElem = std::variant<PAPathAtom, PAPathStar>;

/// PAPath is PA's own field-path carrier. It allows mixing plain field atoms
/// with a bounded amount of repeated suffix summary.
struct PAPath {
  std::vector<PAPathElem> Elems;
};

bool operator==(const PAFieldTag &LHS, const PAFieldTag &RHS);
bool operator<(const PAFieldTag &LHS, const PAFieldTag &RHS);

bool operator==(const PAPathAtom &LHS, const PAPathAtom &RHS);
bool operator<(const PAPathAtom &LHS, const PAPathAtom &RHS);

bool operator==(const PAPathStar &LHS, const PAPathStar &RHS);
bool operator<(const PAPathStar &LHS, const PAPathStar &RHS);

bool operator==(const PAPathElem &LHS, const PAPathElem &RHS);
bool operator<(const PAPathElem &LHS, const PAPathElem &RHS);

bool operator==(const PAPath &LHS, const PAPath &RHS);
bool operator<(const PAPath &LHS, const PAPath &RHS);

std::string formatPAFieldTag(const PAFieldTag &Tag);
std::string formatPAPathAtom(const PAPathAtom &Atom);
std::string formatPAPathElem(const PAPathElem &Elem);
std::string formatPAPath(const PAPath &Path);

OffsetRange summarizeStarBody(llvm::ArrayRef<PAPathAtom> Body);
PAPath appendAndNormalize(PAPath Path, PAPathAtom Atom);
PAPath normalizePAPath(PAPath Path);

} // namespace notdec::mlsub

#endif
