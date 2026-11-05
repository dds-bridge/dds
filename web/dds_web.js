// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Layers (load before this UI file):
//   web/dds_web_deal_import.js — file-format parsers
//   web/dds_web_pdf_import.js — BridgeComposer / ACBL hand-record PDFs
//   web/dds_web_core.js — Card / holdings / handsToPbn
//   web/dds_web_solve.js — WASM queue / DD table / play solves
//   web/dds_web_play.js — card-by-card play state / trick UI
//   web/dds_web_sample_deals.js — SAMPLE_DEALS (from hands/*.pbn via gen_sample_deals_js.py)
// Unit tests: web/tests/dds_web_test.mjs
// Run with: bazelisk test //web:dds_web_js_test
// or: python -m unittest web.tests.test_dds_web_js
// or: node --test web/tests/dds_web_test.mjs

// ESLint configuration
// https://eslint.org/demo
//     ECMA Version: 2015
//     Environment: browser

/* eslint-env es6 */
/* exported clearTestData
            rotateClockwise
            pageLoad
            updateActionButtons
            sanitizeHandSuitInputs
            playIllegalInputBeep
            handleHandSuitInput
            handCardHtml
            undeployedCardHtml
            handHoldingHtml
            escapeHtml
            fitCenterDeckCards
            fitSouthHandCards
            fitEastHandCards
            fitDiagramFonts
            updateHandCardDisplays
            addCardToHand
            removeCardFromHand
            removeCardFromAllHands
            undeployCard
            moveCardToHand
            handContainsCard
            handCardCount
            handleHandCardClick
            handleHandSuitClick
            handleHandCardMouseDown
            handleCardDragStart
            handleCardDragOver
            handleCardDragLeave
            handleCardDrop
            handleCardDragEnd
            handleSuitSelectionChange
            onHandCardClick
            handleResultTableClick
            handleResultTableKeyDown
            selectedContract
            onContractSelect
            ensurePlayForSelectedContract
            denominationDisplayHtml
            contractStatusHtml
            updateContractStatus
            updateResultTableHint
            importDealFromText
            chooseDealFile
            handleDealFileSelected
            fillFormWithTestData
            loadSampleDealById
            populateSampleDealsMenu
            mountMenubar
            updateEditMenuEnablement
            handRecordPrev
            handRecordNext
            showHandRecordBoard
            clearHandRecordSession
            */

// It's also useful to pass the code through
// https://jshint.com/ and https://jslint.com/

"use strict";

function focusNorthSpades() {
    // To allow the user to quickly enter a deal

    document.getElementById("north_spades").focus();
}

function fillFormWithTestData(nesw) {
    if (typeof exitPlay === "function") {
        exitPlay();
    }
    clear_results();

    var holdings = [];

    for (const hand of nesw) {
        for (const holding of hand.split(".")) {
            holdings.push(holding);
        }
    }

    for (const element of hand_elements()) {
        element.value = holdings.shift();
    }

    updateActionButtons();
}

const DDS_ABOUT_TEXT =
    "DDS Web is a front end for DDS, a double-dummy bridge solver. "
    + "Enter or import a deal and the page fills a table of maximum tricks by "
    + "declarer and strain assuming optimal play and defense. "
    + "Click on a cell and you can play through the contract card by card. "
    + "Solving runs entirely in your browser via WebAssembly.\n\n"
    + "Bo Haglund wrote the original version of DDS. "
    + "Adam Wildavsky wrote this web front end.";

function loadSampleDealById(id) {
    const sample = SAMPLE_DEALS[id];
    if (!sample) {
        return;
    }
    clearHandRecordSession();
    // SAMPLE_DEALS.pbn is generated from hands/*.pbn; reuse the import parser.
    const deal = parseFirstDealFromText(`[Deal "${sample.pbn}"]`);
    fillFormWithTestData([
        deal.north,
        deal.east,
        deal.south,
        deal.west,
    ]);
}

function populateSampleDealsMenu() {
    const panel = document.getElementById("menu-sample-deals-panel");
    if (!panel) {
        return;
    }
    panel.textContent = "";
    for (const id of Object.keys(SAMPLE_DEALS)) {
        const deal = SAMPLE_DEALS[id];
        const item = document.createElement("button");
        item.type = "button";
        item.setAttribute("role", "menuitem");
        item.dataset.sampleId = id;
        item.textContent = deal.name;
        panel.appendChild(item);
    }
}

function isMacPlatform(nav) {
    const n = nav || (typeof navigator !== "undefined" ? navigator : {});
    const platform = n.platform || "";
    const ua = n.userAgent || "";
    return /Mac|iPhone|iPad|iPod/.test(platform) || /Mac OS X/.test(ua);
}

function formatMenuShortcut(spec, options) {
    const mac = options && options.mac != null ? options.mac : isMacPlatform();
    const parts = String(spec).toLowerCase().split("+").map(function (p) {
        return p.trim();
    }).filter(Boolean);
    const hasMod = parts.indexOf("mod") >= 0;
    const hasShift = parts.indexOf("shift") >= 0;
    const hasAlt = parts.indexOf("alt") >= 0;
    const key = parts.find(function (p) {
        return p !== "mod" && p !== "shift" && p !== "alt" && p !== "ctrl";
    });
    function formatKey(k) {
        if (!k) {
            return "";
        }
        return k.length === 1 ? k.toUpperCase() : k;
    }
    if (mac) {
        let out = "";
        if (hasAlt) {
            out += "⌥";
        }
        if (hasShift) {
            out += "⇧";
        }
        if (hasMod) {
            out += "⌘";
        }
        out += formatKey(key);
        return out;
    }
    const bits = [];
    if (hasMod) {
        bits.push("Ctrl");
    }
    if (hasAlt) {
        bits.push("Alt");
    }
    if (hasShift) {
        bits.push("Shift");
    }
    const keyLabel = formatKey(key);
    if (keyLabel) {
        bits.push(keyLabel);
    }
    return bits.join("+");
}

function applyMenuShortcuts(root) {
    if (!root || !root.querySelectorAll) {
        return;
    }
    const mac = isMacPlatform();
    const nodes = root.querySelectorAll(".menu-shortcut[data-keys]");
    for (let i = 0; i < nodes.length; i++) {
        const el = nodes[i];
        const spec = el.getAttribute("data-keys");
        if (!spec) {
            continue;
        }
        el.textContent = formatMenuShortcut(spec, { mac: mac });
    }
}

function setMenuOpen(menu, open) {
    if (!menu) {
        return;
    }
    menu.classList.toggle("is-open", open);
    const panel = menu.querySelector(".menu-panel:not(.menu-submenu)");
    const label = menu.querySelector(".menu-label");
    if (panel) {
        panel.hidden = !open;
    }
    if (label) {
        label.setAttribute("aria-expanded", open ? "true" : "false");
    }
    if (!open) {
        const submenu = menu.querySelector(".menu-submenu");
        const submenuTrigger = menu.querySelector("#menu-sample-deals");
        if (submenu) {
            submenu.hidden = true;
        }
        if (submenuTrigger) {
            submenuTrigger.setAttribute("aria-expanded", "false");
        }
    }
}

function setSampleSubmenuOpen(open) {
    const submenu = document.getElementById("menu-sample-deals-panel");
    const trigger = document.getElementById("menu-sample-deals");
    if (submenu) {
        submenu.hidden = !open;
    }
    if (trigger) {
        trigger.setAttribute("aria-expanded", open ? "true" : "false");
    }
}

function menubarItemsInPanel(panel) {
    if (!panel || !panel.querySelectorAll) {
        return [];
    }
    const nodes = panel.querySelectorAll('[role="menuitem"]');
    const items = [];
    for (let i = 0; i < nodes.length; i++) {
        const item = nodes[i];
        if (item.disabled) {
            continue;
        }
        // When scanning a parent panel, skip items that live in a nested submenu.
        if (item.closest) {
            const nested = item.closest(".menu-submenu");
            if (nested && nested !== panel) {
                continue;
            }
        }
        items.push(item);
    }
    return items;
}

function focusAdjacentMenubarItem(panel, current, delta) {
    const items = menubarItemsInPanel(panel);
    if (!items.length) {
        return;
    }
    let index = items.indexOf(current);
    if (index < 0) {
        index = delta > 0 ? -1 : 0;
    }
    const next = items[(index + delta + items.length) % items.length];
    if (next && typeof next.focus === "function") {
        next.focus();
    }
}

function showAboutDialog() {
    const dialog = document.getElementById("dds-about-dialog");
    const body = document.getElementById("dds-about-body");
    if (!dialog) {
        return;
    }
    if (body) {
        body.textContent = DDS_ABOUT_TEXT;
    }
    if (typeof dialog.showModal === "function") {
        dialog.showModal();
    } else {
        dialog.setAttribute("open", "");
    }
}

function updateEditMenuEnablement() {
    const undo = document.getElementById("menu-undo");
    const editHands = document.getElementById("menu-edit-hands");
    const playing = typeof isPlayMode === "function" && isPlayMode();
    if (editHands) {
        editHands.disabled = !playing;
    }
    if (undo) {
        const canUndo = typeof canUndoPlay === "function" && canUndoPlay();
        undo.disabled = !canUndo;
    }
}

function mountMenubar(menubar) {
    if (!menubar) {
        return function () {};
    }
    const menus = menubar.querySelectorAll
        ? Array.prototype.slice.call(menubar.querySelectorAll(".menu"))
        : [];

    function closeAll() {
        for (let i = 0; i < menus.length; i++) {
            setMenuOpen(menus[i], false);
        }
        setSampleSubmenuOpen(false);
    }

    function onMenubarClick(event) {
        const target = event.target;
        if (!target || !target.closest) {
            return;
        }
        const label = target.closest(".menu-label");
        if (label && menubar.contains(label)) {
            event.preventDefault();
            const menu = label.closest(".menu");
            const willOpen = !menu.classList.contains("is-open");
            closeAll();
            if (willOpen) {
                setMenuOpen(menu, true);
            }
            return;
        }

        const sampleTrigger = target.closest("#menu-sample-deals");
        if (sampleTrigger && menubar.contains(sampleTrigger)) {
            event.preventDefault();
            // Selecting Sample Deals always opens the flyout (macOS submenu).
            setSampleSubmenuOpen(true);
            return;
        }

        const item = target.closest("[role=\"menuitem\"]");
        if (!item || !menubar.contains(item) || item.disabled) {
            return;
        }

        const sampleId = item.dataset && item.dataset.sampleId;
        if (sampleId) {
            loadSampleDealById(sampleId);
            closeAll();
            return;
        }

        if (item.id === "menu-about") {
            showAboutDialog();
        } else if (
            item.id === "menu-how-to"
            || item.id === "menu-source-code"
            || item.id === "menu-report-issue"
        ) {
            // Anchor menuitems — close the menu and let the browser follow href.
            closeAll();
            return;
        } else if (item.id === "menu-import-deals") {
            chooseDealFile();
        } else if (item.id === "menu-undo") {
            if (typeof undoPlay === "function") {
                undoPlay();
            }
        } else if (item.id === "menu-rotate") {
            rotateClockwise();
        } else if (item.id === "menu-clear") {
            clearTestData();
        } else if (item.id === "menu-edit-hands") {
            if (typeof exitPlay === "function") {
                exitPlay();
            }
        }
        closeAll();
    }

    function onDocumentPointerDown(event) {
        if (menubar.contains(event.target)) {
            return;
        }
        closeAll();
    }

    function openMenu() {
        for (let i = 0; i < menus.length; i++) {
            if (menus[i].classList.contains("is-open")) {
                return menus[i];
            }
        }
        return null;
    }

    function sampleSubmenuIsOpen() {
        const submenu = document.getElementById("menu-sample-deals-panel");
        return !!(submenu && !submenu.hidden);
    }

    function onKeyDown(event) {
        const key = event.key;
        const menu = openMenu();

        if (key === "Escape") {
            if (sampleSubmenuIsOpen()) {
                setSampleSubmenuOpen(false);
                const sampleTrigger = document.getElementById("menu-sample-deals");
                if (sampleTrigger && typeof sampleTrigger.focus === "function") {
                    sampleTrigger.focus();
                }
                if (typeof event.preventDefault === "function") {
                    event.preventDefault();
                }
                return;
            }
            const trigger = menu ? menu.querySelector(".menu-label") : null;
            closeAll();
            if (trigger && typeof trigger.focus === "function") {
                trigger.focus();
            }
            if (typeof event.preventDefault === "function") {
                event.preventDefault();
            }
            return;
        }

        if (!menu) {
            return;
        }

        const mainPanel = menu.querySelector(".menu-panel:not(.menu-submenu)");
        const submenu = document.getElementById("menu-sample-deals-panel");
        const active = document.activeElement;
        const inSubmenu = !!(
            sampleSubmenuIsOpen()
            && submenu
            && (
                (active && submenu.contains && submenu.contains(active))
                || (active && active.id === "menu-sample-deals")
            )
        );
        const panel = inSubmenu && key !== "ArrowRight" ? submenu : mainPanel;

        if (key === "ArrowDown") {
            if (typeof event.preventDefault === "function") {
                event.preventDefault();
            }
            focusAdjacentMenubarItem(panel || mainPanel, active, 1);
            return;
        }
        if (key === "ArrowUp") {
            if (typeof event.preventDefault === "function") {
                event.preventDefault();
            }
            focusAdjacentMenubarItem(panel || mainPanel, active, -1);
            return;
        }
        if (key === "ArrowRight") {
            if (active && active.id === "menu-sample-deals") {
                if (typeof event.preventDefault === "function") {
                    event.preventDefault();
                }
                setSampleSubmenuOpen(true);
                const first = menubarItemsInPanel(submenu)[0];
                if (first && typeof first.focus === "function") {
                    first.focus();
                }
            }
            return;
        }
        if (key === "ArrowLeft") {
            if (sampleSubmenuIsOpen()) {
                if (typeof event.preventDefault === "function") {
                    event.preventDefault();
                }
                setSampleSubmenuOpen(false);
                const sampleTrigger = document.getElementById("menu-sample-deals");
                if (sampleTrigger && typeof sampleTrigger.focus === "function") {
                    sampleTrigger.focus();
                }
            }
        }
    }

    function onOpenShortcut(event) {
        if (!event || event.shiftKey || event.altKey) {
            return;
        }
        const key = event.key && String(event.key).toLowerCase();
        if (key !== "o") {
            return;
        }
        if (!(event.metaKey || event.ctrlKey)) {
            return;
        }
        event.preventDefault();
        chooseDealFile();
    }

    function fileMenuIsOpen() {
        const fileMenu = menubar.querySelector
            ? menubar.querySelector('[data-menu="file"]')
            : null;
        return !!(fileMenu && fileMenu.classList.contains("is-open"));
    }

    function onMenubarPointerOver(event) {
        const target = event.target;
        if (!target || !target.closest || !fileMenuIsOpen()) {
            return;
        }
        if (
            target.closest("#menu-sample-deals")
            || target.closest("#menu-sample-deals-panel")
            || target.closest(".menu-item-submenu")
        ) {
            setSampleSubmenuOpen(true);
            return;
        }
        // Hovering other File items dismisses the Sample Deals flyout.
        if (target.closest('[data-menu="file"]')) {
            setSampleSubmenuOpen(false);
        }
    }

    menubar.addEventListener("click", onMenubarClick);
    menubar.addEventListener("pointerover", onMenubarPointerOver);
    document.addEventListener("pointerdown", onDocumentPointerDown);
    document.addEventListener("keydown", onKeyDown);
    document.addEventListener("keydown", onOpenShortcut);

    return function () {
        menubar.removeEventListener("click", onMenubarClick);
        menubar.removeEventListener("pointerover", onMenubarPointerOver);
        document.removeEventListener("pointerdown", onDocumentPointerDown);
        document.removeEventListener("keydown", onKeyDown);
        document.removeEventListener("keydown", onOpenShortcut);
    };
}

// Deal-format parsers (PBN/LIN/DLM/sol) live in dds_web_deal_import.js and
// must be loaded first; importDealFromText uses global parseAllDealsFromText.

function dottedDealToSuitHands(deal) {
    const hands = {};
    for (const direction of DIRECTIONS) {
        const parts = String(deal[direction] || "").split(".");
        hands[direction] = {
            spades: parts[0] || "",
            hearts: parts[1] || "",
            diamonds: parts[2] || "",
            clubs: parts[3] || "",
        };
    }
    return hands;
}

function importDealFromText(text) {
    try {
        const entries = parseAllDealsFromText(text);
        if (entries.length === 0) {
            return "No PBN, LIN, DLM, dtest, or sol-style deal found in the file.";
        }
        if (entries.length === 1) {
            clearHandRecordSession();
            const deal = entries[0].deal;
            fillFormWithTestData([
                deal.north,
                deal.east,
                deal.south,
                deal.west,
            ]);
            return "";
        }
        handRecordBoards = entries.map(function (entry) {
            return {
                boardNumber: entry.boardNumber,
                hands: dottedDealToSuitHands(entry.deal),
            };
        });
        handRecordIndex = 0;
        showHandRecordBoard(0);
        return "";
    } catch (err) {
        return err && err.message ? err.message : "Could not import deal.";
    }
}

// var so Node VM tests can assign ctx.handRecordBoards / handRecordIndex.
var handRecordBoards = [];
var handRecordIndex = 0;

function updateHandRecordAreaVisibility() {
    const area = document.getElementById("hand-record");
    if (!area) {
        return;
    }
    const status = document.getElementById("hand-record-status");
    const statusText = status ? String(status.textContent || "").trim() : "";
    const isError = !!(status && status.classList.contains("error"));
    // Multi-board nav, import progress (no boards yet), or an error.
    area.hidden = !(
        handRecordBoards.length > 1
        || isError
        || (handRecordBoards.length === 0 && statusText !== "")
    );
}

function setHandRecordStatus(message, isError, statusGeneration) {
    // Optional generation: stale PDF loaders must not clobber a newer import's
    // progress or clear it on abort.
    if (
        statusGeneration != null
        && statusGeneration !== dealFileSelectionGeneration
    ) {
        return;
    }
    const el = document.getElementById("hand-record-status");
    if (!el) {
        return;
    }
    el.textContent = message || "";
    el.className = isError ? "hand-record-status error" : "hand-record-status";
    updateHandRecordAreaVisibility();
}

function updateHandRecordNav() {
    const nav = document.getElementById("hand-record-nav");
    const label = document.getElementById("hand-record-label");
    const prev = document.getElementById("hand-record-prev");
    const next = document.getElementById("hand-record-next");
    if (!nav || !label || !prev || !next) {
        return;
    }
    // Prev/next only matter when more than one board was imported.
    if (handRecordBoards.length <= 1) {
        nav.hidden = true;
        label.textContent = "";
        prev.disabled = true;
        next.disabled = true;
        updateHandRecordAreaVisibility();
        return;
    }
    nav.hidden = false;
    const board = handRecordBoards[handRecordIndex];
    label.textContent =
        "Board " + board.boardNumber + " of " + handRecordBoards.length;
    prev.disabled = handRecordIndex <= 0;
    next.disabled = handRecordIndex >= handRecordBoards.length - 1;
    updateHandRecordAreaVisibility();
}

function clearHandRecordSession() {
    handRecordBoards = [];
    handRecordIndex = 0;
    updateHandRecordNav();
    setHandRecordStatus("", false);
}

function showHandRecordBoard(index) {
    if (index < 0 || index >= handRecordBoards.length) {
        return;
    }
    handRecordIndex = index;
    const board = handRecordBoards[handRecordIndex];
    fillFormWithTestData(handsToDottedDeal(board.hands));
    updateHandRecordNav();
    setHandRecordStatus("", false);
}
function handRecordPrev() {
    if (handRecordIndex <= 0) {
        return;
    }
    showHandRecordBoard(handRecordIndex - 1);
}

function handRecordNext() {
    if (handRecordIndex >= handRecordBoards.length - 1) {
        return;
    }
    showHandRecordBoard(handRecordIndex + 1);
}

function isHandRecordPdfFile(file) {
    if (!file) {
        return false;
    }
    const name = String(file.name || "").toLowerCase();
    return file.type === "application/pdf" || name.endsWith(".pdf");
}

/**
 * Read and parse a hand-record PDF. Does not apply boards to the diagram —
 * callers must do that after confirming the file selection is still current.
 * @param {File|{arrayBuffer:function():Promise<ArrayBuffer>,name?:string,type?:string}} file
 * @param {{isCurrent?:function():boolean, statusGeneration?:number}=} options
 * @returns {Promise<object[]|null>} parsed boards, or null if superseded mid-load
 */
function loadHandRecordFromFile(file, options) {
    const loadAll = globalThis.loadAllBoardsFromHandRecordPdf;
    if (typeof loadAll !== "function") {
        return Promise.reject(new Error(
            "PDF import unavailable (dds_web_pdf_import.js failed to load)"
        ));
    }
    const isCurrent = options && typeof options.isCurrent === "function"
        ? options.isCurrent
        : null;
    const statusGeneration = options && options.statusGeneration != null
        ? options.statusGeneration
        : dealFileSelectionGeneration;
    function stillCurrent() {
        return !isCurrent || isCurrent();
    }
    function abortIfSuperseded() {
        return !stillCurrent();
    }
    function setProgress(message) {
        setHandRecordStatus(message, false, statusGeneration);
    }
    // Drop any prior session so progress chrome is visible for a re-import.
    clearHandRecordSession();
    setProgress("Reading PDF…");
    if (abortIfSuperseded()) {
        return Promise.resolve(null);
    }
    return file.arrayBuffer().then(function (ab) {
        if (abortIfSuperseded()) {
            return null;
        }
        return new Uint8Array(ab);
    }).then(function (bytes) {
        if (bytes == null) {
            return null;
        }
        if (abortIfSuperseded()) {
            return null;
        }
        setProgress("Parsing boards…");
        if (abortIfSuperseded()) {
            return null;
        }
        return loadAll(bytes);
    }).then(function (boards) {
        if (boards == null) {
            return null;
        }
        if (abortIfSuperseded()) {
            return null;
        }
        return boards;
    });
}

function chooseDealFile() {
    const input = document.getElementById("import-deal-file");
    if (!input) {
        return;
    }
    input.value = "";
    input.click();
}

let dealFileSelectionGeneration = 0;

function dealFileSelectionIsCurrent(input, file, selectionGeneration) {
    return selectionGeneration === dealFileSelectionGeneration
        && input.files
        && input.files[0] === file;
}

async function handleDealFileSelected(input) {
    const file = input && input.files && input.files[0];
    if (!file) {
        return;
    }

    const selectionGeneration = ++dealFileSelectionGeneration;
    const result = document.getElementById("result");

    if (isHandRecordPdfFile(file)) {
        try {
            const boards = await loadHandRecordFromFile(file, {
                statusGeneration: selectionGeneration,
                isCurrent: function () {
                    return dealFileSelectionIsCurrent(
                        input,
                        file,
                        selectionGeneration
                    );
                },
            });
            if (!dealFileSelectionIsCurrent(input, file, selectionGeneration)) {
                return;
            }
            if (!boards) {
                return;
            }
            handRecordBoards = boards;
            handRecordIndex = 0;
            showHandRecordBoard(0);
            if (result) {
                result.innerHTML = "";
            }
        } catch (err) {
            if (!dealFileSelectionIsCurrent(input, file, selectionGeneration)) {
                return;
            }
            clearHandRecordSession();
            setHandRecordStatus(
                String(err && err.message ? err.message : err),
                true,
                selectionGeneration
            );
            invalidateActiveDdTableRequest();
        }
        return;
    }

    try {
        const text = await file.text();
        // A newer pick (or cleared selection) supersedes this read.
        if (!dealFileSelectionIsCurrent(input, file, selectionGeneration)) {
            return;
        }
        const err = importDealFromText(text);
        if (err) {
            // Do not let an in-flight grace/paint overwrite the import failure.
            invalidateActiveDdTableRequest();
            if (result) {
                result.innerHTML = err;
            }
        } else if (result) {
            // Import itself succeeded; do not leave a prior error message.
            result.innerHTML = "";
        }
    } catch (err) {
        if (!dealFileSelectionIsCurrent(input, file, selectionGeneration)) {
            return;
        }
        invalidateActiveDdTableRequest();
        if (result) {
            result.innerHTML = err && err.message
                ? err.message
                : "Could not read the selected file.";
        }
    }
}

function * directions_and_suits() {
    // Generator

    for (const direction of DIRECTIONS) {
        for (const suit of SUITS) {
            yield { "direction": direction, "suit": suit };
        }
    }
}

function * hand_elements() {
    // Generator

    for (const ds of directions_and_suits()) {
        var element_index = ds.direction + "_" + ds.suit;
        var element = document.getElementById(element_index);
        yield element;
    }
}

function clearTestData() {
    if (typeof exitPlay === "function") {
        exitPlay();
    }
    clearHandRecordSession();
    clear_results();

    for (const element of hand_elements()) {
        element.value = "";
    }

    updateActionButtons();
    focusNorthSpades();
}

function rotateClockwise() {
    if (typeof exitPlay === "function") {
        exitPlay();
    }
    clear_results();

    var hands = [];

    for (const element of hand_elements()) {
        hands.push(element.value);
    }

    // rotate west to north, and so on
    for (var i = 0; i < 4; i++) {
        var west = hands.pop();
        hands.unshift(west);
    }

    for (const element of hand_elements()) {
        element.value = hands.shift();
    }

    updateActionButtons();
}

function allDeckCards() {
    const cards = [];

    for (const suit of SUITS) {
        for (const pip of PIPS) {
            cards.push(new Card(suit, pip));
        }
    }

    return cards;
}

function undeployedCardHtml(card) {
    return "<button type=\"button\" class=\"hand-card\" draggable=\"true\"" +
        " data-card=\"" + escapeHtml(card.key()) + "\"" +
        " tabindex=\"-1\"" +
        " aria-label=\"" +
        escapeHtml(handCardAriaLabel("undeployed", card)) + "\">" +
        escapeHtml(card.pip) +
        "</button>";
}

function deckStatusHtml(hands) {
    const enteredCards = {};

    for (const direction of DIRECTIONS) {
        for (const card of hands[direction]) {
            enteredCards[card.key()] = true;
        }
    }

    return SUITS.map((suit) => {
        const remaining = PIPS.split("").filter((pip) => {
            return !enteredCards[new Card(suit, pip).key()];
        });

        if (remaining.length === 0) {
            return "";
        }

        const cardsHtml = remaining.map((pip) => {
            return undeployedCardHtml(new Card(suit, pip));
        }).join("");
        const tag = suitTag(suit);

        // Match dealt holdings: colored suit glyph, then hand-card pips.
        return "<div class=\"deck-suit-row\"><" + tag + ">" +
            SUIT_GLYPHS[suit] + "</" + tag + ">" + cardsHtml + "</div>";
    }).join("");
}

const SEAT_FONT_PX = 30;
const FIT_EPSILON_PX = 0.5;

function maxRowScrollWidth(root, rowSelector) {
    let needed = 0;

    if (!root || typeof root.querySelectorAll !== "function") {
        return 0;
    }

    for (const row of root.querySelectorAll(rowSelector)) {
        needed = Math.max(needed, row.scrollWidth || 0);
    }

    return needed;
}

function contentBoxWidth(element) {
    if (!element || typeof getComputedStyle !== "function") {
        return 0;
    }

    const style = getComputedStyle(element);

    return element.clientWidth -
        (parseFloat(style.paddingLeft) || 0) -
        (parseFloat(style.paddingRight) || 0);
}

function clearFitStyles(element) {
    element.style.fontSize = "";
    element.style.maxWidth = "";
    element.style.overflowX = "";
}

function clipElementToAvailable(element, available) {
    element.style.maxWidth = available + "px";
    element.style.overflowX = "hidden";
}

function fitFontToContentWidth(element, available, rowSelector, baseFontPx) {
    // Prefer clearing any prior shrink so a roomy box returns to the CSS seat
    // size instead of keeping a stale inline fit.
    clearFitStyles(element);

    if (!(available > 0)) {
        return;
    }

    const neededFull = maxRowScrollWidth(element, rowSelector);

    if (!(neededFull > available + FIT_EPSILON_PX)) {
        return;
    }

    // 1px card borders do not scale with font-size. Probe near-zero font to
    // estimate that fixed chrome, then solve for the font that fits in one shot
    // (proportional iteration converges too slowly near the fixed floor).
    element.style.fontSize = "0.01px";
    const fixedPx = maxRowScrollWidth(element, rowSelector);
    const scalablePx = neededFull - fixedPx;

    if (!(fixedPx <= available + FIT_EPSILON_PX) || !(scalablePx > 0)) {
        clearFitStyles(element);
        clipElementToAvailable(element, available);
        return;
    }

    const fontPx = baseFontPx * (available - fixedPx) / scalablePx;

    if (!(fontPx > 0)) {
        clearFitStyles(element);
        clipElementToAvailable(element, available);
        return;
    }

    element.style.fontSize = fontPx + "px";

    if (maxRowScrollWidth(element, rowSelector) > available + FIT_EPSILON_PX) {
        clipElementToAvailable(element, available);
    }
}

function fitCenterDeckCards() {
    const deckStatus = document.getElementById("deck-status");

    if (!deckStatus) {
        return;
    }

    clearFitStyles(deckStatus);

    if (deckStatus.hidden) {
        return;
    }

    const center = typeof document.querySelector === "function"
        ? document.querySelector(".grid-filler-center")
        : null;

    if (!center) {
        return;
    }

    fitFontToContentWidth(
        deckStatus,
        contentBoxWidth(center),
        ".deck-suit-row",
        SEAT_FONT_PX
    );
}

function fitSeatHandCards(seatSelector) {
    const hand = typeof document.querySelector === "function"
        ? document.querySelector(seatSelector)
        : null;

    if (!hand) {
        return;
    }

    // Clear clip/maxWidth before measuring: an earlier narrow fit's inline
    // maxWidth would otherwise keep clientWidth stuck at the clipped size.
    clearFitStyles(hand);

    fitFontToContentWidth(
        hand,
        contentBoxWidth(hand),
        ".hand-suit",
        SEAT_FONT_PX
    );
}

function fitSouthHandCards() {
    fitSeatHandCards(".hand-south");
}

function fitEastHandCards() {
    fitSeatHandCards(".hand-east");
}

function fitDiagramFonts() {
    fitCenterDeckCards();
    fitSouthHandCards();
    fitEastHandCards();
}

function updateDeckStatus(hands) {
    const deckStatus = document.getElementById("deck-status");

    if (deckStatus) {
        deckStatus.innerHTML = deckStatusHtml(hands);
        fitCenterDeckCards();
    }
}

const DENOM_TO_SUIT = {
    C: "clubs",
    D: "diamonds",
    H: "hearts",
    S: "spades"
};

const DENOM_ARIA_NAMES = {
    C: "Clubs",
    D: "Diamonds",
    H: "Hearts",
    S: "Spades",
    N: "Notrump"
};

function denominationDisplayHtml(denomination) {
    if (denomination === "N") {
        return "NT";
    }

    const suit = DENOM_TO_SUIT[denomination];

    if (!suit) {
        return "";
    }

    return suitSymbolHtml(suit);
}

function contractStatusHtml(contract, options) {
    const denomHtml = denominationDisplayHtml(contract.denomination);
    const denomName = DENOM_ARIA_NAMES[contract.denomination];
    const seat = contract.direction;
    const onLead = !!(options && options.onLead);

    if (!DIRECTIONS.includes(seat) || !denomHtml || !denomName) {
        return "";
    }

    const seatLetter = seat.charAt(0).toUpperCase();
    const seatName = capitalize(seat);
    const relation = onLead ? "lead" : "by";
    const aria = onLead
        ? denomName + "; " + seatName + " on lead"
        : denomName + "; " + seatName + " declares";

    return "<div class=\"contract-status-panel\" aria-label=\"" + aria + "\">" +
        "<div class=\"contract-status-denom\">" + denomHtml + "</div>" +
        "<div class=\"contract-status-by\">" + relation + "</div>" +
        "<div class=\"contract-status-declarer\">" + seatLetter + "</div>" +
        "</div>";
}

function updateContractStatus() {
    const status = document.getElementById("contract-status");

    if (!status) {
        return;
    }

    const contract = selectedContractState;

    if (!contract) {
        status.hidden = true;
        status.innerHTML = "";
        return;
    }

    const onLead = typeof dealIsPartial === "function" &&
        dealIsPartial(collectHands());
    status.innerHTML = contractStatusHtml(contract, { onLead });
    status.hidden = false;
}

const RESULT_TABLE_HINT_CONTRACT =
    "Click a cell to play out that contract";
const RESULT_TABLE_HINT_LEAD =
    "Click a cell to place the corresponding player on lead";

function updateResultTableHint(hands) {
    const hint = document.getElementById("result-table-hint");

    if (!hint) {
        return;
    }

    const table = document.getElementById("result-table");
    const populated = typeof ddTableLooksPopulated === "function" &&
        ddTableLooksPopulated(table, hands);

    if (!populated) {
        hint.hidden = true;
        hint.textContent = "";
        return;
    }

    const partial = typeof dealIsPartial === "function" && dealIsPartial(hands);
    hint.hidden = false;
    hint.textContent = partial
        ? RESULT_TABLE_HINT_LEAD
        : RESULT_TABLE_HINT_CONTRACT;
}

function capitalize(word) {
    return word.charAt(0).toUpperCase() + word.slice(1);
}

function escapeHtml(value) {
    if (value == null) {
        return "";
    }

    return String(value)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#39;");
}

function playResultAriaText(badge) {
    if (badge == null || badge === undefined) {
        return "";
    }
    const text = String(badge);
    if (text === "=") {
        return "equals";
    }
    if (text.charAt(0) === "+") {
        return "plus " + text.slice(1);
    }
    if (text.charAt(0) === "\u2013" || text.charAt(0) === "-") {
        return "minus " + text.slice(1);
    }
    return text;
}

function handCardAriaLabel(direction, card, options) {
    const suitName = card.suit.replace(/s$/, "");
    const pipName = PIP_NAMES[card.pip] || card.pip;
    let label = capitalize(direction) + " " + suitName + " " + pipName;

    if (options && options.playable) {
        label += "; playable";
        const result = playResultAriaText(options.result);
        if (result) {
            label += ", result " + result;
        }
    }

    return label;
}

function handCardTrickBadgeClass(badge) {
    const text = String(badge);
    if (text === "=") {
        return " hand-card-tricks diff-zero";
    }
    if (text.charAt(0) === "+") {
        return " hand-card-tricks diff-plus";
    }
    if (text.charAt(0) === "\u2013" || text.charAt(0) === "-") {
        return " hand-card-tricks diff-minus";
    }
    return " hand-card-tricks";
}

function handCardHtml(direction, card, index, leadTricks) {
    const hasTricks = leadTricks != null && leadTricks !== undefined;
    const inPlay = typeof isPlayMode === "function" && isPlayMode();
    const playable = inPlay &&
        typeof isLegalPlayCard === "function" &&
        isLegalPlayCard(playState, direction, card.key());
    let classes = "hand-card";
    if (hasTricks) {
        classes += " hand-card-with-tricks";
    }
    if (playable) {
        classes += " hand-card-playable";
    }
    const badge = hasTricks
        ? "<span class=\"" + handCardTrickBadgeClass(leadTricks).trim() +
            "\" aria-hidden=\"true\">" +
            escapeHtml(leadTricks) +
            "</span>"
        : "";
    const draggable = !inPlay || playable ? "true" : "false";
    // In play mode, illegal cards must not remain keyboard-focusable enabled
    // buttons; edit-mode cards and legal plays stay enabled.
    const disabledAttr = inPlay && !playable ? " disabled" : "";

    const aria = handCardAriaLabel(direction, card, {
        playable: playable,
        result: hasTricks ? leadTricks : null,
    });

    return "<button type=\"button\" class=\"" + classes + "\" draggable=\"" +
        draggable + "\"" +
        disabledAttr +
        " data-direction=\"" + escapeHtml(direction) + "\"" +
        " data-card=\"" + escapeHtml(card.key()) + "\"" +
        " data-index=\"" + escapeHtml(index) + "\"" +
        " aria-label=\"" + escapeHtml(aria) + "\">" +
        escapeHtml(card.pip) +
        badge +
        "</button>";
}

function handCaretHtml() {
    return "<span class=\"hand-caret\" aria-hidden=\"true\"></span>";
}

function handHoldingHtml(direction, suit, cards, caretIndex, tricksByKey) {
    const suitCards = cards.filter((card) => card.suit === suit);
    let html = "";

    for (let i = 0; i < suitCards.length; i++) {
        if (caretIndex === i) {
            html += handCaretHtml();
        }

        const key = suitCards[i].key();
        const tricks = tricksByKey && Object.prototype.hasOwnProperty.call(
            tricksByKey,
            key
        )
            ? tricksByKey[key]
            : undefined;

        html += handCardHtml(direction, suitCards[i], i, tricks);
    }

    if (caretIndex === suitCards.length) {
        html += handCaretHtml();
    }

    return html;
}

function parseHandInputId(id) {
    if (!id) {
        return null;
    }

    const parts = id.split("_");

    if (parts.length !== 2) {
        return null;
    }

    const direction = parts[0];
    const suit = parts[1];

    if (!DIRECTIONS.includes(direction) || !SUITS.includes(suit)) {
        return null;
    }

    return { direction, suit };
}

function focusedSuitCaret() {
    const active = document.activeElement;

    if (!isHandInput(active)) {
        return null;
    }

    const parsed = parseHandInputId(active.id);

    if (!parsed) {
        return null;
    }

    const index = typeof active.selectionStart === "number"
        ? active.selectionStart
        : active.value.length;

    return {
        direction: parsed.direction,
        suit: parsed.suit,
        index: index
    };
}

function placeCaretInSuitInput(input, pos) {
    if (!input) {
        return;
    }

    if (typeof input.focus === "function") {
        input.focus();
    }

    const max = input.value ? input.value.length : 0;
    const clamped = Math.max(0, Math.min(pos, max));

    if (typeof input.setSelectionRange === "function") {
        input.setSelectionRange(clamped, clamped);
    } else {
        input.selectionStart = clamped;
        input.selectionEnd = clamped;
    }
}

function caretPosForCardClick(index, event, button) {
    let pos = index + 1;

    if (typeof event.clientX === "number" &&
            button && typeof button.getBoundingClientRect === "function") {
        const rect = button.getBoundingClientRect();

        if (rect && typeof rect.left === "number" && typeof rect.width === "number" &&
                event.clientX < rect.left + rect.width / 2) {
            pos = index;
        }
    }

    return pos;
}

function updateHandCardDisplays(hands) {
    const caret = focusedSuitCaret();

    for (const direction of DIRECTIONS) {
        for (const suit of SUITS) {
            const holder = document.getElementById(direction + "_" + suit + "_cards");

            if (holder) {
                const caretIndex = caret &&
                    caret.direction === direction &&
                    caret.suit === suit
                    ? caret.index
                    : -1;

                holder.innerHTML = handHoldingHtml(
                    direction,
                    suit,
                    hands[direction] || [],
                    caretIndex,
                    leadTricksByCardKey
                );
            }
        }
    }

    fitSouthHandCards();
    fitEastHandCards();
}

function onHandCardClick(direction, card) {
    if (typeof isPlayMode === "function" && isPlayMode()) {
        tryPlayCard(direction, card.key(), false);
    }
}

function handleHandCardMouseDown(_event) {
    // Intentionally empty: preventDefault on mousedown blocks HTML5 dragstart.
    // Click handlers refocus the suit input via placeCaretInSuitInput.
}

const DDS_CARD_MIME = "application/x-dds-card";
let activeCardDrag = null;
let activeDropTarget = null;

function clearActiveDropTarget() {
    if (activeDropTarget && activeDropTarget.classList) {
        activeDropTarget.classList.remove("drop-target-active");
    }

    activeDropTarget = null;
}

function setActiveDropTarget(element) {
    if (activeDropTarget === element) {
        return;
    }

    clearActiveDropTarget();

    if (element && element.classList) {
        element.classList.add("drop-target-active");
        activeDropTarget = element;
    }
}

function parseCardDragPayload(raw) {
    if (!raw) {
        return null;
    }

    try {
        const parsed = JSON.parse(raw);

        if (!parsed || typeof parsed.key !== "string") {
            return null;
        }

        return {
            key: parsed.key,
            sourceDirection: parsed.sourceDirection || null,
        };
    } catch (_err) {
        return null;
    }
}

function readCardDragPayload(dataTransfer) {
    if (activeCardDrag) {
        return {
            key: activeCardDrag.key,
            sourceDirection: activeCardDrag.sourceDirection,
        };
    }

    if (!dataTransfer || typeof dataTransfer.getData !== "function") {
        return null;
    }

    return parseCardDragPayload(dataTransfer.getData(DDS_CARD_MIME));
}

function handDirectionFromElement(element) {
    if (!element || typeof element.closest !== "function") {
        return null;
    }

    for (const direction of DIRECTIONS) {
        if (element.closest(".hand-" + direction)) {
            return direction;
        }
    }

    return null;
}

function centerDropElement(element) {
    if (!element || typeof element.closest !== "function") {
        return null;
    }

    if (typeof isPlayMode === "function" && isPlayMode()) {
        return element.closest("#trick-status, .grid-filler-center");
    }

    return element.closest("#deck-status, .grid-filler-center");
}

function canDropCardOnHand(card, toDirection) {
    if (!card || !DIRECTIONS.includes(toDirection)) {
        return false;
    }

    // Dropping back onto the same hand is a no-op, not an error.
    if (handContainsCard(toDirection, card)) {
        return true;
    }

    return handCardCount(toDirection) < 13;
}

function handleCardDragStart(event) {
    const target = event.target;

    if (!target || typeof target.closest !== "function") {
        return;
    }

    const button = target.closest(".hand-card");

    if (!button || typeof button.getAttribute !== "function") {
        return;
    }

    const key = button.getAttribute("data-card");

    if (!key) {
        return;
    }

    const sourceDirection = button.getAttribute("data-direction");

    if (typeof isPlayMode === "function" && isPlayMode()) {
        if (!isLegalPlayCard(playState, sourceDirection, key)) {
            if (event.preventDefault) {
                event.preventDefault();
            }
            return;
        }
    }

    const payload = {
        key: key,
        sourceDirection: sourceDirection || null,
    };

    activeCardDrag = {
        key: payload.key,
        sourceDirection: payload.sourceDirection,
        element: button,
    };

    if (event.dataTransfer) {
        if (typeof event.dataTransfer.setData === "function") {
            event.dataTransfer.setData(DDS_CARD_MIME, JSON.stringify(payload));
        }

        event.dataTransfer.effectAllowed = "move";
    }

    if (button.classList) {
        button.classList.add("hand-card-dragging");
    }
}

function handleCardDragOver(event) {
    const payload = readCardDragPayload(event.dataTransfer);

    if (!payload) {
        return;
    }

    const card = cardFromKeySafe(payload.key);

    if (!card) {
        if (event.dataTransfer) {
            event.dataTransfer.dropEffect = "none";
        }

        clearActiveDropTarget();
        return;
    }

    const center = centerDropElement(event.target);

    if (center) {
        if (typeof isPlayMode === "function" && isPlayMode()) {
            const legal = isLegalPlayCard(
                playState,
                payload.sourceDirection,
                payload.key
            );
            if (!legal) {
                if (event.dataTransfer) {
                    event.dataTransfer.dropEffect = "none";
                }
                clearActiveDropTarget();
                return;
            }
        }

        if (event.preventDefault) {
            event.preventDefault();
        }

        if (event.dataTransfer) {
            event.dataTransfer.dropEffect = "move";
        }

        setActiveDropTarget(center);
        return;
    }

    if (typeof isPlayMode === "function" && isPlayMode()) {
        if (event.dataTransfer) {
            event.dataTransfer.dropEffect = "none";
        }
        clearActiveDropTarget();
        return;
    }

    const toDirection = handDirectionFromElement(event.target);

    if (!toDirection || !canDropCardOnHand(card, toDirection)) {
        if (event.dataTransfer) {
            event.dataTransfer.dropEffect = "none";
        }

        clearActiveDropTarget();
        return;
    }

    if (event.preventDefault) {
        event.preventDefault();
    }

    if (event.dataTransfer) {
        event.dataTransfer.dropEffect = "move";
    }

    const handEl = event.target.closest
        ? event.target.closest(".hand-" + toDirection)
        : null;
    setActiveDropTarget(handEl);
}

function handleCardDragLeave(event) {
    const related = event.relatedTarget;

    if (activeDropTarget && related && typeof activeDropTarget.contains === "function") {
        if (activeDropTarget.contains(related)) {
            return;
        }
    }

    clearActiveDropTarget();
}

function handleCardDrop(event) {
    const payload = readCardDragPayload(event.dataTransfer);

    clearActiveDropTarget();

    if (!payload) {
        return;
    }

    const card = cardFromKeySafe(payload.key);

    if (!card) {
        return;
    }

    if (event.preventDefault) {
        event.preventDefault();
    }

    const center = centerDropElement(event.target);

    if (center) {
        if (typeof isPlayMode === "function" && isPlayMode()) {
            tryPlayCard(payload.sourceDirection, payload.key, false);
            return;
        }

        if (undeployCard(card)) {
            updateActionButtons();
        }

        return;
    }

    if (typeof isPlayMode === "function" && isPlayMode()) {
        return;
    }

    const toDirection = handDirectionFromElement(event.target);

    if (!toDirection) {
        return;
    }

    if (!canDropCardOnHand(card, toDirection)) {
        return;
    }

    if (moveCardToHand(card, toDirection)) {
        updateActionButtons();
    }
}

function handleCardDragEnd(_event) {
    if (activeCardDrag && activeCardDrag.element && activeCardDrag.element.classList) {
        activeCardDrag.element.classList.remove("hand-card-dragging");
    }

    activeCardDrag = null;
    clearActiveDropTarget();
}

function handleSuitSelectionChange() {
    if (!isHandInput(document.activeElement)) {
        return;
    }

    updateHandCardDisplays(collectHands());
}

function handleHandSuitClick(event) {
    const target = event.target;

    if (!target || typeof target.closest !== "function") {
        return;
    }

    if (target.closest(".hand-card")) {
        return;
    }

    const suitRow = target.closest(".hand-suit");

    if (!suitRow || typeof suitRow.querySelector !== "function") {
        return;
    }

    const input = suitRow.querySelector(".hand-suit-input");

    if (input) {
        placeCaretInSuitInput(input, input.value ? input.value.length : 0);
        updateHandCardDisplays(collectHands());
    }
}

function handleHandCardClick(event) {
    const target = event.target;

    if (!target || typeof target.closest !== "function") {
        return;
    }

    const button = target.closest(".hand-card");

    if (!button) {
        return;
    }

    const direction = button.getAttribute("data-direction");
    const key = button.getAttribute("data-card");
    const indexAttr = button.getAttribute("data-index");

    if (!direction || !key || !DIRECTIONS.includes(direction)) {
        return;
    }

    const card = Card.fromKey(key);

    if (!card.suit || !card.pip) {
        return;
    }

    if (event.preventDefault) {
        event.preventDefault();
    }

    if (typeof isPlayMode === "function" && isPlayMode()) {
        onHandCardClick(direction, card);
        return;
    }

    const index = indexAttr == null ? -1 : parseInt(indexAttr, 10);
    const input = document.getElementById(direction + "_" + card.suit);

    if (input && index >= 0 && !isNaN(index)) {
        placeCaretInSuitInput(
            input,
            caretPosForCardClick(index, event, button)
        );
        updateHandCardDisplays(collectHands());
    }

    onHandCardClick(direction, card);
}

function selectedContract() {
    return selectedContractState;
}

function ensurePlayForSelectedContract() {
    const contract = selectedContractState;

    if (!contract) {
        return false;
    }

    const cell = resultCellForContract(contract.direction, contract.denomination);
    const target = typeof targetTricksFromCell === "function"
        ? targetTricksFromCell(cell)
        : null;

    if (target == null) {
        return false;
    }

    const hands = collectHands();
    const partial = typeof dealIsPartial === "function" && dealIsPartial(hands);
    const leadSeat = partial ? contract.direction : null;
    const samePlay = typeof isPlayMode === "function" && isPlayMode() &&
        playState &&
        playState.denomination === contract.denomination &&
        playState.targetTricks === target &&
        (partial
            ? playState.leadSeat === contract.direction
            : playState.declarer === contract.direction);

    if (samePlay) {
        return true;
    }

    if (typeof startPlay === "function") {
        return startPlay(
            contract.direction,
            contract.denomination,
            target,
            leadSeat
        );
    }

    return false;
}

function onContractSelect(direction, denomination) {
    if (!direction || !denomination) {
        if (typeof exitPlay === "function") {
            exitPlay();
        }
        return;
    }

    // Table may still be computing; start once the cell has a trick count.
    ensurePlayForSelectedContract();
}

function clearResultCellSelectionHighlight() {
    const table = document.getElementById("result-table");

    if (!table || !table.rows) {
        return;
    }

    for (let row = 1; row <= 4; row++) {
        for (let column = 1; column <= 5; column++) {
            const cell = table.rows[row].cells[column];

            if (cell && cell.classList) {
                cell.classList.remove("result-cell-selected");
            }
        }
    }
}

function resultCellForContract(direction, denomination) {
    const row = DIRECTIONS.indexOf(direction) + 1;
    const column = DENOMINATIONS.indexOf(denomination) + 1;

    if (row < 1 || column < 1) {
        return null;
    }

    const table = document.getElementById("result-table");

    if (!table || !table.rows || !table.rows[row]) {
        return null;
    }

    return table.rows[row].cells[column] || null;
}

function applyResultCellSelection(direction, denomination) {
    clearResultCellSelectionHighlight();

    const cell = resultCellForContract(direction, denomination);

    if (cell && cell.classList) {
        cell.classList.add("result-cell-selected");
    }

    selectedContractState = { direction, denomination };
    // Drop prior-contract numerals / in-flight solves immediately so a slow
    // queued lead solve cannot repaint stale badges after the switch.
    leadTricksRequestId += 1;
    leadTricksByCardKey = null;
    updateContractStatus();
    updateHandCardDisplays(collectHands());
    onContractSelect(direction, denomination);
    void scheduleDealSolve();
}

function clearResultCellSelection() {
    clearResultCellSelectionHighlight();
    selectedContractState = null;
    // Drop in-flight lead solves so a late completion cannot revive numerals.
    leadTricksRequestId += 1;
    leadTricksByCardKey = null;
    updateContractStatus();
    updateHandCardDisplays(collectHands());
    onContractSelect(null, null);
    void scheduleDealSolve();
}

function contractFromResultCell(cell) {
    if (!cell) {
        return null;
    }

    const row = cell.parentElement && typeof cell.parentElement.rowIndex === "number"
        ? cell.parentElement.rowIndex
        : -1;
    const column = typeof cell.cellIndex === "number" ? cell.cellIndex : -1;

    if (row < 1 || row > 4 || column < 1 || column > 5) {
        return null;
    }

    return {
        direction: DIRECTIONS[row - 1],
        denomination: DENOMINATIONS[column - 1]
    };
}

function resultCellAriaLabel(direction, denomination, onLead) {
    const denomName = DENOM_ARIA_NAMES[denomination];

    if (!DIRECTIONS.includes(direction) || !denomName) {
        return "";
    }

    return denomName + "; " + capitalize(direction) +
        (onLead ? " on lead" : " declares");
}

function updateResultTableCellLabels(hands) {
    const table = document.getElementById("result-table");

    if (!table || !table.rows) {
        return;
    }

    const onLead = typeof dealIsPartial === "function" && dealIsPartial(hands);

    for (let row = 1; row <= 4; row++) {
        for (let column = 1; column <= 5; column++) {
            const cell = table.rows[row] && table.rows[row].cells[column];

            if (!cell || typeof cell.setAttribute !== "function") {
                continue;
            }

            cell.setAttribute(
                "aria-label",
                resultCellAriaLabel(
                    DIRECTIONS[row - 1],
                    DENOMINATIONS[column - 1],
                    onLead
                )
            );
        }
    }
}

function enhanceResultTableCells() {
    const table = document.getElementById("result-table");

    if (!table || !table.rows) {
        return;
    }

    for (let row = 1; row <= 4; row++) {
        for (let column = 1; column <= 5; column++) {
            const cell = table.rows[row] && table.rows[row].cells[column];

            if (!cell) {
                continue;
            }

            const direction = DIRECTIONS[row - 1];
            const denomination = DENOMINATIONS[column - 1];
            cell.tabIndex = 0;

            if (typeof cell.setAttribute === "function") {
                cell.setAttribute("role", "button");
                cell.setAttribute(
                    "aria-label",
                    resultCellAriaLabel(direction, denomination, false)
                );
            }
        }
    }
}

function activateResultCell(cell) {
    const contract = contractFromResultCell(cell);

    if (!contract) {
        return;
    }

    const current = selectedContractState;

    if (current &&
            current.direction === contract.direction &&
            current.denomination === contract.denomination) {
        clearResultCellSelection();
        return;
    }

    applyResultCellSelection(contract.direction, contract.denomination);
}

function handleResultTableClick(event) {
    const target = event.target;

    if (!target || typeof target.closest !== "function") {
        return;
    }

    const cell = target.closest("#result-table td");

    if (!cell) {
        return;
    }

    activateResultCell(cell);
}

function handleResultTableKeyDown(event) {
    if (event.key !== "Enter" && event.key !== " ") {
        return;
    }

    const target = event.target;

    if (!target || typeof target.closest !== "function") {
        return;
    }

    const cell = target.closest("#result-table td");

    if (!cell || cell !== target) {
        return;
    }

    if (event.key === " " && typeof event.preventDefault === "function") {
        event.preventDefault();
    }

    activateResultCell(cell);
}

function updateHandCardCounts(hands) {
    for (const direction of DIRECTIONS) {
        const count = hands[direction].length;
        const note = document.getElementById(direction + "-card-count");

        if (note) {
            const show = count !== 0 && count !== 13;
            note.hidden = !show;
            note.innerHTML = show
                ? count + (count === 1 ? " card" : " cards")
                : "";
        }
    }
}

function setHandInputs(direction, holdings) {
    for (const suit of SUITS) {
        document.getElementById(direction + "_" + suit).value = holdings[suit];
    }
}

function suitInputId(direction, suit) {
    return direction + "_" + suit;
}

function handContainsCard(direction, card) {
    if (!direction || !card || !card.suit || !card.pip) {
        return false;
    }

    const input = document.getElementById(suitInputId(direction, card.suit));

    if (!input || !input.value) {
        return false;
    }

    return input.value.toUpperCase().includes(card.pip.toUpperCase());
}

function cardHeldSomewhere(card) {
    for (const direction of DIRECTIONS) {
        if (handContainsCard(direction, card)) {
            return true;
        }
    }

    return false;
}

function handCardCount(direction) {
    if (!DIRECTIONS.includes(direction)) {
        return 0;
    }

    let count = 0;

    for (const suit of SUITS) {
        const input = document.getElementById(suitInputId(direction, suit));

        if (input) {
            count += String(input.value).length;
        }
    }

    return count;
}

function removeCardFromHand(direction, card) {
    if (!direction || !card || !card.suit || !card.pip) {
        return false;
    }

    const input = document.getElementById(suitInputId(direction, card.suit));

    if (!input) {
        return false;
    }

    const value = String(input.value);
    const pip = card.pip.toUpperCase();
    const index = value.toUpperCase().indexOf(pip);

    if (index < 0) {
        return false;
    }

    input.value = value.slice(0, index) + value.slice(index + 1);
    return true;
}

function removeCardFromAllHands(card) {
    let removed = false;

    for (const direction of DIRECTIONS) {
        if (removeCardFromHand(direction, card)) {
            removed = true;
        }
    }

    return removed;
}

function undeployCard(card) {
    return removeCardFromAllHands(card);
}

function addCardToHand(direction, card) {
    if (!DIRECTIONS.includes(direction) || !card || !card.suit || !card.pip) {
        return false;
    }

    if (!SUITS.includes(card.suit) || !PIPS.includes(card.pip.toUpperCase())) {
        return false;
    }

    if (cardHeldSomewhere(card)) {
        return false;
    }

    const input = document.getElementById(suitInputId(direction, card.suit));

    if (!input) {
        return false;
    }

    const pip = card.pip.toUpperCase();
    const value = sanitizeSuitHolding(String(input.value));
    const at = sortedPipInsertIndex(value, pip);

    input.value = value.slice(0, at) + pip + value.slice(at);
    return true;
}

function moveCardToHand(card, toDirection) {
    if (!DIRECTIONS.includes(toDirection) || !card || !card.suit || !card.pip) {
        return false;
    }

    if (handContainsCard(toDirection, card)) {
        return true;
    }

    if (handCardCount(toDirection) >= 13) {
        return false;
    }

    removeCardFromAllHands(card);
    return addCardToHand(toDirection, card);
}

function remainingCardsForEmptyHand(state) {
    return allDeckCards().filter((card) => !state.usedCards[card.key()]);
}

function applyFourthHandFill(hands, emptyHand) {
    const state = fourthHandFillState(hands);

    if (!state.canFill || state.emptyHand !== emptyHand) {
        return false;
    }

    const remaining = remainingCardsForEmptyHand(state);

    if (remaining.length !== 13) {
        return false;
    }

    clear_results();
    setHandInputs(emptyHand, cardsToSuitHoldings(remaining));
    return true;
}

function isHandInput(element) {
    if (!element) {
        return false;
    }

    for (const handElement of hand_elements()) {
        if (handElement === element) {
            return true;
        }
    }

    return false;
}

function suitHoldingWouldExceedHandLimit(element) {
    const parsed = parseHandInputId(element && element.id);

    if (!parsed) {
        return false;
    }

    let otherCount = 0;

    for (const suit of SUITS) {
        if (suit === parsed.suit) {
            continue;
        }

        const other = document.getElementById(suitInputId(parsed.direction, suit));

        if (other) {
            otherCount += sanitizeSuitHolding(other.value).length;
        }
    }

    return otherCount + sanitizeSuitHolding(element.value).length > 13;
}

function suitHoldingPipsAreSubset(candidate, allowed) {
    const allowedSet = {};

    for (const pip of allowed) {
        allowedSet[pip] = true;
    }

    for (const pip of candidate) {
        if (!allowedSet[pip]) {
            return false;
        }
    }

    return true;
}

let lastSuitHoldings = {};

function rememberedSuitHolding(elementId) {
    if (!elementId || !Object.prototype.hasOwnProperty.call(lastSuitHoldings, elementId)) {
        return "";
    }

    return lastSuitHoldings[elementId];
}

function rememberSuitHoldings() {
    for (const element of hand_elements()) {
        if (element && element.id) {
            lastSuitHoldings[element.id] = sanitizeSuitHolding(element.value);
        }
    }
}

function suitHoldingRejectsFullHandEdit(element) {
    const parsed = parseHandInputId(element && element.id);

    if (!parsed) {
        return false;
    }

    const previousHolding = rememberedSuitHolding(element.id);
    const previousTotal =
        sameHandCardCountExcludingSuit(parsed.direction, parsed.suit) +
        previousHolding.length;

    if (previousTotal < 13) {
        return false;
    }

    const proposed = sanitizeSuitHolding(element.value);

    return proposed.length > previousHolding.length ||
        !suitHoldingPipsAreSubset(proposed, previousHolding);
}

let illegalInputAudioContext = null;

function playIllegalInputBeep() {
    try {
        const AudioCtx = typeof window !== "undefined"
            ? (window.AudioContext || window.webkitAudioContext)
            : null;

        if (typeof AudioCtx !== "function") {
            return;
        }

        if (!illegalInputAudioContext) {
            illegalInputAudioContext = new AudioCtx();
        }

        const audio = illegalInputAudioContext;

        if (audio.state === "suspended" && typeof audio.resume === "function") {
            void audio.resume();
        }

        const oscillator = audio.createOscillator();
        const gain = audio.createGain();
        oscillator.type = "square";
        oscillator.frequency.value = 880;
        gain.gain.value = 0.04;
        oscillator.connect(gain);
        gain.connect(audio.destination);
        const now = audio.currentTime;
        oscillator.start(now);
        gain.gain.exponentialRampToValueAtTime(0.0001, now + 0.08);
        oscillator.stop(now + 0.08);
    } catch (_err) {
        // Beep is best-effort; ignore missing Web Audio support.
    }
}

function handleHandSuitInput(event) {
    const input = event && event.target;
    let beep = false;

    if (input) {
        if (suitHoldingHasIllegalChars(input.value)) {
            beep = true;
        }

        if (suitHoldingHasDuplicatePips(input.value)) {
            beep = true;
        }

        if (suitHoldingWouldExceedHandLimit(input)) {
            beep = true;
        }

        if (suitHoldingRejectsFullHandEdit(input)) {
            beep = true;
        }
    }

    if (beep) {
        playIllegalInputBeep();
    }

    updateActionButtons(input);
}

function sameHandCardCountExcludingSuit(direction, excludedSuit) {
    let count = 0;

    for (const suit of SUITS) {
        if (suit === excludedSuit) {
            continue;
        }

        const input = document.getElementById(suitInputId(direction, suit));

        if (input) {
            count += sanitizeSuitHolding(input.value).length;
        }
    }

    return count;
}

function transferTypedCardsFromOtherHands(activeElement) {
    const parsed = parseHandInputId(activeElement && activeElement.id);

    if (!parsed) {
        return;
    }

    const otherCount = sameHandCardCountExcludingSuit(parsed.direction, parsed.suit);
    const previousHolding = rememberedSuitHolding(activeElement.id);
    const previousTotal = otherCount + previousHolding.length;
    const proposed = sanitizeSuitHolding(activeElement.value);
    let kept;

    if (previousTotal >= 13) {
        // Full hand: removals only — never add or swap pips via high-rank clamp.
        if (
            proposed.length > previousHolding.length ||
            !suitHoldingPipsAreSubset(proposed, previousHolding)
        ) {
            kept = previousHolding;
        } else {
            kept = proposed;
        }
    } else {
        const room = Math.max(0, 13 - otherCount);
        kept = sanitizeSuitHolding(activeElement.value, null, null, room);
    }

    activeElement.value = kept;

    for (const pip of kept) {
        const card = new Card(parsed.suit, pip);

        for (const direction of DIRECTIONS) {
            if (direction === parsed.direction) {
                continue;
            }

            removeCardFromHand(direction, card);
        }
    }
}

function sanitizeHandSuitInputs(activeElement) {
    if (activeElement) {
        transferTypedCardsFromOtherHands(activeElement);
    }

    const elements = Array.from(hand_elements());
    const ordered = activeElement && elements.indexOf(activeElement) >= 0
        ? elements.filter((element) => element !== activeElement).concat([activeElement])
        : elements;
    const claimed = {};

    for (const element of ordered) {
        const parsed = parseHandInputId(element && element.id);
        const suit = parsed && parsed.suit;
        const oldValue = element.value || "";
        const sanitized = sanitizeSuitHolding(oldValue, claimed, suit);

        if (sanitized !== oldValue) {
            const caret = typeof element.selectionStart === "number"
                ? element.selectionStart
                : oldValue.length;
            let keptBefore = 0;
            let pipBeforeCaret = "";

            for (let i = 0; i < oldValue.length && i < caret; i++) {
                const pip = oldValue.charAt(i).toUpperCase();

                if (PIPS.includes(pip)) {
                    keptBefore += 1;
                    pipBeforeCaret = pip;
                }
            }

            element.value = sanitized;

            let newCaret = sanitized.length;

            if (keptBefore <= 0) {
                newCaret = 0;
            } else if (pipBeforeCaret) {
                const at = sanitized.indexOf(pipBeforeCaret);
                newCaret = at >= 0 ? at + 1 : sanitized.length;
            }

            if (typeof element.setSelectionRange === "function") {
                element.setSelectionRange(newCaret, newCaret);
            } else {
                element.selectionStart = newCaret;
                element.selectionEnd = newCaret;
            }
        }

        if (suit) {
            for (const pip of sanitized) {
                claimed[new Card(suit, pip).key()] = true;
            }
        }
    }

    rememberSuitHoldings();
}

function updateActionButtons(activeElement) {
    sanitizeHandSuitInputs(activeElement);

    let hands = collectHands();
    const fillState = fourthHandFillState(hands);

    if (fillState.canFill) {
        applyFourthHandFill(hands, fillState.emptyHand);
        hands = collectHands();
    }

    // Leaving a complete 13-card deal drops contract/play selection so a later
    // partial is handled like a diagram entered from scratch (lead-on-click).
    const fullDeal = typeof dealIsFull === "function" && dealIsFull(hands);
    if (lastDealWasFull && !fullDeal) {
        if (typeof isPlayMode === "function" && isPlayMode() &&
                typeof exitPlay === "function") {
            exitPlay();
        } else if (selectedContractState) {
            clearResultCellSelectionHighlight();
            selectedContractState = null;
            leadTricksRequestId += 1;
            leadTricksByCardKey = null;
        }
    }
    lastDealWasFull = fullDeal;

    updateDeckStatus(hands);
    updateHandCardCounts(hands);
    updateResultTableHint(hands);
    updateResultTableCellLabels(hands);
    updateContractStatus();

    // Drop cached lead numerals immediately on any hand change so a slow
    // follow-up solve cannot leave stale badges on screen.
    if (leadTricksByCardKey || selectedContractState) {
        leadTricksRequestId += 1;
        leadTricksByCardKey = null;
    }

    updateHandCardDisplays(hands);

    const dealComplete = allHandsHaveEqualCardCounts(hands) &&
        inputIsValid(hands).length === 0;

    // Any diagram change supersedes an in-flight DD-table request so a delayed
    // Computing… timer cannot paint for the previous PBN during debounce.
    invalidateActiveDdTableRequest();

    // Debounce only while the deal stays solvable so typing on a complete deal
    // does not sync-ccall on every keystroke. Incomplete/invalid edits (and the
    // first transition to a complete deal) schedule immediately: clear/error
    // updates are cheap, and first completion should feel instant.
    if (dealComplete && lastDealWasComplete) {
        void scheduleDealSolveDebounced();
    } else {
        void scheduleDealSolve();
    }

    lastDealWasComplete = dealComplete;
}

function collectHands() {
    var hands = {};

    for (const ds of directions_and_suits()) {
        hands[ds.direction] = hands[ds.direction] || [];

        var element_index = ds.direction + "_" + ds.suit;
        var holding = document.getElementById(element_index).value;

        for (const pip of holding) {
            const upper = pip.toUpperCase();

            if (PIPS.includes(upper)) {
                hands[ds.direction].push(new Card(ds.suit, upper));
            }
        }
    }

    return hands;
}

function inputIsValid(hands) {
    const deck = {};
    const duplicates = [];
    const counts = DIRECTIONS.map(
        (direction) => (hands[direction] || []).length
    );
    const count = counts[0];

    if (count < 1 || count > 13 || !counts.every((n) => n === count)) {
        return "Please enter the same number of cards in each hand (1-13).";
    }

    for (const direction of DIRECTIONS) {
        const hand = hands[direction];

        for (const card of hand) {
            if (!PIPS.includes(card.pip)) {
                return "Please use only these pips: " + PIPS;
            }

            const key = card.key();
            if (deck[key]) {
                if (deck[key] == 1) {
                    duplicates.push(card);
                }

                deck[key]++;
            } else {
                deck[key] = 1;
            }
        }
    }

    if (duplicates.length) {
        var error_message = "Duplicated card";

        if (duplicates.length > 1) {
            error_message += "s";
        }

        error_message += ": ";

        for (const card of duplicates) {
            const suit_symbol = suitSymbolHtml(card.suit);

            error_message += suit_symbol;
            error_message += card.pip;
            error_message += " ";
        }

        return error_message;
    }

    return "";
}

function pageLoad() {
    document.getElementById("valid-pips").innerHTML = PIPS;
    populateSampleDealsMenu();
    applyMenuShortcuts(document);
    mountMenubar(document.querySelector
        ? document.querySelector(".menubar")
        : document.getElementById("menubar"));
    updateEditMenuEnablement();

    for (const element of hand_elements()) {
        element.addEventListener("input", handleHandSuitInput);
    }

    enhanceResultTableCells();
    document.addEventListener("mousedown", handleHandCardMouseDown);
    document.addEventListener("click", handleHandCardClick);
    document.addEventListener("click", handleHandSuitClick);
    document.addEventListener("dragstart", handleCardDragStart);
    document.addEventListener("dragover", handleCardDragOver);
    document.addEventListener("dragleave", handleCardDragLeave);
    document.addEventListener("drop", handleCardDrop);
    document.addEventListener("dragend", handleCardDragEnd);
    document.addEventListener("click", handleResultTableClick);
    document.addEventListener("keydown", handleResultTableKeyDown);
    if (typeof handlePlayUndoKeyDown === "function") {
        document.addEventListener("keydown", handlePlayUndoKeyDown);
    }
    if (typeof handlePlayHistoryUndo === "function") {
        document.addEventListener("beforeinput", handlePlayHistoryUndo);
    }
    document.addEventListener("selectionchange", handleSuitSelectionChange);
    if (typeof window !== "undefined" && window.addEventListener) {
        window.addEventListener("resize", fitDiagramFonts);
    }
    updateActionButtons();
    focusNorthSpades();
}

