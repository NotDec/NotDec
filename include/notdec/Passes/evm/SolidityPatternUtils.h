#ifndef _NOTDEC_PASSES_EVM_SOLIDITY_PATTERN_UTILS_H_
#define _NOTDEC_PASSES_EVM_SOLIDITY_PATTERN_UTILS_H_

#include "Passes/evm/SolidityPatterns.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <optional>
#include <string>

namespace notdec::mlsub {
struct EVMStoreEvidence;
} // namespace notdec::mlsub

namespace notdec::ast {
class HType;
class FieldDecl;
class RecordDecl;
} // namespace notdec::ast

namespace notdec::llvm2c {
struct HTypeResult;
} // namespace notdec::llvm2c

namespace notdec::passes::evm::detail {

inline constexpr llvm::StringLiteral KIND_SOLIDITY_SELECTOR_OUTLINED_BODY(
    "notdec.solidity.selector_outlined_body");
inline constexpr llvm::StringLiteral KIND_SOLIDITY_SELECTOR_OUTLINE_SKIPPED(
    "notdec.solidity.selector_outline_skipped");

// Shared result for Solidity revert-like exits.  Revert and checked/bounds
// passes both need the terminator kind and a small amount of ABI payload shape.
struct SolidityRevertMatch {
  llvm::StringRef Kind = "encoded_candidate";
  llvm::CallBase *Revert = nullptr;
  llvm::CallBase *SelectorStore = nullptr;
  llvm::CallBase *PanicCodeStore = nullptr;
  llvm::CallBase *ReturndataCopy = nullptr;
  std::optional<uint64_t> Selector;
  std::optional<uint64_t> PanicCode;
  std::optional<uint64_t> CustomErrorArgCount;
  std::optional<uint64_t> ErrorStringLength;
  std::optional<std::string> ErrorStringLiteral;
  bool UsedMemoryWriteMarker = false;
};

// Exact pieces of one canonical nonpayable guard.  The pass consumes only this
// proven shape so the CFG rewrite stays tied to the original match.
struct PayabilityGuardMatch {
  llvm::BasicBlock *GuardBlock = nullptr;
  llvm::BasicBlock *SuccessBlock = nullptr;
  llvm::BasicBlock *FailureBlock = nullptr;
  llvm::CallBase *CallValue = nullptr;
  llvm::ICmpInst *Condition = nullptr;
  llvm::BranchInst *Branch = nullptr;
};

// Compiler-inserted checked operation or bounds guard.  The branch is the guard
// site; the panic code is the stable Solidity reason exposed to tests.
struct CheckedBoundsMatch {
  llvm::StringRef Kind = "unknown";
  llvm::StringRef SkipReason = "";
  llvm::BranchInst *Branch = nullptr;
  llvm::BasicBlock *SuccessBlock = nullptr;
  llvm::BasicBlock *FailureBlock = nullptr;
  llvm::CallBase *Revert = nullptr;
  llvm::SmallVector<llvm::Value *, 3> Operands;
  std::optional<uint64_t> PanicCode;
  bool Rewrite = false;
};

// Storage addressing helpers identify common Solidity sha3-based storage
// shapes without writing marker calls.  Callers decide whether the match is
// useful for their own rewrite.
struct StorageScratchKeccakMatch {
  llvm::CallBase *Sha3 = nullptr;
  llvm::Value *Key = nullptr;
  llvm::Value *BaseSlot = nullptr;
};

struct StorageArrayDataKeccakMatch {
  llvm::CallBase *Sha3 = nullptr;
  llvm::Value *BaseSlot = nullptr;
};

struct StorageMappingAccessMatch {
  llvm::CallBase *Access = nullptr;
  llvm::Value *Key = nullptr;
  llvm::Value *BaseSlot = nullptr;
  llvm::Value *Hash = nullptr;
  llvm::Value *StorageSlot = nullptr;
  uint64_t AccessKind = 0;
};

struct StorageArrayDataAccessMatch {
  llvm::CallBase *Access = nullptr;
  llvm::Value *BaseSlot = nullptr;
  llvm::Value *DataHash = nullptr;
  llvm::Value *StorageSlot = nullptr;
  uint64_t AccessKind = 0;
};

struct PackedStorageAccessMatch {
  llvm::CallBase *Access = nullptr;
  llvm::Value *StorageSlot = nullptr;
  uint64_t AccessKind = 0;
};

enum class HTypeBufferGap {
  None,
  NoPointerType,
  NonRecordPointerType,
};

// Thin view over one ABI/revert/event memory buffer type.  It only exposes the
// record pointee already recovered by MLsub; callers keep all Solidity-specific
// classification and fallback decisions local to their pass.
struct HTypeBufferView {
  notdec::ast::RecordDecl *Record = nullptr;
  notdec::ast::HType *BaseType = nullptr;
  HTypeBufferGap Gap = HTypeBufferGap::None;
};

// EVM memory is represented as native LLVM load/store through inttoptr.  These
// views keep pass code focused on address/value semantics.
struct EvmMemoryLoad {
  llvm::Instruction *Inst = nullptr;
  llvm::Value *Address = nullptr;
  llvm::Value *LoadedValue = nullptr;
};

struct EvmMemoryStore {
  llvm::Instruction *Inst = nullptr;
  llvm::Value *Address = nullptr;
  llvm::Value *StoredValue = nullptr;
  unsigned StoreBits = 0;
};

bool isCallTo(const llvm::Value *V, llvm::StringRef Name);
bool isConstantIntValue(const llvm::Value *V, uint64_t N);
llvm::StringRef getCalleeName(const llvm::Value *V);
void addPlainMetadata(llvm::LLVMContext &Ctx, llvm::Instruction &I,
                      llvm::StringRef Kind, llvm::StringRef Value);
void addStringMetadata(llvm::LLVMContext &Ctx, llvm::Instruction &I,
                       llvm::StringRef Kind, llvm::StringRef Value);
void addStringMetadata(llvm::LLVMContext &Ctx, llvm::Function &F,
                       llvm::StringRef Kind, llvm::StringRef Value);
void markBlock(llvm::LLVMContext &Ctx, llvm::BasicBlock &BB,
               llvm::StringRef Kind, llvm::StringRef Value);

bool isSelectorFunction(const llvm::Function &F);
bool isPublicEntryFunction(const llvm::Function &F);
bool isDispatcherBlock(llvm::BasicBlock &BB);
bool isPublicCallStub(llvm::BasicBlock &BB);
bool isEmptyRejectBlock(llvm::BasicBlock &BB);
void collectReachableBody(
    llvm::BasicBlock *Entry,
    const llvm::SmallPtrSetImpl<llvm::BasicBlock *> &Dispatcher,
    llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks,
    llvm::SmallPtrSetImpl<llvm::BasicBlock *> &Region);
bool regionHasBodySignal(
    const llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks);
llvm::StringRef getOutlineSkipReason(
    const llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks,
    const llvm::SmallPtrSetImpl<llvm::BasicBlock *> &Region);
void collectRegionInputs(
    const llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks,
    const llvm::SmallPtrSetImpl<llvm::BasicBlock *> &Region,
    llvm::SmallVectorImpl<llvm::Instruction *> &Inputs);
bool regionInputsAvailableAtEntry(
    llvm::ArrayRef<llvm::Instruction *> Inputs, llvm::BasicBlock *Entry,
    const llvm::SmallPtrSetImpl<llvm::BasicBlock *> &Region,
    llvm::DominatorTree &DT);
llvm::Function *cloneSelectorRegion(
    llvm::Function &F, llvm::ArrayRef<llvm::BasicBlock *> Blocks,
    const llvm::SmallPtrSetImpl<llvm::BasicBlock *> &Region,
    llvm::BasicBlock *Entry, llvm::ArrayRef<llvm::Instruction *> Inputs);
void replaceRegionWithCall(
    llvm::Function &F, llvm::Function &Outlined,
    llvm::ArrayRef<llvm::BasicBlock *> Blocks,
    const llvm::SmallPtrSetImpl<llvm::BasicBlock *> &Region,
    llvm::BasicBlock *Entry, llvm::ArrayRef<llvm::Instruction *> Inputs);
bool hasWholeSelectorOutlineShape(
    llvm::Function &F, const llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks);
void replaceWholeFunctionWithCall(llvm::Function &F, llvm::Function &Outlined);

std::optional<PayabilityGuardMatch>
matchPayabilityGuard(llvm::BasicBlock &BB);
void insertPayabilityCfgRewriteMarker(llvm::LLVMContext &Ctx,
                                      const PayabilityGuardMatch &Match);

bool isReturndataSize(llvm::Value *V);
std::optional<uint64_t> getUInt64Constant(llvm::Value *V);
std::optional<uint64_t> getOffsetFromBase(llvm::Value *Offset,
                                          llvm::Value *Base);
std::optional<uint64_t> getSelectorWord(llvm::Value *V);
HTypeBufferView getHTypeBufferView(notdec::llvm2c::HTypeResult &HTypes,
                                   llvm::Value *Base, llvm::CallBase &Use,
                                   unsigned ArgIndex);
bool hasHTypeFieldAt(notdec::ast::RecordDecl &Record, int64_t Offset);
llvm::SmallVector<llvm::Value *, 2> getHTypeFieldStoreValues(
    llvm::ArrayRef<notdec::mlsub::EVMStoreEvidence> Stores,
    notdec::ast::RecordDecl &Record, llvm::Value *Base, int64_t Offset);
std::optional<uint64_t>
getUniqueUInt64FieldValue(llvm::ArrayRef<llvm::Value *> Values,
                          bool &Conflict);
std::optional<EvmMemoryLoad> matchEvmMemoryLoad(llvm::Value *V);
std::optional<EvmMemoryStore> matchEvmMemoryStore(llvm::Instruction *I);
bool isFreeMemoryPointerLoad(llvm::Value *V);
bool isFreeMemoryPointerStore(llvm::Instruction *I);
bool isSameOrReloadedFreeMemoryBase(llvm::Value *LHS, llvm::Value *RHS);
std::optional<SolidityRevertMatch>
matchSolidityRevert(llvm::BasicBlock &BB, llvm::CallBase &Revert);
void insertPanicRewriteMarker(llvm::LLVMContext &Ctx,
                              const SolidityRevertMatch &Match);
void insertReturndataBubbleRewriteMarker(llvm::LLVMContext &Ctx,
                                         const SolidityRevertMatch &Match);
void insertRevertMemoryWriteMatchMarker(llvm::LLVMContext &Ctx,
                                        const SolidityRevertMatch &Match);
void insertSelectorRewriteMarker(llvm::LLVMContext &Ctx,
                                 const SolidityRevertMatch &Match,
                                 llvm::StringRef MarkerName,
                                 std::optional<uint64_t> Payload);
void addRevertMatchMetadata(llvm::LLVMContext &Ctx,
                            const SolidityRevertMatch &Match);

std::optional<CheckedBoundsMatch>
matchCheckedBoundsGuard(llvm::BasicBlock &BB);
bool checkedBoundsOperandsDominateBranch(const CheckedBoundsMatch &Match,
                                         llvm::DominatorTree &DT);
void downgradeCheckedBoundsRewrite(CheckedBoundsMatch &Match,
                                   llvm::StringRef Reason);
void addCheckedBoundsMetadata(llvm::LLVMContext &Ctx,
                              const CheckedBoundsMatch &Match);
void rewriteCheckedBoundsGuard(const CheckedBoundsMatch &Match);

bool dependsOnCallTo(llvm::Value *V, llvm::StringRef Name, unsigned Depth = 8);
bool dependsOnValue(llvm::Value *V, llvm::Value *Target, unsigned Depth = 8);
bool expressionHasPackedStorageOp(llvm::Value *V, unsigned Depth = 8);
std::optional<StorageScratchKeccakMatch>
matchStorageScratchKeccak(llvm::CallBase &Sha3);
std::optional<StorageArrayDataKeccakMatch>
matchStorageArrayDataKeccak(llvm::CallBase &Sha3);
std::optional<StorageMappingAccessMatch>
matchStorageMappingAccess(llvm::CallBase &Access, llvm::Value *StorageSlot,
                          uint64_t AccessKind);
std::optional<StorageArrayDataAccessMatch>
matchStorageArrayDataAccess(llvm::CallBase &Access, llvm::Value *StorageSlot,
                            uint64_t AccessKind);
std::optional<PackedStorageAccessMatch>
matchPackedStorageAccess(llvm::CallBase &Access, uint64_t AccessKind);
llvm::StringRef classifyExternalCall(llvm::StringRef Name);

} // namespace notdec::passes::evm::detail

#endif
