// Copyright 2020-2026 Adam Wildavsky
//
//   Use of this source code is governed by an MIT-style
//   license that can be found in the LICENSE file or at
//   https://opensource.org/licenses/MIT

// Sample-deal holdings for the DDS Web dropdown. Pure data; loaded before
// dds_web.js. Keys match <option value="...">; name matches option label;
// hands are NESW dotted holdings.

/* eslint-env es6 */
/* exported SAMPLE_DEALS */

"use strict";

(function (global) {
    global.SAMPLE_DEALS = {
        "grand-slam": {
            name: "NS make 7",
            hands: [
                "AKQJ.AKQJ.T98.T9",
                "5432.5432.32.432",
                "T98.T9.AKQJ.AKQJ",
                "76.876.7654.8765"
            ]
        },
        "everyone-3n": {
            name: "Everyone makes 3N",
            hands: [
                "QT9.A8765432.KJ.",
                "KJ..A8765432.QT9",
                "A8765432.QT9..KJ",
                ".KJ.QT9.A8765432"
            ]
        },
        "part-score": {
            name: "Part-score",
            hands: [
                "AQ85.AK976.5.J87",
                "JT.QJ5432.Q9.KQ9",
                "972..JT863.A6432",
                "K643.T8.AK742.T5"
            ]
        }
    };
})(typeof globalThis !== "undefined" ? globalThis : window);
