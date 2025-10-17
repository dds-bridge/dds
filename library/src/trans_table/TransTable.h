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

// Node value/cached card data
struct NodeCards // 8 bytes
{
  char upper_bound; // For N-S
  char lower_bound; // For N-S
  char best_move_suit;
  char best_move_rank;
  char least_win[DDS_SUITS];
};

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
