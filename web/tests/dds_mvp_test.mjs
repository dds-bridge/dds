/**
 * Unit tests for web/dds_mvp.js (Node built-in test runner).
 * 
 * Run with:
 *    bazel test //web:dds_mvp_js_test
 * or python -m unittest web.tests.test_dds_mvp_js
 * or node --test web/tests/dds_mvp_test.mjs
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

    const makeElement = (id) => {
        const element = {
            id,
            value: initialValues[id] ?? "",
            innerHTML: "",
            focus() {},
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
