#!/usr/bin/env bash
set -euo pipefail

ROOT=$(pwd)
OUTDIR="$ROOT/copilot/perf/artifacts"
mkdir -p "$OUTDIR"

echo "Building legacy dtest..."
bazel build //library/tests:dtest --compilation_mode=opt --define=new_heuristic=false

echo "Running legacy dtest (no move-logging instrumentation)..."
bazel-bin/library/tests/dtest --file hands/list10.txt > "$OUTDIR/dtest-legacy.log" 2>&1

echo "Building new-heuristic dtest..."
bazel build //library/tests:dtest --compilation_mode=opt --define=new_heuristic=true

echo "Running new dtest (no move-logging instrumentation)..."
bazel-bin/library/tests/dtest --file hands/list10.txt > "$OUTDIR/dtest-new.log" 2>&1

echo "Note: JSONL move-logging steps removed; reintroduce logging stub if needed."
