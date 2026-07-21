
#include <llvm/Bitcode/BitcodeWriter.h>
#include "Passes/evm/SolidityPatternUtils.h"
#include "notdec/TypeRecovery/mlsub/MLsubGenerator.h"
#include "Utils/CallGraphDotInfo.h"
#include "binarysub/binarysub-core.h"
#include "binarysub/binarysub-infer.h"
#include "binarysub/binarysub-primitive-semantics.h"
#include "binarysub/binarysub.h"
#include "notdec-llvm2c/Interface.h"
#include "binarysub/HType.h"
#include "notdec-llvm2c/Utils.h"
#include "notdec/TypeRecovery/mlsub/HTypeDebug.h"
#include "notdec/TypeRecovery/mlsub/HTypeNormalize.h"
#include "notdec/TypeRecovery/mlsub/Metadata.h"
#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "notdec/TypeRecovery/LowTy.h"
#include "notdec/Utils/AllSCCIterator.h"
#include "notdec/Utils/SingleNodeSCCIterator.h"
#include "notdec/Utils/Utils.h"

#include <cassert>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Operator.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/SHA256.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "mlsub_generator"

#ifndef NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH
#define NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH ""
#endif

namespace notdec::mlsub {

static bool isEVMModule(const llvm::Module &M);

namespace {

struct PolyPolicyConfig {
  std::set<std::string> PolyFuncs;
  std::map<std::string, unsigned int> LevelOverrides;
};

constexpr llvm::StringLiteral kValueTypesFile = "ValueTypes.txt";
constexpr llvm::StringLiteral kVarOriginsFile = "VarOrigins.txt";
constexpr llvm::StringLiteral kValueHTypesFile = "ValueHTypes.txt";
constexpr llvm::StringLiteral kImportantHTypesFile = "ImportantHTypes.txt";
constexpr llvm::StringLiteral kSelectableValuesFile = "SelectableValues.txt";
constexpr llvm::StringLiteral kPNDiffWarnFile = "PNDiff.warn.txt";
constexpr llvm::StringLiteral kMLsubInputIRFile = "02-mlsub-input.ll";
constexpr llvm::StringLiteral kMLsubInputAnchorFile =
    "02-mlsub-input.anchor.json";
constexpr llvm::StringLiteral kPNDiffAnnotatedFile = "03-pndiff-final.ll";
constexpr llvm::StringLiteral kBinarysubTraceFile = "binarysub-trace.log";
constexpr llvm::StringLiteral kBinarysubTraceEnv = "NOTDEC_BINARYSUB_TRACE";

bool envFlagEnabled(llvm::StringRef Name) {
  auto *Value = std::getenv(Name.data());
  return Value != nullptr && llvm::StringRef(Value) == "1";
}

llvm::Value *getPNDiffLLVMValue(const ExtValuePtr &Val) {
  if (auto *V = std::get_if<llvm::Value *>(&Val)) {
    return *V;
  }
  if (auto *C = std::get_if<UConstant>(&Val)) {
    return C->Val;
  }
  return nullptr;
}

std::optional<int64_t> getPNDiffIntConstantValue(const ExtValuePtr &Val) {
  auto *Value = getPNDiffLLVMValue(Val);
  auto *CI = llvm::dyn_cast_or_null<llvm::ConstantInt>(Value);
  if (CI == nullptr || CI->getValue().getSignificantBits() > 64) {
    return std::nullopt;
  }
  return CI->getValue().getSExtValue();
}

OffsetRange getUnknownPNDiffOffsetRange() {
  return OffsetRange{.offset = 0, .access = {{1, 0}}};
}

bool isUnknownPNDiffOffsetRange(const OffsetRange &Range) {
  return Range == getUnknownPNDiffOffsetRange();
}

std::optional<uint64_t> parseConstantFieldOffset(llvm::StringRef FieldName) {
  if (FieldName.empty() || FieldName.contains("+") ||
      FieldName.contains("i")) {
    return std::nullopt;
  }
  if (FieldName.starts_with("@")) {
    FieldName = FieldName.drop_front();
  }
  uint64_t Offset = 0;
  if (FieldName.getAsInteger(10, Offset)) {
    return std::nullopt;
  }
  return Offset;
}

bool isStructFieldEvidence(llvm::StringRef FieldName) {
  auto Offset = parseConstantFieldOffset(FieldName);
  return Offset && *Offset >= 4;
}

std::optional<OffsetRange> matchPNDiffOffsetRange(llvm::Value *I) {
  assert(I->getType()->isIntegerTy());
  if (auto *CI = llvm::dyn_cast<llvm::ConstantInt>(I)) {
    auto Constant = getPNDiffIntConstantValue(ExtValuePtr{CI});
    if (!Constant) {
      return std::nullopt;
    }
    return OffsetRange{.offset = *Constant};
  }
  if (!llvm::isa<llvm::BinaryOperator>(I)) {
    return std::nullopt;
  }

  auto *BinOp = llvm::cast<llvm::BinaryOperator>(I);
  auto Opcode = BinOp->getOpcode();
  if (Opcode != llvm::Instruction::Add && Opcode != llvm::Instruction::Mul &&
      Opcode != llvm::Instruction::Shl) {
    return std::nullopt;
  }

  auto *Src1 = BinOp->getOperand(0);
  auto *Src2 = BinOp->getOperand(1);
  if (llvm::isa<llvm::ConstantInt>(Src1) && llvm::isa<llvm::ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (llvm::isa<llvm::ConstantInt>(Src1) && !llvm::isa<llvm::ConstantInt>(Src2) &&
      Opcode != llvm::Instruction::Shl) {
    assert(false &&
           "Constant cannot be at the left side. Run InstCombine first.");
    std::swap(Src1, Src2);
  }

  if (Opcode == llvm::Instruction::Add) {
    auto Result =
        matchPNDiffOffsetRange(Src1).value_or(getUnknownPNDiffOffsetRange()) +
        matchPNDiffOffsetRange(Src2).value_or(getUnknownPNDiffOffsetRange());
    if (isUnknownPNDiffOffsetRange(Result)) {
      return std::nullopt;
    }
    return Result;
  }
  if (Opcode == llvm::Instruction::Mul) {
    auto Result =
        matchPNDiffOffsetRange(Src1).value_or(getUnknownPNDiffOffsetRange()) *
        matchPNDiffOffsetRange(Src2).value_or(getUnknownPNDiffOffsetRange());
    if (isUnknownPNDiffOffsetRange(Result)) {
      return std::nullopt;
    }
    return Result;
  }
  if (Opcode == llvm::Instruction::Shl && llvm::isa<llvm::ConstantInt>(Src2)) {
    auto Shift = getPNDiffIntConstantValue(ExtValuePtr{Src2});
    if (!Shift || *Shift < 0 || *Shift >= 63) {
      return std::nullopt;
    }
    auto Result =
        matchPNDiffOffsetRange(Src1).value_or(getUnknownPNDiffOffsetRange()) *
        (int64_t{1} << *Shift);
    if (isUnknownPNDiffOffsetRange(Result)) {
      return std::nullopt;
    }
    return Result;
  }
  return std::nullopt;
}

std::optional<OffsetRange> matchPNDiffOffsetRangeNoNegativeAccess(
    const ExtValuePtr &Val) {
  auto *Value = getPNDiffLLVMValue(Val);
  if (Value == nullptr || !Value->getType()->isIntegerTy()) {
    return std::nullopt;
  }
  auto R = matchPNDiffOffsetRange(Value);
  if (!R) {
    return std::nullopt;
  }
  R->access.erase(std::remove_if(R->access.begin(), R->access.end(),
                                 [](ArrayOffset Val) { return Val.Size < 0; }),
                  R->access.end());
  return R;
}

std::string formatPNDiffInstruction(void *Inst) {
  auto *I = static_cast<llvm::Instruction *>(Inst);
  if (I == nullptr) {
    return "<null-inst>";
  }
  std::string S;
  llvm::raw_string_ostream OS(S);
  I->print(OS);
  return OS.str();
}

bool isFunctionPointerTable(const llvm::GlobalVariable &GV) {
  auto *TableTy = GV.getValueType();
  if (!TableTy->isArrayTy()) {
    return false;
  }

  auto *ElemTy = TableTy->getArrayElementType();
  if (!ElemTy->isPointerTy()) {
    return false;
  }

  auto *Init = GV.getInitializer();
  if (Init == nullptr) {
    return false;
  }

  // Opaque pointer mode no longer exposes the pointee from `ptr`, so only
  // trust tables whose initializer entries are visibly functions or casts of
  // functions.
  for (const auto &Op : Init->operands()) {
    auto *Entry = dyn_cast<Constant>(Op.get());
    if (Entry == nullptr) {
      continue;
    }
    if (isa<Function>(Entry)) {
      return true;
    }
    auto *CE = dyn_cast<ConstantExpr>(Entry);
    if (CE == nullptr || !CE->isCast()) {
      continue;
    }
    if (isa<Function>(CE->getOperand(0))) {
      return true;
    }
  }

  return false;
}

std::optional<int64_t> getSigned64ConstantAddress(const llvm::ConstantInt &CI) {
  const llvm::APInt &Value = CI.getValue();
  if (!Value.isSignedIntN(64)) {
    llvm::errs() << "Warning: skip constant memory field outside int64 range: "
                 << CI << "\n";
    return std::nullopt;
  }
  return Value.getSExtValue();
}

std::optional<int64_t> getSigned64APInt(const llvm::APInt &Value) {
  if (!Value.isSignedIntN(64)) {
    return std::nullopt;
  }
  return Value.getSExtValue();
}

std::optional<OffsetRange> getGEPOffsetRange(const llvm::GEPOperator &GEP,
                                             const llvm::DataLayout &DL) {
  if (GEP.getPointerOperandType()->isVectorTy() || GEP.getType()->isVectorTy()) {
    return std::nullopt;
  }

  auto BitWidth = DL.getIndexTypeSizeInBits(GEP.getPointerOperandType());
  llvm::APInt ConstantOffset(BitWidth, 0, true);
  if (GEP.accumulateConstantOffset(DL, ConstantOffset)) {
    auto Offset = getSigned64APInt(ConstantOffset);
    if (!Offset) {
      return std::nullopt;
    }
    return OffsetRange{.offset = *Offset};
  }

  llvm::SmallMapVector<llvm::Value *, llvm::APInt, 4> VariableOffsets;
  ConstantOffset = llvm::APInt(BitWidth, 0, true);
  if (!GEP.collectOffset(DL, BitWidth, VariableOffsets, ConstantOffset)) {
    return std::nullopt;
  }

  auto Offset = getSigned64APInt(ConstantOffset);
  if (!Offset) {
    return std::nullopt;
  }

  OffsetRange Range{.offset = *Offset};
  for (const auto &Entry : VariableOffsets) {
    auto Scale = getSigned64APInt(Entry.second);
    if (!Scale) {
      return std::nullopt;
    }
    if (*Scale == 0) {
      continue;
    }
    // OffsetRange can model array-like dynamic steps, but later record sizing
    // assumes non-negative access sizes. Keep negative dynamic addressing out
    // of field constraints until the type builder has a representation for it.
    if (*Scale < 0) {
      return std::nullopt;
    }
    Range.access.emplace_back(*Scale);
  }
  return Range;
}

bool lowerGEPOperatorAsPtrAdd(ConstraintsGenerator &CG,
                              const llvm::GEPOperator &GEP,
                              llvm::User &GEPUser, const llvm::DataLayout &DL,
                              ExtValuePtr Result) {
  if (GEP.getPointerOperand()->getName().starts_with("table_")) {
    return false;
  }

  auto *BaseValue = const_cast<llvm::Value *>(GEP.getPointerOperand());
  auto Base = getExtValuePtr(BaseValue, &GEPUser,
                             llvm::GEPOperator::getPointerOperandIndex());
  if (GEP.hasAllZeroIndices()) {
    CG.getOrInsertNode(Base);
    CG.addRemapType(Result, Base);
    return true;
  }

  auto Offset = getGEPOffsetRange(GEP, DL);
  if (!Offset || Offset->hasNegativeBaseOffset()) {
    return false;
  }

  CG.setAsPtrAdd(Base, Result, std::move(*Offset));
  return true;
}

bool lowerConstantExprGEPAddress(ConstraintsGenerator &CG, llvm::Value *Ptr,
                                 llvm::User &User, long OpInd,
                                 const llvm::DataLayout &DL) {
  auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(Ptr);
  if (CE == nullptr) {
    return false;
  }

  if (CE->getOpcode() == llvm::Instruction::BitCast) {
    auto *InnerCE = llvm::dyn_cast<llvm::ConstantExpr>(CE->getOperand(0));
    if (InnerCE == nullptr ||
        InnerCE->getOpcode() != llvm::Instruction::GetElementPtr) {
      return false;
    }

    auto InnerResult = getExtValuePtr(InnerCE, CE, 0);
    if (!lowerGEPOperatorAsPtrAdd(CG, llvm::cast<llvm::GEPOperator>(*InnerCE),
                                  *InnerCE, DL, InnerResult)) {
      return false;
    }

    auto BitCastResult = getExtValuePtr(CE, &User, OpInd);
    CG.addRemapType(BitCastResult, InnerResult);
    return true;
  }

  if (CE->getOpcode() != llvm::Instruction::GetElementPtr) {
    return false;
  }

  auto Result = getExtValuePtr(CE, &User, OpInd);
  return lowerGEPOperatorAsPtrAdd(CG, llvm::cast<llvm::GEPOperator>(*CE), *CE,
                                  DL, Result);
}

[[noreturn]] void failSignatureOverride(llvm::StringRef Path,
                                        llvm::StringRef Message) {
  llvm::errs() << "Error: invalid MLsub override";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

[[noreturn]] void failExtraConstraints(llvm::StringRef Path,
                                       llvm::StringRef Message) {
  llvm::errs() << "Error: invalid MLsub extra constraints";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

[[noreturn]] void failPolyPolicy(llvm::StringRef Path,
                                 llvm::StringRef Message) {
  llvm::errs() << "Error: invalid NOTDEC_POLY_FUNCS policy";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

[[noreturn]] void failPNDiffPolicy(llvm::StringRef Path,
                                   llvm::StringRef Message) {
  llvm::errs() << "Error: invalid NOTDEC_PNDIFF_POLICY_OVERRIDE policy";
  if (!Path.empty()) {
    llvm::errs() << " at " << Path;
  }
  llvm::errs() << ": " << Message << "\n";
  std::abort();
}

const llvm::json::Object &requireObject(const llvm::json::Value &Value,
                                        llvm::StringRef Path) {
  if (const auto *Obj = Value.getAsObject()) {
    return *Obj;
  }
  failSignatureOverride(Path, "expected object");
}

const llvm::json::Array &requireArray(const llvm::json::Value &Value,
                                      llvm::StringRef Path) {
  if (const auto *Arr = Value.getAsArray()) {
    return *Arr;
  }
  failSignatureOverride(Path, "expected array");
}

llvm::StringRef requireString(const llvm::json::Object &Obj,
                              llvm::StringRef Key, llvm::StringRef Path) {
  if (auto Value = Obj.getString(Key)) {
    return *Value;
  }
  failSignatureOverride(
      Path,
      ("missing or invalid string field '" + Key.str() + "'").c_str());
}

int64_t requireInteger(const llvm::json::Object &Obj, llvm::StringRef Key,
                       llvm::StringRef Path) {
  if (auto Value = Obj.getInteger(Key)) {
    return *Value;
  }
  failSignatureOverride(
      Path,
      ("missing or invalid integer field '" + Key.str() + "'").c_str());
}

std::string appendJSONPath(llvm::StringRef Base, llvm::StringRef Suffix) {
  std::string Result = Base.str();
  if (!Result.empty()) {
    Result += ".";
  }
  Result += Suffix.str();
  return Result;
}

std::string appendJSONIndexPath(llvm::StringRef Base, size_t Index) {
  return Base.str() + "[" + std::to_string(Index) + "]";
}

PolyPolicyConfig loadPolyPolicyConfig() {
  PolyPolicyConfig Config;
  auto *PolicyFile = std::getenv("NOTDEC_POLY_FUNCS");
  if (PolicyFile == nullptr) {
    return Config;
  }

  auto Content = readFileToString(PolicyFile);
  auto ValE = json::parse(Content);
  if (!ValE) {
    failPolyPolicy(PolicyFile, "JSON parse failed");
  }

  auto *Obj = ValE->getAsObject();
  if (Obj == nullptr) {
    failPolyPolicy(PolicyFile, "expected top-level object");
  }

  auto *PolyArray = Obj->getArray("poly_funcs");
  if (PolyArray == nullptr) {
    failPolyPolicy(PolicyFile, "missing array field 'poly_funcs'");
  }
  for (size_t Index = 0; Index < PolyArray->size(); ++Index) {
    auto Name = (*PolyArray)[Index].getAsString();
    if (!Name) {
      failPolyPolicy(appendJSONIndexPath("poly_funcs", Index),
                     "expected string");
    }
    Config.PolyFuncs.insert(Name->str());
  }

  if (auto *LevelObj = Obj->getObject("level_override")) {
    for (auto &Ent : *LevelObj) {
      auto Level = Ent.second.getAsInteger();
      if (!Level || *Level < 0) {
        failPolyPolicy(appendJSONPath("level_override", Ent.first),
                       "expected non-negative integer");
      }
      Config.LevelOverrides[Ent.first.str()] =
          static_cast<unsigned int>(*Level);
    }
  }

  return Config;
}

PNDiffPolicyConfig loadPNDiffPolicyConfig() {
  PNDiffPolicyConfig Config;
  auto *PolicyFile = std::getenv("NOTDEC_PNDIFF_POLICY_OVERRIDE");
  if (PolicyFile == nullptr || std::strlen(PolicyFile) == 0) {
    return Config;
  }

  auto Parsed = llvm::json::parse(readFileToString(PolicyFile));
  if (!Parsed) {
    failPNDiffPolicy(PolicyFile, "JSON parse failed");
  }

  auto *Obj = Parsed->getAsObject();
  if (Obj == nullptr) {
    failPNDiffPolicy(PolicyFile, "expected top-level object");
  }

  if (auto Version = Obj->getInteger("version")) {
    if (*Version != 1) {
      failPNDiffPolicy(PolicyFile, "unsupported version");
    }
  }

  if (auto *IntConstantPolicy = Obj->getObject("int_constant_policy")) {
    if (auto Threshold =
            IntConstantPolicy->getInteger("non_pointer_abs_lt")) {
      Config.nonPointerAbsLt = *Threshold;
    }
    if (auto ExcludeZero = IntConstantPolicy->getBoolean("exclude_zero")) {
      Config.excludeZero = *ExcludeZero;
    }
  }

  return Config;
}

bool hasFunctionTypeOverride(const llvm::json::Object &Spec,
                             llvm::StringRef FuncPath,
                             bool RequireFunctionType) {
  bool HasArgs = Spec.get("args") != nullptr;
  bool HasRet = Spec.get("ret") != nullptr;
  if (HasArgs != HasRet) {
    failSignatureOverride(
        FuncPath, "override must contain both 'args' and 'ret' or neither");
  }
  if (RequireFunctionType && !HasArgs) {
    failSignatureOverride(FuncPath,
                          "override requires both 'args' and 'ret'");
  }
  return HasArgs;
}

constexpr llvm::StringLiteral kBuiltinEVMI256SemanticLattice = R"dot(
digraph evm_i256_semantics {
  graph [base="uint", bits="256", namespace="evm"];

  root [kind="root", display_name="evm_word"];
  integer;
  address;
  storage_key;
  bytes;
  string;

  integer -> root;
  address -> root;
  storage_key -> root;
  bytes -> root;
  string -> bytes;
}
)dot";

std::string getOverrideArchName(const llvm::Module &M) {
  StringRef Triple = M.getTargetTriple().getTriple();
  if (Triple.starts_with("evm")) {
    return "evm";
  }
  if (Triple.starts_with("wasm32") || Triple.starts_with("wasm64")) {
    return "wasm";
  }

  auto Arch = Triple.split('-').first;
  return Arch.empty() ? std::string("unknown") : Arch.str();
}

bool overrideAllowedForModule(const llvm::json::Object &Spec,
                              llvm::StringRef Path, const llvm::Module &M) {
  auto *AllowedValue = Spec.get("allowed_arch");
  if (AllowedValue == nullptr) {
    return true;
  }

  const auto *Allowed = AllowedValue->getAsArray();
  if (Allowed == nullptr) {
    failSignatureOverride(appendJSONPath(Path, "allowed_arch"),
                          "expected array of strings");
  }

  auto CurrentArch = getOverrideArchName(M);
  for (size_t Index = 0; Index < Allowed->size(); ++Index) {
    auto EntryPath =
        appendJSONIndexPath(appendJSONPath(Path, "allowed_arch"), Index);
    auto Arch = (*Allowed)[Index].getAsString();
    if (!Arch) {
      failSignatureOverride(EntryPath, "expected string");
    }
    if (*Arch == CurrentArch) {
      return true;
    }
  }
  return false;
}

binarysub::expected<void, binarysub::Error>
registerBuiltinPrimitiveSemanticLatticesForModule(const llvm::Module &M) {
  if (getOverrideArchName(M) != "evm") {
    return {};
  }

  auto &Registry = binarysub::globalPrimitiveSemanticRegistry();
  if (Registry.findFamily("uint", 256, "evm") != nullptr) {
    return {};
  }

  auto Family = Registry.registerFamilyFromDot(kBuiltinEVMI256SemanticLattice,
                                               "<builtin-evm-i256>");
  if (!Family) {
    return binarysub::make_unexpected(Family.error());
  }
  return {};
}

enum class OverridePNDiffState {
  Ptr,
  Number,
};

llvm::StringRef getOverridePNDiffStateName(OverridePNDiffState State) {
  switch (State) {
  case OverridePNDiffState::Ptr:
    return "ptr";
  case OverridePNDiffState::Number:
    return "number";
  }
  llvm_unreachable("unhandled OverridePNDiffState");
}

std::string formatLLVMType(llvm::Type *Ty) {
  std::string Buffer;
  llvm::raw_string_ostream OS(Buffer);
  if (Ty == nullptr) {
    OS << "<null>";
  } else {
    Ty->print(OS);
  }
  return Buffer;
}

void warnIgnoredPNDiffOverride(llvm::StringRef Kind, llvm::StringRef Path,
                               llvm::StringRef TargetKey,
                               OverridePNDiffState State,
                               const ExtValuePtr &Value,
                               bool *WroteHeader) {
  std::string Message;
  llvm::raw_string_ostream OS(Message);
  OS << "Warning: skip MLsub " << Kind << " pndiff";
  if (!Path.empty()) {
    OS << " at " << Path;
  }
  OS << ": target '" << TargetKey << "' has non-PNDiff LLVM type '"
     << formatLLVMType(getType(Value))
     << "'; requested state '"
     << getOverridePNDiffStateName(State)
     << "' will be ignored\n";
  OS.flush();

  if (WroteHeader != nullptr && notdec::hasWorkDir()) {
    if (!*WroteHeader) {
      notdec::appendWorkDirLog(
          kPNDiffWarnFile,
          "# Override / validation warnings\n\n");
      *WroteHeader = true;
    }
    notdec::appendWorkDirLog(kPNDiffWarnFile, Message);
    return;
  }

  llvm::errs() << Message;
}

bool isPointerSizedNumberProjection(const llvm::Value &Source,
                                    const llvm::Instruction &User,
                                    unsigned PointerSize) {
  auto *SourceTy = llvm::dyn_cast<llvm::IntegerType>(Source.getType());
  auto *UserTy = llvm::dyn_cast<llvm::IntegerType>(User.getType());
  if (SourceTy == nullptr || UserTy == nullptr) {
    return false;
  }

  auto SourceBits = SourceTy->getBitWidth();
  auto UserBits = UserTy->getBitWidth();
  if (UserBits != PointerSize || SourceBits == PointerSize) {
    return false;
  }

  if (SourceBits > PointerSize) {
    return llvm::isa<llvm::TruncInst>(User);
  }
  return llvm::isa<llvm::ZExtInst, llvm::SExtInst>(User);
}

llvm::SmallVector<llvm::Value *, 4>
collectNumberOverrideUserScanSeeds(const ExtValuePtr &Value) {
  llvm::SmallVector<llvm::Value *, 4> Seeds;
  if (auto *V = std::get_if<llvm::Value *>(&Value)) {
    if (*V != nullptr) {
      Seeds.push_back(*V);
    }
    return Seeds;
  }

  auto *Ret = std::get_if<ReturnValue>(&Value);
  if (Ret == nullptr || Ret->Func == nullptr) {
    return Seeds;
  }

  for (auto *User : Ret->Func->users()) {
    auto *Call = llvm::dyn_cast<llvm::CallBase>(User);
    if (Call == nullptr || Call->getCalledFunction() != Ret->Func ||
        Call->getType()->isVoidTy()) {
      continue;
    }
    Seeds.push_back(Call);
  }
  return Seeds;
}

bool applyNumberOverrideToCompatibleCastUsers(ConstraintsGenerator &G,
                                              const ExtValuePtr &Value) {
  bool Applied = false;
  llvm::SmallPtrSet<llvm::Instruction *, 8> Seen;
  auto Seeds = collectNumberOverrideUserScanSeeds(Value);
  for (auto *Seed : Seeds) {
    if (Seed == nullptr || !Seed->getType()->isIntegerTy()) {
      continue;
    }
    for (auto *User : Seed->users()) {
      auto *Inst = llvm::dyn_cast<llvm::Instruction>(User);
      if (Inst == nullptr || !Seen.insert(Inst).second ||
          !isPointerSizedNumberProjection(*Seed, *Inst, G.PointerSize)) {
        continue;
      }
      auto *Node = G.getPNINodeOrNull(Inst);
      if (Node == nullptr || !Node->isPNRelated()) {
        continue;
      }
      Node->setNonPtr();
      Applied = true;
    }
  }
  return Applied;
}

struct ResolvedPNDiffTarget {
  ExtValuePtr Value;
  std::string Key;
};

struct ResolvedConstraintTarget {
  ExtValuePtr Value;
  std::string Key;
};

ResolvedConstraintTarget resolveSummaryConstraintTarget(
    const llvm::json::Object &Obj, llvm::Function &Func,
    llvm::StringRef Path) {
  auto Kind = Obj.getString("kind");
  if (!Kind) {
    failSignatureOverride(Path, "missing or invalid string field 'kind'");
  }
  if (*Kind == "ret") {
    if (Func.getReturnType()->isVoidTy()) {
      failSignatureOverride(Path, "ret target requires non-void function");
    }
    return {
        .Value = ReturnValue{.Func = &Func},
        .Key = "ret",
    };
  }
  if (*Kind == "arg") {
    auto Index = Obj.getInteger("index");
    if (!Index) {
      failSignatureOverride(Path,
                            "missing or invalid integer field 'index'");
    }
    if (*Index < 0 || *Index >= static_cast<int64_t>(Func.arg_size())) {
      failSignatureOverride(Path, "arg target index out of range");
    }
    auto *Arg = Func.getArg(static_cast<unsigned>(*Index));
    assert(Arg != nullptr);
    return {
        .Value = Arg,
        .Key = "arg:" + std::to_string(*Index),
    };
  }
  failSignatureOverride(Path, "summary constraint target kind must be 'arg' or 'ret'");
}

bool isSummaryConstraintTargetOperand(const llvm::json::Value &Value) {
  const auto *Obj = Value.getAsObject();
  if (Obj == nullptr) {
    return false;
  }
  auto Kind = Obj->getString("kind");
  return Kind && (*Kind == "arg" || *Kind == "ret");
}

const llvm::json::Object *
getExtraConstraintBindings(const llvm::json::Object &Spec,
                           llvm::StringRef FuncPath) {
  auto *BindingsValue = Spec.get("bindings");
  if (BindingsValue == nullptr) {
    return nullptr;
  }
  const auto *Bindings = BindingsValue->getAsObject();
  if (Bindings == nullptr) {
    failExtraConstraints(appendJSONPath(FuncPath, "bindings"), "expected object");
  }
  for (const auto &Ent : *Bindings) {
    auto BindingPath =
        appendJSONPath(appendJSONPath(FuncPath, "bindings"), Ent.first);
    if (Ent.second.getAsObject() == nullptr) {
      failExtraConstraints(BindingPath, "expected object");
    }
  }
  return Bindings;
}

llvm::Instruction *findFirstNamedInstruction(llvm::Function &Func,
                                             llvm::StringRef Name) {
  for (llvm::BasicBlock &BB : Func) {
    for (llvm::Instruction &I : BB) {
      if (I.getType()->isVoidTy()) {
        continue;
      }
      if (!I.hasName()) {
        continue;
      }
      if (I.getName() == Name) {
        return &I;
      }
    }
  }
  return nullptr;
}

llvm::Instruction *findInstructionByStableId(llvm::Function &Func,
                                             llvm::StringRef StableId) {
  for (llvm::BasicBlock &BB : Func) {
    for (llvm::Instruction &I : BB) {
      if (toStableString(ExtValuePtr(&I)) == StableId) {
        return &I;
      }
    }
  }
  return nullptr;
}

llvm::Instruction *resolveOperandInstructionSelector(
    const llvm::json::Object &TargetObj, llvm::Function &Func,
    llvm::StringRef TargetPath, std::string &SelectorLabel) {
  bool HasInst = TargetObj.get("inst") != nullptr;
  bool HasName = TargetObj.get("name") != nullptr;
  if (HasInst == HasName) {
    failExtraConstraints(
        TargetPath,
        "operand target must contain exactly one of 'inst' or 'name'");
  }

  if (HasInst) {
    auto InstId = TargetObj.getString("inst");
    if (!InstId) {
      failExtraConstraints(TargetPath,
                           "missing or invalid string field 'inst'");
    }
    auto *Inst = findInstructionByStableId(Func, *InstId);
    if (Inst == nullptr) {
      failExtraConstraints(
          TargetPath,
          ("operand target instruction '" + InstId->str() +
           "' not found in function")
              .c_str());
    }
    SelectorLabel = InstId->str();
    return Inst;
  }

  auto Name = TargetObj.getString("name");
  if (!Name) {
    failExtraConstraints(TargetPath, "missing or invalid string field 'name'");
  }
  auto *Inst = findFirstNamedInstruction(Func, *Name);
  if (Inst == nullptr) {
    failExtraConstraints(
        TargetPath,
        ("operand target name '" + Name->str() +
         "' not found as a non-void instruction in function")
            .c_str());
  }
  SelectorLabel = "name:" + Name->str();
  return Inst;
}

void appendOverrideConstraints(
    notdec::mlsub::MLsubRecovery::OverrideTypeRecipe &Into,
    const notdec::mlsub::MLsubRecovery::OverrideTypeRecipe &From) {
  Into.Constraints.insert(Into.Constraints.end(),
                          From.Constraints.begin(), From.Constraints.end());
}

constexpr llvm::StringLiteral kDefaultMLsubBuiltinSummaryOverridePath(
    NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH);

llvm::json::Value makeEmptyOverrideDoc() {
  return llvm::json::Object{{"version", int64_t(1)},
                            {"functions", llvm::json::Object{}}};
}

std::string computeSHA256Hex(llvm::StringRef Content) {
  llvm::SHA256 Hash;
  Hash.update(Content);
  return llvm::toHex(Hash.final(), true);
}

std::string renderModuleToString(const llvm::Module &M) {
  std::string Buffer;
  llvm::raw_string_ostream OS(Buffer);
  M.print(OS, nullptr);
  OS.flush();
  return Buffer;
}

void writeJSONFile(llvm::StringRef Path, const llvm::json::Value &Doc) {
  std::error_code EC;
  llvm::raw_fd_ostream OS(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Cannot open output file " << Path << ": "
                 << EC.message() << "\n";
    std::abort();
  }
  OS << llvm::formatv("{0:2}", Doc) << "\n";
}

void writeMLsubInputAnchor(const llvm::Module &M, llvm::StringRef AnchorPath,
                           llvm::StringRef ModuleSHA256Hex) {
  llvm::json::Object Anchor;
  Anchor["version"] = int64_t(1);
  Anchor["stage"] = "mlsub-input";
  Anchor["ir_file"] = kMLsubInputIRFile.str();
  Anchor["sha256"] = ModuleSHA256Hex.str();
  Anchor["data_layout"] = M.getDataLayout().getStringRepresentation();
  Anchor["target_triple"] = M.getTargetTriple().str();
  writeJSONFile(AnchorPath, llvm::json::Value(std::move(Anchor)));
}

void writeTRInputModule(llvm::Module &M, llvm::StringRef OutputPath) {
  auto Suffix = getSuffix(OutputPath.str());
  if (Suffix == ".ll") {
    printModule(M, OutputPath.str().c_str());
    return;
  }
  if (Suffix == ".bc") {
    std::error_code EC;
    llvm::raw_fd_ostream OS(OutputPath, EC);
    if (EC) {
      llvm::errs() << "Cannot open TR input IR output file " << OutputPath
                   << ": " << EC.message() << "\n";
      std::abort();
    }
    llvm::WriteBitcodeToFile(M, OS);
    return;
  }
  llvm::errs() << "Error: --emit-tr-input-ir expects output suffix .ll or .bc, "
                  "got "
               << OutputPath << "\n";
  std::abort();
}

void writeSelectableValues(const llvm::Module &M, llvm::StringRef Path) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Cannot open output file " << Path << ": "
                 << EC.message() << "\n";
    std::abort();
  }

  Out << "# Selectable MLsub extra-constraint targets\n\n";
  for (const llvm::Function &Func : M) {
    if (Func.isDeclaration()) {
      continue;
    }
    Out << "## Function: " << Func.getName() << "\n";
    for (const llvm::Argument &Arg : Func.args()) {
      Out << toStableString(ExtValuePtr(const_cast<llvm::Argument *>(&Arg)))
          << "\n";
    }
    if (!Func.getReturnType()->isVoidTy()) {
      Out << toStableString(ReturnValue{
                 .Func = const_cast<llvm::Function *>(&Func)})
          << "\n";
    }
    for (const llvm::BasicBlock &BB : Func) {
      for (const llvm::Instruction &I : BB) {
        auto InstId =
            toStableString(ExtValuePtr(const_cast<llvm::Instruction *>(&I)));
        Out << InstId << "\n";
        if (!I.getType()->isVoidTy() && I.hasName()) {
          Out << "named_value(" << I.getName() << ") -> " << InstId << "\n";
        }
        for (unsigned Index = 0; Index < I.getNumOperands(); ++Index) {
          auto OperandStable = toStableString(getExtValuePtr(
              I.getOperand(Index), const_cast<llvm::Instruction *>(&I), Index));
          Out << "operand(inst=\"" << InstId << "\", index=" << Index
              << ") -> " << OperandStable << "\n";
          if (!I.getType()->isVoidTy() && I.hasName()) {
            Out << "operand(name=\"" << I.getName() << "\", index=" << Index
                << ") -> " << OperandStable << "\n";
          }
        }
      }
    }
    Out << "\n";
  }
}

llvm::StringRef getOverrideKindLabel(bool RequireDefinition) {
  return RequireDefinition ? "signature" : "summary";
}

OverridePNDiffState parsePNDiffState(const llvm::json::Object &Obj,
                                     llvm::StringRef Path) {
  auto State = requireString(Obj, "state", Path);
  if (State == "ptr") {
    return OverridePNDiffState::Ptr;
  }
  if (State == "number" || State == "num") {
    return OverridePNDiffState::Number;
  }
  failSignatureOverride(Path, "pndiff state must be 'ptr' or 'number'");
}

OverridePNDiffState parseExtraConstraintPNDiffState(const llvm::json::Object &Obj,
                                                    llvm::StringRef Path) {
  auto State = Obj.getString("state");
  if (!State) {
    failExtraConstraints(Path, "missing or invalid string field 'state'");
  }
  if (*State == "ptr") {
    return OverridePNDiffState::Ptr;
  }
  if (*State == "number" || *State == "num") {
    return OverridePNDiffState::Number;
  }
  failExtraConstraints(Path, "pndiff state must be 'ptr' or 'number'");
}

ResolvedPNDiffTarget resolvePNDiffTarget(const llvm::json::Object &Obj,
                                        llvm::Function &Func,
                                        llvm::StringRef Path) {
  auto *TargetValue = Obj.get("target");
  if (TargetValue == nullptr) {
    failSignatureOverride(Path, "missing field 'target'");
  }
  const auto &TargetObj =
      requireObject(*TargetValue, appendJSONPath(Path, "target"));
  auto TargetPath = appendJSONPath(Path, "target");
  auto Kind = requireString(TargetObj, "kind", TargetPath);
  if (Kind == "ret") {
    if (Func.getReturnType()->isVoidTy()) {
      failSignatureOverride(TargetPath, "ret target requires non-void function");
    }
    return {
        .Value = ReturnValue{.Func = &Func},
        .Key = "ret",
    };
  }
  if (Kind == "arg") {
    auto Index = requireInteger(TargetObj, "index", TargetPath);
    if (Index < 0 || Index >= static_cast<int64_t>(Func.arg_size())) {
      failSignatureOverride(TargetPath, "arg target index out of range");
    }
    auto *Arg = Func.getArg(static_cast<unsigned>(Index));
    assert(Arg != nullptr);
    return {
        .Value = Arg,
        .Key = "arg:" + std::to_string(Index),
    };
  }
  failSignatureOverride(TargetPath, "pndiff target kind must be 'arg' or 'ret'");
}

ResolvedConstraintTarget resolveExtraConstraintTargetSelector(
    const llvm::json::Object &TargetObj, llvm::Function &Func,
    const llvm::json::Object *Bindings, llvm::StringRef TargetPath,
    bool ForPNDiff, std::set<std::string> &ResolvingBindings) {
  auto Kind = TargetObj.getString("kind");
  if (!Kind) {
    failExtraConstraints(TargetPath, "missing or invalid string field 'kind'");
  }
  if (*Kind == "ret") {
    if (Func.getReturnType()->isVoidTy()) {
      failExtraConstraints(TargetPath, "ret target requires non-void function");
    }
    return {
        .Value = ReturnValue{.Func = &Func},
        .Key = "ret",
    };
  }
  if (*Kind == "arg") {
    auto Index = TargetObj.getInteger("index");
    if (!Index) {
      failExtraConstraints(TargetPath,
                           "missing or invalid integer field 'index'");
    }
    if (*Index < 0 || *Index >= static_cast<int64_t>(Func.arg_size())) {
      failExtraConstraints(TargetPath, "arg target index out of range");
    }
    auto *Arg = Func.getArg(static_cast<unsigned>(*Index));
    assert(Arg != nullptr);
    return {
        .Value = Arg,
        .Key = "arg:" + std::to_string(*Index),
    };
  }
  if (*Kind == "named_value") {
    auto Name = TargetObj.getString("name");
    if (!Name) {
      failExtraConstraints(TargetPath,
                           "missing or invalid string field 'name'");
    }
    auto *Inst = findFirstNamedInstruction(Func, *Name);
    if (Inst == nullptr) {
      failExtraConstraints(
          TargetPath,
          ("named_value target '" + Name->str() +
           "' not found as a non-void instruction in function")
              .c_str());
    }
    return {
        .Value = Inst,
        .Key = "named_value:" + Name->str(),
    };
  }
  if (*Kind == "inst") {
    auto Id = TargetObj.getString("id");
    if (!Id) {
      failExtraConstraints(TargetPath, "missing or invalid string field 'id'");
    }
    auto *Inst = findInstructionByStableId(Func, *Id);
    if (Inst == nullptr) {
      failExtraConstraints(
          TargetPath,
          ("inst target '" + Id->str() + "' not found in function").c_str());
    }
    if (Inst->getType()->isVoidTy()) {
      failExtraConstraints(TargetPath,
                           "inst target requires non-void instruction");
    }
    return {
        .Value = Inst,
        .Key = "inst:" + Id->str(),
    };
  }
  if (*Kind == "operand") {
    auto Index = TargetObj.getInteger("index");
    if (!Index) {
      failExtraConstraints(TargetPath,
                           "missing or invalid integer field 'index'");
    }
    std::string SelectorLabel;
    auto *Inst = resolveOperandInstructionSelector(TargetObj, Func, TargetPath,
                                                   SelectorLabel);
    if (*Index < 0 ||
        *Index >= static_cast<int64_t>(Inst->getNumOperands())) {
      failExtraConstraints(TargetPath, "operand target index out of range");
    }
    auto OpIndex = static_cast<unsigned>(*Index);
    auto *Operand = Inst->getOperand(OpIndex);
    return {
        .Value = getExtValuePtr(Operand, Inst, *Index),
        .Key = "operand:" + SelectorLabel + ":" + std::to_string(OpIndex),
    };
  }
  if (*Kind == "binding") {
    auto Name = TargetObj.getString("name");
    if (!Name) {
      failExtraConstraints(TargetPath,
                           "missing or invalid string field 'name'");
    }
    if (Bindings == nullptr) {
      failExtraConstraints(TargetPath,
                           "binding target requires function-level 'bindings'");
    }
    std::string BindingName = Name->str();
    if (!ResolvingBindings.insert(BindingName).second) {
      failExtraConstraints(TargetPath,
                           ("binding cycle detected for '" + BindingName + "'")
                               .c_str());
    }
    auto *BindingValue = Bindings->get(*Name);
    if (BindingValue == nullptr) {
      failExtraConstraints(
          TargetPath, ("binding '" + BindingName + "' not found").c_str());
    }
    auto BindingPath = appendJSONPath("bindings", *Name);
    const auto *BindingObj = BindingValue->getAsObject();
    if (BindingObj == nullptr) {
      failExtraConstraints(BindingPath, "expected object");
    }
    auto Resolved = resolveExtraConstraintTargetSelector(
        *BindingObj, Func, Bindings, BindingPath, ForPNDiff, ResolvingBindings);
    ResolvingBindings.erase(BindingName);
    return Resolved;
  }
  if (ForPNDiff) {
    failExtraConstraints(
        TargetPath,
        "pndiff target kind must be 'arg', 'ret', 'named_value', 'inst', 'operand', or 'binding'");
  }
  failExtraConstraints(
      TargetPath,
      "target kind must be 'arg', 'ret', 'named_value', 'inst', 'operand', or 'binding' currently");
}

ResolvedPNDiffTarget resolveExtraConstraintPNDiffTarget(
    const llvm::json::Object &Obj, llvm::Function &Func,
    const llvm::json::Object *Bindings, llvm::StringRef Path) {
  auto *TargetValue = Obj.get("target");
  if (TargetValue == nullptr) {
    failExtraConstraints(Path, "missing field 'target'");
  }
  const auto *TargetObj = TargetValue->getAsObject();
  auto TargetPath = appendJSONPath(Path, "target");
  if (TargetObj == nullptr) {
    failExtraConstraints(TargetPath, "expected object");
  }
  std::set<std::string> ResolvingBindings;
  auto Resolved = resolveExtraConstraintTargetSelector(
      *TargetObj, Func, Bindings, TargetPath, true, ResolvingBindings);
  return {
      .Value = Resolved.Value,
      .Key = Resolved.Key,
  };
}

ResolvedConstraintTarget resolveExtraConstraintTarget(
    const llvm::json::Object &Obj, llvm::Function &Func,
    const llvm::json::Object *Bindings, llvm::StringRef Path) {
  auto *TargetValue = Obj.get("target");
  if (TargetValue == nullptr) {
    failExtraConstraints(Path, "missing field 'target'");
  }
  const auto *TargetObj = TargetValue->getAsObject();
  auto TargetPath = appendJSONPath(Path, "target");
  if (TargetObj == nullptr) {
    failExtraConstraints(TargetPath, "expected object");
  }
  std::set<std::string> ResolvingBindings;
  return resolveExtraConstraintTargetSelector(*TargetObj, Func, Bindings,
                                              TargetPath, false,
                                              ResolvingBindings);
}

void validateExtraConstraintOperand(const llvm::json::Value &Value,
                                    llvm::Function &Func,
                                    const llvm::json::Object *Bindings,
                                    llvm::StringRef Path) {
  const auto *Obj = Value.getAsObject();
  if (Obj == nullptr) {
    failExtraConstraints(Path, "expected object");
  }
  bool HasType = Obj->get("type") != nullptr;
  bool HasTarget = Obj->get("target") != nullptr;
  if (HasType == HasTarget) {
    failExtraConstraints(
        Path, "operand must contain exactly one of 'type' or 'target'");
  }
  if (HasTarget) {
    resolveExtraConstraintTarget(*Obj, Func, Bindings, Path);
    return;
  }
  if (Obj->get("type")->getAsObject() == nullptr &&
      !Obj->get("type")->getAsNull()) {
    failExtraConstraints(appendJSONPath(Path, "type"), "expected object");
  }
}

notdec::mlsub::MLsubRecovery::OverrideTypeRecipe buildExtraConstraintOperand(
    notdec::mlsub::MLsubRecovery &Recovery, ConstraintsGenerator &G,
    notdec::mlsub::MLsubRecovery::OverrideBuildContext &Ctx,
    llvm::Function &Func, const llvm::json::Object *Bindings,
    const llvm::json::Value &Value, llvm::StringRef Path) {
  validateExtraConstraintOperand(Value, Func, Bindings, Path);
  const auto &Obj = *Value.getAsObject();
  if (auto *TypeValue = Obj.get("type")) {
    return Recovery.buildOverrideType(*TypeValue, Ctx,
                                      appendJSONPath(Path, "type"), false);
  }

  auto Target = resolveExtraConstraintTarget(Obj, Func, Bindings, Path);
  auto Node = G.getNodeOrNull(Target.Value);
  if (Node == nullptr) {
    failExtraConstraints(
        Path, ("target '" + Target.Key + "' has no corresponding node").c_str());
  }
  notdec::mlsub::MLsubRecovery::OverrideTypeRecipe Recipe;
  Recipe.Root = Node;
  return Recipe;
}

void applyPNDiffOverrides(ConstraintsGenerator &G, llvm::Function &Func,
                          const llvm::json::Object &Spec,
                          llvm::StringRef FuncPath, bool *WroteHeader) {
  auto *PNDiffValue = Spec.get("pndiff");
  if (PNDiffValue == nullptr) {
    return;
  }

  const auto &PNDiffArray =
      requireArray(*PNDiffValue, appendJSONPath(FuncPath, "pndiff"));
  std::map<std::string, OverridePNDiffState> SeenTargets;
  for (size_t Index = 0; Index < PNDiffArray.size(); ++Index) {
    auto EntryPath =
        appendJSONIndexPath(appendJSONPath(FuncPath, "pndiff"), Index);
    const auto &EntryObj = requireObject(PNDiffArray[Index], EntryPath);
    auto Target = resolvePNDiffTarget(EntryObj, Func, EntryPath);
    auto State = parsePNDiffState(EntryObj, EntryPath);

    auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
    if (!Inserted && It->second != State) {
      failSignatureOverride(
          EntryPath,
          ("conflicting duplicate pndiff target '" + Target.Key + "'").c_str());
    }
    if (!Inserted) {
      continue;
    }

    auto *Node = G.getPNINodeOrNull(Target.Value);
    if (Node == nullptr) {
      failSignatureOverride(
          EntryPath,
          ("pndiff target '" + Target.Key + "' has no corresponding node")
              .c_str());
    }
    if (!Node->isPNRelated()) {
      if (State == OverridePNDiffState::Number) {
        if (applyNumberOverrideToCompatibleCastUsers(G, Target.Value)) {
          continue;
        }
        warnIgnoredPNDiffOverride("override", EntryPath, Target.Key, State,
                                  Target.Value, WroteHeader);
        continue;
      }
      failSignatureOverride(
          EntryPath,
          ("pndiff target '" + Target.Key +
           "' is not pointer-sized int or pointer")
              .c_str());
    }

    switch (State) {
    case OverridePNDiffState::Ptr:
      Node->setPtr();
      break;
    case OverridePNDiffState::Number:
      Node->setNonPtr();
      break;
    }
  }
}

void applyExtraConstraintPNDiffs(ConstraintsGenerator &G, llvm::Function &Func,
                                 const llvm::json::Object &Spec,
                                 llvm::StringRef FuncPath,
                                 bool *WroteHeader) {
  auto *Bindings = getExtraConstraintBindings(Spec, FuncPath);
  auto *ActionsValue = Spec.get("actions");
  if (ActionsValue == nullptr) {
    return;
  }
  const auto *Actions = ActionsValue->getAsArray();
  if (Actions == nullptr) {
    failExtraConstraints(appendJSONPath(FuncPath, "actions"), "expected array");
  }

  std::map<std::string, OverridePNDiffState> SeenTargets;
  for (size_t Index = 0; Index < Actions->size(); ++Index) {
    auto ActionPath =
        appendJSONIndexPath(appendJSONPath(FuncPath, "actions"), Index);
    const auto *ActionObj = (*Actions)[Index].getAsObject();
    if (ActionObj == nullptr) {
      failExtraConstraints(ActionPath, "expected object");
    }
    auto Kind = ActionObj->getString("kind");
    if (!Kind) {
      failExtraConstraints(ActionPath, "missing or invalid string field 'kind'");
    }
    if (*Kind != "pndiff") {
      continue;
    }

    auto Target = resolveExtraConstraintPNDiffTarget(*ActionObj, Func, Bindings,
                                                     ActionPath);
    auto State = parseExtraConstraintPNDiffState(*ActionObj, ActionPath);
    auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
    if (!Inserted && It->second != State) {
      failExtraConstraints(
          ActionPath,
          ("conflicting duplicate pndiff target '" + Target.Key + "'").c_str());
    }
    if (!Inserted) {
      continue;
    }

    auto *Node = G.getPNINodeOrNull(Target.Value);
    if (Node == nullptr) {
      failExtraConstraints(
          ActionPath,
          ("pndiff target '" + Target.Key + "' has no corresponding node")
              .c_str());
    }
    if (!Node->isPNRelated()) {
      if (State == OverridePNDiffState::Number) {
        if (applyNumberOverrideToCompatibleCastUsers(G, Target.Value)) {
          continue;
        }
        warnIgnoredPNDiffOverride("extra constraints", ActionPath, Target.Key,
                                  State, Target.Value, WroteHeader);
        continue;
      }
      failExtraConstraints(
          ActionPath,
          ("pndiff target '" + Target.Key +
           "' is not pointer-sized int or pointer")
              .c_str());
    }

    switch (State) {
    case OverridePNDiffState::Ptr:
      Node->setPtr();
      break;
    case OverridePNDiffState::Number:
      Node->setNonPtr();
      break;
    }
  }
}

void loadOverrideFileImpl(llvm::Module &M, const char *Path,
                          llvm::json::Value &Doc,
                          std::set<llvm::Function *> &Funcs,
                          bool StrictValidation, bool RequireDefinition) {
  llvm::StringRef Kind = getOverrideKindLabel(RequireDefinition);
  llvm::errs() << "Loading MLsub " << Kind << " override from: " << Path
               << "\n";
  auto Parsed = llvm::json::parse(readFileToString(Path));
  if (!Parsed) {
    failSignatureOverride("",
                          ("JSON parse failed for " + std::string(Path)).c_str());
  }

  const auto &Root = requireObject(*Parsed, "<root>");
  auto Version = Root.getInteger("version");
  if (!Version || *Version != 1) {
    failSignatureOverride("<root>", "expected version = 1");
  }

  const auto *Functions = Root.getObject("functions");
  if (Functions == nullptr) {
    failSignatureOverride("<root>", "missing object field 'functions'");
  }

  for (const auto &Ent : *Functions) {
    std::string FuncPath = appendJSONPath("functions", Ent.first);
    const auto &Spec = requireObject(Ent.second, FuncPath);
    if (!overrideAllowedForModule(Spec, FuncPath, M)) {
      continue;
    }

    auto *Func = M.getFunction(Ent.first);
    if (Func == nullptr) {
      llvm::errs() << "Warning: MLsub " << Kind
                   << " override function not found: " << Ent.first << "\n";
      continue;
    }

    bool HasFunctionType =
        hasFunctionTypeOverride(Spec, FuncPath, RequireDefinition);
    if (HasFunctionType) {
      auto *Args = Spec.getArray("args");
      if (Args == nullptr) {
        failSignatureOverride(FuncPath, "field 'args' must be an array");
      }
      if (Func->isVarArg()) {
        if (StrictValidation) {
          failSignatureOverride(FuncPath,
                                "vararg functions are not supported yet");
        }
        llvm::errs() << "Warning: skip MLsub " << Kind
                     << " override for vararg " << Func->getName() << "\n";
        continue;
      }
      if (Args->size() != Func->arg_size()) {
        if (StrictValidation) {
          failSignatureOverride(
              FuncPath,
              ("override arg count does not match LLVM function '"
               + Func->getName().str() + "'")
                  .c_str());
        }
        llvm::errs() << "Warning: skip MLsub " << Kind << " override for "
                     << Func->getName() << " because arg count mismatched\n";
        continue;
      }
      if (RequireDefinition && Func->isDeclaration()) {
        if (StrictValidation) {
          failSignatureOverride(
              FuncPath, "signature override requires a function definition");
        }
        llvm::errs() << "Warning: skip MLsub signature override for declaration "
                     << Func->getName() << "\n";
        continue;
      }
    }
    if (auto *Constraints = Spec.get("constraints")) {
      const auto &ConstraintsArray =
          requireArray(*Constraints, appendJSONPath(FuncPath, "constraints"));
      for (size_t Index = 0; Index < ConstraintsArray.size(); ++Index) {
        auto ConstraintPath = appendJSONIndexPath(
            appendJSONPath(FuncPath, "constraints"), Index);
        const auto &ConstraintObj =
            requireObject(ConstraintsArray[Index], ConstraintPath);
        auto Kind = requireString(ConstraintObj, "kind", ConstraintPath);
        if (Kind != "subtype") {
          failSignatureOverride(ConstraintPath,
                                "only subtype constraints are supported");
        }
        if (ConstraintObj.get("lhs") == nullptr ||
            ConstraintObj.get("rhs") == nullptr) {
          failSignatureOverride(ConstraintPath,
                                "constraint requires lhs and rhs");
        }
      }
    }
    if (auto *PNDiff = Spec.get("pndiff")) {
      const auto &PNDiffArray =
          requireArray(*PNDiff, appendJSONPath(FuncPath, "pndiff"));
      std::map<std::string, OverridePNDiffState> SeenTargets;
      for (size_t Index = 0; Index < PNDiffArray.size(); ++Index) {
        auto EntryPath =
            appendJSONIndexPath(appendJSONPath(FuncPath, "pndiff"), Index);
        const auto &EntryObj = requireObject(PNDiffArray[Index], EntryPath);
        auto Target = resolvePNDiffTarget(EntryObj, *Func, EntryPath);
        auto State = parsePNDiffState(EntryObj, EntryPath);
        auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
        if (!Inserted && It->second != State) {
          failSignatureOverride(
              EntryPath,
              ("conflicting duplicate pndiff target '" + Target.Key + "'")
                  .c_str());
        }
      }
    }
    if (Spec.get("is_polymorphic") != nullptr &&
        !Spec.getBoolean("is_polymorphic")) {
      failSignatureOverride(appendJSONPath(FuncPath, "is_polymorphic"),
                            "expected boolean");
    }

    auto *DocRoot = Doc.getAsObject();
    assert(DocRoot != nullptr);
    auto *DocFunctions = DocRoot->getObject("functions");
    assert(DocFunctions != nullptr);
    (*DocFunctions)[Ent.first] = Ent.second;
    Funcs.insert(Func);
  }
}

const llvm::json::Value *getOverrideSpecImpl(
    const llvm::json::Value &Doc, const std::set<llvm::Function *> &Funcs,
    const llvm::Function &Func) {
  if (Funcs.count(const_cast<llvm::Function *>(&Func)) == 0) {
    return nullptr;
  }

  const auto *Root = Doc.getAsObject();
  if (Root == nullptr) {
    return nullptr;
  }
  const auto *Functions = Root->getObject("functions");
  if (Functions == nullptr) {
    return nullptr;
  }
  return Functions->get(Func.getName());
}

void validateExtraConstraintsAnchor(const llvm::json::Object &Root,
                                    const llvm::Module &M,
                                    llvm::StringRef ModuleSHA256Hex) {
  auto *AnchorValue = Root.get("ir_anchor");
  if (AnchorValue == nullptr) {
    return;
  }
  const auto *Anchor = AnchorValue->getAsObject();
  if (Anchor == nullptr) {
    failExtraConstraints("ir_anchor", "expected object");
  }

  auto Stage = requireString(*Anchor, "stage", "ir_anchor");
  if (Stage != "mlsub-input") {
    failExtraConstraints("ir_anchor.stage",
                         "expected stage = 'mlsub-input'");
  }

  auto SHA256 = Anchor->getString("sha256");
  if (!SHA256) {
    failExtraConstraints("ir_anchor.sha256",
                         "missing or invalid string field 'sha256'");
  }
  if (*SHA256 != ModuleSHA256Hex) {
    failExtraConstraints("ir_anchor.sha256",
                         ("sha256 mismatch: expected " +
                          ModuleSHA256Hex.str() + ", current " + SHA256->str())
                             .c_str());
  }

  if (auto DataLayout = Anchor->getString("data_layout")) {
    auto Current = M.getDataLayout().getStringRepresentation();
    if (*DataLayout != Current) {
      failExtraConstraints("ir_anchor.data_layout",
                           ("data layout mismatch: expected " + Current).c_str());
    }
  }

  if (auto TargetTriple = Anchor->getString("target_triple")) {
    auto Current = M.getTargetTriple().str();
    if (*TargetTriple != Current) {
      failExtraConstraints(
          "ir_anchor.target_triple",
          ("target triple mismatch: expected " + Current).c_str());
    }
  }
}

void validateExtraConstraintFunctions(const llvm::json::Object &Root,
                                      llvm::Module &M,
                                      llvm::json::Value &Doc,
                                      std::set<llvm::Function *> &Funcs) {
  auto *FunctionsValue = Root.get("functions");
  if (FunctionsValue == nullptr) {
    return;
  }
  const auto *Functions = FunctionsValue->getAsObject();
  if (Functions == nullptr) {
    failExtraConstraints("functions", "expected object");
  }

  for (const auto &Ent : *Functions) {
    std::string FuncPath = appendJSONPath("functions", Ent.first);
    const auto *Spec = Ent.second.getAsObject();
    if (Spec == nullptr) {
      failExtraConstraints(FuncPath, "expected object");
    }
    if (!overrideAllowedForModule(*Spec, FuncPath, M)) {
      continue;
    }

    auto *Func = M.getFunction(Ent.first);
    if (Func == nullptr) {
      llvm::errs() << "Warning: MLsub extra constraints function not found: "
                   << Ent.first << "\n";
      continue;
    }

    auto *ActionsValue = Spec->get("actions");
    if (ActionsValue == nullptr) {
      failExtraConstraints(FuncPath, "missing field 'actions'");
    }
    const auto *Actions = ActionsValue->getAsArray();
    if (Actions == nullptr) {
      failExtraConstraints(appendJSONPath(FuncPath, "actions"), "expected array");
    }
    auto *Bindings = getExtraConstraintBindings(*Spec, FuncPath);

    std::map<std::string, OverridePNDiffState> SeenTargets;
    for (size_t Index = 0; Index < Actions->size(); ++Index) {
      auto ActionPath =
          appendJSONIndexPath(appendJSONPath(FuncPath, "actions"), Index);
      const auto *ActionObj = (*Actions)[Index].getAsObject();
      if (ActionObj == nullptr) {
        failExtraConstraints(ActionPath, "expected object");
      }
      auto Kind = ActionObj->getString("kind");
      if (!Kind) {
        failExtraConstraints(ActionPath,
                             "missing or invalid string field 'kind'");
      }
      if (*Kind == "pndiff") {
        auto Target = resolveExtraConstraintPNDiffTarget(*ActionObj, *Func,
                                                         Bindings, ActionPath);
        auto State = parseExtraConstraintPNDiffState(*ActionObj, ActionPath);
        auto [It, Inserted] = SeenTargets.insert({Target.Key, State});
        if (!Inserted && It->second != State) {
          failExtraConstraints(
              ActionPath,
              ("conflicting duplicate pndiff target '" + Target.Key + "'")
                  .c_str());
        }
        continue;
      }
      if (*Kind == "subtype" || *Kind == "equal") {
        auto *LHSValue = ActionObj->get("lhs");
        auto *RHSValue = ActionObj->get("rhs");
        if (LHSValue == nullptr || RHSValue == nullptr) {
          failExtraConstraints(ActionPath, "constraint action requires lhs and rhs");
        }
        validateExtraConstraintOperand(
            *LHSValue, *Func, Bindings, appendJSONPath(ActionPath, "lhs"));
        validateExtraConstraintOperand(
            *RHSValue, *Func, Bindings, appendJSONPath(ActionPath, "rhs"));
        continue;
      }
      failExtraConstraints(
          ActionPath,
          "only actions.kind = 'pndiff', 'subtype', or 'equal' is supported currently");
    }

    auto *DocRoot = Doc.getAsObject();
    assert(DocRoot != nullptr);
    auto *DocFunctions = DocRoot->getObject("functions");
    assert(DocFunctions != nullptr);
    (*DocFunctions)[Ent.first] = Ent.second;
    Funcs.insert(Func);
  }
}

void applyExtraConstraintSubtypeActions(notdec::mlsub::MLsubRecovery &Recovery,
                                        ConstraintsGenerator &G,
                                        llvm::Function &Func,
                                        const llvm::json::Object &Spec,
                                        llvm::StringRef FuncPath) {
  auto *Bindings = getExtraConstraintBindings(Spec, FuncPath);
  auto *ActionsValue = Spec.get("actions");
  if (ActionsValue == nullptr) {
    return;
  }
  const auto *Actions = ActionsValue->getAsArray();
  if (Actions == nullptr) {
    failExtraConstraints(appendJSONPath(FuncPath, "actions"), "expected array");
  }

  notdec::mlsub::MLsubRecovery::OverrideBuildContext Ctx{
      .Generator = G, .Func = Func};
  for (size_t Index = 0; Index < Actions->size(); ++Index) {
    auto ActionPath =
        appendJSONIndexPath(appendJSONPath(FuncPath, "actions"), Index);
    const auto *ActionObj = (*Actions)[Index].getAsObject();
    if (ActionObj == nullptr) {
      failExtraConstraints(ActionPath, "expected object");
    }
    auto Kind = ActionObj->getString("kind");
    if (!Kind) {
      failExtraConstraints(ActionPath, "missing or invalid string field 'kind'");
    }
    if (*Kind != "subtype" && *Kind != "equal") {
      continue;
    }

    auto *LHSValue = ActionObj->get("lhs");
    auto *RHSValue = ActionObj->get("rhs");
    if (LHSValue == nullptr || RHSValue == nullptr) {
      failExtraConstraints(ActionPath, "constraint action requires lhs and rhs");
    }
    auto LHSRecipe =
        buildExtraConstraintOperand(Recovery, G, Ctx, Func, Bindings, *LHSValue,
                                    appendJSONPath(ActionPath, "lhs"));
    auto RHSRecipe =
        buildExtraConstraintOperand(Recovery, G, Ctx, Func, Bindings, *RHSValue,
                                    appendJSONPath(ActionPath, "rhs"));
    Recovery.applyOverrideRecipe(G, LHSRecipe);
    Recovery.applyOverrideRecipe(G, RHSRecipe);
    G.addSubtype(LHSRecipe.Root, RHSRecipe.Root);
    if (*Kind == "equal") {
      G.addSubtype(RHSRecipe.Root, LHSRecipe.Root);
    }
  }
}

struct UTypeVariableDetail {
  std::string Name;
  std::uint32_t Id = 0;
  std::uint32_t Size = 0;
  std::set<std::uint32_t> OriginIds;
};

struct VarOriginEntry {
  UTypeVariableDetail Detail;
  std::set<std::string> RootLabels;
};

std::string formatExtValueMappingLabel(ExtValuePtr Value);
std::string formatExtValueList(const std::set<ExtValuePtr> &Values);
std::string formatOriginIdSummary(const std::set<std::uint32_t> &OriginIds);
void appendVarOriginEntries(std::map<std::uint32_t, VarOriginEntry> &Entries,
                            const binarysub::UTypePtr &Ty,
                            llvm::StringRef RootLabel);

void appendDebugValueTypes(
    llvm::StringRef DebugDir, llvm::StringRef SCCName,
    DSUMap<ExtValuePtr, SimpleType> &V2N,
    const std::set<ExtValuePtr> &ContraVariantValues,
    const std::map<binarysub::PolarVar, binarysub::UTypePtr> &Res,
    const std::map<std::uint32_t, std::set<ExtValuePtr>> &OriginalVariableSources,
    bool SolveMemory, const binarysub::PolarVar &PolMem) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(join(DebugDir.str(), kValueTypesFile.str()), EC,
                           llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Error printing to " << kValueTypesFile << ", "
                 << EC.message() << "\n";
    return;
  }

  std::vector<std::string> Lines;
  Lines.reserve(V2N.size() + (SolveMemory ? 1 : 0));

  for (const auto &Ent : V2N) {
    auto formatSolvedType = [&](bool Pos) {
      if (!Ent.second->isVariableState()) {
        return binarysub::debug_string(Ent.second);
      }
      auto It = Res.find(binarysub::PolarVar{.var = Ent.second, .pos = Pos});
      if (It == Res.end() || !It->second) {
        return std::string("<null>");
      }
      return binarysub::printType(It->second);
    };
    std::string Line = ContraVariantValues.count(Ent.first) == 0 ? "[+]" : "[-]";
    Line += " ";
    Line += formatExtValueMappingLabel(Ent.first);
    Line += " => lower=";
    Line += formatSolvedType(true);
    Line += " ; upper=";
    Line += formatSolvedType(false);
    Lines.push_back(std::move(Line));
  }

  if (SolveMemory) {
    auto It = Res.find(PolMem);
    std::string UTypeStr = "<null>";
    if (It != Res.end() && It->second) {
      UTypeStr = binarysub::printType(It->second);
    }
    Lines.push_back("[memory] <memory> => " + UTypeStr);
  }

  std::sort(Lines.begin(), Lines.end());

  Out << "## SCC: " << SCCName << "\n";
  for (const auto &Line : Lines) {
    Out << Line << "\n";
  }
  Out << "\n";
}

void appendDebugVarOrigins(
    llvm::StringRef DebugDir, llvm::StringRef SCCName,
    DSUMap<ExtValuePtr, SimpleType> &V2N,
    const std::set<ExtValuePtr> &ContraVariantValues,
    const std::map<binarysub::PolarVar, binarysub::UTypePtr> &Res,
    const std::map<std::uint32_t, std::set<ExtValuePtr>> &OriginalVariableSources,
    bool SolveMemory, const binarysub::PolarVar &PolMem) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(join(DebugDir.str(), kVarOriginsFile.str()), EC,
                           llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Error printing to " << kVarOriginsFile << ", "
                 << EC.message() << "\n";
    return;
  }

  auto getPol = [&](ExtValuePtr V) { return !ContraVariantValues.count(V); };
  std::map<std::uint32_t, VarOriginEntry> Entries;

  for (const auto &Ent : V2N) {
    if (!Ent.second->isVariableState()) {
      continue;
    }
    bool Pol = getPol(Ent.first);
    auto It = Res.find(binarysub::PolarVar{.var = Ent.second, .pos = Pol});
    if (It == Res.end() || !It->second) {
      continue;
    }
    appendVarOriginEntries(Entries, It->second, formatExtValueMappingLabel(Ent.first));
  }
  if (SolveMemory) {
    auto It = Res.find(PolMem);
    if (It != Res.end() && It->second) {
      appendVarOriginEntries(Entries, It->second, "<memory>");
    }
  }

  Out << "## SCC: " << SCCName << "\n";
  if (Entries.empty()) {
    Out << "No UType variables in this SCC.\n\n";
    return;
  }

  for (const auto &[Id, Entry] : Entries) {
    const auto &Detail = Entry.Detail;
    Out << "ut#" << Id << " " << Detail.Name << " ; size=" << Detail.Size
        << " ; origins=" << formatOriginIdSummary(Detail.OriginIds) << "\n";
    Out << "  appears-in: " << llvm::join(Entry.RootLabels, " | ") << "\n";
    for (auto OriginId : Detail.OriginIds) {
      Out << "  vs#" << OriginId << " => ";
      if (auto It = OriginalVariableSources.find(OriginId);
          It != OriginalVariableSources.end()) {
        Out << formatExtValueList(It->second);
      } else {
        Out << "<unknown>";
      }
      Out << "\n";
    }
    Out << "\n";
  }
}

std::string formatTypeBuilderRootLabel(ExtValuePtr Value) {
  std::string Label = toString(Value, true);
  std::string Stable = toStableString(Value);
  if (!Stable.empty()) {
    Label += " [stable=" + Stable + "]";
  }
  return Label;
}

std::string sanitizeTraceText(llvm::StringRef Text) {
  std::string Result = Text.str();
  for (char &Ch : Result) {
    if (Ch == '\n' || Ch == '\r' || Ch == '\t') {
      Ch = ' ';
    }
  }
  return Result;
}

std::string formatExtValueMappingLabel(ExtValuePtr Value) {
  std::string Stable = toStableString(Value);
  std::string Verbose = sanitizeTraceText(toString(Value, true));
  if (Stable.empty()) {
    return Verbose;
  }
  if (Verbose.empty() || Verbose == Stable) {
    return Stable;
  }
  return Stable + " (" + Verbose + ")";
}

void collectUTypeVariableDetailsImpl(
    const binarysub::UTypePtr &Ty, std::map<std::uint32_t, UTypeVariableDetail> &Out,
    std::set<const binarysub::UType *> &Seen) {
  if (!Ty || !Seen.insert(Ty).second) {
    return;
  }

  std::visit(
      [&](auto const &Node) {
        using T = std::decay_t<decltype(Node)>;
        if constexpr (std::is_same_v<T, binarysub::UTypeVariable>) {
          auto &Entry = Out[Node.id];
          Entry.Name = Node.name;
          Entry.Id = Node.id;
          Entry.Size = Node.size;
          Entry.OriginIds.insert(Node.originIds.begin(), Node.originIds.end());
        } else if constexpr (std::is_same_v<T, binarysub::UFunctionType>) {
          for (const auto &Arg : Node.args) {
            collectUTypeVariableDetailsImpl(Arg, Out, Seen);
          }
          collectUTypeVariableDetailsImpl(Node.result, Out, Seen);
        } else if constexpr (std::is_same_v<T, binarysub::UUnion> ||
                             std::is_same_v<T, binarysub::UInter>) {
          for (const auto &Term : Node.types) {
            collectUTypeVariableDetailsImpl(Term, Out, Seen);
          }
        } else if constexpr (std::is_same_v<T, binarysub::URecordType>) {
          for (const auto &[_, FieldTy] : Node.fields) {
            collectUTypeVariableDetailsImpl(FieldTy, Out, Seen);
          }
        } else if constexpr (std::is_same_v<T, binarysub::URecursiveType>) {
          collectUTypeVariableDetailsImpl(Node.body, Out, Seen);
        } else if constexpr (std::is_same_v<T, binarysub::UPointerType>) {
          collectUTypeVariableDetailsImpl(Node.load, Out, Seen);
          collectUTypeVariableDetailsImpl(Node.store, Out, Seen);
        }
      },
      Ty->v);
}

std::map<std::uint32_t, UTypeVariableDetail>
collectUTypeVariableDetails(const binarysub::UTypePtr &Ty) {
  std::map<std::uint32_t, UTypeVariableDetail> Details;
  std::set<const binarysub::UType *> Seen;
  collectUTypeVariableDetailsImpl(Ty, Details, Seen);
  return Details;
}

std::string formatExtValueList(const std::set<ExtValuePtr> &Values) {
  if (Values.empty()) {
    return "<none>";
  }
  std::vector<std::string> Labels;
  Labels.reserve(Values.size());
  for (const auto &Value : Values) {
    Labels.push_back(formatExtValueMappingLabel(Value));
  }
  std::sort(Labels.begin(), Labels.end());
  return llvm::join(Labels, " | ");
}

std::string formatOriginIdSummary(const std::set<std::uint32_t> &OriginIds) {
  std::string Summary;
  bool First = true;
  for (auto OriginId : OriginIds) {
    if (!First) {
      Summary += ",";
    }
    First = false;
    Summary += "vs#" + std::to_string(OriginId);
  }
  if (Summary.empty()) {
    return "<none>";
  }
  return Summary;
}

void appendVarOriginEntries(
    std::map<std::uint32_t, VarOriginEntry> &Entries, const binarysub::UTypePtr &Ty,
    llvm::StringRef RootLabel) {
  auto Details = collectUTypeVariableDetails(Ty);
  for (const auto &[Id, Detail] : Details) {
    auto &Entry = Entries[Id];
    Entry.Detail = Detail;
    Entry.RootLabels.insert(RootLabel.str());
  }
}

std::shared_ptr<ConstraintsGenerator> getFuncCG(AllGraphs &AG,
                                                const llvm::Function *F) {
  auto *CGN = AG.CG->getOrInsertFunction(const_cast<llvm::Function *>(F));
  if (!AG.Func2SCCIndex.count(CGN)) {
    return nullptr;
  }
  return AG.AllSCCs.at(AG.Func2SCCIndex.at(CGN)).Generator;
}

struct PNDiffStateInfo {
  std::string State;
  unsigned long NodeId = 0;
};

std::string formatPNDiffStateInfo(const PNDiffStateInfo &Info) {
  return Info.State + "#" + std::to_string(Info.NodeId);
}

std::optional<PNDiffStateInfo> getPNDiffStateInfo(ConstraintsGenerator &CG,
                                                  ExtValuePtr Val) {
  auto *Node = CG.getPNINodeOrNull(Val);
  if (Node == nullptr || !Node->isPNRelated()) {
    return std::nullopt;
  }
  if (Node->isPointer() || Node->isNull()) {
    return PNDiffStateInfo{
        .State = "ptr",
        .NodeId = Node->getId(),
    };
  }
  if (Node->isNumber()) {
    return PNDiffStateInfo{
        .State = "num",
        .NodeId = Node->getId(),
    };
  }
  return PNDiffStateInfo{
      .State = "unknown",
      .NodeId = Node->getId(),
  };
}

std::string joinParts(const std::vector<std::string> &Parts) {
  std::string Result;
  for (size_t I = 0; I < Parts.size(); ++I) {
    if (I != 0) {
      Result += ", ";
    }
    Result += Parts[I];
  }
  return Result;
}

std::optional<std::string>
formatInstructionPNDiffComment(ConstraintsGenerator &CG,
                               const llvm::Instruction &Inst) {
  std::vector<std::string> Parts;
  if (!Inst.getType()->isVoidTy()) {
    if (auto State = getPNDiffStateInfo(
            CG, const_cast<llvm::Instruction *>(&Inst))) {
      Parts.push_back("result=" + formatPNDiffStateInfo(*State));
    }
  }
  for (unsigned I = 0; I < Inst.getNumOperands(); ++I) {
    ExtValuePtr Op = Inst.getOperand(I);
    llvmValue2ExtVal(Op, const_cast<llvm::Instruction *>(&Inst), I);
    if (auto State = getPNDiffStateInfo(CG, Op)) {
      Parts.push_back("op" + std::to_string(I) + "=" +
                      formatPNDiffStateInfo(*State));
    }
  }
  if (Parts.empty()) {
    return std::nullopt;
  }
  return "pndiff: " + joinParts(Parts);
}

std::optional<std::string>
formatFunctionPNDiffComment(ConstraintsGenerator &CG, const llvm::Function &F) {
  std::vector<std::string> Parts;
  if (!F.getReturnType()->isVoidTy()) {
    if (auto State = getPNDiffStateInfo(
            CG, ReturnValue{.Func = const_cast<llvm::Function *>(&F)})) {
      Parts.push_back("ret=" + formatPNDiffStateInfo(*State));
    }
  }
  unsigned ArgIndex = 0;
  for (auto &Arg : F.args()) {
    if (auto State =
            getPNDiffStateInfo(CG, const_cast<llvm::Argument *>(&Arg))) {
      Parts.push_back("arg" + std::to_string(ArgIndex) + "=" +
                      formatPNDiffStateInfo(*State));
    }
    ++ArgIndex;
  }
  if (Parts.empty()) {
    return std::nullopt;
  }
  return "pndiff: " + joinParts(Parts);
}

std::string formatConstraintStateSummary(ConstraintsGenerator &CG,
                                         const ConsNode &Cons) {
  auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
  auto formatStateOrUnknown = [&](PNIValue Val) {
    auto State = getPNDiffStateInfo(CG, CG.getPNIExtValue(Val));
    if (!State) {
      return std::string("unknown");
    }
    return formatPNDiffStateInfo(*State);
  };
  auto ResultState = formatStateOrUnknown(Nodes[2]);
  auto LeftState = formatStateOrUnknown(Nodes[0]);
  auto RightState = formatStateOrUnknown(Nodes[1]);
  return "result=" + ResultState + ", op0=" + LeftState +
         ", op1=" + RightState;
}

bool constraintHasUnknownState(ConstraintsGenerator &CG, const ConsNode &Cons) {
  auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
  for (PNIValue Val : Nodes) {
    auto State = getPNDiffStateInfo(CG, CG.getPNIExtValue(Val));
    if (!State || State->State == "unknown") {
      return true;
    }
  }
  return false;
}

void writePNDiffWarnings(const std::string &Path, AllGraphs &AG,
                         bool PrependBlankLine = false) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC,
                           llvm::sys::fs::OF_Text | llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Cannot open PNDiff warning output file " << Path << ": "
                 << EC.message() << "\n";
    std::abort();
  }

  if (PrependBlankLine) {
    Out << "\n";
  }
  Out << "# Residual PNDiff constraints after solve\n\n";
  bool AnyResidual = false;

  for (auto &Data : AG.AllSCCs) {
    if (!Data.Generator || Data.Generator->PG.Constraints.empty()) {
      continue;
    }

    auto &CG = *Data.Generator;
    bool WroteSCCHeader = false;
    for (const auto &Cons : CG.PG.Constraints) {
      if (!constraintHasUnknownState(CG, Cons)) {
        continue;
      }
      if (!WroteSCCHeader) {
        AnyResidual = true;
        WroteSCCHeader = true;
        Out << "## SCC: " << Data.SCCName << "\n";
      }
      auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
      Out << "kind: " << (Cons.isAdd() ? "Add" : "Sub") << "\n";
      Out << "inst: " << formatPNDiffInstruction(Cons.getInst()) << "\n";
      Out << "result: " << toStableString(CG.getPNIExtValue(Nodes[2])) << "\n";
      Out << "op0: " << toStableString(CG.getPNIExtValue(Nodes[0])) << "\n";
      Out << "op1: " << toStableString(CG.getPNIExtValue(Nodes[1])) << "\n";
      Out << "state: " << formatConstraintStateSummary(CG, Cons) << "\n\n";
    }
  }

  if (!AnyResidual) {
    Out << "No residual Add/Sub constraints with unknown state after solve.\n";
  }
}

class PNDiffAnnotationWriter : public llvm::AssemblyAnnotationWriter {
  AllGraphs &AG;

public:
  explicit PNDiffAnnotationWriter(AllGraphs &AG) : AG(AG) {}

  void emitFunctionAnnot(const llvm::Function *F,
                         llvm::formatted_raw_ostream &OS) override {
    auto CG = getFuncCG(AG, F);
    if (!CG) {
      return;
    }
    auto Comment = formatFunctionPNDiffComment(*CG, *F);
    if (!Comment) {
      return;
    }
    OS << "; " << *Comment << "\n";
  }

  void printInfoComment(const llvm::Value &V,
                        llvm::formatted_raw_ostream &OS) override {
    auto *Inst = llvm::dyn_cast<llvm::Instruction>(&V);
    if (Inst == nullptr) {
      return;
    }
    auto CG = getFuncCG(AG, Inst->getFunction());
    if (!CG) {
      return;
    }
    auto Comment = formatInstructionPNDiffComment(*CG, *Inst);
    if (!Comment) {
      return;
    }
    OS << "; " << *Comment;
  }
};

void writePNDiffAnnotatedModule(const llvm::Module &M, const std::string &Path,
                                AllGraphs &AG) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(Path, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Cannot open PNDiff annotated IR output file " << Path
                 << ": " << EC.message() << "\n";
    std::abort();
  }
  PNDiffAnnotationWriter Writer(AG);
  M.print(Out, &Writer);
}

} // namespace

void ConstraintsGenerator::emitMappingTrace(llvm::StringRef Event,
                                            ExtValuePtr Val,
                                            const SimpleType &Ty) {
  if (TraceStream == nullptr) {
    return;
  }
  std::ostringstream OS;
  OS << "[mapping:" << Event.str() << "] value="
     << formatExtValueMappingLabel(Val) << " simple="
     << binarysub::debug_string(Ty);
  binarysub::binarysub_trace(OS.str());
}

void ConstraintsGenerator::emitRemapTrace(llvm::StringRef Event, ExtValuePtr Val,
                                          ExtValuePtr Target,
                                          const SimpleType &Ty) {
  if (TraceStream == nullptr) {
    return;
  }
  std::ostringstream OS;
  OS << "[mapping:" << Event.str() << "] value="
     << formatExtValueMappingLabel(Val) << " target="
     << formatExtValueMappingLabel(Target) << " simple="
     << binarysub::debug_string(Ty);
  binarysub::binarysub_trace(OS.str());
}

void ConstraintsGenerator::emitMergeTrace(llvm::StringRef Event, SimpleType From,
                                          SimpleType To,
                                          llvm::ArrayRef<ExtValuePtr> MovedValues) {
  if (TraceStream == nullptr) {
    return;
  }
  std::ostringstream OS;
  OS << "[mapping:" << Event.str() << "] from="
     << binarysub::debug_string(From) << " to=" << binarysub::debug_string(To);
  if (MovedValues.empty()) {
    OS << " values=<none>";
  } else {
    OS << " values=";
    for (size_t I = 0; I < MovedValues.size(); ++I) {
      if (I != 0) {
        OS << ", ";
      }
      OS << formatExtValueMappingLabel(MovedValues[I]);
    }
  }
  binarysub::binarysub_trace(OS.str());
}

SimpleType
MemoryAccessRecords::getOrCreateLocType(ConstraintsGenerator &CG,
                                        const MemoryLocKey &Loc) {
  auto It = LocTypes.find(Loc);
  if (It != LocTypes.end()) {
    return It->second;
  }
  auto Ty = binarysub::fresh_variable(CG.lvl, Loc.BitSize);
  LocTypes.insert({Loc, Ty});
  CG.emitPointerAnalysisTrace("[pa:obj-type] loc=" + formatMemoryLocKey(Loc) +
                              " ty=" + binarysub::debug_string(Ty));
  return Ty;
}

void ConstraintsGenerator::emitPointerAnalysisTrace(
    const std::string &Message) {
  if (!shouldTracePointerAnalysis()) {
    return;
  }
  *TraceStream << Message << "\n";
  TraceStream->flush();
}

void ConstraintsGenerator::observeOldMemoryTypeEdge(const SimpleType &Lhs,
                                                    const SimpleType &Rhs) {
  if (PAMode != PointerAnalysisMode::Shadow || TraceStream == nullptr) {
    return;
  }
  auto *Store = Lhs->getAsPtrStore();
  auto *Load = Rhs->getAsPtrLoad();
  if (Store == nullptr || Load == nullptr || Store->Size != Load->Size) {
    return;
  }
  auto Key = std::make_tuple(Store->to, Load->to, Store->Size);
  if (!MemoryAccesses.ObservedOldRelations.insert(Key).second) {
    return;
  }
  emitPointerAnalysisTrace(
      "[pa:old-store-load] size=" + std::to_string(Store->Size) +
      " store=" + binarysub::debug_string(Store->to) +
      " load=" + binarysub::debug_string(Load->to));
}

bool ConstraintsGenerator::configureConstraintContext(
    binarysub::ConstraintContext &Context) {
  bool HasHook = false;
  if (MergeEval) {
    Context.onVariableMerged =
        [Eval = MergeEval](const binarysub::MergeEvent &Event) {
          Eval->observeVariableMerged(Event);
        };
    HasHook = true;
  }
  return HasHook;
}

bool ConstraintsGenerator::hasStructPointerEvidence(SimpleType Ty) const {
  Ty = binarysub::resolve_variable(Ty);
  auto *Var = Ty ? Ty->getAsVariableState() : nullptr;
  if (Var == nullptr || Var->size != PointerSize) {
    return false;
  }

  auto HasStructField = [](const std::vector<SimpleType> &Bounds) {
    for (const auto &Bound : Bounds) {
      auto ResolvedBound = binarysub::resolve_variable(Bound);
      auto *Mem = ResolvedBound ? ResolvedBound->getAsTMemObject() : nullptr;
      if (Mem == nullptr) {
        continue;
      }
      for (const auto &[FieldName, _] : Mem->fields) {
        if (isStructFieldEvidence(FieldName)) {
          return true;
        }
      }
    }
    return false;
  };

  return HasStructField(Var->lowerBounds) || HasStructField(Var->upperBounds);
}

std::vector<ConstraintsGenerator::LoadStoreStructPtrMergeCandidate>
ConstraintsGenerator::collectStructPtrLoadStoreMergeCandidates() const {
  std::vector<LoadStoreStructPtrMergeCandidate> Candidates;
  std::set<std::tuple<const binarysub::TypeNode *, const binarysub::TypeNode *,
                      const binarysub::TypeNode *, unsigned>>
      Seen;

  for (const auto &[_, Node] : V2N) {
    auto Ptr = binarysub::resolve_variable(Node);
    auto *PtrVar = Ptr ? Ptr->getAsVariableState() : nullptr;
    if (PtrVar == nullptr) {
      continue;
    }

    for (const auto &LoadBound : PtrVar->upperBounds) {
      auto *Load = LoadBound ? LoadBound->getAsPtrLoad() : nullptr;
      if (Load == nullptr) {
        continue;
      }
      for (const auto &StoreBound : PtrVar->upperBounds) {
        auto *Store = StoreBound ? StoreBound->getAsPtrStore(Load->Size)
                                 : nullptr;
        if (Store == nullptr) {
          continue;
        }

        auto LoadTarget = binarysub::resolve_variable(Load->to);
        auto StoreTarget = binarysub::resolve_variable(Store->to);
        auto *LoadVar = LoadTarget ? LoadTarget->getAsVariableState() : nullptr;
        auto *StoreVar =
            StoreTarget ? StoreTarget->getAsVariableState() : nullptr;
        if (LoadVar == nullptr || StoreVar == nullptr ||
            LoadTarget.get() == StoreTarget.get()) {
          continue;
        }
        if (LoadVar->level != StoreVar->level ||
            LoadVar->size != StoreVar->size || LoadVar->size != PointerSize) {
          continue;
        }
        if (!hasStructPointerEvidence(LoadTarget) ||
            !hasStructPointerEvidence(StoreTarget)) {
          continue;
        }

        const auto *First = LoadTarget.get();
        const auto *Second = StoreTarget.get();
        if (Second < First) {
          std::swap(First, Second);
        }
        auto Key = std::make_tuple(Ptr.get(), First, Second, Load->Size);
        if (!Seen.insert(Key).second) {
          continue;
        }
        Candidates.push_back(LoadStoreStructPtrMergeCandidate{
            .Pointer = Ptr,
            .LoadTarget = LoadTarget,
            .StoreTarget = StoreTarget,
            .AccessSize = Load->Size});
      }
    }
  }

  return Candidates;
}

std::size_t ConstraintsGenerator::applyStructPtrLoadStoreMergePolicy() {
  std::size_t Merged = 0;
  while (true) {
    bool Changed = false;
    auto Candidates = collectStructPtrLoadStoreMergeCandidates();
    for (const auto &Candidate : Candidates) {
      auto LoadTarget = binarysub::resolve_variable(Candidate.LoadTarget);
      auto StoreTarget = binarysub::resolve_variable(Candidate.StoreTarget);
      auto *LoadVar = LoadTarget ? LoadTarget->getAsVariableState() : nullptr;
      auto *StoreVar =
          StoreTarget ? StoreTarget->getAsVariableState() : nullptr;
      if (LoadVar == nullptr || StoreVar == nullptr ||
          LoadTarget.get() == StoreTarget.get()) {
        continue;
      }
      if (LoadVar->level != StoreVar->level ||
          LoadVar->size != StoreVar->size || LoadVar->size != PointerSize) {
        continue;
      }
      if (!hasStructPointerEvidence(LoadTarget) ||
          !hasStructPointerEvidence(StoreTarget)) {
        continue;
      }

      auto From = LoadVar->id <= StoreVar->id ? StoreTarget : LoadTarget;
      auto Into = LoadVar->id <= StoreVar->id ? LoadTarget : StoreTarget;
      emitMergeTrace("load-store-struct-ptr", From, Into, {});
      maybeUnifyPNDiffTypeVariablePair(From, Into);

      binarysub::ConstraintContext Context;
      binarysub::ConstraintContext *ContextPtr =
          configureConstraintContext(Context) ? &Context : nullptr;
      auto Result = binarysub::merge_variable_into(
          From, Into,
          [this](const SimpleType &Lhs, const SimpleType &Rhs) {
            maybeUnifyPNDiffTypeVariablePair(Lhs, Rhs);
            observeOldMemoryTypeEdge(Lhs, Rhs);
          },
          ContextPtr);
      if (!Result) {
        emitPointerAnalysisTrace(
            "[merge-policy:load-store-struct-ptr:skip] ptr=" +
            binarysub::debug_string(Candidate.Pointer) +
            " load=" + binarysub::debug_string(LoadTarget) +
            " store=" + binarysub::debug_string(StoreTarget) +
            " reason=" + Result.error().msg);
        continue;
      }

      ++Merged;
      Changed = true;
      emitPointerAnalysisTrace(
          "[merge-policy:load-store-struct-ptr] ptr=" +
          binarysub::debug_string(Candidate.Pointer) +
          " from=" + binarysub::debug_string(From) +
          " into=" + binarysub::debug_string(Into) +
          " size=" + std::to_string(Candidate.AccessSize));
      break;
    }
    if (!Changed) {
      return Merged;
    }
  }
}

void ConstraintsGenerator::recordLoad(ExtValuePtr Addr, SimpleType ResultTy,
                                      unsigned BitSize,
                                      llvm::Instruction *Source) {
  if (!isPointerAnalysisEnabled()) {
    return;
  }
  MemoryAccesses.LoadsByAddr[Addr].push_back(
      RecordedLoad{.Addr = Addr, .ResultTy = ResultTy, .BitSize = BitSize,
                   .Source = Source});
  emitPointerAnalysisTrace("[pa:record-load] addr=" + toStableString(Addr) +
                           " size=" + std::to_string(BitSize));
}

void ConstraintsGenerator::recordStore(ExtValuePtr Addr, SimpleType ValueTy,
                                       unsigned BitSize,
                                       llvm::Instruction *Source) {
  bool KeepForEVMEvidence =
      Source != nullptr && Source->getModule() != nullptr &&
      isEVMModule(*Source->getModule());
  if (!isPointerAnalysisEnabled() && !KeepForEVMEvidence) {
    return;
  }
  MemoryAccesses.StoresByAddr[Addr].push_back(
      RecordedStore{.Addr = Addr, .ValueTy = ValueTy, .BitSize = BitSize,
                    .Source = Source});
  emitPointerAnalysisTrace("[pa:record-store] addr=" + toStableString(Addr) +
                           " size=" + std::to_string(BitSize));
}

void ConstraintsGenerator::addEVMConstantMemoryField(ExtValuePtr Addr,
                                                     SimpleType ValueTy) {
  auto *ConstAddr = std::get_if<ConstantAddr>(&Addr);
  if (ConstAddr == nullptr) {
    return;
  }

  // EVM native memory uses inttoptr constants as byte offsets into one memory
  // object. Keep only constant offsets here; dynamic offsets need separate
  // array/unknown modeling.
  auto Offset = getSigned64ConstantAddress(*ConstAddr->Val);
  if (!Offset.has_value()) {
    return;
  }
  std::vector<std::pair<std::string, SimpleType>> Fields;
  Fields.emplace_back(OffsetRange{.offset = *Offset}.str(), ValueTy);
  addSubtype(MemoryType, binarysub::make_record(std::move(Fields)));
}

SimpleType ConstraintsGenerator::getOrCreateStorageField(
    llvm::StringRef FieldName, unsigned BitSize) {
  assert(StorageType != nullptr && "StorageType must be initialized");
  assert(StorageFields != nullptr && "StorageFields must be initialized");

  auto It = StorageFields->find(FieldName.str());
  if (It != StorageFields->end()) {
    return It->second;
  }

  auto FieldTy =
      binarysub::make_variable(0, BitSize == 0 ? PointerSize : BitSize);
  std::string StableName = FieldName.str();
  StorageFields->insert({StableName, FieldTy});

  std::vector<std::pair<std::string, SimpleType>> Fields;
  Fields.emplace_back(StableName, FieldTy);
  addSubtype(StorageType, binarysub::make_record(std::move(Fields)));

  if (TraceStream != nullptr) {
    *TraceStream << "[storage:field] name=" << StableName
                 << " ty=" << binarysub::debug_string(FieldTy) << "\n";
    TraceStream->flush();
  }
  return FieldTy;
}

void ConstraintsGenerator::onPointsToDelta(ExtValuePtr Addr,
                                           MemoryLocKey Loc) {
  if (auto LoadsIt = MemoryAccesses.LoadsByAddr.find(Addr);
      LoadsIt != MemoryAccesses.LoadsByAddr.end()) {
    for (const auto &Load : LoadsIt->second) {
      auto AccessLoc = withAccessSize(Loc, Load.BitSize);
      auto ObjTy = MemoryAccesses.getOrCreateLocType(*this, AccessLoc);
      if (MemoryAccesses.EmittedLoadConstraints.insert({AccessLoc, Load.ResultTy})
              .second) {
        emitPointerAnalysisTrace(
            "[pa:new-load-edge] loc=" + formatMemoryLocKey(AccessLoc) +
            " load=" + binarysub::debug_string(Load.ResultTy));
        if (isPointerAnalysisReplacingBinarysubMemory()) {
          addSubtype(ObjTy, Load.ResultTy);
        }
      }
    }
  }

  if (auto StoresIt = MemoryAccesses.StoresByAddr.find(Addr);
      StoresIt != MemoryAccesses.StoresByAddr.end()) {
    for (const auto &Store : StoresIt->second) {
      auto AccessLoc = withAccessSize(Loc, Store.BitSize);
      auto ObjTy = MemoryAccesses.getOrCreateLocType(*this, AccessLoc);
      if (MemoryAccesses.EmittedStoreConstraints.insert({Store.ValueTy, AccessLoc})
              .second) {
        emitPointerAnalysisTrace(
            "[pa:new-store-edge] store=" +
            binarysub::debug_string(Store.ValueTy) +
            " loc=" + formatMemoryLocKey(AccessLoc));
        if (isPointerAnalysisReplacingBinarysubMemory()) {
          addSubtype(Store.ValueTy, ObjTy);
        }
      }
    }
  }
}

void ConstraintsGenerator::addPointerAccessViews() {
  if (!isPointerAnalysisReplacingBinarysubMemory()) {
    return;
  }
  for (const auto &[Addr, Loads] : MemoryAccesses.LoadsByAddr) {
    auto PtrTy = getNodeOrNull(Addr);
    if (PtrTy == nullptr) {
      continue;
    }
    for (const auto &Load : Loads) {
      auto Key = std::make_tuple(Addr, Load.ResultTy, Load.BitSize);
      if (!MemoryAccesses.EmittedLoadViews.insert(Key).second) {
        continue;
      }
      emitPointerAnalysisTrace(
          "[pa:view-load] addr=" + toStableString(Addr) +
          " ty=" + binarysub::debug_string(Load.ResultTy) +
          " size=" + std::to_string(Load.BitSize));
      addSubtype(PtrTy, binarysub::make_ptr_load(Load.ResultTy, Load.BitSize));
    }
  }
  for (const auto &[Addr, Stores] : MemoryAccesses.StoresByAddr) {
    auto PtrTy = getNodeOrNull(Addr);
    if (PtrTy == nullptr) {
      continue;
    }
    for (const auto &Store : Stores) {
      auto Key = std::make_tuple(Addr, Store.ValueTy, Store.BitSize);
      if (!MemoryAccesses.EmittedStoreViews.insert(Key).second) {
        continue;
      }
      emitPointerAnalysisTrace(
          "[pa:view-store] addr=" + toStableString(Addr) +
          " ty=" + binarysub::debug_string(Store.ValueTy) +
          " size=" + std::to_string(Store.BitSize));
      addSubtype(PtrTy,
                 binarysub::make_ptr_store(Store.ValueTy, Store.BitSize));
    }
  }
}

void ConstraintsGenerator::flushPointerDerivedTypeConstraints() {
  std::map<MemoryLocKey, std::vector<const RecordedLoad *>> LoadsByLoc;
  std::map<MemoryLocKey, std::vector<const RecordedStore *>> StoresByLoc;

  for (const auto &[Slot, Locs] : PA.pointsTo()) {
    for (const auto &Loc : Locs) {
      emitPointerAnalysisTrace("[pa:pts] slot=" + formatPointerSlotKey(Slot) +
                               " loc=" + formatMemoryLocKey(Loc));
    }

    const auto *Addr = std::get_if<ExtValuePtr>(&Slot);
    if (Addr == nullptr) {
      continue;
    }
    for (const auto &Loc : Locs) {
      onPointsToDelta(*Addr, Loc);

      if (auto LoadsIt = MemoryAccesses.LoadsByAddr.find(*Addr);
          LoadsIt != MemoryAccesses.LoadsByAddr.end()) {
        for (const auto &Load : LoadsIt->second) {
          LoadsByLoc[withAccessSize(Loc, Load.BitSize)].push_back(&Load);
        }
      }
      if (auto StoresIt = MemoryAccesses.StoresByAddr.find(*Addr);
          StoresIt != MemoryAccesses.StoresByAddr.end()) {
        for (const auto &Store : StoresIt->second) {
          StoresByLoc[withAccessSize(Loc, Store.BitSize)].push_back(&Store);
        }
      }
    }
  }

  for (const auto &[Loc, Stores] : StoresByLoc) {
    auto LoadsIt = LoadsByLoc.find(Loc);
    if (LoadsIt == LoadsByLoc.end()) {
      continue;
    }
    for (const auto *Store : Stores) {
      for (const auto *Load : LoadsIt->second) {
        auto Key = std::make_tuple(Loc, Store->Source, Load->Source);
        if (!MemoryAccesses.EmittedStoreLoadRelations.insert(Key).second) {
          continue;
        }
        emitPointerAnalysisTrace(
            "[pa:new-store-load] loc=" + formatMemoryLocKey(Loc) +
            " store=" + binarysub::debug_string(Store->ValueTy) +
            " load=" + binarysub::debug_string(Load->ResultTy));
      }
    }
  }
  addPointerAccessViews();
}

void ConstraintsGenerator::addMergeNode(SimpleType From, SimpleType To) {
  if (From == To) {
    return;
  }
  std::vector<ExtValuePtr> MovedValues;
  if (auto It = V2N.rev().find(From); It != V2N.rev().end()) {
    MovedValues = It->second;
  }
  emitMergeTrace("merge", From, To, MovedValues);
  if (MergeEval) {
    MergeEval->observeValueMapMerge(From, To, MovedValues);
  }
  V2N.merge(From, To);
}

void ConstraintsGenerator::configurePNDiffCallbacks() {
  PG.Policy = loadPNDiffPolicyConfig();
  PG.AllocateNodeId = []() { return ValueNamer::getId(); };
  PG.GetIntConstant = [this](PNIValue Val) {
    return getPNDiffIntConstantValue(getPNIExtValue(Val));
  };
  PG.MatchOffsetRange = [this](PNIValue Val) {
    return matchPNDiffOffsetRangeNoNegativeAccess(getPNIExtValue(Val));
  };
  PG.FormatValue = [this](PNIValue Val) {
    return toStableString(getPNIExtValue(Val));
  };
  PG.FormatInstruction = [](PNIInstruction Inst) {
    return formatPNDiffInstruction(Inst);
  };
  PG.OnUpdatePNType = [this](PNIValue Val) {
    onUpdatePNType(getPNIExtValue(Val));
  };
  PG.OnPtrAdd = [this](PNIValue Base, PNIValue Result, OffsetRange Off) {
    setAsPtrAdd(getPNIExtValue(Base), getPNIExtValue(Result), std::move(Off));
  };
}

PNIValue ConstraintsGenerator::getPNIValue(const ExtValuePtr &Val) {
  auto It = PNDiffValueHandles.find(Val);
  if (It == PNDiffValueHandles.end()) {
    auto Inserted = PNDiffValueHandles.emplace(Val, nullptr).first;
    Inserted->second = std::make_unique<ExtValuePtr>(Inserted->first);
    It = Inserted;
  }
  return static_cast<PNIValue>(It->second.get());
}

const ExtValuePtr &ConstraintsGenerator::getPNIExtValue(PNIValue Val) const {
  assert(Val != nullptr);
  return *static_cast<const ExtValuePtr *>(Val);
}

PNTy ConstraintsGenerator::getPNILatticeType(const ExtValuePtr &Val) const {
  return retypd::makePNTyFromLLVMType(getType(Val), PointerSize);
}

PNINode *ConstraintsGenerator::getPNINodeOrNull(ExtValuePtr Val) {
  return PG.getPNIVarOrNull(getPNIValue(Val));
}

PNINode &ConstraintsGenerator::getPNINode(ExtValuePtr Val) {
  return PG.getPNIVar(getPNIValue(Val));
}

PNINode &ConstraintsGenerator::getOrInsertPNINode(ExtValuePtr Val) {
  return PG.getOrInsertPNINode(getPNIValue(Val), getPNILatticeType(Val));
}

PNINode &ConstraintsGenerator::remapPNINode(ExtValuePtr Val,
                                            ExtValuePtr Target) {
  return PG.remapPNIVar(getPNIValue(Val), getPNIValue(Target));
}

void ConstraintsGenerator::unifyPNIValues(ExtValuePtr V1, ExtValuePtr V2) {
  PG.unifyVar(getPNIValue(V1), getPNIValue(V2));
}

SimpleType ConstraintsGenerator::createNode(ExtValuePtr Val) {
  auto N = convertSimpleType(Val);
  auto It = V2N.insert(Val, N);
  if (!It.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "createNode: Value already mapped to "
                 << It.first->second->str() << ", but now set to "
                 << toString(Val) << "\n";
    std::abort();
  }
  if (auto *VS = N->getAsVariableState()) {
    OriginalVariableSources[VS->id].insert(Val);
  }
  if (MergeEval) {
    MergeEval->observeValueNode(Val, N);
  }
  emitMappingTrace("create", Val, N);
  getOrInsertPNINode(Val);
  if (std::get_if<ConstantAddr>(&Val)) {
    setPointer(Val);
    addAddressOf(Val, getRootMemoryObject(Val));
  } else if (auto *V = std::get_if<llvm::Value *>(&Val)) {
    if (llvm::isa<llvm::GlobalVariable>(*V)) {
      addAddressOf(Val, getRootMemoryObject(Val));
    }
  }
  return N;
}

SimpleType ConstraintsGenerator::addRemapType(ExtValuePtr Val,
                                              ExtValuePtr Target) {
  auto Ty = getNodeOrNull(Target);
  assert(Ty != nullptr);
  auto N = getNodeOrNull(Val);
  if (N == Ty) {
    emitRemapTrace("remap-alias", Val, Target, Ty);
    remapPNINode(Val, Target);
    addPointerCopy(Val, Target);
    return N;
  }
  auto It = V2N.insert(Val, Ty);
  if (!It.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "setTypeVar: Value already mapped to "
                 << It.first->second->str() << ", but now set to "
                 << toString(Val) << "\n";
    std::abort();
  }
  emitRemapTrace("remap", Val, Target, Ty);
  if (MergeEval) {
    MergeEval->observeValueNode(Val, Ty);
  }
  remapPNINode(Val, Target);
  addPointerCopy(Val, Target);
  return It.first->second;
}

void MLsubRecovery::run() {
  auto &M = const_cast<llvm::Module &>(Mod);
  if (!MergeEvalDir.empty()) {
    MergeEval = std::make_shared<MergePolicyEval>(M, MergeEvalDir, PointerSize);
  } else {
    MergeEval.reset();
  }

  // 0.4 prepare debug dir and SCCsCatalog
  auto WorkDir = notdec::getWorkDirOpt();
  if (WorkDir) {
    std::error_code EC = llvm::sys::fs::create_directories(*WorkDir);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open create directory " << *WorkDir << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    SCCsCatalog.emplace(join(*WorkDir, "SCCs.txt"), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file SCCs.txt: ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }

    llvm::raw_fd_ostream ValueTypes(join(*WorkDir, kValueTypesFile.str()), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file " << kValueTypesFile.str() << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    ValueTypes << "# Final Value -> binarysub UType mapping\n\n";

    llvm::raw_fd_ostream VarOrigins(join(*WorkDir, kVarOriginsFile.str()), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file " << kVarOriginsFile.str() << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    VarOrigins << "# Final UType variable -> original ExtValuePtr mapping\n\n";
    auto PNDiffWarnPath = join(*WorkDir, kPNDiffWarnFile.str());
    if (std::error_code RemoveEC = llvm::sys::fs::remove(PNDiffWarnPath);
        RemoveEC && RemoveEC != std::errc::no_such_file_or_directory) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot remove output file " << kPNDiffWarnFile.str()
                << ": " << RemoveEC.message() << std::endl;
      std::abort();
    }
  }

  WrotePNDiffOverrideWarningHeader = false;

  if (envFlagEnabled(kBinarysubTraceEnv)) {
    if (!WorkDir) {
      llvm::errs() << "Warning: " << kBinarysubTraceEnv
                   << "=1 but no workdir is configured; skip "
                   << kBinarysubTraceFile << ".\n";
    } else {
      BinarysubTraceFile = std::make_unique<std::ofstream>(
          join(*WorkDir, kBinarysubTraceFile.str()), std::ios::trunc);
      if (!*BinarysubTraceFile) {
        llvm::errs() << "Cannot open " << kBinarysubTraceFile << " in "
                     << *WorkDir << ".\n";
        std::abort();
      }
      *BinarysubTraceFile << "# binarysub / pndiff trace\n";
      *BinarysubTraceFile << "# enabled by " << kBinarysubTraceEnv.str()
                          << "=1\n";
      *BinarysubTraceFile << "# value/simple-type bindings use [mapping:*]\n";
      BinarysubTraceFile->flush();
    }
  } else {
    BinarysubTraceFile.reset();
  }

  binarysub::binarysub_set_trace_stream(BinarysubTraceFile.get());

  if (!MemoryType) {
    MemoryType = binarysub::make_variable(0, PointerSize);
  }
  if (isEVMModule(M) && !StorageType) {
    StorageType = binarysub::make_variable(0, PointerSize);
  }

  SummaryOverrideFuncs.clear();
  SummaryOverrideDoc = makeEmptyOverrideDoc();
  SignatureOverrideFuncs.clear();
  SignatureOverrideDoc = makeEmptyOverrideDoc();
  if (auto RegistrySetup = registerBuiltinPrimitiveSemanticLatticesForModule(M);
      !RegistrySetup) {
    llvm::errs() << "Error: failed to register builtin primitive semantic "
                    "lattice: "
                 << RegistrySetup.error().msg << "\n";
    std::abort();
  }
  if (!kDefaultMLsubBuiltinSummaryOverridePath.empty() &&
      llvm::sys::fs::exists(kDefaultMLsubBuiltinSummaryOverridePath)) {
    loadSummaryFile(M, kDefaultMLsubBuiltinSummaryOverridePath.data(), false);
  }
  if (SummaryFile != nullptr) {
    loadSummaryFile(M, SummaryFile, true);
  }
  if (SignatureFile != nullptr) {
    loadSignatureFile(M, SignatureFile, true);
  }

  // set global pointer size variable for binarysub
  binarysub::pointer_size = PointerSize;

  auto MLsubInputText = renderModuleToString(M);
  auto ModuleSHA256Hex = computeSHA256Hex(MLsubInputText);

  // 0.5 print module for debugging
  emitTRInputArtifacts(M, "");

  if (ExtraConstraintsFile != nullptr) {
    validateExtraConstraintsFile(M, ExtraConstraintsFile, ModuleSHA256Hex);
  }

  CallGraphAnalysis Ana;
  CallG = std::make_unique<CallGraph>(Ana.run(M, MAM));

  if (WorkDir) {
    std::error_code EC;
    auto Path = join(*WorkDir, "CallGraph.txt");
    llvm::raw_fd_ostream CGTxt(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    CallG->print(CGTxt);
    CGTxt.close();
    // print dot
    Path = join(*WorkDir, "CallGraph.dot");
    llvm::raw_fd_ostream CGDot(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    notdec::utils::CallGraphDOTInfo CFGInfo(&M, &*CallG, nullptr, true);
    llvm::WriteGraph(CGDot, &CFGInfo, false);
    CGDot.close();

    Path = join(*WorkDir, "CallGraphFull.dot");
    llvm::raw_fd_ostream CGFullDot(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    notdec::utils::CallGraphDOTInfo FullCFGInfo(&M, &*CallG, nullptr, false);
    llvm::WriteGraph(CGFullDot, &FullCFGInfo, false);
    CGFullDot.close();
  }

  prepareSCC(*CallG);

  bottomUpPhase();

  topDownPhase();

  if (WorkDir && ResultVal == nullptr) {
    genASTTypes(M);
  }

  if (WorkDir) {
    writePNDiffWarnings(join(*WorkDir, kPNDiffWarnFile.str()), AG,
                        WrotePNDiffOverrideWarningHeader);
    writePNDiffAnnotatedModule(
        M, join(*WorkDir, kPNDiffAnnotatedFile.str()), AG);
  }

  std::cerr << "Constraint generation done! SCC count:" << AG.AllSCCs.size()
            << "\n";

  binarysub::binarysub_set_trace_stream(nullptr);
  if (BinarysubTraceFile) {
    BinarysubTraceFile->flush();
    BinarysubTraceFile.reset();
  }
}

void MLsubRecovery::loadSummaryFile(llvm::Module &M, const char *Path,
                                    bool StrictValidation) {
  loadOverrideFileImpl(M, Path, SummaryOverrideDoc, SummaryOverrideFuncs,
                       StrictValidation, false);
}

void MLsubRecovery::loadSignatureFile(llvm::Module &M, const char *Path,
                                      bool StrictValidation) {
  loadOverrideFileImpl(M, Path, SignatureOverrideDoc, SignatureOverrideFuncs,
                       StrictValidation, true);
}

void MLsubRecovery::validateExtraConstraintsFile(llvm::Module &M,
                                                 const char *Path,
                                                 llvm::StringRef ModuleSHA256Hex) {
  llvm::errs() << "Loading MLsub extra constraints from: " << Path << "\n";
  ExtraConstraintsDoc = makeEmptyOverrideDoc();
  ExtraConstraintsFuncs.clear();
  auto Parsed = llvm::json::parse(readFileToString(Path));
  if (!Parsed) {
    failExtraConstraints("",
                         ("JSON parse failed for " + std::string(Path)).c_str());
  }

  const auto *Root = Parsed->getAsObject();
  if (Root == nullptr) {
    failExtraConstraints("<root>", "expected object");
  }

  auto Version = Root->getInteger("version");
  if (!Version || *Version != 1) {
    failExtraConstraints("<root>", "expected version = 1");
  }

  validateExtraConstraintsAnchor(*Root, M, ModuleSHA256Hex);
  validateExtraConstraintFunctions(*Root, M, ExtraConstraintsDoc,
                                   ExtraConstraintsFuncs);
}

void MLsubRecovery::emitTRInputArtifacts(llvm::Module &M,
                                         llvm::StringRef OutputPath) {
  auto MLsubInputText = renderModuleToString(M);
  auto ModuleSHA256Hex = computeSHA256Hex(MLsubInputText);

  if (auto WorkDir = notdec::getWorkDirOpt()) {
    auto MLsubInputPath = join(*WorkDir, kMLsubInputIRFile.str());
    printModule(M, MLsubInputPath.c_str());
    writeMLsubInputAnchor(M, join(*WorkDir, kMLsubInputAnchorFile.str()),
                          ModuleSHA256Hex);
    writeSelectableValues(M, join(*WorkDir, kSelectableValuesFile.str()));
  }

  if (!OutputPath.empty()) {
    writeTRInputModule(M, OutputPath);
    llvm::errs() << "TR input IR emitted to " << OutputPath << "\n";
  }
}

const llvm::json::Value *
MLsubRecovery::getExtraConstraintsSpec(const llvm::Function &Func) const {
  return getOverrideSpecImpl(ExtraConstraintsDoc, ExtraConstraintsFuncs, Func);
}

const llvm::json::Value *
MLsubRecovery::getSummaryOverrideSpec(const llvm::Function &Func) const {
  return getOverrideSpecImpl(SummaryOverrideDoc, SummaryOverrideFuncs, Func);
}

bool MLsubRecovery::isSummaryOverridePolymorphic(
    const llvm::Function &Func) const {
  auto *Spec = getSummaryOverrideSpec(Func);
  if (Spec == nullptr) {
    return false;
  }
  const auto *Obj = Spec->getAsObject();
  if (Obj == nullptr) {
    return false;
  }
  if (auto Flag = Obj->getBoolean("is_polymorphic")) {
    return *Flag;
  }
  return false;
}

const llvm::json::Value *
MLsubRecovery::getSignatureOverrideSpec(const llvm::Function &Func) const {
  return getOverrideSpecImpl(SignatureOverrideDoc, SignatureOverrideFuncs,
                             Func);
}

MLsubRecovery::OverrideTypeRecipe
MLsubRecovery::buildOverrideType(const llvm::json::Value &Expr,
                                 OverrideBuildContext &Ctx,
                                 llvm::StringRef Path, bool AllowNull) {
  OverrideTypeRecipe Recipe;
  if (Expr.getAsNull()) {
    if (!AllowNull) {
      failSignatureOverride(Path, "null is only allowed for function return");
    }
    return Recipe;
  }

  const auto &Obj = requireObject(Expr, Path);
  auto Kind = Obj.getString("kind");
  if (!Kind) {
    failSignatureOverride(Path, "missing string field 'kind'");
  }

  if (*Kind == "primitive") {
    auto Name = requireString(Obj, "name", Path);
    auto Bits = requireInteger(Obj, "bits", Path);
    if (Bits <= 0) {
      failSignatureOverride(Path, "primitive bits must be positive");
    }

    auto &Registry = binarysub::globalPrimitiveSemanticRegistry();
    if (const auto *Family = Registry.findFamilyByCanonicalName(Name)) {
      if (Family->bits != static_cast<std::uint32_t>(Bits)) {
        failSignatureOverride(Path,
                              "semantic primitive bit width mismatch");
      }
    } else if (Name != "bool" && Name != "sint" && Name != "uint" &&
               Name != "float" && Name != "double" && Name != "char") {
      failSignatureOverride(Path, "unknown primitive name");
    }

    Recipe.Root =
        binarysub::make_primitive(Name.str(), static_cast<std::uint32_t>(Bits));
    return Recipe;
  }

  if (*Kind == "var") {
    auto ID = requireInteger(Obj, "id", Path);
    auto Bits = requireInteger(Obj, "bits", Path);
    if (ID < 0 || Bits <= 0) {
      failSignatureOverride(Path, "var id/bits must be positive");
    }
    auto Width = static_cast<std::uint32_t>(Bits);
    auto WidthIt = Ctx.VarBitWidths.find(ID);
    if (WidthIt != Ctx.VarBitWidths.end() && WidthIt->second != Width) {
      failSignatureOverride(Path, "var bits mismatch for reused id");
    }
    auto It = Ctx.Vars.find(ID);
    if (It == Ctx.Vars.end()) {
      auto Ty = binarysub::make_variable(Ctx.Generator.lvl, Width);
      Ctx.Vars.insert({ID, Ty});
      Ctx.VarBitWidths.insert({ID, Width});
      Recipe.Root = Ty;
    } else {
      Recipe.Root = It->second;
    }
    return Recipe;
  }

  if (*Kind == "ref") {
    auto ID = requireInteger(Obj, "id", Path);
    if (ID < 0) {
      failSignatureOverride(Path, "ref id must be non-negative");
    }
    auto It = Ctx.Vars.find(ID);
    if (It == Ctx.Vars.end()) {
      failSignatureOverride(Path, "ref points to an undefined var id");
    }
    Recipe.Root = It->second;
    return Recipe;
  }

  if (*Kind == "ptr") {
    Recipe.Root = binarysub::fresh_variable(Ctx.Generator.lvl,
                                            Ctx.Generator.PointerSize);
    if (auto *Load = Obj.get("load")) {
      auto LoadRecipe = buildOverrideType(
          *Load, Ctx, appendJSONPath(Path, "load"), false);
      appendOverrideConstraints(Recipe, std::move(LoadRecipe));
      auto AccessBits = binarysub::get_size(LoadRecipe.Root);
      Recipe.Constraints.push_back(
          {Recipe.Root, binarysub::make_ptr_load(LoadRecipe.Root, AccessBits)});
    }
    if (auto *Store = Obj.get("store")) {
      auto StoreRecipe = buildOverrideType(
          *Store, Ctx, appendJSONPath(Path, "store"), false);
      appendOverrideConstraints(Recipe, std::move(StoreRecipe));
      auto AccessBits = binarysub::get_size(StoreRecipe.Root);
      Recipe.Constraints.push_back({Recipe.Root,
                                    binarysub::make_ptr_store(StoreRecipe.Root,
                                                              AccessBits)});
    }
    if (Obj.get("load") == nullptr && Obj.get("store") == nullptr) {
      failSignatureOverride(Path, "ptr requires at least one of load/store");
    }
    return Recipe;
  }

  if (*Kind == "record") {
    const auto *FieldsValue = Obj.get("fields");
    if (FieldsValue == nullptr) {
      failSignatureOverride(Path, "missing array field 'fields'");
    }
    const auto &FieldsArray =
        requireArray(*FieldsValue, appendJSONPath(Path, "fields"));
    std::vector<std::pair<std::string, SimpleType>> Fields;
    Fields.reserve(FieldsArray.size());
    for (size_t Index = 0; Index < FieldsArray.size(); ++Index) {
      auto EntryPath = appendJSONIndexPath(appendJSONPath(Path, "fields"), Index);
      const auto &FieldObj = requireObject(FieldsArray[Index], EntryPath);
      auto Offset = requireString(FieldObj, "offset", EntryPath);
      auto *TypeValue = FieldObj.get("type");
      if (TypeValue == nullptr) {
        failSignatureOverride(EntryPath, "missing field 'type'");
      }
      auto FieldRecipe = buildOverrideType(
          *TypeValue, Ctx, appendJSONPath(EntryPath, "type"), false);
      appendOverrideConstraints(Recipe, std::move(FieldRecipe));
      Fields.push_back({Offset.str(), FieldRecipe.Root});
    }
    Recipe.Root = binarysub::make_record(std::move(Fields));
    return Recipe;
  }

  failSignatureOverride(Path, "unsupported override type kind");
}

void MLsubRecovery::applyOverrideRecipe(ConstraintsGenerator &G,
                                        const OverrideTypeRecipe &Recipe) {
  for (const auto &[LHS, RHS] : Recipe.Constraints) {
    G.addSubtype(LHS, RHS);
  }
}

notdec::mlsub::MLsubRecovery::OverrideTypeRecipe buildSummaryConstraintOperand(
    notdec::mlsub::MLsubRecovery &Recovery, ConstraintsGenerator &G,
    notdec::mlsub::MLsubRecovery::OverrideBuildContext &Ctx,
    llvm::Function &Func, const llvm::json::Value &Value,
    llvm::StringRef Path) {
  if (isSummaryConstraintTargetOperand(Value)) {
    auto Target =
        resolveSummaryConstraintTarget(*Value.getAsObject(), Func, Path);
    auto Node = G.getNodeOrNull(Target.Value);
    if (Node == nullptr) {
      failSignatureOverride(
          Path, ("target '" + Target.Key + "' has no corresponding node").c_str());
    }
    notdec::mlsub::MLsubRecovery::OverrideTypeRecipe Recipe;
    Recipe.Root = Node;
    return Recipe;
  }

  return Recovery.buildOverrideType(Value, Ctx, Path, false);
}

void MLsubRecovery::applySummaryOverride(ConstraintsGenerator &G,
                                         llvm::Function &Func,
                                         const llvm::json::Value &Spec) {
  const auto &Obj = requireObject(Spec, Func.getName());
  std::string FuncPath = Func.getName().str();
  OverrideBuildContext Ctx{.Generator = G, .Func = Func};
  OverrideTypeRecipe Aggregate;
  std::vector<SimpleType> Args;
  OverrideTypeRecipe RetRecipe;
  bool HasFunctionType = hasFunctionTypeOverride(Obj, FuncPath, false);
  if (HasFunctionType) {
    const auto &ArgsValue =
        requireArray(*Obj.get("args"), appendJSONPath(FuncPath, "args"));
    Args.reserve(ArgsValue.size());
    for (size_t Index = 0; Index < ArgsValue.size(); ++Index) {
      auto ArgPath = appendJSONIndexPath(appendJSONPath(FuncPath, "args"),
                                         Index);
      auto ArgRecipe = buildOverrideType(ArgsValue[Index], Ctx, ArgPath, false);
      appendOverrideConstraints(Aggregate, std::move(ArgRecipe));
      Args.push_back(ArgRecipe.Root);
    }

    auto *RetValue = Obj.get("ret");
    assert(RetValue != nullptr);
    RetRecipe = buildOverrideType(*RetValue, Ctx,
                                  appendJSONPath(FuncPath, "ret"), true);
    appendOverrideConstraints(Aggregate, std::move(RetRecipe));
  }

  if (auto *ConstraintsValue = Obj.get("constraints")) {
    const auto &Constraints = requireArray(
        *ConstraintsValue, appendJSONPath(FuncPath, "constraints"));
    for (size_t Index = 0; Index < Constraints.size(); ++Index) {
      auto ConstraintPath = appendJSONIndexPath(
          appendJSONPath(FuncPath, "constraints"), Index);
      const auto &ConstraintObj = requireObject(Constraints[Index],
                                                ConstraintPath);
      auto Kind = requireString(ConstraintObj, "kind", ConstraintPath);
      if (Kind != "subtype") {
        failSignatureOverride(ConstraintPath,
                              "only subtype constraints are supported");
      }
      auto *LHSValue = ConstraintObj.get("lhs");
      auto *RHSValue = ConstraintObj.get("rhs");
      if (LHSValue == nullptr || RHSValue == nullptr) {
        failSignatureOverride(ConstraintPath,
                              "constraint requires lhs and rhs");
      }
      auto LHSRecipe = buildSummaryConstraintOperand(
          *this, G, Ctx, Func, *LHSValue, appendJSONPath(ConstraintPath, "lhs"));
      auto RHSRecipe = buildSummaryConstraintOperand(
          *this, G, Ctx, Func, *RHSValue, appendJSONPath(ConstraintPath, "rhs"));
      appendOverrideConstraints(Aggregate, std::move(LHSRecipe));
      appendOverrideConstraints(Aggregate, std::move(RHSRecipe));
      Aggregate.Constraints.push_back({LHSRecipe.Root, RHSRecipe.Root});
    }
  }

  applyOverrideRecipe(G, Aggregate);

  if (HasFunctionType) {
    auto FuncNode = G.getNodeOrNull(&Func);
    assert(FuncNode != nullptr);
    G.addSubtype(binarysub::make_function(Args, RetRecipe.Root), FuncNode);
  }
  applyPNDiffOverrides(G, Func, Obj, FuncPath,
                       &WrotePNDiffOverrideWarningHeader);
}

void MLsubRecovery::applyUpperBoundSignatureOverride(
    ConstraintsGenerator &G, llvm::Function &Func,
    const llvm::json::Value &Spec) {
  const auto &Obj = requireObject(Spec, Func.getName());
  std::string FuncPath = Func.getName().str();
  OverrideBuildContext Ctx{.Generator = G, .Func = Func};

  const auto &ArgsValue =
      requireArray(*Obj.get("args"), appendJSONPath(FuncPath, "args"));
  std::vector<SimpleType> Args;
  Args.reserve(ArgsValue.size());
  OverrideTypeRecipe Aggregate;
  for (size_t Index = 0; Index < ArgsValue.size(); ++Index) {
    auto ArgPath = appendJSONIndexPath(appendJSONPath(FuncPath, "args"),
                                       Index);
    auto ArgRecipe = buildOverrideType(ArgsValue[Index], Ctx, ArgPath, false);
    appendOverrideConstraints(Aggregate, std::move(ArgRecipe));
    Args.push_back(ArgRecipe.Root);
  }

  auto *RetValue = Obj.get("ret");
  assert(RetValue != nullptr);
  auto RetRecipe =
      buildOverrideType(*RetValue, Ctx, appendJSONPath(FuncPath, "ret"), true);
  appendOverrideConstraints(Aggregate, std::move(RetRecipe));

  if (auto *ConstraintsValue = Obj.get("constraints")) {
    const auto &Constraints = requireArray(
        *ConstraintsValue, appendJSONPath(FuncPath, "constraints"));
    for (size_t Index = 0; Index < Constraints.size(); ++Index) {
      auto ConstraintPath = appendJSONIndexPath(
          appendJSONPath(FuncPath, "constraints"), Index);
      const auto &ConstraintObj = requireObject(Constraints[Index],
                                                ConstraintPath);
      auto Kind = requireString(ConstraintObj, "kind", ConstraintPath);
      if (Kind != "subtype") {
        failSignatureOverride(ConstraintPath,
                              "only subtype constraints are supported");
      }
      auto *LHSValue = ConstraintObj.get("lhs");
      auto *RHSValue = ConstraintObj.get("rhs");
      if (LHSValue == nullptr || RHSValue == nullptr) {
        failSignatureOverride(ConstraintPath,
                              "constraint requires lhs and rhs");
      }
      auto LHSRecipe = buildOverrideType(
          *LHSValue, Ctx, appendJSONPath(ConstraintPath, "lhs"), false);
      auto RHSRecipe = buildOverrideType(
          *RHSValue, Ctx, appendJSONPath(ConstraintPath, "rhs"), false);
      appendOverrideConstraints(Aggregate, std::move(LHSRecipe));
      appendOverrideConstraints(Aggregate, std::move(RHSRecipe));
      Aggregate.Constraints.push_back({LHSRecipe.Root, RHSRecipe.Root});
    }
  }

  applyOverrideRecipe(G, Aggregate);

  auto FuncNode = G.getNodeOrNull(&Func);
  assert(FuncNode != nullptr);
  G.addSubtype(FuncNode, binarysub::make_function(Args, RetRecipe.Root));
  applyPNDiffOverrides(G, Func, Obj, FuncPath,
                       &WrotePNDiffOverrideWarningHeader);
}

void MLsubRecovery::applyExtraConstraints(ConstraintsGenerator &G,
                                          llvm::Function &Func,
                                          const llvm::json::Value &Spec) {
  const auto *Obj = Spec.getAsObject();
  if (Obj == nullptr) {
    failExtraConstraints(Func.getName(), "expected object");
  }
  applyExtraConstraintSubtypeActions(*this, G, Func, *Obj, Func.getName());
  applyExtraConstraintPNDiffs(G, Func, *Obj, Func.getName(),
                              &WrotePNDiffOverrideWarningHeader);
}

void MLsubRecovery::bottomUpPhase() {
  // Iterate bottom up.
  for (std::size_t Ind = AG.AllSCCs.size(); Ind-- > 0;) {
    auto &Data = AG.AllSCCs.at(Ind);
    Data.Generator = std::make_shared<ConstraintsGenerator>(
        Data.SCCName, PointerSize, Data.SCCSet, MemoryType, StorageType,
        &StorageFields, Data.level, BinarysubTraceFile.get(), MergeEval,
        EnableStructPtrLoadStoreMerge);
    auto &G = Data.Generator;
    // insert ContraVariantValues
    if (Ind == 0) {
      // // add main arguments
      // if (auto Main = Mod.getFunction("main")) {
      //   for (auto &Arg: Main->args()) {
      //     G->ContraVariantValues.insert(&Arg);
      //   }
      // }
      // TODO parameters with unknown type
    } else {
      // find SCC interface functions.
      for (auto N : Data.Nodes) {
        if (AG.Callee2Callers.count(N) == 0) {
          continue;
        }
        for (auto Caller : AG.Callee2Callers[N]) {
          if (auto F = Caller->getFunction()) {
            for (auto &Arg : F->args()) {
              G->ContraVariantValues.insert(&Arg);
            }
          }
        }
      }
    }

    G->run();

    for (auto *Func : Data.SCCSet) {
      if (auto *ExtraSpec = getExtraConstraintsSpec(*Func)) {
        llvm::errs() << "Applying MLsub extra constraints to "
                     << Func->getName() << "\n";
        applyExtraConstraints(*G, *Func, *ExtraSpec);
      }
      if (auto *SummarySpec = getSummaryOverrideSpec(*Func)) {
        llvm::errs() << "Applying MLsub summary override to "
                     << Func->getName() << "\n";
        applySummaryOverride(*G, *Func, *SummarySpec);
      }
      auto *Spec = getSignatureOverrideSpec(*Func);
      if (Spec == nullptr) {
        continue;
      }
      assert(!Func->isDeclaration() &&
             "signature overrides are validated to require definitions");
      llvm::errs() << "Applying MLsub signature override to "
                   << Func->getName() << "\n";
      applyUpperBoundSignatureOverride(*G, *Func, *Spec);
    }

    // create poly schemes and instantiate for unhandled calls.
    for (auto &Ent : Data.Generator->unhandledCalls) {
      auto F = Ent.first->getCalledFunction();
      auto TargetNode = AG.CG->getOrInsertFunction(F);
      auto TargetIt = AG.Func2SCCIndex.find(TargetNode);
      if (TargetIt == AG.Func2SCCIndex.end()) {
        llvm::errs() << "Warning: skip unhandled call without MLsub SCC: "
                     << F->getName() << "\n";
        continue;
      }
      auto Ind2 = TargetIt->second;
      assert(Ind2 > Ind);
      auto &TData = AG.AllSCCs.at(Ind2);
      auto TargetG = TData.Generator;
      auto TargetFTy = TargetG->getNodeOrNull(F);
      auto PolyScheme = binarysub::TypeScheme(
          binarysub::PolymorphicType(TData.level, TargetFTy));
      auto TargetLevel = binarysub::level_of(TargetFTy);
      assert(TargetLevel >= 0);
      assert(TData.level == static_cast<unsigned int>(TargetLevel));
      assert(TData.level >= Data.level);
      auto InsFunc = PolyScheme.instantiate(Data.level);
      Data.Generator->addSubtype(InsFunc, Ent.second);
    }
    Data.Generator->unhandledCalls.clear();
  }
}

void ConstraintsGenerator::genTypes(TypeBuilderContext &TBCtx,
                                    bool SolveGlobals) {
  binarysub::TypeSimplifier Ts;
  using binarysub::PolarVar;
  SnapshotContraVariantValues = ContraVariantValues;
  // Late merge policies can leave V2N entries pointing at variables that now
  // forward to a representative through mergedInto.  Normalize here so the final
  // solve roots, eval grouping, and HType lowering all use the same root.
  std::vector<std::pair<SimpleType, SimpleType>> ResolvedV2NMerges;
  ResolvedV2NMerges.reserve(V2N.rev().size());
  for (const auto &Ent : V2N.rev()) {
    auto Resolved = binarysub::resolve_variable(Ent.first);
    if (Resolved && Resolved.get() != Ent.first.get()) {
      ResolvedV2NMerges.push_back({Ent.first, Resolved});
    }
  }
  for (const auto &[From, To] : ResolvedV2NMerges) {
    V2N.merge(From, To);
  }
  std::set<PolarVar> Tys;
  for (auto &Ent : V2N) {
    if (!Ent.second->isVariableState()) {
      continue;
    }
    Tys.insert(PolarVar{.var = Ent.second, .pos = true});
    Tys.insert(PolarVar{.var = Ent.second, .pos = false});
  }
  auto PolMem = PolarVar{.var = MemoryType, .pos = false};
  auto PolStorage = PolarVar{.var = StorageType, .pos = false};
  if (SolveGlobals) {
    Tys.insert(PolMem);
    if (StorageType != nullptr) {
      Tys.insert(PolStorage);
    }
  }
  binarysub::BulkSimplifyOptions BulkOptions;
  BulkOptions.enableParallel = true;
  auto BulkResult = Ts.bulkSimplifyDetailed(Tys, false, BulkOptions);
  const auto &Res = BulkResult.types;

  // Create a short-lived builder over the shared context.  The context keeps
  // record/type caches for the whole module-level HType build stage.
  TypeBuilder TB(TBCtx);

  auto convertSolvedType = [&](SimpleType Ty, bool Pos,
                               llvm::StringRef RootLabel) {
    binarysub::UTypePtr SolvedTy = nullptr;
    if (Ty->isVariableState()) {
      auto It = Res.find(PolarVar{.var = Ty, .pos = Pos});
      if (It != Res.end()) {
        SolvedTy = It->second;
      }
    } else {
      SolvedTy = binarysub::coalesceType(Ty, Pos);
    }
    if (!SolvedTy) {
      return static_cast<ast::HType *>(nullptr);
    }
    TB.setDebugRootLabel(RootLabel.str());
    auto *Converted = TB.convert(SolvedTy);
    TB.setDebugRootLabel(std::nullopt);
    return Converted;
  };

  for (auto &Ent : V2N) {
    auto RootLabel = formatTypeBuilderRootLabel(Ent.first);
    auto *Lower = convertSolvedType(Ent.second, true, RootLabel + " lower");
    ValueTypesLower.insert({Ent.first, Lower});
  }
  if (SolveGlobals) {
    auto MemUTy = Res.at(PolMem);
    TB.setDebugRootLabel(std::string("<memory>"));
    ValueTypesUpper.insert({nullptr, TB.convert(MemUTy)});
    TB.setDebugRootLabel(std::nullopt);

    if (StorageType != nullptr && StorageFields != nullptr) {
      auto It = Res.find(PolStorage);
      if (It != Res.end() && It->second != nullptr) {
        TB.setDebugRootLabel(std::string("<storage>"));
        StorageHType = TB.convertStorageRecord(It->second, *StorageFields);
        TB.setDebugRootLabel(std::nullopt);
        if (StorageHType != nullptr && StorageHType->isRecordType()) {
          StorageDecl = StorageHType->getAsRecordDecl();
        }
      }
    }
  }
  for (auto &Ent : V2N) {
    auto RootLabel = formatTypeBuilderRootLabel(Ent.first);
    auto *Upper = convertSolvedType(Ent.second, false, RootLabel + " upper");
    ValueTypesUpper.insert({Ent.first, Upper});
  }

  if (auto WorkDir = notdec::getWorkDirOpt()) {
    appendDebugValueTypes(*WorkDir, Name, V2N, ContraVariantValues, Res,
                          OriginalVariableSources, SolveGlobals, PolMem);
    appendDebugVarOrigins(*WorkDir, Name, V2N, ContraVariantValues, Res,
                          OriginalVariableSources, SolveGlobals, PolMem);
  }
}

void ConstraintsGenerator::releaseBinarysubState() {
  for (auto &Ent : V2N) {
    binarysub::release_type_graph(Ent.second);
  }
  binarysub::release_type_graph(MemoryType);
  V2N = {};
  ContraVariantValues.clear();
  OriginalVariableSources.clear();
  unhandledCalls.clear();
}

void MLsubRecovery::genASTTypes(llvm::Module &M) {
  ResultVal = std::make_unique<Result>();
  ResultVal->PrintEmptyStorageSection = isEVMModule(M);
  EVMStores.clear();
  // 合并所有类型到一个大的 HTypeResult 里面。
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    for (auto &Ent : Data.Generator->ValueTypesLower) {
      // Polymorphic callees temporarily solve caller interface values so calls can
      // instantiate their summaries. Keep the owning SCC's HType and ignore those
      // later interface copies when building the final module-level result.
      ResultVal->ValueTypesLower.insert(Ent);
    }
    for (auto &Ent : Data.Generator->ValueTypesUpper) {
      ResultVal->ValueTypesUpper.insert(Ent);
    }
    ResultVal->ContraVariantValues.insert(
        Data.Generator->SnapshotContraVariantValues.begin(),
        Data.Generator->SnapshotContraVariantValues.end());
    if (isEVMModule(M)) {
      for (const auto &Ent : Data.Generator->MemoryAccesses.StoresByAddr) {
        for (const RecordedStore &Store : Ent.second) {
          auto *StoreInst =
              llvm::dyn_cast_or_null<llvm::StoreInst>(Store.Source);
          if (StoreInst == nullptr) {
            continue;
          }
          EVMStores.push_back(EVMStoreEvidence{
              .Addr = Store.Addr,
              .StoredValue = StoreInst->getValueOperand(),
              .BitSize = Store.BitSize,
              .Source = Store.Source,
          });
        }
      }
    }
  }
  ResultVal->HTCtx = HCtx;
  // handle Memory type.
  auto Mem = AG.AllSCCs.at(0).Generator->ValueTypesUpper.at(nullptr);
  ResultVal->MemoryType = Mem;
  if (Mem->isRecordType()) {
    ResultVal->MemoryDecl = Mem->getAsRecordDecl();
  }
  auto *Storage = AG.AllSCCs.at(0).Generator->StorageHType;
  ResultVal->StorageType = Storage;
  ResultVal->StorageDecl = AG.AllSCCs.at(0).Generator->StorageDecl;
  normalizeHTypeResult(*ResultVal);
  if (auto WorkDir = notdec::getWorkDirOpt()) {
    writeDebugEVMMarkerFacts(*WorkDir, M);
    writeDebugValueHTypes(*WorkDir, *ResultVal);
    writeDebugImportantHTypes(*WorkDir, *ResultVal);
  }
}

void MLsubRecovery::topDownPhase() {
  // TODO 怎么处理TopDown的类型传递？设置多态参数的类型？
  // TODO 对于每个SCCData的所有Caller，都instantiate到 -x level？
  // 也许我该基于最后都优化完毕之后的CompactType？？

  if (!HCtx) {
    HCtx = std::make_shared<ast::HTypeContext>();
  }
  TypeBuilderContext TBCtx(*HCtx, Mod.getDataLayout().getPointerSize(),
                           notdec::getWorkDirOpt());
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    // 尝试运行简化算法，保存到ValueTypes里面。
    // solve memory if ind == 0
    Data.Generator->genTypes(TBCtx, Ind == 0);
  }
  if (MergeEval) {
    MergeEval->finish(AG);
  }
  for (auto &Data : AG.AllSCCs) {
    Data.Generator->releaseBinarysubState();
  }
  binarysub::release_type_graph(StorageType);
  StorageType = nullptr;
  StorageFields.clear();
}

void MLsubRecovery::prepareSCC(CallGraph &CG) {
  AG.CG = &CG;
  AG.AllSCCs.clear();
  AG.Func2SCCIndex.clear();
  AG.Callee2Callers.clear();

  auto PolyPolicy = loadPolyPolicyConfig();
  const auto &PolyFuncs = PolyPolicy.PolyFuncs;
  const auto &LevelOverrides = PolyPolicy.LevelOverrides;

  all_scc_iterator<CallGraph *> CGI = notdec::scc_begin(AG.CG);
  // 把CGI遍历的结果都顺序保存到vector里
  std::vector<std::vector<CallGraphNode *>> SCCResults;
  for (; !CGI.isAtEnd(); ++CGI) {
    SCCResults.push_back(*CGI);
  }
  // 遍历所有的CallGraphNode，然后构建一个反向的，从callee到所有caller的map
  for (auto &KV : *AG.CG) {
    CallGraphNode *Caller = KV.second.get();
    for (auto &CallRecord : *Caller) {
      CallGraphNode *Callee = CallRecord.second;
      AG.Callee2Callers[Callee].insert(Caller);
    }
  }

  auto isAllDeclarationAndIntrinsics =
      [](const std::vector<CallGraphNode *> &NodeVec) -> bool {
    for (auto *CGN : NodeVec) {
      if (auto *Fn = CGN->getFunction()) {
        if (!Fn->isDeclaration() || !Fn->isIntrinsic()) {
          return false;
        }
      }
    }
    return true;
  };

  auto hasPolymorphic =
      [&](const std::vector<CallGraphNode *> &NodeVec) -> bool {
    for (auto *CGN : NodeVec) {
      if (auto *Fn = CGN->getFunction()) {
        if (isSummaryOverridePolymorphic(*Fn)) {
          return true;
        }
        if (Fn->hasName() && PolyFuncs.count(Fn->getName().str())) {
          return true;
        }
        if (Fn->getMetadata(KIND_MLSUB_POLYMORPHIC_FUNCTION) != nullptr) {
          return true;
        }
        if (isPolymorphic(Fn)) {
          return true;
        }
      }
    }
    return false;
  };

  struct RawSCCInfo {
    std::vector<CallGraphNode *> Nodes;
    std::set<std::size_t> Preds;
    std::set<std::size_t> Succs;
    bool IsPolymorphic = false;
    unsigned int UserLevelLowerBound = 0;
    unsigned int Level = 0;
    std::size_t TopoPosition = 0;
  };

  // Phase 1: work on the original call-graph SCC DAG first. Levels are solved
  // on raw SCCs before any same-level groups are merged.
  std::vector<RawSCCInfo> RawSCCs;
  std::map<CallGraphNode *, std::size_t> Node2RawSCCIndex;
  for (auto It = SCCResults.rbegin(); It != SCCResults.rend(); ++It) {
    const std::vector<CallGraphNode *> &NodeVec = *It;
    if (isAllDeclarationAndIntrinsics(NodeVec)) {
      continue;
    }
    std::size_t RawIndex = RawSCCs.size();
    RawSCCs.push_back(RawSCCInfo{.Nodes = NodeVec});
    for (auto *Node : NodeVec) {
      Node2RawSCCIndex[Node] = RawIndex;
    }
  }

  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    auto &Raw = RawSCCs[RawIndex];
    Raw.IsPolymorphic = hasPolymorphic(Raw.Nodes);
    for (auto *Node : Raw.Nodes) {
      auto *Fn = Node->getFunction();
      if (Fn == nullptr || !Fn->hasName()) {
        continue;
      }
      auto It = LevelOverrides.find(Fn->getName().str());
      if (It != LevelOverrides.end()) {
        Raw.UserLevelLowerBound =
            std::max(Raw.UserLevelLowerBound, It->second);
      }
    }
  }

  for (auto &[Callee, Callers] : AG.Callee2Callers) {
    auto CalleeIt = Node2RawSCCIndex.find(Callee);
    if (CalleeIt == Node2RawSCCIndex.end()) {
      continue;
    }
    std::size_t CalleeIndex = CalleeIt->second;
    for (auto *Caller : Callers) {
      auto CallerIt = Node2RawSCCIndex.find(Caller);
      if (CallerIt == Node2RawSCCIndex.end()) {
        continue;
      }
      std::size_t CallerIndex = CallerIt->second;
      if (CallerIndex == CalleeIndex) {
        continue;
      }
      RawSCCs[CallerIndex].Succs.insert(CalleeIndex);
      RawSCCs[CalleeIndex].Preds.insert(CallerIndex);
    }
  }

  // Topologically order the SCC DAG so every caller level is known before its
  // callees are processed.
  std::vector<std::size_t> TopoOrder;
  std::vector<unsigned int> InDegree(RawSCCs.size(), 0);
  std::set<std::size_t> Ready;
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    InDegree[RawIndex] = RawSCCs[RawIndex].Preds.size();
    if (InDegree[RawIndex] == 0) {
      Ready.insert(RawIndex);
    }
  }
  while (!Ready.empty()) {
    std::size_t RawIndex = *Ready.begin();
    Ready.erase(Ready.begin());
    RawSCCs[RawIndex].TopoPosition = TopoOrder.size();
    TopoOrder.push_back(RawIndex);
    for (std::size_t SuccIndex : RawSCCs[RawIndex].Succs) {
      assert(InDegree[SuccIndex] > 0);
      --InDegree[SuccIndex];
      if (InDegree[SuccIndex] == 0) {
        Ready.insert(SuccIndex);
      }
    }
  }
  assert(TopoOrder.size() == RawSCCs.size() &&
         "SCC condensation graph must be a DAG");

  for (std::size_t RawIndex : TopoOrder) {
    auto &Raw = RawSCCs[RawIndex];
    unsigned int BaseLevel = 0;
    for (std::size_t PredIndex : Raw.Preds) {
      BaseLevel = std::max(BaseLevel, RawSCCs[PredIndex].Level);
    }
    // Polymorphic SCCs force a summary boundary, so calls into them instantiate
    // from one level below.
    if (Raw.IsPolymorphic) {
      ++BaseLevel;
    }
    Raw.Level = std::max(BaseLevel, Raw.UserLevelLowerBound);
  }

  // Phase 2: collapse the maximal same-level regions. We union along same-level
  // edges only; after that, inter-group edges always go from lower to higher
  // levels, so the merged graph remains a DAG.
  std::vector<std::size_t> Parent(RawSCCs.size());
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    Parent[RawIndex] = RawIndex;
  }
  auto FindRoot = [&](std::size_t Index) {
    std::size_t Root = Index;
    while (Parent[Root] != Root) {
      Root = Parent[Root];
    }
    while (Parent[Index] != Index) {
      std::size_t Next = Parent[Index];
      Parent[Index] = Root;
      Index = Next;
    }
    return Root;
  };
  auto Union = [&](std::size_t LHS, std::size_t RHS) {
    LHS = FindRoot(LHS);
    RHS = FindRoot(RHS);
    if (LHS == RHS) {
      return;
    }
    if (LHS > RHS) {
      std::swap(LHS, RHS);
    }
    Parent[RHS] = LHS;
  };
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    for (std::size_t SuccIndex : RawSCCs[RawIndex].Succs) {
      if (RawSCCs[RawIndex].Level == RawSCCs[SuccIndex].Level) {
        Union(RawIndex, SuccIndex);
      }
    }
  }

  struct GroupInfo {
    std::size_t Root = 0;
    unsigned int Level = 0;
    std::size_t FirstTopoPosition = 0;
  };

  std::map<std::size_t, std::vector<std::size_t>> Groups;
  std::map<std::size_t, std::size_t> FirstTopoPosition;
  for (std::size_t RawIndex : TopoOrder) {
    std::size_t Root = FindRoot(RawIndex);
    Groups[Root].push_back(RawIndex);
    if (!FirstTopoPosition.count(Root)) {
      FirstTopoPosition[Root] = RawSCCs[RawIndex].TopoPosition;
    }
  }

  std::vector<GroupInfo> OrderedGroups;
  OrderedGroups.reserve(Groups.size());
  for (auto &[Root, Members] : Groups) {
    (void)Members;
    OrderedGroups.push_back(GroupInfo{
        .Root = Root,
        .Level = RawSCCs[Root].Level,
        .FirstTopoPosition = FirstTopoPosition.at(Root),
    });
  }
  std::sort(OrderedGroups.begin(), OrderedGroups.end(),
            [](const GroupInfo &LHS, const GroupInfo &RHS) {
              if (LHS.Level != RHS.Level) {
                return LHS.Level < RHS.Level;
              }
              return LHS.FirstTopoPosition < RHS.FirstTopoPosition;
            });

  std::vector<SCCData> &AllSCCs = AG.AllSCCs;
  std::map<CallGraphNode *, std::size_t> &Func2SCCIndex = AG.Func2SCCIndex;
  for (const auto &Group : OrderedGroups) {
    SCCData Data;
    Data.level = Group.Level;
    for (std::size_t RawIndex : Groups.at(Group.Root)) {
      assert(RawSCCs[RawIndex].Level == Data.level);
      Data.Nodes.insert(Data.Nodes.end(), RawSCCs[RawIndex].Nodes.begin(),
                        RawSCCs[RawIndex].Nodes.end());
    }
    AllSCCs.push_back(Data);
  }

  // 2. Calc name and SCCSet
  for (size_t SCCIndex = 0; SCCIndex < AllSCCs.size(); ++SCCIndex) {
    SCCData &Data = AllSCCs[SCCIndex];
    std::set<llvm::Function *> &SCCSet = Data.SCCSet;
    SCCSet.clear();

    std::string Name;
    for (auto *CGN : Data.Nodes) {
      auto *Fn = CGN->getFunction();
      if (Fn == nullptr) {
        continue;
      }
      if (!Name.empty()) {
        Name += ",";
      }
      Name += Fn->getName().str();
      SCCSet.insert(Fn);
      Func2SCCIndex[CGN] = SCCIndex;
    }
    if (!Name.empty()) {
      Data.SCCName = Name;
    }
    // write the SCC index to file
    if (SCCsCatalog) {
      *SCCsCatalog << "SCC" << SCCIndex << "," << Name
                   << " (level = " << Data.level << ")" << "\n";
    }
  }
}

SimpleType ConstraintsGenerator::convertSimpleType(ExtValuePtr Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertSimpleTypeVal(*V, nullptr, -1);
  } else if (std::get_if<ReturnValue>(&Val)) {
    return binarysub::make_variable(lvl, getSize(Val));
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    return convertSimpleTypeVal(IC->Val, IC->User, IC->OpInd);
  } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
    // as field access.
    auto res = binarysub::fresh_variable(lvl, getSize(Val));
    if (auto Offset = getSigned64ConstantAddress(*CA->Val)) {
      std::vector<std::pair<std::string, SimpleType>> fields;
      fields.push_back({OffsetRange{.offset = *Offset}.str(), res});
      addSubtype(MemoryType, binarysub::make_record(std::move(fields)));
    }
    addSubtype(res, binarysub::make_record({}));
    return res;
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: ConstraintsGenerator::convertSimpleType unhandled "
                  "type of ExtValPtr\n";
  std::abort();
}

SimpleType ConstraintsGenerator::makeAggregateReturnRecord(
    llvm::Type *Ty,
    const std::function<SimpleType(unsigned, llvm::Type *)> &MakeField) {
  std::vector<std::pair<std::string, SimpleType>> Fields;
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    Fields.reserve(StructTy->getNumElements());
    for (unsigned I = 0; I < StructTy->getNumElements(); ++I) {
      Fields.emplace_back(std::to_string(I),
                          MakeField(I, StructTy->getElementType(I)));
    }
  } else if (auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    Fields.reserve(ArrayTy->getNumElements());
    for (unsigned I = 0; I < ArrayTy->getNumElements(); ++I) {
      Fields.emplace_back(std::to_string(I),
                          MakeField(I, ArrayTy->getElementType()));
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: unsupported aggregate return type: " << *Ty
                 << "\n";
    std::abort();
  }
  return binarysub::make_record(std::move(Fields));
}

static SimpleType makeAggregateReturnFieldSeed(llvm::Type *Ty,
                                               unsigned PointerSize) {
  if (Ty->isIntegerTy(1)) {
    return binarysub::make_primitive("bool", 1);
  }
  if (Ty->isIntegerTy()) {
    return binarysub::make_primitive("uint",
                                     Ty->getScalarSizeInBits());
  }
  if (Ty->isFloatingPointTy()) {
    return binarysub::make_primitive("float",
                                     Ty->getScalarSizeInBits());
  }
  if (Ty->isPointerTy() || Ty->isFunctionTy()) {
    return binarysub::make_record({});
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: unsupported aggregate return field type: " << *Ty
               << "\n";
  std::abort();
}

SimpleType
ConstraintsGenerator::makeFunctionAggregateReturnRecord(llvm::Function &Func) {
  return makeAggregateReturnRecord(
      Func.getReturnType(), [&](unsigned Index, llvm::Type *FieldTy) {
        auto Slot = getOrInsertNode(ReturnValue{
            .Func = &Func, .Index = static_cast<int32_t>(Index)});
        addSubtype(makeAggregateReturnFieldSeed(FieldTy, PointerSize), Slot);
        return Slot;
      });
}

SimpleType
ConstraintsGenerator::getOrCreateCallAggregateReturnSlot(llvm::CallBase &Call,
                                                         unsigned Index) {
  auto Key = std::make_pair(&Call, Index);
  auto It = AggregateCallReturnSlots.find(Key);
  if (It != AggregateCallReturnSlots.end()) {
    return It->second;
  }
  auto *Ty = Call.getType();
  llvm::Type *FieldTy = nullptr;
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    FieldTy = StructTy->getElementType(Index);
  } else if (auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    FieldTy = ArrayTy->getElementType();
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: call is not aggregate typed: " << Call << "\n";
    std::abort();
  }
  auto Slot = binarysub::make_variable(lvl, getLLVMTypeSize(FieldTy));
  addSubtype(makeAggregateReturnFieldSeed(FieldTy, PointerSize), Slot);
  AggregateCallReturnSlots.emplace(Key, Slot);
  return Slot;
}

SimpleType
ConstraintsGenerator::makeCallAggregateReturnRecord(llvm::CallBase &Call) {
  return makeAggregateReturnRecord(
      Call.getType(), [&](unsigned Index, llvm::Type *) {
        return getOrCreateCallAggregateReturnSlot(Call, Index);
      });
}

void ConstraintsGenerator::addAggregateReturnConstraints(llvm::Value *Agg,
                                                         llvm::ReturnInst &Ret) {
  auto *Insert = dyn_cast<InsertValueInst>(Agg);
  if (Insert == nullptr) {
    return;
  }
  addAggregateReturnConstraints(Insert->getAggregateOperand(), Ret);
  auto Indices = Insert->getIndices();
  if (Indices.size() != 1) {
    return;
  }
  auto *Inserted = Insert->getInsertedValueOperand();
  if (Inserted->getType()->isAggregateType() || Inserted->getType()->isVoidTy()) {
    return;
  }
  auto Src = getOrInsertNode(getExtValuePtr(
      Inserted, Insert, InsertValueInst::getInsertedValueOperandIndex()));
  auto Dst = getOrInsertNode(ReturnValue{
      .Func = Ret.getFunction(), .Index = static_cast<int32_t>(Indices[0])});
  addSubtype(Src, Dst);
}

void ConstraintsGenerator::maybeUnifyPNDiffTypeVariablePair(
    const SimpleType &Lhs, const SimpleType &Rhs) {
  if (!EnablePNDiffTypeVariableClosureUnification || !Lhs || !Rhs) {
    return;
  }
  if (!Lhs->isVariableState() || !Rhs->isVariableState()) {
    return;
  }

  auto gatherMappedValues = [&](const SimpleType &Ty) {
    std::vector<ExtValuePtr> Values;
    const auto &Reverse = V2N.rev();
    auto It = Reverse.find(Ty);
    if (It == Reverse.end()) {
      return Values;
    }
    Values.assign(It->second.begin(), It->second.end());
    return Values;
  };

  auto LeftValues = gatherMappedValues(Lhs);
  auto RightValues = gatherMappedValues(Rhs);
  if (LeftValues.empty() || RightValues.empty()) {
    return;
  }

  PNINode *Leader = nullptr;
  auto unifyMappedValues = [&](const std::vector<ExtValuePtr> &Values) {
    for (const auto &Val : Values) {
      auto *Node = getPNINodeOrNull(Val);
      if (Node == nullptr) {
        continue;
      }
      if (Leader == nullptr) {
        Leader = Node;
      } else {
        Leader = Leader->unify(*Node);
      }
    }
  };

  unifyMappedValues(LeftValues);
  unifyMappedValues(RightValues);
  if (Leader != nullptr && PG.traceEnabled()) {
    std::ostringstream OS;
    OS << "[pndiff:closure-unify] lhs=" << binarysub::debug_string(Lhs)
       << " rhs=" << binarysub::debug_string(Rhs)
       << " leader=" << Leader->serialize();
    PG.trace(OS.str());
  }
}

SimpleType ConstraintsGenerator::convertSimpleTypeVal(Value *Val,
                                                      llvm::User *User,
                                                      long OpInd) {
  if (Val->getType()->isIntegerTy(1)) {
    return binarysub::make_primitive("bool", 1);
  } else if (Val->getType()->isFloatingPointTy()) {
    return binarysub::make_primitive(
        "float", getSize(getExtValuePtr(Val, User, OpInd)));
  }

  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast ConstantExpr
      if (CE->getOpcode() == Instruction::BitCast) {
        return convertSimpleType(getExtValuePtr(CE->getOperand(0), CE, 0));
      } else if (CE->getOpcode() == Instruction::IntToPtr) {
        if (isa<ConstantInt>(CE->getOperand(0))) {
          assert(false && "Should be converted earlier");
        }
      } else if (CE->getOpcode() == Instruction::GetElementPtr) {
        // getelementptr of table, i.e., function pointer array
        if (auto GV = dyn_cast<GlobalVariable>(CE->getOperand(0))) {
          if (isFunctionPointerTable(*GV)) {
            // if constant offset
            if (auto CI1 = dyn_cast<ConstantInt>(CE->getOperand(1))) {
              if (CI1->isZero()) {
                if (isa<ConstantInt>(CE->getOperand(2))) {
                  assert(false && "TODO");
                }
              }
            }
          }
        }
        return binarysub::make_variable(
            lvl, getSize(getExtValuePtr(C, User, OpInd)));
      } else {
        llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                     << "ERROR: ConstraintsGenerator::convertSimpleTypeVal "
                        "unhandled ConstantExpr: "
                     << *C << "\n";
      }
    } else if (auto gv = dyn_cast<GlobalValue>(C)) { // global variable
      return binarysub::make_variable(lvl, getSize(gv));
      // if (gv == Ctx.StackPointer) {
      //   std::cerr
      //       << "Error: convertTypeVarVal: direct use of stack pointer?,
      //       ensure "
      //          "StackAllocationRecovery is run before, Or add external
      //          summary " "for this function.\n";
      //   return makeTv(Ctx.TRCtx, ValueNamer::getName());
      // } else if (auto Func = dyn_cast<Function>(C)) {
      //   return makeTv(Ctx.TRCtx, getFuncTvName(Func));
      // }
      // return makeTv(Ctx.TRCtx, gv->getName().str());
    } else if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      if (auto CI = dyn_cast<ConstantInt>(C)) {
        return binarysub::make_variable(
            lvl, getSize(getExtValuePtr(CI, User, OpInd)));
      }
      return binarysub::make_primitive(
          "float", getSize(getExtValuePtr(C, User, OpInd)));
    } else if (isa<ConstantPointerNull>(C)) {
      return binarysub::make_variable(
          lvl, getSize(getExtValuePtr(C, User, OpInd)));
    } else if (C->getType()->isAggregateType() || C->getType()->isVectorTy()) {
      return binarysub::make_variable(
          lvl, getSize(getExtValuePtr(C, User, OpInd)));
    } else if (isa<UndefValue>(C)) {
      return binarysub::make_variable(
          lvl, getSize(getExtValuePtr(C, User, OpInd)));
    }
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: ConstraintsGenerator::convertSimpleTypeVal "
                    "unhandled type of constant: "
                 << *C << "\n";
    std::abort();
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    return binarysub::make_variable(lvl, getSize(arg));
  }

  if (auto *I = dyn_cast<Instruction>(Val)) {
    return binarysub::make_variable(lvl, getSize(I));
  }
  llvm::errs()
      << __FILE__ << ":" << __LINE__ << ": "
      << "WARN: ConstraintsGenerator::convertSimpleTypeVal unhandled value: "
      << *Val << "\n";
  return binarysub::make_variable(lvl, getSize(Val));
}

// #region ConstraintsGenerator::MLsubVisitor

// Helper functions
static bool isWithOverflowIntrinsicSigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::sadd_with_overflow ||
      ID == Intrinsic::ssub_with_overflow ||
      ID == Intrinsic::smul_with_overflow) {
    return true;
  }
  return false;
}

static bool isWithOverflowIntrinsicUnsigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::uadd_with_overflow ||
      ID == Intrinsic::usub_with_overflow ||
      ID == Intrinsic::umul_with_overflow) {
    return true;
  }
  return false;
}

static bool isMinMaxIntrinsic(llvm::Intrinsic::ID ID) {
  return ID == Intrinsic::smax || ID == Intrinsic::smin ||
         ID == Intrinsic::umax || ID == Intrinsic::umin;
}

static bool isEVMModule(const llvm::Module &M) {
  return StringRef(M.getTargetTriple().getTriple()).starts_with("evm");
}

static SimpleType makeEVMSemanticPrimitive(llvm::StringRef Name) {
  return binarysub::make_primitive(("prim.uint256.evm." + Name).str(), 256);
}

static bool constantHasAllPointerTagMiddleBits(const ConstantInt &CI) {
  const APInt &Val = CI.getValue();
  unsigned BitWidth = Val.getBitWidth();
  if (BitWidth <= 8) {
    return false;
  }
  APInt Mask = APInt::getBitsSet(BitWidth, 8, std::min(BitWidth, 30u));
  return (Val & Mask) == Mask;
}

static bool constantHasNoPointerTagMiddleBits(const ConstantInt &CI) {
  const APInt &Val = CI.getValue();
  unsigned BitWidth = Val.getBitWidth();
  if (BitWidth <= 8) {
    return true;
  }
  APInt Mask = APInt::getBitsSet(BitWidth, 8, std::min(BitWidth, 30u));
  return !(Val & Mask);
}

static std::optional<std::uint64_t> getUInt64Constant(const Value *V) {
  const auto *CI = dyn_cast_or_null<ConstantInt>(V);
  if (CI == nullptr || CI->getValue().getActiveBits() > 64) {
    return std::nullopt;
  }
  return CI->getZExtValue();
}

static inline void ensureSequence(Value *&Src1, Value *&Src2) {
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      llvm::isa<llvm::ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      !llvm::isa<llvm::ConstantInt>(Src2)) {
    // because of InstCombine canonical form, this should not happen?
    assert(false &&
           "Constant cannot be at the left side. Run InstCombine first.");
    std::swap(Src1, Src2);
  }
}

// Visitor method implementations
void ConstraintsGenerator::MLsubVisitor::visitExtractValueInst(
    ExtractValueInst &I) {
  if (auto Call = dyn_cast<CallBase>(I.getAggregateOperand())) {
    if (auto Target = Call->getCalledFunction()) {
      if (Target->isIntrinsic()) {
        // 这里判断返回值是不是那种extract
        // value的东西，根据llvm类型直接设置为数字类型
        auto Ind = I.getIndices()[0];
        if (Ind == 0) {
          if (isWithOverflowIntrinsicSigned(Target->getIntrinsicID())) {
            auto N = cg.createNode(&I);
            // Overflow intrinsics return {value, overflow}; field 0 is the
            // arithmetic result and should stay a plain number.
            cg.setNonPointer(&I);
            auto SintNode = binarysub::make_primitive("sint", cg.getSize(&I));
            cg.addSubtype(SintNode, N);
            return;
          } else if (isWithOverflowIntrinsicUnsigned(
                         Target->getIntrinsicID())) {
            auto N = cg.createNode(&I);
            // Overflow intrinsics return {value, overflow}; field 0 is the
            // arithmetic result and should stay a plain number.
            cg.setNonPointer(&I);
            auto UintNode = binarysub::make_primitive("uint", cg.getSize(&I));
            cg.addSubtype(UintNode, N);
            return;
          }
        } else if (Ind == 1) {
          assert(I.getType()->isIntegerTy(1));
          cg.createNode(&I);
          return;
        }
      }
    }
    auto Indices = I.getIndices();
    if (Indices.size() == 1 && !I.getType()->isAggregateType()) {
      auto Dst = cg.createNode(&I);
      auto Src = cg.getOrCreateCallAggregateReturnSlot(*Call, Indices[0]);
      cg.addSubtype(Src, Dst);
      return;
    }
  }
  if (!I.getType()->isAggregateType()) {
    cg.createNode(&I);
    return;
  }
  assert(false && "TODO: ExtractValueInst aggregate result");
}

void ConstraintsGenerator::MLsubVisitor::visitInsertValueInst(
    InsertValueInst &I) {
  // The aggregate value itself is only an LLVM carrier for multi-return values.
  // ReturnInst walks the insertvalue chain and connects inserted scalar values
  // to the current function's per-field return slots.
  if (I.getType()->isAggregateType()) {
    return;
  }
  visitInstruction(I);
}

void ConstraintsGenerator::MLsubVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst, PtrToIntInst, IntToPtrInst>(I)) {
    if (isa<IntToPtrInst>(I) && I.getModule() != nullptr &&
        isEVMModule(*I.getModule())) {
      if (auto *CI = dyn_cast<ConstantInt>(I.getOperand(0))) {
        ConstantAddr Addr{.Val = CI};
        cg.getOrInsertNode(Addr);
        cg.addRemapType(&I, Addr);
        return;
      }
    }
    // Treat pointer/int casts as aliases for PNDiff even when the source
    // hasn't been materialized as a MLsub type variable yet.
    auto *Src = I.getOperand(0);
    auto SrcVal = getExtValuePtr(Src, &I, 0);
    cg.getOrInsertNode(SrcVal);
    if (cg.getPNINodeOrNull(SrcVal) != nullptr) {
      // Materialize the cast result first so its LLVM low type participates in
      // the merge; otherwise inttoptr can lose the result-side pointer hint and
      // collapse to the source's unknown node.
      cg.getOrInsertPNINode(&I);
      cg.remapPNINode(&I, SrcVal);
    }
    if (cg.getNodeOrNull(SrcVal) != nullptr) {
      cg.addRemapType(&I, SrcVal);
    }
    return;
  } else if (isa<TruncInst, ZExtInst, SExtInst, FPToUIInst, FPToSIInst,
                 UIToFPInst, SIToFPInst, FPTruncInst, FPExtInst>(&I)) {
    visitInstruction(I);
    return;
  }

  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: unhandled CastInst: " << I << "\n";
  std::abort();
}

bool ConstraintsGenerator::MLsubVisitor::isHeapAllocationCall(
    llvm::CallBase &I) {
  if (auto F = I.getCalledFunction()) {
    StringRef Name = F->getName();
    if (Name == "malloc" || Name == "calloc") {
      return true;
    }
    auto *M = I.getModule();
    bool IsEVM = M != nullptr && isEVMModule(*M);
    if (IsEVM && (Name == "calloc_unbounded" ||
                  Name == "notdec_evm_alloc" ||
                  Name == "notdec_evm_alloc_unbounded")) {
      return true;
    }
  }
  return false;
}

bool ConstraintsGenerator::MLsubVisitor::shouldIgnoreRuntimeCall(
    llvm::CallBase &I) {
  auto *F = I.getCalledFunction();
  auto *M = I.getModule();
  if (F == nullptr || M == nullptr || !isEVMModule(*M)) {
    return false;
  }

  StringRef Name = F->getName();
  if (Name == "notdec_evm_finalize_alloc") {
    return true;
  }
  if (Name.starts_with("notdec_solidity_")) {
    return true;
  }
  return Name.starts_with("evm_");
}

bool ConstraintsGenerator::MLsubVisitor::handleEVMMarkerCall(
    llvm::CallBase &I) {
  auto *F = I.getCalledFunction();
  auto *M = I.getModule();
  if (F == nullptr || M == nullptr || !isEVMModule(*M)) {
    return false;
  }

  StringRef Name = F->getName();
  if (Name == "notdec_evm_calldata_min_size") {
    if (I.arg_size() >= 1 && !I.getType()->isVoidTy()) {
      auto Calldata = getExtValuePtr(I.getArgOperand(0), &I, 0);
      cg.getOrInsertNode(Calldata);
      cg.addRemapType(&I, Calldata);
    }
    if (I.arg_size() >= 2 && I.getArgOperand(1)->getType()->isIntegerTy()) {
      cg.setNonPointer(getExtValuePtr(I.getArgOperand(1), &I, 1));
    }
    return true;
  }

  bool IsWordWrite =
      Name == "notdec_solidity_memory_write" ||
      Name == "notdec_solidity_abi_return_data_word_write" ||
      Name == "notdec_solidity_event_data_word_write" ||
      Name == "notdec_solidity_external_call_input_word_write";
  bool IsWordRead = Name == "notdec_solidity_memory_read" ||
                    Name == "notdec_solidity_external_call_output_word_read";
  if (!IsWordWrite && !IsWordRead) {
    return false;
  }
  if (I.arg_size() < 3) {
    return true;
  }

  auto Offset = getUInt64Constant(I.getArgOperand(1));
  if (!Offset.has_value()) {
    return true;
  }

  OffsetRange Field{.offset = static_cast<OffsetTy>(*Offset)};
  auto BaseTy = cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(0), &I, 0));
  auto ValueTy = cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(2), &I, 2));
  std::vector<std::pair<std::string, SimpleType>> Fields;
  Fields.emplace_back(Field.str(), ValueTy);
  auto RecordTy = binarysub::make_record(std::move(Fields));

  cg.addSubtype(BaseTy, RecordTy);
  return true;
}

void addEVMSemanticConstraint(ConstraintsGenerator &CG, ExtValuePtr Val,
                              llvm::StringRef SemanticName) {
  CG.setNonPointer(Val);
  CG.addSubtype(makeEVMSemanticPrimitive(SemanticName),
                CG.getOrInsertNode(Val));
}

std::optional<std::string> getDirectStorageSlotFieldName(llvm::Value *Slot) {
  auto DirectSlot = getUInt64Constant(Slot);
  if (!DirectSlot.has_value()) {
    return std::nullopt;
  }
  return "slot:" + std::to_string(*DirectSlot);
}

struct MappingStorageKeyConstraint {
  std::string FieldName;
  ExtValuePtr Key = static_cast<llvm::Value *>(nullptr);
};

struct StorageFieldPrefixMatch {
  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct StorageValueFieldPrefixMatch {
  std::string FieldName;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct MappingStorageFieldMatch {
  std::string Prefix;
  std::string ValueFieldName;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct ArrayStorageFieldMatch {
  std::string Prefix;
  ExtValuePtr Index = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct ArrayLengthStorageFieldMatch {
  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct StaticArrayStorageFieldMatch {
  std::string Prefix;
  ExtValuePtr Index = static_cast<llvm::Value *>(nullptr);
};

struct PackedStorageFieldReadMatch {
  std::string FieldName;
  ExtValuePtr Value = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct PackedStorageFieldWriteMatch {
  std::string FieldName;
  ExtValuePtr Value = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

llvm::Value *getStorageEvidenceValue(const ExtValuePtr &Evidence) {
  if (auto *V = std::get_if<llvm::Value *>(&Evidence)) {
    return *V;
  }
  if (auto *C = std::get_if<UConstant>(&Evidence)) {
    return C->Val;
  }
  return nullptr;
}

void addStorageValueEvidenceConstraint(ConstraintsGenerator &cg,
                                       llvm::StringRef FieldName,
                                       const ExtValuePtr &Value) {
  if (getStorageEvidenceValue(Value) == nullptr) {
    return;
  }
  auto FieldTy = cg.getOrCreateStorageField(FieldName, 256);
  auto ValueTy = cg.getOrInsertNode(Value);
  cg.addSubtype(ValueTy, FieldTy);
}

std::optional<SimpleType> getStorageEvidenceType(ConstraintsGenerator &cg,
                                                 const ExtValuePtr &Value) {
  if (getStorageEvidenceValue(Value) == nullptr) {
    return std::nullopt;
  }
  return cg.getOrInsertNode(Value);
}

void addStorageKeyConstraints(
    ConstraintsGenerator &cg,
    llvm::ArrayRef<MappingStorageKeyConstraint> KeyConstraints) {
  for (const auto &KeyConstraint : KeyConstraints) {
    addStorageValueEvidenceConstraint(cg, KeyConstraint.FieldName,
                                      KeyConstraint.Key);
  }
}

struct BytesStorageLengthMatch {
  std::string FieldName;
  ExtValuePtr Length = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct BytesStorageShortDataMatch {
  std::string FieldName;
  ExtValuePtr Data = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

struct BytesStorageLongElemMatch {
  std::string Prefix;
  ExtValuePtr Index = static_cast<llvm::Value *>(nullptr);
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
};

std::optional<StorageFieldPrefixMatch>
getStorageFieldPrefixFromBaseSlot(llvm::Value *BaseSlot, unsigned Depth = 4) {
  if (auto DirectSlot = getUInt64Constant(BaseSlot)) {
    return StorageFieldPrefixMatch{
        .Prefix = "slot:" + std::to_string(*DirectSlot),
    };
  }

  if (Depth == 0) {
    return std::nullopt;
  }

  auto *Sha3 = dyn_cast<llvm::CallBase>(BaseSlot);
  if (Sha3 == nullptr) {
    return std::nullopt;
  }

  auto Scratch =
      notdec::passes::evm::detail::matchStorageScratchKeccak(*Sha3);
  if (!Scratch.has_value()) {
    return std::nullopt;
  }

  auto ScratchBaseSlot = getStorageEvidenceValue(Scratch->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(ScratchBaseSlot, Depth - 1);
  if (!Base.has_value()) {
    return std::nullopt;
  }

  auto Prefix = Base->Prefix + ".map";
  Base->KeyConstraints.push_back(MappingStorageKeyConstraint{
      .FieldName = Prefix + ".key",
      .Key = Scratch->Key,
  });
  Base->Prefix = Prefix + ".value";
  return Base;
}

std::optional<uint64_t> getConstantOffsetFromBase(llvm::Value *V,
                                                  llvm::Value *Base,
                                                  unsigned Depth) {
  if (notdec::passes::evm::detail::isSameValue(V, Base)) {
    return 0;
  }
  if (Depth == 0) {
    return std::nullopt;
  }

  auto getOffsetFromOperands =
      [&](llvm::Value *LHS, llvm::Value *RHS) -> std::optional<uint64_t> {
    if (auto LOffset = getConstantOffsetFromBase(LHS, Base, Depth - 1)) {
      if (auto RConst = getUInt64Constant(RHS)) {
        return *LOffset + *RConst;
      }
    }
    if (auto ROffset = getConstantOffsetFromBase(RHS, Base, Depth - 1)) {
      if (auto LConst = getUInt64Constant(LHS)) {
        return *ROffset + *LConst;
      }
    }
    return std::nullopt;
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getOffsetFromOperands(Add->getOperand(0), Add->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getOffsetFromOperands(Call->getArgOperand(0),
                                   Call->getArgOperand(1));
    }
  }

  return std::nullopt;
}

std::optional<StorageValueFieldPrefixMatch>
getStorageValueFieldPrefixFromSlotExpression(llvm::Value *StorageSlot,
                                             bool UseField0ForMappingBase,
                                             unsigned Depth = 2) {
  if (auto Base = getStorageFieldPrefixFromBaseSlot(StorageSlot)) {
    std::string FieldName = Base->Prefix;
    if (UseField0ForMappingBase && !Base->KeyConstraints.empty()) {
      FieldName += ".field@slot+0";
    }
    return StorageValueFieldPrefixMatch{
        .FieldName = FieldName,
        .KeyConstraints = std::move(Base->KeyConstraints),
    };
  }
  if (Depth == 0) {
    return std::nullopt;
  }

  auto getOffsetPrefix =
      [&](llvm::Value *LHS,
          llvm::Value *RHS) -> std::optional<StorageValueFieldPrefixMatch> {
    if (auto Prefix = getStorageValueFieldPrefixFromSlotExpression(
            LHS, /*UseField0ForMappingBase=*/false, Depth - 1)) {
      if (auto RConst = getUInt64Constant(RHS)) {
        if (*RConst != 0) {
          Prefix->FieldName += ".field@slot+" + std::to_string(*RConst);
        }
        return Prefix;
      }
    }
    if (auto Prefix = getStorageValueFieldPrefixFromSlotExpression(
            RHS, /*UseField0ForMappingBase=*/false, Depth - 1)) {
      if (auto LConst = getUInt64Constant(LHS)) {
        if (*LConst != 0) {
          Prefix->FieldName += ".field@slot+" + std::to_string(*LConst);
        }
        return Prefix;
      }
    }
    return std::nullopt;
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(StorageSlot)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getOffsetPrefix(Add->getOperand(0), Add->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(StorageSlot)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getOffsetPrefix(Call->getArgOperand(0), Call->getArgOperand(1));
    }
  }

  return std::nullopt;
}

// Recognize Solidity's mstore(key, 0), mstore(baseSlot, 32), keccak(0, 64)
// mapping shape. Base slots are normalized to string prefixes; nested mappings
// work when the base slot is itself a matched mapping hash.
std::optional<MappingStorageFieldMatch>
getMappingStorageFieldMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                            uint64_t AccessKind) {
  auto Match = notdec::passes::evm::detail::matchStorageMappingAccess(
      Access, StorageSlot, AccessKind);
  if (!Match.has_value()) {
    return std::nullopt;
  }

  auto MatchBaseSlot = getStorageEvidenceValue(Match->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(MatchBaseSlot);
  if (!Base.has_value()) {
    return std::nullopt;
  }
  auto Prefix = Base->Prefix + ".map";
  Base->KeyConstraints.push_back(MappingStorageKeyConstraint{
      .FieldName = Prefix + ".key",
      .Key = Match->Key,
  });

  std::string ValueFieldName = Prefix + ".value";
  if (auto Offset =
          getConstantOffsetFromBase(StorageSlot, Match->Hash, /*Depth=*/2)) {
    if (*Offset != 0) {
      ValueFieldName += ".field@slot+" + std::to_string(*Offset);
    }
  }

  return MappingStorageFieldMatch{
      .Prefix = Prefix,
      .ValueFieldName = ValueFieldName,
      .KeyConstraints = std::move(Base->KeyConstraints),
  };
}

std::optional<StorageValueFieldPrefixMatch>
getStorageValueFieldPrefixMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                                uint64_t AccessKind,
                                bool UseField0ForMappingBase = false) {
  if (auto FieldName = getDirectStorageSlotFieldName(StorageSlot)) {
    return StorageValueFieldPrefixMatch{.FieldName = *FieldName};
  }

  if (auto FieldPrefix = getStorageValueFieldPrefixFromSlotExpression(
          StorageSlot, UseField0ForMappingBase)) {
    return FieldPrefix;
  }

  auto Mapping = getMappingStorageFieldMatch(Access, StorageSlot, AccessKind);
  if (!Mapping.has_value()) {
    return std::nullopt;
  }

  std::string FieldName = Mapping->ValueFieldName;
  if (UseField0ForMappingBase &&
      FieldName == Mapping->Prefix + ".value") {
    FieldName += ".field@slot+0";
  }

  return StorageValueFieldPrefixMatch{
      .FieldName = FieldName,
      .KeyConstraints = std::move(Mapping->KeyConstraints),
  };
}

ExtValuePtr getDynamicArrayIndex(llvm::Value *StorageSlot,
                                 llvm::Value *DataHash) {
  if (notdec::passes::evm::detail::isSameValue(StorageSlot, DataHash)) {
    return static_cast<llvm::Value *>(nullptr);
  }

  auto getIndexFromOperands = [&](llvm::Value *LHS,
                                  llvm::Value *RHS,
                                  llvm::User *User) -> ExtValuePtr {
    bool LHSIsBase = notdec::passes::evm::detail::dependsOnValue(LHS, DataHash);
    bool RHSIsBase = notdec::passes::evm::detail::dependsOnValue(RHS, DataHash);
    if (LHSIsBase == RHSIsBase) {
      return static_cast<llvm::Value *>(nullptr);
    }
    unsigned Index = LHSIsBase ? 1 : 0;
    return getExtValuePtr(LHSIsBase ? RHS : LHS, User, Index);
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(StorageSlot)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getIndexFromOperands(Add->getOperand(0), Add->getOperand(1), Add);
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(StorageSlot)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getIndexFromOperands(Call->getArgOperand(0), Call->getArgOperand(1),
                                  Call);
    }
  }

  return static_cast<llvm::Value *>(nullptr);
}

std::optional<ArrayStorageFieldMatch>
getArrayStorageFieldMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                          uint64_t AccessKind) {
  auto Match = notdec::passes::evm::detail::matchStorageArrayDataAccess(
      Access, StorageSlot, AccessKind);
  if (!Match.has_value()) {
    return std::nullopt;
  }

  auto MatchBaseSlot = getStorageEvidenceValue(Match->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(MatchBaseSlot);
  auto BaseValue = getStorageValueFieldPrefixFromSlotExpression(
      MatchBaseSlot, /*UseField0ForMappingBase=*/false);
  if (!Base.has_value() && !BaseValue.has_value()) {
    return std::nullopt;
  }

  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
  if (BaseValue.has_value()) {
    Prefix = BaseValue->FieldName;
    KeyConstraints = std::move(BaseValue->KeyConstraints);
  } else {
    Prefix = Base->Prefix;
    KeyConstraints = std::move(Base->KeyConstraints);
  }

  return ArrayStorageFieldMatch{
      .Prefix = Prefix + ".dynamic_array",
      .Index = getDynamicArrayIndex(StorageSlot, Match->DataHash),
      .KeyConstraints = std::move(KeyConstraints),
  };
}

bool functionHasArrayDataAccessForBaseSlot(llvm::Function *F,
                                           llvm::Value *BaseSlot) {
  if (F == nullptr) {
    return false;
  }

  for (llvm::BasicBlock &BB : *F) {
    for (llvm::Instruction &Inst : BB) {
      auto *Call = dyn_cast<llvm::CallBase>(&Inst);
      if (Call == nullptr) {
        continue;
      }

      llvm::Value *StorageSlot = nullptr;
      uint64_t AccessKind = 0;
      if (notdec::passes::evm::detail::isCallTo(Call, "evm_sload") &&
          Call->arg_size() == 1) {
        StorageSlot = Call->getArgOperand(0);
        AccessKind = 1;
      } else if (notdec::passes::evm::detail::isCallTo(Call, "evm_sstore") &&
                 Call->arg_size() == 2) {
        StorageSlot = Call->getArgOperand(0);
        AccessKind = 2;
      } else {
        continue;
      }

      auto Match = notdec::passes::evm::detail::matchStorageArrayDataAccess(
          *Call, StorageSlot, AccessKind);
      if (Match.has_value() &&
          notdec::passes::evm::detail::isSameValue(
              getStorageEvidenceValue(Match->BaseSlot), BaseSlot)) {
        return true;
      }
    }
  }
  return false;
}

std::optional<ArrayLengthStorageFieldMatch>
getArrayLengthStorageFieldMatch(llvm::CallBase &Load) {
  llvm::Value *BaseSlot = Load.getArgOperand(0);
  auto Base = getStorageFieldPrefixFromBaseSlot(BaseSlot);
  auto BaseValue = getStorageValueFieldPrefixFromSlotExpression(
      BaseSlot, /*UseField0ForMappingBase=*/false);
  if (!Base.has_value() && !BaseValue.has_value()) {
    return std::nullopt;
  }
  if (!functionHasArrayDataAccessForBaseSlot(Load.getFunction(), BaseSlot)) {
    return std::nullopt;
  }

  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
  if (BaseValue.has_value()) {
    Prefix = BaseValue->FieldName;
    KeyConstraints = std::move(BaseValue->KeyConstraints);
  } else {
    Prefix = Base->Prefix;
    KeyConstraints = std::move(Base->KeyConstraints);
  }

  return ArrayLengthStorageFieldMatch{
      .Prefix = Prefix + ".dynamic_array",
      .KeyConstraints = std::move(KeyConstraints),
  };
}

std::optional<StaticArrayStorageFieldMatch>
getStaticArrayStorageFieldMatch(llvm::Value *StorageSlot) {
  auto getMatchFromOperands =
      [](llvm::Value *LHS,
         llvm::Value *RHS,
         llvm::User *User) -> std::optional<StaticArrayStorageFieldMatch> {
    auto LSlot = getUInt64Constant(LHS);
    auto RSlot = getUInt64Constant(RHS);
    if (LSlot.has_value() == RSlot.has_value()) {
      return std::nullopt;
    }
    uint64_t BaseSlot = LSlot.has_value() ? *LSlot : *RSlot;
    llvm::Value *Index = LSlot.has_value() ? RHS : LHS;
    unsigned OperandIndex = LSlot.has_value() ? 1 : 0;
    return StaticArrayStorageFieldMatch{
        .Prefix = "slot:" + std::to_string(BaseSlot) + ".static_array",
        .Index = getExtValuePtr(Index, User, OperandIndex),
    };
  };

  if (auto *Add = dyn_cast<llvm::BinaryOperator>(StorageSlot)) {
    if (Add->getOpcode() == llvm::Instruction::Add) {
      return getMatchFromOperands(Add->getOperand(0), Add->getOperand(1), Add);
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(StorageSlot)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_add") &&
        Call->arg_size() == 2) {
      return getMatchFromOperands(Call->getArgOperand(0), Call->getArgOperand(1),
                                  Call);
    }
  }

  return std::nullopt;
}

std::optional<unsigned> getLowMaskBitWidth(llvm::Value *V) {
  auto *CI = dyn_cast_or_null<llvm::ConstantInt>(V);
  if (CI == nullptr) {
    return std::nullopt;
  }

  const llvm::APInt &Mask = CI->getValue();
  unsigned Width = Mask.getActiveBits();
  if (Width == 0 || Width >= Mask.getBitWidth()) {
    return std::nullopt;
  }
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    bool Expected = I < Width;
    if (Mask[I] != Expected) {
      return std::nullopt;
    }
  }
  return Width;
}

struct PackedBitRange {
  unsigned Offset = 0;
  unsigned Width = 0;
};

std::optional<PackedBitRange> getClearMaskBitRange(llvm::Value *V) {
  auto *CI = dyn_cast_or_null<llvm::ConstantInt>(V);
  if (CI == nullptr) {
    return std::nullopt;
  }

  const llvm::APInt &Mask = CI->getValue();
  if (Mask.isAllOnes() || Mask.isZero()) {
    return std::nullopt;
  }

  std::optional<unsigned> Start;
  unsigned End = 0;
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    if (!Mask[I]) {
      if (!Start.has_value()) {
        Start = I;
      }
      End = I + 1;
    } else if (Start.has_value()) {
      for (unsigned J = I + 1; J < Mask.getBitWidth(); ++J) {
        if (!Mask[J]) {
          return std::nullopt;
        }
      }
      break;
    }
  }

  if (!Start.has_value() || End <= *Start) {
    return std::nullopt;
  }
  return PackedBitRange{.Offset = *Start, .Width = End - *Start};
}

llvm::Value *getBitwiseAndOtherOperand(llvm::Value *V, unsigned &MaskWidth) {
  auto getOther = [&](llvm::Value *LHS, llvm::Value *RHS) -> llvm::Value * {
    if (auto Width = getLowMaskBitWidth(LHS)) {
      MaskWidth = *Width;
      return RHS;
    }
    if (auto Width = getLowMaskBitWidth(RHS)) {
      MaskWidth = *Width;
      return LHS;
    }
    return nullptr;
  };

  if (auto *And = dyn_cast<llvm::BinaryOperator>(V)) {
    if (And->getOpcode() == llvm::Instruction::And) {
      return getOther(And->getOperand(0), And->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_and") &&
        Call->arg_size() == 2) {
      return getOther(Call->getArgOperand(0), Call->getArgOperand(1));
    }
  }
  return nullptr;
}

llvm::Value *getBitwiseAndOtherOperandForClearMask(llvm::Value *V,
                                                   PackedBitRange &Range) {
  auto getOther = [&](llvm::Value *LHS, llvm::Value *RHS) -> llvm::Value * {
    if (auto Match = getClearMaskBitRange(LHS)) {
      Range = *Match;
      return RHS;
    }
    if (auto Match = getClearMaskBitRange(RHS)) {
      Range = *Match;
      return LHS;
    }
    return nullptr;
  };

  if (auto *And = dyn_cast<llvm::BinaryOperator>(V)) {
    if (And->getOpcode() == llvm::Instruction::And) {
      return getOther(And->getOperand(0), And->getOperand(1));
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_and") &&
        Call->arg_size() == 2) {
      return getOther(Call->getArgOperand(0), Call->getArgOperand(1));
    }
  }
  return nullptr;
}

bool getBitwiseOrOperands(llvm::Value *V, llvm::Value *&LHS,
                          ExtValuePtr &LHSEvidence, llvm::Value *&RHS,
                          ExtValuePtr &RHSEvidence) {
  if (auto *Or = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Or->getOpcode() == llvm::Instruction::Or) {
      LHS = Or->getOperand(0);
      LHSEvidence = getExtValuePtr(LHS, Or, 0);
      RHS = Or->getOperand(1);
      RHSEvidence = getExtValuePtr(RHS, Or, 1);
      return true;
    }
  }

  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_or") &&
        Call->arg_size() == 2) {
      LHS = Call->getArgOperand(0);
      LHSEvidence = getExtValuePtr(LHS, Call, 0);
      RHS = Call->getArgOperand(1);
      RHSEvidence = getExtValuePtr(RHS, Call, 1);
      return true;
    }
  }
  return false;
}

bool binaryOpHasSameValueOperand(llvm::BinaryOperator *Op, llvm::Value *V) {
  return Op != nullptr &&
         (notdec::passes::evm::detail::isSameValue(Op->getOperand(0), V) ||
          notdec::passes::evm::detail::isSameValue(Op->getOperand(1), V));
}

bool isBytesLengthFullLen(llvm::Value *V, llvm::Value *Slot) {
  if (auto *Call = dyn_cast_or_null<llvm::CallBase>(V)) {
    if (Call->arg_size() != 2) {
      return false;
    }
    bool IsDivByTwo =
        notdec::passes::evm::detail::isCallTo(Call, "evm_div") &&
        notdec::passes::evm::detail::isSameValue(Call->getArgOperand(0), Slot) &&
        notdec::passes::evm::detail::isConstantIntValue(Call->getArgOperand(1),
                                                        2);
    bool IsShrByOne =
        notdec::passes::evm::detail::isCallTo(Call, "evm_shr") &&
        notdec::passes::evm::detail::isConstantIntValue(Call->getArgOperand(0),
                                                        1) &&
        notdec::passes::evm::detail::isSameValue(Call->getArgOperand(1), Slot);
    return IsDivByTwo || IsShrByOne;
  }

  if (auto *Shift = dyn_cast_or_null<llvm::BinaryOperator>(V)) {
    return Shift->getOpcode() == llvm::Instruction::LShr &&
           notdec::passes::evm::detail::isSameValue(Shift->getOperand(0),
                                                    Slot) &&
           notdec::passes::evm::detail::isConstantIntValue(Shift->getOperand(1),
                                                           1);
  }
  return false;
}

bool isBytesLengthShortLen(llvm::Value *V, llvm::Value *FullLen) {
  auto *ShortLen = dyn_cast_or_null<llvm::BinaryOperator>(V);
  return ShortLen != nullptr && ShortLen->getOpcode() == llvm::Instruction::And &&
         binaryOpHasSameValueOperand(ShortLen, FullLen) &&
         (notdec::passes::evm::detail::isConstantIntValue(
              ShortLen->getOperand(0), 127) ||
          notdec::passes::evm::detail::isConstantIntValue(
              ShortLen->getOperand(1), 127));
}

bool matchStorageBytesLengthValue(llvm::Value *Length, llvm::Value *Slot) {
  auto *Select = dyn_cast_or_null<llvm::SelectInst>(Length);
  if (Select == nullptr) {
    return false;
  }

  auto *ShortCond = dyn_cast<llvm::ICmpInst>(Select->getCondition());
  if (ShortCond == nullptr ||
      ShortCond->getPredicate() != llvm::ICmpInst::ICMP_EQ ||
      !notdec::passes::evm::detail::isConstantIntValue(
          ShortCond->getOperand(1), 0)) {
    return false;
  }

  auto *LowBit = dyn_cast<llvm::BinaryOperator>(ShortCond->getOperand(0));
  if (LowBit == nullptr || LowBit->getOpcode() != llvm::Instruction::And ||
      !binaryOpHasSameValueOperand(LowBit, Slot) ||
      !(notdec::passes::evm::detail::isConstantIntValue(LowBit->getOperand(0),
                                                        1) ||
        notdec::passes::evm::detail::isConstantIntValue(LowBit->getOperand(1),
                                                        1))) {
    return false;
  }

  llvm::Value *ShortLen = Select->getTrueValue();
  llvm::Value *FullLen = Select->getFalseValue();
  return isBytesLengthFullLen(FullLen, Slot) &&
         isBytesLengthShortLen(ShortLen, FullLen);
}

llvm::Value *getShiftedValue(llvm::Value *V, unsigned &Offset) {
  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_shr") &&
        Call->arg_size() == 2) {
      auto Shift = getUInt64Constant(Call->getArgOperand(0));
      if (Shift.has_value() && *Shift < 256) {
        Offset = static_cast<unsigned>(*Shift);
        return Call->getArgOperand(1);
      }
    }
  }

  if (auto *Shift = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Shift->getOpcode() == llvm::Instruction::LShr) {
      auto Amount = getUInt64Constant(Shift->getOperand(1));
      if (Amount.has_value() && *Amount < 256) {
        Offset = static_cast<unsigned>(*Amount);
        return Shift->getOperand(0);
      }
    }
  }

  Offset = 0;
  return V;
}

ExtValuePtr getShiftedWriteValue(llvm::Value *V, const ExtValuePtr &Evidence,
                                 unsigned &Offset) {
  if (auto *Call = dyn_cast<llvm::CallBase>(V)) {
    if (notdec::passes::evm::detail::isCallTo(Call, "evm_shl") &&
        Call->arg_size() == 2) {
      auto Shift = getUInt64Constant(Call->getArgOperand(0));
      if (Shift.has_value() && *Shift < 256) {
        Offset = static_cast<unsigned>(*Shift);
        return getExtValuePtr(Call->getArgOperand(1), Call, 1);
      }
    }
  }

  if (auto *Shift = dyn_cast<llvm::BinaryOperator>(V)) {
    if (Shift->getOpcode() == llvm::Instruction::Shl) {
      auto Amount = getUInt64Constant(Shift->getOperand(1));
      if (Amount.has_value() && *Amount < 256) {
        Offset = static_cast<unsigned>(*Amount);
        return getExtValuePtr(Shift->getOperand(0), Shift, 0);
      }
    }
  }

  Offset = 0;
  return Evidence;
}

std::optional<PackedStorageFieldReadMatch>
getPackedReadFromExtractedValue(llvm::CallBase &Load, llvm::Value *Extracted,
                                const StorageValueFieldPrefixMatch &FieldPrefix) {
  unsigned MaskWidth = 0;
  llvm::Value *Masked = getBitwiseAndOtherOperand(Extracted, MaskWidth);
  if (Masked == nullptr) {
    return std::nullopt;
  }

  unsigned Offset = 0;
  llvm::Value *Shifted = getShiftedValue(Masked, Offset);
  if (!notdec::passes::evm::detail::isSameValue(Shifted, &Load)) {
    return std::nullopt;
  }

  return PackedStorageFieldReadMatch{
      .FieldName = FieldPrefix.FieldName + ".packed@" +
                   std::to_string(Offset) + ":" + std::to_string(MaskWidth),
      .Value = Extracted,
      .KeyConstraints = FieldPrefix.KeyConstraints,
  };
}

std::optional<PackedStorageFieldReadMatch>
getPackedStorageFieldReadMatch(llvm::CallBase &Load) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Load, Load.getArgOperand(0), /*AccessKind=*/1,
      /*UseField0ForMappingBase=*/true);
  if (!FieldPrefix.has_value()) {
    return std::nullopt;
  }

  for (llvm::User *User : Load.users()) {
    auto *DirectValue = dyn_cast<llvm::Value>(User);
    if (auto Match =
            getPackedReadFromExtractedValue(Load, DirectValue, *FieldPrefix)) {
      return Match;
    }

    for (llvm::User *NestedUser : User->users()) {
      auto *NestedValue = dyn_cast<llvm::Value>(NestedUser);
      if (auto Match =
              getPackedReadFromExtractedValue(Load, NestedValue, *FieldPrefix)) {
        return Match;
      }
    }
  }
  return std::nullopt;
}

std::optional<BytesStorageLengthMatch>
getBytesStorageLengthMatch(llvm::CallBase &Load) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Load, Load.getArgOperand(0), /*AccessKind=*/1);

  if (!FieldPrefix.has_value()) {
    if (auto Array = getArrayStorageFieldMatch(Load, Load.getArgOperand(0),
                                               /*AccessKind=*/1)) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = Array->Prefix + ".elem",
          .KeyConstraints = std::move(Array->KeyConstraints),
      };
      if (getStorageEvidenceValue(Array->Index) != nullptr) {
        FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
            .FieldName = Array->Prefix + ".index",
            .Key = Array->Index,
        });
      }
    } else if (auto StaticArray =
                   getStaticArrayStorageFieldMatch(Load.getArgOperand(0))) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = StaticArray->Prefix + ".elem",
      };
      FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
          .FieldName = StaticArray->Prefix + ".index",
          .Key = StaticArray->Index,
      });
    }
  }

  if (!FieldPrefix.has_value()) {
    return std::nullopt;
  }

  llvm::SmallVector<llvm::Value *, 8> Worklist;
  llvm::SmallPtrSet<llvm::Value *, 16> Seen;
  for (llvm::User *User : Load.users()) {
    if (auto *V = dyn_cast<llvm::Value>(User)) {
      Worklist.push_back(V);
    }
  }

  unsigned Steps = 0;
  while (!Worklist.empty() && Steps++ < 32) {
    llvm::Value *V = Worklist.pop_back_val();
    if (V == nullptr || !Seen.insert(V).second) {
      continue;
    }
    if (matchStorageBytesLengthValue(V, &Load)) {
      return BytesStorageLengthMatch{
          .FieldName = FieldPrefix->FieldName + ".bytes.length",
          .Length = V,
          .KeyConstraints = std::move(FieldPrefix->KeyConstraints),
      };
    }
    for (llvm::User *User : V->users()) {
      if (auto *UserValue = dyn_cast<llvm::Value>(User)) {
        Worklist.push_back(UserValue);
      }
    }
  }
  return std::nullopt;
}

bool functionHasBytesLengthDecodeForBaseSlot(llvm::Function *F,
                                             llvm::Value *BaseSlot);

bool isBytesShortDataMask(llvm::Value *V) {
  auto *CI = dyn_cast_or_null<llvm::ConstantInt>(V);
  if (CI == nullptr) {
    return false;
  }

  const llvm::APInt &Mask = CI->getValue();
  if (Mask.getBitWidth() <= 8) {
    return false;
  }
  for (unsigned I = 0; I < Mask.getBitWidth(); ++I) {
    bool Expected = I >= 8;
    if (Mask[I] != Expected) {
      return false;
    }
  }
  return true;
}

bool matchStorageBytesShortDataValue(llvm::Value *Data, llvm::Value *Slot) {
  auto *And = dyn_cast_or_null<llvm::BinaryOperator>(Data);
  if (And == nullptr || And->getOpcode() != llvm::Instruction::And) {
    return false;
  }

  return (notdec::passes::evm::detail::isSameValue(And->getOperand(0), Slot) &&
          isBytesShortDataMask(And->getOperand(1))) ||
         (notdec::passes::evm::detail::isSameValue(And->getOperand(1), Slot) &&
          isBytesShortDataMask(And->getOperand(0)));
}

std::optional<BytesStorageShortDataMatch>
getBytesStorageShortDataMatch(llvm::CallBase &Load) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Load, Load.getArgOperand(0), /*AccessKind=*/1);

  if (!FieldPrefix.has_value()) {
    if (auto Array = getArrayStorageFieldMatch(Load, Load.getArgOperand(0),
                                               /*AccessKind=*/1)) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = Array->Prefix + ".elem",
          .KeyConstraints = std::move(Array->KeyConstraints),
      };
      if (getStorageEvidenceValue(Array->Index) != nullptr) {
        FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
            .FieldName = Array->Prefix + ".index",
            .Key = Array->Index,
        });
      }
    } else if (auto StaticArray =
                   getStaticArrayStorageFieldMatch(Load.getArgOperand(0))) {
      FieldPrefix = StorageValueFieldPrefixMatch{
          .FieldName = StaticArray->Prefix + ".elem",
      };
      FieldPrefix->KeyConstraints.push_back(MappingStorageKeyConstraint{
          .FieldName = StaticArray->Prefix + ".index",
          .Key = StaticArray->Index,
      });
    }
  }

  if (!FieldPrefix.has_value() ||
      !functionHasBytesLengthDecodeForBaseSlot(Load.getFunction(),
                                               Load.getArgOperand(0))) {
    return std::nullopt;
  }

  llvm::SmallVector<llvm::Value *, 8> Worklist;
  llvm::SmallPtrSet<llvm::Value *, 16> Seen;
  for (llvm::User *User : Load.users()) {
    if (auto *V = dyn_cast<llvm::Value>(User)) {
      Worklist.push_back(V);
    }
  }

  unsigned Steps = 0;
  while (!Worklist.empty() && Steps++ < 32) {
    llvm::Value *V = Worklist.pop_back_val();
    if (V == nullptr || !Seen.insert(V).second) {
      continue;
    }
    if (matchStorageBytesShortDataValue(V, &Load)) {
      return BytesStorageShortDataMatch{
          .FieldName = FieldPrefix->FieldName + ".bytes.short_data",
          .Data = V,
          .KeyConstraints = std::move(FieldPrefix->KeyConstraints),
      };
    }
    for (llvm::User *User : V->users()) {
      if (auto *UserValue = dyn_cast<llvm::Value>(User)) {
        Worklist.push_back(UserValue);
      }
    }
  }
  return std::nullopt;
}

bool functionHasBytesLengthDecodeForBaseSlot(llvm::Function *F,
                                             llvm::Value *BaseSlot) {
  if (F == nullptr) {
    return false;
  }

  for (llvm::BasicBlock &BB : *F) {
    for (llvm::Instruction &Inst : BB) {
      auto *Load = dyn_cast<llvm::CallBase>(&Inst);
      if (Load == nullptr ||
          !notdec::passes::evm::detail::isCallTo(Load, "evm_sload") ||
          Load->arg_size() != 1 ||
          !notdec::passes::evm::detail::isSameValue(Load->getArgOperand(0),
                                                    BaseSlot)) {
        continue;
      }

      llvm::SmallVector<llvm::Value *, 8> Worklist;
      llvm::SmallPtrSet<llvm::Value *, 16> Seen;
      for (llvm::User *User : Load->users()) {
        if (auto *V = dyn_cast<llvm::Value>(User)) {
          Worklist.push_back(V);
        }
      }

      unsigned Steps = 0;
      while (!Worklist.empty() && Steps++ < 32) {
        llvm::Value *V = Worklist.pop_back_val();
        if (V == nullptr || !Seen.insert(V).second) {
          continue;
        }
        if (matchStorageBytesLengthValue(V, Load)) {
          return true;
        }
        for (llvm::User *User : V->users()) {
          if (auto *UserValue = dyn_cast<llvm::Value>(User)) {
            Worklist.push_back(UserValue);
          }
        }
      }
    }
  }
  return false;
}

std::optional<BytesStorageLongElemMatch>
getBytesStorageLongElemMatch(llvm::CallBase &Access, llvm::Value *StorageSlot,
                             uint64_t AccessKind) {
  auto Match = notdec::passes::evm::detail::matchStorageArrayDataAccess(
      Access, StorageSlot, AccessKind);
  if (!Match.has_value() ||
      !functionHasBytesLengthDecodeForBaseSlot(Access.getFunction(),
                                               getStorageEvidenceValue(
                                                   Match->BaseSlot))) {
    return std::nullopt;
  }

  auto MatchBaseSlot = getStorageEvidenceValue(Match->BaseSlot);
  auto Base = getStorageFieldPrefixFromBaseSlot(MatchBaseSlot);
  auto BaseValue = getStorageValueFieldPrefixFromSlotExpression(
      MatchBaseSlot, /*UseField0ForMappingBase=*/false);
  if (!Base.has_value() && !BaseValue.has_value()) {
    return std::nullopt;
  }

  std::string Prefix;
  std::vector<MappingStorageKeyConstraint> KeyConstraints;
  if (BaseValue.has_value()) {
    Prefix = BaseValue->FieldName;
    KeyConstraints = std::move(BaseValue->KeyConstraints);
  } else {
    Prefix = Base->Prefix;
    KeyConstraints = std::move(Base->KeyConstraints);
  }

  return BytesStorageLongElemMatch{
      .Prefix = Prefix + ".bytes",
      .Index = getDynamicArrayIndex(StorageSlot, Match->DataHash),
      .KeyConstraints = std::move(KeyConstraints),
  };
}

llvm::CallBase *getSameSlotSLoad(llvm::Value *V, llvm::Value *StorageSlot) {
  auto *Load = dyn_cast_or_null<llvm::CallBase>(V);
  if (Load == nullptr ||
      !notdec::passes::evm::detail::isCallTo(Load, "evm_sload") ||
      Load->arg_size() != 1) {
    return nullptr;
  }
  if (!notdec::passes::evm::detail::isSameValue(Load->getArgOperand(0),
                                                StorageSlot)) {
    return nullptr;
  }
  return Load;
}

std::optional<PackedStorageFieldWriteMatch>
getPackedStorageFieldWriteMatch(llvm::Value *StorageSlot,
                                llvm::Value *StoredValue,
                                llvm::CallBase &Store) {
  auto FieldPrefix = getStorageValueFieldPrefixMatch(
      Store, StorageSlot, /*AccessKind=*/2,
      /*UseField0ForMappingBase=*/true);
  if (!FieldPrefix.has_value()) {
    return std::nullopt;
  }

  llvm::Value *OrLHS = nullptr;
  llvm::Value *OrRHS = nullptr;
  ExtValuePtr OrLHSEvidence = static_cast<llvm::Value *>(nullptr);
  ExtValuePtr OrRHSEvidence = static_cast<llvm::Value *>(nullptr);
  if (!getBitwiseOrOperands(StoredValue, OrLHS, OrLHSEvidence, OrRHS,
                            OrRHSEvidence)) {
    return std::nullopt;
  }

  auto tryMatch = [&](llvm::Value *ClearPart, llvm::Value *WritePart,
                      const ExtValuePtr &WritePartEvidence)
      -> std::optional<PackedStorageFieldWriteMatch> {
    PackedBitRange Range;
    llvm::Value *OldValue =
        getBitwiseAndOtherOperandForClearMask(ClearPart, Range);
    if (getSameSlotSLoad(OldValue, StorageSlot) == nullptr) {
      return std::nullopt;
    }

    unsigned Offset = 0;
    ExtValuePtr Value = getShiftedWriteValue(WritePart, WritePartEvidence,
                                             Offset);
    if (Offset != Range.Offset || Range.Width == 0) {
      return std::nullopt;
    }

    return PackedStorageFieldWriteMatch{
        .FieldName = FieldPrefix->FieldName + ".packed@" +
                     std::to_string(Range.Offset) + ":" +
                     std::to_string(Range.Width),
        .Value = Value,
        .KeyConstraints = FieldPrefix->KeyConstraints,
    };
  };

  if (auto Match = tryMatch(OrLHS, OrRHS, OrRHSEvidence)) {
    return Match;
  }
  return tryMatch(OrRHS, OrLHS, OrLHSEvidence);
}

bool isPackedWritePreservedWordLoad(llvm::CallBase &Load) {
  if (!notdec::passes::evm::detail::isCallTo(&Load, "evm_sload") ||
      Load.arg_size() != 1) {
    return false;
  }

  for (llvm::User *AndUser : Load.users()) {
    PackedBitRange Range;
    if (getBitwiseAndOtherOperandForClearMask(dyn_cast<llvm::Value>(AndUser),
                                              Range) == nullptr) {
      continue;
    }
    for (llvm::User *OrUser : AndUser->users()) {
      auto *StoredValue = dyn_cast<llvm::Value>(OrUser);
      if (StoredValue == nullptr) {
        continue;
      }
      for (llvm::User *StoreUser : OrUser->users()) {
        auto *Store = dyn_cast<llvm::CallBase>(StoreUser);
        if (Store == nullptr ||
            !notdec::passes::evm::detail::isCallTo(Store, "evm_sstore") ||
            Store->arg_size() != 2 ||
            !notdec::passes::evm::detail::isSameValue(Store->getArgOperand(0),
                                                      Load.getArgOperand(0)) ||
            !notdec::passes::evm::detail::isSameValue(Store->getArgOperand(1),
                                                      StoredValue)) {
          continue;
        }
        if (getPackedStorageFieldWriteMatch(Store->getArgOperand(0),
                                            Store->getArgOperand(1), *Store)) {
          return true;
        }
      }
    }
  }
  return false;
}

void ConstraintsGenerator::MLsubVisitor::addEVMRuntimeSemanticConstraints(
    llvm::CallBase &I) {
  auto *F = I.getCalledFunction();
  if (F == nullptr) {
    return;
  }

  auto markArg = [&](unsigned Index, llvm::StringRef SemanticName) {
    if (Index < I.arg_size() &&
        I.getArgOperand(Index)->getType()->isIntegerTy(256)) {
      addEVMSemanticConstraint(
          cg, getExtValuePtr(I.getArgOperand(Index), &I, Index), SemanticName);
    }
  };
  auto markRet = [&](llvm::StringRef SemanticName) {
    if (!I.getType()->isVoidTy() && I.getType()->isIntegerTy(256)) {
      addEVMSemanticConstraint(cg, &I, SemanticName);
    }
  };

  StringRef Name = F->getName();
  if (Name == "evm_address" || Name == "evm_caller" ||
      Name == "evm_origin" || Name == "evm_coinbase") {
    markRet("address");
    return;
  }
  if (Name == "evm_create" || Name == "evm_create2") {
    markRet("address");
    markArg(0, "integer");
    markArg(2, "integer");
    if (Name == "evm_create2") {
      markArg(3, "integer");
    }
    return;
  }
  if (Name == "evm_balance" || Name == "evm_extcodesize" ||
      Name == "evm_extcodehash") {
    markArg(0, "address");
    markRet("integer");
    return;
  }
  if (Name == "evm_extcodecopy") {
    markArg(1, "address");
    markArg(3, "integer");
    markArg(4, "integer");
    return;
  }
  if (Name == "evm_sload") {
    markArg(0, "storage_key");
    if (I.arg_size() == 1 && !I.getType()->isVoidTy()) {
      if (auto BytesLength = getBytesStorageLengthMatch(I)) {
        addStorageKeyConstraints(cg, BytesLength->KeyConstraints);

        auto FieldTy = cg.getOrCreateStorageField(BytesLength->FieldName, 256);
        if (auto LengthTy = getStorageEvidenceType(cg, BytesLength->Length)) {
          cg.addSubtype(FieldTy, *LengthTy);
        }

        if (auto ShortData = getBytesStorageShortDataMatch(I)) {
          addStorageKeyConstraints(cg, ShortData->KeyConstraints);

          auto ShortDataFieldTy =
              cg.getOrCreateStorageField(ShortData->FieldName, 256);
          if (auto DataTy = getStorageEvidenceType(cg, ShortData->Data)) {
            cg.addSubtype(ShortDataFieldTy, *DataTy);
          }
        }
      } else if (auto ArrayLength = getArrayLengthStorageFieldMatch(I)) {
        addStorageKeyConstraints(cg, ArrayLength->KeyConstraints);

        auto FieldTy =
            cg.getOrCreateStorageField(ArrayLength->Prefix + ".length");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (isPackedWritePreservedWordLoad(I)) {
        return;
      } else if (auto BytesLong =
                     getBytesStorageLongElemMatch(I, I.getArgOperand(0),
                                                  /*AccessKind=*/1)) {
        addStorageKeyConstraints(cg, BytesLong->KeyConstraints);
        if (getStorageEvidenceValue(BytesLong->Index) != nullptr) {
          addStorageValueEvidenceConstraint(
              cg, BytesLong->Prefix + ".long_index", BytesLong->Index);
        }

        auto FieldTy =
            cg.getOrCreateStorageField(BytesLong->Prefix + ".long_elem");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (auto Mapping =
                     getMappingStorageFieldMatch(I, I.getArgOperand(0),
                                                 /*AccessKind=*/1)) {
        if (auto Packed = getPackedStorageFieldReadMatch(I)) {
          addStorageKeyConstraints(cg, Packed->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
          if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
            cg.addSubtype(FieldTy, *ValueTy);
          }
        } else {
          addStorageKeyConstraints(cg, Mapping->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Mapping->ValueFieldName);
          auto ResultTy = cg.getOrInsertNode(&I);
          cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
        }
      } else if (auto Array =
                     getArrayStorageFieldMatch(I, I.getArgOperand(0),
                                               /*AccessKind=*/1)) {
        addStorageKeyConstraints(cg, Array->KeyConstraints);
        if (getStorageEvidenceValue(Array->Index) != nullptr) {
          addStorageValueEvidenceConstraint(cg, Array->Prefix + ".index",
                                            Array->Index);
        }

        auto FieldTy = cg.getOrCreateStorageField(Array->Prefix + ".elem");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (auto StaticArray =
                     getStaticArrayStorageFieldMatch(I.getArgOperand(0))) {
        addStorageValueEvidenceConstraint(cg, StaticArray->Prefix + ".index",
                                          StaticArray->Index);

        auto FieldTy =
            cg.getOrCreateStorageField(StaticArray->Prefix + ".elem");
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      } else if (auto Packed = getPackedStorageFieldReadMatch(I)) {
        auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
        if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
          cg.addSubtype(FieldTy, *ValueTy);
        }
      } else if (auto FieldName =
                     getDirectStorageSlotFieldName(I.getArgOperand(0))) {
        auto FieldTy = cg.getOrCreateStorageField(*FieldName);
        auto ResultTy = cg.getOrInsertNode(&I);
        cg.addSubtype(FieldTy, binarysub::make_ptr_load(ResultTy, 256));
      }
    }
    return;
  }
  if (Name == "evm_sstore") {
    markArg(0, "storage_key");
    if (I.arg_size() == 2) {
      if (auto Mapping = getMappingStorageFieldMatch(I, I.getArgOperand(0),
                                                     /*AccessKind=*/2)) {
        if (auto Packed = getPackedStorageFieldWriteMatch(
                I.getArgOperand(0), I.getArgOperand(1), I)) {
          addStorageKeyConstraints(cg, Packed->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
          if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
            cg.addSubtype(*ValueTy, FieldTy);
          }
        } else {
          addStorageKeyConstraints(cg, Mapping->KeyConstraints);

          auto FieldTy = cg.getOrCreateStorageField(Mapping->ValueFieldName);
          auto ValueTy =
              cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
          cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
        }
      } else if (auto BytesLong =
                     getBytesStorageLongElemMatch(I, I.getArgOperand(0),
                                                  /*AccessKind=*/2)) {
        addStorageKeyConstraints(cg, BytesLong->KeyConstraints);
        if (getStorageEvidenceValue(BytesLong->Index) != nullptr) {
          addStorageValueEvidenceConstraint(
              cg, BytesLong->Prefix + ".long_index", BytesLong->Index);
        }

        auto FieldTy =
            cg.getOrCreateStorageField(BytesLong->Prefix + ".long_elem");
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      } else if (auto Array =
                     getArrayStorageFieldMatch(I, I.getArgOperand(0),
                                               /*AccessKind=*/2)) {
        addStorageKeyConstraints(cg, Array->KeyConstraints);
        if (getStorageEvidenceValue(Array->Index) != nullptr) {
          addStorageValueEvidenceConstraint(cg, Array->Prefix + ".index",
                                            Array->Index);
        }

        auto FieldTy = cg.getOrCreateStorageField(Array->Prefix + ".elem");
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      } else if (auto StaticArray =
                     getStaticArrayStorageFieldMatch(I.getArgOperand(0))) {
        addStorageValueEvidenceConstraint(cg, StaticArray->Prefix + ".index",
                                          StaticArray->Index);

        auto FieldTy =
            cg.getOrCreateStorageField(StaticArray->Prefix + ".elem");
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      } else if (auto Packed = getPackedStorageFieldWriteMatch(
                     I.getArgOperand(0), I.getArgOperand(1), I)) {
        addStorageKeyConstraints(cg, Packed->KeyConstraints);

        auto FieldTy = cg.getOrCreateStorageField(Packed->FieldName, 256);
        if (auto ValueTy = getStorageEvidenceType(cg, Packed->Value)) {
          cg.addSubtype(*ValueTy, FieldTy);
        }
      } else if (auto FieldName =
                     getDirectStorageSlotFieldName(I.getArgOperand(0))) {
        auto FieldTy = cg.getOrCreateStorageField(*FieldName);
        auto ValueTy =
            cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(1), &I, 1));
        cg.addSubtype(FieldTy, binarysub::make_ptr_store(ValueTy, 256));
      }
    }
    return;
  }
  if (Name == "evm_call" || Name == "evm_callcode") {
    markArg(3, "integer");
    markArg(4, "address");
    markArg(5, "integer");
    markArg(7, "integer");
    markArg(9, "integer");
    markRet("integer");
    return;
  }
  if (Name == "evm_delegatecall" || Name == "evm_staticcall") {
    markArg(3, "integer");
    markArg(4, "address");
    markArg(6, "integer");
    markArg(8, "integer");
    markRet("integer");
    return;
  }
}

bool ConstraintsGenerator::MLsubVisitor::handleIntrinsicCall(
    llvm::CallBase &I) {
  auto Target = I.getCalledFunction();
  if (!Target->isIntrinsic()) {
    return false;
  }
  auto ID = Target->getIntrinsicID();
  switch (ID) {
  case Intrinsic::memset:
  case Intrinsic::memcpy:
  case Intrinsic::memmove:
    return true;
  default:
    break;
  }
  if (ID == Intrinsic::assume) {
    return true;
  }
  if (isMinMaxIntrinsic(ID)) {
    assert(I.arg_size() == 2 && "min/max intrinsic must have two operands");
    auto RetVar = cg.getOrInsertNode(&I);
    auto Src1 = getExtValuePtr(I.getArgOperand(0), &I, 0);
    auto Src2 = getExtValuePtr(I.getArgOperand(1), &I, 1);
    auto Src1Var = cg.getOrInsertNode(Src1);
    auto Src2Var = cg.getOrInsertNode(Src2);
    // LLVM 22 folds integer icmp+select into min/max intrinsics. Keep the
    // old frozen-IR behavior by modeling them as a select-like join.
    cg.setNonPointer(Src1);
    cg.setNonPointer(Src2);
    cg.setNonPointer(&I);
    cg.addSubtype(Src1Var, RetVar);
    cg.addSubtype(Src2Var, RetVar);
    return true;
  }
  // auto ID = Target->getIntrinsicID();
  if (I.getType()->isAggregateType()) {
    // ignore this call and handle the value in visitExtractValue
    return true;
  }
  return false;
}

void ConstraintsGenerator::MLsubVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target) {
    if (Target->getName().starts_with("llvm.dbg") ||
        Target->getName().starts_with("llvm.lifetime")) {
      return;
    }
  }

  if (Target == nullptr) {
    // TODO indirect call
    std::cerr
        << __FILE__ << ":" << __LINE__ << ": "
        << "Warn: ConstraintsGenerator: indirect call not supported yet\n";
    return;
  }

  // TODO if is allocation function, treat as alloca inst
  if (isHeapAllocationCall(I)) {
    auto Node = cg.createNode(&I);
    // set as pointer type
    cg.setPointer(&I);
    cg.ContraVariantValues.insert(&I);
    cg.addAddressOf(&I, cg.getRootMemoryObject(HeapObject{.Allocator = &I}));
  } else if (handleEVMMarkerCall(I)) {
    return;
  } else if (shouldIgnoreRuntimeCall(I)) {
    addEVMRuntimeSemanticConstraints(I);
    return;
  } else if (handleIntrinsicCall(I)) {
    return;
  } else {
    // Call within the SCC:
    auto Func = Target;
    std::vector<SimpleType> Args;
    for (unsigned i = 0; i < I.arg_size(); ++i) {
      auto ValVar = cg.getOrInsertNode(getExtValuePtr(I.getArgOperand(i), &I, i));
      Args.push_back(ValVar);
    }
    SimpleType Ret = nullptr;
    if (!I.getType()->isVoidTy()) {
      if (I.getType()->isAggregateType()) {
        Ret = cg.makeCallAggregateReturnRecord(I);
      } else {
        Ret = cg.getOrInsertNode(&I);
      }
    }
    auto ActualFunc = binarysub::make_function(Args, Ret);
    if (cg.SCCs.count(Target)) {
      auto F = cg.getNodeOrNull(Func);
      cg.addSubtype(F, ActualFunc);
    } else {
      // create and save to CallToInstance map. instance with summary later
      auto It = cg.unhandledCalls.insert({&I, ActualFunc});
      assert(It.second && "Insert unhandledCalls failed!");
    }
  }
}

void ConstraintsGenerator::MLsubVisitor::visitReturnInst(ReturnInst &I) {
  auto *SrcVal = I.getReturnValue();
  if (SrcVal == nullptr) { // ret void.
    return;
  }
  if (SrcVal->getType()->isAggregateType()) {
    cg.addAggregateReturnConstraints(SrcVal, I);
    return;
  }
  auto Src = cg.getOrInsertNode(getExtValuePtr(SrcVal, &I, 0));
  auto Dst = cg.getNodeOrNull(ReturnValue{.Func = I.getFunction()});
  // src is a subtype of dest
  cg.addSubtype(Src, Dst);
}

void ConstraintsGenerator::MLsubVisitor::visitPHINode(PHINode &I) {
  cg.createNode(&I);
  // Defer constraints generation (and unification) to handlePHINodes
  phiNodes.push_back(&I);
}

void ConstraintsGenerator::MLsubVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto P = cg.getNodeOrNull(I);
    for (long i = 0; i < I->getNumIncomingValues(); i++) {
      auto *Src = I->getIncomingValue(i);
      auto SrcVar = cg.getOrInsertNode(getExtValuePtr(Src, I, i));
      cg.addSubtype(SrcVar, P);
      cg.addPointerCopy(I, getExtValuePtr(Src, I, i));
    }
  }
}

unsigned ConstraintsGenerator::getLLVMTypeSize(Type *ElemTy) {
  return llvm2c::getLLVMTypeSize(ElemTy, PointerSize);
}

void ConstraintsGenerator::MLsubVisitor::visitLoadInst(LoadInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 0));
  if (!Node) {
    if (lowerConstantExprGEPAddress(cg, I.getPointerOperand(), I, 0,
                                    I.getModule()->getDataLayout())) {
      Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 0));
    }
  }
  if (!Node) {
    if (auto CE = dyn_cast<ConstantExpr>(I.getPointerOperand())) {
      if (CE->getOpcode() == Instruction::BitCast) {
        if (auto CE2 = dyn_cast<ConstantExpr>(CE->getOperand(0))) {
          CE = CE2;
        }
      }
      if (CE->getOpcode() == Instruction::GetElementPtr) {
        return;
      }
    }
  }

  auto PtrVal = cg.getOrInsertNode(getExtValuePtr(I.getPointerOperand(), &I, 0));
  auto RetVal = cg.getOrInsertNode(&I);
  auto BitSize = cg.getLLVMTypeSize(I.getType());
  auto Addr = getExtValuePtr(I.getPointerOperand(), &I, 0);

  cg.recordLoad(Addr, RetVal, BitSize, &I);
  if (I.getModule() != nullptr && isEVMModule(*I.getModule())) {
    cg.addEVMConstantMemoryField(Addr, RetVal);
  }
  if (cg.isPointerAnalysisEnabled() && I.getType()->isPointerTy()) {
    cg.PA.addLoadPtr(&I, Addr);
  }
  if (!cg.isPointerAnalysisReplacingBinarysubMemory()) {
    cg.addSubtype(PtrVal, binarysub::make_ptr_load(RetVal, BitSize));
  }
}

void ConstraintsGenerator::MLsubVisitor::visitStoreInst(StoreInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 1));
  if (!Node) {
    if (lowerConstantExprGEPAddress(cg, I.getPointerOperand(), I, 1,
                                    I.getModule()->getDataLayout())) {
      Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 1));
    }
  }
  if (!Node) {
    if (auto CE = dyn_cast<ConstantExpr>(I.getPointerOperand())) {
      if (CE->getOpcode() == Instruction::BitCast) {
        if (auto CE2 = dyn_cast<ConstantExpr>(CE->getOperand(0))) {
          CE = CE2;
        }
      }
      if (CE->getOpcode() == Instruction::GetElementPtr) {
        return;
      }
    }
  }

  auto PtrVal = cg.getOrInsertNode(getExtValuePtr(I.getPointerOperand(), &I, 1));
  auto BitSize = cg.getLLVMTypeSize(I.getValueOperand()->getType());
  auto StoreVal = cg.getOrInsertNode(getExtValuePtr(I.getValueOperand(), &I, 0));
  auto Addr = getExtValuePtr(I.getPointerOperand(), &I, 1);
  auto Value = getExtValuePtr(I.getValueOperand(), &I, 0);

  cg.recordStore(Addr, StoreVal, BitSize, &I);
  if (I.getModule() != nullptr && isEVMModule(*I.getModule())) {
    cg.addEVMConstantMemoryField(Addr, StoreVal);
  }
  if (cg.isPointerAnalysisEnabled() &&
      I.getValueOperand()->getType()->isPointerTy()) {
    cg.PA.addStorePtr(Addr, Value);
  }
  if (!cg.isPointerAnalysisReplacingBinarysubMemory()) {
    cg.addSubtype(PtrVal, binarysub::make_ptr_store(StoreVal, BitSize));
  }
}

void ConstraintsGenerator::MLsubVisitor::visitAllocaInst(AllocaInst &I) {
  auto Node = cg.createNode(&I);
  // set as pointer type
  cg.setPointer(&I);
  cg.ContraVariantValues.insert(&I);
  cg.addAddressOf(&I, cg.getRootMemoryObject(StackObject{.Allocator = &I}));
}

void ConstraintsGenerator::MLsubVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  // supress warnings for table gep
  if (Gep.getPointerOperand()->getName().starts_with("table_")) {
    return;
  }

  if (lowerGEPOperatorAsPtrAdd(cg, cast<GEPOperator>(Gep), Gep,
                               Gep.getModule()->getDataLayout(), &Gep)) {
    return;
  }

  std::cerr << "Warning: MLsubVisitor::visitGetElementPtrInst: "
               "skip unsupported GEP offset lowering: "
            << toStableString(&Gep) << "\n";
}

void ConstraintsGenerator::addCmpConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            llvm::ICmpInst *I) {
  auto Left = LHS;
  auto Right = RHS;
  llvmValue2ExtVal(Left, I, 0);
  llvmValue2ExtVal(Right, I, 1);
  getOrInsertNode(Left);
  getOrInsertNode(Right);
  getPNINode(Left).unify(getPNINode(Right));
}

void ConstraintsGenerator::addAddConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &getOrInsertPNINode(LHS);
  auto Right = &getOrInsertPNINode(RHS);
  getOrInsertPNINode(I);
  if (Left->isPNRelated() || Right->isPNRelated()) {
    PG.addAddCons(getPNIValue(LHS), getPNIValue(RHS), getPNIValue(I), I);
  }
}
void ConstraintsGenerator::addSubConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &getOrInsertPNINode(LHS);
  auto Right = &getOrInsertPNINode(RHS);
  getOrInsertPNINode(I);
  if (Left->isPNRelated() || Right->isPNRelated()) {
    PG.addSubCons(getPNIValue(LHS), getPNIValue(RHS), getPNIValue(I), I);
  }
}

void ConstraintsGenerator::MLsubVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addCmpConstraint(Src1, Src2, &I);

  // type the inst as bool?
  assert(I.getType()->isIntegerTy(1));
  cg.createNode(&I);
}

void ConstraintsGenerator::MLsubVisitor::visitSelectInst(SelectInst &I) {
  auto DstVar = cg.getOrInsertNode(&I);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.getOrInsertNode(getExtValuePtr(Src1, &I, 1));
  auto Src2Var = cg.getOrInsertNode(getExtValuePtr(Src2, &I, 2));
  // Not generate boolean constraints. Because it must be i1.
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
  cg.addPointerCopy(&I, getExtValuePtr(Src1, &I, 1));
  cg.addPointerCopy(&I, getExtValuePtr(Src2, &I, 2));
}

void ConstraintsGenerator::MLsubVisitor::visitAdd(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addAddConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::MLsubVisitor::visitSub(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addSubConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::MLsubVisitor::visitAnd(BinaryOperator &I) {
  // llvm::errs() << "visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  auto Src1Node = cg.getOrInsertNode(getExtValuePtr(Src1, &I, 0));
  auto Src2Node = cg.getOrInsertNode(getExtValuePtr(Src2, &I, 1));
  auto RetNode = cg.getOrInsertNode(&I);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if (constantHasAllPointerTagMiddleBits(*CI)) {
      // act as simple assignment
      cg.addSubtype(RetNode, Src1Node);
      return;
    }
  } else {
    // llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
    //              << "Warn: And op without constant: " << I << "\n";
  }
  cg.setNonPointer(getExtValuePtr(Src1, &I, 0));
  cg.setNonPointer(getExtValuePtr(Src2, &I, 1));
  cg.setNonPointer(&I);
  return;
}

void ConstraintsGenerator::MLsubVisitor::visitOr(BinaryOperator &I) {
  // llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  auto Src1Node = cg.getOrInsertNode(getExtValuePtr(Src1, &I, 0));
  auto Src2Node = cg.getOrInsertNode(getExtValuePtr(Src2, &I, 1));
  auto RetNode = cg.getOrInsertNode(&I);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if (constantHasNoPointerTagMiddleBits(*CI)) {
      // act as simple assignment
      cg.addSubtype(RetNode, Src1Node);
      return;
    }
  } else {
    // llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
    //              << "Warn: Or op without constant: " << I << "\n";
  }
  // view as numeric operation?
  cg.setNonPointer(getExtValuePtr(Src1, &I, 0));
  cg.setNonPointer(getExtValuePtr(Src2, &I, 1));
  cg.setNonPointer(&I);
  return;
}

void ConstraintsGenerator::MLsubVisitor::visitInstruction(Instruction &I) {
  // return value
  if (I.getType()->isVoidTy()) {
    // skip void type
  } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
             ConstraintsGenerator::opTypes.at(I.getOpcode())
                 .addRetConstraint(&I, cg)) {
    // good
  } else {
    llvm::errs() << "WARN: MLsubGenerator: unhandled instruction return: " << I
                 << "\n";
  }
  // for each op
  for (unsigned Ind = 0; Ind < I.getNumOperands(); ++Ind) {
    auto Op = I.getOperand(Ind);
    if (Op->getType()->isVoidTy()) {
      // skip void type
    } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
               ConstraintsGenerator::opTypes.at(I.getOpcode())
                   .addOpConstraint(Ind, &I, cg)) {
      // good
    } else {
      llvm::errs() << "WARN: MLsubGenerator: unhandled instruction Op: " << I
                   << "\n";
    }
  }
}

// Helper function for string comparison
static bool strEq(const char *S1, const char *S2) {
  return strcmp(S1, S2) == 0;
}

// PcodeOpType method implementations
bool ConstraintsGenerator::PcodeOpType::addRetConstraint(
    Instruction *I, ConstraintsGenerator &cg) const {
  // only create Covariant constraints, use addSubtype to handle contra-variant.
  auto N = cg.createNode(I);
  if (I->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = output;
  if (ty == nullptr) { // no action
    return true;
  } else if (strEq(ty, "sint")) {
    cg.setNonPointer(I);
    auto SintNode = binarysub::make_primitive("sint", cg.getSize(I));
    cg.addSubtype(SintNode, N);
    return true;
  } else if (strEq(ty, "uint")) {
    cg.setNonPointer(I);
    auto UintNode = binarysub::make_primitive("uint", cg.getSize(I));
    cg.addSubtype(UintNode, N);
    return true;
  } else if (strEq(ty, "int")) {
    cg.setNonPointer(I);
    return true;
  } else if (strEq(ty, "float")) {
    auto FloatNode = binarysub::make_primitive("float", cg.getSize(I));
    cg.addSubtype(FloatNode, N);
    return true;
  }

  return false;
}

bool ConstraintsGenerator::PcodeOpType::addOpConstraint(
    unsigned Index, Instruction *I, ConstraintsGenerator &cg) const {
  assert(size >= 0 && static_cast<unsigned>(size) == I->getNumOperands() &&
         "input size not match");
  auto Op = I->getOperand(Index);
  if (Op->getType()->isVoidTy()) {
    return false;
  }
  auto N = cg.getOrInsertNode(getExtValuePtr(Op, I, Index));
  const char *ty = inputs[Index];
  if (ty == nullptr) {
    return true;
  } else if (strEq(ty, "sint")) {
    cg.setNonPointer(getExtValuePtr(Op, I, Index));
    auto SintNode = binarysub::make_primitive(
        "sint", cg.getSize(getExtValuePtr(Op, I, Index)));
    cg.addSubtype(N, SintNode);
    return true;
  } else if (strEq(ty, "uint")) {
    cg.setNonPointer(getExtValuePtr(Op, I, Index));
    auto UintNode = binarysub::make_primitive(
        "uint", cg.getSize(getExtValuePtr(Op, I, Index)));
    cg.addSubtype(N, UintNode);
    return true;
  } else if (strEq(ty, "int")) {
    cg.setNonPointer(getExtValuePtr(Op, I, Index));
    return true;
  } else if (strEq(ty, "float")) {
    auto FloatNode =
        binarysub::make_primitive("float",
                                  cg.getSize(getExtValuePtr(Op, I, Index)));
    cg.addSubtype(N, FloatNode);
    return true;
  }
  return false;
}

// Static member definition for opTypes
const std::map<unsigned, ConstraintsGenerator::PcodeOpType>
    ConstraintsGenerator::opTypes = {
        // for Trunc, ZExt, SExt
        {Instruction::SExt, {"sint", 1, (const char *[1]){"sint"}}},
        {Instruction::ZExt, {"uint", 1, (const char *[1]){"uint"}}},
        {Instruction::Trunc, {"int", 1, (const char *[1]){"int"}}},

        // other cast insts: FPToUIInst, FPToSIInst, UIToFPInst, SIToFPInst
        {Instruction::FPToUI, {"uint", 1, (const char *[1]){nullptr}}},
        {Instruction::FPToSI, {"sint", 1, (const char *[1]){nullptr}}},
        {Instruction::UIToFP, {nullptr, 1, (const char *[1]){"uint"}}},
        {Instruction::SIToFP, {nullptr, 1, (const char *[1]){"sint"}}},
        {Instruction::FAdd, {"float", 2, (const char *[2]){"float", "float"}}},
        {Instruction::FSub, {"float", 2, (const char *[2]){"float", "float"}}},
        {Instruction::FMul, {"float", 2, (const char *[2]){"float", "float"}}},
        {Instruction::FDiv, {"float", 2, (const char *[2]){"float", "float"}}},

        {Instruction::UDiv, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SDiv, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::URem, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SRem, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::Xor, {"uint", 2, (const char *[2]){"uint", "uint"}}},

        // {Instruction::Add, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Sub, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Mul, {"int", 2, (const char *[2]){"int", "int"}}},

        {Instruction::Shl, {"int", 2, (const char *[2]){"int", "int"}}},
        {Instruction::LShr, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::AShr, {"sint", 2, (const char *[2]){"sint", "uint"}}},
        // {Instruction::And, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Or, {"int", (const char *[2]){"int", "int"}, 2}},
};

// #endregion ConstraintsGenerator::MLsubVisitor

} // namespace notdec::mlsub
