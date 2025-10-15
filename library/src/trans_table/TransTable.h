/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/*
   This is the parent class of TransTableS and TransTableL.
   Those two are different implementations.  The S version has a
   much smaller memory and a somewhat slower execution time.
*/

#ifndef DDS_TRANSTABLE_H
#define DDS_TRANSTABLE_H

#include <fstream>
#include <dds/dds.h>

// New-style types
enum class ResetReason
{
  Unknown = 0,
  TooManyNodes = 1,
  NewDeal = 2,
  NewTrump = 3,
  MemoryExhausted = 4,
  FreeMemory = 5,
  Count = 6
};

inline constexpr int kResetReasonCount = static_cast<int>(ResetReason::Count);

// Legacy-style types kept for compatibility
enum TTresetReason
{
  TT_RESET_UNKNOWN = 0,
  TT_RESET_TOO_MANY_NODES = 1,
  TT_RESET_NEW_DEAL = 2,
  TT_RESET_NEW_TRUMP = 3,
  TT_RESET_MEMORY_EXHAUSTED = 4,
  TT_RESET_FREE_MEMORY = 5,
  TT_RESET_SIZE = 6
};

// Node value/cached card data (modern name)
struct NodeCards // 8 bytes
{
  char ubound; // For N-S
  char lbound; // For N-S
  char bestMoveSuit;
  char bestMoveRank;
  char leastWin[DDS_SUITS];
};

// Legacy alias for compatibility with existing code/tests
using nodeCardsType = NodeCards;

#ifdef _MSC_VER
  // Disable warning for unused arguments.
  #pragma warning(push)
  #pragma warning(disable: 4100)
#endif

#ifdef __APPLE__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

class TransTable
{
  public:
    TransTable(){};

    virtual ~TransTable(){};

    // Legacy virtual API (preserved so existing implementations compile)
    // Implementations in TransTableS/L currently override these.
    virtual void Init(const int handLookup[][15]){};
    virtual void SetMemoryDefault(const int megabytes){};
    virtual void SetMemoryMaximum(const int megabytes){};
    virtual void MakeTT(){};
    virtual void ResetMemory(const TTresetReason /*reason*/ ){};
    virtual void ReturnAllMemory(){};
    virtual double MemoryInUse() const { return 0.; };
    virtual nodeCardsType const * Lookup(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const int handDist[],
      const int limit,
      bool& lowerFlag) { return nullptr; };
    virtual void Add(
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const unsigned short winRanksArg[],
      const nodeCardsType& first,
      const bool flag){};
    virtual void PrintSuits(
      std::ofstream& fout,
      const int trick,
      const int hand) const {};
    virtual void PrintAllSuits(std::ofstream& fout) const {};
    virtual void PrintSuitStats(
      std::ofstream& fout,
      const int trick,
      const int hand) const {};
    virtual void PrintAllSuitStats(std::ofstream& fout) const {};
    virtual void PrintSummarySuitStats(std::ofstream& fout) const {};
    virtual void PrintEntriesDist(
      std::ofstream& fout,
      const int trick,
      const int hand,
      const int handDist[]) const {};
    virtual void PrintEntriesDistAndCards(
      std::ofstream& fout,
      const int trick,
      const int hand,
      const unsigned short aggrTarget[],
      const int handDist[]) const {};
    virtual void PrintEntries(
      std::ofstream& fout,
      const int trick,
      const int hand) const {};
    virtual void PrintAllEntries(std::ofstream& fout) const {};
    virtual void PrintEntryStats(
      std::ofstream& fout,
      const int trick,
      const int hand) const {};
    virtual void PrintAllEntryStats(std::ofstream& fout) const {};
    virtual void PrintSummaryEntryStats(std::ofstream& fout) const {};

    // Modern snake_case convenience API forwarding to legacy virtuals.
    // These are non-virtual to avoid forcing re-implementation in derived classes.
    void init(const int hand_lookup[][15]) { Init(hand_lookup); }
    void set_memory_default(const int megabytes) { SetMemoryDefault(megabytes); }
    void set_memory_maximum(const int megabytes) { SetMemoryMaximum(megabytes); }
    void make_tt() { MakeTT(); }
    void reset_memory(const ResetReason reason) { ResetMemory(static_cast<TTresetReason>(static_cast<int>(reason))); }
    void return_all_memory() { ReturnAllMemory(); }
    double memory_in_use() const { return MemoryInUse(); }
    NodeCards const * lookup(
      const int trick,
      const int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      const int limit,
      bool& lower_flag) const
    {
      // const_cast to call legacy virtual
      return const_cast<TransTable*>(this)->Lookup(trick, hand, aggr_target, hand_dist, limit, lower_flag);
    }
    void add(
      const int trick,
      const int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks[],
      const NodeCards& first,
      const bool flag)
    {
      Add(trick, hand, aggr_target, win_ranks, first, flag);
    }
    void print_suits(std::ofstream& fout, const int trick, const int hand) const { PrintSuits(fout, trick, hand); }
    void print_all_suits(std::ofstream& fout) const { PrintAllSuits(fout); }
    void print_suit_stats(std::ofstream& fout, const int trick, const int hand) const { PrintSuitStats(fout, trick, hand); }
    void print_all_suit_stats(std::ofstream& fout) const { PrintAllSuitStats(fout); }
    void print_summary_suit_stats(std::ofstream& fout) const { PrintSummarySuitStats(fout); }
    void print_entries_dist(std::ofstream& fout, const int trick, const int hand, const int handDist[]) const { PrintEntriesDist(fout, trick, hand, handDist); }
    void print_entries_dist_and_cards(std::ofstream& fout, const int trick, const int hand, const unsigned short aggrTarget[], const int handDist[]) const { PrintEntriesDistAndCards(fout, trick, hand, aggrTarget, handDist); }
    void print_entries(std::ofstream& fout, const int trick, const int hand) const { PrintEntries(fout, trick, hand); }
    void print_all_entries(std::ofstream& fout) const { PrintAllEntries(fout); }
    void print_entry_stats(std::ofstream& fout, const int trick, const int hand) const { PrintEntryStats(fout, trick, hand); }
    void print_all_entry_stats(std::ofstream& fout) const { PrintAllEntryStats(fout); }
    void print_summary_entry_stats(std::ofstream& fout) const { PrintSummaryEntryStats(fout); }
    void print_page_summary(std::ofstream& /*fout*/) const {}
    void print_node_stats(std::ofstream& /*fout*/) const {}
    void print_reset_stats(std::ofstream& /*fout*/) const {}
};

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#ifdef __APPLE__
  #pragma clang diagnostic pop
#endif

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif

#endif
