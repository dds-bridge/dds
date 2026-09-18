/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/*
   Shape → pattern transposition table.

   Positions are keyed by (trick, hand, suit-length shape). Under each key the
   table holds patterns. A pattern records, for the cards that decided a
   search result (all cards at or above the lowest winning rank in each
   suit), which hand holds each of them, in *relative* rank order — the same
   2-bits-per-card encoding TransTableL uses, restricted to the top twelve
   cards of every suit (the thirteenth is implied by the shape).

   A position matches a pattern when it agrees with it on every relevant
   card. Re-adding a pattern that is already stored intersects the bounds.

   The patterns of a shape live in one contiguous array, grouped into buckets
   by the owner of the top card of the pattern's first relevant suit, and
   within a bucket ordered by generality (fewest relevant cards first, oldest
   first among equals): general patterns match the most positions, so trying
   them first gives the earliest cut-offs. A lookup scans, with a fixed
   stride, only the buckets its own top cards allow.

   Experiments with a subsumption tree (storing more specific patterns
   beneath more general ones, as bridge-solver does) trimmed the number of
   patterns visited per lookup by about 15% but made every visit slower,
   since skipping a subtree needs its size, a dependent load that serializes
   the scan. The flat array was faster on every workload tried.
*/

#include "trans_table_p.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <new>
#include <sstream>
#include <string>

#include <api/dds_constants.hpp>

namespace
{

constexpr std::size_t MiB = 1024u * 1024u;
constexpr std::uint64_t HashMultiplier = 0x9E3779B97F4A7C15ull;

/// Fibonacci hashing: the top bits of the product are well mixed, the low
/// bits are not. table_size must be a power of two.
auto hash_slot(std::uint64_t key, std::size_t table_size) -> std::size_t
{
    const int bits = std::countr_zero(table_size);
    return static_cast<std::size_t>((key * HashMultiplier) >> (64 - bits));
}

} // namespace


TransTableP::TransTableP() = default;


TransTableP::~TransTableP()
{
    return_all_memory();
}


auto TransTableP::init(const int hand_lookup[][15]) -> void
{
    // For every 13-bit set of remaining cards in a suit, record which hand
    // holds each remaining card, top card first, 2 bits per card, and spread
    // the result over the three pattern words in the suit's own byte.
    ownership_.assign(8192, Ownership{});
    std::vector<std::array<std::uint32_t, DDS_SUITS>> ranks(8192);

    unsigned top_bit_rank = 1;
    unsigned top_bit_no = 2;
    for (unsigned ind = 1; ind < 8192; ++ind) {
        if (ind >= (top_bit_rank << 1)) {
            top_bit_rank <<= 1;
            ++top_bit_no;
        }
        for (int s = 0; s < DDS_SUITS; ++s) {
            ranks[ind][s] = (ranks[ind ^ top_bit_rank][s] >> 2) |
                (static_cast<std::uint32_t>(hand_lookup[s][top_bit_no]) << 24);
            for (int k = 0; k < PatternWords; ++k) {
                const std::uint32_t top_byte =
                    (ranks[ind][s] << (6 + 8 * k)) & 0xff000000u;
                ownership_[ind].set[s][k] = top_byte >> (8 * s);
            }
        }
    }
    forget_lookups();   // shapes remembered for the previous deal no longer apply
}


auto TransTableP::set_memory_default(const int megabytes) -> void
{
    default_bytes_ = static_cast<std::size_t>(std::max(megabytes, 0)) * MiB;
}


auto TransTableP::set_memory_maximum(const int megabytes) -> void
{
    maximum_bytes_ = static_cast<std::size_t>(std::max(megabytes, 0)) * MiB;
    // A hard cap applies at once: a live table already over the new limit
    // is cleared now rather than the next time a block is allocated, since
    // inserts into blocks with spare capacity never consult the budget.
    if (maximum_bytes_ != 0 && !shapes_.empty() && dynamic_bytes() > maximum_bytes_) {
        reset_memory(ResetReason::MemoryExhausted);
    }
}


auto TransTableP::make_tt() -> void
{
    // Only the hard maximum matters to this table; the default limit merely
    // floors it when the caller configured one. An unset default must not be
    // replaced by a built-in value, or it would lift an explicit small cap.
    if (maximum_bytes_ == 0) {
        maximum_bytes_ = static_cast<std::size_t>(THREADMEM_LARGE_MAX_MB) * MiB;
    }
    maximum_bytes_ = std::max(maximum_bytes_, default_bytes_);

    // Start from an empty table; the ownership table, if init() has already
    // built it, is deal-specific rather than size-specific and is kept.
    delete_trees();
    free_spare_trees();
    shapes_.assign(InitialShapes, ShapeSlot{});
    forget_lookups();
}


auto TransTableP::reset_memory(const ResetReason reason) -> void
{
    if (shapes_.empty()) {
        return;
    }
    ++reset_counts_[static_cast<int>(reason)];

    if (reason == ResetReason::MemoryExhausted) {
        // Over budget: give the blocks back outright. Pooling them first
        // could itself allocate, which is the one thing this path must not do.
        delete_trees();
        free_spare_trees();
    } else {
        release_trees();
    }
    // Free the old shape table before allocating the fresh one, so that a
    // reset never allocates on top of the storage it is about to drop.
    std::vector<ShapeSlot>().swap(shapes_);
    shapes_.resize(InitialShapes);
    forget_lookups();
}


auto TransTableP::return_all_memory() -> void
{
    delete_trees();
    free_spare_trees();
    std::vector<ShapeSlot>().swap(shapes_);
    std::vector<Ownership>().swap(ownership_);   // init() rebuilds it per deal
    forget_lookups();
}


auto TransTableP::forget_lookups() -> void
{
    // The shape a lookup() resolved is only valid for the following add()
    // while the table and the deal it was resolved against still exist;
    // an add() arriving after that without a fresh lookup() must be ignored.
    std::memset(last_key_, 0, sizeof(last_key_));
    std::memset(last_slot_, 0, sizeof(last_slot_));
}


auto TransTableP::dynamic_bytes() const -> std::size_t
{
    std::size_t pool_bytes = 0;
    for (const auto& spares : spare_trees_) {
        pool_bytes += spares.capacity() * sizeof(PatternTree*);
    }
    return tree_bytes_ + pool_bytes + shapes_.capacity() * sizeof(ShapeSlot);
}


auto TransTableP::memory_in_use() const -> double
{
    const std::size_t bytes = ownership_.capacity() * sizeof(Ownership) + dynamic_bytes();
    return static_cast<double>(bytes) / 1024.0;
}


auto TransTableP::node_count() const -> std::size_t
{
    return node_count_;
}


auto TransTableP::shape_count() const -> std::size_t
{
    return shape_count_;
}


// ---------------------------------------------------------------------------
// Keys and pattern encoding
// ---------------------------------------------------------------------------

auto TransTableP::shape_key(const int trick, const int hand, const int hand_dist[])
    -> std::uint64_t
{
    // hand_dist holds 12 bits per hand (spades, hearts, diamonds). The search
    // consults the table only at trick boundaries (ab_search_0), where every
    // hand holds trick + 1 cards, so the club length is implied - the same
    // derivation TransTableL::dist_to_lengths uses. trick + 1 keeps the key
    // non-zero.
    return (static_cast<std::uint64_t>(trick + 1) << 50) |
        (static_cast<std::uint64_t>(hand) << 48) |
        (static_cast<std::uint64_t>(hand_dist[0]) << 36) |
        (static_cast<std::uint64_t>(hand_dist[1]) << 24) |
        (static_cast<std::uint64_t>(hand_dist[2]) << 12) |
        static_cast<std::uint64_t>(hand_dist[3]);
}


auto TransTableP::mask_word(const int suit, const int relevant, const int word) -> std::uint32_t
{
    const int cards_in_word = std::clamp(relevant - 4 * word, 0, 4);
    if (cards_in_word == 0) {
        return 0;
    }
    const std::uint32_t byte = (0xffu << (8 - 2 * cards_in_word)) & 0xffu;
    return byte << (24 - 8 * suit);
}


auto TransTableP::position_set(const unsigned short aggr_target[], std::uint32_t set[]) const
    -> void
{
    for (int k = 0; k < PatternWords; ++k) {
        set[k] = ownership_[aggr_target[0]].set[0][k] |
            ownership_[aggr_target[1]].set[1][k] |
            ownership_[aggr_target[2]].set[2][k] |
            ownership_[aggr_target[3]].set[3][k];
    }
}


auto TransTableP::make_pattern(
    const unsigned short aggr_target[],
    const unsigned short win_ranks[],
    PatternKey& key,
    NodeCards& cards) const -> void
{
    key = PatternKey{};
    for (int s = 0; s < DDS_SUITS; ++s) {
        const unsigned w = win_ranks[s];
        cards.least_win[s] = 0;
        if (w == 0) {
            continue;
        }
        // Everything at or above the lowest winning rank is relevant.
        const unsigned lowest = w & (0u - w);
        const unsigned relevant = aggr_target[s] & ~(lowest - 1u);
        if (relevant == 0) {
            continue;
        }
        const int count = std::popcount(relevant);
        cards.least_win[s] = static_cast<char>(count);
        for (int k = 0; k < PatternWords; ++k) {
            key.word[k].set |= ownership_[relevant].set[s][k];
            key.word[k].mask |= mask_word(s, count, k);
        }
    }
}


auto TransTableP::same_pattern(const PatternKey& a, const PatternKey& b) -> bool
{
    for (int k = 0; k < PatternWords; ++k) {
        if (a.word[k].set != b.word[k].set || a.word[k].mask != b.word[k].mask) {
            return false;
        }
    }
    return true;
}


auto TransTableP::matches(const PatternKey& pattern, const std::uint32_t set[]) -> bool
{
    // The first word (top four cards of every suit) decides most mismatches;
    // test it alone before touching the rest of the node.
    if ((pattern.word[0].set ^ set[0]) & pattern.word[0].mask) {
        return false;
    }
    return (((pattern.word[1].set ^ set[1]) & pattern.word[1].mask) |
            ((pattern.word[2].set ^ set[2]) & pattern.word[2].mask)) == 0;
}


auto TransTableP::weight_of(const PatternKey& key) -> std::uint32_t
{
    // Two mask bits per relevant card.
    return static_cast<std::uint32_t>(
        std::popcount(key.word[0].mask) + std::popcount(key.word[1].mask) +
        std::popcount(key.word[2].mask)) / 2u;
}


auto TransTableP::bucket_of(const PatternKey& key) -> int
{
    for (int s = 0; s < DDS_SUITS; ++s) {
        const int shift = 24 - 8 * s;
        if ((key.word[0].mask >> shift) & 0xffu) {
            const int owner = static_cast<int>((key.word[0].set >> (shift + 6)) & 3u);
            return 1 + DDS_HANDS * s + owner;
        }
    }
    return 0;
}


// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

auto TransTableP::PatternTree::insert(const std::size_t at, const PatternNode& node) -> void
{
    PatternNode* p = nodes() + at;
    std::memmove(p + 1, p, (size - at) * sizeof(PatternNode));
    *p = node;
    ++size;
}


auto TransTableP::size_class(const std::size_t capacity) -> int
{
    return std::countr_zero(capacity / InitialTreeNodes);
}


auto TransTableP::acquire_tree(const std::size_t capacity) -> PatternTree*
{
    // Capacities are InitialTreeNodes << class; tree_bytes_ counts spare
    // blocks too, so reusing one costs nothing against the budget.
    auto& spares = spare_trees_[size_class(capacity)];
    PatternTree* tree;
    if (!spares.empty()) {
        tree = spares.back();
        spares.pop_back();
    } else {
        tree = static_cast<PatternTree*>(
            ::operator new(PatternTree::bytes_for(capacity), std::align_val_t{CacheLine}));
        tree_bytes_ += PatternTree::bytes_for(capacity);
    }
    std::memset(tree, 0, sizeof(PatternTree));
    tree->capacity = static_cast<std::uint32_t>(capacity);
    return tree;
}


auto TransTableP::release_tree(PatternTree* tree) -> void
{
    auto& spares = spare_trees_[size_class(tree->capacity)];
    if (spares.size() == spares.capacity()) {
        // The pool's pointer storage counts against the budget too. Growing
        // it allocates the whole replacement buffer while the old one (already
        // in dynamic_bytes()) is still live, so that full size is what must
        // fit under the cap; otherwise the block goes back to the allocator
        // instead of the pool.
        const std::size_t grown = std::max<std::size_t>(4, 2 * spares.capacity());
        const std::size_t replacement = grown * sizeof(PatternTree*);
        if (dynamic_bytes() + replacement > maximum_bytes_) {
            delete_tree(tree);
            return;
        }
        spares.reserve(grown);
    }
    spares.push_back(tree);
}


auto TransTableP::release_trees() -> void
{
    // Every slot is vacated, key included, so the counts stay exact even if
    // a caller kept the slot array instead of replacing it.
    for (ShapeSlot& slot : shapes_) {
        if (slot.tree) {
            release_tree(slot.tree);
        }
        slot = ShapeSlot{};
    }
    shape_count_ = 0;
    node_count_ = 0;
}


auto TransTableP::delete_tree(PatternTree* tree) -> void
{
    tree_bytes_ -= PatternTree::bytes_for(tree->capacity);
    ::operator delete(tree, std::align_val_t{CacheLine});
}


auto TransTableP::delete_trees() -> void
{
    for (ShapeSlot& slot : shapes_) {
        if (slot.tree) {
            delete_tree(slot.tree);
            slot.tree = nullptr;
        }
    }
    shape_count_ = 0;
    node_count_ = 0;
}


auto TransTableP::free_spare_trees() -> void
{
    // Used only on over-budget and teardown paths, so the pointer storage
    // goes too; it counts against the budget like everything else.
    for (auto& spares : spare_trees_) {
        for (PatternTree* tree : spares) {
            delete_tree(tree);
        }
        std::vector<PatternTree*>().swap(spares);
    }
}


auto TransTableP::reserve_one_more(ShapeSlot& slot) -> bool
{
    PatternTree* old = slot.tree;
    const std::size_t old_capacity = old ? old->capacity : 0;
    if (old && old->size < old_capacity) {
        return true;
    }
    const std::size_t wanted = std::max(InitialTreeNodes, old_capacity * 2);
    if (spare_trees_[size_class(wanted)].empty() &&
        dynamic_bytes() + PatternTree::bytes_for(wanted) > maximum_bytes_) {
        reset_memory(ResetReason::MemoryExhausted);
        return false;
    }
    PatternTree* fresh = acquire_tree(wanted);
    if (old) {
        std::memcpy(fresh, old, PatternTree::bytes_for(old->size));
        fresh->capacity = static_cast<std::uint32_t>(wanted);
    }
    slot.tree = fresh;   // committed: the slot owns the new block from here
    if (old) {
        try {
            release_tree(old);   // pooling may allocate and so may throw
        } catch (...) {
            delete_tree(old);
            throw;
        }
    }
    return true;
}


auto TransTableP::find_shape(const std::uint64_t key) const -> std::size_t
{
    if (shapes_.empty()) {
        return NoSlot;
    }
    const std::size_t mask = shapes_.size() - 1;
    for (std::size_t i = hash_slot(key, shapes_.size()); shapes_[i].key != 0; i = (i + 1) & mask) {
        if (shapes_[i].key == key) {
            return i;
        }
    }
    return NoSlot;
}


auto TransTableP::grow_shapes() -> void
{
    const std::size_t new_size = shapes_.size() * 2;
    // Old and new tables are both live during the rehash, so budget the peak.
    if (dynamic_bytes() + new_size * sizeof(ShapeSlot) > maximum_bytes_) {
        reset_memory(ResetReason::MemoryExhausted);
        return;
    }
    std::vector<ShapeSlot> fresh(new_size);
    const std::size_t mask = new_size - 1;
    for (const ShapeSlot& slot : shapes_) {
        if (slot.key == 0) {
            continue;
        }
        std::size_t i = hash_slot(slot.key, new_size);
        while (fresh[i].key != 0) {
            i = (i + 1) & mask;
        }
        fresh[i] = slot;
    }
    shapes_.swap(fresh);
}


auto TransTableP::find_or_insert_shape(const std::uint64_t key) -> std::size_t
{
    if (shape_count_ * 2 >= shapes_.size()) {
        grow_shapes();
    }
    const std::size_t mask = shapes_.size() - 1;
    std::size_t i = hash_slot(key, shapes_.size());
    while (shapes_[i].key != 0 && shapes_[i].key != key) {
        i = (i + 1) & mask;
    }
    if (shapes_[i].key == 0) {
        shapes_[i].key = key;
        ++shape_count_;
    }
    return i;
}


// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

auto TransTableP::lookup(
    const int trick,
    const int hand,
    const unsigned short aggr_target[],
    const int hand_dist[],
    const int limit,
    bool& lower_flag) -> NodeCards const*
{
    if (shapes_.empty() || trick < 0 || trick >= MaxTricks) {
        return nullptr;
    }
    const std::uint64_t key = shape_key(trick, hand, hand_dist);
    const std::size_t slot = find_shape(key);
    last_key_[trick][hand] = key;
    last_slot_[trick][hand] = slot;
    if (slot == NoSlot || shapes_[slot].tree == nullptr) {
        return nullptr;
    }

    std::uint32_t set[PatternWords];
    position_set(aggr_target, set);
    const PatternTree& tree = *shapes_[slot].tree;
    if (NodeCards const* found = find_cut(tree, 0, tree.bucket_end[0], set, limit, lower_flag)) {
        return found;
    }
    for (int s = 0; s < DDS_SUITS; ++s) {
        const int owner = static_cast<int>((set[0] >> (30 - 8 * s)) & 3u);
        const int bucket = 1 + DDS_HANDS * s + owner;
        if (NodeCards const* found = find_cut(
                tree, tree.bucket_end[bucket - 1], tree.bucket_end[bucket], set, limit, lower_flag)) {
            return found;
        }
    }
    return nullptr;
}


auto TransTableP::find_cut(
    const PatternTree& tree,
    const std::size_t begin,
    const std::size_t end,
    const std::uint32_t set[],
    const int limit,
    bool& lower_flag) -> NodeCards const*
{
    for (std::size_t i = begin; i < end; ++i) {
        const PatternNode& node = tree[i];
        if (!matches(node.key, set)) {
            continue;
        }
        if (node.cards.lower_bound > limit) {
            lower_flag = true;
            return &node.cards;
        }
        if (node.cards.upper_bound <= limit) {
            lower_flag = false;
            return &node.cards;
        }
    }
    return nullptr;
}


// ---------------------------------------------------------------------------
// Insertion
// ---------------------------------------------------------------------------

auto TransTableP::add(
    const int trick,
    const int hand,
    const unsigned short aggr_target[],
    const unsigned short win_ranks[],
    const NodeCards& first,
    const bool flag) -> void
{
    // Without a deal (no init() since make_tt()/return_all_memory()) there is
    // no ownership table to build patterns from; the table stays empty, which
    // also keeps lookup() off position_set().
    if (shapes_.empty() || ownership_.empty() || trick < 0 || trick >= MaxTricks) {
        return;
    }
    const std::uint64_t key = last_key_[trick][hand];
    if (key == 0) {
        return;   // add() without a preceding lookup() for this trick/hand
    }

    PatternKey pattern;
    NodeCards cards = first;
    make_pattern(aggr_target, win_ranks, pattern, cards);
    if (!flag) {
        cards.best_move_suit = 0;
        cards.best_move_rank = 0;
    }

    // The preceding lookup() usually found the slot already; it is only stale
    // if the table was rebuilt or reset in between.
    std::size_t slot = last_slot_[trick][hand];
    if (slot == NoSlot || slot >= shapes_.size() || shapes_[slot].key != key) {
        slot = find_or_insert_shape(key);
    }

    // Within its bucket the pattern goes before the first one with more
    // relevant cards; an identical pattern can only sit among those with
    // exactly as many. An existing pattern is tightened in place, which
    // needs no capacity, so the search precedes any reservation.
    const int bucket = bucket_of(pattern);
    const std::uint32_t weight = weight_of(pattern);
    std::size_t at = 0;
    if (PatternTree* existing = shapes_[slot].tree) {
        at = existing->bucket_begin(bucket);
        const std::size_t end = existing->bucket_end[bucket];
        for (; at < end; ++at) {
            PatternNode& stored = (*existing)[at];
            const std::uint32_t stored_weight = weight_of(stored.key);
            if (stored_weight > weight) {
                break;
            }
            if (stored_weight == weight && same_pattern(stored.key, pattern)) {
                tighten(stored.cards, cards, flag);
                return;
            }
        }
    }

    // Growing a block copies the nodes in order, so `at` stays valid.
    if (!reserve_one_more(shapes_[slot])) {
        return;   // the table was just reset; drop this entry
    }
    PatternTree& tree = *shapes_[slot].tree;
    tree.insert(at, PatternNode{pattern, cards});
    for (int b = bucket; b < BucketCount; ++b) {
        ++tree.bucket_end[b];
    }
    ++node_count_;
    ++num_adds_;
}


auto TransTableP::tighten(NodeCards& stored, const NodeCards& cards, const bool flag) -> void
{
    stored.lower_bound = std::max(stored.lower_bound, cards.lower_bound);
    stored.upper_bound = std::min(stored.upper_bound, cards.upper_bound);
    // Identical keys imply identical relevant-card counts, except that a
    // whole suit (13) and its top twelve share a key: the thirteenth card's
    // owner is implied by the shape. Keep the larger count so the entry never
    // under-reports the winning cards that some store recorded.
    for (int s = 0; s < DDS_SUITS; ++s) {
        stored.least_win[s] = std::max(stored.least_win[s], cards.least_win[s]);
    }
    if (flag) {
        stored.best_move_suit = cards.best_move_suit;
        stored.best_move_rank = cards.best_move_rank;
    }
}


// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

auto TransTableP::print_suits(std::ofstream& fout, const int trick, const int hand) const -> void
{
    std::size_t shapes = 0;
    std::size_t patterns = 0;
    for (const ShapeSlot& slot : shapes_) {
        if (slot.key == 0 || static_cast<int>((slot.key >> 50) - 1) != trick ||
            static_cast<int>((slot.key >> 48) & 3) != hand) {
            continue;
        }
        ++shapes;
        patterns += slot.tree ? slot.tree->size : 0;
    }
    fout << "Trick " << trick << " hand " << hand << ": " << shapes
         << " shapes, " << patterns << " patterns\n";
}


auto TransTableP::print_all_suits(std::ofstream& fout) const -> void
{
    for (int t = 0; t < MaxTricks; ++t) {
        for (int h = 0; h < DDS_HANDS; ++h) {
            print_suits(fout, t, h);
        }
    }
}


auto TransTableP::print_suit_stats(std::ofstream& fout, const int trick, const int hand) const
    -> void
{
    print_suits(fout, trick, hand);
}


auto TransTableP::print_all_suit_stats(std::ofstream& fout) const -> void
{
    print_all_suits(fout);
}


auto TransTableP::print_summary_suit_stats(std::ofstream& fout) const -> void
{
    fout << "Shapes: " << shape_count_ << "\n";
}


auto TransTableP::print_entries_dist(
    std::ofstream& fout, const int trick, const int hand, const int hand_dist[]) const -> void
{
    const std::size_t slot = find_shape(shape_key(trick, hand, hand_dist));
    fout << "Trick " << trick << " hand " << hand << ": "
         << (slot == NoSlot || !shapes_[slot].tree ? 0 : shapes_[slot].tree->size) << " patterns\n";
}


auto TransTableP::print_entries_dist_and_cards(
    std::ofstream& fout,
    const int trick,
    const int hand,
    const unsigned short aggr_target[],
    const int hand_dist[]) const -> void
{
    const std::size_t slot = find_shape(shape_key(trick, hand, hand_dist));
    PatternTree const* tree = slot == NoSlot ? nullptr : shapes_[slot].tree;
    const std::size_t total = tree ? tree->size : 0;

    std::uint32_t set[PatternWords] = {};
    if (!ownership_.empty()) {
        position_set(aggr_target, set);
    }
    std::ostringstream lines;
    std::size_t matched = 0;
    for (std::size_t i = 0; i < total; ++i) {
        const PatternNode& stored = (*tree)[i];
        if (!matches(stored.key, set)) {
            continue;
        }
        ++matched;
        lines << "  [" << static_cast<int>(stored.cards.lower_bound) << ", "
              << static_cast<int>(stored.cards.upper_bound) << "] least_win";
        for (int s = 0; s < DDS_SUITS; ++s) {
            lines << ' ' << static_cast<int>(stored.cards.least_win[s]);
        }
        lines << ' ' << owners_of(stored.key) << " best move "
              << static_cast<int>(stored.cards.best_move_suit) << '/'
              << static_cast<int>(stored.cards.best_move_rank) << '\n';
    }
    fout << "Trick " << trick << " hand " << hand << ": " << total << " patterns, "
         << matched << " match the cards\n" << lines.str();
}


auto TransTableP::owners_of(const PatternKey& key) -> std::string
{
    // Per suit, the owner of each relevant card from the top down, in the
    // same layout position_set() uses: one byte per suit in each word, the
    // top card of the word's four in the byte's high two bits.
    static constexpr char suit_letter[DDS_SUITS] = {'S', 'H', 'D', 'C'};
    static constexpr char seat_letter[DDS_HANDS] = {'N', 'E', 'S', 'W'};
    std::string text;
    for (int s = 0; s < DDS_SUITS; ++s) {
        text += suit_letter[s];
        text += ':';
        bool any = false;
        for (int card = 0; card < 4 * PatternWords; ++card) {
            const PatternWord& word = key.word[card / 4];
            const int bit = 24 - 8 * s + 6 - 2 * (card % 4);
            if ((word.mask >> bit) & 3u) {
                text += seat_letter[(word.set >> bit) & 3u];
                any = true;
            }
        }
        if (!any) {
            text += '-';
        }
        text += ' ';
    }
    return text;
}


auto TransTableP::print_entries(std::ofstream& fout, const int trick, const int hand) const
    -> void
{
    print_suits(fout, trick, hand);
}


auto TransTableP::print_all_entries(std::ofstream& fout) const -> void
{
    print_all_suits(fout);
}


auto TransTableP::print_entry_stats(std::ofstream& fout, const int trick, const int hand) const
    -> void
{
    print_suits(fout, trick, hand);
}


auto TransTableP::print_all_entry_stats(std::ofstream& fout) const -> void
{
    print_all_suits(fout);
}


auto TransTableP::print_summary_entry_stats(std::ofstream& fout) const -> void
{
    fout << "Patterns: " << node_count() << ", shapes: " << shape_count_
         << ", memory KB: " << memory_in_use() << "\n";
}


auto TransTableP::print_reset_stats(std::ofstream& fout) const -> void
{
    for (int r = 0; r < ResetReasonCount; ++r) {
        fout << "Reset reason " << r << ": " << reset_counts_[r] << "\n";
    }
}
