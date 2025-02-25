#include <cassert>
#include <clang/AST/Type.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <llvm/ADT/Optional.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Debug.h>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
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
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/AllSCCIterator.h"
#include "Utils/ValueNamer.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Range.h"
#include "notdec-llvm2c/StructManager.h"
#include "optimizers/ConstraintGenerator.h"
#include "optimizers/StackPointerFinder.h"
#include "utils.h"

#define DEBUG_TYPE "type-recovery"

namespace notdec {

using retypd::OffsetLabel;

ValueNamer ValueNamer::Instance = ValueNamer();
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

std::string getName(const std::set<llvm::Function *> &SCC) {
  std::string SCCNames;
  for (auto *F : SCC) {
    if (!SCCNames.empty()) {
      SCCNames += ",";
    }
    auto FName = F->getName().str();
    assert(!FName.empty());
    SCCNames += FName;
  }
  return SCCNames;
}

// When encountered a primitive node, ignore its edges.
std::set<CGNode *> countClosureFix(const std::set<CGNode *> &N) {
  auto isNotPrimitive = [](const CGNode *Node) {
    return !Node->key.Base.isPrimitive();
  };
  return retypd::NFADeterminizer<>::countClosure(N, isNotPrimitive);
}

std::string toString(const ExtValuePtr &Val) {
  std::string Ret;
  llvm::raw_string_ostream OS(Ret);
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    OS << "Value: " << **V;
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    OS << "ReturnValue: " + ValueNamer::getName(*F->Func, "func_");
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    OS << ValueNamer::getName(*const_cast<llvm::CallBase *>(Arg->Call)) +
              "_CallArg_" + std::to_string(Arg->Index);
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    OS << ValueNamer::getName(*const_cast<llvm::CallBase *>(Ret->Call)) +
              "_CallRet";
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    OS << "IntConstant: " << *IC->Val << ", User: " << *IC->User;
  } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
    OS << "ConstantAddr: " << *CA->Val;
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: getName: unhandled type of ExtValPtr\n";
    std::abort();
  }
  return Ret;
}

void dump(const ExtValuePtr &Val) { llvm::errs() << toString(Val) << "\n"; }

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

// static std::string getLLVMTypeBase(Type *Ty) {
//   if (Ty->isPointerTy()) {
//     // 20231122 why there is pointer constant(not inttoptr constant expr)
//     assert(false && "TODO can this be pointer?");
//     return getLLVMTypeBase(Ty->getPointerElementType()) + "*";
//   }
//   if (Ty->isFloatTy()) {
//     return "float";
//   } else if (Ty->isDoubleTy()) {
//     return "double";
//   } else if (Ty->isIntegerTy()) {
//     if (Ty->getIntegerBitWidth() == 1)
//       return "bool";
//     else
//       return "int";
//     llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
//                  << "Warn: unknown integer type: " << *Ty << "\n";
//     assert(false && "unknown integer type");
//   } else {
//     llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
//                  << "Warn: unknown constant type: " << *Ty << "\n";
//     assert(false && "unknown constant type");
//   }
// }

static inline TypeVariable makeTv(std::shared_ptr<retypd::TRContext> Ctx,
                                  std::string Name) {
  return retypd::TypeVariable::CreateDtv(*Ctx, Name);
}

// static inline TypeVariable getLLVMTypeVar(retypd::TRContext &Ctx, Type *Ty) {
//   return retypd::TypeVariable::CreatePrimitive(Ctx, getLLVMTypeBase(Ty));
// }

// static bool is32Or64Int(const Type *Ty) {
//   if (Ty->isIntegerTy()) {
//     return Ty->getIntegerBitWidth() == 32 || Ty->getIntegerBitWidth() == 64;
//   }
//   return false;
// }

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

llvm::Type *getType(const ExtValuePtr &Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    if (auto F = dyn_cast<Function>(*V)) {
      return F->getFunctionType();
    }
    return (*V)->getType();
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    return F->Func->getReturnType();
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    return Arg->Call->getArgOperand(Arg->Index)->getType();
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    return Ret->Call->getType();
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    return IC->Val->getType();
  } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
    return CA->Val->getType();
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getType: unhandled type of ExtValPtr\n";
  std::abort();
}

unsigned int getSize(llvm::Type *Ty, unsigned int pointer_size) {
  assert(pointer_size != 0);
  if (Ty->isPointerTy()) {
    return pointer_size;
  }
  if (!Ty->isAggregateType() && !Ty->isVectorTy()) {
    return Ty->getScalarSizeInBits();
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getSize: unhandled llvm type: " << *Ty << "\n";
  std::abort();
}

unsigned int getSize(const ExtValuePtr &Val, unsigned int pointer_size) {
  return getSize(getType(Val), pointer_size);
}

// TODO refactor: increase code reusability
ExtValuePtr getExtValuePtr(llvm::Value *V, User *User, long OpInd) {
  ExtValuePtr Val = V;
  if (!isa<GlobalValue>(*V)) {
    if (auto CI = dyn_cast<Constant>(V)) {
      assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
      assert(hasUser(V, User) &&
             "convertTypeVarVal: constant not used by user");
      Val = UConstant{.Val = cast<Constant>(V), .User = User, .OpInd = OpInd};
    }
  }
  return Val;
}

std::string getName(const ExtValuePtr &Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return ValueNamer::getName(**V);
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    return "ReturnValue_" + ValueNamer::getName(*F->Func, "func_");
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    return ValueNamer::getName(*const_cast<llvm::CallBase *>(Arg->Call)) +
           "_CallArg_" + std::to_string(Arg->Index);
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    return ValueNamer::getName(*const_cast<llvm::CallBase *>(Ret->Call)) +
           "_CallRet";
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    if (auto CI = dyn_cast<ConstantInt>(IC->Val)) {
      return "IntConstant_" + int_to_hex(CI->getSExtValue());
    } else {
      return "Constant_" + ValueNamer::getName(*IC->Val, "constant_");
    }
  } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
    return "ConstantAddr_" + int_to_hex(CA->Val->getSExtValue());
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getName: unhandled type of ExtValPtr\n";
  std::abort();
}

inline void dumpTypes(llvm::Value *V, clang::QualType CTy) {
  llvm::errs() << "  Value: " << *V << " has type: " << CTy.getAsString()
               << "\n";
}

// auto isGV = [](const ExtValuePtr &N) {
//   if (auto V = std::get_if<llvm::Value *>(&N)) {
//     if (llvm::isa<llvm::GlobalValue>(*V)) {
//       return true;
//     }
//   }
//   return false;
// };

TypeRecovery::Result TypeRecovery::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== RetypdGenerator  ===============\n");

  auto SP = MAM.getResult<StackPointerFinderAnalysis>(M);
  this->StackPointer = SP.result;

  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();

  std::shared_ptr<BytesManager> MemoryBytes = BytesManager::create(M);

  std::map<std::set<Function *>, std::shared_ptr<ConstraintsGenerator>>
      SummaryOverride;

  std::map<CallBase *, std::shared_ptr<ConstraintsGenerator>>
      CallsiteSummaryOverride;

  // load summary file
  const char *SummaryFile = std::getenv("NOTDEC_SUMMARY_OVERRIDE");
  if (SummaryFile) {
    if (getSuffix(SummaryFile) == ".json") {
      llvm::errs() << "Loading summary from: " << SummaryFile << "\n";
      auto ValE = json::parse(readFileToString(SummaryFile));
      assert(ValE && "JSON parse failed");
      auto Val = ValE->getAsObject();
      for (auto &Ent : *Val) {
        std::set<Function *> FSet;
        llvm::errs() << "Loading summary for function: " << Ent.first << "\n";
        for (auto Str : split(Ent.first, ',')) {
          auto *F = M.getFunction(Str);
          if (F == nullptr) {
            llvm::errs() << "Warning: Function not found: " << Ent.first.str()
                         << "\n";
            continue;
          }
          FSet.insert(F);
        }
        // Summary func not in this module.
        if (FSet.empty()) {
          continue;
        }
        retypd::ConstraintSummary Summary{{}, pointer_size, {}};
        Summary.fromJSON(*TRCtx, *Ent.second.getAsObject());
        auto CG = ConstraintsGenerator::fromConstraints(*this, FSet, Summary);
        SummaryOverride[FSet] = CG;
      }
    } else if (getSuffix(SummaryFile) == ".dot") {
      assert(false && "TODO");
    }
  }

  // load signature file
  std::map<std::set<Function *>, std::shared_ptr<ConstraintsGenerator>>
      SignatureOverride;
  const char *SigFile = std::getenv("NOTDEC_SIGNATURE_OVERRIDE");
  if (SigFile) {
    if (getSuffix(SigFile) == ".json") {
      llvm::errs() << "Loading signature from: " << SigFile << "\n";
      auto ValE = json::parse(readFileToString(SigFile));
      assert(ValE && "JSON parse failed");
      auto Val = ValE->getAsObject();
      for (auto &Ent : *Val) {
        std::set<Function *> FSet;
        llvm::errs() << "Loading signature for function: " << Ent.first << "\n";
        for (auto Str : split(Ent.first, ',')) {
          auto *F = M.getFunction(Str);
          if (F == nullptr) {
            llvm::errs() << "Warning: Function not found: " << Ent.first.str()
                         << "\n";
            continue;
          }
          FSet.insert(F);
        }
        // Sig func not in this module.
        if (FSet.empty()) {
          continue;
        }
        retypd::ConstraintSummary Summary{{}, pointer_size, {}};
        Summary.fromJSON(*TRCtx, *Ent.second.getAsObject());
        auto CG = ConstraintsGenerator::fromConstraints(*this, FSet, Summary);
        CG->CG.linkPrimitives();
        SignatureOverride[FSet] = CG;
      }
    } else if (getSuffix(SigFile) == ".dot") {
      assert(false && "TODO");
    }
  }

  const char *Traces = std::getenv("NOTDEC_TYPE_RECOVERY_TRACE_IDS");
  if (Traces) {
    std::string TraceStr(Traces);
    llvm::errs() << "Tracing: ";
    // split by ','
    for (auto &Id : llvm::split(TraceStr, ',')) {
      llvm::errs() << Id << ", ";
      TraceIds.insert(std::stoul(Id.str()));
    }
    llvm::errs() << "\n";
  }

  const char *DebugDir = std::getenv("NOTDEC_TYPE_RECOVERY_DEBUG_DIR");
  llvm::Optional<llvm::raw_fd_ostream> SCCsCatalog;
  if (DebugDir) {
    llvm::sys::fs::create_directories(DebugDir);
    std::error_code EC;
    SCCsCatalog.emplace(join(DebugDir, "SCCs.txt"), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file SCCs.txt." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
  }

  if (DebugDir) {
    printModule(M, join(DebugDir, "01-Original.ll").c_str());
  }

  // get the CallGraph, iterate by topological order of SCC
  CallGraph &CG = MAM.getResult<CallGraphAnalysis>(M);

  // override summary for printf
  // if (auto PF = M.getFunction("printf")) {
  //   auto N1 = CG[PF];
  //   // for each call, check if target is printf
  //   for (auto &Ent : CG) {
  //     auto Caller = Ent.first;
  //     CallGraphNode &CallerNode = *Ent.second;
  //     for (auto &CallEdge : *Ent.second) {
  //       if (!CallEdge.first.hasValue()) {
  //         continue;
  //       }
  //       CallBase *I =
  //           llvm::cast_or_null<llvm::CallBase>(&*CallEdge.first.getValue());
  //       auto *TargetNode = CallEdge.second;
  //       if (I && TargetNode->getFunction() == PF) {
  //         if (auto *CI = dyn_cast<ConstantInt>(I->getArgOperand(0))) {
  //           auto Format = CI->getValue().getZExtValue();
  //           StringRef FormatStr = MemoryBytes->decodeCStr(Format);
  //           if (!FormatStr.empty()) {
  //             std::shared_ptr<retypd::ConstraintSummary> Summary =
  //                 buildPrintfSummary(*TRCtx, pointer_size, FormatStr);
  //             auto CG =
  //                 ConstraintsGenerator::fromConstraints(*this, {PF},
  //                 *Summary);
  //             CallsiteSummaryOverride[I] = CG;
  //           }
  //         }
  //       }
  //     }
  //   }
  // }

  // TODO: simplify call graph if one func does not have up constraints.
  std::set<CallGraphNode *> Visited;
  all_scc_iterator<CallGraph *> CGI = notdec::scc_begin(&CG);

  struct SCCData {
    std::vector<CallGraphNode *> Nodes;
    std::shared_ptr<ConstraintsGenerator> Generator;
    std::string SCCName;
    std::shared_ptr<ConstraintsGenerator> TopDownGenerator;
  };

  // tuple(SCCNodes, ConstraintsGenerator, SCCName, TopDownGenerator)
  std::vector<SCCData> AllSCCs;
  std::map<CallGraphNode *, std::size_t> Func2SCCIndex;

  // Walk the callgraph in bottom-up SCC order.
  for (; !CGI.isAtEnd(); ++CGI) {
    const std::vector<CallGraphNode *> &NodeVec = *CGI;
    AllSCCs.push_back(SCCData{NodeVec, nullptr, "", nullptr});
    size_t SCCIndex = AllSCCs.size() - 1;
    llvm::Optional<std::string> DirPath;
    std::string SCCName = "SCC" + std::to_string(SCCIndex);
    if (DebugDir) {
      DirPath.emplace(join(DebugDir, SCCName));
      llvm::sys::fs::create_directories(*DirPath);
    }

    // Calc name for current SCC
    std::string Name;
    std::set<llvm::Function *> SCCs;
    for (auto CGN : NodeVec) {
      Visited.insert(CGN);
      if (CGN->getFunction() == nullptr) {
        continue;
      }
      if (!Name.empty()) {
        Name += ",";
      }
      Name += CGN->getFunction()->getName().str();
      SCCs.insert(CGN->getFunction());
      Func2SCCIndex[CGN] = SCCIndex;
    }
    if (SCCs.empty()) {
      continue;
    }

    std::cerr << "(Bottom-Up) Processing SCC: " << Name << "\n";
    std::shared_ptr<ConstraintsGenerator> Generator;

    bool isDeclaration = SCCs.size() == 1 && (*SCCs.begin())->isDeclaration();
    if (isDeclaration) {
      if (SummaryOverride.count(SCCs)) {
        std::cerr << "Override summary for external function: " << Name
                  << ":\n";
        Generator = SummaryOverride.at(SCCs);
        Generator->checkSymmetry();
      } else {
        llvm::errs() << "Warning: Summary and result may be incorrect due to "
                        "external function: "
                     << Name << "\n";
        continue;
      }
    } else {
      Generator = std::make_shared<ConstraintsGenerator>(*this, Name, SCCs);
      Generator->run();
    }

    // TODO: If the SCC/func is not called by any other function out of the SCC,
    // we can skip summary generation.

    if (DirPath) {
      Generator->CG.printGraph(join(*DirPath, "00-Generated.dot").c_str());
    }

    // 1. instantiate the summaries for each call.
    for (auto &Ent : Generator->CallToInstance) {
      auto *Call = Ent.first;
      auto *Target = Call->getCalledFunction();
      std::shared_ptr<ConstraintsGenerator> TargetSummary;
      if (Target->isDeclaration()) {
        if (CallsiteSummaryOverride.count(Call)) {
          llvm::errs() << "Override summary for callsite: " << *Call << "\n";
          TargetSummary = CallsiteSummaryOverride.at(Call);
        } else if (SummaryOverride.count({Target})) {
          std::cerr << "Override summary for external function: " << Name
                    << "\n";
          TargetSummary = SummaryOverride.at({Target});
          assert(TargetSummary->CG.PG->Constraints.size() == 0);
        } else {
          llvm::errs() << "Warning: Summary and result may be incorrect due to "
                          "external call: "
                       << *Call << "\n";
          Generator->UnhandledCalls.insert(Ent);
          continue;
        }
      } else {
        TargetSummary = FuncSummaries.at(Target);
      }
      if (TargetSummary != nullptr) {
        Generator->instantiateSummary(Call, Target, *TargetSummary);
      }
    }
    if (DirPath) {
      Generator->CG.printGraph(
          join(*DirPath, "01-InstantiateSummary.dot").c_str());
    }
    Generator->CG.solve();
    if (DirPath) {
      Generator->CG.printGraph(
          join(*DirPath, "01-InstantiateSummary.sat.dot").c_str());
    }

    std::shared_ptr<ConstraintsGenerator> Summary;
    if (isDeclaration) {
      Summary = Generator;
    } else if (SummaryOverride.count(SCCs)) {
      std::cerr << "Summary Overriden: " << Name << ":\n";
      Summary = SummaryOverride.at(SCCs);
    } else {
      std::cerr << "Generating Summary for " << Name << "\n";
      Summary = Generator->genSummary();
    }

    for (auto F : SCCs) {
      auto It = FuncCtxs.emplace(F, Generator);
      assert(It.second && "Function already in FuncCtxs?");
      auto It2 = FuncSummaries.emplace(F, Summary);
      assert(It2.second && "Function summary already exist?");
    }
    AllSCCs.back().Generator = Generator;
    AllSCCs.back().SCCName = Name;
    // write the SCC index to file
    if (SCCsCatalog) {
      *SCCsCatalog << "SCC" << SCCIndex << "," << Name << "\n";
    }
    if (DirPath) {
      if (Summary != nullptr) {
        Summary->CG.printGraph(join(*DirPath, "01-Summary.dot").c_str());
      } else {
        // write "no summary" to file
        std::ofstream NoSummary(join(*DirPath, "01-Summary.dot"));
        NoSummary << "digraph G {\n"
                     "  label=\"No Summary\"\n"
                     "}\n";
      }
    }
  }

  if (DebugDir) {
    SCCsCatalog->close();
    printModule(M, join(DebugDir, "02-AfterBottomUp.ll").c_str());
  }

  // map from function to all its callers
  std::map<CallGraphNode *,
           std::vector<std::pair<llvm::CallBase *, CallGraphNode *>>>
      FuncCallers;
  // 5. incrementally maintain reverse call edge map
  for (auto &Ent : CG) {
    CallGraphNode *CallerN = Ent.second.get();
    if (CallerN == nullptr) {
      continue;
    }
    auto *Current = CallerN->getFunction();
    if (Current == nullptr) {
      continue;
    }
    for (auto &Edge : *CallerN) {
      auto *CalleeN = Edge.second;
      if (!Edge.first.hasValue()) {
        continue;
      }
      CallBase *I = llvm::cast<llvm::CallBase>(&*Edge.first.getValue());
      auto &CallVec = FuncCallers[CalleeN];
      CallVec.emplace_back(I, CallerN);
    }
  }

  // Top-down Phase: build the result(Map from value to clang C type)
  // We have a big global type graph, corresponds to C AST that link the
  // declared struct type to the real definition to form a graph.

  // Steps:
  // TODO

  TypeRecovery::Result Result;
  retypd::SketchToCTypeBuilder TB(M.getName());

  // merge nodes to current graph by determinize.
  auto determinizeTo = [&](ConstraintsGenerator &CurrentTypes,
                           std::set<CGNode *> &StartNodes,
                           const char *NamePrefix) -> CGNode * {
    auto &CG = CurrentTypes.CG;
    std::map<std::set<CGNode *>, CGNode *> DTrans;
    using EntryTy = typename std::map<std::set<CGNode *>, CGNode *>::iterator;

    auto ignoreForgetAndOne = [](const retypd::EdgeLabel &L) {
      auto isForget = std::holds_alternative<retypd::ForgetLabel>(L);
      auto isOne = std::holds_alternative<retypd::One>(L);
      if (isForget || isOne) {
        return false;
      }
      return true;
    };

    auto getOrSetNewNode = [&](const std::set<CGNode *> &N) -> EntryTy {
      if (DTrans.count(N)) {
        return DTrans.find(N);
      }
      bool hasEnd = false;
      // bool allEnd = true;
      for (auto *Node : N) {
        if (Node->key.Base.isEnd()) {
          hasEnd = true;
        } else {
          // allEnd = false;
        }
      }
      // assert(hasEnd == allEnd);
      CGNode *NewNode;
      if (hasEnd) {
        NewNode = CG.getEndNode();
      } else {
        // auto *PN = notdec::retypd::NFADeterminizer<>::ensureSamePNI(N);
        // assert(PN->getLowTy() != nullptr);
        NewNode = &CG.createNodeClonePNI(
            retypd::NodeKey{TypeVariable::CreateDtv(
                *CG.Ctx, ValueNamer::getName(NamePrefix))},
            (*N.begin())->getPNIVar());
        for (auto N1 : N) {
          NewNode->getPNIVar()->merge(N1->getPNIVar()->getLatticeTy());
        }
        if (NewNode->getPNIVar()->isConflict()) {
          notdec::retypd::NFADeterminizer<>::printPNDiffSet(N);
        }
      }

      auto it = DTrans.emplace(N, NewNode);
      assert(it.second);
      return it.first;
    };

    std::queue<EntryTy> Worklist;
    Worklist.push(getOrSetNewNode(countClosureFix(StartNodes)));
    auto *Ret = Worklist.front()->second;

    while (!Worklist.empty()) {
      auto It = Worklist.front();
      auto &Node = *It->second;
      std::set<retypd::EdgeLabel> outLabels =
          retypd::NFADeterminizer<>::allOutLabels(It->first,
                                                  ignoreForgetAndOne);
      for (auto &L : outLabels) {
        auto S = countClosureFix(retypd::NFADeterminizer<>::move(It->first, L));
        if (DTrans.count(S) == 0) {
          auto NewNodeEnt = getOrSetNewNode(S);
          Worklist.push(NewNodeEnt);
        }
        auto &ToNode = *DTrans.at(S);
        assert(std::holds_alternative<retypd::RecallLabel>(L) ||
               std::holds_alternative<retypd::ForgetBase>(L));
        if (std::holds_alternative<retypd::RecallLabel>(L)) {
          CG.addEdge(Node, ToNode, L);
          CG.addEdge(
              ToNode, Node,
              retypd::ForgetLabel{std::get<retypd::RecallLabel>(L).label});
        } else if (std::holds_alternative<retypd::ForgetBase>(L)) {
          assert(&ToNode == CG.getEndNode());
          CG.addEdge(Node, ToNode, L);
          CG.addEdge(
              *CG.getStartNode(), Node,
              retypd::RecallBase{.Base = std::get<retypd::ForgetBase>(L).Base,
                                 .V = std::get<retypd::ForgetBase>(L).V});
        }
      }
      Worklist.pop();
    }
    return Ret;
  };

  for (std::size_t Index1 = AllSCCs.size(); Index1 > 0; --Index1) {
    std::size_t SCCIndex = Index1 - 1;
    auto &Data = AllSCCs[SCCIndex];
    std::string SCCDebugFolderName = "SCC" + std::to_string(SCCIndex);
    Data.TopDownGenerator =
        std::make_shared<ConstraintsGenerator>(*this, Data.SCCName);
    auto &CurrentTypes = *(Data.TopDownGenerator);
    assert(CurrentTypes.PG);

    llvm::Optional<std::string> DirPath;
    if (DebugDir) {
      DirPath.emplace(join(DebugDir, SCCDebugFolderName));
      // llvm::sys::fs::create_directories(*DirPath);
    }

    const std::vector<CallGraphNode *> &NodeVec = Data.Nodes;
    if (Data.Generator == nullptr) {
      continue;
    }
    std::shared_ptr<ConstraintsGenerator> &Generator = Data.Generator;
    auto &Name = Data.SCCName;

    std::cerr << "(Top-Down) Processing Func: " << Name << "\n";

    // Collect all functions for SCC checking
    std::set<llvm::Function *> SCCSet;
    for (auto CGN : NodeVec) {
      if (CGN->getFunction() == nullptr) {
        continue;
      }
      SCCSet.insert(CGN->getFunction());
    }

    if (DirPath) {
      Generator->CG.printGraph(join(*DirPath, "03-Original.dot").c_str());
    }

    std::map<const CGNode *, CGNode *> Old2New;
    Generator->cloneTo(CurrentTypes, Old2New);

    // 2. collect and merge actual params
    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }

      std::set<CGNode *> FuncNodes;
      std::set<CGNode *> FuncNodesContra;

      if (SignatureOverride.count(SCCSet)) {
        std::cerr << "Function Signature Overriden: " << Name << "\n";
        auto &SigGen = SignatureOverride.at(SCCSet);
        SigGen->CG.linkPrimitives();

        auto SigNode =
            SigGen->getNodeOrNull(Current, nullptr, -1, retypd::Covariant);
        auto SigNodeC =
            SigGen->getNodeOrNull(Current, nullptr, -1, retypd::Contravariant);
        assert(SigNode || SigNodeC);
        if (SigNode) {
          FuncNodes.insert(SigNode);
        }
        if (SigNodeC) {
          FuncNodesContra.insert(SigNodeC);
        }
      } else {
        if (FuncCallers.count(CGN) == 0) {
          continue;
        }
        auto &Callers = FuncCallers.at(CGN);
        if (Callers.empty()) {
          std::cerr << "No actual param types for " << Current->getName().str()
                    << "\n";
        }
        if (Current->isVarArg()) {
          std::cerr << "TODO: Support vararg function: "
                    << Current->getName().str() << "\n";
          std::abort();
        }
        for (auto &Elem : Callers) {
          auto *Call = Elem.first;
          auto &CallerGenerator =
              AllSCCs.at(Func2SCCIndex.at(Elem.second)).TopDownGenerator;
          auto [FN, FNC] = CallerGenerator->CallToInstance.at(Call);

          FuncNodes.insert(FN);
          FuncNodesContra.insert(FNC);
        }
      }

      auto *D = determinizeTo(CurrentTypes, FuncNodes, "act_");
      CurrentTypes.addSubtype(
          CurrentTypes.getNode(Current, nullptr, -1, retypd::Covariant), *D);

      auto *DC = determinizeTo(CurrentTypes, FuncNodes, "actc_");
      CurrentTypes.addSubtype(*DC, CurrentTypes.getNode(Current, nullptr, -1,
                                                        retypd::Contravariant));

      CurrentTypes.makeSymmetry();
    }

    if (DirPath) {
      CurrentTypes.CG.printGraph(join(*DirPath, "05-CurrentTypes.dot").c_str());
    }

    // ensure lower bound is lower than upper bound
    CurrentTypes.linkContraToCovariant();
    CurrentTypes.CG.solve();
    CurrentTypes.CG.linkConstantPtr2Memory();
    // link primitives for all Graphs in TopDownGenerator.
    // 1. later postProcess will also do this.
    // 2. our determinization for actual params need this.
    CurrentTypes.CG.linkPrimitives();

    if (DirPath) {
      CurrentTypes.CG.printGraph(
          join(*DirPath, "05-CurrentTypes.sat.dot").c_str());
    }
  }

  // Create type for memory node
  Global = std::make_shared<ConstraintsGenerator>(*this, "Global");
  std::set<CGNode *> MemoryNodes;
  std::set<CGNode *> MemoryNodesC;
  for (auto &Ent : AllSCCs) {
    auto &G = *(Ent.TopDownGenerator);
    if (auto *M = G.CG.getMemoryNodeOrNull(retypd::Covariant)) {
      if (M->outEdges.empty()) {
        continue;
      }
      MemoryNodes.insert(M);
    }
    if (auto *M = G.CG.getMemoryNodeOrNull(retypd::Contravariant)) {
      if (M->outEdges.empty()) {
        continue;
      }
      MemoryNodesC.insert(M);
    }
  }
  CGNode *MemNode = nullptr;
  if (MemoryNodes.empty()) {
    llvm::errs() << "No memory node found\n";
    MemNode = Global->CG.getMemoryNode(retypd::Covariant);
  } else {
    MemNode = determinizeTo(*Global, MemoryNodes, "mdtm");
    // Global->mergeAfterDeterminize();
  }
  CGNode *MemNodeC = nullptr;
  if (MemoryNodesC.empty()) {
    llvm::errs() << "No memory node found\n";
    MemNodeC = Global->CG.getMemoryNode(retypd::Contravariant);
  } else {
    MemNodeC = determinizeTo(*Global, MemoryNodesC, "mdtmc");
    // Global->mergeAfterDeterminize();
  }
  // a special value to represent the memory node
  MemNode->getPNIVar()->unify(*MemNodeC->getPNIVar());
  Global->CG.onlyAddEdge(*Global->CG.getMemoryNode(retypd::Covariant), *MemNode,
                         retypd::One{});
  Global->CG.onlyAddEdge(*MemNodeC,
                         *Global->CG.getMemoryNode(retypd::Contravariant),
                         retypd::One{});
  // keep node in val2node map for post process
  Global->V2N.insert(ConstantAddr(),
                     Global->CG.getMemoryNode(retypd::Covariant)->key);
  Global->V2NContra.insert(
      ConstantAddr(), Global->CG.getMemoryNode(retypd::Contravariant)->key);

  std::cerr << "Bottom up phase done! SCC count:" << AllSCCs.size() << "\n";

  if (DebugDir) {
    Global->CG.printGraph(join(DebugDir, "GlobalMemory.dot").c_str());
    printAnnotatedModule(M, join(DebugDir, "02-AfterBottomUp.anno.ll").c_str());
  }

  auto Convert = [&](ExtValuePtr Val) -> llvm2c::WValuePtr {
    if (auto V = std::get_if<llvm::Value *>(&Val)) {
      return *V;
    } else if (auto F = std::get_if<ReturnValue>(&Val)) {
      return llvm2c::RetVal{.Func = F->Func, .Index = F->Index};
    } else if (auto Arg = std::get_if<CallArg>(&Val)) {
      assert(false);
    } else if (auto Ret = std::get_if<CallRet>(&Val)) {
      assert(false);
    } else if (auto IC = std::get_if<UConstant>(&Val)) {
      return llvm2c::UsedConstant(IC->Val, IC->User, IC->OpInd);
    } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
      return llvm2c::ConsAddr{.Val = CA->Val};
    }
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: getName: unhandled type of ExtValPtr\n";
    std::abort();
  };

  auto postProcess = [&](ConstraintsGenerator &G,
                         std::map<const CGNode *, CGNode *> &Old2New,
                         std::string DebugDir) -> ConstraintsGenerator {
    ConstraintsGenerator G2 = G.clone(Old2New);
    assert(G2.PG);
    std::string Name = G2.CG.Name;
    G2.CG.Name += "-dtm";
    // if (DebugDir) {
    //   G2.DebugDir = DebugDir;
    // }

    G2.CG.sketchSplit();
    // G2.CG.ensureNoForgetLabel();

    G2.eliminateCycle();
    // G2.CG.ensureNoForgetLabel();

    if (!DebugDir.empty()) {
      G2.CG.printGraph(join(DebugDir, "06-BeforeMerge.dot").c_str());
    }

    // merge nodes that only subtype to another node
    G2.mergeOnlySubtype();
    // G2.CG.ensureNoForgetLabel();

    if (!DebugDir.empty()) {
      G2.CG.printGraph(join(DebugDir, "07-BeforeDtm.dot").c_str());
    }

    G2.determinize();
    // G2.CG.ensureNoForgetLabel();
    G2.CG.aggressiveSimplify();
    // G2.CG.ensureNoForgetLabel();

    if (!DebugDir.empty()) {
      G2.CG.printGraph(join(DebugDir, "08-Final.dot").c_str());
    }

    assert(G2.PG);
    // G2.DebugDir.clear();
    return G2;
  };

  llvm::Optional<llvm::raw_fd_ostream> ValueTypesFile;
  if (DebugDir) {
    std::error_code EC;
    ValueTypesFile.emplace(join(DebugDir, "ValueTypes.txt"), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file ValueTypes.txt." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
  }

  // build AST type for each value in value map
  for (int i = 0; i < AllSCCs.size(); i++) {
    auto &G = *AllSCCs[i].TopDownGenerator;
    assert(G.PG);
    std::map<const CGNode *, CGNode *> Old2New;
    for (auto &Ent : G.CG.Nodes) {
      if (Ent.second.isStartOrEnd() || Ent.second.isMemory()) {
        continue;
      }
    }
    std::string Dir;
    if (DebugDir) {
      Dir = join(DebugDir, "SCC" + std::to_string(i));
    }

    ConstraintsGenerator G2 = postProcess(G, Old2New, Dir);

    if (!Dir.empty()) {
      G2.CG.printGraph(join(Dir, "09-CurrentTypes.dtm.dot").c_str());
      // TODO refactor to annotated function
      // print val2node here
      std::ofstream Val2NodeFile(join(Dir, "09-Val2Node.txt"));
      for (auto &Ent : G2.V2N) {
        auto &N = G2.getNode(Ent.first, nullptr, -1, retypd::Covariant);
        auto &NC = G2.getNode(Ent.first, nullptr, -1, retypd::Contravariant);
        Val2NodeFile << getName(Ent.first) << ", " << toString(N.key) << ", "
                     << toString(NC.key) << "\n";
      }
      Val2NodeFile.close();
    }

    for (auto &Ent : G2.V2N) {
      // TODO support function type.
      if (std::holds_alternative<llvm::Value *>(Ent.first)) {
        if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
          continue;
        }
      }
      auto &Key = Ent.second;
      auto *Node = G2.getNodeOrNull(Key);
      if (Node != nullptr) {
        assert(&Node->Parent == &G2.CG &&
               "RetypdGenerator::getTypeVar: Node is not in the graph");
        auto Size = getSize(Ent.first, pointer_size);
        assert(Size > 0);

        if (ValueTypesFile) {
          // print the current value
          if (auto *V = std::get_if<llvm::Value *>(&Ent.first)) {
            *ValueTypesFile << "  Value: " << **V;
            llvm::Function *F = nullptr;
            if (auto I = llvm::dyn_cast<Instruction>(*V)) {
              F = I->getFunction();

            } else if (auto Arg = llvm::dyn_cast<Argument>(*V)) {
              F = Arg->getParent();
            }
            if (F) {
              *ValueTypesFile << " (In Func: " << F->getName() << ")";
            }
            // DEBUG
            // if ((*V)->getName() == "stack31") {
            //   *ValueTypesFile << "here";
            // }
          } else {
            *ValueTypesFile << "  Special Value: " << getName(Ent.first);
            if (auto *UC = std::get_if<UConstant>(&Ent.first)) {
              *ValueTypesFile << " User: " << *UC->User;
            }
          }
        }

        clang::QualType CTy = TB.buildType(*Node);

        if (Result.ValueTypes.count(Convert(Ent.first)) != 0) {
          llvm::errs() << "Warning: TODO handle Value type merge (LowerBound): "
                       << toString(Ent.first) << "\n";
        }

        Result.ValueTypes[Convert(Ent.first)] = CTy;

        if (ValueTypesFile) {
          *ValueTypesFile << " upper bound: " << CTy.getAsString();
        }
      } else {
        if (ValueTypesFile) {
          *ValueTypesFile << " has no upper bound";
        }
      }
      if (ValueTypesFile) {
        if (Node->getPNIVar() != nullptr) {
          *ValueTypesFile << "  PNI: " << Node->getPNIVar()->str();
        }
        *ValueTypesFile << "\n";
      }
    }

    for (auto &Ent : G2.V2NContra) {
      // TODO support function type.
      if (std::holds_alternative<llvm::Value *>(Ent.first)) {
        if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
          continue;
        }
      }
      auto &Key = Ent.second;
      auto *Node = G2.getNodeOrNull(Key);
      if (Node != nullptr) {
        assert(&Node->Parent == &G2.CG &&
               "RetypdGenerator::getTypeVar: Node is not in the graph");
        auto Size = getSize(Ent.first, pointer_size);
        assert(Size > 0);

        if (ValueTypesFile) {
          // print the current value
          if (auto *V = std::get_if<llvm::Value *>(&Ent.first)) {
            *ValueTypesFile << "  Value: " << **V;
            llvm::Function *F = nullptr;
            if (auto I = llvm::dyn_cast<Instruction>(*V)) {
              F = I->getFunction();

            } else if (auto Arg = llvm::dyn_cast<Argument>(*V)) {
              F = Arg->getParent();
            }
            if (F) {
              *ValueTypesFile << " (In Func: " << F->getName() << ")";
            }
            // DEBUG
            // if ((*V)->getName() == "stack31") {
            //   *ValueTypesFile << "here";
            // }
          } else {
            *ValueTypesFile << "  Special Value: " << getName(Ent.first);
            if (auto *UC = std::get_if<UConstant>(&Ent.first)) {
              *ValueTypesFile << " User: " << *UC->User;
            }
          }
        }

        clang::QualType CTy = TB.buildType(*Node);

        if (Result.ValueTypesLowerBound.count(Convert(Ent.first)) != 0) {
          llvm::errs() << "Warning: TODO handle Value type merge (UpperBound): "
                       << toString(Ent.first) << "\n";
        }
        Result.ValueTypesLowerBound[Convert(Ent.first)] = CTy;
        if (ValueTypesFile) {
          *ValueTypesFile << " lower bound: " << CTy.getAsString();
        }
      } else {
        if (ValueTypesFile) {
          *ValueTypesFile << " has no lower bound";
        }
      }
      if (ValueTypesFile) {
        if (Node->getPNIVar() != nullptr) {
          *ValueTypesFile << "  PNI: " << Node->getPNIVar()->str();
        }
        *ValueTypesFile << "\n";
      }
    }
  }

  std::map<const CGNode *, CGNode *> Old2NewGlobal;
  ConstraintsGenerator Global2 = postProcess(*Global, Old2NewGlobal, "");
  // CGNode &MemNode2 = Global2.getNode(ConstantAddr(), nullptr, -1,
  // retypd::Covariant);
  CGNode *MemNode2 = Global2.CG.getMemoryNode(retypd::Covariant);
  // build AST type for memory node
  clang::QualType CTy = TB.buildType(*MemNode2);
  llvm::errs() << "Memory Type: " << CTy.getAsString();

  clang::RecordDecl *Mem =
      CTy->getPointeeType()->getAs<clang::RecordType>()->getDecl();
  auto &Info = TB.StructInfos[Mem];
  Info.Bytes = MemoryBytes;
  Info.resolveInitialValue();

  Result.MemoryType = CTy->getPointeeType();

  // move the ASTUnit to result
  Result.ASTUnit = std::move(TB.ASTUnit);
  Result.DeclComments = std::move(TB.DeclComments);
  Result.StructInfos = std::move(TB.StructInfos);
  Result.AllDecls = std::move(TB.AllDecls);

  // gen_json("retypd-constrains.json");

  // TODO convert the type back to LLVM IR
  // print(M, "after-TypeRecovery.ll");

  LLVM_DEBUG(errs() << " ============== RetypdGenerator End ===============\n");
  return Result;
}

void ConstraintsGenerator::makeSymmetry() {
  for (auto &Ent : CG.Nodes) {
    if (Ent.second.isStartOrEnd()) {
      continue;
    }
    auto &N = Ent.second;
    auto &NC = CG.getOrInsertNodeWithPNI(retypd::MakeReverseVariant(N.key),
                                         N.getPNIVar());
    for (auto &Edge : N.outEdges) {
      if (std::holds_alternative<retypd::RecallBase>(Edge.getLabel()) ||
          std::holds_alternative<retypd::ForgetBase>(Edge.getLabel())) {
        continue;
      }
      auto &T = const_cast<CGNode &>(Edge.getTargetNode());
      auto &TC = CG.getOrInsertNodeWithPNI(retypd::MakeReverseVariant(T.key),
                                           T.getPNIVar());
      auto &Label = Edge.getLabel();
      if (std::holds_alternative<retypd::One>(Label)) {
        addSubtype(N, T);
      } else if (auto RL = std::get_if<retypd::RecallLabel>(&Label)) {
        CG.addEdge(T, N, retypd::ForgetLabel{RL->label});
        CG.addEdge(NC, TC, retypd::RecallLabel{RL->label});
        CG.addEdge(TC, NC, retypd::ForgetLabel{RL->label});
      } else if (auto FL = std::get_if<retypd::ForgetLabel>(&Label)) {
        CG.addEdge(T, N, retypd::RecallLabel{FL->label});
        CG.addEdge(NC, TC, retypd::ForgetLabel{FL->label});
        CG.addEdge(TC, NC, retypd::RecallLabel{FL->label});
      }
    }
  }
}

bool ConstraintsGenerator::checkSymmetry() {
  for (auto &Ent : CG.Nodes) {
    if (Ent.second.isStartOrEnd()) {
      continue;
    }
    auto &N = Ent.second;
    auto &NC = getNode(retypd::MakeReverseVariant(N.key));
    for (auto &Edge : N.outEdges) {
      auto &T = const_cast<CGNode &>(Edge.getTargetNode());
      auto &TC = getNode(retypd::MakeReverseVariant(T.key));
      auto &Label = Edge.getLabel();
      if (std::holds_alternative<retypd::One>(Label)) {
        // ensure there is a edge for the contra node.
        assert(CG.hasEdge(TC, NC, retypd::One{}) && "Symmetry check failed");
      } else if (auto RL = std::get_if<retypd::RecallLabel>(&Label)) {
        assert(CG.hasEdge(T, N, retypd::ForgetLabel{RL->label}));
        assert(CG.hasEdge(NC, TC, retypd::RecallLabel{RL->label}));
        assert(CG.hasEdge(TC, NC, retypd::ForgetLabel{RL->label}));
      } else if (auto FL = std::get_if<retypd::ForgetLabel>(&Label)) {
        assert(CG.hasEdge(T, N, retypd::RecallLabel{FL->label}));
        assert(CG.hasEdge(NC, TC, retypd::ForgetLabel{FL->label}));
        assert(CG.hasEdge(TC, NC, retypd::RecallLabel{FL->label}));
      }
    }
  }
  return true;
}

ConstraintsGenerator
ConstraintsGenerator::clone(std::map<const CGNode *, CGNode *> &Old2New) {
  ConstraintsGenerator G(Ctx, CG.Name);
  cloneTo(G, Old2New);
  return G;
}

void ConstraintsGenerator::cloneTo(
    ConstraintsGenerator &G, std::map<const CGNode *, CGNode *> &Old2New) {
  ConstraintGraph::clone(Old2New, CG, G.CG);
  assert(&Ctx == &G.Ctx);
  for (auto &Ent : V2N) {
    // this is NodeKey, no need to map.
    G.V2N.insert(Ent.first, Ent.second);
  }
  for (auto &Ent : V2NContra) {
    G.V2NContra.insert(Ent.first, Ent.second);
  }
  G.PG = G.CG.PG.get();
  G.SCCs = SCCs;
  G.DebugDir = DebugDir;
  for (auto &Ent : CallToInstance) {
    G.CallToInstance.insert(
        {Ent.first,
         {Old2New.at(Ent.second.first), Old2New.at(Ent.second.second)}});
  }
  for (auto &Ent : UnhandledCalls) {
    G.UnhandledCalls.insert(
        {Ent.first,
         {Old2New.at(Ent.second.first), Old2New.at(Ent.second.second)}});
  }
}

void ConstraintsGenerator::removeUnreachable() {
  assert(CG.getStartNode()->outEdges.size() == 0);
  std::set<CGNode *> ReachableNodes;

  std::queue<CGNode *> Worklist;
  for (auto &Ent : V2N) {
    auto *Node = &CG.getNode(Ent.second);
    Worklist.push(Node);
  }
  // also add contravariant node
  for (auto &Ent : V2NContra) {
    auto *Node = &CG.getNode(Ent.second);
    Worklist.push(Node);
  }
  Worklist.push(CG.getStartNode());
  Worklist.push(CG.getEndNode());
  if (auto N = CG.getMemoryNodeOrNull(retypd::Covariant)) {
    Worklist.push(N);
  }
  if (auto N = CG.getMemoryNodeOrNull(retypd::Contravariant)) {
    Worklist.push(N);
  }
  while (!Worklist.empty()) {
    auto *Node = Worklist.front();
    if (ReachableNodes.count(Node) == 0) {
      ReachableNodes.insert(Node);
      for (auto &Edge : Node->outEdges) {
        Worklist.push(&const_cast<CGNode &>(Edge.getTargetNode()));
      }
    }
    Worklist.pop();
  }

  // remove all unreachable nodes
  std::vector<CGNode *> ToErase;
  for (auto &Ent : CG) {
    auto *Node = &Ent.second;
    if (Node == CG.getStartNode() || Node == CG.getEndNode()) {
      continue;
    }
    if (ReachableNodes.count(Node) == 0) {
      ToErase.push_back(Node);
      // erase all out edges
      for (auto &Edge : Ent.second.outEdges) {
        CG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
      }
    }
  }
  for (auto *Node : ToErase) {
    CG.removeNode(Node->key);
  }
}

void ConstraintsGenerator::linkContraToCovariant() {
  for (auto &Ent : V2N) {
    auto *N = &CG.getNode(Ent.second);
    if (V2NContra.count(Ent.first) == 0) {
      assert(false);
    }
    auto *CN = &CG.getNode(V2NContra.at(Ent.first));
    assert(CN->getPNIVar() == N->getPNIVar());
    CG.addEdge(*CN, *N, retypd::One{});
  }
}

std::set<CGNode *> moveLoadEqStore(const std::set<CGNode *> &N,
                                   retypd::EdgeLabel L) {
  std::set<CGNode *> ret;
  for (auto Node : N) {
    for (auto &Edge : Node->outEdges) {
      if (Edge.getLabel() == L) {
        ret.insert(&const_cast<CGNode &>(Edge.getTargetNode()));
      }
      // view store == load
      if (auto Rec = std::get_if<retypd::RecallLabel>(&Edge.getLabel())) {
        if (auto Rec2 = std::get_if<retypd::RecallLabel>(&L)) {
          if (std::holds_alternative<retypd::LoadLabel>(Rec->label) &&
              std::holds_alternative<retypd::StoreLabel>(Rec2->label)) {
            ret.insert(&const_cast<CGNode &>(Edge.getTargetNode()));
          }
          if (std::holds_alternative<retypd::StoreLabel>(Rec->label) &&
              std::holds_alternative<retypd::LoadLabel>(Rec2->label)) {
            ret.insert(&const_cast<CGNode &>(Edge.getTargetNode()));
          }
        }
      }
    }
  }
  return ret;
}

// view subtype edges as bidirectional.
// void ConstraintsGenerator::determinizeStructEqual() {
//   // assert(DTrans.empty());
//   DTrans.clear();
//   std::map<const CGNode *, CGNode *> This2Bak;
//   ConstraintGraph Backup = CG.clone(This2Bak, true);
//   // remove all edges in the graph
//   for (auto &Ent : CG) {
//     for (auto &Edge : Ent.second.outEdges) {
//       // TODO optimize:
//       CG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
//     }
//   }
//   // remove all node that is not in the value map
//   std::set<CGNode *> Nodes;
//   for (auto &Ent : Val2Node) {
//     Nodes.insert(&CG.getNode(Ent.second));
//   }
//   for (auto &Ent : CG) {
//     if (&Ent.second == CG.getStartNode() || &Ent.second == CG.getEndNode()) {
//       continue;
//     }
//     if (Nodes.count(&Ent.second) == 0) {
//       CG.removeNode(Ent.second.key);
//     }
//   }

//   using EntryTy = typename std::map<std::set<CGNode *>, CGNode *>::iterator;

//   auto getOrSetNewNode = [this](const std::set<CGNode *> &N) -> EntryTy {
//     if (DTrans.count(N)) {
//       return DTrans.find(N);
//     }
//     auto *OldPN = notdec::retypd::NFADeterminizer<>::ensureSamePNI(N);
//     auto &NewNode =
//         CG.createNodeClonePNI(retypd::NodeKey{TypeVariable::CreateDtv(
//                                   Ctx.TRCtx, ValueNamer::getName("dtm_"))},
//                               OldPN);
//     auto it = DTrans.emplace(N, &NewNode);
//     assert(it.second);
//     return it.first;
//   };

//   // map from replaced node to the new node
//   std::map<CGNode *, CGNode *> ReplaceMap;
//   auto Lookup = [&ReplaceMap](CGNode *Node) -> CGNode * {
//     std::vector<CGNode *> PathNodes;
//     while (ReplaceMap.count(Node)) {
//       PathNodes.push_back(Node);
//       Node = ReplaceMap.at(Node);
//     }
//     for (auto *N : PathNodes) {
//       ReplaceMap[N] = Node;
//     }
//     return Node;
//   };

//   DTrans[{Backup.getEndNode()}] = CG.getEndNode();
//   std::queue<EntryTy> Worklist;
//   // for each node in the value map
//   for (auto &Ent : Val2Node) {
//     auto *BakNode = &Backup.getNode(Ent.second);
//     std::set<CGNode *> StartSet = countClosureStructEq({BakNode});
//     auto pair1 = DTrans.emplace(StartSet, Ent.second);
//     if (pair1.second) {
//       Worklist.push(pair1.first);
//     } else {
//       // Can be a epsilon loop.
//       // TODO merge the node in the value map
//       if (pair1.first->second != Ent.second) {
//         // merge the node in the value map
//         std::cerr
//             << "ConstraintsGenerator::determinizeStructEqual: Merging node "
//             << toString(Ent.second->key) << " to "
//             << toString(pair1.first->second->key) << "\n";
//         // Ent.second = pair1.first->second;
//         // CG.removeNode(Ent.second->key);
//         ReplaceMap.emplace(Ent.second, pair1.first->second);
//         // assert(false);
//       }
//     }
//   }

//   // DSU flatten for Val2Node
//   for (auto &Ent : Val2Node) {
//     if (ReplaceMap.count(Ent.second)) {
//       Ent.second = Lookup(Ent.second);
//     }
//   }

//   // remove all node that is not in the value map again
//   Nodes.clear();
//   for (auto &Ent : Val2Node) {
//     Nodes.insert(Ent.second);
//   }
//   for (auto &Ent : CG) {
//     if (&Ent.second == CG.getStartNode() || &Ent.second == CG.getEndNode()) {
//       continue;
//     }
//     if (Nodes.count(&Ent.second) == 0) {
//       CG.removeNode(Ent.second.key);
//     }
//   }

//   while (!Worklist.empty()) {
//     auto It = Worklist.front();
//     auto &Node = *It->second;
//     std::set<retypd::EdgeLabel> outLabels =
//         retypd::NFADeterminizer<>::allOutLabels(It->first);
//     for (auto &L : outLabels) {
//       auto S = countClosureStructEq(moveLoadEqStore(It->first, L));
//       if (S.count(Backup.getEndNode())) {
//         assert(S.size() == 1);
//       }
//       if (DTrans.count(S) == 0) {
//         auto NewNodeEnt = getOrSetNewNode(S);
//         Worklist.push(NewNodeEnt);
//       }
//       auto &ToNode = *DTrans.at(S);
//       CG.onlyAddEdge(Node, ToNode, L);
//     }
//     Worklist.pop();
//   }
// }

void ConstraintsGenerator::eliminateCycle() {
  std::map<const CGNode *, CGNode *> Old2New;
  ConstraintGraph NewG = CG.clone(Old2New);

  // remove all edges except one edge, also remove one edge to primitive nodes
  for (auto &Ent : NewG) {
    for (auto &Edge : Ent.second.outEdges) {
      if (std::holds_alternative<retypd::One>(Edge.Label)) {
        if (!Edge.TargetNode.key.Base.isPrimitive()) {
          continue;
        }
      }

      NewG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
    }
  }

  // detect loops and merge them
  // get scc iterator
  all_scc_iterator<ConstraintGraph *> SCCI = notdec::scc_begin(&NewG);

  // build New2Old map
  std::map<CGNode *, CGNode *> New2Old;
  for (auto &Ent : Old2New) {
    New2Old.emplace(Ent.second, const_cast<CGNode *>(Ent.first));
  }

  for (; !SCCI.isAtEnd(); ++SCCI) {
    const std::vector<CGNode *> &SCC = *SCCI;
    if (SCC.size() == 1) {
      continue;
    }
    // merge the nodes in the value map:
    // 1. collect all nodes that mapped to values in the SCC
    // these values will be mapped to the single merged node
    std::set<CGNode *> ToMerge;
    std::set<ExtValuePtr> ToMergeVal;
    for (auto *Node : SCC) {
      Node = New2Old.at(Node);
      ToMerge.insert(Node);
      if (V2N.rev().count(Node->key) == 0) {
        continue;
      }
      for (auto &Val : V2N.rev().at(Node->key)) {
        ToMergeVal.insert(Val);
      }
    }
    // 2. output the merged node name map to txt
    auto &Merged = **ToMerge.begin();
    if (!this->DebugDir.empty()) {
      std::ofstream Out(join(DebugDir, "Cycles.txt"), std::ios::app);
      for (auto *Node : ToMerge) {
        Out << toString(Node->key) << ", ";
      }
      Out << "\n";
      Out.close();
    }

    // auto *OldPN =
    notdec::retypd::NFADeterminizer<>::ensureSamePNI(ToMerge);
    // 3. perform node merging
    for (auto *Node : ToMerge) {
      if (Node == &Merged) {
        continue;
      }
      mergeNodeTo(*Node, Merged, true);
    }
  }
}

void ConstraintsGenerator::mergeNodeTo(CGNode &From, CGNode &To,
                                       bool NoSelfLoop) {
  assert(&From.Parent == &CG && &To.Parent == &CG);
  // update the value map
  addMergeNode(From.key, To.key);
  // update the graph
  CG.mergeNodeTo(From, To, NoSelfLoop);
}

void ConstraintsGenerator::mergeOnlySubtype() {
  // perform merging: if node A -> B, and there is no other edge from A to other
  // node, then merge A to B
  // for each node and outgoing edge, check if is one edge
  auto findMergePair = [&]() -> std::pair<CGNode *, CGNode *> {
    for (auto &Ent : CG.Nodes) {
      auto &Node = Ent.second;
      if (Node.isSpecial()) {
        continue;
      }
      if (Node.key.Base.isPrimitive()) {
        continue;
      }

      for (auto &Edge : Node.outEdges) {
        if (!std::holds_alternative<retypd::One>(Edge.getLabel())) {
          continue;
        }
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        if (&Target == CG.getStartNode() || &Target == CG.getEndNode()) {
          continue;
        }
        if (Target.key.Base.isPrimitive()) {
          continue;
        }
        if (Node.outEdges.size() == 1) {
          assert(Node.getPNIVar() == Target.getPNIVar());
          return {&Node, &Target};
        }
      }
    }
    return {nullptr, nullptr};
  };
  CGNode *A, *B;
  std::tie(A, B) = findMergePair();
  while (A != nullptr && B != nullptr) {
    // merge A to B
    LLVM_DEBUG(llvm::dbgs() << "Merge: " << toString(A->key) << " to "
                            << toString(B->key) << "\n");
    mergeNodeTo(*A, *B, true);
    A = nullptr;
    B = nullptr;
    std::tie(A, B) = findMergePair();
  }
}

void ConstraintsGenerator::determinize() {
  // assert(DTrans.empty());
  DTrans.clear();
  std::map<const CGNode *, CGNode *> This2Bak;
  ConstraintGraph Backup = CG.clone(This2Bak);
  Backup.changeStoreToLoad();
  // remove all edges in the graph
  for (auto &Ent : CG) {
    for (auto &Edge : Ent.second.outEdges) {
      CG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
    }
  }
  // remove all node that is not in the value map
  std::set<CGNode *> V2NNodes;
  for (auto &Ent : V2N) {
    if (auto *N = getNodeOrNull(Ent.second)) {
      V2NNodes.insert(N);
    }
  }
  for (auto &Ent : V2NContra) {
    if (auto *N = getNodeOrNull(Ent.second)) {
      V2NNodes.insert(N);
    }
  }
  for (auto &Ent : CG.Nodes) {
    if (Ent.second.isStartOrEnd()) {
      continue;
    }
    if (!Ent.second.isSpecial() && V2NNodes.count(&Ent.second) == 0) {
      CG.removeNode(Ent.second.key);
    }
  }

  using EntryTy = typename std::map<std::set<CGNode *>, CGNode *>::iterator;

  auto getOrSetNewNode = [this](const std::set<CGNode *> &N) -> EntryTy {
    if (DTrans.count(N)) {
      return DTrans.find(N);
    }
    auto *OldPN = notdec::retypd::NFADeterminizer<>::ensureSamePNI(N);
    auto &NewNode =
        CG.createNodeClonePNI(retypd::NodeKey{TypeVariable::CreateDtv(
                                  *Ctx.TRCtx, ValueNamer::getName("dtm_"))},
                              OldPN);
    auto it = DTrans.emplace(N, &NewNode);
    assert(it.second);
    return it.first;
  };

  DTrans[{Backup.getEndNode()}] = CG.getEndNode();
  std::queue<EntryTy> Worklist;

  // for each node in the value map
  for (auto *Node : V2NNodes) {
    auto *BakNode = This2Bak.at(Node);
    std::set<CGNode *> StartSet = countClosureFix({BakNode});
    auto pair1 = DTrans.emplace(StartSet, Node);
    if (pair1.second) {
      Worklist.push(pair1.first);
    } else {
      // Can be a epsilon loop. should be removed earlier
      assert(false);
    }
  }

  while (!Worklist.empty()) {
    auto It = Worklist.front();
    auto &Node = *It->second;
    std::set<retypd::EdgeLabel> outLabels =
        retypd::NFADeterminizer<>::allOutLabels(It->first);
    for (auto &L : outLabels) {
      auto S = countClosureFix(retypd::NFADeterminizer<>::move(It->first, L));
      if (S.count(Backup.getEndNode())) {
        if (S.size() > 1) {
          for (auto *Node : S) {
            llvm::errs() << "Node: " << toString(Node->key) << "\n";
          }
        }
        assert(S.size() == 1);
      }
      if (DTrans.count(S) == 0) {
        auto NewNodeEnt = getOrSetNewNode(S);
        Worklist.push(NewNodeEnt);
      }
      auto &ToNode = *DTrans.at(S);
      CG.onlyAddEdge(Node, ToNode, L);
    }
    Worklist.pop();
  }
  mergeAfterDeterminize();
}

void ConstraintsGenerator::mergeAfterDeterminize() {
  auto SameOutEdges = [&](const CGNode &N1, const CGNode &N2) -> bool {
    assert(&N1 != &N2);
    if (N1.getPNIVar()->getLatticeTy() != N2.getPNIVar()->getLatticeTy()) {
      return false;
    }
    if (N1.outEdges.size() != N2.outEdges.size()) {
      return false;
    }
    for (auto &Edge1 : N1.outEdges) {
      retypd::CGEdge ToFind(const_cast<CGNode &>(N2),
                            const_cast<CGNode &>(Edge1.getTargetNode()),
                            const_cast<retypd::EdgeLabel &>(Edge1.getLabel()));
      if (!N2.outEdges.count(ToFind)) {
        return false;
      }
    }
    for (auto &Edge2 : N2.outEdges) {
      retypd::CGEdge ToFind(const_cast<CGNode &>(N1),
                            const_cast<CGNode &>(Edge2.getTargetNode()),
                            const_cast<retypd::EdgeLabel &>(Edge2.getLabel()));
      if (!N1.outEdges.count(ToFind)) {
        return false;
      }
    }
    return true;
  };
  // if two nodes with same PNI, and same out edges, then merge them.
  auto findMergePair = [&]() -> std::pair<CGNode *, CGNode *> {
    for (auto &Ent : CG.Nodes) {
      auto &Node = Ent.second;
      if (Node.isStartOrEnd()) {
        continue;
      }

      // check for two edges with same label
      for (auto *Edge1 : Node.inEdges) {
        for (auto *Edge2 : Node.inEdges) {
          auto &N1 = Edge1->getSourceNode();
          auto &N2 = Edge2->getSourceNode();
          if (N1.isMemory() || N2.isMemory()) {
            continue;
          }
          if (&N1 == &N2) {
            continue;
          }
          if (Edge1->getLabel() != Edge2->getLabel()) {
            continue;
          }

          if (SameOutEdges(N1, N2)) {
            return {&Edge1->getSourceNode(), &Edge2->getSourceNode()};
          }
        }
      }
    }
    return {nullptr, nullptr};
  };

  CGNode *A, *B;
  std::tie(A, B) = findMergePair();
  while (A != nullptr && B != nullptr) {
    // merge A to B
    mergeNodeTo(*A, *B, true);
    A = nullptr;
    B = nullptr;
    std::tie(A, B) = findMergePair();
  }
}

void ConstraintsGenerator::run() {
  for (llvm::Function *Func : SCCs) {
    // create function nodes
    getOrInsertNode(Func, nullptr, -1, retypd::Covariant);
    // assert convariant node is created
    getNode(Func, nullptr, -1, retypd::Contravariant);
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

void ConstraintsGenerator::instantiateSummary(
    llvm::CallBase *Inst, llvm::Function *Target,
    const ConstraintsGenerator &Summary) {
  checkSymmetry();
  auto [FI, FIC] = CallToInstance.at(Inst);
  auto InstanceId = FI->key.Base.getInstanceId();
  assert(InstanceId != 0);

  // copy the whole graph into it. and add subtype relation
  std::map<const CGNode *, CGNode *> Old2New;
  ConstraintGraph::clone(
      Old2New, Summary.CG, CG,
      [&](const retypd::NodeKey &N) {
        if (N.Base.isPrimitive()) {
          return N;
        }
        retypd::NodeKey Ret = N;
        // 这里即使有了InstanceId，在函数节点和参数节点还是会重复，所以再设置actual标志。
        auto Base = N.Base.markActual();
        Base.instanceId = InstanceId;
        Ret.Base = Base;
        return Ret;
      },
      true);
  // find two function nodes.
  auto OF = Summary.getNodeOrNull(Target, nullptr, -1, retypd::Covariant);
  auto OFC = Summary.getNodeOrNull(Target, nullptr, -1, retypd::Contravariant);
  auto *F = OF != nullptr ? Old2New.at(OF) : nullptr;
  auto *FC = OFC != nullptr ? Old2New.at(OFC) : nullptr;
  assert(F != nullptr || FC != nullptr);
  // should create contra edge
  if (F != nullptr) {
    addSubtype(*F, *FI);
  }
  if (FC != nullptr) {
    addSubtype(*FIC, *FC);
  }
  makeSymmetry();
}

void ConstraintsGenerator::fixSCCFuncMappings() {
  for (auto F : SCCs) {
    // create node key
    auto Dtv = TypeVariable::CreateDtv(*Ctx.TRCtx, F->getName().str());
    retypd::NodeKey K(Dtv, retypd::Covariant);
    retypd::NodeKey KC(Dtv, retypd::Contravariant);
    // find the node
    auto *Node = CG.getNodeOrNull(K);
    // assert(Node != nullptr);
    auto *NodeC = CG.getNodeOrNull(KC);
    // assert(NodeC != nullptr);
    if (Node == nullptr && NodeC == nullptr) {
      llvm::errs() << "Warning: No useful summary for " << F->getName() << "\n";
      continue;
    }
    // insert to value map
    if (Node != nullptr) {
      V2N.insert(F, Node->key);
      // Fix return mapping
      if (!F->getFunctionType()->getReturnType()->isVoidTy()) {
        auto RetKey = Node->key;
        RetKey.Base = RetKey.Base.pushLabel(retypd::OutLabel{});
        auto *RetNode = getNodeOrNull(RetKey);
        if (RetNode) {
          V2N.insert(ReturnValue{.Func = F}, RetNode->key);
        }
      }
      // Fix argument mapping
      for (int i = 0; i < F->arg_size(); i++) {
        auto Arg = F->getArg(i);
        auto ArgKey = Node->key;
        ArgKey.Base = ArgKey.Base.pushLabel(
            retypd::InLabel{std::to_string(Arg->getArgNo())});
        auto *ArgNode = getNodeOrNull(ArgKey);
        if (ArgNode) {
          V2N.insert(Arg, ArgNode->key);
        }
      }
    }
    if (NodeC != nullptr) {
      V2NContra.insert(F, NodeC->key);
      // Fix return mapping
      if (!F->getFunctionType()->getReturnType()->isVoidTy()) {
        auto RetKey = NodeC->key;
        RetKey.Base = RetKey.Base.pushLabel(retypd::OutLabel{});
        auto *RetNodeC = getNodeOrNull(RetKey);
        if (RetNodeC) {
          V2NContra.insert(ReturnValue{.Func = F}, RetNodeC->key);
        }
      }
      // Fix argument mapping
      for (int i = 0; i < F->arg_size(); i++) {
        auto Arg = F->getArg(i);
        auto ArgKey = NodeC->key;
        ArgKey.Base = ArgKey.Base.pushLabel(
            retypd::InLabel{std::to_string(Arg->getArgNo())});
        auto *ArgNodeC = getNodeOrNull(ArgKey);
        if (ArgNodeC) {
          V2NContra.insert(Arg, ArgNodeC->key);
        }
      }
    }
  }
}

std::shared_ptr<ConstraintsGenerator> ConstraintsGenerator::genSummary() {
  std::map<const CGNode *, CGNode *> Old2New;
  auto S = CG.clone(Old2New);
  std::set<std::string> InterestingVars;
  for (auto *F : SCCs) {
    assert(F->hasName());
    InterestingVars.insert(F->getName().str());
  }
  S.linkVars(InterestingVars);
  // if (S.getStartNode()->outEdges.size() == 0) {
  //   llvm::errs() << "Warning: No func nodes, No meaningful types for "
  //                << CG.getName() << "\n";
  //   return nullptr;
  // }
  auto G2 = S.simplify();

  // Wrap the graph as a ConstraintsGenerator, Fix mappings
  std::shared_ptr<ConstraintsGenerator> Ret =
      std::make_shared<ConstraintsGenerator>(Ctx, CG.Name);
  Ret->SCCs = SCCs;
  std::map<const CGNode *, CGNode *> Tmp;
  ConstraintGraph::clone(Tmp, G2, Ret->CG);

  // summary 的关键是要维护两个variance的函数节点的映射。
  Ret->fixSCCFuncMappings();
  if (Ret->V2N.size() == 0 && Ret->V2NContra.size() == 0) {
    return nullptr;
  }
  return Ret;
}

retypd::CGNode *ConstraintsGenerator::getNodeOrNull(ExtValuePtr Val, User *User,
                                                    long OpInd,
                                                    retypd::Variance V) {
  llvmValue2ExtVal(Val, User, OpInd);

  if (V == retypd::Covariant) {
    if (V2N.count(Val)) {
      return getNodeOrNull(V2N.at(Val));
    }
    return nullptr;
  } else {
    if (V2NContra.count(Val)) {
      return getNodeOrNull(V2NContra.at(Val));
    }
    return nullptr;
  }
}

retypd::CGNode &ConstraintsGenerator::getNode(ExtValuePtr Val, User *User,
                                              long OpInd, retypd::Variance V) {
  llvmValue2ExtVal(Val, User, OpInd);
  if (V == retypd::Covariant) {
    return getNode(V2N.at(Val));
  } else {
    return getNode(V2NContra.at(Val));
  }
}

std::pair<retypd::CGNode &, retypd::CGNode &>
ConstraintsGenerator::createNode(ExtValuePtr Val, User *User, long OpInd) {
  llvmValue2ExtVal(Val, User, OpInd);
  auto Dtv = convertTypeVar(Val, User, OpInd);
  retypd::NodeKey K(Dtv, retypd::Covariant);
  auto &N = CG.createNode(K, getType(Val));
  auto It = V2N.insert(Val, N.key);
  if (!It.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "setTypeVar: Value already mapped to "
                 << toString(It.first->second) << ", but now set to "
                 << toString(Dtv) << "\n";
    std::abort();
  }
  K.SuffixVariance = retypd::Contravariant;
  auto &NContra = CG.createNode(K, getType(Val));
  auto It2 = V2NContra.insert(Val, NContra.key);
  if (!It2.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "setTypeVar: Value already mapped to "
                 << toString(It2.first->second) << ", but now set to "
                 << toString(Dtv) << "\n";
    std::abort();
  }
  return {N, NContra};
}

retypd::CGNode &ConstraintsGenerator::getOrInsertNode(ExtValuePtr Val,
                                                      User *User, long OpInd,
                                                      retypd::Variance V) {
  llvmValue2ExtVal(Val, User, OpInd);
  auto Node = getNodeOrNull(Val, User, OpInd, V);
  if (Node != nullptr) {
    return *Node;
  }
  auto [N, NC] = createNode(Val, User, OpInd);
  if (V == retypd::Covariant) {
    return N;
  } else {
    return NC;
  }
}

const TypeVariable &ConstraintsGenerator::getTypeVar(ExtValuePtr Val,
                                                     User *User, long OpInd) {
  return getOrInsertNode(Val, User, OpInd, retypd::Covariant).key.Base;
}

TypeVariable ConstraintsGenerator::convertTypeVar(ExtValuePtr Val, User *User,
                                                  long OpInd) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertTypeVarVal(*V, User, OpInd);
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    auto tv = getTypeVar(F->Func, nullptr, -1);
    return tv.pushLabel(retypd::OutLabel{});
  } else if (auto Arg = std::get_if<CallArg>(&Val)) { // TODO: deprecated
    assert(false);
  } else if (auto Ret = std::get_if<CallRet>(&Val)) { // TODO: deprecated
    assert(false);
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
    if (auto CI = dyn_cast<ConstantInt>(IC->Val)) {
      auto ret = TypeVariable::CreateIntConstant(
          *Ctx.TRCtx, OffsetRange{.offset = CI->getSExtValue()}, User, OpInd);
      return ret;
    }
    return convertTypeVarVal(IC->Val, IC->User, IC->OpInd);
  } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
    auto tv = CG.getMemoryNode(retypd::Covariant)->key.Base;
    tv = addOffset(tv, OffsetRange{.offset = CA->Val->getSExtValue()});
    return tv;
  }
  llvm::errs()
      << __FILE__ << ":" << __LINE__ << ": "
      << "ERROR: RetypdGenerator::convertTypeVar unhandled type of ExtValPtr\n";
  std::abort();
}

TypeVariable ConstraintsGenerator::convertTypeVarVal(Value *Val, User *User,
                                                     long OpInd) {
  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast ConstantExpr
      if (CE->getOpcode() == Instruction::BitCast) {
        return getTypeVar(CE->getOperand(0), CE, 0);
      } else if (CE->getOpcode() == Instruction::IntToPtr) {
        if (auto Addr = dyn_cast<ConstantInt>(CE->getOperand(0))) {
          assert(false && "Should be converted earlier");
        }
      } else if (CE->getOpcode() == Instruction::GetElementPtr) {
        // getelementptr of table, i.e., function pointer array
        if (auto GV = dyn_cast<GlobalVariable>(CE->getOperand(0))) {
          // if is function pointer array
          auto T1 = GV->getType();
          if (T1->isPointerTy() && T1->getPointerElementType()->isArrayTy() &&
              T1->getPointerElementType()
                  ->getArrayElementType()
                  ->isPointerTy() &&
              T1->getPointerElementType()
                      ->getArrayElementType()
                      ->getPointerElementType() != nullptr &&
              T1->getPointerElementType()
                  ->getArrayElementType()
                  ->getPointerElementType()
                  ->isFunctionTy()) {
            // if constant offset
            if (auto CI1 = dyn_cast<ConstantInt>(CE->getOperand(1))) {
              if (CI1->isZero()) {
                if (auto CI = dyn_cast<ConstantInt>(CE->getOperand(2))) {
                  auto tv = getTypeVar(GV, nullptr, -1);
                  tv = tv.pushLabel(retypd::OffsetLabel{
                      .range = OffsetRange{.offset = CI->getSExtValue() *
                                                     Ctx.pointer_size}});
                  return tv;
                }
              }
            }
          }
        }
      } else {
        llvm::errs()
            << __FILE__ << ":" << __LINE__ << ": "
            << "ERROR: RetypdGenerator::getTypeVar unhandled ConstantExpr: "
            << *C << "\n";
      }
    } else if (auto gv = dyn_cast<GlobalValue>(C)) { // global variable
      if (gv == Ctx.StackPointer) {
        std::cerr
            << "Error: convertTypeVarVal: direct use of stack pointer?, ensure "
               "StackAllocationRecovery is run before, Or add external summary "
               "for this function.\n";
        return makeTv(Ctx.TRCtx, ValueNamer::getName());
      } else if (auto Func = dyn_cast<Function>(C)) {
        return makeTv(Ctx.TRCtx, getFuncTvName(Func));
      }
      return makeTv(Ctx.TRCtx, gv->getName().str());
    } else if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      if (auto CI = dyn_cast<ConstantInt>(C)) {
        assert(false && "Should be converted earlier");
      }
      return makeTv(Ctx.TRCtx, ValueNamer::getName("constant_"));
      // auto Ty = C->getType();
      // return getLLVMTypeVar(Ctx.TRCtx, Ty);
    } else if (isa<ConstantPointerNull>(C)) {
      return makeTv(Ctx.TRCtx, ValueNamer::getName("null_"));
    }
    llvm::errs()
        << __FILE__ << ":" << __LINE__ << ": "
        << "ERROR: RetypdGenerator::getTypeVar unhandled type of constant: "
        << *C << "\n";
    std::abort();
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    // Consistent with Call handling
    auto &N = getOrInsertNode(arg->getParent(), nullptr, -1);
    auto tv = N.key.Base;
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
  auto &SrcVar = cg.getOrInsertNode(Src, &I, 0, retypd::Covariant);
  auto &DstVar =
      cg.createNodeCovariant(ReturnValue{.Func = I.getFunction()}, &I, 0);
  // src is a subtype of dest
  cg.addSubtype(SrcVar, DstVar);
}

// std::shared_ptr<retypd::Sketch>
// ConstraintsGenerator::solveType(const TypeVariable &TV) {
//   // Because we always do layer split after clone, so we can refer to Node by
//   // type variable.
//   if (CG.Nodes.count(TV) == 0) {
//     return nullptr;
//   }
//   auto &CGNode = CG.Nodes.at(TV);
//   return CG.solveSketch(CGNode);
// }

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: RetypdGenerator: indirect call not supported yet\n";
    return;
  }
  auto &TargetNode = cg.getOrInsertNode(Target, nullptr, -1);

  if (cg.SCCs.count(Target)) {
    // Call within the SCC:
    // directly link to the function tv.
    for (int i = 0; i < I.arg_size(); i++) {
      auto &ArgVar = cg.getOrInsertNode(Target->getArg(i), &I, i);
      auto &ValVar = cg.getOrInsertNode(I.getArgOperand(i), &I, i);
      // argument is a subtype of param
      cg.addSubtype(ValVar, ArgVar);
    }
    if (!I.getType()->isVoidTy()) {
      // type var should be consistent with return instruction
      auto &FormalRetVar =
          cg.getOrInsertNode(ReturnValue{.Func = Target}, &I, -1);
      auto &ValVar = cg.getOrInsertNode(&I, nullptr, -1);
      // formal return -> actual return
      cg.addSubtype(FormalRetVar, ValVar);
    }
  } else {
    // differentiate different call instances in the same function
    size_t InstanceId = ValueNamer::getId();

    // Create target func instance node with low type.
    auto FuncVar = TargetNode.key.Base;
    FuncVar.instanceId = InstanceId;
    auto &FuncNode = cg.CG.getOrInsertNode(FuncVar, Target->getFunctionType());
    auto &FNC =
        cg.CG.getOrInsertNode(retypd::NodeKey(FuncVar, retypd::Contravariant),
                              Target->getFunctionType());
    cg.CallToInstance.emplace(&I, std::make_pair(&FuncNode, &FNC));
    for (int i = 0; i < I.arg_size(); i++) {
      auto ArgVar = getCallArgTV(FuncNode, i);
      auto &ValVar = cg.getOrInsertNode(I.getArgOperand(i), &I, i);
      auto &ArgNode =
          cg.CG.getOrInsertNode(ArgVar, I.getArgOperand(i)->getType());
      // argument is a subtype of param
      cg.addSubtype(ValVar, ArgNode);
    }
    if (!I.getType()->isVoidTy()) {
      // for return value
      auto FormalRetVar = getCallRetTV(FuncNode);
      auto &ValVar = cg.getOrInsertNode(&I, nullptr, -1);
      auto &RetNode = cg.CG.getOrInsertNode(FormalRetVar, I.getType());
      // formal return -> actual return
      cg.addSubtype(RetNode, ValVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitSelectInst(
    SelectInst &I) {
  auto &DstVar = cg.createNodeCovariant(&I, nullptr, -1);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto &Src1Var = cg.getOrInsertNode(Src1, &I, 0);
  auto &Src2Var = cg.getOrInsertNode(Src2, &I, 1);
  // Not generate boolean constraints. Because it must be i1.
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAllocaInst(
    AllocaInst &I) {
  auto &Node = cg.createNodeCovariant(&I, nullptr, -1);
  // set as pointer type
  cg.setPointer(Node);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  // Defer constraints generation (and unification) to handlePHINodes
  phiNodes.push_back(&I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto &DstVar = cg.getNode(I, nullptr, -1, retypd::Covariant);
    for (long i = 0; i < I->getNumIncomingValues(); i++) {
      auto *Src = I->getIncomingValue(i);
      auto &SrcVar = cg.getOrInsertNode(Src, I, i);
      // src is a subtype of dest
      cg.addSubtype(SrcVar, DstVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto &SrcVar = cg.getOrInsertNode(Src, &I, 0);
    cg.addVarSubtype(&I, SrcVar);
    return;
  } else if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *Src = I.getOperand(0);
    auto &SrcVar = cg.getOrInsertNode(Src, &I, 0);
    /* auto &Node = */ cg.addVarSubtype(&I, SrcVar);
    // cg.setPointer(Node);
    return;
  }
  // // Override implementation for TruncInst, ZExtInst here
  // else if (isa<TruncInst, ZExtInst>(&I)) {
  //   auto *Src = I.getOperand(0);
  //   // if (is32Or64Int(I.getSrcTy()) && is32Or64Int(I.getDestTy())) {
  //   //   auto &SrcVar = cg.getOrInsertNode(Src, &I);
  //   //   cg.addVarSubtype(&I, SrcVar);
  //   // } else {
  //   if (isa<ZExtInst>(&I)) {
  //     assert(I.getType()->isIntegerTy());
  //     auto &Node = cg.getOrInsertNode(&I, nullptr);
  //     auto &UintNodeOut = cg.CG.getOrCreatePrim(
  //         "uint" + std::to_string(I.getType()->getIntegerBitWidth()),
  //         I.getType());
  //     cg.addSubtype(UintNodeOut, Node);
  //     // zext result is a number
  //     Node.getPNIVar()->setNonPtr();

  //     auto &NodeIn = cg.getOrInsertNode(I.getOperand(0), &I);
  //     auto &UintNodeIn = cg.CG.getOrCreatePrim(
  //         "uint" +
  //             std::to_string(I.getOperand(0)->getType()->getIntegerBitWidth()),
  //         I.getOperand(0)->getType());
  //     cg.addSubtype(NodeIn, UintNodeIn);
  //     NodeIn.getPNIVar()->setNonPtr();
  //   } else {
  //     // View as typecast: we have low type in the Node so no need to create
  //     // relations.
  //     auto &Result = cg.createNode(&I, nullptr);
  //   }
  //   // }
  //   return;
  // }
  else if (isa<TruncInst, ZExtInst, SExtInst, FPToUIInst, FPToSIInst,
               UIToFPInst, SIToFPInst, FPTruncInst, FPExtInst>(&I)) {
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
  // A shortcut to create a offseted pointer. the operate type must be i8*.
  // Just like ptradd.
}

void ConstraintsGenerator::addCmpConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            ICmpInst *I) {
  getOrInsertNode(LHS, I, 0).getPNIVar()->unify(
      *getOrInsertNode(RHS, I, 1).getPNIVar());
}

// for pointer sized int, probably is pointer comparision. So we cannot make a
// conclusion of subtype. but create a CmpConstraint.
void ConstraintsGenerator::RetypdGeneratorVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addCmpConstraint(Src1, Src2, &I);

  // type the inst as bool?
  assert(I.getType()->isIntegerTy(1));
  cg.createNode(&I, nullptr, -1);
}

// #region LoadStore

// =========== begin: load/store insts and deref analysis ===========

unsigned ConstraintsGenerator::getPointerElemSize(Type *ty) {
  Type *Elem = ty->getPointerElementType();
  if (Elem->isPointerTy()) {
    assert(Ctx.pointer_size != 0 &&
           "RetypdGenerator: pointer size not initialized");
    return Ctx.pointer_size;
  }
  unsigned Size = Elem->getPrimitiveSizeInBits();
  if (Size != 0) {
    return Size;
  }
  assert(false && "unknown pointer type");
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitStoreInst(
    StoreInst &I) {
  auto DstVar =
      cg.deref(I.getPointerOperand(), &I, 1,
               cg.getPointerElemSize(I.getPointerOperandType()), false);
  auto &StoreNode = cg.CG.getOrInsertNode(
      DstVar, I.getPointerOperandType()->getPointerElementType());
  // actual store -> formal store
  cg.addSubtype(cg.getOrInsertNode(I.getValueOperand(), &I, 0), StoreNode);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  auto LoadedVar =
      cg.deref(I.getPointerOperand(), &I, 0,
               cg.getPointerElemSize(I.getPointerOperandType()), true);
  auto &LoadNode = cg.CG.getOrInsertNode(LoadedVar, I.getType());
  if (TraceIds.count(LoadNode.getId())) {
    llvm::errs() << "TraceID=" << LoadNode.getId()
                 << " Node=" << toString(LoadNode.key)
                 << "RetypdGeneratorVisitor::visitLoadInst: Created node by "
                    "add .load to pointer operand: "
                 << I << "\n";
  }
  // formal load -> actual load
  auto &CN = cg.createNodeCovariant(&I, nullptr, -1);
  if (TraceIds.count(CN.getId())) {
    llvm::errs() << "TraceID=" << CN.getId() << " Node=" << toString(CN.key)
                 << "RetypdGeneratorVisitor::visitLoadInst: Created covariant "
                    "node for load instruction: "
                 << I << "\n";
  }
  cg.addSubtype(LoadNode, CN);
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
TypeVariable ConstraintsGenerator::deref(Value *Val, User *User, long OpInd,
                                         unsigned BitSize, bool isLoadOrStore) {
  auto &Node = getOrInsertNode(Val, User, OpInd);
  setPointer(Node);
  assert(BitSize != 0 && "RetypdGenerator::deref: zero size!?");
  // from the offset, generate a loaded type variable.
  auto DstVar = Node.key.Base;
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
        {Instruction::ZExt, {"uint", 1, (const char *[1]){"uint"}}},
        {Instruction::Trunc, {"int", 1, (const char *[1]){"int"}}},

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
  PG->addAddCons(&getOrInsertNode(LHS, I, 0), &getOrInsertNode(RHS, I, 1),
                 &getOrInsertNode(I, nullptr, -1), I);
}

void ConstraintsGenerator::addSubConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            BinaryOperator *I) {
  PG->addSubCons(&getOrInsertNode(LHS, I, 0), &getOrInsertNode(RHS, I, 1),
                 &getOrInsertNode(I, nullptr, -1), I);
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

  auto &Src1Node = cg.getOrInsertNode(Src1, &I, 0);
  auto &Src2Node = cg.getOrInsertNode(Src2, &I, 1);
  auto &RetNode = cg.getOrInsertNode(&I, nullptr, -1);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if ((CI->getZExtValue() & 0x3fffff00) == 0x3fffff00) {
      // act as simple assignment
      cg.addSubtype(RetNode, Src1Node);
      return;
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: And op without constant: " << I << "\n";
  }
  // view as numeric operation?
  Src1Node.getPNIVar()->setNonPtr();
  Src2Node.getPNIVar()->setNonPtr();
  RetNode.getPNIVar()->setNonPtr();
  return;
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitOr(BinaryOperator &I) {
  // llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  auto &Src1Node = cg.getOrInsertNode(Src1, &I, 0);
  auto &Src2Node = cg.getOrInsertNode(Src2, &I, 1);
  auto &RetNode = cg.getOrInsertNode(&I, nullptr, -1);

  if (auto CI = dyn_cast<ConstantInt>(Src2)) {
    // at least most of the bits are passed, View as pointer alignment.
    if ((CI->getZExtValue() & 0x3fffff00) == 0) {
      // act as simple assignment
      cg.addSubtype(RetNode, Src1Node);
      return;
    }
  } else {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "Warn: Or op without constant: " << I << "\n";
  }
  // view as numeric operation?
  Src1Node.getPNIVar()->setNonPtrIfRelated();
  Src2Node.getPNIVar()->setNonPtrIfRelated();
  RetNode.getPNIVar()->setNonPtrIfRelated();
  return;
}

bool strEq(const char *S1, const char *S2) { return strcmp(S1, S2) == 0; }

bool ConstraintsGenerator::PcodeOpType::addRetConstraint(
    Instruction *I, ConstraintsGenerator &cg) const {
  // only create Covariant constraints, use addSubtype to handle contra-variant.
  auto &N = cg.createNodeCovariant(I, nullptr, -1);
  if (I->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = output;
  if (ty == nullptr) { // no action
    return true;
  } else if (strEq(ty, "sint")) {
    N.getPNIVar()->setNonPtr();
    auto &SintNode = cg.CG.getOrCreatePrim("sint", I->getType());
    cg.addSubtype(SintNode, N);
    return true;
  } else if (strEq(ty, "uint")) {
    N.getPNIVar()->setNonPtr();
    auto &UintNode = cg.CG.getOrCreatePrim("uint", I->getType());
    cg.addSubtype(UintNode, N);
    return true;
  } else if (strEq(ty, "int")) {
    N.getPNIVar()->setNonPtr();
    return true;
  }

  return false;
}

bool ConstraintsGenerator::PcodeOpType::addOpConstraint(
    unsigned Index, Instruction *I, ConstraintsGenerator &cg) const {
  assert(size == I->getNumOperands() && "input size not match");
  auto Op = I->getOperand(Index);
  if (Op->getType()->isVoidTy()) {
    return false;
  }
  auto &N = cg.getOrInsertNode(Op, I, Index);
  const char *ty = inputs[Index];
  if (ty == nullptr) {
    return true;
  } else if (strEq(ty, "sint")) {
    N.getPNIVar()->setNonPtrIfRelated();
    auto &SintNode = cg.CG.getOrCreatePrim(
        "sint" + std::to_string(Op->getType()->getIntegerBitWidth()),
        Op->getType());
    cg.addSubtype(N, SintNode);
    return true;
  } else if (strEq(ty, "uint")) {
    N.getPNIVar()->setNonPtrIfRelated();
    auto &UintNode = cg.CG.getOrCreatePrim(
        "uint" + std::to_string(Op->getType()->getIntegerBitWidth()),
        Op->getType());
    cg.addSubtype(N, UintNode);
    return true;
  } else if (strEq(ty, "int")) {
    N.getPNIVar()->setNonPtrIfRelated();
    return true;
  }
  return false;
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
  std::map<llvm::Function *, std::shared_ptr<ConstraintsGenerator>> &FuncCtxs;

  void emitFunctionAnnot(const llvm::Function *F,
                         llvm::formatted_raw_ostream &OS) override {
    if (FuncCtxs.count(const_cast<llvm::Function *>(F)) == 0) {
      return;
    }
    auto &CG = FuncCtxs.at(const_cast<llvm::Function *>(F));
    OS << "; ";
    if (!F->getReturnType()->isVoidTy()) {
      OS << CG->getOrInsertNode(
                  ReturnValue{.Func = const_cast<llvm::Function *>(F)}, nullptr,
                  -1)
                .str();
      OS << " <- ";
    }
    OS << "(";
    for (auto &Arg : F->args()) {
      OS << CG->getOrInsertNode(const_cast<llvm::Argument *>(&Arg), nullptr, -1)
                .str()
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
    auto &CG = FuncCtxs.at(const_cast<llvm::Function *>(Instr->getFunction()));

    OS << "; ";
    if (!V.getType()->isVoidTy()) {
      OS << CG->getOrInsertNode(const_cast<llvm::Value *>(&V), nullptr, -1)
                .str();
      if (Instr != nullptr) {
        OS << " <- ";
      }
    }
    if (Instr != nullptr) {
      OS << "(";
      for (long i = 0; i < Instr->getNumOperands(); i++) {
        Value *Op = Instr->getOperand(i);
        auto *Node = CG->getNodeOrNull(
            Op, const_cast<llvm::Instruction *>(Instr), i, retypd::Covariant);
        auto *NodeC =
            CG->getNodeOrNull(Op, const_cast<llvm::Instruction *>(Instr), i,
                              retypd::Contravariant);
        OS << (Node == nullptr ? "null" : Node->str()) << "/"
           << (NodeC == nullptr ? "null" : NodeC->str()) << ", ";
      }
      OS << ")";
    }
    // OS << "\n";
  }

public:
  CGAnnotationWriter(std::map<llvm::Function *,
                              std::shared_ptr<ConstraintsGenerator>> &func_ctxs)
      : FuncCtxs(func_ctxs) {}
};

void TypeRecovery::print(llvm::Module &M, std::string path) {
  std::error_code EC;
  llvm::raw_fd_ostream os(path, EC);
  if (EC) {
    std::cerr << "Cannot open output file: " << path << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
}

void TypeRecovery::printAnnotatedModule(llvm::Module &M, std::string path) {
  std::error_code EC;
  llvm::raw_fd_ostream os(path, EC);
  if (EC) {
    std::cerr << "Cannot open output file: " << path << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
  CGAnnotationWriter AW((FuncCtxs));
  M.print(os, &AW);
}

void ConstraintsGenerator::analyzeFieldRange() {
  // iterate as if trying to convert to AST.
  // assert(FieldInfoCache.empty());
  // std::set<CGNode *> Visited;
}

StructInfo ConstraintsGenerator::getFieldInfo(const CGNode &Node) {
  // auto It = FieldInfoCache.find(&Node);
  // if (It != FieldInfoCache.end()) {
  //   return It->second;
  // }

  StructInfo Ret;
  for (auto &Edge : Node.outEdges) {
    auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
    assert(&Target != &Node && "Self loop should not exist");
    // for load/store edges, just return pointer size.
    if (retypd::isLoadOrStore(Edge.getLabel())) {
      Ret.addField(FieldEntry{
          .R = {.Start = OffsetRange(), .Size = (Node.Parent.PointerSize / 8)},
          .Edge = &const_cast<CGEdge &>(Edge)});
      continue;
    }
    // for subtype/offset edges, start offset should be the start(Or the edge
    // should be updated). recursive to calc size.
    llvm::Optional<OffsetRange> BaseOff;
    if (std::holds_alternative<retypd::One>(Edge.getLabel())) {
      BaseOff.emplace();
    } else if (auto *RL = std::get_if<retypd::RecallLabel>(&Edge.getLabel())) {
      if (auto *Off = std::get_if<OffsetLabel>(&RL->label)) {
        BaseOff = Off->range;
      }
    } else if (auto FL = std::get_if<retypd::ForgetLabel>(&Edge.getLabel())) {
      llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                   << "ERROR: ForgetLabel should not exist?: "
                   << toString(Edge.getLabel()) << "\n";
      std::abort();
    }
    assert(BaseOff.hasValue() && "Base offset not set");
    auto Fields = getFieldInfo(Target);
    auto MaxOff = Fields.getMaxOffset();
    Ret.addField(FieldEntry{.R = {.Start = *BaseOff, .Size = MaxOff},
                            .Edge = &const_cast<CGEdge &>(Edge)});
  }

  // FieldInfoCache[&Node] = Ret;
  return Ret;
}

std::shared_ptr<ConstraintsGenerator> ConstraintsGenerator::fromConstraints(
    TypeRecovery &Ctx, std::set<llvm::Function *> SCCs,
    const retypd::ConstraintSummary &Summary) {
  if (Summary.Cons.empty()) {
    return nullptr;
  }
  std::string SCCNames = getName(SCCs);
  auto Ret = std::make_shared<ConstraintsGenerator>(Ctx, SCCNames, SCCs);
  Ret->CG.instantiateConstraints(Summary);
  Ret->fixSCCFuncMappings();
  // std::map<CGNode*,  retypd::Variance> Initial;
  // for (auto &Ent : Ret->V2N) {
  //   Initial[&Ret->getNode(Ent.second)] = retypd::Covariant;
  // }
  // for (auto &Ent : Ret->V2NContra) {
  //   Initial[&Ret->getNode(Ent.second)] = retypd::Contravariant;
  // }
  // Ret->CG.markVariance(&Initial);
  if (Ret->V2N.size() == 0 && Ret->V2NContra.size() == 0) {
    assert(false && "Empty constraints?");
  }
  return Ret;
}

std::shared_ptr<ConstraintsGenerator> ConstraintsGenerator::fromDotSummary(
    TypeRecovery &Ctx, std::set<llvm::Function *> SCCs, retypd::DotGraph &G) {
  auto Ret = std::make_shared<ConstraintsGenerator>(Ctx, G.name, SCCs);
  // Ret->CG.fromDotSummary(G);
  assert(false && "TODO!");
  Ret->fixSCCFuncMappings();
  if (Ret->V2N.size() == 0 && Ret->V2NContra.size() == 0) {
    return nullptr;
  }
  return Ret;
}

} // namespace notdec
