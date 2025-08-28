#include "heuristic_sorting/heuristic_sorting.h"

#include <algorithm>

namespace {

const int lho_d[DDS_HANDS] = {1, 2, 3, 0};
const int rho_d[DDS_HANDS] = {3, 0, 1, 2};
const int partner_d[DDS_HANDS] = {2, 3, 0, 1};

void WeightAllocTrump0(const HeuristicContext& context,
                       ScoredMove* scored_moves,
                       int num_moves);

void WeightAllocNT0(const HeuristicContext& context,
                    ScoredMove* scored_moves,
                    int num_moves);

void WeightAllocTrumpNotvoid1(const HeuristicContext& context,
                              ScoredMove* scored_moves,
                              int num_moves);
void WeightAllocNTNotvoid1(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves);
void WeightAllocTrumpVoid1(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves);
void WeightAllocNTVoid1(const HeuristicContext& context,
                        ScoredMove* scored_moves,
                        int num_moves);
void WeightAllocTrumpNotvoid2(const HeuristicContext& context,
                              ScoredMove* scored_moves,
                              int num_moves);
void WeightAllocNTNotvoid2(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves);
void WeightAllocTrumpVoid2(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves);
void WeightAllocNTVoid2(const HeuristicContext& context,
                        ScoredMove* scored_moves,
                        int num_moves);
void WeightAllocCombinedNotvoid3(const HeuristicContext& context,
                                 ScoredMove* scored_moves,
                                 int num_moves);
void WeightAllocTrumpVoid3(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves);
void WeightAllocNTVoid3(const HeuristicContext& context,
                        ScoredMove* scored_moves,
                        int num_moves);

int RankForcesAce(const HeuristicContext& context, int cards4th, int num_moves, const ScoredMove* scored_moves);
void GetTopNumber(const HeuristicContext& context, int ris, int prank, int& topNumber, int& mno, int num_moves, const ScoredMove* scored_moves);

} // namespace

int score_and_order(const HeuristicContext& context,
                    const CandidateMove* candidate_moves,
                    int num_candidate_moves,
                    ScoredMove* scored_moves) {
  // Copy candidate moves to scored moves
  for (int i = 0; i < num_candidate_moves; ++i) {
    scored_moves[i].move = candidate_moves[i];
    scored_moves[i].weight = 0;
  }

  const bool is_trump = context.trump_suit != 4;
  const int hand_index = context.current_hand_index;
  const bool is_void = context.tpos->length[hand_index][context.lead_suit] == 0;

  // Dispatch to the appropriate weighting function based on the context.
  switch (hand_index) {
    case 0: // First player
      if (is_trump) {
        WeightAllocTrump0(context, scored_moves, num_candidate_moves);
      } else {
        WeightAllocNT0(context, scored_moves, num_candidate_moves);
      }
      break;
    case 1: // Second player
      if (is_void) {
        if (is_trump) {
          WeightAllocTrumpVoid1(context, scored_moves, num_candidate_moves);
        } else {
          WeightAllocNTVoid1(context, scored_moves, num_candidate_moves);
        }
      } else {
        if (is_trump) {
          WeightAllocTrumpNotvoid1(context, scored_moves, num_candidate_moves);
        } else {
          WeightAllocNTNotvoid1(context, scored_moves, num_candidate_moves);
        }
      }
      break;
    case 2: // Third player
      if (is_void) {
        if (is_trump) {
          WeightAllocTrumpVoid2(context, scored_moves, num_candidate_moves);
        } else {
          WeightAllocNTVoid2(context, scored_moves, num_candidate_moves);
        }
      } else {
        if (is_trump) {
          WeightAllocTrumpNotvoid2(context, scored_moves, num_candidate_moves);
        } else {
          WeightAllocNTNotvoid2(context, scored_moves, num_candidate_moves);
        }
      }
      break;
    case 3: // Fourth player
      if (is_void) {
        if (is_trump) {
          WeightAllocTrumpVoid3(context, scored_moves, num_candidate_moves);
        } else {
          WeightAllocNTVoid3(context, scored_moves, num_candidate_moves);
        }
      } else {
        WeightAllocCombinedNotvoid3(context, scored_moves, num_candidate_moves);
      }
      break;
  }

  // Sort the moves by weight in descending order.
  std::sort(scored_moves, scored_moves + num_candidate_moves,
            [](const ScoredMove& a, const ScoredMove& b) {
              return a.weight > b.weight;
            });

  return num_candidate_moves;
}

namespace {

void WeightAllocTrump0(const HeuristicContext& context,
                        ScoredMove* scored_moves,
                        int num_moves) {
  if (context.rel_ranks == nullptr) return;
  const int lead_hand = context.lead_hand;
  const int lho = lho_d[lead_hand];
  const int rho = rho_d[lead_hand];
  const int partner = partner_d[lead_hand];
  const int trump = context.trump_suit;

  for (int i = 0; i < num_moves; ++i) {
    const int suit = scored_moves[i].move.suit;
    const unsigned short suit_count = context.tpos->length[lead_hand][suit];
    const unsigned short suit_count_lho = context.tpos->length[lho][suit];
    const unsigned short suit_count_rho = context.tpos->length[rho][suit];

    int count_lho = (suit_count_lho == 0 ? context.tricks + 1 : suit_count_lho) << 2;
    int count_rho = (suit_count_rho == 0 ? context.tricks + 1 : suit_count_rho) << 2;

    int suit_weight_d = -(((count_lho + count_rho) << 5) / 13);

    int suit_bonus = 0;
    bool win_move = false;

    int r_rank = scored_moves[i].move.rank;

    if ((suit != trump) &&
        (((context.tpos->rankInSuit[lho][suit] == 0) &&
          (context.tpos->rankInSuit[lho][trump] != 0)) ||
         ((context.tpos->rankInSuit[rho][suit] == 0) &&
          (context.tpos->rankInSuit[rho][trump] != 0)))) {
      suit_bonus = -12;
    }

    if ((suit != trump) && (context.tpos->length[partner][suit] == 0) &&
        (context.tpos->length[partner][trump] > 0) && (suit_count_rho > 0)) {
      suit_bonus += 17;
    }

    if ((context.tpos->winner[suit].hand == rho) ||
        (context.tpos->secondBest[suit].hand == rho)) {
      if (suit_count_rho != 1) {
        suit_bonus += -12;
      }
    } else if ((context.tpos->winner[suit].hand == lho) &&
               (context.tpos->secondBest[suit].hand == partner)) {
      if (context.tpos->length[partner][suit] != 1) {
        suit_bonus += 27;
      }
    }

    if ((suit != trump) && (suit_count == 1) &&
        (context.tpos->length[lead_hand][trump] > 0) &&
        (context.tpos->length[partner][suit] > 1) &&
        (context.tpos->winner[suit].hand == partner)) {
      suit_bonus += 19;
    }

    int suit_weight_delta = suit_bonus + suit_weight_d;

    if (context.tpos->winner[suit].rank == scored_moves[i].move.rank) {
      if ((suit != trump)) {
        if ((context.tpos->length[partner][suit] != 0) ||
            (context.tpos->length[partner][trump] == 0)) {
          if (((context.tpos->length[lho][suit] != 0) ||
               (context.tpos->length[lho][trump] == 0)) &&
              ((context.tpos->length[rho][suit] != 0) ||
               (context.tpos->length[rho][trump] == 0))) {
            win_move = true;
          }
        } else if (((context.tpos->length[lho][suit] != 0) ||
                    (context.tpos->rankInSuit[partner][trump] >
                     context.tpos->rankInSuit[lho][trump])) &&
                   ((context.tpos->length[rho][suit] != 0) ||
                    (context.tpos->rankInSuit[partner][trump] >
                     context.tpos->rankInSuit[rho][trump]))) {
          win_move = true;
        }
      } else {
        win_move = true;
      }
    } else if (context.tpos->rankInSuit[partner][suit] >
               (context.tpos->rankInSuit[lho][suit] |
                context.tpos->rankInSuit[rho][suit])) {
      if (suit != trump) {
        if (((context.tpos->length[lho][suit] != 0) ||
             (context.tpos->length[lho][trump] == 0)) &&
            ((context.tpos->length[rho][suit] != 0) ||
             (context.tpos->length[rho][trump] == 0))) {
          win_move = true;
        }
      } else {
        win_move = true;
      }
    } else if (suit != trump) {
      if ((context.tpos->length[partner][suit] == 0) &&
          (context.tpos->length[partner][trump] != 0)) {
        if ((context.tpos->length[lho][suit] == 0) &&
            (context.tpos->length[lho][trump] != 0) &&
            (context.tpos->length[rho][suit] == 0) &&
            (context.tpos->length[rho][trump] != 0)) {
          if (context.tpos->rankInSuit[partner][trump] >
              (context.tpos->rankInSuit[lho][trump] |
               context.tpos->rankInSuit[rho][trump])) {
            win_move = true;
          }
        } else if ((context.tpos->length[lho][suit] == 0) &&
                   (context.tpos->length[lho][trump] != 0)) {
          if (context.tpos->rankInSuit[partner][trump] >
              context.tpos->rankInSuit[lho][trump]) {
            win_move = true;
          }
        } else if ((context.tpos->length[rho][suit] == 0) &&
                   (context.tpos->length[rho][trump] != 0)) {
          if (context.tpos->rankInSuit[partner][trump] >
              context.tpos->rankInSuit[rho][trump]) {
            win_move = true;
          }
        } else {
          win_move = true;
        }
      }
    }

    if (win_move) {
      if (((suit_count_lho == 1) && (context.tpos->winner[suit].hand == lho)) ||
          ((suit_count_rho == 1) && (context.tpos->winner[suit].hand == rho))) {
        scored_moves[i].weight = suit_weight_delta + 35 + r_rank;
      } else if (context.tpos->winner[suit].hand == lead_hand) {
        if (context.tpos->secondBest[suit].hand == partner) {
          scored_moves[i].weight = suit_weight_delta + 48 + r_rank;
        } else if (context.tpos->winner[suit].rank == scored_moves[i].move.rank) {
          scored_moves[i].weight = suit_weight_delta + 31;
        } else {
          scored_moves[i].weight = suit_weight_delta - 3 + r_rank;
        }
      } else if (context.tpos->winner[suit].hand == partner) {
        if (context.tpos->secondBest[suit].hand == lead_hand) {
          scored_moves[i].weight = suit_weight_delta + 42 + r_rank;
        } else {
          scored_moves[i].weight = suit_weight_delta + 28 + r_rank;
        }
      } else if ((scored_moves[i].move.sequence) &&
                 (scored_moves[i].move.rank == context.tpos->secondBest[suit].rank)) {
        scored_moves[i].weight = suit_weight_delta + 40;
      } else if (scored_moves[i].move.sequence) {
        scored_moves[i].weight = suit_weight_delta + 22 + r_rank;
      } else {
        scored_moves[i].weight = suit_weight_delta + 11 + r_rank;
      }

      if ((context.best_move.suit == suit) &&
          (context.best_move.rank == scored_moves[i].move.rank)) {
        scored_moves[i].weight += 55;
      } else if ((context.best_move_tt.suit == suit) &&
                 (context.best_move_tt.rank == scored_moves[i].move.rank)) {
        scored_moves[i].weight += 18;
      }
    } else {
      const int aggr = context.tpos->aggr[suit];
      int third_best_hand = context.rel_ranks[aggr].absRank[3][suit].hand;

      if ((context.tpos->secondBest[suit].hand == partner) &&
          (partner == third_best_hand)) {
        suit_weight_delta += 20;
      } else if (((context.tpos->secondBest[suit].hand == lead_hand) &&
                  (partner == third_best_hand) &&
                  (context.tpos->length[partner][suit] > 1)) ||
                 ((context.tpos->secondBest[suit].hand == partner) &&
                  (lead_hand == third_best_hand) &&
                  (context.tpos->length[partner][suit] > 1))) {
        suit_weight_delta += 13;
      }

      if (((suit_count_lho == 1) && (context.tpos->winner[suit].hand == lho)) ||
          ((suit_count_rho == 1) && (context.tpos->winner[suit].hand == rho))) {
        scored_moves[i].weight = suit_weight_delta + r_rank + 2;
      } else if (context.tpos->winner[suit].hand == lead_hand) {
        if (context.tpos->secondBest[suit].hand == partner) {
          scored_moves[i].weight = suit_weight_delta + 33 + r_rank;
        } else if (context.tpos->winner[suit].rank == scored_moves[i].move.rank) {
          scored_moves[i].weight = suit_weight_delta + 38;
        } else {
          scored_moves[i].weight = suit_weight_delta - 14 + r_rank;
        }
      } else if (context.tpos->winner[suit].hand == partner) {
        scored_moves[i].weight = suit_weight_delta + 34 + r_rank;
      } else if ((scored_moves[i].move.sequence) &&
                 (scored_moves[i].move.rank == context.tpos->secondBest[suit].rank)) {
        scored_moves[i].weight = suit_weight_delta + 35;
      } else {
        scored_moves[i].weight = suit_weight_delta + 17 - (scored_moves[i].move.rank);
      }

      if ((context.best_move.suit == suit) &&
          (context.best_move.rank == scored_moves[i].move.rank)) {
        scored_moves[i].weight += 18;
      }
    }
  }
}

void WeightAllocNT0(const HeuristicContext& context,
                    ScoredMove* scored_moves,
                    int num_moves) {
  if (context.rel_ranks == nullptr) return;
  const int lead_hand = context.lead_hand;
  const int lho = lho_d[lead_hand];
  const int rho = rho_d[lead_hand];
  const int partner = partner_d[lead_hand];

  for (int i = 0; i < num_moves; ++i) {
    const int suit = scored_moves[i].move.suit;

    const unsigned short suit_count_lho = context.tpos->length[lho][suit];
    const unsigned short suit_count_rho = context.tpos->length[rho][suit];

    int count_lho = (suit_count_lho == 0 ? context.tricks + 1 : suit_count_lho) << 2;
    int count_rho = (suit_count_rho == 0 ? context.tricks + 1 : suit_count_rho) << 2;

    int suit_weight_d = -(((count_lho + count_rho) << 5) / 19);
    if (context.tpos->length[partner][suit] == 0) {
      suit_weight_d += -9;
    }

    int suit_weight_delta = suit_weight_d;
    int r_rank = scored_moves[i].move.rank;

    if (context.tpos->winner[suit].rank == scored_moves[i].move.rank ||
        (context.tpos->rankInSuit[partner][suit] >
         (context.tpos->rankInSuit[lho][suit] |
          context.tpos->rankInSuit[rho][suit]))) {
      if (context.tpos->secondBest[suit].hand == rho) {
        if (suit_count_rho != 1) {
          suit_weight_delta += -1;
        }
      } else if (context.tpos->secondBest[suit].hand == lho) {
        if (suit_count_lho != 1) {
          suit_weight_delta += 22;
        } else {
          suit_weight_delta += 16;
        }
      }

      if (((context.tpos->secondBest[suit].hand != lho) || (suit_count_lho == 1)) &&
          ((context.tpos->secondBest[suit].hand != rho) || (suit_count_rho == 1))) {
        scored_moves[i].weight = suit_weight_delta + 45 + r_rank;
      } else {
        scored_moves[i].weight = suit_weight_delta + 18 + r_rank;
      }

      if ((context.best_move.suit == suit) &&
          (context.best_move.rank == scored_moves[i].move.rank)) {
        scored_moves[i].weight += 126;
      } else if ((context.best_move_tt.suit == suit) &&
                 (context.best_move_tt.rank == scored_moves[i].move.rank)) {
        scored_moves[i].weight += 32;
      }
    } else {
      if ((context.tpos->winner[suit].hand == rho) ||
          (context.tpos->secondBest[suit].hand == rho)) {
        if (suit_count_rho != 1) {
          suit_weight_delta += -10;
        }
      }

      else if ((context.tpos->winner[suit].hand == lho) &&
               (context.tpos->secondBest[suit].hand == partner)) {
        if (context.tpos->length[partner][suit] != 1) {
          suit_weight_delta += 31;
        }
      }

      const int aggr = context.tpos->aggr[suit];
      int third_best_hand = context.rel_ranks[aggr].absRank[3][suit].hand;

      if ((context.tpos->secondBest[suit].hand == partner) &&
          (partner == third_best_hand)) {
        suit_weight_delta += 35;
      } else if (((context.tpos->secondBest[suit].hand == lead_hand) &&
                  (partner == third_best_hand) &&
                  (context.tpos->length[partner][suit] > 1)) ||
                 ((context.tpos->secondBest[suit].hand == partner) &&
                  (lead_hand == third_best_hand) &&
                  (context.tpos->length[partner][suit] > 1))) {
        suit_weight_delta += 25;
      }

      if (((suit_count_lho == 1) && (context.tpos->winner[suit].hand == lho)) ||
          ((suit_count_rho == 1) && (context.tpos->winner[suit].hand == rho))) {
        scored_moves[i].weight = suit_weight_delta + 28 + r_rank;
      } else if (context.tpos->winner[suit].hand == lead_hand) {
        scored_moves[i].weight = suit_weight_delta - 17 + r_rank;
      } else if (!scored_moves[i].move.sequence) {
        scored_moves[i].weight = suit_weight_delta + 12 + r_rank;
      } else if (scored_moves[i].move.rank == context.tpos->secondBest[suit].rank) {
        scored_moves[i].weight = suit_weight_delta + 48;
      } else {
        scored_moves[i].weight = suit_weight_delta + 29 - r_rank;
      }

      if ((context.best_move.suit == suit) && (context.best_move.rank == scored_moves[i].move.rank)) {
        scored_moves[i].weight += 47;
      } else if ((context.best_move_tt.suit == suit) &&
                 (context.best_move_tt.rank == scored_moves[i].move.rank)) {
        scored_moves[i].weight += 19;
      }
    }
  }
}

void WeightAllocTrumpNotvoid1(const HeuristicContext& context,
                              ScoredMove* scored_moves,
                              int num_moves) {
  if (context.rel_ranks == nullptr) return;
  const int lead_hand = context.lead_hand;
  const int partner = partner_d[lead_hand];
  const int rho = rho_d[lead_hand];
  const int lead_suit = context.lead_suit;
  const int trump = context.trump_suit;

  const int max3rd = context.highest_rank[
                 context.tpos->rankInSuit[partner][lead_suit]];
  const int maxpd = context.highest_rank[
                 context.tpos->rankInSuit[rho][lead_suit]];
  const int min3rd = context.lowest_rank[
                 context.tpos->rankInSuit[partner][lead_suit]];
  const int minpd = context.lowest_rank[
                 context.tpos->rankInSuit[rho][lead_suit]];

  for (int k = 0; k < num_moves; k++) {
    bool win_move = false;
    int r_rank = scored_moves[k].move.rank;

    if (lead_suit == trump) {
      if (maxpd > context.played_cards[0].rank && maxpd > max3rd)
        win_move = true;
      else if (scored_moves[k].move.rank > context.played_cards[0].rank &&
               scored_moves[k].move.rank > max3rd)
        win_move = true;
    } else {
      if (scored_moves[k].move.rank > context.played_cards[0].rank && scored_moves[k].move.rank > max3rd) {
        if ((max3rd != 0) ||
            (context.tpos->length[partner][trump] == 0))
          win_move = true;
        else if ((maxpd == 0) && (context.tpos->length[rho][trump] != 0) &&
                 (context.tpos->rankInSuit[rho][trump] >
                  context.tpos->rankInSuit[partner][trump]))
          win_move = true;
      } else if (maxpd > context.played_cards[0].rank && maxpd > max3rd) {
        if ((max3rd != 0) ||
            (context.tpos->length[partner][trump] == 0))
          win_move = true;
      } else if (context.played_cards[0].rank > maxpd &&
                 context.played_cards[0].rank > max3rd &&
                 context.played_cards[0].rank > scored_moves[k].move.rank) {
        if ((maxpd == 0) && (context.tpos->length[rho][trump] != 0)) {
          if ((max3rd != 0) ||
              (context.tpos->length[partner][trump] == 0))
            win_move = true;
          else if (context.tpos->rankInSuit[rho][trump] >
                   context.tpos->rankInSuit[partner][trump])
            win_move = true;
        }
      } else if (maxpd == 0 && context.tpos->length[rho][trump] != 0)
        win_move = true;
    }

    if (win_move) {
      if (min3rd > scored_moves[k].move.rank)
        scored_moves[k].weight = 40 + r_rank;
      else if ((maxpd > context.played_cards[0].rank) &&
               (context.tpos->rankInSuit[lead_hand][lead_suit] >
                context.tpos->rankInSuit[rho][lead_suit]))
        scored_moves[k].weight = 41 + r_rank;
      else if (scored_moves[k].move.rank > context.played_cards[0].rank) {
        if (scored_moves[k].move.rank < maxpd)
          scored_moves[k].weight = 78 - (scored_moves[k].move.rank);
        else if (scored_moves[k].move.rank > max3rd)
          scored_moves[k].weight = 73 - (scored_moves[k].move.rank);
        else if (scored_moves[k].move.sequence)
          scored_moves[k].weight = 62 - (scored_moves[k].move.rank);
        else
          scored_moves[k].weight = 49 - (scored_moves[k].move.rank);
      } else if (maxpd > 0)
        scored_moves[k].weight = 47 - (scored_moves[k].move.rank);
      else
        scored_moves[k].weight = 40 - (scored_moves[k].move.rank);
    } else if (scored_moves[k].move.rank < min3rd || scored_moves[k].move.rank < minpd)
      scored_moves[k].weight = -9 + r_rank;
    else if (scored_moves[k].move.rank < context.played_cards[0].rank)
      scored_moves[k].weight = -16 + r_rank;
    else if (scored_moves[k].move.sequence)
      scored_moves[k].weight = 22 - (scored_moves[k].move.rank);
    else
      scored_moves[k].weight = 10 - (scored_moves[k].move.rank);
  }
}

void WeightAllocNTNotvoid1(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves) {
  if (context.rel_ranks == nullptr) return;
  const int lead_hand = context.lead_hand;
  const int partner = partner_d[lead_hand];
  const int rho = rho_d[lead_hand];
  const int lead_suit = context.lead_suit;

  const int max3rd = context.highest_rank[
    context.tpos->rankInSuit[partner][lead_suit]];
  const int maxpd = context.highest_rank[
    context.tpos->rankInSuit[rho][lead_suit]];

  if (maxpd > context.played_cards[0].rank && maxpd > max3rd) {
    for (int k = 0; k < num_moves; k++)
      scored_moves[k].weight = -scored_moves[k].move.rank;
  } else {
    int min3rd = context.lowest_rank[
                   context.tpos->rankInSuit[partner][lead_suit]];
    int minpd = context.lowest_rank[
                   context.tpos->rankInSuit[rho][lead_suit]];

    for (int k = 0; k < num_moves; k++) {
      int r_rank = scored_moves[k].move.rank;

      if (scored_moves[k].move.rank > context.played_cards[0].rank && scored_moves[k].move.rank > max3rd)
        scored_moves[k].weight = 81 - scored_moves[k].move.rank;
      else if ((min3rd > scored_moves[k].move.rank) || (minpd > scored_moves[k].move.rank))
        scored_moves[k].weight = -3 + r_rank;
      else if (scored_moves[k].move.rank < context.played_cards[0].rank)
        scored_moves[k].weight = -11 + r_rank;
      else if (scored_moves[k].move.sequence)
        scored_moves[k].weight = 10 + r_rank;
      else
        scored_moves[k].weight = 13 - scored_moves[k].move.rank;
    }
  }
}

void WeightAllocTrumpVoid1(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves) {
  const int lead_hand = context.lead_hand;
  const int partner = partner_d[lead_hand];
  const int rho = rho_d[lead_hand];
  const int lead_suit = context.lead_suit;
  const int trump = context.trump_suit;
  const int current_hand = (lead_hand + 1) % 4;

  for (int k = 0; k < num_moves; k++) {
    const int suit = scored_moves[k].move.suit;
    const unsigned short suit_count = context.tpos->length[current_hand][suit];
    int suit_add;

    if (lead_suit == trump) {
      if (context.tpos->rankInSuit[rho][lead_suit] >
          (context.tpos->rankInSuit[partner][lead_suit] |
           (1 << (context.played_cards[0].rank - 2))))
        suit_add = (suit_count << 6) / 44;
      else {
        suit_add = (suit_count << 6) / 36;
        if ((suit_count == 2) &&
            (context.tpos->secondBest[suit].hand == current_hand))
          suit_add += -4;
      }
      scored_moves[k].weight = -scored_moves[k].move.rank + suit_add;
    } else if (suit != trump) {
      if (context.tpos->length[partner][lead_suit] != 0) {
        if (context.tpos->rankInSuit[rho][lead_suit] >
            (context.tpos->rankInSuit[partner][lead_suit] |
             (1 << (context.played_cards[0].rank - 2))))
          suit_add = 60 + (suit_count << 6) / 44;
        else if ((context.tpos->length[rho][lead_suit] == 0) &&
                 (context.tpos->length[rho][trump] != 0))
          suit_add = 60 + (suit_count << 6) / 44;
        else {
          suit_add = -2 + (suit_count << 6) / 36;
          if ((suit_count == 2) &&
              (context.tpos->secondBest[suit].hand == current_hand))
            suit_add += -4;
        }
      } else if ((context.tpos->length[rho][lead_suit] == 0) &&
                 (context.tpos->rankInSuit[rho][trump] >
                  context.tpos->rankInSuit[partner][trump]))
        suit_add = 60 + (suit_count << 6) / 44;
      else if ((context.tpos->length[partner][trump] == 0) &&
               (context.tpos->rankInSuit[rho][lead_suit] >
                (1 << (context.played_cards[0].rank - 2))))
        suit_add = 60 + (suit_count << 6) / 44;
      else {
        suit_add = -2 + (suit_count << 6) / 36;
        if ((suit_count == 2) &&
            (context.tpos->secondBest[suit].hand == current_hand))
          suit_add += -4;
      }
      scored_moves[k].weight = -scored_moves[k].move.rank + suit_add;
    } else if (context.tpos->length[partner][lead_suit] != 0) {
      suit_add = (suit_count << 6) / 44;
      scored_moves[k].weight = 24 - (scored_moves[k].move.rank) + suit_add;
    } else if ((context.tpos->length[rho][lead_suit] == 0) &&
               (context.tpos->length[rho][trump] != 0) &&
               (context.tpos->rankInSuit[rho][trump] >
                context.tpos->rankInSuit[partner][trump])) {
      suit_add = (suit_count << 6) / 44;
      scored_moves[k].weight = 24 - (scored_moves[k].move.rank) + suit_add;
    } else {
      if ((1 << (scored_moves[k].move.rank - 2)) >
          context.tpos->rankInSuit[partner][trump]) {
        suit_add = (suit_count << 6) / 44;
        scored_moves[k].weight = 24 - (scored_moves[k].move.rank) + suit_add;
      } else {
        suit_add = (suit_count << 6) / 36;
        if ((suit_count == 2) &&
            (context.tpos->secondBest[suit].hand == current_hand))
          suit_add += -4;
        scored_moves[k].weight = 15 - (scored_moves[k].move.rank) + suit_add;
      }
    }
  }
}

void WeightAllocNTVoid1(const HeuristicContext& context,
                        ScoredMove* scored_moves,
                        int num_moves) {
  const int lead_hand = context.lead_hand;
  const int partner = partner_d[lead_hand];
  const int rho = rho_d[lead_hand];
  const int lead_suit = context.lead_suit;
  const int current_hand = (lead_hand + 1) % 4;

  if (context.tpos->rankInSuit[rho][lead_suit] >
      (context.tpos->rankInSuit[partner][lead_suit] |
       (1 << (context.played_cards[0].rank - 2)))) {
    for (int k = 0; k < num_moves; k++) {
      const int suit = scored_moves[k].move.suit;
      unsigned short suit_count = context.tpos->length[current_hand][suit];
      int suit_add = (suit_count << 6) / 23;
      if (suit_count == 2 && context.tpos->secondBest[suit].hand == current_hand)
        suit_add += -2;
      else if (suit_count == 1 && context.tpos->winner[suit].hand == current_hand)
        suit_add += -3;
      scored_moves[k].weight = -scored_moves[k].move.rank + suit_add;
    }
  } else {
    for (int k = 0; k < num_moves; k++) {
      const int suit = scored_moves[k].move.suit;
      unsigned short suit_count = context.tpos->length[current_hand][suit];
      int suit_add = (suit_count << 6) / 33;

      if ((suit_count == 2) &&
          (context.tpos->secondBest[suit].hand == current_hand))
        suit_add += -6;
      else if ((suit_count == 1) &&
               (context.tpos->winner[suit].hand == current_hand))
        suit_add += -8;
      scored_moves[k].weight = -scored_moves[k].move.rank + suit_add;
    }
  }
}

int RankForcesAce(const HeuristicContext& context, int cards4th, int num_moves, const ScoredMove* scored_moves) {
  const moveGroupType& mp = context.group_data[cards4th];

  int g = mp.lastGroup;
  int removed = context.removed_ranks[context.lead_suit];

  while (g >= 1 && ((mp.gap[g] & removed) == mp.gap[g]))
    g--;

  if (!g)
    return -1;

  int secondRHO = (g == 0 ? 0 : mp.rank[g - 1]);

  if (secondRHO > context.played_cards[1].rank) {
    int k = 0;
    while (k < num_moves && scored_moves[k].move.rank > secondRHO)
      k++;

    if (k)
      return k - 1;
  } else if (context.played_cards[1].rank > context.played_cards[0].rank) {
    int k = 0;
    while (k < num_moves && scored_moves[k].move.rank > context.played_cards[1].rank)
      k++;

    if (k)
      return k - 1;
  }

  return -1;
}

void GetTopNumber(const HeuristicContext& context, int ris, int prank, int& topNumber, int& mno, int num_moves, const ScoredMove* scored_moves) {
  topNumber = -10;

  mno = 0;
  while (mno < num_moves - 1 && scored_moves[1 + mno].move.rank > prank)
    mno++;

  const moveGroupType& mp = context.group_data[ris];
  int g = mp.lastGroup;

  int removed = static_cast<int>(context.removed_ranks[context.lead_suit] |
                                 context.bit_map_rank[prank]);

  int fullseq = mp.fullseq[g];

  while (g >= 1 && ((mp.gap[g] & removed) == mp.gap[g]))
    fullseq |= mp.fullseq[--g];

  topNumber = context.count_table[fullseq] - 1;
}

void WeightAllocTrumpNotvoid2(const HeuristicContext& context,
                              ScoredMove* scored_moves,
                              int num_moves) {
  const int lead_hand = context.lead_hand;
  const int rho = rho_d[lead_hand];
  const int lead_suit = context.lead_suit;
  const int trump = context.trump_suit;

  const int cards4th = context.tpos->rankInSuit[rho][lead_suit];
  const int max4th = context.highest_rank[cards4th];
  const int min4th = context.lowest_rank[cards4th];
  const int max3rd = scored_moves[0].move.rank;

  if (lead_suit == trump) {
    if (context.played_cards[1].rank < context.played_cards[0].rank && context.played_cards[0].rank > max4th) {
      for (int k = 0; k < num_moves; k++)
        scored_moves[k].weight = -scored_moves[k].move.rank;
      return;
    } else if (max3rd < min4th || max3rd < context.played_cards[1].rank) {
      for (int k = 0; k < num_moves; k++)
        scored_moves[k].weight = -scored_moves[k].move.rank;
      return;
    } else if (max3rd > max4th) {
      for (int k = 0; k < num_moves; k++) {
        if (scored_moves[k].move.rank > max4th &&
            scored_moves[k].move.rank > context.played_cards[1].rank)
          scored_moves[k].weight = 58 - scored_moves[k].move.rank;
        else
          scored_moves[k].weight = -scored_moves[k].move.rank;
      }
    } else {
      int kBonus = RankForcesAce(context, cards4th, num_moves, scored_moves);

      for (int k = 0; k < num_moves; k++)
        scored_moves[k].weight = -scored_moves[k].move.rank;

      if (kBonus != -1)
        scored_moves[kBonus].weight += 20;
      return;
    }
  } else if (context.played_cards[1].suit == trump) {
    for (int k = 0; k < num_moves; k++)
      scored_moves[k].weight = -scored_moves[k].move.rank;
    return;
  } else if (context.played_cards[1].rank < context.played_cards[0].rank) {
    if (max4th == 0) {
      for (int k = 0; k < num_moves; k++)
        scored_moves[k].weight = -scored_moves[k].move.rank;
      return;
    }
    else if (context.played_cards[0].rank > max4th) {
      for (int k = 0; k < num_moves; k++)
        scored_moves[k].weight = -scored_moves[k].move.rank;
      return;
    }
    else if (max3rd < min4th || max3rd < context.played_cards[1].rank) {
      for (int k = 0; k < num_moves; k++)
        scored_moves[k].weight = -scored_moves[k].move.rank;
      return;
    }
    else if (max3rd > max4th) {
      for (int k = 0; k < num_moves; k++) {
        if (scored_moves[k].move.rank > max4th)
          scored_moves[k].weight = 58 - scored_moves[k].move.rank;
        else
          scored_moves[k].weight = -scored_moves[k].move.rank;
      }
    } else {
      int kBonus = RankForcesAce(context, cards4th, num_moves, scored_moves);

      for (int k = 0; k < num_moves; k++) {
        if (scored_moves[k].move.rank > context.played_cards[1].rank &&
            scored_moves[k].move.rank > max4th)
          scored_moves[k].weight = 60 - scored_moves[k].move.rank;
        else
          scored_moves[k].weight = -scored_moves[k].move.rank;
      }

      if (kBonus != -1)
        scored_moves[kBonus].weight += 20;
    }
  } else {
    if (max4th == 0) {
      for (int k = 0; k < num_moves; k++) {
        if (scored_moves[k].move.rank > context.played_cards[1].rank)
          scored_moves[k].weight = 20 - scored_moves[k].move.rank;
        else
          scored_moves[k].weight = -scored_moves[k].move.rank;
      }
      return;
    }
    else if (max3rd < min4th || max3rd < context.played_cards[1].rank) {
      for (int k = 0; k < num_moves; k++)
        scored_moves[k].weight = -scored_moves[k].move.rank;
      return;
    }
    else if (max3rd > max4th) {
      for (int k = 0; k < num_moves; k++) {
        if (scored_moves[k].move.rank > context.played_cards[1].rank &&
            scored_moves[k].move.rank > max4th)
          scored_moves[k].weight = 58 - scored_moves[k].move.rank;
        else
          scored_moves[k].weight = -scored_moves[k].move.rank;
      }
      return;
    }
    int kBonus = RankForcesAce(context, cards4th, num_moves, scored_moves);

    for (int k = 0; k < num_moves; k++) {
      if (scored_moves[k].move.rank > context.played_cards[1].rank &&
          scored_moves[k].move.rank > max4th)
        scored_moves[k].weight = 60 - scored_moves[k].move.rank;
      else
        scored_moves[k].weight = -scored_moves[k].move.rank;
    }

    if (kBonus != -1)
      scored_moves[kBonus].weight += 20;
  }
}

void WeightAllocNTNotvoid2(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves) {
  const int lead_hand = context.lead_hand;
  const int rho = rho_d[lead_hand];
  const int lead_suit = context.lead_suit;
  const int current_hand = (lead_hand + 2) % 4;

  const int cards4th = context.tpos->rankInSuit[rho][lead_suit];
  const int max4th = context.highest_rank[cards4th];
  const int min4th = context.lowest_rank[cards4th];
  const int max3rd = scored_moves[0].move.rank;

  if (context.played_cards[1].rank > context.played_cards[0].rank && context.played_cards[0].rank > max4th) {
    for (int k = 0; k < num_moves; k++)
      scored_moves[k].weight = -scored_moves[k].move.rank;

    if (context.tpos->length[lead_hand][lead_suit] == 0 &&
        context.tpos->winner[lead_suit].hand == current_hand) {
      int oppLen = context.tpos->length[rho][lead_suit] - 1;
      int lhoLen = context.tpos->length[lho_d[lead_hand]][lead_suit];
      if (lhoLen > oppLen)
        oppLen = lhoLen;

      int topNumber, mno;
      GetTopNumber(context, context.tpos->rankInSuit[partner_d[lead_hand]][lead_suit],
                   context.played_cards[0].rank, topNumber, mno, num_moves, scored_moves);

      if (oppLen <= topNumber)
        scored_moves[mno].weight += 20;
    }
    return;
  } else if (max3rd < min4th || max3rd < context.played_cards[1].rank) {
    for (int k = 0; k < num_moves; k++)
      scored_moves[k].weight = -scored_moves[k].move.rank;
    return;
  }

  int kBonus = -1;
  if (max4th > max3rd && max4th > context.played_cards[1].rank)
    kBonus = RankForcesAce(context, cards4th, num_moves, scored_moves);

  for (int k = 0; k < num_moves; k++) {
    if (scored_moves[k].move.rank > context.played_cards[1].rank &&
        scored_moves[k].move.rank > max4th)
      scored_moves[k].weight = 60 - scored_moves[k].move.rank;
    else
      scored_moves[k].weight = -scored_moves[k].move.rank;
  }

  if (kBonus != -1)
    scored_moves[kBonus].weight += 20;
}

void WeightAllocTrumpVoid2(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves) {
  const int lead_hand = context.lead_hand;
  const int rho = rho_d[lead_hand];
  const int lead_suit = context.lead_suit;
  const int trump = context.trump_suit;
  const int current_hand = (lead_hand + 2) % 4;

  const int max4th = context.highest_rank[
                 context.tpos->rankInSuit[rho][lead_suit]];

  for (int k = 0; k < num_moves; k++) {
    const int suit = scored_moves[k].move.suit;
    const unsigned short suit_count = context.tpos->length[current_hand][suit];
    int suit_add;

    if (lead_suit == trump || suit != trump) {
      suit_add = (suit_count << 6) / 40;
      scored_moves[k].weight = -scored_moves[k].move.rank + suit_add;
      continue;
    }

    if (context.played_cards[1].rank < context.played_cards[0].rank && context.played_cards[0].rank > max4th &&
        (max4th != 0 || context.tpos->length[rho][trump] == 0)) {
      scored_moves[k].weight = -scored_moves[k].move.rank - 50;
      continue;
    }

    if (context.played_cards[1].suit == trump &&
        scored_moves[k].move.rank < context.played_cards[1].rank) {
      int r_rank = scored_moves[k].move.rank;
      suit_add = (suit_count << 6) / 40;
      scored_moves[k].weight = -32 + r_rank + suit_add;
    } else if (context.played_cards[1].rank < context.played_cards[0].rank) {
      if (max4th != 0) {
        if (context.tpos->secondBest[lead_suit].hand == lead_hand) {
          suit_add = (suit_count << 6) / 50;
          scored_moves[k].weight = 36 - scored_moves[k].move.rank + suit_add;
        } else {
          suit_add = (suit_count << 6) / 50;
          scored_moves[k].weight = 48 - scored_moves[k].move.rank + suit_add;
        }
      } else if ((1 << (scored_moves[k].move.rank - 2)) >
                 context.tpos->rankInSuit[rho][trump]) {
        suit_add = (suit_count << 6) / 50;
        scored_moves[k].weight = 48 - scored_moves[k].move.rank + suit_add;
      } else {
        suit_add = (suit_count << 6) / 50;
        scored_moves[k].weight = -12 - scored_moves[k].move.rank + suit_add;
      }
    } else if (max4th != 0) {
      suit_add = (suit_count << 6) / 50;
      scored_moves[k].weight = 72 - scored_moves[k].move.rank + suit_add;
    } else if ((1 << (scored_moves[k].move.rank - 2)) >
               context.tpos->rankInSuit[rho][trump]) {
      suit_add = (suit_count << 6) / 50;
      scored_moves[k].weight = 48 - scored_moves[k].move.rank + suit_add;
    } else {
      suit_add = (suit_count << 6) / 50;
      scored_moves[k].weight = 36 - scored_moves[k].move.rank + suit_add;
    }
  }
}

void WeightAllocNTVoid2(const HeuristicContext& context,
                        ScoredMove* scored_moves,
                        int num_moves) {
  const int lead_hand = context.lead_hand;
  const int current_hand = (lead_hand + 2) % 4;

  for (int k = 0; k < num_moves; k++) {
    const int suit = scored_moves[k].move.suit;
    const unsigned short suit_count = context.tpos->length[current_hand][suit];
    int suit_add = (suit_count << 6) / 24;

    if (suit_count == 2 && context.tpos->secondBest[suit].hand == current_hand)
      suit_add -= 4;
    else if (suit_count == 1 && context.tpos->winner[suit].hand == current_hand)
      suit_add -= 4;

    scored_moves[k].weight = -(scored_moves[k].move.rank) + suit_add;
  }
}

void WeightAllocCombinedNotvoid3(const HeuristicContext& context,
                                 ScoredMove* scored_moves,
                                 int num_moves) {
  const int lead_suit = context.lead_suit;
  const int trump = context.trump_suit;

  const int winning_rank = context.played_cards[2].rank;
  const int winning_suit = context.played_cards[2].suit;

  if (winning_suit == trump ||
      (lead_suit != trump && context.played_cards[2].suit == trump)) {
    for (int k = 0; k < num_moves; k++)
      scored_moves[k].weight = -scored_moves[k].move.rank;
  } else {
    for (int k = 0; k < num_moves; k++) {
      if (scored_moves[k].move.rank > winning_rank)
        scored_moves[k].weight = 30 - scored_moves[k].move.rank;
      else
        scored_moves[k].weight = -scored_moves[k].move.rank;
    }
  }
}

void WeightAllocTrumpVoid3(const HeuristicContext& context,
                           ScoredMove* scored_moves,
                           int num_moves) {
  if (context.rel_ranks == nullptr) return;
  const int lead_hand = context.lead_hand;
  const int lead_suit = context.lead_suit;
  const int trump = context.trump_suit;
  const int current_hand = (lead_hand + 3) % 4;

  for (int k = 0; k < num_moves; k++) {
    const int suit = scored_moves[k].move.suit;
    const int mylen = context.tpos->length[current_hand][suit];
    int val = (mylen << 6) / 24;
    if ((mylen == 2) && (context.tpos->secondBest[suit].hand == current_hand))
      val -= 2;

    if (lead_suit == trump) {
      scored_moves[k].weight = -scored_moves[k].move.rank + val;
    } else if (context.played_cards[2].rank > context.played_cards[0].rank && context.played_cards[2].rank > context.played_cards[1].rank) {
      if (suit == trump)
        scored_moves[k].weight = 2 - scored_moves[k].move.rank + val;
      else
        scored_moves[k].weight = 25 - scored_moves[k].move.rank + val;
    } else if (context.played_cards[2].suit == trump) {
      if (suit == trump) {
        int r_rank = scored_moves[k].move.rank;
        if (scored_moves[k].move.rank > context.played_cards[2].rank)
          scored_moves[k].weight = 33 + r_rank;
        else
          scored_moves[k].weight = -13 + r_rank;
      }
      else
        scored_moves[k].weight = 14 - (scored_moves[k].move.rank) + val;
    } else if (suit == trump) {
      int r_rank = scored_moves[k].move.rank;
      scored_moves[k].weight = 33 + r_rank;
    } else {
      scored_moves[k].weight = 14 - scored_moves[k].move.rank + val;
    }
  }
}

void WeightAllocNTVoid3(const HeuristicContext& context,
                        ScoredMove* scored_moves,
                        int num_moves) {
  const int lead_hand = context.lead_hand;
  const int current_hand = (lead_hand + 3) % 4;

  for (int k = 0; k < num_moves; k++) {
    const int suit = scored_moves[k].move.suit;
    int mylen = context.tpos->length[current_hand][suit];
    int val = (mylen << 6) / 27;
    if ((mylen == 2) && (context.tpos->secondBest[suit].hand == current_hand))
      val -= 6;
    else if ((mylen == 1) && (context.tpos->winner[suit].hand == current_hand))
      val -= 8;

    scored_moves[k].weight = -scored_moves[k].move.rank + val;
  }
}

}
