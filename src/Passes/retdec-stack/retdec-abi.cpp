/**
 * @file src/bin2llvmir/providers/abi/abi.cpp
 * @brief ABI information.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include "Passes/retdec-stack/retdec-abi.h"
#include <cstdlib>
#include <iostream>
// #include "retdec/bin2llvmir/providers/abi/arm.h"
// #include "retdec/bin2llvmir/providers/abi/arm64.h"
// #include "retdec/bin2llvmir/providers/abi/mips.h"
// #include "retdec/bin2llvmir/providers/abi/ms_x64.h"
// #include "retdec/bin2llvmir/providers/abi/powerpc.h"
// #include "retdec/bin2llvmir/providers/abi/x86.h"
// #include "retdec/bin2llvmir/providers/abi/x64.h"
// #include "retdec/bin2llvmir/providers/abi/pic32.h"

using namespace llvm;

namespace retdec {
namespace bin2llvmir {

//
//==============================================================================
// Abi
//==============================================================================
//

const uint32_t Abi::REG_INVALID = 0;
const unsigned Abi::DEFAULT_ADDR_SPACE = 0;

Abi::Abi(llvm::Module *m) : _module(m) {}

bool Abi::isRegister(const llvm::Value *val) const { return sp == val; }

// bool Abi::isRegister(const llvm::Value* val, uint32_t r) const
// {
// 	return sp == val;
// }

void Abi::setMemory(llvm::GlobalVariable *val) { mem = val; }

void Abi::setStackPointer(llvm::GlobalVariable *val) { sp = val; }

bool Abi::isMemory(const llvm::Value *val) const {
  if (mem == nullptr) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << " abi mem not set";
    std::abort();
  }
  return mem == val;
}

bool Abi::isStackPointerRegister(const llvm::Value *val) const {
  if (sp == nullptr) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << " abi sp not set";
    std::abort();
  }
  return sp == val;
}

bool Abi::isStackVariable(const Value *val) const {
  std::cerr << __FILE__ << ":" << __LINE__ << ": "
            << "unimplemented";
  std::abort();
}

} // namespace bin2llvmir
} // namespace retdec
