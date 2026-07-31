#!/usr/bin/env bash
# Copy Bazel WASM artifacts next to dds_mvp.html for local static serving.
set -euo pipefail
cd "$(dirname "$0")/.."
if command -v bazelisk >/dev/null 2>&1; then
  BAZEL=bazelisk
else
  BAZEL=bazel
fi
"$BAZEL" build //web:dds_mvp_wasm
bin="$("$BAZEL" info bazel-bin)/web"
cp -f "${bin}/dds_mvp_wasm.js" web/
cp -f "${bin}/dds_mvp_wasm.wasm" web/
chmod u+w web/dds_mvp_wasm.js
python3 web/gen_wasm_bin_js.py web/dds_mvp_wasm.wasm web/dds_mvp_wasm_bin.js
# Safe one-line fix for file:// (never global-replace inside the .js bundle).
python3 web/patch_mvp_wasm.py web/dds_mvp_wasm.js
python3 web/verify_wasm_js.py web/dds_mvp_wasm.wasm
echo "Updated web/dds_mvp_wasm.{js,wasm,bin.js}"
