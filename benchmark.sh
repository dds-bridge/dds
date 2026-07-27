#!/usr/bin/env bash
# Thin wrapper around python/tests/benchmark.py for ./benchmark.sh invocation.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec python3 "$SCRIPT_DIR/python/tests/benchmark.py" "$@"
