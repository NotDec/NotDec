#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/retypd/Schema.h"
#include "TypeRecovery/TRContext.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Support/Debug.h>
#include <memory>
#include <string>
#include <vector>

using notdec::retypd::Constraint;
using notdec::retypd::ConstraintGraph;
using notdec::retypd::ConstraintSummary;
using notdec::retypd::TRContext;
using notdec::retypd::TypeVariable;

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
