#!/usr/bin/env python3
"""Verify MVP wasm bytes compile under Node."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def main() -> int:
    wasm_path = Path(sys.argv[1])
    wasm = wasm_path.read_bytes()
    print(f"{wasm_path}: {len(wasm)} bytes, magic={wasm[:4]!r}")

    tmp = Path("/tmp/dds_mvp_test.wasm")
    tmp.write_bytes(wasm)
    proc = subprocess.run(
        [
            "node",
            "-e",
            "const fs=require('fs');"
            "const b=fs.readFileSync(process.argv[1]);"
            "WebAssembly.compile(b).then(()=>console.log('compile OK'),"
            "e=>{console.error(e);process.exit(1);});",
            str(tmp),
        ],
        capture_output=True,
        text=True,
    )
    print(proc.stdout.strip() or proc.stderr.strip())
    return proc.returncode


if __name__ == "__main__":
    raise SystemExit(main())
