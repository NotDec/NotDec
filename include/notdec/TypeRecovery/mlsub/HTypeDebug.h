#ifndef _NOTDEC_MLSUB_HTYPE_DEBUG_H_
#define _NOTDEC_MLSUB_HTYPE_DEBUG_H_

#include "notdec-llvm2c/Interface.h"
#include <llvm/ADT/StringRef.h>

namespace llvm {
class Module;
} // namespace llvm

namespace notdec::mlsub {

void writeDebugEVMMarkerFacts(llvm::StringRef DebugDir,
                              const llvm::Module &M);
void writeDebugValueHTypes(llvm::StringRef DebugDir,
                           const llvm2c::HTypeResult &Result);
void writeDebugImportantHTypes(llvm::StringRef DebugDir,
                               const llvm2c::HTypeResult &Result);

} // namespace notdec::mlsub

#endif
