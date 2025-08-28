#include "gtest/gtest.h"

#include "heuristic_sorting/heuristic_sorting.h"

namespace {

// Helper function to initialize a basic HeuristicContext for tests.
void InitializeContext(HeuristicContext& context) {
  context.trump_suit = 4; // No-trump
  context.lead_hand = 0;
  context.current_hand_index = 0;
  context.tricks = 0;

  static relRanksType rel_ranks{};
  context.rel_ranks = &rel_ranks;

  static int highest_rank[8192]{};
  context.highest_rank = highest_rank;

  static int lowest_rank[8192]{};
  context.lowest_rank = lowest_rank;

  static moveGroupType group_data[8192]{};
  context.group_data = group_data;

  static unsigned short int bit_map_rank[16]{};
  context.bit_map_rank = bit_map_rank;

  static int count_table[8192]{};
  context.count_table = count_table;

  static int removed_ranks[4]{};
  context.removed_ranks = removed_ranks;
}

} // namespace

TEST(HeuristicSortingTest, SortsMovesByRankInSimpleCase) {
  HeuristicContext context;
  InitializeContext(context);
  pos tpos{};
  context.tpos = &tpos;

  CandidateMove candidate_moves[3];
  candidate_moves[0].suit = 0; candidate_moves[0].rank = 10; // Ten of Spades
  candidate_moves[1].suit = 0; candidate_moves[1].rank = 2;  // Two of Spades
  candidate_moves[2].suit = 0; candidate_moves[2].rank = 14; // Ace of Spades

  ScoredMove scored_moves[3];
  int num_scored_moves = score_and_order(context, candidate_moves, 3, scored_moves);

  ASSERT_EQ(num_scored_moves, 3);
  EXPECT_EQ(scored_moves[0].move.rank, 14);
  EXPECT_EQ(scored_moves[1].move.rank, 10);
  EXPECT_EQ(scored_moves[2].move.rank, 2);
}

TEST(HeuristicSortingTest, PrioritizesWinningMove) {
  HeuristicContext context;
  InitializeContext(context);
  pos tpos{};
  context.tpos = &tpos;

  context.current_hand_index = 1; // East is playing
  context.lead_suit = 0; // Spades led
  context.played_cards[0].suit = 0;
  context.played_cards[0].rank = 5;

  // East has Ace and 2 of Spades
  tpos.rankInSuit[1][0] = (1 << (14 - 2)) | (1 << (2 - 2));
  tpos.length[1][0] = 2;

  // West has King and 3 of Spades
  tpos.rankInSuit[2][0] = (1 << (13 - 2)) | (1 << (3 - 2));
  tpos.length[2][0] = 2;

  // South has Queen and 4 of Spades
  tpos.rankInSuit[3][0] = (1 << (12 - 2)) | (1 << (4 - 2));
  tpos.length[3][0] = 2;

  CandidateMove candidate_moves[2];
  candidate_moves[0].suit = 0; candidate_moves[0].rank = 2;  // Two of Spades
  candidate_moves[1].suit = 0; candidate_moves[1].rank = 14; // Ace of Spades

  ScoredMove scored_moves[2];
  int num_scored_moves = score_and_order(context, candidate_moves, 2, scored_moves);

  ASSERT_EQ(num_scored_moves, 2);
  // The Ace of Spades is the winning move and should be first.
  EXPECT_EQ(scored_moves[0].move.rank, 14);
}

TEST(HeuristicSortingTest, ParityWithOldImplementation_Placeholder) {
  // TODO: Implement a full parity test.
  // This requires a mechanism to call the original heuristic functions from
  // Moves.cpp and compare their output with the new score_and_order function
  // for a wide range of game states.
  //
  // One approach is to expose the old functions through a test-only interface
  // and then generate or record a large number of test cases to serve as a
  // golden dataset.
  EXPECT_TRUE(true);
}