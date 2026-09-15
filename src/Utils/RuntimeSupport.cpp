// Lean run-context and file helpers shared by the CLI and the type-recovery
// pass library.  Keep Clang tooling out of this translation unit so the
// type-recovery library does not need clang-cpp.
#include "Utils/Utils.h"

#include <fstream>
#include <iostream>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <optional>
#include <sstream>
#include <string>

std::string getSuffix(std::string fname) {
  std::size_t ind = fname.find_last_of('.');
  if (ind == std::string::npos) {
    return std::string();
  }
  return fname.substr(ind);
}

namespace notdec {

namespace {
std::string CurrentWorkDir;
// Fast workdir type reports keep stable value ids but omit verbose LLVM value
// strings. Keep the mode beside CurrentWorkDir because those report writers
// are spread across passes and already use this process-wide run context.
bool FastWorkDir = false;
} // namespace

std::string getDefaultWorkDir(const std::string &inputPath) {
  return inputPath + ".notdec";
}

void setWorkDir(std::string path) { CurrentWorkDir = std::move(path); }

void setFastWorkDir(bool enabled) { FastWorkDir = enabled; }

llvm::StringRef getWorkDir() { return CurrentWorkDir; }

bool hasWorkDir() { return !CurrentWorkDir.empty(); }

bool isFastWorkDir() { return FastWorkDir; }

std::optional<std::string> getWorkDirOpt() {
  if (!hasWorkDir()) {
    return std::nullopt;
  }
  return CurrentWorkDir;
}

std::string join(std::string path, std::string elem) {
  return path.back() == '/' ? path + elem : path + "/" + elem;
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

  llvm::raw_fd_ostream OS(join(*WorkDir, fileName.str()), EC,
                          llvm::sys::fs::OF_Text | llvm::sys::fs::OF_Append);
  if (EC) {
    return;
  }

  OS << content;
}

std::string readFileToString(const char *path) {
  std::ifstream input(path);
  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

[[nodiscard]] bool printModule(llvm::Module &M, const char *path) {
  std::error_code EC;
  llvm::raw_fd_ostream OS(path, EC);
  if (EC) {
    std::cerr << "Cannot open output file: " << path << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
  M.print(OS, nullptr);
  return true;
}

} // namespace notdec
