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

// Suit glyphs come from CSS :before on these custom tags (see dds_mvp.css).
const SUIT_TAGS = {
    spades: "spade-suit",
    hearts: "heart-suit",
    diamonds: "diamond-suit",
    clubs: "club-suit"
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
    this.suit = suit;
    this.pip = pip;
}

Card.prototype.key = function () {
    return suitLetter(this.suit) + this.pip;
};

Card.prototype.toString = Card.prototype.key;

Card.fromKey = function (key) {
    return new Card(suitFromLetter(key.charAt(0)), key.charAt(1));
};

Card.compare = function (left, right) {
    return PIPS.indexOf(left.pip) - PIPS.indexOf(right.pip);
};

function suitTag(suit) {
    return SUIT_TAGS[suit];
}

function suitSymbolHtml(suit) {
    const tag = suitTag(suit);

    return "<" + tag + "></" + tag + ">";
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

    for (const suit of SUITS) {
        for (const pip of PIPS) {
            cards.push(new Card(suit, pip));
        }
    }

    return cards;
}

function deckStatusHtml(hands) {
    const enteredCards = {};

    for (const direction of DIRECTIONS) {
        for (const card of hands[direction]) {
            enteredCards[card.key()] = true;
        }
    }

    return SUITS.map((suit) => {
        const tag = suitTag(suit);
        const cardsHtml = PIPS.split("").map((pip) => {
            const card = new Card(suit, pip);
            const key = card.key();
            const classes = ["deck-card"];

            if (enteredCards[key]) {
                classes.push("deck-card-entered");
            }

            return "<span class=\"" + classes.join(" ") +
                "\" data-card=\"" + key + "\">" + pip + "</span>";
        }).join("");

        return "<" + tag + ">" + cardsHtml + "</" + tag + ">";
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

        var element_index = ds.direction + "_" + ds.suit;
        var holding = document.getElementById(element_index).value;

        for (const pip of holding) {
            hands[ds.direction].push(new Card(ds.suit, pip.toUpperCase()));
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
