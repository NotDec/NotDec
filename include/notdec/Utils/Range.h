/* ###
 * IP: NotDec
 *
 */

#ifndef _NOTDEC_UTILS_RANGE_H_
#define _NOTDEC_UTILS_RANGE_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace notdec {

/// \brief Represents one array accesses, the `ax` part in `ax+b`.
///
/// The \e Size is the size of the array element (the `a`), and the \e Limit is
/// the optional limit of the range. If the limit is -1, it means the range is
/// unbounded.
struct ArrayOffset {
  uint64_t Size = 0;
  // 0 represents unbounded
  uint64_t Count = 0;

  ArrayOffset(uint64_t Size = 0, uint64_t Count = 0)
      : Size(Size), Count(Count){};

  bool operator==(const ArrayOffset &rhs) const {
    return std::tie(Size, Count) == std::tie(rhs.Size, rhs.Count);
  }
  bool operator<(const ArrayOffset &rhs) const {
    return std::tie(Size, Count) < std::tie(rhs.Size, rhs.Count);
  }
  ArrayOffset operator*(const ArrayOffset &rhs) const {
    return ArrayOffset(Size * rhs.Size, Count * rhs.Count);
  }
};

/// \brief Represents one accesses to an complex object.
///
/// A complex object access is in the form of `offset + bx + cy + ...`, because:
/// - Complex object is consisted of nested struct and arrays.
/// - If there is a struct nested in a struct, we view the access as if there is
///   only one struct by flattening the inner to outer.
/// - If there is an array nested in a array, we view the access as if there is
///   only one dimension array.
/// - All constant term is merged in the offset initially, so we have only one
/// constant term (offset).
/// - The `bx + cy + ...` part each represents a potential dynamically
/// calculated array index.
///
/// This class represents one possible access, in the form of `offset + bx + cy
/// + ...` where `bx + cy + ...` are the ArrayAccesses.
struct OffsetRange {
  int64_t offset = 0;
  std::vector<ArrayOffset> access;

  bool isZero() const { return offset == 0 && access.size() == 0; }
  bool operator==(const OffsetRange &rhs) const {
    return offset == rhs.offset && access == rhs.access;
  }
  bool operator<(const OffsetRange &rhs) const {
    return std::tie(offset, access) < std::tie(rhs.offset, rhs.access);
  }
  std::string str() const {
    std::string s;
    s += "@" + std::to_string(offset);
    for (auto &a : access) {
      s += "+" + std::to_string(a.Size) + "i";
      if (a.Count != -1) {
        s += "[" + std::to_string(a.Count);
      }
    }
    return s;
  }
  OffsetRange operator+(const OffsetRange &rhs) const {
    OffsetRange ret;
    ret.offset += rhs.offset;
    // merge sort
    auto p1 = access.begin();
    auto p2 = rhs.access.begin();
    while (p1 != access.end() || p2 != rhs.access.end()) {
      if (p1 == access.end()) {
        ret.access.push_back(*p2);
        p2++;
      } else if (p2 == rhs.access.end()) {
        ret.access.push_back(*p1);
        p1++;
      } else if (p1->Size < p2->Size) {
        ret.access.push_back(*p1);
        p1++;
      } else if (p1->Size > p2->Size) {
        ret.access.push_back(*p2);
        p2++;
      } else {
        int64_t Count = 0;
        if (p1->Count > 0 && p2->Count > 0) {
          Count = p1->Count + p2->Count;
        }
        ret.access.push_back(ArrayOffset(p1->Size, Count));
        p1++;
        p2++;
      }
    }
    return ret;
  }
  OffsetRange operator*(const int64_t Rhs) const {
    return *this * OffsetRange{.offset = Rhs};
  }
  OffsetRange operator*(const OffsetRange Rhs) const {
    OffsetRange Ret;
    Ret.offset = offset * Rhs.offset;
    std::map<uint64_t, uint64_t> Size2Limit;
    for (uint64_t i = 0; i < access.size() + 1; i++) {
      for (uint64_t j = 0; j < Rhs.access.size() + 1; j++) {
        if (i == 0 && j == 0) {
          continue;
        }
        ArrayOffset Left = i == 0 ? ArrayOffset(offset, 1) : access[i - 1];
        ArrayOffset Right =
            j == 0 ? ArrayOffset(Rhs.offset, 1) : Rhs.access[j - 1];
        ArrayOffset New = Left * Right;
        if (Size2Limit.find(New.Size) == Size2Limit.end()) {
          Size2Limit[New.Size] = New.Count;
        } else {
          uint64_t Count = 0;
          if (New.Count > 0 && Size2Limit[New.Size] > 0) {
            Count = New.Count + Size2Limit[New.Size];
          }
          Size2Limit[New.Size] = Count;
        }
      }
    }
    return Ret;
  }
};

std::string toString(const OffsetRange &a);

} // namespace notdec

#endif
