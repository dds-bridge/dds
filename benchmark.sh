#!/usr/bin/env bash
# Compare dtest performance: DDS 3.0 (this repo) vs DDS 2.9 (libdds).
#
# Runs all combinations of solver (calc, solve) and hand file
# (list1/10/100/1000), then prints per-run timings and a summary.
# Does not pass -n to dtest (library default thread count).
#
# Usage:
#   ./bench_dtest.sh
#   REPEATS=3 ./bench_dtest.sh
#   DTEST_30=/path/to/dtest DTEST_29=/path/to/dtest ./bench_dtest.sh
#
# Environment:
#   DTEST_30   Path to DDS 3.0 dtest (default: bazel-bin in this repo)
#   DTEST_29   Path to DDS 2.9 dtest
#   HANDS_DIR  Directory containing list*.txt files (default: ./hands)
#   REPEATS    Runs per combination per binary (default: 1)
#   DRY_RUN    If 1, print commands only

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DTEST_30="${DTEST_30:-$ROOT/bazel-bin/library/tests/dtest}"
DTEST_29="${DTEST_29:-/Users/adamw/src/bridge-hackathon/dds/libdds/.build/test/dtest}"
HANDS_DIR="${HANDS_DIR:-$ROOT/hands}"
REPEATS="${REPEATS:-1}"
DRY_RUN="${DRY_RUN:-0}"

SOLVERS=(calc solve)
FILES=(list1.txt list10.txt list100.txt)
# FILES=(list1.txt list10.txt list100.txt list1000.txt)

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Compare dtest on DDS 3.0 vs 2.9 across all solver/file combinations.

Options:
  -h, --help    Show this help
  -n REPEATS    Runs per combination per binary (default: $REPEATS)

Environment:
  DTEST_30, DTEST_29, HANDS_DIR, REPEATS, DRY_RUN

Examples:
  ./bench_dtest.sh
  ./bench_dtest.sh -n 5
  DRY_RUN=1 ./bench_dtest.sh
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -n)
      shift
      REPEATS="${1:?missing value for -n}"
      shift
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ ! -x "$DTEST_30" ]]; then
  echo "error: DDS 3.0 dtest not found or not executable: $DTEST_30" >&2
  echo "hint: bazel build //library/tests:dtest" >&2
  exit 1
fi

if [[ ! -x "$DTEST_29" ]]; then
  echo "error: DDS 2.9 dtest not found or not executable: $DTEST_29" >&2
  exit 1
fi

for f in "${FILES[@]}"; do
  if [[ ! -f "$HANDS_DIR/$f" ]]; then
    echo "error: hand file not found: $HANDS_DIR/$f" >&2
    exit 1
  fi
done

branch="unknown"
if git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  branch="$(git -C "$ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)"
fi

RESULTS="$(mktemp)"
trap 'rm -f "$RESULTS"' EXIT

parse_dtest_output() {
  awk '
    /User time \(ms\)/ { user = $4 }
    /Sys time \(ms\)/  { sys = $4 }
    /Avg user time \(ms\)/ { avg = $5 }
    /Ratio/ { ratio = $2 }
    END {
      if (user == "") user = "NA"
      if (sys == "") sys = "NA"
      if (avg == "") avg = "NA"
      if (ratio == "") ratio = "NA"
      print user, sys, avg, ratio
    }
  '
}

run_dtest() {
  local binary="$1"
  local solver="$2"
  local hands="$3"

  if [[ "$DRY_RUN" == "1" ]]; then
    echo "DRY_RUN: $binary -f $hands -s $solver" >&2
    echo "0 0 0.00 0.00"
    return 0
  fi

  local out
  if ! out="$("$binary" -f "$hands" -s "$solver" 2>&1)"; then
    echo "error: dtest failed: $binary -f $hands -s $solver" >&2
    echo "$out" >&2
    exit 1
  fi
  parse_dtest_output <<<"$out"
}

echo "DDS dtest benchmark"
echo "==================="
echo "3.0 binary: $DTEST_30"
echo "2.9 binary: $DTEST_29"
echo "hands dir:  $HANDS_DIR"
echo "branch:     $branch"
echo "repeats:    $REPEATS"
echo

printf "%-6s %-12s %4s %8s %8s %10s %6s %s\n" \
  "solver" "file" "ver" "user_ms" "sys_ms" "avg_user" "ratio" "run"
printf "%-6s %-12s %4s %8s %8s %10s %6s %s\n" \
  "------" "------------" "----" "--------" "--------" "----------" "------" "---"

total_runs=$(( ${#SOLVERS[@]} * ${#FILES[@]} * 2 * REPEATS ))
run_no=0

for solver in "${SOLVERS[@]}"; do
  for file in "${FILES[@]}"; do
    hands="$HANDS_DIR/$file"
    for pair in "2.9:$DTEST_29" "3.0:$DTEST_30"; do
      ver="${pair%%:*}"
      bin="${pair#*:}"

      for (( rep = 1; rep <= REPEATS; rep++ )); do
        run_no=$((run_no + 1))
        if [[ "$REPEATS" -gt 1 ]]; then
          run_label="${rep}/${REPEATS}"
        else
          run_label="1/1"
        fi

        read -r user sys avg ratio < <(run_dtest "$bin" "$solver" "$hands")

        printf "%-6s %-12s %4s %8s %8s %10s %6s %s\n" \
          "$solver" "$file" "$ver" "$user" "$sys" "$avg" "$ratio" "$run_label"

        printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
          "$solver" "$file" "$ver" "$rep" "$user" "$sys" "$avg" "$ratio" \
          >>"$RESULTS"
      done
    done
  done
done

echo
echo "Summary (3.0 vs 2.9, user time)"
echo "==============================="
printf "%-6s %-12s %10s %10s %10s %s\n" \
  "solver" "file" "2.9_user" "3.0_user" "speedup" "note"
printf "%-6s %-12s %10s %10s %10s %s\n" \
  "------" "------------" "----------" "----------" "----------" "----"

awk -F'\t' '
  {
    base = $1 SUBSEP $2
    if ($3 == "2.9") {
      s29[base] += $5
      c29[base]++
    } else if ($3 == "3.0") {
      s30[base] += $5
      c30[base]++
    }
  }
  END {
    split("calc solve", solvers, " ")
    split("list1.txt list10.txt list100.txt list1000.txt", files, " ")

    for (si = 1; si <= 2; si++) {
      for (fi = 1; fi <= 4; fi++) {
        base = solvers[si] SUBSEP files[fi]
        if (!(base in c29) || !(base in c30)) continue
        u29 = s29[base] / c29[base]
        u30 = s30[base] / c30[base]
        speedup = (u30 > 0) ? u29 / u30 : 0
        note = (speedup >= 1) ? "3.0 faster" : "2.9 faster"
        printf "%-6s %-12s %10.1f %10.1f %9.2fx %s\n",
          solvers[si], files[fi], u29, u30, speedup, note
      }
    }
  }
' "$RESULTS"

echo
echo "Completed $run_no runs ($total_runs expected)."
