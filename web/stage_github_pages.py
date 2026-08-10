#!/usr/bin/env python3
"""Stage a publishable DDS Web directory for GitHub Pages (or any static host).

Expects web/ to already contain patched WASM artifacts from ./web/update_wasm.sh.
Copies the static site plus an index.html (same as dds_web.html) for the Pages root.
"""
from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

DEPLOY_FILES = (
    "dds_web.html",
    "dds_web.css",
    "dds_web.js",
    "coi-serviceworker.js",
    "dds_web_wasm.js",
    "dds_web_wasm.wasm",
    "dds_web_wasm_bin.js",
)


def stage_github_pages(web_root: Path, dest: Path) -> Path:
    """Copy deployable DDS Web files from *web_root* into *dest*."""
    missing = [name for name in DEPLOY_FILES if not (web_root / name).is_file()]
    if missing:
        raise FileNotFoundError(
            f"missing {missing[0]} under {web_root} "
            "(run ./web/update_wasm.sh before staging)"
        )
    dest.mkdir(parents=True, exist_ok=True)
    for name in DEPLOY_FILES:
        shutil.copyfile(web_root / name, dest / name)
    shutil.copyfile(dest / "dds_web.html", dest / "index.html")
    return dest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--web-root",
        type=Path,
        default=Path(__file__).resolve().parent,
        help="Directory with DDS Web sources and WASM artifacts (default: web/)",
    )
    parser.add_argument(
        "--out",
        type=Path,
        required=True,
        help="Output directory for the staged site",
    )
    args = parser.parse_args()
    try:
        out = stage_github_pages(args.web_root.resolve(), args.out.resolve())
    except FileNotFoundError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(f"Staged GitHub Pages site at {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
