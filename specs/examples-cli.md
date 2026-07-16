---
capability: examples-cli
owners: [examples, hands]
last-updated: 2026-07-16
related-plans: [write_specs]
---

# Example CLIs

> **Specs vs. source.** Each example's exact I/O is in its `.cpp`. This spec
> records what the example set is *for*, how it is grouped, and the conventions a
> reader should expect — not a per-flag manual.

## Purpose

The `examples/` binaries are the canonical, runnable demonstrations of the public
API: how to build a deal, call the solver, and read results. They double as
reference usage for the bindings and as the C++ programs the [[wasm-emscripten]]
build ports to the browser. They are demonstrations, not a supported product CLI.

## Behaviour & invariants

> Per-binary detail lives in the sources; these are the whole-set conventions.

- **Grouped by what they demonstrate:**
  - **Solve a board:** `solve_board`, `solve_board_pbn`, `solve_all_boards`.
  - **Double-dummy tables:** `calc_dd_table`, `calc_dd_table_pbn`,
    `calc_all_tables`, `calc_all_tables_pbn`, `dd_table_for_deal`.
  - **Par scoring:** `par`, `dealer_par`.
  - **Play analysis:** `AnalysePlayBin` (source `analyse_play_bin.cpp`),
    `analyse_play_pbn`, `analyse_all_plays_bin`, `analyse_all_plays_pbn`.
  - **Modern context API demos:** `migration_example`,
    `calc_par_context_example`.
- **PBN and binary variants come in pairs.** Most examples have a `*` (binary
  `Deal`/`DdTableDeal`) and a `*_pbn` (PBN string) twin, mirroring the paired
  entry points of [[dds-public-api]]; both produce the same results from the same
  underlying deal.
- **`migration_example` and `calc_par_context_example` are the context-API
  references.** They demonstrate creating a [[solver-context]] and driving solves
  through it (the modern path), as opposed to the flat legacy calls the other
  examples show. Use these as the "how to adopt the context API" samples.
- **Sample deals are shared, not duplicated.** The `hands` `cc_library`
  (`hands.cpp`) provides known test deals reused across examples and by the WASM
  `calc_dd_table_pbn_test` — so example outputs are comparable and stable.
- **`all_examples` bundles them** (with `all_examples_wasm` grouping the WASM
  ports); building it is the cheap "do all examples still compile/link" check.

## Key entry points

- `examples/BUILD.bazel` — all example `cc_binary` targets, the `hands` library,
  and the `all_examples` / `all_examples_wasm` groupings.
- `examples/migration_example.cpp`, `examples/calc_par_context_example.cpp` —
  modern context-API usage.
- `examples/hands.cpp` — shared sample deals.
- The paired `*` / `*_pbn` sources for each capability above.

## Known gaps / non-goals

- These are demonstrations and reference code, **not** a supported command-line
  product; input/output formats may be example-specific.
- They add no solver behaviour of their own — they only exercise
  [[dds-public-api]] / [[solver-context]].
- Only a subset is compiled to WASM (see [[wasm-emscripten]]).
