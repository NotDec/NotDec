#include "TypeRecovery/mlsub/TypeBuilder.h"
#include "binarysub/binarysub-primitive-semantics.h"
#include "binarysub/binarysub.h"
#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <memory>

TEST(Retypd, TypeBuilderSemanticPrimitiveAliasTest) {
  static constexpr const char *Dot = R"dot(
      digraph win_uint32 {
        graph [base="uint", bits="32", namespace="win"];
        root [special_role="root"];
        HANDLE;
        HWND;
        HWND -> HANDLE;
        HANDLE -> root;
      }
  )dot";

  binarysub::clearGlobalPrimitiveSemanticRegistry();
  auto Family = binarysub::globalPrimitiveSemanticRegistry().registerFamilyFromDot(
      Dot, "<typebuilder-test>");
  ASSERT_TRUE(Family);

  llvm::LLVMContext LLVMCtx;
  auto M = std::make_unique<llvm::Module>("semantic-primitive-typebuilder",
                                          LLVMCtx);
  M->setDataLayout("e-p:32:32");

  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(HCtx, M->getDataLayout());
  notdec::mlsub::TypeBuilder TB(TBParent);

  auto SemanticTy = binarysub::make_uprimitivetype("prim.uint32.win.HWND", 32);
  auto *HTy = TB.convert(SemanticTy);

  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isTypedefType());

  auto *Decl = HTy->getAsTypedefDecl();
  ASSERT_NE(Decl, nullptr);
  EXPECT_EQ(Decl->getComment(),
            "semantic primitive lattice node: prim.uint32.win.HWND");

  auto *IntTy = llvm::dyn_cast<notdec::ast::IntegerType>(Decl->getType());
  ASSERT_NE(IntTy, nullptr);
  EXPECT_EQ(IntTy->getBitSize(), 32u);
  EXPECT_TRUE(IntTy->isUnsigned());

  binarysub::clearGlobalPrimitiveSemanticRegistry();
}

TEST(Retypd, TypeBuilderTopFieldRecordLayoutTest) {
  llvm::LLVMContext LLVMCtx;
  auto M = std::make_unique<llvm::Module>("typebuilder-top-field-layout",
                                          LLVMCtx);
  M->setDataLayout("e-p:32:32");

  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(HCtx, M->getDataLayout());
  notdec::mlsub::TypeBuilder TB(TBParent);

  auto RecordTy = binarysub::make_urecordtype({
      {"@0", binarysub::make_utop(32)},
      {"@4", binarysub::make_uprimitivetype("uint", 32)},
  });

  auto *HTy = TB.convert(RecordTy);
  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isRecordType());

  auto *Decl = HTy->getAsRecordDecl();
  ASSERT_NE(Decl, nullptr);
  ASSERT_EQ(Decl->getFields().size(), 2u);
  EXPECT_EQ(Decl->getFields()[0].R.Start, 0);
  ASSERT_NE(Decl->getFields()[0].Type, nullptr);
  EXPECT_TRUE(Decl->getFields()[0].Type->isTopType());
  EXPECT_EQ(Decl->getFields()[1].R.Start, 4);
}
