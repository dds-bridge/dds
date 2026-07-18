---
capability: web-mvp
owners: [web]
last-updated: 2026-07-18
---

# Web MVP

> **Specs vs. docs.** Build/patch mechanics overlap with `docs/wasm_build.md`.
> This spec records what the MVP is, how the page and module connect, and what
> each test tier proves.

## Purpose

The web MVP is a minimal browser page that runs the double-dummy solver entirely
client-side via a purpose-built WASM module. It exists to demonstrate the solver
in a browser and to hold the line on the browser integration (module loading,
memory marshalling, DOM wiring) with an automated test pyramid. It is an MVP, not
a full application.

## Behaviour & invariants

> Per-file detail is in the BUILD file and the site sources; these are the
> capability-wide facts.

- **A dedicated, modularised WASM module — not the example CLIs.** `dds_mvp_wasm`
  (`wasm_cc_binary` over `dds_mvp_wasm_cc`, source `dds_mvp_wasm.cpp`) is built
  with `WASM_MVP_LINKOPTS`: `MODULARIZE=1`, `EXPORT_NAME=createDdsModule`,
  a single exported entry `_dds_mvp_calc_table` (plus `_malloc`/`_free`),
  `EXPORTED_RUNTIME_METHODS=['ccall','getValue']`, and `ENVIRONMENT=web,node`. This
  is distinct from [wasm-emscripten](wasm-emscripten.md)'s example ports — the MVP wants one small,
  callable table function, not a CLI. Shared base flags come from `WASM_LINKOPTS`
  ([build-system](build-system.md)).
- **The page is a static trio plus JS glue.** `dds_mvp.html` / `dds_mvp.css` /
  `dds_mvp.js` load the module (`createDdsModule`), marshal a deal into WASM
  memory, call `dds_mvp_calc_table` via `ccall`, and read results with `getValue`.
  `mvp_site` (`tests/mvp_site.py`) serves the site for system/e2e tests. Helper
  scripts `gen_wasm_bin_js.py`, `patch_mvp_wasm.py`, `verify_wasm_js.py` generate
  and sanity-check the JS/wasm glue.
- **Three test tiers, with suite membership as wired in BUILD:**
  - **Unit / JS** (`web_tests`): `dds_mvp_wasm_test` (native `cc_test` over the
    solve logic with `DDS_MVP_WASM_NO_MAIN`), `dds_mvp_js_test` (Node runs
    `dds_mvp.js` against `dds_mvp_test.mjs`), `wasm_scripts_test` (the Python
    helper scripts).
  - **System + e2e bundle** (`web_system_tests`): includes both
    `dds_mvp_wasm_system_test` (stages wasm artifacts and runs a Node smoke via
    `dds_mvp_wasm_node.mjs`) **and** `dds_mvp_e2e_test`.
  - **E2E-only** (`web_e2e_tests`): `dds_mvp_e2e_test` alone — Playwright/Chromium
    against the served page, tagged `e2e`, `no-sandbox`, `requires-network`
    (first run downloads Chromium). Default `.bazelrc` filters `-e2e`.
- **Post-build patch is MVP-owned.** `web/patch_mvp_wasm.py` (driven by
  `./web/update_wasm.sh`) fixes Emscripten's generated `isFileURI` helper for
  browser/`file://` safety on `//web:dds_mvp_wasm` only — not the example WASM
  CLIs. If an emsdk upgrade moves that line, update the regex and the note in
  `docs/wasm_build.md`.
- **The MVP's WASM inherits the single-thread assumption** of
  [wasm-emscripten](wasm-emscripten.md); results come from the same [dds-public-api](dds-public-api.md) core.

## Key entry points

- `web/BUILD.bazel` — `dds_mvp_wasm(_cc)`, `mvp_site`, the five test targets
  (`dds_mvp_wasm_test`, three `py_test`s, `dds_mvp_e2e_test`), and the
  `web_tests` / `web_system_tests` / `web_e2e_tests` suites; `WASM_MVP_LINKOPTS`.
- `web/dds_mvp_wasm.cpp` — the MVP native entry (`dds_mvp_calc_table`).
- `web/{dds_mvp.html,dds_mvp.css,dds_mvp.js}` — the page and JS glue.
- `web/{gen_wasm_bin_js,patch_mvp_wasm,verify_wasm_js}.py` — build/patch helpers.

## Known gaps / non-goals

- **MVP scope only** — one table-calculation flow, not a full bridge app or the
  complete solver API.
- The e2e tier needs network on first run (Chromium download) and is excluded from
  the fast `web_tests` suite.
- Site/module build mechanics that overlap with WASM builds are documented once in
  `docs/wasm_build.md`.
