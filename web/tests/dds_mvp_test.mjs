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
        }
    }
    makeElement("valid-pips");
    makeElement("result");
    makeElement("double-dummy-it");
    makeElement("deck-status");
    for (const direction of DIRECTIONS) {
        makeElement(`${direction}-card-count`);
    }

    const rows = [];
    for (let row = 0; row < 5; row++) {
        const cells = [];
        for (let column = 0; column < 6; column++) {
            cells.push({ innerHTML: "" });
        }
        rows.push({ cells });
    }
    store.set("result-table", { rows });

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

test("fillFormWithTestData focuses the double-dummy button", async () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    document.setActiveElement("north_spades");

    ctx.fillFormWithPartScoreTestData();

    // Focus is deferred so the clicked toolbar button cannot reclaim it.
    assert.notEqual(document.activeElement, document.element("double-dummy-it"));
    await new Promise((resolve) => setTimeout(resolve, 0));

    assert.equal(document.activeElement, document.element("double-dummy-it"));
    assert.equal(document.element("double-dummy-it").disabled, false);
    assert.equal(
        document.element("double-dummy-it").className.includes("default-action"),
        true
    );
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

test("updateActionButtons does not auto-fill when a hand has a non-bridge pip", () => {
    // Arrange: three full hands, but north holds an invalid pip (CX) instead of C7.
    const document = threeHandsPartScoreDocument();
    document.setValue("north_clubs", "J8X");
    const ctx = loadDdsMvp(document);

    // Act
    ctx.updateActionButtons();

    // Assert: the fourth hand stays empty and double-dummy remains disabled.
    assert.equal(document.element("west_spades").value, "");
    assert.equal(document.element("west_hearts").value, "");
    assert.equal(document.element("west_diamonds").value, "");
    assert.equal(document.element("west_clubs").value, "");
    assert.equal(document.element("double-dummy-it").disabled, true);
});

test("updateActionButtons auto-fills the fourth hand for three complete hands", () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsMvp(document);
    ctx.updateActionButtons();
    assert.equal(document.element("west_spades").value, "K643");
    assert.equal(document.element("west_hearts").value, "T8");
    assert.equal(document.element("west_diamonds").value, "AK742");
    assert.equal(document.element("west_clubs").value, "T5");
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
    assert.equal(document.element("double-dummy-it").disabled, false);
});

test("updateActionButtons does not auto-fill with a partial fourth hand", () => {
    const document = threeHandsPartScoreDocument();
    document.setValue("west_spades", "K");
    const ctx = loadDdsMvp(document);
    ctx.updateActionButtons();
    assert.equal(document.element("west_spades").value, "K");
    assert.equal(document.element("west_hearts").value, "");
    assert.equal(document.element("double-dummy-it").disabled, true);
});

test("updateActionButtons enables double-dummy when every hand has 13 cards", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    ctx.updateActionButtons();
    assert.equal(document.element("double-dummy-it").disabled, false);
});

test("updateActionButtons keeps double-dummy disabled without preconditions", () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsMvp(document);
    ctx.updateActionButtons();
    assert.equal(document.element("double-dummy-it").disabled, true);
});

test("updateDefaultAction outlines double-dummy only when Enter would activate it", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    ctx.updateActionButtons();

    document.setActiveElement("north_spades");
    ctx.updateDefaultAction();
    assert.equal(
        document.element("double-dummy-it").className.includes("default-action"),
        true
    );

    document.setActiveElement("double-dummy-it");
    ctx.updateDefaultAction();
    assert.equal(
        document.element("double-dummy-it").className.includes("default-action"),
        true
    );

    document.setActiveElement(null);
    // Simulate focus on a toolbar button that is not a hand input.
    const clearLike = { id: "clear-entries" };
    document.activeElement = clearLike;
    ctx.updateDefaultAction();
    assert.equal(
        document.element("double-dummy-it").className.includes("default-action"),
        false
    );
});

test("updateDefaultAction does not outline double-dummy on incomplete deals", () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsMvp(document);
    document.setActiveElement("north_spades");
    ctx.updateActionButtons();
    ctx.updateDefaultAction();
    assert.equal(
        document.element("double-dummy-it").className.includes("default-action"),
        false
    );
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

test("handleHandKeydown runs double-dummy on Enter from a hand input", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    let sendCalled = false;
    ctx.sendJSON = () => {
        sendCalled = true;
    };
    let prevented = false;
    ctx.handleHandKeydown({
        key: "Enter",
        target: document.element("north_spades"),
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, true);
    assert.equal(sendCalled, true);
});

test("handleHandKeydown ignores Enter from a non-hand control", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    let sendCalled = false;
    ctx.sendJSON = () => {
        sendCalled = true;
    };
    let prevented = false;
    ctx.handleHandKeydown({
        key: "Enter",
        target: document.element("double-dummy-it"),
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, false);
    assert.equal(sendCalled, false);
});

test("Enter on a hand input runs double-dummy after loading a complete deal", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    let sendCalled = false;
    ctx.sendJSON = () => {
        sendCalled = true;
    };
    ctx.pageLoad();
    let prevented = false;

    document.element("north_spades").dispatch("keydown", {
        key: "Enter",
        preventDefault() {
            prevented = true;
        },
    });

    assert.equal(prevented, true);
    assert.equal(sendCalled, true);
});

test("document Enter does not run double-dummy when focus is elsewhere", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    let sendCalled = false;
    ctx.sendJSON = () => {
        sendCalled = true;
    };
    ctx.pageLoad();
    let prevented = false;

    document.dispatch("keydown", {
        key: "Enter",
        target: document.element("double-dummy-it"),
        preventDefault() {
            prevented = true;
        },
    });

    assert.equal(prevented, false);
    assert.equal(sendCalled, false);
});

test("handleHandKeydown ignores Enter when neither action is eligible", () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsMvp(document);
    let sendCalled = false;
    ctx.sendJSON = () => {
        sendCalled = true;
    };
    let prevented = false;
    ctx.handleHandKeydown({
        key: "Enter",
        target: document.element("north_spades"),
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, false);
    assert.equal(sendCalled, false);
    assert.equal(document.element("west_spades").value, "");
});
