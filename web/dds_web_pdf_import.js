// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Hand-record PDF parsers for DDS Web (BridgeComposer / Deep Finesse / compass).
// Loaded after dds_web_deal_import.js; exports extract/parse helpers on globalThis.

/* eslint-env es6 */
/* exported normalizeBridgeHolding parseBridgeComposerBoard
            parseAllBridgeComposerBoards parseAllDeepFinesseBoards
            parseAllCompassHandRecordBoards pdfLatin1
            extractPdfTextItems loadAllBoardsFromHandRecordPdf handsToDottedDeal */

"use strict";

(function (global) {
    const DIRECTIONS = ["north", "east", "south", "west"];
    const SUITS = ["spades", "hearts", "diamonds", "clubs"];

    // BridgeComposer / ACBL hand-record PDFs encode suit symbols as WinAnsi bytes
    // (ª©¨§) and void as U+0097. Holdings write the ten as "10".
    const BRIDGE_SUIT_BY_CHAR = {
        "\u00aa": "spades",
        "\u00a9": "hearts",
        "\u00a8": "diamonds",
        "\u00a7": "clubs"
    };
    const BRIDGE_VOID_CHAR = "\u0097";

    // Typical BridgeComposer NABC sheet: 1200×1260 cells.
    const CELL_W = 1200;
    const CELL_H = 1260;

    // Deep Finesse regional sheets pack ~4×5 diagrams per page (~149×168 cells).
    const DF_CELL_W = 145;
    const DF_CELL_H = 140;

    function normalizeBridgeHolding(raw) {
        if (!raw || raw === BRIDGE_VOID_CHAR || raw === "-" || raw === "—") {
            return "";
        }
        return String(raw).replace(/10/g, "T");
    }

    function isBridgeCardHoldingText(text) {
        return text === BRIDGE_VOID_CHAR
            || text === "-"
            || text === "—"
            || /^[AKQJT9876543210]+$/.test(text);
    }

    function bridgeHoldingBesideSuit(items, suitItem) {
        let best = null;
        for (const item of items) {
            if (item === suitItem) {
                continue;
            }
            if (Math.abs(item.y - suitItem.y) > 8) {
                continue;
            }
            if (item.x <= suitItem.x || item.x >= suitItem.x + 180) {
                continue;
            }
            if (!isBridgeCardHoldingText(item.text)) {
                continue;
            }
            if (!best || item.x < best.x) {
                best = item;
            }
        }
        return best ? normalizeBridgeHolding(best.text) : "";
    }

    function bridgeSeatForSuit(dealerX, dealerY, suitX, suitY) {
        const relX = suitX - dealerX;
        const relY = dealerY - suitY;
        // North / South column sits ~400pt right of the dealer label; West is near
        // dealerX; East is ~750pt right. Vertical bands are relative to "N Deals".
        if (relX >= 250 && relX <= 550 && relY >= 0 && relY <= 320) {
            return "north";
        }
        if (relX >= -40 && relX <= 180 && relY > 280 && relY <= 720) {
            return "west";
        }
        if (relX >= 650 && relX <= 950 && relY > 280 && relY <= 720) {
            return "east";
        }
        // South ends near relY 906; footer/diagram suit marks around 986+ must
        // not overwrite south holdings (they sit in the same X column).
        if (relX >= 250 && relX <= 550 && relY > 600 && relY <= 950) {
            return "south";
        }
        return null;
    }

    function emptySuitHoldings() {
        return { spades: "", hearts: "", diamonds: "", clubs: "" };
    }

    function dealerMarkers(items) {
        return items.filter(function (item) {
            return /^(N|E|S|W) Deals$/.test(item.text);
        });
    }

    function itemsInDealerCell(items, dealer) {
        return items.filter(function (item) {
            return item.x >= dealer.x - 30
                && item.x < dealer.x + CELL_W
                && item.y <= dealer.y + 40
                && item.y > dealer.y - CELL_H;
        });
    }

    function boardNumberCandidates(inCell, dealer, options) {
        const requireDiamondRow = !!(options && options.requireDiamondRow);
        const northDiamonds = inCell.filter(function (item) {
            return item.text === "\u00a8"
                && item.x > dealer.x + 250
                && item.x < dealer.x + 550;
        });
        const candidates = [];
        for (const item of inCell) {
            if (!/^[1-9]\d*$/.test(item.text)) {
                continue;
            }
            // Board numbers sit right of center; left-side digits are holdings
            // or HCP (e.g. diamond holding "43" shares North's diamond row).
            if (item.x <= dealer.x + 800) {
                continue;
            }
            const onDiamondRow = northDiamonds.some(function (nd) {
                return Math.abs(nd.y - item.y) <= 10;
            });
            // Discovery must use the North diamond-row digit; otherwise a lone
            // East "10" holding is mistaken for board 10.
            if (requireDiamondRow && !onDiamondRow) {
                continue;
            }
            candidates.push({
                number: item.text,
                prefer: onDiamondRow,
                x: item.x,
                y: item.y
            });
        }
        candidates.sort(function (a, b) {
            if (a.prefer !== b.prefer) {
                return a.prefer ? -1 : 1;
            }
            return a.x - b.x;
        });
        return candidates;
    }

    function parseHandsInCell(inCell, dealer, boardLabel) {
        const hands = {
            north: emptySuitHoldings(),
            east: emptySuitHoldings(),
            south: emptySuitHoldings(),
            west: emptySuitHoldings()
        };

        for (const item of inCell) {
            const suit = BRIDGE_SUIT_BY_CHAR[item.text];
            if (!suit) {
                continue;
            }
            const seat = bridgeSeatForSuit(dealer.x, dealer.y, item.x, item.y);
            if (!seat) {
                continue;
            }
            hands[seat][suit] = bridgeHoldingBesideSuit(inCell, item);
        }

        for (const direction of DIRECTIONS) {
            let pips = "";
            for (const suit of SUITS) {
                pips += hands[direction][suit];
            }
            if (pips.length !== 13) {
                throw new Error(
                    "board " + boardLabel + ": " + direction
                    + " has " + pips.length + " cards after parse (expected 13)"
                );
            }
        }
        return hands;
    }

    function parseBridgeComposerBoard(items, boardNumber) {
        const want = String(boardNumber).trim();
        if (!/^[1-9]\d*$/.test(want)) {
            throw new Error(
                "board number must be a positive integer (got: " + boardNumber + ")"
            );
        }

        const dealers = dealerMarkers(items);
        if (dealers.length === 0) {
            throw new Error("no BridgeComposer board diagrams found in PDF text");
        }

        for (const dealer of dealers) {
            const inCell = itemsInDealerCell(items, dealer);
            const marks = boardNumberCandidates(inCell, dealer).filter(
                function (candidate) {
                    return candidate.number === want;
                }
            );
            if (marks.length === 0) {
                continue;
            }
            return parseHandsInCell(inCell, dealer, want);
        }

        throw new Error("board " + want + " not found in hand-record PDF");
    }

    /**
     * Parse every BridgeComposer board diagram into an ordered list.
     * @returns {{boardNumber: string, hands: object}[]}
     */
    function parseAllBridgeComposerBoards(items) {
        const dealers = dealerMarkers(items);
        if (dealers.length === 0) {
            throw new Error("no BridgeComposer board diagrams found in PDF text");
        }

        const boards = [];
        const seen = {};
        for (const dealer of dealers) {
            const inCell = itemsInDealerCell(items, dealer);
            const candidates = boardNumberCandidates(inCell, dealer, {
                requireDiamondRow: true,
            });
            if (candidates.length === 0) {
                continue;
            }
            const boardNumber = candidates[0].number;
            if (seen[boardNumber]) {
                continue;
            }
            // Skip incomplete adjacent/clipped cells (common at page edges).
            let hands;
            try {
                hands = parseHandsInCell(inCell, dealer, boardNumber);
            } catch (_err) {
                continue;
            }
            seen[boardNumber] = true;
            boards.push({ boardNumber: boardNumber, hands: hands });
        }

        if (boards.length === 0) {
            throw new Error("no numbered boards found in hand-record PDF");
        }

        boards.sort(function (a, b) {
            return Number(a.boardNumber) - Number(b.boardNumber);
        });
        return boards;
    }

    /**
     * Deep Finesse / ACBL regional sheets use the same "N Deals" anchors as
     * BridgeComposer but print holdings as space-separated ranks (ten as "10",
     * void as "-----") with no WinAnsi suit glyphs.
     */
    function isDeepFinesseHoldingText(text) {
        if (text === "-----" || text === "-" || text === "—") {
            return true;
        }
        return /^(?:10|[AKQJT98765432])(?: (?:10|[AKQJT98765432]))*$/.test(text);
    }

    function normalizeDeepFinesseHolding(raw) {
        if (!raw || raw === "-----" || raw === "-" || raw === "—") {
            return "";
        }
        return String(raw).replace(/ /g, "").replace(/10/g, "T");
    }

    function itemsInDeepFinesseCell(items, dealer) {
        return items.filter(function (item) {
            return item.x >= dealer.x - 5
                && item.x < dealer.x + DF_CELL_W
                && item.y <= dealer.y + 15
                && item.y > dealer.y - DF_CELL_H;
        });
    }

    function deepFinesseBoardNumber(inCell, dealer) {
        const candidates = inCell.filter(function (item) {
            return /^[1-9]\d*$/.test(item.text)
                && item.x >= dealer.x + 100
                && item.x <= dealer.x + 140
                && Math.abs(item.y - dealer.y) <= 20;
        });
        candidates.sort(function (a, b) {
            return a.x - b.x;
        });
        return candidates.length > 0 ? candidates[0].text : null;
    }

    function deepFinesseSeatHoldings(
        inCell, dealer, xMin, xMax, yMin, yMax, boardLabel, seat
    ) {
        const lines = inCell.filter(function (item) {
            const relX = item.x - dealer.x;
            const relY = dealer.y - item.y;
            return relX >= xMin && relX <= xMax
                && relY >= yMin && relY <= yMax
                && isDeepFinesseHoldingText(item.text);
        }).sort(function (a, b) {
            return b.y - a.y;
        });
        if (lines.length < 4) {
            throw new Error(
                "board " + boardLabel + ": " + seat
                + " has " + lines.length + " suit lines (expected 4)"
            );
        }
        return {
            spades: normalizeDeepFinesseHolding(lines[0].text),
            hearts: normalizeDeepFinesseHolding(lines[1].text),
            diamonds: normalizeDeepFinesseHolding(lines[2].text),
            clubs: normalizeDeepFinesseHolding(lines[3].text)
        };
    }

    function parseHandsInDeepFinesseCell(inCell, dealer, boardLabel) {
        const hands = {
            north: deepFinesseSeatHoldings(
                inCell, dealer, 40, 75, -5, 40, boardLabel, "north"
            ),
            west: deepFinesseSeatHoldings(
                inCell, dealer, 0, 25, 40, 85, boardLabel, "west"
            ),
            east: deepFinesseSeatHoldings(
                inCell, dealer, 80, 115, 40, 85, boardLabel, "east"
            ),
            south: deepFinesseSeatHoldings(
                inCell, dealer, 40, 75, 85, 130, boardLabel, "south"
            )
        };
        for (const direction of DIRECTIONS) {
            let pips = "";
            for (const suit of SUITS) {
                pips += hands[direction][suit];
            }
            if (pips.length !== 13) {
                throw new Error(
                    "board " + boardLabel + ": " + direction
                    + " has " + pips.length + " cards after parse (expected 13)"
                );
            }
        }
        return hands;
    }

    /**
     * Parse every Deep Finesse board diagram into an ordered list.
     * @returns {{boardNumber: string, hands: object}[]}
     */
    function parseAllDeepFinesseBoards(items) {
        const dealers = dealerMarkers(items);
        if (dealers.length === 0) {
            throw new Error("no Deep Finesse board diagrams found in PDF text");
        }

        const boards = [];
        const seen = {};
        for (const dealer of dealers) {
            const inCell = itemsInDeepFinesseCell(items, dealer);
            const boardNumber = deepFinesseBoardNumber(inCell, dealer);
            if (!boardNumber || seen[boardNumber]) {
                continue;
            }
            let hands;
            try {
                hands = parseHandsInDeepFinesseCell(inCell, dealer, boardNumber);
            } catch (_err) {
                continue;
            }
            seen[boardNumber] = true;
            boards.push({ boardNumber: boardNumber, hands: hands });
        }

        if (boards.length === 0) {
            throw new Error("no numbered boards found in hand-record PDF");
        }

        boards.sort(function (a, b) {
            return Number(a.boardNumber) - Number(b.boardNumber);
        });
        return boards;
    }

    /**
     * Madeira / MiBO-style sheets place a compact N/W/E/S compass under each
     * diagram. Suit rows fan out from the compass "N" (north above, south below,
     * west/east beside). Ghostscript PDFs typically use Tm under a scaled cm.
     * Pair and teams sheets share the layout at different absolute scales; seat
     * bands are normalized to the pair-scale south gap (~158pt).
     */
    const COMPASS_REF_SOUTH_DY = 158.4;

    function compassBoardAnchors(items) {
        const norths = items.filter(function (item) {
            return item.text === "N";
        });
        const anchors = [];
        for (const north of norths) {
            const west = items.find(function (item) {
                return item.text === "W"
                    && item.x < north.x
                    && north.x - item.x > 40
                    && north.x - item.x < 200
                    && item.y < north.y
                    && north.y - item.y > 50
                    && north.y - item.y < 200;
            });
            if (!west) {
                continue;
            }
            const east = items.find(function (item) {
                return item.text === "E"
                    && item.x > north.x
                    && item.x - north.x > 40
                    && item.x - north.x < 200
                    && item.y < north.y
                    && north.y - item.y > 50
                    && north.y - item.y < 200
                    && Math.abs(item.y - west.y) < 20;
            });
            if (!east) {
                continue;
            }
            const south = items.find(function (item) {
                const southDy = north.y - item.y;
                const westDy = north.y - west.y;
                return item.text === "S"
                    && Math.abs(item.x - north.x) < 40
                    && southDy > 100
                    && southDy < 350
                    && Math.abs(southDy - 2 * westDy) < 40;
            });
            if (west && east && south) {
                anchors.push({ north: north, south: south });
            }
        }
        return anchors;
    }

    function compassScale(anchor) {
        const southDy = anchor.north.y - anchor.south.y;
        if (!(southDy > 0)) {
            return 1;
        }
        return southDy / COMPASS_REF_SOUTH_DY;
    }

    function itemsNearCompass(items, north, scale) {
        const s = scale > 0 ? scale : 1;
        return items.filter(function (item) {
            return item.x >= north.x - 600 * s
                && item.x <= north.x + 700 * s
                && item.y >= north.y - 700 * s
                && item.y <= north.y + 400 * s;
        });
    }

    function compassSeatForSuit(northX, northY, suitX, suitY, scale) {
        const s = scale > 0 ? scale : 1;
        const dx = (suitX - northX) / s;
        const dy = (suitY - northY) / s;
        if (dx >= -250 && dx <= 50 && dy >= 80 && dy <= 380) {
            return "north";
        }
        if (dx >= -550 && dx <= -250 && dy >= -250 && dy <= 50) {
            return "west";
        }
        if (dx >= 80 && dx <= 280 && dy >= -250 && dy <= 50) {
            return "east";
        }
        if (dx >= -250 && dx <= 50 && dy >= -580 && dy <= -250) {
            return "south";
        }
        return null;
    }

    function boardNumberNearCompass(inCell, north, scale) {
        const s = scale > 0 ? scale : 1;
        const candidates = inCell.filter(function (item) {
            return /^\d{1,2}$/.test(item.text)
                && item.y > north.y + 150 * s
                && item.x > north.x + 200 * s;
        });
        candidates.sort(function (a, b) {
            return a.y - b.y || a.x - b.x;
        });
        return candidates.length > 0 ? candidates[0].text : null;
    }

    function parseHandsNearCompass(inCell, north, scale) {
        const hands = {
            north: emptySuitHoldings(),
            east: emptySuitHoldings(),
            south: emptySuitHoldings(),
            west: emptySuitHoldings(),
        };
        for (const item of inCell) {
            const suit = BRIDGE_SUIT_BY_CHAR[item.text];
            if (!suit) {
                continue;
            }
            const seat = compassSeatForSuit(
                north.x, north.y, item.x, item.y, scale
            );
            if (!seat) {
                continue;
            }
            hands[seat][suit] = bridgeHoldingBesideSuit(inCell, item);
        }
        for (const direction of DIRECTIONS) {
            let cards = 0;
            for (const suit of SUITS) {
                cards += hands[direction][suit].length;
            }
            if (cards !== 13) {
                throw new Error(
                    "incomplete " + direction + " hand near compass ("
                    + cards + " cards)"
                );
            }
        }
        return hands;
    }

    /**
     * Parse every compass-anchored board diagram into an ordered list.
     * @returns {{boardNumber: string, hands: object}[]}
     */
    function parseAllCompassHandRecordBoards(items) {
        const anchors = compassBoardAnchors(items);
        if (anchors.length === 0) {
            throw new Error("no compass board diagrams found in PDF text");
        }

        const boards = [];
        const seen = {};
        for (const anchor of anchors) {
            const north = anchor.north;
            const scale = compassScale(anchor);
            const inCell = itemsNearCompass(items, north, scale);
            const boardNumber = boardNumberNearCompass(inCell, north, scale);
            if (!boardNumber || seen[boardNumber]) {
                continue;
            }
            let hands;
            try {
                hands = parseHandsNearCompass(inCell, north, scale);
            } catch (_err) {
                continue;
            }
            seen[boardNumber] = true;
            boards.push({ boardNumber: boardNumber, hands: hands });
        }

        if (boards.length === 0) {
            throw new Error("no numbered boards found in hand-record PDF");
        }

        boards.sort(function (a, b) {
            return Number(a.boardNumber) - Number(b.boardNumber);
        });
        return boards;
    }

    function parseAllHandRecordBoards(items) {
        if (dealerMarkers(items).length > 0) {
            try {
                return parseAllBridgeComposerBoards(items);
            } catch (_err) {
                try {
                    return parseAllDeepFinesseBoards(items);
                } catch (_err2) {
                    // Fall through to compass layouts (Madeira / MiBO).
                }
            }
        }
        return parseAllCompassHandRecordBoards(items);
    }

    function handsToDottedDeal(hands) {
        return DIRECTIONS.map(function (direction) {
            return SUITS.map(function (suit) {
                return hands[direction][suit] || "";
            }).join(".");
        });
    }

    function pdfDecodeLiteral(bytes) {
        // PDF literal string body → Latin-1 text, with \\ octal / escapes.
        let out = "";
        for (let i = 0; i < bytes.length; i++) {
            if (bytes[i] === 0x5c && i + 1 < bytes.length) { // backslash
                const next = bytes[i + 1];
                if (next >= 0x30 && next <= 0x37) {
                    let oct = String.fromCharCode(next);
                    i++;
                    for (let k = 0; k < 2 && i + 1 < bytes.length; k++) {
                        const b = bytes[i + 1];
                        if (b < 0x30 || b > 0x37) {
                            break;
                        }
                        oct += String.fromCharCode(b);
                        i++;
                    }
                    out += String.fromCharCode(parseInt(oct, 8) & 0xff);
                    continue;
                }
                if (next === 0x6e) { out += "\n"; i++; continue; }
                if (next === 0x72) { out += "\r"; i++; continue; }
                if (next === 0x74) { out += "\t"; i++; continue; }
                out += String.fromCharCode(next);
                i++;
                continue;
            }
            out += String.fromCharCode(bytes[i]);
        }
        return out;
    }

    function pdfBytesMatchAt(bytes, offset, mark) {
        if (offset < 0 || offset + mark.length > bytes.length) {
            return false;
        }
        for (let k = 0; k < mark.length; k++) {
            if (bytes[offset + k] !== mark[k]) {
                return false;
            }
        }
        return true;
    }

    /**
     * Read a direct /Length integer from the dictionary just before `stream`.
     * Indirect lengths (/Length 12 0 R) are not resolved here.
     */
    function pdfStreamLengthBefore(bytes, streamKeywordOffset) {
        const lookback = Math.min(streamKeywordOffset, 512);
        const dict = Array.from(
            bytes.subarray(streamKeywordOffset - lookback, streamKeywordOffset),
            function (b) {
                return String.fromCharCode(b);
            }
        ).join("");
        // Prefer the last /Length <int> in the dict (not /Length1 font sizes).
        const re = /\/Length\s+(\d+)\b/g;
        let match;
        let last = null;
        while ((match = re.exec(dict)) !== null) {
            last = match;
        }
        if (!last) {
            return null;
        }
        const length = parseInt(last[1], 10);
        return Number.isFinite(length) && length >= 0 ? length : null;
    }

    function pdfObjectNumberBefore(bytes, streamKeywordOffset) {
        const lookback = Math.min(streamKeywordOffset, 256);
        let text = "";
        for (
            let i = streamKeywordOffset - lookback;
            i < streamKeywordOffset;
            i++
        ) {
            text += String.fromCharCode(bytes[i]);
        }
        const re = /(\d+)\s+0\s+obj\b/g;
        let match;
        let last = null;
        while ((match = re.exec(text)) !== null) {
            last = parseInt(match[1], 10);
        }
        return Number.isFinite(last) ? last : null;
    }

    function pdfFindStreams(bytes) {
        const streams = [];
        const startMark = [0x73, 0x74, 0x72, 0x65, 0x61, 0x6d]; // stream
        const endMark = [0x65, 0x6e, 0x64, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d]; // endstream
        let i = 0;
        while (i < bytes.length) {
            let found = -1;
            for (let j = i; j + startMark.length <= bytes.length; j++) {
                if (!pdfBytesMatchAt(bytes, j, startMark)) {
                    continue;
                }
                // Skip the `stream` suffix inside `endstream`.
                if (j >= 3 && pdfBytesMatchAt(bytes, j - 3, [0x65, 0x6e, 0x64])) {
                    continue;
                }
                found = j;
                break;
            }
            if (found < 0) {
                break;
            }
            let dataStart = found + startMark.length;
            if (bytes[dataStart] === 0x0d && bytes[dataStart + 1] === 0x0a) {
                dataStart += 2;
            } else if (bytes[dataStart] === 0x0a || bytes[dataStart] === 0x0d) {
                dataStart += 1;
            }
            const declaredLength = pdfStreamLengthBefore(bytes, found);
            let dataEnd;
            let end;
            if (
                declaredLength != null
                && dataStart + declaredLength <= bytes.length
            ) {
                // Exact /Length bytes — do not trim; Adler-32 may end in 0x0a/0x0d/0x20.
                dataEnd = dataStart + declaredLength;
                end = dataEnd;
                // Advance past optional whitespace and endstream.
                while (
                    end < bytes.length
                    && (bytes[end] === 0x0a
                        || bytes[end] === 0x0d
                        || bytes[end] === 0x20)
                ) {
                    end++;
                }
                if (pdfBytesMatchAt(bytes, end, endMark)) {
                    end += endMark.length;
                } else {
                    // Length present but endstream not where expected — fall back.
                    end = -1;
                }
            } else {
                end = -1;
            }
            if (end < 0) {
                end = -1;
                for (let j = dataStart; j + endMark.length <= bytes.length; j++) {
                    if (pdfBytesMatchAt(bytes, j, endMark)) {
                        end = j;
                        break;
                    }
                }
                if (end < 0) {
                    break;
                }
                // No reliable /Length: trim producer whitespace before endstream,
                // but only bytes that are pure separators (browser inflate is strict).
                dataEnd = end;
                while (
                    dataEnd > dataStart
                    && (bytes[dataEnd - 1] === 0x0a
                        || bytes[dataEnd - 1] === 0x0d
                        || bytes[dataEnd - 1] === 0x20)
                ) {
                    dataEnd--;
                }
                i = end + endMark.length;
            } else {
                i = end;
            }
            streams.push({
                objNum: pdfObjectNumberBefore(bytes, found),
                data: bytes.subarray(dataStart, dataEnd)
            });
        }
        return streams;
    }

    function inflateZlibBytes(bytes) {
        const inject =
            (typeof globalThis !== "undefined" && globalThis.__ddsInflateZlib)
            || (typeof document !== "undefined" && document.__ddsInflateZlib);
        if (typeof inject === "function") {
            try {
                return Promise.resolve(inject(bytes));
            } catch (error) {
                return Promise.reject(error);
            }
        }
        if (typeof DecompressionStream === "function") {
            // Copy into a standalone buffer — some engines dislike subarray views.
            const copy = new Uint8Array(bytes.byteLength);
            copy.set(bytes);
            const stream = new Blob([copy]).stream().pipeThrough(
                new DecompressionStream("deflate")
            );
            return new Response(stream).arrayBuffer().then(function (ab) {
                return new Uint8Array(ab);
            });
        }
        return Promise.reject(new Error(
            "PDF inflate unavailable (need DecompressionStream or __ddsInflateZlib)"
        ));
    }

    function applyPdfMatrix(matrix, px, py) {
        return {
            x: matrix[0] * px + matrix[2] * py + matrix[4],
            y: matrix[1] * px + matrix[3] * py + matrix[5],
        };
    }

    // Two-byte Identity / CID codespace upper bound (and max bfrange span).
    const PDF_MAX_CHAR_CODE = 0xffff;

    function pdfHexToCodePoints(hexBody, codeByteWidth) {
        const hex = String(hexBody).replace(/\s+/g, "");
        const width = codeByteWidth === 1 ? 1 : 2;
        const step = width * 2;
        const codes = [];
        for (let i = 0; i + step <= hex.length; i += step) {
            const code = parseInt(hex.slice(i, i + step), 16);
            if (Number.isFinite(code)) {
                codes.push(code);
            }
        }
        return codes;
    }

    /**
     * Build a font-code → Unicode map from ToUnicode CMap stream text.
     * Supports beginbfchar and the incremental beginbfrange form.
     * @returns {{map: Object, codeByteWidth: number}}
     */
    function parsePdfToUnicodeCMap(cmapText) {
        const map = Object.create(null);
        let codeByteWidth = 2;
        const spaceRe = /begincodespacerange([\s\S]*?)endcodespacerange/;
        const spaceMatch = spaceRe.exec(cmapText);
        if (spaceMatch) {
            const first = /<([0-9A-Fa-f]+)>/.exec(spaceMatch[1]);
            if (first) {
                const nibbles = first[1].length;
                if (nibbles <= 2) {
                    codeByteWidth = 1;
                } else if (nibbles <= 4) {
                    codeByteWidth = 2;
                }
            }
        }
        const bfcharRe = /beginbfchar([\s\S]*?)endbfchar/g;
        let section;
        while ((section = bfcharRe.exec(cmapText)) !== null) {
            const pairRe = /<([0-9A-Fa-f]+)>\s*<([0-9A-Fa-f]+)>/g;
            let pair;
            while ((pair = pairRe.exec(section[1])) !== null) {
                const src = parseInt(pair[1], 16);
                const dst = parseInt(pair[2], 16);
                if (
                    Number.isSafeInteger(src)
                    && Number.isSafeInteger(dst)
                    && src >= 0
                    && src <= PDF_MAX_CHAR_CODE
                ) {
                    map[src] = dst;
                }
            }
        }
        const bfrangeRe = /beginbfrange([\s\S]*?)endbfrange/g;
        while ((section = bfrangeRe.exec(cmapText)) !== null) {
            const rangeRe =
                /<([0-9A-Fa-f]+)>\s*<([0-9A-Fa-f]+)>\s*<([0-9A-Fa-f]+)>/g;
            let range;
            while ((range = rangeRe.exec(section[1])) !== null) {
                const lo = parseInt(range[1], 16);
                const hi = parseInt(range[2], 16);
                const start = parseInt(range[3], 16);
                if (
                    !Number.isSafeInteger(lo)
                    || !Number.isSafeInteger(hi)
                    || !Number.isSafeInteger(start)
                    || lo < 0
                    || hi < lo
                    || hi > PDF_MAX_CHAR_CODE
                    || start < 0
                ) {
                    continue;
                }
                const span = hi - lo;
                if (
                    !Number.isSafeInteger(start + span)
                    || span > PDF_MAX_CHAR_CODE
                ) {
                    continue;
                }
                for (let code = lo; code <= hi; code++) {
                    map[code] = start + (code - lo);
                }
            }
        }
        return { map: map, codeByteWidth: codeByteWidth };
    }

    function pdfLatin1(bytes) {
        if (typeof TextDecoder === "function") {
            try {
                return new TextDecoder("latin1").decode(bytes);
            } catch (_err) {
                // Fall through when the label is unavailable.
            }
        }
        return Array.from(bytes, function (b) {
            return String.fromCharCode(b);
        }).join("");
    }

    function pdfFontResourceObjectNums(pdfText) {
        const out = Object.create(null);
        const resRe = /\/Font\s*<<([\s\S]*?)>>/g;
        let match;
        while ((match = resRe.exec(pdfText)) !== null) {
            const entryRe = /\/([A-Za-z][\w+\-]*)\s+(\d+)\s+0\s+R/g;
            let entry;
            while ((entry = entryRe.exec(match[1])) !== null) {
                out[entry[1]] = parseInt(entry[2], 10);
            }
        }
        return out;
    }

    function pdfFontObjectToUnicodeObjectNums(pdfText) {
        const out = Object.create(null);
        const dicts = pdfExtractObjectDicts(pdfText);
        for (let i = 0; i < dicts.length; i++) {
            const body = dicts[i].body;
            if (!/\/Type\s*\/Font\b/.test(body)) {
                continue;
            }
            // Skip CID descendants; Type0 parents own /ToUnicode.
            if (/\/Subtype\s*\/CIDFont/.test(body)) {
                continue;
            }
            const tu = /\/ToUnicode\s+(\d+)\s+0\s+R/.exec(body);
            if (!tu) {
                continue;
            }
            out[dicts[i].objNum] = parseInt(tu[1], 10);
        }
        return out;
    }

    function pdfResolveFontNamesToCmaps(fontNameToObj, fontToCmap, byCmapObj) {
        const byFont = Object.create(null);
        for (const name of Object.keys(fontNameToObj)) {
            const fontObj = fontNameToObj[name];
            const cmapObj = fontToCmap[fontObj];
            if (cmapObj != null && byCmapObj[cmapObj]) {
                byFont[name] = byCmapObj[cmapObj];
            }
        }
        return byFont;
    }

    function pdfFontBlockObjectNums(fontDictBody) {
        const out = Object.create(null);
        const entryRe = /\/([A-Za-z][\w+\-]*)\s+(\d+)\s+0\s+R/g;
        let entry;
        while ((entry = entryRe.exec(fontDictBody)) !== null) {
            out[entry[1]] = parseInt(entry[2], 10);
        }
        return out;
    }

    function pdfResourceObjectFontMaps(pdfText) {
        // Associate each `/Font <<...>>` with the nearest preceding object id
        // (page Resources, Pages inheritance, or a shared resource object).
        const out = Object.create(null); // objNum -> { fontName: fontObjNum }
        const fontRe = /\/Font\s*<<([^>]*)>>/g;
        let match;
        while ((match = fontRe.exec(pdfText)) !== null) {
            const before = pdfText.slice(Math.max(0, match.index - 400), match.index);
            const objs = before.match(/(\d+)\s+0\s+obj/g);
            if (!objs || objs.length === 0) {
                continue;
            }
            const last = objs[objs.length - 1].match(/(\d+)/);
            if (!last) {
                continue;
            }
            out[parseInt(last[1], 10)] = pdfFontBlockObjectNums(match[1]);
        }
        return out;
    }

    /**
     * Walk top-level `N 0 obj << ... >>` dictionaries with balanced nesting.
     * @returns {{objNum: number, body: string}[]}
     */
    function pdfExtractObjectDicts(pdfText) {
        const results = [];
        const startRe = /(\d+)\s+0\s+obj\s*<</g;
        let match;
        while ((match = startRe.exec(pdfText)) !== null) {
            const objNum = parseInt(match[1], 10);
            const dictOpen = match.index + match[0].length - 2;
            let depth = 0;
            let i = dictOpen;
            let closed = -1;
            for (; i < pdfText.length - 1; i++) {
                if (pdfText.charAt(i) === "<" && pdfText.charAt(i + 1) === "<") {
                    depth++;
                    i++;
                    continue;
                }
                if (pdfText.charAt(i) === ">" && pdfText.charAt(i + 1) === ">") {
                    depth--;
                    i++;
                    if (depth === 0) {
                        closed = i - 1;
                        break;
                    }
                }
            }
            if (closed < 0) {
                continue;
            }
            results.push({
                objNum: objNum,
                body: pdfText.slice(dictOpen + 2, closed)
            });
            startRe.lastIndex = closed + 2;
        }
        return results;
    }

    function pdfContentObjectNumsFromPageBody(body) {
        const contentNums = [];
        const single = /\/Contents\s+(\d+)\s+0\s+R/.exec(body);
        if (single) {
            contentNums.push(parseInt(single[1], 10));
        }
        const arr = /\/Contents\s*\[([^\]]*)\]/.exec(body);
        if (arr) {
            const refRe = /(\d+)\s+0\s+R/g;
            let ref;
            while ((ref = refRe.exec(arr[1])) !== null) {
                contentNums.push(parseInt(ref[1], 10));
            }
        }
        return contentNums;
    }

    function pdfFontNameToObjFromPageBody(body, resourceFonts) {
        let fontNameToObj = Object.create(null);
        const inlineFont = /\/Font\s*<<([^>]*)>>/.exec(body);
        if (inlineFont) {
            fontNameToObj = pdfFontBlockObjectNums(inlineFont[1]);
        }
        if (Object.keys(fontNameToObj).length === 0) {
            const resRef = /\/Resources\s+(\d+)\s+0\s+R/.exec(body);
            if (resRef) {
                fontNameToObj = resourceFonts[parseInt(resRef[1], 10)]
                    || fontNameToObj;
            }
        }
        if (Object.keys(fontNameToObj).length === 0) {
            const parentRef = /\/Parent\s+(\d+)\s+0\s+R/.exec(body);
            if (parentRef) {
                fontNameToObj = resourceFonts[parseInt(parentRef[1], 10)]
                    || fontNameToObj;
            }
        }
        return fontNameToObj;
    }

    /**
     * Map each content-stream object number to font-name → ToUnicode CMap.
     * Page /Resources (and inherited Pages resources) scope /F1 per page so
     * reused names on later pages do not overwrite earlier boards.
     * @returns {Object<number, Object<string, {map: Object, codeByteWidth: number}>>}
     */
    function pdfCollectContentFontToUnicodeMaps(pdfBytes, decodedStreams) {
        const byCmapObj = Object.create(null);
        for (let i = 0; i < decodedStreams.length; i++) {
            const entry = decodedStreams[i];
            if (!entry || !entry.data || entry.objNum == null) {
                continue;
            }
            const text = pdfLatin1(entry.data);
            if (text.indexOf("begincmap") < 0) {
                continue;
            }
            const parsed = parsePdfToUnicodeCMap(text);
            if (Object.keys(parsed.map).length > 0) {
                byCmapObj[entry.objNum] = parsed;
            }
        }
        const pdfText = pdfLatin1(pdfBytes);
        const fontToCmap = pdfFontObjectToUnicodeObjectNums(pdfText);
        const resourceFonts = pdfResourceObjectFontMaps(pdfText);
        const contentMaps = Object.create(null);
        const dicts = pdfExtractObjectDicts(pdfText);

        for (let d = 0; d < dicts.length; d++) {
            const body = dicts[d].body;
            // Exact page nodes only — not the /Pages tree.
            if (!/\/Type\s*\/Page(?!s)\b/.test(body)) {
                continue;
            }
            const byFont = pdfResolveFontNamesToCmaps(
                pdfFontNameToObjFromPageBody(body, resourceFonts),
                fontToCmap,
                byCmapObj
            );
            const contentNums = pdfContentObjectNumsFromPageBody(body);
            for (let c = 0; c < contentNums.length; c++) {
                contentMaps[contentNums[c]] = byFont;
            }
        }

        // Single-page / missing-page fallback: document-wide font resources.
        if (Object.keys(contentMaps).length === 0) {
            const fontRes = pdfFontResourceObjectNums(pdfText);
            const byFont = pdfResolveFontNamesToCmaps(
                fontRes,
                fontToCmap,
                byCmapObj
            );
            contentMaps[0] = byFont;
        }
        return contentMaps;
    }

    function pdfMapCodesToText(codes, toUnicode) {
        const table = toUnicode && toUnicode.map ? toUnicode.map : toUnicode;
        let text = "";
        for (let i = 0; i < codes.length; i++) {
            const code = codes[i];
            const mapped = table && table[code] != null ? table[code] : code;
            text += String.fromCharCode(mapped);
        }
        return text;
    }

    function pdfActiveToUnicode(byFont, fontName) {
        if (!byFont) {
            return { map: Object.create(null), codeByteWidth: 2 };
        }
        // Legacy single-map shape used by unit tests.
        if (byFont.map) {
            return byFont;
        }
        if (fontName && byFont[fontName]) {
            return byFont[fontName];
        }
        const names = Object.keys(byFont);
        if (names.length === 1) {
            return byFont[names[0]];
        }
        return { map: Object.create(null), codeByteWidth: 2 };
    }

    function extractPdfTextItemsFromContent(decBytes, toUnicodeByFont) {
        const items = [];
        let x = 0;
        let y = 0;
        // Identity CTM; Ghostscript hand-records wrap BT/ET in `10 0 0 10 0 0 cm`.
        let ctm = [1, 0, 0, 1, 0, 0];
        let activeFont = null;
        let activeToUnicode = pdfActiveToUnicode(toUnicodeByFont, null);
        // Walk cm / Tm / Td / Tf / Tj / TJ. Latin-1 decode is fine for ASCII ops.
        const content = pdfLatin1(decBytes);
        const num = "-?\\d+(?:\\.\\d+)?(?:e[+-]?\\d+)?";
        const tokenRe = new RegExp(
            "(" + num + "(?:\\s+" + num + "){5}\\s+(?:cm|Tm))"
            + "|(" + num + "\\s+" + num + "\\s+Td)"
            + "|(/[A-Za-z][\\w+\\-]*\\s+" + num + "\\s+Tf)"
            + "|(\\((?:\\\\.|[^\\\\)])*\\)\\s*Tj)"
            + "|(<[0-9A-Fa-f\\s]+>\\s*Tj)"
            + "|(\\[(?:[^\\]]*)\\]\\s*TJ)",
            "g"
        );
        let match;
        while ((match = tokenRe.exec(content)) !== null) {
            const token = match[0];
            if (token.endsWith("cm")) {
                const parts = token.trim().split(/\s+/);
                ctm = parts.slice(0, 6).map(parseFloat);
                continue;
            }
            if (token.endsWith("Tm")) {
                const parts = token.trim().split(/\s+/);
                const nums = parts.slice(0, 6).map(parseFloat);
                const mapped = applyPdfMatrix(ctm, nums[4], nums[5]);
                x = mapped.x;
                y = mapped.y;
                continue;
            }
            if (token.endsWith("Td")) {
                // BridgeComposer sheets emit absolute-looking Td operands and do
                // not require CTM; applying cm here mis-scales those layouts.
                const parts = token.trim().split(/\s+/);
                x = parseFloat(parts[0]);
                y = parseFloat(parts[1]);
                continue;
            }
            if (token.endsWith("Tf")) {
                const parts = token.trim().split(/\s+/);
                activeFont = parts[0].slice(1);
                activeToUnicode = pdfActiveToUnicode(
                    toUnicodeByFont,
                    activeFont
                );
                continue;
            }
            if (token.endsWith("Tj")) {
                // Hex strings are their own token (`<...>` Tj). Do not treat a
                // literal that merely contains "<" as hex.
                if (/^\s*</.test(token)) {
                    const hexStart = token.indexOf("<");
                    const hexEnd = token.indexOf(">", hexStart);
                    const hexBody = token.slice(hexStart + 1, hexEnd);
                    items.push({
                        x: x,
                        y: y,
                        text: pdfMapCodesToText(
                            pdfHexToCodePoints(
                                hexBody,
                                activeToUnicode.codeByteWidth
                            ),
                            activeToUnicode
                        )
                    });
                    continue;
                }
                const litStart = token.indexOf("(") + 1;
                const litEnd = token.lastIndexOf(")");
                const lit = token.slice(litStart, litEnd);
                const litBytes = Uint8Array.from(lit, function (ch) {
                    return ch.charCodeAt(0) & 0xff;
                });
                items.push({ x: x, y: y, text: pdfDecodeLiteral(litBytes) });
                continue;
            }
            if (token.endsWith("TJ")) {
                const arr = token.slice(token.indexOf("[") + 1, token.lastIndexOf("]"));
                let text = "";
                const pieceRe = /\((?:\\.|[^\\)])*\)|<[0-9A-Fa-f\s]+>/g;
                let piece;
                while ((piece = pieceRe.exec(arr)) !== null) {
                    const raw = piece[0];
                    if (raw.charAt(0) === "<") {
                        text += pdfMapCodesToText(
                            pdfHexToCodePoints(
                                raw.slice(1, -1),
                                activeToUnicode.codeByteWidth
                            ),
                            activeToUnicode
                        );
                        continue;
                    }
                    const body = raw.slice(1, -1);
                    const litBytes = Uint8Array.from(body, function (ch) {
                        return ch.charCodeAt(0) & 0xff;
                    });
                    text += pdfDecodeLiteral(litBytes);
                }
                items.push({ x: x, y: y, text: text });
            }
        }
        return items;
    }

    function extractPdfTextItems(pdfBytes) {
        const bytes = pdfBytes instanceof Uint8Array ? pdfBytes : new Uint8Array(pdfBytes);
        const streams = pdfFindStreams(bytes);
        const inflated = streams.map(function (stream) {
            return inflateZlibBytes(stream.data).then(
                function (dec) {
                    return { objNum: stream.objNum, data: dec };
                },
                function () {
                    return null;
                }
            );
        });
        return Promise.all(inflated).then(function (decodedList) {
            const contentFontMaps = pdfCollectContentFontToUnicodeMaps(
                bytes,
                decodedList
            );
            const items = [];
            // BridgeComposer / ACBL multi-page sheets reuse the same absolute Td
            // space on every page. Concatenating streams without a per-page
            // offset collapses board cells (e.g. 1 with 19) and drops most boards.
            let pageYOffset = 0;
            for (const entry of decodedList) {
                if (!entry || !entry.data) {
                    continue;
                }
                const decoded = entry.data;
                // Only content streams with text operators.
                let hasText = false;
                for (let i = 0; i + 1 < decoded.length; i++) {
                    if (
                        decoded[i] === 0x54
                        && (decoded[i + 1] === 0x6a || decoded[i + 1] === 0x4a)
                    ) {
                        // Tj or TJ
                        hasText = true;
                        break;
                    }
                }
                if (!hasText) {
                    continue;
                }
                const toUnicodeByFont = (
                    entry.objNum != null && contentFontMaps[entry.objNum]
                )
                    ? contentFontMaps[entry.objNum]
                    : (contentFontMaps[0] || Object.create(null));
                const pageItems = extractPdfTextItemsFromContent(
                    decoded,
                    toUnicodeByFont
                );
                let pageMaxY = 0;
                for (let i = 0; i < pageItems.length; i++) {
                    const item = pageItems[i];
                    if (item.y > pageMaxY) {
                        pageMaxY = item.y;
                    }
                    items.push({
                        x: item.x,
                        y: item.y + pageYOffset,
                        text: item.text
                    });
                }
                pageYOffset += pageMaxY + CELL_H;
            }
            if (items.length === 0) {
                throw new Error("no extractable text streams in PDF");
            }
            return items;
        });
    }

    function loadAllBoardsFromHandRecordPdf(pdfBytes) {
        return extractPdfTextItems(pdfBytes).then(function (items) {
            try {
                return parseAllHandRecordBoards(items);
            } catch (error) {
                const message = error && error.message
                    ? String(error.message)
                    : "";
                if (
                    /no (compass|BridgeComposer|Deep Finesse|numbered) board/i
                        .test(message)
                ) {
                    throw new Error(
                        "no hand-record board diagrams found in PDF"
                    );
                }
                throw error;
            }
        });
    }

    global.normalizeBridgeHolding = normalizeBridgeHolding;
    global.parseBridgeComposerBoard = parseBridgeComposerBoard;
    global.parseAllBridgeComposerBoards = parseAllBridgeComposerBoards;
    global.parseAllDeepFinesseBoards = parseAllDeepFinesseBoards;
    global.parseAllCompassHandRecordBoards = parseAllCompassHandRecordBoards;
    global.parsePdfToUnicodeCMap = parsePdfToUnicodeCMap;
    global.pdfHexToCodePoints = pdfHexToCodePoints;
    global.pdfLatin1 = pdfLatin1;
    global.pdfCollectContentFontToUnicodeMaps = pdfCollectContentFontToUnicodeMaps;
    global.extractPdfTextItemsFromContent = extractPdfTextItemsFromContent;
    global.extractPdfTextItems = extractPdfTextItems;
    global.loadAllBoardsFromHandRecordPdf = loadAllBoardsFromHandRecordPdf;
    global.handsToDottedDeal = handsToDottedDeal;
})(typeof globalThis !== "undefined" ? globalThis : this);
