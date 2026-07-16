---
capability: move-generation
owners: [moves]
last-updated: 2026-07-16
related-plans: [write_specs]
---

# Move Generation

> **Specs vs. doxygen.** The `Moves` class and every method are documented inline
> in `moves.hpp`. This spec records the generation/iteration contract the search
> depends on and the stateful lifecycle that no single method reveals.

## Purpose

The `Moves` class generates and iterates the legal card plays the alpha-beta
search explores at each trick, ordered so the most promising moves come first.
Good ordering is what makes the search tractable: it maximises alpha-beta cutoffs
and transposition-table hits. `Moves` is an internal component — it is not part of
[[dds-public-api]] — consumed by the search through the [[solver-context]]
`MoveGenContext` facade.

## Behaviour & invariants

> Per-method signatures are in the header doxygen; these are the whole-object
> guarantees.

- **Stateful, single-solve object with a fixed lifecycle.** Typical order:
  `Init()` (new deal state) → `MoveGen0()` for the opening lead **or**
  `MoveGen123()` for 2nd/3rd/4th hand → iterate with
  `MakeNext()` / `MakeNextSimple()` → `MakeSpecific()` to record the chosen play →
  `RegisterHit()` for statistics. `Reinit()` resets for a new lead hand.
- **No dynamic allocation; RAII stack storage.** Move lists (`moveList[13][DDS_HANDS]`)
  and per-trick tracking (`track[13]`) are stack-allocated arrays. The `trackp`
  and `mply` cursors are **non-owning** pointers into those arrays and are valid
  only for the lifetime of the `Moves` object — they must not outlive it.
- **Ordering is heuristic-weight-driven.** `Sort()` / `MergeSort()` order the
  current list by weight; `call_heuristic()` delegates the weighting to
  [[heuristic-sorting]], seeded with the search's best move and the TT's best move
  so those are tried first. `Reward()` bumps the weight of the last chosen move.
- **Iteration is explicit and rewindable.** `Step()` advances, `Rewind()` resets
  to the list head, `MakeNext(win_ranks)` returns the next move satisfying the
  per-suit winning-rank constraint (or `nullptr` when none remains),
  `MakeNextSimple()` iterates without constraints. `Purge()` removes a set of
  forbidden moves from a list.
- **`MgType` categorises each generation call** by contract (NT vs trump) and
  void situation (0–3 hands void), 13 categories total. It indexes the statistics
  tables and the heuristic's per-situation tracking.
- **Invariants are assertion-checked.** Public methods assume valid input and
  prior initialisation; violations trip asserts in debug builds. The move-finding
  methods signal "no move" by returning `nullptr`, not by throwing.
- **Statistics/printing are diagnostic-only** and emit under `DDS_MOVES` /
  `DDS_MOVES_DETAILS` (see [[constants-and-debug]]); they are off the hot path in
  normal builds.
- **`testable_moves`** exposes the same sources to `//library/tests/moves` for
  white-box testing; behaviour is identical to `moves`.

## Key entry points

- `library/src/moves/moves.hpp` — the `Moves` class, `MgType`, and the statistics
  structs. Doxygen documents each method.
- `library/src/moves/moves.cpp` — generation, ordering, and iteration logic.
- Build targets: `//library/src/moves:{moves,testable_moves}`.
- Guarded by `//library/tests/moves`.

## Known gaps / non-goals

- `Moves` does not evaluate positions or decide cutoffs — it only produces and
  orders candidate plays; the search owns the alpha-beta decisions.
- It computes move *weights* via [[heuristic-sorting]] rather than embedding the
  scoring policy itself.
- Not thread-safe and not reusable across threads; one instance per solving
  thread, consistent with [[solver-context]].
