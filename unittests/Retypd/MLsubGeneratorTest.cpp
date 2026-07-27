#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "binarysub/binarysub.h"
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

namespace {

notdec::mlsub::ConstraintsGenerator
makeMLsubGeneratorForFunctionArgs(llvm::LLVMContext &Ctx,
                                  std::unique_ptr<llvm::Module> &M,
                                  llvm::Argument *&Arg0,
                                  llvm::Argument *&Arg1) {
  M = std::make_unique<llvm::Module>("mlsub-test", Ctx);
  auto *I32 = llvm::Type::getInt32Ty(Ctx);
  auto *FTy = llvm::FunctionType::get(llvm::Type::getVoidTy(Ctx), {I32, I32},
                                      false);
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

} // namespace

TEST(MLsub, PNDiffUnifiesRecursiveVariablePairsByDefault) {
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::Argument *Arg0 = nullptr;
  llvm::Argument *Arg1 = nullptr;
  auto CG = makeMLsubGeneratorForFunctionArgs(Ctx, M, Arg0, Arg1);

  auto LhsTy = CG.createNode(Arg0);
  auto RhsTy = CG.createNode(Arg1);
  auto *LhsPNI = &CG.PG.getPNIVar(Arg0);
  auto *RhsPNI = &CG.PG.getPNIVar(Arg1);

  EXPECT_NE(LhsPNI, RhsPNI);

  CG.addSubtype(
      binarysub::make_function(std::vector<binarysub::SimpleType>{LhsTy},
                               nullptr),
      binarysub::make_function(std::vector<binarysub::SimpleType>{RhsTy},
                               nullptr));

  EXPECT_EQ(&CG.PG.getPNIVar(Arg0), &CG.PG.getPNIVar(Arg1));
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

  EXPECT_NE(&CG.PG.getPNIVar(Arg0), &CG.PG.getPNIVar(Arg1));
  CG.releaseBinarysubState();
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
