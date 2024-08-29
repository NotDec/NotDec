#ifndef _NOTDEC_UTILS_VALUENAMER_H_
#define _NOTDEC_UTILS_VALUENAMER_H_

#include <cstddef>
#include <string>

#include <llvm/ADT/Twine.h>
#include <llvm/IR/Value.h>

namespace notdec {

using llvm::Value;

struct ValueNamer {
protected:
  static size_t ID;

public:
  static size_t getId() { return ID++; }
  static std::string getName(Value &Val,
                             const char *prefix = ValueNamer::DefaultPrefix,
                             bool Unique = false) {
    if (!Val.hasName()) {
      auto Id = ID++;
      Val.setName(prefix + std::to_string(Id));
      return prefix + std::to_string(Id);
    } else if (Unique) {
      auto Id = ID++;
      return prefix + std::to_string(Id) + "_" + Val.getName().str();
    }
    return Val.getName().str();
  }
  static std::string getName(const char *prefix = ValueNamer::DefaultPrefix) {
    return prefix + std::to_string(ID++);
  }
  static const char *DefaultPrefix;
  static const char *FuncPrefix;
  static const char *PhiPrefix;
  static const char *SelectPrefix;
  static const char *NewPrefix;
  static const char *AddPrefix;
  static const char *SubPrefix;
};
} // namespace notdec
#endif
