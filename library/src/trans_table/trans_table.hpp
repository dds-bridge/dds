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

/// \brief Enumeration of reasons that triggered a transposition table memory reset.
///
/// These values are used to track and report why the transposition table's
/// memory and counters were cleared. Understanding reset reasons helps identify
/// performance bottlenecks and memory usage patterns.
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

/// \brief Cached search result for a particular position.
///
/// This structure holds the cached values computed during double dummy analysis
/// for a particular node/trick combination. It stores bounds on the number of
/// tricks the side to move at this node can make (upper and lower bounds), the
/// best move to play, and per-suit lowest winning rank encodings for move
/// ordering.
///
/// Size: 8 bytes (tightly packed to minimize memory footprint in large tables)
///
/// \note The bounds may be exact values or conservative estimates depending on
///       the search depth and the algorithm's progress.
/// \see ResetReason for memory management related to this structure
struct NodeCards // 8 bytes
{
  char upper_bound;     ///< Maximum tricks for side to move at this node (0-13)
  char lower_bound;     ///< Minimum tricks for side to move at this node (0-13)
  char best_move_suit;  ///< Optimal suit index (0=S, 1=H, 2=D, 3=C; matches card_suit)
  char best_move_rank;  ///< Absolute rank (2-14 for 2-A), 0 used as sentinel
  char least_win[DDS_SUITS]; ///< Encoded lowest winning rank (0-13), used as 15 - least_win
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

/// \brief Abstract base class for bridge double dummy search transposition table.
///
/// TransTable defines the interface for managing cached positions during
/// double dummy analysis. The transposition table stores previously computed
/// results to avoid redundant search work. Two implementations are provided:
/// - TransTableS: Memory-efficient small transposition table
/// - TransTableL: Full-featured large transposition table with paging
///
/// \par Memory Management Strategy
/// Implementations use different memory strategies. TransTableS uses a pool-based
/// approach with malloc/calloc, while TransTableL uses paged memory with
/// harvesting. Both support configurable memory limits and graceful degradation.
///
/// \par Thread Safety
/// Not thread-safe. The transposition table must be accessed from a single
/// solver thread. Use synchronization if accessed from multiple threads.
///
/// \par Exception Safety
/// Since Task 7 modernization: Memory allocation failures throw std::bad_alloc.
/// Non-critical allocations may still use fallback strategies.
///
/// \see ResetReason for tracking why the table is reset
/// \see NodeCards for the cached position data
class TransTable
{
  public:
    TransTable() = default;

    virtual ~TransTable() = default;

    /// \brief Initialize the transposition table with hand lookup configuration.
    ///
    /// Sets up the transposition table with the provided hand lookup tables
    /// used for efficient position hashing and lookups.
    ///
    /// \param hand_lookup Array of hand lookup tables with shape [DDS_SUITS][15]
    /// \throws std::bad_alloc if memory allocation fails during initialization
    /// \pre hand_lookup must be a valid [DDS_SUITS][15] array
    virtual auto init(const int hand_lookup[][15]) -> void = 0;

    /// \brief Set the default (soft) memory limit in megabytes.
    ///
    /// The table will try to stay below this limit but may exceed it slightly
    /// during search. When the limit is exceeded, the table may invoke cleanup
    /// strategies like harvesting (in TransTableL).
    ///
    /// \param megabytes Desired soft memory limit in MB (0 = unlimited)
    virtual auto set_memory_default(int megabytes) -> void = 0;

    /// \brief Set the maximum (hard) memory limit in megabytes.
    ///
    /// The table will refuse allocations that would exceed this limit.
    /// A memory reset is triggered if the hard limit would be exceeded.
    ///
    /// \param megabytes Maximum allowed memory in MB
    virtual auto set_memory_maximum(int megabytes) -> void = 0;

    /// \brief Create/allocate the transposition table structures.
    ///
    /// Allocates memory for the transposition table according to configured
    /// limits. Must be called before any lookup/add operations.
    ///
    /// \throws std::bad_alloc if critical allocation fails
    virtual auto make_tt() -> void = 0;

    /// \brief Clear the transposition table and reset memory/statistics.
    ///
    /// Removes all cached positions and resets internal statistics. The memory
    /// structures are retained for reuse.
    ///
    /// \param reason The reason this reset was triggered
    virtual auto reset_memory(ResetReason reason) -> void = 0;

    /// \brief Release all memory used by the transposition table.
    ///
    /// Deallocates all structures. The table must be re-initialized with
    /// make_tt() before further use.
    virtual auto return_all_memory() -> void = 0;

    /// \brief Return the amount of memory currently in use (in KB).
    ///
    /// \return Memory usage in kilobytes
    virtual auto memory_in_use() const -> double = 0;

    /// \brief Lookup a cached result for a position.
    ///
    /// Searches the transposition table for previously cached analysis results
    /// for the given position parameters. Returns nullptr if not found.
    ///
    /// \param trick Current trick number (0-12)
    /// \param hand Current hand to play (0-3)
    /// \param aggr_target Aggregated targets per suit (4 values, one per suit)
    /// \param hand_dist Card distribution for each hand (4 values)
    /// \param limit Threshold for early termination, interpreted for the
    ///              side to move at this node
    /// \param[out] lower_flag Set to true if result is a lower bound
    /// \return Pointer to cached NodeCards if found; nullptr otherwise
    /// \note The returned pointer is only valid until the next add() or reset
    virtual auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag) -> NodeCards const * = 0;

    /// \brief Add a newly computed result to the transposition table.
    ///
    /// Caches the result of a double dummy search for later lookup. If a
    /// conflicting entry exists, it may be updated or replaced.
    ///
    /// \param trick Current trick number (0-12)
    /// \param hand Current hand to play (0-3)
    /// \param aggr_target Aggregated targets per suit
    /// \param win_ranks Winning rank for each suit (optimization data)
    /// \param first The cached NodeCards result to store
    /// \param flag True if entry is a lower bound (incomplete search)
    virtual auto add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks[],
      const NodeCards& first,
      bool flag) -> void = 0;

    /// \brief Print cached results for a specific suit and position.
    ///
    /// Outputs detailed statistics about the cached entries for a given
    /// trick/hand combination. Output format varies by implementation.
    ///
    /// \param fout Output stream for the statistics
    /// \param trick Trick number (0-12)
    /// \param hand Hand to analyze (0-3)
    virtual auto print_suits(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;

    /// \brief Print suits statistics for all tricks and hands.
    virtual auto print_all_suits(std::ofstream& fout) const -> void = 0;

    /// \brief Print suit statistics for a specific trick/hand.
    virtual auto print_suit_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;

    /// \brief Print suit statistics for all tricks and hands.
    virtual auto print_all_suit_stats(std::ofstream& fout) const -> void = 0;

    /// \brief Print summary suit statistics.
    virtual auto print_summary_suit_stats(std::ofstream& fout) const -> void = 0;

    /// \brief Print entries distribution for a specific hand.
    virtual auto print_entries_dist(
      std::ofstream& fout,
      int trick,
      int hand,
      const int hand_dist[]) const -> void = 0;

    /// \brief Print entries distribution with card information.
    virtual auto print_entries_dist_and_cards(
      std::ofstream& fout,
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[]) const -> void = 0;

    /// \brief Print entries for a specific trick/hand.
    virtual auto print_entries(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;

    /// \brief Print all cached entries.
    virtual auto print_all_entries(std::ofstream& fout) const -> void = 0;

    /// \brief Print entry statistics for a specific trick/hand.
    virtual auto print_entry_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void = 0;

    /// \brief Print entry statistics for all tricks and hands.
    virtual auto print_all_entry_stats(std::ofstream& fout) const -> void = 0;

    /// \brief Print summary entry statistics.
    virtual auto print_summary_entry_stats(std::ofstream& fout) const -> void = 0;

    /// \brief Print page summary (implementation-specific, optional).
    virtual auto print_page_summary(std::ofstream& /*fout*/) const -> void
    {
    }

    /// \brief Print node statistics (implementation-specific, optional).
    virtual auto print_node_stats(std::ofstream& /*fout*/) const -> void
    {
    }

    /// \brief Print memory reset statistics (implementation-specific, optional).
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
