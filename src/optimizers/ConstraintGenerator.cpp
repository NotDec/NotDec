#include <cassert>
#include <clang/AST/Type.h>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Value.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>
#include <variant>
#include <vector>

#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/AllSCCIterator.h"
#include "Utils/Range.h"
#include "Utils/ValueNamer.h"
#include "optimizers/ConstraintGenerator.h"
#include "optimizers/StackPointerFinder.h"
#include "utils.h"

#define DEBUG_TYPE "type-recovery"

namespace notdec {

using retypd::OffsetLabel;

ValueNamer ValueNamer::Instance = ValueNamer();
const char *ConstraintGraph::Memory = "MEMORY";
const char *ValueNamer::DefaultPrefix = "v_";
const char *ValueNamer::FuncPrefix = "func_";
const char *ValueNamer::PhiPrefix = "phi_";
const char *ValueNamer::SelectPrefix = "select_";
const char *ValueNamer::NewPrefix = "new_";
const char *ValueNamer::AddPrefix = "add_";
const char *ValueNamer::SubPrefix = "sub_";
const char *ValueNamer::StackPrefix = "stack_";
const char *ValueNamer::AllocaPrefix = "alloca_";
const char *ValueNamer::LoadPrefix = "load_";
const char *ValueNamer::StorePrefix = "store_";

std::string getName(const ExtValuePtr &Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return ValueNamer::getName(**V);
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    return ValueNamer::getName(*F->Func, "ReturnValue_");
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    return ValueNamer::getName(*const_cast<llvm::CallBase *>(Arg->Call)) +
           "_CallArg_" + std::to_string(Arg->Index);
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    return ValueNamer::getName(*const_cast<llvm::CallBase *>(Ret->Call)) +
           "_CallRet";
  } else if (auto IC = std::get_if<IntConstant>(&Val)) {
    return "IntConstant_" + int_to_hex(IC->Val->getSExtValue());
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getName: unhandled type of ExtValPtr\n";
  std::abort();
}

inline void dumpTypes(llvm::Value *V, clang::QualType CTy) {
  llvm::errs() << "  Value: " << *V << " has type: " << CTy.getAsString()
               << "\n";
}

TypeRecovery::Result TypeRecovery::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== RetypdGenerator  ===============\n");

  // LLVM_DEBUG(printModule(M, "before-TypeRecovery.ll"));
  printModule(M, "before-TypeRecovery.ll");

  auto SP = MAM.getResult<StackPointerFinderAnalysis>(M);
  this->StackPointer = SP.result;

  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();

  // get the CallGraph, iterate by topological order of SCC
  CallGraph &CG = MAM.getResult<CallGraphAnalysis>(M);
  std::set<CallGraphNode *> Visited;
  all_scc_iterator<CallGraph *> CGI = notdec::scc_begin(&CG);
  // tuple(SCCNodes, ConstraintsGenerator, SCCName)
  std::vector<std::tuple<std::vector<CallGraphNode *>,
                         std::shared_ptr<ConstraintsGenerator>, std::string>>
      AllSCCs;

  // Walk the callgraph in bottom-up SCC order.
  for (; !CGI.isAtEnd(); ++CGI) {
    const std::vector<CallGraphNode *> &NodeVec = *CGI;
    AllSCCs.emplace_back(std::make_tuple(NodeVec, nullptr, ""));

    // Calc name for current SCC
    std::string Name;
    std::set<llvm::Function *> SCCs;
    for (auto CGN : NodeVec) {
      Visited.insert(CGN);
      if (CGN->getFunction() == nullptr ||
          CGN->getFunction()->isDeclaration()) {
        continue;
      }
      if (!Name.empty()) {
        Name += ",";
      }
      Name += CGN->getFunction()->getName().str();
      SCCs.insert(CGN->getFunction());
    }
    if (SCCs.empty()) {
      continue;
    }

    std::shared_ptr<ConstraintsGenerator> Generator =
        std::make_shared<ConstraintsGenerator>(
            *this, Name, SCCs, [&](auto *F) { return &FuncSummaries.at(F); });

    Generator->run();

    // TODO: If the SCC is not called by any other function out of the SCC, we
    // can skip summary generation. Even start Top-down phase.

    std::vector<retypd::SubTypeConstraint> Summary = Generator->genSummary();

    std::cerr << "Summary for " << Name << ":\n";
    for (auto &C : Summary) {
      std::cerr << "  " << notdec::retypd::toString(C) << "\n";
    }
    for (auto F : SCCs) {
      auto It = FuncCtxs.emplace(F, Generator);
      assert(It.second && "Function already in FuncCtxs?");
      auto It2 = FuncSummaries.emplace(F, Summary);
      assert(It2.second && "Function summary already exist?");
    }
    std::get<1>(AllSCCs.back()) = Generator;
    std::get<2>(AllSCCs.back()) = Name;
  }

  std::cerr << "Bottom up phase done! SCC count:" << AllSCCs.size() << "\n";

  TypeRecovery::Result Result;
  retypd::SketchToCTypeBuilder TB(M.getName());

  // Top-down Phase: build the result(Map from value to clang C type)
  for (auto It = AllSCCs.rbegin(); It != AllSCCs.rend(); ++It) {
    const std::vector<CallGraphNode *> &NodeVec = std::get<0>(*It);
    if (std::get<1>(*It) == nullptr) {
      continue;
    }
    auto &Generator = std::get<1>(*It);
    auto &Name = std::get<2>(*It);

    std::cerr << "Processing SCC: " << Name << "\n";

    // Collect all functions for SCC checking
    std::set<llvm::Function *> SCCSet;
    for (auto CGN : NodeVec) {
      if (CGN->getFunction() == nullptr) {
        continue;
      }
      SCCSet.insert(CGN->getFunction());
    }

    // instantiate the Sketches.
    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }
      if (FuncSketches.count(Current) == 0) {
        continue;
      }
      auto &CurrentSketches = FuncSketches.at(Current);
      for (int i = 0; i < Current->arg_size(); i++) {
        auto *Arg = Current->getArg(i);
        Generator->instantiateSketchAsSub(Arg, CurrentSketches.first.at(i));
      }
      if (!Current->getReturnType()->isVoidTy()) {
        Generator->instantiateSketchAsSup(ReturnValue{.Func = Current},
                                          CurrentSketches.second);
      }
    }

    std::vector<std::pair<TypeVariable,
                          std::function<void(std::shared_ptr<retypd::Sketch>)>>>
        Queries;
    std::map<Value *, std::shared_ptr<retypd::Sketch>> StackSketches;

    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }
      // If the FuncSketches is empty(no any caller), solve and fill by itself.
      if (FuncSketches.count(Current) == 0) {
        // get Or emplace new
        auto &CurrentSketches = FuncSketches[Current];
        if (Current->arg_size() > 0 && CurrentSketches.first.size() == 0) {
          CurrentSketches.first.resize(Current->arg_size(), nullptr);
        }

        for (int i = 0; i < Current->arg_size(); i++) {
          assert(CurrentSketches.first[i] == nullptr);
          // auto Sk = Generator->solveType(getCallArgTV(TRCtx, Current, 0, i));
          // CurrentSketches.first[i] = Sk;
          Queries.emplace_back(getCallArgTV(TRCtx, Current, 0, i),
                               [&CurrentSketches, i](auto Sk) {
                                 CurrentSketches.first[i] = Sk;
                               });
        }
        if (!Current->getReturnType()->isVoidTy()) {
          assert(CurrentSketches.second == nullptr);
          // auto Sk = Generator->solveType(getCallRetTV(TRCtx, Current, 0));
          // CurrentSketches.second = Sk;
          Queries.emplace_back(
              getCallRetTV(TRCtx, Current, 0),
              [&CurrentSketches](auto Sk) { CurrentSketches.second = Sk; });
        }
      }
      // Solve types for callee actual arguments.
      for (auto &Edge : *CGN) {
        if (!Edge.first.hasValue()) {
          continue;
        }
        CallBase *I = llvm::cast<llvm::CallBase>(&*Edge.first.getValue());
        auto Target = Edge.second->getFunction();
        if (Target == nullptr || SCCSet.count(Target) > 0) {
          continue;
        }
        auto TargetName = getFuncTvName(Target);
        llvm::errs() << "Solving Call in SCC: " << *I << "\n";

        // get Or emplace new
        auto &TargetSketches = FuncSketches[Target];
        if (I->arg_size() > 0 && TargetSketches.first.size() == 0) {
          TargetSketches.first.resize(I->arg_size(), nullptr);
        }

        size_t InstanceId = Generator->CallToID.at(I);
        for (int i = 0; i < I->arg_size(); i++) {
          // auto TV = getCallArgTV(TRCtx, Target, InstanceId, i);
          // auto Sketch = Generator->solveType(TV);
          // if (TargetSketches.first[i] == nullptr) {
          //   TargetSketches.first[i] = Sketch;
          // } else {
          //   TargetSketches.first[i]->join(*Sketch);
          // }
          Queries.emplace_back(getCallArgTV(TRCtx, Target, InstanceId, i),
                               [&TargetSketches, i](auto Sk) {
                                 if (TargetSketches.first[i] == nullptr) {
                                   TargetSketches.first[i] = Sk;
                                 } else {
                                   TargetSketches.first[i]->join(*Sk);
                                 }
                               });
        }
        if (!I->getType()->isVoidTy()) {
          // auto TV = getCallRetTV(TRCtx, Target, InstanceId);
          // auto Sketch = Generator->solveType(TV);
          // if (TargetSketches.second == nullptr) {
          //   TargetSketches.second = Sketch;
          // } else {
          //   TargetSketches.second->join(*Sketch);
          // }
          Queries.emplace_back(getCallRetTV(TRCtx, Target, InstanceId),
                               [&TargetSketches](auto Sk) {
                                 if (TargetSketches.second == nullptr) {
                                   TargetSketches.second = Sk;
                                 } else {
                                   TargetSketches.second->join(*Sk);
                                 }
                               });
        }
      }
      // Solve Stacks
      if (!Current->isDeclaration()) {
        // find all alloca in entry block and convert.
        for (auto &I : Current->getEntryBlock()) {
          if (auto *AI = dyn_cast<AllocaInst>(&I)) {
            // auto Sk =
            //     Generator->solveType(Generator->getNode(AI,
            //     nullptr).key.Base);
            // if (Sk != nullptr) {
            //   StackSketches.emplace(AI, Sk);
            // }
            Queries.emplace_back(Generator->getNode(AI, nullptr).key.Base,
                                 [&StackSketches, AI](auto Sk) {
                                   StackSketches.emplace(AI, Sk);
                                 });
          }
        }
      }
    }

    // Solve all queries
    Generator->CG.solveSketchQueries(Queries);

    // Convert the Arg and return type of functions in current SCC and save to
    // result
    // Solve and convert stack (alloca) types and save to result
    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }
      auto &CurrentSketches = FuncSketches.at(Current);
      for (int i = 0; i < Current->arg_size(); i++) {
        auto *Arg = Current->getArg(i);
        auto Sk = CurrentSketches.first[i];
        if (Sk == nullptr) {
          llvm::errs() << "Warn: Arg " << *Arg
                       << " of Func: " << Current->getName()
                       << " has no type info\n";
          continue;
        }
        auto CTy = TB.buildType(*Sk, Arg->getType()->getPrimitiveSizeInBits());
        dumpTypes(Arg, CTy);
        Result.ValueTypes.emplace(Arg, CTy);
      }
      if (!Current->getReturnType()->isVoidTy()) {
        auto Sk = CurrentSketches.second;
        if (Sk == nullptr) {
          llvm::errs() << "Warn: Return value of Func: " << Current->getName()
                       << " has no type info\n";
          continue;
        }
        auto CTy = TB.buildType(
            *Sk, Current->getReturnType()->getPrimitiveSizeInBits());
        llvm::errs() << "Return Value of Func: " << Current->getName()
                     << " has type: " << CTy.getAsString() << "\n";
        Result.FuncRetTypes.emplace(Current, CTy);
      }
      if (!Current->isDeclaration()) {
        // find all alloca in entry block and convert.
        for (auto &I : Current->getEntryBlock()) {
          if (auto *AI = dyn_cast<AllocaInst>(&I)) {
            if (StackSketches.count(AI) == 0) {
              continue;
            }
            auto Sk = StackSketches.at(AI);
            auto CTy =
                TB.buildType(*Sk, AI->getType()->getPrimitiveSizeInBits());
            dumpTypes(AI, CTy);
            Result.ValueTypes.emplace(AI, CTy);
          }
        }
      }
    }
  }

  // move the ASTUnit to result
  Result.ASTUnit = std::move(TB.ASTUnit);

  // gen_json("retypd-constrains.json");

  // TODO convert the type back to LLVM IR
  print(M, "after-TypeRecovery.ll");

  LLVM_DEBUG(errs() << " ============== RetypdGenerator End ===============\n");
  return Result;
}

void ConstraintsGenerator::instantiateSketchAsSub(
    ExtValuePtr Val, std::shared_ptr<retypd::Sketch> Sk) {
  CGNode &Root = CG.instantiateSketch(Sk);
  addSubtype(Root.key.Base, getTypeVar(Val, nullptr));
}

void ConstraintsGenerator::instantiateSketchAsSup(
    ExtValuePtr Val, std::shared_ptr<retypd::Sketch> Sk) {
  CGNode &Root = CG.instantiateSketch(Sk);
  addSubtype(getTypeVar(Val, nullptr), Root.key.Base);
}

void ConstraintsGenerator::run() {
  for (llvm::Function *Func : SCCs) {
    RetypdGeneratorVisitor Visitor(*this);
    Visitor.visit(Func);
    Visitor.handlePHINodes();
  }
  if (const char *path = std::getenv("DEBUG_TRANS_INIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(CG.Name.c_str(), path))) {
      CG.printGraph("trans_init.dot");
    }
  }
}

size_t ConstraintsGenerator::instantiateSummary(llvm::Function *Target) {
  auto ID = ValueNamer::getId();
  const std::vector<retypd::SubTypeConstraint> *Sum = GetSummary(Target);
  CG.instantiate(*Sum, ID);
  return ID;
}

std::vector<retypd::SubTypeConstraint> ConstraintsGenerator::genSummary() {
  std::set<std::string> InterestingVars;
  for (auto *F : SCCs) {
    assert(F->hasName());
    InterestingVars.insert(F->getName().str());
  }
  if (CG.MemoryNode != nullptr) {
    InterestingVars.insert(CG.Memory);
  }
  CG.solve();
  return CG.simplifiedExpr(InterestingVars);
}

bool mustBePrimitive(const llvm::Type *Ty) {
  if (Ty->isFloatTy() || Ty->isDoubleTy()) {
    return true;
  }
  if (Ty->isIntegerTy()) {
    return Ty->getIntegerBitWidth() < 32;
  }
  if (Ty->isPointerTy()) {
    return false;
  }
  return false;
}

static std::string getLLVMTypeBase(Type *Ty) {
  if (Ty->isPointerTy()) {
    // 20231122 why there is pointer constant(not inttoptr constant expr)
    assert(false && "TODO can this be pointer?");
    return getLLVMTypeBase(Ty->getPointerElementType()) + "*";
  }
  if (Ty->isFloatTy()) {
    return "float";
  } else if (Ty->isDoubleTy()) {
    return "double";
  } else if (Ty->isIntegerTy()) {
    if (Ty->getIntegerBitWidth() == 1)
      return "bool";
    else
      return "int";
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown integer type: " << *Ty << "\n";
    assert(false && "unknown integer type");
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: unknown constant type: " << *Ty << "\n";
    assert(false && "unknown constant type");
  }
}

static inline TypeVariable makeTv(retypd::TRContext &Ctx, std::string Name) {
  return retypd::TypeVariable::CreateDtv(Ctx, Name);
}

static inline TypeVariable getLLVMTypeVar(retypd::TRContext &Ctx, Type *Ty) {
  return retypd::TypeVariable::CreatePrimitive(Ctx, getLLVMTypeBase(Ty));
}

static bool is32Or64Int(const Type *Ty) {
  if (Ty->isIntegerTy()) {
    return Ty->getIntegerBitWidth() == 32 || Ty->getIntegerBitWidth() == 64;
  }
  return false;
}

// Check if a primitive type is final. Currently only int is not final for
// unknown signedness.
bool isFinal(const std::string &Name) {
  if (Name == "int") {
    return false;
  }
  return true;
}

bool hasUser(const Value *Val, const User *User) {
  for (auto U : Val->users()) {
    if (U == User) {
      return true;
    }
  }
  return false;
}

unsigned int getSize(const ExtValuePtr &Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return (*V)->getType()->getScalarSizeInBits();
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    return F->Func->getReturnType()->getScalarSizeInBits();
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    // TODO what if function pointer?
    return Arg->Call->getArgOperand(Arg->Index)
        ->getType()
        ->getScalarSizeInBits();
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    return Ret->Call->getType()->getScalarSizeInBits();
  } else if (auto IC = std::get_if<IntConstant>(&Val)) {
    return IC->Val->getBitWidth();
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getSize: unhandled type of ExtValPtr\n";
  std::abort();
}

retypd::CGNode &ConstraintsGenerator::getNode(ExtValuePtr Val, User *User) {
  // Differentiate int32/int64 by User.
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    if (auto CI = dyn_cast<ConstantInt>(*V)) {
      if (CI->getBitWidth() == 32 || CI->getBitWidth() == 64) {
        assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
        assert(hasUser(*V, User) &&
               "convertTypeVarVal: constant not used by user");
        Val = IntConstant{.Val = cast<ConstantInt>(*V), .User = User};
      }
    }
  }
  if (Val2Node.count(Val)) {
    return *Val2Node.at(Val);
  }
  auto ret = convertTypeVar(Val, User);
  return setTypeVar(Val, ret, User, getSize(Val));
}

const TypeVariable &ConstraintsGenerator::getTypeVar(ExtValuePtr Val,
                                                     User *User) {
  return getNode(Val, User).key.Base;
}

TypeVariable ConstraintsGenerator::convertTypeVar(ExtValuePtr Val, User *User) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertTypeVarVal(*V);
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    auto tv = makeTv(Ctx.TRCtx, getFuncTvName(F->Func));
    tv = tv.pushLabel(retypd::OutLabel{});
    return tv;
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    assert(false);
    // TODO what if function pointer?
    auto tv = makeTv(Ctx.TRCtx, getFuncTvName(Arg->Call->getCalledFunction()));
    // tv.getInstanceId() = Arg->InstanceId;
    tv = tv.pushLabel(retypd::InLabel{std::to_string(Arg->Index)});
    return tv;
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    assert(false);
    // TODO what if function pointer?
    auto tv = makeTv(Ctx.TRCtx, getFuncTvName(Ret->Call->getCalledFunction()));
    // tv.getInstanceId() = Ret->InstanceId;
    tv = tv.pushLabel(retypd::OutLabel{});
    return tv;
  } else if (auto IC = std::get_if<IntConstant>(&Val)) {
    assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
    auto ret = TypeVariable::CreateIntConstant(
        Ctx.TRCtx, OffsetRange{.offset = IC->Val->getSExtValue()}, User);
    return ret;
  }
  llvm::errs()
      << __FILE__ << ":" << __LINE__ << ": "
      << "ERROR: RetypdGenerator::convertTypeVar unhandled type of ExtValPtr\n";
  std::abort();
}

TypeVariable ConstraintsGenerator::convertTypeVarVal(Value *Val, User *User) {
  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast ConstantExpr
      if (CE->getOpcode() == Instruction::BitCast) {
        return getTypeVar(CE->getOperand(0), User);
      }
      if (CE->getOpcode() == Instruction::IntToPtr) {
        if (auto Addr = dyn_cast<ConstantInt>(CE->getOperand(0))) {
          auto tv = CG.getMemoryNode()->key.Base;
          tv = addOffset(tv, OffsetRange{.offset = Addr->getSExtValue()});
          return tv;
        }
      }
    } else if (auto gv = dyn_cast<GlobalValue>(C)) { // global variable
      if (gv == Ctx.StackPointer) {
        std::cerr
            << "Error: convertTypeVarVal: direct use of stack pointer?, ensure "
               "StackAllocationRecovery is run before, Or add external summary "
               "for this function.\n";
        return makeTv(Ctx.TRCtx, ValueNamer::getName());
      } else if (auto Func = dyn_cast<Function>(C)) {
        // Consistent with Call handling
        return makeTv(Ctx.TRCtx, getFuncTvName(Func));
      }
      return makeTv(Ctx.TRCtx, gv->getName().str());
    } else if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      if (auto CI = dyn_cast<ConstantInt>(C)) {
        assert(CI->getBitWidth() != 32 && CI->getBitWidth() != 64 &&
               "Should be handled earlier");
      }
      auto Ty = C->getType();
      return getLLVMTypeVar(Ctx.TRCtx, Ty);
    }
    llvm::errs()
        << __FILE__ << ":" << __LINE__ << ": "
        << "ERROR: RetypdGenerator::getTypeVar unhandled type of constant: "
        << *C << "\n";
    std::abort();
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    // Consistent with Call handling
    TypeVariable tv = getTypeVar(arg->getParent(), User);
    tv = tv.pushLabel(retypd::InLabel{std::to_string(arg->getArgNo())});
    return tv;
  }

  // Use different suffix for different type of value.
  if (auto *Sel = dyn_cast<SelectInst>(Val)) {
    return makeTv(Ctx.TRCtx,
                  ValueNamer::getName(*Sel, ValueNamer::SelectPrefix, true));
  } else if (auto *Alloca = dyn_cast<AllocaInst>(Val)) {
    const char *prefix = ValueNamer::AllocaPrefix;
    if (Alloca->getParent()->isEntryBlock()) {
      prefix = ValueNamer::StackPrefix;
    }
    return makeTv(Ctx.TRCtx, ValueNamer::getName(*Alloca, prefix, true));
  } else if (auto Phi = dyn_cast<PHINode>(Val)) {
    return makeTv(Ctx.TRCtx,
                  ValueNamer::getName(*Phi, ValueNamer::PhiPrefix, true));
  } else if (auto *I = dyn_cast<Instruction>(Val)) {
    return makeTv(Ctx.TRCtx,
                  ValueNamer::getName(*I, ValueNamer::NewPrefix, true));
  } else if (auto *Load = dyn_cast<LoadInst>(Val)) {
    return makeTv(Ctx.TRCtx,
                  ValueNamer::getName(*Load, ValueNamer::LoadPrefix, true));
  } else if (auto *Store = dyn_cast<StoreInst>(Val)) {
    return makeTv(Ctx.TRCtx,
                  ValueNamer::getName(*Store, ValueNamer::StorePrefix, true));
  } else if (auto *Add = dyn_cast<BinaryOperator>(Val)) {
    return makeTv(Ctx.TRCtx,
                  ValueNamer::getName(*Add, ValueNamer::AddPrefix, true));
  } else if (auto *Sub = dyn_cast<BinaryOperator>(Val)) {
    return makeTv(Ctx.TRCtx,
                  ValueNamer::getName(*Sub, ValueNamer::SubPrefix, true));
  }

  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "WARN: RetypdGenerator::getTypeVar unhandled value: " << *Val
               << "\n";
  return makeTv(Ctx.TRCtx,
                ValueNamer::getName(*Val, ValueNamer::NewPrefix, true));
}

// TODO: accept any character in name by using single quotes like LLVM IR.
// static std::string sanitize_name(std::string S) {
//   std::replace(S.begin(), S.end(), '.', '_');
//   std::replace(S.begin(), S.end(), '@', '_');
//   return S;
// }

void ConstraintsGenerator::RetypdGeneratorVisitor::visitReturnInst(
    ReturnInst &I) {
  auto *Src = I.getReturnValue();
  if (Src == nullptr) { // ret void.
    return;
  }
  auto SrcVar = cg.getTypeVar(Src, &I);
  auto DstVar = cg.getTypeVar(ReturnValue{.Func = I.getFunction()}, &I);
  // src is a subtype of dest
  cg.addSubtype(SrcVar, DstVar);
}

std::shared_ptr<retypd::Sketch>
ConstraintsGenerator::solveType(const TypeVariable &TV) {
  // Because we always do layer split after clone, so we can refer to Node by
  // type variable.
  if (CG.Nodes.count(TV) == 0) {
    return nullptr;
  }
  auto &CGNode = CG.Nodes.at(TV);
  return CG.solveSketch(CGNode);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: RetypdGenerator: indirect call not supported yet\n";
    return;
  }

  if (cg.SCCs.count(Target)) {
    // Call within the SCC:
    // directly link to the function tv.
    for (int i = 0; i < I.arg_size(); i++) {
      auto ArgVar = cg.getTypeVar(Target->getArg(i), &I);
      auto ValVar = cg.getTypeVar(I.getArgOperand(i), &I);
      // argument is a subtype of param
      cg.addSubtype(ValVar, ArgVar);
    }
    if (!I.getType()->isVoidTy()) {
      // type var should be consistent with return instruction
      auto FormalRetVar = cg.getTypeVar(ReturnValue{.Func = Target}, &I);
      auto ValVar = cg.getTypeVar(&I, nullptr);
      // formal return -> actual return
      cg.addSubtype(FormalRetVar, ValVar);
    }
  } else {
    size_t InstanceId;
    if (Target->isDeclaration()) {
      // empty summary, still differentiate different call instances
      InstanceId = ValueNamer::getId();
    } else {
      // differentiate different call instances in the same function
      InstanceId = cg.instantiateSummary(Target);
    }
    cg.CallToID.emplace(&I, InstanceId);
    for (int i = 0; i < I.arg_size(); i++) {
      auto ArgVar = getCallArgTV(cg.Ctx.TRCtx, Target, InstanceId, i);
      auto ValVar = cg.getTypeVar(I.getArgOperand(i), &I);
      // argument is a subtype of param
      cg.addSubtype(ValVar, ArgVar);
    }
    if (!I.getType()->isVoidTy()) {
      // for return value
      auto FormalRetVar = getCallRetTV(cg.Ctx.TRCtx, Target, InstanceId);
      auto ValVar = cg.getTypeVar(&I, nullptr);
      // formal return -> actual return
      cg.addSubtype(FormalRetVar, ValVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitSelectInst(
    SelectInst &I) {
  auto DstVar = cg.getTypeVar(&I, nullptr);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.getTypeVar(Src1, &I);
  auto Src2Var = cg.getTypeVar(Src2, &I);
  // Not generate boolean constraints. Because it must be i1.
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAllocaInst(
    AllocaInst &I) {
  auto &Node = cg.getNode(&I, nullptr);
  // set as pointer type
  cg.setPointer(Node);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  // Defer constraints generation (and unification) to handlePHINodes
}

void ConstraintsGenerator::RetypdGeneratorVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto DstVar = cg.getTypeVar(I, nullptr);
    for (auto &Op : I->incoming_values()) {
      auto *Src = Op.get();
      auto SrcVar = cg.getTypeVar(Src, I);
      // src is a subtype of dest
      cg.addSubtype(SrcVar, DstVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto SrcVar = cg.getTypeVar(Src, &I);
    cg.addVarSubtype(&I, SrcVar);
    return;
  } else if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *Src = I.getOperand(0);
    auto SrcVar = cg.getTypeVar(Src, &I);
    /*auto &Node = */ cg.addVarSubtype(&I, SrcVar);
    // cg.setPointer(Node);
    return;
  } else if (isa<TruncInst, ZExtInst>(&I)) {
    auto *Src = I.getOperand(0);
    if (is32Or64Int(I.getSrcTy()) && is32Or64Int(I.getDestTy())) {
      auto SrcVar = cg.getTypeVar(Src, &I);
      cg.addVarSubtype(&I, SrcVar);
    } else {
      if (isa<ZExtInst>(&I)) {
        cg.addVarSubtype(&I,
                         TypeVariable::CreatePrimitive(cg.Ctx.TRCtx, "uint"));
        cg.addSubtype(cg.getTypeVar(I.getOperand(0), &I),
                      TypeVariable::CreatePrimitive(cg.Ctx.TRCtx, "uint"));
      } else {
        cg.addVarSubtype(&I, getLLVMTypeVar(cg.Ctx.TRCtx, I.getType()));
        cg.addSubtype(cg.getTypeVar(I.getOperand(0), &I),
                      getLLVMTypeVar(cg.Ctx.TRCtx, I.getOperand(0)->getType()));
      }
    }
    return;
  } else if (isa<SExtInst>(&I)) {
    auto *Src = I.getOperand(0);
    auto SrcVar = cg.getTypeVar(Src, &I);
    cg.addSubtype(SrcVar, TypeVariable::CreatePrimitive(cg.Ctx.TRCtx, "sint"));
    cg.addVarSubtype(&I, TypeVariable::CreatePrimitive(cg.Ctx.TRCtx, "sint"));
    cg.addSubtype(cg.getTypeVar(I.getOperand(0), &I),
                  TypeVariable::CreatePrimitive(cg.Ctx.TRCtx, "sint"));
    return;
  } else if (isa<FPToUIInst, FPToSIInst, UIToFPInst, SIToFPInst, FPTruncInst,
                 FPExtInst>(&I)) {
    visitInstruction(I);
    return;
  }

  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: RetypdGenerator::getTypeVar unhandled CastInst: " << I
               << "\n";
  std::abort();
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  std::cerr << "Warning: RetypdGeneratorVisitor::visitGetElementPtrInst: "
               "Gep should not exist before this pass!\n";
  // But if we really want to support this, handle it the same way as AddInst.
  // A shortcut to create a offseted pointer. the operate type must be i8*. Just
  // like ptradd.
}

void ConstraintsGenerator::addCmpConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            ICmpInst *I) {
  getNode(LHS, I).getPNIVar()->unifyPN(*getNode(RHS, I).getPNIVar());
}

// for pointer sized int, probably is pointer comparision. So we cannot make a
// conclusion of subtype. but create a CmpConstraint.
void ConstraintsGenerator::RetypdGeneratorVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addCmpConstraint(Src1, Src2, &I);

  // type the inst as bool
  assert(I.getType()->isIntegerTy(1));
  cg.addVarSubtype(&I, getLLVMTypeVar(cg.Ctx.TRCtx, I.getType()));
}

// #region LoadStore

// =========== begin: load/store insts and deref analysis ===========

unsigned ConstraintsGenerator::getPointerElemSize(Type *ty) {
  Type *Elem = ty->getPointerElementType();
  unsigned Size = Elem->getPrimitiveSizeInBits();
  if (Size != 0) {
    return Size;
  }
  if (Elem->isPointerTy()) {
    assert(Ctx.pointer_size != 0 &&
           "RetypdGenerator: pointer size not initialized");
    return Ctx.pointer_size;
  }
  assert(false && "unknown pointer type");
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitStoreInst(
    StoreInst &I) {
  auto DstVar =
      cg.deref(I.getPointerOperand(), &I,
               cg.getPointerElemSize(I.getPointerOperandType()), false);
  cg.addSubtype(cg.getTypeVar(I.getValueOperand(), &I), DstVar);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  auto LoadedVar =
      cg.deref(I.getPointerOperand(), &I,
               cg.getPointerElemSize(I.getPointerOperandType()), true);
  auto ValVar = cg.getNode(&I, nullptr);
  // formal load -> actual load
  cg.addSubtype(LoadedVar, ValVar.key.Base);
  if (mustBePrimitive(I.getType())) {
    cg.addSubtype(ValVar.key.Base, getLLVMTypeVar(cg.Ctx.TRCtx, I.getType()));
  }
}

std::string ConstraintsGenerator::offset(APInt Offset, int Count) {
  std::string OffsetStr = toString(Offset, 10, false);
  // case -1:
  //   OffsetStr = "0*[nobound]";
  //   break;
  // case -2:
  //   OffsetStr = "0*[nullterm]";

  if (Count > 1) {
    OffsetStr += "*" + std::to_string(Count);
  }
  return OffsetStr;
}

TypeVariable ConstraintsGenerator::addOffset(TypeVariable &dtv,
                                             OffsetRange Offset) {
  if (!dtv.getLabels().empty() &&
      std::holds_alternative<OffsetLabel>(dtv.getLabels().back())) {
    OffsetLabel LastLabel = std::get<OffsetLabel>(dtv.getLabels().back());
    LastLabel.range = LastLabel.range + Offset;
    return dtv.popLabel().pushLabel(LastLabel);
  } else {
    return dtv.pushLabel(OffsetLabel{.range = Offset});
  }
}

// Special logics for load and store when generating type variables.
TypeVariable ConstraintsGenerator::deref(Value *Val, User *User,
                                         unsigned BitSize, bool isLoadOrStore) {
  setPointer(getNode(Val, User));
  assert(BitSize != 0 && "RetypdGenerator::deref: zero size!?");
  // from the offset, generate a loaded type variable.
  auto DstVar = getTypeVar(Val, User);
  assert(BitSize % 8 == 0 && "size is not byte aligned!?");
  if (isLoadOrStore) {
    DstVar = DstVar.pushLabel(retypd::LoadLabel{.Size = BitSize});
  } else {
    DstVar = DstVar.pushLabel(retypd::StoreLabel{.Size = BitSize});
  }
  return DstVar;
}

// =========== end: load/store insts ===========

// #endregion LoadStore

// =========== begin: other insts ===========

const std::map<unsigned, ConstraintsGenerator::PcodeOpType>
    ConstraintsGenerator::opTypes = {
        // for Trunc, ZExt, SExt
        {Instruction::SExt, {"sint", 1, (const char *[1]){"sint"}}},

        // other cast insts: FPToUIInst, FPToSIInst, UIToFPInst, SIToFPInst
        {Instruction::FPToUI, {"uint", 1, (const char *[1]){nullptr}}},
        {Instruction::FPToSI, {"sint", 1, (const char *[1]){nullptr}}},
        {Instruction::UIToFP, {nullptr, 1, (const char *[1]){"uint"}}},
        {Instruction::SIToFP, {nullptr, 1, (const char *[1]){"sint"}}},

        {Instruction::UDiv, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SDiv, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::URem, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::SRem, {"sint", 2, (const char *[2]){"sint", "sint"}}},
        {Instruction::Xor, {"uint", 2, (const char *[2]){"uint", "uint"}}},

        // {Instruction::Add, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Sub, {"int", (const char *[2]){"int", "int"}, 2}},
        {Instruction::Mul, {"int", 2, (const char *[2]){"int", "int"}}},

        {Instruction::Shl, {"int", 2, (const char *[2]){"int", "int"}}},
        {Instruction::LShr, {"uint", 2, (const char *[2]){"uint", "uint"}}},
        {Instruction::AShr, {"sint", 2, (const char *[2]){"sint", "uint"}}},
        // {Instruction::And, {"int", (const char *[2]){"int", "int"}, 2}},
        // {Instruction::Or, {"int", (const char *[2]){"int", "int"}, 2}},
};

void ConstraintsGenerator::RetypdGeneratorVisitor::visitInstruction(
    Instruction &I) {
  // return value
  if (I.getType()->isVoidTy()) {
    // skip void type
  } else if (mustBePrimitive(I.getType())) {
    cg.addVarSubtype(&I, getLLVMTypeVar(cg.Ctx.TRCtx, I.getType()));
  } else if (opTypes.count(I.getOpcode()) &&
             opTypes.at(I.getOpcode()).addRetConstraint(&I, cg)) {
    // good
  } else {
    llvm::errs() << "WARN: RetypdGenerator: unhandled instruction return: " << I
                 << "\n";
  }
  // for each op
  for (unsigned Ind = 0; Ind < I.getNumOperands(); ++Ind) {
    auto Op = I.getOperand(Ind);
    if (Op->getType()->isVoidTy()) {
      // skip void type
    } else if (mustBePrimitive(Op->getType())) {
      cg.addSubtype(cg.getTypeVar(Op, &I),
                    getLLVMTypeVar(cg.Ctx.TRCtx, Op->getType()));
    } else if (opTypes.count(I.getOpcode()) &&
               opTypes.at(I.getOpcode()).addOpConstraint(Ind, &I, cg)) {
      // good
    } else {
      llvm::errs() << "WARN: RetypdGenerator: unhandled instruction Op: " << I
                   << "\n";
    }
  }
}

// void ConstraintsGenerator::addSubTypeCons(SSGNode *LHS, SSGNode *RHS,
//                                           OffsetRange Offset) {
//   SSG.addSubTypeCons(RHS, LHS, Offset);
// }

// void ConstraintsGenerator::addSubTypeCons(llvm::Value *LHS,
//                                           llvm::BinaryOperator *RHS,
//                                           OffsetRange Offset) {
//   addSubTypeCons(getSSGNode(RHS, nullptr), getSSGNode(LHS, RHS), Offset);
// }

void ConstraintsGenerator::addAddConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            BinaryOperator *I) {
  PG.addAddCons(&getNode(LHS, I), &getNode(RHS, I), &getNode(I, nullptr), I);
}

void ConstraintsGenerator::addSubConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            BinaryOperator *I) {
  PG.addSubCons(&getNode(LHS, I), &getNode(RHS, I), &getNode(I, nullptr), I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAdd(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addAddConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitSub(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addSubConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAnd(BinaryOperator &I) {
  // llvm::errs() << "visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if ((CI->getZExtValue() & 0x3fffff00) == 0x3fffff00) {
      // act as simple assignment
      cg.addVarSubtype(&I, cg.getTypeVar(Src1, &I));
      return;
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: And op without constant: " << I << "\n";
    cg.addSubtype(cg.getTypeVar(Src2, &I),
                  getLLVMTypeVar(cg.Ctx.TRCtx, Src2->getType()));
  }
  // view as numeric operation?
  cg.addVarSubtype(&I, getLLVMTypeVar(cg.Ctx.TRCtx, I.getType()));
  cg.addSubtype(cg.getTypeVar(Src1, &I),
                getLLVMTypeVar(cg.Ctx.TRCtx, Src1->getType()));
  return;
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitOr(BinaryOperator &I) {
  // llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if ((CI->getZExtValue() & 0x3fffff00) == 0) {
      // act as simple assignment
      cg.addVarSubtype(&I, cg.getTypeVar(Src1, &I));
      return;
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: And op without constant: " << I << "\n";
    cg.addSubtype(cg.getTypeVar(Src2, &I),
                  getLLVMTypeVar(cg.Ctx.TRCtx, Src2->getType()));
  }
  // view as numeric operation?
  cg.addVarSubtype(&I, getLLVMTypeVar(cg.Ctx.TRCtx, I.getType()));
  cg.addSubtype(cg.getTypeVar(Src1, &I),
                getLLVMTypeVar(cg.Ctx.TRCtx, Src1->getType()));
  return;
}

bool ConstraintsGenerator::PcodeOpType::addRetConstraint(
    Instruction *I, ConstraintsGenerator &cg) const {
  if (I->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = output;
  if (ty == nullptr) {
    return false;
  }
  cg.addVarSubtype(I, TypeVariable::CreatePrimitive(cg.Ctx.TRCtx, ty));
  return true;
}

bool ConstraintsGenerator::PcodeOpType::addOpConstraint(
    unsigned Index, Instruction *I, ConstraintsGenerator &cg) const {
  assert(size == I->getNumOperands() && "input size not match");
  auto Op = I->getOperand(Index);
  if (Op->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = inputs[Index];
  if (ty == nullptr) {
    return false;
  }
  cg.addSubtype(cg.getTypeVar(Op, I),
                TypeVariable::CreatePrimitive(cg.Ctx.TRCtx, ty));
  return true;
}

// =========== end: other insts ===========

void TypeRecovery::gen_json(std::string OutputFilename) {
  // json::Object Root({{"data_layout", data_layout}});

  // json::Object Constraints;
  // // iterate func_constrains
  // for (auto &kv : func_ctxs) {
  //   auto funcName = ValueNamer::getName(*kv.first);
  //   json::Array FuncConstrainsJson;
  //   for (auto &c : kv.second.CG.toConstraints()) {
  //     FuncConstrainsJson.push_back(toString(c));
  //   }
  //   Constraints[funcName] = json::Value(std::move(FuncConstrainsJson));
  // }
  // Root["constraints"] = json::Value(std::move(Constraints));

  // // write to file
  // std::error_code EC;
  // llvm::raw_fd_ostream os(OutputFilename, EC);
  // if (EC) {
  //   std::cerr << __FILE__ << ":" << __LINE__ << ": "
  //             << "Cannot open output json file." << std::endl;
  //   std::cerr << EC.message() << std::endl;
  //   std::abort();
  // }
  // json::OStream J(os, 2);
  // J.value(std::move(Root));
}

class CGAnnotationWriter : public llvm::AssemblyAnnotationWriter {
  // ConstraintsGenerator &CG;
  std::map<Function *, ConstraintsGenerator> &func_ctxs;

  void emitFunctionAnnot(const llvm::Function *F,
                         llvm::formatted_raw_ostream &OS) override {
    auto &CG = func_ctxs.at(const_cast<llvm::Function *>(F));
    OS << "; ";
    if (!F->getReturnType()->isVoidTy()) {
      OS << CG.getNode(ReturnValue{.Func = const_cast<llvm::Function *>(F)},
                       nullptr)
                .str();
      OS << " <- ";
    }
    OS << "(";
    for (auto &Arg : F->args()) {
      OS << CG.getNode(const_cast<llvm::Argument *>(&Arg), nullptr).str()
         << ", ";
    }
    OS << ")";
    OS << "\n";
  }

  void printInfoComment(const llvm::Value &V,
                        llvm::formatted_raw_ostream &OS) override {

    // TODO: print CallArg and CallRet
    // if (auto Call = llvm::dyn_cast<CallBase>(&V)) {
    //   OS << " ; ";
    //   if (!Call->getType()->isVoidTy()) {
    //     OS << CG.getNode(CallArg{.Func = const_cast<llvm::Function *>(F)},
    //                      nullptr)
    //               .str();
    //     OS << " -> ";
    //   }
    //   OS << "(";
    //   for (auto &Arg : F->args()) {
    //     OS << CG.getNode(CallRet{.Call = Call, .Index = }, nullptr).str()
    //        << ", ";
    //   }
    //   OS << ")";
    //   OS << "\n";
    // }

    const Instruction *Instr = dyn_cast<Instruction>(&V);
    if (Instr == nullptr) {
      return;
    }
    auto &CG = func_ctxs.at(const_cast<llvm::Function *>(Instr->getFunction()));

    OS << "; ";
    if (!V.getType()->isVoidTy()) {
      OS << CG.getNode(const_cast<llvm::Value *>(&V), nullptr).str();
      if (Instr != nullptr) {
        OS << " <- ";
      }
    }
    if (Instr != nullptr) {
      OS << "(";
      for (const Use &Op : Instr->operands()) {
        OS << CG.getNode(Op.get(), const_cast<llvm::Instruction *>(Instr)).str()
           << ", ";
      }
      OS << ")";
    }
    // OS << "\n";
  }

public:
  CGAnnotationWriter(std::map<Function *, ConstraintsGenerator> &func_ctxs)
      : func_ctxs(func_ctxs) {}
};

void TypeRecovery::print(llvm::Module &M, std::string path) {
  std::error_code EC;
  llvm::raw_fd_ostream os(path, EC);
  if (EC) {
    std::cerr << "Cannot open output file: " << path << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
  // CGAnnotationWriter AW(func_ctxs);
  // M.print(os, &AW);
}

} // namespace notdec
