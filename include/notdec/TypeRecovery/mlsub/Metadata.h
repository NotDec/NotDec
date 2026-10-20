#ifndef _NOTDEC_TYPE_RECOVERY_MLSUB_METADATA_H_
#define _NOTDEC_TYPE_RECOVERY_MLSUB_METADATA_H_

#include <optional>
#include <string>

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include "notdec/TypeRecovery/LowTy.h"

namespace notdec::mlsub {

inline constexpr llvm::StringLiteral KIND_MLSUB_POLYMORPHIC_FUNCTION(
    "notdec.mlsub.polymorphic_function");

// Partial parameter polymorphism: the positions of a polymorphic function that
// must stay monomorphic (shared across call sites) instead of being freshened
// per call site.  Payload: a string such as "args=0,2;ret=1".  See
// PolymorphicPositions.h for the reader/writer.
inline constexpr llvm::StringLiteral KIND_MLSUB_MONOMORPHIC_POSITIONS(
    "notdec.mlsub.monomorphic_positions");

// Set by the frontend on every memory instruction the LLVM Scalarizer created
// when it split one wide (SIMD / copy-shaped) access into per-element accesses.
// A lane is not a source-level member access: the constraint generation keeps
// the lane's value typeable but must not turn it into pointer/pointee (field)
// evidence, otherwise one 16-byte copy becomes 16 one-byte struct members.
inline constexpr llvm::StringLiteral KIND_SCALARIZER_LANE(
    "notdec.mlsub.scalarizer_lane");

// Identity of the value a function returns and one of its arguments: the
// returned object IS the object that was passed in as argument N.  The
// declaration-level counterpart is the policy field "ret_alias_of_arg"; the
// "local" prefix marks that the relation is asserted on a function with a body
// and applied at its own ret instruction, so it holds per call-site instance
// instead of leaking through a callee's shared formal.
//
// The relation is a type equality, not a dataflow relation: the recovery adds
// both subtype directions, which merges the two type variables, and the PNDiff
// values bound to those variables are unified as well (PNDiff union is an
// equal-width operation).  It may therefore only be attached when the returned
// value and the argument have the same width; setLocalRetAliasOfArg enforces
// that and refuses the relation otherwise.
//
// Payload: "arg<N>".
inline constexpr llvm::StringLiteral KIND_MLSUB_LOCAL_RET_ALIAS_OF_ARG(
    "notdec.mlsub.local_ret_alias_of_arg");

// The argument index recorded by KIND_MLSUB_LOCAL_RET_ALIAS_OF_ARG, or nullopt
// when the metadata is absent/malformed or the index is out of range.
inline std::optional<unsigned> getLocalRetAliasOfArg(const llvm::Function &F) {
  const llvm::MDNode *MD = F.getMetadata(KIND_MLSUB_LOCAL_RET_ALIAS_OF_ARG);
  if (MD == nullptr || MD->getNumOperands() != 1) {
    return std::nullopt;
  }
  const auto *Spec = llvm::dyn_cast<llvm::MDString>(MD->getOperand(0));
  if (Spec == nullptr) {
    return std::nullopt;
  }
  llvm::StringRef Text = Spec->getString();
  unsigned ArgNo = 0;
  if (!Text.consume_front("arg") || Text.getAsInteger(10, ArgNo) ||
      ArgNo >= F.arg_size()) {
    return std::nullopt;
  }
  return ArgNo;
}

// Attach KIND_MLSUB_LOCAL_RET_ALIAS_OF_ARG after checking that the returned
// value and argument ArgNo have the same width (the width PNDiff derives for
// the two values).  A mismatch means the two values cannot be the same object:
// print an error and leave the function unannotated (the polymorphic marking
// itself is unaffected).
inline bool setLocalRetAliasOfArg(llvm::Function &F, unsigned ArgNo,
                                  llvm::Type *ReturnedTy) {
  if (ReturnedTy == nullptr || ArgNo >= F.arg_size() ||
      F.getParent() == nullptr) {
    llvm::errs() << "Error: " << F.getName() << ": cannot record "
                 << KIND_MLSUB_LOCAL_RET_ALIAS_OF_ARG << "=arg" << ArgNo
                 << ": invalid argument index, type, or missing module\n";
    return false;
  }
  const llvm::DataLayout &DL = F.getParent()->getDataLayout();
  const unsigned PointerBits = DL.getPointerSizeInBits(0);
  const unsigned long RetBits =
      notdec::retypd::getSize(ReturnedTy, PointerBits);
  const unsigned long ArgBits =
      notdec::retypd::getSize(F.getArg(ArgNo)->getType(), PointerBits);
  if (RetBits != ArgBits) {
    llvm::errs() << "Error: " << F.getName() << ": refusing "
                 << KIND_MLSUB_LOCAL_RET_ALIAS_OF_ARG << "=arg" << ArgNo
                 << ": the returned value is " << RetBits
                 << " bits wide but argument " << ArgNo << " is " << ArgBits
                 << " bits wide; the two values cannot be the same object\n";
    return false;
  }
  const std::string Spec = "arg" + std::to_string(ArgNo);
  F.setMetadata(KIND_MLSUB_LOCAL_RET_ALIAS_OF_ARG,
                llvm::MDNode::get(F.getContext(),
                                  llvm::MDString::get(F.getContext(), Spec)));
  return true;
}

} // namespace notdec::mlsub

#endif
