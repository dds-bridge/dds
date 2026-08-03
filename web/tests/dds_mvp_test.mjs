/**
 * Unit tests for web/dds_mvp.js (Node built-in test runner).
 *
 * Run with:
 *    bazel test //web:dds_mvp_js_test
 * or: python -m unittest web.tests.test_dds_mvp_js
 * or: node --test web/tests/dds_mvp_test.mjs
 */
import assert from "node:assert/strict";
import { existsSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { createContext, runInContext } from "node:vm";

const DIRECTIONS = ["north", "east", "south", "west"];
const SUITS = ["spades", "hearts", "diamonds", "clubs"];

function findDdsMvpJsPath() {
    if (process.env.DDS_MVP_JS && existsSync(process.env.DDS_MVP_JS)) {
        return process.env.DDS_MVP_JS;
    }

    const here = dirname(fileURLToPath(import.meta.url));
    const adjacent = join(here, "..", "dds_mvp.js");
    if (existsSync(adjacent)) {
        return adjacent;
    }

    for (const base of [process.env.TEST_SRCDIR, process.env.RUNFILES_DIR]) {
        if (!base) {
            continue;
        }
        for (const sub of ["web/dds_mvp.js", "_main/web/dds_mvp.js"]) {
            const candidate = join(base, sub);
            if (existsSync(candidate)) {
                return candidate;
            }
        }
    }

    throw new Error("dds_mvp.js not found");
}

function createMockDocument(initialValues = {}) {
    const store = new Map();
    const listeners = new Map();

    const makeElement = (id) => {
        const elementListeners = new Map();
        const element = {
            id,
            value: initialValues[id] ?? "",
            innerHTML: "",
            disabled: false,
            hidden: false,
            className: "",
            selectionStart: 0,
            selectionEnd: 0,
            classList: {
                add(name) {
                    const classes = new Set(
                        element.className.split(/\s+/).filter(Boolean)
                    );
                    classes.add(name);
                    element.className = [...classes].join(" ");
                },
                remove(name) {
                    const classes = new Set(
                        element.className.split(/\s+/).filter(Boolean)
                    );
                    classes.delete(name);
                    element.className = [...classes].join(" ");
                },
                contains(name) {
                    return element.className.split(/\s+/).includes(name);
                },
            },
            focus() {
                documentRef.activeElement = element;
            },
            setSelectionRange(start, end = start) {
                this.selectionStart = start;
                this.selectionEnd = end;
            },
            addEventListener(type, listener) {
                const typeListeners = elementListeners.get(type) ?? [];
                typeListeners.push(listener);
                elementListeners.set(type, typeListeners);
            },
            dispatch(type, event) {
                for (const listener of elementListeners.get(type) ?? []) {
                    listener({ ...event, target: element });
                }
            },
        };
        store.set(id, element);
        return element;
    };

    for (const direction of DIRECTIONS) {
        for (const suit of SUITS) {
            makeElement(`${direction}_${suit}`);
            makeElement(`${direction}_${suit}_cards`);
        }
    }
    makeElement("valid-pips");
    makeElement("result");
    makeElement("deck-status");
    makeElement("contract-status");
    for (const direction of DIRECTIONS) {
        makeElement(`${direction}-card-count`);
    }

    const rows = [];
    for (let row = 0; row < 5; row++) {
        const cells = [];
        const rowObj = { cells, rowIndex: row };
        for (let column = 0; column < 6; column++) {
            const cell = {
                innerHTML: "",
                className: "",
                cellIndex: column,
                parentElement: rowObj,
                tagName: row === 0 || column === 0 ? "TH" : "TD",
                classList: {
                    add(name) {
                        const classes = new Set(
                            cell.className.split(/\s+/).filter(Boolean)
                        );
                        classes.add(name);
                        cell.className = [...classes].join(" ");
                    },
                    remove(name) {
                        const classes = new Set(
                            cell.className.split(/\s+/).filter(Boolean)
                        );
                        classes.delete(name);
                        cell.className = [...classes].join(" ");
                    },
                    contains(name) {
                        return cell.className.split(/\s+/).includes(name);
                    },
                },
            };
            cells.push(cell);
        }
        rows.push(rowObj);
    }
    store.set("result-table", {
        rows,
        querySelectorAll(selector) {
            if (selector !== "td") {
                return [];
            }
            const tds = [];
            for (let row = 1; row < rows.length; row++) {
                for (let column = 1; column < rows[row].cells.length; column++) {
                    tds.push(rows[row].cells[column]);
                }
            }
            return tds;
        },
    });

    const documentRef = {
        activeElement: null,
        addEventListener(type, listener) {
            const typeListeners = listeners.get(type) ?? [];
            typeListeners.push(listener);
            listeners.set(type, typeListeners);
        },
        dispatch(type, event) {
            for (const listener of listeners.get(type) ?? []) {
                listener(event);
            }
        },
        getElementById(id) {
            return store.get(id) ?? null;
        },
        element(id) {
            return store.get(id);
        },
        setActiveElement(id) {
            this.activeElement = store.get(id) ?? null;
        },
        setValue(id, value) {
            store.get(id).value = value;
        },
        values() {
            const out = {};
            for (const [id, element] of store) {
                if (id.includes("_")) {
                    out[id] = element.value;
                }
            }
            return out;
        },
    };
    return documentRef;
}

function loadDdsMvp(document) {
    const code = readFileSync(findDdsMvpJsPath(), "utf8");
    const sandbox = {
        document,
        console,
        Promise,
        Error,
        setTimeout,
    };
    const context = createContext(sandbox);
    runInContext(code, context, { filename: "dds_mvp.js" });
    return context;
}

function cardsFromKeys(ctx, keys) {
    return keys.map(ctx.Card.fromKey);
}

function handsFromKeys(ctx, hands) {
    return Object.fromEntries(
        Object.entries(hands).map(([direction, keys]) => [
            direction,
            cardsFromKeys(ctx, keys),
        ])
    );
}

function threeHandsPartScoreDocument() {
    return createMockDocument({
        north_spades: "AQ85",
        north_hearts: "AK976",
        north_diamonds: "5",
        north_clubs: "J87",
        east_spades: "JT",
        east_hearts: "QJ5432",
        east_diamonds: "Q9",
        east_clubs: "KQ9",
        south_spades: "972",
        south_hearts: "",
        south_diamonds: "JT863",
        south_clubs: "A6432",
        west_spades: "",
        west_hearts: "",
        west_diamonds: "",
        west_clubs: "",
    });
}

test("Card converts between named suits and compact keys", () => {
    // Arrange / Act
    const ctx = loadDdsMvp(createMockDocument());
    const card = new ctx.Card("hearts", "K");
    const decoded = ctx.Card.fromKey("D2");

    // Assert
    assert.equal(card.suit, "hearts");
    assert.equal(card.pip, "K");
    assert.equal(card.key(), "HK");
    assert.equal(card.toString(), "HK");
    assert.equal(decoded.suit, "diamonds");
    assert.equal(decoded.pip, "2");
});

test("Card.compare sorts cards from ace through deuce", () => {
    // Arrange
    const ctx = loadDdsMvp(createMockDocument());
    const cards = ["S2", "SK", "SA"].map(ctx.Card.fromKey);

    // Act
    cards.sort(ctx.Card.compare);

    // Assert
    assert.deepEqual(cards.map((card) => card.key()), ["SA", "SK", "S2"]);
});

test("cardsToSuitHoldings groups cards by suit name", () => {
    const ctx = loadDdsMvp(createMockDocument());
    // Copy out of the VM realm so deepEqual compares same-realm prototypes.
    const holdings = {
        ...ctx.cardsToSuitHoldings(cardsFromKeys(ctx, ["SA", "SK", "HA", "C2"])),
    };
    assert.deepEqual(holdings, {
        spades: "AK",
        hearts: "A",
        diamonds: "",
        clubs: "2",
    });
});

test("deck status lists suits in S H D C order", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.updateActionButtons();
    const deckStatus = document.element("deck-status").innerHTML;
    const suitIndexes = [
        "<spade-suit>",
        "<heart-suit>",
        "<diamond-suit>",
        "<club-suit>",
    ].map((tag) => deckStatus.indexOf(tag));
    assert.ok(suitIndexes.every((index) => index >= 0));
    assert.deepEqual(
        [...suitIndexes].sort((a, b) => a - b),
        suitIndexes
    );
});

test("handsToPbn formats part-score deal", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    const pbn = ctx.handsToPbn(ctx.collectHands());
    assert.equal(
        pbn,
        "N:AQ85.AK976.5.J87 JT.QJ5432.Q9.KQ9 972..JT863.A6432 K643.T8.AK742.T5"
    );
});

test("inputIsValid rejects incomplete deal", () => {
    const ctx = loadDdsMvp(createMockDocument());
    assert.equal(
        ctx.inputIsValid(handsFromKeys(ctx, {
            north: ["SA"],
            east: [],
            south: [],
            west: [],
        })),
        "Please enter 13 cards per hand."
    );
});

test("inputIsValid rejects invalid pip", () => {
    const ctx = loadDdsMvp(createMockDocument());
    const hands = handsFromKeys(ctx, {
        north: ["S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "ST", "SJ", "SQ", "SK"],
        east: ["HA", "H2", "H3", "H4", "H5", "H6", "H7", "H8", "H9", "HT", "HJ", "HQ", "HK"],
        south: ["DA", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DT", "DJ", "DQ", "DK"],
        west: ["CA", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CT", "CJ", "CQ", "CK"],
    });
    assert.match(ctx.inputIsValid(hands), /^Please use only these pips:/);
});

test("inputIsValid rejects duplicate cards", () => {
    const ctx = loadDdsMvp(createMockDocument());
    const hands = handsFromKeys(ctx, {
        north: ["SA", "SA", "HA", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "ST", "SJ", "SQ"],
        east: ["HA", "H2", "H3", "H4", "H5", "H6", "H7", "H8", "H9", "HT", "HJ", "HQ", "HK"],
        south: ["DA", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DT", "DJ", "DQ", "DK"],
        west: ["CA", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CT", "CJ", "CQ", "CK"],
    });
    const message = ctx.inputIsValid(hands);
    assert.match(message, /^Duplicated card/);
    // Suit glyphs are real DOM text (not CSS :before) for accessibility.
    assert.match(message, /<spade-suit>\u2660<\/spade-suit>A/);
    assert.match(message, /<heart-suit>\u2665<\/heart-suit>A/);
    assert.doesNotMatch(message, /style=['"]color: red['"]/);
    assert.doesNotMatch(message, /&spades;|&hearts;|&diams;|&clubs;/);
});

test("inputIsValid accepts part-score deal", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
});

test("collectHands reads suit holdings from inputs", () => {
    const document = createMockDocument({
        north_spades: "AKQ",
        north_hearts: "JT",
        north_diamonds: "987",
        north_clubs: "65432",
        east_spades: "",
        east_hearts: "AKQ",
        east_diamonds: "",
        east_clubs: "JT98765432",
        south_spades: "JT98765432",
        south_hearts: "",
        south_diamonds: "AKQ",
        south_clubs: "",
        west_spades: "",
        west_hearts: "98765432",
        west_diamonds: "JT",
        west_clubs: "AKQ",
    });
    const ctx = loadDdsMvp(document);
    const hands = ctx.collectHands();
    assert.equal(hands.north.length, 13);
    assert.equal(hands.east.length, 13);
    assert.equal(hands.south.length, 13);
    assert.equal(hands.west.length, 13);
    assert.ok(hands.north.some((card) => card.key() === "SA"));
    assert.ok(hands.north.some((card) => card.key() === "SK"));
    assert.ok(hands.east.some((card) => card.key() === "CJ"));
});

test("clearTestData clears all hand inputs", () => {
    const document = createMockDocument({ north_spades: "AKQ", west_clubs: "JT" });
    const ctx = loadDdsMvp(document);
    ctx.clearTestData();
    assert.equal(document.element("north_spades").value, "");
    assert.equal(document.element("west_clubs").value, "");
});

test("rotateClockwise shifts holdings west to north", () => {
    const document = createMockDocument();
    let index = 1;
    for (const direction of DIRECTIONS) {
        for (const suit of SUITS) {
            document.setValue(`${direction}_${suit}`, String(index));
            index += 1;
        }
    }
    const ctx = loadDdsMvp(document);
    ctx.rotateClockwise();
    assert.equal(document.element("north_spades").value, "13");
    assert.equal(document.element("north_hearts").value, "14");
    assert.equal(document.element("north_diamonds").value, "15");
    assert.equal(document.element("north_clubs").value, "16");
    assert.equal(document.element("east_spades").value, "1");
});

test("fillFormWithPartScoreTestData populates inputs", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    assert.equal(document.element("north_spades").value, "AQ85");
    assert.equal(document.element("west_clubs").value, "T5");
});

test("fillFormWithTestData does not require a double-dummy button", async () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };

    // Act
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert: loading a complete deal auto-refreshes the table (no button to focus).
    assert.equal(refreshed, 1);
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
});

test("page has no double-dummy button", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const html = readFileSync(join(here, "..", "dds_mvp.html"), "utf8");
    const css = readFileSync(join(here, "..", "dds_mvp.css"), "utf8");

    // Assert
    assert.doesNotMatch(html, /double-dummy-it/);
    assert.doesNotMatch(html, /Double-dummy it!/);
    assert.doesNotMatch(css, /#double-dummy-it/);
});

test("updateActionButtons finishes dd table before opening-lead refresh", async () => {
    // Arrange: a selected contract must not race CalcDDtable vs SolveBoard.
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    const order = [];

    ctx.refreshDdTable = async () => {
        order.push("dd-start");
        await new Promise((resolve) => setTimeout(resolve, 30));
        order.push("dd-end");
    };
    ctx.refreshOpeningLeadTricks = async () => {
        order.push("leads");
    };

    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 50));
    order.length = 0;

    ctx.handleResultTableClick({
        target: {
            closest() {
                return document.element("result-table").rows[3].cells[5];
            },
        },
    });
    await new Promise((resolve) => setTimeout(resolve, 80));

    // Assert: contract click runs DD (skip/no-op ok) then leads in one job.
    assert.deepEqual(order, ["dd-start", "dd-end", "leads"]);
});

test("contract selection waits for an in-flight dd table before lead refresh", async () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    const order = [];

    ctx.refreshDdTable = async () => {
        order.push("dd-start");
        await new Promise((resolve) => setTimeout(resolve, 40));
        order.push("dd-end");
    };
    ctx.refreshOpeningLeadTricks = async () => {
        order.push("leads");
    };

    // Act: start auto-solve, then select a contract before it finishes.
    ctx.fillFormWithPartScoreTestData();
    ctx.handleResultTableClick({
        target: {
            closest() {
                return document.element("result-table").rows[3].cells[5];
            },
        },
    });
    await new Promise((resolve) => setTimeout(resolve, 120));

    // Assert: coalesced jobs still run DD before leads; obsolete jobs no-op.
    assert.ok(order.indexOf("dd-start") >= 0);
    assert.ok(order.indexOf("dd-end") >= 0);
    assert.equal(order[order.length - 1], "leads");
    assert.ok(order.lastIndexOf("dd-end") < order.lastIndexOf("leads"));
});

test("rapid scheduleDealSolve coalesces to one trailing leads refresh", async () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    let leads = 0;
    let dd = 0;

    ctx.refreshDdTable = async () => {
        dd += 1;
        await new Promise((resolve) => setTimeout(resolve, 20));
    };
    ctx.refreshOpeningLeadTricks = async () => {
        leads += 1;
    };

    ctx.fillFormWithPartScoreTestData();
    ctx.handleResultTableClick({
        target: {
            closest() {
                return document.element("result-table").rows[3].cells[5];
            },
        },
    });
    ctx.scheduleDealSolve();
    ctx.scheduleDealSolve();
    await new Promise((resolve) => setTimeout(resolve, 120));

    assert.equal(leads, 1);
    assert.ok(dd >= 1);
});

test("pageLoad shows valid pips", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.pageLoad();
    assert.equal(document.element("valid-pips").innerHTML, "AKQJT98765432");
});

test("loadDdsModule rejects missing wasm globals", async () => {
    const ctx = loadDdsMvp(createMockDocument());
    await assert.rejects(
        () => ctx.loadDdsModule(),
        /WASM module not found/
    );
});

test("wasmSolveEnvironmentError explains file:// cannot load WASM workers", () => {
    // Arrange: browser opened as a local file (origin null).
    const code = readFileSync(findDdsMvpJsPath(), "utf8");
    const sandbox = {
        document: createMockDocument(),
        console,
        Promise,
        Error,
        setTimeout,
        location: { protocol: "file:" },
    };
    const context = createContext(sandbox);
    runInContext(code, context, { filename: "dds_mvp.js" });

    // Act / Assert
    assert.match(
        context.wasmSolveEnvironmentError(),
        /python3 web\/serve_mvp\.py/
    );
});

test("wasmSolveEnvironmentError is null in non-browser sandboxes", () => {
    const ctx = loadDdsMvp(createMockDocument());
    assert.equal(ctx.wasmSolveEnvironmentError(), null);
});

test("fillFormWithGrandSlamTestData populates inputs", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithGrandSlamTestData();
    assert.equal(document.element("north_spades").value, "AKQJ");
    assert.equal(document.element("east_clubs").value, "432");
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
});

test("fillFormWithEveryoneMakes3nTestData populates inputs", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithEveryoneMakes3nTestData();
    assert.equal(document.element("north_hearts").value, "A8765432");
    assert.equal(document.element("west_spades").value, "");
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
});

test("fourthHandFillState accepts three full hands and one empty", () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsMvp(document);
    const state = ctx.fourthHandFillState(ctx.collectHands());
    assert.equal(state.canFill, true);
    assert.equal(state.emptyHand, "west");
});

test("fourthHandFillState rejects partial fourth hand", () => {
    const document = threeHandsPartScoreDocument();
    document.setValue("west_spades", "K");
    const ctx = loadDdsMvp(document);
    assert.equal(ctx.fourthHandFillState(ctx.collectHands()).canFill, false);
});

test("fourthHandFillState rejects a duplicate card across three full hands", () => {
    // Arrange: keep all three hands at 13 cards but duplicate D9 (east & north).
    const document = threeHandsPartScoreDocument();
    document.setValue("north_diamonds", "9");
    const ctx = loadDdsMvp(document);

    // Act
    const state = ctx.fourthHandFillState(ctx.collectHands());

    // Assert: 39 slots yield only 38 distinct cards, so the deal is not fillable.
    assert.equal(state.canFill, false);
});

test("fourthHandFillState rejects a non-bridge pip among three full hands", () => {
    // Arrange: 13 cards per filled hand, but north clubs uses X instead of a real pip.
    const document = threeHandsPartScoreDocument();
    document.setValue("north_clubs", "J8X");
    const ctx = loadDdsMvp(document);

    // Act
    const state = ctx.fourthHandFillState(ctx.collectHands());

    // Assert: invalid pips must not be treated as used cards for auto-fill.
    assert.equal(state.canFill, false);
});

test("fourthHandFillState rejects a missing card object among three full hands", () => {
    // Arrange: three full hands and one empty, then corrupt one card entry.
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsMvp(document);
    const hands = ctx.collectHands();
    hands.north[0] = null;

    // Act / Assert
    assert.equal(ctx.fourthHandFillState(hands).canFill, false);
});

test("updateActionButtons does not auto-fill when a hand has a non-bridge pip", async () => {
    // Arrange: three full hands, but north holds an invalid pip (CX) instead of C7.
    const document = threeHandsPartScoreDocument();
    document.setValue("north_clubs", "J8X");
    const ctx = loadDdsMvp(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };

    // Act
    ctx.updateActionButtons();
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert: the fourth hand stays empty and the table is not solved.
    assert.equal(document.element("west_spades").value, "");
    assert.equal(document.element("west_hearts").value, "");
    assert.equal(document.element("west_diamonds").value, "");
    assert.equal(document.element("west_clubs").value, "");
    assert.equal(refreshed, 1);
});

test("updateActionButtons auto-fills the fourth hand for three complete hands", async () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsMvp(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };
    ctx.updateActionButtons();
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(document.element("west_spades").value, "K643");
    assert.equal(document.element("west_hearts").value, "T8");
    assert.equal(document.element("west_diamonds").value, "AK742");
    assert.equal(document.element("west_clubs").value, "T5");
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
    assert.equal(refreshed, 1);
});

test("updateActionButtons does not auto-fill with a partial fourth hand", async () => {
    const document = threeHandsPartScoreDocument();
    document.setValue("west_spades", "K");
    const ctx = loadDdsMvp(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };
    ctx.updateActionButtons();
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(document.element("west_spades").value, "K");
    assert.equal(document.element("west_hearts").value, "");
    assert.equal(refreshed, 1);
});

test("updateActionButtons auto-solves when every hand has 13 cards", async () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(refreshed, 1);
});

test("updateActionButtons refreshes the table state for incomplete deals", async () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsMvp(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };
    ctx.updateActionButtons();
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(refreshed, 1);
});

test("refreshDdTable clears the results table when the deal is incomplete", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsMvp(document);
    const cell = document.element("result-table").rows[1].cells[1];
    cell.innerHTML = "9";
    document.element("result").innerHTML = "stale";

    // Act
    ctx.refreshDdTable();

    // Assert
    assert.equal(cell.innerHTML, "");
    assert.equal(document.element("result").innerHTML, "");
});

test("updateActionButtons displays all 52 cards in the deck status", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);

    ctx.updateActionButtons();

    const deckStatus = document.element("deck-status").innerHTML;
    assert.equal((deckStatus.match(/data-card=/g) ?? []).length, 52);
    assert.match(deckStatus, /<spade-suit>\u2660/);
    assert.match(deckStatus, /<heart-suit>\u2665/);
    assert.match(deckStatus, /<diamond-suit>\u2666/);
    assert.match(deckStatus, /<club-suit>\u2663/);
    assert.doesNotMatch(deckStatus, /&spades;|&hearts;|&diams;|&clubs;/);
    assert.match(deckStatus, /data-card="SA"/);
    assert.match(deckStatus, /data-card="C2"/);
    assert.match(
        deckStatus,
        /<heart-suit>\u2665<span class="deck-card" data-card="HA">/
    );
    assert.doesNotMatch(
        deckStatus,
        /class="deck-card [^"]*red[^"]*" data-card="HA"/
    );
});

test("updateActionButtons grays cards entered in any hand, including lowercase pips", () => {
    const document = createMockDocument({
        north_spades: "A",
        east_hearts: "k",
    });
    const ctx = loadDdsMvp(document);

    ctx.updateActionButtons();

    const deckStatus = document.element("deck-status").innerHTML;
    assert.match(
        deckStatus,
        /class="deck-card deck-card-entered" data-card="SA"/
    );
    assert.match(
        deckStatus,
        /class="deck-card deck-card-entered" data-card="HK"/
    );
    assert.match(
        deckStatus,
        /class="deck-card" data-card="SK"/
    );
});

test("updateActionButtons shows a card-count note for a hand over 13 cards", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765432",
        north_hearts:  "A",
    });
    const ctx = loadDdsMvp(document);

    ctx.updateActionButtons();

    const note = document.element("north-card-count");
    assert.equal(note.hidden, false);
    assert.equal(note.innerHTML, "14 cards");
    assert.equal(document.element("east-card-count").hidden, true);
});

test("updateActionButtons hides the card-count note at the 13-card boundary", () => {
    const document = createMockDocument({
        north_spades:  "AKQJT98765432",
        north_hearts:  "A",
    });
    const ctx = loadDdsMvp(document);
    ctx.updateActionButtons();
    document.setValue("north_hearts", "");

    ctx.updateActionButtons();

    const note = document.element("north-card-count");
    assert.equal(note.hidden, true);
    assert.equal(note.innerHTML, "");
});

test("handCardHtml renders a clickable button for a card in a hand", () => {
    // Arrange
    const ctx = loadDdsMvp(createMockDocument());
    const card = new ctx.Card("spades", "A");

    // Act
    const html = ctx.handCardHtml("north", card, 0);

    // Assert
    assert.match(html, /<button\b/);
    assert.match(html, /type="button"/);
    assert.match(html, /class="hand-card"/);
    assert.match(html, /data-direction="north"/);
    assert.match(html, /data-card="SA"/);
    assert.match(html, /data-index="0"/);
    assert.match(html, />A<\/button>/);
    assert.match(html, /aria-label="North spade ace"/);
});

test("hand holdings render pips in input order, not sorted", () => {
    // Arrange: caret mapping requires display order to match the input string.
    const document = createMockDocument({ north_spades: "QA" });
    const ctx = loadDdsMvp(document);

    // Act
    const html = ctx.handHoldingHtml("north", "spades", ctx.collectHands().north, -1);

    // Assert
    const q = html.indexOf('data-card="SQ"');
    const a = html.indexOf('data-card="SA"');
    assert.ok(q >= 0 && a >= 0);
    assert.ok(q < a, "Q before A matches typed order QA");
});

test("handHoldingHtml inserts a caret marker at the given index", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "AK" });
    const ctx = loadDdsMvp(document);
    const cards = ctx.collectHands().north;

    // Act
    const beforeFirst = ctx.handHoldingHtml("north", "spades", cards, 0);
    const between = ctx.handHoldingHtml("north", "spades", cards, 1);
    const atEnd = ctx.handHoldingHtml("north", "spades", cards, 2);

    // Assert
    assert.match(
        beforeFirst,
        /^<span class="hand-caret"[^>]*><\/span><button/
    );
    assert.match(
        between,
        /data-card="SA"[^>]*>A<\/button><span class="hand-caret"[^>]*><\/span><button[^>]*data-card="SK"/
    );
    assert.match(
        atEnd,
        /data-card="SK"[^>]*>K<\/button><span class="hand-caret"[^>]*><\/span>$/
    );
});

test("updateHandCardDisplays mirrors suit holdings as hand-card buttons", () => {
    // Arrange
    const document = createMockDocument({
        north_spades: "AQ",
        north_hearts: "k",
        east_clubs: "",
    });
    const ctx = loadDdsMvp(document);

    // Act
    ctx.updateHandCardDisplays(ctx.collectHands());

    // Assert
    const northSpades = document.element("north_spades_cards").innerHTML;
    assert.match(
        northSpades,
        /data-direction="north" data-card="SA"[^>]*>A<\/button>/
    );
    assert.match(
        northSpades,
        /data-direction="north" data-card="SQ"[^>]*>Q<\/button>/
    );
    assert.equal((northSpades.match(/class="hand-card"/g) ?? []).length, 2);

    const northHearts = document.element("north_hearts_cards").innerHTML;
    assert.match(
        northHearts,
        /data-direction="north" data-card="HK"[^>]*>K<\/button>/
    );

    assert.equal(document.element("east_clubs_cards").innerHTML, "");
});

test("updateActionButtons refreshes hand-card displays from inputs", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "JT" });
    const ctx = loadDdsMvp(document);

    // Act
    ctx.updateActionButtons();

    // Assert
    const html = document.element("north_spades_cards").innerHTML;
    assert.match(html, /data-card="SJ"/);
    assert.match(html, /data-card="ST"/);
});

test("handleHandCardClick notifies onHandCardClick with direction and card", () => {
    // Arrange
    const document = createMockDocument({ south_hearts: "KQ" });
    const ctx = loadDdsMvp(document);
    const clicks = [];
    ctx.onHandCardClick = (direction, card) => {
        clicks.push({ direction, key: card.key() });
    };
    const target = {
        closest(selector) {
            assert.equal(selector, ".hand-card");
            return {
                getAttribute(name) {
                    if (name === "data-direction") {
                        return "south";
                    }
                    if (name === "data-card") {
                        return "HK";
                    }
                    if (name === "data-index") {
                        return "0";
                    }
                    return null;
                },
            };
        },
    };

    // Act
    ctx.handleHandCardClick({ target, preventDefault() {} });

    // Assert
    assert.deepEqual(clicks, [{ direction: "south", key: "HK" }]);
});

test("handleHandCardClick places the suit-input caret for editing", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "AQ8" });
    const ctx = loadDdsMvp(document);
    const input = document.element("north_spades");
    const button = {
        getAttribute(name) {
            const attrs = {
                "data-direction": "north",
                "data-card": "SQ",
                "data-index": "1",
            };
            return attrs[name] ?? null;
        },
        getBoundingClientRect() {
            return { left: 100, width: 20 };
        },
    };

    // Act: click the right half → caret after Q (index 2).
    ctx.handleHandCardClick({
        target: {
            closest(selector) {
                return selector === ".hand-card" ? button : null;
            },
        },
        clientX: 115,
        preventDefault() {},
    });

    // Assert
    assert.equal(document.activeElement, input);
    assert.equal(input.selectionStart, 2);
    assert.equal(input.selectionEnd, 2);
    assert.match(
        document.element("north_spades_cards").innerHTML,
        /data-card="SQ"[^>]*>Q<\/button><span class="hand-caret"/
    );
});

test("handleHandCardClick places the caret before a card on a left-half click", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "AQ8" });
    const ctx = loadDdsMvp(document);
    const input = document.element("north_spades");
    const button = {
        getAttribute(name) {
            const attrs = {
                "data-direction": "north",
                "data-card": "SQ",
                "data-index": "1",
            };
            return attrs[name] ?? null;
        },
        getBoundingClientRect() {
            return { left: 100, width: 20 };
        },
    };

    // Act: click the left half → caret before Q (index 1).
    ctx.handleHandCardClick({
        target: {
            closest(selector) {
                return selector === ".hand-card" ? button : null;
            },
        },
        clientX: 105,
        preventDefault() {},
    });

    // Assert
    assert.equal(document.activeElement, input);
    assert.equal(input.selectionStart, 1);
});

test("backspace at the caret removes the pip to the left", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "AQ8" });
    const ctx = loadDdsMvp(document);
    ctx.pageLoad();
    const input = document.element("north_spades");
    input.focus();
    input.setSelectionRange(2, 2); // after Q

    // Act: simulate Backspace editing the value, then the input event.
    input.value = "A8";
    input.setSelectionRange(1, 1);
    input.dispatch("input", {});

    // Assert
    assert.equal(input.value, "A8");
    const html = document.element("north_spades_cards").innerHTML;
    assert.match(html, /data-card="SA"/);
    assert.match(html, /data-card="S8"/);
    assert.doesNotMatch(html, /data-card="SQ"/);
});

test("typing at the caret inserts a pip at the insertion point", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "A8" });
    const ctx = loadDdsMvp(document);
    ctx.pageLoad();
    const input = document.element("north_spades");
    input.focus();
    input.setSelectionRange(1, 1); // between A and 8

    // Act
    input.value = "AQ8";
    input.setSelectionRange(2, 2);
    input.dispatch("input", {});

    // Assert
    assert.equal(input.value, "AQ8");
    const html = document.element("north_spades_cards").innerHTML;
    const a = html.indexOf('data-card="SA"');
    const q = html.indexOf('data-card="SQ"');
    const eight = html.indexOf('data-card="S8"');
    assert.ok(a < q && q < eight);
});

test("handleHandSuitClick focuses the suit input at end of the holding", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "AK" });
    const ctx = loadDdsMvp(document);
    const input = document.element("north_spades");
    const suitRow = {
        querySelector(selector) {
            assert.equal(selector, ".hand-suit-input");
            return input;
        },
    };
    const target = {
        closest(selector) {
            if (selector === ".hand-card") {
                return null;
            }
            if (selector === ".hand-suit") {
                return suitRow;
            }
            return null;
        },
    };

    // Act
    ctx.handleHandSuitClick({ target });

    // Assert
    assert.equal(document.activeElement, input);
    assert.equal(input.selectionStart, 2);
    assert.equal(input.selectionEnd, 2);
});

test("handleHandCardClick ignores clicks outside a hand-card", () => {
    // Arrange
    const ctx = loadDdsMvp(createMockDocument());
    let called = false;
    ctx.onHandCardClick = () => {
        called = true;
    };

    // Act
    ctx.handleHandCardClick({
        target: {
            closest() {
                return null;
            },
        },
        preventDefault() {},
    });

    // Assert
    assert.equal(called, false);
});

test("pageLoad wires hand-card clicks on the diagram", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    const clicks = [];
    ctx.onHandCardClick = (direction, card) => {
        clicks.push({ direction, key: card.key() });
    };

    // Act
    ctx.pageLoad();
    document.dispatch("click", {
        target: {
            closest(selector) {
                if (selector !== ".hand-card") {
                    return null;
                }
                return {
                    getAttribute(name) {
                        if (name === "data-direction") {
                            return "west";
                        }
                        if (name === "data-card") {
                            return "C2";
                        }
                        return null;
                    },
                };
            },
        },
        preventDefault() {},
    });

    // Assert
    assert.deepEqual(clicks, [{ direction: "west", key: "C2" }]);
});

test("openingLeader is the declarer LHO", () => {
    // Arrange
    const ctx = loadDdsMvp(createMockDocument());

    // Assert
    assert.equal(ctx.openingLeader("south"), "west");
    assert.equal(ctx.openingLeader("west"), "north");
    assert.equal(ctx.openingLeader("north"), "east");
    assert.equal(ctx.openingLeader("east"), "south");
});

test("pipFromDdsRank maps DDS ranks 2-14 onto pips", () => {
    const ctx = loadDdsMvp(createMockDocument());

    assert.equal(ctx.pipFromDdsRank(14), "A");
    assert.equal(ctx.pipFromDdsRank(13), "K");
    assert.equal(ctx.pipFromDdsRank(12), "Q");
    assert.equal(ctx.pipFromDdsRank(11), "J");
    assert.equal(ctx.pipFromDdsRank(10), "T");
    assert.equal(ctx.pipFromDdsRank(9), "9");
    assert.equal(ctx.pipFromDdsRank(2), "2");
});

test("leadTricksMapFromSolverOutput expands suit/rank/score triples to card keys", () => {
    // Arrange: flat buffer from dds_mvp_solve_leads
    const ctx = loadDdsMvp(createMockDocument());
    const out = [
        2,
        0, 14, 7, // SA → 7
        3, 10, 5, // CT → 5
    ];

    // Act
    const map = ctx.leadTricksMapFromSolverOutput(out);

    // Assert
    assert.equal(map.SA, 7);
    assert.equal(map.CT, 5);
});

test("handCardHtml renders a lower-right tricks numeral when provided", () => {
    // Arrange
    const ctx = loadDdsMvp(createMockDocument());
    const card = new ctx.Card("spades", "K");

    // Act
    const html = ctx.handCardHtml("west", card, 0, 7);

    // Assert
    assert.match(html, /class="hand-card[^"]*hand-card-with-tricks/);
    assert.match(
        html,
        /<span class="hand-card-tricks"[^>]*>7<\/span>/
    );
    assert.match(html, />K<span class="hand-card-tricks"/);
});

test("handCardHtml omits tricks numeral when score is absent", () => {
    const ctx = loadDdsMvp(createMockDocument());
    const html = ctx.handCardHtml("west", new ctx.Card("spades", "K"), 0);

    assert.doesNotMatch(html, /hand-card-tricks/);
    assert.doesNotMatch(html, /hand-card-with-tricks/);
});

test("handHoldingHtml badges only cards present in the lead-tricks map", () => {
    // Arrange
    const document = createMockDocument({ west_spades: "KQ" });
    const ctx = loadDdsMvp(document);
    const cards = ctx.collectHands().west;

    // Act
    const html = ctx.handHoldingHtml(
        "west",
        "spades",
        cards,
        -1,
        { SK: 8, SQ: 5 }
    );

    // Assert
    assert.match(
        html,
        /data-card="SK"[^>]*>K<span class="hand-card-tricks"[^>]*>8<\/span>/
    );
    assert.match(
        html,
        /data-card="SQ"[^>]*>Q<span class="hand-card-tricks"[^>]*>5<\/span>/
    );
});

test("hand-card-tricks CSS places a small numeral in the lower-right corner", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_mvp.css"), "utf8");
    const handCardMatch = css.match(/\.hand-card\s*\{([^}]*)\}/s);

    // Assert
    assert.ok(handCardMatch, ".hand-card rule present");
    const handCardRules = handCardMatch[1];
    assert.match(handCardRules, /position:\s*relative/);
    // Smaller than the seat's 30px so the corner tricks digit does not intersect the pip.
    assert.match(handCardRules, /font-size:\s*(?:0\.\d+em|[1-2]?\d(?:\.\d+)?px)/);
    assert.doesNotMatch(handCardRules, /font-size:\s*30px/);
    assert.match(css, /\.hand-card-tricks\s*\{[^}]*position:\s*absolute/s);
    assert.match(css, /\.hand-card-tricks\s*\{[^}]*right:/s);
    assert.match(css, /\.hand-card-tricks\s*\{[^}]*bottom:/s);
    assert.match(css, /\.hand-card-tricks\s*\{[^}]*font-size:\s*0\.\d+em/s);
});

test("denominationDisplayHtml renders suit glyphs and NT", () => {
    // Arrange
    const ctx = loadDdsMvp(createMockDocument());

    // Act / Assert
    assert.match(ctx.denominationDisplayHtml("H"), /<heart-suit>♥<\/heart-suit>/);
    assert.match(ctx.denominationDisplayHtml("S"), /<spade-suit>♠<\/spade-suit>/);
    assert.match(ctx.denominationDisplayHtml("D"), /<diamond-suit>♦<\/diamond-suit>/);
    assert.match(ctx.denominationDisplayHtml("C"), /<club-suit>♣<\/club-suit>/);
    assert.equal(ctx.denominationDisplayHtml("N"), "NT");
    assert.equal(ctx.denominationDisplayHtml("X"), "");
});

test("contractStatusHtml shows denomination and declarer", () => {
    // Arrange: East declares hearts.
    const ctx = loadDdsMvp(createMockDocument());

    // Act
    const html = ctx.contractStatusHtml({ direction: "east", denomination: "H" });

    // Assert: denomination, then "by", then declarer on one row.
    assert.match(html, /class="contract-status-denom"/);
    assert.match(html, /<heart-suit>♥<\/heart-suit>/);
    assert.match(html, /class="contract-status-by"[^>]*>by</);
    assert.match(html, /class="contract-status-declarer"[^>]*>E</);
    assert.match(
        html,
        /contract-status-denom[\s\S]*contract-status-by[\s\S]*contract-status-declarer/
    );
    assert.match(html, /aria-label="Hearts; East declares"/);
});

test("contractStatusHtml uses NT and South when South declares notrump", () => {
    const ctx = loadDdsMvp(createMockDocument());
    const html = ctx.contractStatusHtml({ direction: "south", denomination: "N" });

    assert.match(html, /class="contract-status-denom"[^>]*>NT</);
    assert.match(html, /class="contract-status-by"[^>]*>by</);
    assert.match(html, /class="contract-status-declarer"[^>]*>S</);
    assert.match(html, /aria-label="Notrump; South declares"/);
});

test("updateContractStatus hides the NE panel when no contract is selected", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    const status = document.element("contract-status");
    status.hidden = false;
    status.innerHTML = "stale";

    // Act
    ctx.updateContractStatus();

    // Assert
    assert.equal(status.hidden, true);
    assert.equal(status.innerHTML, "");
});

test("handleResultTableClick selects declarer and denomination from a cell", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    const table = document.element("result-table");
    const cell = table.rows[2].cells[3]; // East / Hearts
    const selections = [];
    ctx.onContractSelect = (direction, denomination) => {
        selections.push({ direction, denomination });
    };

    // Act
    ctx.handleResultTableClick({
        target: {
            closest(selector) {
                assert.equal(selector, "#result-table td");
                return cell;
            },
        },
    });

    // Assert
    const selected = ctx.selectedContract();
    assert.equal(selected.direction, "east");
    assert.equal(selected.denomination, "H");
    assert.deepEqual(selections, [{ direction: "east", denomination: "H" }]);
    assert.equal(cell.classList.contains("result-cell-selected"), true);

    const status = document.element("contract-status");
    assert.equal(status.hidden, false);
    assert.match(status.innerHTML, /<heart-suit>♥<\/heart-suit>/);
    assert.match(status.innerHTML, /class="contract-status-by"[^>]*>by</);
    assert.match(status.innerHTML, /class="contract-status-declarer"[^>]*>E</);
});

test("handleResultTableClick moves the highlight to the newly clicked cell", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    const table = document.element("result-table");
    const first = table.rows[1].cells[1]; // North / Clubs
    const second = table.rows[4].cells[5]; // West / NT

    // Act
    ctx.handleResultTableClick({
        target: {
            closest() {
                return first;
            },
        },
    });
    ctx.handleResultTableClick({
        target: {
            closest() {
                return second;
            },
        },
    });

    // Assert
    assert.equal(first.classList.contains("result-cell-selected"), false);
    assert.equal(second.classList.contains("result-cell-selected"), true);
    const selected = ctx.selectedContract();
    assert.equal(selected.direction, "west");
    assert.equal(selected.denomination, "N");
});

test("handleResultTableClick ignores clicks outside a result data cell", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    let called = false;
    ctx.onContractSelect = () => {
        called = true;
    };

    // Act
    ctx.handleResultTableClick({
        target: {
            closest() {
                return null;
            },
        },
    });

    // Assert
    assert.equal(called, false);
    assert.equal(ctx.selectedContract(), null);
});

test("pageLoad wires result-table cell clicks", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    const cell = document.element("result-table").rows[3].cells[4]; // South / Spades
    const selections = [];
    ctx.onContractSelect = (direction, denomination) => {
        selections.push({ direction, denomination });
    };

    // Act
    ctx.pageLoad();
    document.dispatch("click", {
        target: {
            closest(selector) {
                return selector === "#result-table td" ? cell : null;
            },
        },
    });

    // Assert
    assert.deepEqual(selections, [{ direction: "south", denomination: "S" }]);
    assert.equal(cell.classList.contains("result-cell-selected"), true);
});

test("result-cell-selected highlight is defined in CSS", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_mvp.css"), "utf8");

    // Assert
    assert.match(css, /#result-table\s+td\.result-cell-selected\s*\{/s);
    assert.match(css, /#result-table\s+td\s*\{[^}]*cursor:\s*pointer/s);
});

test("result table lives in the hand diagram southeast corner", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const html = readFileSync(join(here, "..", "dds_mvp.html"), "utf8");
    const css = readFileSync(join(here, "..", "dds_mvp.css"), "utf8");

    // Assert: table is a child of the SE filler cell, not below the diagram.
    const seOpen = html.match(
        /<div class="[^"]*grid-filler-se[^"]*"[^>]*>/
    );
    assert.ok(seOpen, "southeast filler cell present");
    const afterSe = html.slice(html.indexOf(seOpen[0]) + seOpen[0].length);
    // Hint sits above the table inside the SE cell.
    assert.match(
        afterSe,
        /class="[^"]*result-table-hint[^"]*"[^>]*>Click to set declarer and denomination</
    );
    assert.match(
        afterSe,
        /result-table-hint[\s\S]*?id="result-table"/
    );
    assert.match(afterSe, /id="result-table"/);
    // SE cell must be readable (not aria-hidden) and sized for the table.
    assert.doesNotMatch(seOpen[0], /aria-hidden="true"/);
    assert.match(css, /\.grid-item\.grid-filler-se\s*\{[^}]*font-size:/s);
    assert.match(css, /\.grid-item\.grid-filler-se\s*\{[^}]*flex-direction:\s*column/s);
    assert.match(css, /\.result-table-hint\s*\{/s);
});

test("contract status lives in the hand diagram northeast corner", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const html = readFileSync(join(here, "..", "dds_mvp.html"), "utf8");
    const css = readFileSync(join(here, "..", "dds_mvp.css"), "utf8");

    // Assert
    const neMatch = html.match(
        /<div class="[^"]*grid-filler-ne[^"]*"[^>]*>([\s\S]*?)<\/div>/
    );
    assert.ok(neMatch, "northeast filler cell present");
    assert.match(neMatch[1], /id="contract-status"/);
    assert.doesNotMatch(neMatch[0], /aria-hidden="true"/);
    assert.match(css, /\.grid-item\.grid-filler-ne\s*\{[^}]*font-size:/s);
    assert.match(css, /\.contract-status-denom/);
    assert.match(css, /\.contract-status-by/);
    assert.match(css, /\.contract-status-declarer/);
    // Declarer matches denomination size; "by" sits between them on one row.
    assert.match(
        css,
        /\.contract-status-denom\s*\{[^}]*font-size:\s*1\.6em/s
    );
    assert.match(
        css,
        /\.contract-status-declarer\s*\{[^}]*font-size:\s*1\.6em/s
    );
    assert.match(
        css,
        /\.contract-status-panel\s*\{[^}]*display:\s*flex/s
    );
    assert.doesNotMatch(
        css,
        /\.contract-status-panel\s*\{[^}]*flex-direction:\s*column/s
    );
    assert.doesNotMatch(css, /\.contract-status-declarer\s*\{[^}]*margin-top:/s);
});

test("hand diagram markup uses hand-suit rows with concealed text inputs", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const htmlPath = join(here, "..", "dds_mvp.html");
    const cssPath = join(here, "..", "dds_mvp.css");

    // Act
    const html = readFileSync(htmlPath, "utf8");
    const css = readFileSync(cssPath, "utf8");

    // Assert: each suit is a hand-suit row; cards replace visible text.
    assert.match(
        html,
        /<span class="hand-suit">\s*<spade-suit>[\s\S]*?id="north_spades_cards"[\s\S]*?id="north_spades"[^>]*class="[^"]*hand-suit-input/
    );
    assert.equal((html.match(/class="hand-suit"/g) ?? []).length, 16);
    assert.equal((html.match(/class="hand-suit-input"/g) ?? []).length, 16);
    assert.match(css, /\.hand-suit-input\s*\{[^}]*color:\s*transparent/s);
    assert.match(css, /\.hand-suit-input\s*\{[^}]*caret-color:/s);
});

test("hand seats left-align suit symbols in the diagram", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_mvp.css"), "utf8");

    // Assert: beat .grid-item { text-align: center } via higher specificity
    // and source order so filled holdings stay left-aligned, not centered.
    const gridItemAlign = css.search(/\.grid-item\s*\{[^}]*text-align:\s*center/s);
    const handAlign = css.search(
        /\.grid-item\.hand-north,\s*\n\s*\.grid-item\.hand-east,\s*\n\s*\.grid-item\.hand-south,\s*\n\s*\.grid-item\.hand-west\s*\{[^}]*text-align:\s*left/s
    );

    assert.ok(gridItemAlign >= 0, "grid-item centers by default");
    assert.ok(handAlign >= 0, "hand seats declare left alignment");
    assert.ok(
        handAlign > gridItemAlign,
        "hand left-align must follow .grid-item so it wins the cascade"
    );
    // All seats need room for typical 8-card suits (e.g. Everyone makes 3N).
    // min-width:0 stops longer holdings from expanding columns (layout jump).
    assert.match(
        css,
        /\.grid-container\s*\{[^}]*grid-template-columns:\s*1\.5fr\s+1\.5fr\s+1\.6fr/s
    );
    assert.match(css, /\.grid-item\s*\{[^}]*min-width:\s*0/s);
    assert.match(css, /\.grid-outer\s*\{[^}]*max-width:\s*1100px/s);
});

test("hand-card pips show a light outline affordance for clickability", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_mvp.css"), "utf8");
    const handCardMatch = css.match(/\.hand-card\s*\{([^}]*)\}/s);

    // Assert: resting state (not only :hover) has a visible clickable cue.
    assert.ok(handCardMatch, ".hand-card rule present");
    const rules = handCardMatch[1];
    assert.doesNotMatch(rules, /border:\s*none/);
    assert.match(
        rules,
        /(?:outline:\s*1px\s+solid|border:\s*1px\s+solid|box-shadow:\s*0\s+0\s+0\s+1px)/
    );
});

test("typing a pip into a suit input inserts the matching hand-card glyph", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.pageLoad();

    // Act: type as the user would — set value then fire input.
    document.setValue("north_spades", "A");
    document.element("north_spades").dispatch("input", {});

    // Assert
    assert.equal(document.element("north_spades").value, "A");
    assert.match(
        document.element("north_spades_cards").innerHTML,
        /class="hand-card"[^>]*data-card="SA"[^>]*>A<\/button>/
    );
});

test("handleHandSuitClick does not steal focus from a hand-card click", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    document.setActiveElement("east_hearts");
    let focused = false;
    const input = document.element("north_spades");
    input.focus = () => {
        focused = true;
    };
    const target = {
        closest(selector) {
            if (selector === ".hand-card") {
                return { className: "hand-card" };
            }
            if (selector === ".hand-suit") {
                return {
                    querySelector() {
                        return input;
                    },
                };
            }
            return null;
        },
    };

    // Act
    ctx.handleHandSuitClick({ target });

    // Assert
    assert.equal(focused, false);
});
