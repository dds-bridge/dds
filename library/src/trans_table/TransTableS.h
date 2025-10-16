/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TRANSTABLES_H
#define DDS_TRANSTABLES_H

/*
   This is an object for managing transposition tables and the
   associated memory.  Compared to TransTableL it uses a lot less
   memory and takes somewhat longer time.
*/


#include <vector>
#include <string>

#include "TransTable.h"


class TransTableS: public TransTable
{
  private:

    // Structures for the small memory option.

    struct winCardType
    {
      int orderSet;
      int winMask;
      NodeCards * first;
      winCardType * prevWin;
      winCardType * nextWin;
      winCardType * next;
    };

    struct posSearchTypeSmall
    {
      winCardType * posSearchPoint;
      long long suitLengths;
      posSearchTypeSmall * left;
      posSearchTypeSmall * right;
    };

    struct ttAggrType
    {
      int aggrRanks[DDS_SUITS];
      int winMask[DDS_SUITS];
    };

    struct statsResetsType
    {
      int noOfResets;
      int aggrResets[kResetReasonCount];
    };


    long long aggrLenSets[14];
    statsResetsType statsResets;

    winCardType temp_win[5];
    int nodeSetSizeLimit;
    int winSetSizeLimit;
    unsigned long long maxmem;
    unsigned long long allocmem;
    unsigned long long summem;
    int wmem;
    int nmem;
    int maxIndex;
    int wcount;
    int ncount;
    bool clearTTflag;
    int windex;
    ttAggrType * aggp;

    posSearchTypeSmall * rootnp[14][DDS_HANDS];
    winCardType ** pw;
  NodeCards ** pn;
    posSearchTypeSmall ** pl[14][DDS_HANDS];
  NodeCards * nodeCards;
    winCardType * winCards;
    posSearchTypeSmall * posSearch[14][DDS_HANDS];
    int nodeSetSize; /* Index with range 0 to nodeSetSizeLimit */
    int winSetSize;  /* Index with range 0 to winSetSizeLimit */
    int lenSetInd[14][DDS_HANDS];
    int lcount[14][DDS_HANDS];

  std::vector<std::string> resetText;

    long long suitLengths[14];

    int TTInUse;

  // Constants are provided via internal function-local static tables.

    void Wipe();

    void InitTT();

    void AddWinSet();

    void AddNodeSet();

    void AddLenSet(
      const int trick, 
      const int firstHand);

    void BuildSOP(
  const unsigned short ourWinRanks[DDS_SUITS],
  const unsigned short aggr[DDS_SUITS],
  const NodeCards& first,
  const long long suitLengths,
      const int tricks,
      const int firstHand,
      const bool flag);

    NodeCards * BuildPath(
      const int winMask[],
      const int winOrderSet[],
      const int ubound,
      const int lbound,
      const char bestMoveSuit,
      const char bestMoveRank,
      posSearchTypeSmall * node,
      bool& result);

    struct posSearchTypeSmall * SearchLenAndInsert(
      posSearchTypeSmall * rootp,
      const long long key,
      const bool insertNode,
      const int trick,
      const int firstHand,
      bool& result);

    NodeCards * UpdateSOP(
      const int ubound,
      const int lbound,
      const char bestMoveSuit,
      const char bestMoveRank,
      NodeCards * nodep);

    NodeCards const * FindSOP(
      const int orderSet[],
      const int limit,
      winCardType * nodeP,
      bool& lowerFlag);

    // Legacy implementation helpers (kept private)
    void Init(const int handLookup[][15]);
    void SetMemoryDefault(const int megabytes);
    void SetMemoryMaximum(const int megabytes);
    void MakeTT();
    void ResetMemory(const ResetReason reason);
    void ReturnAllMemory();
    double MemoryInUse() const;
    NodeCards const * Lookup(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const int handDist[],
      const int limit,
      bool& lowerFlag);
    void Add(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const unsigned short winRanksArg[],
      const NodeCards& first,
      const bool flag);

  public:

    TransTableS();

    ~TransTableS();

    // Modern overrides implemented as inline wrappers
    void init(const int hand_lookup[][15]) override { Init(hand_lookup); }
    void set_memory_default(int megabytes) override { SetMemoryDefault(megabytes); }
    void set_memory_maximum(int megabytes) override { SetMemoryMaximum(megabytes); }
    void make_tt() override { MakeTT(); }
    void reset_memory(ResetReason reason) override { ResetMemory(reason); }
    void return_all_memory() override { ReturnAllMemory(); }
    auto memory_in_use() const -> double override { return MemoryInUse(); }
    auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag) -> NodeCards const * override { return Lookup(trick, hand, aggr_target, hand_dist, limit, lower_flag); }
    void add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks_arg[],
      const NodeCards& first,
      bool flag) override { Add(trick, hand, aggr_target, win_ranks_arg, first, flag); }

    // The small TT does not provide verbose dumping; implement no-op printers
    void print_suits(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
    void print_all_suits(std::ofstream& /*fout*/) const override {}
    void print_suit_stats(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
    void print_all_suit_stats(std::ofstream& /*fout*/) const override {}
    void print_summary_suit_stats(std::ofstream& /*fout*/) const override {}
    void print_entries_dist(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/, const int /*hand_dist*/[]) const override {}
    void print_entries_dist_and_cards(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/, const unsigned short /*aggr_target*/[], const int /*hand_dist*/[]) const override {}
    void print_entries(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
    void print_all_entries(std::ofstream& /*fout*/) const override {}
    void print_entry_stats(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
    void print_all_entry_stats(std::ofstream& /*fout*/) const override {}
    void print_summary_entry_stats(std::ofstream& /*fout*/) const override {}

    // Bridge stats printers to existing small-TT implementations
    void print_node_stats(std::ofstream& fout) const override { PrintNodeStats(fout); }
    void print_reset_stats(std::ofstream& fout) const override { PrintResetStats(fout); }

  void PrintNodeStats(std::ofstream& fout) const;

  void PrintResetStats(std::ofstream& fout) const;
};

#endif
