#include <gtest/gtest.h>

#include <solver_context/SolverContext.hpp>
#include <trans_table/TransTableS.hpp>
#include <trans_table/TransTableL.hpp>

namespace {

TEST(ConfigureTtApiTest, SwitchKindRecreatesTable)
{
  // Default context: Large TT by default (unless env overrides)
  SolverContext ctx;
  auto* tt1 = ctx.transTable();
  ASSERT_NE(tt1, nullptr);
  // Determine current kind via RTTI
  const bool was_small = dynamic_cast<TransTableS*>(tt1) != nullptr;

  // Flip kind
  const TTKind new_kind = was_small ? TTKind::Large : TTKind::Small;
  ctx.ConfigureTT(new_kind, /*defMB=*/8, /*maxMB=*/8);

  auto* tt2 = ctx.maybeTransTable();
  ASSERT_NE(tt2, nullptr);
  if (new_kind == TTKind::Small)
    EXPECT_NE(nullptr, dynamic_cast<TransTableS*>(tt2));
  else
    EXPECT_NE(nullptr, dynamic_cast<TransTableL*>(tt2));
}

TEST(ConfigureTtApiTest, ResizeInPlaceWhenKindUnchanged)
{
  SolverContext ctx;
  auto* tt1 = ctx.transTable();
  ASSERT_NE(tt1, nullptr);
  // Determine current kind via RTTI
  const bool is_small = dynamic_cast<TransTableS*>(tt1) != nullptr;
  const TTKind same_kind = is_small ? TTKind::Small : TTKind::Large;

  // Resize should not replace the instance when kind does not change
  ctx.ConfigureTT(same_kind, /*defMB=*/16, /*maxMB=*/32);
  auto* tt2 = ctx.maybeTransTable();
  ASSERT_NE(tt2, nullptr);
  EXPECT_EQ(tt1, tt2) << "Resize should keep the same TT instance";
}

} // namespace
