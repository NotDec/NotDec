#include "Utils/Range.h"

namespace notdec {

std::string toString(const OffsetRange &a) { return a.str(); }

std::vector<int64_t> PrimeFactors(int64_t n) {
  std::vector<int64_t> r;
  for (int64_t i = 2; i * i <= n; i += 1 + (i > 2)) {
    while ((n % i) == 0) {
      r.push_back(i);
      n /= i;
    }
  }
  if (n != 1)
    r.push_back(n);
  return r;
}

OffsetRange OffsetRange::operator+(const OffsetRange &rhs) const {
  OffsetRange ret = *this;
  ret.offset += rhs.offset;
  // merge accesses
  // ignore Count as for now
  std::set<int64_t> Muls;
  for (auto &a : access) {
    Muls.insert(a.Size);
  }
  for (auto &a : rhs.access) {
    Muls.insert(a.Size);
  }
  std::vector<int64_t> MulsV(Muls.begin(), Muls.end());
  // sort the muls
  std::sort(MulsV.begin(), MulsV.end());
  // remove each number that has a factor already in set
  for (size_t i = 0; i < MulsV.size(); i++) {
    bool hasFactor = false;
    for (size_t j = 0; j < i; j++) {
      if (MulsV[i] % MulsV[j] == 0) {
        hasFactor = true;
        break;
      }
    }
    if (!hasFactor) {
      ret.access.push_back(ArrayOffset(MulsV[i]));
    }
  }
  return ret;
}

OffsetRange OffsetRange::operator*(const OffsetRange Rhs) const {
  OffsetRange Ret = *this;
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
  std::vector<int64_t> Keys;
  for (auto &Ent : Size2Limit) {
    Keys.push_back(Ent.first);
  }
  std::sort(Keys.begin(), Keys.end());
  // remove each number that has a factor already in set
  for (size_t i = 0; i < Keys.size(); i++) {
    bool hasFactor = false;
    for (size_t j = 0; j < i; j++) {
      if (Keys[i] % Keys[j] == 0) {
        hasFactor = true;
        break;
      }
    }
    if (!hasFactor) {
      Ret.access.push_back(ArrayOffset(Keys[i]));
    }
  }
  return Ret;
}

} // namespace notdec
