#!/usr/bin/env bash
# Run Bazel clean, then remove web/ WASM copies (not part of bazel-out).
# Usage: ./clean.sh [--expunge] [other bazel clean flags...]
set -euo pipefail
cd "$(dirname "$0")"
if command -v bazelisk >/dev/null 2>&1; then
  bazelisk clean "$@"
else
  bazel clean "$@"
fi
./web/clean_wasm.sh
