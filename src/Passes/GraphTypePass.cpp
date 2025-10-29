#include "Passes/ConstraintGenerator.h"
#include "TypeRecovery/NFAMinimize.h"
#include "TypeRecovery/Schema.h"
#include "notdec-llvm2c/Interface/Range.h"

#define DEBUG_TYPE "graph-pass"

using namespace llvm;

namespace notdec {

using retypd::NodeKey;
using retypd::OffsetLabel;

std::map<CGNode *, TypeInfo> ConstraintsGenerator::organizeTypes() {
  assert(TypeInfos.empty());
  // after post process, organize the node with offset edges.
  // Requirement 1. the graph has only recall, has no one edge.(after
  // determinize)
  for (auto &N : CG) {
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

    // if there is self offset loop, ignore.
    std::set<const CGEdge *> SelfEdges;
    for (auto &E : N.outEdges) {
      if (&E.getTargetNode() == &N) {
        if (auto *OL = retypd::getOffsetLabel(E.Label)) {
          SelfEdges.insert(&E);
        }
      }
    }
    // remove all self offset loop
    std::vector<std::pair<const CGNode *, retypd::EdgeLabel>> ToRemove;
    for (auto E : SelfEdges) {
      ToRemove.push_back({&E->getTargetNode(), E->getLabel()});
    }
    for (auto Ent : ToRemove) {
      CG.removeEdge(N, *const_cast<CGNode *>(Ent.first), Ent.second);
    }

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
      if (*TI.Size != 0) {
        Fields.push_back(FieldEntry{
            .R = SimpleRange{.Start = OL->range.offset, .Size = *TI.Size},
            .Edge = &E});
      }
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

  for (auto &N : CG) {
    // only consider pointer node.
    if (!N.isPNIPointer()) {
      continue;
    }
    bool mustStruct = N.isMemory();
    doBuild(N, mustStruct);
  }
  return TypeInfos;
}

void ConstraintsGenerator::mergeArrayUnions() {
  size_t MergeCount = 0;

  auto doMerge = [&](CGNode &UN, CGNode &Arr, CGNode &Other) -> bool {
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
    return true;
  };

  // 如果存在union里面一个array和一个大小完全相等的成员，那么就合并这个union。
  bool Changed = false;
  do {
    Changed = false;
    for (auto &N : CG) {
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
  } else {
    // has type infos
    TypeInfo &FTI = TypeInfos.at(&From);
    TypeInfo &TTI = TypeInfos.at(&To);
    if (TypeInfos.at(&From).isArray() && TypeInfos.at(&To).isArray()) {
      // TODO merge if the access range in edge label is different
      assert(false && "TODO");
    } else if (TypeInfos.at(&From).isUnion() && TypeInfos.at(&To).isUnion()) {
      // TODO merge all union members to one.
      assert(false && "TODO");
    } else {
      assert(false && "TODO");
    }
  }
}

} // namespace notdec
