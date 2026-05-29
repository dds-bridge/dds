/// @file analyse_play_consistency.cpp
/// @brief Self-consistency regression tests for AnalysePlay.
/// @details For any deal and play, the trick count AnalysePlayPBN reports after
/// each card must equal an independent SolveBoardPBN of that same position.
/// These two code paths share no transposition-table state, so agreement is a
/// strong correctness check that needs no external reference solver. This guards
/// the regression in issue #156, where analyse_later_board used a fresh (cold)
/// transposition table per card and under-counted tricks.

// C++ standard library headers
#include <algorithm>
#include <array>
#include <cstring>
#include <random>
#include <string>
#include <vector>

// Third-party headers
#include <gtest/gtest.h>

// Project headers
#include <api/dds.h>

namespace {

constexpr int kStrainNT = 4;  // trump value for No Trump

using Card = std::pair<int, int>;          // (suit 0..3, rank 2..14)
using Hands = std::array<std::vector<Card>, 4>;  // indexed N,E,S,W

const char kSuitChar[4] = {'S', 'H', 'D', 'C'};

auto rank_char(int r) -> char
{
  static const char* kRanks = "23456789TJQKA";  // index 0 -> rank 2
  return kRanks[r - 2];
}

auto rank_value(char c) -> int
{
  switch (c) {
    case 'A': return 14;
    case 'K': return 13;
    case 'Q': return 12;
    case 'J': return 11;
    case 'T': return 10;
    default:  return c - '0';
  }
}

auto suit_index(char c) -> int
{
  switch (c) {
    case 'S': return 0;
    case 'H': return 1;
    case 'D': return 2;
    default:  return 3;  // 'C'
  }
}

// Emit a PBN deal string ("N:<N> <E> <S> <W>") from the four hands.
auto hands_to_pbn(const Hands& hands) -> std::string
{
  std::string out = "N:";
  for (int h = 0; h < 4; ++h) {
    if (h) out += ' ';
    std::array<std::vector<int>, 4> by_suit;
    for (const auto& [s, r] : hands[static_cast<size_t>(h)])
      by_suit[static_cast<size_t>(s)].push_back(r);
    for (int s = 0; s < 4; ++s) {
      if (s) out += '.';
      auto& v = by_suit[static_cast<size_t>(s)];
      std::sort(v.rbegin(), v.rend());
      for (int r : v) out += rank_char(r);
    }
  }
  return out;
}

// Winner (0..3) of a completed four-card trick; cards are in play order
// starting from `leader`. trump == kStrainNT means no trump.
auto trick_winner(const std::array<Card, 4>& cards, int trump, int leader) -> int
{
  const int lead_suit = cards[0].first;
  int best = 0;
  for (int i = 1; i < 4; ++i) {
    const auto [si, ri] = cards[static_cast<size_t>(i)];
    const auto [sb, rb] = cards[static_cast<size_t>(best)];
    const bool i_trump = (trump != kStrainNT && si == trump);
    const bool b_trump = (trump != kStrainNT && sb == trump);
    if (i_trump && !b_trump) best = i;
    else if (i_trump && b_trump) { if (ri > rb) best = i; }
    else if (!i_trump && !b_trump) {
      if (si == lead_suit && (sb != lead_suit || ri > rb)) best = i;
    }
  }
  return (leader + best) % 4;
}

// SolveBoardPBN: max tricks for the side to play from the given position.
auto solve_max(int trump, int leader, const std::vector<Card>& cur,
               const Hands& hands) -> int
{
  struct DealPBN dl;
  std::memset(&dl, 0, sizeof(dl));
  dl.trump = trump;
  dl.first = leader;
  for (size_t i = 0; i < cur.size() && i < 3; ++i) {
    dl.currentTrickSuit[i] = cur[i].first;
    dl.currentTrickRank[i] = cur[i].second;
  }
  const std::string pbn = hands_to_pbn(hands);
  std::strncpy(dl.remainCards, pbn.c_str(), sizeof(dl.remainCards) - 1);

  struct FutureTricks fut;
  const int rc = SolveBoardPBN(dl, -1, 1, 1, &fut, 0);
  EXPECT_EQ(RETURN_NO_FAULT, rc);
  return fut.score[0];
}

// Core check: AnalysePlayPBN's per-card trick count must match an independent
// SolveBoardPBN of the reconstructed position at every analysed ply.
auto check_self_consistency(const Hands& hands, int trump, int opening_leader,
                            const std::vector<Card>& play) -> void
{
  // --- Run AnalysePlayPBN over the whole play. ---
  struct DealPBN dl;
  std::memset(&dl, 0, sizeof(dl));
  dl.trump = trump;
  dl.first = opening_leader;
  const std::string deal_pbn = hands_to_pbn(hands);
  std::strncpy(dl.remainCards, deal_pbn.c_str(), sizeof(dl.remainCards) - 1);

  struct PlayTracePBN trace;
  std::memset(&trace, 0, sizeof(trace));
  trace.number = static_cast<int>(play.size());
  std::string play_str;
  for (const auto& [s, r] : play) {
    play_str += kSuitChar[s];
    play_str += rank_char(r);
  }
  std::strncpy(trace.cards, play_str.c_str(), sizeof(trace.cards) - 1);

  struct SolvedPlay solved;
  std::memset(&solved, 0, sizeof(solved));
  ASSERT_EQ(RETURN_NO_FAULT, AnalysePlayPBN(dl, trace, &solved, 0));

  // --- Reconstruct each position and compare. ---
  // AnalysePlay reports cumulative tricks for the declaring side (the side NOT
  // on opening lead). tricks[0..number-1] are meaningful; tricks[number] is a
  // terminal boundary entry that is not compared.
  const int decl_parity = 1 - (opening_leader % 2);

  Hands cur_hands = hands;
  std::vector<Card> cur;          // cards in the current (incomplete) trick
  int leader = opening_leader;
  int completed = 0;              // completed tricks so far
  int decl_won = 0;               // completed tricks won by the declaring side

  for (size_t k = 0; k < play.size(); ++k) {
    if (static_cast<int>(k) < solved.number) {
      const int remaining = 13 - completed;
      const int player_to_act = (leader + static_cast<int>(cur.size())) % 4;
      const int sb = solve_max(trump, leader, cur, cur_hands);
      const int decl_remaining =
        (player_to_act % 2 == decl_parity) ? sb : (remaining - sb);
      const int expected = decl_won + decl_remaining;
      EXPECT_EQ(expected, solved.tricks[k])
        << "AnalysePlay disagrees with SolveBoard at ply " << k
        << " (deal " << deal_pbn << ", trump " << trump
        << ", leader " << opening_leader << ")";
    }

    // Advance the reconstruction by playing card k.
    const Card card = play[k];
    const int player = (leader + static_cast<int>(cur.size())) % 4;
    auto& ph = cur_hands[static_cast<size_t>(player)];
    ph.erase(std::remove(ph.begin(), ph.end(), card), ph.end());
    cur.push_back(card);
    if (cur.size() == 4) {
      std::array<Card, 4> t{cur[0], cur[1], cur[2], cur[3]};
      const int w = trick_winner(t, trump, leader);
      if (w % 2 == decl_parity) ++decl_won;
      ++completed;
      leader = w;
      cur.clear();
    }
  }
}

// Deal 52 cards into four hands using a deterministic generator.
auto make_deal(std::mt19937& rng) -> Hands
{
  std::vector<Card> deck;
  for (int s = 0; s < 4; ++s)
    for (int r = 2; r <= 14; ++r)
      deck.emplace_back(s, r);
  std::shuffle(deck.begin(), deck.end(), rng);
  Hands hands;
  for (int h = 0; h < 4; ++h)
    for (int i = 0; i < 13; ++i)
      hands[static_cast<size_t>(h)].push_back(deck[static_cast<size_t>(h * 13 + i)]);
  return hands;
}

// Generate a full legal 52-card play, following suit.
auto make_play(const Hands& deal, int leader, int trump, std::mt19937& rng)
  -> std::vector<Card>
{
  Hands hands = deal;
  std::vector<Card> out;
  std::vector<Card> cur;
  int cur_leader = leader;
  int player = leader;
  for (int ply = 0; ply < 52; ++ply) {
    auto& hand = hands[static_cast<size_t>(player)];
    Card card;
    if (cur.empty()) {
      card = hand[rng() % hand.size()];
    } else {
      const int ls = cur[0].first;
      std::vector<Card> follow;
      for (const auto& c : hand) if (c.first == ls) follow.push_back(c);
      const auto& pool = follow.empty() ? hand : follow;
      card = pool[rng() % pool.size()];
    }
    hand.erase(std::remove(hand.begin(), hand.end(), card), hand.end());
    out.push_back(card);
    cur.push_back(card);
    player = (player + 1) % 4;
    if (cur.size() == 4) {
      std::array<Card, 4> t{cur[0], cur[1], cur[2], cur[3]};
      cur_leader = trick_winner(t, trump, cur_leader);
      player = cur_leader;
      cur.clear();
    }
  }
  return out;
}

// Parse "S:<S> <W> <N> <E>"-ordered PBN-like literal hands into our N,E,S,W
// layout from explicit per-seat holding strings (suits "s.h.d.c").
auto hand_from_holdings(const std::array<std::string, 4>& suits) -> std::vector<Card>
{
  std::vector<Card> cards;
  for (int s = 0; s < 4; ++s)
    for (char c : suits[static_cast<size_t>(s)])
      cards.emplace_back(s, rank_value(c));
  return cards;
}

class AnalysePlayConsistency : public ::testing::Test
{
protected:
  void SetUp() override { SetMaxThreads(0); }
};

// The exact deal from dds-bridge/dds issue #156.
TEST_F(AnalysePlayConsistency, Issue156)
{
  Hands hands;
  // N,E,S,W (suits S.H.D.C):
  hands[0] = hand_from_holdings({"AKT4", "5", "762", "J9864"});       // North
  hands[1] = hand_from_holdings({"8", "AT98", "A54", "KT753"});       // East
  hands[2] = hand_from_holdings({"97652", "K632", "K8", "AQ"});       // South
  hands[3] = hand_from_holdings({"QJ3", "QJ74", "QJT93", "2"});       // West

  const int trump = 1;   // Hearts
  const int leader = 0;  // North leads
  const std::string play_str =
    "SKS8S2S3H5HTHKH4H2HQD2H8SQSAHAS5H9H3HJC4H7D6C5H6DQD7DAD8"
    "D5DKD3C6S9SJS4C7DJC8D4S6DTC9CTCQD9CJCKS7C2STC3CA";
  std::vector<Card> play;
  for (size_t i = 0; i + 1 < play_str.size(); i += 2)
    play.emplace_back(suit_index(play_str[i]), rank_value(play_str[i + 1]));

  check_self_consistency(hands, trump, leader, play);
}

// Broad coverage: deterministic random deals, full legal play-outs.
TEST_F(AnalysePlayConsistency, MatchesSolveBoardAcrossRandomPlayouts)
{
  std::mt19937 rng(20260529u);
  constexpr int kDeals = 25;
  for (int d = 0; d < kDeals; ++d) {
    const Hands hands = make_deal(rng);
    const int trump = static_cast<int>(rng() % 5);
    const int leader = static_cast<int>(rng() % 4);
    const std::vector<Card> play = make_play(hands, leader, trump, rng);
    check_self_consistency(hands, trump, leader, play);
  }
}

}  // namespace
