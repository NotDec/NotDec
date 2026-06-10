#include "Passes/evm/SolidityPatternUtils.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

namespace notdec::passes::evm::detail {

std::optional<uint64_t>
getUniqueCallsiteArgUInt64Constant(const Value *V) {
  auto *Arg = dyn_cast_or_null<Argument>(V);
  if (Arg == nullptr) {
    return std::nullopt;
  }

  const Function *F = Arg->getParent();
  unsigned ArgNo = Arg->getArgNo();
  std::optional<uint64_t> Result;
  bool SawCallsite = false;

  // Public EVM entry wrappers often pass ABI constants such as 0, 4, and 64
  // through formal arguments.  Treat them as constants only when every direct
  // callsite agrees; any indirect or conflicting use keeps the value unknown.
  for (const Use &U : F->uses()) {
    auto *Call = dyn_cast<CallBase>(U.getUser());
    if (Call == nullptr || Call->getCalledFunction() != F ||
        ArgNo >= Call->arg_size()) {
      return std::nullopt;
    }

    auto *C = dyn_cast<ConstantInt>(Call->getArgOperand(ArgNo));
    if (C == nullptr || C->getValue().getActiveBits() > 64) {
      return std::nullopt;
    }

    uint64_t Value = C->getZExtValue();
    if (Result.has_value() && *Result != Value) {
      return std::nullopt;
    }
    Result = Value;
    SawCallsite = true;
  }

  if (!SawCallsite) {
    return std::nullopt;
  }
  return Result;
}

} // namespace notdec::passes::evm::detail
