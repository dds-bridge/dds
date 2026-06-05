#!/usr/bin/env python3
"""Verify MVP wasm bytes compile under Node."""
from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

NODE_COMPILE_SNIPPET = (
    "const fs=require('fs');"
    "const b=fs.readFileSync(process.argv[1]);"
    "WebAssembly.compile(b).then(()=>console.log('compile OK'),"
    "e=>{console.error(e);process.exit(1);});"
)


def usage() -> None:
    name = Path(sys.argv[0]).name
    print(f"usage: {name} WASM_FILE", file=sys.stderr)


def wasm_magic_ok(wasm: bytes) -> bool:
    return len(wasm) >= 4 and wasm[:4] == b"\x00asm"


def main() -> int:
    if len(sys.argv) != 2:
        usage()
        return 2

    wasm_path = Path(sys.argv[1])
    wasm = wasm_path.read_bytes()
    if not wasm_magic_ok(wasm):
        print(f"{wasm_path}: invalid wasm magic (got {wasm[:4]!r})", file=sys.stderr)
        return 1
    print(f"{wasm_path}: {len(wasm)} bytes, magic={wasm[:4]!r}")

    node = shutil.which("node")
    if not node:
        print("verify_wasm_js: node not found in PATH", file=sys.stderr)
        return 127

    proc = subprocess.run(
        [node, "-e", NODE_COMPILE_SNIPPET, str(wasm_path)],
        capture_output=True,
        text=True,
    )
    print(proc.stdout.strip() or proc.stderr.strip())
    return proc.returncode


if __name__ == "__main__":
    raise SystemExit(main())
