#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/mlsub/MLsubGenerator.h"
#include "TypeRecovery/mlsub/TypeBuilder.h"
#include "TypeRecovery/retypd/Schema.h"
#include "TypeRecovery/TRContext.h"
#include "binarysub/binarysub.h"
#include "binarysub/binarysub-core.h"
#include "binarysub/binarysub-primitive-semantics.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Support/Debug.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

using notdec::retypd::Constraint;
using notdec::retypd::ConstraintGraph;
using notdec::retypd::ConstraintSummary;
using notdec::retypd::TRContext;
using notdec::retypd::TypeVariable;

static notdec::mlsub::ConstraintsGenerator
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

TypeVariable parseTV(TRContext &Ctx, llvm::StringRef str, size_t PointerSize = 32) {
  auto res = notdec::retypd::parseTypeVariable(Ctx, str, PointerSize);
  EXPECT_EQ(res.first.size(), 0);
  EXPECT_TRUE(res.second.isOk());
  if (res.second.isErr()) {
    std::cerr << res.second.msg().str() << "\n";
  }
  return res.second.get();
}

std::vector<Constraint> parse_constraints(TRContext &Ctx,
                                          std::vector<const char *> cons_str, uint32_t PointerSize) {
  std::vector<Constraint> ret;
  for (const char *con : cons_str) {
    auto res = notdec::retypd::parseSubTypeConstraint(Ctx, con, PointerSize);
    EXPECT_EQ(res.first.size(), 0);
    EXPECT_TRUE(res.second.isOk());
    if (res.second.isErr()) {
      std::cerr << res.second.msg().str() << "\n";
    }
    ret.push_back(res.second.get());
    std::cerr << notdec::retypd::toString(res.second.get()) << "\n";
  }
  return ret;
}

bool check(std::vector<notdec::retypd::SubTypeConstraint> &Cons,
           std::set<std::string> Answer) {
  EXPECT_EQ(Cons.size(), Answer.size());
  for (auto &C : Cons) {
    std::cerr << "Checking: " << notdec::retypd::toString(C) << std::endl;
    EXPECT_TRUE(Answer.count(notdec::retypd::toString(C)) == 1);
  }
  return true;
}

// A simple example from the paper.
TEST(Retypd, SaturationPaperTest) {
  std::shared_ptr<TRContext> Ctx = std::make_shared<TRContext>();
  llvm::DebugFlag = true;
  // llvm::setCurrentDebugType("retypd_graph");
  std::vector<notdec::retypd::Constraint> cons = parse_constraints(
      *Ctx, {"y <= p", "p <= x", "A <= x.store4", "y.load4 <= B"}, 32);
  std::map<TypeVariable, std::string> PNIMap = {
      {parseTV(*Ctx, "x"), "ptr 32 #1"},
      {parseTV(*Ctx, "y"), "ptr 32 #2"},
      {parseTV(*Ctx, "p"), "ptr 32 #3"},
      {parseTV(*Ctx, "A"), "int 4 #4"},
      {parseTV(*Ctx, "B"), "int 4 #5"},
      {parseTV(*Ctx, "x.store4"), "int 4 #6"},
      {parseTV(*Ctx, "y.load4"), "int 4 #7"},
  };
  ConstraintSummary Sum{.Cons = cons, .PointerSize = 32, .PNIMap = PNIMap};
  ConstraintGraph CG =
      ConstraintGraph::fromConstraints(Ctx, "SaturationPaper", Sum);

  std::set<std::string> InterestingVars = {"A", "B"};
  CG.solve();
  auto Cons = CG.simplifiedExpr(InterestingVars);
  // CG.printGraph("SaturationPaper.dot");
  std::cerr << "Simplified Constraints:" << std::endl;
  for (auto &C : Cons) {
    std::cerr << notdec::retypd::toString(C) << "\n";
  }

  check(Cons, {"A <= B"});
}

// A simple example from the paper.
TEST(Retypd, SaturationOffsetTest) {
  std::shared_ptr<TRContext> Ctx = std::make_shared<TRContext>();
  llvm::DebugFlag = true;
  // llvm::setCurrentDebugType("retypd_graph");
  std::vector<notdec::retypd::Constraint> cons =
      parse_constraints(*Ctx, {"x.@2 <= C", "C.@2 <= D", "D <= y.@4",
                               "A <= x.load4", "y.load4 <= B"}, 32);
  std::map<TypeVariable, std::string> PNIMap = {
      {parseTV(*Ctx, "x"), "ptr 32 #1"},
      {parseTV(*Ctx, "x.@2"), "ptr 32 #1"},
      {parseTV(*Ctx, "C"), "ptr 32 #1"},
      {parseTV(*Ctx, "C.@2"), "ptr 32 #1"},
      {parseTV(*Ctx, "D"), "ptr 32 #1"},
      {parseTV(*Ctx, "y"), "ptr 32 #1"},
      {parseTV(*Ctx, "y.@4"), "ptr 32 #1"},
      {parseTV(*Ctx, "x.load4"), "int 4 #2"},
      {parseTV(*Ctx, "y.load4"), "int 4 #2"},
      {parseTV(*Ctx, "A"), "int 4 #2"},
      {parseTV(*Ctx, "B"), "int 4 #2"},
  };
  ConstraintSummary Sum{.Cons = cons, .PointerSize = 32, .PNIMap = PNIMap};

  ConstraintGraph CG =
      ConstraintGraph::fromConstraints(Ctx, "SaturationOffsetTest", Sum);

  std::set<std::string> InterestingVars = {"A", "B"};
  CG.solve();
  auto Cons = CG.simplifiedExpr(InterestingVars);
  // CG.printGraph("SaturationOffsetTest.dot");
  std::cerr << "Simplified Constraints:" << std::endl;
  for (auto &C : Cons) {
    std::cerr << notdec::retypd::toString(C) << "\n";
  }

  check(Cons, {"A <= B"});
}

// A simple example from the paper.
TEST(Retypd, SlidesExampleTest) {
  std::shared_ptr<TRContext> Ctx = std::make_shared<TRContext>();
  llvm::DebugFlag = true;
  // llvm::setCurrentDebugType("retypd_graph");
  std::vector<Constraint> cons =
      parse_constraints(*Ctx, {
                                  "F.in_stack0 <= 𝛿",
                                  "𝛼 <= 𝜑",
                                  "𝛿 <= 𝜑",
                                  "𝜑.load4 <= 𝛼",
                                  "𝜑.load4.@4 <= 𝛼'",
                                  "𝛼' <= close.in_stack0",
                                  "close.out_eax <= F.out_eax",
                                  "close.in_stack0 <= #FileDescriptor",
                                  "#SuccessZ <= close.out_eax",
                              }, 32);
  std::map<TypeVariable, std::string> PNIMap = {
      {parseTV(*Ctx, "F"), "func 32 #1"},
      {parseTV(*Ctx, "F.in_stack0"), "ptr 32 #2"},
      {parseTV(*Ctx, "F.out_eax"), "int 32 #3"},
      {parseTV(*Ctx, "𝛼"), "ptr 32 #2"},
      {parseTV(*Ctx, "𝛿"), "ptr 32 #2"},
      {parseTV(*Ctx, "𝜑"), "ptr 32 #2"},
      {parseTV(*Ctx, "𝜑.load4"), "ptr 32 #2"},
      {parseTV(*Ctx, "𝜑.load4.@4"), "ptr 32 #2"},
      {parseTV(*Ctx, "𝛼'"), "ptr 32 #2"},
      {parseTV(*Ctx, "close"), "func 32 #1"},
      {parseTV(*Ctx, "close.in_stack0"), "ptr 32 #2"},
      {parseTV(*Ctx, "close.out_eax"), "int 32 #3"},
      {parseTV(*Ctx, "#FileDescriptor"), "ptr 32 #2"},
      {parseTV(*Ctx, "#SuccessZ"), "int 32 #3"},
  };
  ConstraintSummary Sum{.Cons = cons, .PointerSize = 32, .PNIMap = PNIMap};
  ConstraintGraph CG =
      ConstraintGraph::fromConstraints(Ctx, "SlideExample", Sum);
  std::set<std::string> InterestingVars;
  InterestingVars.insert("F");
  CG.solve();
  auto Cons = CG.simplifiedExpr(InterestingVars);
  // CG.printGraph("SlideExample.dot");

  std::cerr << "Simplified Constraints:" << std::endl;
  for (auto &C : Cons) {
    std::cerr << notdec::retypd::toString(C) << "\n";
  }
  check(Cons,
        {"__temp_0.load4 <= __temp_0", "#SuccessZ <= F.out_eax",
         "F.in_stack0.load4 <= __temp_0", "__temp_0.@4 <= #FileDescriptor"});
}

void printConstraints(
    const std::vector<notdec::retypd::SubTypeConstraint> &Cons) {
  std::cerr << "To Constraints: " << std::endl;
  for (auto &C : Cons) {
    std::cerr << "  " << toString(C) << "\n";
  }
}

// A test for the ExprToConstraints
TEST(Retypd, ExpToConstraint1Test) {
  using namespace notdec::retypd::rexp;
  using namespace notdec::retypd;
  std::shared_ptr<TRContext> Ctx = std::make_shared<TRContext>();
  auto Prefix = create({RecallBase{TypeVariable::CreateDtv(*Ctx, "alpha")}});
  auto Suffix = create({ForgetBase{TypeVariable::CreateDtv(*Ctx, "beta")}});

  auto StarForget1 =
      Prefix & createStar(create({ForgetLabel{LoadLabel{}}})) & Suffix;
  std::cerr << "Converting Expr: " << toString(StarForget1) << std::endl;
  printConstraints(expToConstraints(Ctx, StarForget1));

  auto StarRecall1 =
      Prefix & createStar(create({RecallLabel{LoadLabel{}}})) & Suffix;
  std::cerr << "Converting Expr: " << toString(StarRecall1) << std::endl;
  printConstraints(expToConstraints(Ctx, StarRecall1));
}

// A test for the ExprToConstraints
TEST(Retypd, EdgeLabel1) {
  using notdec::retypd::EdgeLabel;
  using notdec::retypd::TRContext;
  std::shared_ptr<TRContext> Ctx = std::make_shared<TRContext>();
  auto EL1 = notdec::retypd::RecallBase{
      .Base = notdec::retypd::TypeVariable::CreateDtv(*Ctx, "alpha"),
      .V = notdec::retypd::Covariant};
  auto EL2 = notdec::retypd::RecallBase{
      .Base = notdec::retypd::TypeVariable::CreateDtv(*Ctx, "alpha"),
      .V = notdec::retypd::Contravariant};
  EXPECT_TRUE(EL1 != EL2);
  EXPECT_FALSE(EL1 == EL2);
}

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
}
