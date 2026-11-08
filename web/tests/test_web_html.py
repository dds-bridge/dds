"""Static checks for web/dds_web.html encoding and GitHub Pages COI wiring."""
from __future__ import annotations

import re
import unittest
from pathlib import Path

WEB_ROOT = Path(__file__).resolve().parents[1]
HTML_PATH = WEB_ROOT / "dds_web.html"
COI_PATH = WEB_ROOT / "coi-serviceworker.js"


class DdsWebHtmlProfessionalizationTest(unittest.TestCase):
    def test_visible_page_title(self) -> None:
        # Document <title> alone is not enough; the page needs a visible
        # product heading so first-time visitors know what they opened.
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'(?is)<h1[^>]*>\s*DDS Web\s*</h1>',
            msg="dds_web.html must show a visible DDS Web title",
        )
        head_title = re.search(r"(?is)<title>\s*(.*?)\s*</title>", text)
        self.assertIsNotNone(head_title)
        self.assertIn("DDS Web", head_title.group(1))

    def test_link_to_github_repository(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*href="https://github\.com/dds-bridge/dds"[^>]*>',
            msg="dds_web.html must link to the dds-bridge/dds repository",
        )
        # Prefer opening the repo in a new tab without leaking the opener.
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*href="https://github\.com/dds-bridge/dds"[^>]*'
            r'rel="[^"]*\bnoopener\b[^"]*"[^>]*>',
        )

    def test_faq_lives_on_its_own_page(self) -> None:
        main = HTML_PATH.read_text(encoding="utf-8")
        faq_path = WEB_ROOT / "dds_web_faq.html"
        self.assertTrue(faq_path.is_file(), msg="dds_web_faq.html must exist")
        faq = faq_path.read_text(encoding="utf-8")
        self.assertNotRegex(
            main,
            r'(?is)<(?:section|div)\b[^>]*id="faq"[^>]*>',
            msg="dds_web.html must not embed the FAQ section",
        )
        self.assertRegex(
            faq,
            r'(?is)<(?:section|div)\b[^>]*id="faq"[^>]*>',
            msg="dds_web_faq.html must include an FAQ landmark",
        )
        self.assertRegex(faq, r"(?is)<h1[^>]*>\s*FAQ\s*</h1>")
        self.assertRegex(
            faq,
            r'(?is)<a\b[^>]*href="dds_web\.html"[^>]*>',
            msg="FAQ page must link back to the solver",
        )
        for question in (
            "What is DDS Web?",
            "Does my deal leave my browser?",
            "How do I play through a contract?",
            "Can I load a tournament hand-record PDF?",
            "Do sample deals have to be complete 52-card deals?",
        ):
            self.assertIn(question, faq)
            self.assertNotIn(question, main)
        self.assertIn("partial deals", faq)
        self.assertNotIn("Why won't solving start?", faq)


class DdsWebHtmlCharsetTest(unittest.TestCase):
    def test_declares_utf8_charset_within_first_1024_bytes(self) -> None:
        # Browsers only honor <meta charset> in the first 1024 bytes. Without it,
        # servers that send Content-Type: text/html (no charset) may decode
        # suit glyphs (♠♥♦♣) as Latin-1/Windows-1252 mojibake.
        raw = HTML_PATH.read_bytes()
        prefix = raw[:1024].lower()
        self.assertRegex(
            prefix,
            rb'<meta\s+charset\s*=\s*["\']?utf-8["\']?\s*/?>',
            msg="dds_web.html must declare UTF-8 early so suit symbols render",
        )

    def test_meta_charset_is_first_in_head(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        head_match = re.search(r"<head\b[^>]*>(.*?)</head>", text, re.I | re.S)
        self.assertIsNotNone(head_match)
        head_inner = head_match.group(1).lstrip()
        self.assertRegex(
            head_inner,
            r'(?is)^<meta\s+charset\s*=\s*["\']?utf-8["\']?\s*/?>',
            msg="<meta charset=utf-8> must be the first tag in <head>",
        )

    def test_html_contains_utf8_suit_glyphs(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        for glyph in ("♠", "♥", "♦", "♣"):
            self.assertIn(glyph, text)

    def test_deal_entry_hint_mentions_partial_deals(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'(?is)<p\b[^>]*class="deal-entry-hint"[^>]*>\s*'
            r"Enter a full or partial deal using only these pips:",
        )
        self.assertNotIn("Enter a 52-card deal", text)

    def test_import_deal_button_and_file_input(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertIn('id="menu-import-deals"', text)
        self.assertIn("Import Deal(s)…", text)
        self.assertNotIn(">Import deal…</button>", text)
        self.assertRegex(
            text,
            r'<input[^>]*id="import-deal-file"[^>]*type="file"',
        )
        self.assertIn("handleDealFileSelected(this)", text)
        self.assertIn(".pbn,.lin,.dlm,.txt", text)
        self.assertRegex(text, r'accept="[^"]*\.pdf[^"]*"')
        self.assertNotIn("hand-record-file", text)
        self.assertNotIn("Choose File", text)
        self.assertNotIn("handleHandRecordFileSelected", text)
        self.assertNotIn('onclick="chooseDealFile()"', text)
        self.assertNotIn("Clear entries</button>", text)
        self.assertNotIn("Rotate the hands clockwise", text)

    def test_hand_record_pdf_nav_controls(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertNotIn("hand-record-url", text)
        self.assertNotIn("PDF URL", text)
        self.assertNotIn("CORS", text)
        self.assertNotIn("hand-record-load", text)
        self.assertNotIn("Load PDF", text)
        self.assertNotIn("hand-record-file", text)
        self.assertNotIn("Hand-record boards", text)
        self.assertNotIn("<fieldset", text)
        self.assertNotIn("<legend>", text)
        self.assertRegex(
            text,
            r'<div[^>]*id="hand-record"[^>]*\bhidden\b',
        )
        self.assertIn('id="hand-record-nav"', text)
        self.assertIn('onclick="handRecordPrev()"', text)
        self.assertIn('onclick="handRecordNext()"', text)
        self.assertIn('id="hand-record-label"', text)
        self.assertIn('id="hand-record-status"', text)

    def test_menubar_replaces_toolbar_and_sample_select(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(text, r'<nav[^>]*class="menubar"[^>]*role="menubar"')
        # Top-level triggers must be menuitems so the menubar has required
        # children when panels are closed (not bare button roles only).
        self.assertRegex(
            text,
            r'(?is)<button\b[^>]*class="menu-label"[^>]*role="menuitem"[^>]*>\s*DDS\s*</button>',
        )
        self.assertRegex(
            text,
            r'(?is)<button\b[^>]*class="menu-label"[^>]*role="menuitem"[^>]*>\s*File\s*</button>',
        )
        self.assertRegex(
            text,
            r'(?is)<button\b[^>]*class="menu-label"[^>]*role="menuitem"[^>]*>\s*Edit\s*</button>',
        )
        self.assertIn('data-menu="dds"', text)
        self.assertIn(">DDS</button>", text)
        self.assertIn('id="menu-about"', text)
        self.assertIn(">About…</button>", text)
        self.assertIn('id="menu-how-to"', text)
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-how-to"[^>]*href="dds_web_faq\.html"[^>]*>\s*FAQ\s*</a>',
        )
        # Keep the solver session: FAQ opens in another tab.
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-how-to"[^>]*target="_blank"[^>]*>',
        )
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-how-to"[^>]*rel="[^"]*\bnoopener\b[^"]*"[^>]*>',
        )
        self.assertRegex(
            text,
            r'(?is)<dialog\b[^>]*id="dds-about-dialog"[^>]*'
            r'aria-labelledby="dds-about-title"[^>]*>',
        )
        # Deal entry starts on North spades without a click; About OK must not
        # steal the page-level autofocus while the dialog is closed.
        self.assertRegex(
            text,
            r'(?is)<input\b[^>]*id="north_spades"[^>]*\bautofocus\b',
        )
        self.assertNotRegex(
            text,
            r'(?is)<button\b[^>]*id="dds-about-ok"[^>]*\bautofocus\b',
        )

        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-source-code"[^>]*'
            r'href="https://github\.com/dds-bridge/dds"[^>]*>\s*Source code\s*</a>',
        )
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-changelog"[^>]*'
            r'href="https://github\.com/dds-bridge/dds/blob/develop/ChangeLog"[^>]*>'
            r'\s*Changelog\s*</a>',
        )
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-changelog"[^>]*target="_blank"[^>]*>',
        )
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-changelog"[^>]*rel="[^"]*\bnoopener\b[^"]*"[^>]*>',
        )
        self.assertRegex(
            text,
            r'(?is)<a\b[^>]*id="menu-report-issue"[^>]*'
            r'href="https://github\.com/dds-bridge/dds/issues"[^>]*>',
        )
        dds_source = text.index('id="menu-source-code"')
        dds_changelog = text.index('id="menu-changelog"')
        dds_report = text.index('id="menu-report-issue"')
        self.assertLess(dds_source, dds_changelog)
        self.assertLess(dds_changelog, dds_report)

        self.assertIn('data-menu="file"', text)
        self.assertIn('id="menu-sample-deals"', text)
        self.assertIn("Sample Deals", text)
        self.assertIn('id="menu-sample-deals-panel"', text)
        self.assertIn('data-menu="edit"', text)
        self.assertIn('id="menu-undo"', text)
        self.assertIn('id="menu-rotate"', text)
        self.assertIn('id="menu-clear"', text)
        self.assertIn("Clear Entries", text)
        self.assertIn('id="menu-edit-hands"', text)
        self.assertNotIn("<select", text)
        self.assertNotIn('id="sample-deals"', text)
        self.assertNotIn("handleSampleDealSelected(this)", text)
        self.assertNotIn("site-repo", text)
        self.assertNotIn("Source on GitHub", text)
        # About is the first DDS menu item.
        dds_about = text.index('id="menu-about"')
        dds_how = text.index('id="menu-how-to"')
        self.assertLess(dds_about, dds_how)
        # FAQ is a separate page, not an in-page scroll target.
        self.assertNotIn('id="faq"', text)
        # Sample deal labels still come from SAMPLE_DEALS at pageLoad.
        self.assertNotIn("fillFormWithGrandSlamTestData()", text)
        self.assertNotIn('value="ns_make_7"', text)
        self.assertNotIn("NS make 7", text)


class DdsWebHtmlCoiTest(unittest.TestCase):
    def test_loads_coi_serviceworker_in_head_before_app_scripts(self) -> None:
        # GitHub Pages cannot set COOP/COEP; coi-serviceworker.js injects them.
        # Load it in <head> before the large WASM scripts so the first-visit
        # reload does not waste a full module download.
        text = HTML_PATH.read_text(encoding="utf-8")
        head_match = re.search(r"<head\b[^>]*>(.*?)</head>", text, re.I | re.S)
        self.assertIsNotNone(head_match)
        self.assertRegex(
            head_match.group(1),
            r'<script\s+src="coi-serviceworker\.js"\s*>\s*</script>',
        )
        coi_at = text.index('src="coi-serviceworker.js"')
        wasm_at = text.index('src="dds_web_wasm.js"')
        import_at = text.index('src="dds_web_deal_import.js"')
        app_at = text.index('src="dds_web.js"')
        self.assertLess(coi_at, wasm_at)
        self.assertLess(coi_at, app_at)
        self.assertLess(import_at, app_at)

    def test_loads_deal_import_script_before_dds_web_js(self) -> None:
        # File-format parsers live in dds_web_deal_import.js so dds_web.js stays
        # focused on UI and solver glue (#382).
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'<script\s+src="dds_web_deal_import\.js"\s*>\s*</script>',
        )
        import_at = text.index('src="dds_web_deal_import.js"')
        wasm_at = text.index('src="dds_web_wasm.js"')
        app_at = text.index('src="dds_web.js"')
        self.assertLess(wasm_at, import_at)
        self.assertLess(import_at, app_at)

    def test_loads_core_solve_and_play_scripts_before_dds_web_js(self) -> None:
        # Deal model (core), WASM/queue (solve), then play state before UI.
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'<script\s+src="dds_web_core\.js"\s*>\s*</script>',
        )
        self.assertRegex(
            text,
            r'<script\s+src="dds_web_solve\.js"\s*>\s*</script>',
        )
        self.assertRegex(
            text,
            r'<script\s+src="dds_web_play\.js"\s*>\s*</script>',
        )
        import_at = text.index('src="dds_web_deal_import.js"')
        pdf_at = text.index('src="dds_web_pdf_import.js"')
        core_at = text.index('src="dds_web_core.js"')
        solve_at = text.index('src="dds_web_solve.js"')
        play_at = text.index('src="dds_web_play.js"')
        app_at = text.index('src="dds_web.js"')
        self.assertLess(import_at, pdf_at)
        self.assertLess(pdf_at, core_at)
        self.assertLess(core_at, solve_at)
        self.assertLess(solve_at, play_at)
        self.assertLess(play_at, app_at)

    def test_loads_pdf_import_script_before_core(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'<script\s+src="dds_web_pdf_import\.js"\s*>\s*</script>',
        )
        import_at = text.index('src="dds_web_deal_import.js"')
        pdf_at = text.index('src="dds_web_pdf_import.js"')
        core_at = text.index('src="dds_web_core.js"')
        self.assertLess(import_at, pdf_at)
        self.assertLess(pdf_at, core_at)

    def test_loads_sample_deals_script_before_dds_web_js(self) -> None:
        # SAMPLE_DEALS is pure data; load before UI so the File menu can read it.
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'<script\s+src="dds_web_sample_deals\.js"\s*>\s*</script>',
        )
        play_at = text.index('src="dds_web_play.js"')
        samples_at = text.index('src="dds_web_sample_deals.js"')
        app_at = text.index('src="dds_web.js"')
        self.assertLess(play_at, samples_at)
        self.assertLess(samples_at, app_at)

    def test_play_mode_controls_and_trick_status(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertIn('id="play-bar"', text)
        self.assertIn('id="trick-status"', text)
        self.assertIn('id="play-score"', text)
        self.assertIn('onclick="exitPlay()"', text)
        self.assertNotIn('id="undo-play"', text)
        self.assertNotIn('onclick="undoPlay()"', text)
        self.assertNotIn('id="undo-trick"', text)
        self.assertNotIn('onclick="undoTrickPlay()"', text)
        # Last-choice undo is ⌘/Ctrl-Z only (no Undo buttons; Edit → Undo needs
        # an editable host, which play mode does not keep focused).
        self.assertNotRegex(text, r"Edit\s*→\s*Undo|Edit\s*-\s*Undo")
        self.assertRegex(text, r"⌘/Ctrl-Z|Ctrl-Z")
        self.assertIn('id="result-table-hint"', text)
        self.assertRegex(text, r'id="result-table-hint"[^>]*\bhidden\b')
        # Trick totals live in the diagram NW corner next to the hands.
        nw = text.index('grid-filler-nw')
        score = text.index('id="play-score"')
        ne = text.index('grid-filler-ne')
        self.assertLess(nw, score)
        self.assertLess(score, ne)

    def test_disables_coep_credentialless_before_coi_serviceworker(self) -> None:
        # coi-serviceworker defaults to COEP: credentialless. Safari / iOS WebKit
        # (including Firefox on iPhone) do not honor that value for isolation, so
        # SharedArrayBuffer stays unavailable. DDS Web is same-origin only, so
        # require-corp is correct — configure window.coi before the worker script.
        text = HTML_PATH.read_text(encoding="utf-8")
        head_match = re.search(r"<head\b[^>]*>(.*?)</head>", text, re.I | re.S)
        self.assertIsNotNone(head_match)
        head = head_match.group(1)
        config_at = head.find("coepCredentialless")
        worker_at = head.find('src="coi-serviceworker.js"')
        self.assertNotEqual(config_at, -1, msg="must set window.coi.coepCredentialless")
        self.assertNotEqual(worker_at, -1)
        self.assertLess(config_at, worker_at)
        self.assertRegex(
            head,
            r"coepCredentialless\s*:\s*\(\)\s*=>\s*false",
        )

    def test_coi_serviceworker_sets_cross_origin_isolation_headers(self) -> None:
        self.assertTrue(COI_PATH.is_file(), msg="vendored coi-serviceworker.js missing")
        text = COI_PATH.read_text(encoding="utf-8")
        self.assertIn("Cross-Origin-Opener-Policy", text)
        self.assertIn("same-origin", text)
        self.assertIn("Cross-Origin-Embedder-Policy", text)
        self.assertIn("require-corp", text)


if __name__ == "__main__":
    unittest.main()
