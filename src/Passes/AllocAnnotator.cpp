#include "Passes/AllocAnnotator.h"
#include <llvm/IR/Instructions.h>

namespace notdec {

const char *KIND_ALLOC_SIZE = "notdec.alloc_size";

void AllocAnnotator::addSizeMetadata(Instruction *I, uint64_t Size) {
  LLVMContext &Context = I->getContext();
  std::string SizeStr = std::to_string(Size);
  MDString *SizeMD = MDString::get(Context, SizeStr);
  MDNode *Node = MDNode::get(Context, {SizeMD});
  I->setMetadata(KIND_ALLOC_SIZE, Node);
}

PreservedAnalyses AllocAnnotator::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  DataLayout DL = F.getParent()->getDataLayout();

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      // 处理Alloca指令
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        if (!AI->isArrayAllocation()) {
          Type *AllocTy = AI->getAllocatedType();
          uint64_t Size = DL.getTypeAllocSize(AllocTy).getFixedValue();
          if (Size != 0) {
            addSizeMetadata(&I, Size);
          }
        }
      }
      // 处理malloc调用
      else if (auto *CI = dyn_cast<CallInst>(&I)) {
        Function *Callee = CI->getCalledFunction();
        if (Callee && Callee->getName() == "malloc") {
          if (CI->getNumOperands() > 0) {
            Value *SizeArg = CI->getArgOperand(0);
            if (ConstantInt *C = dyn_cast<ConstantInt>(SizeArg)) {
              uint64_t Size = C->getZExtValue();
              addSizeMetadata(&I, Size);
            }
          }
        }
      }
    }
  }

  return PreservedAnalyses::all();
}

} // namespace notdec
