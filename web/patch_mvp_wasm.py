#!/usr/bin/env python3
"""Patch the single isFileURI line in Emscripten output.

Do not use global substitution on SINGLE_FILE builds: the embedded wasm
bytes can contain arbitrary text that would corrupt the module.
"""
from __future__ import annotations

import sys
from pathlib import Path

OLD = "var isFileURI = (filename) => filename.startsWith('file://');"
NEW = (
    "var isFileURI = (filename) => "
    "(typeof filename === 'string' ? filename : (filename && filename.href) || '')"
    ".startsWith('file://');"
)


def main() -> int:
    path = Path(sys.argv[1])
    text = path.read_text(encoding="utf-8")
    if NEW in text:
        return 0
    count = text.count(OLD)
    if count == 0:
        print("patch_mvp_wasm: isFileURI line not found", file=sys.stderr)
        return 1
    if count != 1:
        print(f"patch_mvp_wasm: expected 1 match, got {count}", file=sys.stderr)
        return 1
    path.write_text(text.replace(OLD, NEW, 1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
