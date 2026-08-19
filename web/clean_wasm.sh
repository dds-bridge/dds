#!/usr/bin/env bash
# Remove WASM artifacts copied into web/ by update_wasm.sh.
set -euo pipefail
cd "$(dirname "$0")"
rm -f dds_web_wasm.js dds_web_wasm.wasm dds_web_wasm_bin.js
echo "Removed web/dds_web_wasm.{js,wasm,bin.js} (if present)"
