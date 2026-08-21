#!/usr/bin/env bash

# This script generates a complete set of listNNN.txt files
# containing deals of fewer than 13 cards per hand.
# The files are mainly used for benchmarking.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$ROOT"

PARENT_DIR="${ROOT}/hands/partial"
mkdir -p "$PARENT_DIR"

for CARDS in $(seq -w 1 12); do
  OUT_DIR="${PARENT_DIR}/${CARDS}_cards" \
    ./utilities/src/regenerate_hand_lists.sh \
    --cards "$CARDS"
done

# Idempotent rename: replace 01_card if a previous run left it behind.
rm -rf "${PARENT_DIR}/01_card"
mv "${PARENT_DIR}/01_cards" "${PARENT_DIR}/01_card"
