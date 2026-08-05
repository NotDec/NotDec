#include "Passes/DangerousTypePatternScan.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

using namespace llvm;

namespace notdec {
namespace {

// 无类型 IR 的指针偏移规范化。
//
// 源码 IR、wasm 转出的 IR（i32 指针 + inttoptr）和二进制转出的 IR
// （i8 GEP）都没有可靠的 struct 类型，所以这里只把指针值拆成
// (base, 常量偏移)：
//   - GEP i8/ptr, ptr %p, i64 C        -> (%p, C)
//   - add i32/i64/ptr %p, C            -> (%p, C)
//   - inttoptr i32 %addr to ptr        -> 递归拆 %addr（wasm32）
//   - bitcast                          -> 递归拆输入
// 拆不出来的就当作 base 本身。
struct NormPtr {
  Value *Base = nullptr;
  int64_t Offset = 0;
};

bool isByteGEP(const GetElementPtrInst &GEP) {
  Type *Src = GEP.getSourceElementType();
  return Src->isIntegerTy(8) || Src->isPointerTy();
}

NormPtr normalizePointer(Value *V, const DataLayout &DL, unsigned Depth = 0) {
  if (Depth > 6) {
    return NormPtr{V, 0};
  }
  if (auto *IT = dyn_cast<IntToPtrInst>(V)) {
    // wasm32: 指针先算成 i32 再 inttoptr
    return normalizePointer(IT->getOperand(0), DL, Depth + 1);
  }
  if (auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
    // 只处理"单索引常量"的字节/指针 GEP（无类型 IR 的主流形式）。
    if (GEP->getNumIndices() == 1 && isByteGEP(*GEP)) {
      auto *C = dyn_cast<ConstantInt>(GEP->getOperand(1));
      uint64_t Scale = 1;
      if (GEP->getSourceElementType()->isPointerTy()) {
        Scale = DL.getPointerTypeSizeInBits(
                    cast<PointerType>(GEP->getSourceElementType())) /
                8;
      }
      if (C && Scale != 0) {
        NormPtr P = normalizePointer(GEP->getPointerOperand(), DL, Depth + 1);
        P.Offset += static_cast<int64_t>(C->getZExtValue()) * Scale;
        return P;
      }
    }
    // 多索引常量 GEP（如 wasm 全局内存 [N x i8]）不好在无类型下算大小，
    // 保守退回 base 本身。
    return NormPtr{V, 0};
  }
  if (auto *Add = dyn_cast<BinaryOperator>(V)) {
    if (Add->getOpcode() == Instruction::Add) {
      if (auto *C = dyn_cast<ConstantInt>(Add->getOperand(1))) {
        NormPtr P = normalizePointer(Add->getOperand(0), DL, Depth + 1);
        P.Offset += static_cast<int64_t>(C->getZExtValue());
        return P;
      }
    }
  }
  if (auto *BC = dyn_cast<BitCastInst>(V)) {
    return normalizePointer(BC->getOperand(0), DL, Depth + 1);
  }
  return NormPtr{V, 0};
}

// 在偏移集合里找最长等差子序列，返回 (项数, 间距)。unroll 的大对象数组
// 展开会留下等差偏移（如 resp_allocate 的 151, 1335, 2519, ...，间距
// 1184）。排序后对高频相邻差做贪心链统计即可，warning 启发式不需要精确。
std::pair<unsigned, int64_t>
longestArithmeticProgression(std::vector<int64_t> Offsets) {
  std::sort(Offsets.begin(), Offsets.end());
  Offsets.erase(std::unique(Offsets.begin(), Offsets.end()), Offsets.end());
  if (Offsets.size() < 2) {
    return {0, 0};
  }
  std::map<int64_t, unsigned> DiffFreq;
  for (size_t I = 1; I < Offsets.size(); ++I) {
    int64_t D = Offsets[I] - Offsets[I - 1];
    if (D > 0) {
      ++DiffFreq[D];
    }
  }
  std::vector<std::pair<int64_t, unsigned>> Diffs(DiffFreq.begin(),
                                                  DiffFreq.end());
  std::sort(Diffs.begin(), Diffs.end(),
            [](auto &A, auto &B) { return A.second > B.second; });

  unsigned BestCount = 0;
  int64_t BestStride = 0;
  for (auto &[Stride, _Freq] : Diffs) {
    if (Diffs.size() > 24 && BestCount >= 8) {
      break;
    }
    unsigned Chain = 0;
    unsigned BestForStride = 0;
    int64_t Prev = 0;
    bool HasPrev = false;
    for (int64_t O : Offsets) {
      if (HasPrev && O - Prev == Stride) {
        ++Chain;
      } else {
        Chain = 1;
      }
      BestForStride = std::max(BestForStride, Chain);
      Prev = O;
      HasPrev = true;
    }
    if (BestForStride > BestCount) {
      BestCount = BestForStride;
      BestStride = Stride;
    }
  }
  return {BestCount, BestStride};
}

// 无类型的"池分配挂链表"形状：把某个 call 的返回值 store 到
// load 出来的对象的偏移处（load -> add/GEP/inttoptr -> store call result）。
// 例如 resp_start：
//   %call = call resp_allocate(%c)
//   %5 = load ptr, c->resp
//   %next = getelementptr i8, %5, 8
//   store ptr %call, %next
// 这是"对象从池中取出后挂进另一个结构（链表/树）"，意味着该对象类型是
// 递归共享类型，类型推理里会被反复含折叠展开。
//
// 返回被挂链的函数集合（上面例子里是 resp_allocate）：只有这些被调用方
// 的数组展开才带递归权重，避免把同模块里无关的大数组遍历（如 crc32 表
// 初始化）也误判成高危。
void collectPoolLinkCallees(Module &M, const DataLayout &DL,
                            DenseSet<Function *> &LinkCallees) {
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *SI = dyn_cast<StoreInst>(&I);
        if (!SI) {
          continue;
        }
        auto *CI = dyn_cast<CallInst>(SI->getValueOperand());
        if (!CI) {
          continue;
        }
        NormPtr P = normalizePointer(SI->getPointerOperand(), DL);
        // store 的目标是"load 出来的对象的偏移"
        if (isa<LoadInst>(P.Base) && P.Offset != 0) {
          if (Function *Callee = CI->getCalledFunction()) {
            LinkCallees.insert(Callee);
          }
        }
      }
    }
  }
}

// 收集所有 load/store/memset/memcpy 的指针操作数，拆成 (base, 常量偏移)，
// 按 (函数, base) 分组。base 是任意 Value（SSA 或全局变量），不依赖类型。
void collectAccessOffsets(
    Module &M, const DataLayout &DL,
    DenseMap<Function *, std::map<Value *, std::vector<int64_t>>> &PerFunc) {
  auto VisitPtr = [&](Function *F, Value *Ptr) {
    NormPtr P = normalizePointer(Ptr, DL);
    if (P.Base) {
      PerFunc[F][P.Base].push_back(P.Offset);
    }
  };
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *LI = dyn_cast<LoadInst>(&I)) {
          VisitPtr(&F, LI->getPointerOperand());
        } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
          VisitPtr(&F, SI->getPointerOperand());
        } else if (auto *MI = dyn_cast<MemSetInst>(&I)) {
          VisitPtr(&F, MI->getArgOperand(0));
        } else if (auto *MC = dyn_cast<MemCpyInst>(&I)) {
          VisitPtr(&F, MC->getArgOperand(0));
          VisitPtr(&F, MC->getArgOperand(1));
        }
      }
    }
  }
}

} // namespace

PreservedAnalyses DangerousTypePatternScan::run(Module &M,
                                                ModuleAnalysisManager &AM) {
  const DataLayout &DL = M.getDataLayout();

  // 1. 无类型偏移分组。
  DenseMap<Function *, std::map<Value *, std::vector<int64_t>>> PerFunc;
  collectAccessOffsets(M, DL, PerFunc);

  // 2. 递归/共享权重：池分配挂链表模式（无类型形状检测）。
  DenseSet<Function *> LinkCallees;
  collectPoolLinkCallees(M, DL, LinkCallees);

  // 3. 分数：每个函数对每个 base 找等差偏移序列，count*stride 累加；
  //    命中挂链模式时全局权重 ×3。
  uint64_t Threshold = 10000;
  if (const char *Env = std::getenv("NOTDEC_DANGEROUS_SCAN_THRESHOLD")) {
    Threshold = std::strtoull(Env, nullptr, 10);
  }
  bool WarnedAny = false;
  for (auto &Entry : PerFunc) {
    Function *F = Entry.first;
    uint64_t Score = 0;
    bool HitAP = false;
    std::string Details;
    for (auto &BaseEntry : Entry.second) {
      auto [Count, Stride] = longestArithmeticProgression(BaseEntry.second);
      if (Count >= 8 && Stride >= 128) {
        HitAP = true;
        uint64_t Part = static_cast<uint64_t>(Count) * Stride;
        Score += Part;
        if (!Details.empty()) {
          Details += ", ";
        }
        Details += "base=%" +
                   (BaseEntry.first->hasName()
                        ? BaseEntry.first->getName().str()
                        : "<anon>") +
                   " ap=" + std::to_string(Count) + "x" +
                   std::to_string(Stride);
      }
    }
    if (!HitAP) {
      continue;
    }
    bool IsLinkCallee = LinkCallees.contains(F);
    if (IsLinkCallee) {
      Score *= 3;
    }
    // 递归命中（挂链）且分数超阈值是高危；无递归证据的大数组遍历要到
    // 10 倍阈值才提示，避免噪音（实测线程表遍历不爆）。
    if (Score >= Threshold && IsLinkCallee) {
      WarnedAny = true;
      errs() << "Warning: function " << F->getName()
             << " may blow up type inference (score=" << Score
             << ", recursive struct array): " << Details << "\n";
    } else if (Score >= Threshold * 10) {
      WarnedAny = true;
      errs() << "Note: function " << F->getName()
             << " has large struct array access (score=" << Score
             << ", no recursive evidence): " << Details << "\n";
    }
  }
  if (WarnedAny) {
    errs() << "DangerousTypePatternScan: set "
              "NOTDEC_DANGEROUS_SCAN_THRESHOLD to tune warning threshold.\n";
  }
  return PreservedAnalyses::all();
}

} // namespace notdec
