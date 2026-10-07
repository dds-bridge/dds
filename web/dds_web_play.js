// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Card-by-card play state for DDS Web. Loaded after dds_web_solve.js and
// before dds_web.js (UI).

/* eslint-env es6 */
/* exported nextDirection prevDirection winningPlay formatPlayDiff
            playDiffFromSolverScore createPlayState replayPlayState
            solverPositionFromPlay appendPlay undoLastChoice undoCurrentTrick
            playDiffMapFromSolverOutput isLegalPlayCard ddsRankFromPip
            remainingCardsForSeat playState startPlay exitPlay tryPlayCard
            undoPlay undoTrickPlay renderPlayUi isPlayMode
            playBadgeMapFromPending targetTricksFromCell */

"use strict";

(function (global) {
    const DIRECTIONS = global.DIRECTIONS;
    const SUITS = global.SUITS;
    const PIPS = global.PIPS;
    const Card = global.Card;
    const handsToPbn = global.handsToPbn;
    const openingLeader = global.openingLeader;
    const pipFromDdsRank = global.pipFromDdsRank;
    const DIR_TO_HAND = global.DIR_TO_HAND;
    const leadTricksMapFromSolverOutput = global.leadTricksMapFromSolverOutput;

    const SUIT_LETTER_TO_INDEX = { S: 0, H: 1, D: 2, C: 3 };

    let playState = null;

    function nextDirection(direction) {
        const index = DIRECTIONS.indexOf(direction);
        if (index < 0) {
            return null;
        }
        return DIRECTIONS[(index + 1) % 4];
    }

    function prevDirection(direction) {
        const index = DIRECTIONS.indexOf(direction);
        if (index < 0) {
            return null;
        }
        return DIRECTIONS[(index + 3) % 4];
    }

    function ddsRankFromPip(pip) {
        if (pip === "A") {
            return 14;
        }
        if (pip === "K") {
            return 13;
        }
        if (pip === "Q") {
            return 12;
        }
        if (pip === "J") {
            return 11;
        }
        if (pip === "T") {
            return 10;
        }
        const n = Number(pip);
        if (n >= 2 && n <= 9) {
            return n;
        }
        return null;
    }

    function pipRankValue(pip) {
        return PIPS.indexOf(pip);
    }

    function winningPlay(trick, trumpLetter) {
        if (!trick || trick.length === 0) {
            return null;
        }
        let best = trick[0];
        for (let i = 1; i < trick.length; i++) {
            const card = trick[i];
            const suit = card.key.charAt(0);
            const pip = card.key.charAt(1);
            const bestSuit = best.key.charAt(0);
            const bestPip = best.key.charAt(1);
            let beats = false;
            if (suit === bestSuit) {
                // PIPS is high-to-low, so a lower index is a higher card.
                beats = pipRankValue(pip) < pipRankValue(bestPip);
            } else if (trumpLetter && suit === trumpLetter) {
                beats = bestSuit !== trumpLetter;
            }
            if (beats) {
                best = card;
            }
        }
        return best;
    }

    function formatPlayDiff(diff) {
        const n = Number(diff);
        if (n === 0) {
            return "=";
        }
        if (n > 0) {
            return "+" + n;
        }
        return "\u2013" + (-n);
    }

    function isNs(direction) {
        return direction === "north" || direction === "south";
    }

    function playDiffFromSolverScore({
        declarer,
        seatToPlay,
        nsTricks,
        ewTricks,
        remainingTricks,
        sideToPlayScore,
        targetTricks,
    }) {
        const sideIsNs = isNs(seatToPlay);
        const nsRemaining = sideIsNs
            ? sideToPlayScore
            : remainingTricks - sideToPlayScore;
        const ewRemaining = sideIsNs
            ? remainingTricks - sideToPlayScore
            : sideToPlayScore;
        const projected = isNs(declarer)
            ? nsTricks + nsRemaining
            : ewTricks + ewRemaining;
        return projected - targetTricks;
    }

    function cloneHands(hands) {
        const out = {};
        for (const direction of DIRECTIONS) {
            out[direction] = (hands[direction] || []).map(
                (card) => new Card(card.suit, card.pip)
            );
        }
        return out;
    }

    function createPlayState({ hands, declarer, denomination, targetTricks }) {
        const trumpLetter = denomination === "N" ? null : denomination;
        return {
            hands: cloneHands(hands),
            declarer,
            denomination,
            trumpLetter,
            targetTricks: Number(targetTricks),
            leadSeat: openingLeader(declarer),
            history: [],
            pendingDiffs: null,
            autoPlay: true,
            requestId: 0,
        };
    }

    function replayPlayState(state) {
        let seat = state.leadSeat;
        let nsTricks = 0;
        let ewTricks = 0;
        let trick = [];
        let lastTrick = [];

        for (const play of state.history) {
            trick.push(play);
            if (trick.length === 4) {
                const winner = winningPlay(trick, state.trumpLetter);
                if (isNs(winner.seat)) {
                    nsTricks += 1;
                } else {
                    ewTricks += 1;
                }
                seat = winner.seat;
                lastTrick = trick;
                trick = [];
            } else {
                seat = nextDirection(seat);
            }
        }

        return {
            seat,
            nsTricks,
            ewTricks,
            trick: trick.length ? trick : [],
            lastTrick,
        };
    }

    function remainingCardsForSeat(state, seat) {
        const played = new Set(
            state.history.filter((p) => p.seat === seat).map((p) => p.key)
        );
        return state.hands[seat].filter((card) => !played.has(card.key()));
    }

    function remainingHands(state) {
        const out = {};
        for (const direction of DIRECTIONS) {
            out[direction] = remainingCardsForSeat(state, direction);
        }
        return out;
    }

    function solverPositionFromPlay(state) {
        const replay = replayPlayState(state);
        const currentTrick = replay.trick;
        const inTrick = new Set(currentTrick.map((p) => p.key));
        const remaining = {};

        for (const direction of DIRECTIONS) {
            remaining[direction] = remainingCardsForSeat(state, direction)
                .filter((card) => !inTrick.has(card.key()));
        }

        const trickSuits = [0, 0, 0];
        const trickRanks = [0, 0, 0];
        for (let i = 0; i < currentTrick.length && i < 3; i++) {
            const key = currentTrick[i].key;
            trickSuits[i] = SUIT_LETTER_TO_INDEX[key.charAt(0)];
            trickRanks[i] = ddsRankFromPip(key.charAt(1));
        }

        const firstSeat = currentTrick.length
            ? currentTrick[0].seat
            : replay.seat;

        return {
            remainingHands: remaining,
            pbn: handsToPbn(remaining),
            first: DIR_TO_HAND[firstSeat],
            trickSuits,
            trickRanks,
            seatToPlay: replay.seat,
            nsTricks: replay.nsTricks,
            ewTricks: replay.ewTricks,
            remainingTricks: 13 - replay.nsTricks - replay.ewTricks,
        };
    }

    function appendPlay(state, seat, key, auto) {
        state.history.push({
            seat,
            key: String(key).toUpperCase(),
            auto: !!auto,
        });
        state.pendingDiffs = null;
    }

    function undoLastChoice(state) {
        while (
            state.history.length > 0 &&
            state.history[state.history.length - 1].auto
        ) {
            state.history.pop();
        }
        if (state.history.length > 0) {
            state.history.pop();
        }
        state.pendingDiffs = null;
        state.autoPlay = false;
    }

    function undoCurrentTrick(state) {
        if (state.history.length === 0) {
            return;
        }
        const trickStart = Math.floor((state.history.length - 1) / 4) * 4;
        state.history.length = trickStart;
        state.pendingDiffs = null;
        state.autoPlay = false;
    }

    function playDiffMapFromSolverOutput(out, context) {
        const scores = leadTricksMapFromSolverOutput(out);
        const map = {};
        for (const key of Object.keys(scores)) {
            map[key] = playDiffFromSolverScore({
                declarer: context.declarer,
                seatToPlay: context.seatToPlay,
                nsTricks: context.nsTricks,
                ewTricks: context.ewTricks,
                remainingTricks: context.remainingTricks,
                sideToPlayScore: scores[key],
                targetTricks: context.targetTricks,
            });
        }
        return map;
    }

    function isLegalPlayCard(state, direction, key) {
        if (!state || !state.pendingDiffs) {
            return false;
        }
        const replay = replayPlayState(state);
        if (replay.seat !== direction) {
            return false;
        }
        return Object.prototype.hasOwnProperty.call(
            state.pendingDiffs,
            String(key).toUpperCase()
        );
    }

    function isPlayMode() {
        return playState != null;
    }

    function targetTricksFromCell(cell) {
        if (!cell) {
            return null;
        }
        const text = String(cell.textContent || cell.innerHTML || "").trim();
        if (!/^\d+$/.test(text)) {
            return null;
        }
        return Number(text);
    }

    function playBadgeMapFromPending(pendingDiffs) {
        if (!pendingDiffs) {
            return null;
        }
        const map = {};
        for (const key of Object.keys(pendingDiffs)) {
            map[key] = formatPlayDiff(pendingDiffs[key]);
        }
        return map;
    }

    function setPlayModeChrome(active) {
        const body = typeof document !== "undefined" ? document.body : null;
        if (body && body.classList) {
            if (active) {
                body.classList.add("playing");
            } else {
                body.classList.remove("playing");
            }
        }

        const ids = ["play-bar", "play-hint", "play-score", "trick-status"];
        for (const id of ids) {
            const el = document.getElementById(id);
            if (el) {
                el.hidden = !active;
            }
        }

        const deck = document.getElementById("deck-status");
        if (deck) {
            deck.hidden = !!active;
        }

        for (const direction of DIRECTIONS) {
            for (const suit of SUITS) {
                const input = document.getElementById(direction + "_" + suit);
                if (input) {
                    input.disabled = !!active;
                }
            }
        }
    }

    function capitalizeSeat(seat) {
        return seat.charAt(0).toUpperCase() + seat.slice(1);
    }

    function trickCardAriaLabel(seat, play) {
        if (!play) {
            return "";
        }
        const card = Card.fromKey(play.key);
        const suitName = card.suit.replace(/s$/, "");
        const pipNames = global.PIP_NAMES || {};
        const pipName = pipNames[card.pip] || card.pip;
        return capitalizeSeat(seat) + " " + suitName + " " + pipName;
    }

    function trickCardHtml(play) {
        if (!play) {
            return "";
        }
        const card = Card.fromKey(play.key);
        return global.suitSymbolHtml(card.suit) + escapePlayHtml(card.pip);
    }

    function trickSeatCellHtml(seatClass, seat, play) {
        // Omit empty seats entirely — a hidden empty .trick-card still paints
        // white bordered boxes in the center before the first card is played.
        if (!play) {
            return "";
        }
        return "<div class=\"" + seatClass + " trick-card\" aria-label=\"" +
            escapePlayHtml(trickCardAriaLabel(seat, play)) + "\">" +
            trickCardHtml(play) +
            "</div>";
    }

    function escapePlayHtml(value) {
        if (typeof global.escapeHtml === "function") {
            return global.escapeHtml(value);
        }
        return String(value)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;");
    }

    function renderTrickStatus(state) {
        const el = document.getElementById("trick-status");
        if (!el) {
            return;
        }
        if (typeof el.setAttribute === "function") {
            el.setAttribute("aria-live", "polite");
            el.setAttribute("role", "status");
            el.setAttribute("aria-label", "Current trick");
        }
        const replay = replayPlayState(state);
        const shown = replay.trick.length ? replay.trick : replay.lastTrick;
        const bySeat = { north: null, east: null, south: null, west: null };
        for (const play of shown) {
            bySeat[play.seat] = play;
        }
        el.innerHTML =
            trickSeatCellHtml("trick-n", "north", bySeat.north) +
            trickSeatCellHtml("trick-w", "west", bySeat.west) +
            trickSeatCellHtml("trick-e", "east", bySeat.east) +
            trickSeatCellHtml("trick-s", "south", bySeat.south);
    }

    function renderPlayScore(state) {
        const el = document.getElementById("play-score");
        if (!el) {
            return;
        }
        const replay = replayPlayState(state);
        const done = state.history.length >= 52;
        el.textContent =
            "NS " + replay.nsTricks + " – EW " + replay.ewTricks +
            (done ? " (final)" : "");
    }

    function updateUndoButtons(state) {
        const undoBtn = document.getElementById("undo-play");
        const undoTrickBtn = document.getElementById("undo-trick");
        if (undoBtn) {
            undoBtn.disabled = !state.history.some((p) => !p.auto);
        }
        if (undoTrickBtn) {
            undoTrickBtn.disabled = state.history.length === 0;
        }
    }

    function handsForDisplay(state) {
        const out = {};
        for (const direction of DIRECTIONS) {
            out[direction] = remainingCardsForSeat(state, direction);
        }
        return out;
    }

    function renderPlayUi() {
        if (!playState) {
            return;
        }
        renderTrickStatus(playState);
        renderPlayScore(playState);
        updateUndoButtons(playState);
        if (typeof global.updateHandCardDisplays === "function") {
            global.leadTricksByCardKey = playBadgeMapFromPending(
                playState.pendingDiffs
            );
            global.updateHandCardDisplays(handsForDisplay(playState));
        }
    }

    function startPlay(declarer, denomination, targetTricks) {
        if (
            typeof global.collectHands !== "function" ||
            typeof global.inputIsValid !== "function"
        ) {
            return false;
        }
        const hands = global.collectHands();
        if (global.inputIsValid(hands).length) {
            return false;
        }
        if (targetTricks == null || isNaN(targetTricks)) {
            return false;
        }

        playState = createPlayState({
            hands,
            declarer,
            denomination,
            targetTricks,
        });
        setPlayModeChrome(true);
        renderPlayUi();
        // Do not schedule here: applyResultCellSelection already schedules, and
        // the in-flight deal-solve worker calls refreshPlayTricks right after
        // ensurePlayForSelectedContract. Nesting scheduleDealSolve would solve
        // the same opening position a second time.
        return true;
    }

    function exitPlay() {
        // Even before play starts, a pending DD-cell click must be cleared so
        // load/clear/rotate (which call exitPlay) cannot later enter play on a
        // different deal via ensurePlayForSelectedContract.
        if (!playState) {
            if (global.selectedContractState &&
                    typeof global.clearResultCellSelection === "function") {
                global.clearResultCellSelection();
            }
            return;
        }
        playState = null;
        global.leadTricksByCardKey = null;
        setPlayModeChrome(false);
        const trick = document.getElementById("trick-status");
        if (trick) {
            trick.innerHTML = "";
        }
        const score = document.getElementById("play-score");
        if (score) {
            score.textContent = "";
        }
        if (typeof global.updateHandCardDisplays === "function" &&
                typeof global.collectHands === "function") {
            global.updateHandCardDisplays(global.collectHands());
        }
        // Clear the DD-table selection so a following scheduleDealSolve does
        // not call ensurePlayForSelectedContract and re-enter play (Edit hands).
        if (global.selectedContractState &&
                typeof global.clearResultCellSelection === "function") {
            global.clearResultCellSelection();
            return;
        }
        if (typeof global.scheduleDealSolve === "function") {
            void global.scheduleDealSolve();
        }
    }

    function afterPlayChange() {
        if (!playState) {
            return;
        }
        playState.pendingDiffs = null;
        renderPlayUi();
        // Bump before scheduling so an in-flight solve for the prior history
        // cannot repaint badges or auto-play onto the new position (undo race).
        if (typeof global.leadTricksRequestId === "number") {
            global.leadTricksRequestId += 1;
        }
        if (typeof global.scheduleDealSolve === "function") {
            void global.scheduleDealSolve();
        }
    }

    function tryPlayCard(direction, key, auto) {
        if (!playState) {
            return false;
        }
        const normalized = String(key).toUpperCase();
        if (!auto && !isLegalPlayCard(playState, direction, normalized)) {
            if (typeof global.playIllegalInputBeep === "function") {
                global.playIllegalInputBeep();
            }
            return false;
        }
        appendPlay(playState, direction, normalized, !!auto);
        playState.autoPlay = true;
        afterPlayChange();
        return true;
    }

    function undoPlay() {
        if (!playState) {
            return;
        }
        undoLastChoice(playState);
        afterPlayChange();
    }

    function undoTrickPlay() {
        if (!playState) {
            return;
        }
        undoCurrentTrick(playState);
        afterPlayChange();
    }

    function applyAutoPlayIfForced() {
        if (!playState || !playState.pendingDiffs || !playState.autoPlay) {
            return false;
        }
        const keys = Object.keys(playState.pendingDiffs);
        if (keys.length !== 1) {
            return false;
        }
        const replay = replayPlayState(playState);
        return tryPlayCard(replay.seat, keys[0], true);
    }

    global.nextDirection = nextDirection;
    global.prevDirection = prevDirection;
    global.winningPlay = winningPlay;
    global.formatPlayDiff = formatPlayDiff;
    global.playDiffFromSolverScore = playDiffFromSolverScore;
    global.createPlayState = createPlayState;
    global.replayPlayState = replayPlayState;
    global.solverPositionFromPlay = solverPositionFromPlay;
    global.appendPlay = appendPlay;
    global.undoLastChoice = undoLastChoice;
    global.undoCurrentTrick = undoCurrentTrick;
    global.playDiffMapFromSolverOutput = playDiffMapFromSolverOutput;
    global.isLegalPlayCard = isLegalPlayCard;
    global.ddsRankFromPip = ddsRankFromPip;
    global.remainingCardsForSeat = remainingCardsForSeat;
    global.startPlay = startPlay;
    global.exitPlay = exitPlay;
    global.tryPlayCard = tryPlayCard;
    global.undoPlay = undoPlay;
    global.undoTrickPlay = undoTrickPlay;
    global.renderPlayUi = renderPlayUi;
    global.isPlayMode = isPlayMode;
    global.playBadgeMapFromPending = playBadgeMapFromPending;
    global.targetTricksFromCell = targetTricksFromCell;
    global.applyAutoPlayIfForced = applyAutoPlayIfForced;
    global.handsForDisplay = handsForDisplay;
    Object.defineProperty(global, "playState", {
        get() {
            return playState;
        },
        set(value) {
            playState = value;
        },
        configurable: true,
    });
})(typeof globalThis !== "undefined" ? globalThis : window);
