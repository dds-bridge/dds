"""Style contract tests for web/dds_mvp.css."""
from __future__ import annotations

import re
import unittest
from pathlib import Path

WEB_ROOT = Path(__file__).resolve().parents[1]


def _rule_body_for_selector(css: str, selector: str) -> str:
    """Return the body of the first rule whose selector list includes selector."""
    css_without_comments = re.sub(r"/\*.*?\*/", "", css, flags=re.DOTALL)
    for match in re.finditer(r"([^{}]+)\{([^}]*)\}", css_without_comments):
        selectors = [part.strip() for part in match.group(1).split(",") if part.strip()]
        if selector in selectors:
            return match.group(2)
    raise AssertionError(f"missing rule containing selector {selector}")


class DdsMvpCssTest(unittest.TestCase):
    def test_default_action_buttons_have_bold_outline_when_enabled(self) -> None:
        css = (WEB_ROOT / "dds_mvp.css").read_text(encoding="utf-8")
        for button_id in ("fill-fourth-hand", "double-dummy-it"):
            body = _rule_body_for_selector(css, f"#{button_id}:enabled")
            self.assertRegex(body, r"outline\s*:")
            width = re.search(r"outline\s*:\s*(\d+)px", body)
            self.assertIsNotNone(width, f"{button_id} outline should set pixel width")
            self.assertGreaterEqual(int(width.group(1)), 2)

    def test_default_action_buttons_are_grayed_out_when_disabled(self) -> None:
        css = (WEB_ROOT / "dds_mvp.css").read_text(encoding="utf-8")
        for button_id in ("fill-fourth-hand", "double-dummy-it"):
            body = _rule_body_for_selector(css, f"#{button_id}:disabled")
            self.assertRegex(body, r"opacity\s*:")
            opacity = re.search(r"opacity\s*:\s*([0-9.]+)", body)
            self.assertIsNotNone(opacity, f"{button_id} disabled rule should set opacity")
            self.assertLess(float(opacity.group(1)), 1.0)


if __name__ == "__main__":
    unittest.main()
