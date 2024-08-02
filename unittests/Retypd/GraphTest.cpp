#include "TypeRecovery/ConstraintGraph.h"
#include "TypeRecovery/Parser.h"
#include "TypeRecovery/RExp.h"
#include "TypeRecovery/Schema.h"
#include <gtest/gtest.h>
#include <llvm/Support/Debug.h>
#include <string>
#include <vector>

using notdec::retypd::Constraint;
using notdec::retypd::ConstraintGraph;

std::vector<Constraint> parse_constraints(std::vector<const char *> cons_str) {
  std::vector<Constraint> ret;
  for (const char *con : cons_str) {
    auto res = notdec::retypd::parseSubTypeConstraint(con);
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
  llvm::DebugFlag = true;
  llvm::setCurrentDebugType("retypd_graph");
  std::vector<notdec::retypd::Constraint> cons = parse_constraints(
      {"y <= p", "p <= x", "#A <= x.store4", "y.load4 <= #B"});
  ConstraintGraph CG =
      ConstraintGraph::fromConstraints("SaturationPaper", cons);

  std::set<std::string> InterestingVars;
  auto Cons = CG.simplify(InterestingVars);
  CG.printGraph("SaturationPaper.dot");
  std::cerr << "Simplified Constraints:" << std::endl;
  for (auto &C : Cons) {
    std::cerr << notdec::retypd::toString(C) << "\n";
  }

  check(Cons, {"#A <= #B"});
}

// A simple example from the paper.
TEST(Retypd, SlidesExampleTest) {
  llvm::DebugFlag = true;
  llvm::setCurrentDebugType("retypd_graph");
  std::vector<Constraint> cons = parse_constraints({
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
  ConstraintGraph CG = ConstraintGraph::fromConstraints("SlideExample", cons);
  std::set<std::string> InterestingVars;
  InterestingVars.insert("F");
  auto Cons = CG.simplify(InterestingVars);
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
  auto Prefix = create(RecallBase{TypeVariable::CreateDtv("alpha")});
  auto Suffix = create(ForgetBase{TypeVariable::CreateDtv("beta")});

  auto StarForget1 =
      Prefix & createStar(create(ForgetLabel{LoadLabel{}})) & Suffix;
  std::cerr << "Converting Expr: " << toString(StarForget1) << std::endl;
  printConstraints(expToConstraints(StarForget1));

  auto StarRecall1 =
      Prefix & createStar(create(RecallLabel{LoadLabel{}})) & Suffix;
  std::cerr << "Converting Expr: " << toString(StarRecall1) << std::endl;
  printConstraints(expToConstraints(StarRecall1));
}
