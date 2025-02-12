#ifndef _NOTDEC_UTILS_VALUENAMER_H_
#define _NOTDEC_UTILS_VALUENAMER_H_

#include <cstddef>
#include <set>
#include <string>

#include <llvm/ADT/Twine.h>
#include <llvm/IR/Value.h>

namespace notdec {

using llvm::Value;

extern std::set<size_t> TraceIds;

struct ValueNamer {
protected:
  static ValueNamer Instance;
  size_t ID = 1;

public:
  size_t getNewId() { return ID++; }
  std::string getNewName(Value &Val,
                         const char *prefix = ValueNamer::DefaultPrefix,
                         bool Unique = false) {
    auto Id = getId();
    if (!Val.hasName()) {
      Val.setName(prefix + std::to_string(Id));
      return prefix + std::to_string(Id);
    } else if (Unique) {
      return prefix + std::to_string(Id) + "_" + Val.getName().str();
    }
    return Val.getName().str();
  }
  std::string getNewName(const char *prefix = ValueNamer::DefaultPrefix) {
    return prefix + std::to_string(getId());
  }
  static size_t getId() { return Instance.getNewId(); }
  static std::string getName(Value &Val,
                             const char *prefix = ValueNamer::DefaultPrefix,
                             bool Unique = false) {
    return Instance.getNewName(Val, prefix, Unique);
  }
  static std::string getName(const char *prefix = ValueNamer::DefaultPrefix) {
    return Instance.getNewName(prefix);
  }
  static const char *DefaultPrefix;
  static const char *FuncPrefix;
  static const char *PhiPrefix;
  static const char *SelectPrefix;
  static const char *NewPrefix;
  static const char *AddPrefix;
  static const char *SubPrefix;
  static const char *StackPrefix;
  static const char *AllocaPrefix;
  static const char *LoadPrefix;
  static const char *StorePrefix;
};
} // namespace notdec
#endif
