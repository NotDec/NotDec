
#include "Utils/Utils.h"

#include <clang/AST/ASTDumper.h>
#include <clang/AST/Type.h>
#include <fstream>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <optional>
#include <sstream>
#include <string>

namespace notdec {

std::string getFuncSetName(const std::set<llvm::Function *> &SCC) {
  std::string SCCNames;
  for (auto *F : SCC) {
    if (!SCCNames.empty()) {
      SCCNames += ",";
    }
    auto FName = F->getName().str();
    assert(!FName.empty());
    SCCNames += FName;
  }
  return SCCNames;
}

[[nodiscard]] bool equal(llvm::StringRef S1, const char* S2) {
  return S1 == S2;
}

[[nodiscard]] std::string toString(const clang::QualType &QT) {
  std::string Out;
  llvm::raw_string_ostream SS(Out);
  clang::ASTDumper Dumper(llvm::errs(), /*ShowColors=*/false);
  Dumper.Visit(QT);
  return Out;
}

} // namespace notdec
