
#include "optimizers/pointer-type-recovery.h"
#include "datalog/fact-generator.h"
#include "optimizers/stack-pointer-finder.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <ostream>
#include <souffle/RamTypes.h>
#include <system_error>
#include <variant>

namespace souffle {
extern "C" {
extern void *__factory_Sf_pointer_main_instance;
}
} // namespace souffle

// Force linking of the main instance of the program
static void *volatile dummy = &souffle::__factory_Sf_pointer_main_instance;

static const bool is_debug = true;
using namespace llvm;

// TODO 重构直接输入facts
namespace notdec::optimizers {

void static inline assert_sizet(Type *ty, Module &M) {
  assert(ty->isIntegerTy(M.getDataLayout().getPointerSizeInBits()));
}

void static inline assert_sizet(Value *val, Module &M) {
  assert_sizet(val->getType(), M);
}

long PointerTypeRecovery::get_ty_or_negative1(llvm::Value *val) {
  if (val2hty.find(val) == val2hty.end()) {
    return -1;
  }
  return val2hty.at(val);
}

long PointerTypeRecovery::get_ty_or_negative1(aval val) {
  if (val2hty.find(val) == val2hty.end()) {
    return -1;
  }
  return val2hty.at(val);
}

void PointerTypeRecovery::fetch_result(datalog::FactGenerator &fg,
                                       souffle::SouffleProgram *prog) {
  using aval = datalog::FactGenerator::aval;
  if (souffle::Relation *rel = prog->getRelation("highType")) {
    souffle::RamUnsigned vid;
    souffle::RamSigned ptr_domain;
    for (auto &output : *rel) {
      output >> vid >> ptr_domain;
      // std::cerr << "vid: " << vid << ", isptr: " << ptr_domain << std::endl;

      auto value_var = fg.get_value_by_id(vid);
      if (ptr_domain == datalog::ARITY_highTypeDomain::Top) {
        if (std::holds_alternative<const llvm::Value *>(value_var)) {
          auto val = const_cast<llvm::Value *>(
              std::get<const llvm::Value *>(value_var));
          llvm::errs() << "Datalog: type conflict(top): " << *val << "\n";
        } else if (std::holds_alternative<aval>(value_var)) {
          auto val = std::get<aval>(value_var);
          llvm::errs() << "Datalog: type conflict(top): " << val.second
                       << " of " << *val.first << "\n";
        }
      }
      val2hty.emplace(value_var, ptr_domain);
    }
  } else {
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Failed to get output relation!\n"
              << std::endl;
    std::abort();
  }
}

PreservedAnalyses PointerTypeRecovery::run(Module &M,
                                           ModuleAnalysisManager &MAM) {
  // supress warning that dummy is unused.
  (void)dummy;
  datalog::FactGenerator fg(M);
  fg.visit_module();

  // 1. find mem/sp, export facts.
  Value *mem = nullptr;
  for (GlobalVariable &gv : M.getGlobalList()) {
    if (gv.getName().equals(MEM_NAME)) {
      mem = &gv;
      break;
    }
  }
  if (mem == nullptr) {
    std::cerr << "ERROR: mem not found!!";
  } else {
    fg.append_fact(datalog::FACT_isMemory,
                   datalog::to_fact_str(fg.get_value_id(mem)));
  }

  // find sp and set isPointer
  auto sp = MAM.getResult<StackPointerFinderAnalysis>(M);
  if (sp.result == nullptr) {
    std::cerr << "ERROR: Stack Pointer not found!!";
  } else {
    fg.append_fact(datalog::FACT_point2Pointer,
                   datalog::to_fact_str(fg.get_value_id(sp.result)));
  }

  // 2. run souffle
  const char *Name = "pointer_main";
  if (souffle::SouffleProgram *prog =
          souffle::ProgramFactory::newInstance(Name)) {
    // create temp dir
    SmallString<128> Path;
    std::error_code EC;
    EC = llvm::sys::fs::createUniqueDirectory(Name, Path);
    if (!EC) {
      // Resolve any symlinks in the new directory.
      std::string UnresolvedPath(Path.str());
      EC = llvm::sys::fs::real_path(UnresolvedPath, Path);
    }
    const char *temp_path = Path.c_str();

    std::cerr << "Souffle: Running on directory " << temp_path << std::endl;
    fg.output_files(temp_path);
    // run the program...
    prog->loadAll(temp_path);
    prog->run();
    prog->printAll(temp_path);

    // read analysis result
    fetch_result(fg, prog);
    std::cerr << "Souffle: Running on directory " << temp_path << std::endl;
    // add sp to result
    val2hty.emplace(sp.result, datalog::ARITY_highTypeDomain::Pointer);

    // clean up
    // delete directory if not debug
    if (!is_debug && !Path.empty()) {
      assert(llvm::sys::fs::remove_directories(Path.str()) == std::errc());
    }
    delete prog;
  } else {
    std::cerr << "Souffle: Failed to create instance!!!" << std::endl;
    std::cerr << "PointerTypeRecovery: Failed!" << std::endl;
    return PreservedAnalyses::all();
  }

  // 3. update pointer types
  auto pty = get_pointer_type(M);
  IRBuilder builder(M.getContext());
  // erase later to prevent memory address reuse
  std::vector<llvm::GlobalVariable *> gv2erase;
  std::vector<llvm::Instruction *> inst2erase;

  // llvm\lib\Transforms\Utils\CloneModule.cpp
  // GlobalVariable: change type
  for (GlobalVariable &gv : M.globals()) {
    long ty = get_ty_or_negative1(&gv);

    if (ty == datalog::ARITY_highTypeDomain::Pointer) {
      assert_sizet(gv.getValueType(), M);
      // handle initializer
      auto init = gv.getInitializer();
      init = ConstantExpr::getIntToPtr(init, pty);
      gv.setInitializer(nullptr);

      GlobalVariable *new_gv = new GlobalVariable(
          M, pty, gv.isConstant(), gv.getLinkage(), init, gv.getName(), &gv,
          gv.getThreadLocalMode(), gv.getType()->getAddressSpace());
      new_gv->copyAttributesFrom(&gv);
      new_gv->takeName(&gv); // so that no numeric suffix in name
      gv.replaceAllUsesWith(new_gv);

      gv2erase.emplace_back(&gv);
    }
  }

  for (Function &F : M) {
    bool isChanged = false;
    // copy functino type, and modify arg or ret type.
    Type **llvmArgTypes;
    size_t numParameters = F.arg_size();
    llvmArgTypes = (Type **)alloca(sizeof(Type *) * numParameters);
    Type *retType = F.getReturnType();

    // if arg typed to pointer, change it
    for (size_t i = 0; i < numParameters; i++) {
      Argument *arg = F.getArg(i);
      long ty = get_ty_or_negative1(arg);
      if (ty != datalog::ARITY_highTypeDomain::Pointer) {
        llvmArgTypes[i] = arg->getType();
      } else { // is Pointer
        assert_sizet(arg, M);
        llvmArgTypes[i] = pty;
        arg->mutateType(pty);
        isChanged = true;
      }
    }
    // if ret typed to pointer
    long ty = get_ty_or_negative1(aval{&F, datalog::FACT_FuncRet});
    if (ty == datalog::ARITY_highTypeDomain::Pointer) {
      assert_sizet(F.getReturnType(), M);
      retType = pty;
      isChanged = true;
    }

    if (isChanged) {
      FunctionType *newType = FunctionType::get(
          retType, ArrayRef<Type *>(llvmArgTypes, numParameters), F.isVarArg());
      F.mutateType(newType);
    }

    // handle insts
    for (Instruction &inst : instructions(F)) {
      long ty = get_ty_or_negative1(&inst);
      if (ty == datalog::ARITY_highTypeDomain::Pointer) {
        assert_sizet(&inst, M);
        inst.mutateType(pty);
      }
      if (auto add = dyn_cast<BinaryOperator>(&inst)) {
        if (add->getOpcode() == Instruction::Add) {
          // find ptr and number operand
          Value *op1 = add->getOperand(0);
          Value *op2 = add->getOperand(1);
          long ty1 = get_ty_or_negative1(op1);
          long ty2 = get_ty_or_negative1(op2);
          if (ty1 != datalog::Pointer && ty2 != datalog::Pointer) {
            llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                         << "Warning: ptr = unk + unk: " << *add << "\n";
            continue;
          }
          // not pointer + pointer
          assert(!(ty1 == datalog::Pointer && ty2 == datalog::Pointer));
          // op1 = ptr, op2 = number
          if (ty1 != datalog::Pointer) {
            std::swap(op1, op2);
            std::swap(ty1, ty2);
          }
          builder.SetInsertPoint(add);
          if (op1->getType()->isIntegerTy()) {
            op1 = builder.CreateIntToPtr(op1, pty);
          }
          auto gep = builder.CreateGEP(pty->getPointerElementType(), op1, op2,
                                       add->getName());
          add->replaceAllUsesWith(gep);
          inst2erase.emplace_back(add);
        }
      }
    }
  }

  // free all old values
  for (auto v : gv2erase) {
    assert(v->use_empty());
    v->eraseFromParent();
  }
  // erase insts
  for (auto v : inst2erase) {
    v->eraseFromParent();
  }

  return PreservedAnalyses::none();
}

Type *PointerTypeRecovery::get_pointer_type(Module &M) {
  // char* type
  return PointerType::get(IntegerType::get(M.getContext(), 8), 0);
}

struct stack : PassInfoMixin<stack> {
  // 判断一个Value是否与栈指针有关
  bool backtraceVar(Value *v, std::set<Value *> &realtedValue,
                    std::set<Value *> &realtedVariable) {
    if (!isa<Instruction>(v)) // 如果不是Inst 那就是参数
      return false;
    Instruction *i = dyn_cast<Instruction>(v);
    errs() << "backtrace " << *i << "\n";
    if (i->isBinaryOp()) {
      Value *op1 = i->getOperand(0);
      Value *op2 = i->getOperand(1);
      if (realtedValue.find(op1) != realtedValue.end() ||
          realtedValue.find(op2) != realtedValue.end()) {
        realtedValue.insert(i);
        return true;
      } else
        return (isa<Constant>(op1)
                    ? false
                    : backtraceVar(op1, realtedValue, realtedVariable)) ||
               (isa<Constant>(op2)
                    ? false
                    : backtraceVar(op2, realtedValue, realtedVariable));
    } else if (i->isUnaryOp() || i->getOpcode() == Instruction::Load ||
               i->getOpcode() == Instruction::BitCast) {
      Value *op = i->getOperand(0);
      if (isa<ConstantInt>(op))
        return false;
      if (realtedValue.find(op) != realtedValue.end()) {
        realtedValue.insert(i);
        return true;
      } else
        return backtraceVar(op, realtedValue, realtedVariable);
    } else if (i->getOpcode() ==
               Instruction::GetElementPtr) { // 只有两种可能 从栈中取或从mem取
      Value *op = i->getOperand(2);
      if (realtedVariable.find(op) != realtedVariable.end())
        return true;
    } else
      errs() << "[-] unhandled instruction " << *i << "\n";
    return false;
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // init utils
    std::set<Value *> realtedValue; // LLVM IR中的Value
    std::set<Value *>
        realtedVariable; // 实际栈上的变量 ,存的是caloffset对应的Value
    BasicBlock *entry = &F.getEntryBlock();
    IRBuilder<> builder(F.getContext());
    builder.SetInsertPoint(entry->getFirstNonPHI());
    std::vector<Instruction *> removeList;

    // find sp Value and stacksize
    GlobalValue *gStackPointer =
        F.getParent()->getNamedValue("$__stack_pointer");
    Value *sp = nullptr, *stackSize = nullptr;
    for (User *u : gStackPointer->users()) {
      Instruction *inst = dyn_cast<Instruction>(u);
      if (inst->getFunction() == &F) {
        removeList.push_back(inst);
        for (User *u : inst->users()) {
          inst = dyn_cast<Instruction>(u);
          if (inst->getOpcode() == Instruction::Sub) {
            stackSize = inst->getOperand(1);
            sp = inst;
            break;
          }
        }
      }
    }
    if (stackSize == nullptr) {
      errs() << "[-] Stack Pointer not found in function " << F.getName()
             << "\n";
      errs() << "PointerTypeRecovery cannot proceed!\n";
      return PreservedAnalyses::all();
    } else
      errs() << "[+] Stack Pointer found in function " << F.getName()
             << "size: " << *stackSize << "\n";

    // create newsp
    ArrayType *stackType =
        ArrayType::get(Type::getInt8Ty(F.getContext()),
                       dyn_cast<ConstantInt>(stackSize)->getZExtValue());
    Value *newStack = builder.CreateAlloca(stackType, nullptr, "newStack");
    Value *zeroSP = builder.CreateAlloca(Type::getInt32Ty(F.getContext()),
                                         nullptr, "newSP_buf");
    builder.CreateStore(ConstantInt::get(Type::getInt32Ty(F.getContext()), 0),
                        zeroSP);
    Value *newSP =
        builder.CreateLoad(Type::getInt32Ty(F.getContext()), zeroSP, "newSP");

    // replace sp
    sp->replaceAllUsesWith(newSP);
    realtedValue.insert(newSP);

    // recursively remove all user of  $__stack_pointer
    // https://stackoverflow.com/questions/34715947/delete-replace-code-from-llvm-ir
    for (Instruction *toRemove : removeList) {
      // for(User* u:toRemove->users())
      //     removeList.push_back(dyn_cast<Instruction>(u));
      toRemove->replaceAllUsesWith(UndefValue::get(toRemove->getType()));
      toRemove->eraseFromParent();
    }

    // look for stack access instruction and replace GEP
    for (BasicBlock &BB : F) {
      for (Instruction &inst : BB) {
        errs() << "INS " << inst << "\n";
        if (inst.getName().str().find("calcOffset") != std::string::npos &&
            backtraceVar(dyn_cast<Value>(&inst), realtedValue,
                         realtedVariable)) {
          errs() << "[+] found stack access " << inst << "\n";
          GetElementPtrInst *GEPInst =
              dyn_cast<GetElementPtrInst>(inst.getNextNode());
          GEPInst->setOperand(0, newStack); // mem -> stack
          GEPInst->setSourceElementType(stackType);

          // 判断栈变量的指针相关性 ，即load的操作数
          Instruction *next = GEPInst->getNextNode();
          if (isa<BitCastInst>(next))
            next = next->getNextNode();
          if (isa<StoreInst>(next)) {
            Value *op = next->getOperand(0);
            if (!isa<Constant>(op) &&
                backtraceVar(op, realtedValue, realtedVariable))
              realtedVariable.insert(&inst);
            else if (realtedVariable.find(&inst) !=
                     realtedVariable.end()) // store的不是栈指针相关的值
                                            // 如果在集合里，进行移除
              realtedVariable.erase(&inst);
          }
        }
      }
    }

    return PreservedAnalyses::all();
  }
};
} // namespace notdec::optimizers
