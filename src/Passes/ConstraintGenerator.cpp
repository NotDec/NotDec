#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Intrinsics.h>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <llvm/ADT/Optional.h>
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
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

#include "Passes/AllocAnnotator.h"
#include "Passes/ConstraintGenerator.h"
#include "Passes/DSROA.h"
#include "Passes/StackAlloca.h"
#include "Passes/StackBreaker.h"
#include "Passes/StackPointerFinder.h"
#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/LowTy.h"
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/AllSCCIterator.h"
#include "Utils/CallGraphDotInfo.h"
#include "Utils/SingleNodeSCCIterator.h"
#include "Utils/Utils.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/Range.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Interface/Utils.h"
#include "notdec-llvm2c/Interface/ValueNamer.h"
#include "notdec-llvm2c/Utils.h"

#define DEBUG_TYPE "type-recovery"

using namespace llvm;

namespace notdec {

using retypd::NodeKey;
using retypd::OffsetLabel;

// NOTDEC_TYPE_RECOVERY_DEBUG_DIR
const char *getTRDebugDir() {
  return std::getenv("NOTDEC_TYPE_RECOVERY_DEBUG_DIR");
}

bool isDisableInterFunction() {
  bool DisableInterFunction = false;
  if (auto E1 = std::getenv("NOTDEC_DISABLE_INTERPROC")) {
    if (std::strcmp(E1, "1") == 0) {
      DisableInterFunction = true;
    }
  }
  return DisableInterFunction;
}

std::optional<std::string> getSCCDebugDir(std::size_t SCCIndex) {
  const char *DebugDir = getTRDebugDir();
  if (DebugDir) {
    std::string DirPath = join(DebugDir, "SCC" + std::to_string(SCCIndex));
    auto EC = llvm::sys::fs::create_directories(DirPath);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open create directory " << DirPath << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    return DirPath;
  }
  return std::nullopt;
}

static inline TypeVariable makeTv(std::shared_ptr<retypd::TRContext> Ctx,
                                  std::string Name) {
  return retypd::TypeVariable::CreateDtv(*Ctx, Name);
}

std::string getUniquePath(const std::string &basePath, const char *suffix) {
  static unsigned int index = 0;
  std::string candidate = basePath + "." + std::to_string(index) + suffix;

  while (llvm::sys::fs::exists(candidate)) {
    candidate = basePath + "." + std::to_string(index) + suffix;
    ++index;
  }

  return candidate;
}

// #region TypeRecovery

void TypeRecovery::loadSummaryFile(Module &M, const char *SummaryFile) {
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

void TypeRecovery::loadSignatureFile(Module &M, const char *SigFile) {
  if (getSuffix(SigFile) == ".json") {
    llvm::errs() << "Loading signature from: " << SigFile << "\n";
    auto ValE = json::parse(readFileToString(SigFile));
    assert(ValE && "JSON parse failed");
    auto Val = ValE->getAsObject();
    for (auto &Ent : *Val) {
      llvm::errs() << "Loading signature for function: " << Ent.first << "\n";
      auto *F = M.getFunction(Ent.first);
      if (F == nullptr) {
        llvm::errs() << "Warning: Function not found: " << Ent.first.str()
                     << "\n";
        continue;
      }
      retypd::ConstraintSummary Summary{{}, pointer_size, {}};
      Summary.fromJSON(*TRCtx, *Ent.second.getAsObject());
      auto CG = ConstraintsGenerator::fromConstraints(*this, {F}, Summary);
      CG->CG.linkPrimitives();
      SignatureOverride[F] = CG;
    }
  } else if (getSuffix(SigFile) == ".dot") {
    assert(false && "TODO");
  }
}

CGNode *
TypeRecovery::multiGraphDeterminizeTo(ConstraintsGenerator &CurrentTypes,
                                      std::set<CGNode *> &StartNodes,
                                      const char *NamePrefix) {
  auto &CG = CurrentTypes.CG;
  std::map<std::set<CGNode *>, CGNode *> DTrans;
  using EntryTy = typename std::map<std::set<CGNode *>, CGNode *>::iterator;

  auto ignoreForgetAndOne = [](const retypd::EdgeLabel &L) {
    auto isForget = L.isForgetLabel();
    auto isOne = L.isOne();
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
      NewNode =
          &CG.createNodeClonePNI(retypd::NodeKey{TypeVariable::CreateDtv(
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
  Worklist.push(getOrSetNewNode(countClosureNoPrimitiveEdges(StartNodes)));
  auto *Ret = Worklist.front()->second;

  while (!Worklist.empty()) {
    auto It = Worklist.front();
    auto &Node = *It->second;
    std::set<retypd::EdgeLabel> outLabels =
        retypd::NFADeterminizer<>::allOutLabels(It->first, ignoreForgetAndOne);
    for (auto &L : outLabels) {
      auto S = countClosureNoPrimitiveEdges(
          retypd::NFADeterminizer<>::move(It->first, L));
      if (DTrans.count(S) == 0) {
        auto NewNodeEnt = getOrSetNewNode(S);
        Worklist.push(NewNodeEnt);
      }
      auto &ToNode = *DTrans.at(S);
      assert(L.isRecallLabel() || L.isForgetBase());
      if (auto RL = L.getAs<retypd::RecallLabel>()) {
        CG.addEdge(Node, ToNode, L);
        CG.addEdge(ToNode, Node, {retypd::ForgetLabel{RL->label}});
      } else if (auto FB = L.getAs<retypd::ForgetBase>()) {
        assert(&ToNode == CG.getEndNode());
        CG.addEdge(Node, ToNode, L);
        CG.addEdge(*CG.getStartNode(), Node,
                   {retypd::RecallBase{.Base = FB->Base, .V = FB->V}});
      }
    }
    Worklist.pop();
  }
  return Ret;
}

// #endregion TypeRecovery

void ConstraintsGenerator::dumpV2N() {
  llvm::errs() << CG.getName() << " V2N:\n";
  for (auto &Ent : V2N) {
    llvm::errs() << toString(Ent.first) << " -> " << Ent.second->str() << "\n";
  }
  llvm::errs() << "V2NContra:\n";
  for (auto &Ent : V2NContra) {
    llvm::errs() << toString(Ent.first) << " -> " << Ent.second->str() << "\n";
  }
}

// void ConstraintsGenerator::removeNode(const retypd::NodeKey &K) {
//   assert(!V2N.count(K));
//   assert(!V2NContra.count(K));
//   return CG.removeNode(K);
// }

void ConstraintsGenerator::offZeroToOne() {
  for (auto &N : CG) {
    if (N.isStartOrEnd()) {
      continue;
    }
    std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToUpdate;
    for (auto &Edge : N.outEdges) {
      auto Source = const_cast<CGNode *>(&Edge.getSourceNode());
      auto Target = const_cast<CGNode *>(&Edge.getTargetNode());
      if (auto Off = retypd::getOffsetLabel(Edge.getLabel())) {
        if (Off->range.access.empty() && Off->range.offset == 0) {
          ToUpdate.push_back(std::make_tuple(Source, Target, Edge.getLabel()));
        }
      }
    }
    for (auto &Ent : ToUpdate) {
      auto Source = std::get<0>(Ent);
      auto Target = std::get<1>(Ent);
      auto &Label = std::get<2>(Ent);
      CG.removeEdge(*Source, *Target, Label);
      CG.addEdge(*Source, *Target, {retypd::One{}});
    }
  }
}

void ConstraintsGenerator::removeNode(retypd::CGNode &N) {
  assert(!TypeInfos.count(&N));
  CG.removeNode(N);
}

std::shared_ptr<ConstraintsGenerator>
TypeRecovery::postProcess(ConstraintsGenerator &G,
                          std::optional<std::string> DebugDir) {
  const clock_t begin_time = clock();

  int Level = 0;
  if (auto S = std::getenv("NOTDEC_POSTPROCESS_LEVEL")) {
    if (strlen(S) > 0) {
      if (S[0] == '0') {
        Level = 0;
      } else if (S[0] == '1') {
        Level = 1;
      } else if (S[0] == '2') {
        Level = 2;
      } else {
        std::cerr << "Error: Unrecognized value in NOTDEC_POSTPROCESS_LEVEL: "
                  << S << "\n";
      }
    }
  }

  if (G.PG) {
    G.PG->clearConstraints();
  }
  std::map<const CGNode *, CGNode *> Old2New;

  std::shared_ptr<ConstraintsGenerator> G2S;
  if (Level != 0) {
    G2S = G.cloneShared(Old2New);

    assert(!G2S->CG.isNotSymmetry && "postProcess: already not Symmetry?");
    G2S->CG.isNotSymmetry = true;

    G2S->CallToInstance.clear();
    G2S->UnhandledCalls.clear();

    ConstraintsGenerator &G2 = *G2S;
    assert(G2.PG);
    std::string Name = G2.CG.Name;
    G2.CG.Name += "-dtm";

    // if (G2.CG.Name == "strlen-sig-dtm") {
    //   std::cerr << "here\n";
    // }

    G2.CG.sketchSplit();
    G2.CG.changeStoreToLoad();
    // G2.CG.ensureNoForgetLabel();
    G2.offZeroToOne();

    if (DebugDir) {
      G2.CG.printGraph(
          getUniquePath(join(*DebugDir, "07-BeforeDtm"), ".dot").c_str());
    }
  }

  std::shared_ptr<ConstraintsGenerator> G3S;

  // level 0: subtype equal
  // level 1: determinize.
  // level 2: minimize.
  if (Level == 0) {

    // G2.eliminateCycle(DebugDir);
    // G2.makePointerEqual(DebugDir);
    // G2.quotientMerge();

    G2S = G.genSketch(Old2New, DebugDir);
    ConstraintsGenerator &G2 = *G2S;
    G2.eliminateCycle(DebugDir);
    G2.hasNoOnes();
    G3S = G2S;
  } else if (Level == 1) {
    ConstraintsGenerator &G2 = *G2S;
    G2.eliminateCycle(DebugDir);
    // // merge nodes that only subtype to another node
    // G2.mergeOnlySubtype();
    // // G2.CG.ensureNoForgetLabel();
    G2.determinize();
    // G2.CG.aggressiveSimplify();
    G3S = G2S;
  } else if (Level == 2) {
    ConstraintsGenerator &G2 = *G2S;
    G2.eliminateCycle(DebugDir);
    std::map<const CGNode *, CGNode *> Old2New2;
    G3S = G2.minimizeShared(Old2New2);
  } else {
    assert(false);
  }

  auto &G3 = *G3S;
  if (DebugDir) {
    G3.CG.printGraph(
        getUniquePath(join(*DebugDir, "08-Final"), ".dot").c_str());
  }

  // Graph Pass
  G3.organizeTypes();

  // G3.mergeArrayUnions();
  // G3.mergeArrayWithMember();
  // G3.elimSingleStruct();

  assert(G3.PG);

  auto DurationMS = (float(clock() - begin_time) * 1000 / CLOCKS_PER_SEC);
  if (DurationMS > 100) {
    std::cerr << "TypeRecovery::postProcess: " << DurationMS << "ms for "
              << G3.CG.Name << ".\n";
  }

  return G3S;
}

void ConstraintsGenerator::elimSingleStruct() {
  assert(false && "TODO");
  // 如果存在一个只有一个成员的struct，那么就消除它。
  for (auto &N : CG) {
    if (TypeInfos.count(&N)) {
      auto &Info = TypeInfos.at(&N);
      if (auto SInfo = Info.getAs<StructInfo>()) {
        if (SInfo->Fields.size() == 1) {
        }
      }
    }
  }
}

void ConstraintsGenerator::mergeArrayWithMember() {}

void ConstraintsGenerator::preSimplify() {
  // 注意维护对称性

  // 对简单的顺序子类型关系A -> x -> B可以直接优化：
  // - x节点的下界是A的下界，x节点的上界是B的上界。
  // 具体操作：
  // - 如果x节点在V2N（上界）里面，则将x替换为B
  // - 如果x节点在V2NContra（下界）里面，则将X替换为A。

  auto MatchCritiria = [](CGNode *N) {
    if (N->outEdges.size() == 1 && N->inEdges.size() == 1) {
      auto &O = *N->outEdges.begin();
      auto I = *N->inEdges.begin();
      if (O.getLabel().isOne() && I->getLabel().isOne()) {
        return true;
      }
    }
    return false;
  };

  auto DoSimplify = [&](CGNode *N) {
    assert(N != nullptr);
    auto &O = *N->outEdges.begin();
    auto I = *N->inEdges.begin();
    // 优化该节点：1 处理V2N，V2NContra。
    if (V2N.count(N)) {
      V2N.merge(N, const_cast<CGNode *>(&O.getTargetNode()));
    }
    if (V2NContra.count(N)) {
      V2NContra.merge(N, &I->getSourceNode());
    }
    // 2. 直接连接为A->B。
    CG.addEdge(I->getSourceNode(), const_cast<CGNode &>(O.getTargetNode()),
               {retypd::One{}});
    // 3. 移除该节点和相关的两条边
    CG.removeEdge(const_cast<CGNode &>(O.getSourceNode()),
                  const_cast<CGNode &>(O.getTargetNode()), O.getLabel());
    CG.removeEdge(I->getSourceNode(), I->getTargetNode(), I->getLabel());
    CG.removeNode(*N);
  };

  // 1 收集所有可能的节点，主要关注Covariant的
  std::vector<CGNode *> PossibleNodes;
  for (auto &N : CG) {
    if (N.isStartOrEnd() || N.isMemory()) {
      continue;
    }
    if (N.getVariance() == retypd::Contravariant) {
      continue;
    }
    if (MatchCritiria(&N)) {
      PossibleNodes.push_back(&N);
    }
  }

  auto SimplifyCount = 0;
  for (auto N : PossibleNodes) {
    // 同时找到对称的节点
    auto &NC = CG.getReverseVariant(*N);
    // 0 如果节点在PNGraph里，即可能之后被增加额外的边，不能被删
    if (CG.PG->NodeToCons.count(N)) {
      continue;
    }
    if (CG.PG->NodeToCons.count(&NC)) {
      continue;
    }
    // 再次检查是否符合条件
    if (MatchCritiria(N)) {
      assert(MatchCritiria(&NC));
      DoSimplify(N);
      DoSimplify(&NC);
      SimplifyCount += 2;
    }
  }

  if (SimplifyCount > 0) {
    llvm::errs() << "preSimplify: linear subtype eliminated "
                 << std::to_string(SimplifyCount) << " Nodes!\n";
  }
}

bool isFuncPtr(ExtValuePtr Val) {
  if (auto Ty = getType(Val)) {
    if (Ty->isPointerTy() && Ty->getPointerElementType()->isFunctionTy()) {
      return true;
    }
  }
  return false;
}

bool isSPGlobal(ExtValuePtr Val) {
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    if (auto GV = dyn_cast<GlobalVariable>(*V)) {
      return llvm2c::isSPByMetadata(GV);
    }
  }
  return false;
}

void TypeRecovery::bottomUpPhase() {
  assert(AG.CG != nullptr);
  // TODO: simplify call graph if one func does not have up constraints.
  std::vector<SCCData> &AllSCCs = AG.AllSCCs;

  // 1 Bottom-up Phase: build the summary
  // Walk the callgraph in bottom-up SCC order.
  for (size_t SCCIndex = 0; SCCIndex < AllSCCs.size(); ++SCCIndex) {
    SCCData &Data = AllSCCs.at(SCCIndex);
    const std::set<llvm::Function *> &SCCSet = Data.SCCSet;
    if (SCCSet.empty()) {
      continue;
    }

    if (SCCIndex == (AllSCCs.size() - 1)) {
      // no need to generate summary for last node in SCC
      continue;
    }

    // Print for debug dir
    std::optional<std::string> DirPath = getSCCDebugDir(SCCIndex);
    llvm::Optional<llvm::raw_fd_ostream> SCCsPerf;
    if (DirPath) {
      std::error_code EC;
      SCCsPerf.emplace(join(*DirPath, "Perf.txt"), EC); // , sys::fs::OF_Append
      if (EC) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": "
                  << "Cannot open output file Perf.txt." << std::endl;
        std::cerr << EC.message() << std::endl;
        std::abort();
      }
    }

    auto Name = Data.SCCName;
    std::cerr << "(Bottom-Up) Processing SCC: " << Name << "\n";
    std::shared_ptr<ConstraintsGenerator> Generator;

    auto Start1 = std::chrono::steady_clock::now();

    Generator = getBottomUpGraph(Data, DirPath);
    // 1.3 solve more subtype relations
    Generator->CG.solve();

    if (DirPath) {
      auto SatOut =
          getUniquePath(join(*DirPath, "01-InstantiateSummary"), ".sat.dot");
      Generator->CG.printGraph(SatOut.c_str());
    }

    // 1.4 generate summary
    std::shared_ptr<ConstraintsGenerator> Summary;
    bool isDeclaration =
        SCCSet.size() == 1 && (*SCCSet.begin())->isDeclaration();
    if (SummaryOverride.count(SCCSet)) {
      // summary overriden
      std::cerr << "Summary Overriden: " << Name << ":\n";
      Summary = SummaryOverride.at(SCCSet);
    } else if (isDeclaration) {
      Summary = Generator;
    } else {
      //!! normal case, generate summary
      std::cerr << "Generating Summary for " << Name << "\n";
      Summary = Generator->genSummary(DirPath);
    }

    if (SCCsPerf) {
      *SCCsPerf << "01 SummaryGen Elapsed: " << since(Start1).count()
                << " ms\n";
      SCCsPerf->close();
    }

    // 1.5 save the summary
    for (auto F : SCCSet) {
      auto It2 = FuncSummaries.emplace(F, Summary);
      assert(It2.second && "Function summary already exist?");
    }

    Data.SCCName = Name;
    // print summary
    if (DirPath) {
      auto SummaryOut = getUniquePath(join(*DirPath, "02-Summary"), ".dot");
      if (Summary != nullptr) {
        Summary->CG.printGraph(SummaryOut.c_str());
      } else {
        // write "no summary" to file
        std::ofstream NoSummary(SummaryOut);
        NoSummary << "digraph G {\n"
                     "  label=\"No Summary\"\n"
                     "}\n";
      }
    }
  }
}

std::shared_ptr<ConstraintsGenerator>
TypeRecovery::getBottomUpGraph(SCCData &Data,
                               std::optional<std::string> SCCDebugPath) {
  if (Data.BottomUpGenerator) {
    return Data.BottomUpGenerator;
  }

  std::shared_ptr<ConstraintsGenerator> &Generator = Data.BottomUpGenerator;
  // 1.1 Check for Summary override.
  // for external function(isDeclaration), Use the summary as graph.
  // for non-external but summary overriden, still build the graph but
  // override summary. nomal: build the graph and summary.
  auto &SCCSet = Data.SCCSet;
  bool isExternalFunc =
      SCCSet.size() == 1 && (*SCCSet.begin())->isDeclaration();
  bool isOverride = false;
  if (isExternalFunc && SummaryOverride.count(SCCSet)) {
    std::cerr << "Override summary for external function: " << Data.SCCName
              << ":\n";
    Generator = SummaryOverride.at(SCCSet);
    Generator->checkSymmetry();
    isOverride = true;
  } else if (isExternalFunc) {
    static std::set<std::string> Dedup;
    if (!Dedup.count(Data.SCCName)) {
      Dedup.emplace(Data.SCCName);
      llvm::errs() << "Warning: Summary and result may be incorrect due to "
                      "external function: "
                   << Data.SCCName << "\n";
    }
    // empty graph
    Generator = std::make_shared<ConstraintsGenerator>(*this, Data.SCCName,
                                                       Data.SCCSet);
    Generator->run();
  } else {
    //!! normal case, create the initial constraint graph
    Generator = std::make_shared<ConstraintsGenerator>(*this, Data.SCCName,
                                                       Data.SCCSet);
    Data.BottomUpGenerator = Generator;
    Generator->run();
  }

  assert(Generator != nullptr);

  if (SCCDebugPath) {
    Generator->CG.printGraph(
        getUniquePath(
            join(*SCCDebugPath, isOverride ? "00-Override" : "00-Generated"),
            ".dot")
            .c_str());
  }

  // 1.2 instantiate the summaries for each call.
  bool DisableInterFunc = isDisableInterFunction();
  for (auto &Ent : Generator->CallToInstance) {
    auto *Call = Ent.first;
    auto *Target = Call->getCalledFunction();
    std::shared_ptr<ConstraintsGenerator> TargetSummary;

    if (CallsiteSummaryOverride.count(Call)) {
      llvm::errs() << "Override summary for callsite: " << *Call << "\n";
      TargetSummary = CallsiteSummaryOverride.at(Call);
    } else if (SummaryOverride.count({Target})) {
      llvm::errs() << "Override summary for external function call: " << *Call
                   << "\n";
      TargetSummary = SummaryOverride.at({Target});
      assert(TargetSummary->CG.PG->Constraints.size() == 0);
    } else if (DisableInterFunc) {
      // inter function type recovery disabled !
      // only use if there is summary.
      if (FuncSummaries.count(Target)) {
        TargetSummary = FuncSummaries.at(Target);
      } else {
        Generator->UnhandledCalls.insert(Ent);
      }
    } else if (!Target->isDeclaration()) {
      // should have summary.
      TargetSummary = FuncSummaries.at(Target);
    } else {
      // llvm::errs() << "Warning: Summary and result may be incorrect due
      // to external call: " << *Call << "\n";
      Generator->UnhandledCalls.insert(Ent);
      continue;
    }

    if (TargetSummary != nullptr) {
      Generator->instantiateSummary(Call, Target, *TargetSummary);
    }
  }

  if (SCCDebugPath) {
    Generator->CG.printGraph(
        getUniquePath(join(*SCCDebugPath, "01-InstantiateSummary"), ".dot")
            .c_str());
  }

  return Data.BottomUpGenerator;
}

void TypeRecovery::topDownPhase() {
  assert(AG.CG != nullptr);

  // 2 Top-down Phase: build the result(Map from value to clang C type)
  // We have a big global type graph, corresponds to C AST that link the
  // declared struct type to the real definition to form a graph.
  auto &FuncCallers = AG.FuncCallers;
  auto &AllSCCs = AG.AllSCCs;
  for (std::size_t Index1 = AllSCCs.size(); Index1 > 0; --Index1) {
    std::size_t SCCIndex = Index1 - 1;
    auto &Data = AllSCCs[SCCIndex];
    std::string SCCDebugFolderName = "SCC" + std::to_string(SCCIndex);
    const std::vector<CallGraphNode *> &NodeVec = Data.Nodes;
    auto &Name = Data.SCCName;
    // Collect all functions for SCC checking
    // const std::set<llvm::Function *> &SCCSet = Data.SCCSet;

    // for debug print
    std::optional<std::string> DirPath = getSCCDebugDir(SCCIndex);
    llvm::Optional<llvm::raw_fd_ostream> SCCsPerf;
    if (DirPath) {
      std::error_code EC;
      SCCsPerf.emplace(join(*DirPath, "Perf.txt"), EC, sys::fs::OF_Append);
      if (EC) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": "
                  << "Cannot open output file " << *DirPath << "/Perf.txt."
                  << std::endl;
        std::cerr << EC.message() << std::endl;
        std::abort();
      }
    }

    std::cerr << "(Top-Down) Processing Func: " << Name << "\n";
    auto Start2 = std::chrono::steady_clock::now();

    // 2.1 Calc SignatureTypes
    SCCSignatureTypes &SigTy = Data.SigTy;
    SigTy.SignatureGenerator =
        std::make_shared<ConstraintsGenerator>(*this, Data.SCCName + "-sig");
    auto &SigTypes = *(SigTy.SignatureGenerator);
    assert(SigTypes.PG);

    // 2.1 Gen SigTy: collect and merge actual params
    for (CallGraphNode *CGN : NodeVec) {
      auto *Current = CGN->getFunction();
      if (Current == nullptr) {
        continue;
      }

      std::set<CGNode *> FuncNodes;
      std::set<CGNode *> FuncNodesContra;

      if (SignatureOverride.count(Current)) {
        std::cerr << "Function Signature Overriden: " << Name << "\n";
        auto &SigGen = SignatureOverride.at(Current);
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
        // find all caller's function instance, then merge to get the final
        // signature
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
          auto CallerInd = AG.Func2SCCIndex.at(Elem.second);
          auto &CallerData = AllSCCs.at(CallerInd);
          auto &CallerGenerator = CallerData.TopDownGenerator;
          if (CallerGenerator && CallerGenerator->CallToInstance.count(Call)) {
            auto [FN, FNC] = CallerGenerator->CallToInstance.at(Call);

            FuncNodes.insert(FN);
            FuncNodesContra.insert(FNC);
          } else {
            assert(Data.SCCSet.count(Elem.second->getFunction()));
          }
        }
      }

      CGNode *D = nullptr;
      CGNode *DC = nullptr;
      if (FuncNodes.size() > 0) {
        D = multiGraphDeterminizeTo(SigTypes, FuncNodes, "act_");
        SigTy.FuncNodeMap.insert({Current, D});
      }
      // for symmetric
      if (FuncNodesContra.size() > 0) {
        DC = multiGraphDeterminizeTo(SigTypes, FuncNodesContra, "actc_");
      }
      if (D && DC) {
        SigTypes.CG.RevVariance.insert({D, DC});
        SigTypes.CG.RevVariance.insert({DC, D});
      }
    }

    if (DirPath) {
      if (SigTy.SignatureGenerator) {
        auto SigOut = getUniquePath(join(*DirPath, "03-Signature"), ".dot");
        SigTy.SignatureGenerator->CG.printGraph(SigOut.c_str());
      }
    }

    auto TDG = getTopDownGraph(Data, DirPath);

    if (SCCsPerf) {
      *SCCsPerf << "02 TopDown Elapsed: " << since(Start2).count() << " ms\n";
      SCCsPerf->close();
    }
  }
}

std::shared_ptr<ConstraintsGenerator>
TypeRecovery::getTopDownGraph(SCCData &Data,
                              std::optional<std::string> SCCDebugPath) {
  if (Data.TopDownGenerator) {
    return Data.TopDownGenerator;
  }
  const std::shared_ptr<ConstraintsGenerator> &Generator =
      getBottomUpGraph(Data, SCCDebugPath);

  Data.TopDownGenerator =
      std::make_shared<ConstraintsGenerator>(*this, Data.SCCName);
  auto &CurrentTypes = *(Data.TopDownGenerator);
  assert(CurrentTypes.PG);

  std::map<const CGNode *, CGNode *> Old2New;
  if (Generator) {
    Generator->cloneTo(CurrentTypes, Old2New);
  }

  bool DisableInterFunc = isDisableInterFunction();
  if (!DisableInterFunc) {
    assert(Data.SigTy.SignatureGenerator);
    // enable inter function type recovery
    Data.SigTy.instantiate(CurrentTypes);
  }

  if (SCCDebugPath) {
    CurrentTypes.CG.printGraph(
        getUniquePath(join(*SCCDebugPath, "04-TopDown"), ".dot").c_str());
  }

  // 2.3 solve again and fixups
  // ensure lower bound is lower than upper bound
  // CurrentTypes.makeSymmetry();
  // CurrentTypes.linkContraToCovariant();
  CurrentTypes.CG.solve();
  CurrentTypes.CG.linkConstantPtr2Memory();
  // link primitives for all Graphs in TopDownGenerator.
  // 1. later postProcess will also do this.
  // 2. our determinization for actual params need this.
  CurrentTypes.CG.linkPrimitives();

  if (SCCDebugPath) {
    CurrentTypes.CG.printGraph(
        getUniquePath(join(*SCCDebugPath, "04-TopDown.sat"), ".dot").c_str());
  }

  return Data.TopDownGenerator;
}

void SCCSignatureTypes::instantiate(ConstraintsGenerator &To) {
  std::map<const CGNode *, CGNode *> Old2New;
  SignatureGenerator->cloneTo(To, Old2New);
  for (auto Ent : FuncNodeMap) {
    Function *Current = Ent.first;
    To.addSubtype(To.getNode(Current, nullptr, -1, retypd::Covariant),
                  *Old2New.at(Ent.second));
  }
}

void TypeRecovery::prepareSCC(CallGraph &CG) {
  AG.CG = &CG;

  all_scc_iterator<CallGraph *> CGI = notdec::scc_begin(AG.CG);
  SingleNodeSCCIterator SNI(notdec::scc_begin(AG.CG));

  std::vector<SCCData> &AllSCCs = AG.AllSCCs;
  std::map<CallGraphNode *, std::size_t> &Func2SCCIndex = AG.Func2SCCIndex;

  auto PolyFuncFiles = std::getenv("NOTDEC_POLY_FUNCS");
  std::set<std::string> PolyFuncs;
  if (PolyFuncFiles) {
    auto Content = readFileToString(PolyFuncFiles);
    auto ValE = json::parse(Content);
    if (!ValE) {
      assert(false && "JSON parse failed, invalid NOTDEC_POLY_FUNCS content");
    }
    auto ValArr = ValE->getAsArray();
    assert(ValArr != nullptr);
    for (auto S : *ValArr) {
      PolyFuncs.insert(S.getAsString()->str());
    }
  }

  // TODO assign a level to each SCC node by walking the call tree.
  // For polymorphic and non polymorphic funcs, we can only merge consecutive
  // non-poly funcs. maintain a bool var PrevNotPolymorphic, if prev SCC is
  // poly, we cannot merge.
  bool HasPolymorphic = false;
  bool PrevPolymorphic = true;
  bool NoSCC = this->NoSCC;
  bool DisableInterFunc = isDisableInterFunction();
  if (DisableInterFunc) {
    NoSCC = true;
  }
  // 1. Split by SCC post order
  for (; (NoSCC ? !SNI.isAtEnd() : !CGI.isAtEnd()); (NoSCC ? ++SNI : ++CGI)) {
    const std::vector<CallGraphNode *> &NodeVec = (NoSCC ? *SNI : *CGI);
    PrevPolymorphic = HasPolymorphic;
    HasPolymorphic = false;
    bool AllDeclaration = true;
    bool AllIntrinsics = true;

    for (auto *CGN : NodeVec) {
      if (auto *Fn = CGN->getFunction()) {
        if (!Fn->isDeclaration()) {
          AllDeclaration = false;
        }
        if (!Fn->isIntrinsic()) {
          AllIntrinsics = false;
        }
        auto FName = Fn->getName();
        if (Fn->hasName() && PolyFuncs.count(FName.str())) {
          HasPolymorphic = true;
        }
        if (isPolymorphic(Fn)) {
          HasPolymorphic = true;
          break;
        }
      }
    }

    if (AllDeclaration && AllIntrinsics) {
      continue;
    }

    if (!AllSCCs.empty() && !DisableInterFunc && !HasPolymorphic &&
        !AllDeclaration && !PrevPolymorphic) {
      auto &PrevNodes = AllSCCs.back().Nodes;
      PrevNodes.insert(PrevNodes.end(), NodeVec.begin(), NodeVec.end());
    } else {
      AllSCCs.push_back(SCCData{.Nodes = NodeVec});
    }
  }
  // assert(HasPolymorphic == false && "Last SCC cannot be polymorphic!");

  Func2SCCIndex.clear();
  // 2. Calc name and SCCSet
  for (size_t SCCIndex = 0; SCCIndex < AllSCCs.size(); ++SCCIndex) {
    SCCData &Data = AllSCCs[SCCIndex];
    std::set<llvm::Function *> &SCCSet = Data.SCCSet;
    SCCSet.clear();

    std::string Name;
    for (auto *CGN : Data.Nodes) {
      auto *Fn = CGN->getFunction();
      if (Fn == nullptr) {
        continue;
      }
      if (!Name.empty()) {
        Name += ",";
      }
      Name += Fn->getName().str();
      SCCSet.insert(Fn);
      Func2SCCIndex[CGN] = SCCIndex;
    }
    if (!Name.empty()) {
      Data.SCCName = Name;
    }
    // write the SCC index to file
    if (SCCsCatalog) {
      *SCCsCatalog << "SCC" << SCCIndex << "," << Name << "\n";
    }
  }

  // 3. calc reverse call edge map
  // map from function to all its callers
  std::map<CallGraphNode *,
           std::vector<std::pair<llvm::CallBase *, CallGraphNode *>>>
      &FuncCallers = AG.FuncCallers;
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
}

void TypeRecovery::run(Module &M1, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== TypeRecovery::run  ===============\n");

  bool DisableInterFunction = isDisableInterFunction();

  assert(&M1 == &Mod);
  auto &M = const_cast<Module &>(Mod);
  auto SP = MAM.getResult<StackPointerFinderAnalysis>(M);
  this->StackPointer = SP.result;

  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();

  MemoryBytes = BytesManager::create(M);

  // 0 Preparation
  // 0.1 load summary file
  if (SummaryFile) {
    loadSummaryFile(M, SummaryFile);
  }

  // 0.2 load signature file
  if (SigFile) {
    loadSignatureFile(M, SigFile);
  }

  // 0.3 load trace ids
  if (Traces) {
    loadTraceStr(Traces);
  }

  // 0.4 prepare debug dir and SCCsCatalog
  auto DebugDir = getTRDebugDir();
  if (getTRDebugDir()) {
    std::error_code EC = llvm::sys::fs::create_directories(DebugDir);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open create directory " << DebugDir << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    SCCsCatalog.emplace(join(DebugDir, "SCCs.txt"), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file SCCs.txt: ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
  }

  // 0.5 print module for debugging
  if (DebugDir) {
    printModule(M, join(DebugDir, "01-Optimized.ll").c_str());
  }

  CallGraphAnalysis Ana;
  CallG = std::make_unique<CallGraph>(Ana.run(M, MAM));

  if (DebugDir) {
    std::error_code EC;
    auto Path = join(DebugDir, "CallGraph.txt");
    llvm::raw_fd_ostream CGTxt(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    CallG->print(CGTxt);
    CGTxt.close();
    // print dot
    Path = join(DebugDir, "CallGraph.dot");
    llvm::raw_fd_ostream CGDot(Path, EC);
    if (EC) {
      llvm::errs() << "Error printing to " << Path << ", " << EC.message()
                   << "\n";
    }
    notdec::utils::CallGraphDOTInfo CFGInfo(&M, &*CallG, nullptr);
    llvm::WriteGraph(CGDot, &CFGInfo, false);
    CGDot.close();
  }

  prepareSCC(*CallG);

  if (DebugDir) {
    SCCsCatalog->close();
  }

  if (DisableInterFunction) {
    return;
  }

  bottomUpPhase();

  if (DebugDir) {
    printModule(M, join(DebugDir, "02-AfterBottomUp.ll").c_str());
  }

  topDownPhase();

  std::cerr << "Bottom up phase done! SCC count:" << AG.AllSCCs.size() << "\n";

  if (DebugDir) {
    printAnnotatedModule(M, join(DebugDir, "02-AfterBottomUp.anno1.ll").c_str(),
                         1);
    // std::error_code EC;
    // ValueTypesFile.emplace(join(DebugDir, "ValueTypes.txt"), EC);
    // if (EC) {
    //   std::cerr << __FILE__ << ":" << __LINE__ << ": "
    //             << "Cannot open output file ValueTypes.txt." << std::endl;
    //   std::cerr << EC.message() << std::endl;
    //   std::abort();
    // }
  }

  LLVM_DEBUG(
      errs() << " ============== TypeRecovery::run End ===============\n");
}

std::shared_ptr<ConstraintsGenerator>
TypeRecovery::getGlobalGraph(std::optional<std::string> DebugDir) {
  if (AG.Global) {
    return AG.Global;
  }

  // 2.4 solve the global memory node
  // Create type for memory node
  std::shared_ptr<ConstraintsGenerator> &Global = AG.Global;
  Global = std::make_shared<ConstraintsGenerator>(*this, "Global");
  std::set<CGNode *> MemoryNodes;
  // std::set<CGNode *> MemoryNodesC;
  auto &AllSCCs = AG.AllSCCs;
  for (auto &Ent : AllSCCs) {
    if (!Ent.TopDownGenerator)
      continue;
    auto &G = *(Ent.TopDownGenerator);
    if (auto *M = G.CG.getMemoryNodeOrNull(retypd::Covariant)) {
      if (M->outEdges.empty()) {
        continue;
      }
      MemoryNodes.insert(M);
    }
    // if (auto *M = G.CG.getMemoryNodeOrNull(retypd::Contravariant)) {
    //   if (M->outEdges.empty()) {
    //     continue;
    //   }
    //   MemoryNodesC.insert(M);
    // }
  }
  CGNode *MemNode = nullptr;
  if (MemoryNodes.empty()) {
    llvm::errs() << "No memory node found\n";
    MemNode = Global->CG.getMemoryNode(retypd::Covariant);
  } else {
    MemNode = multiGraphDeterminizeTo(*Global, MemoryNodes, "mdtm");
    // Global->mergeAfterDeterminize();
  }
  // CGNode *MemNodeC = nullptr;
  // if (MemoryNodesC.empty()) {
  //   llvm::errs() << "No memory node found\n";
  //   MemNodeC = Global->CG.getMemoryNode(retypd::Contravariant);
  // } else {
  //   MemNodeC = multiGraphDeterminizeTo(*Global, MemoryNodesC, "mdtmc");
  //   // Global->mergeAfterDeterminize();
  // }
  // // a special value to represent the memory node
  // MemNode->getPNIVar()->unify(*MemNodeC->getPNIVar());
  Global->CG.addEdge(*Global->CG.getMemoryNode(retypd::Covariant), *MemNode,
                     {retypd::One{}});
  // Global->CG.onlyAddEdge(*MemNodeC,
  //                        *Global->CG.getMemoryNode(retypd::Contravariant),
  //                        {retypd::One{}});
  // keep node in val2node map for post process
  // Global->makeSymmetry();
  Global->CG.solve();

  if (DebugDir) {
    AG.Global->CG.printGraph(
        getUniquePath(join(*DebugDir, "Global"), ".dot").c_str());
  }
  return AG.Global;
}

std::shared_ptr<ConstraintsGenerator>
TypeRecovery::getGlobalSketchGraph(std::optional<std::string> DebugDir) {
  if (AG.GlobalSketch) {
    return AG.GlobalSketch;
  }
  std::shared_ptr<ConstraintsGenerator> Global = getGlobalGraph(DebugDir);
  std::shared_ptr<ConstraintsGenerator> &GlobalSkS = AG.GlobalSketch;
  GlobalSkS = postProcess(*AG.Global);

  if (DebugDir) {
    AG.GlobalSketch->CG.printGraph(
        getUniquePath(join(*DebugDir, "GlobalSketch"), ".dot").c_str());
  }
  return AG.GlobalSketch;
}

void TypeRecovery::genASTTypes(Module &M) {
  // 3 build AST type for each value in value map
  ResultVal = std::make_unique<TypeRecovery::Result>();
  assert(HTCtx != nullptr);
  using notdec::ast::HType;
  using notdec::ast::HTypeContext;

  auto &AllSCCs = AG.AllSCCs;
  for (int i = 0; i < AllSCCs.size(); i++) {
    auto &Data = AllSCCs[i];

    auto Dir = getSCCDebugDir(i);
    auto &G = *getTopDownGraph(Data, Dir);
    auto SCCName = Data.SCCName;
    assert(G.PG);
    for (auto &N : G.CG) {
      if (N.isStartOrEnd() || N.isMemory()) {
        continue;
      }
    }

    // do getASTTypes for each SCC
    auto SCCTypes = getASTTypes(Data, Dir);

    // put the result into ResultVal
    ConstraintsGenerator &G2 = *Data.SketchGenerator;

    for (auto &Ent : G2.V2N) {
      // TODO support function type.
      if (std::holds_alternative<llvm::Value *>(Ent.first)) {
        if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
          continue;
        }
      }
      auto *Node = Ent.second;
      if (Node != nullptr) {
        assert(&Node->Parent == &G2.CG &&
               "RetypdGenerator::getTypeVar: Node is not in the graph");
        // merge AST type
        if (SCCTypes->ValueTypes.count(Ent.first)) {
          HType *CTy = SCCTypes->ValueTypes.at(Ent.first);
          ResultVal->ValueTypes[Ent.first] = CTy;
        }
      }
    }

    for (auto &Ent : G2.V2NContra) {
      // TODO support function type.
      if (std::holds_alternative<llvm::Value *>(Ent.first)) {
        if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
          continue;
        }
      }
      auto *Node = Ent.second;
      if (Node != nullptr) {
        assert(&Node->Parent == &G2.CG &&
               "RetypdGenerator::getTypeVar: Node is not in the graph");
        //!! build AST type for the node
        if (SCCTypes->ValueTypesLowerBound.count(Ent.first)) {
          HType *CTy = SCCTypes->ValueTypesLowerBound.at(Ent.first);
          ResultVal->ValueTypesLowerBound[Ent.first] = CTy;
        }
      }
    }
  }

  // 3.4 build AST type for memory node
  std::optional<std::string> DebugDir;
  if (auto D = getTRDebugDir()) {
    DebugDir.emplace(D);
  }
  std::shared_ptr<ConstraintsGenerator> GlobalSkS =
      getGlobalSketchGraph(DebugDir);

  if (DebugDir) {
    printAnnotatedModule(
        M,
        getUniquePath(join(*DebugDir, "02-AfterBottomUp.anno1"), ".ll").c_str(),
        1);
  }

  ConstraintsGenerator &GlobalSk = *GlobalSkS;
  // CGNode &MemNode2 = Global2.getNode(ConstantAddr(), nullptr, -1,
  // retypd::Covariant);
  CGNode *MemNode2 = GlobalSk.CG.getMemoryNode(retypd::Covariant);

  retypd::TypeBuilderContext TBC(*HTCtx, Mod.getName(), Mod.getDataLayout());
  retypd::TypeBuilder TBG(TBC, GlobalSk.TypeInfos);

  // build AST type for memory node
  HType *CTy = TBG.buildType(*MemNode2, retypd::Covariant);
  llvm::errs() << "Memory Type: " << CTy->getAsString() << "\n";

  using notdec::ast::RecordDecl;
  using notdec::ast::RecordType;
  RecordDecl *Mem = nullptr;
  // if Memory type is not void
  if (auto MTy = CTy->getPointeeType()) {
    if (auto RD = CTy->getPointeeType()->getAs<RecordType>()) {
      Mem = RD->getDecl();
      Mem->setBytesManager(MemoryBytes);
    } else {
      llvm::errs() << "ERROR: Memory Type is not struct type: "
                   << CTy->getAsString() << "\n";
      std::abort();
    }
    // auto &Info = TBC.StructInfos[Mem];
    // Info.Bytes = MemoryBytes;
    // Info.resolveInitialValue();
  } else {
    llvm::errs() << "ERROR: Memory Type is void!" << CTy->getAsString() << "\n";
    // std::abort();
  }

  // analyze the signed/unsigned info
  // Framework:
  // 1.
  // 我可能需要增加跨图，跨函数的PNI节点的关系。一边传入所有的sketches图，一边传入Result。然后首先处理所有的有primitive类型的节点，给PNI打标签。然后根据标签修改类型。
  // analyzeSignedness(AG, Mem);

  // 4 Save the result
  ResultVal->MemoryType = CTy->getPointeeType();
  ResultVal->MemoryDecl = Mem;

  // move the ASTUnit to result
  ResultVal->HTCtx = HTCtx;

  // gen_json("retypd-constrains.json");

  if (DebugDir) {
    printAnnotatedModule(Mod, join(*DebugDir, "03-Final.anno2.ll").c_str(), 2);
  }
}

std::optional<int64_t> getAllocSize(ExtValuePtr Val) {
  auto *V = std::get_if<llvm::Value *>(&Val);
  if (V == nullptr) {
    return std::nullopt;
  }
  if (auto Inst = dyn_cast<Instruction>(*V)) {
    // 检查指令是否包含目标元数据
    std::optional<uint64_t> SizeRet;
    if (llvm::MDNode *MD = Inst->getMetadata(KIND_ALLOC_SIZE)) {
      // 验证元数据格式：应该包含一个MDString元素
      if (MD->getNumOperands() > 0) {
        if (auto *SizeMD = llvm::dyn_cast<llvm::MDString>(MD->getOperand(0))) {
          // 尝试将字符串转换为整数
          llvm::StringRef SizeStr = SizeMD->getString();
          uint64_t Size = 0;
          if (!SizeStr.getAsInteger(10, Size)) {
            SizeRet = Size;
          }
        }
      }
    }
    if (SizeRet) {
      assert(*SizeRet < (uint64_t)std::numeric_limits<int64_t>::max());
      if (isGrowNegative(Inst)) {
        return -*SizeRet;
      } else {
        return *SizeRet;
      }
    }
  }

  return std::nullopt;
}

std::shared_ptr<ConstraintsGenerator>
TypeRecovery::getSketchGraph(SCCData &Data,
                             std::optional<std::string> DebugDir) {
  //!! 3.1 post process the graph for type generation
  if (Data.SketchGenerator) {
    return Data.SketchGenerator;
  }

  const std::shared_ptr<ConstraintsGenerator> &G =
      getTopDownGraph(Data, DebugDir);
  Data.SketchGenerator = postProcess(*G, DebugDir);
  return Data.SketchGenerator;
}

// Deprecated
std::shared_ptr<SCCTypeResult>
TypeRecovery::getASTTypes(SCCData &Data, std::optional<std::string> DebugDir) {
  if (Data.TypeResult) {
    return Data.TypeResult;
  }
  Data.TypeResult = std::make_shared<SCCTypeResult>();

  auto SCCTypes = Data.TypeResult;

  //!! 3.1 post process the graph for type generation
  auto SkG = getSketchGraph(Data, DebugDir);
  ConstraintsGenerator &G2 = *SkG;

  retypd::TypeBuilderContext TBC(*HTCtx, Mod.getName(), Mod.getDataLayout());
  retypd::TypeBuilder TB(TBC, G2.TypeInfos);

  using notdec::ast::HType;
  using notdec::ast::HTypeContext;

  llvm::Optional<llvm::raw_fd_ostream> LocalValueTypesFile;
  // 3.2 print the graph for debugging
  if (DebugDir) {
    G2.CG.printGraph(
        getUniquePath(join(*DebugDir, "10-PostProcess.dtm"), ".dot").c_str());
    std::error_code EC;
    auto VTFP = getUniquePath(join(*DebugDir, "10-ValueTypes"), ".txt");
    LocalValueTypesFile.emplace(VTFP, EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file " << VTFP << "." << std::endl;
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    // TODO refactor to annotated function
    // print val2node here
    std::ofstream Val2NodeFile(
        getUniquePath(join(*DebugDir, "10-Val2Node"), ".txt"));
    for (auto &Ent : G2.V2N) {
      auto N = G2.getNodeOrNull(Ent.first, nullptr, -1, retypd::Covariant);
      auto NC = G2.getNodeOrNull(Ent.first, nullptr, -1, retypd::Contravariant);
      Val2NodeFile << getName(Ent.first) << ", "
                   << (N ? toString(N->key) : "none") << ", "
                   << (NC ? toString(NC->key) : "none") << "\n";
    }
    Val2NodeFile.close();
  }

  if (LocalValueTypesFile) {
    *LocalValueTypesFile << "UpperBounds:\n";
  }

  // 3.3 build type for each value
  for (auto &Ent : G2.V2N) {
    // TODO support function type.
    if (std::holds_alternative<llvm::Value *>(Ent.first)) {
      if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
        continue;
      }
    }
    auto *Node = Ent.second;
    if (Node != nullptr) {
      assert(&Node->Parent == &G2.CG &&
             "RetypdGenerator::getTypeVar: Node is not in the graph");
      auto Size = getSize(Ent.first, pointer_size);
      assert(Size > 0);

      if (LocalValueTypesFile) {
        // print the current value
        *LocalValueTypesFile << "  " << toString(Ent.first, true);
        *LocalValueTypesFile << "  Node: " << toString(Node->key);
        if (Node->getPNIVar() != nullptr) {
          *LocalValueTypesFile << "  PNI: " << Node->getPNIVar()->str();
        }
        LocalValueTypesFile->flush();
      }

      if (TraceIds.count(Node->getId())) {
        PRINT_TRACE(Node->getId())
            << "Generating Type for " << toString(Node->key) << "...\n";
      }

      //!! build AST type for the node
      HType *CTy = TB.buildType(*Node, retypd::Covariant);

      if (TraceIds.count(Node->getId())) {
        PRINT_TRACE(Node->getId()) << "Type is " << CTy->getAsString() << "\n";
      }

      if (SCCTypes->ValueTypes.count(Ent.first) != 0) {
        llvm::errs() << "Warning: TODO handle Value type merge (LowerBound): "
                     << toString(Ent.first) << "\n";
      }

      // save the result
      if (!isFuncPtr(Ent.first) && !isSPGlobal(Ent.first)) {
        SCCTypes->ValueTypes[Ent.first] = CTy;
      }

      if (LocalValueTypesFile) {
        *LocalValueTypesFile << " upper bound: " << CTy->getAsString();
      }

    } else {
      if (LocalValueTypesFile) {
        *LocalValueTypesFile << " has no upper bound";
      }
    }
    if (LocalValueTypesFile) {
      *LocalValueTypesFile << "\n";
    }
  }

  if (LocalValueTypesFile) {
    *LocalValueTypesFile << "LowerBounds:\n";
  }

  for (auto &Ent : G2.V2NContra) {
    // TODO support function type.
    if (std::holds_alternative<llvm::Value *>(Ent.first)) {
      if (llvm::isa<llvm::Function>(std::get<llvm::Value *>(Ent.first))) {
        continue;
      }
    }
    auto *Node = Ent.second;
    if (Node != nullptr) {
      assert(&Node->Parent == &G2.CG &&
             "RetypdGenerator::getTypeVar: Node is not in the graph");
      auto Size = getSize(Ent.first, pointer_size);
      assert(Size > 0);

      if (LocalValueTypesFile) {
        *LocalValueTypesFile << "  " << toString(Ent.first, true);
        *LocalValueTypesFile << "  Node: " << toString(Node->key);
        if (Node->getPNIVar() != nullptr) {
          *LocalValueTypesFile << "  PNI: " << Node->getPNIVar()->str();
        }
        LocalValueTypesFile->flush();
      }

      if (TraceIds.count(Node->getId())) {
        PRINT_TRACE(Node->getId()) << "Generating Type...\n";
      }

      //!! build AST type for the node
      HType *CTy = TB.buildType(*Node, retypd::Contravariant);

      if (TraceIds.count(Node->getId())) {
        PRINT_TRACE(Node->getId()) << "Type is " << CTy->getAsString() << "\n";
      }

      if (SCCTypes->ValueTypesLowerBound.count(Ent.first) != 0) {
        llvm::errs() << "Warning: TODO handle Value type merge (UpperBound): "
                     << toString(Ent.first) << "\n";
      }
      // save the result
      if (!isFuncPtr(Ent.first) && !isSPGlobal(Ent.first)) {
        SCCTypes->ValueTypesLowerBound[Ent.first] = CTy;
      }

      if (LocalValueTypesFile) {
        *LocalValueTypesFile << " lower bound: " << CTy->getAsString();
      }
    } else {
      if (LocalValueTypesFile) {
        *LocalValueTypesFile << " has no lower bound";
      }
    }
    if (LocalValueTypesFile) {
      *LocalValueTypesFile << "\n";
    }
  }

  return Data.TypeResult;
}

static AllocaInst *findAllocaWithName(llvm::BasicBlock &BB, std::string Name) {
  for (auto &I : BB) {
    if (auto AI = dyn_cast<AllocaInst>(&I)) {
      if (AI->getName() == Name) {
        return AI;
      }
    }
  }
  return nullptr;
}

void RecoverDeadAlloca::recoverAlloca(
    Function &F, std::vector<std::pair<SimpleRange, std::string>> &Vec) {
  auto &Entry = F.getEntryBlock();
  IRBuilder<> Builder(F.getContext());
  Builder.SetInsertPoint(&Entry.front());
  for (auto &Ent : Vec) {
    if (auto AI = findAllocaWithName(Entry, Ent.second)) {
      Builder.SetInsertPoint(AI->getNextNode());
    } else {
      auto &R1 = Ent.first;
      // auto NewAlloca =
      Builder.CreateAlloca(ArrayType::get(Builder.getInt8Ty(), R1.Size),
                           nullptr, Ent.second);
    }
  }
}

PreservedAnalyses RecoverDeadAlloca::run(Module &M,
                                         ModuleAnalysisManager &MAM) {
  if (!TR.hasAllocaRanges()) {
    return PreservedAnalyses::all();
  }
  for (auto &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    recoverAlloca(F, TR.getOrCreateFuncAllocaRange(&F));
  }
  return PreservedAnalyses::all();
}

PreservedAnalyses TypeRecoveryOpt::run(Module &M, ModuleAnalysisManager &MAM) {
  // FunctionAnalysisManager &FAM =
  //     MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  const char *DebugDir = getTRDebugDir();
  std::vector<SCCData> &AllSCCs = TR.AG.AllSCCs;

  for (size_t SCCIndex = 0; SCCIndex < AllSCCs.size(); ++SCCIndex) {
    SCCData &Data = AllSCCs.at(SCCIndex);
    bool SCCChanged = false;

    auto SCCDebugDir = getSCCDebugDir(SCCIndex);
    auto SCCTys = TR.getASTTypes(Data, SCCDebugDir);

    for (auto F : Data.SCCSet) {
      if (F->isDeclaration()) {
        continue;
      }

      AllocaInst *Stack = nullptr;
      BasicBlock &EntryBB = F->getEntryBlock();
      for (BasicBlock::iterator I = EntryBB.begin(),
                                E = std::prev(EntryBB.end());
           I != E; ++I) {
        if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
          if (AI->getName() == "stack") {
            Stack = AI;
          }
        }
      }
      if (Stack == nullptr) {
        continue;
      }

      // 处理entry块中确定大小的alloca指令。
      StackBreaker SB;
      SCCChanged |=
          SB.runOnAlloca(*Stack, *SCCTys, &TR.getOrCreateFuncAllocaRange(F));
    }

    if (SCCChanged) {
      // invalidate all passes
      Data.onIRChanged();
    }
  }

  TR.AG.onIRChanged();

  if (DebugDir) {
    printModule(M, join(DebugDir, "TROpt.ll").c_str());
  }

  auto PA = PreservedAnalyses::none();
  PA.preserve<CallGraphAnalysis>();
  return PA;
}

bool ConstraintsGenerator::checkSymmetry() {
  for (auto &N : CG) {
    if (N.isStartOrEnd()) {
      continue;
    }
    auto &NC = CG.getReverseVariant(N);
    for (auto &Edge : N.outEdges) {
      auto &T = const_cast<CGNode &>(Edge.getTargetNode());
      auto &TC = CG.getReverseVariant(T);
      auto &Label = Edge.getLabel();
      if (Label.isOne()) {
        // ensure there is a edge for the contra node.
        assert(CG.hasEdge(TC, NC, {retypd::One{}}) && "Symmetry check failed");
      } else if (auto RL = Label.getAs<retypd::RecallLabel>()) {
        assert(CG.hasEdge(T, N, {retypd::ForgetLabel{RL->label}}));
        assert(CG.hasEdge(NC, TC, {retypd::RecallLabel{RL->label}}));
        assert(CG.hasEdge(TC, NC, {retypd::ForgetLabel{RL->label}}));
      } else if (auto FL = Label.getAs<retypd::ForgetLabel>()) {
        assert(CG.hasEdge(T, N, {retypd::RecallLabel{FL->label}}));
        assert(CG.hasEdge(NC, TC, {retypd::ForgetLabel{FL->label}}));
        assert(CG.hasEdge(TC, NC, {retypd::RecallLabel{FL->label}}));
      }
    }
  }
  return true;
}

// ConstraintsGenerator
// ConstraintsGenerator::clone(std::map<const CGNode *, CGNode *> &Old2New) {
//   ConstraintsGenerator G(Ctx, CG.Name);
//   cloneTo(G, Old2New);
//   return G;
// }

std::shared_ptr<ConstraintsGenerator>
ConstraintsGenerator::genSketch(std::map<const CGNode *, CGNode *> &Old2New,
                                std::optional<std::string> DebugDir) {
  // 为了提升效率，不要调用clone方法，而是直接：
  // 1. 忽视部分边，仿佛sketch split过。
  // 2. 把store和load看作相同边。
  // 3. 把offset 0 看作子类型边

  // 2. DSU构建，在当前图上构建等价类关系。
  // 子类型关系看作等价关系的合并，以及同一等价类的相同出边合并

  // 使用并查集来管理等价类，但是直接维护每个节点映射到的等价类集合。
  // 直接在堆上分配std::set<CGNode *>对象。
  struct DSUSet {
    std::set<CGNode *> Set;
    CGNode *R = nullptr;

    using iteratorTy = std::list<DSUSet>::iterator;
    iteratorTy getIterator() {
      size_t iterOffset = (size_t)&(*((iteratorTy) nullptr));
      iteratorTy iter;
      *(intptr_t *)&iter = (intptr_t)this - iterOffset;
      return iter;
    }
  };
  std::map<CGNode *, DSUSet *> equivalenceClasses;
  std::list<DSUSet> groups;
  // 无需初始化，如果节点不在parent map中，则默认它映射到自己。

  // 查找根节点的函数
  std::function<CGNode *(CGNode *)> find = [&](CGNode *node) -> CGNode * {
    auto It = equivalenceClasses.find(node);
    if (It == equivalenceClasses.end()) {
      return node;
    } else {
      return It->second->R;
    }
  };

  // 合并两个等价类
  auto unionNodes = [&](CGNode *a, CGNode *b) -> bool {
    DSUSet *setA = nullptr;
    DSUSet *setB = nullptr;

    if (a->getPNIVar()->getSize() != b->getPNIVar()->getSize()) {
      return false;
    }

    // 获取节点a的集合
    auto itA = equivalenceClasses.find(a);
    if (itA == equivalenceClasses.end()) {
      // 节点a不在任何集合中，创建新集合
      setA = &groups.emplace_back();
      setA->Set.insert(a);
      setA->R = a;
      equivalenceClasses.insert({a, setA});
    } else {
      setA = itA->second;
    }

    // 获取节点b的集合
    auto itB = equivalenceClasses.find(b);
    if (itB == equivalenceClasses.end()) {
      // 节点b不在任何集合中，创建新集合
      setB = &groups.emplace_back();
      setB->Set.insert(b);
      setB->R = b;
      equivalenceClasses.insert({b, setB});
    } else {
      setB = itB->second;
    }

    // 如果已经在同一个集合中，直接返回
    if (setA != nullptr && setA == setB)
      return false;

    if (setA->R->getPNIVar()->getSize() != setB->R->getPNIVar()->getSize()) {
      return false;
    }

    // 选择id较小的节点作为新的代表节点
    bool isASmaller = setA->R->getId() < setB->R->getId();
    CGNode *newRep = isASmaller ? setA->R : setB->R;
    // 合并集合：将setB合并到setA中
    DSUSet *targetSet = isASmaller ? setA : setB;
    DSUSet *sourceSet = isASmaller ? setB : setA;

    // 将源集合的所有节点添加到目标集合
    targetSet->Set.insert(sourceSet->Set.begin(), sourceSet->Set.end());

    // 更新所有源集合节点的映射指向目标集合
    for (CGNode *node : sourceSet->Set) {
      equivalenceClasses[node] = targetSet;
    }

    // 更新目标集合的代表节点
    targetSet->R = newRep;

    // 删除源集合（避免内存泄漏）
    groups.erase(sourceSet->getIterator());
    return true;
  };

  // 3 根据DSU关系，构建等价类，然后将所有等价类映射为单个新的图中的节点。
  // 等价类内部的边，子类型的话就去掉，其他的变成自己指向自己。
  // 2. DSU构建，在当前图上构建等价类关系

  // 3. 根据规则合并等价类
  // 规则1: 子类型关系看作等价关系的合并
  for (auto &node : CG) {
    for (auto &edge : node.outEdges) {
      CGNode *source = const_cast<CGNode *>(&edge.getSourceNode());
      CGNode *target = const_cast<CGNode *>(&edge.getTargetNode());
      if (edge.getLabel().isOne()) {
        // 子类型边，合并源节点和目标节点
        unionNodes(source, target);
      } else if (edge.getLabel().isRecallLabel()) { // offset 0 也看作子类型边。
        if (auto Off = retypd::getOffsetLabel(edge.getLabel())) {
          if (Off->range.isZero()) {
            unionNodes(source, target);
          }
        }
      }
    }
  }

  if (DebugDir) {
    std::string SketchMergeFile =
        getUniquePath(join(*DebugDir, "SketchMerge1"), ".txt");
    std::ofstream Out(SketchMergeFile);
    // TODO 打印
    for (auto &Ent : groups) {
      Out << toString(Ent.R->key) << ": ";
      for (auto N : Ent.Set) {
        Out << toString(N->key) << ", ";
      }
      Out << "\n";
    }
    Out.close();
  }

  std::optional<std::ofstream> MergeLog;
  if (DebugDir) {
    std::string File = getUniquePath(join(*DebugDir, "SketchMergeLog"), ".txt");
    MergeLog.emplace(File);
  }

  // 规则2: 同一等价组内，相同出边的节点合并。load边看作store边。
  bool Changed = true;
  while (Changed) {
    Changed = false;
    // TODO
    // 遍历所有等价组，遍历所有离开等价组的RecallLabel边，整理从边label到目标节点的集合
    // 复制当前的groups
    for (auto &group : groups) {
      std::map<retypd::FieldLabel, std::set<CGNode *>> labelToNodes;
      for (auto N : group.Set) {
        for (auto &E : N->outEdges) {
          auto Target = const_cast<CGNode *>(&E.getTargetNode());
          if (group.Set.count(Target)) {
            continue;
          }
          if (auto RL = E.getLabel().getAs<retypd::RecallLabel>()) {
            // normalize store and load.
            auto Label = RL->label;
            if (auto S = Label.getAs<retypd::StoreLabel>()) {
              Label = FieldLabel{retypd::LoadLabel{.Size = S->Size}};
            }
            labelToNodes[Label].insert(Target);
          }
        }
      }
      // 遍历得到的集合，大小大于1的执行合并。
      for (auto Ent : labelToNodes) {
        if (Ent.second.size() > 1) {
          auto It = Ent.second.begin();
          auto Rep = *It;
          It++;
          for (; It != Ent.second.end(); It++) {
            auto Updated = unionNodes(Rep, *It);
            if (Updated) {
              if (MergeLog) {
                *MergeLog << "Merging node " << toString(Rep->key) << " with "
                          << toString((*It)->key) << ": because of Label "
                          << toString(Ent.first) << "\n";
              }
              Changed = true;
            }
          }
        }
      }
    }
  };

  if (DebugDir) {
    std::string SketchMergeFile =
        getUniquePath(join(*DebugDir, "SketchMerge2"), ".txt");
    std::ofstream Out(SketchMergeFile);
    // TODO 打印
    for (auto &Ent : groups) {
      Out << toString(Ent.R->key) << ": ";
      for (auto N : Ent.Set) {
        Out << toString(N->key) << ", ";
      }
      Out << "\n";
    }
    Out.close();
  }

  // 5. 创建新的shared_ptr ConstraintsGenerator，复制关键节点，start end memory
  auto To = std::make_shared<ConstraintsGenerator>(Ctx, CG.Name + "-sketch");
  if (CG.Start) {
    Old2New.insert({CG.Start, To->CG.getStartNode()});
  }
  if (CG.End) {
    Old2New.insert({CG.End, To->CG.getEndNode()});
  }

  std::function<CGNode *(CGNode *)> doMap = [&](CGNode *N) -> CGNode * {
    auto It = Old2New.find(N);
    if (It != Old2New.end()) {
      return It->second;
    }
    assert(!N->isStartOrEnd());
    auto It2 = equivalenceClasses.find(N);
    CGNode *NewNode = nullptr;
    if (It2 == equivalenceClasses.end()) {
      NewNode = &To->CG.createNodeNoPNI(N->key, N->Size);
    } else {
      auto Rep = It2->second->R;
      if (N == Rep) {
        NewNode = &To->CG.createNodeNoPNI(N->key, N->Size);
      } else {
        NewNode = doMap(Rep);
      }
    }
    assert(NewNode != nullptr);
    auto It3 = Old2New.insert({N, NewNode});
    assert(It3.second && "clone: Node already cloned!?");
    return NewNode;
  };

  // 创建新节点
  for (auto &N : CG) {
    if (N.isStartOrEnd()) {
      continue;
    }
    doMap(&N);
  }

  // clone all edges
  for (auto &Node : CG.Nodes) {
    auto NewNode = Old2New.at(&Node);
    for (auto &Edge : Node.outEdges) {
      // sketchSplit
      if (Edge.getLabel().isForgetLabel() || Edge.getLabel().isRecallBase()) {
        continue;
      }
      if (Edge.Label.isOne() && Node.key.Base.isPrimitive() &&
          Edge.getTargetNode().key.Base.isPrimitive()) {
        // one edge between primitive is invalid.
        continue;
      }
      auto &Target = Edge.getTargetNode();
      auto NewTarget = Old2New.at(&Target);
      auto Label = Edge.Label;
      if (retypd::isStore(Label)) {
        Label = retypd::toLoad(Label);
      }
      if (NewNode == NewTarget) {
        // filter self edge
        if (Label.isOne()) {
          continue;
        }
        if (auto Off = retypd::getOffsetLabel(Label)) {
          if (Off->range.isZero()) {
            continue;
          }
        }
      }
      To->CG.onlyAddEdge(*NewNode, *NewTarget, Label);
    }
  }

  // clone PNI Graph
  if (CG.PG) {
    To->CG.PG->cloneFrom(*CG.PG, Old2New);
  }

  if (CG.Memory) {
    To->CG.Memory = Old2New.at(CG.Memory);
    if (To->CG.Memory->key.Base.getBaseName() != "MEMORY") {
      To->CG.Memory->key = CG.Memory->key;
    }
  }
  if (CG.MemoryC) {
    To->CG.MemoryC = Old2New.at(CG.MemoryC);
    if (To->CG.MemoryC->key.Base.getBaseName() != "MEMORY") {
      To->CG.MemoryC->key = CG.MemoryC->key;
    }
  }

  // 更新V2N映射
  for (auto &Ent : V2N) {
    // this is NodeKey, no need to map.
    auto It = To->V2N.insert(Ent.first, Old2New.at(Ent.second));
    assert(It.second);
  }
  for (auto &Ent : V2NContra) {
    auto It = To->V2NContra.insert(Ent.first, Old2New.at(Ent.second));
    assert(It.second);
  }

  // 复制其他必要的信息
  To->SCCs = SCCs;
  // skip CallToInstance UnhandledCalls
  To->CG.isNotSymmetry = true;
  To->CG.isSketchSplit = true;
  return To;
}

std::shared_ptr<ConstraintsGenerator>
ConstraintsGenerator::cloneShared(std::map<const CGNode *, CGNode *> &Old2New,
                                  bool isMergeClone) {
  auto G = std::make_shared<ConstraintsGenerator>(Ctx, CG.Name);
  cloneTo(*G, Old2New, isMergeClone);
  return G;
}

void ConstraintsGenerator::cloneTo(
    ConstraintsGenerator &G, std::map<const CGNode *, CGNode *> &Old2New,
    bool isMergeClone,
    std::function<retypd::ConstraintGraph::SubtypeRelation(retypd::CGNode &,
                                                           retypd::CGNode &)>
        ConflictKeyRelation) {
  ConstraintGraph::clone(Old2New, CG, G.CG, isMergeClone, nullptr,
                         ConflictKeyRelation);
  assert(&Ctx == &G.Ctx);
  for (auto &Ent : V2N) {
    // this is NodeKey, no need to map.
    auto It = G.V2N.insert(Ent.first, Old2New.at(Ent.second));
    assert(It.second);
  }
  for (auto &Ent : V2NContra) {
    auto It = G.V2NContra.insert(Ent.first, Old2New.at(Ent.second));
    assert(It.second);
  }
  G.SCCs = SCCs;
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
    auto *Node = Ent.second;
    Worklist.push(Node);
  }
  // also add contravariant node
  for (auto &Ent : V2NContra) {
    auto *Node = Ent.second;
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
  std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToRemove;
  for (auto &Node : CG) {
    if (&Node == CG.getStartNode() || &Node == CG.getEndNode()) {
      continue;
    }
    if (ReachableNodes.count(&Node) == 0) {
      ToErase.push_back(&Node);
      // erase all out edges
      for (auto &Edge : Node.outEdges) {
        // CG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
        ToRemove.emplace_back(&Edge.FromNode, &Edge.TargetNode, Edge.Label);
      }
    }
  }
  for (auto &Ent : ToRemove) {
    CG.removeEdge(*std::get<0>(Ent), *std::get<1>(Ent), std::get<2>(Ent));
  }
  for (auto *Node : ToErase) {
    removeNode(*Node);
  }
}

void ConstraintsGenerator::linkContraToCovariant() {
  // assert(CG.isNotSymmetry);
  for (auto &Ent : V2N) {
    auto *N = Ent.second;
    if (V2NContra.count(Ent.first) == 0) {
      llvm::errs() << "Error: Cannot find " << toString(Ent.second) << "\n";
      // assert(false);
      continue;
    }
    auto *CN = V2NContra.at(Ent.first);
    assert(CN->getPNIVar() == N->getPNIVar());
    CG.addEdge(*CN, *N, {retypd::One{}});
  }
}

void ConstraintsGenerator::quotientMerge() {
  // 基于图上的等价关系，合并节点。
  // 子类型边已经被之前的算法处理了。现在只需要处理非子类型边。
  // 核心规则：如果某个节点在相同的出边下指向了两个不同的节点，则合并这两个节点。不断重复直到不再能合并节点
  // TODO
  bool changed = true;

  while (changed) {
    changed = false;

    // 遍历所有节点
    for (auto &Node : CG) {
      if (Node.isStartOrEnd()) {
        continue;
      }

      // 收集所有需要合并的节点对
      // std::vector<std::pair<CGNode*, CGNode*>> toMerge;
      std::map<CGNode *, CGNode *> toMerge;

      // 查找根节点（路径压缩）
      std::function<CGNode *(CGNode *)> Find = [&](CGNode *node) -> CGNode * {
        auto It = toMerge.find(node);
        if (It == toMerge.end()) {
          // 如果节点不在并查集中，即表示没有被合并
          return node;
        }

        if (It->second == node) {
          return node;
        }

        // 路径压缩
        auto To = Find(It->second);
        if (To != It->second) {
          It->second = To;
        }
        return To;
      };

      auto Merge = [&](CGNode *From, CGNode *To) {
        CGNode *rootA = Find(From);
        CGNode *rootB = Find(To);

        if (rootA != rootB) {
          // 总是合并到较小的节点（避免非确定性）
          if (rootA->getId() < rootB->getId()) {
            toMerge[rootB] = rootA;
          } else {
            toMerge[rootA] = rootB;
          }
        }
      };

      // 按出边标签分组，检查相同标签是否指向不同节点
      std::map<retypd::EdgeLabel, std::set<CGNode *>> outEdgesByLabel;

      for (auto &edge : Node.outEdges) {
        CGNode *target = const_cast<CGNode *>(&edge.getTargetNode());
        // 跳过指向自己的边（自环）
        if (target == &Node) {
          continue;
        }
        outEdgesByLabel[edge.getLabel()].insert(target);
      }

      // 对于每个标签，如果指向多个不同节点，则这些目标节点应该合并
      for (auto &entry : outEdgesByLabel) {
        auto &targets = entry.second;
        if (targets.size() > 1) {
          // 找到所有需要合并的节点对
          auto it = targets.begin();
          CGNode *firstTarget = *it;
          ++it;

          for (; it != targets.end(); ++it) {
            CGNode *currentTarget = *it;
            Merge(firstTarget, currentTarget);
          }
        }
      }

      // 执行合并
      for (auto &mergePair : toMerge) {
        CGNode *from = mergePair.first;
        CGNode *to = Find(mergePair.second);

        // 确保两个节点都还存在且不同
        if (from != to) {
          if (TraceIds.count(from->getId()) || TraceIds.count(to->getId())) {
            llvm::errs() << "quotientMerge: Merging Node "
                         << toString(from->key) << " into " << toString(to->key)
                         << "\n";
          }

          mergeNodeTo(*from, *to, false);
          changed = true;
        }
      }
    }
  }
}

void ConstraintsGenerator::makePointerEqual(
    std::optional<std::string> DebugDir) {
  // std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToAdd;
  // the iteration is safe as long as we keep N exist.
  for (auto &N : CG) {
    if (N.isStartOrEnd()) {
      continue;
    }
    // if (N.isPNIPointer() || N.getPNIVar().isFunc()) {
    //   continue;
    // }
    bool Merged = false;
    do {
      Merged = false;
      std::set<CGNode *> ToMerge;
      for (auto &Edge : N.outEdges) {
        auto Target = const_cast<CGNode *>(&Edge.getTargetNode());
        if (Edge.Label.isOne() && Target != &N) {
          // ToAdd.emplace_back(&Edge.TargetNode, &Edge.FromNode, Edge.Label);
          ToMerge.insert(Target);
        }
      }
      for (auto T : ToMerge) {
        if (TraceIds.count(T->getId())) {
          llvm::errs()
              << "ConstraintsGenerator::makePointerEqual: Merging Node "
              << toString(T->key) << " into " << toString(N.key) << "\n";
        }
        mergeNodeTo(*T, N);
        Merged = true;
      }
    } while (Merged);
  }
  // for (auto &Ent : ToAdd) {
  //   CG.addEdge(*std::get<0>(Ent), *std::get<1>(Ent), std::get<2>(Ent));
  // }
}

void ConstraintsGenerator::eliminateCycle(std::optional<std::string> DebugDir) {
  std::optional<std::string> CycleFile;
  if (DebugDir) {
    CycleFile = getUniquePath(join(*DebugDir, "Cycles"), ".txt");
  }

  // detect loops and merge them
  // get scc iterator
  all_scc_iterator<OffsetOnly<ConstraintGraph *>> SCCI =
      notdec::scc_begin(OffsetOnly<ConstraintGraph *>(&CG));

  std::vector<std::set<CGNode *>> SCCs;
  for (; !SCCI.isAtEnd(); ++SCCI) {
    SCCs.emplace_back();
    auto &SCCSet = SCCs.back();
    for (auto N : *SCCI) {
      SCCSet.insert(N.Graph);
    }
  }

  for (const auto &SCCSet : SCCs) {
    std::set<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> inSCCEdges;
    std::set<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> outSCCEdges;
    std::set<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>>
        InternalSCCEdges;

    // 1. Collect and classify all edges.
    for (auto N : SCCSet) {
      for (auto E : N->inEdges) {
        bool SourceInSCC = SCCSet.count(&E->getSourceNode());
        if (SourceInSCC) {
          InternalSCCEdges.insert(std::make_tuple(
              &E->getSourceNode(), &E->getTargetNode(), E->getLabel()));
        } else {
          inSCCEdges.insert(std::make_tuple(
              &E->getSourceNode(), &E->getTargetNode(), E->getLabel()));
        }
      }
      for (auto &E : N->outEdges) {
        auto Src = const_cast<CGNode *>(&E.getSourceNode());
        auto Dst = const_cast<CGNode *>(&E.getTargetNode());

        bool TargetInSCC = SCCSet.count(Dst);
        if (TargetInSCC) {
          InternalSCCEdges.insert(std::make_tuple(Src, Dst, E.getLabel()));
        } else {
          outSCCEdges.insert(std::make_tuple(Src, Dst, E.getLabel()));
        }
      }
    }

    // for each SCCEdge, find the lowest offset
    std::optional<long> MinOffset;
    std::tuple<CGNode *, CGNode *, retypd::EdgeLabel> MinOffEdge = {};

    // find min offset and store in MinOffset. Update MinOffEdge accordingly.
    auto VisitOffset =
        [&](int64_t Off,
            const std::tuple<CGNode *, CGNode *, retypd::EdgeLabel> &Ent) {
          // visit offset
          if (Off > 0) {
            if (!MinOffset.has_value()) {
              MinOffset = Off;
              MinOffEdge = Ent;
            } else {
              if (Off < *MinOffset) {
                MinOffset = Off;
                MinOffEdge = Ent;
              }
            }
          }
        };

    for (auto &Ent : InternalSCCEdges) {
      // auto Src = std::get<0>(Ent);
      // auto Dst = std::get<1>(Ent);
      auto &L = std::get<2>(Ent);
      if (auto Off = retypd::getOffsetLabel(L)) {
        VisitOffset(Off->range.offset, Ent);
        for (auto A : Off->range.access) {
          VisitOffset(A.Size, Ent);
        }
      }
    }

    // 1 remove all edges within SCC.
    for (auto Ent : InternalSCCEdges) {
      auto Src = std::get<0>(Ent);
      auto Dst = std::get<1>(Ent);
      auto &L = std::get<2>(Ent);
      CG.removeEdge(*Src, *Dst, L);
    }

    if (!MinOffset.has_value()) {
      if (SCCSet.size() != 1) {
        // no offset, just a epsilon loop. then merge into one node.
        auto &Merged = **SCCSet.begin();
        // output the merged node name map to txt
        if (CycleFile) {
          std::ofstream Out(*CycleFile, std::ios::app);
          Out << "NON-OFF, ";
          for (auto *Node : SCCSet) {
            Out << toString(Node->key) << ", ";
          }
          Out << "\n";
          Out.close();
        }

        // auto *OldPN =
        notdec::retypd::NFADeterminizer<>::ensureSamePNI(SCCSet);
        // 3. perform node merging
        for (auto *Node : SCCSet) {
          if (Node == &Merged) {
            continue;
          }
          mergeNodeTo(*Node, Merged, true);
        }
      }
    } else {
      // has offset edge, merge into two nodes.
      // output the merged node name map to txt

      // move incoming and outgoing edges, remove unnecessary incoming or
      // outgoing offset
      auto N1 = std::get<0>(MinOffEdge);
      auto N2 = std::get<1>(MinOffEdge);
      EdgeLabel L1 = {retypd::RecallLabel{OffsetLabel{
          .range =
              OffsetRange{.offset = 0, .access = {ArrayOffset(*MinOffset)}}}}};
      CG.addEdge(*N1, *N2, L1);

      if (CycleFile) {
        std::ofstream Out(*CycleFile, std::ios::app);
        Out << "OFF, " << toString(N1->key) << ", " << toString(N2->key)
            << ", ";
        for (auto *N : SCCSet) {
          if (N == N1 || N == N2) {
            continue;
          }
          Out << toString(N->key) << ", ";
        }
        Out << "\n";
        Out.close();
      }

      for (auto Ent : inSCCEdges) {
        auto Src = std::get<0>(Ent);
        auto Dst = std::get<1>(Ent);
        auto L = std::get<2>(Ent);
        CG.removeEdge(*Src, *Dst, L);

        auto LinkTo = N1;
        if (auto Off = retypd::getOffsetLabel(L)) {
          if (Off->range.access.empty() && Off->range.offset == *MinOffset) {
            LinkTo = N2;
            L = L1;
          } else if (Off->range.access.size() == 1 &&
                     Off->range.access.front() == MinOffset) {
            if (Off->range.offset == 0) {
              LinkTo = N2;
            } else {
              auto &OffRef = retypd::getLabelOffsetRef(L);
              OffRef.access.clear();
            }
          }
        }
        CG.addEdge(*Src, *LinkTo, L);
      }
      for (auto Ent : outSCCEdges) {
        auto Src = std::get<0>(Ent);
        auto Dst = std::get<1>(Ent);
        auto L = std::get<2>(Ent);
        CG.removeEdge(*Src, *Dst, L);
        auto LinkFrom = N2;
        if (auto Off = retypd::getOffsetLabel(L)) {
          if (Off->range.access.empty() && Off->range.offset == *MinOffset) {
            LinkFrom = N1;
            L = L1;
          } else if (Off->range.access.size() == 1 &&
                     Off->range.access.front() == MinOffset) {
            if (Off->range.offset == 0) {
              LinkFrom = N1;
            } else {
              auto &OffRef = retypd::getLabelOffsetRef(L);
              OffRef.access.clear();
            }
          }
        }
        CG.addEdge(*LinkFrom, *Dst, L);
      }
      notdec::retypd::NFADeterminizer<>::ensureSamePNI(SCCSet);
      // merge all node to src
      for (auto N : SCCSet) {
        if (N == N1 || N == N2) {
          continue;
        }
        mergeNodeTo(*N, *N1, true);
      }
    }
  }
}

std::map<const CGEdge *, const CGEdge *>
ConstraintsGenerator::mergeNodeTo(CGNode &From, CGNode &To, bool NoSelfLoop) {
  assert(&From.Parent == &CG && &To.Parent == &CG);
  assert(&From != CG.Start && &To != CG.End);
  // update the value map
  addMergeNode(From, To);
  // update the graph
  return CG.mergeNodeTo(From, To, NoSelfLoop);
}

void ConstraintsGenerator::mergeOnlySubtype() {
  // perform merging: if node A -> B, and there is no other edge from A to other
  // node, then merge A to B
  // for each node and outgoing edge, check if is one edge
  auto findMergePair = [&]() -> std::pair<CGNode *, CGNode *> {
    for (auto &Node : CG.Nodes) {
      if (Node.isSpecial()) {
        continue;
      }
      if (Node.key.Base.isPrimitive()) {
        continue;
      }

      for (auto &Edge : Node.outEdges) {
        if (!Edge.getLabel().isOne()) {
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
    mergeNodeTo(*A, *B, false);
    A = nullptr;
    B = nullptr;
    std::tie(A, B) = findMergePair();
  }
}

std::map<std::pair<OffsetRange, retypd::EdgeLabel>, std::set<CGNode *>>
allOutOffLabels(std::set<CGNode *> StartSet) {
  std::map<std::pair<OffsetRange, retypd::EdgeLabel>, std::set<CGNode *>>
      Result;
  for (auto *Node : StartSet) {
    // 1. non offset edges
    for (auto &Edge : Node->outEdges) {
      if (retypd::isOffsetOrOne(Edge)) {
        continue;
      }
      auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
      Result[{OffsetRange(), Edge.getLabel()}].insert(&Target);
    }
    for (auto &Ent : Node->Parent.getNodeReachableOffset(*Node)) {
      auto &Target = Ent.first;
      auto &Offset = Ent.second;
      for (auto &Edge : Target->outEdges) {
        if (retypd::isOffsetOrOne(Edge)) {
          continue;
        }
        auto &Target = const_cast<CGNode &>(Edge.getTargetNode());
        Result[{Offset, Edge.getLabel()}].insert(&Target);
      }
    }
  }
  return Result;
}

std::shared_ptr<ConstraintsGenerator> ConstraintsGenerator::minimizeShared(
    std::map<const CGNode *, CGNode *> &Old2New) {
  auto G = std::make_shared<ConstraintsGenerator>(Ctx, CG.Name);
  minimizeTo(*G, Old2New);
  return G;
}

void ConstraintsGenerator::minimizeTo(
    ConstraintsGenerator &Target, std::map<const CGNode *, CGNode *> &Old2New) {

  assert(CG.isNotSymmetry && CG.isSketchSplit);
  Target.CG.isNotSymmetry = true;
  Target.CG.isSketchSplit = true;

  // link all V2N Nodes.
  linkNodes();
  std::map<std::set<CGNode *>, CGNode *> NodeMap;
  retypd::minimizeTo(&CG, &Target.CG, &NodeMap, true);

  // maintain V2N.
  Target.recoverNodes(*this);

  Target.SCCs = SCCs;
}

void ConstraintsGenerator::recoverNodes(ConstraintsGenerator &From) {
  // use with linkNodes.
  std::set<CGNode *> Updated;
  // // For a old node in label, to the set of new nodes.
  // std::vector<std::pair<CGNode *, std::set<CGNode *> *>> Node2Nodes;
  std::map<CGNode *, CGNode *> V2NMap;

  std::map<retypd::EdgeLabel, std::set<CGNode *>> StartEdgeMap;
  for (auto &OE : CG.Start->outEdges) {
    StartEdgeMap[OE.getLabel()].insert(
        const_cast<CGNode *>(&OE.getTargetNode()));
  }

  auto UpdateKey = [&](NodeKey &NK, CGNode &N) {
    if (!Updated.count(&N)) {
      Updated.insert(&N);
      N.key = NK;
      return true;
    }
    return false;
  };

  // 1 in edges
  std::map<retypd::EdgeLabel, std::set<CGNode *>> EndEdgeMap;
  for (auto *IE : CG.End->inEdges) {
    EndEdgeMap[IE->getLabel()].insert(
        const_cast<CGNode *>(&IE->getSourceNode()));
  }

  // handle primitive ForgetNode and ForgetString
  for (auto Ent : EndEdgeMap) {
    if (auto FN = Ent.first.getAs<retypd::ForgetNode>()) {
      auto OldN = FN->Base;
      // auto &NewN = CG.createNodeClonePNI(OldN->key, OldN->getPNIVar());
      for (auto N : Ent.second) {
        // CG.addEdge(*N, NewN, {retypd::One{}});
        N->getPNIVar()->merge(OldN->getPNIVar()->getLatticeTy());
      }
    } else if (auto FS = Ent.first.getAs<retypd::ForgetString>()) {
      auto OldPNTy = FS->Base;
      retypd::PNTy NewPN(OldPNTy);
      for (auto N : Ent.second) {
        N->getPNIVar()->merge(NewPN);
      }
    } else if (auto FB = Ent.first.getAs<retypd::ForgetBase>()) {
      // assert(Ent.second.size() == 1);
      // auto NewN = *Ent.second.begin();
      // auto NewKey = NodeKey{FB->Base, FB->V};
      // auto isUpdated = UpdateKey(NewKey, *NewN);
      // assert(isUpdated);
    } else if (auto FS = Ent.first.getAs<retypd::ForgetSize>()) {
      // skip
    } else {
      assert(false);
    }
  }

  // 2 handle memory nodes.
  for (auto &Ent : StartEdgeMap) {
    auto RN = Ent.first.getAs<retypd::RecallNode>();
    auto OldN = RN->Base;
    // handle memory nodes first
    if (OldN->isMemory()) {
      assert(CG.getMemoryNodeOrNull(OldN->getVariance()) == nullptr);
      if (Ent.second.size() == 1) {
        auto NewN = const_cast<CGNode *>(*Ent.second.begin());
        // just set the node as memory
        auto NewKey =
            NodeKey{TypeVariable::CreateDtv(*CG.Ctx, TypeVariable::Memory),
                    OldN->getVariance()};
        // auto isUpdated =
        UpdateKey(NewKey, *NewN);
        // assert(isUpdated);
        if (OldN->getVariance() == retypd::Covariant) {
          CG.Memory = NewN;
        } else {
          CG.MemoryC = NewN;
        }
      } else {
        assert(false);
        // create new mem node, and add subtype edges.
        // auto Mem = CG.getMemoryNode(OldN->getVariance());
        // for (auto NewN : Ent.second) {
        //   // no longer symmetry, so no need to consider contravariant reverse
        //   // direction.
        //   CG.addEdge(*Mem, *NewN, {retypd::One{}});
        // }
      }
    }
  }

  // 2 handle other V2N recall nodes edges.
  for (auto &Ent : StartEdgeMap) {
    auto RN = Ent.first.getAs<retypd::RecallNode>();
    auto OldN = RN->Base;
    // handle memory nodes first
    if (OldN->isMemory()) {
      continue;
    }
    // must be v2n or v2n contra node. add to node map.
    CGNode *NewN = nullptr;
    if (Ent.second.size() == 1) {
      NewN = const_cast<CGNode *>(*Ent.second.begin());
      // auto isUpdated =
      UpdateKey(OldN->key, *NewN);
      V2NMap.insert({OldN, NewN});
    } else {
      assert(false);
      // defer in Node2Nodes
      // Node2Nodes.push_back({OldN, &Ent.second});
    }
  }

  // // handle Node2Nodes
  // std::sort(Node2Nodes.begin(), Node2Nodes.end(),
  //           [](const auto &a, const auto &b) {
  //             return a.second->size() < b.second->size();
  //           });
  // for (long I = 0; I < Node2Nodes.size(); I++) {
  //   auto &Ent = Node2Nodes.at(I);
  //   auto OldN = Ent.first;
  //   // create new node that represent target nodes' merge.
  //   auto &NewN = CG.createNodeClonePNI(OldN->key, OldN->getPNIVar());
  //   auto NodeSet = *Ent.second;
  //   // reverse iterate each entry in Node2Nodes
  //   for (long J = I - 1; J >= 0; J--) {
  //     auto &CurrentEnt = Node2Nodes.at(J);
  //     bool AllContains = true;
  //     for (auto Elem : *CurrentEnt.second) {
  //       if (!NodeSet.count(Elem)) {
  //         AllContains = false;
  //       }
  //     }
  //     if (AllContains) {
  //       for (auto Elem : *CurrentEnt.second) {
  //         NodeSet.erase(Elem);
  //       }
  //       CG.addEdge(NewN, *V2NMap.at(CurrentEnt.first), {retypd::One{}});
  //     }
  //   }
  //   // handle left nodes
  //   for (auto N : NodeSet) {
  //     assert(false);
  //     CG.addEdge(NewN, *N, {retypd::One{}});
  //   }
  //   V2NMap.insert({OldN, &NewN});
  // }

  // recover V2N Maps
  for (auto &Ent : From.V2N) {
    V2N.insert(Ent.first, V2NMap.at(Ent.second));
  }
  for (auto &Ent : From.V2NContra) {
    V2NContra.insert(Ent.first, V2NMap.at(Ent.second));
  }

  // remove all start and end edges.
  CG.Start->removeOutEdges();
  std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToRemove;
  for (auto E : CG.End->inEdges) {
    // TODO Debug: try to early detect that one node have conflicting size hint.
    if (E->getLabel().isForgetSize()) {
      E->getSourceNode().getSizeHint();
    }
    // keep forget primitive and size hint edges.
    if (E->getLabel().isForgetBase() || E->getLabel().isForgetSize()) {
      continue;
    }
    ToRemove.push_back(std::make_tuple(&E->getSourceNode(), &E->getTargetNode(),
                                       E->getLabel()));
  }
  for (auto &Ent : ToRemove) {
    auto [Source, Target, Label] = Ent;
    CG.removeEdge(*Source, *Target, Label);
  }
}

void ConstraintsGenerator::linkNodes() {
  // Run sketchSplit first!
  assert(CG.isSketchSplit);
  // link V2N nodes
  auto Start = CG.getStartNode();
  auto End = CG.getEndNode();
  for (auto Ent : V2N) {
    CG.addEdge(*Start, *Ent.second, {retypd::RecallNode{.Base = Ent.second}});
    // CG.addEdge(*Ent.second, *End, {retypd::ForgetNode{.Base = Ent.second}});
  }
  for (auto Ent : V2NContra) {
    CG.addEdge(*Start, *Ent.second, {retypd::RecallNode{.Base = Ent.second}});
    // CG.addEdge(*Ent.second, *End, {retypd::ForgetNode{.Base = Ent.second}});
  }
  // link prim or memory or load/store
  for (auto &Node : CG) {
    if (Node.isStartOrEnd()) {
      continue;
    }
    if (Node.isMemory()) {
      CG.addEdge(*Start, Node, {retypd::RecallNode{.Base = &Node}});
      // CG.addEdge(Node, *End, {retypd::ForgetNode{.Base = &Node}});
    } else if (isPrimitive(Node)) {
      // CG.addEdge(*Start, Node, {retypd::RecallNode{.Base = &Node}});
      CG.addEdge(Node, *End, {retypd::ForgetNode{.Base = &Node}});
    } else if (Node.outEdges.empty()) {
      // Link by PNI type.
      // CG.addEdge(*Start, Node, {retypd::RecallString{.Base =
      // Node.getPNIVar()->getLatticeTy().str()}});
      CG.addEdge(Node, *End,
                 {retypd::ForgetString{
                     .Base = Node.getPNIVar()->getLatticeTy().str()}});
    }
  }

  // To preserve all possible paths,
  // find unreachablenodes and add forget string edges.

  std::set<CGNode *> UnreachableNodes;
  for (auto &Node : CG) {
    if (Node.isStartOrEnd()) {
      continue;
    }
    UnreachableNodes.insert(&Node);
  }

  auto NewReachable = [&](CGNode *New) {
    // traverse start from #End, find all reverse unreachable nodes.
    std::queue<CGNode *> Queue;
    if (UnreachableNodes.count(New)) {
      UnreachableNodes.erase(New);
      Queue.push(New);
    }
    while (!Queue.empty()) {
      auto Front = Queue.front();
      Queue.pop();
      for (auto E : Front->inEdges) {
        auto Src = &E->getSourceNode();
        if (UnreachableNodes.count(Src)) {
          UnreachableNodes.erase(Src);
          Queue.push(Src);
        }
      }
    }
  };

  UnreachableNodes.insert(End);
  NewReachable(End);

  while (!UnreachableNodes.empty()) {
    // if there is still unreachable nodes, we find node with incoming
    // load/store, add forgetString edge, and update the reachable node map.
    for (auto N : UnreachableNodes) {
      if (N->hasIncomingLoadOrStore()) {
        CG.addEdge(*N, *End,
                   {retypd::ForgetString{
                       .Base = N->getPNIVar()->getLatticeTy().str()}});
        NewReachable(N);
        break;
      }
    }

    // if not working, just link with forgetString
    for (auto N : UnreachableNodes) {
      CG.addEdge(
          *N, *End,
          {retypd::ForgetString{.Base = N->getPNIVar()->getLatticeTy().str()}});
      NewReachable(N);
      break;
    }
  }
}

void ConstraintsGenerator::determinize() {
  // assert(DTrans.empty());
  DTrans.clear();
  std::map<const CGNode *, CGNode *> This2Bak;
  ConstraintGraph Backup = CG.clone(This2Bak);

  // remove all edges in the graph
  std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToRemove;
  for (auto &N : CG) {
    for (auto &Edge : N.outEdges) {
      // CG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
      ToRemove.push_back({&Edge.FromNode, &Edge.TargetNode, Edge.Label});
    }
  }
  for (auto &Ent : ToRemove) {
    CG.removeEdge(*std::get<0>(Ent), *std::get<1>(Ent), std::get<2>(Ent));
  }
  // remove all node that is not in the value map
  std::set<CGNode *> V2NNodes;
  for (auto &Ent : V2N) {
    if (auto *N = (Ent.second)) {
      V2NNodes.insert(N);
    }
  }
  for (auto &Ent : V2NContra) {
    if (auto *N = (Ent.second)) {
      V2NNodes.insert(N);
    }
  }
  std::vector<CGNode *> ToRemove1;
  for (auto &N : CG.Nodes) {
    if (N.isStartOrEnd()) {
      continue;
    }
    if (!N.isSpecial() && V2NNodes.count(&N) == 0) {
      // removeNode(N.key);
      ToRemove1.push_back(&N);
    }
  }
  for (auto *N : ToRemove1) {
    removeNode(*N);
  }

  using EntryTy = typename std::map<std::set<CGNode *>, CGNode *>::iterator;

  auto getOrSetNewNode = [this](const std::set<CGNode *> &N) -> EntryTy {
    if (DTrans.count(N)) {
      return DTrans.find(N);
    }

    CGNode *NewNode;
    NewNode =
        &CG.createNodeClonePNI(retypd::NodeKey{TypeVariable::CreateDtv(
                                   *Ctx.TRCtx, ValueNamer::getName("dtm_"))},
                               (*N.begin())->getPNIVar());

    for (auto N1 : N) {
      NewNode->getPNIVar()->merge(N1->getPNIVar()->getLatticeTy());
    }
    if (NewNode->getPNIVar()->isConflict()) {
      notdec::retypd::NFADeterminizer<>::printPNDiffSet(N);
    }
    auto it = DTrans.emplace(N, NewNode);
    assert(it.second);
    return it.first;
  };

  DTrans[{Backup.getEndNode()}] = CG.getEndNode();
  std::queue<EntryTy> Worklist;

  // for each node in the value map
  for (auto *Node : V2NNodes) {
    auto *BakNode = This2Bak.at(Node);
    std::set<CGNode *> StartSet = retypd::NFADeterminizer<>::countClosure(
        {BakNode},
        [](const CGNode *Node) { return !Node->key.Base.isPrimitive(); });
    auto pair1 = DTrans.emplace(StartSet, Node);
    if (pair1.second) {
      Worklist.push(pair1.first);
    } else {
      llvm::errs() << "Node: " << toString(Node->key) << "\n";
      llvm::errs() << "StartSet: " << toString(StartSet) << "\n";
      llvm::errs() << "Prev Set: " << toString(pair1.first->first) << "\n";
      llvm::errs() << "Prev Node: " << toString(pair1.first->second->key)
                   << "\n";
      // Can be a epsilon loop. should be removed earlier
      assert(false);
    }
  }

  while (!Worklist.empty()) {
    auto It = Worklist.front();
    auto &Node = *It->second;
    auto outLabelsMap = allOutOffLabels(It->first);
    std::map<OffsetRange, CGNode *> OfftmpNodes;
    for (auto &L : outLabelsMap) {
      auto &S = L.second;
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

      auto *FromNode = &Node;
      if (!L.first.first.isZero()) {
        CGNode *TmpNode;
        if (OfftmpNodes.count(L.first.first)) {
          TmpNode = OfftmpNodes.at(L.first.first);
        } else {
          TmpNode = &CG.createNodeClonePNI(
              retypd::NodeKey{TypeVariable::CreateDtv(
                  *Ctx.TRCtx, ValueNamer::getName("offtmp_"))},
              FromNode->getPNIVar());
          CG.addEdge(*FromNode, *TmpNode,
                     {retypd::RecallLabel{OffsetLabel{L.first.first}}});
          OfftmpNodes.insert({L.first.first, TmpNode});
        }
        FromNode = TmpNode;
      }
      CG.onlyAddEdge(*FromNode, ToNode, L.first.second);
    }
    Worklist.pop();
  }
  // mergeAfterDeterminize();
}

void ConstraintsGenerator::mergeAfterDeterminize() {
  const clock_t begin_time = clock();

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
    for (auto &Node : CG.Nodes) {
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
  unsigned long count = 0;
  while (A != nullptr && B != nullptr) {
    // merge A to B
    // TODO eliminate offset loop earlier
    count += 1;
    mergeNodeTo(*A, *B, false);
    A = nullptr;
    B = nullptr;
    std::tie(A, B) = findMergePair();
  }
  auto DurationMS = (float(clock() - begin_time) * 1000 / CLOCKS_PER_SEC);
  std::cerr << "ConstraintsGenerator::mergeAfterDeterminize: Merged " << count
            << " Nodes in " << DurationMS << "ms for " << CG.Name << ".\n";
}

void ConstraintsGenerator::run() {
  for (llvm::Function *Func : SCCs) {
    // create function nodes
    auto &F = getOrInsertNode(Func, nullptr, -1, retypd::Covariant);
    for (int i = 0; i < Func->arg_size(); i++) {
      auto &Arg =
          getOrInsertNode(Func->getArg(i), nullptr, i, retypd::Contravariant);
      addConstraint(F, Arg, {retypd::InLabel{.name = std::to_string(i)}});
    }
    if (!Func->getReturnType()->isVoidTy()) {
      auto &Ret = getOrInsertNode(ReturnValue{.Func = Func}, nullptr, -1,
                                  retypd::Covariant);
      addConstraint(F, Ret, {retypd::OutLabel{}});
    }
  }
  for (llvm::Function *Func : SCCs) {
    RetypdGeneratorVisitor Visitor(*this);
    Visitor.visit(Func);
    Visitor.handlePHINodes();
  }
  CG.PG->solve();
  // preSimplify();
  if (const char *path = std::getenv("DEBUG_TRANS_INIT_GRAPH")) {
    if ((std::strcmp(path, "1") == 0) || (std::strstr(CG.Name.c_str(), path))) {
      CG.printGraph("trans_init.dot");
    }
  }
}

void ConstraintsGenerator::instantiateSummary(
    llvm::CallBase *Inst, llvm::Function *Target,
    const ConstraintsGenerator &Summary) {
  // checkSymmetry();
  auto [FI, FIC] = CallToInstance.at(Inst);
  assert(FI->key.Base.getContextId().size() == 1);
  auto CurrentId = FI->key.Base.getContextId().front();
  auto NKN = FI->key.Base.getBaseName();

  // copy the whole graph into it. and add subtype relation
  std::map<const CGNode *, CGNode *> Old2New;
  ConstraintGraph::clone(
      Old2New, Summary.CG, CG, true, [&](const retypd::NodeKey &N) {
        if (N.Base.isPrimitive()) {
          return N;
        }
        retypd::NodeKey Ret = N;
        // 这里即使有了ContextId，在函数节点和参数节点还是会重复，所以再设置actual标志。
        auto Base = N.Base;
        if (N.Base.hasBaseName() && N.Base.getBaseName() == NKN) {
          Base = Base.markActual();
        }
        Base.pushContextId(CurrentId);
        Ret.Base = Base;
        return Ret;
      });
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
}

void ConstraintsGenerator::fixSCCFuncMappings() {
  for (auto F : SCCs) {
    // create node key
    auto Dtv = TypeVariable::CreateDtv(*Ctx.TRCtx, F->getName().str());
    retypd::NodeKey K(Dtv, retypd::Covariant);
    retypd::NodeKey KC(Dtv, retypd::Contravariant);
    // find the node by key
    CGNode *Node = nullptr;
    // assert(Node != nullptr);
    CGNode *NodeC = nullptr;
    // assert(NodeC != nullptr);

    // try to find node with correct key.
    for (auto &N : CG) {
      if (N.key == K) {
        assert(Node == nullptr);
        Node = &N;
        continue;
      } else if (N.key == KC) {
        assert(NodeC == nullptr);
        NodeC = &N;
        continue;
      }
    }

    if (Node == nullptr && NodeC == nullptr) {
      llvm::errs() << "Warning: No useful summary for " << F->getName() << "\n";
      continue;
    }
    // insert to value map
    if (Node != nullptr) {
      V2N.insert(F, Node);
      // Fix return mapping
      if (!F->getFunctionType()->getReturnType()->isVoidTy()) {
        retypd::EdgeLabel RetLabel = {retypd::RecallLabel{retypd::OutLabel{}}};

        // try to find node with this edge. ensure only one
        CGNode *RetNode = Node->getLabelTarget(RetLabel);
        if (RetNode) {
          V2N.insert(ReturnValue{.Func = F}, RetNode);
        }
      }
      // Fix argument mapping
      for (int i = 0; i < F->arg_size(); i++) {
        auto Arg = F->getArg(i);
        retypd::EdgeLabel ArgLabel = {retypd::RecallLabel{
            retypd::InLabel{std::to_string(Arg->getArgNo())}}};
        CGNode *ArgNode = Node->getLabelTarget(ArgLabel);
        if (ArgNode) {
          V2N.insert(Arg, ArgNode);
        }
      }
    }
    if (NodeC != nullptr) {
      V2NContra.insert(F, NodeC);
      // Fix return mapping
      if (!F->getFunctionType()->getReturnType()->isVoidTy()) {

        retypd::EdgeLabel RetLabel = {retypd::RecallLabel{retypd::OutLabel{}}};
        CGNode *RetNodeC = NodeC->getLabelTarget(RetLabel);
        if (RetNodeC) {
          V2NContra.insert(ReturnValue{.Func = F}, RetNodeC);
        }
      }
      // Fix argument mapping
      for (int i = 0; i < F->arg_size(); i++) {
        auto Arg = F->getArg(i);
        retypd::EdgeLabel ArgLabel = {retypd::RecallLabel{
            retypd::InLabel{std::to_string(Arg->getArgNo())}}};
        CGNode *ArgNodeC = NodeC->getLabelTarget(ArgLabel);
        if (ArgNodeC) {
          V2NContra.insert(Arg, ArgNodeC);
        }
      }
    }
  }
  // dumpV2N();
}

std::shared_ptr<ConstraintsGenerator>
ConstraintsGenerator::genSummary(std::optional<std::string> DebugDir) {
  std::map<const CGNode *, CGNode *> Old2New;
  auto S = CG.clone(Old2New);
  std::set<std::string> InterestingVars;
  for (auto *F : SCCs) {
    assert(F->hasName());
    InterestingVars.insert(F->getName().str());
  }
  S.linkVars(InterestingVars, true);
  // if (S.getStartNode()->outEdges.size() == 0) {
  //   llvm::errs() << "Warning: No func nodes, No meaningful types for "
  //                << CG.getName() << "\n";
  //   return nullptr;
  // }

  if (DebugDir) {
    auto SatOut = getUniquePath(join(*DebugDir, "02-1-Linked"), ".dot");
    S.printGraph(SatOut.c_str());
  }

  auto G2 = S.simplify(DebugDir);

  // Wrap the graph as a ConstraintsGenerator, Fix mappings
  std::shared_ptr<ConstraintsGenerator> Ret =
      std::make_shared<ConstraintsGenerator>(Ctx, CG.Name);
  Ret->SCCs = SCCs;
  std::map<const CGNode *, CGNode *> Tmp;
  ConstraintGraph::clone(Tmp, G2, Ret->CG);
  Ret->makeSymmetry();

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
      return V2N.at(Val);
    }
    return nullptr;
  } else {
    if (V2NContra.count(Val)) {
      return V2NContra.at(Val);
    }
    return nullptr;
  }
}

retypd::CGNode &ConstraintsGenerator::getNode(ExtValuePtr Val, User *User,
                                              long OpInd, retypd::Variance V) {
  llvmValue2ExtVal(Val, User, OpInd);
  if (V == retypd::Covariant) {
    return *V2N.at(Val);
  } else {
    return *V2NContra.at(Val);
  }
}

std::pair<retypd::CGNode &, retypd::CGNode &>
ConstraintsGenerator::createNode(ExtValuePtr Val, User *User, long OpInd) {
  llvmValue2ExtVal(Val, User, OpInd);
  auto Dtv = convertTypeVar(Val, User, OpInd);
  retypd::NodeKey K(Dtv, retypd::Covariant);
  auto [N, NContra] = CG.createNodePair(K, getType(Val));
  auto It = V2N.insert(Val, &N);
  if (!It.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "setTypeVar: Value already mapped to "
                 << toString(It.first->second->key) << ", but now set to "
                 << toString(Dtv) << "\n";
    std::abort();
  }
  auto It2 = V2NContra.insert(Val, &NContra);
  if (!It2.second) {
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "setTypeVar: Value already mapped to "
                 << toString(It2.first->second->key) << ", but now set to "
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
  // // temporary allow by key for ConstantExpr
  // if (auto UC = std::get_if<UConstant>(&Val)) {
  //   if (auto CE = llvm::dyn_cast<ConstantExpr>(UC->Val)) {
  //     auto Dtv = convertTypeVar(Val, User, OpInd);
  //     retypd::NodeKey K(Dtv, V);
  //     auto Node1 = getNodeOrNull(K);
  //     if (Node1) {
  //       V2N.insert(Val, retypd::NodeKey(Dtv, retypd::Covariant));
  //       V2N.insert(Val, retypd::NodeKey(Dtv, retypd::Contravariant));
  //       return *Node1;
  //     }
  //   }
  // }
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
    return tv.pushLabel({retypd::OutLabel{}});
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
                  tv = tv.pushLabel({retypd::OffsetLabel{
                      .range = OffsetRange{.offset = CI->getSExtValue() *
                                                     (Ctx.pointer_size / 8)}}});
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
    } else if (isa<UndefValue>(C)) {
      return makeTv(Ctx.TRCtx, ValueNamer::getName("undef_"));
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
    tv = tv.pushLabel({retypd::InLabel{std::to_string(arg->getArgNo())}});
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
  auto *SrcVal = I.getReturnValue();
  if (SrcVal == nullptr) { // ret void.
    return;
  }
  auto &Src = cg.getOrInsertNode(SrcVal, &I, 0, retypd::Covariant);
  auto &Dst = cg.getNode(ReturnValue{.Func = I.getFunction()}, &I, 0,
                         retypd::Covariant);
  // src is a subtype of dest
  cg.addSubtype(Src, Dst);
}

bool isWithOverflowIntrinsicSigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::sadd_with_overflow ||
      ID == Intrinsic::ssub_with_overflow ||
      ID == Intrinsic::smul_with_overflow) {
    return true;
  }
  return false;
}

bool isWithOverflowIntrinsicUnsigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::uadd_with_overflow ||
      ID == Intrinsic::usub_with_overflow ||
      ID == Intrinsic::umul_with_overflow) {
    return true;
  }
  return false;
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitExtractValueInst(
    ExtractValueInst &I) {
  if (auto Call = dyn_cast<CallBase>(I.getAggregateOperand())) {
    if (auto Target = Call->getCalledFunction()) {
      if (Target->isIntrinsic()) {
        // 这里判断返回值是不是那种extract
        // value的东西，根据llvm类型直接设置为数字类型
        auto Ind = I.getIndices()[0];
        if (Ind == 0) {
          if (isWithOverflowIntrinsicSigned(Target->getIntrinsicID())) {
            auto &N = cg.createNodeCovariant(&I, nullptr, -1);
            if (N.getPNIVar()->isPNRelated()) {
              N.getPNIVar()->setNonPtr();
            }
            auto &SintNode = cg.getOrCreatePrim(
                retypd::getNameForInt("sint", I.getType()), I.getType());
            cg.addSubtype(SintNode, N);
            return;
          } else if (isWithOverflowIntrinsicUnsigned(
                         Target->getIntrinsicID())) {
            auto &N = cg.createNodeCovariant(&I, nullptr, -1);
            if (N.getPNIVar()->isPNRelated()) {
              N.getPNIVar()->setNonPtr();
            }
            auto &UintNode = cg.getOrCreatePrim(
                retypd::getNameForInt("uint", I.getType()), I.getType());
            cg.addSubtype(UintNode, N);
            return;
          }
        } else if (Ind == 1) {
          assert(I.getType()->isIntegerTy(1));
          // auto &N =
          cg.createNodeCovariant(&I, nullptr, -1);

          return;
        }
      }
    }
  }
  llvm::errs() << "Unable to handle ExtractValueInst.";
  std::abort();
}

bool ConstraintsGenerator::RetypdGeneratorVisitor::handleIntrinsicCall(
    CallBase &I) {
  auto Target = I.getCalledFunction();
  if (!Target->isIntrinsic()) {
    return false;
  }
  // auto ID = Target->getIntrinsicID();
  if (I.getType()->isAggregateType()) {
    // ignore this call and handle the value in visitExtractValue
    return true;
  }
  return false;
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target) {
    if (Target->getName().startswith("llvm.dbg") ||
        Target->getName().startswith("llvm.lifetime")) {
      return;
    }
  }

  if (Target == nullptr) {
    // TODO indirect call
    std::cerr
        << __FILE__ << ":" << __LINE__ << ": "
        << "Warn: ConstraintsGenerator: indirect call not supported yet\n";
    return;
  }

  if (handleIntrinsicCall(I)) {
    return;
  } else if (cg.SCCs.count(Target)) { // Call within the SCC:
    // directly link to the function tv.
    for (int i = 0; i < I.arg_size(); i++) {
      auto &ArgVar = cg.getNode(Target->getArg(i), &I, i, retypd::Covariant);
      auto &ValVar =
          cg.getOrInsertNode(I.getArgOperand(i), &I, i, retypd::Covariant);
      // argument is a subtype of param
      cg.addSubtype(ValVar, ArgVar);
    }
    if (!I.getType()->isVoidTy()) {
      // type var should be consistent with return instruction
      auto &FormalRetVar =
          cg.getNode(ReturnValue{.Func = Target}, &I, -1, retypd::Covariant);
      auto &ValVar = cg.getOrInsertNode(&I, nullptr, -1);
      // formal return -> actual return
      cg.addSubtype(FormalRetVar, ValVar);
    }
  } else {
    // create and save to CallToInstance map. instance with summary later in
    // getBottomUpGraph
    auto FuncVar = cg.convertTypeVar(Target, nullptr, -1);
    // differentiate different call instances in the same function
    uintptr_t ContextId = (uintptr_t)&I;
    FuncVar.pushContextId(ContextId);
    auto [FuncNode, FNC] =
        cg.CG.createNodePair(FuncVar, Target->getFunctionType());
    cg.CallToInstance.emplace(&I, std::make_pair(&FuncNode, &FNC));

    for (int i = 0; i < I.arg_size(); i++) {
      auto ArgVar = getCallArgTV(FuncVar, i);
      auto [ArgNode, ANC] =
          cg.CG.createNodePair(ArgVar, I.getArgOperand(i)->getType());
      cg.addConstraint(FuncNode, ANC, getCallArgLabel(i));

      // argument is a subtype of param
      auto &ValVar = cg.getOrInsertNode(I.getArgOperand(i), &I, i);
      cg.addSubtype(ValVar, ArgNode);
    }

    if (!I.getType()->isVoidTy()) {
      // for return value
      auto FormalRetVar = getCallRetTV(FuncVar);
      auto [RetNode, RNC] = cg.CG.createNodePair(FormalRetVar, I.getType());
      cg.addConstraint(FuncNode, RetNode, getCallRetLabel());

      // formal return -> actual return
      auto &ValVar = cg.getOrInsertNode(&I, nullptr, -1);
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
  // if has size hint, then we add forget size edge.
  auto AS = getAllocSize(&I);
  if (AS) {
    cg.CG.addEdge(Node, *cg.CG.getEndNode(), {retypd::ForgetSize{.Base = *AS}});
    cg.CG.addEdge(cg.CG.getReverseVariant(Node), *cg.CG.getEndNode(),
                  {retypd::ForgetSize{.Base = *AS}});
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitPHINode(PHINode &I) {
  // auto &Node =
  cg.createNodeCovariant(&I, nullptr, -1);
  // Defer constraints generation (and unification) to handlePHINodes
  phiNodes.push_back(&I);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto &DstVar = cg.getNode(I, nullptr, -1, retypd::Covariant);
    for (long i = 0; i < I->getNumIncomingValues(); i++) {
      auto *Src = I->getIncomingValue(i);
      auto &SrcVar = cg.getOrInsertNode(Src, I, i, retypd::Covariant);
      // src is a subtype of dest
      cg.addSubtype(SrcVar, DstVar);
    }
  }
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto SrcNode = cg.getNodeOrNull(Src, &I, 0, retypd::Covariant);
    if (SrcNode) {
      cg.addVarSubtype(&I, *SrcNode);
    }
    return;
  } else if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *Src = I.getOperand(0);
    auto SrcNode = cg.getNodeOrNull(Src, &I, 0, retypd::Covariant);
    if (SrcNode) {
      cg.addVarSubtype(&I, *SrcNode);
    }
    // cg.setPointer(SrcNode);
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
  //     auto &UintNodeOut = cg.getOrCreatePrim(
  //         "uint" + std::to_string(I.getType()->getIntegerBitWidth()),
  //         I.getType());
  //     cg.addSubtype(UintNodeOut, Node);
  //     // zext result is a number
  //     Node.getPNIVar()->setNonPtr();

  //     auto &NodeIn = cg.getOrInsertNode(I.getOperand(0), &I);
  //     auto &UintNodeIn = cg.getOrCreatePrim(
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
               << "ERROR: unhandled CastInst: " << I << "\n";
  std::abort();
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  // supress warnings for table gep
  if (Gep.getPointerOperand()->getName().startswith("table_")) {
    return;
  } else if (Gep.hasAllZeroIndices()) {
  }
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
  return llvm2c::getLLVMTypeSize(Elem, Ctx.pointer_size);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitStoreInst(
    StoreInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(I.getPointerOperand(), &I, 0, retypd::Covariant);
  if (!Node) {
    if (auto CE = dyn_cast<ConstantExpr>(I.getPointerOperand())) {
      if (CE->getOpcode() == Instruction::BitCast) {
        if (auto CE2 = dyn_cast<ConstantExpr>(CE->getOperand(0))) {
          CE = CE2;
        }
      }
      if (CE->getOpcode() == Instruction::GetElementPtr) {
        return;
      }
    }
  }
  if (!Node) {
    Node = &cg.getOrInsertNode(I.getPointerOperand(), &I, 0, retypd::Covariant);
  }

  auto &PtrVal = cg.getOrInsertNode(I.getPointerOperand(), &I, 1);
  auto BitSize = cg.getPointerElemSize(I.getPointerOperandType());
  auto &StoreVal =
      cg.getOrInsertNode(I.getValueOperand(), &I, 0, retypd::Contravariant);

  retypd::EdgeLabel SL = {
      retypd::RecallLabel{retypd::StoreLabel{.Size = BitSize}}};
  auto StoreNode = PtrVal.getLabelTarget(SL);
  if (StoreNode == nullptr) {
    auto NewKey = PtrVal.key;
    NewKey.Base = NewKey.Base.pushLabel({retypd::StoreLabel{.Size = BitSize}});
    NewKey.SuffixVariance = !NewKey.SuffixVariance;
    auto [SN, SNC] = cg.CG.createNodePairWithPNI(NewKey, StoreVal.getPNIVar());
    StoreNode = &SN;
    cg.addConstraint(PtrVal, *StoreNode, {retypd::StoreLabel{.Size = BitSize}});
    assert(PtrVal.getLabelTarget(SL) == StoreNode);
  }

  cg.addSubtype(StoreVal, *StoreNode);
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitLoadInst(LoadInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(I.getPointerOperand(), &I, 0, retypd::Covariant);
  if (!Node) {
    if (auto CE = dyn_cast<ConstantExpr>(I.getPointerOperand())) {
      if (CE->getOpcode() == Instruction::BitCast) {
        if (auto CE2 = dyn_cast<ConstantExpr>(CE->getOperand(0))) {
          CE = CE2;
        }
      }
      if (CE->getOpcode() == Instruction::GetElementPtr) {
        return;
      }
    }
  }

  auto &PtrVal = cg.getOrInsertNode(I.getPointerOperand(), &I, 0);
  auto BitSize = cg.getPointerElemSize(I.getPointerOperandType());
  auto &LoadNode = cg.createNodeCovariant(&I, nullptr, -1);

  if (TraceIds.count(LoadNode.getId())) {
    llvm::errs() << "TraceID=" << LoadNode.getId()
                 << " Node=" << toString(LoadNode.key)
                 << "RetypdGeneratorVisitor::visitLoadInst: Created node by "
                    "add .load to pointer operand: "
                 << I << "\n";
  }

  cg.addConstraint(PtrVal, LoadNode, {retypd::LoadLabel{.Size = BitSize}});
}

TypeVariable ConstraintsGenerator::addOffset(TypeVariable &dtv,
                                             OffsetRange Offset) {
  if (!dtv.getLabels().empty() && dtv.getLabels().back().isOffset()) {
    OffsetLabel LastLabel = *dtv.getLabels().back().getAs<OffsetLabel>();
    LastLabel.range = LastLabel.range + Offset;
    return dtv.popLabel().pushLabel({LastLabel});
  } else {
    return dtv.pushLabel({OffsetLabel{.range = Offset}});
  }
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
  auto Left = &getOrInsertNode(LHS, I, 0);
  auto Right = &getOrInsertNode(RHS, I, 1);
  auto Res = &getOrInsertNode(I, nullptr, -1);
  if (Left->getPNIVar()->isPNRelated() || Right->getPNIVar()->isPNRelated()) {
    PG->addAddCons(Left, Right, Res, I);
  }
}

void ConstraintsGenerator::addSubConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            BinaryOperator *I) {
  auto Left = &getOrInsertNode(LHS, I, 0);
  auto Right = &getOrInsertNode(RHS, I, 1);
  auto Res = &getOrInsertNode(I, nullptr, -1);
  if (Left->getPNIVar()->isPNRelated() || Right->getPNIVar()->isPNRelated()) {
    PG->addAddCons(Left, Right, Res, I);
  }
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
    // llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
    //              << "Warn: And op without constant: " << I << "\n";
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
    // llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
    //              << "Warn: Or op without constant: " << I << "\n";
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
    if (N.getPNIVar()->isPNRelated()) {
      N.getPNIVar()->setNonPtr();
    }
    auto &SintNode = cg.getOrCreatePrim(
        retypd::getNameForInt("sint", I->getType()), I->getType());
    cg.addSubtype(SintNode, N);
    return true;
  } else if (strEq(ty, "uint")) {
    if (N.getPNIVar()->isPNRelated()) {
      N.getPNIVar()->setNonPtr();
    }
    auto &UintNode = cg.getOrCreatePrim(
        retypd::getNameForInt("uint", I->getType()), I->getType());
    cg.addSubtype(UintNode, N);
    return true;
  } else if (strEq(ty, "int")) {
    if (N.getPNIVar()->isPNRelated()) {
      N.getPNIVar()->setNonPtr();
    }
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
    auto &SintNode = cg.getOrCreatePrim(
        retypd::getNameForInt("sint", Op->getType()), Op->getType());
    cg.addSubtype(N, SintNode);
    return true;
  } else if (strEq(ty, "uint")) {
    N.getPNIVar()->setNonPtrIfRelated();
    auto &UintNode = cg.getOrCreatePrim(
        retypd::getNameForInt("uint", Op->getType()), Op->getType());
    cg.addSubtype(N, UintNode);
    return true;
  } else if (strEq(ty, "int")) {
    N.getPNIVar()->setNonPtrIfRelated();
    return true;
  }
  return false;
}

// =========== end: other insts ===========

class CGAnnotationWriter : public llvm::AssemblyAnnotationWriter {
  // ConstraintsGenerator &CG;
  AllGraphs &AG;
  int Level = 0;

  std::shared_ptr<ConstraintsGenerator> getFuncCG(const llvm::Function *F) {
    auto CGN = AG.CG->getOrInsertFunction(F);
    if (!AG.Func2SCCIndex.count(CGN)) {
      return nullptr;
    }
    std::shared_ptr<ConstraintsGenerator> CG = nullptr;
    auto Ind = AG.Func2SCCIndex.at(CGN);
    if (Level == 0) {
      CG = AG.AllSCCs.at(Ind).BottomUpGenerator;
    } else if (Level == 1) {
      CG = AG.AllSCCs.at(Ind).TopDownGenerator;
    } else if (Level == 2) {
      CG = AG.AllSCCs.at(Ind).SketchGenerator;
    }
    return CG;
  }

  void emitFunctionAnnot(const llvm::Function *F,
                         llvm::formatted_raw_ostream &OS) override {
    // auto F1 = const_cast<llvm::Function *>(F);
    std::shared_ptr<ConstraintsGenerator> CG = getFuncCG(F);
    if (!CG) {
      return;
    }
    OS << "; ";
    if (!F->getReturnType()->isVoidTy()) {
      auto N = CG->getNodeOrNull(
          ReturnValue{.Func = const_cast<llvm::Function *>(F)}, nullptr, -1,
          retypd::Covariant);
      OS << (N ? N->str() : "none");
      OS << " <- ";
    }
    OS << "(";
    for (auto &Arg : F->args()) {
      auto N = CG->getNodeOrNull(const_cast<llvm::Argument *>(&Arg), nullptr,
                                 -1, retypd::Covariant);
      OS << (N ? N->str() : "none") << ", ";
    }
    OS << ")";
    OS << "\n";
  }

  void printInfoComment(const llvm::Value &V,
                        llvm::formatted_raw_ostream &OS) override {

    const Instruction *Instr = dyn_cast<Instruction>(&V);
    if (Instr == nullptr) {
      return;
    }
    auto F = Instr->getFunction();
    std::shared_ptr<ConstraintsGenerator> CG = getFuncCG(F);
    if (!CG) {
      return;
    }

    OS << "; ";
    if (!V.getType()->isVoidTy()) {
      auto N = CG->getNodeOrNull(const_cast<llvm::Value *>(&V), nullptr, -1,
                                 retypd::Covariant);
      OS << (N ? N->str() : "none");
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
  CGAnnotationWriter(AllGraphs &AG, int Level) : AG(AG), Level(Level) {}
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

void TypeRecovery::printAnnotatedModule(const llvm::Module &M, std::string path,
                                        int level) {
  std::error_code EC;
  llvm::raw_fd_ostream os(path, EC);
  if (EC) {
    std::cerr << "Cannot open output file: " << path << std::endl;
    std::cerr << EC.message() << std::endl;
    std::abort();
  }
  CGAnnotationWriter AW(AG, level);
  M.print(os, &AW);
}

std::shared_ptr<ConstraintsGenerator> ConstraintsGenerator::fromConstraints(
    TypeRecovery &Ctx, std::set<llvm::Function *> SCCs,
    const retypd::ConstraintSummary &Summary) {
  if (Summary.Cons.empty()) {
    return nullptr;
  }
  std::string SCCNames = getFuncSetName(SCCs);
  auto Ret = std::make_shared<ConstraintsGenerator>(Ctx, SCCNames, SCCs);
  Ret->CG.instantiateConstraints(Summary);
  Ret->fixSCCFuncMappings();
  // std::map<CGNode *, retypd::Variance> Initial;
  // for (auto &Ent : Ret->V2N) {
  //   Initial[Ent.second] = retypd::Covariant;
  // }
  // for (auto &Ent : Ret->V2NContra) {
  //   Initial[Ent.second] = retypd::Contravariant;
  // }
  // Ret->CG.markVariance(&Initial);
  // Ret->makeSymmetry();
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

void ConstraintsGenerator::makeSymmetry() {
  // Fix node symmetry.
  for (auto &N : CG.Nodes) {
    if (N.isStartOrEnd()) {
      continue;
    }
    CGNode *NC = nullptr;
    if (CG.RevVariance.count(&N)) {
      NC = &CG.getReverseVariant(N);
    } else {
      NC = &CG.createNodeWithPNI(retypd::MakeReverseVariant(N.key),
                                 N.getPNIVar());
      CG.RevVariance.insert({&N, NC});
      CG.RevVariance.insert({NC, &N});
    }
  }
  // Fix edge symmetry
  for (auto &N : CG.Nodes) {
    for (auto &Edge : N.outEdges) {
      if (Edge.getLabel().isRecallBase() || Edge.getLabel().isForgetBase()) {
        continue;
      }
      auto &NC = CG.getReverseVariant(N);
      auto &T = const_cast<CGNode &>(Edge.getTargetNode());
      auto &TC = CG.getReverseVariant(T);
      auto &Label = Edge.getLabel();
      if (Label.isOne()) {
        // 这里不一定要recall labels forget
        // labels。因为summary里面可能有特殊的节点。 addSubtype(N, T);
        CG.addEdge(TC, NC, {retypd::One{}});
      } else if (auto RL = Label.getAs<retypd::RecallLabel>()) {
        auto FL = toForget(*RL);
        CG.addEdge(T, N, {FL});
        CG.addEdge(NC, TC, {*RL});
        CG.addEdge(TC, NC, {FL});
      } else if (auto FL = Label.getAs<retypd::ForgetLabel>()) {
        auto RL = toRecall(*FL);
        CG.addEdge(T, N, {RL});
        CG.addEdge(NC, TC, {*FL});
        CG.addEdge(TC, NC, {RL});
      } else if (auto RB = Label.getAs<retypd::RecallBase>()) {
        auto FB = retypd::toForget(*RB);
        CG.addEdge(T, N, {FB});
        CG.addEdge(NC, TC, {*RB});
        CG.addEdge(TC, NC, {FB});
      } else if (auto FB = Label.getAs<retypd::ForgetBase>()) {
        auto RB = retypd::toRecall(*FB);
        CG.addEdge(T, N, {RB});
        CG.addEdge(NC, TC, {*FB});
        CG.addEdge(TC, NC, {RB});
      } else {
        std::cerr << "Unhandled type!";
        std::abort();
      }
    }
  }
}

} // namespace notdec
