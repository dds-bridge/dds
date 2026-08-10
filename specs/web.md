---
capability: web
owners: [web]
last-updated: 2026-08-10
---

# DDS Web

> **Specs vs. docs.** Build/patch mechanics overlap with `docs/wasm_build.md`.
> This spec records what DDS Web is, how the page and module connect, and what
> each test tier proves.

## Purpose

DDS Web is a browser page that runs the double-dummy solver entirely client-side
via a purpose-built WASM module. It demonstrates the solver in a browser and
holds the line on the browser integration (module loading, memory marshalling,
DOM wiring) with an automated test pyramid.

## Behaviour & invariants

> Per-file detail is in the BUILD file and the site sources; these are the
> capability-wide facts.

- **A dedicated, modularised WASM module — not the example CLIs.** `dds_web_wasm`
  (`wasm_cc_binary` over `dds_web_wasm_cc`, source `dds_web_wasm.cpp`,
  `threads = "emscripten"`) is built with `WASM_WEB_LINKOPTS`: `MODULARIZE=1`,
  `EXPORT_NAME=createDdsModule`, a single exported entry `_dds_web_calc_table`
  (plus `_malloc`/`_free`), `EXPORTED_RUNTIME_METHODS=['ccall','getValue']`, and
  `ENVIRONMENT=web,worker,node`. This is distinct from
  [wasm-emscripten](wasm-emscripten.md)'s example ports — DDS Web wants one small,
  callable table function, not a CLI. Shared base flags come from `WASM_LINKOPTS`
  ([build-system](build-system.md)), including pthreads / `PTHREAD_POOL_SIZE`.
- **The page is a static trio plus JS glue.** `dds_web.html` / `dds_web.css` /
  `dds_web.js` load the module (`createDdsModule`), marshal a deal into WASM
  memory, call `dds_web_calc_table` via `ccall`, and read results with `getValue`.
  `web_site` (`tests/web_site.py`) stages the site and provides
  `make_isolated_http_handler` (COOP/COEP) for system/e2e tests and
  `web/serve_web.py`. Helper scripts `gen_wasm_bin_js.py`, `patch_web_wasm.py`,
  `verify_wasm_js.py` generate and sanity-check the JS/wasm glue.
- **Browser solves require cross-origin isolation.** Pthread WASM needs
  `SharedArrayBuffer`, which Chromium only exposes when
  `Cross-Origin-Opener-Policy: same-origin` and
  `Cross-Origin-Embedder-Policy: require-corp` are set. Serve with
  `python3 web/serve_web.py` (not plain `http.server`). `file://` remains useful
  for UI-only checks; instantiating the solver module for a solve needs HTTP +
  those headers on any host. Guarded by `dds_web_e2e_test` (`test_http_is_cross_origin_isolated`,
  HTTP part-score solve / auto-filled DD table). GitHub Pages cannot set those
  headers; `coi-serviceworker.js` (loaded from `dds_web.html`) injects them via
  a service worker. Staging for Pages is `web/stage_github_pages.py`; deploy is
  `.github/workflows/deploy_pages.yml`.
- **Three test tiers, with suite membership as wired in BUILD:**
  - **Unit / JS** (`web_tests`): `dds_web_wasm_test` (native `cc_test` over the
    solve logic with `DDS_WEB_WASM_NO_MAIN`), `dds_web_js_test` (Node runs
    `dds_web.js` against `dds_web_test.mjs`), `wasm_scripts_test` (the Python
    helper scripts + isolation-header constants).
  - **System + e2e bundle** (`web_system_tests`): includes both
    `dds_web_wasm_system_test` (stages wasm artifacts and runs a Node smoke via
    `dds_web_wasm_node.mjs`) **and** `dds_web_e2e_test`.
  - **E2E-only** (`web_e2e_tests`): `dds_web_e2e_test` alone — Playwright/Chromium
    against the served page, tagged `e2e`, `no-sandbox`, `requires-network`
    (first run downloads Chromium). Default `.bazelrc` filters `-e2e`; WASM CI
    (`ci_wasm.yml`) clears that filter so `web_system_tests` runs e2e there.
- **Post-build patch is web-owned.** `web/patch_web_wasm.py` (driven by
  `./web/update_wasm.sh`) fixes Emscripten's generated `isFileURI` helper for
  browser/`file://` safety on `//web:dds_web_wasm` only — not the example WASM
  CLIs. If an emsdk upgrade moves that line, update the regex and the note in
  `docs/wasm_build.md`.
- **The module holds one session-scoped `SolverContext` for its lifetime.**
  `dds_web_calc_table` (`web/dds_web_wasm.cpp`) constructs a function-local
  `static SolverContext` lazily on first call and reuses it — including its
  transposition table — for every subsequent call in that module instance,
  calling `reset_for_solve()` between deals to recycle the TT memory pool
  without freeing the underlying allocation. This mirrors a WASM module
  instance's own lifetime (one `WebAssembly.Memory`, static constructors run
  once) instead of allocating a fresh [solver-context](solver-context.md) per
  call. Because the context is shared, it is **not safe for concurrent
  solves** — a future move to Web Workers would need one context per worker.
  The DD-table path itself remains sequential over strains; batch/multi-hand
  APIs under [wasm-emscripten](wasm-emscripten.md) can use pthreads via
  [system-concurrency](system-concurrency.md).

## Key entry points

- `web/BUILD.bazel` — `dds_web_wasm(_cc)`, `web_site`, the test targets
  (`dds_web_wasm_test`, py_tests, `dds_web_e2e_test`), and the
  `web_tests` / `web_system_tests` / `web_e2e_tests` suites; `WASM_WEB_LINKOPTS`.
- `web/dds_web_wasm.cpp` — the native WASM bridge (`dds_web_calc_table`).
- `web/{dds_web.html,dds_web.css,dds_web.js}` — the page and JS glue.
- `web/coi-serviceworker.js` — COOP/COEP via service worker for hosts without
  custom headers (GitHub Pages).
- `web/stage_github_pages.py` — stage static site + `index.html` for Pages.
- `web/serve_web.py` — local HTTP server with COOP/COEP.
- `web/{gen_wasm_bin_js,patch_web_wasm,verify_wasm_js}.py` — build/patch helpers.

## Known gaps / non-goals

- **Not a full bridge app** — focused on deal entry, DD table, and lead analysis,
  not the complete solver API.
- The e2e tier needs network on first run (Chromium download) and is excluded from
  the fast `web_tests` suite.
- Site/module build mechanics that overlap with WASM builds are documented once in
  `docs/wasm_build.md`.
