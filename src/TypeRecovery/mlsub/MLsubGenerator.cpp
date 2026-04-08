
#include "notdec/TypeRecovery/mlsub/MLsubGenerator.h"
#include "Utils/CallGraphDotInfo.h"
#include "binarysub/binarysub-core.h"
#include "binarysub/binarysub-infer.h"
#include "binarysub/binarysub.h"
#include "notdec-llvm2c/Interface.h"
#include "notdec-llvm2c/Interface/HType.h"
#include "notdec-llvm2c/Interface/StructManager.h"
#include "notdec-llvm2c/Utils.h"
#include "notdec/TypeRecovery/Lattice.h"
#include "notdec/TypeRecovery/mlsub/TypeBuilder.h"
#include "notdec/Utils/AllSCCIterator.h"
#include "notdec/Utils/SingleNodeSCCIterator.h"
#include "notdec/Utils/Utils.h"

#include <cassert>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <algorithm>
#include <memory>
#include <string>

using namespace llvm;

#define DEBUG_TYPE "mlsub_generator"

namespace notdec::mlsub {

namespace {

constexpr llvm::StringLiteral kValueTypesFile = "ValueTypes.txt";

void appendDebugValueTypes(
    llvm::StringRef DebugDir, llvm::StringRef SCCName,
    DSUMap<ExtValuePtr, SimpleType> &V2N,
    const std::set<ExtValuePtr> &ContraVariantValues,
    const std::map<binarysub::PolarVar, binarysub::UTypePtr> &Res,
    bool SolveMemory, const binarysub::PolarVar &PolMem) {
  std::error_code EC;
  llvm::raw_fd_ostream Out(join(DebugDir.str(), kValueTypesFile.str()), EC,
                           llvm::sys::fs::OF_Append);
  if (EC) {
    llvm::errs() << "Error printing to " << kValueTypesFile << ", "
                 << EC.message() << "\n";
    return;
  }

  auto getPol = [&](ExtValuePtr V) { return !ContraVariantValues.count(V); };
  std::vector<std::string> Lines;
  Lines.reserve(V2N.size() + (SolveMemory ? 1 : 0));

  for (const auto &Ent : V2N) {
    bool Pol = getPol(Ent.first);
    auto It = Res.find(binarysub::PolarVar{.var = Ent.second, .pos = Pol});
    std::string UTypeStr = "<null>";
    if (It != Res.end() && It->second) {
      UTypeStr = binarysub::printType(It->second);
    }
    std::string Line = Pol ? "[+]" : "[-]";
    Line += " ";
    Line += toString(Ent.first, true);
    Line += " => ";
    Line += UTypeStr;
    Lines.push_back(std::move(Line));
  }

  if (SolveMemory) {
    auto It = Res.find(PolMem);
    std::string UTypeStr = "<null>";
    if (It != Res.end() && It->second) {
      UTypeStr = binarysub::printType(It->second);
    }
    Lines.push_back("[memory] <memory> => " + UTypeStr);
  }

  std::sort(Lines.begin(), Lines.end());

  Out << "## SCC: " << SCCName << "\n";
  for (const auto &Line : Lines) {
    Out << Line << "\n";
  }
  Out << "\n";
}

} // namespace

void MLsubRecovery::run() {
  auto &M = const_cast<llvm::Module &>(Mod);
  // set global pointer size variable for binarysub
  binarysub::pointer_size = PointerSize;

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

    llvm::raw_fd_ostream ValueTypes(join(DebugDir, kValueTypesFile.str()), EC);
    if (EC) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Cannot open output file " << kValueTypesFile.str() << ": ";
      std::cerr << EC.message() << std::endl;
      std::abort();
    }
    ValueTypes << "# Final Value -> binarysub UType mapping\n\n";
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

  bottomUpPhase();

  topDownPhase();

  std::cerr << "Constraint generation done! SCC count:" << AG.AllSCCs.size()
            << "\n";
}

void MLsubRecovery::bottomUpPhase() {
  // Iterate bottom up.
  for (std::size_t Ind = AG.AllSCCs.size(); Ind-- > 0;) {
    auto &Data = AG.AllSCCs.at(Ind);
    Data.Generator = std::make_shared<ConstraintsGenerator>(
        Data.SCCName, PointerSize, Data.SCCSet, MemoryType, Data.level);
    auto &G = Data.Generator;
    // insert ContraVariantValues
    if (Ind == 0) {
      // // add main arguments
      // if (auto Main = Mod.getFunction("main")) {
      //   for (auto &Arg: Main->args()) {
      //     G->ContraVariantValues.insert(&Arg);
      //   }
      // }
      // TODO parameters with unknown type
    } else {
      // find SCC interface functions.
      for (auto N : Data.Nodes) {
        if (AG.Callee2Callers.count(N) == 0) {
          continue;
        }
        for (auto Caller : AG.Callee2Callers[N]) {
          if (auto F = Caller->getFunction()) {
            for (auto &Arg : F->args()) {
              G->ContraVariantValues.insert(&Arg);
            }
          }
        }
      }
    }

    G->run();

    // create poly schemes and instantiate for unhandled calls.
    for (auto &Ent : Data.Generator->unhandledCalls) {
      auto F = Ent.first->getCalledFunction();
      auto Ind2 = AG.Func2SCCIndex.at(AG.CG->getOrInsertFunction(F));
      assert(Ind2 > Ind);
      auto &TData = AG.AllSCCs.at(Ind2);
      auto TargetG = TData.Generator;
      auto TargetFTy = TargetG->getNodeOrNull(F, nullptr, -1);
      auto PolyScheme = binarysub::TypeScheme(
          binarysub::PolymorphicType(TData.level, TargetFTy));
      auto TargetLevel = binarysub::level_of(TargetFTy);
      assert(TargetLevel >= 0);
      assert(TData.level == static_cast<unsigned int>(TargetLevel));
      assert(TData.level >= Data.level);
      auto InsFunc = PolyScheme.instantiate(Data.level);
      Data.Generator->addSubtype(InsFunc, Ent.second);
    }
    Data.Generator->unhandledCalls.clear();
  }
}

void ConstraintsGenerator::genTypes(ast::HTypeContext &HCtx,
                                    const llvm::DataLayout &DL,
                                    bool SolveMemory) {
  binarysub::TypeSimplifier Ts;
  using binarysub::PolarVar;
  std::set<PolarVar> Tys;
  auto getPol = [&](ExtValuePtr V) { return !ContraVariantValues.count(V); };
  for (auto &Ent : V2N) {
    Tys.insert(PolarVar{.var = Ent.second, .pos = getPol(Ent.first)});
  }
  auto PolMem = PolarVar{.var = MemoryType, .pos = false};
  if (SolveMemory) {
    Tys.insert(PolMem);
  }
  std::map<PolarVar, binarysub::UTypePtr> Res = Ts.bulkSimplify(Tys, false);

  // Create TypeBuilder context and builder
  TypeBuilderContext TBCtx(HCtx, DL);
  TypeBuilder TB(TBCtx);

  for (auto &Ent : V2N) {
    auto It = Res.find(PolarVar{.var = Ent.second, .pos = getPol(Ent.first)});
    ast::HType *Converted = nullptr;
    if (It != Res.end() && It->second) {
      Converted = TB.convert(It->second);
    }
    ValueTypes.insert({Ent.first, Converted});
  }
  if (SolveMemory) {
    auto MemUTy = Res.at(PolMem);
    ValueTypes.insert({nullptr, TB.convert(MemUTy)});
  }

  if (auto DebugDir = std::getenv("NOTDEC_TYPE_RECOVERY_DEBUG_DIR")) {
    appendDebugValueTypes(DebugDir, Name, V2N, ContraVariantValues, Res,
                          SolveMemory, PolMem);
  }
}

void ConstraintsGenerator::releaseBinarysubState() {
  for (auto &Ent : V2N) {
    binarysub::release_type_graph(Ent.second);
  }
  binarysub::release_type_graph(MemoryType);
  V2N = {};
  ContraVariantValues.clear();
  unhandledCalls.clear();
}

void MLsubRecovery::genASTTypes(llvm::Module &M) {
  ResultVal = std::make_unique<Result>();
  // 合并所有类型到一个大的ValueTypes里面。
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    for (auto &Ent : Data.Generator->ValueTypes) {
      auto It = ResultVal->ValueTypes.insert(Ent);
      assert(It.second && "Duplicated Entry?");
    }
  }
  ResultVal->HTCtx = HCtx;
  // handle Memory type.
  auto Mem = AG.AllSCCs.at(0).Generator->ValueTypes.at(nullptr);
  ResultVal->MemoryType = Mem;
  if (Mem->isRecordType()) {
    ResultVal->MemoryDecl = Mem->getAsRecordDecl();
  }
}

void MLsubRecovery::topDownPhase() {
  // TODO 怎么处理TopDown的类型传递？设置多态参数的类型？
  // TODO 对于每个SCCData的所有Caller，都instantiate到 -x level？
  // 也许我该基于最后都优化完毕之后的CompactType？？

  if (!HCtx) {
    HCtx = std::make_shared<ast::HTypeContext>();
  }
  for (std::size_t Ind = 0; Ind < AG.AllSCCs.size(); ++Ind) {
    auto &Data = AG.AllSCCs.at(Ind);
    // 尝试运行简化算法，保存到ValueTypes里面。
    // solve memory if ind == 0
    Data.Generator->genTypes(*HCtx, Mod.getDataLayout(), Ind == 0);
  }
  for (auto &Data : AG.AllSCCs) {
    Data.Generator->releaseBinarysubState();
  }
}

void MLsubRecovery::prepareSCC(CallGraph &CG) {
  AG.CG = &CG;
  AG.AllSCCs.clear();
  AG.Func2SCCIndex.clear();
  AG.Callee2Callers.clear();

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

  auto LevelOverrideFile = std::getenv("NOTDEC_LEVEL_OVERRIDE");
  std::map<std::string, unsigned int> LevelOverrides;
  if (LevelOverrideFile) {
    // The override file provides per-function lower bounds. We still solve for
    // the minimal valid level that satisfies call-graph and polymorphism
    // constraints, so overly small user values are automatically raised.
    auto Content = readFileToString(LevelOverrideFile);
    auto ValE = json::parse(Content);
    if (!ValE) {
      assert(false && "JSON parse failed, invalid NOTDEC_LEVEL_OVERRIDE content");
    }
    auto ValObj = ValE->getAsObject();
    assert(ValObj != nullptr);
    for (auto &Ent : *ValObj) {
      auto Level = Ent.second.getAsInteger();
      assert(Level && *Level >= 0 &&
             "NOTDEC_LEVEL_OVERRIDE values must be non-negative integers");
      LevelOverrides[Ent.first.str()] = static_cast<unsigned int>(*Level);
    }
  }

  all_scc_iterator<CallGraph *> CGI = notdec::scc_begin(AG.CG);
  // 把CGI遍历的结果都顺序保存到vector里
  std::vector<std::vector<CallGraphNode *>> SCCResults;
  for (; !CGI.isAtEnd(); ++CGI) {
    SCCResults.push_back(*CGI);
  }
  // 遍历所有的CallGraphNode，然后构建一个反向的，从callee到所有caller的map
  for (auto &KV : *AG.CG) {
    CallGraphNode *Caller = KV.second.get();
    for (auto &CallRecord : *Caller) {
      CallGraphNode *Callee = CallRecord.second;
      AG.Callee2Callers[Callee].insert(Caller);
    }
  }

  auto isAllDeclarationAndIntrinsics =
      [](const std::vector<CallGraphNode *> &NodeVec) -> bool {
    for (auto *CGN : NodeVec) {
      if (auto *Fn = CGN->getFunction()) {
        if (!Fn->isDeclaration() || !Fn->isIntrinsic()) {
          return false;
        }
      }
    }
    return true;
  };

  auto hasPolymorphic =
      [&](const std::vector<CallGraphNode *> &NodeVec) -> bool {
    for (auto *CGN : NodeVec) {
      if (auto *Fn = CGN->getFunction()) {
        if (Fn->hasName() && PolyFuncs.count(Fn->getName().str())) {
          return true;
        }
        if (isPolymorphic(Fn)) {
          return true;
        }
      }
    }
    return false;
  };

  struct RawSCCInfo {
    std::vector<CallGraphNode *> Nodes;
    std::set<std::size_t> Preds;
    std::set<std::size_t> Succs;
    bool IsPolymorphic = false;
    unsigned int UserLevelLowerBound = 0;
    unsigned int Level = 0;
    std::size_t TopoPosition = 0;
  };

  // Phase 1: work on the original call-graph SCC DAG first. Levels are solved
  // on raw SCCs before any same-level groups are merged.
  std::vector<RawSCCInfo> RawSCCs;
  std::map<CallGraphNode *, std::size_t> Node2RawSCCIndex;
  for (auto It = SCCResults.rbegin(); It != SCCResults.rend(); ++It) {
    const std::vector<CallGraphNode *> &NodeVec = *It;
    if (isAllDeclarationAndIntrinsics(NodeVec)) {
      continue;
    }
    std::size_t RawIndex = RawSCCs.size();
    RawSCCs.push_back(RawSCCInfo{.Nodes = NodeVec});
    for (auto *Node : NodeVec) {
      Node2RawSCCIndex[Node] = RawIndex;
    }
  }

  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    auto &Raw = RawSCCs[RawIndex];
    Raw.IsPolymorphic = hasPolymorphic(Raw.Nodes);
    for (auto *Node : Raw.Nodes) {
      auto *Fn = Node->getFunction();
      if (Fn == nullptr || !Fn->hasName()) {
        continue;
      }
      auto It = LevelOverrides.find(Fn->getName().str());
      if (It != LevelOverrides.end()) {
        Raw.UserLevelLowerBound =
            std::max(Raw.UserLevelLowerBound, It->second);
      }
    }
  }

  for (auto &[Callee, Callers] : AG.Callee2Callers) {
    auto CalleeIt = Node2RawSCCIndex.find(Callee);
    if (CalleeIt == Node2RawSCCIndex.end()) {
      continue;
    }
    std::size_t CalleeIndex = CalleeIt->second;
    for (auto *Caller : Callers) {
      auto CallerIt = Node2RawSCCIndex.find(Caller);
      if (CallerIt == Node2RawSCCIndex.end()) {
        continue;
      }
      std::size_t CallerIndex = CallerIt->second;
      if (CallerIndex == CalleeIndex) {
        continue;
      }
      RawSCCs[CallerIndex].Succs.insert(CalleeIndex);
      RawSCCs[CalleeIndex].Preds.insert(CallerIndex);
    }
  }

  // Topologically order the SCC DAG so every caller level is known before its
  // callees are processed.
  std::vector<std::size_t> TopoOrder;
  std::vector<unsigned int> InDegree(RawSCCs.size(), 0);
  std::set<std::size_t> Ready;
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    InDegree[RawIndex] = RawSCCs[RawIndex].Preds.size();
    if (InDegree[RawIndex] == 0) {
      Ready.insert(RawIndex);
    }
  }
  while (!Ready.empty()) {
    std::size_t RawIndex = *Ready.begin();
    Ready.erase(Ready.begin());
    RawSCCs[RawIndex].TopoPosition = TopoOrder.size();
    TopoOrder.push_back(RawIndex);
    for (std::size_t SuccIndex : RawSCCs[RawIndex].Succs) {
      assert(InDegree[SuccIndex] > 0);
      --InDegree[SuccIndex];
      if (InDegree[SuccIndex] == 0) {
        Ready.insert(SuccIndex);
      }
    }
  }
  assert(TopoOrder.size() == RawSCCs.size() &&
         "SCC condensation graph must be a DAG");

  for (std::size_t RawIndex : TopoOrder) {
    auto &Raw = RawSCCs[RawIndex];
    unsigned int BaseLevel = 0;
    for (std::size_t PredIndex : Raw.Preds) {
      BaseLevel = std::max(BaseLevel, RawSCCs[PredIndex].Level);
    }
    // Polymorphic SCCs force a summary boundary, so calls into them instantiate
    // from one level below.
    if (Raw.IsPolymorphic) {
      ++BaseLevel;
    }
    Raw.Level = std::max(BaseLevel, Raw.UserLevelLowerBound);
  }

  // Phase 2: collapse the maximal same-level regions. We union along same-level
  // edges only; after that, inter-group edges always go from lower to higher
  // levels, so the merged graph remains a DAG.
  std::vector<std::size_t> Parent(RawSCCs.size());
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    Parent[RawIndex] = RawIndex;
  }
  auto FindRoot = [&](std::size_t Index) {
    std::size_t Root = Index;
    while (Parent[Root] != Root) {
      Root = Parent[Root];
    }
    while (Parent[Index] != Index) {
      std::size_t Next = Parent[Index];
      Parent[Index] = Root;
      Index = Next;
    }
    return Root;
  };
  auto Union = [&](std::size_t LHS, std::size_t RHS) {
    LHS = FindRoot(LHS);
    RHS = FindRoot(RHS);
    if (LHS == RHS) {
      return;
    }
    if (LHS > RHS) {
      std::swap(LHS, RHS);
    }
    Parent[RHS] = LHS;
  };
  for (std::size_t RawIndex = 0; RawIndex < RawSCCs.size(); ++RawIndex) {
    for (std::size_t SuccIndex : RawSCCs[RawIndex].Succs) {
      if (RawSCCs[RawIndex].Level == RawSCCs[SuccIndex].Level) {
        Union(RawIndex, SuccIndex);
      }
    }
  }

  struct GroupInfo {
    std::size_t Root = 0;
    unsigned int Level = 0;
    std::size_t FirstTopoPosition = 0;
  };

  std::map<std::size_t, std::vector<std::size_t>> Groups;
  std::map<std::size_t, std::size_t> FirstTopoPosition;
  for (std::size_t RawIndex : TopoOrder) {
    std::size_t Root = FindRoot(RawIndex);
    Groups[Root].push_back(RawIndex);
    if (!FirstTopoPosition.count(Root)) {
      FirstTopoPosition[Root] = RawSCCs[RawIndex].TopoPosition;
    }
  }

  std::vector<GroupInfo> OrderedGroups;
  OrderedGroups.reserve(Groups.size());
  for (auto &[Root, Members] : Groups) {
    (void)Members;
    OrderedGroups.push_back(GroupInfo{
        .Root = Root,
        .Level = RawSCCs[Root].Level,
        .FirstTopoPosition = FirstTopoPosition.at(Root),
    });
  }
  std::sort(OrderedGroups.begin(), OrderedGroups.end(),
            [](const GroupInfo &LHS, const GroupInfo &RHS) {
              if (LHS.Level != RHS.Level) {
                return LHS.Level < RHS.Level;
              }
              return LHS.FirstTopoPosition < RHS.FirstTopoPosition;
            });

  std::vector<SCCData> &AllSCCs = AG.AllSCCs;
  std::map<CallGraphNode *, std::size_t> &Func2SCCIndex = AG.Func2SCCIndex;
  for (const auto &Group : OrderedGroups) {
    SCCData Data;
    Data.level = Group.Level;
    for (std::size_t RawIndex : Groups.at(Group.Root)) {
      assert(RawSCCs[RawIndex].Level == Data.level);
      Data.Nodes.insert(Data.Nodes.end(), RawSCCs[RawIndex].Nodes.begin(),
                        RawSCCs[RawIndex].Nodes.end());
    }
    AllSCCs.push_back(Data);
  }

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
      *SCCsCatalog << "SCC" << SCCIndex << "," << Name
                   << " (level = " << Data.level << ")" << "\n";
    }
  }
}

SimpleType ConstraintsGenerator::convertSimpleType(ExtValuePtr Val,
                                                   llvm::User *User,
                                                   long OpInd) {
  llvmValue2ExtVal(Val, User, OpInd);
  if (auto V = std::get_if<llvm::Value *>(&Val)) {
    return convertSimpleTypeVal(*V, User, OpInd);
  } else if (std::get_if<ReturnValue>(&Val)) {
    return binarysub::make_variable(lvl, getSize(Val));
  } else if (auto IC = std::get_if<UConstant>(&Val)) {
    assert(User != nullptr && "RetypdGenerator::getTypeVar: User is Null!");
    return convertSimpleTypeVal(IC->Val, IC->User, IC->OpInd);
  } else if (auto CA = std::get_if<ConstantAddr>(&Val)) {
    // as field access.
    auto res = binarysub::fresh_variable(lvl, getSize(Val));
    std::vector<std::pair<std::string, SimpleType>> fields;
    fields.push_back(
        {OffsetRange{.offset = CA->Val->getSExtValue()}.str(), res});
    addSubtype(MemoryType, binarysub::make_record(std::move(fields)));
    addSubtype(res, binarysub::make_record({}));
    return res;
  }
  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: ConstraintsGenerator::convertSimpleType unhandled "
                  "type of ExtValPtr\n";
  std::abort();
}

SimpleType ConstraintsGenerator::convertSimpleTypeVal(Value *Val,
                                                      llvm::User *User,
                                                      long OpInd) {
  if (Val->getType()->isIntegerTy(1)) {
    return binarysub::make_primitive("bool", 1);
  } else if (Val->getType()->isFloatingPointTy()) {
    return binarysub::make_primitive("float", getSize(Val));
  }

  if (Constant *C = dyn_cast<Constant>(Val)) {
    // check for constantExpr
    if (auto CE = dyn_cast<ConstantExpr>(C)) {
      // ignore bitcast ConstantExpr
      if (CE->getOpcode() == Instruction::BitCast) {
        return convertSimpleType(CE->getOperand(0), CE, 0);
      } else if (CE->getOpcode() == Instruction::IntToPtr) {
        if (isa<ConstantInt>(CE->getOperand(0))) {
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
                if (isa<ConstantInt>(CE->getOperand(2))) {
                  assert(false && "TODO");
                }
              }
            }
          }
        }
      } else {
        llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                     << "ERROR: ConstraintsGenerator::convertSimpleTypeVal "
                        "unhandled ConstantExpr: "
                     << *C << "\n";
      }
    } else if (auto gv = dyn_cast<GlobalValue>(C)) { // global variable
      return binarysub::make_variable(lvl, getSize(gv));
      // if (gv == Ctx.StackPointer) {
      //   std::cerr
      //       << "Error: convertTypeVarVal: direct use of stack pointer?,
      //       ensure "
      //          "StackAllocationRecovery is run before, Or add external
      //          summary " "for this function.\n";
      //   return makeTv(Ctx.TRCtx, ValueNamer::getName());
      // } else if (auto Func = dyn_cast<Function>(C)) {
      //   return makeTv(Ctx.TRCtx, getFuncTvName(Func));
      // }
      // return makeTv(Ctx.TRCtx, gv->getName().str());
    } else if (isa<ConstantInt>(C) || isa<ConstantFP>(C)) {
      if (auto CI = dyn_cast<ConstantInt>(C)) {
        return binarysub::make_variable(lvl, getSize(CI, User, OpInd));
      }
      assert(false && "TODO");
      // return makeTv(Ctx.TRCtx, ValueNamer::getName("constant_"));
      // auto Ty = C->getType();
      // return getLLVMTypeVar(Ctx.TRCtx, Ty);
    } else if (isa<ConstantPointerNull>(C)) {
      assert(false && "TODO");
      // return makeTv(Ctx.TRCtx, ValueNamer::getName("null_"));
    } else if (isa<UndefValue>(C)) {
      assert(false && "TODO");
      // return makeTv(Ctx.TRCtx, ValueNamer::getName("undef_"));
    }
    llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
                 << "ERROR: ConstraintsGenerator::convertSimpleTypeVal "
                    "unhandled type of constant: "
                 << *C << "\n";
    std::abort();
  } else if (auto arg = dyn_cast<Argument>(Val)) { // for function argument
    return binarysub::make_variable(lvl, getSize(arg));
  }

  if (auto *I = dyn_cast<Instruction>(Val)) {
    return binarysub::make_variable(lvl, getSize(I));
  }
  llvm::errs()
      << __FILE__ << ":" << __LINE__ << ": "
      << "WARN: ConstraintsGenerator::convertSimpleTypeVal unhandled value: "
      << *Val << "\n";
  return binarysub::make_variable(lvl, getSize(Val));
}

// #region ConstraintsGenerator::MLsubVisitor

// Helper functions
static bool isWithOverflowIntrinsicSigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::sadd_with_overflow ||
      ID == Intrinsic::ssub_with_overflow ||
      ID == Intrinsic::smul_with_overflow) {
    return true;
  }
  return false;
}

static bool isWithOverflowIntrinsicUnsigned(llvm::Intrinsic::ID ID) {
  if (ID == Intrinsic::uadd_with_overflow ||
      ID == Intrinsic::usub_with_overflow ||
      ID == Intrinsic::umul_with_overflow) {
    return true;
  }
  return false;
}

static inline void ensureSequence(Value *&Src1, Value *&Src2) {
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      llvm::isa<llvm::ConstantInt>(Src2)) {
    assert(false && "Constant at both sides. Run Optimization first!");
  }
  if (llvm::isa<llvm::ConstantInt>(Src1) &&
      !llvm::isa<llvm::ConstantInt>(Src2)) {
    // because of InstCombine canonical form, this should not happen?
    assert(false &&
           "Constant cannot be at the left side. Run InstCombine first.");
    std::swap(Src1, Src2);
  }
}

// Visitor method implementations
void ConstraintsGenerator::MLsubVisitor::visitExtractValueInst(
    ExtractValueInst &I) {
  if (auto Call = dyn_cast<CallBase>(I.getAggregateOperand())) {
    if (auto Target = Call->getCalledFunction()) {
      if (Target->isIntrinsic()) {
        // 这里判断返回值是不是那种extract
        // value的东西，根据llvm类型直接设置为数字类型
        auto Ind = I.getIndices()[0];
        if (Ind == 0) {
          if (isWithOverflowIntrinsicSigned(Target->getIntrinsicID())) {
            auto N = cg.createNode(&I, nullptr, -1);
            assert(false && "TODO: PNI setNonPtr");
            assert(false && "TODO: getOrCreatePrim");
            assert(false && "TODO: addSubtype");
            return;
          } else if (isWithOverflowIntrinsicUnsigned(
                         Target->getIntrinsicID())) {
            auto N = cg.createNode(&I, nullptr, -1);
            assert(false && "TODO: PNI setNonPtr");
            assert(false && "TODO: getOrCreatePrim");
            assert(false && "TODO: addSubtype");
            return;
          }
        } else if (Ind == 1) {
          assert(I.getType()->isIntegerTy(1));
          cg.createNode(&I, nullptr, -1);
          return;
        }
      }
    }
  }
  assert(false && "TODO: ExtractValueInst general case");
}

void ConstraintsGenerator::MLsubVisitor::visitCastInst(CastInst &I) {
  if (isa<BitCastInst>(I)) {
    // ignore cast, propagate the type of the operand.
    auto *Src = I.getOperand(0);
    auto SrcNode = cg.getNodeOrNull(Src, &I, 0);
    if (SrcNode) {
      cg.addRemapType(&I, nullptr, -1, SrcNode);
    }
    return;
  } else if (isa<PtrToIntInst, IntToPtrInst, BitCastInst>(I)) {
    // ignore cast, view as assignment.
    auto *Src = I.getOperand(0);
    auto SrcNode = cg.getNodeOrNull(Src, &I, 0);
    if (SrcNode) {
      cg.addRemapType(&I, nullptr, -1, SrcNode);
    }
    return;
  } else if (isa<TruncInst, ZExtInst, SExtInst, FPToUIInst, FPToSIInst,
                 UIToFPInst, SIToFPInst, FPTruncInst, FPExtInst>(&I)) {
    visitInstruction(I);
    return;
  }

  llvm::errs() << __FILE__ << ":" << __LINE__ << ": "
               << "ERROR: unhandled CastInst: " << I << "\n";
  std::abort();
}

bool ConstraintsGenerator::MLsubVisitor::isHeapAllocationCall(
    llvm::CallBase &I) {
  // TODO add more func names
  if (auto F = I.getCalledFunction()) {
    if (F->getName() == "malloc") {
      return true;
    }
  }
  return false;
}

bool ConstraintsGenerator::MLsubVisitor::handleIntrinsicCall(
    llvm::CallBase &I) {
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

void ConstraintsGenerator::MLsubVisitor::visitCallBase(CallBase &I) {
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

  // TODO if is allocation function, treat as alloca inst
  if (isHeapAllocationCall(I)) {
    auto Node = cg.createNode(&I, nullptr, -1);
    // set as pointer type
    cg.setPointer(&I, nullptr, -1);
    cg.ContraVariantValues.insert(&I);
  } else if (handleIntrinsicCall(I)) {
    return;
  } else {
    // Call within the SCC:
    auto Func = Target;
    std::vector<SimpleType> Args;
    for (unsigned i = 0; i < I.arg_size(); ++i) {
      auto ValVar = cg.getOrInsertNode(I.getArgOperand(i), &I, i);
      Args.push_back(ValVar);
    }
    SimpleType Ret = nullptr;
    if (!I.getType()->isVoidTy()) {
      Ret = cg.getOrInsertNode(&I, nullptr, -1);
    }
    auto ActualFunc = binarysub::make_function(Args, Ret);
    if (cg.SCCs.count(Target)) {
      auto F = cg.getNodeOrNull(Func, nullptr, -1);
      cg.addSubtype(F, ActualFunc);
    } else {
      // create and save to CallToInstance map. instance with summary later
      auto It = cg.unhandledCalls.insert({&I, ActualFunc});
      assert(It.second && "Insert unhandledCalls failed!");
    }
  }
}

void ConstraintsGenerator::MLsubVisitor::visitReturnInst(ReturnInst &I) {
  auto *SrcVal = I.getReturnValue();
  if (SrcVal == nullptr) { // ret void.
    return;
  }
  auto Src = cg.getOrInsertNode(SrcVal, &I, 0);
  auto Dst = cg.getNodeOrNull(ReturnValue{.Func = I.getFunction()}, &I, 0);
  // src is a subtype of dest
  cg.addSubtype(Src, Dst);
}

void ConstraintsGenerator::MLsubVisitor::visitPHINode(PHINode &I) {
  cg.createNode(&I, nullptr, -1);
  // Defer constraints generation (and unification) to handlePHINodes
  phiNodes.push_back(&I);
}

void ConstraintsGenerator::MLsubVisitor::handlePHINodes() {
  for (auto I : phiNodes) {
    auto P = cg.getNodeOrNull(I, nullptr, -1);
    for (long i = 0; i < I->getNumIncomingValues(); i++) {
      auto *Src = I->getIncomingValue(i);
      auto SrcVar = cg.getOrInsertNode(Src, I, i);
      cg.addSubtype(SrcVar, P);
    }
  }
}

unsigned ConstraintsGenerator::getPointerElemSize(Type *ty) {
  Type *Elem = ty->getPointerElementType();
  return llvm2c::getLLVMTypeSize(Elem, PointerSize);
}

void ConstraintsGenerator::MLsubVisitor::visitLoadInst(LoadInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(I.getPointerOperand(), &I, 0);
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

  auto PtrVal = cg.getOrInsertNode(I.getPointerOperand(), &I, 0);
  auto RetVal = cg.getOrInsertNode(&I, nullptr, -1);
  auto BitSize = cg.getPointerElemSize(I.getPointerOperandType());

  cg.addSubtype(PtrVal, binarysub::make_ptr_load(RetVal, BitSize));
}

void ConstraintsGenerator::MLsubVisitor::visitStoreInst(StoreInst &I) {
  // if this is access to table, then we ignore the type, and return func ptr.
  auto Node = cg.getNodeOrNull(I.getPointerOperand(), &I, 0);
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

  auto PtrVal = cg.getOrInsertNode(I.getPointerOperand(), &I, 1);
  auto BitSize = cg.getPointerElemSize(I.getPointerOperandType());
  auto StoreVal = cg.getOrInsertNode(I.getValueOperand(), &I, 0);

  cg.addSubtype(PtrVal, binarysub::make_ptr_store(StoreVal, BitSize));
}

void ConstraintsGenerator::MLsubVisitor::visitAllocaInst(AllocaInst &I) {
  auto Node = cg.createNode(&I, nullptr, -1);
  // set as pointer type
  cg.setPointer(&I, nullptr, -1);
  cg.ContraVariantValues.insert(&I);
}

void ConstraintsGenerator::MLsubVisitor::visitGetElementPtrInst(
    GetElementPtrInst &Gep) {
  // supress warnings for table gep
  if (Gep.getPointerOperand()->getName().startswith("table_")) {
    return;
  } else if (Gep.hasAllZeroIndices()) {
    auto SrcNode = cg.getOrInsertNode(Gep.getPointerOperand(), &Gep, 0);
    cg.addRemapType(&Gep, nullptr, -1, SrcNode);
    return;
  }
  std::cerr << "Warning: MLsubVisitor::visitGetElementPtrInst: "
               "Gep should not exist before this pass!\n";
  // But if we really want to support this, handle it the same way as AddInst.
  // A shortcut to create a offseted pointer. the operate type must be i8*.
  // Just like ptradd.
}

void ConstraintsGenerator::addCmpConstraint(const ExtValuePtr LHS,
                                            const ExtValuePtr RHS,
                                            llvm::ICmpInst *I) {
  PG.unifyVar(LHS, RHS);
}

void ConstraintsGenerator::addAddConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &PG.getOrInsertPNINode(LHS, I, 0);
  auto Right = &PG.getOrInsertPNINode(RHS, I, 1);
  // auto Res = &
  PG.getOrInsertPNINode(I, nullptr, -1);
  if (Left->isPNRelated() || Right->isPNRelated()) {
    PG.addAddCons(LHS, RHS, I, I);
  }
}
void ConstraintsGenerator::addSubConstraint(ExtValuePtr LHS, ExtValuePtr RHS,
                                            llvm::BinaryOperator *I) {
  llvmValue2ExtVal(LHS, I, 0);
  llvmValue2ExtVal(RHS, I, 1);
  auto Left = &PG.getOrInsertPNINode(LHS, I, 0);
  auto Right = &PG.getOrInsertPNINode(RHS, I, 1);
  // auto Res = &
  PG.getOrInsertPNINode(I, nullptr, -1);
  if (Left->isPNRelated() || Right->isPNRelated()) {
    PG.addAddCons(LHS, RHS, I, I);
  }
}

void ConstraintsGenerator::MLsubVisitor::visitICmpInst(ICmpInst &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addCmpConstraint(Src1, Src2, &I);

  // type the inst as bool?
  assert(I.getType()->isIntegerTy(1));
  cg.createNode(&I, nullptr, -1);
}

void ConstraintsGenerator::MLsubVisitor::visitSelectInst(SelectInst &I) {
  auto DstVar = cg.createNode(&I, nullptr, -1);
  auto *Src1 = I.getTrueValue();
  auto *Src2 = I.getFalseValue();
  auto Src1Var = cg.getOrInsertNode(Src1, &I, 0);
  auto Src2Var = cg.getOrInsertNode(Src2, &I, 1);
  // Not generate boolean constraints. Because it must be i1.
  cg.addSubtype(Src1Var, DstVar);
  cg.addSubtype(Src2Var, DstVar);
}

void ConstraintsGenerator::MLsubVisitor::visitAdd(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addAddConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::MLsubVisitor::visitSub(BinaryOperator &I) {
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);

  cg.addSubConstraint(Src1, Src2, &I);
}

void ConstraintsGenerator::MLsubVisitor::visitAnd(BinaryOperator &I) {
  // llvm::errs() << "visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  auto Src1Node = cg.getOrInsertNode(Src1, &I, 0);
  auto Src2Node = cg.getOrInsertNode(Src2, &I, 1);
  auto RetNode = cg.getOrInsertNode(&I, nullptr, -1);

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
  cg.setNonPointer(Src1, &I, 0);
  cg.setNonPointer(Src2, &I, 1);
  cg.setNonPointer(&I, nullptr, -1);
  return;
}

void ConstraintsGenerator::MLsubVisitor::visitOr(BinaryOperator &I) {
  // llvm::errs() << "Visiting " << __FUNCTION__ << " \n";
  auto *Src1 = I.getOperand(0);
  auto *Src2 = I.getOperand(1);
  ensureSequence(Src1, Src2);

  auto Src1Node = cg.getOrInsertNode(Src1, &I, 0);
  auto Src2Node = cg.getOrInsertNode(Src2, &I, 1);
  auto RetNode = cg.getOrInsertNode(&I, nullptr, -1);

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
  cg.setNonPointer(Src1, &I, 0);
  cg.setNonPointer(Src2, &I, 1);
  cg.setNonPointer(&I, nullptr, -1);
  return;
}

void ConstraintsGenerator::MLsubVisitor::visitInstruction(Instruction &I) {
  // return value
  if (I.getType()->isVoidTy()) {
    // skip void type
  } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
             ConstraintsGenerator::opTypes.at(I.getOpcode())
                 .addRetConstraint(&I, cg)) {
    // good
  } else {
    llvm::errs() << "WARN: MLsubGenerator: unhandled instruction return: " << I
                 << "\n";
  }
  // for each op
  for (unsigned Ind = 0; Ind < I.getNumOperands(); ++Ind) {
    auto Op = I.getOperand(Ind);
    if (Op->getType()->isVoidTy()) {
      // skip void type
    } else if (ConstraintsGenerator::opTypes.count(I.getOpcode()) &&
               ConstraintsGenerator::opTypes.at(I.getOpcode())
                   .addOpConstraint(Ind, &I, cg)) {
      // good
    } else {
      llvm::errs() << "WARN: MLsubGenerator: unhandled instruction Op: " << I
                   << "\n";
    }
  }
}

// Helper function for string comparison
static bool strEq(const char *S1, const char *S2) {
  return strcmp(S1, S2) == 0;
}

// PcodeOpType method implementations
bool ConstraintsGenerator::PcodeOpType::addRetConstraint(
    Instruction *I, ConstraintsGenerator &cg) const {
  // only create Covariant constraints, use addSubtype to handle contra-variant.
  auto N = cg.createNode(I, nullptr, -1);
  if (I->getType()->isVoidTy()) {
    return false;
  }
  const char *ty = output;
  if (ty == nullptr) { // no action
    return true;
  } else if (strEq(ty, "sint")) {
    cg.setNonPointer(I, nullptr, -1);
    auto SintNode = binarysub::make_primitive("sint", cg.getSize(I));
    cg.addSubtype(SintNode, N);
    return true;
  } else if (strEq(ty, "uint")) {
    cg.setNonPointer(I, nullptr, -1);
    auto UintNode = binarysub::make_primitive("uint", cg.getSize(I));
    cg.addSubtype(UintNode, N);
    return true;
  } else if (strEq(ty, "int")) {
    cg.setNonPointer(I, nullptr, -1);
    return true;
  }

  return false;
}

bool ConstraintsGenerator::PcodeOpType::addOpConstraint(
    unsigned Index, Instruction *I, ConstraintsGenerator &cg) const {
  assert(size >= 0 && static_cast<unsigned>(size) == I->getNumOperands() &&
         "input size not match");
  auto Op = I->getOperand(Index);
  if (Op->getType()->isVoidTy()) {
    return false;
  }
  auto N = cg.getOrInsertNode(Op, I, Index);
  const char *ty = inputs[Index];
  if (ty == nullptr) {
    return true;
  } else if (strEq(ty, "sint")) {
    cg.setNonPointer(Op, I, Index);
    auto SintNode = binarysub::make_primitive("sint", cg.getSize(Op));
    cg.addSubtype(N, SintNode);
    return true;
  } else if (strEq(ty, "uint")) {
    cg.setNonPointer(Op, I, Index);
    auto UintNode = binarysub::make_primitive("uint", cg.getSize(Op));
    cg.addSubtype(N, UintNode);
    return true;
  } else if (strEq(ty, "int")) {
    cg.setNonPointer(Op, I, Index);
    return true;
  }
  return false;
}

// Static member definition for opTypes
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

// #endregion ConstraintsGenerator::MLsubVisitor

} // namespace notdec::mlsub
