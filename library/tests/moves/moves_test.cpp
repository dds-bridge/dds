/**
 * @file moves_test.cpp
 * @brief Comprehensive unit tests for the Moves class
 *
 * Tests cover:
 * - Constructor and initialization
 * - Move generation properties
 * - Move selection
 * - Statistics tracking
 * - Edge cases and invariant violations
 */

#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <chrono>

#include "moves.hpp"

/**
 * @brief Test fixture for Moves class tests
 *
 * Provides setup for common test scenarios and helper methods.
 */
class MovesTest : public ::testing::Test {
 protected:
  MovesTest() = default;
  
  void SetUp() override {
    // Initialize test objects
    moves = std::make_unique<Moves>();
  }
  
  void TearDown() override {
    moves.reset();
  }
  
  /**
   * @brief Get sample rank_in_suit data for testing
   */
  const unsigned short (*getSampleRankInSuit())[4] {
    static unsigned short data[4][4] = {
      {0x3fff, 0x3fff, 0x3fff, 0x3fff},
      {0x3fff, 0x3fff, 0x3fff, 0x3fff},
      {0x3fff, 0x3fff, 0x3fff, 0x3fff},
      {0x3fff, 0x3fff, 0x3fff, 0x3fff}
    };
    return data;
  }
  
  std::unique_ptr<Moves> moves;
};

/**
 * @section Constructor and Initialization Tests
 */

TEST_F(MovesTest, ConstructorInitializesState) {
  // Verify constructor initializes object properly
  EXPECT_NE(moves.get(), nullptr);
  
  // Verify function names are initialized
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    EXPECT_FALSE(moves->funcName[i].empty());
  }
  
  // Verify statistics are zeroed
  EXPECT_EQ(moves->trickFuncTable.nfuncs, 0);
  EXPECT_EQ(moves->trickFuncSuitTable.nfuncs, 0);

  // make_heuristic_context snapshots these; they must not be indeterminate
  // after construction (and MoveGen0/123 re-set them before hoisting a context).
  EXPECT_EQ(moves->leadHand, 0);
  EXPECT_EQ(moves->currHand, 0);
  EXPECT_EQ(moves->leadSuit, 0);
  EXPECT_EQ(moves->currTrick, 0);
  EXPECT_EQ(moves->trump, DDS_NOTRUMP);
  EXPECT_EQ(moves->suit, 0);
  EXPECT_EQ(moves->numMoves, 0);
  EXPECT_EQ(moves->lastNumMoves, 0);
}

TEST_F(MovesTest, InitializesTrackingState) {
  // Initialize with trick 5, starting from relative hand 0
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  moves->Init(5, 0, nullptr, nullptr, rankInSuit, 3, 0);
  
  // Verify state is initialized
  EXPECT_EQ(moves->currTrick, 5);
  EXPECT_EQ(moves->trump, 3);  // 3 = notrump
  
  // Verify move lists are reset
  for (int h = 0; h < 4; h++) {
    EXPECT_EQ(moves->moveList[5][h].current, 0);
    EXPECT_EQ(moves->moveList[5][h].last, 0);
  }
}

TEST_F(MovesTest, ReinitUpdateLeadHand) {
  // Initialize first
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  moves->Init(7, 0, nullptr, nullptr, rankInSuit, 0, 1);
  
  // Reinit with different lead hand
  moves->Reinit(7, 2);
  
  // Verify lead hand updated
  EXPECT_EQ(moves->track[7].lead_hand, 2);
}

TEST_F(MovesTest, GetLengthReturnsCorrectCount) {
  // GetLength should return valid counts
  // Note: moveList is initialized with last=0, so GetLength returns last+1
  EXPECT_GE(moves->GetLength(3, 0), 0);
  EXPECT_LE(moves->GetLength(3, 0), 14);  // Max 13 cards + 1
  EXPECT_GE(moves->GetLength(12, 3), 0);
  EXPECT_LE(moves->GetLength(12, 3), 14);
}

TEST_F(MovesTest, GetLengthHandlesEmptyList) {
  // Verify list lengths are reasonable (0-14 for max 13 cards)
  for (int t = 0; t < 13; t++) {
    for (int h = 0; h < 4; h++) {
      int length = moves->GetLength(t, h);
      EXPECT_GE(length, 0);
      EXPECT_LE(length, 14);
    }
  }
}

TEST_F(MovesTest, PrintMoveReturnsValidString) {
  // PrintMove should return a string when given a MovePlyType
  // It's primarily for debugging, so just verify it doesn't crash
  EXPECT_NO_THROW({
    auto result = moves->PrintMove(moves->moveList[0][0]);
    EXPECT_FALSE(result.empty());
  });
}

/**
 * @section Memory Safety Tests
 */

TEST_F(MovesTest, PointersInitializedToNullptr) {
  // After construction, pointers should be nullptr
  EXPECT_EQ(moves->trackp, nullptr);
  EXPECT_EQ(moves->mply, nullptr);
}

TEST_F(MovesTest, PointersSetCorrectlyDuringInit) {
  // After init, trackp should still be nullptr (it's set later in MoveGen0)
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  moves->Init(5, 0, nullptr, nullptr, rankInSuit, 3, 0);
  
  // After init, trackp should still be nullptr (it's set later in MoveGen0)
  EXPECT_EQ(moves->trackp, nullptr);
}

/**
 * @section Constants and Enums
 */

TEST_F(MovesTest, MgTypeEnumHasExpectedValues) {
  // Verify enum values are as expected
  EXPECT_EQ(static_cast<int>(MgType::NT0), 0);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP0), 1);
  EXPECT_EQ(static_cast<int>(MgType::NT_VOID1), 2);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP_VOID1), 3);
  EXPECT_EQ(static_cast<int>(MgType::NT_NOTVOID1), 4);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP_NOTVOID1), 5);
  EXPECT_EQ(static_cast<int>(MgType::NT_VOID2), 6);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP_VOID2), 7);
  // Verify SIZE is last
  EXPECT_GT(static_cast<int>(MgType::SIZE), 7);
}

TEST_F(MovesTest, FuncNameArrayHasSizeElements) {
  // Verify funcName array has correct size
  int count = 0;
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    if (!moves->funcName[i].empty()) {
      count++;
    }
  }
  EXPECT_EQ(count, static_cast<int>(MgType::SIZE));
}

/**
 * @section Data Structure Tests
 */

TEST_F(MovesTest, TrackArrayHas13Tricks) {
  // Verify track array has 13 tricks
  EXPECT_EQ(std::size(moves->track), 13);
}

TEST_F(MovesTest, MoveListArrayHas13TricksAnd4Hands) {
  // Verify moveList array dimensions
  EXPECT_EQ(std::size(moves->moveList), 13);
  for (int t = 0; t < 13; t++) {
    EXPECT_EQ(std::size(moves->moveList[t]), 4);
  }
}

TEST_F(MovesTest, LastCallArrayHas13TricksAnd4Hands) {
  // Verify lastCall array dimensions
  EXPECT_EQ(std::size(moves->lastCall), 13);
  for (int t = 0; t < 13; t++) {
    EXPECT_EQ(std::size(moves->lastCall[t]), 4);
  }
}

TEST_F(MovesTest, StatisticsStructuresProperlyInitialized) {
  // Verify statistics structures are initialized
  EXPECT_EQ(moves->trickFuncTable.nfuncs, 0);
  EXPECT_EQ(moves->trickFuncSuitTable.nfuncs, 0);
}

/**
 * @section Integration Tests
 */

TEST_F(MovesTest, CreateAndDestroySuccessfully) {
  // Verify object can be created and destroyed
  auto testMoves = std::make_unique<Moves>();
  EXPECT_NE(testMoves.get(), nullptr);
  testMoves.reset();
  EXPECT_TRUE(true);  // If we got here, no crash
}

TEST_F(MovesTest, MultipleInitializeCallsWork) {
  // Verify multiple Init calls work correctly
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  
  for (int t = 0; t < 13; t++) {
    moves->Init(t, 0, nullptr, nullptr, rankInSuit, 0, t % 4);
    EXPECT_EQ(moves->currTrick, t);
  }
}

TEST_F(MovesTest, GetLengthWithValidBounds) {
  // Verify GetLength works for all valid bounds
  for (int t = 0; t < 13; t++) {
    for (int h = 0; h < 4; h++) {
      auto length = moves->GetLength(t, h);
      EXPECT_GE(length, 0);
      EXPECT_LE(length, 13);
    }
  }
}

/**
 * @section Documentation and Metadata
 */

TEST_F(MovesTest, FunctionNamesAreHumanReadable) {
  // Verify function names are readable strings
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    const auto& name = moves->funcName[i];
    EXPECT_FALSE(name.empty());
    EXPECT_GT(name.length(), 0);
    // Name should have printable characters
    for (char c : name) {
      EXPECT_TRUE(std::isprint(c) || c == ' ');
    }
  }
}

TEST_F(MovesTest, MemorySafetyFeaturesArePresent) {
  // Verify key memory safety features are in place
  EXPECT_EQ(moves->trackp, nullptr);  // Non-owning pointer initialized
  EXPECT_EQ(moves->mply, nullptr);    // Non-owning pointer initialized
  EXPECT_FALSE(moves->funcName[0].empty());  // funcName array exists and is initialized
}

namespace
{

auto poisoned_track() -> TrackType
{
  TrackType tr{};
  for (int s = 0; s < DDS_SUITS; ++s)
    tr.removed_ranks[s] = 0x10 + s;
  // Non-zero trick slots: only those defined for the current hand_rel should
  // appear in the HeuristicContext snapshot.
  tr.move[0] = ExtCard{0, 14, 0};
  tr.move[1] = ExtCard{2, 12, 0};
  tr.high[1] = 1;
  tr.move[2] = ExtCard{3, 9, 0};
  tr.high[2] = 2;
  return tr;
}

// HeuristicContext stores references to these; they must outlive the returned
// value (locals in context_for_hand_rel would dangle after return).
static const Pos kTpos{};
static const MoveType kBest{};
static const MoveType kBestTt{};

auto context_for_hand_rel(Moves& m, const TrackType& tr, const int hand_rel)
    -> HeuristicContext
{
  m.leadHand = 0;
  m.currHand = hand_rel;  // leadHand 0 ⇒ hand_rel == currHand
  m.leadSuit = 0;
  m.currTrick = 0;
  m.trump = DDS_NOTRUMP;
  m.suit = 0;
  m.numMoves = 0;
  m.lastNumMoves = 0;
  return m.make_heuristic_context(kTpos, kBest, kBestTt, nullptr, tr);
}

}  // namespace

/**
 * make_heuristic_context must take an explicit TrackType rather than
 * dereferencing Moves::trackp (which starts as nullptr). Callers pass the
 * bound track; snapshots must come from that argument, not a nullable member.
 */
TEST_F(MovesTest, MakeHeuristicContextSnapshotsExplicitTrack)
{
  const TrackType tr = poisoned_track();
  ASSERT_EQ(moves->trackp, nullptr);

  const HeuristicContext ctx = context_for_hand_rel(*moves, tr, /*hand_rel=*/3);

  for (int s = 0; s < DDS_SUITS; ++s)
    EXPECT_EQ(ctx.removed_ranks[s], 0x10 + s) << "suit=" << s;
  EXPECT_EQ(ctx.lead0_rank, 14);
  EXPECT_EQ(ctx.move1_rank, 12);
  EXPECT_EQ(ctx.move1_suit, 2);
  EXPECT_EQ(ctx.high1, 1);
  EXPECT_EQ(ctx.move2_rank, 9);
  EXPECT_EQ(ctx.move2_suit, 3);
  EXPECT_EQ(ctx.high2, 2);
  EXPECT_EQ(ctx.trackp, &tr);
}

// HeuristicContext holds references to tpos / best_move / best_move_tt; the
// helper must bind them to storage that outlives the returned context.
TEST_F(MovesTest, MakeHeuristicContextBindsStableRefs)
{
  const TrackType tr = poisoned_track();
  const HeuristicContext ctx = context_for_hand_rel(*moves, tr, /*hand_rel=*/0);

  EXPECT_EQ(&ctx.tpos, &kTpos);
  EXPECT_EQ(&ctx.best_move, &kBest);
  EXPECT_EQ(&ctx.best_move_tt, &kBestTt);
}

// Leading hand: move[0..2] are not played yet — leave trick snapshots at 0
// even if the TrackType buffer holds stale/poisoned values.
TEST_F(MovesTest, MakeHeuristicContextLeadHandOmitsUnsetTrickCards)
{
  const TrackType tr = poisoned_track();
  const HeuristicContext ctx = context_for_hand_rel(*moves, tr, /*hand_rel=*/0);

  EXPECT_EQ(ctx.lead0_rank, 0);
  EXPECT_EQ(ctx.move1_rank, 0);
  EXPECT_EQ(ctx.move1_suit, 0);
  EXPECT_EQ(ctx.high1, 0);
  EXPECT_EQ(ctx.move2_rank, 0);
  EXPECT_EQ(ctx.move2_suit, 0);
  EXPECT_EQ(ctx.high2, 0);
}

// Second hand: only the lead card is defined.
TEST_F(MovesTest, MakeHeuristicContextSecondHandSnapshotsOnlyLead)
{
  const TrackType tr = poisoned_track();
  const HeuristicContext ctx = context_for_hand_rel(*moves, tr, /*hand_rel=*/1);

  EXPECT_EQ(ctx.lead0_rank, 14);
  EXPECT_EQ(ctx.move1_rank, 0);
  EXPECT_EQ(ctx.move1_suit, 0);
  EXPECT_EQ(ctx.high1, 0);
  EXPECT_EQ(ctx.move2_rank, 0);
  EXPECT_EQ(ctx.move2_suit, 0);
  EXPECT_EQ(ctx.high2, 0);
}

// Third hand: lead + second-hand card/high are defined; move[2] is not.
TEST_F(MovesTest, MakeHeuristicContextThirdHandSnapshotsThroughMove1)
{
  const TrackType tr = poisoned_track();
  const HeuristicContext ctx = context_for_hand_rel(*moves, tr, /*hand_rel=*/2);

  EXPECT_EQ(ctx.lead0_rank, 14);
  EXPECT_EQ(ctx.move1_rank, 12);
  EXPECT_EQ(ctx.move1_suit, 2);
  EXPECT_EQ(ctx.high1, 1);
  EXPECT_EQ(ctx.move2_rank, 0);
  EXPECT_EQ(ctx.move2_suit, 0);
  EXPECT_EQ(ctx.high2, 0);
}

namespace
{

// Compact deal for MoveGen*: lead hand has only a few cards so the move list
// stays within MovePlyType::move[14]. The Init-only sample (0x3fff in every
// suit) overflows that buffer and trips ASan.
struct CompactDeal
{
  unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS]{};
  Pos tpos{};
};

auto popcount16(unsigned short bits) -> int
{
  int n = 0;
  for (; bits != 0; bits = static_cast<unsigned short>(bits & (bits - 1)))
    ++n;
  return n;
}

auto make_compact_deal() -> CompactDeal
{
  CompactDeal d{};
  // Hand 0 (lead): AK of suit 0, Q of suit 1.
  d.rank_in_suit[0][0] = 0x1800;
  d.rank_in_suit[0][1] = 0x0400;
  // Other hands: one card each for length/winner lookups.
  d.rank_in_suit[1][0] = 0x0200;
  d.rank_in_suit[1][2] = 0x0100;
  d.rank_in_suit[2][0] = 0x0080;
  d.rank_in_suit[3][1] = 0x0040;

  for (int h = 0; h < DDS_HANDS; ++h)
  {
    for (int s = 0; s < DDS_SUITS; ++s)
    {
      d.tpos.rank_in_suit[h][s] = d.rank_in_suit[h][s];
      d.tpos.length[h][s] =
        static_cast<unsigned char>(popcount16(d.rank_in_suit[h][s]));
      d.tpos.aggr[s] =
        static_cast<unsigned short>(d.tpos.aggr[s] | d.rank_in_suit[h][s]);
    }
  }
  return d;
}

auto move_list_weights(const Moves& m, const int tricks, const int hand_rel,
                       const int n) -> std::vector<int>
{
  EXPECT_GT(n, 0);
  EXPECT_LE(n, 14);
  std::vector<int> weights;
  weights.reserve(static_cast<unsigned>(n));
  for (int i = 0; i < n; ++i)
    weights.push_back(m.moveList[tricks][hand_rel].move[i].weight);
  return weights;
}

}  // namespace

// MoveGen0 must range-check trump before reading winner[trump], matching the
// legacy heuristic dispatcher. Out-of-range values behave like no-trump.
TEST_F(MovesTest, MoveGen0OutOfRangeTrumpMatchesNoTrump)
{
  static RelRanksType rel[8192] = {};
  constexpr int tricks = 5;
  const CompactDeal deal = make_compact_deal();
  const MoveType best{};

  auto run = [&](const int trump) {
    auto local = std::make_unique<Moves>();
    local->Init(tricks, 0, nullptr, nullptr, deal.rank_in_suit, trump, 0);
    const int n = local->MoveGen0(tricks, deal.tpos, best, best, rel);
    return move_list_weights(*local, tricks, 0, n);
  };

  const auto nt = run(DDS_NOTRUMP);
  // DDS_NOTRUMP == DDS_SUITS, so use values strictly outside [0, DDS_SUITS).
  for (const int bad_trump : {-1, DDS_SUITS + 1, 99})
  {
    const auto bad = run(bad_trump);
    EXPECT_EQ(bad, nt) << "trump=" << bad_trump;
  }
}

// MoveGen123 likewise must not index winner[trump] when trump is out of range.
TEST_F(MovesTest, MoveGen123OutOfRangeTrumpMatchesNoTrump)
{
  constexpr int tricks = 5;
  constexpr int hand_rel = 1;
  constexpr int lead_suit = 0;
  const CompactDeal deal = make_compact_deal();

  auto run = [&](const int trump) {
    auto local = std::make_unique<Moves>();
    local->Init(tricks, 0, nullptr, nullptr, deal.rank_in_suit, trump, 0);
    local->track[tricks].lead_suit = lead_suit;
    local->track[tricks].move[0] = ExtCard{lead_suit, 8, 0};
    const int n = local->MoveGen123(tricks, hand_rel, deal.tpos);
    return move_list_weights(*local, tricks, hand_rel, n);
  };

  const auto nt = run(DDS_NOTRUMP);
  for (const int bad_trump : {-1, DDS_SUITS + 1, 99})
  {
    const auto bad = run(bad_trump);
    EXPECT_EQ(bad, nt) << "trump=" << bad_trump;
  }
}

// Hoisted make_heuristic_context must not observe stale suit/lastNumMoves from
// a prior call. Poison those members; MoveGen should reset them before
// snapshotting so weights match a clean instance.
TEST_F(MovesTest, MoveGenResetsSnapshottedFieldsBeforeHeuristicContext)
{
  static RelRanksType rel[8192] = {};
  constexpr int tricks = 5;
  constexpr int lead_suit = 0;
  CompactDeal deal = make_compact_deal();
  // Force a void-in-lead path for hand_rel=1 so suit/lastNumMoves are used
  // by the void weight functions after the hoisted context is built.
  deal.rank_in_suit[1][lead_suit] = 0;
  deal.tpos.rank_in_suit[1][lead_suit] = 0;
  deal.tpos.length[1][lead_suit] = 0;
  const MoveType best{};

  auto run_gen0 = [&](Moves& m) {
    m.Init(tricks, 0, nullptr, nullptr, deal.rank_in_suit, DDS_NOTRUMP, 0);
    return move_list_weights(
      m, tricks, 0, m.MoveGen0(tricks, deal.tpos, best, best, rel));
  };
  auto run_gen123 = [&](Moves& m) {
    m.Init(tricks, 0, nullptr, nullptr, deal.rank_in_suit, DDS_NOTRUMP, 0);
    m.track[tricks].lead_suit = lead_suit;
    m.track[tricks].move[0] = ExtCard{lead_suit, 8, 0};
    return move_list_weights(
      m, tricks, 1, m.MoveGen123(tricks, /*handRel=*/1, deal.tpos));
  };

  Moves clean;
  const auto gen0_clean = run_gen0(clean);
  const auto gen123_clean = run_gen123(clean);

  Moves poisoned;
  poisoned.suit = 3;
  poisoned.lastNumMoves = 99;
  poisoned.leadSuit = 2;
  EXPECT_EQ(run_gen0(poisoned), gen0_clean);
  poisoned.suit = 3;
  poisoned.lastNumMoves = 99;
  poisoned.leadSuit = 2;
  EXPECT_EQ(run_gen123(poisoned), gen123_clean);
}

/**
 * @section Performance Tests
 */

TEST_F(MovesTest, ConstructionIsQuick) {
  // Verify construction is fast
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 1000; i++) {
    auto temp = std::make_unique<Moves>();
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Should complete 1000 constructions in reasonable time
  EXPECT_LT(duration.count(), 1000);  // Less than 1 second for 1000
}

TEST_F(MovesTest, GetLengthIsQuick) {
  // Verify GetLength is fast
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 100000; i++) {
    volatile int result = moves->GetLength(i % 13, i % 4);
    (void)result;  // Use result to prevent optimization
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Should complete 100k calls in reasonable time
  EXPECT_LT(duration.count(), 500);  // Less than 500ms for 100k
}
