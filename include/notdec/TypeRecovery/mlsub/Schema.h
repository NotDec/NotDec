#ifndef _BINARYSUB_SCHEMA_H_
#define _BINARYSUB_SCHEMA_H_

#include "notdec-llvm2c/Interface/Range.h"
#include <cassert>
#include <cstdint>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallVector.h>
#include <string>
#include <variant>

namespace notdec::mlsub {

struct InLabel {
  std::string name;
  // prevent variant to be default constructed.
  InLabel() = delete;
  bool operator<(const InLabel &rhs) const { return name < rhs.name; }
  bool operator==(const InLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct OutLabel {
  std::string name;
  bool operator<(const OutLabel &rhs) const { return name < rhs.name; }
  bool operator==(const OutLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct OffsetLabel {
  OffsetRange range;
  bool operator<(const OffsetLabel &rhs) const { return range < rhs.range; }
  bool operator==(const OffsetLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct LoadLabel {
  uint32_t Size;
  bool operator<(const LoadLabel &rhs) const { return Size < rhs.Size; }
  bool operator==(const LoadLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct StoreLabel {
  uint32_t Size;
  bool operator<(const StoreLabel &rhs) const { return Size < rhs.Size; }
  bool operator==(const StoreLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct One {
  bool operator<(const One &rhs) const { return false; }
  bool operator==(const One &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }
};

struct EdgeLabel {
  std::variant<InLabel, OutLabel, OffsetLabel, LoadLabel, StoreLabel, One> L;
  bool operator<(const EdgeLabel &rhs) const {
    return std::tie(L) < std::tie(rhs.L);
  }
  bool operator==(const EdgeLabel &rhs) const {
    return !(*this < rhs) && !(rhs < *this);
  }

  bool isIn() const { return std::holds_alternative<InLabel>(L); }
  bool isOut() const { return std::holds_alternative<OutLabel>(L); }
  bool isOffset() const { return std::holds_alternative<OffsetLabel>(L); }
  bool isLoad() const { return std::holds_alternative<LoadLabel>(L); }
  bool isStore() const { return std::holds_alternative<StoreLabel>(L); }
  bool isOne() const { return std::holds_alternative<One>(L); }

  template <typename T> T *getAs() { return std::get_if<T>(&L); }
  template <typename T> const T *getAs() const { return std::get_if<T>(&L); }
};

std::string toString(const EdgeLabel &f);

inline StoreLabel toStore(LoadLabel L) { return StoreLabel{.Size = L.Size}; }

inline uint32_t getLoadOrStoreSize(const EdgeLabel &RL) {
  if (auto *LL = RL.getAs<LoadLabel>()) {
    return LL->Size;
  } else if (auto *SL = RL.getAs<StoreLabel>()) {
    return SL->Size;
  } else {
    assert(false && "getLoadOrStoreSize: Not a LoadLabel or StoreLabel");
  }
}

inline bool operator!=(const EdgeLabel &N1, const EdgeLabel &N2) {
  return !(N1 == N2);
}

} // namespace notdec::mlsub

#endif
