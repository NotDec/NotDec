#include "notdec/TypeRecovery/mlsub/PAPath.h"

#include <algorithm>
#include <sstream>
#include <tuple>
#include <utility>

namespace notdec::mlsub {

namespace {

const PAPathAtom *getAtom(const PAPathElem &Elem) {
  return std::get_if<PAPathAtom>(&Elem);
}

const PAPathStar *getStar(const PAPathElem &Elem) {
  return std::get_if<PAPathStar>(&Elem);
}

PAPathStar makeStar(llvm::ArrayRef<PAPathAtom> Body) {
  return PAPathStar{.Body = std::vector<PAPathAtom>(Body.begin(), Body.end()),
                    .Summary = summarizeStarBody(Body)};
}

std::string formatPAPathBody(llvm::ArrayRef<PAPathAtom> Body) {
  std::ostringstream OS;
  for (size_t I = 0; I < Body.size(); ++I) {
    if (I != 0) {
      OS << ".";
    }
    OS << formatPAPathAtom(Body[I]);
  }
  return OS.str();
}

std::string formatDetailedPAPathBody(llvm::ArrayRef<PAPathAtom> Body) {
  std::ostringstream OS;
  for (size_t I = 0; I < Body.size(); ++I) {
    if (I != 0) {
      OS << ".";
    }
    OS << formatDetailedPAPathAtom(Body[I]);
  }
  return OS.str();
}

bool absorbTrailingBodyIntoLastStar(PAPath &Path) {
  if (Path.Elems.empty()) {
    return false;
  }
  size_t StarIndex = Path.Elems.size();
  for (size_t I = Path.Elems.size(); I > 0; --I) {
    if (std::holds_alternative<PAPathStar>(Path.Elems[I - 1])) {
      StarIndex = I - 1;
      break;
    }
  }
  if (StarIndex == Path.Elems.size()) {
    return false;
  }

  const auto &Star = std::get<PAPathStar>(Path.Elems[StarIndex]);
  if (Star.Body.empty() || Path.Elems.size() <= StarIndex + Star.Body.size()) {
    return false;
  }

  size_t SuffixStart = Path.Elems.size() - Star.Body.size();
  if (SuffixStart <= StarIndex) {
    return false;
  }
  for (size_t I = 0; I < Star.Body.size(); ++I) {
    const auto *Atom = getAtom(Path.Elems[SuffixStart + I]);
    if (Atom == nullptr || !(*Atom == Star.Body[I])) {
      return false;
    }
  }

  Path.Elems.erase(Path.Elems.begin() + SuffixStart, Path.Elems.end());
  return true;
}

bool collapseRepeatedAtomSuffix(PAPath &Path) {
  if (Path.Elems.size() < 2) {
    return false;
  }

  size_t SuffixStart = 0;
  for (size_t I = Path.Elems.size(); I > 0; --I) {
    if (std::holds_alternative<PAPathStar>(Path.Elems[I - 1])) {
      SuffixStart = I;
      break;
    }
  }

  size_t AtomCount = Path.Elems.size() - SuffixStart;
  for (size_t BodyLen = 1; BodyLen * 2 <= AtomCount; ++BodyLen) {
    size_t FirstStart = Path.Elems.size() - BodyLen * 2;
    bool Matches = true;
    for (size_t I = 0; I < BodyLen; ++I) {
      const auto *LHS = getAtom(Path.Elems[FirstStart + I]);
      const auto *RHS = getAtom(Path.Elems[FirstStart + BodyLen + I]);
      if (LHS == nullptr || RHS == nullptr || !(*LHS == *RHS)) {
        Matches = false;
        break;
      }
    }
    if (!Matches) {
      continue;
    }

    std::vector<PAPathAtom> Body;
    Body.reserve(BodyLen);
    for (size_t I = 0; I < BodyLen; ++I) {
      Body.push_back(std::get<PAPathAtom>(Path.Elems[FirstStart + I]));
    }

    Path.Elems.erase(Path.Elems.begin() + FirstStart, Path.Elems.end());
    Path.Elems.push_back(makeStar(Body));
    return true;
  }

  return false;
}

bool mergeAdjacentEqualStars(PAPath &Path) {
  bool Changed = false;
  for (size_t I = 1; I < Path.Elems.size();) {
    const auto *Prev = getStar(Path.Elems[I - 1]);
    const auto *Cur = getStar(Path.Elems[I]);
    if (Prev == nullptr || Cur == nullptr || !(*Prev == *Cur)) {
      ++I;
      continue;
    }
    Path.Elems.erase(Path.Elems.begin() + I);
    Changed = true;
  }
  return Changed;
}

} // namespace

bool operator==(const PAFieldTag &LHS, const PAFieldTag &RHS) {
  return std::tie(LHS.K, LHS.ContextId, LHS.VirtualId, LHS.Value) ==
         std::tie(RHS.K, RHS.ContextId, RHS.VirtualId, RHS.Value);
}

bool operator<(const PAFieldTag &LHS, const PAFieldTag &RHS) {
  return std::tie(LHS.K, LHS.ContextId, LHS.VirtualId, LHS.Value) <
         std::tie(RHS.K, RHS.ContextId, RHS.VirtualId, RHS.Value);
}

bool operator==(const PAPathAtom &LHS, const PAPathAtom &RHS) {
  return std::tie(LHS.Tag, LHS.Offset) == std::tie(RHS.Tag, RHS.Offset);
}

bool operator<(const PAPathAtom &LHS, const PAPathAtom &RHS) {
  return std::tie(LHS.Tag, LHS.Offset) < std::tie(RHS.Tag, RHS.Offset);
}

bool operator==(const PAPathStar &LHS, const PAPathStar &RHS) {
  return std::tie(LHS.Body, LHS.Summary) == std::tie(RHS.Body, RHS.Summary);
}

bool operator<(const PAPathStar &LHS, const PAPathStar &RHS) {
  return std::tie(LHS.Body, LHS.Summary) < std::tie(RHS.Body, RHS.Summary);
}

bool operator==(const PAPathElem &LHS, const PAPathElem &RHS) {
  if (LHS.index() != RHS.index()) {
    return false;
  }
  if (const auto *Atom = std::get_if<PAPathAtom>(&LHS)) {
    return *Atom == std::get<PAPathAtom>(RHS);
  }
  return std::get<PAPathStar>(LHS) == std::get<PAPathStar>(RHS);
}

bool operator<(const PAPathElem &LHS, const PAPathElem &RHS) {
  if (LHS.index() != RHS.index()) {
    return LHS.index() < RHS.index();
  }
  if (const auto *Atom = std::get_if<PAPathAtom>(&LHS)) {
    return *Atom < std::get<PAPathAtom>(RHS);
  }
  return std::get<PAPathStar>(LHS) < std::get<PAPathStar>(RHS);
}

bool operator==(const PAPath &LHS, const PAPath &RHS) {
  return LHS.Elems == RHS.Elems;
}

bool operator<(const PAPath &LHS, const PAPath &RHS) {
  return std::lexicographical_compare(LHS.Elems.begin(), LHS.Elems.end(),
                                      RHS.Elems.begin(), RHS.Elems.end());
}

std::string formatPAFieldTag(const PAFieldTag &Tag) {
  if (Tag.K == PAFieldTag::Kind::IRPtrAdd) {
    return "ir:" + toStableString(Tag.Value);
  }
  return "virt:" + std::to_string(Tag.ContextId) + ":" +
         std::to_string(Tag.VirtualId);
}

std::string formatPAPathAtom(const PAPathAtom &Atom) { return Atom.Offset.str(); }

std::string formatDetailedPAPathAtom(const PAPathAtom &Atom) {
  return Atom.Offset.str() + "{" + formatPAFieldTag(Atom.Tag) + "}";
}

std::string formatPAPathElem(const PAPathElem &Elem) {
  if (const auto *Atom = getAtom(Elem)) {
    return formatPAPathAtom(*Atom);
  }
  const auto &Star = std::get<PAPathStar>(Elem);
  return "(" + formatPAPathBody(Star.Body) + ")*";
}

std::string formatDetailedPAPathElem(const PAPathElem &Elem) {
  if (const auto *Atom = getAtom(Elem)) {
    return formatDetailedPAPathAtom(*Atom);
  }
  const auto &Star = std::get<PAPathStar>(Elem);
  return "(" + formatDetailedPAPathBody(Star.Body) + ")*";
}

std::string formatPAPath(const PAPath &Path) {
  std::ostringstream OS;
  for (size_t I = 0; I < Path.Elems.size(); ++I) {
    if (I != 0) {
      OS << ".";
    }
    OS << formatPAPathElem(Path.Elems[I]);
  }
  return OS.str();
}

std::string formatDetailedPAPath(const PAPath &Path) {
  std::ostringstream OS;
  for (size_t I = 0; I < Path.Elems.size(); ++I) {
    if (I != 0) {
      OS << ".";
    }
    OS << formatDetailedPAPathElem(Path.Elems[I]);
  }
  return OS.str();
}

OffsetRange summarizeStarBody(llvm::ArrayRef<PAPathAtom> Body) {
  OffsetRange Sum;
  for (const auto &Atom : Body) {
    Sum = Sum + Atom.Offset;
  }
  return Sum.mulx();
}

PAPath appendAndNormalize(PAPath Path, PAPathAtom Atom) {
  Path.Elems.push_back(std::move(Atom));
  return normalizePAPath(std::move(Path));
}

PAPath normalizePAPath(PAPath Path) {
  bool Changed = true;
  while (Changed) {
    Changed = false;
    Changed |= absorbTrailingBodyIntoLastStar(Path);
    if (Changed) {
      continue;
    }
    Changed |= collapseRepeatedAtomSuffix(Path);
    if (Changed) {
      continue;
    }
    Changed |= mergeAdjacentEqualStars(Path);
  }
  return Path;
}

} // namespace notdec::mlsub
