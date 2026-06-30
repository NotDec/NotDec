#ifndef _NOTDEC_MLSUB_POINTER_ANALYSIS_H_
#define _NOTDEC_MLSUB_POINTER_ANALYSIS_H_

#include <functional>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <llvm/IR/Value.h>

#include "notdec-llvm2c/Interface/ExtValuePtr.h"
#include "binarysub/Range.h"
#include "notdec/TypeRecovery/mlsub/PAPath.h"

namespace notdec::mlsub {

enum class PointerAnalysisMode {
  Original,
  Shadow,
  Replace,
};

/// Explicit memory object identity used by PointerAnalysis.
///
/// Root is the allocation/global/constant-address object. Path keeps every
/// reified ptradd step, including repeated suffix summaries, so loop-carried
/// field traversals can converge without losing field order. BitSize is part
/// of the key because different width accesses to the same path intentionally
/// get separate object-content types.
struct MemoryLocKey {
  ExtValuePtr Root = static_cast<llvm::Value *>(nullptr);
  PAPath Path;
  unsigned BitSize = 0;

  bool operator<(const MemoryLocKey &RHS) const {
    return std::tie(Root, Path, BitSize) <
           std::tie(RHS.Root, RHS.Path, RHS.BitSize);
  }
  bool operator==(const MemoryLocKey &RHS) const {
    return !(*this < RHS) && !(RHS < *this);
  }
};

/// A pointer slot is either an SSA value slot or a memory cell slot.
///
/// Keeping these cases separate prevents mixing an alloca's returned pointer
/// value with the stack object allocated by that alloca.
using PointerSlotKey = std::variant<ExtValuePtr, MemoryLocKey>;

struct PointsToDelta {
  PointerSlotKey Slot;
  MemoryLocKey Loc;
  std::string Reason;
};

class PointerAnalysis {
public:
  using Slot = PointerSlotKey;
  using PointsToDeltaCallback = std::function<void(const PointsToDelta &)>;

  /// One bounded duplicate example in the solve trace. It records which
  /// slot/loc pair was re-inserted, by which propagation reason, and how many
  /// times that exact duplicate showed up in the current iteration.
  struct DuplicateSample {
    std::string Reason;
    Slot SlotKey;
    MemoryLocKey Loc;
    std::uint64_t Count = 0;
  };

private:
  /// These counters are only for aggregated tracing. They let the workdir
  /// trace show whether PA is doing useful inserts or mostly paying comparison
  /// cost on duplicate candidates.
  struct AddPointsToStats {
    std::uint64_t Attempts = 0;
    std::uint64_t Inserted = 0;
    std::uint64_t Duplicates = 0;
  };

  std::map<Slot, std::set<MemoryLocKey>> PointsTo;
  std::vector<std::pair<Slot, Slot>> CopyEdges;

  struct FieldEdge {
    ExtValuePtr Dst;
    ExtValuePtr Base;
    OffsetRange Field;
    PAFieldTag Tag;
    unsigned BitSize = 0;
  };
  std::vector<FieldEdge> FieldEdges;

  struct LoadPtrEdge {
    ExtValuePtr Dst;
    ExtValuePtr Addr;
  };
  std::vector<LoadPtrEdge> LoadPtrEdges;

  struct StorePtrEdge {
    ExtValuePtr Addr;
    ExtValuePtr Src;
  };
  std::vector<StorePtrEdge> StorePtrEdges;

  std::vector<PointsToDeltaCallback> DeltaCallbacks;
  AddPointsToStats AddStats;
  std::vector<DuplicateSample> IterDuplicateSamples;
  std::uint64_t IterDuplicateSampleOverflow = 0;
  bool CollectIterDuplicateSamples = false;

  bool addPointsTo(Slot SlotKey, MemoryLocKey Loc, std::string Reason);
  void recordDuplicateSample(const Slot &SlotKey, const MemoryLocKey &Loc,
                             llvm::StringRef Reason);

public:
  Slot valueSlot(ExtValuePtr Value) const { return Slot{Value}; }
  Slot memorySlot(MemoryLocKey Loc) const { return Slot{Loc}; }

  MemoryLocKey getRootObject(ExtValuePtr Root, unsigned BitSize) const;
  MemoryLocKey getFieldObject(MemoryLocKey Base, OffsetRange Field,
                              PAFieldTag Tag,
                              unsigned BitSize) const;

  void addAddrOf(ExtValuePtr Dst, MemoryLocKey Obj);
  void addCopy(ExtValuePtr Dst, ExtValuePtr Src);
  void addField(ExtValuePtr Dst, ExtValuePtr Base, OffsetRange Field,
                unsigned BitSize);
  void addLoadPtr(ExtValuePtr Dst, ExtValuePtr Addr);
  void addStorePtr(ExtValuePtr Addr, ExtValuePtr Src);
  void addPointsToDeltaCallback(PointsToDeltaCallback Callback);

  const std::map<Slot, std::set<MemoryLocKey>> &pointsTo() const {
    return PointsTo;
  }
  bool solve();
};

std::string formatMemoryLocKey(const MemoryLocKey &Loc);
std::string formatPointerSlotKey(const PointerSlotKey &Slot);

} // namespace notdec::mlsub

#endif
