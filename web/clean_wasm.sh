#!/usr/bin/env bash
# Remove WASM artifacts copied into web/ by update_wasm.sh.
set -euo pipefail
cd "$(dirname "$0")"
rm -f dds_mvp_wasm.js dds_mvp_wasm.wasm dds_mvp_wasm_bin.js
echo "Removed web/dds_mvp_wasm.{js,wasm,bin.js} (if present)"
