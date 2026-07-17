// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Unit tests: web/tests/dds_mvp_test.mjs
// Run with: bazel test //web:dds_mvp_js_test
// or: python -m unittest web.tests.test_dds_mvp_js
// or: node --test web/tests/dds_mvp_test.mjs

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
            fourthHandFillState
            updateActionButtons
            updateDefaultAction
            handleHandKeydown
            */

// It's also useful to pass the code through
// https://jshint.com/ and https://jslint.com/

"use strict";

const DIRECTIONS = ["north", "east", "south", "west"];
const SUITS = ["spades", "hearts", "diamonds", "clubs"];
const PIPS = "AKQJT98765432";
const DENOMINATIONS = ["C", "D", "H", "S", "N"];

// DDS res_table strain index (S,H,D,C,N) to MVP table column key.
const DENOM_TO_STRAIN = { C: 3, D: 2, H: 1, S: 0, N: 4 };
const DIR_TO_HAND = { north: 0, east: 1, south: 2, west: 3 };

// TODO: Clean up our HTML rendering, perhaps using custom elements.
//       See https://developers.google.com/web/fundamentals/web-components/customelements
const SUIT_SYMBOLS = {
    "S" : "&spades;",
    "H" : "&hearts;",
    "D" : "&diams;",
    "C" : "&clubs;"
};

function isRedSuit(suit) {
    return suit === "H" || suit === "D";
}

// Suit symbols are always black or red; the red is applied via CSS so callers
// that also gray out card pips (e.g. the deck status) can color independently.
function suitSymbolHtml(suit) {
    const symbol = SUIT_SYMBOLS[suit];

    return isRedSuit(suit)
        ? "<span class=\"suit-red\">" + symbol + "</span>"
        : symbol;
}

let ddsModulePromise = null;

function loadDdsModule() {
    if (typeof createDdsModule !== "function") {
        return Promise.reject(new Error(
            "WASM module not found. From the repo root run: ./web/update_wasm.sh"
        ));
    }

    if (!ddsModulePromise) {
        if (typeof ddsMvpWasmBytes !== "function") {
            return Promise.reject(new Error(
                "WASM bytes not found. From the repo root run: ./web/update_wasm.sh"
            ));
        }
        ddsModulePromise = createDdsModule({
            wasmBinary: ddsMvpWasmBytes()
        }).catch((error) => {
            // Allow retry after transient initialization failures.
            ddsModulePromise = null;
            throw error;
        });
    }

    return ddsModulePromise;
}

function handsToPbn(hands) {
    const suitOrder = ["S", "H", "D", "C"];
    const handStrings = DIRECTIONS.map((direction) => {
        return suitOrder.map((suit) => {
            return hands[direction]
                .filter((card) => card.charAt(0) === suit)
                .map((card) => card.charAt(1))
                .sort((a, b) => PIPS.indexOf(a) - PIPS.indexOf(b))
                .join("");
        }).join(".");
    });
    return "N:" + handStrings.join(" ");
}

function focusNorthSpades() {
    // To allow the user to quickly enter a deal

    document.getElementById("north_spades").focus();
}

function focusDoubleDummyButton() {
    const doubleDummyButton = document.getElementById("double-dummy-it");

    if (doubleDummyButton && !doubleDummyButton.disabled) {
        doubleDummyButton.focus();
        updateDefaultAction();
    }
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
    // Defer so the clicked test-deal button cannot reclaim focus after click.
    setTimeout(focusDoubleDummyButton, 0);
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

    for (const suit of ["S", "H", "D", "C"]) {
        for (const pip of PIPS) {
            cards.push(suit + pip);
        }
    }

    return cards;
}

function deckStatusHtml(hands) {
    const enteredCards = {};

    for (const direction of DIRECTIONS) {
        for (const card of hands[direction]) {
            enteredCards[card] = true;
        }
    }

    return ["S", "H", "D", "C"].map((suit) => {
        const redSuit = isRedSuit(suit);
        const symbolClasses = ["deck-suit-symbol"];

        if (redSuit) {
            symbolClasses.push("suit-red");
        }

        const cardsHtml = PIPS.split("").map((pip) => {
            const card = suit + pip;
            const classes = ["deck-card"];

            if (enteredCards[card]) {
                classes.push("deck-card-entered");
            }

            return "<span class=\"" + classes.join(" ") +
                "\" data-card=\"" + card + "\">" + pip + "</span>";
        }).join("");

        return "<span class=\"deck-suit-group\"><span class=\"" +
            symbolClasses.join(" ") + "\">" + SUIT_SYMBOLS[suit] +
            "</span>" + cardsHtml + "</span>";
    }).join("");
}

function updateDeckStatus(hands) {
    const deckStatus = document.getElementById("deck-status");

    if (deckStatus) {
        deckStatus.innerHTML = deckStatusHtml(hands);
    }
}

function updateHandCardCounts(hands) {
    for (const direction of DIRECTIONS) {
        const count = hands[direction].length;
        const note = document.getElementById(direction + "-card-count");

        if (note) {
            note.hidden = count <= 13;
            note.innerHTML = count > 13 ? count + " cards" : "";
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
            const pip = card.substring(1);

            if (!PIPS.includes(pip)) {
                return { canFill: false };
            }

            if (usedCards[card]) {
                return { canFill: false };
            }

            usedCards[card] = true;
        }
    }

    if (Object.keys(usedCards).length !== 39) {
        return { canFill: false };
    }

    return { canFill: true, emptyHand, usedCards };
}

function cardsToSuitHoldings(cards) {
    const holdings = { S: "", H: "", D: "", C: "" };

    for (const card of cards) {
        holdings[card.charAt(0)] += card.charAt(1);
    }

    for (const suit of ["S", "H", "D", "C"]) {
        holdings[suit] = holdings[suit]
            .split("")
            .sort((a, b) => PIPS.indexOf(a) - PIPS.indexOf(b))
            .join("");
    }

    return holdings;
}

function setHandInputs(direction, holdings) {
    for (const suit of SUITS) {
        const suitLetter = suit.charAt(0).toUpperCase();
        document.getElementById(direction + "_" + suit).value = holdings[suitLetter];
    }
}

function remainingCardsForEmptyHand(state) {
    return allDeckCards().filter((card) => !state.usedCards[card]);
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

function enterWouldActivateDoubleDummy() {
    const doubleDummyButton = document.getElementById("double-dummy-it");
    const activeElement = document.activeElement;

    if (!doubleDummyButton || doubleDummyButton.disabled) {
        return false;
    }

    return isHandInput(activeElement) || activeElement === doubleDummyButton;
}

function updateDefaultAction() {
    const doubleDummyButton = document.getElementById("double-dummy-it");

    if (!doubleDummyButton) {
        return;
    }

    if (enterWouldActivateDoubleDummy()) {
        doubleDummyButton.classList.add("default-action");
    } else {
        doubleDummyButton.classList.remove("default-action");
    }
}

function updateActionButtons() {
    let hands = collectHands();
    const fillState = fourthHandFillState(hands);
    const doubleDummyButton = document.getElementById("double-dummy-it");

    if (fillState.canFill) {
        applyFourthHandFill(hands, fillState.emptyHand);
        hands = collectHands();
    }

    updateDeckStatus(hands);
    updateHandCardCounts(hands);

    if (doubleDummyButton) {
        doubleDummyButton.disabled = !allHandsHaveThirteenCards(hands);
    }

    updateDefaultAction();
}

function handleHandKeydown(event) {
    if (event.key !== "Enter" || !isHandInput(event.target)) {
        return;
    }

    const hands = collectHands();

    if (allHandsHaveThirteenCards(hands)) {
        event.preventDefault();
        sendJSON();
    }
}

function collectHands() {
    var hands = {};

    for (const ds of directions_and_suits()) {
        hands[ds.direction] = hands[ds.direction] || [];

        var suit_letter = ds.suit.charAt(0).toUpperCase();
        var element_index = ds.direction + "_" + ds.suit;
        var holding = document.getElementById(element_index).value;

        for (const card of holding) {
            hands[ds.direction].push(suit_letter + card.toUpperCase());
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
            const pip = card.substring(1);

            if (!PIPS.includes(pip)) {
                return "Please use only these pips: " + PIPS;
            }

            if (deck[card]) {
                if (deck[card] == 1) {
                    duplicates.push(card);
                }

                deck[card]++;
            } else {
                deck[card] = 1;
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
            const suit_letter = card.substring(0, 1);
            const pip = card.substring(1);
            const suit_symbol = suitSymbolHtml(suit_letter);

            error_message += suit_symbol;
            error_message += pip;
            error_message += " ";
        }

        return error_message;
    }

    return "";
}

function pageLoad() {
    document.getElementById("valid-pips").innerHTML = PIPS;

    for (const element of hand_elements()) {
        element.addEventListener("input", updateActionButtons);
        element.addEventListener("keydown", handleHandKeydown);
    }

    document.addEventListener("focusin", updateDefaultAction);
    document.addEventListener("focusout", updateDefaultAction);
    updateActionButtons();
    focusNorthSpades();
}

function clear_results() {
    var result = document.getElementById("result");
    var result_table = document.getElementById("result-table");

    result.innerHTML = "";

    for (var row = 1; row <= 4; row++) {
        for (var column = 1; column <= 5; column++) {
            var cell = result_table.rows[row].cells[column];
            cell.innerHTML = "";
        }
    }
}

async function sendJSON() {
    const result = document.getElementById("result");
    const result_table = document.getElementById("result-table");

    var hands = collectHands();

    const error_message = inputIsValid(hands);

    if (error_message.length) {
        clear_results();
        result.innerHTML = error_message;
        return;
    }

    clear_results();
    result.innerHTML = "Computing&hellip;"; // horizontal ellipsis

    try {
        const module = await loadDdsModule();
        const pbn = handsToPbn(hands);
        const outPtr = module._malloc(20 * 4);

        try {
            const rc = module.ccall(
                "dds_mvp_calc_table",
                "number",
                ["string", "number"],
                [pbn, outPtr]
            );

            if (rc !== 1) {
                result.innerHTML = "DDS error (code " + rc + ").";
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

            result.innerHTML = "";
        } finally {
            module._free(outPtr);
        }
    } catch (err) {
        clear_results();
        result.innerHTML = err instanceof Error
            ? err.message
            : err == null
                ? "Unknown error"
                : String(err);
    }
}
