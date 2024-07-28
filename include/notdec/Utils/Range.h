/* ###
 * IP: NotDec
 *
 */

#ifndef _NOTDEC_UTILS_RANGE_H_
#define _NOTDEC_UTILS_RANGE_H_

#include <cstdint>
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
  int64_t Limit = -1;

  ArrayOffset(uint64_t Size = 0, int64_t Limit = -1)
      : Size(Size), Limit(Limit){};

  bool operator==(const ArrayOffset &rhs) const {
    return std::tie(Size, Limit) == std::tie(rhs.Size, rhs.Limit);
  }
  bool operator<(const ArrayOffset &rhs) const {
    return std::tie(Size, Limit) < std::tie(rhs.Size, rhs.Limit);
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
  uint64_t offset = 0;
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
      if (a.Limit != -1) {
        s += "[" + std::to_string(a.Limit);
      }
    }
    return s;
  }
};

std::string toString(const OffsetRange &a);

} // namespace notdec

#endif
