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

#include <trans_table/trans_table.hpp>


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
  TtTricks = 12
};

inline constexpr double TtPercentile = 0.9;


/// \brief Full-featured transposition table implementation with paging.
///
/// TransTableL provides a high-performance transposition table suitable for
/// analysis with substantial memory available. It uses a paged architecture
/// with memory pooling and harvesting strategies for efficient memory management.
///
/// \par Architecture
/// - Pages: Dynamically allocated pools of memory blocks (default 15, max 25 pages)
/// - Blocks: 1000 blocks per page, each block holds multiple WinMatch entries
/// - Hash table: 3D array indexed by trick, hand, and suit distribution hash
/// - Harvesting: Automatic cleanup of old entries when memory limit approached
///
/// \par Memory Strategy
/// Initial allocations throw std::bad_alloc (see Task 7 modernization).
/// The harvesting system provides graceful degradation without hard stops:
/// - Entries marked with age for LRU eviction
/// - Harvest triggered at 90th percentile of memory usage
/// - Fallback to reuse strategies when harvest needed
///
/// \par Performance Characteristics
/// Significantly faster than TransTableS due to larger cache and direct hashing.
/// Suitable for analysis with ample memory (analysis.max_memory_mb >= 1000).
///
/// \par Thread Safety
/// Not thread-safe. Must be accessed from a single thread.
///
/// \par Usage Example
/// \code
/// TransTableL tt;
/// tt.init(hand_lookup);
/// tt.set_memory_default(2000);  // 2 GB soft limit
/// tt.set_memory_maximum(4000);  // 4 GB hard limit
/// tt.make_tt();
/// // ... use with lookup/add during search ...
/// NodeCards const * result = tt.lookup(...);
/// if (result) { /* use cached result */ }
/// else { /* compute and add result */ }
/// tt.return_all_memory();
/// \endcode
///
/// \see TransTableS for the memory-efficient implementation
/// \see NodeCards for cached position data
class TransTableL: public TransTable
{
  private:

    /// \brief A cached position match in the transposition table (52 bytes).
    struct WinMatch // 52 bytes
    {
      unsigned xor_set_;              ///< XOR of card holdings
      unsigned top_set1_, top_set2_, top_set3_, top_set4_;  ///< Top card sets
      unsigned top_mask1_, top_mask2_, top_mask3_, top_mask4_; ///< Top masks
      int mask_index_;                ///< Index into mask array
      int last_mask_no_;              ///< Last mask number
      NodeCards first_;               ///< Cached search result
    };

    /// \brief Block of match entries (6508 bytes).
    struct WinBlock // 6508 bytes when BlocksPerEntry == 125
    {
      int next_match_no_;             ///< Index of next available entry
      int next_write_no_;             ///< Index for next write
      int timestamp_read_;            ///< When last accessed (for harvesting)
      WinMatch list_[BlocksPerEntry]; ///< Array of match entries
    };

    /// \brief Hash entry for a particular card distribution (16 bytes).
    struct PosSearch // 16 bytes (inefficiency, 12 bytes enough)
    {
      WinBlock * pos_block_;          ///< Block containing this distribution
      long long key_;                 ///< Distribution hash key
    };

    /// \brief Hash table for a particular trick/hand (520 bytes).
    struct DistHash // 520 bytes when DistsPerEntry == 32
    {
      int next_no_;                   ///< Next entry index
      int next_write_no_;             ///< Next write index
      PosSearch list_[DistsPerEntry]; ///< Hash entries for distributions
    };

    /// \brief Aggregated targeting information per hand (80 bytes).
    struct Aggr // 80 bytes
    {
      unsigned aggr_ranks_[DDS_SUITS];        ///< Target tricks per suit
      unsigned aggr_bytes_[DDS_SUITS][TtBytes]; ///< Encoded bytes per suit
    };

    /// \brief Pool node for memory block linked list (16 bytes).
    struct Pool // 16 bytes
    {
      Pool * next_;                   ///< Next pool in list
      Pool * prev_;                   ///< Previous pool in list
      int next_block_no_;             ///< Next available block index
      WinBlock * list_;               ///< Array of blocks in this pool
    };

    /// \brief Statistics tracking for memory page usage.
    struct PageStats
    {
      int num_resets_;                ///< Total resets performed
      int num_callocs_;               ///< Total allocations
      int num_frees_;                 ///< Total deallocations
      int num_harvests_;              ///< Total harvest operations
      int last_current_;              ///< Last current page number
    };

    /// \brief Harvested blocks saved for potential reuse (16 bytes).
    struct Harvested // 16 bytes
    {
      int next_block_no_;             ///< Index of next available block
      WinBlock * list_[BlocksPerPage]; ///< Array of harvested blocks
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
    /// \brief Construct a large transposition table instance.
    ///
    /// Initializes the large TT with default memory limits. Must call
    /// make_tt() to actually allocate memory.
    TransTableL();

    /// \brief Destroy the large transposition table.
    ///
    /// Releases all allocated memory and internal structures.
    ~TransTableL();

    /// \brief Initialize the transposition table with hand lookup tables.
    ///
    /// Sets up the TT with hand lookup configuration for position hashing.
    ///
    /// \param hand_lookup Hand lookup table array of size [DDS_SUITS][15]
    ///   (i.e., parameter type const int hand_lookup[DDS_SUITS][15], 4 suits by 15 ranks).
    /// \throws std::bad_alloc if initialization fails
    /// \par Usage Example
    /// \code
    /// unsigned short ag[DDS_HANDS] = { 0x1fff, 0x1fff, 0x0f75, 0x1fff };
    /// int hd[DDS_HANDS] = { 0x0342, 0x0334, 0x0232, 0x0531 };
    /// bool lower_flag = false;
    /// int wr = 0;
    /// int result = 0;
    /// thrp->transTable.lookup(11, 1, ag, hd, 13, lower_flag);
    /// thrp->transTable.add(11, 1, ag, wr, result, false);
    /// \endcode
    auto init(const int hand_lookup[][15]) -> void override;

    /// \brief Set the default (soft) memory limit.
    ///
    /// \param megabytes Desired soft memory limit in MB
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
    /// \note Lookup updates the timestamp for harvesting considerations
    auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag) -> NodeCards const * override;

    /// \brief Add a computed result to the transposition table.
    ///
    /// Caches a newly computed search result for later lookup. May trigger
    /// harvesting if memory limits are approached.
    ///
    /// \param trick Current trick (0-12)
    /// \param hand Hand to play (0-3)
    /// \param aggr_target Target tricks per suit
    /// \param win_ranks_arg Winning ranks (optimization data)
    /// \param first Computed result to cache
    /// \param flag True if this is a lower bound (incomplete search)
    /// \throws std::bad_alloc if critical memory allocation fails
    /// \note May trigger harvest if soft memory limit exceeded
    auto add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks_arg[],
      const NodeCards& first,
      bool flag) -> void override;

    /// \brief Print cached results for a specific suit and position.
    ///
    /// Outputs detailed analysis of cached entries for the given trick/hand.
    /// Large TT provides detailed suit-level statistics.
    ///
    /// \param fout Output stream
    /// \param trick Trick number (0-12)
    /// \param hand Hand (0-3)
    auto print_suits(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;

    /// \brief Print all suit results in the transposition table.
    ///
    /// \param fout Output stream
    auto print_all_suits(std::ofstream& fout) const -> void override;

    /// \brief Print suit statistics for a specific position.
    ///
    /// \param fout Output stream
    /// \param trick Trick number (0-12)
    /// \param hand Hand (0-3)
    auto print_suit_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;

    /// \brief Print suit statistics for all positions.
    ///
    /// \param fout Output stream
    auto print_all_suit_stats(std::ofstream& fout) const -> void override;

    /// \brief Print summary suit statistics.
    ///
    /// \param fout Output stream
    auto print_summary_suit_stats(std::ofstream& fout) const -> void override;

    /// \brief Print entries for a specific hand distribution.
    ///
    /// \param fout Output stream
    /// \param trick Trick number (0-12)
    /// \param hand Hand (0-3)
    /// \param hand_dist Card distribution array
    auto print_entries_dist(
      std::ofstream& fout,
      int trick,
      int hand,
      const int hand_dist[]) const -> void override;

    /// \brief Print entries and card details for a hand distribution.
    ///
    /// \param fout Output stream
    /// \param trick Trick number (0-12)
    /// \param hand Hand (0-3)
    /// \param aggr_target Target tricks per suit
    /// \param hand_dist Card distribution array
    auto print_entries_dist_and_cards(
      std::ofstream& fout,
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[]) const -> void override;

    /// \brief Print entries for a specific trick/hand.
    ///
    /// \param fout Output stream
    /// \param trick Trick number (0-12)
    /// \param hand Hand (0-3)
    auto print_entries(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;

    /// \brief Print all cached entries in the table.
    ///
    /// \param fout Output stream
    auto print_all_entries(std::ofstream& fout) const -> void override;

    /// \brief Print entry statistics for a specific position.
    ///
    /// \param fout Output stream
    /// \param trick Trick number (0-12)
    /// \param hand Hand (0-3)
    auto print_entry_stats(
      std::ofstream& fout,
      int trick,
      int hand) const -> void override;

    /// \brief Print entry statistics for all positions.
    ///
    /// \param fout Output stream
    auto print_all_entry_stats(std::ofstream& fout) const -> void override;

    /// \brief Print summary entry statistics.
    ///
    /// \param fout Output stream
    auto print_summary_entry_stats(std::ofstream& fout) const -> void override;
};