---
capability: heuristic-sorting
owners: [heuristic_sorting]
last-updated: 2026-07-18
---

# Heuristic Sorting

> **Specs vs. doxygen / docs.** The move-ordering *algorithm* is described in
> `doc/heuristic-sorting.md`; the `HeuristicContext` fields and `call_heuristic`
> are documented inline in `heuristic_sorting.hpp`. This spec is deliberately thin
> — it records only the capability's role, its contract with the search, and the
> context-refactor intent.

## Purpose

Heuristic sorting assigns a *weight* to each candidate card in a move list so
[move-generation](move-generation.md) can order the most promising plays first, which is what makes
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
  first. See [transposition-table](transposition-table.md).
- **Scoring policy detail lives in `doc/heuristic-sorting.md`.** That narrative
  (including lead/void per-suit top-card bonuses) may lag the code; treat the
  doc as algorithm intent and the `weight_alloc_*` helpers as ground truth when
  they disagree.
- **`call_heuristic` takes a pre-built `HeuristicContext`.** The caller constructs
  one context (position, move array, best moves, relative ranks, and cached
  per-trick snapshots such as `removed_ranks`, `move1_rank`, `high1`) and passes
  it by const reference. Implementations `const_cast` and mutate move weights
  in place. There is a single free overload — the older parameterized form is
  gone.
- **`TrackType` is shared per-trick position state** defined here and consumed by
  both [move-generation](move-generation.md) and the heuristics. `Moves` owns the `track[]` array
  and mutates `trackp`; heuristics read the snapshots copied into
  `HeuristicContext`.
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
- Guarded by `//library/tests/heuristic_sorting/...` and
  `//library/tests/regression/heuristic_sorting`.

## Known gaps / non-goals

- Heuristic sorting only *orders* moves; it never changes which moves are legal or
  what the search concludes. A wrong weight can only slow the search, not
  mis-solve a board — the regression tests exist to catch ordering drift.
- The scoring policy is intentionally not restated here; `doc/heuristic-sorting.md`
  is the algorithm's source of truth.
