#!/usr/bin/env bash
# Copy Bazel WASM artifacts next to dds_web.html for local static serving.
set -euo pipefail
cd "$(dirname "$0")/.."
if command -v bazelisk >/dev/null 2>&1; then
  BAZEL=bazelisk
else
  BAZEL=bazel
fi
"$BAZEL" build //web:dds_web_wasm
bin="$("$BAZEL" info bazel-bin)/web"
cp -f "${bin}/dds_web_wasm.js" web/
cp -f "${bin}/dds_web_wasm.wasm" web/
chmod u+w web/dds_web_wasm.js
python3 web/gen_wasm_bin_js.py web/dds_web_wasm.wasm web/dds_web_wasm_bin.js
# Safe one-line fix for file:// (never global-replace inside the .js bundle).
python3 web/patch_web_wasm.py web/dds_web_wasm.js
python3 web/verify_wasm_js.py web/dds_web_wasm.wasm
echo "Updated web/dds_web_wasm.{js,wasm,bin.js}"
