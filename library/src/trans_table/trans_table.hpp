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

#pragma once

#include <fstream>
#include <api/dll.h>

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

inline constexpr int ResetReasonCount = static_cast<int>(ResetReason::Count);

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
    virtual auto init(const int hand_lookup[][15]) -> void = 0;
    virtual auto set_memory_default(int megabytes) -> void = 0;
    virtual auto set_memory_maximum(int megabytes) -> void = 0;
    virtual auto make_tt() -> void = 0;
    virtual auto reset_memory(ResetReason reason) -> void = 0;
    virtual auto return_all_memory() -> void = 0;
    virtual auto memory_in_use() const -> double = 0;
    virtual auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag) -> NodeCards const * = 0;
    virtual auto add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks[],
      const NodeCards& first,
      bool flag) -> void = 0;
    virtual auto print_suits(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;
    virtual auto print_all_suits(std::ofstream& fout) const -> void = 0;
    virtual auto print_suit_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;
    virtual auto print_all_suit_stats(std::ofstream& fout) const -> void = 0;
    virtual auto print_summary_suit_stats(std::ofstream& fout) const -> void = 0;
    virtual auto print_entries_dist(
      std::ofstream& fout,
      int trick,
      int hand,
      const int hand_dist[]) const -> void = 0;
    virtual auto print_entries_dist_and_cards(
      std::ofstream& fout,
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[]) const -> void = 0;
    virtual auto print_entries(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;
    virtual auto print_all_entries(std::ofstream& fout) const -> void = 0;
    virtual auto print_entry_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;
    virtual auto print_all_entry_stats(std::ofstream& fout) const -> void = 0;
    virtual auto print_summary_entry_stats(std::ofstream& fout) const -> void = 0;
    virtual auto print_page_summary(std::ofstream& /*fout*/) const -> void
    {
    }
    virtual auto print_node_stats(std::ofstream& /*fout*/) const -> void
    {
    }
    virtual auto print_reset_stats(std::ofstream& /*fout*/) const -> void
    {
    }
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
