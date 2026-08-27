---
capability: constants-and-debug
owners: [utility]
last-updated: 2026-07-18
---

# Constants & Debug

> **Specs vs. doxygen.** The value of each constant and the meaning of each
> debug flag live in the doxygen for `constants.h` / `debug.h`. This spec records
> only the shared conventions other capabilities depend on and must not break.

## Purpose

This capability is the shared vocabulary of the whole solver: the fixed bridge
dimensions (strains, hands, suits), the card-representation lookup tables that
convert between rank/suit/hand encodings, and the compile-time debug-flag
conventions. Every other module — move generation, heuristic sorting, the
transposition table, the scheduler — encodes cards, hands, and strains the same
way because they all draw those definitions from here. It exists so there is a
single, authoritative definition of "how a card/hand/strain is represented" and
"how diagnostic output is switched on", rather than each module inventing its own.

## Behaviour & invariants

> Per-symbol values (e.g. the exact contents of `bit_map_rank`) live in doxygen
> (`utility_constants` group). Only the cross-cutting conventions are recorded here.

- **Fixed dimensions.** `DDS_STRAINS = 5` (4 suits + NT), `DDS_HANDS = 4`,
  `DDS_SUITS = 4`, `DDS_NOTRUMP = 4`. These are `constexpr` and are treated as
  immutable across the codebase — array sizes and loop bounds everywhere assume
  them.
- **Hand-relationship arrays** (`lho`, `rho`, `partner`) map an absolute hand
  0–3 to its left-hand opponent / right-hand opponent / partner. Consumers rely
  on the seating convention: hands are ordered N(0)/E(1)/S(2)/W(3) going
  clockwise, so `lho` advances +1 mod 4.
- **Card encodings are round-trippable and O(1).** `bit_map_rank` (rank→bitmask,
  rank 2→0x0001 … Ace→0x1000), `card_rank` (rank→char), `card_suit`
  (strain→S/H/D/C/N), `card_hand` (hand→N/E/S/W). Sentinel indices exist
  (rank table indices 0, 1, 15) and must not be treated as real cards.
- **These arrays are `extern const`** — defined once in `constants.cpp`, never
  mutated at runtime. Any capability may read them concurrently.
- **Debug flags are compile-time, opt-in, and off by default.** Each flag in
  `debug.h` is a commented-out preprocessor macro (`#ifdef`-probed); enabling one
  makes the solver emit one diagnostic file per thread with a fixed name prefix
  (e.g. `ABstats`, `TTstats`, `timer`). The prefixes themselves are `constexpr`
  string constants (`DDS_*_PREFIX`, `DDS_DEBUG_SUFFIX`), not macros.
  `DDS_DEBUG_ALL` turns on the debug.h diagnostic set it wraps (AB stats/details,
  TT stats, timing, moves, …) but **not** build-gated flags such as
  `DDS_SCHEDULER`. Because they are compile-time, a normal build carries zero
  cost from them.
- **`DDS_AB_STATS` is also driven by the build system** (`--define=ab_stats=true`
  → the `ab_stats` config setting), which is the supported way to enable
  alpha-beta statistics without editing `debug.h`. See [ab-stats](ab-stats.md) and
  [build-system](build-system.md).

## Key entry points

- `library/src/utility/constants.h` — dimensions, hand-relationship arrays, and
  card lookup tables (declarations). Doxygen: group `utility_constants`.
- `library/src/utility/constants.cpp` — the single definition of the `extern const`
  tables.
- `library/src/utility/debug.h` — compile-time diagnostic flags, their file-name
  prefixes, and the `DDS_DEBUG_ALL` aggregate. Doxygen: group `utility_debug`.
- Build target: `//library/src/utility:constants` (headers `include_prefix`ed as
  `utility/`).

## Known gaps / non-goals

- No runtime configuration of the debug flags — they are compile-time only. The
  one exception, `ab_stats`, is surfaced through the build system, not this
  capability.
- This capability defines representations, not behaviour: it computes nothing and
  owns no algorithm. Precomputed *derived* tables (bit/rank/trick tables built for
  the search) live in [lookup-tables](lookup-tables.md), not here.
