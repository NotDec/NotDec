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

#include "Passes/ConstraintGenerator.h"
#include "Passes/StackPointerFinder.h"
#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Lattice.h"
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/PointerNumberIdentification.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/Sketch.h"
#include "TypeRecovery/SketchToCTypeBuilder.h"
#include "Utils/AllSCCIterator.h"
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

namespace notdec {

using retypd::OffsetLabel;

static inline TypeVariable makeTv(std::shared_ptr<retypd::TRContext> Ctx,
                                  std::string Name) {
  return retypd::TypeVariable::CreateDtv(*Ctx, Name);
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
    llvm::errs() << toString(Ent.first) << " -> " << Ent.second.str() << "\n";
  }
  llvm::errs() << "V2NContra:\n";
  for (auto &Ent : V2NContra) {
    llvm::errs() << toString(Ent.first) << " -> " << Ent.second.str() << "\n";
  }
}

inline CGNode &getTarget(FieldEntry &F) {
  return const_cast<CGNode &>(F.Edge->getTargetNode());
}

void ConstraintsGenerator::removeNode(const retypd::NodeKey &K) {
  assert(!V2N.count(K));
  assert(!V2NContra.count(K));
  return CG.removeNode(K);
}

void ConstraintsGenerator::removeNode(retypd::CGNode &N) {
  assert(!TypeInfos.count(&N));
  removeNode(N.key);
}

void ConstraintsGenerator::mergeFixTypeInfo(CGNode &From, CGNode &To) {
  std::vector<const CGNode *> RelatedNodes;
  for (auto N : std::vector<CGNode *>{&From, &To}) {
    for (auto Ent : N->inEdges) {
      RelatedNodes.push_back(&Ent->getSourceNode());
    }
    for (auto &Ent : N->outEdges) {
      RelatedNodes.push_back(&Ent.getTargetNode());
    }
  }

  auto EdgeMap = mergeNodeTo(From, To);

  for (auto CN : RelatedNodes) {
    auto N = const_cast<CGNode *>(CN);
    if (TypeInfos.count(N)) {
      TypeInfos.at(N).fixEdge(EdgeMap);
    }
  }
}

void ConstraintsGenerator::mergeNodeAndType(CGNode &From, CGNode &To) {
  if (&From == &To) {
    return;
  }
  if (TypeInfos.count(&From) == 0 && TypeInfos.count(&To) == 0) {
    assert(From.getPNIVar()->getSize() == To.getPNIVar()->getSize());
    // 如果是两个常量类型，直接合并
    mergeFixTypeInfo(From, To);
  } else if ((TypeInfos.count(&To) && !TypeInfos.count(&From)) ||
             (TypeInfos.at(&From).isSimple() && TypeInfos.at(&To).isSimple()) ||
             (TypeInfos.at(&From).isStruct() && TypeInfos.at(&To).isStruct())) {
    // 如果1. From没有typeinfo，但是To有
    // 2. 是相同的simple或者struct类型
    // TODO 重新计算struct的成员！
    mergeFixTypeInfo(From, To);
    std::set<retypd::EdgeLabel> outLabels =
        retypd::NFADeterminizer<>::allOutLabels({&To});
    for (auto &L : outLabels) {
      auto S = retypd::NFADeterminizer<>::move({&To}, L);
      if (S.size() > 1) {
        CGNode *To = nullptr;
        for (auto N : S) {
          if (To == nullptr) {
            To = N;
          } else {
            // do the merge
            if (TypeInfos.count(N) && !(TypeInfos.count(To))) {
              std::swap(N, To);
            }
            mergeNodeAndType(*N, *To);
          }
        }
      }
    }
  } else if (TypeInfos.at(&From).isArray() && TypeInfos.at(&To).isArray()) {
    // TODO merge if the access range in edge label is different
    assert(false && "TODO");
  } else if (TypeInfos.at(&From).isUnion() && TypeInfos.at(&To).isUnion()) {
    // TODO merge all union members to one.
    assert(false && "TODO");
  } else {
    assert(false && "TODO");
  }
}

void ConstraintsGenerator::mergeArrayUnions() {
  size_t MergeCount = 0;

  auto doMerge = [&](CGNode &UN, CGNode &Arr, CGNode &Other) {
    MergeCount += 1;
    // 1. remove all(two) outgoing edges for UN
    std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToRemove;
    for (auto &Edge : UN.outEdges) {
      ToRemove.emplace_back(&Edge.FromNode, &Edge.TargetNode, Edge.Label);
    }
    for (auto &Ent : ToRemove) {
      CG.removeEdge(*std::get<0>(Ent), *std::get<1>(Ent), std::get<2>(Ent));
    }
    ToRemove.clear();
    // 2. move all incoming edges to the array node. erase the union node.
    TypeInfos.erase(&UN);
    mergeFixTypeInfo(UN, Arr);
    // std::map<const retypd::CGEdge *, const retypd::CGEdge *> EdgeMap;
    // for (auto Edge : UN.inEdges) {
    //   // // TypeInfos may be invalidated. Or we should update the edge
    //   pointer?
    //   // TypeInfos.erase(&Edge->FromNode);
    //   ToRemove.emplace_back(&Edge->FromNode, &Edge->TargetNode, Edge->Label);
    //   auto NewEdge = CG.addEdge(Edge->FromNode, Arr, Edge->Label);
    //   EdgeMap.emplace(Edge, NewEdge);
    // }
    // for (auto &Ent : ToRemove) {
    //   CG.removeEdge(*std::get<0>(Ent), *std::get<1>(Ent), std::get<2>(Ent));
    //   // edge change may invalidate relevant type infos.
    //   TypeInfos.at(std::get<0>(Ent)).fixEdge(EdgeMap);
    // }
    // TypeInfos.erase(&UN);
    // mergeNodeTo(UN, Arr);
    // 3. merge Other node with array element node.
    auto &AE = (*Arr.outEdges.begin()).getTargetNode();
    mergeNodeAndType(Other, const_cast<CGNode &>(AE));
  };

  // 如果存在union里面一个array和一个大小完全相等的成员，那么就合并这个union。
  bool Changed = false;
  do {
    Changed = false;
    for (auto &Ent : CG.Nodes) {
      auto &N = Ent.second;
      if (TypeInfos.count(&N)) {
        auto &Info = TypeInfos.at(&N);
        if (auto UInfo = Info.getAs<UnionInfo>()) {
          if (UInfo->Members.size() == 2) {
            auto L0 = UInfo->Members.at(0);
            auto L1 = UInfo->Members.at(1);
            auto N0 = const_cast<CGNode *>(&L0->getTargetNode());
            auto N1 = const_cast<CGNode *>(&L1->getTargetNode());
            auto N0Info = TypeInfos.at(N0);
            auto N1Info = TypeInfos.at(N1);
            if (N0Info.isArray() && N1Info.isArray()) {
              // TODO: "How to handle two array type?");
              break;
            } else if (auto AI = N0Info.getAs<ArrayInfo>()) {
              auto ElemSize = AI->ElemSize.value();
              auto OtherSize = N1Info.Size.value();
              if (ElemSize == OtherSize) {
                doMerge(N, *N0, *N1);
                Changed = true;
                // because we will remove node within the loop, we cannot
                // continue the loop.
                break;
              }
            } else if (auto AI = N1Info.getAs<ArrayInfo>()) {
              auto ElemSize = AI->ElemSize.value();
              auto OtherSize = N0Info.Size.value();
              if (ElemSize == OtherSize) {
                doMerge(N, *N1, *N0);
                Changed = true;
                // because we will remove node within the loop, we cannot
                // continue the loop.
                break;
              }
            }
          }
        }
      }
    }
  } while (Changed);

  if (MergeCount > 0) {
    llvm::errs() << "SCC " << CG.Name << ": Merged "
                 << std::to_string(MergeCount)
                 << " unions with element and array!\n";
  }
}

std::map<CGNode *, TypeInfo> ConstraintsGenerator::organizeTypes() {
  assert(TypeInfos.empty());
  // after post process, organize the node with offset edges.
  // Requirement 1. the graph has only recall, has no one edge.(after
  // determinize)
  for (auto &Ent : CG.Nodes) {
    auto &N = Ent.second;
    for (auto &E : N.outEdges) {
      if (E.Label.isOne()) {
        llvm::errs() << "organizeTypes: Node " << N.key.str()
                     << " has one edge, should not exist\n";
        std::abort();
      }
      if (E.Label.isForgetLabel()) {
        llvm::errs() << "organizeTypes: Node " << N.key.str()
                     << " has forget label, should not exist\n";
        std::abort();
      }
      if (retypd::isStore(E.Label)) {
        llvm::errs() << "organizeTypes: Node " << N.key.str()
                     << " has store label, should not exist\n";
        std::abort();
      }
    }
  }
  // We consider Pointer related edges: subtype(one), offset, load/store.
  // after determinize, there are only load and recall offset.

  // Requirement 2. require the graph is acyclic in subtype and offset edges.
  // Requirement 3. load and store edges are merged. there are only load edges.
  // Requirement 4. determinize: only one layer of offset edge.

  // auxiliary function, split in the middle
  auto splitEdge = [&](const retypd::CGEdge &E, retypd::EdgeLabel E1,
                       retypd::EdgeLabel E2, std::string Name) -> CGNode & {
    auto &Source = const_cast<CGNode &>(E.getSourceNode());
    auto &Target = const_cast<CGNode &>(E.getTargetNode());

    auto *NewNode = &CG.createNodeClonePNI(
        retypd::NodeKey{TypeVariable::CreateDtv(*CG.Ctx, Name)},
        Source.getPNIVar());
    CG.addEdge(Source, *NewNode, E1);
    CG.addEdge(*NewNode, Target, E2);
    // remove old edge.
    CG.removeEdge(Source, Target, E.Label);
    return *NewNode;
  };

  std::set<CGNode *> Visited;
  // See test/Access2Struct/main.py
  std::function<void(CGNode &, bool)> doBuild = [&](CGNode &N,
                                                    bool mustBeStruct = false) {
    if (!N.isPNIPointer()) {
      return;
    }
    // Use TypeInfos as finished set.
    if (TypeInfos.count(&N)) {
      return;
    }

    if (Visited.count(&N)) {
      llvm::errs() << "Cycle detected in organizeTypes!!\n";
      std::abort();
    }
    Visited.insert(&N);

    // if no offset edge, only load edge, this is a simple pointer
    if (!mustBeStruct && !retypd::hasOffsetEdge(N) &&
        retypd::countLoadOrStoreEdge(N) <= 1) {
      // Check for load or store edge
      std::optional<const retypd::CGEdge *> LoadEdge;
      for (auto &Edge : N.outEdges) {
        if (retypd::isLoadOrStore(Edge.Label)) {
          assert(!LoadEdge.has_value());
          LoadEdge = &Edge;
        }
      }
      unsigned LoadSize = 0;
      if (!LoadEdge.has_value()) {
        // has no edge, a void pointer?
        LoadEdge = nullptr;
      } else {
        LoadSize = retypd::getLoadOrStoreSize((*LoadEdge)->Label);
      }
      assert(LoadSize % 8 == 0);
      TypeInfos[&N] = TypeInfo{.Size = LoadSize / 8,
                               .Info = SimpleTypeInfo{.Edge = *LoadEdge}};
      return;
    }
    // check if it is a simple array. If there is only one offset edge.
    unsigned edgeCount = 0;
    const CGEdge *OffsetEdge = nullptr;
    for (auto &Edge : N.outEdges) {
      if (retypd::getOffsetLabel(Edge.Label)) {
        edgeCount++;
        OffsetEdge = &Edge;
      } else if (retypd::isLoadOrStore(Edge.Label)) {
        edgeCount++;
      }
    }
    if (!mustBeStruct && edgeCount == 1 && OffsetEdge != nullptr) {
      auto &Target = const_cast<CGNode &>(OffsetEdge->getTargetNode());
      if (auto *OL = retypd::getOffsetLabel(OffsetEdge->Label)) {
        if (OL->range.offset == 0 && OL->range.access.size() == 1) {
          // this is an array, but we first assume element count = 1
          auto AccessSize = OL->range.access.begin()->Size;
          TypeInfos[&N] = TypeInfo{
              .Size = AccessSize,
              .Info = ArrayInfo{.Edge = OffsetEdge, .ElemSize = AccessSize}};
          doBuild(Target, false);
          return;
        }
      }
    }
    // this is a struct or union.
    std::vector<FieldEntry> Fields;

    // normalize load edge as zero offset
    for (auto &E : N.outEdges) {
      // auto &Target = const_cast<CGNode &>(E.getTargetNode());
      if (retypd::isLoadOrStore(E.Label)) {
        // separate the load/store edge as a simple pointer node.
        // auto &New =
        splitEdge(E, {retypd::RecallLabel{OffsetLabel{OffsetRange()}}}, E.Label,
                  ValueNamer::getName("F0_"));
        // break;
      }
    }

    // 处理所有数组类型的成员
    // 提取所有乘数项
    std::set<int64_t> AllStrides;
    std::set<const CGEdge *> RemainingOffsetEdges;
    for (auto &E : N.outEdges) {
      if (auto *OL = retypd::getOffsetLabel(E.Label)) {
        RemainingOffsetEdges.insert(&E);
        for (auto &A : OL->range.access) {
          if (A.Size > 0) {
            AllStrides.insert(A.Size);
          }
        }
      }
    }

    while (!AllStrides.empty()) {
      auto MaxStride = *AllStrides.rbegin();
      assert(MaxStride > 0);
      AllStrides.erase(MaxStride);

      // 提取包含max_stride的模式
      std::vector<const CGEdge *> HasStrideOffsetEdges;
      for (auto *Edge : RemainingOffsetEdges) {
        auto *OL = retypd::getOffsetLabel(Edge->Label);
        assert(OL != nullptr);
        if (std::find(OL->range.access.begin(), OL->range.access.end(),
                      MaxStride) != OL->range.access.end()) {
          HasStrideOffsetEdges.push_back(Edge);
        }
      }
      for (auto *Edge : HasStrideOffsetEdges) {
        RemainingOffsetEdges.erase(Edge);
      }

      // 按基址从小到大排序
      std::sort(HasStrideOffsetEdges.begin(), HasStrideOffsetEdges.end(),
                [](const CGEdge *E1, const CGEdge *E2) {
                  auto *OL1 = retypd::getOffsetLabel(E1->Label);
                  auto *OL2 = retypd::getOffsetLabel(E2->Label);
                  return OL1->range.offset < OL2->range.offset;
                });

      while (!HasStrideOffsetEdges.empty()) {
        auto FrontLabel =
            retypd::getOffsetLabel(HasStrideOffsetEdges.front()->getLabel());
        auto RangeStart = FrontLabel->range.offset;
        auto RangeEnd = FrontLabel->range.offset + MaxStride;

        // 提取当前组中Base在当前范围内的模式
        auto InitialSize = HasStrideOffsetEdges.size();
        std::vector<const CGEdge *> InRangeEdges;
        std::optional<size_t> RemoveStart = std::nullopt;
        std::optional<size_t> RemoveEnd = HasStrideOffsetEdges.size();
        for (size_t I = 0; I < HasStrideOffsetEdges.size(); I++) {
          auto *Edge = HasStrideOffsetEdges.at(I);
          auto CurrentLabel = retypd::getOffsetLabel(Edge->getLabel());
          if (CurrentLabel->range.offset >= RangeStart &&
              CurrentLabel->range.offset < RangeEnd) {
            InRangeEdges.push_back(Edge);
            if (!RemoveStart) {
              RemoveStart = I;
            }
          } else {
            RemoveEnd = I;
            break;
          }
        }
        assert(RemoveStart);
        HasStrideOffsetEdges.erase(HasStrideOffsetEdges.begin() + *RemoveStart,
                                   HasStrideOffsetEdges.begin() + *RemoveEnd);
        assert(InitialSize ==
               (HasStrideOffsetEdges.size() + InRangeEdges.size()));

        // 处理子问题仅有一条offset=0的边的简单情况:
        if (InRangeEdges.size() == 1) {
          if (auto Acc =
                  retypd::getOffsetLabel((*InRangeEdges.begin())->getLabel())) {
            if (Acc->range.offset == RangeStart &&
                Acc->range.access.size() == 1) {
              auto &TargetNode = const_cast<CGNode &>(
                  (*InRangeEdges.begin())->getTargetNode());
              auto &NewFieldNode = CG.createNodeClonePNI(
                  retypd::NodeKey{TypeVariable::CreateDtv(
                      *CG.Ctx, ValueNamer::getName("Field_"))},
                  N.getPNIVar());
              auto FieldEdge =
                  CG.addEdge(N, NewFieldNode,
                             {retypd::RecallLabel{OffsetLabel{
                                 .range = {.offset = RangeStart}}}});
              // auto ArrEdge =
              CG.addEdge(NewFieldNode, TargetNode,
                         {retypd::RecallLabel{
                             OffsetLabel{OffsetRange{.access = {MaxStride}}}}});
              Fields.push_back(FieldEntry{
                  .R = SimpleRange{.Start = RangeStart, .Size = MaxStride},
                  .Edge = FieldEdge});
              doBuild(NewFieldNode, false);
              for (auto *Edge : InRangeEdges) {
                auto &From = const_cast<CGNode &>(Edge->getSourceNode());
                auto &To = const_cast<CGNode &>(Edge->getTargetNode());
                CG.removeEdge(From, To, Edge->getLabel());
              }
              continue;
            }
          }
        }

        // 转换为子问题的数组节点
        auto &NewArrElemNode = CG.createNodeClonePNI(
            retypd::NodeKey{TypeVariable::CreateDtv(
                *CG.Ctx, ValueNamer::getName("ArrElem_"))},
            N.getPNIVar());
        for (auto *Edge : InRangeEdges) {
          auto &Target = const_cast<CGNode &>(Edge->getTargetNode());
          auto NewRange = retypd::getOffsetLabel(Edge->getLabel())->range;
          NewRange.access.erase(std::remove(NewRange.access.begin(),
                                            NewRange.access.end(), MaxStride),
                                NewRange.access.end());
          NewRange.offset = NewRange.offset - RangeStart;
          assert(NewRange.offset >= 0);
          CG.addEdge(NewArrElemNode, Target,
                     {retypd::RecallLabel{OffsetLabel{.range = NewRange}}});
        }
        for (auto *Edge : InRangeEdges) {
          auto &From = const_cast<CGNode &>(Edge->getSourceNode());
          auto &To = const_cast<CGNode &>(Edge->getTargetNode());
          CG.removeEdge(From, To, Edge->getLabel());
        }

        auto &NewFieldNode =
            CG.createNodeClonePNI(retypd::NodeKey{TypeVariable::CreateDtv(
                                      *CG.Ctx, ValueNamer::getName("Field_"))},
                                  N.getPNIVar());
        auto FieldEdge = CG.addEdge(N, NewFieldNode,
                                    {retypd::RecallLabel{OffsetLabel{
                                        .range = {.offset = RangeStart}}}});
        /*auto ArrEdge = */ CG.addEdge(
            NewFieldNode, NewArrElemNode,
            {retypd::RecallLabel{
                OffsetLabel{OffsetRange{.access = {MaxStride}}}}});
        Fields.push_back(
            FieldEntry{.R = SimpleRange{.Start = RangeStart, .Size = MaxStride},
                       .Edge = FieldEdge});
        doBuild(NewFieldNode, false);
        // reduced to simple array, so we set array info in recursive call.
        // TypeInfos[&NewArrNode] =
        //     TypeInfo{.Size = MaxStride, .Info = ArrayInfo{.Edge = ArrEdge}};
      }
    }

    for (auto *EP : RemainingOffsetEdges) {
      auto &E = *EP;
      auto &Target = const_cast<CGNode &>(E.getTargetNode());
      auto *OL = retypd::getOffsetLabel(E.Label);
      assert(OL && "Other kinds of Edge should already be eliminated!");
      assert(OL->range.access.size() == 0 &&
             "should be handled by previous array pass");
      doBuild(Target, false);
      auto &TI = TypeInfos.at(&Target);
      Fields.push_back(FieldEntry{
          .R = SimpleRange{.Start = OL->range.offset, .Size = *TI.Size},
          .Edge = &E});
    }

    auto IsOverlap = [](OffsetTy S1, OffsetTy E1, OffsetTy S2, OffsetTy E2) {
      assert(S1 < E1);
      assert(S2 < E2);
      if (std::max(S1, S2) < std::min(E1, E2)) {
        return true;
      }
      return false;
    };

    // Fields should not be mutated during the lifetime of ret vector
    auto FilterInRange = [&](const std::vector<FieldEntry> &Fields,
                             OffsetTy Start,
                             OffsetTy End) -> std::vector<size_t> {
      std::vector<size_t> Ret;
      for (size_t I = 0; I < Fields.size(); I++) {
        auto &F = Fields.at(I);
        auto FS = F.R.Start;
        auto FE = F.R.Size + FS;
        // overlaps
        if (IsOverlap(Start, End, FS, FE)) {
          Ret.push_back(I);
        }
      }
      return Ret;
    };

    while (true) {
      // 收集所有分割点
      std::set<OffsetTy> AllIndex;
      for (auto &F : Fields) {
        AllIndex.insert(F.R.Start);
        AllIndex.insert(F.R.end());
      }

      // 遍历所有最小范围区间，如果出现重叠则以此开始创建union类型。
      bool NoUpdate = true;
      for (auto It = AllIndex.begin(); It != AllIndex.end(); ++It) {
        auto NextIt = std::next(It);
        if (NextIt == AllIndex.end()) {
          break;
        }
        auto Start = *It;
        auto End = *NextIt;
        auto InRangeFieldIndex = FilterInRange(Fields, Start, End);
        if (InRangeFieldIndex.size() <= 1) {
          continue;
        }
        NoUpdate = false;

        // create a union here.
        auto OldSize = 1;
        auto NewSize = InRangeFieldIndex.size();
        while (
            NewSize >
            OldSize) { // 根据小的重叠区域，左右拓展找到需要处理创建union的所有Fields。
          OldSize = NewSize;
          for (auto Ind : InRangeFieldIndex) {
            auto &F = Fields.at(Ind);
            Start = std::min(Start, F.R.Start);
            End = std::max(End, F.R.end());
          }
          InRangeFieldIndex = FilterInRange(Fields, Start, End);
          NewSize = InRangeFieldIndex.size();
          assert(NewSize >= 2);
        }

        auto UnionStart = Start;
        auto UnionEnd = End;
        std::vector<FieldEntry> OtherFields;
        std::vector<FieldEntry> OverlapFields;
        for (auto &F : Fields) {
          if (IsOverlap(Start, End, F.R.Start, F.R.end())) {
            OverlapFields.push_back(FieldEntry{
                .R = {.Start = F.R.Start - UnionStart, .Size = F.R.Size},
                .Edge = F.Edge});
          } else {
            OtherFields.push_back(F);
          }
        }
        // #region build members using OverlapFields;

        // sort the entry by end offset.
        std::sort(OverlapFields.begin(), OverlapFields.end(),
                  [](const FieldEntry &A, const FieldEntry &B) {
                    return A.R.end() < B.R.end();
                  });
        // use std::min to find the min start offset.
        auto MinStartOff =
            std::min_element(OverlapFields.begin(), OverlapFields.end(),
                             [](const FieldEntry &A, const FieldEntry &B) {
                               return A.R.Start < B.R.Start;
                             })
                ->R.Start;
        auto MaxOff = OverlapFields.back().R.end();
        // unified start to 0
        assert(MinStartOff == 0);
        assert(MaxOff == (UnionEnd - UnionStart));
        // after determinize, there will not be nested struct. We assume Offset
        // to struct == Min Start Offset. So set size as MaxOff - MinStartOff.
        auto OurSize = MaxOff - MinStartOff;

        std::vector<std::vector<FieldEntry>> UnionPanels;
        for (auto &F : OverlapFields) {
          bool inserted = false;
          for (auto &Panel : UnionPanels) {
            if (Panel.back().R.Start + Panel.back().R.Size <= F.R.Start) {
              Panel.push_back(F);
              inserted = true;
              break;
            }
          }
          if (!inserted) {
            UnionPanels.push_back({F});
          }
        }
        assert(UnionPanels.size() > 1);
        // if there is only one field and off=0, make N a union node.
        CGNode *UN = nullptr;
        const CGEdge *UE = nullptr;
        if (!mustBeStruct && OtherFields.size() == 0 && UnionStart == 0) {
          UN = &N;
        } else {
          std::string Name = ValueNamer::getName("Un_");
          UN = &CG.createNodeClonePNI(
              retypd::NodeKey{TypeVariable::CreateDtv(*CG.Ctx, Name)},
              N.getPNIVar());
          // move all related edge under UN
          for (auto &Panel : UnionPanels) {
            for (auto &F : Panel) {
              // subtract by UnionStart
              auto Off = *retypd::getOffsetLabel(F.Edge->Label);
              Off.range.offset -= UnionStart;
              auto *NE =
                  CG.addEdge(*UN, getTarget(F), {retypd::RecallLabel{Off}});
              CG.removeEdge(N, getTarget(F), F.Edge->Label);
              F.Edge = NE;
            }
          }
          UE = CG.addEdge(N, *UN,
                          {retypd::RecallLabel{
                              OffsetLabel{OffsetRange{.offset = UnionStart}}}});
        }
        // create new node for each panel struct.
        std::vector<const retypd::CGEdge *> Members;
        for (auto &Panel : UnionPanels) {
          if (Panel.size() == 1) {
            // we do not need to create a struct
            Members.push_back(Panel.front().Edge);
            continue;
          }
          // create a struct here
          std::string Name = ValueNamer::getName("Us_");
          auto NN = &CG.createNodeClonePNI(
              retypd::NodeKey{TypeVariable::CreateDtv(*CG.Ctx, Name)},
              UN->getPNIVar());
          // move edges under the struct
          for (auto &F : Panel) {
            auto *NE = CG.addEdge(*NN, getTarget(F), F.Edge->Label);
            CG.removeEdge(*UN, getTarget(F), F.Edge->Label);
            F.Edge = NE;
          }
          TypeInfos[NN] =
              TypeInfo{.Size = OurSize, .Info = StructInfo{.Fields = Panel}};
          auto *E1 = CG.addEdge(*UN, *NN,
                                {retypd::RecallLabel{OffsetLabel{
                                    OffsetRange{.offset = MinStartOff}}}});
          Members.push_back(E1);
        }
        TypeInfos[UN] =
            TypeInfo{.Size = OurSize, .Info = UnionInfo{.Members = Members}};

        if (UN == &N) {
          assert(OtherFields.empty());
          // already set typeinfo for N, so we are done
          return;
        }
        // push the merged union back to fields, and iterate again
        OtherFields.push_back(
            {FieldEntry{.R = SimpleRange{.Start = UnionStart + MinStartOff,
                                         .Size = OurSize},
                        .Edge = UE}});
        assert(NoUpdate == false);
        // #endregion build members using OverlapFields;

        // reiterate with merged fields.
        Fields = OtherFields;
        break;
      }
      // no overlap, cannot create unions
      if (NoUpdate) {
        break;
      }
    } // end of while true

    // Now there is no overlap, create struct for Fields.
    // sort the entry by start offset.
    if (Fields.empty()) {
      TypeInfos[&N] = TypeInfo{.Size = 0, .Info = StructInfo{}};
      return;
    }
    std::sort(Fields.begin(), Fields.end(),
              [](const FieldEntry &A, const FieldEntry &B) {
                return A.R.Start < B.R.Start;
              });
    auto MaxEndOff =
        std::max_element(Fields.begin(), Fields.end(),
                         [](const FieldEntry &A, const FieldEntry &B) {
                           return A.R.end() < B.R.end();
                         })
            ->R.end();
    auto Size = MaxEndOff - Fields.front().R.Start;
    assert(Size >= 0);
    TypeInfos[&N] =
        TypeInfo{.Size = Size, .Info = StructInfo{.Fields = Fields}};

    return;
  };

  for (auto &Ent : CG.Nodes) {
    auto &N = Ent.second;
    // only consider pointer node.
    if (!N.isPNIPointer()) {
      continue;
    }
    bool mustStruct = N.isMemory();
    doBuild(N, mustStruct);
  }
  return TypeInfos;
}

std::shared_ptr<ConstraintsGenerator>
TypeRecovery::postProcess(ConstraintsGenerator &G, std::string DebugDir) {
  if (G.PG) {
    G.PG->clearConstraints();
  }
  std::map<const CGNode *, CGNode *> Old2New;
  std::shared_ptr<ConstraintsGenerator> G2S = G.cloneShared(Old2New);
  ConstraintsGenerator &G2 = *G2S;
  assert(G2.PG);
  std::string Name = G2.CG.Name;
  G2.CG.Name += "-dtm";
  if (!DebugDir.empty()) {
    G2.DebugDir = DebugDir;
  }

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

  // Graph Pass
  G2.organizeTypes();
  G2.mergeArrayUnions();

  // G2.mergeArrayWithMember();
  // G2.elimSingleStruct();

  assert(G2.PG);
  // G2.DebugDir.clear();
  return G2S;
}

void ConstraintsGenerator::elimSingleStruct() {
  assert(false && "TODO");
  // 如果存在一个只有一个成员的struct，那么就消除它。
  for (auto &Ent : CG.Nodes) {
    auto &N = Ent.second;
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
    if (V2N.count(N->key)) {
      V2N.merge(N->key, O.getTargetNode().key);
    }
    if (V2NContra.count(N->key)) {
      V2NContra.merge(N->key, I->getSourceNode().key);
    }
    // 2. 直接连接为A->B。
    CG.addEdge(I->getSourceNode(), const_cast<CGNode &>(O.getTargetNode()),
               {retypd::One{}});
    // 3. 移除该节点和相关的两条边
    CG.removeEdge(const_cast<CGNode &>(O.getSourceNode()),
                  const_cast<CGNode &>(O.getTargetNode()), O.getLabel());
    CG.removeEdge(I->getSourceNode(), I->getTargetNode(), I->getLabel());
    CG.removeNode(N->key);
  };

  // 1 收集所有可能的节点，主要关注Covariant的
  std::vector<CGNode *> PossibleNodes;
  for (auto &Ent : CG.Nodes) {
    auto &N = Ent.second;
    if (N.isStartOrEnd() || N.isMemory()) {
      continue;
    }
    if (N.key.SuffixVariance == retypd::Contravariant) {
      continue;
    }
    if (MatchCritiria(&N)) {
      PossibleNodes.push_back(&N);
    }
  }

  auto SimplifyCount = 0;
  for (auto N : PossibleNodes) {
    // 同时找到对称的节点
    auto NK = N->key;
    assert(NK.SuffixVariance == retypd::Covariant);
    NK.SuffixVariance = retypd::invert(NK.SuffixVariance);
    auto &NC = CG.getNode(NK);
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

    llvm::Optional<std::string> DirPath;
    std::string SCCName = "SCC" + std::to_string(SCCIndex);
    if (DebugDir) {
      DirPath.emplace(join(DebugDir, SCCName));
      llvm::sys::fs::create_directories(*DirPath);
    }
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
    // 1.1 Check for Summary override.
    // for external function(isDeclaration), Use the summary as graph.
    // for non-external but summary overriden, still build the graph but
    // override summary. nomal: build the graph and summary.
    bool isDeclaration =
        SCCSet.size() == 1 && (*SCCSet.begin())->isDeclaration();
    if (isDeclaration) {
      if (SummaryOverride.count(SCCSet)) {
        std::cerr << "Override summary for external function: " << Name
                  << ":\n";
        Generator = SummaryOverride.at(SCCSet);
        Generator->checkSymmetry();
      } else {
        llvm::errs() << "Warning: Summary and result may be incorrect due to "
                        "external function: "
                     << Name << "\n";
        continue;
      }
    } else {
      //!! normal case, create the initial constraint graph
      Generator = std::make_shared<ConstraintsGenerator>(*this, Name, SCCSet);
      Generator->run();
    }

    // TODO: If the SCC/func is not called by any other function out of the SCC,
    // we can skip summary generation.

    if (DirPath) {
      Generator->CG.printGraph(join(*DirPath, "00-Generated.dot").c_str());
    }

    // 1.2 instantiate the summaries for each call.
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
    // 1.3 solve more subtype relations
    Generator->CG.solve();
    if (DirPath) {
      Generator->CG.printGraph(
          join(*DirPath, "01-InstantiateSummary.sat.dot").c_str());
    }

    // 1.4 generate summary
    std::shared_ptr<ConstraintsGenerator> Summary;
    // for declaration, summary is already overriden
    if (isDeclaration) {
      Summary = Generator;
    } else if (SummaryOverride.count(SCCSet)) {
      // Non-declaration, but summary overriden
      std::cerr << "Summary Overriden: " << Name << ":\n";
      Summary = SummaryOverride.at(SCCSet);
    } else {
      //!! normal case, generate summary
      std::cerr << "Generating Summary for " << Name << "\n";
      Summary = Generator->genSummary();
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
    Data.BottomUpGenerator = Generator;
    Data.SCCName = Name;
    // write the SCC index to file
    if (SCCsCatalog) {
      *SCCsCatalog << "SCC" << SCCIndex << "," << Name << "\n";
    }
    // print summary
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
    Data.TopDownGenerator =
        std::make_shared<ConstraintsGenerator>(*this, Data.SCCName);
    auto &CurrentTypes = *(Data.TopDownGenerator);
    assert(CurrentTypes.PG);

    llvm::Optional<std::string> DirPath;
    if (DebugDir) {
      DirPath.emplace(join(DebugDir, SCCDebugFolderName));
      llvm::sys::fs::create_directories(*DirPath);
    }
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

    const std::vector<CallGraphNode *> &NodeVec = Data.Nodes;
    if (Data.BottomUpGenerator == nullptr) {
      continue;
    }
    std::shared_ptr<ConstraintsGenerator> &Generator = Data.BottomUpGenerator;
    auto &Name = Data.SCCName;

    std::cerr << "(Top-Down) Processing Func: " << Name << "\n";
    auto Start2 = std::chrono::steady_clock::now();

    // Collect all functions for SCC checking
    const std::set<llvm::Function *> &SCCSet = Data.SCCSet;

    if (DirPath) {
      Generator->CG.printGraph(join(*DirPath, "03-Original.dot").c_str());
    }

    // 2.1 clone the graph to CurrentTypes. Previous graph is for summary.
    // CurrentTypes is temporary for build types for internal variables.
    std::map<const CGNode *, CGNode *> Old2New;
    Generator->cloneTo(CurrentTypes, Old2New);

    // 2.2 collect and merge actual params
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
              AllSCCs.at(AG.Func2SCCIndex.at(Elem.second)).TopDownGenerator;
          if (CallerGenerator->CallToInstance.count(Call)) {
            auto [FN, FNC] = CallerGenerator->CallToInstance.at(Call);

            FuncNodes.insert(FN);
            FuncNodesContra.insert(FNC);
          } else {
            assert(Data.SCCSet.count(Elem.second->getFunction()));
          }
        }
      }

      auto *D = multiGraphDeterminizeTo(CurrentTypes, FuncNodes, "act_");
      CurrentTypes.addSubtype(
          CurrentTypes.getNode(Current, nullptr, -1, retypd::Covariant), *D);

      // We do not need to do it again for Contravariant because it is
      // symmetric?

      // auto *DC = multiGraphDeterminizeTo(CurrentTypes, FuncNodesContra,
      // "actc_"); CurrentTypes.addSubtype(*DC, CurrentTypes.getNode(Current,
      // nullptr, -1, retypd::Contravariant));

      CurrentTypes.makeSymmetry();
      // Run saturation again
      CurrentTypes.CG.solve();
    }

    if (DirPath) {
      CurrentTypes.CG.printGraph(join(*DirPath, "05-CurrentTypes.dot").c_str());
    }

    // 2.3 solve again and fixups
    // ensure lower bound is lower than upper bound
    // CurrentTypes.makeSymmetry();
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
    if (SCCsPerf) {
      *SCCsPerf << "02 TopDown Elapsed: " << since(Start2).count() << " ms\n";
      SCCsPerf->close();
    }
  }

  handleGlobals();
}

void TypeRecovery::handleGlobals() {
  // 2.4 solve the global memory node
  // Create type for memory node
  std::shared_ptr<ConstraintsGenerator> &Global = AG.Global;
  Global = std::make_shared<ConstraintsGenerator>(*this, "Global");
  std::set<CGNode *> MemoryNodes;
  std::set<CGNode *> MemoryNodesC;
  auto &AllSCCs = AG.AllSCCs;
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
  Global->CG.onlyAddEdge(*Global->CG.getMemoryNode(retypd::Covariant), *MemNode,
                         {retypd::One{}});
  // Global->CG.onlyAddEdge(*MemNodeC,
  //                        *Global->CG.getMemoryNode(retypd::Contravariant),
  //                        {retypd::One{}});
  // keep node in val2node map for post process
  Global->makeSymmetry();
  Global->CG.solve();
  Global->V2N.insert(ConstantAddr(),
                     Global->CG.getMemoryNode(retypd::Covariant)->key);
  Global->V2NContra.insert(
      ConstantAddr(), Global->CG.getMemoryNode(retypd::Contravariant)->key);
}

void TypeRecovery::prepareSCC(CallGraph &CG) {
  AG.CG = &CG;

  all_scc_iterator<CallGraph *> CGI = notdec::scc_begin(AG.CG);
  SingleNodeSCCIterator SNI(notdec::scc_begin(AG.CG));

  std::vector<SCCData> &AllSCCs = AG.AllSCCs;
  std::map<CallGraphNode *, std::size_t> &Func2SCCIndex = AG.Func2SCCIndex;
  std::set<CallGraphNode *> Visited;

  // Split by SCC post order
  for (; (NoSCC ? !SNI.isAtEnd() : !CGI.isAtEnd()); (NoSCC ? ++SNI : ++CGI)) {
    const std::vector<CallGraphNode *> &NodeVec = (NoSCC ? *SNI : *CGI);
    AllSCCs.push_back(SCCData{.Nodes = NodeVec});
    size_t SCCIndex = AllSCCs.size() - 1;
    SCCData &Data = AllSCCs.back();

    // Calc name and SCCSet
    std::set<llvm::Function *> &SCCSet = Data.SCCSet;
    std::string Name;
    for (auto CGN : NodeVec) {
      Visited.insert(CGN);
      if (CGN->getFunction() == nullptr) {
        continue;
      }
      if (!Name.empty()) {
        Name += ",";
      }
      Name += CGN->getFunction()->getName().str();
      SCCSet.insert(CGN->getFunction());
      Func2SCCIndex[CGN] = SCCIndex;
    }
    if (!Name.empty()) {
      Data.SCCName = Name;
    }
  }

  // 2. calc reverse call edge map
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

std::unique_ptr<TypeRecovery::Result>
TypeRecovery::run(Module &M, ModuleAnalysisManager &MAM) {
  LLVM_DEBUG(errs() << " ============== RetypdGenerator  ===============\n");

  auto SP = MAM.getResult<StackPointerFinderAnalysis>(M);
  this->StackPointer = SP.result;

  data_layout = std::move(M.getDataLayoutStr());
  pointer_size = M.getDataLayout().getPointerSizeInBits();

  std::shared_ptr<BytesManager> MemoryBytes = BytesManager::create(M);

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

  // 0.5 print module for debugging
  if (DebugDir) {
    printModule(M, join(DebugDir, "01-Optimized.ll").c_str());
  }

  prepareSCC(MAM.getResult<CallGraphAnalysis>(M));

  bottomUpPhase();

  if (DebugDir) {
    SCCsCatalog->close();
    printModule(M, join(DebugDir, "02-AfterBottomUp.ll").c_str());
  }

  topDownPhase();

  std::cerr << "Bottom up phase done! SCC count:" << AG.AllSCCs.size() << "\n";

  if (DebugDir) {
    AG.Global->CG.printGraph(join(DebugDir, "GlobalMemory.dot").c_str());
    printAnnotatedModule(M, join(DebugDir, "02-AfterBottomUp.anno1.ll").c_str(),
                         1);
  }

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

  std::unique_ptr<TypeRecovery::Result> Result =
      std::make_unique<TypeRecovery::Result>();
  auto HTCtx = std::make_shared<ast::HTypeContext>();
  retypd::TypeBuilderContext TBC(*HTCtx, M.getName(), M.getDataLayout());
  using notdec::ast::HType;
  using notdec::ast::HTypeContext;

  // 3 build AST type for each value in value map
  auto &AllSCCs = AG.AllSCCs;
  for (int i = 0; i < AllSCCs.size(); i++) {
    auto &G = *AllSCCs[i].TopDownGenerator;
    auto SCCName = AllSCCs[i].SCCName;
    assert(G.PG);
    for (auto &Ent : G.CG.Nodes) {
      if (Ent.second.isStartOrEnd() || Ent.second.isMemory()) {
        continue;
      }
    }
    std::string Dir;
    if (DebugDir) {
      Dir = join(DebugDir, "SCC" + std::to_string(i));
    }

    //!! 3.1 post process the graph for type generation
    AllSCCs[i].SketchGenerator = postProcess(G, Dir);
    ConstraintsGenerator &G2 = *AllSCCs[i].SketchGenerator;

    // 3.2 print the graph for debugging
    if (!Dir.empty()) {
      G2.CG.printGraph(join(Dir, "09-PostProcess.dtm.dot").c_str());
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

    retypd::TypeBuilder TB(TBC, G2.TypeInfos);

    if (ValueTypesFile) {
      *ValueTypesFile << "UpperBounds:\n";
    }

    // 3.3 build type for each value
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
          *ValueTypesFile << "  " << toString(Ent.first, true);
          *ValueTypesFile << "  Node: " << toString(Node->key);
          if (Node->getPNIVar() != nullptr) {
            *ValueTypesFile << "  PNI: " << Node->getPNIVar()->str();
          }
          ValueTypesFile->flush();
        }

        if (TraceIds.count(Node->getId())) {
          PRINT_TRACE(Node->getId()) << ": Generating Type...\n";
        }

        //!! build AST type for the node
        HType *CTy = TB.buildType(*Node, retypd::Covariant);

        if (TraceIds.count(Node->getId())) {
          PRINT_TRACE(Node->getId())
              << ": Type is " << CTy->getAsString() << "\n";
        }

        if (Result->ValueTypes.count(Ent.first) != 0) {
          llvm::errs() << "Warning: TODO handle Value type merge (LowerBound): "
                       << toString(Ent.first) << "\n";
        }

        if (!isFuncPtr(Ent.first)) {
          Result->ValueTypes[Ent.first] = CTy;
        }

        if (ValueTypesFile) {
          *ValueTypesFile << " upper bound: " << CTy->getAsString();
        }
      } else {
        if (ValueTypesFile) {
          *ValueTypesFile << " has no upper bound";
        }
      }
      if (ValueTypesFile) {
        *ValueTypesFile << "\n";
      }
    }

    if (ValueTypesFile) {
      *ValueTypesFile << "LowerBounds:\n";
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
          *ValueTypesFile << "  " << toString(Ent.first, true);
          *ValueTypesFile << "  Node: " << toString(Node->key);
          if (Node->getPNIVar() != nullptr) {
            *ValueTypesFile << "  PNI: " << Node->getPNIVar()->str();
          }
          ValueTypesFile->flush();
        }

        if (TraceIds.count(Node->getId())) {
          PRINT_TRACE(Node->getId()) << ": Generating Type...\n";
        }

        //!! build AST type for the node
        HType *CTy = TB.buildType(*Node, retypd::Contravariant);

        if (TraceIds.count(Node->getId())) {
          PRINT_TRACE(Node->getId())
              << ": Type is " << CTy->getAsString() << "\n";
        }

        if (Result->ValueTypesLowerBound.count(Ent.first) != 0) {
          llvm::errs() << "Warning: TODO handle Value type merge (UpperBound): "
                       << toString(Ent.first) << "\n";
        }
        if (!isFuncPtr(Ent.first)) {
          Result->ValueTypesLowerBound[Ent.first] = CTy;
        }
        if (ValueTypesFile) {
          *ValueTypesFile << " lower bound: " << CTy->getAsString();
        }
      } else {
        if (ValueTypesFile) {
          *ValueTypesFile << " has no lower bound";
        }
      }
      if (ValueTypesFile) {
        *ValueTypesFile << "\n";
      }
    }
  }

  // 3.4 build AST type for memory node
  std::shared_ptr<ConstraintsGenerator> &GlobalSkS = AG.GlobalSketch;
  GlobalSkS = postProcess(*AG.Global, "");
  ConstraintsGenerator &GlobalSk = *GlobalSkS;
  // CGNode &MemNode2 = Global2.getNode(ConstantAddr(), nullptr, -1,
  // retypd::Covariant);
  CGNode *MemNode2 = GlobalSk.CG.getMemoryNode(retypd::Covariant);
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
  Result->MemoryType = CTy->getPointeeType();
  Result->MemoryDecl = Mem;

  // move the ASTUnit to result
  Result->HTCtx = HTCtx;

  // gen_json("retypd-constrains.json");

  // TODO convert the type back to LLVM IR??
  if (DebugDir) {
    printAnnotatedModule(M, join(DebugDir, "03-Final.anno2.ll").c_str(), 2);
  }

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
      if (Edge.getLabel().isRecallBase() || Edge.getLabel().isForgetBase()) {
        continue;
      }
      auto &T = const_cast<CGNode &>(Edge.getTargetNode());
      auto &TC = CG.getOrInsertNodeWithPNI(retypd::MakeReverseVariant(T.key),
                                           T.getPNIVar());
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

ConstraintsGenerator
ConstraintsGenerator::clone(std::map<const CGNode *, CGNode *> &Old2New) {
  ConstraintsGenerator G(Ctx, CG.Name);
  cloneTo(G, Old2New);
  return G;
}

std::shared_ptr<ConstraintsGenerator>
ConstraintsGenerator::cloneShared(std::map<const CGNode *, CGNode *> &Old2New) {
  auto G = std::make_shared<ConstraintsGenerator>(Ctx, CG.Name);
  cloneTo(*G, Old2New);
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
  std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToRemove;
  for (auto &Ent : CG) {
    auto *Node = &Ent.second;
    if (Node == CG.getStartNode() || Node == CG.getEndNode()) {
      continue;
    }
    if (ReachableNodes.count(Node) == 0) {
      ToErase.push_back(Node);
      // erase all out edges
      for (auto &Edge : Ent.second.outEdges) {
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
  for (auto &Ent : V2N) {
    auto *N = &CG.getNode(Ent.second);
    if (V2NContra.count(Ent.first) == 0) {
      llvm::errs() << "Error: Cannot find " << toString(Ent.second) << "\n";
      // assert(false);
      continue;
    }
    auto *CN = &CG.getNode(V2NContra.at(Ent.first));
    assert(CN->getPNIVar() == N->getPNIVar());
    CG.addEdge(*CN, *N, {retypd::One{}});
  }
}

void ConstraintsGenerator::eliminateCycle() {
  std::map<const CGNode *, CGNode *> Old2New;
  ConstraintGraph NewG = CG.clone(Old2New);

  // remove all edges except one edge, also remove one edge to primitive nodes
  std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToRemove;
  for (auto &Ent : NewG) {
    for (auto &Edge : Ent.second.outEdges) {
      if (Edge.Label.isOne()) {
        if (!Edge.TargetNode.key.Base.isPrimitive()) {
          continue;
        }
      }

      // NewG.removeEdge(Edge.FromNode, Edge.TargetNode, Edge.Label);
      ToRemove.emplace_back(&Edge.FromNode, &Edge.TargetNode, Edge.Label);
    }
  }
  for (auto &Ent : ToRemove) {
    NewG.removeEdge(*std::get<0>(Ent), *std::get<1>(Ent), std::get<2>(Ent));
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
      mergeNodeTo(*Node, Merged, false);
    }
  }
}

std::map<const CGEdge *, const CGEdge *>
ConstraintsGenerator::mergeNodeTo(CGNode &From, CGNode &To, bool NoSelfLoop) {
  assert(&From.Parent == &CG && &To.Parent == &CG);
  // update the value map
  addMergeNode(From.key, To.key);
  // update the graph
  return CG.mergeNodeTo(From, To, NoSelfLoop);
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

void ConstraintsGenerator::determinize() {
  // assert(DTrans.empty());
  DTrans.clear();
  std::map<const CGNode *, CGNode *> This2Bak;
  ConstraintGraph Backup = CG.clone(This2Bak);
  Backup.changeStoreToLoad();
  // remove all edges in the graph
  std::vector<std::tuple<CGNode *, CGNode *, retypd::EdgeLabel>> ToRemove;
  for (auto &Ent : CG) {
    for (auto &Edge : Ent.second.outEdges) {
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
    if (auto *N = getNodeOrNull(Ent.second)) {
      V2NNodes.insert(N);
    }
  }
  for (auto &Ent : V2NContra) {
    if (auto *N = getNodeOrNull(Ent.second)) {
      V2NNodes.insert(N);
    }
  }
  std::vector<retypd::NodeKey> ToRemove1;
  for (auto &Ent : CG.Nodes) {
    if (Ent.second.isStartOrEnd()) {
      continue;
    }
    if (!Ent.second.isSpecial() && V2NNodes.count(&Ent.second) == 0) {
      // removeNode(Ent.second.key);
      ToRemove1.push_back(Ent.second.key);
    }
  }
  for (auto &Key : ToRemove1) {
    removeNode(Key);
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
    // TODO eliminate offset loop earlier
    mergeNodeTo(*A, *B, false);
    A = nullptr;
    B = nullptr;
    std::tie(A, B) = findMergePair();
  }
}

void ConstraintsGenerator::run() {
  for (llvm::Function *Func : SCCs) {
    // create function nodes
    getOrInsertNode(Func, nullptr, -1, retypd::Covariant);
    for (int i = 0; i < Func->arg_size(); i++) {
      getOrInsertNode(Func->getArg(i), nullptr, i, retypd::Covariant);
    }
    if (!Func->getReturnType()->isVoidTy()) {
      getOrInsertNode(ReturnValue{.Func = Func}, nullptr, -1,
                      retypd::Covariant);
    }
    // assert convariant node is created
    getNode(Func, nullptr, -1, retypd::Contravariant);
    RetypdGeneratorVisitor Visitor(*this);
    Visitor.visit(Func);
    Visitor.handlePHINodes();
  }
  CG.PG->solve();
  preSimplify();
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
  auto NKN = FI->key.Base.getBaseName();

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
        auto Base = N.Base;
        if (N.Base.hasBaseName() && N.Base.getBaseName() == NKN) {
          Base = Base.markActual();
        }
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
        RetKey.Base = RetKey.Base.pushLabel({retypd::OutLabel{}});
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
            {retypd::InLabel{std::to_string(Arg->getArgNo())}});
        ArgKey.SuffixVariance = !ArgKey.SuffixVariance;
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
        RetKey.Base = RetKey.Base.pushLabel({retypd::OutLabel{}});
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
            {retypd::InLabel{std::to_string(Arg->getArgNo())}});
        ArgKey.SuffixVariance = !ArgKey.SuffixVariance;
        auto *ArgNodeC = getNodeOrNull(ArgKey);
        if (ArgNodeC) {
          V2NContra.insert(Arg, ArgNodeC->key);
        }
      }
    }
  }
  // dumpV2N();
}

std::shared_ptr<ConstraintsGenerator> ConstraintsGenerator::genSummary() {
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
  CG.addRecalls(N);
  CG.addForgets(N);
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
  CG.addRecalls(NContra);
  CG.addForgets(NContra);
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
  // temporary allow by key for ConstantExpr
  if (auto UC = std::get_if<UConstant>(&Val)) {
    if (auto CE = llvm::dyn_cast<ConstantExpr>(UC->Val)) {
      auto Dtv = convertTypeVar(Val, User, OpInd);
      retypd::NodeKey K(Dtv, V);
      auto Node1 = getNodeOrNull(K);
      if (Node1) {
        V2N.insert(Val, retypd::NodeKey(Dtv, retypd::Covariant));
        V2N.insert(Val, retypd::NodeKey(Dtv, retypd::Contravariant));
        return *Node1;
      }
    }
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
  auto *Src = I.getReturnValue();
  if (Src == nullptr) { // ret void.
    return;
  }
  auto &SrcVar = cg.getOrInsertNode(Src, &I, 0, retypd::Covariant);
  auto &DstVar = cg.getNode(ReturnValue{.Func = I.getFunction()}, &I, 0,
                            retypd::Covariant);
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
            auto &SintNode = cg.CG.getOrCreatePrim(
                retypd::getNameForInt("sint", I.getType()), I.getType());
            cg.addSubtype(SintNode, N);
            return;
          } else if (isWithOverflowIntrinsicUnsigned(
                         Target->getIntrinsicID())) {
            auto &N = cg.createNodeCovariant(&I, nullptr, -1);
            if (N.getPNIVar()->isPNRelated()) {
              N.getPNIVar()->setNonPtr();
            }
            auto &UintNode = cg.CG.getOrCreatePrim(
                retypd::getNameForInt("uint", I.getType()), I.getType());
            cg.addSubtype(UintNode, N);
            return;
          }
        } else if (Ind == 1) {
          assert(I.getType()->isIntegerTy(1));
          auto &N = cg.createNodeCovariant(&I, nullptr, -1);

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
  auto ID = Target->getIntrinsicID();
  if (I.getType()->isAggregateType()) {
    // ignore this call and handle the value in visitExtractValue
    return true;
  }
  return false;
}

void ConstraintsGenerator::RetypdGeneratorVisitor::visitCallBase(CallBase &I) {
  auto Target = I.getCalledFunction();
  if (Target == nullptr) {
    // TODO indirect call
    std::cerr << __FILE__ << ":" << __LINE__ << ": "
              << "Warn: RetypdGenerator: indirect call not supported yet\n";
    return;
  }

  if (handleIntrinsicCall(I)) {
    return;
  } else if (cg.SCCs.count(Target)) {
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
    // create and save to CallToInstance map.
    auto &TargetNode = cg.getOrInsertNode(Target, nullptr, -1);

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
    auto &DstVar = cg.getOrInsertNode(I, nullptr, -1, retypd::Covariant);
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
  return llvm2c::getLLVMTypeSize(Elem, Ctx.pointer_size);
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
  if (!dtv.getLabels().empty() && dtv.getLabels().back().isOffset()) {
    OffsetLabel LastLabel = *dtv.getLabels().back().getAs<OffsetLabel>();
    LastLabel.range = LastLabel.range + Offset;
    return dtv.popLabel().pushLabel({LastLabel});
  } else {
    return dtv.pushLabel({OffsetLabel{.range = Offset}});
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
    DstVar = DstVar.pushLabel({retypd::LoadLabel{.Size = BitSize}});
  } else {
    DstVar = DstVar.pushLabel({retypd::StoreLabel{.Size = BitSize}});
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
    if (N.getPNIVar()->isPNRelated()) {
      N.getPNIVar()->setNonPtr();
    }
    auto &SintNode = cg.CG.getOrCreatePrim(
        retypd::getNameForInt("sint", I->getType()), I->getType());
    cg.addSubtype(SintNode, N);
    return true;
  } else if (strEq(ty, "uint")) {
    if (N.getPNIVar()->isPNRelated()) {
      N.getPNIVar()->setNonPtr();
    }
    auto &UintNode = cg.CG.getOrCreatePrim(
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
    auto &SintNode = cg.CG.getOrCreatePrim(
        retypd::getNameForInt("sint", Op->getType()), Op->getType());
    cg.addSubtype(N, SintNode);
    return true;
  } else if (strEq(ty, "uint")) {
    N.getPNIVar()->setNonPtrIfRelated();
    auto &UintNode = cg.CG.getOrCreatePrim(
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

void TypeRecovery::gen_json(std::string OutputFilename) {
  assert(false && "depricated");
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
    auto F1 = const_cast<llvm::Function *>(F);
    std::shared_ptr<ConstraintsGenerator> CG = getFuncCG(F);
    if (!CG) {
      return;
    }
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

void TypeRecovery::printAnnotatedModule(llvm::Module &M, std::string path,
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
