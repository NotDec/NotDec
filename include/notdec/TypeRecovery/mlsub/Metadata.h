#ifndef _NOTDEC_TYPE_RECOVERY_MLSUB_METADATA_H_
#define _NOTDEC_TYPE_RECOVERY_MLSUB_METADATA_H_

#include <llvm/ADT/StringRef.h>

namespace notdec::mlsub {

inline constexpr llvm::StringLiteral KIND_MLSUB_POLYMORPHIC_FUNCTION(
    "notdec.mlsub.polymorphic_function");

// Set by the frontend on every memory instruction the LLVM Scalarizer created
// when it split one wide (SIMD / copy-shaped) access into per-element accesses.
// A lane is not a source-level member access: the constraint generation keeps
// the lane's value typeable but must not turn it into pointer/pointee (field)
// evidence, otherwise one 16-byte copy becomes 16 one-byte struct members.
inline constexpr llvm::StringLiteral KIND_SCALARIZER_LANE(
    "notdec.mlsub.scalarizer_lane");

} // namespace notdec::mlsub

#endif
