// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Deal model for DDS Web (Card, holdings, PBN marshal). No DOM or WASM.
// Loaded after dds_web_deal_import.js and before dds_web_solve.js.

/* eslint-env es6 */
/* exported DIRECTIONS SUITS PIPS DENOMINATIONS DENOM_TO_STRAIN DIR_TO_HAND
            Card handsToPbn openingLeader pipFromDdsRank leadTricksMapFromSolverOutput
            fourthHandFillState cardsToSuitHoldings sortedPipInsertIndex
            allHandsHaveThirteenCards sanitizeSuitHolding
            suitHoldingHasDuplicatePips suitHoldingHasIllegalChars */

"use strict";

(function (global) {
    // Shared with dds_web_deal_import.js (loaded first); keep a single definition.
    const DIRECTIONS = global.DIRECTIONS;
    const SUITS = ["spades", "hearts", "diamonds", "clubs"];
    const PIPS = global.PIPS;
    const DENOMINATIONS = ["C", "D", "H", "S", "N"];

    // DDS res_table strain index (S,H,D,C,N) to DDS Web table column key.
    const DENOM_TO_STRAIN = { C: 3, D: 2, H: 1, S: 0, N: 4 };
    const DIR_TO_HAND = { north: 0, east: 1, south: 2, west: 3 };

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
        if (typeof key !== "string" || key.length !== 2) {
            throw new Error("Invalid card key: " + key);
        }

        const normalized = key.toUpperCase();
        const suit = suitFromLetter(normalized.charAt(0));

        if (!suit) {
            throw new Error("Invalid card key: " + key);
        }

        return new Card(suit, normalized.charAt(1));
    };

    function cardFromKeySafe(key) {
        try {
            return Card.fromKey(key);
        } catch (_err) {
            return null;
        }
    }

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

    function openingLeader(declarerDirection) {
        const index = DIRECTIONS.indexOf(declarerDirection);

        if (index < 0) {
            return null;
        }

        return DIRECTIONS[(index + 1) % 4];
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

    function sortedPipInsertIndex(holding, pip) {
        const rank = PIPS.indexOf(pip);

        for (let i = 0; i < holding.length; i++) {
            if (PIPS.indexOf(holding.charAt(i)) > rank) {
                return i;
            }
        }

        return holding.length;
    }

    function allHandsHaveThirteenCards(hands) {
        return DIRECTIONS.every((direction) => hands[direction].length === 13);
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

    global.DIRECTIONS = DIRECTIONS;
    global.SUITS = SUITS;
    global.PIPS = PIPS;
    global.DENOMINATIONS = DENOMINATIONS;
    global.DENOM_TO_STRAIN = DENOM_TO_STRAIN;
    global.DIR_TO_HAND = DIR_TO_HAND;
    global.SUIT_TAGS = SUIT_TAGS;
    global.SUIT_GLYPHS = SUIT_GLYPHS;
    global.PIP_NAMES = PIP_NAMES;
    global.suitLetter = suitLetter;
    global.suitFromLetter = suitFromLetter;
    global.Card = Card;
    global.cardFromKeySafe = cardFromKeySafe;
    global.suitTag = suitTag;
    global.suitSymbolHtml = suitSymbolHtml;
    global.handsToPbn = handsToPbn;
    global.openingLeader = openingLeader;
    global.pipFromDdsRank = pipFromDdsRank;
    global.leadTricksMapFromSolverOutput = leadTricksMapFromSolverOutput;
    global.fourthHandFillState = fourthHandFillState;
    global.cardsToSuitHoldings = cardsToSuitHoldings;
    global.sortedPipInsertIndex = sortedPipInsertIndex;
    global.allHandsHaveThirteenCards = allHandsHaveThirteenCards;
    global.sanitizeSuitHolding = sanitizeSuitHolding;
    global.suitHoldingHasDuplicatePips = suitHoldingHasDuplicatePips;
    global.suitHoldingHasIllegalChars = suitHoldingHasIllegalChars;
})(typeof globalThis !== "undefined" ? globalThis : this);
