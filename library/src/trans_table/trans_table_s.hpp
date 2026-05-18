/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/*
   This is an object for managing transposition tables and the
   associated memory.  Compared to TransTableL it uses a lot less
   memory and takes somewhat longer time.
*/


#include <vector>
#include <string>

#include <trans_table/trans_table.hpp>


/// \brief Memory-efficient transposition table implementation.
///
/// TransTableS provides a smaller-footprint implementation of the transposition
/// table suitable for memory-constrained environments. It uses a tree-based
/// structure with card sets and win rank sets to organize cached positions.
///
/// \par Memory Strategy
/// Uses malloc/calloc for pool-based allocation with fixed-size arrays:
/// - WinCard pool for storing rank/card information
/// - NodeCards pool for caching search results
/// - PosSearchSmall trees for position organization
///
/// Allocations throw std::bad_alloc on failure (see Task 7 modernization).
/// Non-critical allocations may use fallback strategies.
///
/// \par Performance Characteristics
/// Slower than TransTableL due to smaller cache and binary tree lookups.
/// Suitable for analysis with memory constraints (analysis.max_memory_mb < 1000).
///
/// \par Thread Safety
/// Not thread-safe. Must be accessed from a single thread.
///
/// \see TransTableL for the full-featured implementation
/// \see NodeCards for cached position data
class TransTableS: public TransTable
{
  private:

    /// \brief Card entry in the small TT with win mask information.
    struct WinCard
    {
      int order_set_;              ///< Bitmask of card orders
      int win_mask_;               ///< Bitmask of winning ranks
      NodeCards * first_;          ///< Pointer to cached result
      WinCard * prev_win_;         ///< Link in win set list
      WinCard * next_win_;         ///< Link in win set list
      WinCard * next_;             ///< Link in position search tree
    };

    /// \brief Tree node for binary search on card distributions.
    struct PosSearchSmall
    {
      WinCard * pos_search_point_; ///< Associated card entry
      long long suit_lengths_;     ///< Card length distribution key
      PosSearchSmall * left_;      ///< Left subtree
      PosSearchSmall * right_;     ///< Right subtree
    };

    /// \brief Aggregated targets and win masks for a given hand.
    struct TtAggr
    {
      int aggr_ranks_[DDS_SUITS];  ///< Target tricks per suit
      int win_mask_[DDS_SUITS];    ///< Win mask per suit
    };

    /// \brief Statistics about table resets.
    struct StatsResets
    {
      int no_of_resets_;           ///< Total number of resets
      int aggr_resets_[ResetReasonCount]; ///< Reset counts by reason
    };


    long long aggr_len_sets_[14];
    StatsResets stats_resets_;

    WinCard temp_win_[5];
    int node_set_size_limit_;
    int win_set_size_limit_;
    unsigned long long maxmem_;
    unsigned long long allocmem_;
    unsigned long long summem_;
    int wmem_;
    int nmem_;
    int max_index_;
    int wcount_;
    int ncount_;
    bool clear_tt_flag_;
    int windex_;
    TtAggr * aggp_;

    PosSearchSmall * rootnp_[14][DDS_HANDS];
    WinCard ** pw_;
    NodeCards ** pn_;
    PosSearchSmall ** pl_[14][DDS_HANDS];
    NodeCards * node_cards_;
    WinCard * win_cards_;
    PosSearchSmall * pos_search_[14][DDS_HANDS];
    int node_set_size_; /* Index with range 0 to node_set_size_limit_ */
    int win_set_size_;  /* Index with range 0 to win_set_size_limit_ */
    int len_set_ind_[14][DDS_HANDS];
    int lcount_[14][DDS_HANDS];

    std::vector<std::string> reset_text_;

    long long suit_lengths_[14];

    int tt_in_use_;

    // Constants are provided via internal function-local static tables.

    auto wipe() -> void;

    auto init_tt() -> void;

    auto add_win_set() -> void;

    auto add_node_set() -> void;

    auto add_len_set(
      int trick, 
      int first_hand) -> void;

    auto build_sop(
      const unsigned short our_win_ranks[DDS_SUITS],
      const unsigned short aggr_arg[DDS_SUITS],
      const NodeCards& first,
      long long lengths,
      int tricks,
      int first_hand,
      bool flag
    ) -> void;

    auto build_path(
      const int win_mask[],
      const int win_order_set[],
      int u_bound,
      int l_bound,
      char best_move_suit,
      char best_move_rank,
      PosSearchSmall * node_ptr,
      bool& result
    ) -> NodeCards *;

    auto search_len_and_insert(
      PosSearchSmall * root_ptr,
      long long key,
      bool insert_node,
      int trick,
      int first_hand,
      bool& result
    ) -> PosSearchSmall *;

    auto update_sop(
      int u_bound,
      int l_bound,
      char best_move_suit,
      char best_move_rank,
      NodeCards * node
    ) -> NodeCards *;

    auto find_sop(
      const int order_set[],
      int limit,
      WinCard * node_p,
      bool& lower_flag
    ) -> NodeCards const *;

  public:

    /// \brief Construct a small transposition table instance.
    ///
    /// Initializes the small TT with default memory limits. Must call
    /// make_tt() to actually allocate memory.
    TransTableS();

    /// \brief Destroy the small transposition table.
    ///
    /// Releases all allocated memory and internal structures.
    ~TransTableS();

    /// \brief Initialize the transposition table with hand lookup tables.
    ///
    /// Sets up the TT with hand lookup configuration for position hashing.
    ///
    /// \param hand_lookup Array of hand lookup tables [DDS_SUITS][15]
    /// \throws std::bad_alloc if initialization fails
    auto init(const int hand_lookup[][15]) -> void override;

    /// \brief Set the default (soft) memory limit (no-op for TransTableS).
    ///
    /// For the small transposition table implementation, this function is
    /// intentionally a no-op and exists only to satisfy the TransTable
    /// interface. TransTableS does not track or enforce a separate soft
    /// memory limit; only the hard limit set via set_memory_maximum() is
    /// honored.
    ///
    /// \param megabytes Ignored for TransTableS
    auto set_memory_default(int megabytes) -> void override;

    /// \brief Set the maximum (hard) memory limit.
    ///
    /// \param megabytes Maximum allowed memory in MB
    auto set_memory_maximum(int megabytes) -> void override;

    /// \brief Allocate transposition table memory structures.
    ///
    /// \throws std::bad_alloc if memory allocation fails
    auto make_tt() -> void override;

    /// \brief Clear cached entries and reset statistics.
    ///
    /// \param reason Why the reset is occurring
    auto reset_memory(ResetReason reason) -> void override;

    /// \brief Deallocate all transposition table memory.
    ///
    /// After calling this, must call make_tt() before further lookups.
    auto return_all_memory() -> void override;

    /// \brief Return current memory usage in kilobytes.
    ///
    /// \return Memory in use (KB)
    auto memory_in_use() const -> double override;

    /// \brief Lookup a cached position result.
    ///
    /// Searches for previously cached analysis of the given position.
    /// Returns nullptr if not found.
    ///
    /// \param trick Current trick (0-12)
    /// \param hand Hand to play (0-3)
    /// \param aggr_target Target tricks per suit
    /// \param hand_dist Card distribution
    /// \param limit Early termination threshold
    /// \param[out] lower_flag Set to true if result is a lower bound
    /// \return Cached result or nullptr
    auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag
    ) -> NodeCards const * override;

    /// \brief Add a computed result to the transposition table.
    ///
    /// Caches a newly computed search result for later lookup.
    ///
    /// \param trick Current trick (0-12)
    /// \param hand Hand to play (0-3)
    /// \param aggr_target Target tricks per suit
    /// \param win_ranks_arg Winning ranks (optimization data)
    /// \param first Computed result to cache
    /// \param flag True if this is a lower bound (incomplete search)
    auto add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks_arg[],
      const NodeCards& first,
      bool flag
    ) -> void override;

    /// \brief No-op print implementation for small TT.
    ///
    /// The small transposition table does not support detailed dumping.
    /// These methods are no-op implementations of the base class interface.
    auto print_suits(
      std::ofstream& /*fout*/,
      int /*trick*/,
      int /*hand*/) const -> void override
    {
    }
    auto print_all_suits(std::ofstream& /*fout*/) const -> void override
    {
    }
    auto print_suit_stats(
      std::ofstream& /*fout*/,
      int /*trick*/,
      int /*hand*/) const -> void override
    {
    }
    auto print_all_suit_stats(std::ofstream& /*fout*/) const -> void override
    {
    }
    auto print_summary_suit_stats(std::ofstream& /*fout*/) const -> void override
    {
    }
    auto print_entries_dist(
      std::ofstream& /*fout*/,
      int /*trick*/,
      int /*hand*/,
      const int /*hand_dist*/[]) const -> void override
    {
    }
    auto print_entries_dist_and_cards(
      std::ofstream& /*fout*/,
      int /*trick*/,
      int /*hand*/,
      const unsigned short /*aggr_target*/[],
      const int /*hand_dist*/[]) const -> void override
    {
    }
    auto print_entries(
      std::ofstream& /*fout*/,
      int /*trick*/,
      int /*hand*/) const -> void override
    {
    }
    auto print_all_entries(std::ofstream& /*fout*/) const -> void override
    {
    }
    auto print_entry_stats(
      std::ofstream& /*fout*/,
      int /*trick*/,
      int /*hand*/) const -> void override
    {
    }
    auto print_all_entry_stats(std::ofstream& /*fout*/) const -> void override
    {
    }
    auto print_summary_entry_stats(std::ofstream& /*fout*/) const -> void override
    {
    }

    /// \brief Bridge to node statistics printer.
    ///
    /// Delegates to print_node_stats_impl() implementation.
    auto print_node_stats(std::ofstream& fout) const -> void override
    {
      print_node_stats_impl(fout);
    }

    /// \brief Bridge to reset statistics printer.
    ///
    /// Delegates to print_reset_stats_impl() implementation.
    auto print_reset_stats(std::ofstream& fout) const -> void override
    {
      print_reset_stats_impl(fout);
    }

    /// \brief Print node statistics from the small TT.
    ///
    /// Outputs statistics about the cached nodes and memory usage.
    ///
    /// \param fout Output stream for statistics
    auto print_node_stats_impl(std::ofstream& fout) const -> void;

    /// \brief Print reset statistics from the small TT.
    ///
    /// Outputs statistics about when and why the table was reset.
    ///
    /// \param fout Output stream for statistics
    auto print_reset_stats_impl(std::ofstream& fout) const -> void;
};
