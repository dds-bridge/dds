// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Solver session for DDS Web: queue, WASM load, DD table, opening leads.
// Loaded after dds_web_core.js and before dds_web.js (UI).

/* eslint-env es6 */
/* exported scheduleDealSolve setDealSolveDebounceMs invalidateActiveDdTableRequest
            wasmSolveEnvironmentError loadDdsModule refreshDdTable refreshOpeningLeadTricks
            clear_results setDdTableComputingDelayMs paintStatusFrame formatSolveTimeMs
            sendJSON */

"use strict";

(function (global) {

    global.selectedContractState = null;
    global.leadTricksByCardKey = null;
    global.leadTricksRequestId = 0;
    global.ddTableRequestId = 0;
    global.ddTableComputingTimer = null;
    global.lastDdTablePbn = null;
    global.solveQueue = Promise.resolve();
    global.dealSolveEpoch = 0;
    global.dealSolveQueued = false;
    global.dealSolvePending = false;
    global.dealSolveDebounceMs = 250;
    global.dealSolveDebounceTimer = null;
    global.lastDealWasComplete = false;
    global.ddsModulePromise = null;
    global.ddTableComputingDelayMs = 300;

    function enqueueSolve(task) {
        const run = solveQueue.then(task, task);

        // Keep the queue alive after a rejected solve.
        solveQueue = run.catch(() => {});
        return run;
    }

    function setDealSolveDebounceMs(ms) {
        dealSolveDebounceMs = ms;

        // Disabling debounce must not leave a previously scheduled trailing solve
        // to fire later with the old delay.
        if (ms <= 0 && dealSolveDebounceTimer != null) {
            clearTimeout(dealSolveDebounceTimer);
            dealSolveDebounceTimer = null;
        }
    }

    function scheduleDealSolveDebounced() {
        if (dealSolveDebounceTimer != null) {
            clearTimeout(dealSolveDebounceTimer);
            dealSolveDebounceTimer = null;
        }

        if (dealSolveDebounceMs <= 0) {
            void global.scheduleDealSolve();
            return;
        }

        dealSolveDebounceTimer = setTimeout(() => {
            dealSolveDebounceTimer = null;
            void global.scheduleDealSolve();
        }, dealSolveDebounceMs);
    }

    // Coalesce DD-table + lead solves onto one queued job so rapid hand edits and
    // contract clicks cannot interleave CalcDDtable with SolveBoard, and so
    // intermediate schedules do not each add a stale promise-chain callback.
    function scheduleDealSolve() {
        // A direct schedule (contract click, etc.) supersedes a pending debounced
        // hand-edit solve so we do not fire a redundant trailing job afterward.
        if (dealSolveDebounceTimer != null) {
            clearTimeout(dealSolveDebounceTimer);
            dealSolveDebounceTimer = null;
        }

        dealSolveEpoch += 1;
        dealSolvePending = true;

        if (dealSolveQueued) {
            return solveQueue;
        }

        dealSolveQueued = true;
        return global.enqueueSolve(async () => {
            try {
                while (true) {
                    const epoch = dealSolveEpoch;
                    dealSolvePending = false;

                    await global.refreshDdTable();
                    if (epoch !== dealSolveEpoch) {
                        // Invalidation alone must not restart; only a newer
                        // scheduleDealSolve (pending) should continue. A pending
                        // debounce will start a fresh job when it fires.
                        if (dealSolvePending) {
                            continue;
                        }
                        break;
                    }

                    if (selectedContractState) {
                        await global.refreshOpeningLeadTricks();
                    } else if (leadTricksByCardKey) {
                        leadTricksByCardKey = null;
                        global.updateHandCardDisplays(global.collectHands());
                    }

                    // Stale+pending → another iteration; else exit (success or
                    // invalidate). Gate release lives in finally.
                    if (epoch !== dealSolveEpoch && dealSolvePending) {
                        continue;
                    }
                    break;
                }
            } finally {
                const restart = dealSolvePending;
                dealSolveQueued = false;
                if (restart) {
                    void global.scheduleDealSolve();
                }
            }
        });
    }

    function wasmSolveEnvironmentError() {
        if (typeof location === "undefined" || !location) {
            return null;
        }

        if (location.protocol === "file:") {
            return "Solving needs HTTP with cross-origin isolation. " +
                "From the repo root run: python3 web/serve_web.py";
        }

        if (typeof SharedArrayBuffer === "undefined") {
            return "Solving needs SharedArrayBuffer (cross-origin isolation). " +
                "Serve responses with Cross-Origin-Opener-Policy: same-origin and " +
                "Cross-Origin-Embedder-Policy: require-corp " +
                "(locally: python3 web/serve_web.py).";
        }

        return null;
    }

    function loadDdsModule() {
        if (typeof createDdsModule !== "function") {
            return Promise.reject(new Error(
                "WASM module not found. From the repo root run: ./web/update_wasm.sh"
            ));
        }

        if (typeof ddsWebWasmBytes !== "function") {
            return Promise.reject(new Error(
                "WASM bytes not found. From the repo root run: ./web/update_wasm.sh"
            ));
        }

        const envError = global.wasmSolveEnvironmentError();

        if (envError) {
            return Promise.reject(new Error(envError));
        }

        if (!ddsModulePromise) {
            ddsModulePromise = createDdsModule({
                wasmBinary: ddsWebWasmBytes()
            }).catch((error) => {
                // Allow retry after transient initialization failures.
                ddsModulePromise = null;
                throw error;
            });
        }

        return ddsModulePromise;
    }

    function invalidateActiveDdTableRequest() {
        ddTableRequestId += 1;
        leadTricksRequestId += 1;
        dealSolveEpoch += 1;
        // Drop a coalesced direct-schedule flag so the worker does not immediately
        // continue after this invalidate; a following scheduleDealSolve() sets it
        // again, while a debounced schedule sets it when its timer fires.
        dealSolvePending = false;
        global.clearDdTableComputingTimer();
        if (dealSolveDebounceTimer != null) {
            clearTimeout(dealSolveDebounceTimer);
            dealSolveDebounceTimer = null;
        }
        const result = document.getElementById("result");
        // Drop a painted Computing… for the abandoned request; keep solved/error text.
        if (result && /Computing/i.test(String(result.innerHTML || ""))) {
            result.innerHTML = "";
        }
    }

    async function solveOpeningLeadTricks(hands, contract) {
        const leader = openingLeader(contract.direction);
        const trump = DENOM_TO_STRAIN[contract.denomination];
        const first = DIR_TO_HAND[leader];

        if (trump == null || first == null) {
            throw new Error("Invalid contract for lead analysis");
        }

        const module = await global.loadDdsModule();
        const pbn = global.handsToPbn(hands);
        const outPtr = module._malloc((1 + 13 * 3) * 4);

        try {
            const rc = module.ccall(
                "dds_web_solve_leads",
                "number",
                ["string", "number", "number", "number"],
                [pbn, trump, first, outPtr]
            );

            if (rc !== 1) {
                throw new Error("DDS lead solve error (code " + rc + ")");
            }

            const n = module.getValue(outPtr, "i32");
            if (n < 0 || n > 13) {
                throw new Error(
                    "DDS lead solve returned invalid card count (" + n + ")"
                );
            }
            const out = [n];

            for (let i = 0; i < n; i++) {
                const base = outPtr + (1 + 3 * i) * 4;
                out.push(module.getValue(base, "i32"));
                out.push(module.getValue(base + 4, "i32"));
                out.push(module.getValue(base + 8, "i32"));
            }

            return leadTricksMapFromSolverOutput(out);
        } finally {
            module._free(outPtr);
        }
    }

    async function refreshOpeningLeadTricks() {
        const requestId = ++leadTricksRequestId;
        const contract = selectedContractState;
        const hands = global.collectHands();

        if (!contract || global.inputIsValid(hands).length) {
            leadTricksByCardKey = null;
            if (requestId === leadTricksRequestId) {
                global.updateHandCardDisplays(hands);
            }
            return;
        }

        try {
            const map = await global.solveOpeningLeadTricks(hands, contract);

            if (requestId !== leadTricksRequestId) {
                return;
            }

            leadTricksByCardKey = map;
            global.updateHandCardDisplays(global.collectHands());
        } catch (err) {
            if (requestId !== leadTricksRequestId) {
                return;
            }

            leadTricksByCardKey = null;
            global.updateHandCardDisplays(global.collectHands());

            const result = document.getElementById("result");

            if (result) {
                result.innerHTML = err instanceof Error
                    ? err.message
                    : err == null
                        ? "Unknown error"
                        : String(err);
            }
        }
    }

    function clear_results() {
        var result = document.getElementById("result");
        var result_table = document.getElementById("result-table");

        global.clearDdTableComputingTimer();
        lastDdTablePbn = null;
        result.innerHTML = "";

        for (var row = 1; row <= 4; row++) {
            for (var column = 1; column <= 5; column++) {
                var cell = result_table.rows[row].cells[column];
                cell.innerHTML = "";
            }
        }
    }

    /** Delay before showing Computing… under the DD matrix (see refreshDdTable). */

    function setDdTableComputingDelayMs(ms) {
        ddTableComputingDelayMs = ms;
    }

    function clearDdTableComputingTimer() {
        if (ddTableComputingTimer != null) {
            clearTimeout(ddTableComputingTimer);
            ddTableComputingTimer = null;
        }
    }

    function scheduleDdTableComputingMessage(requestId, result) {
        global.clearDdTableComputingTimer();
        if (ddTableComputingDelayMs <= 0) {
            if (result) {
                result.innerHTML = "Computing&hellip;";
            }
            return;
        }
        ddTableComputingTimer = setTimeout(() => {
            ddTableComputingTimer = null;
            if (requestId !== ddTableRequestId || !result) {
                return;
            }
            result.innerHTML = "Computing&hellip;"; // horizontal ellipsis
        }, ddTableComputingDelayMs);
    }

    /** Yield until the browser has painted the current status (needed before sync ccall). */
    function paintStatusFrame() {
        // Hidden tabs often pause rAF; do not block the solve queue forever.
        if (
            typeof document !== "undefined"
            && document.visibilityState === "hidden"
        ) {
            return Promise.resolve();
        }
        if (typeof requestAnimationFrame === "function") {
            return new Promise((resolve) => {
                let settled = false;
                const done = () => {
                    if (settled) {
                        return;
                    }
                    settled = true;
                    clearTimeout(fallbackTimer);
                    resolve();
                };
                // If the tab hides mid-wait, the second rAF may never run.
                const fallbackTimer = setTimeout(done, 50);
                requestAnimationFrame(() => {
                    if (
                        typeof document !== "undefined"
                        && document.visibilityState === "hidden"
                    ) {
                        done();
                        return;
                    }
                    requestAnimationFrame(done);
                });
            });
        }
        return new Promise((resolve) => setTimeout(resolve, 16));
    }

    /**
     * Show Computing… and wait for a paint. The WASM ccall is synchronous and
     * blocks timers, so this must run before ccall or the message is never seen.
     */
    async function showComputingStatus(requestId, result) {
        global.clearDdTableComputingTimer();
        if (requestId !== ddTableRequestId || !result) {
            return false;
        }
        result.innerHTML = "Computing&hellip;"; // horizontal ellipsis
        await global.paintStatusFrame();
        return requestId === ddTableRequestId;
    }

    /** Format wall elapsed time for the status line (whole milliseconds). */
    function formatSolveTimeMs(elapsedMs) {
        return "Solved in " + Math.round(elapsedMs) + " ms.";
    }

    async function refreshDdTable() {
        const requestId = ++ddTableRequestId;
        const result = document.getElementById("result");
        const result_table = document.getElementById("result-table");
        const hands = global.collectHands();

        if (!global.allHandsHaveThirteenCards(hands)) {
            if (requestId === ddTableRequestId) {
                lastDdTablePbn = null;
                global.clear_results();
            }
            return;
        }

        const error_message = global.inputIsValid(hands);

        if (error_message.length) {
            if (requestId === ddTableRequestId) {
                lastDdTablePbn = null;
                global.clear_results();
                if (result) {
                    result.innerHTML = error_message;
                }
            }
            return;
        }

        const pbn = global.handsToPbn(hands);

        if (pbn === lastDdTablePbn && global.ddTableLooksPopulated(result_table)) {
            return;
        }

        if (requestId !== ddTableRequestId) {
            return;
        }

        global.clear_results();
        // Computing… grace / pre-ccall paint tradeoff (intentional until CalcTable
        // runs off the main thread):
        // The WASM ccall is synchronous and blocks timers and rAF, so a timer-only
        // Computing… message can never appear during a long solve. Painting before
        // ccall is the only way to show status while the UI is frozen. That means
        // uncached solves wait for any remaining grace period and briefly show
        // Computing… even when ccall itself would be fast — a minimum-latency tax
        // preferred over silent multi-second freezes. Module load time counts
        // toward the grace. Tests set the delay to 0.
        global.scheduleDdTableComputingMessage(requestId, result);
        const waitStartedAt = performance.now();

        try {
            const module = await global.loadDdsModule();
            const outPtr = module._malloc(20 * 4);

            try {
                const remainingMs =
                    ddTableComputingDelayMs - (performance.now() - waitStartedAt);
                if (remainingMs > 0) {
                    await new Promise((resolve) => setTimeout(resolve, remainingMs));
                    if (requestId !== ddTableRequestId) {
                        return;
                    }
                }

                // An import/edit during the grace wait can change the diagram while
                // this invocation still holds the old PBN; do not solve stale input.
                if (global.handsToPbn(global.collectHands()) !== pbn) {
                    global.clearDdTableComputingTimer();
                    if (requestId === ddTableRequestId && result) {
                        result.innerHTML = "";
                    }
                    return;
                }

                if (!(await global.showComputingStatus(requestId, result))) {
                    return;
                }

                if (global.handsToPbn(global.collectHands()) !== pbn) {
                    global.clearDdTableComputingTimer();
                    if (requestId === ddTableRequestId && result) {
                        result.innerHTML = "";
                    }
                    return;
                }

                const startedAt = performance.now();
                const rc = module.ccall(
                    "dds_web_calc_table",
                    "number",
                    ["string", "number"],
                    [pbn, outPtr]
                );
                const elapsedMs = performance.now() - startedAt;

                if (requestId !== ddTableRequestId) {
                    return;
                }

                global.clearDdTableComputingTimer();

                if (rc !== 1) {
                    lastDdTablePbn = null;
                    if (result) {
                        result.innerHTML = "DDS error (code " + rc + ").";
                    }
                    return;
                }

                for (var row = 1; row <= 4; row++) {
                    for (var column = 1; column <= 5; column++) {
                        const cell = result_table.rows[row].cells[column];
                        const denomination = DENOMINATIONS[column - 1];
                        const direction = DIRECTIONS[row - 1];
                        const strain = DENOM_TO_STRAIN[denomination];
                        const hand = DIR_TO_HAND[direction];
                        const index = strain * 4 + hand;
                        cell.innerHTML = module.getValue(
                            outPtr + index * 4,
                            "i32"
                        );
                    }
                }

                lastDdTablePbn = pbn;

                if (result) {
                    result.innerHTML = global.formatSolveTimeMs(elapsedMs);
                }
            } finally {
                module._free(outPtr);
            }
        } catch (err) {
            if (requestId !== ddTableRequestId) {
                return;
            }

            lastDdTablePbn = null;
            global.clear_results();
            if (result) {
                result.innerHTML = err instanceof Error
                    ? err.message
                    : err == null
                        ? "Unknown error"
                        : String(err);
            }
        }
    }

    function ddTableLooksPopulated(result_table) {
        if (!result_table || !result_table.rows || !result_table.rows[1]) {
            return false;
        }

        const cell = result_table.rows[1].cells[1];

        return !!(cell && cell.innerHTML && /\d/.test(String(cell.innerHTML)));
    }

    function sendJSON() {
        return global.refreshDdTable();
    }

    global.enqueueSolve = enqueueSolve;
    global.setDealSolveDebounceMs = setDealSolveDebounceMs;
    global.scheduleDealSolveDebounced = scheduleDealSolveDebounced;
    global.scheduleDealSolve = scheduleDealSolve;
    global.wasmSolveEnvironmentError = wasmSolveEnvironmentError;
    global.loadDdsModule = loadDdsModule;
    global.invalidateActiveDdTableRequest = invalidateActiveDdTableRequest;
    global.solveOpeningLeadTricks = solveOpeningLeadTricks;
    global.refreshOpeningLeadTricks = refreshOpeningLeadTricks;
    global.clear_results = clear_results;
    global.setDdTableComputingDelayMs = setDdTableComputingDelayMs;
    global.clearDdTableComputingTimer = clearDdTableComputingTimer;
    global.scheduleDdTableComputingMessage = scheduleDdTableComputingMessage;
    global.paintStatusFrame = paintStatusFrame;
    global.showComputingStatus = showComputingStatus;
    global.formatSolveTimeMs = formatSolveTimeMs;
    global.refreshDdTable = refreshDdTable;
    global.ddTableLooksPopulated = ddTableLooksPopulated;
    global.sendJSON = sendJSON;
})(typeof globalThis !== "undefined" ? globalThis : this);
