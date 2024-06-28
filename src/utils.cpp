
#include <iostream>
#include <llvm/IR/Module.h>

namespace notdec {

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

} // namespace notdec
