
#include "optimizers/pointer-type-recovery.h"
#include "datalog/fact-generator.h"
#include "optimizers/stack-pointer-finder.h"
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <map>
#include <ostream>
#include <souffle/RamTypes.h>
#include <system_error>
#include <utility>
#include <variant>

namespace souffle {
extern "C" {
extern void *__factory_Sf_pointer_main_instance;
}
} // namespace souffle

// Force linking of the main instance of the program
static void *volatile dummy = &souffle::__factory_Sf_pointer_main_instance;

using namespace llvm;

// TODO 重构直接输入facts
namespace notdec::optimizers {

template <typename T>
void static replaceAllUseWithExcept(Value *from, Value *to) {
  Value *except = to;
  std::vector<User *> users(from->user_begin(), from->user_end());
  for (User *use : users) {
    if (use == except) {
      continue;
    }
    if (isa<T>(use)) {
      continue;
    }
    use->replaceUsesOfWith(from, to);
  }
}

void replaceAllUseWithExcept(Value *from, Value *to) {
  Value *except = to;
  std::vector<User *> users(from->user_begin(), from->user_end());
  for (User *use : users) {
    if (use == except) {
      continue;
    }
    use->replaceUsesOfWith(from, to);
  }
  // assert number of user is 1
  // size_t num = 0;
  // for (User *use : from->users()) {
  //   num++;
  // }
  // assert(num == 1);
}

void replaceAllUseWithExcept(Value *from, Value *to, Value *except) {
  std::vector<User *> users(from->user_begin(), from->user_end());
  for (User *use : users) {
    if (use == except) {
      continue;
    }
    use->replaceUsesOfWith(from, to);
  }
}

bool static inline is_sizet(Type *ty, Module &M) {
  return ty->isIntegerTy(M.getDataLayout().getPointerSizeInBits());
}

bool static inline is_sizet(Value *val, Module &M) {
  return is_sizet(val->getType(), M);
}

void static inline assert_sizet(Type *ty, Module &M) {
  assert(is_sizet(ty, M));
}

void static inline assert_sizet(Value *val, Module &M) {
  assert(is_sizet(val, M));
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

/// Insert cast for one operand of PHI or Call.
Value *insertOpCastAtInd(IRBuilder<> *builder, Instruction *inst, Value *op,
                         Type *DestTy, unsigned index) {
  Value *casted = nullptr;
  if (auto phi = dyn_cast<PHINode>(inst)) {
    builder->SetInsertPoint(phi->getIncomingBlock(index)->getTerminator());
    auto casted = builder->CreateBitOrPointerCast(op, DestTy);
    phi->setIncomingValue(index, casted);
  } else if (auto call = dyn_cast<CallBase>(inst)) {
    assert(call->getArgOperand(index) == op);
    builder->SetInsertPoint(inst);
    casted = builder->CreateBitOrPointerCast(op, DestTy);
    call->setArgOperand(index, casted);
  } else {
    assert(false && "insertOpCastAtInd: not supported inst!!");
  }
  return casted;
}

/// Insert cast for one operand of an instruction.
Value *insertOpCast(IRBuilder<> *builder, Instruction *inst, Value *op,
                    Type *DestTy) {
  // assert op is used by inst
  if (auto phi = dyn_cast<PHINode>(inst)) {
    assert(false &&
           "insertOpCast: phi not supported, use insertPhiOpCast instead!!");
  } else {
    builder->SetInsertPoint(inst);
  }
  auto cast = builder->CreateBitOrPointerCast(op, DestTy);
  inst->replaceUsesOfWith(op, cast);
  return cast;
}

// must ensure datalog::Pointer
Value *PointerTypeRecovery::castBack(IRBuilder<> *builder, Instruction *inst,
                                     Type *old_ty, long hty) {
  assert(inst->getType()->isPointerTy());
  if (isa<PHINode>(inst)) {
    builder->SetInsertPoint(inst->getParent()->getFirstNonPHIOrDbgOrLifetime());
  } else {
    builder->SetInsertPoint(inst->getNextNonDebugInstruction());
  }
  auto ptr2int = builder->CreateBitOrPointerCast(inst, old_ty);
  if (hty != -1) {
    val2hty.emplace(ptr2int, hty);
  }
  replaceAllUseWithExcept(inst, ptr2int);
  return ptr2int;
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
          if (isa<Function>(val.first)) {
            llvm::errs() << "Datalog: type conflict(top): " << val.second
                         << " of Function " << val.first->getName() << "\n";
          } else {
            llvm::errs() << "Datalog: type conflict(top): " << val.second
                         << " of " << *val.first << "\n";
          }
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
  errs() << " ============== PointerTypeRecovery  ===============\n";
  // supress warning that dummy is unused.
  (void)dummy;
  datalog::FactGenerator fg(M);

  // 1. find mem/sp, export facts.
  // Value *mem = nullptr;
  // for (GlobalVariable &gv : M.getGlobalList()) {
  //   if (gv.getName().equals(MEM_NAME)) {
  //     mem = &gv;
  //     break;
  //   }
  // }
  // if (mem == nullptr) {
  //   std::cerr << "ERROR: mem not found!!";
  // } else {
  //   fg.append_fact(datalog::FACT_isMemory,
  //                  datalog::to_fact_str(fg.get_value_id(mem)));
  // }

  // 2. run souffle
  const char *Name = "pointer_main";
  if (souffle::SouffleProgram *prog =
          souffle::ProgramFactory::newInstance(Name)) {
    if (debug) {
      fg.visit_module();
      // create a temporary directory
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
      prog->loadAll(temp_path);
      // run the program...
      prog->run();
      prog->printAll(temp_path);
      // std::cerr << "Souffle: Running on directory " << temp_path << std::endl;

      // clean up
      // delete directory if not debug
      // if (!is_debug && !Path.empty()) {
      //   assert(llvm::sys::fs::remove_directories(Path.str()) == std::errc());
      // }

      // read analysis result
      fetch_result(fg, prog);
    } else {
      fg.set_program(prog);
      fg.visit_module();
      prog->run();
      // read analysis result
      fetch_result(fg, prog);
    }

    // ignore mem
    // val2hty.erase(mem);

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
  std::vector<llvm::Function *> func2erase;
  std::vector<Value *> not_inferred_add;

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
      gv.replaceAllUsesWith(ConstantExpr::getBitCast(new_gv, gv.getType()));

      gv2erase.emplace_back(&gv);
    }
  }

  // handle inst first, before function cloning.
  for (Function &F : make_early_inc_range(M)) {
    // handle insts
    for (Instruction &inst : make_early_inc_range(instructions(F))) {
      // ignore casts?
      if (isa<CastInst>(&inst)) {
        continue;
      }
      // only handle size_t
      if (!is_sizet(&inst, M)) {
        continue;
      }
      long ty = get_ty_or_negative1(&inst);
      // only handle pointer type change
      if (ty != datalog::ARITY_highTypeDomain::Pointer) {
        continue;
      }

      if (auto add = dyn_cast<BinaryOperator>(&inst)) {
        if (add->getOpcode() == Instruction::Add) {
          // find ptr and number operand
          Value *op1 = add->getOperand(0);
          Value *op2 = add->getOperand(1);
          long ty1 = get_ty_or_negative1(op1);
          long ty2 = get_ty_or_negative1(op2);
          if (ty1 != datalog::Pointer && ty2 != datalog::Pointer) {
            static const char *names[] = {"None", "Number", "Pointer", "Top"};
            assert(ty1 >= -1 && ty2 >= -1);
            assert(ty1 < 3 && ty2 < 3);
            llvm::errs() << "Warning: ptr(" << fg.get_value_id(add)
                         << ") = " << (names + 1)[ty1] << " + "
                         << (names + 1)[ty2] << ": " << *add << "\n ";
            // castBack(&builder, &inst, pty, datalog::Pointer);
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
          // only used by gep, no need to add val2hty.
          op1 = builder.CreateIntToPtr(op1, pty);
          auto gep = builder.CreateGEP(pty->getPointerElementType(), op1, op2,
                                       add->getName());
          val2hty.emplace(gep, datalog::Pointer);
          gep = builder.CreatePtrToInt(gep, add->getType());
          val2hty.emplace(gep, datalog::Pointer);
          add->replaceAllUsesWith(gep);
          inst2erase.emplace_back(add);
        } else {
          llvm::errs() << "Warning: unhandled BinaryOperator: " << inst << "\n";
        }
      } else if (auto phi = dyn_cast<PHINode>(&inst)) {
        auto old_ty = phi->getType();
        phi->mutateType(pty);
        for (auto i = 0; i < phi->getNumIncomingValues(); i++) {
          auto op = phi->getIncomingValue(i);
          // auto blk = phi->getIncomingBlock(i);
          insertOpCastAtInd(&builder, phi, op, pty, i);
        }
        castBack(&builder, &inst, old_ty, datalog::Pointer);
      } else if (auto load = dyn_cast<LoadInst>(&inst)) {
        // change to load i8* i8** xx
        auto old_ty = load->getType();
        // insert bitcast before load
        auto ptr = load->getPointerOperand();
        assert_sizet(load->getType(), M);
        load->mutateType(pty);
        insertOpCast(&builder, load, ptr, PointerType::getUnqual(pty));
        // cast back after load
        castBack(&builder, load, old_ty, datalog::Pointer);
      } else if (isa<CallInst>(&inst)) {
        // do nothing
      } else {
        llvm::errs() << "Warning: unhandled inst: " << inst << "\n";
        // builder.SetInsertPoint(inst.getNextNode());
        // auto int2ptr = builder.CreateIntToPtr(&inst, pty);
        // val2hty.emplace(int2ptr, datalog::Pointer);
        // replaceAllUseWithExcept<IntToPtrInst>(&inst, int2ptr);
      }
    }
  }
  // erase insts before function cloning
  for (auto v : inst2erase) {
    v->eraseFromParent();
  }

  // change function type using cloning
  for (Function &F : make_early_inc_range(M)) {
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
      if (ty != datalog::ARITY_highTypeDomain::Pointer ||
          arg->getType()->isPointerTy()) {
        llvmArgTypes[i] = arg->getType();
      } else { // is Pointer
        assert_sizet(arg, M);
        llvmArgTypes[i] = pty;
        isChanged = true;
      }
    }
    // if ret typed to pointer
    long ty = get_ty_or_negative1(aval{&F, datalog::FACT_FuncRet});
    if (ty == datalog::ARITY_highTypeDomain::Pointer &&
        !F.getReturnType()->isPointerTy()) {
      assert_sizet(F.getReturnType(), M);
      retType = pty;
      isChanged = true;
    }

    if (isChanged) {
      // Change function type: there is no way other than creating a new
      // function. see: lib\Transforms\Utils\CloneFunction.cpp
      // llvm::CloneFunction
      FunctionType *newType = FunctionType::get(
          retType, ArrayRef<Type *>(llvmArgTypes, numParameters), F.isVarArg());
      Function *NewF = Function::Create(newType, F.getLinkage(),
                                        F.getAddressSpace(), "", F.getParent());
      NewF->takeName(&F);
      ValueToValueMapTy VMap;

      // copying the names of the mapped arguments over
      Function::arg_iterator DestI = NewF->arg_begin();
      for (const Argument &I : F.args()) {
        DestI->setName(I.getName()); // Copy the name over...
        VMap[&I] = &*DestI++;        // Add mapping to VMap
      }

      SmallVector<ReturnInst *, 8> Returns; // Ignore returns cloned.
      CloneFunctionInto(NewF, &F, VMap,
                        CloneFunctionChangeType::LocalChangesOnly, Returns, "");

      // cast argument in entry block
      if (!F.isDeclaration()) {
        for (auto i = 0; i < NewF->arg_size(); i++) {
          auto arg = NewF->getArg(i);
          auto old_arg = F.getArg(i);
          if (arg->getType() != old_arg->getType()) {
            builder.SetInsertPoint(
                &*NewF->getEntryBlock().getFirstInsertionPt());
            auto casted =
                builder.CreateBitOrPointerCast(arg, old_arg->getType());
            val2hty.emplace(casted, datalog::Pointer);
            replaceAllUseWithExcept(arg, casted);
          }
        }
        // handle cast before return insts
        auto ret_ty = NewF->getReturnType();
        for (auto &B : *NewF) {
          for (auto &I : B) {
            if (auto ret = dyn_cast<ReturnInst>(&I)) {
              if (auto op = ret->getReturnValue()) {
                insertOpCast(&builder, ret, op, ret_ty);
              }
            }
          }
        }
      }

      F.replaceAllUsesWith(NewF);
      std::vector<User *> users(NewF->user_begin(), NewF->user_end());
      for (User *use : users) {
        if (CallBase *cb = dyn_cast<CallBase>(use)) {
          auto old_fty = cb->getFunctionType();
          auto new_fty = NewF->getFunctionType();
          cb->mutateFunctionType(new_fty);
          // insert int2ptr for call arg
          for (unsigned i = 0; i < cb->arg_size(); i++) {
            if (old_fty->getParamType(i) != new_fty->getParamType(i)) {
              auto op = cb->getArgOperand(i);
              insertOpCastAtInd(&builder, cb, op, new_fty->getParamType(i), i);
            }
          }
          // insert ptr2int for return value.
          if (old_fty->getReturnType() != new_fty->getReturnType()) {
            castBack(&builder, cb, old_fty->getReturnType(), datalog::Pointer);
          }
        }
      }

      func2erase.push_back(&F);
      // TODO what about indirect call? replace in table
    }
  }

  // free all old values
  for (auto v : gv2erase) {
    assert(v->use_empty());
    v->eraseFromParent();
  }
  // erase func
  for (auto v : func2erase) {
    v->eraseFromParent();
  }

  errs() << " ============== PointerTypeRecovery finished  ===============\n";
  return PreservedAnalyses::none();
}

Type *PointerTypeRecovery::get_pointer_type(Module &M) {
  // char* type
  return PointerType::get(IntegerType::get(M.getContext(), 8), 0);
}

} // namespace notdec::optimizers
