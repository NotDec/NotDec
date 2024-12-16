/* ###
 * IP: NotDec
 *
 */

#ifndef _NOTDEC_UTILS_RANGE_H_
#define _NOTDEC_UTILS_RANGE_H_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace notdec {

std::vector<int64_t> PrimeFactors(int64_t n);

/// \brief Represents one array accesses, the `ax` part in `ax+b`.
///
/// The \e Size is the size of the array element (the `a`), and the \e Limit is
/// the optional limit of the range. If the limit is -1, it means the range is
/// unbounded.
struct ArrayOffset {
  uint64_t Size = 0;
  // 0 represents unbounded
  // Unused now, so make it const to prevent error.
  const uint64_t Count = 0;

  ArrayOffset(uint64_t Size = 0, uint64_t Count = 0)
      : Size(Size), Count(Count){};

  // copy constructor
  ArrayOffset(const ArrayOffset &other)
      : Size(other.Size), Count(other.Count) {}

  ArrayOffset &operator=(const ArrayOffset &other) {
    Size = other.Size;
    assert(Count == 0 && "Count should be 0");
    return *this;
  }

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
        s += "[" + std::to_string(a.Count) + "]";
      }
    }
    return s;
  }

  OffsetRange mulx() const {
    OffsetRange ret = *this;
    if (offset == 0) {
      return ret;
    }
    auto New = OffsetRange();
    New.access.push_back(ArrayOffset(offset));
    return ret + New;
  }

  bool has1x() const {
    for (auto &a : access) {
      if (a.Size == 1) {
        return true;
      }
    }
    return false;
  }

  // unary operator -
  OffsetRange operator-() const {
    OffsetRange ret = *this;
    ret.offset = -ret.offset;
    return ret;
  }

  OffsetRange operator+(const OffsetRange &rhs) const;

  OffsetRange operator*(const int64_t Rhs) const {
    return *this * OffsetRange{.offset = Rhs};
  }

  OffsetRange operator*(const OffsetRange Rhs) const;
};

std::string toString(const OffsetRange &a);

} // namespace notdec

#endif
