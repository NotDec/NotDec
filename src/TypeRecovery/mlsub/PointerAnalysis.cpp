#include "notdec/TypeRecovery/mlsub/PointerAnalysis.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <utility>

#include "binarysub/binarysub-core.h"

namespace notdec::mlsub {

namespace {

PAFieldTag makeIRPtrAddTag(ExtValuePtr Value) {
  return PAFieldTag{.K = PAFieldTag::Kind::IRPtrAdd, .Value = Value};
}

struct PASolveReasonStats {
  std::uint64_t Attempts = 0;
  std::uint64_t Inserted = 0;
};

/// This snapshot is intentionally small and aggregated. The goal is to make
/// `binarysub-trace.log` show whether PA is growing many objects, whether paths
/// are still getting long, and which slot owns the biggest points-to set.
struct PASolveSnapshot {
  std::size_t Slots = 0;
  std::size_t Locs = 0;
  std::size_t MaxSetSize = 0;
  std::string MaxSetSlot;
  std::size_t MaxPathElems = 0;
  std::string MaxPathLoc;
  std::size_t StarLocs = 0;
  std::size_t PlainLocs = 0;
  std::size_t StarElems = 0;
  std::vector<std::pair<std::size_t, std::string>> TopSlots;
};

std::size_t countPathStars(const PAPath &Path) {
  std::size_t Count = 0;
  for (const auto &Elem : Path.Elems) {
    if (std::holds_alternative<PAPathStar>(Elem)) {
      ++Count;
    }
  }
  return Count;
}

PASolveSnapshot collectPASolveSnapshot(
    const std::map<PointerSlotKey, std::set<MemoryLocKey>> &PointsTo) {
  PASolveSnapshot Snapshot;
  Snapshot.Slots = PointsTo.size();

  for (const auto &[Slot, Locs] : PointsTo) {
    Snapshot.Locs += Locs.size();
    if (Locs.size() > Snapshot.MaxSetSize) {
      Snapshot.MaxSetSize = Locs.size();
      Snapshot.MaxSetSlot = formatPointerSlotKey(Slot);
    }
    Snapshot.TopSlots.emplace_back(Locs.size(), formatPointerSlotKey(Slot));

    for (const auto &Loc : Locs) {
      Snapshot.MaxPathElems =
          std::max(Snapshot.MaxPathElems, Loc.Path.Elems.size());
      if (Loc.Path.Elems.size() == Snapshot.MaxPathElems) {
        Snapshot.MaxPathLoc = formatMemoryLocKey(Loc);
      }
      auto StarCount = countPathStars(Loc.Path);
      Snapshot.StarElems += StarCount;
      if (StarCount == 0) {
        ++Snapshot.PlainLocs;
      } else {
        ++Snapshot.StarLocs;
      }
    }
  }

  std::sort(Snapshot.TopSlots.begin(), Snapshot.TopSlots.end(),
            [](const auto &LHS, const auto &RHS) {
              if (LHS.first != RHS.first) {
                return LHS.first > RHS.first;
              }
              return LHS.second < RHS.second;
            });
  if (Snapshot.TopSlots.size() > 3) {
    Snapshot.TopSlots.resize(3);
  }
  return Snapshot;
}

std::string formatPASolveSnapshot(const PASolveSnapshot &Snapshot) {
  std::ostringstream OS;
  OS << "slots=" << Snapshot.Slots << " locs=" << Snapshot.Locs
     << " plain_locs=" << Snapshot.PlainLocs
     << " star_locs=" << Snapshot.StarLocs
     << " star_elems=" << Snapshot.StarElems
     << " max_set=" << Snapshot.MaxSetSize;
  if (!Snapshot.MaxSetSlot.empty()) {
    OS << " max_slot=" << Snapshot.MaxSetSlot;
  }
  OS << " max_path_elems=" << Snapshot.MaxPathElems;
  if (!Snapshot.MaxPathLoc.empty()) {
    OS << " max_path_loc=" << Snapshot.MaxPathLoc;
  }
  if (!Snapshot.TopSlots.empty()) {
    OS << " top_slots=";
    for (std::size_t I = 0; I < Snapshot.TopSlots.size(); ++I) {
      if (I != 0) {
        OS << ",";
      }
      OS << Snapshot.TopSlots[I].second << ":" << Snapshot.TopSlots[I].first;
    }
  }
  return OS.str();
}

std::string formatPAReasonStats(llvm::StringRef Name,
                                const PASolveReasonStats &Stats) {
  std::ostringstream OS;
  OS << Name.str() << "{attempts=" << Stats.Attempts
     << ",inserted=" << Stats.Inserted
     << ",duplicates=" << (Stats.Attempts - Stats.Inserted) << "}";
  return OS.str();
}

std::string formatPADuplicateSamples(
    const std::vector<PointerAnalysis::DuplicateSample> &Samples,
    std::uint64_t Overflow) {
  if (Samples.empty() && Overflow == 0) {
    return "";
  }

  std::ostringstream OS;
  OS << " dup_samples=";
  bool NeedComma = false;
  for (const auto &Sample : Samples) {
    if (NeedComma) {
      OS << ",";
    }
    NeedComma = true;
    OS << Sample.Reason << "@"
       << formatPointerSlotKey(Sample.SlotKey) << "->"
       << formatMemoryLocKey(Sample.Loc) << ":" << Sample.Count;
  }
  if (Overflow != 0) {
    if (NeedComma) {
      OS << ",";
    }
    OS << "other:" << Overflow;
  }
  return OS.str();
}

} // namespace

std::string formatMemoryLocKey(const MemoryLocKey &Loc) {
  std::ostringstream OS;
  OS << toStableString(Loc.Root);
  auto Path = formatDetailedPAPath(Loc.Path);
  if (!Path.empty()) {
    OS << "." << Path;
  }
  if (Loc.BitSize != 0) {
    OS << "[" << Loc.BitSize << "]";
  }
  return OS.str();
}

std::string formatPointerSlotKey(const PointerSlotKey &Slot) {
  if (const auto *Val = std::get_if<ExtValuePtr>(&Slot)) {
    return "value(" + toStableString(*Val) + ")";
  }
  return "memory(" + formatMemoryLocKey(std::get<MemoryLocKey>(Slot)) + ")";
}

bool PointerAnalysis::addPointsTo(Slot SlotKey, MemoryLocKey Loc,
                                  std::string Reason) {
  ++AddStats.Attempts;
  auto &Set = PointsTo[SlotKey];
  auto [It, Inserted] = Set.insert(std::move(Loc));
  if (!Inserted) {
    ++AddStats.Duplicates;
    if (CollectIterDuplicateSamples) {
      recordDuplicateSample(SlotKey, *It, Reason);
    }
    return false;
  }
  ++AddStats.Inserted;
  PointsToDelta Delta{.Slot = std::move(SlotKey), .Loc = *It,
                      .Reason = std::move(Reason)};
  for (const auto &Callback : DeltaCallbacks) {
    Callback(Delta);
  }
  return true;
}

void PointerAnalysis::recordDuplicateSample(const Slot &SlotKey,
                                            const MemoryLocKey &Loc,
                                            llvm::StringRef Reason) {
  static constexpr std::size_t MaxSamples = 4;

  for (auto &Sample : IterDuplicateSamples) {
    if (Sample.Reason == Reason && Sample.SlotKey == SlotKey &&
        Sample.Loc == Loc) {
      ++Sample.Count;
      return;
    }
  }

  if (IterDuplicateSamples.size() >= MaxSamples) {
    ++IterDuplicateSampleOverflow;
    return;
  }

  IterDuplicateSamples.push_back(DuplicateSample{
      .Reason = Reason.str(), .SlotKey = SlotKey, .Loc = Loc, .Count = 1});
}

MemoryLocKey PointerAnalysis::getRootObject(ExtValuePtr Root,
                                            unsigned BitSize) const {
  return MemoryLocKey{.Root = Root, .Path = PAPath{}, .BitSize = BitSize};
}

MemoryLocKey PointerAnalysis::getFieldObject(MemoryLocKey Base,
                                             OffsetRange Field,
                                             PAFieldTag Tag,
                                             unsigned BitSize) const {
  Base.Path = appendAndNormalize(std::move(Base.Path),
                                 PAPathAtom{.Offset = std::move(Field),
                                            .Tag = std::move(Tag)});
  Base.BitSize = BitSize;
  return Base;
}

void PointerAnalysis::addAddrOf(ExtValuePtr Dst, MemoryLocKey Obj) {
  addPointsTo(valueSlot(Dst), std::move(Obj), "addr-of");
}

void PointerAnalysis::addCopy(ExtValuePtr Dst, ExtValuePtr Src) {
  CopyEdges.push_back({valueSlot(Dst), valueSlot(Src)});
}

void PointerAnalysis::addField(ExtValuePtr Dst, ExtValuePtr Base,
                               OffsetRange Field, unsigned BitSize) {
  FieldEdges.push_back(FieldEdge{.Dst = Dst,
                                 .Base = Base,
                                 .Field = Field,
                                 .Tag = makeIRPtrAddTag(Dst),
                                 .BitSize = BitSize});
}

void PointerAnalysis::addLoadPtr(ExtValuePtr Dst, ExtValuePtr Addr) {
  LoadPtrEdges.push_back(LoadPtrEdge{.Dst = Dst, .Addr = Addr});
}

void PointerAnalysis::addStorePtr(ExtValuePtr Addr, ExtValuePtr Src) {
  StorePtrEdges.push_back(StorePtrEdge{.Addr = Addr, .Src = Src});
}

void PointerAnalysis::addPointsToDeltaCallback(
    PointsToDeltaCallback Callback) {
  DeltaCallbacks.push_back(std::move(Callback));
}

bool PointerAnalysis::solve() {
  AddStats = {};
  binarysub::binarysub_trace(
      "[pa:solve-begin] copy_edges=" + std::to_string(CopyEdges.size()) +
      " field_edges=" + std::to_string(FieldEdges.size()) +
      " load_ptr_edges=" + std::to_string(LoadPtrEdges.size()) +
      " store_ptr_edges=" + std::to_string(StorePtrEdges.size()) + " " +
      formatPASolveSnapshot(collectPASolveSnapshot(PointsTo)));

  bool Changed = false;
  bool IterChanged = true;
  std::uint64_t Iteration = 0;
  while (IterChanged) {
    ++Iteration;
    IterChanged = false;
    IterDuplicateSamples.clear();
    IterDuplicateSampleOverflow = 0;
    CollectIterDuplicateSamples = binarysub::binarysub_trace_enabled();
    PASolveReasonStats CopyStats;
    PASolveReasonStats FieldStats;
    PASolveReasonStats LoadPtrStats;
    PASolveReasonStats StorePtrStats;

    for (const auto &[Dst, Src] : CopyEdges) {
      auto It = PointsTo.find(Src);
      if (It == PointsTo.end()) {
        continue;
      }
      for (const auto &Loc : It->second) {
        ++CopyStats.Attempts;
        if (addPointsTo(Dst, Loc, "copy")) {
          ++CopyStats.Inserted;
          IterChanged = true;
        }
      }
    }

    for (const auto &Edge : FieldEdges) {
      auto It = PointsTo.find(valueSlot(Edge.Base));
      if (It == PointsTo.end()) {
        continue;
      }
      for (const auto &BaseLoc : It->second) {
        auto FieldLoc =
            getFieldObject(BaseLoc, Edge.Field, Edge.Tag, Edge.BitSize);
        ++FieldStats.Attempts;
        if (addPointsTo(valueSlot(Edge.Dst), std::move(FieldLoc), "field")) {
          ++FieldStats.Inserted;
          IterChanged = true;
        }
      }
    }

    for (const auto &Edge : LoadPtrEdges) {
      auto AddrIt = PointsTo.find(valueSlot(Edge.Addr));
      if (AddrIt == PointsTo.end()) {
        continue;
      }
      for (const auto &AddrLoc : AddrIt->second) {
        auto MemIt = PointsTo.find(memorySlot(AddrLoc));
        if (MemIt == PointsTo.end()) {
          continue;
        }
        for (const auto &LoadedLoc : MemIt->second) {
          ++LoadPtrStats.Attempts;
          if (addPointsTo(valueSlot(Edge.Dst), LoadedLoc, "load-ptr")) {
            ++LoadPtrStats.Inserted;
            IterChanged = true;
          }
        }
      }
    }

    for (const auto &Edge : StorePtrEdges) {
      auto AddrIt = PointsTo.find(valueSlot(Edge.Addr));
      auto SrcIt = PointsTo.find(valueSlot(Edge.Src));
      if (AddrIt == PointsTo.end() || SrcIt == PointsTo.end()) {
        continue;
      }
      for (const auto &AddrLoc : AddrIt->second) {
        for (const auto &SrcLoc : SrcIt->second) {
          ++StorePtrStats.Attempts;
          if (addPointsTo(memorySlot(AddrLoc), SrcLoc, "store-ptr")) {
            ++StorePtrStats.Inserted;
            IterChanged = true;
          }
        }
      }
    }

    auto Snapshot = collectPASolveSnapshot(PointsTo);
    std::ostringstream IterOS;
    IterOS << "[pa:solve-iter] iter=" << Iteration
           << " changed=" << (IterChanged ? "true" : "false") << " "
           << formatPASolveSnapshot(Snapshot) << " "
           << formatPAReasonStats("copy", CopyStats) << " "
           << formatPAReasonStats("field", FieldStats) << " "
           << formatPAReasonStats("load_ptr", LoadPtrStats) << " "
           << formatPAReasonStats("store_ptr", StorePtrStats)
           << " total_attempts=" << AddStats.Attempts
           << " total_inserted=" << AddStats.Inserted
           << " total_duplicates=" << AddStats.Duplicates
           << formatPADuplicateSamples(IterDuplicateSamples,
                                       IterDuplicateSampleOverflow);
    binarysub::binarysub_trace(IterOS.str());

    Changed |= IterChanged;
  }
  CollectIterDuplicateSamples = false;

  binarysub::binarysub_trace(
      "[pa:solve-end] changed=" + std::string(Changed ? "true" : "false") +
      " iterations=" + std::to_string(Iteration) + " " +
      formatPASolveSnapshot(collectPASolveSnapshot(PointsTo)) +
      " total_attempts=" + std::to_string(AddStats.Attempts) +
      " total_inserted=" + std::to_string(AddStats.Inserted) +
      " total_duplicates=" + std::to_string(AddStats.Duplicates));
  return Changed;
}

} // namespace notdec::mlsub
