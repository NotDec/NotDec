#include "TypeRecovery/mlsub/PAPath.h"

#include <gtest/gtest.h>

#include <set>
#include <string>

namespace {

using notdec::OffsetRange;
using notdec::ReturnValue;
using notdec::mlsub::PAFieldTag;
using notdec::mlsub::PAPath;
using notdec::mlsub::PAPathAtom;
using notdec::mlsub::PAPathStar;
using notdec::mlsub::appendAndNormalize;
using notdec::mlsub::formatPAPath;

PAFieldTag makeTag(int Index) {
  return PAFieldTag{.K = PAFieldTag::Kind::IRPtrAdd,
                    .Value = ReturnValue{.Func = nullptr, .Index = Index}};
}

PAPathAtom makeAtom(int Index, int64_t Offset) {
  return PAPathAtom{.Offset = OffsetRange{.offset = Offset},
                    .Tag = makeTag(Index)};
}

const PAPathStar &getOnlyStar(const PAPath &Path) {
  EXPECT_EQ(Path.Elems.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<PAPathStar>(Path.Elems[0]));
  return std::get<PAPathStar>(Path.Elems[0]);
}

} // namespace

TEST(Retypd, PAPathSingleStepCycleCollapsesToStar) {
  PAPath Path;
  Path = appendAndNormalize(std::move(Path), makeAtom(1, 1));
  Path = appendAndNormalize(std::move(Path), makeAtom(1, 1));

  const auto &Star = getOnlyStar(Path);
  ASSERT_EQ(Star.Body.size(), 1u);
  EXPECT_EQ(Star.Body[0], makeAtom(1, 1));
  EXPECT_EQ(formatPAPath(Path), "(@1)*");
}

TEST(Retypd, PAPathRepeatedSuffixCollapsesToStar) {
  PAPath Path;
  for (const auto &Atom :
       {makeAtom(1, 1), makeAtom(2, 2), makeAtom(3, 3), makeAtom(1, 1),
        makeAtom(2, 2), makeAtom(3, 3)}) {
    Path = appendAndNormalize(std::move(Path), Atom);
  }

  const auto &Star = getOnlyStar(Path);
  ASSERT_EQ(Star.Body.size(), 3u);
  EXPECT_EQ(Star.Body[0], makeAtom(1, 1));
  EXPECT_EQ(Star.Body[1], makeAtom(2, 2));
  EXPECT_EQ(Star.Body[2], makeAtom(3, 3));
  EXPECT_EQ(formatPAPath(Path), "(@1.@2.@3)*");
}

TEST(Retypd, PAPathStarAbsorbsTrailingBody) {
  PAPath Path;
  for (const auto &Atom :
       {makeAtom(1, 1), makeAtom(2, 2), makeAtom(3, 3), makeAtom(1, 1),
        makeAtom(2, 2), makeAtom(3, 3), makeAtom(1, 1), makeAtom(2, 2),
        makeAtom(3, 3)}) {
    Path = appendAndNormalize(std::move(Path), Atom);
  }

  const auto &Star = getOnlyStar(Path);
  ASSERT_EQ(Star.Body.size(), 3u);
  EXPECT_EQ(formatPAPath(Path), "(@1.@2.@3)*");
}

TEST(Retypd, PAPathDifferentTagsDoNotCollapse) {
  PAPath Path;
  Path = appendAndNormalize(std::move(Path), makeAtom(1, 1));
  Path = appendAndNormalize(std::move(Path), makeAtom(2, 1));

  ASSERT_EQ(Path.Elems.size(), 2u);
  EXPECT_EQ(formatPAPath(Path), "@1.@1");
}

TEST(Retypd, PAPathSetDeduplicatesEqualPaths) {
  PAPath PathA;
  PathA = appendAndNormalize(std::move(PathA), makeAtom(1, 4));
  PathA = appendAndNormalize(std::move(PathA), makeAtom(1, 4));

  PAPath PathB;
  PathB = appendAndNormalize(std::move(PathB), makeAtom(1, 4));
  PathB = appendAndNormalize(std::move(PathB), makeAtom(1, 4));

  std::set<PAPath> Paths;
  Paths.insert(PathA);
  Paths.insert(PathB);
  EXPECT_EQ(Paths.size(), 1u);
}
