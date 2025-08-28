#include "gtest/gtest.h"

#include "heuristic_sorting/heuristic_sorting.h"

TEST(HeuristicSortingTest, SortsMovesByWeight) {
  HeuristicContext context{};
  context.trump_suit = 4; // No-trump
  context.lead_hand = 0;
  context.current_hand_index = 0;

  Pos pos{};
  context.pos = pos;

  RelRanksType rel_ranks{};
  for (int i = 0; i < 15; ++i) {
    rel_ranks.rank[i] = i;
  }
  context.rel_ranks = &rel_ranks;

  int highest_rank[8192]{};
  context.highest_rank = highest_rank;

  int lowest_rank[8192]{};
  context.lowest_rank = lowest_rank;

  moveGroupType group_data[8192]{};
  context.group_data = group_data;

  unsigned short int bit_map_rank[16]{};
  context.bit_map_rank = bit_map_rank;

  int count_table[8192]{};
  context.count_table = count_table;

  int removed_ranks[4]{};
  context.removed_ranks = removed_ranks;

  CandidateMove candidate_moves[3];
  candidate_moves[0].card = {0, 10};
  candidate_moves[1].card = {0, 2};
  candidate_moves[2].card = {0, 14};

  ScoredMove scored_moves[3];
  int num_scored_moves = score_and_order(context, candidate_moves, 3, scored_moves);

  ASSERT_EQ(num_scored_moves, 3);
  EXPECT_EQ(scored_moves[0].move.card.rank, 14);
  EXPECT_EQ(scored_moves[1].move.card.rank, 10);
  EXPECT_EQ(scored_moves[2].move.card.rank, 2);
}

TEST(HeuristicSortingTest, ParityWithOldImplementation) {
  // TODO: Implement this test
}

TEST(HeuristicSortingTest, Placeholder) {
    EXPECT_TRUE(true);
}
