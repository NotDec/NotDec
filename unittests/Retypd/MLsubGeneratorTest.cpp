#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "TypeRecovery/mlsub/Metadata.h"
#include "binarysub/binarysub.h"
#include "notdec/TypeRecovery/mlsub/AnonymousPolyBoundaryAnalysis.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace {

notdec::mlsub::ConstraintsGenerator
makeMLsubGeneratorForFunctionArgs(llvm::LLVMContext &Ctx,
                                  std::unique_ptr<llvm::Module> &M,
                                  llvm::Argument *&Arg0,
                                  llvm::Argument *&Arg1) {
  M = std::make_unique<llvm::Module>("mlsub-test", Ctx);
  auto *I32 = llvm::Type::getInt32Ty(Ctx);
  auto *FTy = llvm::FunctionType::get(I32, {I32, I32}, false);
  auto *F = llvm::Function::Create(FTy, llvm::Function::ExternalLinkage, "f",
                                   M.get());
  auto It = F->arg_begin();
  Arg0 = &*It++;
  Arg0->setName("arg0");
  Arg1 = &*It++;
  Arg1->setName("arg1");

  static std::set<llvm::Function *> SCCs;
  SCCs.clear();
  SCCs.insert(F);
  return notdec::mlsub::ConstraintsGenerator(
      "mlsub-test", 32, SCCs, binarysub::make_variable(0, 32));
}

void addStructLayout(
    notdec::mlsub::ConstraintsGenerator &CG, binarysub::SimpleType Root,
    const std::vector<std::pair<unsigned, unsigned>> &OffsetSizeBytes) {
  std::vector<std::pair<std::string, binarysub::SimpleType>> Fields;
  for (const auto &[Offset, SizeBytes] : OffsetSizeBytes) {
    auto FieldAddr = binarysub::make_variable(0, 32);
    auto FieldValue = binarysub::make_variable(0, SizeBytes * 8);
    CG.addSubtype(FieldAddr,
                  binarysub::make_ptr_load(FieldValue, SizeBytes * 8));
    Fields.push_back({std::to_string(Offset), FieldAddr});
  }
  CG.addSubtype(Root, binarysub::make_record(std::move(Fields)));
}

} // namespace

TEST(MLsub, PNDiffUnifiesRecursiveVariablePairsByDefault) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto LhsTy = CG.createNode(Arg0);
  auto RhsTy = CG.createNode(Arg1);
  auto *LhsPNI = &CG.getPNINode(Arg0);
  auto *RhsPNI = &CG.getPNINode(Arg1);

  EXPECT_NE(LhsPNI, RhsPNI);

  CG.addSubtype(
      binarysub::make_function(std::vector<binarysub::SimpleType>{LhsTy},
                               nullptr),
      binarysub::make_function(std::vector<binarysub::SimpleType>{RhsTy},
                               nullptr));

  EXPECT_EQ(&CG.getPNINode(Arg0), &CG.getPNINode(Arg1));
  CG.releaseBinarysubState();
}

TEST(MLsub, PNDiffRecursiveVariablePairUnificationCanBeDisabled) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);
  CG.EnablePNDiffTypeVariableClosureUnification = false;

  auto LhsTy = CG.createNode(Arg0);
  auto RhsTy = CG.createNode(Arg1);

  CG.addSubtype(
      binarysub::make_function(std::vector<binarysub::SimpleType>{LhsTy},
                               nullptr),
      binarysub::make_function(std::vector<binarysub::SimpleType>{RhsTy},
                               nullptr));

  EXPECT_NE(&CG.getPNINode(Arg0), &CG.getPNINode(Arg1));
  CG.releaseBinarysubState();
}

TEST(MLsub, CallArgSlotConflictSkipsEveryMember) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto Formal = binarysub::make_variable(0, 32);
  auto CompatibleActual = binarysub::make_variable(0, 32);
  auto ConflictingActual = binarysub::make_variable(0, 32);
  addStructLayout(CG, Formal, {{4, 4}});
  addStructLayout(CG, CompatibleActual, {{8, 4}});
  addStructLayout(CG, ConflictingActual, {{4, 8}});

  CG.CallArgStructPtrMergeCandidates.push_back(
      {.ArgIndex = 0, .ActualArg = CompatibleActual, .FormalArg = Formal});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.ArgIndex = 0, .ActualArg = ConflictingActual, .FormalArg = Formal});

  EXPECT_EQ(CG.applyCallArgStructPtrMergePolicy(), 0U);
  EXPECT_NE(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(CompatibleActual).get());
  EXPECT_NE(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(ConflictingActual).get());
  EXPECT_TRUE(CG.CallArgStructPtrMergeCandidates.empty());
  ASSERT_EQ(CG.CallSlotMergeDecisions.size(), 1U);
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Policy, "call-arg-slot-group");
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Decision, "precheck-skipped");
  EXPECT_NE(CG.CallSlotMergeDecisions[0].Reason.find("overlap-conflict"),
            std::string::npos);
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Entries.size(), 2U);
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Roots.size(), 3U);

  binarysub::release_type_graph(Formal);
  binarysub::release_type_graph(CompatibleActual);
  binarysub::release_type_graph(ConflictingActual);
  CG.releaseBinarysubState();
}

TEST(MLsub, ReturnSlotMergesEveryCompatibleMember) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto Formal = binarysub::make_variable(0, 32);
  auto FirstReturn = binarysub::make_variable(0, 32);
  auto SecondReturn = binarysub::make_variable(0, 32);
  addStructLayout(CG, Formal, {{4, 4}});
  addStructLayout(CG, FirstReturn, {{4, 4}});
  addStructLayout(CG, SecondReturn, {{8, 4}});

  CG.ReturnValueMergeCandidates.push_back(
      {.Operand = FirstReturn, .FunctionReturn = Formal});
  CG.ReturnValueMergeCandidates.push_back(
      {.Operand = SecondReturn, .FunctionReturn = Formal});

  EXPECT_EQ(CG.applyReturnValueMergePolicy(), 2U);
  EXPECT_EQ(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(FirstReturn).get());
  EXPECT_EQ(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(SecondReturn).get());
  EXPECT_TRUE(CG.ReturnValueMergeCandidates.empty());
  ASSERT_EQ(CG.CallSlotMergeDecisions.size(), 1U);
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Policy, "return-slot-group");
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Decision, "committed");
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Reason, "compatible-layouts");
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Entries.size(), 2U);

  binarysub::release_type_graph(Formal);
  CG.releaseBinarysubState();
}

TEST(MLsub, ReturnOperandMayBeTheSameNodeAsAFormalArgument) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);
  auto *Target = Arg0->getParent();

  auto ArgFormal = CG.createNode(Arg0);
  auto ReturnFormal = CG.createNode(notdec::ReturnValue{Target});
  auto ArgActual = CG.createNode(Arg1);
  addStructLayout(CG, ArgFormal, {{4, 4}});
  addStructLayout(CG, ArgActual, {{8, 4}});

  // Returning an argument reuses the exact formal node. This is normal
  // dataflow, not an alias produced by an earlier failed merge.
  CG.ReturnValueMergeCandidates.push_back(
      {.Target = Target,
       .Operand = ArgFormal,
       .FunctionReturn = ReturnFormal});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Target,
       .ArgIndex = 0,
       .ActualArg = ArgActual,
       .FormalArg = ArgFormal});

  EXPECT_EQ(CG.applyCallInterfaceMergePolicy(), 2U);
  EXPECT_EQ(binarysub::resolve_variable(ArgFormal).get(),
            binarysub::resolve_variable(ReturnFormal).get());
  EXPECT_EQ(binarysub::resolve_variable(ArgActual).get(),
            binarysub::resolve_variable(ReturnFormal).get());
  EXPECT_EQ(&CG.getPNINode(Arg0),
            &CG.getPNINode(notdec::ReturnValue{Target}));
  EXPECT_EQ(&CG.getPNINode(Arg0), &CG.getPNINode(Arg1));
  ASSERT_EQ(CG.CallSlotMergeDecisions.size(), 2U);
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Decision, "committed");
  EXPECT_EQ(CG.CallSlotMergeDecisions[1].Decision, "committed");

  CG.releaseBinarysubState();
}

TEST(MLsub, ReturnSlotConflictSkipsEveryMember) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto Formal = binarysub::make_variable(0, 32);
  auto CompatibleReturn = binarysub::make_variable(0, 32);
  auto ConflictingReturn = binarysub::make_variable(0, 32);
  addStructLayout(CG, Formal, {{4, 4}});
  addStructLayout(CG, CompatibleReturn, {{8, 4}});
  addStructLayout(CG, ConflictingReturn, {{4, 8}});

  CG.ReturnValueMergeCandidates.push_back(
      {.Operand = CompatibleReturn, .FunctionReturn = Formal});
  CG.ReturnValueMergeCandidates.push_back(
      {.Operand = ConflictingReturn, .FunctionReturn = Formal});

  EXPECT_EQ(CG.applyReturnValueMergePolicy(), 0U);
  EXPECT_NE(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(CompatibleReturn).get());
  EXPECT_NE(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(ConflictingReturn).get());
  EXPECT_TRUE(CG.ReturnValueMergeCandidates.empty());

  binarysub::release_type_graph(Formal);
  binarysub::release_type_graph(CompatibleReturn);
  binarysub::release_type_graph(ConflictingReturn);
  CG.releaseBinarysubState();
}

TEST(MLsub, CallArgSlotWithoutStructEvidenceRecordsSkippedDecision) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto Formal = binarysub::make_variable(0, 32);
  auto Actual = binarysub::make_variable(0, 32);
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.ArgIndex = 0, .ActualArg = Actual, .FormalArg = Formal});

  EXPECT_EQ(CG.applyCallArgStructPtrMergePolicy(), 0U);
  EXPECT_NE(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(Actual).get());
  ASSERT_EQ(CG.CallSlotMergeDecisions.size(), 1U);
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Decision, "precheck-skipped");
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].Reason,
            "missing-struct-evidence");
  ASSERT_EQ(CG.CallSlotMergeDecisions[0].Roots.size(), 2U);
  EXPECT_NE(CG.CallSlotMergeDecisions[0].Roots[1].find("action=skipped"),
            std::string::npos);

  binarysub::release_type_graph(Formal);
  binarysub::release_type_graph(Actual);
  CG.releaseBinarysubState();
}

TEST(MLsub, StructPointerEvidenceRequiresDirectNonzeroFieldAccess) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto RootWithoutAccess = binarysub::make_variable(0, 32);
  auto FieldWithoutAccess = binarysub::make_variable(0, 32);
  CG.addSubtype(RootWithoutAccess,
                binarysub::make_record({{"4", FieldWithoutAccess}}));
  EXPECT_FALSE(CG.hasStructPointerEvidence(RootWithoutAccess));

  auto RootAtZero = binarysub::make_variable(0, 32);
  auto FieldAtZero = binarysub::make_variable(0, 32);
  auto FieldAtZeroValue = binarysub::make_variable(0, 32);
  CG.addSubtype(FieldAtZero,
                binarysub::make_ptr_load(FieldAtZeroValue, 32));
  CG.addSubtype(RootAtZero, binarysub::make_record({{"0", FieldAtZero}}));
  EXPECT_FALSE(CG.hasStructPointerEvidence(RootAtZero));

  auto RootWithAccess = binarysub::make_variable(0, 32);
  auto FieldWithAccess = binarysub::make_variable(0, 32);
  auto FieldValue = binarysub::make_variable(0, 32);
  CG.addSubtype(FieldWithAccess, binarysub::make_ptr_load(FieldValue, 32));
  CG.addSubtype(RootWithAccess,
                binarysub::make_record({{"4", FieldWithAccess}}));
  EXPECT_TRUE(CG.hasStructPointerEvidence(RootWithAccess));

  binarysub::release_type_graph(RootWithoutAccess);
  binarysub::release_type_graph(RootAtZero);
  binarysub::release_type_graph(RootWithAccess);
  CG.releaseBinarysubState();
}

TEST(MLsub, PointerLocalSubtypeModeDoesNotRequireStructEvidence) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto LHS = CG.createNode(Arg0);
  auto RHS = CG.createNode(Arg1);
  auto LHSField = binarysub::make_variable(0, 32);
  auto RHSField = binarysub::make_variable(0, 32);
  CG.addSubtype(LHS, binarysub::make_record({{"4", LHSField}}));
  CG.addSubtype(RHS, binarysub::make_record({{"8", RHSField}}));

  CG.LocalSubtypeMode =
      notdec::mlsub::ConstraintsGenerator::LocalSubtypeMergeMode::StructPointer;
  auto DefaultDecision = CG.shouldMergeSameFunctionStructPtrSubtype(LHS, RHS);
  ASSERT_TRUE(DefaultDecision);
  EXPECT_FALSE(DefaultDecision.value());

  CG.LocalSubtypeMode =
      notdec::mlsub::ConstraintsGenerator::LocalSubtypeMergeMode::Pointer;
  auto PointerDecision = CG.shouldMergeSameFunctionStructPtrSubtype(LHS, RHS);
  ASSERT_TRUE(PointerDecision);
  EXPECT_TRUE(PointerDecision.value());

  CG.releaseBinarysubState();
}

TEST(MLsub, LocalSubtypeRescanOnlyOnPointerEvidenceUpgrade) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);
  CG.EnablePNDiffTypeVariableClosureUnification = false;

  auto LHS = CG.createNode(Arg0);
  auto RHS = CG.createNode(Arg1);
  CG.addSubtype(LHS, RHS);
  const auto BeforeEvidence = CG.LocalSubtypeHookChecks;

  CG.addSubtype(LHS, binarysub::make_function({}, nullptr));
  const auto AfterFirstEvidence = CG.LocalSubtypeHookChecks;
  EXPECT_GT(AfterFirstEvidence, BeforeEvidence);

  // A second function bound does not change pointer evidence. The existing
  // variable neighbor therefore must not be rescanned.
  CG.addSubtype(LHS,
                binarysub::make_function({binarysub::make_variable(0, 32)},
                                         nullptr));
  EXPECT_EQ(CG.LocalSubtypeHookChecks, AfterFirstEvidence);
  CG.releaseBinarysubState();
}

TEST(MLsub, AllLocalSubtypeModeStillRequiresSameFunction) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);
  auto LHS = CG.createNode(Arg0);
  auto RHS = CG.createNode(Arg1);

  CG.LocalSubtypeMode =
      notdec::mlsub::ConstraintsGenerator::LocalSubtypeMergeMode::AllLocal;
  auto SameFunction = CG.shouldMergeSameFunctionStructPtrSubtype(LHS, RHS);
  ASSERT_TRUE(SameFunction);
  EXPECT_TRUE(SameFunction.value());

  auto *I32 = llvm::Type::getInt32Ty(Ctx);
  auto *OtherTy = llvm::FunctionType::get(I32, {I32}, false);
  auto *Other = llvm::Function::Create(OtherTy, llvm::Function::ExternalLinkage,
                                       "other", M.get());
  auto OtherArg = CG.createNode(Other->getArg(0));
  auto CrossFunction =
      CG.shouldMergeSameFunctionStructPtrSubtype(LHS, OtherArg);
  ASSERT_TRUE(CrossFunction);
  EXPECT_FALSE(CrossFunction.value());

  CG.releaseBinarysubState();
}

TEST(MLsub, EarlyCallInterfaceModeMergesBeforeDeferredSubtype) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);
  auto *Target = Arg0->getParent();

  auto Formal = CG.createNode(Arg0);
  auto Actual = CG.createNode(Arg1);
  addStructLayout(CG, Formal, {{4, 4}});
  addStructLayout(CG, Actual, {{8, 4}});
  auto FormalFunc = binarysub::make_function({Formal}, nullptr);
  auto ActualFunc = binarysub::make_function({Actual}, nullptr);

  // A real CallBase is kept in the deferred record for diagnostics, but this
  // test supplies the SimpleType call signature directly.
  auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", Target);
  llvm::IRBuilder<> Builder(Entry);
  auto *Call = Builder.CreateCall(Target, {Arg0, Arg1});
  Builder.CreateRet(Call);

  CG.EnableEarlyCallInterfaceMerge = true;
  CG.deferCallConstraint(*Call, *Target, FormalFunc, ActualFunc, FormalFunc);
  CG.applyDeferredCallConstraints();

  EXPECT_EQ(CG.EarlyCallInterfaceMerged, 1U);
  EXPECT_EQ(binarysub::resolve_variable(Formal).get(),
            binarysub::resolve_variable(Actual).get());
  EXPECT_TRUE(CG.DeferredCallConstraints.empty());
  EXPECT_TRUE(CG.CallArgStructPtrMergeCandidates.empty());
  EXPECT_TRUE(CG.SubtypeConstraintFailures.empty());

  CG.releaseBinarysubState();
}

TEST(MLsub, DifferentFormalSlotsRemainIndependent) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto FirstFormal = binarysub::make_variable(0, 32);
  auto FirstActual = binarysub::make_variable(0, 32);
  auto SecondFormal = binarysub::make_variable(0, 32);
  auto SecondActual = binarysub::make_variable(0, 32);
  addStructLayout(CG, FirstFormal, {{4, 4}});
  addStructLayout(CG, FirstActual, {{8, 4}});
  addStructLayout(CG, SecondFormal, {{12, 4}});
  addStructLayout(CG, SecondActual, {{16, 4}});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.ArgIndex = 0, .ActualArg = FirstActual, .FormalArg = FirstFormal});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.ArgIndex = 1, .ActualArg = SecondActual, .FormalArg = SecondFormal});

  EXPECT_EQ(CG.applyCallArgStructPtrMergePolicy(), 2U);
  EXPECT_EQ(binarysub::resolve_variable(FirstFormal).get(),
            binarysub::resolve_variable(FirstActual).get());
  EXPECT_EQ(binarysub::resolve_variable(SecondFormal).get(),
            binarysub::resolve_variable(SecondActual).get());
  EXPECT_NE(binarysub::resolve_variable(FirstFormal).get(),
            binarysub::resolve_variable(SecondFormal).get());

  binarysub::release_type_graph(FirstFormal);
  binarysub::release_type_graph(SecondFormal);
  CG.releaseBinarysubState();
}

TEST(MLsub, RecursiveCrossFormalMergeRollsBackWholeFunction) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);
  auto *Target = Arg0->getParent();

  auto Formal0 = CG.createNode(Arg0);
  auto Formal1 = CG.createNode(Arg1);
  auto Actual0 = binarysub::make_variable(0, 32);
  auto Actual1 = binarysub::make_variable(0, 32);
  auto SkippedFormal = binarysub::make_variable(0, 32);
  auto SkippedActual = binarysub::make_variable(0, 32);
  addStructLayout(CG, Formal0, {{4, 4}});
  addStructLayout(CG, Actual0, {{8, 4}});
  addStructLayout(CG, Formal1, {{12, 4}});
  addStructLayout(CG, Actual1, {{16, 4}});

  // This old edge is harmless until Actual0 is merged into Formal0. Rewriting
  // it then creates Formal0 <: Formal1 and invokes the same-function hook.
  CG.addSubtype(Actual0, Formal1);
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Target,
       .ArgIndex = 0,
       .ActualArg = Actual0,
       .FormalArg = Formal0});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Target,
       .ArgIndex = 1,
       .ActualArg = Actual1,
       .FormalArg = Formal1});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Target,
       .ArgIndex = 2,
       .ActualArg = SkippedActual,
       .FormalArg = SkippedFormal});

  EXPECT_NE(&CG.getPNINode(Arg0), &CG.getPNINode(Arg1));
  EXPECT_EQ(CG.applyCallInterfaceMergePolicy(), 0U);
  EXPECT_EQ(binarysub::resolve_variable(Formal0).get(), Formal0.get());
  EXPECT_EQ(binarysub::resolve_variable(Formal1).get(), Formal1.get());
  EXPECT_EQ(binarysub::resolve_variable(Actual0).get(), Actual0.get());
  EXPECT_EQ(binarysub::resolve_variable(Actual1).get(), Actual1.get());
  EXPECT_NE(&CG.getPNINode(Arg0), &CG.getPNINode(Arg1));

  ASSERT_EQ(CG.CallSlotMergeDecisions.size(), 3U);
  EXPECT_EQ(CG.CallSlotMergeDecisions[0].TransactionId,
            CG.CallSlotMergeDecisions[1].TransactionId);
  EXPECT_EQ(CG.CallSlotMergeDecisions[1].TransactionId,
            CG.CallSlotMergeDecisions[2].TransactionId);
  for (std::size_t I = 0; I < 2; ++I) {
    const auto &Decision = CG.CallSlotMergeDecisions[I];
    EXPECT_EQ(Decision.Target, "f");
    EXPECT_EQ(Decision.Decision, "rolled-back");
    EXPECT_NE(Decision.Reason.find("crosses formal slots"), std::string::npos);
    EXPECT_GT(Decision.TouchedNodes, 0U);
    EXPECT_FALSE(Decision.RecursiveMerges.empty());
  }
  const auto &Skipped = CG.CallSlotMergeDecisions[2];
  EXPECT_EQ(Skipped.Target, "f");
  EXPECT_EQ(Skipped.Decision, "precheck-skipped");
  EXPECT_EQ(Skipped.Reason, "missing-struct-evidence");
  EXPECT_EQ(Skipped.TouchedNodes, 0U);
  EXPECT_TRUE(Skipped.RecursiveMerges.empty());
  for (const auto &Root : Skipped.Roots) {
    EXPECT_NE(Root.find("action=skipped-missing-struct-evidence"),
              std::string::npos);
  }

  binarysub::release_type_graph(Actual0);
  binarysub::release_type_graph(Actual1);
  binarysub::release_type_graph(SkippedFormal);
  binarysub::release_type_graph(SkippedActual);
  CG.releaseBinarysubState();
}

TEST(MLsub, RecursiveMergeKeepsFormalLabelsFromOtherTargets) {
  llvm::LLVMContext Ctx;
  auto M = std::make_unique<llvm::Module>("cross-target-call-merge-test", Ctx);
  auto *PtrTy = llvm::PointerType::getUnqual(Ctx);
  auto *FTy = llvm::FunctionType::get(llvm::Type::getVoidTy(Ctx),
                                      {PtrTy, PtrTy}, false);
  auto *Source = llvm::Function::Create(
      FTy, llvm::Function::ExternalLinkage, "source", M.get());
  auto *Target = llvm::Function::Create(
      FTy, llvm::Function::ExternalLinkage, "target", M.get());
  auto SourceIt = Source->arg_begin();
  auto *SourceArg0 = &*SourceIt++;
  auto *SourceArg1 = &*SourceIt++;

  static std::set<llvm::Function *> SCCs;
  SCCs.clear();
  SCCs.insert(Source);
  SCCs.insert(Target);
  notdec::mlsub::ConstraintsGenerator CG(
      "cross-target-call-merge-test", 32, SCCs,
      binarysub::make_variable(0, 32));

  auto SourceFormal0 = CG.createNode(SourceArg0);
  auto SourceFormal1 = CG.createNode(SourceArg1);
  auto TargetFormal = binarysub::make_variable(0, 32);
  auto EvidenceActual = binarysub::make_variable(0, 32);

  // This edge is initially harmless because neither source formal has struct
  // evidence. The target transaction first moves SourceFormal1 (and its LLVM
  // owner handle) into TargetFormal, then adds EvidenceActual's record bound.
  // The same-function hook consequently tries to merge the target root into
  // SourceFormal0. Global formal labels must reject and roll back that step.
  CG.addSubtype(SourceFormal1, SourceFormal0);
  addStructLayout(CG, EvidenceActual, {{8, 4}});

  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Source,
       .ArgIndex = 0,
       .ActualArg = SourceFormal0,
       .FormalArg = SourceFormal0});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Source,
       .ArgIndex = 1,
       .ActualArg = SourceFormal1,
       .FormalArg = SourceFormal1});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Target,
       .ArgIndex = 0,
       .ActualArg = SourceFormal1,
       .FormalArg = TargetFormal});
  CG.CallArgStructPtrMergeCandidates.push_back(
      {.Target = Target,
       .ArgIndex = 0,
       .ActualArg = EvidenceActual,
       .FormalArg = TargetFormal});

  EXPECT_EQ(CG.applyCallInterfaceMergePolicy(), 0U);
  EXPECT_EQ(binarysub::resolve_variable(SourceFormal0).get(),
            SourceFormal0.get());
  EXPECT_EQ(binarysub::resolve_variable(SourceFormal1).get(),
            SourceFormal1.get());
  EXPECT_EQ(binarysub::resolve_variable(TargetFormal).get(),
            TargetFormal.get());
  EXPECT_EQ(binarysub::resolve_variable(EvidenceActual).get(),
            EvidenceActual.get());

  auto TargetDecision = std::find_if(
      CG.CallSlotMergeDecisions.begin(), CG.CallSlotMergeDecisions.end(),
      [](const auto &Decision) { return Decision.Target == "target"; });
  ASSERT_NE(TargetDecision, CG.CallSlotMergeDecisions.end());
  EXPECT_EQ(TargetDecision->Decision, "rolled-back");
  EXPECT_NE(TargetDecision->Reason.find("source::arg1"), std::string::npos);
  EXPECT_NE(TargetDecision->Reason.find("source::arg0"), std::string::npos);

  binarysub::release_type_graph(TargetFormal);
  binarysub::release_type_graph(EvidenceActual);
  CG.releaseBinarysubState();
}

TEST(MLsub, GenericFFmpegContextAPIsArePolymorphic) {
  llvm::LLVMContext Ctx;
  auto M = std::make_unique<llvm::Module>("generic-ffmpeg-api-test", Ctx);
  auto *PtrTy = llvm::PointerType::getUnqual(Ctx);
  auto *VoidTy = llvm::Type::getVoidTy(Ctx);
  auto *I32Ty = llvm::Type::getInt32Ty(Ctx);
  auto *LogTy = llvm::FunctionType::get(
      VoidTy, {PtrTy, I32Ty, PtrTy}, /*isVarArg=*/true);
  auto *SetTy = llvm::FunctionType::get(
      I32Ty, {PtrTy, PtrTy, PtrTy, I32Ty}, /*isVarArg=*/false);
  auto *Log = llvm::Function::Create(
      LogTy, llvm::Function::ExternalLinkage, "av_log", M.get());
  auto *OptSet = llvm::Function::Create(
      SetTy, llvm::Function::ExternalLinkage, "av_opt_set", M.get());
  auto *Ordinary = llvm::Function::Create(
      SetTy, llvm::Function::ExternalLinkage, "ordinary", M.get());

  llvm::ModuleAnalysisManager MAM;
  notdec::mlsub::MLsubRecovery Recovery(*M, MAM);
  Recovery.markBuiltinPolymorphicBufferFunctions(*M);

  EXPECT_NE(Log->getMetadata(
                notdec::mlsub::KIND_MLSUB_POLYMORPHIC_FUNCTION),
            nullptr);
  EXPECT_NE(OptSet->getMetadata(
                notdec::mlsub::KIND_MLSUB_POLYMORPHIC_FUNCTION),
            nullptr);
  EXPECT_EQ(Ordinary->getMetadata(
                notdec::mlsub::KIND_MLSUB_POLYMORPHIC_FUNCTION),
            nullptr);
}

TEST(MLsub, AnonymousPolymorphicBoundaryEvidenceIgnoresNames) {
  llvm::LLVMContext Ctx;
  auto M = std::make_unique<llvm::Module>("anonymous-poly-test", Ctx);
  M->setDataLayout("e-p:64:64");
  auto *PtrTy = llvm::PointerType::getUnqual(Ctx);
  auto *VoidTy = llvm::Type::getVoidTy(Ctx);
  auto *I8Ty = llvm::Type::getInt8Ty(Ctx);
  auto *I16Ty = llvm::Type::getInt16Ty(Ctx);
  auto *I64Ty = llvm::Type::getInt64Ty(Ctx);

  // The base allocator is deliberately just an anonymous-looking declaration.
  // The wrapper must be recognized from return forwarding and heterogeneous
  // caller uses, without consulting either symbol.
  auto *AllocatorTy = llvm::FunctionType::get(PtrTy, {I64Ty}, false);
  auto *BaseAllocator = llvm::Function::Create(
      AllocatorTy, llvm::Function::ExternalLinkage, "source_alloc", M.get());
  auto *Wrapper = llvm::Function::Create(
      AllocatorTy, llvm::Function::ExternalLinkage, "project_alloc", M.get());
  {
    auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", Wrapper);
    llvm::IRBuilder<> B(Entry);
    B.CreateRet(B.CreateCall(BaseAllocator, {Wrapper->getArg(0)}));
  }

  auto *Factory = llvm::Function::Create(
      AllocatorTy, llvm::Function::ExternalLinkage, "fixed_factory", M.get());
  {
    auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", Factory);
    llvm::IRBuilder<> B(Entry);
    auto *Storage = B.CreateCall(BaseAllocator, {B.getInt64(16)});
    auto *Field = B.CreateGEP(I8Ty, Storage, B.getInt64(8));
    B.CreateStore(B.getInt64(7), Field);
    B.CreateRet(Storage);
  }

  auto *SinkTy = llvm::FunctionType::get(
      VoidTy, {PtrTy, PtrTy, I64Ty}, false);
  auto *Sink = llvm::Function::Create(SinkTy, llvm::Function::ExternalLinkage,
                                      "byte_sink", M.get());
  auto *RawWrapper = llvm::Function::Create(
      SinkTy, llvm::Function::ExternalLinkage, "project_write", M.get());
  {
    auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", RawWrapper);
    llvm::IRBuilder<> B(Entry);
    auto It = RawWrapper->arg_begin();
    llvm::Value *Owner = &*It++;
    llvm::Value *Buffer = &*It++;
    llvm::Value *Length = &*It++;
    B.CreateCall(Sink, {Owner, Buffer, Length});
    B.CreateRetVoid();
  }

  auto *ReleaseTy = llvm::FunctionType::get(VoidTy, {PtrTy}, false);
  auto *BaseRelease = llvm::Function::Create(
      ReleaseTy, llvm::Function::ExternalLinkage, "source_release", M.get());
  auto *Release = llvm::Function::Create(
      ReleaseTy, llvm::Function::ExternalLinkage, "project_release", M.get());
  auto *ReleaseCounter = new llvm::GlobalVariable(
      *M, I64Ty, false, llvm::GlobalValue::ExternalLinkage,
      llvm::ConstantInt::get(I64Ty, 0), "release_counter");
  {
    auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", Release);
    auto *Forward = llvm::BasicBlock::Create(Ctx, "forward", Release);
    auto *Done = llvm::BasicBlock::Create(Ctx, "done", Release);
    llvm::IRBuilder<> B(Entry);
    B.CreateCondBr(B.CreateICmpEQ(Release->getArg(0),
                                 llvm::ConstantPointerNull::get(PtrTy)),
                   Done, Forward);
    B.SetInsertPoint(Forward);
    llvm::Value *OldCount = B.CreateLoad(I64Ty, ReleaseCounter);
    B.CreateStore(B.CreateAdd(OldCount, B.getInt64(1)), ReleaseCounter);
    B.CreateCall(BaseRelease, {Release->getArg(0)});
    B.CreateBr(Done);
    B.SetInsertPoint(Done);
    B.CreateRetVoid();
  }

  auto *FixedDestructor = llvm::Function::Create(
      ReleaseTy, llvm::Function::ExternalLinkage, "fixed_destructor", M.get());
  {
    auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", FixedDestructor);
    llvm::IRBuilder<> B(Entry);
    auto *Field = B.CreateGEP(I8Ty, FixedDestructor->getArg(0), B.getInt64(8));
    B.CreateLoad(I64Ty, Field);
    B.CreateCall(BaseRelease, {FixedDestructor->getArg(0)});
    B.CreateRetVoid();
  }

  // Four structurally different allocation-result and raw-buffer uses provide
  // call-site diversity.  The owner objects intentionally keep one common
  // shape, so whole-function raw-buffer classification would be observably
  // less precise than slot-level evidence.
  for (unsigned Index = 0; Index < 4; ++Index) {
    auto *CallerTy = llvm::FunctionType::get(VoidTy, {}, false);
    auto *Caller = llvm::Function::Create(
        CallerTy, llvm::Function::ExternalLinkage,
        "caller" + std::to_string(Index), M.get());
    auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", Caller);
    llvm::IRBuilder<> B(Entry);

    llvm::Value *Allocated = B.CreateCall(Wrapper, {B.getInt64(32 + Index)});
    llvm::Value *DirectAllocated =
        B.CreateCall(BaseAllocator, {B.getInt64(64 + Index)});
    if (Index == 0) {
      B.CreateStore(B.getInt32(1), Allocated);
      B.CreateStore(B.getInt8(1), DirectAllocated);
    } else if (Index == 1) {
      auto *Field = B.CreateGEP(I8Ty, Allocated, B.getInt64(8));
      B.CreateStore(B.getInt64(2), Field);
      auto *DirectField = B.CreateGEP(I8Ty, DirectAllocated, B.getInt64(4));
      B.CreateStore(B.getInt32(2), DirectField);
    } else if (Index == 2) {
      auto *Field = B.CreateGEP(I8Ty, Allocated, B.getInt64(16));
      B.CreateLoad(I16Ty, Field);
      auto *DirectField = B.CreateGEP(I8Ty, DirectAllocated, B.getInt64(12));
      B.CreateLoad(I16Ty, DirectField);
    } else {
      B.CreateLoad(I8Ty, Allocated);
      auto *DirectField = B.CreateGEP(I8Ty, DirectAllocated, B.getInt64(24));
      B.CreateLoad(I64Ty, DirectField);
    }

    auto *Owner = B.CreateAlloca(I64Ty);
    B.CreateStore(B.getInt64(Index), Owner);
    auto *Buffer = B.CreateAlloca(I8Ty, B.getInt32(8 + Index));
    auto *Byte = B.CreateGEP(I8Ty, Buffer, B.getInt64(Index));
    B.CreateStore(B.getInt8(Index), Byte);
    B.CreateCall(RawWrapper, {Owner, Buffer, B.getInt64(8 + Index)});
    llvm::Value *Released = nullptr;
    if (Index == 0) {
      Released = Allocated;
    } else if (Index == 1) {
      Released = Owner;
    } else if (Index == 2) {
      Released = Buffer;
    } else {
      Released = Byte;
    }
    B.CreateCall(Release, {Released});
    B.CreateRetVoid();
  }

  // Keep a real caller for the factory so its fixed initialization appears in
  // the same report and exercises the allocator negative gate.
  auto *FactoryCallerTy = llvm::FunctionType::get(VoidTy, {}, false);
  auto *FactoryCaller = llvm::Function::Create(
      FactoryCallerTy, llvm::Function::ExternalLinkage, "factory_caller",
      M.get());
  {
    auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", FactoryCaller);
    llvm::IRBuilder<> B(Entry);
    B.CreateCall(Factory, {B.getInt64(16)});
    auto *Fixed = B.CreateAlloca(I64Ty, B.getInt32(2));
    B.CreateCall(FixedDestructor, {Fixed});
    // Real optimized modules may pass poison/null through an irrelevant slot.
    // Such constants have no materialized use list and must remain auditable.
    B.CreateCall(RawWrapper,
                 {llvm::PoisonValue::get(PtrTy),
                  llvm::PoisonValue::get(PtrTy), B.getInt64(0)});
    B.CreateRetVoid();
  }

  auto Analyze = [&]() {
    auto Rows = notdec::mlsub::analyzeAnonymousPolymorphicBoundaries(*M);
    std::map<const llvm::Function *, notdec::mlsub::AnonymousPolyBoundaryEvidence>
        ByFunction;
    for (auto &Row : Rows) {
      ByFunction.emplace(Row.Function, std::move(Row));
    }
    return ByFunction;
  };

  auto BeforeRename = Analyze();
  ASSERT_NE(BeforeRename.find(Wrapper), BeforeRename.end());
  ASSERT_NE(BeforeRename.find(Factory), BeforeRename.end());
  ASSERT_NE(BeforeRename.find(RawWrapper), BeforeRename.end());
  ASSERT_NE(BeforeRename.find(Release), BeforeRename.end());
  ASSERT_NE(BeforeRename.find(FixedDestructor), BeforeRename.end());
  EXPECT_EQ(BeforeRename.at(Wrapper).AllocatorConfidence, "high");
  EXPECT_TRUE(BeforeRename.at(Wrapper).PureReturnedCallForwarder);
  EXPECT_NE(BeforeRename.at(Factory).AllocatorConfidence, "high");
  EXPECT_TRUE(BeforeRename.at(Factory).ReturnedStorageAccessedInBody);

  const auto &RawSlots = BeforeRename.at(RawWrapper).RawBufferSlots;
  ASSERT_EQ(RawSlots.size(), 2U);
  EXPECT_EQ(RawSlots[1].ArgIndex, 1U);
  EXPECT_EQ(RawSlots[1].Confidence, "high");
  EXPECT_GT(RawSlots[1].Score, RawSlots[0].Score);
  ASSERT_EQ(BeforeRename.at(Release).DeallocatorSlots.size(), 1U);
  EXPECT_EQ(BeforeRename.at(Release).DeallocatorSlots[0].Confidence, "high");
  ASSERT_EQ(BeforeRename.at(FixedDestructor).DeallocatorSlots.size(), 1U);
  EXPECT_EQ(BeforeRename.at(FixedDestructor).DeallocatorSlots[0].Confidence,
            "low");

  unsigned RenameIndex = 0;
  for (llvm::Function &F : *M) {
    F.setName("f" + std::to_string(RenameIndex++));
  }
  auto AfterRename = Analyze();
  ASSERT_EQ(BeforeRename.size(), AfterRename.size());
  for (const auto &[Function, Before] : BeforeRename) {
    const auto &After = AfterRename.at(Function);
    EXPECT_EQ(Before.AllocatorScore, After.AllocatorScore);
    EXPECT_EQ(Before.AllocatorConfidence, After.AllocatorConfidence);
    ASSERT_EQ(Before.RawBufferSlots.size(), After.RawBufferSlots.size());
    for (std::size_t I = 0; I < Before.RawBufferSlots.size(); ++I) {
      EXPECT_EQ(Before.RawBufferSlots[I].Score,
                After.RawBufferSlots[I].Score);
      EXPECT_EQ(Before.RawBufferSlots[I].Confidence,
                After.RawBufferSlots[I].Confidence);
    }
    ASSERT_EQ(Before.DeallocatorSlots.size(), After.DeallocatorSlots.size());
    for (std::size_t I = 0; I < Before.DeallocatorSlots.size(); ++I) {
      EXPECT_EQ(Before.DeallocatorSlots[I].Score,
                After.DeallocatorSlots[I].Score);
      EXPECT_EQ(Before.DeallocatorSlots[I].Confidence,
                After.DeallocatorSlots[I].Confidence);
    }
  }
}

TEST(MLsub, PhiNodeCanBeUsedByAnEarlierListedBlock) {
  llvm::LLVMContext Ctx;
  auto M = std::make_unique<llvm::Module>("phi-block-order-test", Ctx);
  auto *PtrTy = llvm::PointerType::getUnqual(Ctx);
  auto *FTy = llvm::FunctionType::get(llvm::Type::getVoidTy(Ctx), {PtrTy},
                                      false);
  auto *F = llvm::Function::Create(FTy, llvm::Function::ExternalLinkage, "f",
                                   M.get());

  // LLVM block order does not have to follow control-flow order. Put the
  // latch before the header so the visitor sees the PHI use before its def.
  auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", F);
  auto *Latch = llvm::BasicBlock::Create(Ctx, "latch", F);
  auto *Header = llvm::BasicBlock::Create(Ctx, "header", F);
  llvm::IRBuilder<> HeaderBuilder(Header);
  auto *Phi = HeaderBuilder.CreatePHI(PtrTy, 2, "p");
  HeaderBuilder.CreateBr(Latch);

  llvm::IRBuilder<> EntryBuilder(Entry);
  EntryBuilder.CreateBr(Header);
  llvm::IRBuilder<> LatchBuilder(Latch);
  LatchBuilder.CreatePtrToInt(Phi, llvm::Type::getInt32Ty(Ctx));
  LatchBuilder.CreateBr(Header);
  Phi->addIncoming(F->getArg(0), Entry);
  Phi->addIncoming(Phi, Latch);

  static std::set<llvm::Function *> SCCs;
  SCCs.clear();
  SCCs.insert(F);
  notdec::mlsub::ConstraintsGenerator CG(
      "phi-block-order-test", 32, SCCs, binarysub::make_variable(0, 32));
  CG.run();
  EXPECT_NE(CG.getNodeOrNull(Phi), nullptr);
  CG.releaseBinarysubState();
}

TEST(MLsub, OpaqueBodySkipsInstructionConstraints) {
  llvm::LLVMContext Ctx;
  auto M = std::make_unique<llvm::Module>("opaque-body-test", Ctx);
  auto *I32 = llvm::Type::getInt32Ty(Ctx);
  auto *FTy = llvm::FunctionType::get(I32, {I32, I32}, false);
  auto *F = llvm::Function::Create(FTy, llvm::Function::ExternalLinkage, "f",
                                   M.get());
  auto *Entry = llvm::BasicBlock::Create(Ctx, "entry", F);
  llvm::IRBuilder<> Builder(Entry);
  auto *Add = Builder.CreateAdd(F->getArg(0), F->getArg(1), "sum");
  Builder.CreateRet(Add);

  static std::set<llvm::Function *> SCCs;
  SCCs.clear();
  SCCs.insert(F);

  notdec::mlsub::ConstraintsGenerator CG(
      "opaque-body-test", 32, SCCs, binarysub::make_variable(0, 32), nullptr,
      nullptr, 0, nullptr, nullptr, false, {F});
  CG.run();
  EXPECT_EQ(CG.getNodeOrNull(Add), nullptr);
  EXPECT_NE(CG.getNodeOrNull(F->getArg(0)), nullptr);
  EXPECT_NE(CG.getNodeOrNull(F->getArg(1)), nullptr);
  CG.releaseBinarysubState();

  notdec::mlsub::ConstraintsGenerator CG2(
      "opaque-body-test", 32, SCCs, binarysub::make_variable(0, 32));
  CG2.run();
  EXPECT_NE(CG2.getNodeOrNull(Add), nullptr);
  CG2.releaseBinarysubState();
}

TEST(MLsub, EVMStorageDirectSlotConnectsStoreToLoad) {
  static std::set<llvm::Function *> SCCs;
  SCCs.clear();

  auto MemoryType = binarysub::make_variable(0, 32);
  auto StorageType = binarysub::make_variable(0, 32);
  std::map<std::string, binarysub::SimpleType> StorageFields;
  notdec::mlsub::ConstraintsGenerator CG("storage-test", 32, SCCs, MemoryType,
                                         StorageType, &StorageFields);

  auto StoredTy = binarysub::make_variable(0, 256);
  auto LoadedTy = binarysub::make_variable(0, 256);
  auto FieldTy = CG.getOrCreateStorageField("slot:0");

  CG.addSubtype(FieldTy, binarysub::make_ptr_store(StoredTy, 256));
  CG.addSubtype(FieldTy, binarysub::make_ptr_load(LoadedTy, 256));

  auto *LoadedVar = LoadedTy->getAsVariableState();
  ASSERT_NE(LoadedVar, nullptr);
  EXPECT_NE(std::find(LoadedVar->lowerBounds.begin(),
                      LoadedVar->lowerBounds.end(), StoredTy),
            LoadedVar->lowerBounds.end());

  CG.releaseBinarysubState();
  binarysub::release_type_graph(StorageType);
}
