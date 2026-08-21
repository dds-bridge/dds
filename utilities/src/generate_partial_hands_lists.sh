#!/usr/bin/env bash

# This script generates a complete set of listNNN.txt files
# containing deals of fewer than 13 cards per hand.
# The files aremainly used for benchmarking.

export PARENT_DIR=./hands/partial

mkdir -p $PARENT_DIR

for CARDS in $(seq -w 1 12); do
  OUT_DIR="${PARENT_DIR}/${CARDS}_cards" \
    ./utilities/src/regenerate_hand_lists.sh \
    --cards $CARDS
done

mv ${PARENT_DIR}/01_cards/ ${PARENT_DIR}/01_card/
