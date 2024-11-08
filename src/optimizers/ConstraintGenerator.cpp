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
  } else if (auto IC = std::get_if<IntConstant>(&Val)) {
    llvm::errs() << "IntConstant: " << *IC->Val;
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

// TODO refactor: increase code reusability
ExtValuePtr getExtValuePtr(llvm::Value *V, User *User) {
  ExtValuePtr Val = V;
  if (auto CI = dyn_cast<ConstantInt>(V)) {
    if (CI->getBitWidth() == 32 || CI->getBitWidth() == 64) {
      assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
      assert(hasUser(V, User) &&
             "convertTypeVarVal: constant not used by user");
      Val = IntConstant{.Val = cast<ConstantInt>(V), .User = User};
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
  ConstraintsGenerator GlobalTypes(*this, "GlobalTypes");

  TypeRecovery::Result Result;
  retypd::SketchToCTypeBuilder TB(M.getName());

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

    Generator->CG.printGraph("Current.before-sks.dot");

    // check if saturated
    // 0. pre-process the big graph: focus on the push/recall subgraph.
    Generator->CG.sketchSplit();
    Generator->removeUnreachable();

    // TODO remove debug output
    Generator->CG.printGraph("Current.sks.dot");
    GlobalTypes.CG.printGraph("Global.before.dot");

    // copy all nodes into the global type graph
    for (auto &Ent : Generator->CG) {
      auto &Node = Ent.second;
      auto &NewNode = GlobalTypes.CG.getOrInsertNode(Node.key);
      for (auto &Edge : Node.outEdges) {
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        auto &NewTarget = GlobalTypes.CG.getOrInsertNode(Target.key);
        GlobalTypes.CG.onlyAddEdge(NewNode, NewTarget, Edge.Label);
      }
    }
    // maintain the value map
    for (auto &Ent : Generator->Val2Node) {
      auto &Node = *Ent.second;
      auto &NewNode = GlobalTypes.CG.getOrInsertNode(Node.key);
      GlobalTypes.Val2Node.emplace(Ent.first, &NewNode);
    }

    // 2. link the argument/return value subtype relations.
    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }
      if (FuncSketches.count(Current) == 0) {
        continue;
      }
      auto &CurrentSketches = FuncSketches.at(Current);
      if (CurrentSketches.empty()) {
        std::cerr << "No actual param types for " << Current->getName().str()
                  << "\n";
      }
      for (auto *Call : CurrentSketches) {
        for (int i = 0; i < Current->arg_size(); i++) {
          auto *Arg = Current->getArg(i);
          // assume the type variable is now in the graph.
          GlobalTypes.CG.onlyAddEdge(
              GlobalTypes.getNode(Call->getArgOperand(i), Call),
              GlobalTypes.getNode(Arg, nullptr), retypd::One{});
        }
        if (!Current->getReturnType()->isVoidTy()) {
          GlobalTypes.CG.onlyAddEdge(
              GlobalTypes.getNode(ReturnValue{.Func = Current}, nullptr),
              GlobalTypes.getNode(Call, nullptr), retypd::One{});
        }
      }
    }

    GlobalTypes.CG.printGraph("GlobalTypes.linked.dot");

    // 3. perform quotient equivalence on the graph..?
    // 4. determinize the graph
    GlobalTypes.determinize();
    GlobalTypes.CG.printGraph("GlobalTypes.dtm.dot");

    // 5. save the special actual arg and return value nodes.
    // for each callee
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

        auto &TargetSketches = FuncSketches[Target];
        TargetSketches.push_back(I);
      }
    }

    // TODO further merge and simplify the graph?
  }

  // build AST type for each value in value map
  for (auto &Ent : GlobalTypes.Val2Node) {
    // TODO support function type.
    if (std::holds_alternative<llvm::Value *>(Ent.first)) {
      if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
        continue;
      }
    }
    auto *Node = Ent.second;
    clang::QualType CTy = TB.buildType(*Node, Node->Size);
    if (auto *V = std::get_if<llvm::Value *>(&Ent.first)) {
      llvm::errs() << "  Value: " << **V << " has type: " << CTy.getAsString()
                   << "\n";
    } else {
      llvm::errs() << "  Special Value: " << getName(Ent.first)
                   << " has type: " << CTy.getAsString() << "\n";
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

void ConstraintsGenerator::removeUnreachable() {
  assert(CG.getStartNode()->outEdges.size() == 0);
  std::set<CGNode *> ReachableNodes;

  std::queue<CGNode *> Worklist;
  for (auto &Ent : Val2Node) {
    Worklist.push(Ent.second);
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
    auto &NewNode = CG.getOrInsertNode(retypd::NodeKey{
        TypeVariable::CreateDtv(Ctx.TRCtx, ValueNamer::getName("dtm_"))});
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

void ConstraintsGenerator::determinize() {
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
    auto &NewNode = CG.getOrInsertNode(retypd::NodeKey{
        TypeVariable::CreateDtv(Ctx.TRCtx, ValueNamer::getName("dtm_"))});
    auto it = DTrans.emplace(N, &NewNode);
    assert(it.second);
    return it.first;
  };

  DTrans[{Backup.getEndNode()}] = CG.getEndNode();
  std::queue<EntryTy> Worklist;
  // for each node in the value map
  for (auto &Ent : Val2Node) {
    auto *BakNode = This2Bak.at(Ent.second);
    std::set<CGNode *> StartSet =
        retypd::NFADeterminizer<>::countClosure({BakNode});
    auto pair1 = DTrans.emplace(StartSet, Ent.second);
    if (pair1.second) {
      Worklist.push(pair1.first);
    } else {
      // Can be a epsilon loop.
      // TODO merge the node in the value map
      if (pair1.first->second != Ent.second) {
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
      auto S = retypd::NFADeterminizer<>::countClosure(
          retypd::NFADeterminizer<>::move(It->first, L));
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
    // differentiate different call instances in the same function
    size_t InstanceId = ValueNamer::getId();
    if (!Target->isDeclaration()) {
      cg.instantiateSummary(Target, InstanceId);
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
