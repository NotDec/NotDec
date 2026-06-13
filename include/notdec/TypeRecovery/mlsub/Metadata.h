#ifndef _NOTDEC_TYPE_RECOVERY_MLSUB_METADATA_H_
#define _NOTDEC_TYPE_RECOVERY_MLSUB_METADATA_H_

#include <llvm/ADT/StringRef.h>

namespace notdec::mlsub {

inline constexpr llvm::StringLiteral KIND_MLSUB_POLYMORPHIC_FUNCTION(
    "notdec.mlsub.polymorphic_function");

} // namespace notdec::mlsub

#endif
