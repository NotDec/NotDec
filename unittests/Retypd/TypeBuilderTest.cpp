#include "TypeRecovery/mlsub/TypeBuilder.h"
#include "binarysub/binarysub-primitive-semantics.h"
#include "binarysub/binarysub.h"
#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <memory>

namespace {

class ExposedTypeBuilder : public notdec::mlsub::TypeBuilder {
public:
  using notdec::mlsub::TypeBuilder::convertFieldType;
  using notdec::mlsub::TypeBuilder::TypeBuilder;
};

} // namespace

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
  notdec::mlsub::TypeBuilderContext TBParent(
      HCtx, M->getDataLayout().getPointerSize());
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

TEST(Retypd, SemanticPrimitiveStringIsBytesSubtype) {
  static constexpr const char *Dot = R"dot(
      digraph evm_uint256 {
        graph [base="uint", bits="256", namespace="evm"];
        root [special_role="root"];
        bytes;
        string;
        string -> bytes;
        bytes -> root;
      }
  )dot";

  binarysub::clearGlobalPrimitiveSemanticRegistry();
  auto Family = binarysub::globalPrimitiveSemanticRegistry().registerFamilyFromDot(
      Dot, "<evm-bytes-string-test>");
  ASSERT_TRUE(Family);

  binarysub::Cache Cache;
  auto StringTy = binarysub::make_primitive("prim.uint256.evm.string", 256);
  auto BytesTy = binarysub::make_primitive("prim.uint256.evm.bytes", 256);

  EXPECT_TRUE(binarysub::constrain(StringTy, BytesTy, Cache));
  EXPECT_FALSE(binarysub::constrain(BytesTy, StringTy, Cache));

  llvm::LLVMContext LLVMCtx;
  auto M = std::make_unique<llvm::Module>("semantic-string-typebuilder",
                                          LLVMCtx);
  M->setDataLayout("e-p:256:256");

  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(
      HCtx, M->getDataLayout().getPointerSize());
  notdec::mlsub::TypeBuilder TB(TBParent);

  auto *HTy = TB.convert(
      binarysub::make_uprimitivetype("prim.uint256.evm.string", 256));
  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isTypedefType());
  auto *Decl = HTy->getAsTypedefDecl();
  ASSERT_NE(Decl, nullptr);
  EXPECT_EQ(Decl->getComment(),
            "semantic primitive lattice node: prim.uint256.evm.string");

  binarysub::clearGlobalPrimitiveSemanticRegistry();
}

TEST(Retypd, TypeBuilderTopFieldRecordLayoutTest) {
  llvm::LLVMContext LLVMCtx;
  auto M = std::make_unique<llvm::Module>("typebuilder-top-field-layout",
                                          LLVMCtx);
  M->setDataLayout("e-p:32:32");

  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(
      HCtx, M->getDataLayout().getPointerSize());
  notdec::mlsub::TypeBuilder TB(TBParent);

  auto RecordTy = binarysub::make_urecordtype({
      {"@0", binarysub::make_utop(32)},
      {"@4", binarysub::make_upointertype(
                 binarysub::make_uprimitivetype("uint", 32),
                 binarysub::make_uprimitivetype("uint", 32), 32)},
  });

  auto *HTy = TB.convert(RecordTy);
  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isPointerType());

  auto *Pointee = HTy->getPointeeType();
  ASSERT_NE(Pointee, nullptr);
  ASSERT_TRUE(Pointee->isRecordType());

  auto *Decl = Pointee->getAsRecordDecl();
  ASSERT_NE(Decl, nullptr);
  ASSERT_EQ(Decl->getFields().size(), 2u);
  EXPECT_EQ(Decl->getFields()[0].R.Start, 0);
  ASSERT_NE(Decl->getFields()[0].Type, nullptr);
  EXPECT_TRUE(Decl->getFields()[0].Type->isTopType());
  EXPECT_EQ(Decl->getFields()[1].R.Start, 4);
}

TEST(Retypd, TypeBuilderDirectPrimitiveFieldFallsBackToFieldWidth) {
  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(HCtx, 8);
  ExposedTypeBuilder TB(TBParent);

  auto *HTy =
      TB.convertFieldType(binarysub::make_uprimitivetype("uint", 64), 1);

  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isTopType());
  EXPECT_EQ(HTy->getAs<notdec::ast::TopType>()->getBitSize(), 8u);
}

TEST(Retypd, TypeBuilderDirectBottomFieldFallsBackToTop) {
  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(HCtx, 8);
  ExposedTypeBuilder TB(TBParent);

  auto *HTy = TB.convertFieldType(binarysub::make_ubot(64), 2);

  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isTopType());
  EXPECT_EQ(HTy->getAs<notdec::ast::TopType>()->getBitSize(), 16u);
}

TEST(Retypd, TypeBuilderFieldIntersectionIgnoresPrimitiveAddressEvidence) {
  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(HCtx, 8);
  ExposedTypeBuilder TB(TBParent);

  auto Layout = binarysub::make_urecordtype({{"@0", binarysub::make_utop(8)}});
  auto Mixed = binarysub::make_uinter(
      binarysub::make_uprimitivetype("uint", 64), Layout);
  auto *HTy = TB.convertFieldType(Mixed, 1);

  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isRecordType());
  auto *Decl = HTy->getAsRecordDecl();
  ASSERT_NE(Decl, nullptr);
  ASSERT_EQ(Decl->getFields().size(), 1u);
  EXPECT_TRUE(Decl->getFields().front().Type->isTopType());
  EXPECT_FALSE(HTy->isSetInterType());
}

TEST(Retypd, TypeBuilderRecursiveRecordSetAnchorsPointeeRecord) {
  llvm::LLVMContext LLVMCtx;
  auto M = std::make_unique<llvm::Module>("typebuilder-recursive-record",
                                          LLVMCtx);
  M->setDataLayout("e-p:32:32");

  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(
      HCtx, M->getDataLayout().getPointerSize());
  notdec::mlsub::TypeBuilder TB(TBParent);

  auto RecRef = binarysub::make_utypevariable("r", 32, 1);
  auto RecordTy = binarysub::make_urecordtype({{"@0", RecRef}});
  auto BodyTy = binarysub::make_uinter(binarysub::make_ubot(32), RecordTy);
  auto RecursiveTy = binarysub::make_urecursivetype("r", BodyTy);

  auto *HTy = TB.convert(RecursiveTy);
  ASSERT_NE(HTy, nullptr);
  ASSERT_TRUE(HTy->isPointerType());

  auto *Binding =
      llvm::dyn_cast<notdec::ast::RecursiveBindingType>(HTy->getPointeeType());
  ASSERT_NE(Binding, nullptr);
  auto *Binder = Binding->getBinder();
  ASSERT_NE(Binder, nullptr);
  ASSERT_NE(Binder->getBody(), nullptr);
  ASSERT_TRUE(Binder->getBody()->isSetInterType());

  auto *BodySet = llvm::cast<notdec::ast::SetInterType>(Binder->getBody());
  ASSERT_EQ(BodySet->getTypes().size(), 2u);
  auto *RecordPtr = BodySet->getTypes()[1];
  ASSERT_TRUE(RecordPtr->isPointerType());

  auto *Pointee = RecordPtr->getPointeeType();
  ASSERT_NE(Pointee, nullptr);
  ASSERT_TRUE(Pointee->isRecordType());
  EXPECT_EQ(Binder->getAnchorDecl(), Pointee->getAsRecordDecl());
  ASSERT_EQ(Pointee->getAsRecordDecl()->getFields().size(), 1u);
  EXPECT_TRUE(
      Pointee->getAsRecordDecl()->getFields()[0].Type->isRecursiveRefType());
}

TEST(Retypd, HTypeSetPrettyPrintingFlattensChains) {
  notdec::ast::HTypeContext HCtx;

  auto *I8 = HCtx.getIntegerType(false, 8, false);
  auto *I16 = HCtx.getIntegerType(false, 16, false);
  auto *I32 = HCtx.getIntegerType(false, 32, false);
  auto *F32 = HCtx.getFloatType(false, 32);

  auto *FlatUnion = HCtx.getSetUnionType(
      false, HCtx.getSetUnionType(false, I8, I16),
      HCtx.getSetUnionType(false, I32, F32));
  auto *FlatUnionSet = llvm::dyn_cast<notdec::ast::SetUnionType>(FlatUnion);
  ASSERT_NE(FlatUnionSet, nullptr);
  EXPECT_EQ(FlatUnionSet->getTypes().size(), 4u);
  EXPECT_EQ(FlatUnion->getAsString(), "i8 | i16 | i32 | f32");

  auto *Mixed =
      HCtx.getSetInterType(false, HCtx.getSetInterType(false, I8, I16),
                           HCtx.getSetUnionType(false, I32, F32));
  auto *MixedSet = llvm::dyn_cast<notdec::ast::SetInterType>(Mixed);
  ASSERT_NE(MixedSet, nullptr);
  EXPECT_EQ(MixedSet->getTypes().size(), 3u);
  EXPECT_EQ(Mixed->getAsString(), "i8 & i16 & (i32 | f32)");

  notdec::ast::HTypeSnapshotFormatter Formatter(&HCtx);
  EXPECT_EQ(Formatter.formatType(FlatUnion), "f32 | i16 | i32 | i8");
}

TEST(Retypd, TypeBuilderSignedUnsignedByteSetPrinting) {
  llvm::LLVMContext LLVMCtx;
  auto M =
      std::make_unique<llvm::Module>("typebuilder-byte-set-print", LLVMCtx);
  M->setDataLayout("e-p:32:32");

  notdec::ast::HTypeContext HCtx;
  notdec::mlsub::TypeBuilderContext TBParent(
      HCtx, M->getDataLayout().getPointerSize());
  notdec::mlsub::TypeBuilder TB(TBParent);

  auto ByteSet = binarysub::make_uinter(
      binarysub::make_uprimitivetype("char", 8),
      binarysub::make_uprimitivetype("uint", 8));
  auto *HTy = TB.convert(ByteSet);

  ASSERT_NE(HTy, nullptr);
  EXPECT_EQ(HTy->getAsString(), "i8 & u8");

  notdec::ast::HTypeSnapshotFormatter Formatter(&HCtx);
  EXPECT_EQ(Formatter.formatType(HTy), "i8 & u8");
}
