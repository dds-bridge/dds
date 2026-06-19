#!/usr/bin/env bash
# Benchmark dtest performance on one or two binaries.
#
# Runs all combinations of solver (calc, solve) and hand file
# (list1/10/100/1000/10000), then prints per-run timings and an optional summary.
# Does not pass -n to dtest (library default thread count).
#
# Usage:
#   ./benchmark.sh
#   ./benchmark.sh --build
#   ./benchmark.sh --build --dtest2 /path/to/other/dtest
#   REPEATS=3 ./benchmark.sh
#
# Environment:
#   DTEST1     Path to first dtest (default: bazel-bin in this repo)
#   DTEST2     Optional second dtest for comparison
#   HANDS_DIR  Directory containing list*.txt files (default: ./hands)
#   REPEATS    Runs per combination per binary (default: 1)
#   MAX_DEALS  Include listN.txt files where N <= this value (default: 100)
#   DRY_RUN    If 1, print commands only

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DTEST1="${DTEST1:-$ROOT/bazel-bin/library/tests/dtest}"
HANDS_DIR="${HANDS_DIR:-$ROOT/hands}"
REPEATS="${REPEATS:-1}"
MAX_DEALS="${MAX_DEALS:-100}"
DRY_RUN="${DRY_RUN:-0}"
BUILD=0

SOLVERS=(calc solve)

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Benchmark dtest across solver/file combinations. With --dtest2, compare two binaries.

Options:
  -h, --help          Show this help
  -n REPEATS          Runs per combination per binary (default: $REPEATS)
  --max-deals N       Include list10^n.txt files with 10^n <= N (default: $MAX_DEALS)
                      (alias: --max_deals)
  --build             Run bazel build //library/tests:dtest before benchmarking
  --dtest1 PATH       First dtest binary (default: $DTEST1)
  --dtest2 PATH       Optional second dtest binary for comparison

Environment:
  DTEST1, DTEST2, HANDS_DIR, REPEATS, MAX_DEALS, DRY_RUN

Examples:
  ./benchmark.sh
  ./benchmark.sh --build
  ./benchmark.sh --dtest2 /path/to/dtest
  ./benchmark.sh -n 5 --dtest2 /path/to/dtest
  DRY_RUN=1 ./benchmark.sh
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
    --max-deals|--max_deals|-max-deals|-max_deals)
      shift
      MAX_DEALS="${1:?missing value for --max-deals}"
      shift
      ;;
    --build)
      BUILD=1
      shift
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if ! [[ "$MAX_DEALS" =~ ^[0-9]+$ ]] || (( MAX_DEALS < 1 )); then
  echo "error: max_deals must be a positive integer (got: $MAX_DEALS)" >&2
  exit 1
fi

select_hand_files() {
  is_power_of_10() {
    local n="$1"
    (( n >= 1 )) || return 1
    while (( n > 1 )); do
      (( n % 10 == 0 )) || return 1
      n=$(( n / 10 ))
    done
    return 0
  }

  local -a candidates=()
  local path base count

  shopt -s nullglob
  for path in "$HANDS_DIR"/list*.txt; do
    base="${path##*/}"
    if [[ "$base" =~ ^list([0-9]+)\.txt$ ]]; then
      count="${BASH_REMATCH[1]}"
      if is_power_of_10 "$count" && (( count <= MAX_DEALS )); then
        candidates+=("${count}:${base}")
      fi
    fi
  done
  shopt -u nullglob

  if ((${#candidates[@]} == 0)); then
    echo "error: no list10^n.txt files with n <= $MAX_DEALS in $HANDS_DIR" >&2
    exit 1
  fi

  FILES=()
  local item
  while IFS= read -r item; do
    FILES+=("${item#*:}")
  done < <(printf '%s\n' "${candidates[@]}" | sort -t: -k1,1n)
}

select_hand_files

if [[ "$BUILD" == "1" ]]; then
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "DRY_RUN: (cd $ROOT && bazel build //library/tests:dtest)" >&2
  else
    echo "Building //library/tests:dtest..." >&2
    (cd "$ROOT" && bazel build //library/tests:dtest)
  fi
fi

if [[ ! -x "$DTEST1" ]]; then
  echo "error: dtest1 not found or not executable: $DTEST1" >&2
  echo "hint: bazel build //library/tests:dtest" >&2
  exit 1
fi

if [[ -n "${DTEST2:-}" && ! -x "$DTEST2" ]]; then
  echo "error: dtest2 not found or not executable: $DTEST2" >&2
  exit 1
fi

BIN_PAIRS=("dtest1:$DTEST1")
if [[ -n "${DTEST2:-}" ]]; then
  BIN_PAIRS=("dtest2:$DTEST2" "dtest1:$DTEST1")
fi
num_bins=${#BIN_PAIRS[@]}

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
if [[ -n "${DTEST2:-}" ]]; then
  echo "dtest2:     $DTEST2"
fi
echo "hands dir:  $HANDS_DIR"
echo "max_deals:  $MAX_DEALS"
echo "files:      ${FILES[*]}"
echo "branch:     $branch"
echo "repeats:    $REPEATS"
echo

printf "%-6s %-12s %4s %8s %8s %10s %6s %s\n" \
  "solver" "file" "ver" "user_ms" "sys_ms" "avg_user" "ratio" "run"
printf "%-6s %-12s %4s %8s %8s %10s %6s %s\n" \
  "------" "------------" "----" "--------" "--------" "----------" "------" "---"

total_runs=$(( ${#SOLVERS[@]} * ${#FILES[@]} * num_bins * REPEATS ))
run_no=0

for solver in "${SOLVERS[@]}"; do
  for file in "${FILES[@]}"; do
    hands="$HANDS_DIR/$file"
    for pair in "${BIN_PAIRS[@]}"; do
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

if [[ -n "${DTEST2:-}" ]]; then
  echo
  echo "Summary (dtest1 vs dtest2, user time)"
  echo "====================================="
  printf "%-6s %-12s %10s %10s %10s %s\n" \
    "solver" "file" "dtest2_user" "dtest1_user" "speedup" "note"
  printf "%-6s %-12s %10s %10s %10s %s\n" \
    "------" "------------" "----------" "----------" "----------" "----"

  awk -F'\t' -v files="${FILES[*]}" '
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
      nfiles = split(files, filearr, " ")

      for (si = 1; si <= 2; si++) {
        for (fi = 1; fi <= nfiles; fi++) {
          base = solvers[si] SUBSEP filearr[fi]
          if (!(base in c2) || !(base in c1)) continue
          u2 = s2[base] / c2[base]
          u1 = s1[base] / c1[base]
          speedup = (u1 > 0) ? u2 / u1 : 0
          note = (speedup >= 1) ? "dtest1 faster" : "dtest2 faster"
          printf "%-6s %-12s %10.1f %10.1f %9.2fx %s\n",
            solvers[si], filearr[fi], u2, u1, speedup, note
        }
      }
    }
  ' "$RESULTS"
fi

echo
echo "Completed $run_no runs ($total_runs expected)."
