/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <trans_table/trans_table.hpp>

/// \brief Transposition table organised as shape → relative-rank patterns.
///
/// This implementation follows the "shape → pattern" cache of macroxue's
/// bridge-solver. A position is keyed by its suit-length shape (plus trick
/// count and hand to play). Under each shape the cached results are
/// *patterns*: the relative-rank ownership of the cards that mattered for the
/// result (the cards at or above the lowest winning rank in each suit), with
/// trick bounds. A lookup position matches a pattern when it agrees with the
/// pattern on every relevant card.
///
/// Compared with \ref TransTableL, a shape may hold any number of patterns
/// (no fixed per-shape capacity forces older entries out), the patterns of a
/// shape are ordered most general first (fewest relevant cards), since those
/// match the most positions and so give the earliest cut-offs, and they are
/// partitioned into buckets by the owner of the top card of the first suit
/// with a relevant card, so that a lookup only scans the buckets it can
/// possibly match.
///
/// Memory grows on demand up to the configured maximum; when exhausted the
/// whole table is cleared (\ref ResetReason::MemoryExhausted) and filling
/// resumes. The maximum governs the cache storage (pattern blocks, the spare
/// pool and the shape table, transients included). The fixed per-deal
/// card-ownership table that `init()` builds (about 384 KB, plus a similar
/// transient while building) is reported by `memory_in_use()` but not charged
/// against the maximum.
///
/// \par Lifecycle
/// `make_tt()` creates an empty table; `init(hand_lookup)` then builds the
/// deal-specific card-ownership table, which patterns are derived from.
/// `return_all_memory()` releases both, so after `return_all_memory();
/// make_tt();` a further `init()` is required before anything can be stored;
/// until then the table is inert (lookups miss, adds are ignored). The
/// production context calls `init()` for every deal, so this only matters to
/// standalone users.
///
/// \par Thread Safety
/// Not thread-safe. Must be accessed from a single thread.
class TransTableP : public TransTable
{
  public:
    TransTableP();
    ~TransTableP() override;

    /// Owns raw pattern blocks; copying would alias and then double-free them.
    TransTableP(const TransTableP&) = delete;
    auto operator=(const TransTableP&) -> TransTableP& = delete;

    auto init(const int hand_lookup[][15]) -> void override;
    auto set_memory_default(int megabytes) -> void override;
    auto set_memory_maximum(int megabytes) -> void override;
    auto make_tt() -> void override;
    auto reset_memory(ResetReason reason) -> void override;
    auto return_all_memory() -> void override;
    auto memory_in_use() const -> double override;

    auto lookup(
        int trick,
        int hand,
        const unsigned short aggr_target[],
        const int hand_dist[],
        int limit,
        bool& lower_flag) -> NodeCards const* override;

    auto add(
        int trick,
        int hand,
        const unsigned short aggr_target[],
        const unsigned short win_ranks[],
        const NodeCards& first,
        bool flag) -> void override;

    auto print_suits(std::ofstream& fout, int trick, int hand) const -> void override;
    auto print_all_suits(std::ofstream& fout) const -> void override;
    auto print_suit_stats(std::ofstream& fout, int trick, int hand) const -> void override;
    auto print_all_suit_stats(std::ofstream& fout) const -> void override;
    auto reset_op_stats() -> void override
    {
      num_adds_ = 0;
      num_overwrites_ = 0;
    }
    auto get_op_stats(int& adds, int& overwrites, int& harvests) const -> void override
    {
      adds = num_adds_;
      overwrites = num_overwrites_;
      harvests = 0; // TransTableP has no harvest mechanism
    }
    // Instrumentation counters
    mutable int num_adds_ = 0;
    mutable int num_overwrites_ = 0;

    auto print_summary_suit_stats(std::ofstream& fout) const -> void override;
    auto print_entries_dist(
        std::ofstream& fout, int trick, int hand, const int hand_dist[]) const -> void override;
    auto print_entries_dist_and_cards(
        std::ofstream& fout,
        int trick,
        int hand,
        const unsigned short aggr_target[],
        const int hand_dist[]) const -> void override;
    auto print_entries(std::ofstream& fout, int trick, int hand) const -> void override;
    auto print_all_entries(std::ofstream& fout) const -> void override;
    auto print_entry_stats(std::ofstream& fout, int trick, int hand) const -> void override;
    auto print_all_entry_stats(std::ofstream& fout) const -> void override;
    auto print_summary_entry_stats(std::ofstream& fout) const -> void override;
    auto print_reset_stats(std::ofstream& fout) const -> void override;

    /// \brief Number of stored patterns (white-box diagnostics and tests).
    auto node_count() const -> std::size_t;

    /// \brief Number of distinct (trick, hand, shape) keys with stored patterns.
    auto shape_count() const -> std::size_t;

  private:
    /// Relative-rank ownership uses 2 bits per card and 4 cards per suit per
    /// word; three words cover the top twelve cards of every suit. The
    /// thirteenth card is implied by the shape and the other twelve.
    static constexpr int PatternWords = 3;
    static constexpr int MaxTricks = 13;
    static constexpr std::size_t InitialShapes = 1024;
    static constexpr std::size_t InitialTreeNodes = 8;
    static constexpr std::size_t NoSlot = static_cast<std::size_t>(-1);
    static constexpr std::size_t CacheLine = 64;

    /// Patterns are partitioned by their first relevant suit and the owner
    /// of that suit's top card (bucket 1 + 4 * suit + owner); patterns with
    /// no relevant card go in bucket 0. A position can only match patterns
    /// in bucket 0 or, per suit, in the bucket of the hand holding its own
    /// top card of that suit, so a lookup scans 5 of the 17 buckets.
    static constexpr int BucketCount = 1 + DDS_SUITS * DDS_HANDS;

    /// One word covers four relative cards per suit (2 bits each): `set`
    /// holds the owners, `mask` which of those cards are relevant. Set and
    /// mask are interleaved so that the first word's test, which decides
    /// almost every mismatch, touches eight contiguous bytes.
    struct PatternWord
    {
        std::uint32_t set;
        std::uint32_t mask;
    };

    struct PatternKey
    {
        PatternWord word[PatternWords];
    };

    /// One stored pattern; two fit in a cache line.
    struct PatternNode
    {
        PatternKey key;
        NodeCards cards;
    };

    /// A shape's patterns: one heap block headed by this struct, followed by
    /// the nodes, bucket by bucket. The header is padded to whole cache
    /// lines so that the nodes are line-aligned. Nodes are trivially
    /// copyable, so the block is managed with plain memory moves.
    ///
    /// The padding is spelled out rather than left to `alignas` so that the
    /// layout is identical on every compiler (and MSVC's C4324 stays quiet).
    static constexpr std::size_t TreeHeaderBytes = 2 * sizeof(std::uint32_t) +
                                                   BucketCount * sizeof(std::uint32_t);
    static constexpr std::size_t TreePaddingBytes =
        (CacheLine - TreeHeaderBytes % CacheLine) % CacheLine;

    struct alignas(CacheLine) PatternTree
    {
        std::uint32_t size;
        std::uint32_t capacity;
        std::uint32_t bucket_end[BucketCount];   ///< End offset of each bucket.
        std::uint8_t padding[TreePaddingBytes];  ///< Rounds the header up to whole lines.

        auto nodes() -> PatternNode* { return reinterpret_cast<PatternNode*>(this + 1); }
        auto nodes() const -> const PatternNode*
        {
            return reinterpret_cast<const PatternNode*>(this + 1);
        }
        auto operator[](std::size_t i) -> PatternNode& { return nodes()[i]; }
        auto operator[](std::size_t i) const -> const PatternNode& { return nodes()[i]; }
        auto bucket_begin(int bucket) const -> std::size_t
        {
            return bucket == 0 ? 0 : bucket_end[bucket - 1];
        }
        static auto bytes_for(std::size_t capacity) -> std::size_t
        {
            return sizeof(PatternTree) + capacity * sizeof(PatternNode);
        }
        auto insert(std::size_t at, const PatternNode& node) -> void;
    };
    static_assert(sizeof(PatternTree) % CacheLine == 0 &&
                  sizeof(PatternTree) == TreeHeaderBytes + TreePaddingBytes,
                  "PatternTree header must fill whole cache lines with no implicit padding");
    static_assert(sizeof(PatternNode) == 32 && CacheLine % sizeof(PatternNode) == 0,
                  "two PatternNodes must fit exactly in a cache line");

    struct ShapeSlot
    {
        std::uint64_t key = 0;        ///< 0 marks an empty slot.
        PatternTree* tree = nullptr;  ///< Null until the first pattern is added.
    };

    /// Ownership encoding of one 13-bit remaining-cards set, per suit and word.
    struct Ownership
    {
        std::uint32_t set[DDS_SUITS][PatternWords];
    };

    /// Tree blocks come in power-of-two capacities; released blocks are kept
    /// per size class for reuse so that the search never touches the heap
    /// allocator in steady state.
    static constexpr int SizeClasses = 24;

    std::vector<Ownership> ownership_;
    std::vector<ShapeSlot> shapes_;
    std::vector<PatternTree*> spare_trees_[SizeClasses];
    std::size_t shape_count_ = 0;
    std::size_t node_count_ = 0;
    std::size_t tree_bytes_ = 0;   ///< All tree blocks, in use or spare.
    std::uint64_t last_key_[MaxTricks][DDS_HANDS] = {};
    std::size_t last_slot_[MaxTricks][DDS_HANDS] = {};
    std::size_t default_bytes_ = 0;
    std::size_t maximum_bytes_ = 0;
    int reset_counts_[ResetReasonCount] = {};

    static auto shape_key(int trick, int hand, const int hand_dist[]) -> std::uint64_t;
    static auto mask_word(int suit, int relevant, int word) -> std::uint32_t;
    static auto same_pattern(const PatternKey& a, const PatternKey& b) -> bool;
    static auto matches(const PatternKey& pattern, const std::uint32_t set[]) -> bool;
    static auto weight_of(const PatternKey& key) -> std::uint32_t;
    static auto bucket_of(const PatternKey& key) -> int;
    static auto owners_of(const PatternKey& key) -> std::string;

    auto position_set(const unsigned short aggr_target[], std::uint32_t set[]) const -> void;
    auto make_pattern(
        const unsigned short aggr_target[],
        const unsigned short win_ranks[],
        PatternKey& key,
        NodeCards& cards) const -> void;

    auto dynamic_bytes() const -> std::size_t;
    auto reserve_one_more(ShapeSlot& slot) -> bool;
    auto acquire_tree(std::size_t capacity) -> PatternTree*;
    auto release_tree(PatternTree* tree) -> void;   ///< To the pool (may allocate).
    auto release_trees() -> void;
    auto delete_tree(PatternTree* tree) -> void;    ///< To the allocator (never allocates).
    auto delete_trees() -> void;
    auto forget_lookups() -> void;
    auto free_spare_trees() -> void;
    static auto size_class(std::size_t capacity) -> int;

    auto find_shape(std::uint64_t key) const -> std::size_t;
    auto find_or_insert_shape(std::uint64_t key) -> std::size_t;
    auto grow_shapes() -> void;

    static auto find_cut(
        const PatternTree& tree,
        std::size_t begin,
        std::size_t end,
        const std::uint32_t set[],
        int limit,
        bool& lower_flag) -> NodeCards const*;

    static auto tighten(NodeCards& stored, const NodeCards& cards, bool flag) -> void;
};
