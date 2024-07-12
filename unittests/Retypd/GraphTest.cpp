#include "Retypd/Graph.h"
#include "Retypd/Parser.h"
#include "Retypd/Schema.h"
#include <gtest/gtest.h>
#include <vector>

std::vector<notdec::retypd::Constraint>
parse_constraints(std::vector<const char *> cons_str) {
  std::vector<notdec::retypd::Constraint> ret;
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

// A simple example from the paper.
TEST(Retypd, SaturationPaperTest) {
  std::vector<notdec::retypd::Constraint> cons =
      parse_constraints({"y <= p", "p <= x", "#A <= x.store", "y.load <= #B"});
  notdec::retypd::ConstraintGraph CG("SaturationPaper");
  CG.build(cons);
  CG.printGraph("SaturationPaper.dot");
}

// A simple example from the paper.
TEST(Retypd, SlidesExampleTest) {
  std::vector<notdec::retypd::Constraint> cons = parse_constraints({
      "F.in_stack0 <= 𝛿",
      "𝛼 <= 𝜑",
      "𝛿 <= 𝜑",
      "𝜑.load.σ4@0 <= 𝛼",
      "𝜑.load.σ4@4 <= 𝛼'",
      "𝛼' <= close.in_stack0",
      "close.out_eax <= F.out_eax",
      "close.in_stack0 <= #FileDescriptor",
      "#SuccessZ <= close.out_eax",
  });
  notdec::retypd::ConstraintGraph CG("SlideExample");
  CG.build(cons);
  CG.printGraph("SlideExample.dot");
}
