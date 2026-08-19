#!/usr/bin/env python3
"""Serve web/ with COOP/COEP so SharedArrayBuffer / WASM pthreads work."""
from __future__ import annotations

import argparse
import http.server
import sys
from pathlib import Path

# Allow `python3 web/serve_web.py` from the repo root without installing a package.
_TESTS = Path(__file__).resolve().parent / "tests"
if str(_TESTS) not in sys.path:
    sys.path.insert(0, str(_TESTS))

from web_site import CROSS_ORIGIN_ISOLATION_HEADERS, make_isolated_http_handler  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--directory",
        type=Path,
        default=Path(__file__).resolve().parent,
        help="Directory to serve (default: web/)",
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()

    directory = args.directory.resolve()
    if not (directory / "dds_web.html").is_file():
        print(f"error: {directory}/dds_web.html not found", file=sys.stderr)
        return 1

    handler = make_isolated_http_handler(directory, quiet=False)

    httpd = http.server.ThreadingHTTPServer((args.host, args.port), handler)
    print(f"Serving {directory} at http://{args.host}:{args.port}/dds_web.html")
    print(
        "Cross-origin isolation:",
        ", ".join(f"{k}: {v}" for k, v in CROSS_ORIGIN_ISOLATION_HEADERS.items()),
    )
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
