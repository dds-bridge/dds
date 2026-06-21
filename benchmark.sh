#!/usr/bin/env bash
# Benchmark dtest performance on one or two binaries.
#
# Runs all combinations of solver (solve, calc) and hand file
# (list100/1000/…/1), largest files first, then prints per-run timings and an optional summary.
# Does not pass dtest options unless given after "--" (see below).
#
# Usage:
#   ./benchmark.sh
#   ./benchmark.sh --build
#   ./benchmark.sh -- -n 8 -r
#   ./benchmark.sh --build --compare /path/to/other/dtest
#   ./benchmark.sh --repeats 5 -- -n 4
#   REPEATS=3 ./benchmark.sh
#
# Environment:
#   BRANCH     Path to branch dtest (default: bazel-bin in this repo)
#   COMPARE    Optional second dtest binary for comparison
#   HANDS_DIR  Directory containing list*.txt files (default: ./hands)
#   REPEATS    Runs per combination per binary (default: 1)
#   MAX_DEALS  Include list10^n.txt files with 10^n <= N (default: 100)
#   DRY_RUN    If 1, print commands only

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BRANCH="${BRANCH:-$ROOT/bazel-bin/library/tests/dtest}"
HANDS_DIR="${HANDS_DIR:-$ROOT/hands}"
REPEATS="${REPEATS:-1}"
MAX_DEALS="${MAX_DEALS:-100}"
DRY_RUN="${DRY_RUN:-0}"
BUILD=0
REVERSE=0
DTEST_EXTRA=()

SOLVERS=(solve calc)

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Benchmark dtest across solver/file combinations. With --compare, compare two binaries.

Options:
  -h, --help          Show this help
  --repeats N         Runs per combination per binary (default: 1; env: REPEATS)
  --max-deals N       Include list10^n.txt files with 10^n <= N (default: 100; env: MAX_DEALS)
                      (alias: --max_deals)
  --build             Build branch dtest only (bazel build //library/tests:dtest)
  --branch PATH       Branch dtest binary (default: $BRANCH)
  --compare PATH      Optional second dtest binary for comparison
  --reverse           With --compare, run compare before branch (default: branch first)
  --                  End benchmark options; remaining args are passed to dtest
                      (e.g. -- -n 8 -r for 8 threads and slow-board report)

Environment:
  BRANCH, COMPARE, HANDS_DIR, REPEATS, MAX_DEALS, DRY_RUN

Examples:
  ./benchmark.sh
  ./benchmark.sh --build
  ./benchmark.sh -- -n 8
  ./benchmark.sh --repeats 3 -- -n 4 -r
  ./benchmark.sh --compare /path/to/dtest
  ./benchmark.sh --compare /path/to/dtest --reverse
  ./benchmark.sh --repeats 5 --compare /path/to/dtest
  DRY_RUN=1 ./benchmark.sh
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --repeats)
      shift
      REPEATS="${1:?missing value for --repeats}"
      shift
      ;;
    --branch)
      shift
      BRANCH="${1:?missing value for --branch}"
      shift
      ;;
    --compare)
      shift
      COMPARE="${1:?missing value for --compare}"
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
    --reverse)
      REVERSE=1
      shift
      ;;
    --)
      shift
      DTEST_EXTRA=("$@")
      break
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

if ! [[ "$REPEATS" =~ ^[0-9]+$ ]] || (( REPEATS < 1 )); then
  echo "error: repeats must be a positive integer (got: $REPEATS)" >&2
  exit 1
fi

if [[ "$REVERSE" == "1" && -z "${COMPARE:-}" ]]; then
  echo "error: --reverse requires --compare" >&2
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
    echo "error: no list10^n.txt files with 10^n <= $MAX_DEALS in $HANDS_DIR" >&2
    exit 1
  fi

  FILES=()
  local item
  while IFS= read -r item; do
    FILES+=("${item#*:}")
  done < <(printf '%s\n' "${candidates[@]}" | sort -t: -k1,1rn)
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

if [[ "$DRY_RUN" != "1" ]]; then
  if [[ ! -x "$BRANCH" ]]; then
    echo "error: branch binary not found or not executable: $BRANCH" >&2
    echo "hint: bazel build //library/tests:dtest" >&2
    exit 1
  fi

  if [[ -n "${COMPARE:-}" && ! -x "$COMPARE" ]]; then
    echo "error: compare binary not found or not executable: $COMPARE" >&2
    exit 1
  fi
fi

BIN_PAIRS=("branch:$BRANCH")
if [[ -n "${COMPARE:-}" ]]; then
  if [[ "$REVERSE" == "1" ]]; then
    BIN_PAIRS=("compare:$COMPARE" "branch:$BRANCH")
  else
    BIN_PAIRS=("branch:$BRANCH" "compare:$COMPARE")
  fi
fi
num_bins=${#BIN_PAIRS[@]}

for f in "${FILES[@]}"; do
  if [[ ! -f "$HANDS_DIR/$f" ]]; then
    echo "error: hand file not found: $HANDS_DIR/$f" >&2
    exit 1
  fi
done

git_branch="unknown"
if git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git_branch="$(git -C "$ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)"
fi

RESULTS="$(mktemp "${TMPDIR:-/tmp}/dds-benchmark.XXXXXX")"
trap 'rm -f "$RESULTS"' EXIT

parse_dtest_output() {
  awk '
    /^Number of hands/ { hands = $NF }
    /^User time \(ms\)/ { user = ($NF == "zero" ? 0 : $NF) }
    /^Sys time \(ms\)/  { sys = ($NF == "zero" ? 0 : $NF) }
    /^Avg user time \(ms\)/ { avg = ($NF == "zero" ? 0 : $NF) }
    /^Ratio[[:space:]]/ { ratio = $NF }
    END {
      if (user == "") user = "NA"
      if (sys == "") sys = "NA"
      if (avg == "") {
        if (user == 0) avg = 0
        else if (hands != "" && user != "NA" && hands > 0) avg = user / hands
        else avg = "NA"
      }
      if (ratio == "") ratio = "NA"
      print user, sys, avg, ratio
    }
  '
}

run_dtest() {
  local binary="$1"
  local solver="$2"
  local hands="$3"
  local -a cmd=("$binary" -f "$hands" -s "$solver")
  if ((${#DTEST_EXTRA[@]} > 0)); then
    cmd+=("${DTEST_EXTRA[@]}")
  fi

  if [[ "$DRY_RUN" == "1" ]]; then
    echo "DRY_RUN: ${cmd[*]}" >&2
    return 0
  fi

  local out
  if ! out="$("${cmd[@]}" 2>&1)"; then
    echo "error: dtest failed: ${cmd[*]}" >&2
    echo "$out" >&2
    exit 1
  fi
  local parsed
  parsed="$(parse_dtest_output <<<"$out")"
  local parsed_user parsed_sys
  read -r parsed_user parsed_sys _ _ <<<"$parsed"
  if [[ "$parsed_user" == "NA" || "$parsed_sys" == "NA" ]]; then
    echo "warning: incomplete dtest timing output: ${cmd[*]}" >&2
  fi
  echo "$parsed"
}

echo "DDS dtest benchmark"
echo "==================="
printf "%-12s %s\n" "branch:" "$BRANCH"
if [[ -n "${COMPARE:-}" ]]; then
  printf "%-12s %s\n" "compare:" "$COMPARE"
  if [[ "$REVERSE" == "1" ]]; then
    printf "%-12s %s\n" "run order:" "compare, branch"
  else
    printf "%-12s %s\n" "run order:" "branch, compare"
  fi
fi
printf "%-12s %s\n" "hands dir:" "$HANDS_DIR"
printf "%-12s %s\n" "max_deals:" "$MAX_DEALS"
printf "%-12s %s\n" "files:" "${FILES[*]}"
printf "%-12s %s\n" "git branch:" "$git_branch"
printf "%-12s %s\n" "repeats:" "$REPEATS"
if ((${#DTEST_EXTRA[@]} > 0)); then
  printf "%-12s %s\n" "dtest args:" "${DTEST_EXTRA[*]}"
fi
echo

if [[ "$DRY_RUN" != "1" ]]; then
  printf "%-6s %-13s %7s %8s %8s %10s %6s %s\n" \
    "solver" "file" "ver" "user_ms" "sys_ms" "avg_user" "ratio" "run"
  printf "%-6s %-13s %7s %8s %8s %10s %6s %s\n" \
    "------" "-------------" "-------" "--------" "--------" "----------" "------" "---"
fi

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

        if [[ "$DRY_RUN" == "1" ]]; then
          run_dtest "$bin" "$solver" "$hands"
          continue
        fi

        if [[ "$REPEATS" -gt 1 ]]; then
          run_label="${rep}/${REPEATS}"
        else
          run_label="1/1"
        fi

        read -r user sys avg ratio < <(run_dtest "$bin" "$solver" "$hands")

        printf "%-6s %-13s %7s %8s %8s %10s %6s %s\n" \
          "$solver" "$file" "$ver" "$user" "$sys" "$avg" "$ratio" "$run_label"

        printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
          "$solver" "$file" "$ver" "$rep" "$user" "$sys" "$avg" "$ratio" \
          >>"$RESULTS"
      done
    done
  done
done

if [[ -n "${COMPARE:-}" && "$DRY_RUN" != "1" ]]; then
  echo
  echo "Summary (branch vs compare, avg user ms; cmp/branch > 1 => branch faster)"
  echo "=============================================================================="
  printf "%-6s %-13s %12s %12s %10s %-15s\n" \
    "solver" "file" "compare_avg" "branch_avg" "cmp/branch" "note"
  printf "%-6s %-13s %12s %12s %10s %-15s\n" \
    "------" "-------------" "------------" "------------" "----------" "---------------"

  awk -F'\t' -v files="${FILES[*]}" '
    {
      base = $1 SUBSEP $2
      if ($3 == "compare") {
        s2[base] += $7
        c2[base]++
      } else if ($3 == "branch") {
        s1[base] += $7
        c1[base]++
      }
    }
    END {
      split("solve calc", solvers, " ")
      nfiles = split(files, filearr, " ")

      for (si = 1; si <= 2; si++) {
        for (fi = 1; fi <= nfiles; fi++) {
          base = solvers[si] SUBSEP filearr[fi]
          if (!(base in c2) || !(base in c1)) continue
          u2 = s2[base] / c2[base]
          u1 = s1[base] / c1[base]
          if (u1 == 0 && u2 == 0) {
            sp = "      n/a"
            note = "equal"
          } else if (u1 == 0) {
            sp = "      inf"
            note = "branch faster"
          } else {
            cmp_branch = u2 / u1
            if (cmp_branch >= 1) note = "branch faster"
            else note = "compare faster"
            sp = sprintf("%9.2fx", cmp_branch)
          }
          printf "%-6s %-13s %12.2f %12.2f %10s %-15s\n",
            solvers[si], filearr[fi], u2, u1, sp, note
        }
      }
    }
  ' "$RESULTS"
fi

echo
if [[ "$DRY_RUN" == "1" ]]; then
  echo "DRY_RUN: $total_runs dtest invocations (not run)."
else
  echo "Completed $run_no runs ($total_runs expected)."
fi
