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

    struct winMatchType // 52 bytes
    {
      unsigned xorSet;
      unsigned topSet1 , topSet2 , topSet3 , topSet4 ;
      unsigned topMask1, topMask2, topMask3, topMask4;
      int maskIndex;
      int lastMaskNo;
      NodeCards first;
    };

    struct winBlockType // 6508 bytes when BLOCKS_PER_ENTRY == 125
    {
      int nextMatchNo;
      int nextWriteNo;
      int timestampRead;
      winMatchType list[BLOCKS_PER_ENTRY];
    };

    struct posSearchType // 16 bytes (inefficiency, 12 bytes enough)
    {
      winBlockType * posBlock;
      long long key;
    };

    struct distHashType // 520 bytes when DISTS_PER_ENTRY == 32
    {
      int nextNo;
      int nextWriteNo;
      posSearchType list[DISTS_PER_ENTRY];
    };

    struct aggrType // 80 bytes
    {
      unsigned aggrRanks[DDS_SUITS];
      unsigned aggrBytes[DDS_SUITS][TT_BYTES];
    };

    struct poolType // 16 bytes
    {
      poolType * next;
      poolType * prev;
      int nextBlockNo;
      winBlockType * list;
    };

    struct pageStatsType
    {
      int numResets;
      int numCallocs;
      int numFrees;
      int numHarvests;
      int lastCurrent;
    };

    struct harvestedType // 16 bytes
    {
      int nextBlockNo;
      winBlockType * list [BLOCKS_PER_PAGE];
    };

    enum memStateType
    {
      FROM_POOL,
      FROM_HARVEST
    };

    // Private data for the full memory version.
    memStateType mem_state_;

    int pages_default_;
    int pages_current_;
    int pages_maximum_;

    int harvest_trick_;
    int harvest_hand_;

    pageStatsType page_stats_;

    // aggr is constant for a given hand.
    aggrType aggr_[8192]; // 64 KB

    // This is the real transposition table.
    // The last index is the hash.
    // 6240 KB with above assumptions
    // distHashType tt_root_[TT_TRICKS][DDS_HANDS][256];
    distHashType * tt_root_[TT_TRICKS][DDS_HANDS];

    // It is useful to remember the last block we looked at.
    winBlockType * last_block_seen_[TT_TRICKS][DDS_HANDS];

    // The pool of card entries for a given suit distribution.
    poolType * pool_;
    winBlockType * next_block_;
    harvestedType harvested_;

    int timestamp_;
    int tt_in_use_;


    void InitTT();

    void ReleaseTT();

  // Constants are provided via internal function-local static tables.

    int hash8(const int handDist[]) const;

    winBlockType * GetNextCardBlock();

    winBlockType * LookupSuit(
      distHashType * dp,
      const long long key,
      bool& empty);

    NodeCards * LookupCards(
      const winMatchType& search,
      winBlockType * bp,
      const int limit,
      bool& lowerFlag);

    void CreateOrUpdate(
      winBlockType * bp,
      const winMatchType& search,
      const bool flag);

    bool Harvest();

    // Debug functions from here on.

    void KeyToDist(
      const long long key,
      int handDist[]) const;

    void DistToLengths(
      const int trick,
      const int handDist[],
      unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    std::string SingleLenToStr(const unsigned char length[]) const;

    std::string LenToStr(
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    void MakeHistStats(
      const int hist[],
      int& count,
      int& prod_sum,
      int& prod_sumsq,
      int& max_len,
      const int last_index) const;

    int CalcPercentile(
      const int hist[],
      const double threshold,
      const int last_index) const;

    void PrintHist(
      std::ofstream& fout,
      const int hist[],
      const int num_wraps,
      const int last_index) const;

    void UpdateSuitHist(
      const int trick,
      const int hand,
      int hist[],
      int& num_wraps) const;

    void UpdateSuitHist(
      const int trick,
      const int hand,
      int hist[],
      int suitHist[],
      int& num_wraps,
      int& suitWraps) const;

    winBlockType const * FindMatchingDist(
      const int trick,
      const int hand,
      const int handDistSought[]) const;

    void PrintEntriesBlock(
      std::ofstream& fout,
      winBlockType const * bp,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    void UpdateEntryHist(
      const int trick,
      const int hand,
      int hist[],
      int& num_wraps) const;

    void UpdateEntryHist(
      const int trick,
      const int hand,
      int hist[],
      int suitHist[],
      int& num_wraps,
      int& suitWraps) const;

    int EffectOfBlockBound(
      const int hist[],
      const int size) const;

    void PrintNodeValues(
      std::ofstream& fout,
  const NodeCards& np) const;

    void PrintMatch(
      std::ofstream& fout,
      const winMatchType& wp,
      const unsigned char lengths[DDS_HANDS][DDS_SUITS]) const;

    std::string MakeHolding(
      const std::string& high,
      const unsigned len) const;

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
