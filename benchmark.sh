#!/usr/bin/env bash
# Compare dtest performance between two binaries (e.g. DDS 3.0 vs 2.9).
#
# Runs all combinations of solver (calc, solve) and hand file
# (list1/10/100/1000), then prints per-run timings and a summary.
# Does not pass -n to dtest (library default thread count).
#
# Usage:
#   ./benchmark.sh --dtest2 /path/to/other/dtest
#   REPEATS=3 ./benchmark.sh --dtest2 /path/to/other/dtest
#   DTEST1=/path/to/dtest1 DTEST2=/path/to/dtest2 ./benchmark.sh
#
# Environment:
#   DTEST1     Path to first dtest (default: bazel-bin in this repo)
#   DTEST2     Path to second dtest (required unless --dtest2 is given)
#   HANDS_DIR  Directory containing list*.txt files (default: ./hands)
#   REPEATS    Runs per combination per binary (default: 1)
#   DRY_RUN    If 1, print commands only

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DTEST1="${DTEST1:-$ROOT/bazel-bin/library/tests/dtest}"
HANDS_DIR="${HANDS_DIR:-$ROOT/hands}"
REPEATS="${REPEATS:-1}"
DRY_RUN="${DRY_RUN:-0}"

SOLVERS=(calc solve)
FILES=(list1.txt list10.txt list100.txt)
# FILES=(list1.txt list10.txt list100.txt list1000.txt)

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Compare dtest on two binaries across all solver/file combinations.

Options:
  -h, --help       Show this help
  -n REPEATS       Runs per combination per binary (default: $REPEATS)
  --dtest1 PATH    First dtest binary (default: $DTEST1)
  --dtest2 PATH    Second dtest binary (required if DTEST2 is unset)

Environment:
  DTEST1, DTEST2, HANDS_DIR, REPEATS, DRY_RUN

Examples:
  ./benchmark.sh --dtest2 /path/to/dtest
  ./benchmark.sh -n 5 --dtest2 /path/to/dtest
  DRY_RUN=1 ./benchmark.sh --dtest2 /path/to/dtest
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
    --dtest1)
      shift
      DTEST1="${1:?missing value for --dtest1}"
      shift
      ;;
    --dtest2)
      shift
      DTEST2="${1:?missing value for --dtest2}"
      shift
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -z "${DTEST2:-}" ]]; then
  echo "error: dtest2 required (use --dtest2 PATH or set DTEST2)" >&2
  usage >&2
  exit 1
fi

if [[ ! -x "$DTEST1" ]]; then
  echo "error: dtest1 not found or not executable: $DTEST1" >&2
  echo "hint: bazel build //library/tests:dtest" >&2
  exit 1
fi

if [[ ! -x "$DTEST2" ]]; then
  echo "error: dtest2 not found or not executable: $DTEST2" >&2
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
echo "dtest1:     $DTEST1"
echo "dtest2:     $DTEST2"
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
    for pair in "dtest2:$DTEST2" "dtest1:$DTEST1"; do
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
echo "Summary (dtest1 vs dtest2, user time)"
echo "====================================="
printf "%-6s %-12s %10s %10s %10s %s\n" \
  "solver" "file" "dtest2_user" "dtest1_user" "speedup" "note"
printf "%-6s %-12s %10s %10s %10s %s\n" \
  "------" "------------" "----------" "----------" "----------" "----"

awk -F'\t' '
  {
    base = $1 SUBSEP $2
    if ($3 == "dtest2") {
      s2[base] += $5
      c2[base]++
    } else if ($3 == "dtest1") {
      s1[base] += $5
      c1[base]++
    }
  }
  END {
    split("calc solve", solvers, " ")
    split("list1.txt list10.txt list100.txt list1000.txt", files, " ")

    for (si = 1; si <= 2; si++) {
      for (fi = 1; fi <= 4; fi++) {
        base = solvers[si] SUBSEP files[fi]
        if (!(base in c2) || !(base in c1)) continue
        u2 = s2[base] / c2[base]
        u1 = s1[base] / c1[base]
        speedup = (u1 > 0) ? u2 / u1 : 0
        note = (speedup >= 1) ? "dtest1 faster" : "dtest2 faster"
        printf "%-6s %-12s %10.1f %10.1f %9.2fx %s\n",
          solvers[si], files[fi], u2, u1, speedup, note
      }
    }
  }
' "$RESULTS"

echo
echo "Completed $run_no runs ($total_runs expected)."
