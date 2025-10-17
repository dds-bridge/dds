/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TRANSTABLEL_H
#define DDS_TRANSTABLEL_H

/*
   This is an implementation of the transposition table that requires
   a lot of memory and is somewhat faster than the small version.
*/


#include <vector>
#include <string>

#include "TransTable.h"


enum {
  NUM_PAGES_DEFAULT = 15,
  NUM_PAGES_MAXIMUM = 25,
  BLOCKS_PER_PAGE = 1000,
  DISTS_PER_ENTRY = 32,
  BLOCKS_PER_ENTRY = 125,
  FIRST_HARVEST_TRICK = 8,
  HARVEST_AGE = 10000,
  TT_BYTES = 4,
  TT_TRICKS = 12,
  TT_LINE_LEN = 20
};

inline constexpr double TT_PERCENTILE = 0.9;


class TransTableL: public TransTable
{
  private:

    struct WinMatch // 52 bytes
    {
      unsigned xorSet;
      unsigned topSet1 , topSet2 , topSet3 , topSet4 ;
      unsigned topMask1, topMask2, topMask3, topMask4;
      int maskIndex;
      int lastMaskNo;
      NodeCards first;
    };

    struct WinBlock // 6508 bytes when BLOCKS_PER_ENTRY == 125
    {
      int nextMatchNo;
      int nextWriteNo;
      int timestampRead;
      WinMatch list[BLOCKS_PER_ENTRY];
    };

    struct PosSearch // 16 bytes (inefficiency, 12 bytes enough)
    {
      WinBlock * posBlock;
      long long key;
    };

    struct DistHash // 520 bytes when DISTS_PER_ENTRY == 32
    {
      int nextNo;
      int nextWriteNo;
      PosSearch list[DISTS_PER_ENTRY];
    };

    struct Aggr // 80 bytes
    {
      unsigned aggrRanks[DDS_SUITS];
      unsigned aggrBytes[DDS_SUITS][TT_BYTES];
    };

    struct Pool // 16 bytes
    {
      Pool * next;
      Pool * prev;
      int nextBlockNo;
      WinBlock * list;
    };

    struct PageStats
    {
      int numResets;
      int numCallocs;
      int numFrees;
      int numHarvests;
      int lastCurrent;
    };

    struct Harvested // 16 bytes
    {
      int nextBlockNo;
      WinBlock * list [BLOCKS_PER_PAGE];
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
    // DistHash tt_root_[TT_TRICKS][DDS_HANDS][256];
    DistHash * tt_root_[TT_TRICKS][DDS_HANDS];

    // It is useful to remember the last block we looked at.
    WinBlock * last_block_seen_[TT_TRICKS][DDS_HANDS];

    // The pool of card entries for a given suit distribution.
    Pool * pool_;
    WinBlock * next_block_;
    Harvested harvested_;

    int timestamp_;
    int tt_in_use_;


  auto initTT() -> void;

  auto releaseTT() -> void;

  // Constants are provided via internal function-local static tables.

    auto hash8(const int handDist[]) const -> int;

    auto getNextCardBlock() -> WinBlock *;

    auto lookupSuit(
      DistHash * dp,
      long long key,
      bool& empty) -> WinBlock *;

    auto lookupCards(
      const WinMatch& search,
      WinBlock * bp,
      int limit,
      bool& lowerFlag) -> NodeCards *;

    auto createOrUpdate(
      WinBlock * bp,
      const WinMatch& search,
      bool flag) -> void;

    auto harvest() -> bool;

    // Debug functions from here on.

    auto keyToDist(
      long long key,
      int handDist[]) const -> void;

    auto distToLengths(
      int trick,
      const int handDist[],
      unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> void;

    auto singleLenToStr(const unsigned char length[]) const -> std::string;

    auto lenToStr(
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> std::string;

    auto makeHistStats(
      const int hist[],
      int& count,
      int& prodSum,
      int& prodSumsq,
      int& maxLen,
      int lastIndex) const -> void;

    auto calcPercentile(
      const int hist[],
      double threshold,
      int lastIndex) const -> int;

    auto printHist(
      std::ofstream& fout,
      const int hist[],
      int numWraps,
      int lastIndex) const -> void;

    auto updateSuitHist(
      int trick,
      int hand,
      int hist[],
      int& numWraps) const -> void;

    auto updateSuitHist(
      int trick,
      int hand,
      int hist[],
      int suitHist[],
      int& numWraps,
      int& suitWraps) const -> void;

    auto findMatchingDist(
      int trick,
      int hand,
      const int handDistSought[]) const -> WinBlock const *;

    auto printEntriesBlock(
      std::ofstream& fout,
      WinBlock const * bp,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> void;

    auto updateEntryHist(
      int trick,
      int hand,
      int hist[],
      int& numWraps) const -> void;

    auto updateEntryHist(
      int trick,
      int hand,
      int hist[],
      int suitHist[],
      int& numWraps,
      int& suitWraps) const -> void;

    auto effectOfBlockBound(
      const int hist[],
      int size) const -> int;

    auto printNodeValues(
      std::ofstream& fout,
      const NodeCards& node) const -> void;

    auto printMatch(
      std::ofstream& fout,
      const WinMatch& match,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const -> void;

    auto makeHolding(
      const std::string& high,
      unsigned len) const -> std::string;

    void DumpHands(
      std::ofstream& fout,
      const std::vector<std::vector<std::string>>& hands,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    void SetToPartialHands(
      const unsigned set,
      const unsigned mask,
      const int maxRank,
      const int numRanks,
      std::vector<std::vector<std::string>>& hands) const;

    int BlocksInUse() const;

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
    void init(const int hand_lookup[][15]) override;
    void set_memory_default(int megabytes) override;
    void set_memory_maximum(int megabytes) override;
    void make_tt() override;
    void reset_memory(ResetReason reason) override;
    void return_all_memory() override;
    auto memory_in_use() const -> double override;
    auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag) -> NodeCards const * override;
    void add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks_arg[],
      const NodeCards& first,
      bool flag) override;
    void print_suits(std::ofstream& fout, int trick, int hand) const override;
    void print_all_suits(std::ofstream& fout) const override;
    void print_suit_stats(std::ofstream& fout, int trick, int hand) const override;
    void print_all_suit_stats(std::ofstream& fout) const override;
    void print_summary_suit_stats(std::ofstream& fout) const override;
    void print_entries_dist(std::ofstream& fout, int trick, int hand, const int hand_dist[]) const override;
    void print_entries_dist_and_cards(std::ofstream& fout, int trick, int hand, const unsigned short aggr_target[], const int hand_dist[]) const override;
    void print_entries(std::ofstream& fout, int trick, int hand) const override;
    void print_all_entries(std::ofstream& fout) const override;
    void print_entry_stats(std::ofstream& fout, int trick, int hand) const override;
    void print_all_entry_stats(std::ofstream& fout) const override;
    void print_summary_entry_stats(std::ofstream& fout) const override;
};

#endif
