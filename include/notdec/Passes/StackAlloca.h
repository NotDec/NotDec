#ifndef _NOTDEC_PASSES_STACK_ALLOCA_H_
#define _NOTDEC_PASSES_STACK_ALLOCA_H_

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <vector>

namespace notdec {

using namespace llvm;
extern const char *KIND_STACK_DIRECTION;
extern const char *KIND_STACK_DIRECTION_NEGATIVE;

struct LinearAllocationRecovery : PassInfoMixin<LinearAllocationRecovery> {
  static std::string offsetStr(int64_t offset) {
    return offset < 0 ? "N" + std::to_string(-offset) : std::to_string(offset);
  }

  static void matchDynamicAllocas(Function &F, Value *SP, Instruction *LoadSP,
                                  Instruction *add_load_sp, Value *space,
                                  bool isGrowNegative);

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
};

// struct Range {
//   int64_t start;
//   int64_t size;
// };

// struct RangeSpliting {
//   std::vector<Range> ranges;

//   std::string str() {
//     std::string s;
//     for (auto r : ranges) {
//       s += "(" + std::to_string(r.start) + "," +
//            std::to_string(r.start + r.size) + ") ";
//     }
//     return "{" + s + "}";
//   }

//   int64_t last() {
//     if (ranges.empty()) {
//       return 0;
//     }
//     return ranges.back().start + ranges.back().size;
//   }
//   void splitAt(int64_t offset) {
//     bool empty = ranges.empty();
//     // out of the range, so extend the range.
//     int64_t range_start = empty ? 0 : ranges[0].start;
//     if (offset < range_start) {
//       Range r = {offset, range_start - offset};
//       ranges.insert(ranges.begin(), r);
//       return;
//     }
//     for (long i = 0; i < ranges.size(); i++) {
//       if (offset == ranges[i].start) {
//         return;
//       }
//       if (offset > ranges[i].start &&
//           offset < ranges[i].start + ranges[i].size) {
//         // split the range
//         Range r1 = {ranges[i].start, offset - ranges[i].start};
//         Range r2 = {offset, ranges[i].size - r1.size};
//         ranges[i] = r1;
//         ranges.insert(ranges.begin() + i + 1, r2);
//         return;
//       }
//       if (offset == ranges[i].start + ranges[i].size) {
//         return;
//       }
//     }
//     int64_t range_end = empty ? 0 : ranges.back().start + ranges.back().size;
//     // out of the range, so extend the range.
//     if (offset > range_end) {
//       Range r = {range_end, offset - range_end};
//       ranges.push_back(r);
//       return;
//     }
//   }
// };

} // namespace notdec

#endif
