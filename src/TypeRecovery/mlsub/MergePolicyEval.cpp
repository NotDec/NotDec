#include "notdec/TypeRecovery/mlsub/MergePolicyEval.h"

#include "notdec/TypeRecovery/mlsub/MLsubGenerator.h"
#include "notdec-llvm2c/Utils.h"

#include <llvm/ADT/StringExtras.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

namespace notdec::mlsub {
namespace {

using Clock = std::chrono::steady_clock;

struct OracleType {
  std::string Pretty;
  std::string StrictKey;
  std::string Source;
  bool IsStrictStructPointer = false;
};

struct BadUnionRecord {
  std::string Rule;
  std::string Reason;
  std::string TypeA;
  std::string TypeB;
  std::string NodeA;
  std::string NodeB;
  std::string Location;
  std::string Evidence;
};

struct FragmentedValueRecord {
  std::string Value;
  std::string Source;
  std::string DebugType;
};

using FragmentedComponentMap =
    std::map<std::string, std::map<std::string,
                                   std::vector<FragmentedValueRecord>>>;

struct TruthSet {
  std::map<std::string, std::string> SamplesByType;
};

llvm::DIType *stripAliasAndQualifiers(llvm::DIType *Ty) {
  while (auto *Derived = llvm::dyn_cast_or_null<llvm::DIDerivedType>(Ty)) {
    switch (Derived->getTag()) {
    case llvm::dwarf::DW_TAG_typedef:
    case llvm::dwarf::DW_TAG_const_type:
    case llvm::dwarf::DW_TAG_volatile_type:
    case llvm::dwarf::DW_TAG_restrict_type:
    case llvm::dwarf::DW_TAG_atomic_type:
      Ty = Derived->getBaseType();
      continue;
    default:
      return Ty;
    }
  }
  return Ty;
}

std::string formatStructKey(const llvm::DICompositeType &Ty) {
  if (!Ty.getIdentifier().empty()) {
    return ("struct:" + Ty.getIdentifier()).str();
  }
  if (!Ty.getName().empty()) {
    return ("struct:" + Ty.getName()).str();
  }

  std::string Ret = "struct:<anon>";
  if (auto *File = Ty.getFile()) {
    Ret += "@";
    Ret += File->getFilename().str();
  }
  if (Ty.getLine() != 0) {
    Ret += ":";
    Ret += std::to_string(Ty.getLine());
  }
  return Ret;
}

std::optional<std::string> getStrictStructPointerKey(llvm::DIType *Ty) {
  Ty = stripAliasAndQualifiers(Ty);
  auto *PtrTy = llvm::dyn_cast_or_null<llvm::DIDerivedType>(Ty);
  if (PtrTy == nullptr || PtrTy->getTag() != llvm::dwarf::DW_TAG_pointer_type) {
    return std::nullopt;
  }

  auto *Pointee = stripAliasAndQualifiers(PtrTy->getBaseType());
  auto *StructTy = llvm::dyn_cast_or_null<llvm::DICompositeType>(Pointee);
  if (StructTy == nullptr ||
      StructTy->getTag() != llvm::dwarf::DW_TAG_structure_type ||
      StructTy->isForwardDecl()) {
    return std::nullopt;
  }
  return formatStructKey(*StructTy) + "*";
}

std::string formatDITypeName(const llvm::DIType *Ty) {
  if (Ty == nullptr) {
    return "<null>";
  }
  if (auto *Base = llvm::dyn_cast<llvm::DIBasicType>(Ty)) {
    return Base->getName().empty() ? "<basic>" : Base->getName().str();
  }
  if (auto *Derived = llvm::dyn_cast<llvm::DIDerivedType>(Ty)) {
    switch (Derived->getTag()) {
    case llvm::dwarf::DW_TAG_pointer_type:
      return formatDITypeName(Derived->getBaseType()) + "*";
    case llvm::dwarf::DW_TAG_typedef:
      if (!Derived->getName().empty()) {
        return Derived->getName().str();
      }
      return formatDITypeName(Derived->getBaseType());
    case llvm::dwarf::DW_TAG_const_type:
      return "const " + formatDITypeName(Derived->getBaseType());
    case llvm::dwarf::DW_TAG_volatile_type:
      return "volatile " + formatDITypeName(Derived->getBaseType());
    default:
      if (!Derived->getName().empty()) {
        return Derived->getName().str();
      }
      return formatDITypeName(Derived->getBaseType());
    }
  }
  if (auto *Composite = llvm::dyn_cast<llvm::DICompositeType>(Ty)) {
    std::string Prefix;
    switch (Composite->getTag()) {
    case llvm::dwarf::DW_TAG_structure_type:
      Prefix = "struct ";
      break;
    case llvm::dwarf::DW_TAG_union_type:
      Prefix = "union ";
      break;
    case llvm::dwarf::DW_TAG_array_type:
      Prefix = "array ";
      break;
    default:
      Prefix = "composite ";
      break;
    }
    return Prefix + (Composite->getName().empty() ? "<anon>"
                                                  : Composite->getName().str());
  }
  return Ty->getName().empty() ? "<type>" : Ty->getName().str();
}

OracleType makeOracleType(llvm::DIType *Ty, std::string Source) {
  OracleType Out;
  Out.Pretty = formatDITypeName(Ty);
  Out.Source = std::move(Source);
  if (auto Key = getStrictStructPointerKey(Ty)) {
    Out.StrictKey = *Key;
    Out.IsStrictStructPointer = true;
  }
  return Out;
}

bool isDirectDebugExpression(const llvm::DIExpression *Expr) {
  if (Expr == nullptr) {
    return true;
  }
  return !Expr->getFragmentInfo().has_value();
}

std::string formatSimpleTypeKey(const binarysub::SimpleType &Ty) {
  if (!Ty) {
    return "<null>";
  }
  if (auto *Var = Ty->getAsVariableState()) {
    return "v" + std::to_string(Var->id) + "@L" +
           std::to_string(Var->level);
  }
  return binarysub::debug_string(Ty);
}

binarysub::SimpleType resolveRoot(binarysub::SimpleType Ty) {
  if (!Ty || !Ty->getAsVariableState()) {
    return Ty;
  }
  return binarysub::resolve_variable(Ty);
}

binarysub::SimpleType resolveRootBeforeMerge(binarysub::SimpleType Ty,
                                             binarysub::SimpleType From) {
  while (Ty && Ty->getAsVariableState()) {
    if (Ty == From) {
      return From;
    }
    auto *Var = Ty->getAsVariableState();
    if (!Var->mergedInto) {
      return Ty;
    }
    Ty = Var->mergedInto;
  }
  return Ty;
}

bool hasPointerSizedLLVMType(const ExtValuePtr &Val, unsigned PointerSize) {
  llvm::Type *Ty = nullptr;
  if (auto *V = std::get_if<llvm::Value *>(&Val)) {
    if (*V == nullptr) {
      return false;
    }
    Ty = (*V)->getType();
  } else if (auto *Ret = std::get_if<ReturnValue>(&Val)) {
    if (Ret->Func == nullptr) {
      return false;
    }
    Ty = Ret->Func->getReturnType();
  } else if (auto *C = std::get_if<UConstant>(&Val)) {
    Ty = C->Val == nullptr ? nullptr : C->Val->getType();
  } else if (auto *C = std::get_if<ConstantAddr>(&Val)) {
    Ty = C->Val == nullptr ? nullptr : C->Val->getType();
  } else if (auto *Obj = std::get_if<StackObject>(&Val)) {
    Ty = Obj->Allocator == nullptr ? nullptr : Obj->Allocator->getType();
  } else if (auto *Obj = std::get_if<HeapObject>(&Val)) {
    Ty = Obj->Allocator == nullptr ? nullptr : Obj->Allocator->getType();
  }

  if (Ty == nullptr || Ty->isVoidTy()) {
    return false;
  }
  if (Ty->isPointerTy() || Ty->isFunctionTy()) {
    return true;
  }
  if (auto *IntTy = llvm::dyn_cast<llvm::IntegerType>(Ty)) {
    return IntTy->getBitWidth() == PointerSize;
  }
  return false;
}

std::uint64_t currentPeakRSSMB() {
#if defined(__unix__) || defined(__APPLE__)
  struct rusage Usage;
  if (getrusage(RUSAGE_SELF, &Usage) != 0) {
    return 0;
  }
#if defined(__APPLE__)
  return static_cast<std::uint64_t>(Usage.ru_maxrss / (1024 * 1024));
#else
  return static_cast<std::uint64_t>(Usage.ru_maxrss / 1024);
#endif
#else
  return 0;
#endif
}

} // namespace

struct MergePolicyEval::Impl {
  const llvm::Module &M;
  std::string OutputDir;
  unsigned PointerSize = 0;
  Clock::time_point Start = Clock::now();
  std::map<ExtValuePtr, OracleType> OracleByValue;
  std::map<ExtValuePtr, binarysub::SimpleType> ValueToNode;
  std::vector<BadUnionRecord> BadUnions;
  std::uint64_t AddSubtypeCalls = 0;

  Impl(const llvm::Module &M, llvm::StringRef OutputDir, unsigned PointerSize)
      : M(M), OutputDir(OutputDir.str()), PointerSize(PointerSize) {
    collectDebugInfo();
    writeDebugInfoValueTypes();
  }

  void recordOracle(const ExtValuePtr &Val, llvm::DIType *Ty,
                    std::string Source) {
    if (Ty == nullptr) {
      return;
    }
    OracleByValue.insert_or_assign(Val, makeOracleType(Ty, std::move(Source)));
  }

  bool recordGlobalPointerSlotContentOracle(const llvm::GlobalVariable &GV,
                                            llvm::DIType *Ty,
                                            llvm::StringRef Name) {
    bool Recorded = false;
    auto makeGlobalSlotSource = [&](llvm::StringRef Kind) {
      std::string Source = "global ";
      Source += Name.str();
      Source += " ";
      Source += Kind.str();
      return Source;
    };
    for (const llvm::User *ConstUser : GV.users()) {
      auto *User = const_cast<llvm::User *>(ConstUser);
      if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(User)) {
        if (Load->getPointerOperand() != &GV) {
          continue;
        }
        ExtValuePtr Val{Load};
        if (!hasPointerSizedLLVMType(Val, PointerSize)) {
          continue;
        }
        recordOracle(Val, Ty, makeGlobalSlotSource("load"));
        Recorded = true;
        continue;
      }

      auto *Store = llvm::dyn_cast<llvm::StoreInst>(User);
      if (Store == nullptr || Store->getPointerOperand() != &GV) {
        continue;
      }
      auto Val = getExtValuePtr(Store->getValueOperand(), Store, 0);
      if (!hasPointerSizedLLVMType(Val, PointerSize)) {
        continue;
      }
      recordOracle(Val, Ty, makeGlobalSlotSource("store"));
      Recorded = true;
    }
    return Recorded;
  }

  void collectGlobalDebugInfo() {
    for (const llvm::GlobalVariable &GV : M.globals()) {
      auto *Dbg = llvm::dyn_cast_or_null<llvm::DIGlobalVariableExpression>(
          GV.getMetadata(llvm::LLVMContext::MD_dbg));
      if (Dbg == nullptr || Dbg->getVariable() == nullptr ||
          !isDirectDebugExpression(Dbg->getExpression())) {
        continue;
      }
      auto *Var = Dbg->getVariable();
      std::string Source = "global ";
      Source += Var->getName().str();
      // A source-level `T *g` is an LLVM global slot whose value is `T *`.
      // The global address itself is closer to `T **`, so strict struct-pointer
      // oracle samples should attach to direct slot loads/stores instead.
      if (GV.getValueType()->isPointerTy() &&
          getStrictStructPointerKey(Var->getType()).has_value()) {
        recordGlobalPointerSlotContentOracle(GV, Var->getType(),
                                             Var->getName());
        continue;
      }
      recordOracle(ExtValuePtr{const_cast<llvm::GlobalVariable *>(&GV)},
                   Var->getType(), std::move(Source));
    }
  }

  void collectFunctionDebugInfo() {
    for (const llvm::Function &F : M) {
      auto *SP = F.getSubprogram();
      if (SP == nullptr) {
        continue;
      }
      if (auto *SubTy = SP->getType()) {
        auto Types = SubTy->getTypeArray();
        if (Types.size() > 0) {
          std::string Source = "function return ";
          Source += F.getName().str();
          recordOracle(ReturnValue{.Func = const_cast<llvm::Function *>(&F)},
                       Types[0], std::move(Source));
        }
      }

      for (llvm::MDNode *Node : SP->getRetainedNodes()) {
        auto *Local = llvm::dyn_cast_or_null<llvm::DILocalVariable>(Node);
        if (Local == nullptr || !Local->isParameter() ||
            Local->getArg() == 0) {
          continue;
        }
        unsigned ArgIndex = Local->getArg() - 1;
        if (ArgIndex >= F.arg_size()) {
          continue;
        }
        auto It = F.arg_begin();
        std::advance(It, ArgIndex);
        std::string Source = "arg ";
        Source += F.getName().str();
        Source += "::";
        Source += Local->getName().str();
        recordOracle(ExtValuePtr{const_cast<llvm::Argument *>(&*It)},
                     Local->getType(), std::move(Source));
      }
    }
  }

  void collectDbgIntrinsicInfo() {
    for (const llvm::Function &F : M) {
      for (const llvm::BasicBlock &BB : F) {
        for (const llvm::Instruction &I : BB) {
          auto *Dbg = llvm::dyn_cast<llvm::DbgVariableIntrinsic>(&I);
          if (Dbg == nullptr || Dbg->getVariable() == nullptr ||
              Dbg->getNumVariableLocationOps() != 1 || Dbg->hasArgList() ||
              !isDirectDebugExpression(Dbg->getExpression())) {
            continue;
          }
          llvm::Value *Location = Dbg->getVariableLocationOp(0);
          if (Location == nullptr ||
              llvm::isa<llvm::UndefValue, llvm::PoisonValue>(Location)) {
            continue;
          }
          std::string Source = "dbg ";
          Source += Dbg->getVariable()->getName().str();
          recordOracle(ExtValuePtr{Location}, Dbg->getVariable()->getType(),
                       std::move(Source));
        }
      }
    }
  }

  void collectDebugInfo() {
    collectGlobalDebugInfo();
    collectFunctionDebugInfo();
    collectDbgIntrinsicInfo();
  }

  bool isTargetValue(const ExtValuePtr &Val) const {
    auto It = OracleByValue.find(Val);
    if (It != OracleByValue.end() && It->second.IsStrictStructPointer) {
      return true;
    }
    return hasPointerSizedLLVMType(Val, PointerSize);
  }

  const OracleType *getOracle(const ExtValuePtr &Val) const {
    auto It = OracleByValue.find(Val);
    if (It == OracleByValue.end()) {
      return nullptr;
    }
    return &It->second;
  }

  TruthSet collectTruthForRoot(binarysub::SimpleType Root) const {
    TruthSet Out;
    Root = resolveRoot(Root);
    for (const auto &Ent : ValueToNode) {
      const auto *Oracle = getOracle(Ent.first);
      if (Oracle == nullptr || !Oracle->IsStrictStructPointer ||
          !isTargetValue(Ent.first)) {
        continue;
      }
      if (resolveRoot(Ent.second) != Root) {
        continue;
      }
      Out.SamplesByType.emplace(Oracle->StrictKey, toStableString(Ent.first));
    }
    return Out;
  }

  TruthSet collectTruthBeforeMerge(binarysub::SimpleType Root,
                                   binarysub::SimpleType From) const {
    TruthSet Out;
    for (const auto &Ent : ValueToNode) {
      const auto *Oracle = getOracle(Ent.first);
      if (Oracle == nullptr || !Oracle->IsStrictStructPointer ||
          !isTargetValue(Ent.first)) {
        continue;
      }
      if (resolveRootBeforeMerge(Ent.second, From) != Root) {
        continue;
      }
      Out.SamplesByType.emplace(Oracle->StrictKey, toStableString(Ent.first));
    }
    return Out;
  }

  void maybeRecordBadUnion(const TruthSet &FromSet, const TruthSet &ToSet,
                           llvm::StringRef Rule, llvm::StringRef Reason,
                           llvm::StringRef Location,
                           llvm::StringRef Evidence) {
    if (FromSet.SamplesByType.empty() || ToSet.SamplesByType.empty() ||
        FromSet.SamplesByType.size() > 1 || ToSet.SamplesByType.size() > 1) {
      return;
    }
    auto FromIt = FromSet.SamplesByType.begin();
    auto ToIt = ToSet.SamplesByType.begin();
    if (FromIt->first == ToIt->first) {
      return;
    }

    BadUnionRecord Rec;
    Rec.Rule = Rule.str();
    Rec.Reason = Reason.str();
    Rec.TypeA = FromIt->first;
    Rec.TypeB = ToIt->first;
    Rec.NodeA = FromIt->second;
    Rec.NodeB = ToIt->second;
    Rec.Location = Location.str();
    Rec.Evidence = Evidence.str();
    BadUnions.push_back(std::move(Rec));
  }

  void observeValueNode(const ExtValuePtr &Val, binarysub::SimpleType Ty) {
    if (Ty != nullptr) {
      ValueToNode.insert_or_assign(Val, Ty);
    }
  }

  void observeValueMapMerge(binarysub::SimpleType From,
                            binarysub::SimpleType To,
                            llvm::ArrayRef<ExtValuePtr> MovedValues) {
    auto FromSet = collectTruthForRoot(From);
    auto ToSet = collectTruthForRoot(To);
    maybeRecordBadUnion(FromSet, ToSet, "v2n_merge", "v2n_merge",
                        formatSimpleTypeKey(To), formatSimpleTypeKey(From));
    for (const auto &Val : MovedValues) {
      ValueToNode.insert_or_assign(Val, To);
    }
  }

  void observeVariableMerged(const binarysub::MergeEvent &Event) {
    auto FromSet = collectTruthBeforeMerge(Event.from, Event.from);
    auto ToSet = collectTruthBeforeMerge(Event.into, Event.from);
    std::string Evidence = "lhs=" + binarysub::debug_string(Event.originalLhs) +
                           " rhs=" +
                           binarysub::debug_string(Event.originalRhs);
    maybeRecordBadUnion(FromSet, ToSet, "binarysub_merge",
                        binarysub::merge_reason_name(Event.reason),
                        formatSimpleTypeKey(Event.into), Evidence);
  }

  void writeDebugInfoValueTypes() const {
    std::error_code EC = llvm::sys::fs::create_directories(OutputDir);
    if (EC) {
      llvm::errs() << "Warning: failed to create merge eval dir " << OutputDir
                   << ": " << EC.message() << "\n";
      return;
    }
    llvm::raw_fd_ostream Out(
        llvm2c::join(OutputDir, "DebugInfoValueTypes.txt"), EC,
        llvm::sys::fs::OF_Text);
    if (EC) {
      llvm::errs() << "Warning: failed to write DebugInfoValueTypes.txt: "
                   << EC.message() << "\n";
      return;
    }

    Out << "# DebugInfoValueTypes\n\n";
    std::vector<std::pair<std::string, const OracleType *>> Rows;
    Rows.reserve(OracleByValue.size());
    for (const auto &Ent : OracleByValue) {
      Rows.push_back({toStableString(Ent.first), &Ent.second});
    }
    std::sort(Rows.begin(), Rows.end(),
              [](const auto &LHS, const auto &RHS) {
                return LHS.first < RHS.first;
              });
    for (const auto &Row : Rows) {
      Out << Row.first << "\n";
      Out << "  debug_type => " << Row.second->Pretty << "\n";
      Out << "  strict => "
          << (Row.second->IsStrictStructPointer ? Row.second->StrictKey
                                                : "<none>")
          << "\n";
      Out << "  source => " << Row.second->Source << "\n\n";
    }
  }

  void writeBadUnions() const {
    std::error_code EC;
    llvm::raw_fd_ostream Out(llvm2c::join(OutputDir, "bad_unions.jsonl"), EC,
                             llvm::sys::fs::OF_Text);
    if (EC) {
      llvm::errs() << "Warning: failed to write bad_unions.jsonl: "
                   << EC.message() << "\n";
      return;
    }
    for (const auto &Rec : BadUnions) {
      llvm::json::Object Obj{{"rule", Rec.Rule},
                             {"reason", Rec.Reason},
                             {"type_a", Rec.TypeA},
                             {"type_b", Rec.TypeB},
                             {"node_a", Rec.NodeA},
                             {"node_b", Rec.NodeB},
                             {"location", Rec.Location},
                             {"evidence", Rec.Evidence}};
      Out << llvm::formatv("{0}", llvm::json::Value(std::move(Obj))) << "\n";
    }
  }

  void writeFragmentedTypes(const FragmentedComponentMap &TypeComponents) const {
    std::error_code EC;
    llvm::raw_fd_ostream Out(
        llvm2c::join(OutputDir, "fragmented_types.jsonl"), EC,
        llvm::sys::fs::OF_Text);
    if (EC) {
      llvm::errs() << "Warning: failed to write fragmented_types.jsonl: "
                   << EC.message() << "\n";
      return;
    }

    for (const auto &TypeEnt : TypeComponents) {
      if (TypeEnt.second.size() <= 1) {
        continue;
      }

      llvm::json::Array Components;
      for (const auto &Comp : TypeEnt.second) {
        auto Records = Comp.second;
        std::sort(Records.begin(), Records.end(),
                  [](const auto &LHS, const auto &RHS) {
                    return LHS.Value < RHS.Value;
                  });

        llvm::json::Array Values;
        for (const auto &Rec : Records) {
          Values.push_back(llvm::json::Object{{"value", Rec.Value},
                                              {"source", Rec.Source},
                                              {"debug_type", Rec.DebugType}});
        }
        Components.push_back(llvm::json::Object{
            {"root", Comp.first},
            {"count", static_cast<int64_t>(Comp.second.size())},
            {"values", std::move(Values)}});
      }

      llvm::json::Object Obj{{"type", TypeEnt.first},
                             {"components", std::move(Components)}};
      Out << llvm::formatv("{0}", llvm::json::Value(std::move(Obj))) << "\n";
    }
  }

  void finish(const AllGraphs &AG) {
    std::map<binarysub::SimpleType, std::vector<ExtValuePtr>,
             binarysub::SimpleTypePointerLess>
        Components;
    std::set<ExtValuePtr> SeenValues;
    std::uint64_t TargetNodes = 0;
    std::uint64_t TypedTargetNodes = 0;

    for (const auto &Data : AG.AllSCCs) {
      if (Data.Generator == nullptr) {
        continue;
      }
      for (const auto &Ent : Data.Generator->V2N) {
        if (!SeenValues.insert(Ent.first).second || !isTargetValue(Ent.first)) {
          continue;
        }
        ++TargetNodes;
        const auto *Oracle = getOracle(Ent.first);
        if (Oracle != nullptr && Oracle->IsStrictStructPointer) {
          ++TypedTargetNodes;
        }
        Components[resolveRoot(Ent.second)].push_back(Ent.first);
      }
    }

    std::uint64_t PollutedComponents = 0;
    std::uint64_t ExtraTypes = 0;
    std::map<std::string, std::map<std::string, std::uint64_t>> TypeComponents;
    FragmentedComponentMap TypeComponentValues;

    for (const auto &Component : Components) {
      std::set<std::string> TruthTypes;
      auto RootKey = formatSimpleTypeKey(Component.first);
      for (const auto &Val : Component.second) {
        const auto *Oracle = getOracle(Val);
        if (Oracle == nullptr || !Oracle->IsStrictStructPointer) {
          continue;
        }
        TruthTypes.insert(Oracle->StrictKey);
        TypeComponents[Oracle->StrictKey][RootKey] += 1;
        TypeComponentValues[Oracle->StrictKey][RootKey].push_back(
            FragmentedValueRecord{.Value = toStableString(Val),
                                  .Source = Oracle->Source,
                                  .DebugType = Oracle->Pretty});
      }
      if (TruthTypes.size() > 1) {
        ++PollutedComponents;
        ExtraTypes += TruthTypes.size() - 1;
      }
    }

    std::uint64_t FragmentedTypes = 0;
    std::uint64_t FragmentedNodes = 0;
    for (const auto &Ent : TypeComponents) {
      if (Ent.second.size() <= 1) {
        continue;
      }
      ++FragmentedTypes;
      std::uint64_t Total = 0;
      std::uint64_t Largest = 0;
      for (const auto &Comp : Ent.second) {
        Total += Comp.second;
        Largest = std::max(Largest, Comp.second);
      }
      FragmentedNodes += Total - Largest;
    }

    auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       Clock::now() - Start)
                       .count();
    auto Stats = binarysub::globalTypeContext().simpleTypePool.graphStats();

    std::error_code EC = llvm::sys::fs::create_directories(OutputDir);
    if (EC) {
      llvm::errs() << "Warning: failed to create merge eval dir " << OutputDir
                   << ": " << EC.message() << "\n";
      return;
    }

    writeBadUnions();
    writeFragmentedTypes(TypeComponentValues);

    llvm::json::Object Summary{
        {"coverage",
         llvm::json::Object{
             {"target_nodes", static_cast<int64_t>(TargetNodes)},
             {"typed_target_nodes", static_cast<int64_t>(TypedTargetNodes)},
             {"typed_coverage",
              TargetNodes == 0
                  ? 0.0
                  : static_cast<double>(TypedTargetNodes) /
                        static_cast<double>(TargetNodes)}}},
        {"wrong_merge",
         llvm::json::Object{
             {"bad_unions", static_cast<int64_t>(BadUnions.size())},
             {"polluted_components", static_cast<int64_t>(PollutedComponents)},
             {"extra_types", static_cast<int64_t>(ExtraTypes)}}},
        {"fragmentation",
         llvm::json::Object{
             {"fragmented_types", static_cast<int64_t>(FragmentedTypes)},
             {"fragmented_nodes", static_cast<int64_t>(FragmentedNodes)}}},
        {"performance",
         llvm::json::Object{
             {"wall_ms", static_cast<int64_t>(Elapsed)},
             {"peak_rss_mb", static_cast<int64_t>(currentPeakRSSMB())},
             {"nodes_created", static_cast<int64_t>(Stats.variableNodes)},
             {"representative_nodes",
              static_cast<int64_t>(Stats.representativeVariableNodes)},
             {"merged_nodes", static_cast<int64_t>(Stats.mergedVariableNodes)},
             {"add_subtype_calls", static_cast<int64_t>(AddSubtypeCalls)}}}};

    EC = {};
    llvm::raw_fd_ostream SummaryOut(
        llvm2c::join(OutputDir, "merge-eval-summary.json"), EC,
        llvm::sys::fs::OF_Text);
    if (EC) {
      llvm::errs() << "Warning: failed to write merge-eval-summary.json: "
                   << EC.message() << "\n";
      return;
    }
    SummaryOut << llvm::formatv("{0:2}\n",
                                llvm::json::Value(std::move(Summary)));
  }
};

MergePolicyEval::MergePolicyEval(const llvm::Module &M,
                                 llvm::StringRef OutputDir,
                                 unsigned PointerSize)
    : P(std::make_unique<Impl>(M, OutputDir, PointerSize)) {}

MergePolicyEval::~MergePolicyEval() = default;

void MergePolicyEval::observeValueNode(const ExtValuePtr &Val,
                                       binarysub::SimpleType Ty) {
  P->observeValueNode(Val, Ty);
}

void MergePolicyEval::observeValueMapMerge(
    binarysub::SimpleType From, binarysub::SimpleType To,
    llvm::ArrayRef<ExtValuePtr> MovedValues) {
  P->observeValueMapMerge(From, To, MovedValues);
}

void MergePolicyEval::observeVariableMerged(
    const binarysub::MergeEvent &Event) {
  P->observeVariableMerged(Event);
}

void MergePolicyEval::observeAddSubtype() { P->AddSubtypeCalls += 1; }

void MergePolicyEval::finish(const AllGraphs &AG) { P->finish(AG); }

} // namespace notdec::mlsub
