#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include "TypeRecovery/TRContext.h"
#include <gtest/gtest.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Support/Debug.h>
#include <string>
#include <vector>

using notdec::retypd::Constraint;
using notdec::retypd::ConstraintGraph;
using notdec::retypd::TRContext;

std::vector<Constraint> parse_constraints(TRContext &Ctx,
                                          std::vector<const char *> cons_str) {
  std::vector<Constraint> ret;
  for (const char *con : cons_str) {
    auto res = notdec::retypd::parseSubTypeConstraint(Ctx, con);
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
  TRContext Ctx;
  llvm::DebugFlag = true;
  llvm::setCurrentDebugType("retypd_graph");
  std::vector<notdec::retypd::Constraint> cons = parse_constraints(
      Ctx, {"y <= p", "p <= x", "#A <= x.store4", "y.load4 <= #B"});
  ConstraintGraph CG =
      ConstraintGraph::fromConstraints(Ctx, "SaturationPaper", cons);

  std::set<std::string> InterestingVars;
  CG.solve();
  auto Cons = CG.simplifiedExpr(InterestingVars);
  CG.printGraph("SaturationPaper.dot");
  std::cerr << "Simplified Constraints:" << std::endl;
  for (auto &C : Cons) {
    std::cerr << notdec::retypd::toString(C) << "\n";
  }

  check(Cons, {"#A <= #B"});
}

// A simple example from the paper.
TEST(Retypd, SlidesExampleTest) {
  TRContext Ctx;
  llvm::DebugFlag = true;
  llvm::setCurrentDebugType("retypd_graph");
  std::vector<Constraint> cons =
      parse_constraints(Ctx, {
                                 "F.in_stack0 <= 𝛿",
                                 "𝛼 <= 𝜑",
                                 "𝛿 <= 𝜑",
                                 "𝜑.load4.@0 <= 𝛼",
                                 "𝜑.load4.@4 <= 𝛼'",
                                 "𝛼' <= close.in_stack0",
                                 "close.out_eax <= F.out_eax",
                                 "close.in_stack0 <= #FileDescriptor",
                                 "#SuccessZ <= close.out_eax",
                             });
  ConstraintGraph CG =
      ConstraintGraph::fromConstraints(Ctx, "SlideExample", cons);
  std::set<std::string> InterestingVars;
  InterestingVars.insert("F");
  CG.solve();
  auto Cons = CG.simplifiedExpr(InterestingVars);
  // CG.printGraph("SlideExample.dot");

  // std::cerr << "Simplified Constraints:" << std::endl;
  // for (auto &C : Cons) {
  //   std::cerr << notdec::retypd::toString(C) << "\n";
  // }
  check(Cons,
        {"__temp_0.@0.load4 <= __temp_0", "#SuccessZ <= F.out_eax",
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
  TRContext Ctx;
  auto Prefix = create(RecallBase{TypeVariable::CreateDtv(Ctx, "alpha")});
  auto Suffix = create(ForgetBase{TypeVariable::CreateDtv(Ctx, "beta")});

  auto StarForget1 =
      Prefix & createStar(create(ForgetLabel{LoadLabel{}})) & Suffix;
  std::cerr << "Converting Expr: " << toString(StarForget1) << std::endl;
  printConstraints(expToConstraints(Ctx, StarForget1));

  auto StarRecall1 =
      Prefix & createStar(create(RecallLabel{LoadLabel{}})) & Suffix;
  std::cerr << "Converting Expr: " << toString(StarRecall1) << std::endl;
  printConstraints(expToConstraints(Ctx, StarRecall1));
}

// A test for the ExprToConstraints
TEST(Retypd, EdgeLabel1) {
  using notdec::retypd::EdgeLabel;
  using notdec::retypd::TRContext;
  TRContext Ctx;
  auto EL1 = notdec::retypd::RecallBase{
      .Base = notdec::retypd::TypeVariable::CreateDtv(Ctx, "alpha"),
      .V = notdec::retypd::Covariant};
  auto EL2 = notdec::retypd::RecallBase{
      .Base = notdec::retypd::TypeVariable::CreateDtv(Ctx, "alpha"),
      .V = notdec::retypd::Contravariant};
  EXPECT_TRUE(EL1 != EL2);
  EXPECT_FALSE(EL1 == EL2);
}
