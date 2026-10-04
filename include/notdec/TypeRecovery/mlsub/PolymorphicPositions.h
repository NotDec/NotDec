#pragma once

#include <optional>
#include <string>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>

#include "notdec/TypeRecovery/mlsub/Metadata.h"

namespace notdec::mlsub {

// Partial parameter polymorphism.
//
// A polymorphic function is instantiated per call site, which freshens every
// type variable of its signature.  For a container operation that is only
// generic in the element positions (hash_initialize(table, hasher, ...),
// _obstack_free(h, obj)) the shared object parameter must instead keep one
// type across all call sites, exactly like a monomorphic callee.  This spec
// records the positions that must NOT be freshened: the listed parameters
// (and, optionally, the return value) stay shared with the callee's own
// formal, everything else stays polymorphic.
struct PolymorphicPositionSpec {
  std::vector<unsigned> MonomorphicArgs;
  bool MonomorphicReturn = false;

  bool empty() const { return MonomorphicArgs.empty() && !MonomorphicReturn; }

  // "args=0,2;ret=1" (empty string means "all positions polymorphic").
  std::string str() const {
    std::string Result;
    if (!MonomorphicArgs.empty()) {
      Result += "args=";
      for (std::size_t I = 0; I < MonomorphicArgs.size(); ++I) {
        if (I != 0) {
          Result += ",";
        }
        Result += std::to_string(MonomorphicArgs[I]);
      }
    }
    if (MonomorphicReturn) {
      if (!Result.empty()) {
        Result += ";";
      }
      Result += "ret=1";
    }
    return Result;
  }

  static std::optional<PolymorphicPositionSpec> parse(llvm::StringRef Text) {
    PolymorphicPositionSpec Spec;
    llvm::SmallVector<llvm::StringRef, 4> Parts;
    Text.split(Parts, ';');
    for (llvm::StringRef Part : Parts) {
      Part = Part.trim();
      if (Part.empty()) {
        continue;
      }
      auto [Key, Value] = Part.split('=');
      Key = Key.trim();
      Value = Value.trim();
      if (Key == "args") {
        llvm::SmallVector<llvm::StringRef, 8> Indices;
        Value.split(Indices, ',');
        for (llvm::StringRef Index : Indices) {
          unsigned Parsed = 0;
          if (Index.trim().getAsInteger(10, Parsed)) {
            return std::nullopt;
          }
          Spec.MonomorphicArgs.push_back(Parsed);
        }
      } else if (Key == "ret") {
        Spec.MonomorphicReturn = Value == "1" || Value == "true";
      } else {
        return std::nullopt;
      }
    }
    return Spec;
  }
};

// The metadata is written by PolymorphicMarkingPass (from the policy file, the
// annotation or the NOTDEC_POLY_MONOMORPHIC override) and read by the summary
// instantiation in MLsubGenerator.
inline std::optional<PolymorphicPositionSpec>
getPolymorphicPositionSpec(const llvm::Function &F) {
  const llvm::MDNode *MD = F.getMetadata(KIND_MLSUB_MONOMORPHIC_POSITIONS);
  if (MD == nullptr || MD->getNumOperands() != 1) {
    return std::nullopt;
  }
  const auto *Text = llvm::dyn_cast<llvm::MDString>(MD->getOperand(0));
  if (Text == nullptr) {
    return std::nullopt;
  }
  return PolymorphicPositionSpec::parse(Text->getString());
}

inline void setPolymorphicPositionSpec(llvm::Function &F,
                                       const PolymorphicPositionSpec &Spec) {
  if (Spec.empty()) {
    F.setMetadata(KIND_MLSUB_MONOMORPHIC_POSITIONS, nullptr);
    return;
  }
  llvm::LLVMContext &Ctx = F.getContext();
  F.setMetadata(KIND_MLSUB_MONOMORPHIC_POSITIONS,
                llvm::MDNode::get(Ctx, llvm::MDString::get(Ctx, Spec.str())));
}

// Resolve the spec of a function from, in order: the function's own metadata,
// the NOTDEC_POLY_FUNCS policy field "monomorphic_positions", and the
// NOTDEC_POLY_MONOMORPHIC override.  A function with no spec is fully
// polymorphic (today's behaviour).
PolymorphicPositionSpec resolvePolymorphicPositionSpec(const llvm::Function &F);

} // namespace notdec::mlsub
