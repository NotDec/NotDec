
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

std::string getSuffix(std::string fname) {
  std::size_t ind = fname.find_last_of('.');
  if (ind != std::string::npos) {
    return fname.substr(ind);
  }
  return std::string();
}
namespace notdec {

namespace {
std::string CurrentWorkDir;
}

std::string getDefaultWorkDir(const std::string &inputPath) {
  return inputPath + ".notdec";
}

void setWorkDir(std::string path) { CurrentWorkDir = std::move(path); }

llvm::StringRef getWorkDir() { return CurrentWorkDir; }

bool hasWorkDir() { return !CurrentWorkDir.empty(); }

std::optional<std::string> getWorkDirOpt() {
  if (!hasWorkDir()) {
    return std::nullopt;
  }
  return CurrentWorkDir;
}

void appendWorkDirLog(llvm::StringRef fileName, llvm::StringRef content) {
  auto WorkDir = getWorkDirOpt();
  if (!WorkDir) {
    return;
  }

  std::error_code EC = llvm::sys::fs::create_directories(*WorkDir);
  if (EC) {
    return;
  }

  auto Path = join(*WorkDir, fileName.str());
  llvm::raw_fd_ostream OS(
      Path, EC, llvm::sys::fs::OF_Text | llvm::sys::fs::OF_Append);
  if (EC) {
    return;
  }

  OS << content;
}

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

std::string readFileToString(const char *path) {
  std::ifstream t(path);
  std::stringstream buffer;
  buffer << t.rdbuf();
  return buffer.str();
}

std::string join(std::string path, std::string elem) {
  return path.back() == '/' ? path + elem : path + "/" + elem;
}

[[nodiscard]] bool equal(llvm::StringRef S1, const char* S2) {
  return S1 == S2;
}

[[nodiscard]] bool printModule(llvm::Module &M, const char *path) {
  std::error_code EC;
  llvm::raw_fd_ostream os(path, EC);
  if (EC) {
    std::cerr << "Cannot open output file: " << path << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
  M.print(os, nullptr);
  return true;
}

[[nodiscard]] std::string toString(const clang::QualType &QT) {
  std::string Out;
  llvm::raw_string_ostream SS(Out);
  clang::ASTDumper Dumper(llvm::errs(), /*ShowColors=*/false);
  Dumper.Visit(QT);
  return Out;
}

} // namespace notdec
