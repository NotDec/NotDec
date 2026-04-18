
#include <llvm/Bitcode/BitcodeWriter.h>
#include "notdec/TypeRecovery/mlsub/MLsubGenerator.h"
#include "Utils/CallGraphDotInfo.h"
#include "binarysub/binarysub-core.h"
#include "binarysub/binarysub-infer.h"
#include "binarysub/binarysub-primitive-semantics.h"
#include "binarysub/binarysub.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Utils.h"
#include "notdec/TypeRecovery/Lattice.h"
#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "notdec/Utils/AllSCCIterator.h"
#include "notdec/Utils/SingleNodeSCCIterator.h"
#include "notdec/Utils/Utils.h"

#include <cassert>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/SHA256.h>
#include <algorithm>
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

namespace {

struct PolyPolicyConfig {
  std::set<std::string> PolyFuncs;
  std::map<std::string, unsigned int> LevelOverrides;
};

constexpr llvm::StringLiteral kValueTypesFile = "ValueTypes.txt";
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
      auto *Node = G.PG.getPNIVarOrNull(Inst);
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
  llvm::json::Object Anchor{
      {"version", int64_t(1)},
      {"stage", "mlsub-input"},
      {"ir_file", kMLsubInputIRFile.str()},
      {"sha256", ModuleSHA256Hex},
      {"data_layout", M.getDataLayout().getStringRepresentation()},
      {"target_triple", M.getTargetTriple()},
  };
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

    auto *Node = G.PG.getPNIVarOrNull(Target.Value);
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

    auto *Node = G.PG.getPNIVarOrNull(Target.Value);
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
    auto *Func = M.getFunction(Ent.first);
    if (Func == nullptr) {
      llvm::errs() << "Warning: MLsub " << Kind
                   << " override function not found: " << Ent.first << "\n";
      continue;
    }

    const auto &Spec = requireObject(Ent.second, FuncPath);
    auto *Args = Spec.getArray("args");
    if (Args == nullptr) {
      failSignatureOverride(FuncPath, "missing array field 'args'");
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
        failSignatureOverride(FuncPath,
                              "signature override requires a function definition");
      }
      llvm::errs() << "Warning: skip MLsub signature override for declaration "
                   << Func->getName() << "\n";
      continue;
    }
    if (Spec.get("ret") == nullptr) {
      failSignatureOverride(FuncPath, "missing field 'ret'");
    }
    if (auto *Constraints = Spec.get("constraints")) {
      requireArray(*Constraints, appendJSONPath(FuncPath, "constraints"));
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
                          ModuleSHA256Hex.str())
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
    auto Current = M.getTargetTriple();
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
    auto *Func = M.getFunction(Ent.first);
    if (Func == nullptr) {
      llvm::errs() << "Warning: MLsub extra constraints function not found: "
                   << Ent.first << "\n";
      continue;
    }

    const auto *Spec = Ent.second.getAsObject();
    if (Spec == nullptr) {
      failExtraConstraints(FuncPath, "expected object");
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

void appendDebugValueTypes(
    llvm::StringRef DebugDir, llvm::StringRef SCCName,
    DSUMap<ExtValuePtr, SimpleType> &V2N,
    const std::set<ExtValuePtr> &ContraVariantValues,
    const std::map<binarysub::PolarVar, binarysub::UTypePtr> &Res,
    bool SolveMemory, const binarysub::PolarVar &PolMem) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(join(DebugDir.str(), kValueTypesFile.str()), EC,
                           llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Error printing to " << kValueTypesFile << ", "
                 << EC.message() << "\n";
    return;
  }

  auto getPol = [&](ExtValuePtr V) { return !ContraVariantValues.count(V); };
  std::vector<std::string> Lines;
  Lines.reserve(V2N.size() + (SolveMemory ? 1 : 0));

  for (const auto &Ent : V2N) {
    bool Pol = getPol(Ent.first);
    auto It = Res.find(binarysub::PolarVar{.var = Ent.second, .pos = Pol});
    std::string UTypeStr = "<null>";
    if (It != Res.end() && It->second) {
      UTypeStr = binarysub::printType(It->second);
    }
    std::string Line = Pol ? "[+]" : "[-]";
    Line += " ";
    Line += toString(Ent.first, true);
    Line += " => ";
    Line += UTypeStr;
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

std::shared_ptr<ConstraintsGenerator> getFuncCG(AllGraphs &AG,
                                                const llvm::Function *F) {
  auto *CGN = AG.CG->getOrInsertFunction(const_cast<llvm::Function *>(F));
  if (!AG.Func2SCCIndex.count(CGN)) {
    return nullptr;
  }
  return AG.AllSCCs.at(AG.Func2SCCIndex.at(CGN)).Generator;
}

std::optional<std::string> getPNDiffState(ConstraintsGenerator &CG,
                                          ExtValuePtr Val) {
  auto *Node = CG.PG.getPNIVarOrNull(Val);
  if (Node == nullptr || !Node->isPNRelated()) {
    return std::nullopt;
  }
  if (Node->isPointer() || Node->isNull()) {
    return "ptr";
  }
  if (Node->isNumber()) {
    return "num";
  }
  return "unknown";
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
    if (auto State =
            getPNDiffState(CG, const_cast<llvm::Instruction *>(&Inst))) {
      Parts.push_back("result=" + *State);
    }
  }
  for (unsigned I = 0; I < Inst.getNumOperands(); ++I) {
    ExtValuePtr Op = Inst.getOperand(I);
    llvmValue2ExtVal(Op, const_cast<llvm::Instruction *>(&Inst), I);
    if (auto State = getPNDiffState(CG, Op)) {
      Parts.push_back("op" + std::to_string(I) + "=" + *State);
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
    if (auto State =
            getPNDiffState(CG, ReturnValue{.Func = const_cast<llvm::Function *>(&F)})) {
      Parts.push_back("ret=" + *State);
    }
  }
  unsigned ArgIndex = 0;
  for (auto &Arg : F.args()) {
    if (auto State =
            getPNDiffState(CG, const_cast<llvm::Argument *>(&Arg))) {
      Parts.push_back("arg" + std::to_string(ArgIndex) + "=" + *State);
    }
    ++ArgIndex;
  }
  if (Parts.empty()) {
    return std::nullopt;
  }
  return "pndiff: " + joinParts(Parts);
}

std::string formatInstructionText(const llvm::Instruction &Inst) {
  std::string S;
  llvm::raw_string_ostream OS(S);
  Inst.print(OS);
  return OS.str();
}

std::string formatConstraintStateSummary(ConstraintsGenerator &CG,
                                         const ConsNode &Cons) {
  auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
  auto ResultState = getPNDiffState(CG, Nodes[2]).value_or("unknown");
  auto LeftState = getPNDiffState(CG, Nodes[0]).value_or("unknown");
  auto RightState = getPNDiffState(CG, Nodes[1]).value_or("unknown");
  return "result=" + ResultState + ", op0=" + LeftState +
         ", op1=" + RightState;
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

    AnyResidual = true;
    auto &CG = *Data.Generator;
    Out << "## SCC: " << Data.SCCName << "\n";
    for (const auto &Cons : CG.PG.Constraints) {
      auto Nodes = const_cast<ConsNode &>(Cons).getNodes();
      Out << "kind: " << (Cons.isAdd() ? "Add" : "Sub") << "\n";
      Out << "inst: " << formatInstructionText(*Cons.getInst()) << "\n";
      Out << "result: " << toStableString(Nodes[2]) << "\n";
      Out << "op0: " << toStableString(Nodes[0]) << "\n";
      Out << "op1: " << toStableString(Nodes[1]) << "\n";
      Out << "state: " << formatConstraintStateSummary(CG, Cons) << "\n\n";
    }
  }

  if (!AnyResidual) {
    Out << "No residual Add/Sub constraints after solve.\n";
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

void MLsubRecovery::run() {
  auto &M = const_cast<llvm::Module &>(Mod);
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
    llvm::sys::fs::remove(join(*WorkDir, kPNDiffWarnFile.str()));
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
      BinarysubTraceFile->flush();
    }
  } else {
    BinarysubTraceFile.reset();
  }

  binarysub::binarysub_set_trace_stream(BinarysubTraceFile.get());

  if (!MemoryType) {
    MemoryType = binarysub::make_variable(0, PointerSize);
  }

  SummaryOverrideFuncs.clear();
  SummaryOverrideDoc = makeEmptyOverrideDoc();
  SignatureOverrideFuncs.clear();
  SignatureOverrideDoc = makeEmptyOverrideDoc();
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
    notdec::utils::CallGraphDOTInfo CFGInfo(&M, &*CallG, nullptr);
    llvm::WriteGraph(CGDot, &CFGInfo, false);
    CGDot.close();
  }

  prepareSCC(*CallG);

  bottomUpPhase();

  topDownPhase();

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

void MLsubRecovery::applySummaryOverride(ConstraintsGenerator &G,
                                         llvm::Function &Func,
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
  G.addSubtype(binarysub::make_function(Args, RetRecipe.Root), FuncNode);
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
        Data.SCCName, PointerSize, Data.SCCSet, MemoryType, Data.level,
        BinarysubTraceFile.get());
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
      auto Ind2 = AG.Func2SCCIndex.at(AG.CG->getOrInsertFunction(F));
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

void ConstraintsGenerator::genTypes(ast::HTypeContext &HCtx,
                                    const llvm::DataLayout &DL,
                                    bool SolveMemory) {
  binarysub::TypeSimplifier Ts;
  using binarysub::PolarVar;
  SnapshotContraVariantValues = ContraVariantValues;
  std::set<PolarVar> Tys;
  auto getPol = [&](ExtValuePtr V) { return !ContraVariantValues.count(V); };
  for (auto &Ent : V2N) {
    Tys.insert(PolarVar{.var = Ent.second, .pos = getPol(Ent.first)});
  }
  auto PolMem = PolarVar{.var = MemoryType, .pos = false};
  if (SolveMemory) {
    Tys.insert(PolMem);
  }
  std::map<PolarVar, binarysub::UTypePtr> Res = Ts.bulkSimplify(Tys, false);

  // Create TypeBuilder context and builder
  TypeBuilderContext TBCtx(HCtx, DL);
  TypeBuilder TB(TBCtx);

  for (auto &Ent : V2N) {
    auto It = Res.find(PolarVar{.var = Ent.second, .pos = getPol(Ent.first)});
    ast::HType *Converted = nullptr;
    if (It != Res.end() && It->second) {
      TB.setDebugRootLabel(toString(Ent.first, true));
      Converted = TB.convert(It->second);
      TB.setDebugRootLabel(std::nullopt);
    }
    ValueTypes.insert({Ent.first, Converted});
  }
  if (SolveMemory) {
    auto MemUTy = Res.at(PolMem);
    TB.setDebugRootLabel(std::string("<memory>"));
    ValueTypes.insert({nullptr, TB.convert(MemUTy)});
    TB.setDebugRootLabel(std::nullopt);
  }

  if (auto WorkDir = notdec::getWorkDirOpt()) {
    appendDebugValueTypes(*WorkDir, Name, V2N, ContraVariantValues, Res,
                          SolveMemory, PolMem);
  }
}

void ConstraintsGenerator::releaseBinarysubState() {
  for (auto &Ent : V2N) {
    binarysub::release_type_graph(Ent.second);
  }
  binarysub::release_type_graph(MemoryType);
  V2N = {};
  ContraVariantValues.clear();
  unhandledCalls.clear();
}

void MLsubRecovery::genASTTypes(llvm::Module &M) {
  ResultVal = std::make_unique<Result>();
  // 合并所有类型到一个大的ValueTypes里面。
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    for (auto &Ent : Data.Generator->ValueTypes) {
      auto It = ResultVal->ValueTypes.insert(Ent);
      assert(It.second && "Duplicated Entry?");
    }
    ResultVal->ContraVariantValues.insert(
        Data.Generator->SnapshotContraVariantValues.begin(),
        Data.Generator->SnapshotContraVariantValues.end());
  }
  ResultVal->HTCtx = HCtx;
  // handle Memory type.
  auto Mem = AG.AllSCCs.at(0).Generator->ValueTypes.at(nullptr);
  ResultVal->MemoryType = Mem;
  if (Mem->isRecordType()) {
    ResultVal->MemoryDecl = Mem->getAsRecordDecl();
  }
}

void MLsubRecovery::topDownPhase() {
  // TODO 怎么处理TopDown的类型传递？设置多态参数的类型？
  // TODO 对于每个SCCData的所有Caller，都instantiate到 -x level？
  // 也许我该基于最后都优化完毕之后的CompactType？？

  if (!HCtx) {
    HCtx = std::make_shared<ast::HTypeContext>();
  }
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    // 尝试运行简化算法，保存到ValueTypes里面。
    // solve memory if ind == 0
    Data.Generator->genTypes(*HCtx, Mod.getDataLayout(), Ind == 0);
  }
  for (auto &Data : AG.AllSCCs) {
    Data.Generator->releaseBinarysubState();
  }
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
        if (Fn->hasName() && PolyFuncs.count(Fn->getName().str())) {
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
    std::vector<std::pair<std::string, SimpleType>> fields;
    fields.push_back(
        {OffsetRange{.offset = CA->Val->getSExtValue()}.str(), res});
    addSubtype(MemoryType, binarysub::make_record(std::move(fields)));
    addSubtype(res, binarysub::make_record({}));
    return res;
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: ConstraintsGenerator::convertSimpleType unhandled "
                  "type of ExtValPtr\n";
  std::abort();
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
      auto *Node = PG.getPNIVarOrNull(Val);
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
          // if is function pointer array
          auto T1 = GV->getType();
          if (T1->isPointerTy() && T1->getPointerElementType()->isArrayTy() &&
              T1->getPointerElementType()
                  ->getArrayElementType()
                  ->isPointerTy() &&
              T1->getPointerElementType()
                      ->getArrayElementType()
                      ->getPointerElementType() != nullptr &&
              T1->getPointerElementType()
                  ->getArrayElementType()
                  ->getPointerElementType()
                  ->isFunctionTy()) {
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
            assert(false && "TODO: PNI setNonPtr");
            assert(false && "TODO: getOrCreatePrim");
            assert(false && "TODO: addSubtype");
            return;
          } else if (isWithOverflowIntrinsicUnsigned(
                         Target->getIntrinsicID())) {
            auto N = cg.createNode(&I);
            assert(false && "TODO: PNI setNonPtr");
            assert(false && "TODO: getOrCreatePrim");
            assert(false && "TODO: addSubtype");
            return;
          }
        } else if (Ind == 1) {
          assert(I.getType()->isIntegerTy(1));
          cg.createNode(&I);
          return;
        }
      }
    }
  }
  assert(false && "TODO: ExtractValueInst general case");
}

void ConstraintsGenerator::MLsubVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto SrcNode = cg.getNodeOrNull(getExtValuePtr(Src, &I, 0));
    if (SrcNode) {
      cg.addRemapType(&I, getExtValuePtr(Src, &I, 0));
    }
    return;
  } else if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *Src = I.getOperand(0);
    auto SrcNode = cg.getNodeOrNull(getExtValuePtr(Src, &I, 0));
    if (SrcNode) {
      cg.addRemapType(&I, getExtValuePtr(Src, &I, 0));
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
  // TODO add more func names
  if (auto F = I.getCalledFunction()) {
    if (F->getName() == "malloc") {
      return true;
    }
  }
  return false;
}

bool ConstraintsGenerator::MLsubVisitor::handleIntrinsicCall(
    llvm::CallBase &I) {
  auto Target = I.getCalledFunction();
  if (!Target->isIntrinsic()) {
    return false;
  }
  switch (Target->getIntrinsicID()) {
  case Intrinsic::memset:
  case Intrinsic::memcpy:
  case Intrinsic::memmove:
    return true;
  default:
    break;
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
    if (Target->getName().startswith("llvm.dbg") ||
        Target->getName().startswith("llvm.lifetime")) {
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
      Ret = cg.getOrInsertNode(&I);
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
    }
  }
}

unsigned ConstraintsGenerator::getPointerElemSize(Type *ty) {
  Type *Elem = ty->getPointerElementType();
  return llvm2c::getLLVMTypeSize(Elem, PointerSize);
}

void ConstraintsGenerator::MLsubVisitor::visitLoadInst(LoadInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 0));
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
  auto BitSize = cg.getPointerElemSize(I.getPointerOperandType());

  cg.addSubtype(PtrVal, binarysub::make_ptr_load(RetVal, BitSize));
}

void ConstraintsGenerator::MLsubVisitor::visitStoreInst(StoreInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(getExtValuePtr(I.getPointerOperand(), &I, 0));
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
  auto BitSize = cg.getPointerElemSize(I.getPointerOperandType());
  auto StoreVal = cg.getOrInsertNode(getExtValuePtr(I.getValueOperand(), &I, 0));

  cg.addSubtype(PtrVal, binarysub::make_ptr_store(StoreVal, BitSize));
}

void ConstraintsGenerator::MLsubVisitor::visitAllocaInst(AllocaInst &I) {
  auto Node = cg.createNode(&I);
  // set as pointer type
  cg.setPointer(&I);
  cg.ContraVariantValues.insert(&I);
}

void ConstraintsGenerator::MLsubVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  // supress warnings for table gep
  if (Gep.getPointerOperand()->getName().startswith("table_")) {
    return;
  } else if (Gep.hasAllZeroIndices()) {
    auto Src = getExtValuePtr(Gep.getPointerOperand(), &Gep, 0);
    cg.getOrInsertNode(Src);
    cg.addRemapType(&Gep, Src);
    return;
  }
  std::cerr << "Warning: MLsubVisitor::visitGetElementPtrInst: "
               "Gep should not exist before this pass!\n";
  // But if we really want to support this, handle it the same way as AddInst.
  // A shortcut to create a offseted pointer. the operate type must be i8*.
  // Just like ptradd.
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
  PG.getPNIVar(Left).unify(PG.getPNIVar(Right));
}

void ConstraintsGenerator::addAddConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &PG.getOrInsertPNINode(LHS);
  auto Right = &PG.getOrInsertPNINode(RHS);
  // auto Res = &
  PG.getOrInsertPNINode(I);
  if (Left->isPNRelated() || Right->isPNRelated()) {
    PG.addAddCons(LHS, RHS, I, I);
  }
}
void ConstraintsGenerator::addSubConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &PG.getOrInsertPNINode(LHS);
  auto Right = &PG.getOrInsertPNINode(RHS);
  // auto Res = &
  PG.getOrInsertPNINode(I);
  if (Left->isPNRelated() || Right->isPNRelated()) {
    PG.addSubCons(LHS, RHS, I, I);
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
  auto DstVar = cg.createNode(&I);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.getOrInsertNode(getExtValuePtr(Src1, &I, 0));
  auto Src2Var = cg.getOrInsertNode(getExtValuePtr(Src2, &I, 1));
  // Not generate boolean constraints. Because it must be i1.
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
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
    if ((CI->getZExtValue() & 0x3fffff00) == 0x3fffff00) {
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
    if ((CI->getZExtValue() & 0x3fffff00) == 0) {
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
