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
#include <map>
#include <memory>
#include <optional>
#include <queue>
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
#include "Utils/Range.h"
#include "Utils/ValueNamer.h"
#include "notdec-llvm2c/Interface.h"
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

// When encountered a primitive node, ignore its edges.
std::set<CGNode *> countClosureFix(const std::set<CGNode *> &N) {
  std::set<CGNode *> Ret;
  std::queue<CGNode *> Worklist;
  for (auto Node : N) {
    Ret.insert(Node);
    Worklist.push(Node);
  }
  while (!Worklist.empty()) {
    auto Node = Worklist.front();
    Worklist.pop();
    if (Node->key.Base.isPrimitive()) {
      continue;
    }
    for (auto &Edge : Node->outEdges) {
      if (std::holds_alternative<retypd::One>(Edge.getLabel())) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        if (Ret.count(&Target) == 0) {
          Ret.insert(&Target);
          Worklist.push(&Target);
        }
      }
    }
  }
  return Ret;
}

void dump(const ExtValuePtr &Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    llvm::errs() << "Value: " << **V << "\n";
    return;
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    llvm::errs() << "ReturnValue: " + ValueNamer::getName(*F->Func, "func_");
    return;
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    llvm::errs() << ValueNamer::getName(
                        *const_cast<llvm::CallBase *>(Arg->Call)) +
                        "_CallArg_" + std::to_string(Arg->Index);
    return;
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    llvm::errs() << ValueNamer::getName(
                        *const_cast<llvm::CallBase *>(Ret->Call)) +
                        "_CallRet";
    return;
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    llvm::errs() << "IntConstant: " << *IC->Val << ", User: " << *IC->User;
    return;
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getName: unhandled type of ExtValPtr\n";
  std::abort();
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

static inline TypeVariable makeTv(retypd::TRContext &Ctx, std::string Name) {
  return retypd::TypeVariable::CreateDtv(Ctx, Name);
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
    return (*V)->getType();
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    return F->Func->getReturnType();
  } else if (auto Arg = std::get_if<CallArg>(&Val)) {
    return Arg->Call->getArgOperand(Arg->Index)->getType();
  } else if (auto Ret = std::get_if<CallRet>(&Val)) {
    return Ret->Call->getType();
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    return IC->Val->getType();
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getType: unhandled type of ExtValPtr\n";
  std::abort();
}

unsigned int getSize(const ExtValuePtr &Val, unsigned int pointer_size) {
  assert(pointer_size != 0);
  auto Ty = getType(Val);
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

// TODO refactor: increase code reusability
ExtValuePtr getExtValuePtr(llvm::Value *V, User *User) {
  ExtValuePtr Val = V;
  if (!isa<GlobalValue>(*V)) {
    if (auto CI = dyn_cast<Constant>(V)) {
      assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
      assert(hasUser(V, User) &&
             "convertTypeVarVal: constant not used by user");
      Val = UConstant{.Val = cast<Constant>(V), .User = User};
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
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: getName: unhandled type of ExtValPtr\n";
  std::abort();
}

inline void dumpTypes(llvm::Value *V, clang::QualType CTy) {
  llvm::errs() << "  Value: " << *V << " has type: " << CTy.getAsString()
               << "\n";
}

std::string join(std::string path, std::string elem) {
  return path.back() == '/' ? path + elem : path + "/" + elem;
}

auto isGV = [](const ExtValuePtr &N) {
  if (auto V = std::get_if<llvm::Value *>(&N)) {
    if (llvm::isa<llvm::GlobalValue>(*V)) {
      return true;
    }
  }
  return false;
};

TypeRecovery::Result TypeRecovery::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== RetypdGenerator  ===============\n");

  const char *DebugDir = std::getenv("NOTDEC_TYPE_RECOVERY_DEBUG_DIR");
  llvm::Optional<llvm::raw_fd_ostream> SCCsCatalog;
  if (DebugDir) {
    llvm::sys::fs::create_directories(DebugDir);
    std::error_code EC;
    SCCsCatalog.emplace(join(DebugDir, "SCCs.txt"), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output json file." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
  }

  if (DebugDir) {
    printModule(M, join(DebugDir, "00-before-TypeRecovery.ll").c_str());
  }

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
  std::map<CallGraphNode *, std::size_t> Func2SCCIndex;
  // map from function to its actual argument and return type nodes in the
  // global graph.
  std::map<CallGraphNode *,
           std::vector<std::pair<llvm::CallBase *, CallGraphNode *>>>
      FuncCalls;

  std::map<std::string, std::vector<retypd::SubTypeConstraint>>
      SummaryOverride = {
          {std::string("memset"),
           retypd::parse_subtype_constraints(
               TRCtx, {"memset.in_0 <= memset.out", "memset.in_1 <= #uint"})},
          {std::string("memcpy"),
           retypd::parse_subtype_constraints(
               TRCtx, {"memcpy.in_1 <= memcpy.in_0",
                       "memcpy.in_0 <= memcpy.out", "memcpy.in_2 <= #uint"})},
          {std::string("__memcpy"),
           retypd::parse_subtype_constraints(TRCtx,
                                             {"__memcpy.in_1 <= __memcpy.in_0",
                                              "__memcpy.in_0 <= __memcpy.out",
                                              "__memcpy.in_2 <= #uint"})},
          {std::string("memchr"), {}},
          {std::string("pop_arg"), {}},
          {std::string("fmt_fp"), {}},
      };

  bool DisableOverride = false;
  if (DisableOverride) {
    SummaryOverride = {};
  }

  // Walk the callgraph in bottom-up SCC order.
  for (; !CGI.isAtEnd(); ++CGI) {
    const std::vector<CallGraphNode *> &NodeVec = *CGI;
    AllSCCs.emplace_back(std::make_tuple(NodeVec, nullptr, ""));

    // Calc name for current SCC
    std::string Name;
    std::set<llvm::Function *> SCCs;
    for (auto CGN : NodeVec) {
      Func2SCCIndex[CGN] = AllSCCs.size() - 1;
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
            *this, M.getContext(), Name, SCCs,
            [&](auto *F) { return &FuncSummaries.at(F); });

    Generator->run();

    // TODO: If the SCC is not called by any other function out of the SCC, we
    // can skip summary generation. Even start Top-down phase.

    if (const char *path = std::getenv("DEBUG_TRANS_INIT_GRAPH")) {
      if ((std::strcmp(path, "1") == 0) || (std::strstr(path, Name.c_str()))) {
        Generator->CG.printGraph("trans_init.dot");
      }
    }

    std::vector<retypd::SubTypeConstraint> Summary;
    if (SummaryOverride.count(Name)) {
      std::cerr << "Summary Overriden: " << Name << ":\n";
      Summary = SummaryOverride.at(Name);
      Generator->CG.solve();
    } else {
      std::cerr << "Summary for " << Name << ":\n";
      Summary = Generator->genSummary();
    }

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
    // write the SCC index to file
    if (SCCsCatalog) {
      *SCCsCatalog << "SCC" << AllSCCs.size() - 1 << "," << Name << "\n";
    }
  }
  if (DebugDir) {
    SCCsCatalog->close();
    printModule(M, join(DebugDir, "01-BottomUp.ll").c_str());
  }

  std::cerr << "Bottom up phase done! SCC count:" << AllSCCs.size() << "\n";

  if (DebugDir) {
    printAnnotatedModule(M, join(DebugDir, "01-BottomUp.anno.ll").c_str());
  }

  // Top-down Phase: build the result(Map from value to clang C type)
  // We have a big global type graph, corresponds to C AST that link the
  // declared struct type to the real definition to form a graph.

  // Steps:
  // 1. sketchSplit
  // 2. extended powerset construction to global type graph.
  // 3. convert to map.
  // TODO: how to handle function type

  // Global type graph
  // std::unique_ptr<ConstraintGraph> GlobalTypes =
  //     std::make_unique<ConstraintGraph>(TRCtx, "GlobalTypes", true);
  // Global Value Map
  // std::unique_ptr<std::map<ExtValuePtr, retypd::CGNode *>> Val2NodeGlobal;
  // ConstraintsGenerator GlobalTypes(*this, "GlobalTypes");
  std::deque<ConstraintsGenerator> SCCGraphs;

  TypeRecovery::Result Result;
  retypd::SketchToCTypeBuilder TB(M.getName());

  for (std::size_t Index1 = AllSCCs.size(); Index1 > 0; --Index1) {
    std::size_t SCCIndex = Index1 - 1;
    auto &It = AllSCCs[SCCIndex];
    std::string SCCName = "SCC" + std::to_string(SCCIndex);
    SCCGraphs.emplace_front(*this, M.getContext(), SCCName);
    auto &CurrentTypes = SCCGraphs.front();
    assert(CurrentTypes.PG);

    llvm::Optional<std::string> DirPath;
    if (DebugDir) {
      DirPath.emplace(join(DebugDir, SCCName));
      llvm::sys::fs::create_directories(*DirPath);
    }

    const std::vector<CallGraphNode *> &NodeVec = std::get<0>(It);
    if (std::get<1>(It) == nullptr) {
      continue;
    }
    auto &Generator = std::get<1>(It);
    auto &Name = std::get<2>(It);

    std::cerr << "Processing SCC: " << Name << "\n";

    // Collect all functions for SCC checking
    std::set<llvm::Function *> SCCSet;
    for (auto CGN : NodeVec) {
      if (CGN->getFunction() == nullptr) {
        continue;
      }
      SCCSet.insert(CGN->getFunction());
    }

    if (DirPath) {
      Generator->CG.printGraph(join(*DirPath, "00-Original.dot").c_str());
    }

    // check if saturated
    // 0. pre-process the big graph: focus on the push/recall subgraph.
    Generator->CG.sketchSplit();
    Generator->removeUnreachable();

    if (DirPath) {
      Generator->CG.printGraph(join(*DirPath, "01-Original.sks.dot").c_str());
      // CurrentTypes.CG.printGraph(
      //     join(*DirPath, "01-Global.before.dot").c_str());
    }

    std::map<const CGNode *, CGNode *> Old2New;
    Generator->cloneTo(CurrentTypes, Old2New);

    if (DirPath) {
      CurrentTypes.CG.printGraph(join(*DirPath, "02-Global.added.dot").c_str());
    }

    // 2. link the argument/return value subtype relations.
    // TODO handle arg aliasing
    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }
      if (FuncCalls.count(CGN) == 0) {
        continue;
      }
      auto &CurrentCalls = FuncCalls.at(CGN);
      if (CurrentCalls.empty()) {
        std::cerr << "No actual param types for " << Current->getName().str()
                  << "\n";
      }
      if (Current->isVarArg()) {
        std::cerr << "TODO: Support vararg function: "
                  << Current->getName().str() << "\n";
        std::abort();
      }

      std::vector<std::set<CGNode *>> ArgNodes(Current->arg_size());
      std::vector<std::set<CGNode *>> ArgNodesContra(Current->arg_size());
      std::set<CGNode *> RetNodes;
      std::set<CGNode *> RetNodesContra;

      for (auto &Elem : CurrentCalls) {
        auto *Call = Elem.first;
        auto &CallerGenerator =
            std::get<1>(AllSCCs.at(Func2SCCIndex.at(Elem.second)));

        for (int i = 0; i < Call->arg_size(); i++) {
          auto &NFrom =
              CallerGenerator->getOrInsertNode(Call->getArgOperand(i), Call);
          ArgNodes[i].insert(&NFrom);
          ArgNodesContra[i].insert(
              &CallerGenerator->CG.getNode(MakeContraVariant(NFrom.key)));
        }
        if (!Current->getReturnType()->isVoidTy()) {
          auto &NFrom = CallerGenerator->getOrInsertNode(Call, nullptr);
          RetNodes.insert(&NFrom);
          RetNodesContra.insert(
              &CallerGenerator->CG.getNode(MakeContraVariant(NFrom.key)));
        }
      }

      // merge nodes to current graph by determinize.
      auto determinizeTo = [&](std::set<CGNode *> &StartNodes,
                               const char *NamePrefix) -> CGNode * {
        auto &CG = CurrentTypes.CG;
        std::map<std::set<CGNode *>, CGNode *> DTrans;
        using EntryTy =
            typename std::map<std::set<CGNode *>, CGNode *>::iterator;

        auto getOrSetNewNode = [&](const std::set<CGNode *> &N) -> EntryTy {
          if (DTrans.count(N)) {
            return DTrans.find(N);
          }
          bool hasEnd = false;
          bool allEnd = true;
          for (auto *Node : N) {
            if (Node->key.Base.isEnd()) {
              hasEnd = true;
            } else {
              allEnd = false;
            }
          }
          assert(hasEnd == allEnd);
          CGNode *NewNode;
          if (hasEnd) {
            NewNode = CG.getEndNode();
          } else {
            auto *PN = notdec::retypd::NFADeterminizer<>::ensureSamePNI(N);
            NewNode = &CG.createNodeClonePNI(
                retypd::NodeKey{TypeVariable::CreateDtv(
                    CG.Ctx, ValueNamer::getName(NamePrefix))},
                PN);
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
              retypd::NFADeterminizer<>::allOutLabels(It->first);
          for (auto &L : outLabels) {
            auto S =
                countClosureFix(retypd::NFADeterminizer<>::move(It->first, L));
            if (DTrans.count(S) == 0) {
              auto NewNodeEnt = getOrSetNewNode(S);
              Worklist.push(NewNodeEnt);
            }
            auto &ToNode = *DTrans.at(S);
            CG.onlyAddEdge(Node, ToNode, L);
          }
          Worklist.pop();
        }
        return Ret;
      };
      for (int i = 0; i < ArgNodes.size(); i++) {
        auto Prefix = ("argd_" + std::to_string(i));
        auto &ArgSet = ArgNodes[i];
        auto *D = determinizeTo(ArgSet, Prefix.c_str());
        auto &To = CurrentTypes.getOrInsertNode(Current->getArg(i), nullptr);
        CurrentTypes.CG.onlyAddEdge(*D, To, retypd::One{});
        auto &ArgSetContra = ArgNodesContra[i];
        auto *DContra = determinizeTo(ArgSetContra, Prefix.c_str());
        auto *ToContra =
            CurrentTypes.CG.getNodeOrNull(MakeContraVariant(To.key));
        if (ToContra != nullptr) {
          CurrentTypes.CG.onlyAddEdge(*ToContra, *DContra, retypd::One{});
        }
      }
      if (!Current->getReturnType()->isVoidTy()) {
        auto *D = determinizeTo(RetNodes, "retd");
        auto &From =
            CurrentTypes.getOrInsertNode(ReturnValue{.Func = Current}, nullptr);
        CurrentTypes.CG.onlyAddEdge(From, *D, retypd::One{});
        auto *DContra = determinizeTo(RetNodesContra, "retd");
        auto *FromContra =
            CurrentTypes.CG.getNodeOrNull(MakeContraVariant(From.key));
        if (FromContra != nullptr) {
          CurrentTypes.CG.onlyAddEdge(*DContra, *FromContra, retypd::One{});
        }
      }
    }

    if (DirPath) {
      CurrentTypes.CG.printGraph(
          join(*DirPath, "03-CurrentTypes.linked.dot").c_str());
    }

    // 3. perform quotient equivalence on the graph..?
    // 4. determinize the graph
    // CurrentTypes.determinize();

    // if (DirPath) {
    //   CurrentTypes.CG.printGraph(
    //       join(*DirPath, "04-CurrentTypes.dtm.dot").c_str());
    // }

    // 5. maintain reverse call edge map
    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }
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

        auto &CallVec = FuncCalls[Edge.second];
        CallVec.emplace_back(I, CGN);
      }
    }

    // TODO further merge and simplify the graph?
  }

  if (DebugDir) {
    printAnnotatedModule(M,
                         join(DebugDir, "03-After-BottomUp.anno.ll").c_str());
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
      return llvm2c::UsedConstant(IC->Val, IC->User);
    }
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: getName: unhandled type of ExtValPtr\n";
    std::abort();
  };

  assert(SCCGraphs.size() == AllSCCs.size());
  // build AST type for each value in value map
  for (int i = 0; i < SCCGraphs.size(); i++) {
    auto &G = SCCGraphs[i];
    assert(G.PG);
    std::map<const CGNode *, CGNode *> Old2New;
    ConstraintsGenerator G2 = G.clone(Old2New);
    assert(G2.PG);
    std::string Name = G2.CG.Name;
    G2.CG.Name += "-dtm";
    if (DebugDir) {
      G2.DebugDir = DebugDir;
    }

    G2.eliminateCycle();
    // ensure lower bound is lower than upper bound
    G2.CG.linkContraToCovariant();

    if (DebugDir) {
      std::string Dir = join(DebugDir, "SCC" + std::to_string(i));
      G2.CG.printGraph(join(Dir, "04-BeforeMerge.dot").c_str());
    }

    // merge nodes that only subtype to another node
    G2.mergeOnlySubtype();

    G2.determinize();
    assert(G2.PG);
    G2.DebugDir.clear();

    if (DebugDir) {
      std::string Dir = join(DebugDir, "SCC" + std::to_string(i));
      G2.CG.printGraph(join(Dir, "04-CurrentTypes.dtm.dot").c_str());
    }

    for (auto &Ent : G2.Val2Node) {
      // TODO support function type.
      if (std::holds_alternative<llvm::Value *>(Ent.first)) {
        if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
          continue;
        }
      }
      auto *Node = Ent.second;
      assert(&Node->Parent == &G2.CG &&
             "RetypdGenerator::getTypeVar: Node is not in the graph");
      auto Size = getSize(Ent.first, pointer_size);
      assert(Size > 0);

      if (auto *V = std::get_if<llvm::Value *>(&Ent.first)) {
        llvm::errs() << "  Value: " << **V;
        llvm::Function *F = nullptr;
        if (auto I = llvm::dyn_cast<Instruction>(*V)) {
          F = I->getFunction();

        } else if (auto Arg = llvm::dyn_cast<Argument>(*V)) {
          F = Arg->getParent();
        }
        if (F) {
          llvm::errs() << " (In Func: " << F->getName() << ")";
        }
      } else {
        llvm::errs() << "  Special Value: " << getName(Ent.first);
        if (auto *UC = std::get_if<UConstant>(&Ent.first)) {
          llvm::errs() << " User: " << *UC->User;
        }
      }

      clang::QualType CTy = TB.buildType(*Node, Size);
      if (isGV(Ent.first) && Result.ValueTypes.count(Convert(Ent.first)) != 0) {
        llvm::errs() << "Warning: TODO handle Global Value type merge: "
                     << getName(Ent.first) << "\n";
      }
      assert(Result.ValueTypes.count(Convert(Ent.first)) == 0 ||
             isGV(Ent.first));
      Result.ValueTypes[Convert(Ent.first)] = CTy;

      llvm::errs() << " upper bound: " << CTy.getAsString();

      if (G2.CG.hasNode(retypd::MakeContraVariant(Node->key))) {
        CTy = TB.buildType(G2.CG.getNode(retypd::MakeContraVariant(Node->key)),
                           Size);

        if (auto *V = std::get_if<llvm::Value *>(&Ent.first)) {
          llvm::errs() << " lower bound: " << CTy.getAsString();
        } else {
          llvm::errs() << " lower bound: " << CTy.getAsString();
        }
        assert(Result.ValueTypesUpperBound.count(Convert(Ent.first)) == 0 ||
               isGV(Ent.first));
        Result.ValueTypesUpperBound[Convert(Ent.first)] = CTy;
      } else {
        llvm::errs() << " has no lower bound";
      }
      if (Node->getLowTy() != nullptr) {
        llvm::errs() << "  LowTy: " << *Node->getLowTy();
      }
      llvm::errs() << "\n";
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

ConstraintsGenerator
ConstraintsGenerator::clone(std::map<const CGNode *, CGNode *> &Old2New) {
  ConstraintsGenerator G(Ctx, LLCtx, CG.Name);
  ConstraintGraph::clone(Old2New, CG, G.CG);
  for (auto &Ent : Val2Node) {
    G.Val2Node.emplace(Ent.first, Old2New.at(Ent.second));
  }
  G.PG = G.CG.PG.get();
  G.SCCs = SCCs;
  G.CallToID = CallToID;
  return G;
}

void ConstraintsGenerator::cloneTo(
    ConstraintsGenerator &G, std::map<const CGNode *, CGNode *> &Old2New) {
  ConstraintGraph::clone(Old2New, CG, G.CG);
  for (auto &Ent : Val2Node) {
    G.Val2Node.emplace(Ent.first, Old2New.at(Ent.second));
  }
  G.PG = G.CG.PG.get();
  G.SCCs = SCCs;
  G.CallToID = CallToID;
}

void ConstraintsGenerator::removeUnreachable() {
  assert(CG.getStartNode()->outEdges.size() == 0);
  std::set<CGNode *> ReachableNodes;

  std::queue<CGNode *> Worklist;
  for (auto &Ent : Val2Node) {
    auto *Node = Ent.second;
    Worklist.push(Node);
    // also add contravariant node
    if (auto N = CG.getNodeOrNull(MakeContraVariant(Node->key))) {
      Worklist.push(N);
    }
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

std::set<CGNode *> countClosureBidirectionalEqual(const std::set<CGNode *> &N) {
  std::set<CGNode *> Ret;
  std::queue<CGNode *> Worklist;
  for (auto Node : N) {
    Ret.insert(Node);
    Worklist.push(Node);
  }
  while (!Worklist.empty()) {
    auto Node = Worklist.front();
    Worklist.pop();
    for (auto &Edge : Node->outEdges) {
      if (std::holds_alternative<retypd::One>(Edge.getLabel())) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        if (Ret.count(&Target) == 0) {
          Ret.insert(&Target);
          Worklist.push(&Target);
        }
      }
    }

    for (auto *Edge : Node->inEdges) {
      if (std::holds_alternative<retypd::One>(Edge->getLabel())) {
        auto &Target = const_cast<CGNode &>(Edge->getSourceNode());
        if (Ret.count(&Target) == 0) {
          Ret.insert(&Target);
          Worklist.push(&Target);
        }
      }
    }
  }
  return Ret;
}

std::set<CGNode *> countClosureStructEq(const std::set<CGNode *> &N) {
  auto Ret = countClosureBidirectionalEqual(N);
  bool HasPrimitive = false;
  for (auto Node : Ret) {
    if (Node->key.Base.isPrimitive()) {
      HasPrimitive = true;
      break;
    }
  }
  if (!HasPrimitive) {
    return Ret;
  } else {
    return retypd::NFADeterminizer<>::countClosure(N);
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
void ConstraintsGenerator::determinizeStructEqual() {
  // assert(DTrans.empty());
  DTrans.clear();
  std::map<const CGNode *, CGNode *> This2Bak;
  ConstraintGraph Backup = CG.clone(This2Bak, true);
  // remove all edges in the graph
  for (auto &Ent : CG) {
    for (auto &Edge : Ent.second.outEdges) {
      // TODO optimize:
      CG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
    }
  }
  // remove all node that is not in the value map
  std::set<CGNode *> Nodes;
  for (auto &Ent : Val2Node) {
    Nodes.insert(Ent.second);
  }
  for (auto &Ent : CG) {
    if (&Ent.second == CG.getStartNode() || &Ent.second == CG.getEndNode()) {
      continue;
    }
    if (Nodes.count(&Ent.second) == 0) {
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
                                  Ctx.TRCtx, ValueNamer::getName("dtm_"))},
                              OldPN);
    auto it = DTrans.emplace(N, &NewNode);
    assert(it.second);
    return it.first;
  };

  // map from replaced node to the new node
  std::map<CGNode *, CGNode *> ReplaceMap;
  auto Lookup = [&ReplaceMap](CGNode *Node) -> CGNode * {
    std::vector<CGNode *> PathNodes;
    while (ReplaceMap.count(Node)) {
      PathNodes.push_back(Node);
      Node = ReplaceMap.at(Node);
    }
    for (auto *N : PathNodes) {
      ReplaceMap[N] = Node;
    }
    return Node;
  };

  DTrans[{Backup.getEndNode()}] = CG.getEndNode();
  std::queue<EntryTy> Worklist;
  // for each node in the value map
  for (auto &Ent : Val2Node) {
    auto *BakNode = This2Bak.at(Ent.second);
    std::set<CGNode *> StartSet = countClosureStructEq({BakNode});
    auto pair1 = DTrans.emplace(StartSet, Ent.second);
    if (pair1.second) {
      Worklist.push(pair1.first);
    } else {
      // Can be a epsilon loop.
      // TODO merge the node in the value map
      if (pair1.first->second != Ent.second) {
        // merge the node in the value map
        std::cerr
            << "ConstraintsGenerator::determinizeStructEqual: Merging node "
            << toString(Ent.second->key) << " to "
            << toString(pair1.first->second->key) << "\n";
        // Ent.second = pair1.first->second;
        // CG.removeNode(Ent.second->key);
        ReplaceMap.emplace(Ent.second, pair1.first->second);
        // assert(false);
      }
    }
  }

  // DSU flatten for Val2Node
  for (auto &Ent : Val2Node) {
    if (ReplaceMap.count(Ent.second)) {
      Ent.second = Lookup(Ent.second);
    }
  }

  // remove all node that is not in the value map again
  Nodes.clear();
  for (auto &Ent : Val2Node) {
    Nodes.insert(Ent.second);
  }
  for (auto &Ent : CG) {
    if (&Ent.second == CG.getStartNode() || &Ent.second == CG.getEndNode()) {
      continue;
    }
    if (Nodes.count(&Ent.second) == 0) {
      CG.removeNode(Ent.second.key);
    }
  }

  while (!Worklist.empty()) {
    auto It = Worklist.front();
    auto &Node = *It->second;
    std::set<retypd::EdgeLabel> outLabels =
        retypd::NFADeterminizer<>::allOutLabels(It->first);
    for (auto &L : outLabels) {
      auto S = countClosureStructEq(moveLoadEqStore(It->first, L));
      if (S.count(Backup.getEndNode())) {
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
}

void ConstraintsGenerator::eliminateCycle() {
  std::map<const CGNode *, CGNode *> Old2New;
  ConstraintGraph NewG = CG.clone(Old2New, true);

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

  // build the reverse value map: TODO migrate to maintain
  std::map<const retypd::CGNode *, std::set<ExtValuePtr>> Node2Val;
  for (auto &Ent : Val2Node) {
    Node2Val[Ent.second].insert(Ent.first);
  }

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
      if (Node2Val.count(Node) == 0) {
        continue;
      }
      for (auto &Val : Node2Val.at(Node)) {
        ToMergeVal.insert(Val);
      }
    }
    // 2. output the merged node name map to txt
    auto &Merged = **ToMerge.begin();
    if (!DebugDir.empty()) {
      std::ofstream Out(join(DebugDir, "Cycles.txt"), std::ios::app);
      for (auto *Node : ToMerge) {
        Out << toString(Node->key) << ", ";
      }
      Out << "\n";
      Out.close();
    }

    auto *OldPN = notdec::retypd::NFADeterminizer<>::ensureSamePNI(ToMerge);
    // 3. make the value map to only one node
    // Update value map
    for (auto &Val : ToMergeVal) {
      Val2Node[Val] = &Merged;
    }
    // ensure other nodes are not in the value map
    for (auto &Ent : Val2Node) {
      if (Ent.second == &Merged) {
        continue;
      }
      assert(ToMerge.count(Ent.second) == 0);
    }
    // move all incoming edges and all outgoing edges to the merged node
    for (auto *Node : ToMerge) {
      if (Node == &Merged) {
        continue;
      }
      for (auto &Edge : Node->outEdges) {
        if (ToMerge.count(&Edge.TargetNode) != 0) {
          CG.removeEdge(*Node, Edge.TargetNode, Edge.Label);
          continue;
        }
        CG.onlyAddEdge(Merged, Edge.TargetNode, Edge.Label);
        CG.removeEdge(*Node, Edge.TargetNode, Edge.Label);
      }
      for (auto *Edge : Node->inEdges) {
        if (ToMerge.count(&Edge->getSourceNode()) != 0) {
          CG.removeEdge(Edge->getSourceNode(), *Node, Edge->getLabel());
          continue;
        }
        CG.onlyAddEdge(Edge->getSourceNode(), Merged, Edge->getLabel());
        CG.removeEdge(Edge->getSourceNode(), *Node, Edge->getLabel());
      }
    }
    // erase all other nodes
    for (auto *Node : ToMerge) {
      if (Node == &Merged) {
        continue;
      }
      CG.removeNode(Node->key);
    }
  }
}

void ConstraintsGenerator::mergeNodeTo(CGNode &From, CGNode &To,
                                       bool NoSelfLoop) {
  assert(&From.Parent == &CG && &To.Parent == &CG);
  // Fix up value map first
  for (auto &Ent : Val2Node) {
    if (Ent.second == &From) {
      Val2Node[Ent.first] = &To;
    }
  }
  CG.mergeNodeTo(From, To, NoSelfLoop);
}

void ConstraintsGenerator::mergeOnlySubtype() {
  // perform merging: if node A -> B, and there is no other edge from A to other
  // node, then merge A to B
  // for each node and outgoing edge, check if is one edge
  auto findMergePair = [&]() -> std::pair<CGNode *, CGNode *> {
    for (auto &Ent : CG.Nodes) {
      auto &Node = Ent.second;
      if (&Node == CG.getStartNode() || &Node == CG.getEndNode()) {
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
  ConstraintGraph Backup = CG.clone(This2Bak, false);
  // remove all edges in the graph
  for (auto &Ent : CG) {
    for (auto &Edge : Ent.second.outEdges) {
      CG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
    }
  }
  // remove all node that is not in the value map
  std::set<CGNode *> V2NNodes;
  for (auto &Ent : Val2Node) {
    V2NNodes.insert(Ent.second);
    // contravariant nodes.
    if (CG.Nodes.count(MakeContraVariant(Ent.second->key)) != 0) {
      V2NNodes.insert(&CG.getNode(MakeContraVariant(Ent.second->key)));
    }
  }
  for (auto &Ent : CG) {
    if (&Ent.second == CG.getStartNode() || &Ent.second == CG.getEndNode()) {
      continue;
    }
    if (V2NNodes.count(&Ent.second) == 0) {
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
                                  Ctx.TRCtx, ValueNamer::getName("dtm_"))},
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
      // Can be a epsilon loop.
      // TODO merge the node in the value map
      if (pair1.first->second != Node) {
        Backup.printEpsilonLoop(
            "debugloop",
            *reinterpret_cast<std::set<const CGNode *> *>(&StartSet));
        Backup
            .getSubGraph(
                *reinterpret_cast<std::set<const CGNode *> *>(&StartSet), false)
            .printGraph("debug_sub.dot");
        assert(false);
      }
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
  auto CastNode = [&](const CGNode &Node) -> CGNode & {
    return const_cast<CGNode &>(Node);
  };
  auto SameOutEdges = [&](const CGNode &N1, const CGNode &N2) -> bool {
    assert(&N1 != &N2);
    if (N1.getPNIVar() != N2.getPNIVar()) {
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
      if (&Node == CG.getStartNode() || &Node == CG.getEndNode()) {
        continue;
      }
      if (Node.key.Base.isPrimitive()) {
        assert(false);
      }

      // check for two edges with same label
      for (auto *Edge1 : Node.inEdges) {
        for (auto *Edge2 : Node.inEdges) {
          auto &N1 = Edge1->getSourceNode();
          auto &N2 = Edge2->getSourceNode();
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

// void ConstraintsGenerator::instantiateSketchAsSub(
//     ExtValuePtr Val, std::shared_ptr<retypd::Sketch> Sk) {
//   CGNode &Root = CG.instantiateSketch(Sk);
//   addSubtype(Root.key.Base, getTypeVar(Val, nullptr));
// }

// void ConstraintsGenerator::instantiateSketchAsSup(
//     ExtValuePtr Val, std::shared_ptr<retypd::Sketch> Sk) {
//   CGNode &Root = CG.instantiateSketch(Sk);
//   addSubtype(getTypeVar(Val, nullptr), Root.key.Base);
// }

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

void ConstraintsGenerator::instantiateSummary(llvm::Function *Target,
                                              size_t InstanceId) {
  const std::vector<retypd::SubTypeConstraint> *Sum = GetSummary(Target);
  CG.instantiate(*Sum, InstanceId);
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

retypd::CGNode *ConstraintsGenerator::getNodeOrNull(ExtValuePtr Val,
                                                    User *User) {
  wrapExtValuePtrWithUser(Val, User);

  if (Val2Node.count(Val)) {
    return Val2Node.at(Val);
  }
  return nullptr;
}

retypd::CGNode &ConstraintsGenerator::getNode(ExtValuePtr Val, User *User) {
  wrapExtValuePtrWithUser(Val, User);

  return *Val2Node.at(Val);
}

retypd::CGNode &ConstraintsGenerator::createNode(ExtValuePtr Val, User *User) {
  wrapExtValuePtrWithUser(Val, User);
  auto Dtv = convertTypeVar(Val, User);
  auto It = Val2Node.emplace(Val, &CG.createNode(Dtv, getType(Val)));
  if (!It.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "setTypeVar: Value already mapped to "
                 << toString(It.first->second->key.Base) << ", but now set to "
                 << toString(Dtv) << "\n";
    std::abort();
  }
  return *It.first->second;
}

retypd::CGNode &ConstraintsGenerator::getOrInsertNode(ExtValuePtr Val,
                                                      User *User) {
  wrapExtValuePtrWithUser(Val, User);
  auto Node = getNodeOrNull(Val, User);
  if (Node != nullptr) {
    return *Node;
  }
  return createNode(Val, User);
}

const TypeVariable &ConstraintsGenerator::getTypeVar(ExtValuePtr Val,
                                                     User *User) {
  return getOrInsertNode(Val, User).key.Base;
}

TypeVariable ConstraintsGenerator::convertTypeVar(ExtValuePtr Val, User *User) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertTypeVarVal(*V, User);
  } else if (auto F = std::get_if<ReturnValue>(&Val)) {
    auto tv = makeTv(Ctx.TRCtx, getFuncTvName(F->Func));
    tv = tv.pushLabel(retypd::OutLabel{});
    return tv;
  } else if (auto Arg = std::get_if<CallArg>(&Val)) { // TODO: deprecated
    assert(false);
    // TODO what if function pointer?
    auto tv = getCallArgTV(Ctx.TRCtx, Arg->Call->getCalledFunction(),
                           CallToID.at(Arg->Call), Arg->Index);
    // makeTv(Ctx.TRCtx, getFuncTvName(Arg->Call->getCalledFunction()));
    // // tv.getInstanceId() = Arg->InstanceId;
    // tv = tv.pushLabel(retypd::InLabel{std::to_string(Arg->Index)});
    return tv;
  } else if (auto Ret = std::get_if<CallRet>(&Val)) { // TODO: deprecated
    assert(false);
    // TODO what if function pointer?
    auto tv = getCallRetTV(Ctx.TRCtx, Ret->Call->getCalledFunction(),
                           CallToID.at(Ret->Call));
    // makeTv(Ctx.TRCtx, getFuncTvName(Ret->Call->getCalledFunction()));
    // // tv.getInstanceId() = Ret->InstanceId;
    // tv = tv.pushLabel(retypd::OutLabel{});
    return tv;
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
    if (auto CI = dyn_cast<ConstantInt>(IC->Val)) {
      auto ret = TypeVariable::CreateIntConstant(
          Ctx.TRCtx, OffsetRange{.offset = CI->getSExtValue()}, User);
      return ret;
    }
    return convertTypeVarVal(IC->Val, IC->User);
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
        return getTypeVar(CE->getOperand(0), CE);
      } else if (CE->getOpcode() == Instruction::IntToPtr) {
        if (auto Addr = dyn_cast<ConstantInt>(CE->getOperand(0))) {
          auto tv = CG.getMemoryNode()->key.Base;
          tv = addOffset(tv, OffsetRange{.offset = Addr->getSExtValue()});
          return tv;
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
                  auto tv = makeTv(Ctx.TRCtx, GV->getName().str());
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
        // Consistent with Call handling
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
    }
    llvm::errs()
        << __FILE__ << ":" << __LINE__ << ": "
        << "ERROR: RetypdGenerator::getTypeVar unhandled type of constant: "
        << *C << "\n";
    std::abort();
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    // Consistent with Call handling
    TypeVariable tv = getTypeVar(arg->getParent(), nullptr);
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
  auto &SrcVar = cg.getOrInsertNode(Src, &I);
  auto &DstVar = cg.createNode(ReturnValue{.Func = I.getFunction()}, &I);
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

  if (cg.SCCs.count(Target)) {
    // Call within the SCC:
    // directly link to the function tv.
    for (int i = 0; i < I.arg_size(); i++) {
      auto &ArgVar = cg.getOrInsertNode(Target->getArg(i), &I);
      auto &ValVar = cg.getOrInsertNode(I.getArgOperand(i), &I);
      // argument is a subtype of param
      cg.addSubtype(ValVar, ArgVar);
    }
    if (!I.getType()->isVoidTy()) {
      // type var should be consistent with return instruction
      auto &FormalRetVar = cg.getOrInsertNode(ReturnValue{.Func = Target}, &I);
      auto &ValVar = cg.getOrInsertNode(&I, nullptr);
      // formal return -> actual return
      cg.addSubtype(FormalRetVar, ValVar);
    }
  } else {
    // differentiate different call instances in the same function
    size_t InstanceId = ValueNamer::getId();
    if (!Target->isDeclaration()) {
      cg.instantiateSummary(Target, InstanceId);
    }
    cg.CallToID.emplace(&I, InstanceId);
    for (int i = 0; i < I.arg_size(); i++) {
      auto ArgVar = getCallArgTV(cg.Ctx.TRCtx, Target, InstanceId, i);
      auto &ValVar = cg.getOrInsertNode(I.getArgOperand(i), &I);
      auto &ArgNode =
          cg.CG.getOrInsertNode(ArgVar, I.getArgOperand(i)->getType());
      // argument is a subtype of param
      cg.addSubtype(ValVar, ArgNode);
    }
    if (!I.getType()->isVoidTy()) {
      // for return value
      auto FormalRetVar = getCallRetTV(cg.Ctx.TRCtx, Target, InstanceId);
      auto &ValVar = cg.getOrInsertNode(&I, nullptr);
      auto &RetNode = cg.CG.getOrInsertNode(FormalRetVar, I.getType());
      // formal return -> actual return
      cg.addSubtype(RetNode, ValVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitSelectInst(
    SelectInst &I) {
  auto &DstVar = cg.createNode(&I, nullptr);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto &Src1Var = cg.getOrInsertNode(Src1, &I);
  auto &Src2Var = cg.getOrInsertNode(Src2, &I);
  // Not generate boolean constraints. Because it must be i1.
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitAllocaInst(
    AllocaInst &I) {
  auto &Node = cg.createNode(&I, nullptr);
  // set as pointer type
  cg.setPointer(Node);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  // Defer constraints generation (and unification) to handlePHINodes
  phiNodes.push_back(&I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto &DstVar = cg.getNode(I, nullptr);
    for (auto &Op : I->incoming_values()) {
      auto *Src = Op.get();
      auto &SrcVar = cg.getOrInsertNode(Src, I);
      // src is a subtype of dest
      cg.addSubtype(SrcVar, DstVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto &SrcVar = cg.getOrInsertNode(Src, &I);
    cg.addVarSubtype(&I, SrcVar);
    return;
  } else if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *Src = I.getOperand(0);
    auto &SrcVar = cg.getOrInsertNode(Src, &I);
    /*auto &Node = */ cg.addVarSubtype(&I, SrcVar);
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
  getOrInsertNode(LHS, I).getPNIVar()->unify(
      *getOrInsertNode(RHS, I).getPNIVar());
}

// for pointer sized int, probably is pointer comparision. So we cannot make a
// conclusion of subtype. but create a CmpConstraint.
void ConstraintsGenerator::RetypdGeneratorVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addCmpConstraint(Src1, Src2, &I);

  // type the inst as bool?
  assert(I.getType()->isIntegerTy(1));
  cg.createNode(&I, nullptr);
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
      cg.deref(I.getPointerOperand(), &I,
               cg.getPointerElemSize(I.getPointerOperandType()), false);
  auto &StoreNode = cg.CG.createNode(
      DstVar, I.getPointerOperandType()->getPointerElementType());
  // actual store -> formal store
  cg.addSubtype(cg.createNode(I.getValueOperand(), &I), StoreNode);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  auto LoadedVar =
      cg.deref(I.getPointerOperand(), &I,
               cg.getPointerElemSize(I.getPointerOperandType()), true);
  auto &LoadNode = cg.CG.createNode(LoadedVar, I.getType());
  // formal load -> actual load
  cg.addSubtype(LoadNode, cg.createNode(&I, nullptr));
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
  setPointer(getOrInsertNode(Val, User));
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
  PG->addAddCons(&getOrInsertNode(LHS, I), &getOrInsertNode(RHS, I),
                 &getOrInsertNode(I, nullptr), I);
}

void ConstraintsGenerator::addSubConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            BinaryOperator *I) {
  PG->addSubCons(&getOrInsertNode(LHS, I), &getOrInsertNode(RHS, I),
                 &getOrInsertNode(I, nullptr), I);
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

  auto &Src1Node = cg.getOrInsertNode(Src1, &I);
  auto &Src2Node = cg.getOrInsertNode(Src2, &I);
  auto &RetNode = cg.getOrInsertNode(&I, nullptr);

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

  auto &Src1Node = cg.getOrInsertNode(Src1, &I);
  auto &Src2Node = cg.getOrInsertNode(Src2, &I);
  auto &RetNode = cg.getOrInsertNode(&I, nullptr);

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
  auto &N = cg.createNode(I, nullptr);
  if (I->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = output;
  if (ty == nullptr) { // no action
    return true;
  } else if (strEq(ty, "sint")) {
    N.getPNIVar()->setNonPtr();
    auto &SintNode = cg.CG.getOrCreatePrim(
        "sint" + std::to_string(I->getType()->getIntegerBitWidth()),
        I->getType());
    cg.addSubtype(SintNode, N);
    return true;
  } else if (strEq(ty, "uint")) {
    N.getPNIVar()->setNonPtr();
    auto &UintNode = cg.CG.getOrCreatePrim(
        "uint" + std::to_string(I->getType()->getIntegerBitWidth()),
        I->getType());
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
  auto &N = cg.getOrInsertNode(Op, I);
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
                  ReturnValue{.Func = const_cast<llvm::Function *>(F)}, nullptr)
                .str();
      OS << " <- ";
    }
    OS << "(";
    for (auto &Arg : F->args()) {
      OS << CG->getOrInsertNode(const_cast<llvm::Argument *>(&Arg), nullptr)
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
      OS << CG->getOrInsertNode(const_cast<llvm::Value *>(&V), nullptr).str();
      if (Instr != nullptr) {
        OS << " <- ";
      }
    }
    if (Instr != nullptr) {
      OS << "(";
      for (const Use &Op : Instr->operands()) {
        auto *Node =
            CG->getNodeOrNull(Op.get(), const_cast<llvm::Instruction *>(Instr));
        OS << (Node == nullptr ? "null" : Node->str()) << ", ";
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

} // namespace notdec
