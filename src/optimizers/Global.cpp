#include <llvm/ADT/StringMap.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/GlobalVariable.h>

#include <map>
using namespace llvm;
//TOOD 如何化简临时变量为实际的esp偏移量
// 如果存的是动态的指针怎么办
namespace notdec::frontend::optimizers
{

  struct stack : PassInfoMixin<stack>
  {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &)
    {
      std::map<Value *, Value *> map;  
      IRBuilder<> builder(M.getContext());
      for(Function &F :M){

        for (BasicBlock &BB : F){
          for (Instruction &inst : BB){
            //errs() << "(stack) visiting " << inst << "\n";
            if (inst.getName().str().find("calcOffset") != std::string::npos){
                Value *ptr = inst.getOperand(0);
                Value *varOffset = inst.getOperand(1);
                if(dyn_cast<ConstantInt>(ptr)->getSExtValue() != 0) //discard stack acces
                  continue;
                  
                  
                  

                Instruction *nextInst = inst.getNextNode()->getNextNode();
                Type *varType;
                Instruction *memInst;
                if(nextInst->isCast()){ //if type isn't i8*
                  varType = nextInst->getType()->getPointerElementType();
                  memInst = nextInst->getNextNode();
                }
                else{
                  varType = Type::getInt8Ty(F.getContext());
                  memInst = nextInst;
                }

                if (map.find(varOffset) == map.end())
                  map[varOffset] = builder.CreateAlloca(varType);



              if(varType != map[varOffset]->getType()){
                if(1){}//判断类型 是否需要扩展变量大小 需要的话则修改alloc 并对use进行bitcast
              }
              if(LoadInst* load = dyn_cast<LoadInst>(memInst))
                load->setOperand(0, map[varOffset]);
              else if (StoreInst* store = dyn_cast<StoreInst>(memInst))
                store->setOperand(1, map[varOffset]);
              else
                assert(false && "not a load or store instruction");
          


            }
          }
        }
      }
    

    return PreservedAnalyses::all();
  }
  };}


