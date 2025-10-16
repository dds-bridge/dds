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

// Reset reason for table memory and counters
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

// Legacy-style enum kept for compatibility wrappers
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

// Node value/cached card data
struct NodeCards // 8 bytes
{
  char ubound; // For N-S
  char lbound; // For N-S
  char bestMoveSuit;
  char bestMoveRank;
  char leastWin[DDS_SUITS];
};

// Legacy alias used widely in code/tests
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
    TransTable() = default;

    virtual ~TransTable() = default;

    // Pure-virtual modern API
    virtual void init(const int hand_lookup[][15]) = 0;
    virtual void set_memory_default(int megabytes) = 0;
    virtual void set_memory_maximum(int megabytes) = 0;
    virtual void make_tt() = 0;
    virtual void reset_memory(ResetReason reason) = 0;
    virtual void return_all_memory() = 0;
    virtual auto memory_in_use() const -> double = 0;
    virtual auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag) -> NodeCards const * = 0;
    virtual void add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks[],
      const NodeCards& first,
      bool flag) = 0;
    virtual void print_suits(std::ofstream& fout, int trick, int hand) const = 0;
    virtual void print_all_suits(std::ofstream& fout) const = 0;
    virtual void print_suit_stats(std::ofstream& fout, int trick, int hand) const = 0;
    virtual void print_all_suit_stats(std::ofstream& fout) const = 0;
    virtual void print_summary_suit_stats(std::ofstream& fout) const = 0;
    virtual void print_entries_dist(std::ofstream& fout, int trick, int hand, const int hand_dist[]) const = 0;
    virtual void print_entries_dist_and_cards(std::ofstream& fout, int trick, int hand, const unsigned short aggr_target[], const int hand_dist[]) const = 0;
    virtual void print_entries(std::ofstream& fout, int trick, int hand) const = 0;
    virtual void print_all_entries(std::ofstream& fout) const = 0;
    virtual void print_entry_stats(std::ofstream& fout, int trick, int hand) const = 0;
    virtual void print_all_entry_stats(std::ofstream& fout) const = 0;
    virtual void print_summary_entry_stats(std::ofstream& fout) const = 0;
    virtual void print_page_summary(std::ofstream& /*fout*/) const {}
    virtual void print_node_stats(std::ofstream& /*fout*/) const {}
    virtual void print_reset_stats(std::ofstream& /*fout*/) const {}

    // Legacy wrapper API (non-virtual) maintained for existing callers
    void Init(const int hand_lookup[][15]);
    void SetMemoryDefault(const int megabytes);
    void SetMemoryMaximum(const int megabytes);
    void MakeTT();
    void ResetMemory(const TTresetReason reason);
    void ReturnAllMemory();
    double MemoryInUse() const;
    nodeCardsType const * Lookup(
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
      const nodeCardsType& first,
      const bool flag);
    void PrintSuits(std::ofstream& fout, const int trick, const int hand) const;
    void PrintAllSuits(std::ofstream& fout) const;
    void PrintSuitStats(std::ofstream& fout, const int trick, const int hand) const;
    void PrintAllSuitStats(std::ofstream& fout) const;
    void PrintSummarySuitStats(std::ofstream& fout) const;
    void PrintEntriesDist(std::ofstream& fout, const int trick, const int hand, const int handDist[]) const;
    void PrintEntriesDistAndCards(std::ofstream& fout, const int trick, const int hand, const unsigned short aggrTarget[], const int handDist[]) const;
    void PrintEntries(std::ofstream& fout, const int trick, const int hand) const;
    void PrintAllEntries(std::ofstream& fout) const;
    void PrintEntryStats(std::ofstream& fout, const int trick, const int hand) const;
    void PrintAllEntryStats(std::ofstream& fout) const;
    void PrintSummaryEntryStats(std::ofstream& fout) const;
};

// Legacy non-virtual wrappers to ease migration
inline void TransTable::Init(const int hand_lookup[][15])
{
  init(hand_lookup);
}

inline void TransTable::SetMemoryDefault(const int megabytes)
{
  set_memory_default(megabytes);
}

inline void TransTable::SetMemoryMaximum(const int megabytes)
{
  set_memory_maximum(megabytes);
}

inline void TransTable::MakeTT()
{
  make_tt();
}

inline void TransTable::ResetMemory(const TTresetReason reason)
{
  reset_memory(static_cast<ResetReason>(static_cast<int>(reason)));
}

inline void TransTable::ReturnAllMemory()
{
  return_all_memory();
}

inline double TransTable::MemoryInUse() const
{
  return memory_in_use();
}

inline nodeCardsType const * TransTable::Lookup(
  const int trick,
  const int hand,
  const unsigned short aggrTarget[],
  const int handDist[],
  const int limit,
  bool& lowerFlag)
{
  return lookup(trick, hand, aggrTarget, handDist, limit, lowerFlag);
}

inline void TransTable::Add(
  const int trick,
  const int hand,
  const unsigned short aggrTarget[],
  const unsigned short winRanksArg[],
  const nodeCardsType& first,
  const bool flag)
{
  add(trick, hand, aggrTarget, winRanksArg, first, flag);
}

inline void TransTable::PrintSuits(std::ofstream& fout, const int trick, const int hand) const
{
  print_suits(fout, trick, hand);
}

inline void TransTable::PrintAllSuits(std::ofstream& fout) const
{
  print_all_suits(fout);
}

inline void TransTable::PrintSuitStats(std::ofstream& fout, const int trick, const int hand) const
{
  print_suit_stats(fout, trick, hand);
}

inline void TransTable::PrintAllSuitStats(std::ofstream& fout) const
{
  print_all_suit_stats(fout);
}

inline void TransTable::PrintSummarySuitStats(std::ofstream& fout) const
{
  print_summary_suit_stats(fout);
}

inline void TransTable::PrintEntriesDist(std::ofstream& fout, const int trick, const int hand, const int handDist[]) const
{
  print_entries_dist(fout, trick, hand, handDist);
}

inline void TransTable::PrintEntriesDistAndCards(std::ofstream& fout, const int trick, const int hand, const unsigned short aggrTarget[], const int handDist[]) const
{
  print_entries_dist_and_cards(fout, trick, hand, aggrTarget, handDist);
}

inline void TransTable::PrintEntries(std::ofstream& fout, const int trick, const int hand) const
{
  print_entries(fout, trick, hand);
}

inline void TransTable::PrintAllEntries(std::ofstream& fout) const
{
  print_all_entries(fout);
}

inline void TransTable::PrintEntryStats(std::ofstream& fout, const int trick, const int hand) const
{
  print_entry_stats(fout, trick, hand);
}

inline void TransTable::PrintAllEntryStats(std::ofstream& fout) const
{
  print_all_entry_stats(fout);
}

inline void TransTable::PrintSummaryEntryStats(std::ofstream& fout) const
{
  print_summary_entry_stats(fout);
}

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
