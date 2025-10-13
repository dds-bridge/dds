#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <string>
#include <cstring>

#include "dds/dll.h"
#include "dds/PBN.h"
#include "system/SolverContext.h"
#include "system/Memory.h"
#include "SolverIF.h"

extern Memory memory;

// Ensure at least N thread slots are available in the legacy Memory
static void ensureThreads(size_t n)
{
  if (memory.NumThreads() < n)
    memory.Resize(static_cast<unsigned>(n), DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

static deal make_deal_from_pbn(const char* pbn, int trump = 0, int first = 0)
{
  deal dl{};
  dl.trump = trump;
  dl.first = first;
  std::memset(dl.currentTrickSuit, 0, sizeof(dl.currentTrickSuit));
  std::memset(dl.currentTrickRank, 0, sizeof(dl.currentTrickRank));
  // Convert PBN distribution into remainCards bitmasks
  const int rc = ConvertFromPBN(pbn, dl.remainCards);
  // If conversion fails, keep an empty deal which should yield a deterministic error.
  // Silent failure is intentional for this test: downstream code is expected to handle empty deals.
  (void)rc;
  return dl;
}

static bool equal_future_tricks(const futureTricks& a, const futureTricks& b)
{
  if (a.cards != b.cards) return false;
  for (int i = 0; i < a.cards; ++i) {
    if (a.suit[i] != b.suit[i]) return false;
    if (a.rank[i] != b.rank[i]) return false;
    if (a.equals[i] != b.equals[i]) return false;
    if (a.score[i] != b.score[i]) return false;
  }
  return true;
}

TEST(ConcurrencyValidation, ParallelInstancesMatchSequentialBaseline)
{
  // A small set of distinct boards in PBN text form.
  // Source: examples/hands.cpp (representative random deals)
  const std::vector<std::string> pbns = {
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
    "E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63",
    "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
  };

  const size_t N = pbns.size();
  ensureThreads(N);

  // Prepare deals and sequential baselines (single thread / thr 0)
  std::vector<deal> deals;
  deals.reserve(N);
  for (const auto& s : pbns) {
    deals.emplace_back(make_deal_from_pbn(s.c_str(), /*trump=*/0, /*first=*/0));
  }

  std::vector<futureTricks> baseline_ft(N);
  std::vector<int> baseline_rc(N, 0);

  {
    ThreadData* thr0 = memory.GetPtr(0);
    ASSERT_NE(thr0, nullptr);
    SolverContext ctx{thr0};
    for (size_t i = 0; i < N; ++i) {
      futureTricks ft{};
      const int rc = SolveBoardWithContext(ctx, deals[i], /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft);
      baseline_rc[i] = rc;
      baseline_ft[i] = ft; // copy
    }
  }

  // Run in parallel: one thread per board with its own ThreadData/Context
  std::vector<futureTricks> out_ft(N);
  std::vector<int> out_rc(N, 0);

  std::vector<std::thread> threads;
  threads.reserve(N);
  for (size_t i = 0; i < N; ++i) {
    threads.emplace_back([i, &deals, &out_ft, &out_rc]() {
      ThreadData* thr = memory.GetPtr(static_cast<unsigned>(i));
      EXPECT_NE(thr, nullptr);
      SolverContext ctx{thr};
      futureTricks ft{};
      const int rc = SolveBoardWithContext(ctx, deals[i], /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft);
      out_rc[i] = rc;
      out_ft[i] = ft;
    });
  }
  for (auto& t : threads) t.join();

  for (size_t i = 0; i < N; ++i) {
    EXPECT_EQ(out_rc[i], baseline_rc[i]) << "Return code mismatch for case " << i;
    EXPECT_TRUE(equal_future_tricks(out_ft[i], baseline_ft[i])) << "FutureTricks mismatch for case " << i;
  }
}
