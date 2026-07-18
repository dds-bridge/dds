---
capability: lookup-tables
owners: [lookup_tables]
last-updated: 2026-07-18
related-plans: []
---

# Lookup Tables

> **Specs vs. doxygen.** Each table's exact indexing and return encoding is
> documented inline in `lookup_tables.hpp`. This spec records what the tables are
> collectively, their initialisation/immutability contract, and who depends on
> them.

## Purpose

This capability precomputes, once, the derived suit-analysis tables the solver
consults on every node of the search: for any suit holding it can answer "highest
card", "lowest card", "how many cards", "relative rank of a card", "the top-N
cards", and "decompose this holding into runs of adjacent ranks". Computing these
by table lookup instead of on the fly is a core performance decision — the hot
inner loops of [[move-generation]] and the search index these arrays directly.

## Behaviour & invariants

> Per-table encodings live in the header doxygen; these are the whole-capability
> facts.

- **Everything is indexed by a 13-bit suit "aggregate".** A suit holding is a
  bitmask, bit 0 = deuce (2) … bit 12 = Ace (14); valid aggregate range
  `0..8191`. Every exposed table is `[8192]`-sized on that index.
- **The tables:** `highest_rank`, `lowest_rank` (absolute rank 2..14, or 0 for an
  empty suit), `count_table` (popcount, 0..13), `rel_rank[8192][15]` (ordinal
  position of an absolute rank within the holding, from the top),
  `win_ranks[8192][14]` (bitmask of the top-N cards), and `group_data[8192]`
  (`MoveGroupType` run decomposition — up to 7 runs of adjacent ranks with their
  top card, tail sequence, full sequence, and inter-run gaps).
- **Initialised once, eagerly, at startup.** `init_lookup_tables()` fills the
  storage via static initialisation (`DdsLutInitGuard`), guarded by
  `std::call_once`. It is **thread-safe, idempotent, and a no-op after startup** —
  explicit calls are safe but redundant. No consumer needs to initialise them.
- **Read-only and immutable after init.** The tables are exposed as `const`
  references to fixed-size arrays, giving zero-overhead direct indexing
  (`highest_rank[aggr]`, `rel_rank[aggr][rank]`). Any thread may read them
  concurrently; nothing mutates them post-initialisation.
- **`MoveGroupType` is the run-decomposition contract** consumed by move
  generation: `last_group_` (−1 for empty, up to 6), and per-group `rank_`,
  `sequence_`, `fullseq_`, and `gap_` arrays, valid only for indices
  `0..last_group_`. `fullseq_[g] == bit_map_rank[rank_[g]] | sequence_[g]`, tying
  it back to [[constants-and-debug]].

## Key entry points

- `library/src/lookup_tables/lookup_tables.hpp` — `init_lookup_tables()`, the
  `const&` table views, and `MoveGroupType`. Doxygen documents each table.
- `library/src/lookup_tables/lookup_tables.cpp` — the one-time table computation.
- Build target: `//library/src/lookup_tables:lookup_tables`.
- Guarded by `//library/tests/utility:lookup_tables_test`.

## Known gaps / non-goals

- These are *derived* tables computed from the fixed vocabulary in
  [[constants-and-debug]] — that capability owns the primitive constants; this one
  owns the precomputed analysis over them.
- The tables are static and global by design; they are not per-context state and
  do not participate in [[solver-context]] lifecycle or reset.
