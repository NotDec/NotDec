#include "Passes/DangerousTypePatternScan.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdlib>
#include <map>
#include <vector>

using namespace llvm;

namespace notdec {
namespace {

// 把 GEP 的 source element type 剥到最内层 struct（支持 [N x struct] 包装）。
StructType *stripToStructType(Type *Ty) {
  while (auto *ArrTy = dyn_cast<ArrayType>(Ty)) {
    Ty = ArrTy->getElementType();
  }
  return dyn_cast<StructType>(Ty);
}

// 最后一个索引是否动态（非常量 0）。静态字段访问的数组索引是常量 0。
bool isDynamicArrayIndex(const GetElementPtrInst &GEP) {
  if (GEP.getNumIndices() == 0) {
    return false;
  }
  Value *Last = GEP.getOperand(GEP.getNumOperands() - 1);
  if (auto *C = dyn_cast<ConstantInt>(Last)) {
    return !C->isZero();
  }
  return true;
}

// 收集"返回 S* 的函数"：函数内 GEP(S) 结果经 phi/bitcast/select 传播后
// 出现在 return 操作数里。memcached 里 resp_allocate 返回 &b->r[x]（GEP
// %struct._mc_resp 的结果），item_alloc 类似，都是池分配器。
void collectPoolReturners(Module &M, StructType *S,
                          DenseSet<Function *> &PoolReturners) {
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    DenseSet<Value *> SStar;
    SmallVector<Value *, 16> Worklist;
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *GEP = dyn_cast<GetElementPtrInst>(&I);
        if (GEP && stripToStructType(GEP->getSourceElementType()) == S) {
          SStar.insert(GEP);
          Worklist.push_back(GEP);
        }
      }
    }
    while (!Worklist.empty()) {
      Value *V = Worklist.pop_back_val();
      for (User *U : V->users()) {
        Value *Out = nullptr;
        if (auto *Phi = dyn_cast<PHINode>(U)) {
          Out = Phi;
        } else if (auto *BC = dyn_cast<BitCastInst>(U)) {
          Out = BC;
        } else if (auto *Sel = dyn_cast<SelectInst>(U)) {
          Out = Sel;
        }
        if (Out && SStar.insert(Out).second) {
          Worklist.push_back(Out);
        }
      }
    }
    for (BasicBlock &BB : F) {
      if (auto *Ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
        if (Ret->getReturnValue() && SStar.contains(Ret->getReturnValue())) {
          PoolReturners.insert(&F);
          break;
        }
      }
    }
  }
}

// 命中"池分配挂链表"模式：调用返回 S* 的函数后，把返回值 store 到
// load 出来的对象的字段里（GEP(i8, load, off)）。
//
// 这是无 DebugInfo 下对"struct 自引用/递归"的近似：对象从池中取出后
// 被挂进另一个结构（链表/树），意味着该结构是递归共享类型，类型推理里
// 会被反复含折叠展开。memcached 的 resp_start 就是：
//   %call = call resp_allocate(%c)          ; 返回 &b->r[x]
//   %5 = load ptr, c->resp                  ; 取链表尾
//   %next = getelementptr i8, %5, 8
//   store ptr %call, %next                  ; 挂到 next 字段
bool hasPoolLinkPattern(Module &M,
                        const DenseSet<Function *> &PoolReturners) {
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *CI = dyn_cast<CallInst>(&I);
        if (!CI) {
          continue;
        }
        Function *Callee = CI->getCalledFunction();
        if (!Callee || !PoolReturners.contains(Callee)) {
          continue;
        }
        for (User *U : CI->users()) {
          auto *SI = dyn_cast<StoreInst>(U);
          if (!SI || SI->getValueOperand() != CI) {
            continue;
          }
          auto *GEP = dyn_cast<GetElementPtrInst>(SI->getPointerOperand());
          if (GEP && isa<LoadInst>(GEP->getPointerOperand())) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

} // namespace

PreservedAnalyses DangerousTypePatternScan::run(Module &M,
                                                ModuleAnalysisManager &AM) {
  const DataLayout &DL = M.getDataLayout();

  // 1. 动态 struct 数组 GEP，按 (函数, struct) 计数。
  DenseMap<Function *, DenseMap<StructType *, uint64_t>> DynCounts;
  DenseSet<StructType *> DynStructs;
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        auto *GEP = dyn_cast<GetElementPtrInst>(&I);
        if (!GEP) {
          continue;
        }
        StructType *ST = stripToStructType(GEP->getSourceElementType());
        if (!ST || !isDynamicArrayIndex(*GEP)) {
          continue;
        }
        ++DynCounts[&F][ST];
        DynStructs.insert(ST);
      }
    }
  }
  if (DynStructs.empty()) {
    return PreservedAnalyses::all();
  }

  // 2. struct 大小，全部来自 IR 类型 + DataLayout，不依赖 DebugInfo。
  DenseMap<StructType *, uint64_t> Sizes;
  for (StructType *ST : DynStructs) {
    if (ST->isOpaque()) {
      Sizes[ST] = 0;
      continue;
    }
    Sizes[ST] = DL.getTypeAllocSize(ST).getFixedValue();
  }

  // 3. 递归近似：struct 是否命中"池分配挂链表"模式。
  DenseSet<StructType *> Recursive;
  for (StructType *ST : DynStructs) {
    DenseSet<Function *> PoolReturners;
    collectPoolReturners(M, ST, PoolReturners);
    if (!PoolReturners.empty() && hasPoolLinkPattern(M, PoolReturners)) {
      Recursive.insert(ST);
    }
  }

  // 4. 分数与 warning。
  uint64_t Threshold = 10000;
  if (const char *Env = std::getenv("NOTDEC_DANGEROUS_SCAN_THRESHOLD")) {
    Threshold = std::strtoull(Env, nullptr, 10);
  }
  bool WarnedAny = false;
  for (auto &Entry : DynCounts) {
    Function *F = Entry.first;
    uint64_t Score = 0;
    bool HasRecursive = false;
    std::string Details;
    for (auto &CountEntry : Entry.second) {
      StructType *ST = CountEntry.first;
      uint64_t N = CountEntry.second;
      uint64_t Weight = Recursive.contains(ST) ? 3 : 1;
      uint64_t Size = Sizes[ST];
      uint64_t Part = N * Size * Weight;
      Score += Part;
      if (Recursive.contains(ST)) {
        HasRecursive = true;
      }
      if (!Details.empty()) {
        Details += ", ";
      }
      Details += (ST->hasName() ? ST->getName().str() : "<anon>");
      Details += " geps=" + std::to_string(N) + " size=" + std::to_string(Size) +
                 " weight=" + std::to_string(Weight);
    }
    // 递归命中的是真正会反复含折叠展开的高危模式（如 resp 链表），低阈值。
    // 无递归证据的大 struct 数组遍历（如全局线程表）实测不爆，分数要到
    // 10 倍阈值才提示，避免噪音。
    if (Score >= Threshold && HasRecursive) {
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
