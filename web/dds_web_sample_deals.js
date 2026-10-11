// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Sample deals for the DDS Web dropdown. Loaded after dds_web.js so
// fillFormWithTestData is available when a sample is selected.

/* eslint-env es6 */
/* exported fillFormWithGrandSlamTestData
            fillFormWithEveryoneMakes3nTestData
            fillFormWithPartScoreTestData
            handleSampleDealSelected */

"use strict";

(function (global) {
    const fillFormWithTestData = global.fillFormWithTestData;

    function fillFormWithGrandSlamTestData() {
        fillFormWithTestData([
            "AKQJ.AKQJ.T98.T9",
            "5432.5432.32.432",
            "T98.T9.AKQJ.AKQJ",
            "76.876.7654.8765"
        ]);
    }

    function fillFormWithEveryoneMakes3nTestData() {
        fillFormWithTestData([
            "QT9.A8765432.KJ.",
            "KJ..A8765432.QT9",
            "A8765432.QT9..KJ",
            ".KJ.QT9.A8765432"
        ]);
    }

    function fillFormWithPartScoreTestData() {
        fillFormWithTestData([
            "AQ85.AK976.5.J87",
            "JT.QJ5432.Q9.KQ9",
            "972..JT863.A6432",
            "K643.T8.AK742.T5"
        ]);
    }

    function handleSampleDealSelected(select) {
        const loaders = {
            "grand-slam": fillFormWithGrandSlamTestData,
            "everyone-3n": fillFormWithEveryoneMakes3nTestData,
            "part-score": fillFormWithPartScoreTestData
        };
        const load = loaders[select.value];
        if (!load) {
            return;
        }
        load();
        // Restore the placeholder so the control reads like a button again and the
        // same sample can be re-selected after editing.
        select.value = "";
    }

    global.fillFormWithGrandSlamTestData = fillFormWithGrandSlamTestData;
    global.fillFormWithEveryoneMakes3nTestData =
        fillFormWithEveryoneMakes3nTestData;
    global.fillFormWithPartScoreTestData = fillFormWithPartScoreTestData;
    global.handleSampleDealSelected = handleSampleDealSelected;
})(typeof globalThis !== "undefined" ? globalThis : window);
