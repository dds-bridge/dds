/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/*
   This is an implementation of the transposition table that requires
   a lot of memory and is somewhat faster than the small version.
*/


#include <vector>
#include <string>

#include "trans_table.hpp"


enum
{
  NumPagesDefault = 15,
  NumPagesMaximum = 25,
  BlocksPerPage = 1000,
  DistsPerEntry = 32,
  BlocksPerEntry = 125,
  FirstHarvestTrick = 8,
  HarvestAge = 10000,
  TtBytes = 4,
  TtTricks = 12,
  TT_LINE_LEN = 20
};

inline constexpr double TtPercentile = 0.9;


class TransTableL: public TransTable
{
  private:

    struct WinMatch // 52 bytes
    {
      unsigned xor_set_;
      unsigned top_set1_ , top_set2_ , top_set3_ , top_set4_ ;
      unsigned top_mask1_, top_mask2_, top_mask3_, top_mask4_;
      int mask_index_;
      int last_mask_no_;
      NodeCards first_;
    };

    struct WinBlock // 6508 bytes when BlocksPerEntry == 125
    {
      int next_match_no_;
      int next_write_no_;
      int timestamp_read_;
      WinMatch list_[BlocksPerEntry];
    };

    struct PosSearch // 16 bytes (inefficiency, 12 bytes enough)
    {
      WinBlock * pos_block_;
      long long key_;
    };

    struct DistHash // 520 bytes when DistsPerEntry == 32
    {
      int next_no_;
      int next_write_no_;
      PosSearch list_[DistsPerEntry];
    };

    struct Aggr // 80 bytes
    {
      unsigned aggr_ranks_[DDS_SUITS];
      unsigned aggr_bytes_[DDS_SUITS][TtBytes];
    };

    struct Pool // 16 bytes
    {
      Pool * next_;
      Pool * prev_;
      int next_block_no_;
      WinBlock * list_;
    };

    struct PageStats
    {
      int num_resets_;
      int num_callocs_;
      int num_frees_;
      int num_harvests_;
      int last_current_;
    };

    struct Harvested // 16 bytes
    {
      int next_block_no_;
      WinBlock * list_ [BlocksPerPage];
    };

    enum class MemState
    {
      FROM_POOL,
      FROM_HARVEST
    };

    // Private data for the full memory version.
    MemState mem_state_;

    int pages_default_;
    int pages_current_;
    int pages_maximum_;

    int harvest_trick_;
    int harvest_hand_;

    PageStats page_stats_;

    // aggr is constant for a given hand.
    Aggr aggr_[8192]; // 64 KB

    // This is the real transposition table.
    // The last index is the hash.
    // 6240 KB with above assumptions
    // DistHash tt_root_[TtTricks][DDS_HANDS][256];
    DistHash * tt_root_[TtTricks][DDS_HANDS];

    // It is useful to remember the last block we looked at.
    WinBlock * last_block_seen_[TtTricks][DDS_HANDS];

    // The pool of card entries for a given suit distribution.
    Pool * pool_;
    WinBlock * next_block_;
    Harvested harvested_;

    int timestamp_;
    int tt_in_use_;


    auto init_tt() -> void;

    auto release_tt() -> void;

  // Constants are provided via internal function-local static tables.

    auto hash8(const int hand_dist[]) const -> int;

    auto get_next_card_block() -> WinBlock *;

    auto lookup_suit(
      DistHash * dp,
      long long key,
      bool& empty) -> WinBlock *;

    auto lookup_cards(
      const WinMatch& search,
      WinBlock * bp,
      int limit,
      bool& lowerFlag) -> NodeCards *;

    auto create_or_update(
      WinBlock * bp,
      const WinMatch& search,
      bool flag) -> void;

    auto harvest() -> bool;

    // Debug functions from here on.

    auto key_to_dist(
      long long key,
      int hand_dist[]) const -> void;

    auto dist_to_lengths(
      int trick,
      const int hand_dist[],
      unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> void;

    auto single_len_to_str(const unsigned char length[]) const -> std::string;

    auto len_to_str(
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> std::string;

    auto make_hist_stats(
      const int hist[],
      int& count,
      int& prodSum,
      int& prodSumsq,
      int& maxLen,
      int lastIndex) const -> void;

    auto calc_percentile(
      const int hist[],
      double threshold,
      int lastIndex) const -> int;

    auto print_hist(
      std::ofstream& fout,
      const int hist[],
      int numWraps,
      int lastIndex) const -> void;

    auto update_suit_hist(
      int trick,
      int hand,
      int hist[],
      int& numWraps) const -> void;

    auto update_suit_hist(
      int trick,
      int hand,
      int hist[],
      int suit_hist[],
      int& num_wraps,
      int& suit_wraps) const -> void;

    auto find_matching_dist(
      int trick,
      int hand,
      const int hand_dist_sought[]) const -> WinBlock const *;

    auto print_entries_block(
      std::ofstream& fout,
      WinBlock const * bp,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> void;

    auto update_entry_hist(
      int trick,
      int hand,
      int hist[],
      int& numWraps) const -> void;

    auto update_entry_hist(
      int trick,
      int hand,
      int hist[],
      int suitHist[],
      int& numWraps,
      int& suitWraps) const -> void;

    auto effect_of_block_bound(
      const int hist[],
      int size) const -> int;

    auto print_node_values(
      std::ofstream& fout,
      const NodeCards& node) const -> void;

    auto print_match(
      std::ofstream& fout,
      const WinMatch& match,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> void;

    auto make_holding(
      const std::string& high,
      unsigned len) const -> std::string;

    auto dump_hands(
      std::ofstream& fout,
      const std::vector<std::vector<std::string>>& hands,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> void;

    auto set_to_partial_hands(
      const unsigned set,
      const unsigned mask,
      const int max_rank,
      const int num_ranks,
      std::vector<std::vector<std::string>>& hands) const -> void;

    auto blocks_in_use() const -> int;

    // Legacy implementation helpers removed; modern overrides are canonical.

  public:
    TransTableL();

    ~TransTableL();
    // Examples:
    // int hd[DDS_HANDS] = { 0x0342, 0x0334, 0x0232, 0x0531 };
    // thrp->transTable.PrintEntriesDist(cout, 11, 1, hd);
    // unsigned short ag[DDS_HANDS] =
    // { 0x1fff, 0x1fff, 0x0f75, 0x1fff };
    // thrp->transTable.PrintEntriesDistAndCards(cout, 11, 1, ag, hd);

    // Modern overrides (out-of-line implementations in .cpp)
    auto init(const int hand_lookup[][15]) -> void override;
    auto set_memory_default(int megabytes) -> void override;
    auto set_memory_maximum(int megabytes) -> void override;
    auto make_tt() -> void override;
    auto reset_memory(ResetReason reason) -> void override;
    auto return_all_memory() -> void override;
    auto memory_in_use() const -> double override;
    auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag) -> NodeCards const * override;
    auto add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks_arg[],
      const NodeCards& first,
      bool flag) -> void override;
    auto print_suits(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;
    auto print_all_suits(std::ofstream& fout) const -> void override;
    auto print_suit_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;
    auto print_all_suit_stats(std::ofstream& fout) const -> void override;
    auto print_summary_suit_stats(std::ofstream& fout) const -> void override;
    auto print_entries_dist(
      std::ofstream& fout,
      int trick,
      int hand,
      const int hand_dist[]) const -> void override;
    auto print_entries_dist_and_cards(
      std::ofstream& fout,
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[]) const -> void override;
    auto print_entries(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;
    auto print_all_entries(std::ofstream& fout) const -> void override;
    auto print_entry_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;
    auto print_all_entry_stats(std::ofstream& fout) const -> void override;
    auto print_summary_entry_stats(std::ofstream& fout) const -> void override;
};