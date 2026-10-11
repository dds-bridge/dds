// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Sample deals for the DDS Web dropdown. Loaded after dds_web.js so
// fillFormWithTestData is available when a sample is selected.
// Only handleSampleDealSelected is exported; holdings stay private.

/* eslint-env es6 */
/* exported handleSampleDealSelected */

"use strict";

(function (global) {
    const fillFormWithTestData = global.fillFormWithTestData;

    // Keys match <option value="..."> in dds_web.html (NESW dotted holdings).
    const SAMPLE_DEALS = {
        "grand-slam": [
            "AKQJ.AKQJ.T98.T9",
            "5432.5432.32.432",
            "T98.T9.AKQJ.AKQJ",
            "76.876.7654.8765"
        ],
        "everyone-3n": [
            "QT9.A8765432.KJ.",
            "KJ..A8765432.QT9",
            "A8765432.QT9..KJ",
            ".KJ.QT9.A8765432"
        ],
        "part-score": [
            "AQ85.AK976.5.J87",
            "JT.QJ5432.Q9.KQ9",
            "972..JT863.A6432",
            "K643.T8.AK742.T5"
        ]
    };

    function handleSampleDealSelected(select) {
        const nesw = SAMPLE_DEALS[select.value];
        if (!nesw) {
            return;
        }
        fillFormWithTestData(nesw);
        // Restore the placeholder so the control reads like a button again and the
        // same sample can be re-selected after editing.
        select.value = "";
    }

    global.handleSampleDealSelected = handleSampleDealSelected;
})(typeof globalThis !== "undefined" ? globalThis : window);
