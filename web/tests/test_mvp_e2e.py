"""End-to-end browser tests for web/dds_mvp.html (file:// and HTTP)."""
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

from mvp_site import stage_mvp_site

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


def _make_http_handler(directory: Path) -> type[http.server.SimpleHTTPRequestHandler]:
    root = str(directory)

    class Handler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, request, client_address, server) -> None:
            super().__init__(request, client_address, server, directory=root)

    return Handler


class _HttpSite:
    def __init__(self, directory: Path) -> None:
        self._directory = directory
        self._httpd: http.server.ThreadingHTTPServer | None = None
        self._thread: threading.Thread | None = None
        self.url = ""

    def __enter__(self) -> _HttpSite:
        self._httpd = http.server.ThreadingHTTPServer(
            ("127.0.0.1", 0),
            _make_http_handler(self._directory),
        )
        port = self._httpd.server_address[1]
        self.url = f"http://127.0.0.1:{port}/dds_mvp.html"
        self._thread = threading.Thread(target=self._httpd.serve_forever, daemon=True)
        self._thread.start()
        return self

    def __exit__(self, *args: object) -> None:
        if self._httpd:
            self._httpd.shutdown()
        if self._thread:
            self._thread.join(timeout=5)


@unittest.skipIf(sync_playwright is None, "playwright not installed")
class DdsMvpHtmlE2eTest(unittest.TestCase):
    browsers_dir: Path
    site_dir: Path

    @classmethod
    def setUpClass(cls) -> None:
        if not shutil.which("node"):
            raise unittest.SkipTest("node not found (wasm sanity)")
        cls.browsers_dir = Path(tempfile.mkdtemp(prefix="pw-browsers-"))
        _ensure_playwright_chromium(cls.browsers_dir)
        cls.site_dir = Path(tempfile.mkdtemp(prefix="dds-mvp-site-"))
        stage_mvp_site(cls.site_dir)

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
            "() => typeof createDdsModule === 'function' && typeof ddsMvpWasmBytes === 'function'"
        )
        return page, errors

    def _fill_part_score_deal(self, page) -> None:
        page.get_by_role("button", name="Part-score test deal").click()

    def _run_double_dummy(self, page) -> None:
        page.get_by_role("button", name="Double-dummy it!").click()
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
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            pips = page.locator("#valid-pips").inner_text()
            self.assertIn("A", pips)
            self.assertIn("2", pips)
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_deck_status_grays_cards_entered_in_the_diagram(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            cards = page.locator("#deck-status .deck-card")
            self.assertEqual(cards.count(), 52)
            self.assertEqual(
                page.locator('#deck-status [data-card="SA"]').get_attribute("class"),
                "deck-card",
            )

            page.locator("#north_spades").fill("A")

            entered_card = page.locator('#deck-status [data-card="SA"]')
            self.assertIn("deck-card-entered", entered_card.get_attribute("class"))
            self.assertEqual(
                float(entered_card.evaluate("el => getComputedStyle(el).opacity")),
                1.0,
            )
            self.assertNotEqual(
                entered_card.evaluate("el => getComputedStyle(el).color"),
                "rgb(0, 0, 0)",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_deck_status_displays_all_cards_in_one_row(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            deck = page.locator("#deck-status")
            style = deck.evaluate(
                """el => {
                const s = getComputedStyle(el);
                return {
                  display: s.display,
                  flexWrap: s.flexWrap,
                  overflowX: s.overflowX,
                };
              }"""
            )
            self.assertEqual(style["display"], "flex")
            self.assertEqual(style["flexWrap"], "nowrap")
            self.assertNotIn(style["overflowX"], ("auto", "scroll"))
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_suit_tags_expose_glyphs_in_dom(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
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

            # Deck pips are nested inside suit tags; they must stay black/gray.
            pip_color = page.locator('#deck-status [data-card="HA"]').evaluate(
                "el => getComputedStyle(el).color"
            )
            self.assertEqual(pip_color, "rgb(0, 0, 0)", msg="heart Ace pip stays black")
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_double_dummy_button_has_bold_outline_when_default(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            button = page.locator("#double-dummy-it")
            self.assertNotIn("default-action", button.get_attribute("class") or "")

            self._fill_part_score_deal(page)
            page.wait_for_function(
                "() => document.getElementById('double-dummy-it').classList.contains('default-action')"
            )

            self.assertIn("default-action", button.get_attribute("class"))
            outline_width = float(
                button.evaluate("el => parseFloat(getComputedStyle(el).outlineWidth)")
            )
            self.assertGreaterEqual(outline_width, 2.0)
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_hand_over_13_cards_shows_its_card_count(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            note = page.locator(".hand-north #north-card-count")
            self.assertTrue(note.is_hidden())

            page.locator("#north_spades").fill("AKQJT98765432A")

            self.assertTrue(note.is_visible())
            self.assertEqual(note.inner_text(), "14 cards")
            self.assertTrue(page.locator("#east-card-count").is_hidden())
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_file_url_part_score_table(self) -> None:
        url = self.site_dir.joinpath("dds_mvp.html").as_uri()
        page, errors = self._open_page(url)
        try:
            self._fill_part_score_deal(page)
            self._run_double_dummy(page)
            self._assert_part_score_table(page)
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_http_part_score_table(self) -> None:
        with _HttpSite(self.site_dir) as site:
            page, errors = self._open_page(site.url)
            try:
                self._fill_part_score_deal(page)
                self._run_double_dummy(page)
                self._assert_part_score_table(page)
                self.assertEqual(errors, [])
            finally:
                page.close()

    def test_test_deal_button_focuses_double_dummy(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            page.get_by_role("button", name="Part-score test deal").click()
            page.wait_for_function(
                "() => document.activeElement && document.activeElement.id === 'double-dummy-it'"
            )
            self.assertFalse(page.get_by_role("button", name="Double-dummy it!").is_disabled())
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_enter_runs_double_dummy_after_loading_a_complete_deal(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            self._fill_part_score_deal(page)

            page.wait_for_function(
                "() => document.activeElement && document.activeElement.id === 'double-dummy-it'"
            )
            page.keyboard.press("Enter")
            page.wait_for_function(
                """() => {
                const cell = document.getElementById('result-table').rows[1].cells[1];
                return cell && /^\\d+$/.test(cell.textContent.trim());
              }""",
                timeout=120_000,
            )

            self._assert_part_score_table(page)
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_double_dummy_disabled_on_incomplete_deal(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            page.get_by_role("button", name="Clear entries").click()
            double_dummy = page.get_by_role("button", name="Double-dummy it!")
            self.assertTrue(double_dummy.is_disabled())
            self.assertEqual(
                float(double_dummy.evaluate("el => getComputedStyle(el).opacity")),
                1.0,
            )
            self.assertNotEqual(
                double_dummy.evaluate("el => getComputedStyle(el).color"),
                "rgb(0, 0, 0)",
            )
            self.assertEqual(errors, [])
        finally:
            page.close()

    def test_auto_fills_fourth_hand_when_three_hands_are_complete(self) -> None:
        page, errors = self._open_page(self.site_dir.joinpath("dds_mvp.html").as_uri())
        try:
            self._fill_part_score_deal(page)
            for suit in ("spades", "hearts", "diamonds", "clubs"):
                page.locator(f"#west_{suit}").fill("")

            double_dummy = page.get_by_role("button", name="Double-dummy it!")
            self.assertEqual(page.locator("#west_spades").input_value(), "K643")
            self.assertEqual(page.locator("#west_hearts").input_value(), "T8")
            self.assertEqual(page.locator("#west_diamonds").input_value(), "AK742")
            self.assertEqual(page.locator("#west_clubs").input_value(), "T5")
            self.assertTrue(double_dummy.is_enabled())
            self.assertEqual(errors, [])
        finally:
            page.close()


if __name__ == "__main__":
    unittest.main()
