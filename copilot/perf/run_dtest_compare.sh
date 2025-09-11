#!/usr/bin/env bash
set -euo pipefail

ROOT=$(pwd)
OUTDIR="$ROOT/copilot/perf/artifacts"
mkdir -p "$OUTDIR"

echo "Building legacy dtest..."
bazel build //library/tests:dtest --compilation_mode=opt --define=new_heuristic=false

LEGACY_JSONL="$OUTDIR/dtest-legacy.jsonl"
NEW_JSONL="$OUTDIR/dtest-new.jsonl"

rm -f "$LEGACY_JSONL" "$NEW_JSONL"

echo "Running legacy dtest (logging to $LEGACY_JSONL)..."
DDS_LOG_MOVES=1 DDS_MOVES_JSONL="$LEGACY_JSONL" bazel-bin/library/tests/dtest --file hands/list10.txt > "$OUTDIR/dtest-legacy.log" 2>&1

echo "Normalizing legacy log..."
python3 copilot/perf/normalize_moves_log.py "$LEGACY_JSONL" "$OUTDIR/dtest-legacy.tsv"

echo "Building new-heuristic dtest..."
bazel build //library/tests:dtest --compilation_mode=opt --define=new_heuristic=true

echo "Running new dtest (logging to $NEW_JSONL)..."
DDS_LOG_MOVES=1 DDS_MOVES_JSONL="$NEW_JSONL" bazel-bin/library/tests/dtest --file hands/list10.txt > "$OUTDIR/dtest-new.log" 2>&1

echo "Normalizing new log..."
python3 copilot/perf/normalize_moves_log.py "$NEW_JSONL" "$OUTDIR/dtest-new.tsv"

echo "Diffing normalized outputs..."
python3 copilot/perf/compare_moves.py "$OUTDIR/dtest-legacy.tsv" "$OUTDIR/dtest-new.tsv" "$OUTDIR/dtest-discrepancy-summary.txt"

echo "Wrote logs, TSVs and discrepancy summary to $OUTDIR"
