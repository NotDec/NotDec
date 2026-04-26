#include "notdec/TypeRecovery/mlsub/PointerAnalysis.h"

#include <sstream>
#include <utility>

namespace notdec::mlsub {

std::string formatMemoryLocKey(const MemoryLocKey &Loc) {
  std::ostringstream OS;
  OS << toStableString(Loc.Root);
  for (const auto &Field : Loc.Path) {
    OS << "." << Field.str();
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
  auto &Set = PointsTo[SlotKey];
  auto [It, Inserted] = Set.insert(std::move(Loc));
  if (!Inserted) {
    return false;
  }
  PointsToDelta Delta{.Slot = std::move(SlotKey), .Loc = *It,
                      .Reason = std::move(Reason)};
  for (const auto &Callback : DeltaCallbacks) {
    Callback(Delta);
  }
  return true;
}

MemoryLocKey PointerAnalysis::getRootObject(ExtValuePtr Root,
                                            unsigned BitSize) const {
  return MemoryLocKey{.Root = Root, .Path = {}, .BitSize = BitSize};
}

MemoryLocKey PointerAnalysis::getFieldObject(MemoryLocKey Base,
                                             OffsetRange Field,
                                             unsigned BitSize) const {
  Base.Path.push_back(std::move(Field));
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
  FieldEdges.push_back(FieldEdge{.Dst = Dst, .Base = Base, .Field = Field,
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
  bool Changed = false;
  bool IterChanged = true;
  while (IterChanged) {
    IterChanged = false;

    for (const auto &[Dst, Src] : CopyEdges) {
      auto It = PointsTo.find(Src);
      if (It == PointsTo.end()) {
        continue;
      }
      for (const auto &Loc : It->second) {
        IterChanged |= addPointsTo(Dst, Loc, "copy");
      }
    }

    for (const auto &Edge : FieldEdges) {
      auto It = PointsTo.find(valueSlot(Edge.Base));
      if (It == PointsTo.end()) {
        continue;
      }
      for (const auto &BaseLoc : It->second) {
        auto FieldLoc = getFieldObject(BaseLoc, Edge.Field, Edge.BitSize);
        IterChanged |= addPointsTo(valueSlot(Edge.Dst), std::move(FieldLoc),
                                   "field");
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
          IterChanged |= addPointsTo(valueSlot(Edge.Dst), LoadedLoc,
                                     "load-ptr");
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
          IterChanged |= addPointsTo(memorySlot(AddrLoc), SrcLoc,
                                     "store-ptr");
        }
      }
    }

    Changed |= IterChanged;
  }
  return Changed;
}

} // namespace notdec::mlsub
