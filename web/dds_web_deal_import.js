// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Pure PBN / LIN / DLM / dtest / sol-style deal parsers for DDS Web.
// Loaded before dds_web.js; exports parseFirstDealFromText on globalThis.

/* eslint-env es6 */
/* exported parseFirstDealFromText */

"use strict";

(function (global) {
    const DIRECTIONS = ["north", "east", "south", "west"];
    const PIPS = "AKQJT98765432";
    const SUIT_LETTERS = ["S", "H", "D", "C"];
    const DIR_FROM_LETTER = { N: "north", E: "east", S: "south", W: "west" };
    const LIN_HAND_ORDER = ["south", "west", "north", "east"];

    /** Sort pips high-to-low using the diagram's pip order. */
    function sortPips(holding) {
        return String(holding)
            .toUpperCase()
            .split("")
            .filter((pip) => PIPS.includes(pip))
            .sort((a, b) => PIPS.indexOf(a) - PIPS.indexOf(b))
            .join("");
    }

    /**
     * True when a dotted hand has exactly four suit components of legal ranks only.
     * A lone "-" is accepted as the PBN void-suit marker. Does not pad or truncate.
     */
    function isValidRawHandHolding(dotted) {
        const parts = String(dotted).split(".");
        if (parts.length !== 4) {
            return false;
        }
        for (const part of parts) {
            if (part === "" || part === "-") {
                continue;
            }
            for (const ch of part) {
                if (!PIPS.includes(ch.toUpperCase())) {
                    return false;
                }
            }
        }
        return true;
    }

    function normalizeHandHolding(dotted) {
        if (!isValidRawHandHolding(dotted)) {
            throw new Error("Deal has a malformed hand holding.");
        }
        return String(dotted)
            .split(".")
            .map((part) => (part === "-" ? "" : sortPips(part)))
            .join(".");
    }

    function emptySuitHoldings() {
        return { S: "", H: "", D: "", C: "" };
    }

    function holdingsToDotted(holdings) {
        return SUIT_LETTERS.map((suit) => sortPips(holdings[suit] || "")).join(".");
    }

    /** True when the four hands are 13 cards each with no duplicates (full deck). */
    function dealHasUniqueCards(deal) {
        const seen = {};
        for (const direction of DIRECTIONS) {
            const holding = deal[direction];
            if (!holding) {
                return false;
            }
            const parts = holding.split(".");
            let count = 0;
            for (let i = 0; i < 4; i++) {
                const suit = SUIT_LETTERS[i];
                for (const pip of (parts[i] || "").toUpperCase()) {
                    if (!PIPS.includes(pip)) {
                        return false;
                    }
                    const key = suit + pip;
                    if (seen[key]) {
                        return false;
                    }
                    seen[key] = true;
                    count += 1;
                }
            }
            if (count !== 13) {
                return false;
            }
        }
        return Object.keys(seen).length === 52;
    }

    function dealFromDirectionMap(byDirection) {
        const deal = {};
        for (const direction of DIRECTIONS) {
            if (!byDirection[direction]) {
                return null;
            }
            deal[direction] = normalizeHandHolding(byDirection[direction]);
            if (deal[direction].replace(/\./g, "").length !== 13) {
                return null;
            }
        }
        if (!dealHasUniqueCards(deal)) {
            throw new Error("Deal has duplicated cards.");
        }
        return deal;
    }

    function completeMissingHand(byDirection) {
        const present = DIRECTIONS.filter((direction) => byDirection[direction]);
        if (present.length === 4) {
            return byDirection;
        }
        if (present.length !== 3) {
            return null;
        }

        const used = {};
        for (const direction of present) {
            const parts = byDirection[direction].split(".");
            for (let i = 0; i < 4; i++) {
                for (const pip of parts[i] || "") {
                    used[SUIT_LETTERS[i] + pip.toUpperCase()] = true;
                }
            }
        }

        const missing = DIRECTIONS.find((direction) => !byDirection[direction]);
        const holdings = emptySuitHoldings();
        for (let i = 0; i < 4; i++) {
            const suit = SUIT_LETTERS[i];
            for (const pip of PIPS) {
                if (!used[suit + pip]) {
                    holdings[suit] += pip;
                }
            }
        }
        byDirection[missing] = holdingsToDotted(holdings);
        return byDirection;
    }

    /**
     * Parse a PBN remainCards string such as "N:AKQ.... ..." into NESW holdings.
     * Later hands are clockwise from the first seat letter; no extra seat letters.
     */
    function parsePbnDealString(raw) {
        const text = String(raw).trim();
        const match = /^([NESWnesw]):\s*(.+)$/.exec(text);
        if (!match) {
            return null;
        }

        const start = match[1].toUpperCase();
        const hands = match[2].trim().split(/\s+/).filter(Boolean);
        if (hands.length !== 4) {
            return null;
        }

        const startIndex = "NESW".indexOf(start);
        const byDirection = {};
        for (let i = 0; i < 4; i++) {
            const direction = DIR_FROM_LETTER["NESW"[(startIndex + i) % 4]];
            const holding = normalizeHandHolding(hands[i]);
            if (holding.replace(/\./g, "").length !== 13) {
                return null;
            }
            byDirection[direction] = holding;
        }
        return dealFromDirectionMap(byDirection);
    }

    function parseLinHand(raw) {
        const holdings = emptySuitHoldings();
        let suit = null;
        for (const ch of String(raw)) {
            const upper = ch.toUpperCase();
            if (SUIT_LETTERS.includes(upper)) {
                suit = upper;
                continue;
            }
            if (PIPS.includes(upper)) {
                if (!suit) {
                    throw new Error("LIN hand has a rank before a suit.");
                }
                holdings[suit] += upper;
                continue;
            }
            throw new Error("LIN hand has illegal characters.");
        }
        return holdingsToDotted(holdings);
    }

    function parseLinDealPayload(payload) {
        // md|<dealer-digit><hand>,<hand>,<hand>,[<hand>]
        // BBO lists hands in fixed South, West, North, East order. The leading
        // digit is only the dealer (1=South, 2=West, 3=North, 4=East); it does not
        // rotate which hand comes first in the list.
        const body = String(payload).replace(/^[1-4]/, "");
        const parts = body.split(",");
        // Trailing commas are common in BBO exports; ignore empty trailing slots.
        while (parts.length && !String(parts[parts.length - 1]).trim()) {
            parts.pop();
        }
        if (parts.length < 3 || parts.length > 4) {
            return null;
        }

        const byDirection = {};
        for (let i = 0; i < 4; i++) {
            const raw = (parts[i] || "").trim();
            if (!raw) {
                continue;
            }
            byDirection[LIN_HAND_ORDER[i]] = parseLinHand(raw);
        }
        const completed = completeMissingHand(byDirection);
        return completed ? dealFromDirectionMap(completed) : null;
    }

    function parseDlmBoardPayload(letters) {
        // 26 letters a-p; each encodes owners of a fixed high/low card pair.
        const pairs = [
            ["SA", "SK"], ["SQ", "SJ"], ["ST", "S9"], ["S8", "S7"],
            ["S6", "S5"], ["S4", "S3"], ["S2", "HA"],
            ["HK", "HQ"], ["HJ", "HT"], ["H9", "H8"], ["H7", "H6"],
            ["H5", "H4"], ["H3", "H2"],
            ["DA", "DK"], ["DQ", "DJ"], ["DT", "D9"], ["D8", "D7"],
            ["D6", "D5"], ["D4", "D3"], ["D2", "CA"],
            ["CK", "CQ"], ["CJ", "CT"], ["C9", "C8"], ["C7", "C6"],
            ["C5", "C4"], ["C3", "C2"],
        ];
        const firstOwner = "NNNNEEEESSSSWWWW";
        const secondOwner = "NESWNESWNESWNESW";
        const byDirection = {
            north: emptySuitHoldings(),
            east: emptySuitHoldings(),
            south: emptySuitHoldings(),
            west: emptySuitHoldings(),
        };

        for (let i = 0; i < 26; i++) {
            const code = letters.charCodeAt(i) - "a".charCodeAt(0);
            if (code < 0 || code > 15) {
                return null;
            }
            const [firstCard, secondCard] = pairs[i];
            const firstDirection = DIR_FROM_LETTER[firstOwner.charAt(code)];
            byDirection[firstDirection][firstCard.charAt(0)] += firstCard.charAt(1);
            const secondDirection = DIR_FROM_LETTER[secondOwner.charAt(code)];
            byDirection[secondDirection][secondCard.charAt(0)] += secondCard.charAt(1);
        }

        return dealFromDirectionMap({
            north: holdingsToDotted(byDirection.north),
            east: holdingsToDotted(byDirection.east),
            south: holdingsToDotted(byDirection.south),
            west: holdingsToDotted(byDirection.west),
        });
    }

    /**
     * Parse a sol*.txt style line: four NESW holdings, optional ":results" suffix.
     * Example: T5.K4.652.A98542 K6.... AQJ987.8532.84.K:6565...
     * An optional leading board number ("1. ") is accepted and ignored.
     */
    function parseSolStyleDealLine(line) {
        let beforeColon = String(line).split(":")[0].trim();
        if (!beforeColon) {
            return null;
        }
        beforeColon = beforeColon.replace(/^\d+\.\s+/, "");
        const hands = beforeColon.split(/\s+/).filter(Boolean);
        if (hands.length !== 4) {
            return null;
        }
        for (const hand of hands) {
            if ((hand.match(/\./g) || []).length !== 3) {
                return null;
            }
        }
        return parsePbnDealString("N:" + hands.join(" "));
    }

    /**
     * Extract the first deal from PBN, LIN, DLM, dtest, or sol-style .txt content.
     * @returns {{north:string,east:string,south:string,west:string}}
     */
    function parseFirstDealFromText(text) {
        const source = String(text == null ? "" : text);

        const pbnTag = /\[Deal\s+"([^"]+)"\s*\]/i.exec(source);
        if (pbnTag) {
            const deal = parsePbnDealString(pbnTag[1]);
            if (deal) {
                return deal;
            }
        }

        const dtestLine = /^PBN\s+\d+\s+\d+\s+\d+\s+\d+\s+"([^"]+)"/im.exec(source);
        if (dtestLine) {
            const deal = parsePbnDealString(dtestLine[1]);
            if (deal) {
                return deal;
            }
        }

        const linMatch = /\bmd\|([^|]+)/i.exec(source);
        if (linMatch) {
            const deal = parseLinDealPayload(linMatch[1]);
            if (deal) {
                return deal;
            }
        }

        const dlmMatch = /Board\s*\d+\s*=\s*([a-p]{26})/i.exec(source);
        if (dlmMatch) {
            const deal = parseDlmBoardPayload(dlmMatch[1].toLowerCase());
            if (deal) {
                return deal;
            }
        }

        // Bare PBN remainCards line (no tag).
        const bare = /^\s*([NESWnesw]:[^\n\r"]+)/m.exec(source);
        if (bare) {
            const deal = parsePbnDealString(bare[1].trim());
            if (deal) {
                return deal;
            }
        }

        // sol10.txt-style: four NESW holdings, optional CalcTable suffix after ':'.
        for (const line of source.split(/\r?\n/)) {
            const deal = parseSolStyleDealLine(line);
            if (deal) {
                return deal;
            }
        }

        throw new Error(
            "No PBN, LIN, DLM, dtest, or sol-style deal found in the file."
        );
    }

    global.parseFirstDealFromText = parseFirstDealFromText;
})(typeof globalThis !== "undefined" ? globalThis : this);
