/**
 * Unit tests for DDS Web JS layers (Node built-in test runner).
 *
 * Run with:
 *    bazelisk test //web:dds_web_js_test
 * or: python -m unittest web.tests.test_dds_web_js
 * or: node --test web/tests/dds_web_test.mjs
 */
import assert from "node:assert/strict";
import { existsSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { createContext, runInContext } from "node:vm";

const DIRECTIONS = ["north", "east", "south", "west"];
const SUITS = ["spades", "hearts", "diamonds", "clubs"];

function findWebJsPath(fileName, envKey) {
    if (process.env[envKey] && existsSync(process.env[envKey])) {
        return process.env[envKey];
    }

    const here = dirname(fileURLToPath(import.meta.url));
    const adjacent = join(here, "..", fileName);
    if (existsSync(adjacent)) {
        return adjacent;
    }

    for (const base of [process.env.TEST_SRCDIR, process.env.RUNFILES_DIR]) {
        if (!base) {
            continue;
        }
        for (const sub of [`web/${fileName}`, `_main/web/${fileName}`]) {
            const candidate = join(base, sub);
            if (existsSync(candidate)) {
                return candidate;
            }
        }
    }

    throw new Error(`${fileName} not found`);
}

/** Reject if `promise` does not settle within `ms` (clears the timer either way). */
function withTimeout(promise, ms, message) {
    let timer;
    const timeout = new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error(message)), ms);
    });
    return Promise.race([promise, timeout]).finally(() => clearTimeout(timer));
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
            style: {},
            selectionStart: 0,
            selectionEnd: 0,
            attributes: {},
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
            setAttribute(name, value) {
                this.attributes[name] = String(value);
            },
            getAttribute(name) {
                return Object.prototype.hasOwnProperty.call(this.attributes, name)
                    ? this.attributes[name]
                    : null;
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
    makeElement("trick-status");
    makeElement("contract-status");
    makeElement("sample-deals");
    makeElement("play-bar");
    makeElement("play-hint");
    makeElement("play-score");
    makeElement("edit-hands");
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
                tabIndex: -1,
                attributes: {},
                setAttribute(name, value) {
                    this.attributes[name] = String(value);
                    if (name === "tabindex") {
                        this.tabIndex = Number(value);
                    }
                },
                getAttribute(name) {
                    return Object.prototype.hasOwnProperty.call(
                        this.attributes,
                        name
                    )
                        ? this.attributes[name]
                        : null;
                },
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

    const body = {
        className: "",
        classList: {
            add(name) {
                const classes = new Set(body.className.split(/\s+/).filter(Boolean));
                classes.add(name);
                body.className = [...classes].join(" ");
            },
            remove(name) {
                const classes = new Set(body.className.split(/\s+/).filter(Boolean));
                classes.delete(name);
                body.className = [...classes].join(" ");
            },
            contains(name) {
                return body.className.split(/\s+/).includes(name);
            },
        },
    };

    const documentRef = {
        activeElement: null,
        body,
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

function runDdsWebScripts(context) {
    runInContext(
        readFileSync(
            findWebJsPath("dds_web_deal_import.js", "DDS_WEB_DEAL_IMPORT_JS"),
            "utf8"
        ),
        context,
        { filename: "dds_web_deal_import.js" }
    );
    runInContext(
        readFileSync(findWebJsPath("dds_web_core.js", "DDS_WEB_CORE_JS"), "utf8"),
        context,
        { filename: "dds_web_core.js" }
    );
    runInContext(
        readFileSync(
            findWebJsPath("dds_web_solve.js", "DDS_WEB_SOLVE_JS"),
            "utf8"
        ),
        context,
        { filename: "dds_web_solve.js" }
    );
    runInContext(
        readFileSync(findWebJsPath("dds_web_play.js", "DDS_WEB_PLAY_JS"), "utf8"),
        context,
        { filename: "dds_web_play.js" }
    );
    runInContext(
        readFileSync(findWebJsPath("dds_web.js", "DDS_WEB_JS"), "utf8"),
        context,
        { filename: "dds_web.js" }
    );
}

function loadDdsWeb(document, extras = {}) {
    const sandbox = {
        document,
        console,
        Promise,
        Error,
        setTimeout,
        clearTimeout,
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
        performance: {
            now() {
                return 0;
            },
        },
        window: {
            addEventListener() {},
        },
        ...extras,
    };
    if (!sandbox.window) {
        sandbox.window = { addEventListener() {} };
    }
    const context = createContext(sandbox);
    runDdsWebScripts(context);
    // Existing tests expect hand edits to schedule immediately; debounce is
    // covered by dedicated tests that opt into a non-zero delay.
    if (typeof context.setDealSolveDebounceMs === "function") {
        context.setDealSolveDebounceMs(0);
    }
    // Same for the Computing… grace period: most tests want an immediate solve.
    if (typeof context.setDdTableComputingDelayMs === "function") {
        context.setDdTableComputingDelayMs(0);
    }
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
    const ctx = loadDdsWeb(createMockDocument());
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
    const ctx = loadDdsWeb(createMockDocument());
    const cards = ["S2", "SK", "SA"].map(ctx.Card.fromKey);

    // Act
    cards.sort(ctx.Card.compare);

    // Assert
    assert.deepEqual(cards.map((card) => card.key()), ["SA", "SK", "S2"]);
});

test("cardsToSuitHoldings groups cards by suit name", () => {
    const ctx = loadDdsWeb(createMockDocument());
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
    const ctx = loadDdsWeb(document);
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

test("deck status puts each suit in its own row wrapper", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);

    // Act
    ctx.updateActionButtons();
    const deckStatus = document.element("deck-status").innerHTML;

    // Assert: four rows, spades on top through clubs at the bottom.
    const rows = [...deckStatus.matchAll(/class="deck-suit-row"/g)];
    assert.equal(rows.length, 4);
    assert.match(
        deckStatus,
        /class="deck-suit-row"[^>]*>\s*<spade-suit>[\s\S]*?<\/spade-suit>[\s\S]*?<\/div>\s*<div class="deck-suit-row"[^>]*>\s*<heart-suit>[\s\S]*?<\/heart-suit>[\s\S]*?<\/div>\s*<div class="deck-suit-row"[^>]*>\s*<diamond-suit>[\s\S]*?<\/diamond-suit>[\s\S]*?<\/div>\s*<div class="deck-suit-row"[^>]*>\s*<club-suit>/
    );
});

test("handsToPbn formats part-score deal", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.fillFormWithPartScoreTestData();
    const pbn = ctx.handsToPbn(ctx.collectHands());
    assert.equal(
        pbn,
        "N:AQ85.AK976.5.J87 JT.QJ5432.Q9.KQ9 972..JT863.A6432 K643.T8.AK742.T5"
    );
});

test("inputIsValid rejects incomplete deal", () => {
    const ctx = loadDdsWeb(createMockDocument());
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
    const ctx = loadDdsWeb(createMockDocument());
    const hands = handsFromKeys(ctx, {
        north: ["S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "ST", "SJ", "SQ", "SK", "SA"],
        east: ["HA", "H2", "H3", "H4", "H5", "H6", "H7", "H8", "H9", "HT", "HJ", "HQ", "HK"],
        south: ["DA", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DT", "DJ", "DQ", "DK"],
        west: ["CA", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CT", "CJ", "CQ", "CK"],
    });
    hands.north[0] = { suit: "spades", pip: "1", key() { return "S1"; } };
    assert.match(ctx.inputIsValid(hands), /^Please use only these pips:/);
});

test("inputIsValid rejects duplicate cards", () => {
    const ctx = loadDdsWeb(createMockDocument());
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
    ctx.clearTestData();
    assert.equal(document.element("north_spades").value, "");
    assert.equal(document.element("west_clubs").value, "");
});

test("rotateClockwise shifts holdings west to north", () => {
    // Markers must be valid pips — rotateClockwise ends in updateActionButtons,
    // which strips non-pip characters.
    const document = createMockDocument();
    const markers = [
        "A", "K", "Q", "J",
        "T", "9", "8", "7",
        "6", "5", "4", "3",
        "2", "AK", "AQ", "AJ",
    ];
    let index = 0;
    for (const direction of DIRECTIONS) {
        for (const suit of SUITS) {
            document.setValue(`${direction}_${suit}`, markers[index]);
            index += 1;
        }
    }
    const ctx = loadDdsWeb(document);
    ctx.rotateClockwise();
    assert.equal(document.element("north_spades").value, "2");
    assert.equal(document.element("north_hearts").value, "AK");
    assert.equal(document.element("north_diamonds").value, "AQ");
    assert.equal(document.element("north_clubs").value, "AJ");
    assert.equal(document.element("east_spades").value, "A");
});

test("fillFormWithPartScoreTestData populates inputs", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.fillFormWithPartScoreTestData();
    assert.equal(document.element("north_spades").value, "AQ85");
    assert.equal(document.element("west_clubs").value, "T5");
});

test("handleSampleDealSelected loads part-score and resets the select", () => {
    const document = createMockDocument();
    const select = document.element("sample-deals");
    select.value = "part-score";
    const ctx = loadDdsWeb(document);

    ctx.handleSampleDealSelected(select);

    assert.equal(document.element("north_spades").value, "AQ85");
    assert.equal(document.element("west_clubs").value, "T5");
    assert.equal(select.value, "");
});

test("handleSampleDealSelected loads grand-slam deal", () => {
    const document = createMockDocument();
    const select = document.element("sample-deals");
    select.value = "grand-slam";
    const ctx = loadDdsWeb(document);

    ctx.handleSampleDealSelected(select);

    assert.equal(document.element("north_spades").value, "AKQJ");
    assert.equal(select.value, "");
});

test("handleSampleDealSelected loads everyone-3n deal", () => {
    const document = createMockDocument();
    const select = document.element("sample-deals");
    select.value = "everyone-3n";
    const ctx = loadDdsWeb(document);

    ctx.handleSampleDealSelected(select);

    assert.equal(document.element("north_spades").value, "QT9");
    assert.equal(select.value, "");
});

test("handleSampleDealSelected ignores empty selection", () => {
    const document = createMockDocument({ north_spades: "AK" });
    const select = document.element("sample-deals");
    select.value = "";
    const ctx = loadDdsWeb(document);

    ctx.handleSampleDealSelected(select);

    assert.equal(document.element("north_spades").value, "AK");
});

test("handleSampleDealSelected can reload the same sample after an edit", () => {
    // Arrange: load part-score, then edit a holding so the deal no longer matches.
    const document = createMockDocument();
    const select = document.element("sample-deals");
    const ctx = loadDdsWeb(document);
    select.value = "part-score";
    ctx.handleSampleDealSelected(select);
    assert.equal(document.element("north_spades").value, "AQ85");
    assert.equal(select.value, "");

    document.element("north_spades").value = "A";

    // Act: choose the same sample again (possible because the select reset).
    select.value = "part-score";
    ctx.handleSampleDealSelected(select);

    // Assert
    assert.equal(document.element("north_spades").value, "AQ85");
    assert.equal(select.value, "");
});

test("fillFormWithTestData does not require a double-dummy button", async () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
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
    const html = readFileSync(join(here, "..", "dds_web.html"), "utf8");
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    // Assert
    assert.doesNotMatch(html, /double-dummy-it/);
    assert.doesNotMatch(html, /Double-dummy it!/);
    assert.doesNotMatch(css, /#double-dummy-it/);
});

test("withTimeout rejects when the promise never settles", async () => {
    await assert.rejects(
        () => withTimeout(new Promise(() => {}), 20, "timed out waiting"),
        /timed out waiting/
    );
});

test("withTimeout propagates rejection from the wrapped promise", async () => {
    await assert.rejects(
        () => withTimeout(Promise.reject(new Error("lead failed")), 1000, "timed out"),
        /lead failed/
    );
});

test("updateActionButtons finishes dd table before play-position refresh", async () => {
    // Arrange: a selected contract must not race CalcDDtable vs SolveBoard.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const order = [];
    const southNt = document.element("result-table").rows[3].cells[5];
    southNt.innerHTML = "6";
    southNt.textContent = "6";

    ctx.refreshDdTable = async () => {
        order.push("dd-start");
        await new Promise((resolve) => setTimeout(resolve, 30));
        order.push("dd-end");
    };
    ctx.refreshPlayTricks = async () => {
        order.push("play");
    };

    ctx.fillFormWithPartScoreTestData();
    // Drain fillForm's auto-solve so a late dd-end cannot land after we clear.
    await ctx.scheduleDealSolve();
    order.length = 0;

    const playDone = new Promise((resolve, reject) => {
        const refreshPlay = ctx.refreshPlayTricks;
        ctx.refreshPlayTricks = async () => {
            try {
                await refreshPlay();
                resolve();
            } catch (err) {
                reject(err);
            }
        };
    });

    ctx.handleResultTableClick({
        target: {
            closest() {
                return southNt;
            },
        },
    });
    await withTimeout(
        playDone,
        1000,
        "timed out waiting for refreshPlayTricks"
    );

    // Assert: contract click runs DD then play scores in one job.
    assert.deepEqual(order, ["dd-start", "dd-end", "play"]);
});

test("contract selection waits for an in-flight dd table before play refresh", async () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const order = [];
    const southNt = document.element("result-table").rows[3].cells[5];
    southNt.innerHTML = "6";
    southNt.textContent = "6";

    ctx.refreshDdTable = async () => {
        order.push("dd-start");
        await new Promise((resolve) => setTimeout(resolve, 40));
        order.push("dd-end");
    };
    ctx.refreshPlayTricks = async () => {
        order.push("play");
    };

    // Act: start auto-solve, then select a contract before it finishes.
    ctx.fillFormWithPartScoreTestData();
    ctx.handleResultTableClick({
        target: {
            closest() {
                return southNt;
            },
        },
    });
    await new Promise((resolve) => setTimeout(resolve, 120));

    // Assert: coalesced jobs still run DD before play; obsolete jobs no-op.
    assert.ok(order.indexOf("dd-start") >= 0);
    assert.ok(order.indexOf("dd-end") >= 0);
    assert.equal(order[order.length - 1], "play");
    assert.ok(order.lastIndexOf("dd-end") < order.lastIndexOf("play"));
});

test("rapid scheduleDealSolve coalesces to one trailing play refresh", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    let play = 0;
    let dd = 0;
    const southNt = document.element("result-table").rows[3].cells[5];
    southNt.innerHTML = "6";
    southNt.textContent = "6";

    ctx.refreshDdTable = async () => {
        dd += 1;
        await new Promise((resolve) => setTimeout(resolve, 20));
    };
    ctx.refreshPlayTricks = async () => {
        play += 1;
    };

    ctx.fillFormWithPartScoreTestData();
    ctx.handleResultTableClick({
        target: {
            closest() {
                return southNt;
            },
        },
    });
    ctx.scheduleDealSolve();
    ctx.scheduleDealSolve();
    await new Promise((resolve) => setTimeout(resolve, 120));

    assert.equal(play, 1);
    assert.ok(dd >= 1);
});

test("rapid scheduleDealSolve does not enqueue one queue job per call", async () => {
    // Arrange: block the first DD solve so later schedules land while a job runs.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    let enqueued = 0;
    const realEnqueue = ctx.enqueueSolve;
    ctx.enqueueSolve = (task) => {
        enqueued += 1;
        return realEnqueue(task);
    };

    let releaseDd;
    let ddRuns = 0;
    ctx.refreshDdTable = () => {
        ddRuns += 1;
        return new Promise((resolve) => {
            releaseDd = resolve;
        });
    };
    ctx.refreshOpeningLeadTricks = async () => {};

    // fillForm → updateActionButtons → scheduleDealSolve (one queue job).
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 20));
    assert.equal(enqueued, 1);
    assert.equal(ddRuns, 1);
    const enqueuedWhileBlocked = enqueued;

    // Act: contract click + many schedules while that job is in flight.
    ctx.handleResultTableClick({
        target: {
            closest() {
                return document.element("result-table").rows[3].cells[5];
            },
        },
    });
    for (let i = 0; i < 20; i++) {
        ctx.scheduleDealSolve();
    }

    // Assert: no additional promise-chain entries for the burst.
    assert.equal(enqueued, enqueuedWhileBlocked);

    releaseDd();
    await new Promise((resolve) => setTimeout(resolve, 40));
    // Trailing epoch may re-run work inside the same job, but still one enqueue.
    assert.equal(enqueued, enqueuedWhileBlocked);
    assert.ok(ddRuns >= 2);
});

test("subsequent edits of a still-complete deal are debounced", async () => {
    // Arrange: complete deal; reordering pips keeps the deal complete so each
    // keystroke would otherwise schedule a solve immediately.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(50);
    let ddRuns = 0;
    ctx.refreshDdTable = async () => {
        ddRuns += 1;
    };
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 80));
    assert.equal(ddRuns, 1);
    ddRuns = 0;

    // Act: burst of still-complete edits (reorder South's spade holding).
    document.setValue("south_spades", "927");
    ctx.updateActionButtons(document.element("south_spades"));
    document.setValue("south_spades", "279");
    ctx.updateActionButtons(document.element("south_spades"));
    document.setValue("south_spades", "972");
    ctx.updateActionButtons(document.element("south_spades"));

    // Assert: no solve until the debounce window elapses, then one trailing run.
    assert.equal(ddRuns, 0);
    await new Promise((resolve) => setTimeout(resolve, 20));
    assert.equal(ddRuns, 0);
    await new Promise((resolve) => setTimeout(resolve, 50));
    assert.equal(ddRuns, 1);
});

test("scheduleDealSolveDebounced does not return an awaitable for the trailing solve", () => {
    // Arrange: non-zero debounce so the timer path is used (not the sync fallback).
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(200);
    ctx.refreshDdTable = async () => {};

    // Act
    const returned = ctx.scheduleDealSolveDebounced();

    // Assert: callers must not treat the return as "debounced work finished".
    assert.equal(returned, undefined);
});

test("disabling debounce cancels a pending debounced solve", async () => {
    // Arrange: complete deal with a pending trailing hand-edit solve.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(100);
    let ddRuns = 0;
    ctx.refreshDdTable = async () => {
        ddRuns += 1;
    };
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(ddRuns, 1);
    ddRuns = 0;

    document.setValue("south_spades", "927");
    ctx.updateActionButtons(document.element("south_spades"));
    assert.equal(ddRuns, 0);

    // Act: disable debounce while the timer is still pending.
    ctx.setDealSolveDebounceMs(0);

    // Assert: the previously scheduled trailing solve must not fire.
    await new Promise((resolve) => setTimeout(resolve, 150));
    assert.equal(ddRuns, 0);
});

test("contract selection still schedules a deal solve immediately", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(200);
    let ddRuns = 0;
    ctx.refreshDdTable = async () => {
        ddRuns += 1;
    };
    ctx.fillFormWithPartScoreTestData();
    // fillForm completes the deal, which schedules immediately (not debounced).
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(ddRuns, 1);
    ddRuns = 0;

    ctx.handleResultTableClick({
        target: {
            closest() {
                return document.element("result-table").rows[3].cells[5];
            },
        },
    });

    // Contract click must not wait for the hand-edit debounce.
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(ddRuns, 1);
});

test("fourth-hand auto-fill schedules a solve immediately despite debounce", async () => {
    // Arrange: three complete hands; completing the third triggers auto-fill.
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(200);
    let ddRuns = 0;
    ctx.refreshDdTable = async () => {
        ddRuns += 1;
    };

    // Act
    ctx.updateActionButtons();
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert: West is filled and the DD solve does not wait for debounce.
    assert.equal(document.element("west_spades").value, "K643");
    assert.equal(ddRuns, 1);
});

test("completing the fourth hand manually schedules a solve immediately", async () => {
    // Arrange: South is one card short of a complete deal.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(200);
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 250));

    document.setValue("south_spades", "97"); // was 972; now 12 cards in South
    ctx.updateActionButtons(document.element("south_spades"));
    await new Promise((resolve) => setTimeout(resolve, 250));

    let ddRuns = 0;
    ctx.refreshDdTable = async () => {
        ddRuns += 1;
    };

    // Act: type the final pip that restores 13 cards.
    document.setValue("south_spades", "972");
    ctx.updateActionButtons(document.element("south_spades"));
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert
    assert.equal(ddRuns, 1);
});

test("breaking a complete deal clears DD results immediately despite debounce", async () => {
    // Arrange: complete deal with populated results; deleting a card must not
    // leave stale DD numerals visible for the debounce window.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(200);
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 250));

    document.element("result").innerHTML = "Solved in 12 ms.";
    document.element("result-table").innerHTML =
        "<tr><td>N</td><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td></tr>";

    let ddRuns = 0;
    ctx.refreshDdTable = async () => {
        ddRuns += 1;
        // Mirror production: incomplete deals clear immediately.
        document.element("result").innerHTML = "";
        document.element("result-table").innerHTML = "";
    };

    // Act: remove a card so the deal is no longer complete.
    document.setValue("south_spades", "97");
    ctx.updateActionButtons(document.element("south_spades"));
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert: clear path runs now, not after the debounce delay.
    assert.equal(ddRuns, 1);
    assert.equal(document.element("result").innerHTML, "");
    assert.equal(document.element("result-table").innerHTML, "");
});

test("edits that keep the deal incomplete refresh immediately despite debounce", async () => {
    // Arrange: incomplete deal; clear/error updates are cheap (no WASM) and
    // must stay responsive so validation/status does not linger.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(200);
    let ddRuns = 0;
    ctx.refreshDdTable = async () => {
        ddRuns += 1;
    };

    document.setValue("north_spades", "A");
    ctx.updateActionButtons(document.element("north_spades"));
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(ddRuns, 1);
    ddRuns = 0;

    // Act: another still-incomplete edit.
    document.setValue("north_spades", "AK");
    ctx.updateActionButtons(document.element("north_spades"));
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert: not deferred by the hand-edit debounce window.
    assert.equal(ddRuns, 1);
});

test("pageLoad shows valid pips", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.pageLoad();
    assert.equal(document.element("valid-pips").innerHTML, "AKQJT98765432");
});

test("loadDdsModule rejects missing wasm globals", async () => {
    const ctx = loadDdsWeb(createMockDocument());
    await assert.rejects(
        () => ctx.loadDdsModule(),
        /WASM module not found/
    );
});

test("wasmSolveEnvironmentError explains file:// cannot load WASM workers", () => {
    // Arrange: browser opened as a local file (origin null).
    const sandbox = {
        document: createMockDocument(),
        console,
        Promise,
        Error,
        setTimeout,
        location: { protocol: "file:" },
    };
    const context = createContext(sandbox);
    runDdsWebScripts(context);

    // Act / Assert
    assert.match(
        context.wasmSolveEnvironmentError(),
        /python3 web\/serve_web\.py/
    );
});

test("wasmSolveEnvironmentError explains missing SharedArrayBuffer headers", () => {
    // Arrange: HTTPS page without cross-origin isolation (no SAB).
    const sandbox = {
        document: createMockDocument(),
        console,
        Promise,
        Error,
        setTimeout,
        location: { protocol: "https:" },
        SharedArrayBuffer: undefined,
    };
    const context = createContext(sandbox);
    runDdsWebScripts(context);

    // Act
    const message = context.wasmSolveEnvironmentError();

    // Assert: name the headers so any host can be fixed.
    assert.match(message, /SharedArrayBuffer/);
    assert.match(message, /Cross-Origin-Opener-Policy:\s*same-origin/);
    assert.match(message, /Cross-Origin-Embedder-Policy:\s*require-corp/);
});

test("wasmSolveEnvironmentError is null in non-browser sandboxes", () => {
    const ctx = loadDdsWeb(createMockDocument());
    assert.equal(ctx.wasmSolveEnvironmentError(), null);
});

test("fillFormWithGrandSlamTestData populates inputs", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.fillFormWithGrandSlamTestData();
    assert.equal(document.element("north_spades").value, "AKQJ");
    assert.equal(document.element("east_clubs").value, "432");
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
});

test("fillFormWithEveryoneMakes3nTestData populates inputs", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.fillFormWithEveryoneMakes3nTestData();
    assert.equal(document.element("north_hearts").value, "A8765432");
    assert.equal(document.element("west_spades").value, "");
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
});

test("fourthHandFillState accepts three full hands and one empty", () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsWeb(document);
    const state = ctx.fourthHandFillState(ctx.collectHands());
    assert.equal(state.canFill, true);
    assert.equal(state.emptyHand, "west");
});

test("fourthHandFillState rejects partial fourth hand", () => {
    const document = threeHandsPartScoreDocument();
    document.setValue("west_spades", "K");
    const ctx = loadDdsWeb(document);
    assert.equal(ctx.fourthHandFillState(ctx.collectHands()).canFill, false);
});

test("fourthHandFillState rejects a duplicate card across three full hands", () => {
    // Arrange: keep all three hands at 13 cards but duplicate D9 (east & north).
    const document = threeHandsPartScoreDocument();
    document.setValue("north_diamonds", "9");
    const ctx = loadDdsWeb(document);

    // Act
    const state = ctx.fourthHandFillState(ctx.collectHands());

    // Assert: 39 slots yield only 38 distinct cards, so the deal is not fillable.
    assert.equal(state.canFill, false);
});

test("fourthHandFillState rejects a non-bridge pip among three full hands", () => {
    // Arrange: three full hands and one empty, then inject an invalid pip.
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsWeb(document);
    const hands = ctx.collectHands();
    hands.north[12] = { suit: "clubs", pip: "X", key() { return "CX"; } };

    // Act
    const state = ctx.fourthHandFillState(hands);

    // Assert: invalid pips must not be treated as used cards for auto-fill.
    assert.equal(state.canFill, false);
});

test("fourthHandFillState rejects a missing card object among three full hands", () => {
    // Arrange: three full hands and one empty, then corrupt one card entry.
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsWeb(document);
    const hands = ctx.collectHands();
    hands.north[0] = null;

    // Act / Assert
    assert.equal(ctx.fourthHandFillState(hands).canFill, false);
});

test("sanitizeSuitHolding keeps only bridge pips and uppercases them", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.sanitizeSuitHolding("akq"), "AKQ");
    assert.equal(ctx.sanitizeSuitHolding("t9"), "T9");
    assert.equal(ctx.sanitizeSuitHolding("AKx!7x"), "AK7");
    assert.equal(ctx.sanitizeSuitHolding("QA2"), "AQ2");
    assert.equal(ctx.sanitizeSuitHolding("\"&<>'"), "");
    assert.equal(ctx.sanitizeSuitHolding(""), "");
    assert.equal(ctx.sanitizeSuitHolding(null), "");
});

test("sanitizeSuitHolding drops duplicate pips within a suit", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.sanitizeSuitHolding("AA"), "A");
    assert.equal(ctx.sanitizeSuitHolding("AKA"), "AK");
    assert.equal(ctx.sanitizeSuitHolding("aKa"), "AK");
    assert.equal(ctx.sanitizeSuitHolding("AKQJT98765432A"), "AKQJT98765432");
});

test("suitHoldingHasIllegalChars is true when any non-pip is present", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.suitHoldingHasIllegalChars("AKQ"), false);
    assert.equal(ctx.suitHoldingHasIllegalChars("akq"), false);
    assert.equal(ctx.suitHoldingHasIllegalChars("AKx"), true);
    assert.equal(ctx.suitHoldingHasIllegalChars("!"), true);
    assert.equal(ctx.suitHoldingHasIllegalChars(""), false);
    assert.equal(ctx.suitHoldingHasIllegalChars(null), false);
});

test("handleHandSuitInput beeps when the user enters a non-pip character", () => {
    const document = createMockDocument({ north_spades: "AK" });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const input = document.element("north_spades");

    // Act: type an illegal character (as the browser would leave it before sanitize).
    input.value = "AKx";
    ctx.handleHandSuitInput({ target: input });

    // Assert
    assert.equal(beeps, 1);
    assert.equal(input.value, "AK");
});

test("handleHandSuitInput does not beep for lowercase legal pips", () => {
    const document = createMockDocument({ north_spades: "" });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const input = document.element("north_spades");

    input.value = "akq";
    ctx.handleHandSuitInput({ target: input });

    assert.equal(beeps, 0);
    assert.equal(input.value, "AKQ");
});

test("handleHandSuitInput rejects a duplicate pip within the same suit", () => {
    const document = createMockDocument({ north_spades: "A" });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const input = document.element("north_spades");

    input.value = "AA";
    ctx.handleHandSuitInput({ target: input });

    assert.equal(beeps, 1);
    assert.equal(input.value, "A");
});

test("handleHandSuitInput moves a card typed from another hand", () => {
    const document = createMockDocument({
        north_spades: "A",
        east_spades: "",
    });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const east = document.element("east_spades");

    east.value = "A";
    ctx.handleHandSuitInput({ target: east });

    assert.equal(beeps, 0);
    assert.equal(document.element("north_spades").value, "");
    assert.equal(east.value, "A");
});

test("handleHandSuitInput rejects typing that would take a hand over 13 cards", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765432",
        north_hearts: "",
    });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    hearts.value = "A";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 1);
    assert.equal(document.element("north_spades").value, "AKQJT98765432");
    assert.equal(hearts.value, "");
    assert.equal(document.element("north-card-count").hidden, true);
});

test("handleHandSuitInput does not steal when the typed hand is already full", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765432",
        north_hearts: "",
        east_hearts: "A",
    });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    hearts.value = "A";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 1);
    assert.equal(hearts.value, "");
    assert.equal(document.element("east_hearts").value, "A");
});

test("handleHandSuitInput rejects adding a card that would replace another on a full hand", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765",
        north_hearts: "QJT",
    });
    const ctx = loadDdsWeb(document);
    ctx.updateActionButtons();
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    // At 13 cards, typing A must not displace T (high-rank clamp).
    hearts.value = "QJTA";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 1);
    assert.equal(hearts.value, "QJT");
    assert.equal(document.element("north_spades").value, "AKQJT98765");
});

test("handleHandSuitInput rejects switching pips on a full hand without changing count", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765",
        north_hearts: "QJT",
    });
    const ctx = loadDdsWeb(document);
    ctx.updateActionButtons();
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    hearts.value = "AJT";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 1);
    assert.equal(hearts.value, "QJT");
});

test("handleHandSuitInput allows deleting a card from a full hand", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765",
        north_hearts: "QJT",
    });
    const ctx = loadDdsWeb(document);
    ctx.updateActionButtons();
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    hearts.value = "QJ";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 0);
    assert.equal(hearts.value, "QJ");
});

test("handleHandSuitInput accepts typing that fills a hand to exactly 13 cards", () => {
    const document = createMockDocument({
        north_spades: "AKQJT9876543",
        north_hearts: "",
    });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    hearts.value = "A";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 0);
    assert.equal(hearts.value, "A");
    assert.equal(document.element("north-card-count").hidden, true);
});

test("handleHandSuitInput keeps only cards that fit when pasting past 13", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765",
        north_hearts: "",
    });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    // 10 spades already; only three hearts may be kept.
    hearts.value = "AKQJ";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 1);
    assert.equal(hearts.value, "AKQ");
});

test("handleHandSuitInput moves only kept pips when pasting from another hand past the limit", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765",
        east_hearts: "AKQJ",
        north_hearts: "",
    });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const hearts = document.element("north_hearts");

    // 10 spades already; only three hearts fit. Steal AKQ, leave J on east.
    hearts.value = "AKQJ";
    ctx.handleHandSuitInput({ target: hearts });

    assert.equal(beeps, 1);
    assert.equal(hearts.value, "AKQ");
    assert.equal(document.element("east_hearts").value, "J");
});

test("handleHandSuitInput moves typed pips out of another hand and keeps the rest", () => {
    const document = createMockDocument({
        east_spades: "AK",
        north_spades: "",
    });
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    const north = document.element("north_spades");

    north.value = "AQ";
    ctx.handleHandSuitInput({ target: north });

    assert.equal(beeps, 0);
    assert.equal(document.element("east_spades").value, "K");
    assert.equal(north.value, "AQ");
});

test("pageLoad wires suit inputs to handleHandSuitInput", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    let beeps = 0;
    ctx.playIllegalInputBeep = () => {
        beeps += 1;
    };
    ctx.pageLoad();

    const input = document.element("north_spades");
    input.value = "X";
    input.dispatch("input", { target: input });

    assert.equal(beeps, 1);
    assert.equal(input.value, "");
});

test("updateActionButtons strips non-pip characters from suit inputs", () => {
    const document = createMockDocument({ north_spades: "AKx7" });
    const ctx = loadDdsWeb(document);

    ctx.updateActionButtons();

    assert.equal(document.element("north_spades").value, "AK7");
});

test("updateActionButtons uppercases lowercase pips in suit inputs", () => {
    const document = createMockDocument({ east_hearts: "kq" });
    const ctx = loadDdsWeb(document);

    ctx.updateActionButtons();

    assert.equal(document.element("east_hearts").value, "KQ");
});

test("updateActionButtons strips a non-bridge pip and does not auto-fill", async () => {
    // Arrange: three full hands, but north clubs has an invalid pip (X).
    const document = threeHandsPartScoreDocument();
    document.setValue("north_clubs", "J8X");
    const ctx = loadDdsWeb(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };

    // Act
    ctx.updateActionButtons();
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert: X is removed (north incomplete) and the fourth hand stays empty.
    assert.equal(document.element("north_clubs").value, "J8");
    assert.equal(document.element("west_spades").value, "");
    assert.equal(document.element("west_hearts").value, "");
    assert.equal(document.element("west_diamonds").value, "");
    assert.equal(document.element("west_clubs").value, "");
    assert.equal(refreshed, 1);
});

test("updateActionButtons auto-fills the fourth hand for three complete hands", async () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
    let refreshed = 0;
    ctx.refreshDdTable = () => {
        refreshed += 1;
    };
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.equal(refreshed, 1);
});

test("hand edit exits play and clears play badges", async () => {
    // Arrange: play mode with badges, then edit a holding (exits play).
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.refreshDdTable = async () => {};

    let playCalls = 0;
    ctx.solvePlayPosition = () => {
        playCalls += 1;
        return Promise.resolve({ SK: 0, HA: -1 });
    };

    ctx.fillFormWithPartScoreTestData();
    const cell = document.element("result-table").rows[3].cells[5]; // South / NT
    cell.innerHTML = "6";
    cell.textContent = "6";
    ctx.handleResultTableClick({
        target: {
            closest() {
                return cell;
            },
        },
    });
    await new Promise((resolve) => setTimeout(resolve, 40));
    assert.equal(ctx.isPlayMode(), true);
    assert.match(
        document.element("west_spades_cards").innerHTML,
        /hand-card-tricks/
    );

    // Act: edit a holding — deal entry changes leave play mode.
    document.element("north_spades").value = "AQ8";
    ctx.updateActionButtons();
    // Play mode locks inputs; simulate Edit hands then edit.
    ctx.exitPlay();
    document.element("north_spades").value = "AQ8";
    ctx.updateActionButtons();

    // Assert: play badges are gone after exiting play.
    assert.equal(ctx.isPlayMode(), false);
    assert.doesNotMatch(
        document.element("west_spades_cards").innerHTML,
        /hand-card-tricks/
    );
    assert.doesNotMatch(
        document.element("west_hearts_cards").innerHTML,
        /hand-card-tricks/
    );
    assert.ok(playCalls >= 1);
    await new Promise((resolve) => setTimeout(resolve, 0));
});

test("updateActionButtons refreshes the table state for incomplete deals", async () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
    const cell = document.element("result-table").rows[1].cells[1];
    cell.innerHTML = "9";
    document.element("result").innerHTML = "stale";

    // Act
    ctx.refreshDdTable();

    // Assert
    assert.equal(cell.innerHTML, "");
    assert.equal(document.element("result").innerHTML, "");
});

test("formatSolveTimeMs rounds wall time to whole milliseconds", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.formatSolveTimeMs(0), "Solved in 0 ms.");
    assert.equal(ctx.formatSolveTimeMs(0.4), "Solved in 0 ms.");
    assert.equal(ctx.formatSolveTimeMs(0.5), "Solved in 1 ms.");
    assert.equal(ctx.formatSolveTimeMs(12.3), "Solved in 12 ms.");
    assert.equal(ctx.formatSolveTimeMs(41.9), "Solved in 42 ms.");
});

test("refreshDdTable shows Computing under the matrix only after 300 ms, painted before ccall", async () => {
    // Arrange: WASM ccall is sync and blocks timers, so Computing… must be
    // painted before ccall — after the 300 ms grace — or the user never sees it.
    let ccallSawComputing = false;
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    ctx.setDdTableComputingDelayMs(300);
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall() {
            ccallSawComputing = /Computing/i.test(
                document.element("result").innerHTML
            );
            return 1;
        },
        getValue() {
            return 7;
        },
    });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    // fillForm schedules a solve; wait for it so it does not race the Act call.
    await new Promise((resolve) => setTimeout(resolve, 350));
    ccallSawComputing = false;
    // Force a fresh solve (same PBN would otherwise short-circuit as cached).
    document.element("result-table").rows[1].cells[1].innerHTML = "";

    // Act
    const solve = ctx.refreshDdTable();
    assert.equal(document.element("result").innerHTML, "");

    await new Promise((resolve) => setTimeout(resolve, 50));
    assert.equal(
        document.element("result").innerHTML,
        "",
        "Computing… must wait for the 300 ms grace"
    );

    await solve;

    // Assert
    assert.equal(ccallSawComputing, true);
    assert.match(document.element("result").innerHTML, /^Solved in \d+ ms\.$/);
});

test("refreshDdTable shows wall solve time in ms after a successful solve", async () => {
    // Arrange: full part-score deal; mock WASM and a clock that advances 12.4 ms.
    let clock = 1000;
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        performance: {
            now() {
                return clock;
            },
        },
    });
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall() {
            clock += 12.4;
            return 1;
        },
        getValue() {
            return 7;
        },
    });
    ctx.fillFormWithPartScoreTestData();
    await new Promise((resolve) => setTimeout(resolve, 0));
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Act
    await ctx.refreshDdTable();

    // Assert
    assert.equal(document.element("result").innerHTML, "Solved in 12 ms.");
    assert.equal(
        String(document.element("result-table").rows[1].cells[1].innerHTML),
        "7"
    );
});

test("updateActionButtons displays all 52 cards in the deck status", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);

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
    // Same pip chrome as dealt holdings: suit glyph, then hand-card buttons.
    assert.match(
        deckStatus,
        /<heart-suit>\u2665<\/heart-suit><button\b[^>]*class="hand-card"[^>]*data-card="HA"/
    );
    assert.doesNotMatch(deckStatus, /deck-card/);
    assert.equal((deckStatus.match(/class="hand-card"/g) ?? []).length, 52);
});

test("updateActionButtons omits cards entered in any hand, including lowercase pips", () => {
    const document = createMockDocument({
        north_spades: "A",
        east_hearts: "k",
    });
    const ctx = loadDdsWeb(document);

    ctx.updateActionButtons();

    assert.equal(document.element("east_hearts").value, "K");
    const deckStatus = document.element("deck-status").innerHTML;
    assert.equal((deckStatus.match(/data-card=/g) ?? []).length, 50);
    assert.doesNotMatch(deckStatus, /data-card="SA"/);
    assert.doesNotMatch(deckStatus, /data-card="HK"/);
    assert.match(deckStatus, /class="hand-card"[^>]*data-card="SK"/);
    assert.doesNotMatch(deckStatus, /deck-card-entered/);
    assert.doesNotMatch(deckStatus, /deck-card/);
});

test("updateActionButtons hides a suit row when all of its cards are in the diagram", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765432",
    });
    const ctx = loadDdsWeb(document);

    ctx.updateActionButtons();

    const deckStatus = document.element("deck-status").innerHTML;
    assert.doesNotMatch(deckStatus, /<spade-suit>/);
    assert.match(deckStatus, /<heart-suit>/);
    assert.equal((deckStatus.match(/data-card=/g) ?? []).length, 39);
});

test("updateActionButtons shows a card-count note for a partial hand", () => {
    const document = createMockDocument({
        north_spades: "AKQ",
    });
    const ctx = loadDdsWeb(document);

    ctx.updateActionButtons();

    const note = document.element("north-card-count");
    assert.equal(note.hidden, false);
    assert.equal(note.innerHTML, "3 cards");
    assert.equal(document.element("east-card-count").hidden, true);
});

test("updateActionButtons uses singular card for a one-card hand", () => {
    const document = createMockDocument({
        north_spades: "A",
    });
    const ctx = loadDdsWeb(document);

    ctx.updateActionButtons();

    const note = document.element("north-card-count");
    assert.equal(note.hidden, false);
    assert.equal(note.innerHTML, "1 card");
});

test("updateActionButtons hides the card-count note at 0 and 13 cards", () => {
    const document = createMockDocument({
        north_spades: "AKQ",
        east_hearts: "AKQJT98765432",
    });
    const ctx = loadDdsWeb(document);

    ctx.updateActionButtons();

    assert.equal(document.element("north-card-count").hidden, false);
    assert.equal(document.element("east-card-count").hidden, true);
    assert.equal(document.element("east-card-count").innerHTML, "");
    assert.equal(document.element("south-card-count").hidden, true);
    assert.equal(document.element("south-card-count").innerHTML, "");

    document.setValue("north_spades", "");
    ctx.updateActionButtons();

    const note = document.element("north-card-count");
    assert.equal(note.hidden, true);
    assert.equal(note.innerHTML, "");
});

test("handCardHtml renders a clickable button for a card in a hand", () => {
    // Arrange
    const ctx = loadDdsWeb(createMockDocument());
    const card = new ctx.Card("spades", "A");

    // Act
    const html = ctx.handCardHtml("north", card, 0);

    // Assert
    assert.match(html, /<button\b/);
    assert.match(html, /type="button"/);
    assert.match(html, /class="hand-card"/);
    assert.match(html, /draggable="true"/);
    assert.match(html, /data-direction="north"/);
    assert.match(html, /data-card="SA"/);
    assert.match(html, /data-index="0"/);
    assert.match(html, />A<\/button>/);
    assert.match(html, /aria-label="North spade ace"/);
});

test("escapeHtml encodes characters unsafe in HTML text and attributes", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.escapeHtml("&"), "&amp;");
    assert.equal(ctx.escapeHtml("<"), "&lt;");
    assert.equal(ctx.escapeHtml(">"), "&gt;");
    assert.equal(ctx.escapeHtml("\""), "&quot;");
    assert.equal(ctx.escapeHtml("'"), "&#39;");
    assert.equal(ctx.escapeHtml("A&B<C>\"D'"), "A&amp;B&lt;C&gt;&quot;D&#39;");
    assert.equal(ctx.escapeHtml(7), "7");
    assert.equal(ctx.escapeHtml(null), "");
    assert.equal(ctx.escapeHtml(undefined), "");
});

test("hand holdings appear high to low after sanitize", () => {
    // Arrange: typed order may be arbitrary; display follows PIPS rank.
    const document = createMockDocument({ north_spades: "QA" });
    const ctx = loadDdsWeb(document);

    // Act
    ctx.updateActionButtons();
    const html = document.element("north_spades_cards").innerHTML;

    // Assert
    assert.equal(document.element("north_spades").value, "AQ");
    const a = html.indexOf('data-card="SA"');
    const q = html.indexOf('data-card="SQ"');
    assert.ok(a >= 0 && q >= 0);
    assert.ok(a < q, "A before Q matches high-to-low order");
});

test("handHoldingHtml inserts a caret marker at the given index", () => {
    // Arrange
    const document = createMockDocument({ north_spades: "AK" });
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);

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
    const ctx = loadDdsWeb(document);

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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(createMockDocument());
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(createMockDocument());

    // Assert
    assert.equal(ctx.openingLeader("south"), "west");
    assert.equal(ctx.openingLeader("west"), "north");
    assert.equal(ctx.openingLeader("north"), "east");
    assert.equal(ctx.openingLeader("east"), "south");
});

test("pipFromDdsRank maps DDS ranks 2-14 onto pips", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.pipFromDdsRank(14), "A");
    assert.equal(ctx.pipFromDdsRank(13), "K");
    assert.equal(ctx.pipFromDdsRank(12), "Q");
    assert.equal(ctx.pipFromDdsRank(11), "J");
    assert.equal(ctx.pipFromDdsRank(10), "T");
    assert.equal(ctx.pipFromDdsRank(9), "9");
    assert.equal(ctx.pipFromDdsRank(2), "2");
});

test("leadTricksMapFromSolverOutput expands suit/rank/score triples to card keys", () => {
    // Arrange: flat buffer from dds_web_solve_leads
    const ctx = loadDdsWeb(createMockDocument());
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

test("solveOpeningLeadTricks rejects when WASM reports more than 13 leads", async () => {
    // Arrange: corrupted out_leads[0] past the fixed 13-card buffer.
    const ctx = loadDdsWeb(createMockDocument());
    const heap = new Int32Array(1 + 13 * 3);
    heap[0] = 14;
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall: () => 1,
        getValue(ptr) {
            return heap[(ptr / 4) | 0] ?? 0;
        },
    });
    const hands = {
        north: [],
        east: [],
        south: [],
        west: [],
    };

    // Act / Assert: must not walk past the malloc'd buffer.
    await assert.rejects(
        () => ctx.solveOpeningLeadTricks(hands, {
            direction: "south",
            denomination: "N",
        }),
        /invalid card count \(14\)/
    );
});

test("solveOpeningLeadTricks rejects a negative lead count from WASM", async () => {
    const ctx = loadDdsWeb(createMockDocument());
    const heap = new Int32Array(1);
    heap[0] = -1;
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall: () => 1,
        getValue(ptr) {
            return heap[(ptr / 4) | 0] ?? 0;
        },
    });

    await assert.rejects(
        () => ctx.solveOpeningLeadTricks(
            { north: [], east: [], south: [], west: [] },
            { direction: "south", denomination: "N" }
        ),
        /invalid card count \(-1\)/
    );
});

test("readExpandedLeads error does not say lead solve for play path", async () => {
    // Arrange: shared reader used by solvePlayPosition; message must be generic.
    const ctx = loadDdsWeb(createMockDocument());
    const heap = new Int32Array(1);
    heap[0] = 14;
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall: () => 1,
        getValue(ptr) {
            return heap[(ptr / 4) | 0] ?? 0;
        },
    });
    const state = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });

    // Act / Assert
    await assert.rejects(
        () => ctx.solvePlayPosition(state),
        (err) => {
            assert.match(String(err.message), /DDS solve returned invalid card count/);
            assert.doesNotMatch(String(err.message), /lead solve/);
            return true;
        }
    );
});

test("solveOpeningLeadTricks accepts a full 13-lead buffer from WASM", async () => {
    const ctx = loadDdsWeb(createMockDocument());
    const heap = new Int32Array(1 + 13 * 3);
    heap[0] = 13;
    for (let i = 0; i < 13; i++) {
        heap[1 + 3 * i] = 0; // spades
        heap[1 + 3 * i + 1] = 14 - i; // A..2
        heap[1 + 3 * i + 2] = i;
    }
    let getValueCalls = 0;
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall: () => 1,
        getValue(ptr) {
            getValueCalls += 1;
            return heap[(ptr / 4) | 0] ?? 0;
        },
    });

    const map = await ctx.solveOpeningLeadTricks(
        { north: [], east: [], south: [], west: [] },
        { direction: "south", denomination: "N" }
    );

    assert.equal(map.SA, 0);
    assert.equal(map.S2, 12);
    // 1 count + 13 * 3 fields
    assert.equal(getValueCalls, 1 + 13 * 3);
});

test("handCardHtml renders a lower-right tricks numeral when provided", () => {
    // Arrange
    const ctx = loadDdsWeb(createMockDocument());
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
    const ctx = loadDdsWeb(createMockDocument());
    const html = ctx.handCardHtml("west", new ctx.Card("spades", "K"), 0);

    assert.doesNotMatch(html, /hand-card-tricks/);
    assert.doesNotMatch(html, /hand-card-with-tricks/);
});

test("handHoldingHtml badges only cards present in the lead-tricks map", () => {
    // Arrange
    const document = createMockDocument({ west_spades: "KQ" });
    const ctx = loadDdsWeb(document);
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
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");
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
    const ctx = loadDdsWeb(createMockDocument());

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
    const ctx = loadDdsWeb(createMockDocument());

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
    const ctx = loadDdsWeb(createMockDocument());
    const html = ctx.contractStatusHtml({ direction: "south", denomination: "N" });

    assert.match(html, /class="contract-status-denom"[^>]*>NT</);
    assert.match(html, /class="contract-status-by"[^>]*>by</);
    assert.match(html, /class="contract-status-declarer"[^>]*>S</);
    assert.match(html, /aria-label="Notrump; South declares"/);
});

test("updateContractStatus hides the NE panel when no contract is selected", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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

test("switching contracts restarts play and clears prior play badges", async () => {
    // Arrange: first contract has badges; switch while a follow-up solve is slow.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.refreshDdTable = async () => {};

    let playCalls = 0;
    let resolveSecondSolve;
    const targetsSeen = [];
    ctx.solvePlayPosition = (state) => {
        playCalls += 1;
        targetsSeen.push(state.targetTricks);
        if (playCalls === 1) {
            return Promise.resolve({ SK: 0, HA: -1 });
        }
        return new Promise((resolve) => {
            resolveSecondSolve = resolve;
        });
    };

    ctx.fillFormWithPartScoreTestData();
    const southNt = document.element("result-table").rows[3].cells[5];
    const northClubs = document.element("result-table").rows[1].cells[1];
    southNt.innerHTML = "6";
    southNt.textContent = "6";
    northClubs.innerHTML = "5";
    northClubs.textContent = "5";

    ctx.handleResultTableClick({
        target: {
            closest() {
                return southNt;
            },
        },
    });
    await new Promise((resolve) => setTimeout(resolve, 40));
    assert.match(
        document.element("west_spades_cards").innerHTML,
        /hand-card-tricks/
    );

    // Act: select a different contract.
    ctx.handleResultTableClick({
        target: {
            closest() {
                return northClubs;
            },
        },
    });

    // Assert: selection moved; prior badges cleared until the new solve settles.
    assert.equal(ctx.selectedContract().direction, "north");
    assert.equal(ctx.selectedContract().denomination, "C");
    assert.equal(ctx.playState.targetTricks, 5);
    assert.doesNotMatch(
        document.element("west_spades_cards").innerHTML,
        /hand-card-tricks/
    );

    if (typeof resolveSecondSolve === "function") {
        resolveSecondSolve({ CK: 1 });
    }
    await new Promise((resolve) => setTimeout(resolve, 0));
    assert.ok(targetsSeen.length >= 1);
    assert.equal(targetsSeen[0], 6);
});

test("handleResultTableClick clears selection when the selected cell is clicked again", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const table = document.element("result-table");
    const cell = table.rows[2].cells[3]; // East / Hearts
    const selections = [];
    ctx.onContractSelect = (direction, denomination) => {
        selections.push({ direction, denomination });
    };

    // Act: select, then click the same cell again.
    ctx.handleResultTableClick({
        target: {
            closest() {
                return cell;
            },
        },
    });
    ctx.handleResultTableClick({
        target: {
            closest() {
                return cell;
            },
        },
    });

    // Assert
    assert.equal(ctx.selectedContract(), null);
    assert.equal(cell.classList.contains("result-cell-selected"), false);
    assert.deepEqual(selections, [
        { direction: "east", denomination: "H" },
        { direction: null, denomination: null },
    ]);

    const status = document.element("contract-status");
    assert.equal(status.hidden, true);
    assert.equal(status.innerHTML, "");
});

test("clearing contract ignores an in-flight opening-lead solve", async () => {
    // Arrange: select a contract, start a lead solve, then clear without the
    // deal-solve queue cleaning up afterward (so only request invalidation
    // protects against a late write).
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const cell = document.element("result-table").rows[3].cells[5]; // South / NT
    let resolveSolve;
    let tricksPassedToHolding = null;

    ctx.fillFormWithPartScoreTestData();
    ctx.scheduleDealSolve = () => Promise.resolve();
    ctx.handleResultTableClick({
        target: {
            closest() {
                return cell;
            },
        },
    });
    assert.equal(ctx.selectedContract().direction, "south");

    ctx.solveOpeningLeadTricks = () => new Promise((resolve) => {
        resolveSolve = resolve;
    });
    const inflight = ctx.refreshOpeningLeadTricks();

    // Act: clear selection while the lead solve is still pending.
    ctx.handleResultTableClick({
        target: {
            closest() {
                return cell;
            },
        },
    });
    assert.equal(ctx.selectedContract(), null);

    // Watch handHoldingHtml after clear for a late leadTricksByCardKey write.
    const origHolding = ctx.handHoldingHtml;
    ctx.handHoldingHtml = (direction, suit, cards, caretIndex, tricksByKey) => {
        if (tricksByKey) {
            tricksPassedToHolding = { ...tricksByKey };
        }
        return origHolding(direction, suit, cards, caretIndex, tricksByKey);
    };

    resolveSolve({ SK: 7, HA: 5 });
    await inflight;
    await new Promise((resolve) => setTimeout(resolve, 0));

    // Assert: late completion must not feed tricks into the hand diagram.
    assert.equal(tricksPassedToHolding, null);
});

test("handleResultTableClick ignores clicks outside a result data cell", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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

test("pageLoad makes result data cells keyboard-operable buttons", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const table = document.element("result-table");

    // Act
    ctx.pageLoad();

    // Assert: every data cell is a focusable button with a contract label.
    const northClubs = table.rows[1].cells[1];
    assert.equal(northClubs.tabIndex, 0);
    assert.equal(northClubs.getAttribute("role"), "button");
    assert.equal(
        northClubs.getAttribute("aria-label"),
        "Clubs; North declares"
    );

    const southNt = table.rows[3].cells[5];
    assert.equal(southNt.tabIndex, 0);
    assert.equal(southNt.getAttribute("role"), "button");
    assert.equal(
        southNt.getAttribute("aria-label"),
        "Notrump; South declares"
    );

    // Header cells stay non-interactive.
    assert.notEqual(table.rows[0].cells[1].tabIndex, 0);
    assert.equal(table.rows[0].cells[1].getAttribute("role"), null);
    assert.notEqual(table.rows[1].cells[0].tabIndex, 0);
});

test("handleResultTableKeyDown selects a contract on Enter", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const cell = document.element("result-table").rows[2].cells[3]; // East / Hearts
    cell.closest = (selector) =>
        selector === "#result-table td" ? cell : null;
    const selections = [];
    ctx.onContractSelect = (direction, denomination) => {
        selections.push({ direction, denomination });
    };

    // Act
    ctx.handleResultTableKeyDown({
        key: "Enter",
        target: cell,
        preventDefault() {
            assert.fail("Enter should not call preventDefault");
        },
    });

    // Assert
    assert.deepEqual(selections, [{ direction: "east", denomination: "H" }]);
    assert.equal(ctx.selectedContract().direction, "east");
    assert.equal(ctx.selectedContract().denomination, "H");
});

test("handleResultTableKeyDown selects a contract on Space and prevents scroll", () => {
    // Arrange
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const cell = document.element("result-table").rows[4].cells[5]; // West / NT
    cell.closest = (selector) =>
        selector === "#result-table td" ? cell : null;
    let prevented = false;

    // Act
    ctx.handleResultTableKeyDown({
        key: " ",
        target: cell,
        preventDefault() {
            prevented = true;
        },
    });

    // Assert
    assert.equal(prevented, true);
    assert.equal(ctx.selectedContract().direction, "west");
    assert.equal(ctx.selectedContract().denomination, "N");
});

test("handleResultTableKeyDown ignores keys outside result cells", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    let prevented = false;

    ctx.handleResultTableKeyDown({
        key: "Enter",
        target: {
            closest() {
                return null;
            },
        },
        preventDefault() {
            prevented = true;
        },
    });
    ctx.handleResultTableKeyDown({
        key: "Tab",
        target: document.element("result-table").rows[1].cells[1],
        preventDefault() {
            prevented = true;
        },
    });

    assert.equal(prevented, false);
    assert.equal(ctx.selectedContract(), null);
});

test("pageLoad wires result-table keyboard activation", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const cell = document.element("result-table").rows[3].cells[4];
    cell.closest = (selector) =>
        selector === "#result-table td" ? cell : null;

    ctx.pageLoad();
    document.dispatch("keydown", {
        key: "Enter",
        target: cell,
        preventDefault() {},
    });

    assert.equal(ctx.selectedContract().direction, "south");
    assert.equal(ctx.selectedContract().denomination, "S");
});

test("result-cell-selected highlight is defined in CSS", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    // Assert
    assert.match(css, /#result-table\s+td\.result-cell-selected\s*\{/s);
    assert.match(css, /#result-table\s+td\s*\{[^}]*cursor:\s*pointer/s);
    assert.match(css, /#result-table\s+td:focus-visible\s*\{/s);
});

test("sample-deals select uses the same background as native buttons", () => {
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    assert.match(
        css,
        /body\s*>\s*button\[type="button"\],\s*#sample-deals\s*\{[^}]*background(?:-color)?:\s*#efefef/is
    );
});

test("sample-deals select matches native toolbar button height chrome", () => {
    // Shared toolbar rule sizes buttons and the Sample deals select together.
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    assert.match(
        css,
        /body\s*>\s*button\[type="button"\],\s*#sample-deals\s*\{[^}]*box-sizing:\s*border-box/is
    );
    assert.match(
        css,
        /body\s*>\s*button\[type="button"\],\s*#sample-deals\s*\{[^}]*height:\s*20px/is
    );
});

test("sample-deals select uses the same border as native buttons", () => {
    // Flat 1px solid border (no outset shadow), matching bridge-solver toolbar.
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    assert.match(
        css,
        /body\s*>\s*button\[type="button"\],\s*#sample-deals\s*\{[^}]*border:\s*1px\s+solid\s+#767676/is
    );
});

test("sample-deals select uses the same rectangular shape as native buttons", () => {
    // appearance:none + shared radius; only the select keeps a dropdown chevron.
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    assert.match(
        css,
        /body\s*>\s*button\[type="button"\],\s*#sample-deals\s*\{[^}]*appearance:\s*none/is
    );
    assert.match(
        css,
        /body\s*>\s*button\[type="button"\],\s*#sample-deals\s*\{[^}]*border-radius:\s*2px/is
    );
    assert.match(css, /#sample-deals\s*\{[^}]*background-image:\s*url\(/is);
});

test("dds_web html does not cache-bust css or js with query params", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const html = readFileSync(join(here, "..", "dds_web.html"), "utf8");

    // Assert: plain asset URLs; deploy/HTTP caching is enough for DDS Web.
    assert.match(html, /href="dds_web\.css"/);
    assert.match(html, /src="dds_web\.js"/);
    assert.doesNotMatch(html, /dds_web\.(css|js)\?/);
});

test("undeployed cards live in the hand diagram center in four suit rows", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const html = readFileSync(join(here, "..", "dds_web.html"), "utf8");
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    // Assert: deck-status is the direct child of the center filler (not
    // below the diagram). Match opening tags only — a non-greedy </div>
    // stops at #deck-status's closer and misses nested structure.
    assert.match(
        html,
        /<div class="[^"]*grid-filler-center[^"]*"[^>]*>\s*<div id="deck-status"/
    );
    assert.doesNotMatch(
        html,
        /<div class="[^"]*grid-filler-center[^"]*"[^>]*aria-hidden="true"/
    );
    assert.equal(
        (html.match(/id="deck-status"/g) || []).length,
        1,
        "deck-status appears once"
    );
    assert.match(
        html,
        /id="deck-status"[^>]*role="group"[^>]*aria-label="Cards not yet entered in the diagram"|id="deck-status"[^>]*aria-label="Cards not yet entered in the diagram"[^>]*role="group"/
    );

    assert.match(
        css,
        /#deck-status\s*\{[^}]*flex-direction:\s*column/s
    );
    assert.match(css, /\.deck-suit-row\s*\{/s);
    assert.match(css, /\.grid-item\.grid-filler-center\s*\{/s);
    // Same middle column as N/S: left-align with their suit symbols, do not
    // horizontally center the undeployed strip inside the cell.
    assert.match(
        css,
        /\.grid-item\.grid-filler-center\s*\{[^}]*justify-content:\s*flex-start/s
    );
    assert.doesNotMatch(
        css,
        /\.grid-item\.grid-filler-center\s*\{[^}]*justify-content:\s*center/s
    );
    // Match dealt-hand type size so .hand-card 0.72em chrome matches N/S.
    assert.match(
        css,
        /\.grid-item\.grid-filler-center\s*\{[^}]*font-size:\s*30px/s
    );
    assert.doesNotMatch(css, /\.deck-card\s*\{/);
});

test("undeployedCardHtml matches dealt hand-card button chrome", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const card = new ctx.Card("hearts", "A");

    const html = ctx.undeployedCardHtml(card);

    assert.match(html, /<button\b/);
    assert.match(html, /type="button"/);
    assert.match(html, /class="hand-card"/);
    assert.match(html, /draggable="true"/);
    assert.match(html, /data-card="HA"/);
    assert.match(html, />A<\/button>/);
    assert.doesNotMatch(html, /data-direction=/);
    assert.doesNotMatch(html, /deck-card/);
    // Buttons must not be aria-hidden while remaining focusable (tabindex=-1).
    assert.doesNotMatch(html, /aria-hidden=/);
    assert.match(html, /tabindex="-1"/);
    // Suit+pip name like dealt cards; "Undeployed" replaces seat so identical
    // pips across suits are distinguishable to assistive tech.
    assert.match(html, /aria-label="Undeployed heart ace"/);
    assert.equal(ctx.handCardAriaLabel("undeployed", card), "Undeployed heart ace");
});

test("Card.fromKey accepts a two-character key and normalizes case", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const card = ctx.Card.fromKey("sa");

    assert.equal(card.suit, "spades");
    assert.equal(card.pip, "A");
    assert.equal(card.key(), "SA");
});

test("Card.fromKey rejects malformed keys", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.throws(() => ctx.Card.fromKey("SAX"));
    assert.throws(() => ctx.Card.fromKey("S"));
    assert.throws(() => ctx.Card.fromKey(""));
    assert.throws(() => ctx.Card.fromKey(null));
    assert.throws(() => ctx.Card.fromKey(undefined));
    assert.throws(() => ctx.Card.fromKey(42));
});

test("Card rejects invalid suit or pip", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.throws(() => new ctx.Card("not-a-suit", "A"));
    assert.throws(() => new ctx.Card("spades", "X"));
    assert.throws(() => new ctx.Card("spades", "\"&<>'"));
    assert.throws(() => ctx.Card.fromKey("XX"));
});

test("addCardToHand inserts a pip in high-to-low order", () => {
    const document = createMockDocument({ north_spades: "AK" });
    const ctx = loadDdsWeb(document);

    // Out-of-order inserts must still yield sorted high→low; appending would not.
    for (const pip of ["5", "2", "Q", "T", "7"]) {
        assert.equal(ctx.addCardToHand("north", new ctx.Card("spades", pip)), true);
    }
    assert.equal(document.element("north_spades").value, "AKQT752");
});

test("addCardToHand inserts between existing ranks", () => {
    const document = createMockDocument({ north_spades: "AQ" });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.addCardToHand("north", new ctx.Card("spades", "K")), true);
    assert.equal(document.element("north_spades").value, "AKQ");
});

test("addCardToHand rejects a card already present in any hand", () => {
    const document = createMockDocument({
        north_spades: "A",
        east_hearts: "K",
    });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.addCardToHand("south", new ctx.Card("spades", "A")), false);
    assert.equal(document.element("south_spades").value, "");
    assert.equal(document.element("north_spades").value, "A");

    assert.equal(ctx.addCardToHand("west", new ctx.Card("hearts", "K")), false);
    assert.equal(document.element("west_hearts").value, "");
    assert.equal(document.element("east_hearts").value, "K");
});

test("handCardCount sums suit inputs without rebuilding every hand", () => {
    const document = createMockDocument({
        north_spades: "AKQ",
        north_hearts: "JT9",
        north_diamonds: "87",
        north_clubs: "65432",
        east_spades: "T",
    });
    const ctx = loadDdsWeb(document);
    let collectCalls = 0;
    const originalCollect = ctx.collectHands;
    ctx.collectHands = (...args) => {
        collectCalls += 1;
        return originalCollect(...args);
    };

    assert.equal(ctx.handCardCount("north"), 13);
    assert.equal(ctx.handCardCount("east"), 1);
    assert.equal(ctx.handCardCount("south"), 0);
    assert.equal(collectCalls, 0);
});

test("removeCardFromHand deletes a matching pip and leaves remaining ranks ordered", () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.removeCardFromHand("north", new ctx.Card("spades", "A")), true);
    assert.equal(document.element("north_spades").value, "KQ");
    assert.equal(ctx.removeCardFromHand("north", new ctx.Card("spades", "A")), false);
    assert.equal(document.element("north_spades").value, "KQ");
});

test("undeployCard removes the card from every hand", () => {
    const document = createMockDocument({
        north_spades: "A",
        east_hearts: "K",
    });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.undeployCard(new ctx.Card("spades", "A")), true);
    assert.equal(document.element("north_spades").value, "");
    assert.equal(document.element("east_hearts").value, "K");
});

test("moveCardToHand moves from undeployed into a hand in rank order", () => {
    const document = createMockDocument({ north_spades: "AQ" });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.moveCardToHand(new ctx.Card("spades", "K"), "north"), true);
    assert.equal(document.element("north_spades").value, "AKQ");
});

test("moveCardToHand moves a card between hands", () => {
    const document = createMockDocument({ north_spades: "AQ", east_spades: "K" });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.moveCardToHand(new ctx.Card("spades", "A"), "east"), true);
    assert.equal(document.element("north_spades").value, "Q");
    assert.equal(document.element("east_spades").value, "AK");
});

test("moveCardToHand is a no-op when the card is already in the target hand", () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.moveCardToHand(new ctx.Card("spades", "A"), "north"), true);
    assert.equal(document.element("north_spades").value, "AKQ");
});

test("moveCardToHand rejects a drop onto a full 13-card hand from outside", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765432",
        east_hearts: "A",
    });
    const ctx = loadDdsWeb(document);

    assert.equal(ctx.moveCardToHand(new ctx.Card("hearts", "A"), "north"), false);
    assert.equal(document.element("north_spades").value, "AKQJT98765432");
    assert.equal(document.element("north_hearts").value, "");
    assert.equal(document.element("east_hearts").value, "A");
});

test("handleHandCardMouseDown does not preventDefault so HTML5 drag can start", () => {
    const ctx = loadDdsWeb(createMockDocument());
    let prevented = false;

    ctx.handleHandCardMouseDown({
        target: {
            closest(selector) {
                return selector === ".hand-card" ? {} : null;
            },
        },
        preventDefault() {
            prevented = true;
        },
    });

    assert.equal(prevented, false);
});

test("handleCardDragStart sets the card payload and dragging class", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const button = {
        className: "hand-card",
        classList: {
            add(name) {
                button.className += " " + name;
            },
            remove() {},
            contains() {
                return false;
            },
        },
        getAttribute(name) {
            if (name === "data-card") {
                return "SA";
            }
            if (name === "data-direction") {
                return "north";
            }
            return null;
        },
    };
    let stored = null;
    const event = {
        target: {
            closest(selector) {
                return selector === ".hand-card" ? button : null;
            },
        },
        dataTransfer: {
            setData(type, value) {
                stored = { type, value };
            },
            effectAllowed: "none",
        },
    };

    ctx.handleCardDragStart(event);

    assert.equal(stored.type, "application/x-dds-card");
    assert.deepEqual(JSON.parse(stored.value), {
        key: "SA",
        sourceDirection: "north",
    });
    assert.equal(event.dataTransfer.effectAllowed, "move");
    assert.match(button.className, /hand-card-dragging/);
});

test("handleCardDragOver accepts a drop on a hand that is not full", () => {
    const document = createMockDocument({ north_spades: "A" });
    const ctx = loadDdsWeb(document);
    let prevented = false;
    let dropEffect = "none";
    const hand = {
        classList: {
            add() {},
            remove() {},
            contains() {
                return false;
            },
        },
        className: "grid-item hand-north",
    };
    const sourceButton = {
        className: "hand-card",
        classList: {
            add() {},
            remove() {},
            contains() {
                return false;
            },
        },
        getAttribute(name) {
            if (name === "data-card") {
                return "SK";
            }
            return null;
        },
    };

    ctx.handleCardDragStart({
        target: {
            closest(selector) {
                return selector === ".hand-card" ? sourceButton : null;
            },
        },
        dataTransfer: {
            setData() {},
            effectAllowed: "none",
        },
    });

    ctx.handleCardDragOver({
        preventDefault() {
            prevented = true;
        },
        dataTransfer: {
            types: ["application/x-dds-card"],
            getData() {
                return "";
            },
            set dropEffect(value) {
                dropEffect = value;
            },
            get dropEffect() {
                return dropEffect;
            },
        },
        target: {
            closest(selector) {
                if (selector === ".hand-north") {
                    return hand;
                }
                return null;
            },
        },
    });

    assert.equal(prevented, true);
    assert.equal(dropEffect, "move");
});

test("handleCardDrop moves an undeployed card onto a hand in rank order", () => {
    const document = createMockDocument({ north_spades: "AK" });
    const ctx = loadDdsWeb(document);
    const targetCard = {
        getAttribute(name) {
            if (name === "data-card") {
                return "SK";
            }
            if (name === "data-direction") {
                return "north";
            }
            if (name === "data-index") {
                return "1";
            }
            return null;
        },
        getBoundingClientRect() {
            return { left: 100, width: 20 };
        },
    };
    const hand = {
        className: "grid-item hand-north",
        classList: { add() {}, remove() {}, contains() { return false; } },
    };

    ctx.handleCardDrop({
        preventDefault() {},
        clientX: 105,
        dataTransfer: {
            getData(type) {
                assert.equal(type, "application/x-dds-card");
                return JSON.stringify({ key: "SQ", sourceDirection: null });
            },
        },
        target: {
            closest(selector) {
                if (selector === ".hand-card") {
                    return targetCard;
                }
                if (
                    selector === ".hand-north, .hand-east, .hand-south, .hand-west" ||
                    selector === ".hand-north"
                ) {
                    return hand;
                }
                return null;
            },
        },
    });

    // Drop position is ignored; Q sorts between K and … → AKQ
    assert.equal(document.element("north_spades").value, "AKQ");
});

test("handleCardDrop is a no-op when a card is dropped back on its hand", () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsWeb(document);
    const hand = {
        className: "grid-item hand-north",
        classList: { add() {}, remove() {}, contains() { return false; } },
    };

    ctx.handleCardDrop({
        preventDefault() {},
        dataTransfer: {
            getData() {
                return JSON.stringify({ key: "SA", sourceDirection: "north" });
            },
        },
        target: {
            closest(selector) {
                if (selector === ".hand-north") {
                    return hand;
                }
                return null;
            },
        },
    });

    assert.equal(document.element("north_spades").value, "AKQ");
});

test("handleCardDrop undeploys a card dropped on the center", () => {
    const document = createMockDocument({ north_spades: "A" });
    const ctx = loadDdsWeb(document);
    const center = {
        className: "grid-item grid-filler grid-filler-center",
        id: "deck-status",
        classList: { add() {}, remove() {}, contains() { return false; } },
    };

    ctx.handleCardDrop({
        preventDefault() {},
        dataTransfer: {
            getData() {
                return JSON.stringify({ key: "SA", sourceDirection: "north" });
            },
        },
        target: {
            closest(selector) {
                if (
                    selector === "#deck-status, .grid-filler-center" ||
                    selector === "#deck-status" ||
                    selector === ".grid-filler-center"
                ) {
                    return center;
                }
                return null;
            },
        },
    });

    assert.equal(document.element("north_spades").value, "");
});

test("handleCardDrop rejects dropping onto a full hand from outside", () => {
    const document = createMockDocument({
        north_spades: "AKQJT98765432",
        east_hearts: "A",
    });
    const ctx = loadDdsWeb(document);
    const hand = {
        className: "grid-item hand-north",
        classList: { add() {}, remove() {}, contains() { return false; } },
    };

    ctx.handleCardDrop({
        preventDefault() {},
        dataTransfer: {
            getData() {
                return JSON.stringify({ key: "HA", sourceDirection: "east" });
            },
        },
        target: {
            closest(selector) {
                if (
                    selector === ".hand-north, .hand-east, .hand-south, .hand-west" ||
                    selector === ".hand-north"
                ) {
                    return hand;
                }
                return null;
            },
        },
    });

    assert.equal(document.element("north_hearts").value, "");
    assert.equal(document.element("east_hearts").value, "A");
});

test("handleCardDragOver ignores invalid drag payload keys", () => {
    const ctx = loadDdsWeb(createMockDocument());
    let dropEffect = "move";

    assert.doesNotThrow(() => {
        ctx.handleCardDragOver({
            preventDefault() {},
            dataTransfer: {
                getData() {
                    return JSON.stringify({ key: "XX" });
                },
                get dropEffect() {
                    return dropEffect;
                },
                set dropEffect(value) {
                    dropEffect = value;
                },
            },
            target: {
                closest() {
                    return null;
                },
            },
        });
    });

    assert.equal(dropEffect, "none");
});

test("handleCardDrop ignores invalid drag payload keys", () => {
    const document = createMockDocument({ north_spades: "A" });
    const ctx = loadDdsWeb(document);

    assert.doesNotThrow(() => {
        ctx.handleCardDrop({
            preventDefault() {},
            dataTransfer: {
                getData() {
                    return JSON.stringify({ key: "XX" });
                },
            },
            target: {
                closest() {
                    return null;
                },
            },
        });
    });

    assert.equal(document.element("north_spades").value, "A");
});

test("pageLoad wires card drag-and-drop listeners", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const types = [];
    const original = document.addEventListener.bind(document);
    document.addEventListener = (type, listener) => {
        types.push(type);
        original(type, listener);
    };

    ctx.pageLoad();

    for (const type of ["dragstart", "dragover", "drop", "dragend", "dragleave"]) {
        assert.ok(types.includes(type), "pageLoad listens for " + type);
    }
});

test("card drag CSS provides grab cursor and drop-target affordances", () => {
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    assert.match(css, /\.hand-card\[draggable="true"\]\s*\{[^}]*cursor:\s*grab/s);
    assert.match(css, /\.hand-card-dragging\s*\{/s);
    assert.match(css, /\.drop-target-active\s*\{/s);
});

test("result table lives in the hand diagram southeast corner", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const html = readFileSync(join(here, "..", "dds_web.html"), "utf8");
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    // Assert: table is a child of the SE filler cell, not below the diagram.
    const seOpen = html.match(
        /<div class="[^"]*grid-filler-se[^"]*"[^>]*>/
    );
    assert.ok(seOpen, "southeast filler cell present");
    const afterSe = html.slice(html.indexOf(seOpen[0]) + seOpen[0].length);
    // Hint sits above the table inside the SE cell.
    assert.match(
        afterSe,
        /class="[^"]*result-table-hint[^"]*"[^>]*>Click a cell to play out that contract</
    );
    assert.match(
        afterSe,
        /result-table-hint[\s\S]*?id="result-table"/
    );
    assert.match(afterSe, /id="result-table"/);
    // Solve status (Computing… / wall time / errors) sits under the table in
    // the SE cell so it is visible next to the results users are watching.
    assert.match(
        afterSe,
        /id="result-table"[\s\S]*?<p\b[^>]*\bid="result"[^>]*>/
    );
    assert.match(
        afterSe,
        /<p\b[^>]*\bid="result"[^>]*\baria-live="polite"/
    );
    // Only one #result, and it is not left below the diagram.
    assert.equal((html.match(/\bid="result"/g) || []).length, 1);
    assert.doesNotMatch(
        html.slice(html.indexOf("</div>\n    </div>\n    </div>")),
        /id="result"/
    );
    // SE cell must be readable (not aria-hidden) and sized for the table.
    assert.doesNotMatch(seOpen[0], /aria-hidden="true"/);
    assert.match(css, /\.grid-item\.grid-filler-se\s*\{[^}]*font-size:/s);
    assert.match(css, /\.grid-item\.grid-filler-se\s*\{[^}]*flex-direction:\s*column/s);
    assert.match(css, /\.result-table-hint\s*\{/s);
    assert.match(css, /#result\s*\{/s);
});

test("contract status lives in the hand diagram northeast corner", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const html = readFileSync(join(here, "..", "dds_web.html"), "utf8");
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

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
    const htmlPath = join(here, "..", "dds_web.html");
    const cssPath = join(here, "..", "dds_web.css");

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
    // Concealed inputs still need accessible names for keyboard/AT users.
    assert.match(
        html,
        /id="north_spades"[^>]*aria-label="North spades"/
    );
    assert.match(
        html,
        /id="west_clubs"[^>]*aria-label="West clubs"/
    );
    assert.equal((html.match(/class="hand-suit-input"[^>]*aria-label="/g) ?? []).length, 16);
    assert.match(css, /\.hand-suit-input\s*\{[^}]*color:\s*transparent/s);
    assert.match(css, /\.hand-suit-input\s*\{[^}]*caret-color:/s);
});

test("hand seats left-align suit symbols in the diagram", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

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
    // Equal fractional columns; min-width:0 stops longer holdings from
    // expanding a column (layout jump).
    assert.match(
        css,
        /\.grid-container\s*\{[^}]*grid-template-columns:\s*1\.5fr\s+1\.5fr\s+1\.5fr/s
    );
    assert.match(css, /\.grid-item\s*\{[^}]*min-width:\s*0/s);
    assert.match(css, /\.grid-outer\s*\{[^}]*max-width:\s*1100px/s);
});

test("suit rows pin glyphs so card columns left-align vertically", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");

    // Assert: fixed-width suit glyph column for hands and center undeployed.
    assert.match(css, /\.hand-suit\s*\{[^}]*display:\s*inline-flex/s);
    assert.match(css, /\.deck-suit-row\s*\{[^}]*display:\s*flex/s);
    // Tight glyph-to-card gap shared by hands and center.
    assert.match(
        css,
        /\.hand-suit,\s*\n\s*\.deck-suit-row\s*\{[^}]*gap:\s*0(?:em|px)?/s
    );
    assert.match(
        css,
        /\.hand-suit\s*>\s*:is\(spade-suit,\s*heart-suit,\s*diamond-suit,\s*club-suit\)\s*,\s*\n\s*\.deck-suit-row\s*>\s*:is\(spade-suit,\s*heart-suit,\s*diamond-suit,\s*club-suit\)\s*\{[^}]*flex:\s*0\s+0\s+1em/s
    );
});

test("hand card-count note indents to the suit pip column", () => {
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");
    const count = css.match(/\.hand-card-count\s*\{([^}]*)\}/s);

    assert.ok(count, ".hand-card-count rule present");
    // Count uses font-size 1rem, so em would be wrong; match the seat's
    // 30px glyph column (.grid-item font-size × 1em suit width).
    assert.match(count[1], /padding-left:\s*30px/);
});

test("center undeployed pips share dealt hand-card spacing", () => {
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");
    const dealt = css.match(/\.hand-card\s*\{([^}]*)\}/s);

    assert.ok(dealt, ".hand-card rule present");
    assert.match(dealt[1], /margin:\s*0\s+0\.04em\s+0\s+0/);
    assert.match(dealt[1], /padding:\s*0\s+0\.28em\s+0\.18em\s+0\.02em/);
    // No center-only spacing override — undeployed cards match dealt pips.
    assert.doesNotMatch(css, /#deck-status\s+\.hand-card\s*\{[^}]*margin/s);
    assert.doesNotMatch(css, /#deck-status\s+\.hand-card\s*\{[^}]*padding/s);
});

function mockCenterDeckLayout({
    clientWidth,
    paddingLeft = "20px",
    paddingRight = "0.25rem",
    rowScrollWidths,
    hidden = false,
    // Extra fixed px that does not shrink with font-size (simulates 1px borders).
    fixedOverflowPx = 0,
    initialValues = {},
}) {
    let fontPx = 30;
    const baseWidths = rowScrollWidths.slice();
    const deck = {
        id: "deck-status",
        hidden,
        style: {
            maxWidth: "",
            overflowX: "",
            get fontSize() {
                return fontPx === 30 ? "" : fontPx + "px";
            },
            set fontSize(value) {
                if (value === "" || value == null) {
                    fontPx = 30;
                    return;
                }
                const parsed = parseFloat(value);
                fontPx = Number.isFinite(parsed) ? parsed : 30;
            },
        },
        querySelectorAll(selector) {
            if (selector !== ".deck-suit-row") {
                return [];
            }
            const scale = fontPx / 30;
            return baseWidths.map((scrollWidth) => ({
                scrollWidth: scrollWidth * scale + fixedOverflowPx,
            }));
        },
    };
    const center = {
        className: "grid-item grid-filler grid-filler-center",
        clientWidth,
    };
    const document = createMockDocument(initialValues);
    const realGet = document.getElementById.bind(document);
    document.getElementById = (id) => {
        if (id === "deck-status") {
            return deck;
        }
        return realGet(id);
    };
    document.querySelector = (selector) => {
        if (selector === ".grid-filler-center") {
            return center;
        }
        return null;
    };
    return {
        document,
        deck,
        available:
            clientWidth -
            (parseFloat(paddingLeft) || 0) -
            (parseFloat(paddingRight) || 0),
        getComputedStyle() {
            return { paddingLeft, paddingRight };
        },
    };
}

test("fitCenterDeckCards shrinks deck font when suit rows overflow the center cell", () => {
    // Arrange: center content box is narrower than the longest undeployed row.
    const layout = mockCenterDeckLayout({
        clientWidth: 220,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [400, 350, 300, 280],
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitCenterDeckCards();

    // Assert: scale ≈ 30 * (220 - 20 - 4) / 400 = 14.7px (fits on first pass).
    const size = parseFloat(layout.deck.style.fontSize);
    assert.ok(Number.isFinite(size), "font-size set on #deck-status");
    assert.ok(size < 30, "center cards shrink below the seat default");
    assert.ok(size > 10, "center cards remain readable");
    assert.ok(Math.abs(size - 14.7) < 0.2, `expected ~14.7px, got ${size}`);
});

test("fitCenterDeckCards solves font size so fixed chrome still fits", () => {
    // Arrange: pure proportional shrink leaves ~216px needed for available=196
    // because 20px of borders do not scale; the closed-form fit must land inside.
    const layout = mockCenterDeckLayout({
        clientWidth: 220,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [400],
        fixedOverflowPx: 20,
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitCenterDeckCards();

    // Assert: final row width fits the content box (not merely "smaller font").
    const size = parseFloat(layout.deck.style.fontSize);
    assert.ok(size < 14.7, `should shrink below the single-pass 14.7px, got ${size}`);
    const needed = layout.deck.querySelectorAll(".deck-suit-row")[0].scrollWidth;
    assert.ok(
        needed <= layout.available + 0.5,
        `row must fit with fixed chrome, needed=${needed} available=${layout.available}`
    );
});

test("fitCenterDeckCards scales below 10px when the center is extremely narrow", () => {
    // Arrange: at a 10px floor the longest row would still be ~133px wide, but
    // only 40px of content box remains — overflow into East unless we go lower.
    const layout = mockCenterDeckLayout({
        clientWidth: 64,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [400],
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitCenterDeckCards();

    // Assert: available = 40; scale ≈ 30 * 40 / 400 = 3px.
    const size = parseFloat(layout.deck.style.fontSize);
    assert.ok(size < 10, `must shrink below the old 10px floor, got ${size}`);
    assert.ok(Math.abs(size - 3) < 0.2, `expected ~3px, got ${size}`);
    const needed = layout.deck.querySelectorAll(".deck-suit-row")[0].scrollWidth;
    assert.ok(needed <= 40, `row must fit the center content box, needed=${needed}`);
});

test("fitCenterDeckCards accounts for per-card fixed borders at extreme width", () => {
    // Arrange: 13 cards × 2px borders ≈ 26px fixed; available=30 leaves little
    // scalable room — proportional passes converge too slowly without a closed form.
    const layout = mockCenterDeckLayout({
        clientWidth: 54,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [400],
        fixedOverflowPx: 26,
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitCenterDeckCards();

    // Assert
    const needed = layout.deck.querySelectorAll(".deck-suit-row")[0].scrollWidth;
    assert.ok(
        needed <= layout.available + 0.5,
        `must fit with real fixed-border cost, needed=${needed} available=${layout.available}`
    );
    assert.equal(layout.deck.style.overflowX, "");
});

test("fitCenterDeckCards clips when fixed chrome alone exceeds the center", () => {
    // Arrange: non-scaling chrome wider than the content box — font cannot fit.
    const layout = mockCenterDeckLayout({
        clientWidth: 54,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [400],
        fixedOverflowPx: 40,
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitCenterDeckCards();

    // Assert: clip fallback keeps painted overflow inside the center cell.
    assert.equal(layout.deck.style.maxWidth, "30px");
    assert.equal(layout.deck.style.overflowX, "hidden");
});

test("fitCenterDeckCards restores full size when rows fit the center cell", () => {
    // Arrange
    const layout = mockCenterDeckLayout({
        clientWidth: 400,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [200, 180],
    });
    layout.deck.style.fontSize = "12px";
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitCenterDeckCards();

    // Assert: clear inline size so CSS 30px applies again.
    assert.equal(layout.deck.style.fontSize, "");
});

test("fitCenterDeckCards skips shrinking while the undeployed deck is hidden", () => {
    // Arrange
    const layout = mockCenterDeckLayout({
        clientWidth: 220,
        rowScrollWidths: [400],
        hidden: true,
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitCenterDeckCards();

    // Assert
    assert.equal(layout.deck.style.fontSize, "");
});

test("exitPlay refits center cards after a narrow resize during play", () => {
    // Arrange: enter play (hides deck), resize while hidden clears the fit, then
    // exit — unhide must refit so rows do not reappear at the full 30px size.
    const layout = mockCenterDeckLayout({
        clientWidth: 64,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [400],
        initialValues: {
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
            west_spades: "K643",
            west_hearts: "T8",
            west_diamonds: "AK742",
            west_clubs: "T5",
        },
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
        scheduleDealSolve() {
            return Promise.resolve();
        },
    });
    assert.equal(ctx.startPlay("south", "N", 6), true);
    assert.equal(layout.deck.hidden, true);

    // Act: simulate resize during play, then leave play mode.
    ctx.fitCenterDeckCards();
    assert.equal(layout.deck.style.fontSize, "");
    ctx.exitPlay();

    // Assert
    assert.equal(layout.deck.hidden, false);
    const size = parseFloat(layout.deck.style.fontSize);
    assert.ok(size < 10, `exitPlay must refit below full size, got ${size}`);
    const needed = layout.deck.querySelectorAll(".deck-suit-row")[0].scrollWidth;
    assert.ok(needed <= layout.available, `needed=${needed} available=${layout.available}`);
});

test("updateDeckStatus refits center cards after rewriting the undeployed strip", () => {
    // Arrange
    const layout = mockCenterDeckLayout({
        clientWidth: 220,
        paddingLeft: "20px",
        paddingRight: "4px",
        rowScrollWidths: [400],
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });
    const emptyHands = {
        north: [],
        east: [],
        south: [],
        west: [],
    };

    // Act
    ctx.updateDeckStatus(emptyHands);

    // Assert: innerHTML rewrite then fit — overflow forces a smaller font.
    assert.match(layout.deck.innerHTML, /deck-suit-row/);
    const size = parseFloat(layout.deck.style.fontSize);
    assert.ok(size < 30);
});

test("pageLoad listens for window resize to refit diagram fonts", () => {
    // Arrange
    const document = createMockDocument();
    const resizeListeners = [];
    const ctx = loadDdsWeb(document, {
        window: {
            addEventListener(type, listener) {
                resizeListeners.push({ type, listener });
            },
        },
    });

    // Act
    ctx.pageLoad();

    // Assert
    assert.ok(
        resizeListeners.some(
            (entry) =>
                entry.type === "resize" && entry.listener === ctx.fitDiagramFonts
        ),
        "pageLoad wires window resize to fitDiagramFonts"
    );
});

function mockSouthHandLayout({
    clientWidth,
    paddingLeft = "20px",
    paddingRight = "20px",
    rowScrollWidths,
    // Extra fixed px that does not shrink with font-size (simulates 1px borders).
    fixedOverflowPx = 0,
    initialValues = {},
}) {
    let fontPx = 30;
    const baseWidths = rowScrollWidths.slice();
    const south = {
        className: "grid-item hand-south",
        clientWidth,
        style: {
            maxWidth: "",
            overflowX: "",
            get fontSize() {
                return fontPx === 30 ? "" : fontPx + "px";
            },
            set fontSize(value) {
                if (value === "" || value == null) {
                    fontPx = 30;
                    return;
                }
                const parsed = parseFloat(value);
                fontPx = Number.isFinite(parsed) ? parsed : 30;
            },
        },
        querySelectorAll(selector) {
            if (selector !== ".hand-suit") {
                return [];
            }
            const scale = fontPx / 30;
            return baseWidths.map((scrollWidth) => ({
                scrollWidth: scrollWidth * scale + fixedOverflowPx,
            }));
        },
    };
    const document = createMockDocument(initialValues);
    document.querySelector = (selector) => {
        if (selector === ".hand-south") {
            return south;
        }
        return null;
    };
    return {
        document,
        south,
        available:
            clientWidth -
            (parseFloat(paddingLeft) || 0) -
            (parseFloat(paddingRight) || 0),
        getComputedStyle(element) {
            if (element === south) {
                return { paddingLeft, paddingRight };
            }
            return { paddingLeft: "0px", paddingRight: "0px" };
        },
    };
}

test("fitSouthHandCards shrinks font when suit rows overflow the south cell", () => {
    // Arrange: south content box narrower than the longest holding row.
    const layout = mockSouthHandLayout({
        clientWidth: 220,
        paddingLeft: "20px",
        paddingRight: "20px",
        rowScrollWidths: [400, 350, 300, 280],
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitSouthHandCards();

    // Assert: available = 180; scale ≈ 30 * (180 - 0) / 400 when no fixed chrome.
    const size = parseFloat(layout.south.style.fontSize);
    assert.ok(Number.isFinite(size), "font-size set on .hand-south");
    assert.ok(size < 30, "south cards shrink below the seat default");
    assert.ok(Math.abs(size - 13.5) < 0.2, `expected ~13.5px, got ${size}`);
});

test("fitSouthHandCards solves font size so fixed chrome still fits", () => {
    // Arrange: 20px non-scaling borders; closed form must land inside available=180.
    const layout = mockSouthHandLayout({
        clientWidth: 220,
        rowScrollWidths: [400],
        fixedOverflowPx: 20,
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitSouthHandCards();

    // Assert
    const size = parseFloat(layout.south.style.fontSize);
    assert.ok(size < 13.5, `should shrink below pure proportional 13.5px, got ${size}`);
    const needed = layout.south.querySelectorAll(".hand-suit")[0].scrollWidth;
    assert.ok(
        needed <= layout.available + 0.5,
        `south row must not spill into the contract matrix, needed=${needed}`
    );
});

test("fitSouthHandCards clips when fixed chrome alone exceeds the south cell", () => {
    // Arrange
    const layout = mockSouthHandLayout({
        clientWidth: 60,
        rowScrollWidths: [400],
        fixedOverflowPx: 40,
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitSouthHandCards();

    // Assert: available = 20; clip keeps overflow out of the SE matrix cell.
    assert.equal(layout.south.style.maxWidth, "20px");
    assert.equal(layout.south.style.overflowX, "hidden");
});

test("fitSouthHandCards restores full size when rows fit the south cell", () => {
    // Arrange
    const layout = mockSouthHandLayout({
        clientWidth: 400,
        rowScrollWidths: [200, 180],
    });
    layout.south.style.fontSize = "12px";
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });

    // Act
    ctx.fitSouthHandCards();

    // Assert
    assert.equal(layout.south.style.fontSize, "");
});

test("updateHandCardDisplays refits south after rewriting holdings", () => {
    // Arrange
    const layout = mockSouthHandLayout({
        clientWidth: 220,
        rowScrollWidths: [400],
    });
    const ctx = loadDdsWeb(layout.document, {
        getComputedStyle: layout.getComputedStyle,
    });
    const emptyHands = {
        north: [],
        east: [],
        south: [],
        west: [],
    };

    // Act
    ctx.updateHandCardDisplays(emptyHands);

    // Assert
    const size = parseFloat(layout.south.style.fontSize);
    assert.ok(size < 30, "updateHandCardDisplays must refit south holdings");
});

test("hand-card pips show a light outline affordance for clickability", () => {
    // Arrange
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");
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
    const ctx = loadDdsWeb(document);
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
    const ctx = loadDdsWeb(document);
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

const GRAND_SLAM_PBN =
    "N:AKQJ.AKQJ.T98.T9 5432.5432.32.432 T98.T9.AKQJ.AKQJ 76.876.7654.8765";
const EVERYONE_3N_PBN =
    'N:QT9.A8765432.KJ. KJ..A8765432.QT9 A8765432.QT9..KJ .KJ.QT9.A8765432';
const LIST1_PBN =
    "N:Q87.T8.AKJT64.J6 964.AJ765.Q73.74 AKJT2.Q943..AK95 53.K2.9852.QT832";
const DLM_BOARD_01 =
    "Board 01=fnbkmmincldklcfcofoiefnapm018";
const LIN_DEAL =
    "pn|a,b,c,d|st||md|3S27AH3489TD5JC45J,S358QKH56D4KAC3QK,S4JH2JQD2678TC678,|rh||ah|Board 1|sv|o|";

test("runDdsWebScripts exposes parseFirstDealFromText", () => {
    // Arrange / Act: same classic-script order as the deployed page.
    const sandbox = {
        document: createMockDocument(),
        console,
        Promise,
        Error,
        setTimeout,
        clearTimeout,
    };
    const ctx = createContext(sandbox);
    runDdsWebScripts(ctx);
    const deal = ctx.parseFirstDealFromText(`[Deal "${EVERYONE_3N_PBN}"]`);

    // Assert: deal-import API is available after the full load sequence.
    assert.equal(deal.north, "QT9.A8765432.KJ.");
    assert.equal(deal.east, "KJ..A8765432.QT9");
    assert.equal(deal.south, "A8765432.QT9..KJ");
    assert.equal(deal.west, ".KJ.QT9.A8765432");
});

test("runDdsWebScripts exposes Card and handsToPbn", () => {
    // Arrange / Act: same classic-script order as the deployed page.
    const sandbox = {
        document: createMockDocument(),
        console,
        Promise,
        Error,
        setTimeout,
        clearTimeout,
    };
    const ctx = createContext(sandbox);
    runDdsWebScripts(ctx);
    const card = new ctx.Card("hearts", "K");
    const pbn = ctx.handsToPbn({
        north: cardsFromKeys(ctx, ["SA", "SK", "SQ", "SJ", "ST", "S9", "S8", "S7", "S6", "S5", "S4", "S3", "S2"]),
        east: cardsFromKeys(ctx, ["HA", "HK", "HQ", "HJ", "HT", "H9", "H8", "H7", "H6", "H5", "H4", "H3", "H2"]),
        south: cardsFromKeys(ctx, ["DA", "DK", "DQ", "DJ", "DT", "D9", "D8", "D7", "D6", "D5", "D4", "D3", "D2"]),
        west: cardsFromKeys(ctx, ["CA", "CK", "CQ", "CJ", "CT", "C9", "C8", "C7", "C6", "C5", "C4", "C3", "C2"]),
    });

    // Assert: deal-model API is available after the full load sequence.
    assert.equal(card.key(), "HK");
    assert.equal(
        pbn,
        "N:AKQJT98765432... .AKQJT98765432.. ..AKQJT98765432. ...AKQJT98765432"
    );
    assert.equal(ctx.openingLeader("south"), "west");
    assert.equal(ctx.pipFromDdsRank(14), "A");
});

function assertImportedDeal(ctx, document, expected) {
    assert.equal(document.element("north_spades").value, expected.north[0]);
    assert.equal(document.element("north_hearts").value, expected.north[1]);
    assert.equal(document.element("north_diamonds").value, expected.north[2]);
    assert.equal(document.element("north_clubs").value, expected.north[3]);
    assert.equal(document.element("east_spades").value, expected.east[0]);
    assert.equal(document.element("east_hearts").value, expected.east[1]);
    assert.equal(document.element("east_diamonds").value, expected.east[2]);
    assert.equal(document.element("east_clubs").value, expected.east[3]);
    assert.equal(document.element("south_spades").value, expected.south[0]);
    assert.equal(document.element("south_hearts").value, expected.south[1]);
    assert.equal(document.element("south_diamonds").value, expected.south[2]);
    assert.equal(document.element("south_clubs").value, expected.south[3]);
    assert.equal(document.element("west_spades").value, expected.west[0]);
    assert.equal(document.element("west_hearts").value, expected.west[1]);
    assert.equal(document.element("west_diamonds").value, expected.west[2]);
    assert.equal(document.element("west_clubs").value, expected.west[3]);
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
}

test("parseFirstDealFromText reads a PBN Deal tag", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const deal = ctx.parseFirstDealFromText(`[Deal "${EVERYONE_3N_PBN}"]`);
    assert.equal(deal.north, "QT9.A8765432.KJ.");
    assert.equal(deal.east, "KJ..A8765432.QT9");
    assert.equal(deal.south, "A8765432.QT9..KJ");
    assert.equal(deal.west, ".KJ.QT9.A8765432");
});

test("parseFirstDealFromText uses the first PBN deal when several are present", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const text = [
        `[Deal "${LIST1_PBN}"]`,
        `[Deal "${EVERYONE_3N_PBN}"]`,
    ].join("\n");
    const deal = ctx.parseFirstDealFromText(text);
    assert.equal(deal.north, "Q87.T8.AKJT64.J6");
    assert.equal(deal.west, "53.K2.9852.QT832");
});

test("parseFirstDealFromText reads a dtest .txt PBN line", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const text =
        "NUMBER 1 \n" +
        `PBN 1 0 2 0 "${LIST1_PBN}" \n` +
        "TABLE 11 2 11 1\n";
    const deal = ctx.parseFirstDealFromText(text);
    assert.equal(deal.north, "Q87.T8.AKJT64.J6");
    assert.equal(deal.east, "964.AJ765.Q73.74");
    assert.equal(deal.south, "AKJT2.Q943..AK95");
    assert.equal(deal.west, "53.K2.9852.QT832");
});

test("parseFirstDealFromText reads a LIN md| deal and fills the omitted hand", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const deal = ctx.parseFirstDealFromText(LIN_DEAL);
    assert.equal(deal.south, "A72.T9843.J5.J54");
    assert.equal(deal.west, "KQ853.65.AK4.KQ3");
    assert.equal(deal.north, "J4.QJ2.T8762.876");
    assert.equal(deal.east, "T96.AK7.Q93.AT92");
});

test("parseFirstDealFromText keeps LIN hands in S,W,N,E order for every dealer digit", () => {
    // BBO md| hands are always South, West, North, East; the leading digit is
    // only the dealer (1=S … 4=E), not a rotation of the hand list.
    const ctx = loadDdsWeb(createMockDocument());
    const hands =
        "SQ953HJ84D6CQ9843,S64HA96DT2CAKJ652,ST82HT5DAKJ743CT7,";
    for (const dealer of ["1", "2", "3", "4"]) {
        const deal = ctx.parseFirstDealFromText("md|" + dealer + hands);
        assert.equal(deal.south, "Q953.J84.6.Q9843", "dealer " + dealer);
        assert.equal(deal.west, "64.A96.T2.AKJ652", "dealer " + dealer);
        assert.equal(deal.north, "T82.T5.AKJ743.T7", "dealer " + dealer);
    }
});

test("parseFirstDealFromText uses the first LIN deal when several are present", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const second =
        "md|3S6HKQ65432DAT32C6,SK952H87D965CAJT8,SAQJ73HAJ9DK84C32,|";
    const deal = ctx.parseFirstDealFromText(LIN_DEAL + "\n" + second);
    assert.equal(deal.north, "J4.QJ2.T8762.876");
});

test("parseFirstDealFromText reads the first DLM board", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const text = [
        "[DOCUMENT]",
        "From board=1",
        "To board=2",
        DLM_BOARD_01,
        "Board 02=aaaaaaaeeeeeeeiiiiiiimmmmmmm000",
    ].join("\r\n");
    const deal = ctx.parseFirstDealFromText(text);
    assert.equal(deal.north, "T53.AJ7.AT.AQ762");
    assert.equal(deal.east, "AKJ9.Q.QJ65.KJT8");
    assert.equal(deal.south, "872.T9543.K9732.");
    assert.equal(deal.west, "Q64.K862.84.9543");
});

test("importDealFromText loads a PBN deal into the diagram", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const err = ctx.importDealFromText(`[Deal "${GRAND_SLAM_PBN}"]`);
    assert.equal(err, "");
    assertImportedDeal(ctx, document, {
        north: ["AKQJ", "AKQJ", "T98", "T9"],
        east: ["5432", "5432", "32", "432"],
        south: ["T98", "T9", "AKQJ", "AKQJ"],
        west: ["76", "876", "7654", "8765"],
    });
});

test("parseFirstDealFromText reads a sol-style .txt line without a seat letter", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const text =
        "T5.K4.652.A98542 K6.QJT976.QT7.Q6 432.A.AKJ93.JT73 AQJ987.8532.84.K:65658888888843433232\n" +
        "T98.AKQT4.K853.8 Q6532.8.AJ2.9753 AK.76532.96.QJ62 J74.J9.QT74.AKT4:66769999333376769999\n";
    const deal = ctx.parseFirstDealFromText(text);
    assert.equal(deal.north, "T5.K4.652.A98542");
    assert.equal(deal.east, "K6.QJT976.QT7.Q6");
    assert.equal(deal.south, "432.A.AKJ93.JT73");
    assert.equal(deal.west, "AQJ987.8532.84.K");
});

test("importDealFromText loads the first sol-style deal into the diagram", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const err = ctx.importDealFromText(
        "T5.K4.652.A98542 K6.QJT976.QT7.Q6 432.A.AKJ93.JT73 AQJ987.8532.84.K:65658888888843433232\n"
    );
    assert.equal(err, "");
    assertImportedDeal(ctx, document, {
        north: ["T5", "K4", "652", "A98542"],
        east: ["K6", "QJT976", "QT7", "Q6"],
        south: ["432", "A", "AKJ93", "JT73"],
        west: ["AQJ987", "8532", "84", "K"],
    });
});

test("importDealFromText reports when no deal is found", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const err = ctx.importDealFromText("not a bridge deal file");
    assert.match(err, /deal/i);
    assert.match(err, /sol/i);
});

test("importDealFromText rejects a duplicated card without changing the diagram", () => {
    // Arrange: SA appears in both North and East (S5 missing).
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    document.setValue("north_spades", "T");

    // Act
    const err = ctx.importDealFromText(
        '[Deal "N:AKQJ.AKQJ.T98.T9 A432.5432.32.432 T98.T9.AKQJ.AKQJ 76.876.7654.8765"]'
    );

    // Assert
    assert.notEqual(err, "");
    assert.match(err, /deal|card|duplicate/i);
    assert.equal(document.element("north_spades").value, "T");
});

test("handleDealFileSelected imports through the file input path", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const input = {
        files: [
            {
                text: async () =>
                    '[Deal "N:AKQJ.AKQJ.T98.T9 5432.5432.32.432 T98.T9.AKQJ.AKQJ 76.876.7654.8765"]',
            },
        ],
    };

    await ctx.handleDealFileSelected(input);

    assert.equal(document.element("north_spades").value, "AKQJ");
    assert.equal(document.element("west_clubs").value, "8765");
    // A trailing solve may paint Computing…; import itself must not leave an error.
    assert.doesNotMatch(
        document.element("result").innerHTML,
        /PBN|LIN|DLM|sol-style|Could not|duplicated/i
    );
});

test("handleDealFileSelected reports an import error through the file input path", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.setDealSolveDebounceMs(500);
    const input = {
        files: [
            {
                text: async () => "not a bridge deal file",
            },
        ],
    };

    await ctx.handleDealFileSelected(input);

    assert.match(document.element("result").innerHTML, /PBN|LIN|DLM|sol-style/i);
});

test("handleDealFileSelected ignores a superseded slower file read", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    let releaseSlow;
    const slowText = new Promise((resolve) => {
        releaseSlow = resolve;
    });
    const slowFile = {
        text: async () => slowText,
    };
    const fastFile = {
        text: async () =>
            '[Deal "N:AKQJ.AKQJ.T98.T9 5432.5432.32.432 T98.T9.AKQJ.AKQJ 76.876.7654.8765"]',
    };
    const input = { files: [slowFile] };

    const first = ctx.handleDealFileSelected(input);
    input.files = [fastFile];
    await ctx.handleDealFileSelected(input);
    releaseSlow(
        '[Deal "N:AQ85.AK976.5.J87 JT.QJ5432.Q9.KQ9 972..JT863.A6432 K643.T8.AK742.T5"]'
    );
    await first;

    assert.equal(document.element("north_spades").value, "AKQJ");
});

test("refreshDdTable clears Computing when abandoning a stale PBN", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    ctx.setDdTableComputingDelayMs(80);
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall() {
            return 1;
        },
        getValue() {
            return 9;
        },
    });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await new Promise((resolve) => setTimeout(resolve, 120));
    document.element("result-table").rows[1].cells[1].innerHTML = "";

    const first = ctx.refreshDdTable();
    await new Promise((resolve) => setTimeout(resolve, 20));
    // Edit mid-grace without scheduling an immediate trailing solve.
    ctx.setDealSolveDebounceMs(500);
    document.setValue("north_spades", "AQ8");
    document.setValue("north_hearts", "5AK976");
    await first;

    assert.doesNotMatch(document.element("result").innerHTML, /Computing/i);
});

test("refreshDdTable abandons a stale PBN after the Computing grace period", async () => {
    // Arrange: a slow grace period so an import can change the diagram mid-wait.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    ctx.setDdTableComputingDelayMs(80);
    const seenPbn = [];
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall(_name, _ret, _args, args) {
            seenPbn.push(args[0]);
            return 1;
        },
        getValue() {
            return 9;
        },
    });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await new Promise((resolve) => setTimeout(resolve, 120));
    document.element("result-table").rows[1].cells[1].innerHTML = "";
    seenPbn.length = 0;

    // Act: start a solve, then import a different deal during the grace wait.
    const first = ctx.refreshDdTable();
    await new Promise((resolve) => setTimeout(resolve, 20));
    ctx.importDealFromText(
        '[Deal "N:AKQJ.AKQJ.T98.T9 5432.5432.32.432 T98.T9.AKQJ.AKQJ 76.876.7654.8765"]'
    );
    await first;
    await new Promise((resolve) => setTimeout(resolve, 120));

    // Assert: WASM must not run for the pre-import PBN after the diagram changed.
    assert.ok(
        seenPbn.every((pbn) => !pbn.includes("AQ85")),
        "stale part-score PBN must not be solved after import; saw " +
            JSON.stringify(seenPbn)
    );
    assert.ok(
        seenPbn.some((pbn) => pbn.includes("AKQJ")),
        "imported deal should still be solved; saw " + JSON.stringify(seenPbn)
    );
    assert.equal(document.element("north_spades").value, "AKQJ");
});

test("importDealFromText rejects a hand with more than four suit components", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    document.setValue("north_spades", "T");

    const err = ctx.importDealFromText(
        '[Deal "N:AKQJ.AKQJ.T98.T9.2 5432.5432.32.432 T98.T9.AKQJ.AKQJ 76.876.7654.8765"]'
    );

    assert.notEqual(err, "");
    assert.match(err, /deal|hand|suit|invalid|malformed/i);
    assert.equal(document.element("north_spades").value, "T");
});

test("importDealFromText rejects a hand with an illegal rank character", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    document.setValue("north_spades", "T");

    // X would be stripped by sortPips, leaving a 13-card looking hand.
    const err = ctx.importDealFromText(
        '[Deal "N:AKQJ.AKQJ.T98X.T9 5432.5432.32.432 T98.T9.AKQJ.AKQJ 76.876.7654.8765"]'
    );

    assert.notEqual(err, "");
    assert.match(err, /deal|hand|rank|pip|invalid|malformed/i);
    assert.equal(document.element("north_spades").value, "T");
});

test("handleDealFileSelected keeps an import error over a stale Computing status", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    ctx.setDdTableComputingDelayMs(80);
    ctx.loadDdsModule = async () => {
        await new Promise((resolve) => setTimeout(resolve, 50));
        return {
            _malloc: () => 0,
            _free() {},
            ccall() {
                return 1;
            },
            getValue() {
                return 9;
            },
        };
    };
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await new Promise((resolve) => setTimeout(resolve, 120));
    document.element("result-table").rows[1].cells[1].innerHTML = "";

    const first = ctx.refreshDdTable();
    await new Promise((resolve) => setTimeout(resolve, 20));
    await ctx.handleDealFileSelected({
        files: [{ text: async () => "not a bridge deal file" }],
    });
    await first;
    await new Promise((resolve) => setTimeout(resolve, 100));

    assert.match(document.element("result").innerHTML, /PBN|LIN|DLM|sol-style/i);
    assert.doesNotMatch(document.element("result").innerHTML, /Computing|Solved/i);
});

test("refreshDdTable still solves when the tab is hidden", async () => {
    const document = createMockDocument();
    document.visibilityState = "hidden";
    let rAFScheduled = false;
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame() {
            rAFScheduled = true;
            // Never invoke the callback — hidden tabs may pause rAF.
            return 1;
        },
    });
    ctx.setDdTableComputingDelayMs(0);
    let solved = false;
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall() {
            solved = true;
            return 1;
        },
        getValue() {
            return 9;
        },
    });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await withTimeout(
        ctx.refreshDdTable(),
        500,
        "refreshDdTable hung while visibilityState was hidden"
    );

    assert.equal(solved, true);
    assert.equal(rAFScheduled, false);
});

test("parseFirstDealFromText accepts an optional sol-style board-number prefix", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const deal = ctx.parseFirstDealFromText(
        "1. T5.K4.652.A98542 K6.QJT976.QT7.Q6 432.A.AKJ93.JT73 AQJ987.8532.84.K:6565\n"
    );
    assert.equal(deal.north, "T5.K4.652.A98542");
    assert.equal(deal.west, "AQJ987.8532.84.K");
});

test("parseFirstDealFromText does not treat a leading numeric pip as a board number", () => {
    const ctx = loadDdsWeb(createMockDocument());
    // Without requiring whitespace after "<n>.", the "2." spade holding is
    // mistaken for a board-number prefix and the line fails to parse.
    const deal = ctx.parseFirstDealFromText(
        "2543.5432.32.432 AKQJT9876.AKQJ.. .T9876.AKQJT987. ..654.AKQJT98765\n"
    );
    assert.equal(deal.north, "5432.5432.32.432");
    assert.equal(deal.east, "AKQJT9876.AKQJ..");
});

test("parseFirstDealFromText rejects a LIN deal with more than four hands", () => {
    const ctx = loadDdsWeb(createMockDocument());
    // Four valid S,W,N,E hands plus a fifth garbage hand must not silently
    // import only the first four.
    const fourHands =
        "S27AH3489TD5JC45J,S358QKH56D4KAC3QK,S4JH2JQD2678TC678,ST96HAK7DQ93CAT92";
    const deal = ctx.parseFirstDealFromText("md|3" + fourHands + "|");
    assert.equal(deal.north, "J4.QJ2.T8762.876");

    assert.throws(
        () =>
            ctx.parseFirstDealFromText(
                "md|3" + fourHands + ",SEXTRA|"
            ),
        /PBN|LIN|DLM|sol-style|malformed|hands/i
    );
});

test("parseFirstDealFromText rejects a LIN hand with an illegal character", () => {
    const ctx = loadDdsWeb(createMockDocument());
    assert.throws(
        () =>
            ctx.parseFirstDealFromText(
                "md|3SQ953HJ84D6CQ9843,S64HA96DT2CAKJ652,ST82HT5DAKJ743CT7X,|"
            ),
        /LIN|malformed|illegal|invalid|character/i
    );
});

test("updateActionButtons clears a pending Computing timer from a prior solve", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    ctx.setDdTableComputingDelayMs(80);
    let releaseModule;
    ctx.loadDdsModule = () =>
        new Promise((resolve) => {
            releaseModule = resolve;
        });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await new Promise((resolve) => setTimeout(resolve, 120));
    document.element("result-table").rows[1].cells[1].innerHTML = "";
    document.element("result").innerHTML = "";

    const first = ctx.refreshDdTable();
    await new Promise((resolve) => setTimeout(resolve, 20));
    // Keep the deal complete so only a debounced trailing solve is scheduled.
    ctx.setDealSolveDebounceMs(500);
    document.setValue("north_spades", "AQ58");
    ctx.updateActionButtons();
    await new Promise((resolve) => setTimeout(resolve, 100));

    assert.doesNotMatch(
        document.element("result").innerHTML,
        /Computing/i,
        "stale Computing timer must not paint after a diagram edit"
    );

    releaseModule({
        _malloc: () => 0,
        _free() {},
        ccall() {
            return 1;
        },
        getValue() {
            return 9;
        },
    });
    await first;
});

test("invalidateActiveDdTableRequest clears painted Computing but keeps other status", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    document.element("result").innerHTML = "Computing&hellip;";

    ctx.invalidateActiveDdTableRequest();

    assert.equal(document.element("result").innerHTML, "");

    document.element("result").innerHTML = "Solved in 12 ms.";
    ctx.invalidateActiveDdTableRequest();
    assert.equal(document.element("result").innerHTML, "Solved in 12 ms.");

    document.element("result").innerHTML = "No PBN, LIN, DLM, dtest, or sol-style deal found in the file.";
    ctx.invalidateActiveDdTableRequest();
    assert.match(document.element("result").innerHTML, /PBN|LIN|DLM|sol-style/);
});

test("failed file import cancels a pending debounced solve", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    let ccallCount = 0;
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall() {
            ccallCount += 1;
            return 1;
        },
        getValue() {
            return 9;
        },
    });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await new Promise((resolve) => setTimeout(resolve, 80));
    ccallCount = 0;

    // Arm a trailing solve, then fail a file import before it fires.
    ctx.setDealSolveDebounceMs(100);
    document.setValue("north_spades", "AQ58");
    ctx.updateActionButtons();
    assert.notEqual(
        document.element("result").innerHTML,
        "sentinel",
        "precondition: debounce arming does not require a status sentinel"
    );

    await ctx.handleDealFileSelected({
        files: [{ text: async () => "not a bridge deal file" }],
    });
    const statusAfterImport = document.element("result").innerHTML;
    assert.match(statusAfterImport, /PBN|LIN|DLM|sol-style/i);

    await new Promise((resolve) => setTimeout(resolve, 200));

    assert.equal(
        document.element("result").innerHTML,
        statusAfterImport,
        "debounced solve must not overwrite the import error"
    );
    assert.equal(ccallCount, 0, "debounced solve must not run after import failure");
});

test("importDealFromText accepts a PBN void suit marked with a dash", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const err = ctx.importDealFromText(
        '[Deal "N:QT9.A8765432.KJ.- KJ.-.A8765432.QT9 A8765432.QT9.-.KJ -.KJ.QT9.A8765432"]'
    );
    assert.equal(err, "");
    assert.equal(document.element("north_clubs").value, "");
    assert.equal(document.element("east_hearts").value, "");
    assert.equal(document.element("west_spades").value, "");
});

test("paintStatusFrame resolves if the tab hides before the second animation frame", async () => {
    const document = createMockDocument();
    document.visibilityState = "visible";
    let frames = 0;
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            frames += 1;
            if (frames === 1) {
                document.visibilityState = "hidden";
                setTimeout(cb, 0);
                return 1;
            }
            // A hung second frame would block the solve queue without a fallback.
            return 2;
        },
    });

    await withTimeout(
        ctx.paintStatusFrame(),
        200,
        "paintStatusFrame hung after the tab became hidden mid-wait"
    );
});

test("failed file import stops an in-flight solve job from continuing", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    ctx.setDdTableComputingDelayMs(80);
    let ccallCount = 0;
    ctx.loadDdsModule = async () => {
        await new Promise((resolve) => setTimeout(resolve, 40));
        return {
            _malloc: () => 0,
            _free() {},
            ccall() {
                ccallCount += 1;
                return 1;
            },
            getValue() {
                return 9;
            },
        };
    };
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    document.element("result-table").rows[1].cells[1].innerHTML = "9";
    ctx.onContractSelect("north", "C");

    const solve = ctx.scheduleDealSolve();
    await new Promise((resolve) => setTimeout(resolve, 20));
    await ctx.handleDealFileSelected({
        files: [{ text: async () => "not a bridge deal file" }],
    });
    await withTimeout(solve, 500, "solve job did not finish after import failure");
    await new Promise((resolve) => setTimeout(resolve, 120));

    assert.match(document.element("result").innerHTML, /PBN|LIN|DLM|sol-style/i);
    assert.equal(ccallCount, 0, "solve job must not continue after import failure");
});

test("invalidateActiveDdTableRequest clears dealSolvePending so a coalesced job stops", async () => {
    // Arrange: a queued direct solve sets dealSolvePending while the worker is
    // mid-refresh; invalidation must clear that flag or the worker continues
    // immediately and can overwrite an import error.
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    let releaseModule;
    let ccallCount = 0;
    ctx.loadDdsModule = () =>
        new Promise((resolve) => {
            releaseModule = () =>
                resolve({
                    _malloc: () => 0,
                    _free() {},
                    ccall() {
                        ccallCount += 1;
                        return 1;
                    },
                    getValue() {
                        return 9;
                    },
                });
        });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await new Promise((resolve) => setTimeout(resolve, 10));

    const solve = ctx.scheduleDealSolve();
    await new Promise((resolve) => setTimeout(resolve, 10));
    // Coalesce another direct schedule while the first refresh is still waiting.
    ctx.scheduleDealSolve();
    await ctx.handleDealFileSelected({
        files: [{ text: async () => "not a bridge deal file" }],
    });
    releaseModule();
    await withTimeout(solve, 500, "solve job did not finish after invalidate");
    await new Promise((resolve) => setTimeout(resolve, 30));

    assert.match(document.element("result").innerHTML, /PBN|LIN|DLM|sol-style/i);
    assert.equal(
        ccallCount,
        0,
        "coalesced pending flag must not restart work after invalidate"
    );
});

test("failed file import invalidates an in-flight play-position solve", async () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document, {
        requestAnimationFrame(cb) {
            return setTimeout(cb, 0);
        },
    });
    ctx.loadDdsModule = async () => ({
        _malloc: () => 0,
        _free() {},
        ccall() {
            return 1;
        },
        getValue() {
            return 9;
        },
    });
    ctx.fillFormWithTestData([
        "AQ85.AK976.5.J87",
        "JT.QJ5432.Q9.KQ9",
        "972..JT863.A6432",
        "K643.T8.AK742.T5",
    ]);
    await new Promise((resolve) => setTimeout(resolve, 80));
    document.element("result-table").rows[1].cells[1].innerHTML = "9";

    let releasePlay;
    let playStarted = false;
    ctx.solvePlayPosition = () =>
        new Promise((resolve, reject) => {
            playStarted = true;
            releasePlay = () => reject(new Error("stale play failure"));
        });
    ctx.handleResultTableClick({
        target: {
            closest() {
                return document.element("result-table").rows[1].cells[1];
            },
        },
    });
    await new Promise((resolve) => setTimeout(resolve, 40));
    assert.equal(playStarted, true);
    assert.equal(ctx.isPlayMode(), true);

    await ctx.handleDealFileSelected({
        files: [{ text: async () => "not a bridge deal file" }],
    });
    releasePlay();
    await new Promise((resolve) => setTimeout(resolve, 40));

    assert.match(document.element("result").innerHTML, /PBN|LIN|DLM|sol-style/i);
    assert.doesNotMatch(document.element("result").innerHTML, /stale play failure/i);
});

test("nextDirection and prevDirection walk NESW clockwise", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.nextDirection("north"), "east");
    assert.equal(ctx.nextDirection("east"), "south");
    assert.equal(ctx.nextDirection("south"), "west");
    assert.equal(ctx.nextDirection("west"), "north");
    assert.equal(ctx.prevDirection("north"), "west");
    assert.equal(ctx.prevDirection("west"), "south");
});

test("winningPlay prefers higher rank then trump", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const trick = [
        { seat: "west", key: "S2" },
        { seat: "north", key: "SA" },
        { seat: "east", key: "HK" },
        { seat: "south", key: "H3" },
    ];

    assert.equal(ctx.winningPlay(trick, null).seat, "north");
    assert.equal(ctx.winningPlay(trick, "H").seat, "east");
});

test("formatPlayDiff renders equals plus and minus", () => {
    const ctx = loadDdsWeb(createMockDocument());

    assert.equal(ctx.formatPlayDiff(0), "=");
    assert.equal(ctx.formatPlayDiff(2), "+2");
    assert.equal(ctx.formatPlayDiff(-3), "\u20133");
});

test("playDiffFromSolverScore projects declarer total vs target", () => {
    const ctx = loadDdsWeb(createMockDocument());

    // South declares; West to play; EW side-to-play score 7 of 13 remaining;
    // no tricks cashed yet; target 9 → declarer gets 6 → diff -3.
    assert.equal(
        ctx.playDiffFromSolverScore({
            declarer: "south",
            seatToPlay: "west",
            nsTricks: 0,
            ewTricks: 0,
            remainingTricks: 13,
            sideToPlayScore: 7,
            targetTricks: 9,
        }),
        -3
    );

    // After NS took 2; North to play; NS score 5 of 11 remaining; target 9 →
    // projected 7 → diff -2.
    assert.equal(
        ctx.playDiffFromSolverScore({
            declarer: "south",
            seatToPlay: "north",
            nsTricks: 2,
            ewTricks: 0,
            remainingTricks: 11,
            sideToPlayScore: 5,
            targetTricks: 9,
        }),
        -2
    );
});

function partScoreHands(ctx) {
    // Matches fillFormWithPartScoreTestData / kPbnPartScore.
    return {
        north: cardsFromKeys(ctx, [
            "SA", "SQ", "S8", "S5", "HA", "HK", "H9", "H7", "H6", "D5", "CJ", "C8", "C7",
        ]),
        east: cardsFromKeys(ctx, [
            "SJ", "ST", "HQ", "HJ", "H5", "H4", "H3", "H2", "DQ", "D9", "CK", "CQ", "C9",
        ]),
        south: cardsFromKeys(ctx, [
            "S9", "S7", "S2", "DJ", "DT", "D8", "D6", "D3", "CA", "C6", "C4", "C3", "C2",
        ]),
        west: cardsFromKeys(ctx, [
            "SK", "S6", "S4", "S3", "HT", "H8", "DA", "DK", "D7", "D4", "D2", "CT", "C5",
        ]),
    };
}

test("createPlayState starts with opening leader and empty history", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const hands = partScoreHands(ctx);

    const state = ctx.createPlayState({
        hands,
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });

    assert.equal(state.leadSeat, "west");
    assert.equal(state.declarer, "south");
    assert.equal(state.denomination, "N");
    assert.equal(state.targetTricks, 6);
    assert.equal(state.trumpLetter, null);
    assert.equal(state.history.length, 0);
    const replay = ctx.replayPlayState(state);
    assert.equal(replay.seat, "west");
    assert.equal(replay.nsTricks, 0);
    assert.equal(replay.ewTricks, 0);
    assert.equal(replay.trick.length, 0);
});

test("replayPlayState advances seat and scores completed tricks", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const state = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });
    state.history = [
        { seat: "west", key: "SK", auto: false },
        { seat: "north", key: "SA", auto: false },
        { seat: "east", key: "ST", auto: false },
        { seat: "south", key: "S2", auto: false },
        { seat: "north", key: "HA", auto: false },
    ];

    const replay = ctx.replayPlayState(state);
    assert.equal(replay.nsTricks, 1);
    assert.equal(replay.ewTricks, 0);
    assert.equal(replay.seat, "east");
    assert.equal(replay.trick.length, 1);
    assert.equal(replay.trick[0].key, "HA");
});

test("solverPositionFromPlay strips played and current-trick cards", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const state = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });
    state.history = [{ seat: "west", key: "SK", auto: false }];

    const pos = ctx.solverPositionFromPlay(state);
    assert.equal(pos.first, 3); // West led
    assert.equal(pos.trickSuits.join(","), "0,0,0");
    assert.equal(pos.trickRanks.join(","), "13,0,0");
    assert.equal(pos.seatToPlay, "north");
    assert.equal(
        pos.remainingHands.west.some((c) => c.key() === "SK"),
        false
    );
    assert.match(pos.pbn, /643\.T8\.AK742\.T5/);
});

test("appendPlay and undoLastChoice remove auto plays together", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const state = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });

    ctx.appendPlay(state, "west", "SK", false);
    ctx.appendPlay(state, "north", "SA", true);
    assert.equal(state.history.length, 2);

    ctx.undoLastChoice(state);
    assert.equal(state.history.length, 0);
});

test("play mode exposes undo APIs without an Undo button", () => {
    const ctx = loadDdsWeb(createMockDocument());
    assert.equal(typeof ctx.undoPlay, "function");
    assert.equal(typeof ctx.undoLastChoice, "function");
    assert.equal(typeof ctx.handlePlayUndoKeyDown, "function");
    assert.equal(typeof ctx.handlePlayHistoryUndo, "function");
    assert.equal(ctx.undoTrickPlay, undefined);
    assert.equal(ctx.undoCurrentTrick, undefined);
    assert.equal(ctx.document.getElementById("undo-play"), null);
});

test("pageLoad wires play undo to Cmd/Ctrl-Z and beforeinput historyUndo", () => {
    const document = createMockDocument();
    const types = [];
    const original = document.addEventListener.bind(document);
    document.addEventListener = (type, listener) => {
        types.push(type);
        return original(type, listener);
    };
    const ctx = loadDdsWeb(document);
    ctx.pageLoad();
    assert.ok(types.includes("keydown"), "pageLoad listens for keydown");
    assert.ok(types.includes("beforeinput"), "pageLoad listens for beforeinput");
});

test("Cmd/Ctrl-Z undoes the last play choice in play mode", () => {
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document, {
        scheduleDealSolve() {
            return Promise.resolve();
        },
        solvePlayPosition() {
            return Promise.resolve({ SK: 0 });
        },
    });
    assert.equal(ctx.startPlay("south", "N", 6), true);
    ctx.playState.pendingDiffs = { SK: 0 };
    assert.equal(ctx.tryPlayCard("west", "SK", false), true);
    assert.equal(ctx.playState.history.length, 1);

    let prevented = false;
    ctx.handlePlayUndoKeyDown({
        key: "z",
        metaKey: true,
        ctrlKey: false,
        shiftKey: false,
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, true);
    assert.equal(ctx.playState.history.length, 0);

    ctx.playState.pendingDiffs = { SK: 0 };
    assert.equal(ctx.tryPlayCard("west", "SK", false), true);
    prevented = false;
    ctx.handlePlayUndoKeyDown({
        key: "z",
        metaKey: false,
        ctrlKey: true,
        shiftKey: false,
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, true);
    assert.equal(ctx.playState.history.length, 0);
});

test("Shift-Cmd/Ctrl-Z does not undo play (redo chord)", () => {
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document, {
        scheduleDealSolve() {
            return Promise.resolve();
        },
    });
    assert.equal(ctx.startPlay("south", "N", 6), true);
    ctx.playState.pendingDiffs = { SK: 0 };
    assert.equal(ctx.tryPlayCard("west", "SK", false), true);

    let prevented = false;
    ctx.handlePlayUndoKeyDown({
        key: "z",
        metaKey: true,
        ctrlKey: false,
        shiftKey: true,
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, false);
    assert.equal(ctx.playState.history.length, 1);
});

test("beforeinput historyUndo undoes the last play choice when emitted", () => {
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document, {
        scheduleDealSolve() {
            return Promise.resolve();
        },
    });
    assert.equal(ctx.startPlay("south", "N", 6), true);
    ctx.playState.pendingDiffs = { SK: 0 };
    assert.equal(ctx.tryPlayCard("west", "SK", false), true);

    let prevented = false;
    ctx.handlePlayHistoryUndo({
        inputType: "historyUndo",
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, true);
    assert.equal(ctx.playState.history.length, 0);
});

test("play undo shortcuts are ignored outside play mode", () => {
    const ctx = loadDdsWeb(createMockDocument());
    let prevented = false;
    ctx.handlePlayUndoKeyDown({
        key: "z",
        metaKey: true,
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, false);
    prevented = false;
    ctx.handlePlayHistoryUndo({
        inputType: "historyUndo",
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, false);
});

test("playDiffMapFromSolverOutput converts scores to contract diffs", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const out = [2, 0, 14, 7, 0, 12, 6]; // SA → 7, SQ → 6 for side to play (EW)

    const map = ctx.playDiffMapFromSolverOutput(out, {
        declarer: "south",
        seatToPlay: "west",
        nsTricks: 0,
        ewTricks: 0,
        remainingTricks: 13,
        targetTricks: 6,
    });

    // EW 7 → NS 6 → diff 0; EW 6 → NS 7 → diff +1
    assert.equal(map.SA, 0);
    assert.equal(map.SQ, 1);
});

test("isLegalPlayCard requires seat to play and pending diff", () => {
    const ctx = loadDdsWeb(createMockDocument());
    const state = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });
    state.pendingDiffs = { SK: 0, HT: -1 };

    assert.equal(ctx.isLegalPlayCard(state, "west", "SK"), true);
    assert.equal(ctx.isLegalPlayCard(state, "west", "SA"), false);
    assert.equal(ctx.isLegalPlayCard(state, "north", "SK"), false);
});

test("targetTricksFromCell parses numeric cell text", () => {
    const ctx = loadDdsWeb(createMockDocument());
    assert.equal(ctx.targetTricksFromCell({ textContent: "9", innerHTML: "9" }), 9);
    assert.equal(ctx.targetTricksFromCell({ textContent: "", innerHTML: "<br>" }), null);
});

test("undoPlay invalidates in-flight play solves so stale results cannot apply", async () => {
    // Arrange: play mode with one slow solve; do not schedule replacements so
    // only the stale request can complete.
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document);
    let releaseSolve;
    const blocked = new Promise((resolve) => {
        releaseSolve = resolve;
    });
    ctx.solvePlayPosition = async () => {
        await blocked;
        return { SK: 0 };
    };
    ctx.scheduleDealSolve = () => Promise.resolve();

    assert.equal(ctx.startPlay("south", "N", 6), true);
    void ctx.refreshPlayTricks();
    const staleId = ctx.leadTricksRequestId;

    ctx.playState.pendingDiffs = { SK: 0 };
    assert.equal(ctx.tryPlayCard("west", "SK", false), true);
    ctx.undoPlay();
    assert.ok(ctx.leadTricksRequestId > staleId);
    assert.equal(ctx.playState.history.length, 0);
    assert.equal(ctx.playState.pendingDiffs, null);

    // Act: stale solve finishes after undo.
    releaseSolve();
    await new Promise((resolve) => setTimeout(resolve, 20));

    // Assert: must not repaint badges or append via auto-play.
    assert.equal(ctx.playState.history.length, 0);
    assert.equal(ctx.playState.pendingDiffs, null);
});

test("startPlay does not nest scheduleDealSolve", () => {
    // applyResultCellSelection / the deal-solve worker already schedule; a
    // nested call from startPlay doubles the opening-position SolveBoard.
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document);
    let scheduled = 0;
    // Assign after load so we overwrite the real scheduleDealSolve export.
    ctx.scheduleDealSolve = () => {
        scheduled += 1;
        return Promise.resolve();
    };

    assert.equal(ctx.startPlay("south", "N", 6), true);
    assert.equal(scheduled, 0);
});

test("startPlay and exitPlay toggle play chrome and trick status", () => {
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document, {
        scheduleDealSolve() {
            return Promise.resolve();
        },
    });

    assert.equal(ctx.startPlay("south", "N", 6), true);
    assert.equal(ctx.isPlayMode(), true);
    assert.equal(document.body.classList.contains("playing"), true);
    assert.equal(document.element("play-bar").hidden, false);
    assert.equal(document.element("trick-status").hidden, false);
    assert.equal(document.element("play-score").hidden, false);
    assert.equal(document.element("play-score").textContent, "NS 0 – EW 0");
    assert.equal(document.element("deck-status").hidden, true);

    ctx.playState.pendingDiffs = { SK: 0 };
    assert.equal(ctx.tryPlayCard("west", "SK", false), true);
    assert.match(document.element("trick-status").innerHTML, /K/);
    assert.equal(ctx.playState.history.length, 1);

    ctx.exitPlay();
    assert.equal(ctx.isPlayMode(), false);
    assert.equal(document.body.classList.contains("playing"), false);
    assert.equal(document.element("deck-status").hidden, false);
});

test("handCardHtml marks legal play cards as playable in play mode", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.playState = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });
    ctx.playState.pendingDiffs = { SK: 0 };

    const html = ctx.handCardHtml("west", new ctx.Card("spades", "K"), 0, "=");
    assert.match(html, /hand-card-playable/);
    assert.match(html, /diff-zero/);
    assert.match(html, />=<\/span>/);
});

test("handCardHtml disables non-playable cards in play mode only", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    const king = new ctx.Card("spades", "K");
    const queen = new ctx.Card("spades", "Q");

    // Edit mode: every card stays an enabled control.
    assert.doesNotMatch(ctx.handCardHtml("west", king, 0), /\sdisabled\b/);

    ctx.playState = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });
    ctx.playState.pendingDiffs = { SK: 0 };

    const playable = ctx.handCardHtml("west", king, 0, "=");
    assert.match(playable, /hand-card-playable/);
    assert.doesNotMatch(playable, /\sdisabled\b/);

    const blocked = ctx.handCardHtml("west", queen, 1);
    assert.doesNotMatch(blocked, /hand-card-playable/);
    assert.match(blocked, /\sdisabled(?:\s|=|>)/);
});

test("renderTrickStatus omits empty seat cells so no blank card chrome shows", () => {
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document, {
        scheduleDealSolve() {
            return Promise.resolve();
        },
    });

    // Arrange / Act: enter play with no cards yet — four empty seat shells
    // would otherwise paint as white bars in the center.
    assert.equal(ctx.startPlay("south", "N", 6), true);
    assert.equal(document.element("trick-status").innerHTML, "");

    ctx.playState.pendingDiffs = { SK: 0 };
    ctx.tryPlayCard("west", "SK", false);
    ctx.playState.pendingDiffs = { SA: 0 };
    ctx.tryPlayCard("north", "SA", false);

    const html = document.element("trick-status").innerHTML;
    assert.match(html, /aria-label="West spade king"/);
    assert.match(html, /aria-label="North spade ace"/);
    // Empty East/South seats must not be in the DOM (no blank card boxes).
    assert.doesNotMatch(html, /trick-e/);
    assert.doesNotMatch(html, /trick-s/);
    assert.equal(document.element("trick-status").getAttribute("aria-live"), "polite");
});

test("trick-status CSS hides empty card shells", () => {
    const here = dirname(fileURLToPath(import.meta.url));
    const css = readFileSync(join(here, "..", "dds_web.css"), "utf8");
    assert.match(css, /\.trick-status\s+\.trick-card:empty/);
    assert.match(css, /\.trick-status\s+\.trick-card\[hidden\]/);
});

test("exitPlay clears a pending contract even when play has not started", () => {
    // Arrange: user clicked a DD cell before it had a trick count (no play yet).
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document);
    ctx.selectedContractState = { direction: "south", denomination: "N" };
    assert.equal(ctx.isPlayMode(), false);

    // Act: deal clear/load/rotate calls exitPlay while playState is still null.
    ctx.exitPlay();

    // Assert: pending selection must not survive onto the next deal.
    assert.equal(ctx.selectedContract(), null);
    assert.equal(ctx.selectedContractState, null);
});

test("exitPlay clears the selected contract so Edit hands stays in edit mode", async () => {
    // Arrange: play mode with a selected contract; scheduleDealSolve would
    // otherwise call ensurePlayForSelectedContract and re-enter play.
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document);
    ctx.refreshDdTable = async () => {};
    ctx.refreshPlayTricks = async () => {};
    ctx.solvePlayPosition = async () => ({ SK: 0 });

    const southNt = document.element("result-table").rows[3].cells[5];
    southNt.innerHTML = "6";
    southNt.textContent = "6";
    ctx.fillFormWithPartScoreTestData();
    ctx.handleResultTableClick({
        target: {
            closest() {
                return southNt;
            },
        },
    });
    await new Promise((resolve) => setTimeout(resolve, 20));
    assert.equal(ctx.isPlayMode(), true);
    assert.ok(ctx.selectedContract());

    // Act: Edit hands
    ctx.exitPlay();
    await new Promise((resolve) => setTimeout(resolve, 20));

    // Assert: stay out of play; contract cell no longer selected.
    assert.equal(ctx.isPlayMode(), false);
    assert.equal(ctx.selectedContract(), null);
    assert.equal(document.element("north_spades").disabled, false);
    assert.equal(southNt.classList.contains("result-cell-selected"), false);
});

test("handCardHtml aria-label includes playable state and contract result", () => {
    const document = createMockDocument();
    const ctx = loadDdsWeb(document);
    ctx.playState = ctx.createPlayState({
        hands: partScoreHands(ctx),
        declarer: "south",
        denomination: "N",
        targetTricks: 6,
    });
    ctx.playState.pendingDiffs = { SK: 0, HT: 1, S6: -2 };

    const equals = ctx.handCardHtml("west", new ctx.Card("spades", "K"), 0, "=");
    assert.match(equals, /aria-label="[^"]*playable[^"]*equals[^"]*"/i);

    const plus = ctx.handCardHtml("west", new ctx.Card("hearts", "T"), 0, "+1");
    assert.match(plus, /aria-label="[^"]*playable[^"]*plus 1[^"]*"/i);

    const minus = ctx.handCardHtml("west", new ctx.Card("spades", "6"), 1, "\u20132");
    assert.match(minus, /aria-label="[^"]*playable[^"]*minus 2[^"]*"/i);
});

test("play score stays NS 0 until a trick completes, then updates", () => {
    const document = createMockDocument({
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
        west_spades: "K643",
        west_hearts: "T8",
        west_diamonds: "AK742",
        west_clubs: "T5",
    });
    const ctx = loadDdsWeb(document, {
        scheduleDealSolve() {
            return Promise.resolve();
        },
    });

    assert.equal(ctx.startPlay("south", "N", 6), true);
    assert.equal(document.element("play-score").hidden, false);
    assert.equal(document.element("play-score").textContent, "NS 0 – EW 0");

    // Incomplete trick: West SK, North SA, East ST — still 0–0.
    ctx.playState.pendingDiffs = { SK: 0 };
    ctx.tryPlayCard("west", "SK", false);
    ctx.playState.pendingDiffs = { SA: 0 };
    ctx.tryPlayCard("north", "SA", false);
    ctx.playState.pendingDiffs = { ST: 0 };
    ctx.tryPlayCard("east", "ST", false);
    assert.equal(document.element("play-score").textContent, "NS 0 – EW 0");

    // Fourth card completes the trick; North's ace wins → NS 1.
    ctx.playState.pendingDiffs = { S2: 0 };
    ctx.tryPlayCard("south", "S2", false);
    assert.equal(document.element("play-score").textContent, "NS 1 – EW 0");
});
