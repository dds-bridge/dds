// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Unit tests: web/tests/dds_web_test.mjs
// Run with: bazelisk test //web:dds_web_js_test
// or: python -m unittest web.tests.test_dds_web_js
// or: node --test web/tests/dds_web_test.mjs

// ESLint configuration
// https://eslint.org/demo
//     ECMA Version: 2015
//     Environment: browser

/* eslint-env es6 */
/* exported fillFormWithGrandSlamTestData
            fillFormWithEveryoneMakes3nTestData
            fillFormWithPartScoreTestData
            clearTestData
            rotateClockwise
            pageLoad
            sendJSON
            refreshDdTable
            refreshOpeningLeadTricks
            scheduleDealSolve
            fourthHandFillState
            updateActionButtons
            sanitizeSuitHolding
            sanitizeHandSuitInputs
            suitHoldingHasIllegalChars
            playIllegalInputBeep
            handleHandSuitInput
            handCardHtml
            undeployedCardHtml
            handHoldingHtml
            escapeHtml
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
            openingLeader
            denominationDisplayHtml
            contractStatusHtml
            updateContractStatus
            pipFromDdsRank
            leadTricksMapFromSolverOutput
            wasmSolveEnvironmentError
            */

// It's also useful to pass the code through
// https://jshint.com/ and https://jslint.com/

"use strict";

const DIRECTIONS = ["north", "east", "south", "west"];
const SUITS = ["spades", "hearts", "diamonds", "clubs"];
const PIPS = "AKQJT98765432";
const DENOMINATIONS = ["C", "D", "H", "S", "N"];

// DDS res_table strain index (S,H,D,C,N) to DDS Web table column key.
const DENOM_TO_STRAIN = { C: 3, D: 2, H: 1, S: 0, N: 4 };
const DIR_TO_HAND = { north: 0, east: 1, south: 2, west: 3 };

let selectedContractState = null;
let leadTricksByCardKey = null;
let leadTricksRequestId = 0;
let ddTableRequestId = 0;
let lastDdTablePbn = null;
let solveQueue = Promise.resolve();
let dealSolveEpoch = 0;
let dealSolveQueued = false;

function enqueueSolve(task) {
    const run = solveQueue.then(task, task);

    // Keep the queue alive after a rejected solve.
    solveQueue = run.catch(() => {});
    return run;
}

// Coalesce DD-table + lead solves onto one queued job so rapid hand edits and
// contract clicks cannot interleave CalcDDtable with SolveBoard, and so
// intermediate schedules do not each add a stale promise-chain callback.
function scheduleDealSolve() {
    dealSolveEpoch += 1;

    if (dealSolveQueued) {
        return solveQueue;
    }

    dealSolveQueued = true;
    return enqueueSolve(async () => {
        try {
            while (true) {
                const epoch = dealSolveEpoch;

                await refreshDdTable();

                if (epoch !== dealSolveEpoch) {
                    continue;
                }

                if (selectedContractState) {
                    await refreshOpeningLeadTricks();
                } else if (leadTricksByCardKey) {
                    leadTricksByCardKey = null;
                    updateHandCardDisplays(collectHands());
                }

                if (epoch !== dealSolveEpoch) {
                    continue;
                }

                // Release the gate only once the epoch is stable; if a schedule
                // sneaks in between the check and the clear, take the flag back
                // and loop again instead of dropping the trailing request.
                dealSolveQueued = false;

                if (epoch !== dealSolveEpoch) {
                    dealSolveQueued = true;
                    continue;
                }

                break;
            }
        } catch (err) {
            dealSolveQueued = false;
            throw err;
        }
    });
}

// Suit glyphs are real text in these custom tags (see dds_web.css for color).
const SUIT_TAGS = {
    spades: "spade-suit",
    hearts: "heart-suit",
    diamonds: "diamond-suit",
    clubs: "club-suit"
};

const SUIT_GLYPHS = {
    spades: "\u2660",
    hearts: "\u2665",
    diamonds: "\u2666",
    clubs: "\u2663"
};

const PIP_NAMES = {
    A: "ace",
    K: "king",
    Q: "queen",
    J: "jack",
    T: "ten",
    "9": "nine",
    "8": "eight",
    "7": "seven",
    "6": "six",
    "5": "five",
    "4": "four",
    "3": "three",
    "2": "two"
};

function suitLetter(suit) {
    return suit.charAt(0).toUpperCase();
}

function suitFromLetter(letter) {
    for (const suit of SUITS) {
        if (suitLetter(suit) === letter) {
            return suit;
        }
    }

    return undefined;
}

function Card(suit, pip) {
    if (!SUITS.includes(suit)) {
        throw new Error("Invalid card suit: " + suit);
    }

    const normalizedPip = String(pip).toUpperCase();

    if (!PIPS.includes(normalizedPip)) {
        throw new Error("Invalid card pip: " + pip);
    }

    this.suit = suit;
    this.pip = normalizedPip;
}

Card.prototype.key = function () {
    return suitLetter(this.suit) + this.pip;
};

Card.prototype.toString = Card.prototype.key;

Card.fromKey = function (key) {
    const suit = suitFromLetter(key.charAt(0));

    if (!suit) {
        throw new Error("Invalid card key: " + key);
    }

    return new Card(suit, key.charAt(1));
};

Card.compare = function (left, right) {
    return PIPS.indexOf(left.pip) - PIPS.indexOf(right.pip);
};

function suitTag(suit) {
    return SUIT_TAGS[suit];
}

function suitSymbolHtml(suit) {
    const tag = suitTag(suit);

    return "<" + tag + ">" + SUIT_GLYPHS[suit] + "</" + tag + ">";
}

let ddsModulePromise = null;

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

    const envError = wasmSolveEnvironmentError();

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

function handsToPbn(hands) {
    const handStrings = DIRECTIONS.map((direction) => {
        return SUITS.map((suit) => {
            return hands[direction]
                .filter((card) => card.suit === suit)
                .sort(Card.compare)
                .map((card) => card.pip)
                .join("");
        }).join(".");
    });
    return "N:" + handStrings.join(" ");
}

function focusNorthSpades() {
    // To allow the user to quickly enter a deal

    document.getElementById("north_spades").focus();
}

function fillFormWithTestData(nesw) {
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

function fillFormWithGrandSlamTestData() {
    fillFormWithTestData([
        "AKQJ.AKQJ.T98.T9",
        "5432.5432.32.432",
        "T98.T9.AKQJ.AKQJ",
        "76.876.7654.8765"
    ]);
}

function fillFormWithEveryoneMakes3nTestData() {
    fillFormWithTestData([
        "QT9.A8765432.KJ.",
        "KJ..A8765432.QT9",
        "A8765432.QT9..KJ",
        ".KJ.QT9.A8765432"
    ]);
}

function fillFormWithPartScoreTestData() {
    fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5"
    ]);
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
    clear_results();

    for (const element of hand_elements()) {
        element.value = "";
    }

    updateActionButtons();
    focusNorthSpades();
}

function rotateClockwise() {
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

function updateDeckStatus(hands) {
    const deckStatus = document.getElementById("deck-status");

    if (deckStatus) {
        deckStatus.innerHTML = deckStatusHtml(hands);
    }
}

function openingLeader(declarerDirection) {
    const index = DIRECTIONS.indexOf(declarerDirection);

    if (index < 0) {
        return null;
    }

    return DIRECTIONS[(index + 1) % 4];
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

function contractStatusHtml(contract) {
    const denomHtml = denominationDisplayHtml(contract.denomination);
    const denomName = DENOM_ARIA_NAMES[contract.denomination];
    const declarer = contract.direction;

    if (!DIRECTIONS.includes(declarer) || !denomHtml || !denomName) {
        return "";
    }

    const declarerLetter = declarer.charAt(0).toUpperCase();
    const declarerName = capitalize(declarer);
    const aria = denomName + "; " + declarerName + " declares";

    return "<div class=\"contract-status-panel\" aria-label=\"" + aria + "\">" +
        "<div class=\"contract-status-denom\">" + denomHtml + "</div>" +
        "<div class=\"contract-status-by\">by</div>" +
        "<div class=\"contract-status-declarer\">" + declarerLetter + "</div>" +
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

    status.innerHTML = contractStatusHtml(contract);
    status.hidden = false;
}

function pipFromDdsRank(rank) {
    if (rank === 14) {
        return "A";
    }
    if (rank === 13) {
        return "K";
    }
    if (rank === 12) {
        return "Q";
    }
    if (rank === 11) {
        return "J";
    }
    if (rank === 10) {
        return "T";
    }
    if (rank >= 2 && rank <= 9) {
        return String(rank);
    }

    return null;
}

function leadTricksMapFromSolverOutput(out) {
    const map = {};
    const n = out[0] | 0;

    for (let i = 0; i < n; i++) {
        const suitIndex = out[1 + 3 * i];
        const rank = out[1 + 3 * i + 1];
        const score = out[1 + 3 * i + 2];
        const suit = SUITS[suitIndex];
        const pip = pipFromDdsRank(rank);

        if (suit && pip) {
            map[suitLetter(suit) + pip] = score;
        }
    }

    return map;
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

function handCardAriaLabel(direction, card) {
    const suitName = card.suit.replace(/s$/, "");
    const pipName = PIP_NAMES[card.pip] || card.pip;

    return capitalize(direction) + " " + suitName + " " + pipName;
}

function handCardHtml(direction, card, index, leadTricks) {
    const hasTricks = leadTricks != null && leadTricks !== undefined;
    const classes = hasTricks ? "hand-card hand-card-with-tricks" : "hand-card";
    const badge = hasTricks
        ? "<span class=\"hand-card-tricks\" aria-hidden=\"true\">" +
            escapeHtml(leadTricks) +
            "</span>"
        : "";

    return "<button type=\"button\" class=\"" + classes + "\" draggable=\"true\"" +
        " data-direction=\"" + escapeHtml(direction) + "\"" +
        " data-card=\"" + escapeHtml(card.key()) + "\"" +
        " data-index=\"" + escapeHtml(index) + "\"" +
        " aria-label=\"" + escapeHtml(handCardAriaLabel(direction, card)) + "\">" +
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
}

function onHandCardClick(_direction, _card) {
    // Hook for a future play-through PR; intentionally a no-op for now.
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

    const card = Card.fromKey(payload.key);

    if (!card.suit || !card.pip) {
        return;
    }

    const center = centerDropElement(event.target);

    if (center) {
        if (event.preventDefault) {
            event.preventDefault();
        }

        if (event.dataTransfer) {
            event.dataTransfer.dropEffect = "move";
        }

        setActiveDropTarget(center);
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

    const card = Card.fromKey(payload.key);

    if (!card.suit || !card.pip) {
        return;
    }

    if (event.preventDefault) {
        event.preventDefault();
    }

    const center = centerDropElement(event.target);

    if (center) {
        if (undeployCard(card)) {
            updateActionButtons();
        }

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

function onContractSelect(_direction, _denomination) {
    // Hook for play-through / UI that needs the chosen contract.
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

async function solveOpeningLeadTricks(hands, contract) {
    const leader = openingLeader(contract.direction);
    const trump = DENOM_TO_STRAIN[contract.denomination];
    const first = DIR_TO_HAND[leader];

    if (trump == null || first == null) {
        throw new Error("Invalid contract for lead analysis");
    }

    const module = await loadDdsModule();
    const pbn = handsToPbn(hands);
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
    const hands = collectHands();

    if (!contract || inputIsValid(hands).length) {
        leadTricksByCardKey = null;
        if (requestId === leadTricksRequestId) {
            updateHandCardDisplays(hands);
        }
        return;
    }

    try {
        const map = await solveOpeningLeadTricks(hands, contract);

        if (requestId !== leadTricksRequestId) {
            return;
        }

        leadTricksByCardKey = map;
        updateHandCardDisplays(collectHands());
    } catch (err) {
        if (requestId !== leadTricksRequestId) {
            return;
        }

        leadTricksByCardKey = null;
        updateHandCardDisplays(collectHands());

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

function resultCellAriaLabel(direction, denomination) {
    const denomName = DENOM_ARIA_NAMES[denomination];

    if (!DIRECTIONS.includes(direction) || !denomName) {
        return "";
    }

    return denomName + "; " + capitalize(direction) + " declares";
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
                    resultCellAriaLabel(direction, denomination)
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

function fourthHandFillState(hands) {
    const handCounts = DIRECTIONS.map((direction) => hands[direction].length);
    const fullHands = handCounts.filter((count) => count === 13).length;
    const emptyHands = handCounts.filter((count) => count === 0).length;
    const partialHands = handCounts.filter((count) => count > 0 && count < 13).length;

    if (fullHands !== 3 || emptyHands !== 1 || partialHands > 0) {
        return { canFill: false };
    }

    const emptyHand = DIRECTIONS[handCounts.indexOf(0)];
    const usedCards = {};

    for (const direction of DIRECTIONS) {
        if (direction === emptyHand) {
            continue;
        }

        for (const card of hands[direction]) {
            if (!card || !SUITS.includes(card.suit) || !PIPS.includes(card.pip)) {
                return { canFill: false };
            }

            usedCards[card.key()] = true;
        }
    }

    // Three full hands hold 39 cards; fewer distinct keys means a duplicate,
    // so the remaining 13 cannot be dealt to the empty hand.
    if (Object.keys(usedCards).length !== 39) {
        return { canFill: false };
    }

    return { canFill: true, emptyHand, usedCards };
}

function cardsToSuitHoldings(cards) {
    const holdings = {};

    for (const suit of SUITS) {
        holdings[suit] = "";
    }

    for (const card of cards) {
        holdings[card.suit] += card.pip;
    }

    for (const suit of SUITS) {
        holdings[suit] = holdings[suit]
            .split("")
            .sort((a, b) => PIPS.indexOf(a) - PIPS.indexOf(b))
            .join("");
    }

    return holdings;
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

function sortedPipInsertIndex(holding, pip) {
    const rank = PIPS.indexOf(pip);

    for (let i = 0; i < holding.length; i++) {
        if (PIPS.indexOf(holding.charAt(i)) > rank) {
            return i;
        }
    }

    return holding.length;
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

function allHandsHaveThirteenCards(hands) {
    return DIRECTIONS.every((direction) => hands[direction].length === 13);
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

function sanitizeSuitHolding(value, claimedKeys, suit, maxPips) {
    if (value == null) {
        return "";
    }

    const pips = [];
    const seen = {};

    for (const ch of String(value)) {
        const pip = ch.toUpperCase();

        if (!PIPS.includes(pip) || seen[pip]) {
            continue;
        }

        if (claimedKeys && suit) {
            const key = new Card(suit, pip).key();

            if (claimedKeys[key]) {
                continue;
            }
        }

        seen[pip] = true;
        pips.push(pip);
    }

    pips.sort((left, right) => PIPS.indexOf(left) - PIPS.indexOf(right));

    if (typeof maxPips === "number" && maxPips >= 0 && pips.length > maxPips) {
        return pips.slice(0, maxPips).join("");
    }

    return pips.join("");
}

function suitHoldingHasDuplicatePips(value) {
    if (value == null) {
        return false;
    }

    const seen = {};

    for (const ch of String(value)) {
        const pip = ch.toUpperCase();

        if (!PIPS.includes(pip)) {
            continue;
        }

        if (seen[pip]) {
            return true;
        }

        seen[pip] = true;
    }

    return false;
}

function suitHoldingHasIllegalChars(value) {
    if (value == null) {
        return false;
    }

    for (const ch of String(value)) {
        if (!PIPS.includes(ch.toUpperCase())) {
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

    const room = Math.max(0, 13 - sameHandCardCountExcludingSuit(parsed.direction, parsed.suit));
    const kept = sanitizeSuitHolding(activeElement.value, null, null, room);

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
    const handCounts = {};

    for (const element of ordered) {
        const parsed = parseHandInputId(element && element.id);
        const suit = parsed && parsed.suit;
        const direction = parsed && parsed.direction;
        const used = direction ? (handCounts[direction] || 0) : 0;
        const room = direction ? Math.max(0, 13 - used) : undefined;
        const oldValue = element.value || "";
        const sanitized = sanitizeSuitHolding(oldValue, claimed, suit, room);

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

        if (direction) {
            handCounts[direction] = used + sanitized.length;
        }
    }
}

function updateActionButtons(activeElement) {
    sanitizeHandSuitInputs(activeElement);

    let hands = collectHands();
    const fillState = fourthHandFillState(hands);

    if (fillState.canFill) {
        applyFourthHandFill(hands, fillState.emptyHand);
        hands = collectHands();
    }

    updateDeckStatus(hands);
    updateHandCardCounts(hands);

    // Drop cached lead numerals immediately on any hand change so a slow
    // follow-up solve cannot leave stale badges on screen.
    if (leadTricksByCardKey || selectedContractState) {
        leadTricksRequestId += 1;
        leadTricksByCardKey = null;
    }

    updateHandCardDisplays(hands);

    void scheduleDealSolve();
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

    for (const direction of Object.keys(hands)) {
        const hand = hands[direction];

        if (hand.length != 13) {
            return "Please enter 13 cards per hand.";
        }

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
    document.addEventListener("selectionchange", handleSuitSelectionChange);
    updateActionButtons();
    focusNorthSpades();
}

function clear_results() {
    var result = document.getElementById("result");
    var result_table = document.getElementById("result-table");

    lastDdTablePbn = null;
    result.innerHTML = "";

    for (var row = 1; row <= 4; row++) {
        for (var column = 1; column <= 5; column++) {
            var cell = result_table.rows[row].cells[column];
            cell.innerHTML = "";
        }
    }
}

async function refreshDdTable() {
    const requestId = ++ddTableRequestId;
    const result = document.getElementById("result");
    const result_table = document.getElementById("result-table");
    const hands = collectHands();

    if (!allHandsHaveThirteenCards(hands)) {
        if (requestId === ddTableRequestId) {
            lastDdTablePbn = null;
            clear_results();
        }
        return;
    }

    const error_message = inputIsValid(hands);

    if (error_message.length) {
        if (requestId === ddTableRequestId) {
            lastDdTablePbn = null;
            clear_results();
            if (result) {
                result.innerHTML = error_message;
            }
        }
        return;
    }

    const pbn = handsToPbn(hands);

    if (pbn === lastDdTablePbn && ddTableLooksPopulated(result_table)) {
        return;
    }

    if (requestId !== ddTableRequestId) {
        return;
    }

    clear_results();
    if (result) {
        result.innerHTML = "Computing&hellip;"; // horizontal ellipsis
    }

    try {
        const module = await loadDdsModule();
        const outPtr = module._malloc(20 * 4);

        try {
            const rc = module.ccall(
                "dds_web_calc_table",
                "number",
                ["string", "number"],
                [pbn, outPtr]
            );

            if (requestId !== ddTableRequestId) {
                return;
            }

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
                result.innerHTML = "";
            }
        } finally {
            module._free(outPtr);
        }
    } catch (err) {
        if (requestId !== ddTableRequestId) {
            return;
        }

        lastDdTablePbn = null;
        clear_results();
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
    return refreshDdTable();
}
