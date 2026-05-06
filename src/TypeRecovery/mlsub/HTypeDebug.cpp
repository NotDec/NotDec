#include "notdec/TypeRecovery/mlsub/HTypeDebug.h"

#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Utils.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace notdec::mlsub {
namespace {

constexpr llvm::StringLiteral kValueHTypesFile = "ValueHTypes.txt";
constexpr llvm::StringLiteral kImportantHTypesFile = "ImportantHTypes.txt";

void primeSnapshotFormatter(const llvm2c::HTypeResult &Result,
                            ast::HTypeSnapshotFormatter &Formatter) {
  std::vector<std::pair<std::string, const ast::HType *>> Entries;
  Entries.reserve(Result.ValueTypesLower.size() + Result.ValueTypesUpper.size());
  for (const auto &Ent : Result.ValueTypesLower) {
    if (Ent.second == nullptr) {
      continue;
    }
    Entries.emplace_back(toStableString(Ent.first), Ent.second);
  }
  for (const auto &Ent : Result.ValueTypesUpper) {
    if (Ent.second == nullptr) {
      continue;
    }
    Entries.emplace_back(toStableString(Ent.first), Ent.second);
  }
  std::sort(Entries.begin(), Entries.end());
  for (const auto &Ent : Entries) {
    Formatter.collectType(Ent.second);
  }
  if (Result.MemoryType != nullptr) {
    Formatter.collectType(Result.MemoryType);
  }
  if (Result.MemoryDecl != nullptr) {
    Formatter.collectDecl(*Result.MemoryDecl);
  }
}

const ast::FunctionType *unwrapFunctionType(const ast::HType *Ty) {
  if (Ty == nullptr) {
    return nullptr;
  }
  if (auto *FT = llvm::dyn_cast<ast::FunctionType>(Ty)) {
    return FT;
  }
  if (auto *PT = llvm::dyn_cast<ast::PointerType>(Ty)) {
    return llvm::dyn_cast_or_null<ast::FunctionType>(PT->getPointeeType());
  }
  return nullptr;
}

std::string formatJoinedHTypes(llvm::ArrayRef<ast::HType *> Types,
                               ast::HTypeSnapshotFormatter &Formatter) {
  if (Types.empty()) {
    return "void";
  }
  std::vector<std::string> Parts;
  Parts.reserve(Types.size());
  for (auto *Ty : Types) {
    Parts.push_back(Formatter.formatType(Ty));
  }
  return llvm::join(Parts, " | ");
}

struct ImportantFunctionEntry {
  std::string Name;
  std::string StableKey;
  std::string RawType;
  std::vector<std::string> Args;
  std::string ReturnType;
};

} // namespace

void writeDebugValueHTypes(llvm::StringRef DebugDir,
                           const llvm2c::HTypeResult &Result) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(
      llvm2c::join(DebugDir.str(), kValueHTypesFile.str()), EC,
      llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Error printing to " << kValueHTypesFile << ", "
                 << EC.message() << "\n";
    return;
  }
  Result.print(Out);
}

void writeDebugImportantHTypes(llvm::StringRef DebugDir,
                               const llvm2c::HTypeResult &Result) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(
      llvm2c::join(DebugDir.str(), kImportantHTypesFile.str()), EC,
      llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Error printing to " << kImportantHTypesFile << ", "
                 << EC.message() << "\n";
    return;
  }

  ast::HTypeSnapshotFormatter Formatter(Result.HTCtx.get());
  primeSnapshotFormatter(Result, Formatter);

  std::vector<ImportantFunctionEntry> Functions;

  for (const auto &Ent : Result.ValueTypesLower) {
    auto *Value = std::get_if<llvm::Value *>(&Ent.first);
    if (Value == nullptr || *Value == nullptr) {
      continue;
    }
    auto *Func = llvm::dyn_cast<llvm::Function>(*Value);
    if (Func == nullptr) {
      continue;
    }

    ImportantFunctionEntry Entry;
    Entry.Name =
        Func->hasName() ? Func->getName().str() : toStableString(Ent.first);
    Entry.StableKey = toStableString(Ent.first);
    auto *DefaultTy = Result.getDefaultValueType(Ent.first);
    Entry.RawType =
        DefaultTy == nullptr ? "<null>" : Formatter.formatType(DefaultTy);
    Entry.ReturnType = "<unsupported>";

    if (const auto *FT = unwrapFunctionType(DefaultTy)) {
      Entry.ReturnType = formatJoinedHTypes(FT->getReturnType(), Formatter);
      for (auto *ParamTy : FT->getParamTypes()) {
        Entry.Args.push_back(Formatter.formatType(ParamTy));
      }
    }
    Functions.push_back(std::move(Entry));
  }

  std::sort(Functions.begin(), Functions.end(),
            [](const ImportantFunctionEntry &LHS,
               const ImportantFunctionEntry &RHS) {
              return std::tie(LHS.Name, LHS.StableKey) <
                     std::tie(RHS.Name, RHS.StableKey);
            });

  Out << "# ImportantHTypes\n\n";

  Out << "[memory]\n";
  if (Result.MemoryDecl != nullptr) {
    Out << "decl => " << Formatter.formatDeclName(*Result.MemoryDecl) << "\n";
  }
  if (Result.MemoryType != nullptr) {
    Out << "type => " << Formatter.formatType(Result.MemoryType) << "\n";
  }
  Out << "\n";

  Out << "[functions]\n";
  for (const auto &Entry : Functions) {
    Out << Entry.StableKey << "\n";
    Out << "  type => " << Entry.RawType << "\n";
    Out << "  ret => " << Entry.ReturnType << "\n";
    for (std::size_t Index = 0; Index < Entry.Args.size(); ++Index) {
      Out << "  arg" << Index << " => " << Entry.Args[Index] << "\n";
    }
    Out << "\n";
  }

  Out << "[decls]\n";
  if (Result.HTCtx != nullptr) {
    for (const auto *Decl : Formatter.getOrderedDecls()) {
      Out << Formatter.formatDecl(*Decl) << "\n";
    }
  }
}

} // namespace notdec::mlsub
