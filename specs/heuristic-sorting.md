---
capability: heuristic-sorting
owners: [heuristic_sorting]
last-updated: 2026-07-16
related-plans: [write_specs]
---

# Heuristic Sorting

> **Specs vs. doxygen / docs.** The move-ordering *algorithm* is described in
> `doc/heuristic-sorting.md`; the `HeuristicContext` fields and `call_heuristic`
> are documented inline in `heuristic_sorting.hpp`. This spec is deliberately thin
> — it records only the capability's role, its contract with the search, and the
> context-refactor intent.

## Purpose

Heuristic sorting assigns a *weight* to each candidate card in a move list so
[[move-generation]] can order the most promising plays first, which is what makes
the alpha-beta search prune effectively. It exists as a separate capability
because the weighting policy is intricate (position-, trump-, and
void-situation-dependent) and is the single hottest inner-loop computation in a
solve.

## Behaviour & invariants

> The scoring formulas live in `doc/heuristic-sorting.md` and the code; these are
> the facts the rest of the system relies on.

- **Higher weight = tried earlier.** `call_heuristic` mutates the move list's
  weighting so the search visits high-weight cards first. Correctness of the
  *result* never depends on ordering; only performance does.
- **The two "best moves" get bonus priority.** The search's alpha-beta best move
  and the transposition-table best move (`best_move`, `best_move_tt` in the
  context) are weighted to sort to the front, so known-good moves are re-tried
  first. See [[transposition-table]].
- **Leading/void hands get a per-suit top-card bonus.** When the hand to play
  leads the trick or is void in the led suit, the highest-weight card of each
  present suit receives a large additional bonus (per `doc/heuristic-sorting.md`).
- **`call_heuristic` takes a pre-built `HeuristicContext`.** This is the
  refactor's central invariant: the caller constructs one `HeuristicContext`
  (position, move array, best moves, relative ranks, and cached per-trick
  snapshots such as `removed_ranks`, `move1_rank`, `high1`) and passes it by
  const reference. The context localises the mutable buffers and snapshots the few
  `trackp` fields hot helpers need, so the heuristics no longer read through
  `Moves::trackp` mutation. Prefer this overload — it minimises per-call
  construction overhead on the hot path.
- **`TrackType` is the shared per-trick position state** consumed by both
  [[move-generation]] and the heuristics; it is defined here because the
  heuristics are its primary reader.
- **`testable_heuristic_sorting`** exposes the same sources to the heuristic-sorting
  test packages; behaviour matches `heuristic_sorting`.

## Key entry points

- `library/src/heuristic_sorting/heuristic_sorting.hpp` — `HeuristicContext`,
  `TrackType`, and `call_heuristic(const HeuristicContext&)`. Doxygen documents the
  fields.
- `library/src/heuristic_sorting/heuristic_sorting.cpp` +
  `internal.hpp` — the per-situation weighting helpers.
- Narrative algorithm: `doc/heuristic-sorting.md`.
- Build targets: `//library/src/heuristic_sorting:{heuristic_sorting,testable_heuristic_sorting}`.
- Guarded by `//library/tests/heuristic_sorting` and
  `//library/tests/regression/heuristic_sorting`.

## Known gaps / non-goals

- Heuristic sorting only *orders* moves; it never changes which moves are legal or
  what the search concludes. A wrong weight can only slow the search, not
  mis-solve a board — the regression tests exist to catch ordering drift.
- The scoring policy is intentionally not restated here; `doc/heuristic-sorting.md`
  is the algorithm's source of truth.
