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
        const element = {
            id,
            value: initialValues[id] ?? "",
            innerHTML: "",
            disabled: false,
            focus() {},
            addEventListener() {},
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
    makeElement("fill-fourth-hand");
    makeElement("double-dummy-it");

    const rows = [];
    for (let row = 0; row < 5; row++) {
        const cells = [];
        for (let column = 0; column < 6; column++) {
            cells.push({ innerHTML: "" });
        }
        rows.push({ cells });
    }
    store.set("result-table", { rows });

    return {
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
}

function loadDdsMvp(document) {
    const code = readFileSync(findDdsMvpJsPath(), "utf8");
    const sandbox = {
        document,
        console,
        Promise,
        Error,
    };
    const context = createContext(sandbox);
    runInContext(code, context, { filename: "dds_mvp.js" });
    return context;
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
        ctx.inputIsValid({ N: ["SA"], E: [], S: [], W: [] }),
        "Please enter 13 cards per hand."
    );
});

test("inputIsValid rejects invalid pip", () => {
    const ctx = loadDdsMvp(createMockDocument());
    const hands = {
        N: ["S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "ST", "SJ", "SQ", "SK"],
        E: ["HA", "H2", "H3", "H4", "H5", "H6", "H7", "H8", "H9", "HT", "HJ", "HQ", "HK"],
        S: ["DA", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DT", "DJ", "DQ", "DK"],
        W: ["CA", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CT", "CJ", "CQ", "CK"],
    };
    assert.match(ctx.inputIsValid(hands), /^Please use only these pips:/);
});

test("inputIsValid rejects duplicate cards", () => {
    const ctx = loadDdsMvp(createMockDocument());
    const hands = {
        N: ["SA", "SA", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "ST", "SJ", "SQ"],
        E: ["HA", "H2", "H3", "H4", "H5", "H6", "H7", "H8", "H9", "HT", "HJ", "HQ", "HK"],
        S: ["DA", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DT", "DJ", "DQ", "DK"],
        W: ["CA", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CT", "CJ", "CQ", "CK"],
    };
    const message = ctx.inputIsValid(hands);
    assert.match(message, /^Duplicated card/);
    assert.match(message, /&spades;A/);
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
    assert.equal(hands.N.length, 13);
    assert.equal(hands.E.length, 13);
    assert.equal(hands.S.length, 13);
    assert.equal(hands.W.length, 13);
    assert.ok(hands.N.includes("SA"));
    assert.ok(hands.N.includes("SK"));
    assert.ok(hands.E.includes("CJ"));
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
    assert.equal(state.emptyHand, "W");
});

test("fourthHandFillState rejects partial fourth hand", () => {
    const document = threeHandsPartScoreDocument();
    document.setValue("west_spades", "K");
    const ctx = loadDdsMvp(document);
    assert.equal(ctx.fourthHandFillState(ctx.collectHands()).canFill, false);
});

test("fourthHandFillState rejects duplicate cards", () => {
    const document = threeHandsPartScoreDocument();
    document.setValue("north_spades", "AQ85A");
    document.setValue("north_hearts", "AK975");
    const ctx = loadDdsMvp(document);
    assert.equal(ctx.fourthHandFillState(ctx.collectHands()).canFill, false);
});

test("fillFourthHand completes the part-score west hand", () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFourthHand();
    assert.equal(document.element("west_spades").value, "K643");
    assert.equal(document.element("west_hearts").value, "T8");
    assert.equal(document.element("west_diamonds").value, "AK742");
    assert.equal(document.element("west_clubs").value, "T5");
    assert.equal(ctx.inputIsValid(ctx.collectHands()), "");
    assert.equal(document.element("fill-fourth-hand").disabled, true);
    assert.equal(document.element("double-dummy-it").disabled, false);
});

test("updateActionButtons enables fill without enabling double-dummy for three hands", () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsMvp(document);
    ctx.updateActionButtons();
    assert.equal(document.element("fill-fourth-hand").disabled, false);
    assert.equal(document.element("double-dummy-it").disabled, true);
    assert.equal(document.element("west_spades").value, "");
    assert.equal(document.element("west_clubs").value, "");
});

test("updateActionButtons enables double-dummy when every hand has 13 cards", () => {
    const document = createMockDocument();
    const ctx = loadDdsMvp(document);
    ctx.fillFormWithPartScoreTestData();
    ctx.updateActionButtons();
    assert.equal(document.element("fill-fourth-hand").disabled, true);
    assert.equal(document.element("double-dummy-it").disabled, false);
});

test("updateActionButtons keeps both action buttons disabled without preconditions", () => {
    const document = createMockDocument({ north_spades: "AKQ" });
    const ctx = loadDdsMvp(document);
    ctx.updateActionButtons();
    assert.equal(document.element("fill-fourth-hand").disabled, true);
    assert.equal(document.element("double-dummy-it").disabled, true);
});

test("handleHandKeydown fills the fourth hand on Enter when eligible", () => {
    const document = threeHandsPartScoreDocument();
    const ctx = loadDdsMvp(document);
    let sendCalled = false;
    ctx.sendJSON = () => {
        sendCalled = true;
    };
    let prevented = false;
    ctx.handleHandKeydown({
        key: "Enter",
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, true);
    assert.equal(sendCalled, false);
    assert.equal(document.element("west_spades").value, "K643");
    assert.equal(document.element("west_clubs").value, "T5");
    assert.equal(document.element("fill-fourth-hand").disabled, true);
    assert.equal(document.element("double-dummy-it").disabled, false);
});

test("handleHandKeydown runs double-dummy on Enter when every hand has 13 cards", () => {
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
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, true);
    assert.equal(sendCalled, true);
});

test("page default runs double-dummy on Enter after loading a complete deal", () => {
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
        preventDefault() {
            prevented = true;
        },
    });

    assert.equal(prevented, true);
    assert.equal(sendCalled, true);
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
        preventDefault() {
            prevented = true;
        },
    });
    assert.equal(prevented, false);
    assert.equal(sendCalled, false);
    assert.equal(document.element("west_spades").value, "");
});
