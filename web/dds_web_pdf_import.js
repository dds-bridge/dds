// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Hand-record PDF parsers for DDS Web (BridgeComposer / ACBL and compass layouts).
// Loaded after dds_web_deal_import.js; exports extract/parse helpers on globalThis.

/* eslint-env es6 */
/* exported normalizeBridgeHolding parseBridgeComposerBoard
            parseAllBridgeComposerBoards parseAllCompassHandRecordBoards
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
     * Madeira / MiBO-style sheets place a compact N/W/E/S compass under each
     * diagram. Suit rows fan out from the compass "N" (north above, south below,
     * west/east beside). Ghostscript PDFs typically use Tm under a scaled cm.
     */
    function compassBoardAnchors(items) {
        const norths = items.filter(function (item) {
            return item.text === "N";
        });
        const anchors = [];
        for (const north of norths) {
            const west = items.find(function (item) {
                return item.text === "W"
                    && Math.abs(item.x - north.x) < 120
                    && item.y < north.y
                    && north.y - item.y < 120;
            });
            const east = items.find(function (item) {
                return item.text === "E"
                    && item.x > north.x
                    && Math.abs(item.x - north.x) < 120
                    && item.y < north.y
                    && north.y - item.y < 120;
            });
            const south = items.find(function (item) {
                return item.text === "S"
                    && Math.abs(item.x - north.x) < 40
                    && north.y - item.y > 100
                    && north.y - item.y < 220;
            });
            if (west && east && south) {
                anchors.push(north);
            }
        }
        return anchors;
    }

    function itemsNearCompass(items, north) {
        return items.filter(function (item) {
            return item.x >= north.x - 600
                && item.x <= north.x + 700
                && item.y >= north.y - 700
                && item.y <= north.y + 400;
        });
    }

    function compassSeatForSuit(northX, northY, suitX, suitY) {
        const dx = suitX - northX;
        const dy = suitY - northY;
        if (dx >= -200 && dx <= 50 && dy >= 80 && dy <= 380) {
            return "north";
        }
        if (dx >= -550 && dx <= -250 && dy >= -250 && dy <= 50) {
            return "west";
        }
        if (dx >= 80 && dx <= 280 && dy >= -250 && dy <= 50) {
            return "east";
        }
        if (dx >= -200 && dx <= 50 && dy >= -580 && dy <= -250) {
            return "south";
        }
        return null;
    }

    function boardNumberNearCompass(inCell, north) {
        const candidates = inCell.filter(function (item) {
            return /^\d{1,2}$/.test(item.text)
                && item.y > north.y + 150
                && item.x > north.x + 200;
        });
        candidates.sort(function (a, b) {
            return a.y - b.y || a.x - b.x;
        });
        return candidates.length > 0 ? candidates[0].text : null;
    }

    function parseHandsNearCompass(inCell, north) {
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
            const seat = compassSeatForSuit(north.x, north.y, item.x, item.y);
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
        for (const north of anchors) {
            const inCell = itemsNearCompass(items, north);
            const boardNumber = boardNumberNearCompass(inCell, north);
            if (!boardNumber || seen[boardNumber]) {
                continue;
            }
            let hands;
            try {
                hands = parseHandsNearCompass(inCell, north);
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
                // Fall through to compass layouts (Madeira / MiBO).
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
            streams.push(bytes.subarray(dataStart, dataEnd));
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

    function extractPdfTextItemsFromContent(decBytes) {
        const items = [];
        let x = 0;
        let y = 0;
        // Identity CTM; Ghostscript hand-records wrap BT/ET in `10 0 0 10 0 0 cm`.
        let ctm = [1, 0, 0, 1, 0, 0];
        // Walk cm / Tm / Td / Tj / TJ. Latin-1 decode of content is fine for ASCII ops.
        const content = Array.from(decBytes, function (b) {
            return String.fromCharCode(b);
        }).join("");
        const num = "-?\\d+(?:\\.\\d+)?(?:e[+-]?\\d+)?";
        const tokenRe = new RegExp(
            "(" + num + "(?:\\s+" + num + "){5}\\s+(?:cm|Tm))"
            + "|(" + num + "\\s+" + num + "\\s+Td)"
            + "|(\\((?:\\\\.|[^\\\\)])*\\)\\s*Tj)"
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
            if (token.endsWith("Tj")) {
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
                const litRe = /\((?:\\.|[^\\)])*\)/g;
                let litMatch;
                while ((litMatch = litRe.exec(arr)) !== null) {
                    const body = litMatch[0].slice(1, -1);
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
        const inflated = streams.map(function (raw) {
            return inflateZlibBytes(raw).then(
                function (dec) {
                    return dec;
                },
                function () {
                    return null;
                }
            );
        });
        return Promise.all(inflated).then(function (decodedList) {
            const items = [];
            // BridgeComposer / ACBL multi-page sheets reuse the same absolute Td
            // space on every page. Concatenating streams without a per-page
            // offset collapses board cells (e.g. 1 with 19) and drops most boards.
            let pageYOffset = 0;
            for (const decoded of decodedList) {
                if (!decoded) {
                    continue;
                }
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
                const pageItems = extractPdfTextItemsFromContent(decoded);
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
            return parseAllHandRecordBoards(items);
        });
    }

    global.normalizeBridgeHolding = normalizeBridgeHolding;
    global.parseBridgeComposerBoard = parseBridgeComposerBoard;
    global.parseAllBridgeComposerBoards = parseAllBridgeComposerBoards;
    global.parseAllCompassHandRecordBoards = parseAllCompassHandRecordBoards;
    global.extractPdfTextItems = extractPdfTextItems;
    global.loadAllBoardsFromHandRecordPdf = loadAllBoardsFromHandRecordPdf;
    global.handsToDottedDeal = handsToDottedDeal;
})(typeof globalThis !== "undefined" ? globalThis : this);
