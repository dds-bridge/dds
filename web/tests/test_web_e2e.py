"""End-to-end browser tests for web/dds_web.html (file:// UI and isolated HTTP)."""
from __future__ import annotations

import http.server
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest
from pathlib import Path

from web_site import CROSS_ORIGIN_ISOLATION_HEADERS, make_isolated_http_handler, stage_web_site

try:
    from playwright.sync_api import sync_playwright
except ImportError:
    sync_playwright = None  # type: ignore[misc, assignment]

# res_table[strain][hand], strains S,H,D,C,N — from CalcDDtablePBN on part-score test deal.
PART_SCORE_TABLE = [
    8, 5, 7, 5, 6, 6, 6, 7, 7, 6, 7, 6, 9, 4, 8, 4, 7, 6, 6, 6,
]

STRAIN_INDEX = {"S": 0, "H": 1, "D": 2, "C": 3, "N": 4}
HAND_INDEX = {"N": 0, "E": 1, "S": 2, "W": 3}
TABLE_ROW = {"N": 1, "E": 2, "S": 3, "W": 4}
TABLE_COL = {"C": 1, "D": 2, "H": 3, "S": 4, "N": 5}


def _expected_tricks(direction: str, denomination: str) -> int:
    return PART_SCORE_TABLE[
        STRAIN_INDEX[denomination] * 4 + HAND_INDEX[direction]
    ]


def _ensure_playwright_chromium(browsers_dir: Path) -> None:
    env = os.environ.copy()
    env["PLAYWRIGHT_BROWSERS_PATH"] = str(browsers_dir)
    proc = subprocess.run(
        [sys.executable, "-m", "playwright", "install", "chromium"],
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        raise unittest.SkipTest(
            "playwright install chromium failed "
            f"(exit {proc.returncode}): {proc.stderr or proc.stdout}"
        )


class _HttpSite:
    def __init__(self, directory: Path) -> None:
        self._directory = directory
        self._httpd: http.server.ThreadingHTTPServer | None = None
        self._thread: threading.Thread | None = None
        self.url = ""

    def __enter__(self) -> _HttpSite:
        self._httpd = http.server.ThreadingHTTPServer(
            ("127.0.0.1", 0),
            make_isolated_http_handler(self._directory),
        )
        port = self._httpd.server_address[1]
        self.url = f"http://127.0.0.1:{port}/dds_web.html"
        self._thread = threading.Thread(target=self._httpd.serve_forever, daemon=True)
        self._thread.start()
        return self

    def __exit__(self, *args: object) -> None:
        if self._httpd:
            self._httpd.shutdown()
        if self._thread:
            self._thread.join(timeout=5)


@unittest.skipIf(sync_playwright is None, "playwright not installed")
class DdsWebHtmlE2eTest(unittest.TestCase):
    browsers_dir: Path
    site_dir: Path

    @classmethod
    def setUpClass(cls) -> None:
        if not shutil.which("node"):
            raise unittest.SkipTest("node not found (wasm sanity)")
        cls.browsers_dir = Path(tempfile.mkdtemp(prefix="pw-browsers-"))
        _ensure_playwright_chromium(cls.browsers_dir)
        cls.site_dir = Path(tempfile.mkdtemp(prefix="dds-web-site-"))
        stage_web_site(cls.site_dir)

    @classmethod
    def tearDownClass(cls) -> None:
        shutil.rmtree(cls.browsers_dir, ignore_errors=True)
        shutil.rmtree(cls.site_dir, ignore_errors=True)

    def setUp(self) -> None:
        os.environ["PLAYWRIGHT_BROWSERS_PATH"] = str(self.browsers_dir)
        self._pw = sync_playwright().start()
        self._browser = self._pw.chromium.launch(headless=True)

    def tearDown(self) -> None:
        self._browser.close()
        self._pw.stop()

    def _open_page(self, url: str):
        page = self._browser.new_page()
        errors: list[str] = []
        page.on("pageerror", lambda exc: errors.append(str(exc)))
        page.goto(url, wait_until="load")
        page.wait_for_function(
            "() => typeof createDdsModule === 'function' && typeof ddsWebWasmBytes === 'function'"
        )
        return page, errors

    def _fill_part_score_deal(self, page) -> None:
        page.get_by_role("button", name="Part-score test deal").click()

    def _wait_for_dd_table(self, page) -> None:
        page.wait_for_function(
            """() => {
            const cell = document.getElementById('result-table').rows[1].cells[1];
            return cell && /^\\d+$/.test(cell.textContent.trim());
          }""",
            timeout=120_000,
        )

    def _read_table_cell(self, page, direction: str, denomination: str) -> str:
        row = TABLE_ROW[direction]
        col = TABLE_COL[denomination]
        return page.evaluate(
            f"""() => {{
            return document.getElementById('result-table')
              .rows[{row}].cells[{col}].textContent.trim();
          }}"""
        )

    def _assert_part_score_table(self, page) -> None:
        for direction in ("N", "E", "S", "W"):
            for denomination in ("C", "D", "H", "S", "N"):
                expected = str(_expected_tricks(direction, denomination))
                actual = self._read_table_cell(page, direction, denomination)
                self.assertEqual(
                    actual,
                    expected,
                    msg=f"{direction}/{denomination}",
                )
        result_text = page.locator("#result").inner_text()
        self.assertEqual(result_text.strip(), "")

    def test_page_load_shows_valid_pips(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            pips = page.locator("#valid-pips").inner_text()
            self.assertIn("A", pips)
            self.assertIn("2", pips)
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_deck_status_omits_cards_entered_in_the_diagram(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            cards = page.locator("#deck-status .hand-card")
            self.assertEqual(cards.count(), 52)
            self.assertEqual(
                page.locator('#deck-status [data-card="SA"]').count(),
                1,
            )

            page.locator("#north_spades").fill("A")

            self.assertEqual(
                page.locator('#deck-status [data-card="SA"]').count(),
                0,
            )
            self.assertEqual(page.locator("#deck-status .hand-card").count(), 51)
            self.assertEqual(
                page.locator("#deck-status .deck-card-entered").count(),
                0,
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_drag_undeployed_card_onto_north_hand(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator('#deck-status .hand-card[data-card="SA"]').drag_to(
                page.locator(".hand-north")
            )

            self.assertEqual(page.locator("#north_spades").input_value(), "A")
            self.assertEqual(
                page.locator('#deck-status .hand-card[data-card="SA"]').count(),
                0,
            )
            self.assertEqual(
                page.locator(
                    '#north_spades_cards .hand-card[data-card="SA"]'
                ).count(),
                1,
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_drag_card_onto_hand_sorts_high_to_low(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("A2")
            page.locator("#north_spades").dispatch_event("input")
            page.locator('#deck-status .hand-card[data-card="SK"]').drag_to(
                page.locator(".hand-north")
            )

            self.assertEqual(page.locator("#north_spades").input_value(), "AK2")
            cards = page.locator("#north_spades_cards .hand-card")
            self.assertEqual(
                [cards.nth(i).get_attribute("data-card") for i in range(3)],
                ["SA", "SK", "S2"],
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_drag_within_same_hand_is_ignored(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("AKQ")
            page.locator("#north_spades").dispatch_event("input")
            page.locator(
                '#north_spades_cards .hand-card[data-card="SA"]'
            ).drag_to(page.locator(".hand-north"))

            self.assertEqual(page.locator("#north_spades").input_value(), "AKQ")
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_drag_card_between_hands_then_back_to_center(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("A")
            page.locator("#north_spades").dispatch_event("input")

            page.locator(
                '#north_spades_cards .hand-card[data-card="SA"]'
            ).drag_to(page.locator(".hand-east"))

            self.assertEqual(page.locator("#north_spades").input_value(), "")
            self.assertEqual(page.locator("#east_spades").input_value(), "A")

            page.locator(
                '#east_spades_cards .hand-card[data-card="SA"]'
            ).drag_to(page.locator("#deck-status"))

            self.assertEqual(page.locator("#east_spades").input_value(), "")
            self.assertEqual(
                page.locator('#deck-status .hand-card[data-card="SA"]').count(),
                1,
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_drag_onto_full_hand_is_rejected(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("AKQJT98765432")
            page.locator("#north_spades").dispatch_event("input")
            self.assertEqual(
                page.locator("#deck-status .hand-card").count(),
                39,
            )

            page.locator('#deck-status .hand-card[data-card="HA"]').drag_to(
                page.locator(".hand-north")
            )

            self.assertEqual(page.locator("#north_hearts").input_value(), "")
            self.assertEqual(page.locator("#north_spades").input_value(), "AKQJT98765432")
            self.assertEqual(
                page.locator('#deck-status .hand-card[data-card="HA"]').count(),
                1,
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_http_opening_lead_tricks_appear_on_leader_cards(self) -> None:
        with _HttpSite(self.site_dir) as site:
            page, errors = self._open_page(site.url)
            try:
                page.get_by_role("button", name="Part-score test deal").click()
                self._wait_for_dd_table(page)

                # South / NT → West leads.
                # td nth is among <td> only (C=0 … S=3, NT=4); not HTML cells[]
                # which includes the direction <th> at index 0.
                page.locator("#result-table tr").nth(3).locator("td").nth(4).click()
                self.assertEqual(
                    page.evaluate("() => selectedContract()"),
                    {"direction": "south", "denomination": "N"},
                )
                page.wait_for_function(
                    """() => document.querySelectorAll(
                      '#west_spades_cards .hand-card-tricks').length > 0"""
                )

                west_sk = page.locator(
                    '#west_spades_cards .hand-card[data-card="SK"]'
                )
                self.assertIn(
                    "hand-card-with-tricks",
                    west_sk.get_attribute("class") or "",
                )
                tricks = west_sk.locator(".hand-card-tricks")
                self.assertEqual(tricks.count(), 1)
                score = int(tricks.inner_text())
                self.assertGreaterEqual(score, 0)
                self.assertLessEqual(score, 13)

                self.assertEqual(
                    page.locator(
                        "#north_spades_cards .hand-card-tricks"
                    ).count(),
                    0,
                )
                # Stickiness: numerals must survive a short settle period and a
                # leader-card click (caret placement re-renders holdings).
                page.wait_for_timeout(2000)
                self.assertGreater(
                    page.locator(
                        "#west_spades_cards .hand-card-tricks"
                    ).count(),
                    0,
                    msg=f"tricks vanished; result={page.locator('#result').inner_text()!r}",
                )
                page.locator(
                    '#west_spades_cards .hand-card[data-card="SK"]'
                ).click()
                page.wait_for_timeout(500)
                self.assertGreater(
                    page.locator(
                        "#west_spades_cards .hand-card-tricks"
                    ).count(),
                    0,
                    msg=f"tricks lost after card click; result={page.locator('#result').inner_text()!r}",
                )
                self.assertEqual(errors, [])
            finally:
                page.close()

    def test_http_opening_lead_tricks_when_contract_clicked_during_dd(self) -> None:
        """Selecting a contract while the table is still computing must still
        show lead-trick numerals after both solves finish."""
        with _HttpSite(self.site_dir) as site:
            page, errors = self._open_page(site.url)
            try:
                page.get_by_role("button", name="Part-score test deal").click()
                # Click South/NT immediately — cells may still be empty.
                # td nth is among <td> only (C=0 … S=3, NT=4); not HTML cells[]
                # which includes the direction <th> at index 0.
                page.locator("#result-table tr").nth(3).locator("td").nth(4).click()
                self.assertEqual(
                    page.evaluate("() => selectedContract()"),
                    {"direction": "south", "denomination": "N"},
                )
                page.wait_for_function(
                    """() => document.querySelectorAll(
                      '#west_spades_cards .hand-card-tricks').length > 0""",
                    timeout=120_000,
                )
                self._wait_for_dd_table(page)
                page.wait_for_timeout(1500)
                self.assertGreater(
                    page.locator(
                        "#west_spades_cards .hand-card-tricks"
                    ).count(),
                    0,
                    msg=f"tricks missing after race; result={page.locator('#result').inner_text()!r}",
                )
                self.assertEqual(errors, [])
            finally:
                page.close()

    def test_result_table_cell_click_selects_contract_and_highlights(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            north_clubs = page.locator("#result-table tr").nth(1).locator("td").nth(0)
            west_nt = page.locator("#result-table tr").nth(4).locator("td").nth(4)
            status = page.locator("#contract-status")

            self.assertTrue(status.is_hidden())

            north_clubs.click()
            self.assertIn(
                "result-cell-selected",
                north_clubs.get_attribute("class") or "",
            )
            selected = page.evaluate("() => selectedContract()")
            self.assertEqual(
                selected, {"direction": "north", "denomination": "C"}
            )
            self.assertTrue(status.is_visible())
            self.assertEqual(
                status.locator(".contract-status-denom").inner_text(), "♣"
            )
            self.assertEqual(
                status.locator(".contract-status-by").inner_text(), "by"
            )
            self.assertEqual(
                status.locator(".contract-status-declarer").inner_text(), "N"
            )
            layout = page.evaluate(
                """() => {
                const denom = document.querySelector('.contract-status-denom');
                const by = document.querySelector('.contract-status-by');
                const declarer = document.querySelector(
                  '.contract-status-declarer'
                );
                const denomRect = denom.getBoundingClientRect();
                const byRect = by.getBoundingClientRect();
                const declarerRect = declarer.getBoundingClientRect();
                const denomSize = parseFloat(getComputedStyle(denom).fontSize);
                const declarerSize = parseFloat(
                  getComputedStyle(declarer).fontSize
                );
                return {
                  denomRight: denomRect.right,
                  byLeft: byRect.left,
                  byRight: byRect.right,
                  declarerLeft: declarerRect.left,
                  denomMidY: denomRect.top + denomRect.height / 2,
                  declarerMidY: declarerRect.top + declarerRect.height / 2,
                  denomSize,
                  declarerSize,
                };
              }"""
            )
            self.assertGreater(layout["byLeft"], layout["denomRight"] - 1.0)
            self.assertGreater(layout["declarerLeft"], layout["byRight"] - 1.0)
            self.assertLess(
                abs(layout["declarerMidY"] - layout["denomMidY"]), 8.0
            )
            self.assertAlmostEqual(
                layout["declarerSize"], layout["denomSize"], delta=0.5
            )
            self.assertEqual(
                status.locator("[aria-label]").get_attribute("aria-label"),
                "Clubs; North declares",
            )

            west_nt.click()
            self.assertNotIn(
                "result-cell-selected",
                north_clubs.get_attribute("class") or "",
            )
            self.assertIn(
                "result-cell-selected",
                west_nt.get_attribute("class") or "",
            )
            selected = page.evaluate("() => selectedContract()")
            self.assertEqual(
                selected, {"direction": "west", "denomination": "N"}
            )
            self.assertEqual(
                status.locator(".contract-status-denom").inner_text(), "NT"
            )
            self.assertEqual(
                status.locator(".contract-status-by").inner_text(), "by"
            )
            self.assertEqual(
                status.locator(".contract-status-declarer").inner_text(), "W"
            )
            self.assertEqual(
                status.locator("[aria-label]").get_attribute("aria-label"),
                "Notrump; West declares",
            )

            west_nt.click()
            self.assertNotIn(
                "result-cell-selected",
                west_nt.get_attribute("class") or "",
            )
            self.assertIsNone(page.evaluate("() => selectedContract()"))
            self.assertTrue(status.is_hidden())
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_result_table_is_in_diagram_southeast_corner(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            metrics = page.evaluate(
                """() => {
                const outer = document.querySelector('.grid-outer').getBoundingClientRect();
                const table = document.getElementById('result-table').getBoundingClientRect();
                const se = document.querySelector('.grid-filler-se').getBoundingClientRect();
                return {
                  outerRight: outer.right,
                  outerBottom: outer.bottom,
                  outerMidX: outer.left + outer.width / 2,
                  outerMidY: outer.top + outer.height / 2,
                  tableLeft: table.left,
                  tableTop: table.top,
                  tableRight: table.right,
                  tableBottom: table.bottom,
                  seLeft: se.left,
                  seTop: se.top,
                  seRight: se.right,
                  seBottom: se.bottom,
                  tableInsideSe: document
                    .querySelector('.grid-filler-se')
                    .contains(document.getElementById('result-table')),
                };
              }"""
            )
            self.assertTrue(metrics["tableInsideSe"])
            self.assertGreater(metrics["tableLeft"], metrics["outerMidX"])
            self.assertGreater(metrics["tableTop"], metrics["outerMidY"])
            self.assertLessEqual(metrics["tableRight"], metrics["seRight"] + 1.0)
            self.assertLessEqual(metrics["tableBottom"], metrics["seBottom"] + 1.0)

            hint = page.locator(".result-table-hint")
            self.assertEqual(
                hint.inner_text(), "Click to set declarer and denomination"
            )
            hint_layout = page.evaluate(
                """() => {
                const hint = document.querySelector('.result-table-hint')
                  .getBoundingClientRect();
                const table = document.getElementById('result-table')
                  .getBoundingClientRect();
                const se = document.querySelector('.grid-filler-se');
                return {
                  hintBottom: hint.bottom,
                  tableTop: table.top,
                  hintInsideSe: se.contains(
                    document.querySelector('.result-table-hint')
                  ),
                };
              }"""
            )
            self.assertTrue(hint_layout["hintInsideSe"])
            self.assertLessEqual(
                hint_layout["hintBottom"], hint_layout["tableTop"] + 1.0
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_hand_diagram_cards_are_clickable(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("AQ")
            north_ace = page.locator(
                '#north_spades_cards .hand-card[data-card="SA"]'
            )
            self.assertEqual(north_ace.count(), 1)
            self.assertEqual(north_ace.get_attribute("data-direction"), "north")
            self.assertEqual(north_ace.get_attribute("type"), "button")
            self.assertEqual(north_ace.get_attribute("aria-label"), "North spade ace")
            self.assertEqual(north_ace.inner_text(), "A")
            self.assertTrue(north_ace.is_enabled())

            clicked = page.evaluate(
                """() => {
                const seen = [];
                window.onHandCardClick = (direction, card) => {
                  seen.push({ direction, key: card.key() });
                };
                document
                  .querySelector('#north_spades_cards .hand-card[data-card="SA"]')
                  .click();
                return seen;
              }"""
            )
            self.assertEqual(clicked, [{"direction": "north", "key": "SA"}])
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_typed_pips_appear_only_as_hand_card_glyphs(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            north_spades = page.locator("#north_spades")
            north_spades.click()
            north_spades.type("AK")

            self.assertEqual(north_spades.input_value(), "AK")
            cards = page.locator("#north_spades_cards .hand-card")
            self.assertEqual(cards.count(), 2)
            self.assertEqual(cards.nth(0).inner_text(), "A")
            self.assertEqual(cards.nth(1).inner_text(), "K")

            input_style = north_spades.evaluate(
                """el => {
                const s = getComputedStyle(el);
                return { color: s.color, caretColor: s.caretColor, width: s.width };
              }"""
            )
            self.assertEqual(input_style["color"], "rgba(0, 0, 0, 0)")
            # Native caret is hidden; insertion point is the .hand-caret among glyphs.
            self.assertEqual(input_style["caretColor"], "rgba(0, 0, 0, 0)")
            self.assertEqual(page.locator("#north_spades_cards .hand-caret").count(), 1)
            # Typed text must not consume layout width beside the glyphs.
            self.assertLess(float(input_style["width"].replace("px", "")), 40.0)
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_click_places_caret_so_backspace_edits_holding(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            north_spades = page.locator("#north_spades")
            north_spades.fill("AQ8")

            # Click the right half of Q → caret after Q; Backspace removes Q.
            q = page.locator('#north_spades_cards .hand-card[data-card="SQ"]')
            box = q.bounding_box()
            self.assertIsNotNone(box)
            page.mouse.click(box["x"] + box["width"] * 0.75, box["y"] + box["height"] / 2)
            page.keyboard.press("Backspace")

            self.assertEqual(north_spades.input_value(), "A8")
            cards = page.locator("#north_spades_cards .hand-card")
            self.assertEqual(cards.count(), 2)
            self.assertEqual(cards.nth(0).inner_text(), "A")
            self.assertEqual(cards.nth(1).inner_text(), "8")
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_everyone_makes_3n_east_eight_card_suit_stays_in_diagram(self) -> None:
        """East's 8-card diamond suit must not expand/jump the diagram past .grid-outer."""
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            outer_before = page.locator(".grid-outer").bounding_box()
            self.assertIsNotNone(outer_before)

            page.get_by_role("button", name="Everyone makes 3N test deal").click()
            page.wait_for_function(
                "() => document.querySelectorAll('#east_diamonds_cards .hand-card').length === 8"
            )

            metrics = page.evaluate(
                """() => {
                const outer = document.querySelector('.grid-outer').getBoundingClientRect();
                const holding = document
                  .querySelector('#east_diamonds_cards')
                  .getBoundingClientRect();
                const east = document.querySelector('.hand-east').getBoundingClientRect();
                return {
                  outerRight: outer.right,
                  outerWidth: outer.width,
                  holdingRight: holding.right,
                  eastLeft: east.left,
                  eastWidth: east.width,
                };
              }"""
            )
            self.assertAlmostEqual(
                metrics["outerWidth"],
                outer_before["width"],
                delta=2.0,
                msg="diagram width must not jump when east fills an 8-card suit",
            )
            self.assertLessEqual(
                metrics["holdingRight"],
                metrics["outerRight"] + 1.0,
                msg="east 8-card suit must stay within the diagram",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_everyone_makes_3n_eight_card_suits_stay_in_hand_cells(self) -> None:
        """N/S/W 8-card suits must not spill out of their hand cells (washed-out overflow)."""
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.get_by_role("button", name="Everyone makes 3N test deal").click()
            page.wait_for_function(
                "() => document.querySelectorAll('#north_hearts_cards .hand-card').length === 8"
            )

            spills = page.evaluate(
                """() => {
                const checks = [
                  ['#north_hearts_cards', '.hand-north'],
                  ['#south_spades_cards', '.hand-south'],
                  ['#west_clubs_cards', '.hand-west'],
                  ['#east_diamonds_cards', '.hand-east'],
                ];
                return checks.map(([holdingSel, handSel]) => {
                  const holding = document.querySelector(holdingSel).getBoundingClientRect();
                  const hand = document.querySelector(handSel).getBoundingClientRect();
                  const last = document
                    .querySelector(holdingSel + ' .hand-card:last-child')
                    .getBoundingClientRect();
                  return {
                    holdingSel,
                    lastRight: last.right,
                    handRight: hand.right,
                    overflows: last.right > hand.right + 1,
                  };
                });
              }"""
            )
            for item in spills:
                self.assertFalse(
                    item["overflows"],
                    msg=f"{item['holdingSel']} last pip spills past hand cell "
                    f"({item['lastRight']} > {item['handRight']})",
                )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_deck_status_lives_in_diagram_center_as_four_suit_rows(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            deck = page.locator(".grid-filler-center #deck-status")
            self.assertEqual(deck.count(), 1)
            style = deck.evaluate(
                """el => {
                const s = getComputedStyle(el);
                return {
                  display: s.display,
                  flexDirection: s.flexDirection,
                };
              }"""
            )
            self.assertEqual(style["display"], "flex")
            self.assertEqual(style["flexDirection"], "column")
            rows = page.locator("#deck-status .deck-suit-row")
            self.assertEqual(rows.count(), 4)
            tags = [
                rows.nth(i).locator(":scope > *").first.evaluate("el => el.tagName")
                for i in range(4)
            ]
            self.assertEqual(
                tags,
                ["SPADE-SUIT", "HEART-SUIT", "DIAMOND-SUIT", "CLUB-SUIT"],
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_undeployed_cards_left_align_with_ns_suit_symbols(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            lefts = page.evaluate(
                """() => {
                  const leftOf = (sel) =>
                    document.querySelector(sel).getBoundingClientRect().left;
                  return {
                    north: leftOf('.hand-north spade-suit'),
                    south: leftOf('.hand-south spade-suit'),
                    undeployed: leftOf('#deck-status .deck-suit-row spade-suit'),
                  };
                }"""
            )
            self.assertAlmostEqual(
                lefts["undeployed"],
                lefts["north"],
                delta=1,
                msg="undeployed spades must share N's left edge",
            )
            self.assertAlmostEqual(
                lefts["undeployed"],
                lefts["south"],
                delta=1,
                msg="undeployed spades must share S's left edge",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_ns_cards_left_align_with_center_cards(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("A")
            page.locator("#north_spades").dispatch_event("input")
            page.locator("#south_spades").fill("K")
            page.locator("#south_spades").dispatch_event("input")

            lefts = page.evaluate(
                """() => {
                  const leftOf = (sel) =>
                    document.querySelector(sel).getBoundingClientRect().left;
                  return {
                    north: leftOf('.hand-north .hand-card'),
                    south: leftOf('.hand-south .hand-card'),
                    center: leftOf('#deck-status .hand-card'),
                  };
                }"""
            )
            self.assertAlmostEqual(
                lefts["north"],
                lefts["center"],
                delta=1,
                msg="North cards must share the center cards' left edge",
            )
            self.assertAlmostEqual(
                lefts["south"],
                lefts["center"],
                delta=1,
                msg="South cards must share the center cards' left edge",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_suit_card_columns_left_align_vertically_in_hands_and_center(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            for suit, pips in (
                ("spades", "A"),
                ("hearts", "K"),
                ("diamonds", "Q"),
                ("clubs", "J"),
            ):
                page.locator(f"#north_{suit}").fill(pips)
                page.locator(f"#north_{suit}").dispatch_event("input")

            def first_card_lefts(scope: str) -> list[float]:
                return page.evaluate(
                    """(scope) => {
                      const root = document.querySelector(scope);
                      const cards = [...root.querySelectorAll('.hand-card')]
                        .filter((el, _, all) => {
                          const row = el.closest('.hand-suit, .deck-suit-row');
                          return row && el === row.querySelector('.hand-card');
                        });
                      return cards.map((el) => el.getBoundingClientRect().left);
                    }""",
                    scope,
                )

            north_lefts = first_card_lefts(".hand-north")
            self.assertEqual(len(north_lefts), 4)
            for left in north_lefts[1:]:
                self.assertAlmostEqual(
                    left,
                    north_lefts[0],
                    delta=1,
                    msg="north suit card columns must share a left edge",
                )

            center_lefts = first_card_lefts("#deck-status")
            self.assertGreaterEqual(len(center_lefts), 4)
            for left in center_lefts[1:]:
                self.assertAlmostEqual(
                    left,
                    center_lefts[0],
                    delta=1,
                    msg="center suit card columns must share a left edge",
                )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_center_undeployed_full_suit_stays_clear_of_east(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            bounds = page.evaluate(
                """() => {
                  const last = document.querySelector(
                    '#deck-status .deck-suit-row .hand-card[data-card="S2"]'
                  );
                  const east = document.querySelector('.hand-east');
                  return {
                    lastRight: last.getBoundingClientRect().right,
                    eastLeft: east.getBoundingClientRect().left,
                  };
                }"""
            )
            self.assertLessEqual(
                bounds["lastRight"],
                bounds["eastLeft"] - 1,
                msg="full undeployed spade row must not run into East",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_undeployed_cards_match_dealt_hand_card_chrome(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("K")
            page.locator("#north_spades").dispatch_event("input")

            styles = page.evaluate(
                """() => {
                  const props = [
                    'borderTopWidth', 'borderTopStyle', 'borderTopColor',
                    'borderRadius', 'fontSize', 'paddingTop', 'paddingRight',
                    'paddingBottom', 'paddingLeft', 'marginTop', 'marginRight',
                    'marginBottom', 'marginLeft', 'lineHeight', 'color',
                  ];
                  const read = (sel) => {
                    const s = getComputedStyle(document.querySelector(sel));
                    return Object.fromEntries(props.map((p) => [p, s[p]]));
                  };
                  return {
                    dealt: read('.hand-north .hand-card[data-card="SK"]'),
                    undeployed: read('#deck-status .hand-card[data-card="SA"]'),
                  };
                }"""
            )
            self.assertEqual(
                styles["undeployed"],
                styles["dealt"],
                msg="undeployed pips must use the same hand-card chrome as dealt",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_suit_tags_expose_glyphs_in_dom(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            for tag, glyph, red in (
                ("spade-suit", "♠", False),
                ("heart-suit", "♥", True),
                ("diamond-suit", "♦", True),
                ("club-suit", "♣", False),
            ):
                el = page.locator(f".hand-north {tag}").first
                self.assertEqual(el.inner_text(), glyph)
                tag_color = el.evaluate("el => getComputedStyle(el).color")
                if red:
                    self.assertNotEqual(
                        tag_color, "rgb(0, 0, 0)", msg=f"{tag} glyph is red"
                    )
                else:
                    self.assertEqual(
                        tag_color, "rgb(0, 0, 0)", msg=f"{tag} glyph is black"
                    )

            # Undeployed pips use .hand-card (color: inherit) outside the suit
            # tag, so they stay black even for hearts/diamonds.
            pip_color = page.locator('#deck-status [data-card="HA"]').evaluate(
                "el => getComputedStyle(el).color"
            )
            self.assertEqual(pip_color, "rgb(0, 0, 0)", msg="heart Ace pip stays black")
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_typing_past_13_cards_is_rejected(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            note = page.locator(".hand-north #north-card-count")
            self.assertTrue(note.is_hidden())

            page.locator("#north_spades").fill("AKQJT98765432")
            page.locator("#north_spades").dispatch_event("input")
            page.locator("#north_hearts").fill("A")
            page.locator("#north_hearts").dispatch_event("input")

            self.assertEqual(page.locator("#north_spades").input_value(), "AKQJT98765432")
            self.assertEqual(page.locator("#north_hearts").input_value(), "")
            self.assertTrue(note.is_hidden())
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_typing_moves_a_card_from_another_hand(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("A")
            page.locator("#north_spades").dispatch_event("input")

            page.locator("#east_spades").fill("A")
            page.locator("#east_spades").dispatch_event("input")

            self.assertEqual(page.locator("#north_spades").input_value(), "")
            self.assertEqual(page.locator("#east_spades").input_value(), "A")
            self.assertEqual(
                page.locator('#north_spades_cards .hand-card[data-card="SA"]').count(),
                0,
            )
            self.assertEqual(
                page.locator('#east_spades_cards .hand-card[data-card="SA"]').count(),
                1,
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_partial_hand_shows_its_card_count(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            note = page.locator(".hand-north #north-card-count")
            self.assertTrue(note.is_hidden())

            page.locator("#north_spades").fill("AKQ")
            page.locator("#north_spades").dispatch_event("input")

            self.assertTrue(note.is_visible())
            self.assertEqual(note.inner_text(), "3 cards")

            page.locator("#north_spades").fill("A")
            page.locator("#north_spades").dispatch_event("input")

            self.assertTrue(note.is_visible())
            self.assertEqual(note.inner_text(), "1 card")

            page.locator("#north_spades").fill("AKQJT98765432")
            page.locator("#north_spades").dispatch_event("input")

            self.assertTrue(note.is_hidden())
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_card_count_left_aligns_with_suit_pips(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            page.locator("#north_spades").fill("AKQ")
            page.locator("#north_spades").dispatch_event("input")

            lefts = page.evaluate(
                """() => {
                  const note = document.querySelector('#north-card-count');
                  const pip = document.querySelector(
                    '.hand-north .hand-card[data-card="SA"]'
                  );
                  const pad = parseFloat(getComputedStyle(note).paddingLeft);
                  return {
                    countText: note.getBoundingClientRect().left + pad,
                    pip: pip.getBoundingClientRect().left,
                  };
                }"""
            )
            self.assertAlmostEqual(
                lefts["countText"],
                lefts["pip"],
                delta=1,
                msg="card-count text must share the suit pips' left edge",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_http_is_cross_origin_isolated(self) -> None:
        with _HttpSite(self.site_dir) as site:
            page, errors = self._open_page(site.url)
            try:
                self.assertTrue(page.evaluate("() => window.crossOriginIsolated"))
                self.assertTrue(
                    page.evaluate("() => typeof SharedArrayBuffer === 'function'")
                )
                headers = page.evaluate(
                    """async () => {
                      const res = await fetch(location.href, { cache: 'no-store' });
                      return {
                        coop: res.headers.get('Cross-Origin-Opener-Policy'),
                        coep: res.headers.get('Cross-Origin-Embedder-Policy'),
                      };
                    }"""
                )
                self.assertEqual(
                    headers["coop"], CROSS_ORIGIN_ISOLATION_HEADERS[
                        "Cross-Origin-Opener-Policy"
                    ]
                )
                self.assertEqual(
                    headers["coep"], CROSS_ORIGIN_ISOLATION_HEADERS[
                        "Cross-Origin-Embedder-Policy"
                    ]
                )
                self.assertEqual(errors, [])
            finally:
                page.close()

    def test_http_part_score_table_auto_fills_on_complete_deal(self) -> None:
        with _HttpSite(self.site_dir) as site:
            page, errors = self._open_page(site.url)
            try:
                self.assertEqual(
                    page.evaluate("() => document.characterSet"),
                    "UTF-8",
                    msg="HTTP must decode HTML as UTF-8 so suit glyphs are not mojibake",
                )
                self.assertEqual(page.locator("#double-dummy-it").count(), 0)
                self._fill_part_score_deal(page)
                self._wait_for_dd_table(page)
                self._assert_part_score_table(page)
                self.assertEqual(errors, [])
            finally:
                page.close()

    def test_clearing_deal_clears_results_table(self) -> None:
        with _HttpSite(self.site_dir) as site:
            page, errors = self._open_page(site.url)
            try:
                self._fill_part_score_deal(page)
                self._wait_for_dd_table(page)
                page.get_by_role("button", name="Clear entries").click()
                page.wait_for_function(
                    """() => {
                    const cell = document.getElementById('result-table')
                      .rows[1].cells[1];
                    return cell && cell.textContent.trim() === '';
                  }"""
                )
                self.assertEqual(errors, [])
            finally:
                page.close()

    def test_auto_fills_fourth_hand_when_three_hands_are_complete(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_web.html").as_uri())
        try:
            self._fill_part_score_deal(page)
            for suit in ("spades", "hearts", "diamonds", "clubs"):
                page.locator(f"#west_{suit}").fill("")

            self.assertEqual(page.locator("#west_spades").input_value(), "K643")
            self.assertEqual(page.locator("#west_hearts").input_value(), "T8")
            self.assertEqual(page.locator("#west_diamonds").input_value(), "AK742")
            self.assertEqual(page.locator("#west_clubs").input_value(), "T5")
            self.assertEqual(errors, [])
        finally:
            page.close()


if __name__ == "__main__":
    unittest.main()
