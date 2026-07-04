#!/usr/bin/env bash
# Benchmark dtest performance across one or more binaries.
#
# Runs all combinations of solver (solve, calc) and hand file
# (list100/1000/…/1), largest files first. Always prints a summary. Per-run
# timing rows and build (git/bazel) output are shown transiently, then hidden
# unless the --details flag is set. Passes dtest options if given after
# "--" (see below).
#
# Usage:
#   ./benchmark.sh
#   ./benchmark.sh --build
#   ./benchmark.sh -- -n 8 -r
#   ./benchmark.sh --build --compare /path/to/other/dtest
#   ./benchmark.sh --branch develop -- -n 8
#   ./benchmark.sh --compare /path/to/other/dtest --epsilon 1
#   ./benchmark.sh --repeats 5 -- -n 4
#   REPEATS=3 ./benchmark.sh
#
# Environment:
#   BRANCH     Path to the baseline dtest (default: bazel-bin under the current dir)
#   COMPARE    Optional extra dtest binary to benchmark (like a trailing --compare)
#   HANDS_DIR  Directory containing list*.txt files (default: ./hands)
#   REPEATS    Runs per combination per binary (default: 1)
#   MAX_DEALS  Include list10^n.txt files with 10^n <= N (default: 100)
#   DRY_RUN    If 1, print commands only
#   DETAILS    If 1, keep per-run rows and build output (default: 0, summary only)
#   EPSILON    For a two-binary comparison, max % diff treated as equal (default: 0.5)

set -euo pipefail

# Operate on the current working directory (the repo you invoke this from),
# not the directory the script happens to live in.
ROOT="$(pwd)"

# Fail fast if ROOT is not the root of a DDS checkout: the git/bazel operations
# and the default branch-binary and hands paths are all relative to it.
if [[ ! -f "$ROOT/MODULE.bazel" || ! -f "$ROOT/library/tests/BUILD.bazel" ]]; then
  echo "error: '$ROOT' is not a DDS checkout root" >&2
  echo "       cd to the root of the dds repository before running benchmark.sh" >&2
  exit 1
fi
BRANCH="${BRANCH:-$ROOT/bazel-bin/library/tests/dtest}"
HANDS_DIR="${HANDS_DIR:-$ROOT/hands}"
REPEATS="${REPEATS:-1}"
MAX_DEALS="${MAX_DEALS:-100}"
DRY_RUN="${DRY_RUN:-0}"
DETAILS="${DETAILS:-0}"
EPSILON="${EPSILON:-0.5}"
BUILD=0
REVERSE=0
# Ordered list of binaries to benchmark, captured in command-line order. Each
# --branch/--compare appends one spec; the first spec is the baseline.
SPEC_KINDS=()   # parallel: "branch" (git ref) or "compare" (path to a binary)
SPEC_VALS=()
CLI_COMPARE_GIVEN=0
REPEATS_GIVEN=0
DTEST_EXTRA=()

# Cleanup state (set later). The EXIT trap restores the original git branch if
# --branch switched away, and removes temp files.
RESULTS=""
TIME_FILE=""
ORIG_BRANCH=""
TMP_BINS=()
BUILD_LOG=""

cleanup() {
  # Leave the alternate screen first so any restore/error messages and the shell
  # prompt land on the normal screen.
  if [[ "${ALT_SCREEN_ACTIVE:-0}" == "1" ]]; then
    printf '\033[?1049l' >/dev/tty 2>/dev/null || true
    ALT_SCREEN_ACTIVE=0
  fi
  if [[ -n "$ORIG_BRANCH" ]]; then
    local cur
    cur="$(git -C "$ROOT" symbolic-ref --quiet --short HEAD 2>/dev/null \
      || git -C "$ROOT" rev-parse HEAD 2>/dev/null || true)"
    if [[ -n "$cur" && "$cur" != "$ORIG_BRANCH" ]]; then
      echo "Restoring git branch '$ORIG_BRANCH'..." >&2
      git -C "$ROOT" checkout "$ORIG_BRANCH" >/dev/null 2>&1 || true
    fi
  fi
  if ((${#TMP_BINS[@]} > 0)); then
    rm -f "${TMP_BINS[@]}"
  fi
  [[ -n "$BUILD_LOG" ]] && rm -f "$BUILD_LOG"
  [[ -n "$RESULTS" ]] && rm -f "$RESULTS"
  [[ -n "$TIME_FILE" ]] && rm -f "$TIME_FILE"
  return 0
}
trap cleanup EXIT

SOLVERS=(solve calc)

usage() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Benchmark dtest across solver/file combinations. Always prints a summary; use
--details for per-run rows and build (git/bazel) output.

Options:
  -h, --help          Show this help
  --repeats N         Runs per combination per binary (default: 1; env: REPEATS)
  --max-deals N       Include list10^n.txt files with 10^n <= N (default: 100; env: MAX_DEALS)
                      (alias: --max_deals)
  --build             Build branch dtest only (bazel build //library/tests:dtest)
  --branch NAME       Git branch to build and benchmark ("." means the current branch).
                      Repeatable. Each named branch is checked out, dtest is built and
                      its binary saved, then the original branch is restored (requires a
                      clean tree).
  --compare PATH      Path to a prebuilt dtest binary to benchmark. Repeatable.
  --details           Keep per-run timing rows and build (git/bazel) output
  --epsilon PCT       For a two-binary comparison, treat timings within PCT% as equal
                      (default: 0.5; env: EPSILON)
  --reverse           Reverse the per-repeat dispatch order of the binaries
  --                  End benchmark options; remaining args are passed to dtest
                      (e.g. -- -n 8 -r for 8 threads and slow-board report)

--branch and --compare may be given any number of times and combined freely; the
binaries are benchmarked in the order specified and the first is the baseline. The
current checkout is benchmarked by default only when neither flag is given. With two
binaries the summary adds a ratio and a "faster" note; with three or more it shows
only the per-binary averages (no note).

Environment:
  BRANCH, COMPARE, HANDS_DIR, REPEATS, MAX_DEALS, DRY_RUN, DETAILS, EPSILON

Examples:
  ./benchmark.sh
  ./benchmark.sh --build
  ./benchmark.sh -- -n 8
  ./benchmark.sh --repeats 3 -- -n 4 -r
  ./benchmark.sh --branch develop
  ./benchmark.sh --branch develop --branch opus-two-percent
  ./benchmark.sh --branch develop --branch opus-two-percent --branch fastest
  ./benchmark.sh --branch opus-two-percent --compare /path/to/dtest
  ./benchmark.sh --branch develop --repeats 3 -- -n 8
  ./benchmark.sh --compare /path/to/dtest
  ./benchmark.sh --compare /path/to/dtest --details
  ./benchmark.sh --compare /path/to/dtest --epsilon 1
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
      if (( REPEATS_GIVEN )); then
        echo "error: --repeats may be given only once" >&2
        exit 1
      fi
      REPEATS_GIVEN=1
      shift
      REPEATS="${1:?missing value for --repeats}"
      shift
      ;;
    --branch)
      shift
      val="${1:?missing value for --branch}"
      if [[ "$val" == -* ]]; then
        echo "error: --branch ref must not start with '-': $val" >&2
        exit 1
      fi
      SPEC_KINDS+=("branch")
      SPEC_VALS+=("$val")
      shift
      ;;
    --compare)
      shift
      SPEC_KINDS+=("compare")
      SPEC_VALS+=("${1:?missing value for --compare}")
      CLI_COMPARE_GIVEN=1
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
    --details)
      DETAILS=1
      shift
      ;;
    --epsilon)
      shift
      EPSILON="${1:?missing value for --epsilon}"
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

if ! [[ "$EPSILON" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
  echo "error: epsilon must be a non-negative number (got: $EPSILON)" >&2
  exit 1
fi

# A compare binary supplied via the COMPARE env var behaves like a trailing
# --compare PATH, unless --compare was already given on the command line.
if [[ "$CLI_COMPARE_GIVEN" == "0" && -n "${COMPARE:-}" ]]; then
  SPEC_KINDS+=("compare")
  SPEC_VALS+=("$COMPARE")
fi

nspecs=${#SPEC_KINDS[@]}
nbranch=0
if (( nspecs > 0 )); then
  for k in "${SPEC_KINDS[@]}"; do
    [[ "$k" == "branch" ]] && nbranch=$((nbranch + 1))
  done
fi

if [[ "$REVERSE" == "1" && "$nspecs" -lt 2 ]]; then
  echo "error: --reverse requires at least two binaries (two or more --branch/--compare)" >&2
  exit 1
fi

# Check out $1, build dtest, and copy the binary to $2.
# Build output (git checkout + bazel) is noise unless --details was given.
# Trying to show it live and erase it afterward with ANSI does not work: bazel
# drives its own cursor save/restore for its progress display, clobbering any
# saved position, so a restore+clear erases nothing. Instead, capture the
# output to a log and surface it only on failure (or with --details). bazel sees
# a non-tty here and emits plain, line-based output. The short "Building..."
# labels are kept as progress markers.
bazel_dtest() { ( cd "$ROOT" && bazel build //library/tests:dtest ); }
checkout_and_build() { git -C "$ROOT" checkout "$1" && bazel_dtest; }

run_build() {
  if [[ "$DETAILS" == "1" ]]; then
    "$@"
    return
  fi
  if [[ -z "$BUILD_LOG" ]]; then
    BUILD_LOG="$(mktemp "${TMPDIR:-/tmp}/dds-dtest-build.XXXXXX")"
  fi
  if ! "$@" >"$BUILD_LOG" 2>&1; then
    cat "$BUILD_LOG" >&2
    return 1
  fi
}

build_branch_binary() {
  local name="$1" dest="$2"
  local dtest_rel="bazel-bin/library/tests/dtest"
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "DRY_RUN: git -C $ROOT checkout $name" >&2
    echo "DRY_RUN: (cd $ROOT && bazel build //library/tests:dtest)" >&2
    echo "DRY_RUN: cp -L $ROOT/$dtest_rel $dest" >&2
    return 0
  fi
  echo "Building dtest from '$name'..." >&2
  run_build checkout_and_build "$name"
  cp -L "$ROOT/$dtest_rel" "$dest"
  chmod +x "$dest"
}

# Display label for a compare binary given by path: its basename, or the raw
# path if basename is somehow empty.
label_for_path() {
  local b
  b="$(basename -- "$1")"
  [[ -n "$b" ]] && printf '%s' "$b" || printf '%s' "$1"
}

# Label for the current checkout's binary: the git branch, else "branch".
current_label() {
  if [[ -n "$git_branch" && "$git_branch" != "unknown" ]]; then
    printf '%s' "$git_branch"
  else
    printf 'branch'
  fi
}

# Validate the git work tree and branch refs before any checkout. Only called
# when at least one --branch spec is present.
git_prep_for_branches() {
  if ! git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "error: --branch requires a git work tree at $ROOT" >&2
    exit 1
  fi
  ORIG_BRANCH="$(git -C "$ROOT" symbolic-ref --quiet --short HEAD 2>/dev/null \
    || git -C "$ROOT" rev-parse HEAD)"

  local i
  # "." is shorthand for the current branch.
  for i in "${!SPEC_VALS[@]}"; do
    if [[ "${SPEC_KINDS[$i]}" == "branch" && "${SPEC_VALS[$i]}" == "." ]]; then
      SPEC_VALS[$i]="$ORIG_BRANCH"
    fi
  done
  for i in "${!SPEC_VALS[@]}"; do
    if [[ "${SPEC_KINDS[$i]}" == "branch" ]] \
       && ! git -C "$ROOT" rev-parse --verify --quiet -- "${SPEC_VALS[$i]}" >/dev/null; then
      echo "error: --branch: unknown git ref '${SPEC_VALS[$i]}'" >&2
      exit 1
    fi
  done
  # Untracked files can also block a checkout ("would be overwritten"), so treat
  # any working tree change (tracked or untracked) as non-clean.
  if [[ -n "$(git -C "$ROOT" status --porcelain --untracked-files=normal)" ]]; then
    echo "error: working tree not clean; commit, stash, or remove changes (tracked or untracked) before using --branch" >&2
    exit 1
  fi
}

# Restore the original branch. Pass 1 to also rebuild dtest (needed only when
# the current checkout's bazel-bin binary is used as a baseline).
restore_branch() {
  local rebuild="$1"
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "DRY_RUN: git -C $ROOT checkout $ORIG_BRANCH" >&2
    [[ "$rebuild" == "1" ]] && \
      echo "DRY_RUN: (cd $ROOT && bazel build //library/tests:dtest)" >&2
    return 0
  fi
  if [[ "$rebuild" == "1" ]]; then
    echo "Restoring '$ORIG_BRANCH' and rebuilding..." >&2
    run_build checkout_and_build "$ORIG_BRANCH"
  else
    echo "Restoring '$ORIG_BRANCH'..." >&2
    run_build git -C "$ROOT" checkout "$ORIG_BRANCH"
  fi
}

new_tmp_bin() {
  local t
  t="$(mktemp "${TMPDIR:-/tmp}/dds-dtest-bin.XXXXXX")"
  TMP_BINS+=("$t")
  printf '%s' "$t"
}

# Build the ordered list of binaries to benchmark into BIN_LABELS / BIN_PATHS.
# Binary-set selection:
#   (no spec)          : one binary, the current checkout (the default)
#   one or more specs  : exactly those specs, in order; the checkout is ignored
# The first binary is the baseline. --branch specs are built from git; --compare
# specs are existing binary paths.
build_binaries() {
  BIN_LABELS=()
  BIN_PATHS=()

  if (( nspecs == 0 )); then
    BIN_PATHS=("$BRANCH")
    BIN_LABELS=("$(current_label)")
    return 0
  fi

  if (( nbranch > 0 )); then
    git_prep_for_branches
  fi

  # Benchmark exactly the specified binaries, in order; the current checkout is
  # only used when no spec is given at all.
  local i kind val t
  for i in "${!SPEC_KINDS[@]}"; do
    kind="${SPEC_KINDS[$i]}"
    val="${SPEC_VALS[$i]}"
    if [[ "$kind" == "branch" ]]; then
      t="$(new_tmp_bin)"
      build_branch_binary "$val" "$t"
      BIN_PATHS+=("$t")
      BIN_LABELS+=("$val")
    else
      BIN_PATHS+=("$val")
      BIN_LABELS+=("$(label_for_path "$val")")
    fi
  done
  if (( nbranch > 0 )); then
    restore_branch 0
  fi
}

git_branch="unknown"
if git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git_branch="$(git -C "$ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)"
fi

build_binaries
num_bins=${#BIN_PATHS[@]}
if (( nspecs == 0 || nbranch == nspecs )); then
  RUN_LABEL_COL="branch"
elif (( nbranch == 0 )); then
  RUN_LABEL_COL="binary"
else
  RUN_LABEL_COL="label"
fi
if (( nbranch > 0 )); then
  BUILD=0  # build already done as part of the branch workflow
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
    run_build bazel_dtest
  fi
fi

if [[ "$DRY_RUN" != "1" ]]; then
  for i in "${!BIN_PATHS[@]}"; do
    if [[ ! -x "${BIN_PATHS[$i]}" ]]; then
      echo "error: binary not found or not executable: ${BIN_PATHS[$i]} (${BIN_LABELS[$i]})" >&2
      (( i == 0 )) && echo "hint: bazel build //library/tests:dtest" >&2
      exit 1
    fi
  done
fi

# Dispatch order of the binaries within each (solver, file, repeat). The
# baseline (index 0) leads by default; --reverse flips it. The summary keys on
# the stored binary index, so order never affects the reported results.
RUN_ORDER=()
for (( i = 0; i < num_bins; i++ )); do
  RUN_ORDER+=("$i")
done
if [[ "$REVERSE" == "1" ]]; then
  rev=()
  for (( i = num_bins - 1; i >= 0; i-- )); do
    rev+=("${RUN_ORDER[$i]}")
  done
  RUN_ORDER=("${rev[@]}")
fi

for f in "${FILES[@]}"; do
  if [[ ! -f "$HANDS_DIR/$f" ]]; then
    echo "error: hand file not found: $HANDS_DIR/$f" >&2
    exit 1
  fi
done

RESULTS="$(mktemp "${TMPDIR:-/tmp}/dds-benchmark.XXXXXX")"
TIME_FILE="$(mktemp "${TMPDIR:-/tmp}/dds-benchmark-time.XXXXXX")"
# Removal handled by the cleanup() EXIT trap installed near the top.

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

  # Bash time keyword (TIMEFORMAT=%R) captures wall-clock seconds on a side
  # channel; dtest stdout/stderr stay separate for parse_dtest_output.
  local out wall
  local TIMEFORMAT='%R'
  if ! { time out="$("${cmd[@]}" 2>&1)"; } 2>"$TIME_FILE"; then
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
  wall="$(<"$TIME_FILE")"
  [[ -z "$wall" ]] && wall="NA"
  echo "$parsed $wall"
}

show_run_lines=1

print_run_table_header() {
  printf "%-6s %-13s %-12s %8s %8s %10s %6s %s\n" \
    "solver" "file" "$RUN_LABEL_COL" "user_ms" "sys_ms" "avg_user" "ratio" "run"
  printf "%-6s %-13s %-12s %8s %8s %10s %6s %s\n" \
    "------" "-------------" "------------" "--------" "--------" "----------" "------" "---"
}

print_run_row() {
  printf "%-6s %-13s %-12s %8s %8s %10s %6s %s\n" \
    "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8"
}

echo "DDS dtest benchmark"
echo "==================="
# One line per binary, baseline first. The label is the branch name (--branch),
# the binary's basename (--compare), or the current git branch (checkout).
for i in "${!BIN_PATHS[@]}"; do
  tag="binary $((i + 1)):"
  (( i == 0 )) && tag="baseline:"
  printf "%-12s %s\n" "$tag" "${BIN_LABELS[$i]}  (${BIN_PATHS[$i]})"
done
if (( num_bins >= 2 )); then
  if [[ "$DETAILS" == "1" ]]; then
    printf "%-12s %s\n" "details:" "on (per-run rows + build output)"
  else
    printf "%-12s %s\n" "details:" "off (summary only)"
  fi
  order_str=""
  for idx in "${RUN_ORDER[@]}"; do
    order_str+="${order_str:+, }${BIN_LABELS[$idx]}"
  done
  printf "%-12s %s\n" "run order:" "interleaved $order_str"
  # The note column (and thus epsilon) only applies to a two-binary comparison.
  if (( num_bins == 2 )); then
    printf "%-12s %s\n" "epsilon:" "${EPSILON}%"
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

# The per-run rows are the script's live progress. With --details they are kept
# in the final output. Without --details, on a tty, they are shown on the
# alternate screen so the user sees progress, then discarded when we switch back
# to the main screen just before the summary; off a tty they are suppressed
# (summary only), since there is nothing to hide them.
show_run_lines=0
ALT_SCREEN=0
if [[ "$DRY_RUN" != "1" ]]; then
  if [[ "$DETAILS" == "1" ]]; then
    show_run_lines=1
  elif [[ -t 1 ]]; then
    show_run_lines=1
    ALT_SCREEN=1
  fi
fi

if [[ "$ALT_SCREEN" == "1" ]]; then
  printf '\033[?1049h\033[H\033[2J'   # enter alt screen, home, clear
  ALT_SCREEN_ACTIVE=1
fi

if [[ "$DRY_RUN" != "1" && "$show_run_lines" == "1" ]]; then
  print_run_table_header
fi

total_runs=$(( ${#SOLVERS[@]} * ${#FILES[@]} * num_bins * REPEATS ))
run_no=0

for solver in "${SOLVERS[@]}"; do
  for file in "${FILES[@]}"; do
    hands="$HANDS_DIR/$file"

    for (( rep = 1; rep <= REPEATS; rep++ )); do
      if [[ "$REPEATS" -gt 1 ]]; then
        run_label="${rep}/${REPEATS}"
      else
        run_label="1/1"
      fi

      for idx in "${RUN_ORDER[@]}"; do
        bin="${BIN_PATHS[$idx]}"
        run_no=$((run_no + 1))

        if [[ "$DRY_RUN" == "1" ]]; then
          run_dtest "$bin" "$solver" "$hands"
          continue
        fi

        read -r user sys avg ratio wall < <(run_dtest "$bin" "$solver" "$hands")

        if [[ "$show_run_lines" == "1" ]]; then
          print_run_row "$solver" "$file" "${BIN_LABELS[$idx]:0:12}" \
            "$user" "$sys" "$avg" "$ratio" "$run_label"
        fi

        # Column 3 is the binary index; the summary keys on it.
        printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
          "$solver" "$file" "$idx" "$rep" "$user" "$sys" "$avg" "$ratio" "$wall" \
          >>"$RESULTS"
      done
    done
  done
done

# Return to the normal screen, discarding the live dtest output, before the
# summary so the final output is just the header and the summary.
if [[ "$ALT_SCREEN" == "1" ]]; then
  printf '\033[?1049l'
  ALT_SCREEN_ACTIVE=0
fi

if [[ "$DRY_RUN" != "1" ]]; then
  echo
  echo "Summary (avg user ms)"
  echo "=============================================================================="

  # One avg column per binary (baseline first). A ratio + note column are added
  # only for a two-binary comparison; with three or more binaries the note has
  # no single meaning and is dropped, leaving just the per-binary averages.
  # Labels go through the environment (ENVIRON) rather than -v: BSD awk rejects
  # newlines in a -v value, and labels are newline-joined.
  labels_joined="$(printf '%s\n' "${BIN_LABELS[@]}")"

  LABELS="$labels_joined" awk -F'\t' -v nb="$num_bins" \
      -v files="${FILES[*]}" -v eps="$EPSILON" '
    function within_epsilon(a, b,    e, hi, lo) {
      e = eps / 100
      if (a > b) { hi = a; lo = b } else { hi = b; lo = a }
      return (hi <= 0 || (hi - lo) / hi <= e)
    }
    function L(b)  { return substr(lab[b + 1], 1, 12) }   # truncated, for columns
    function Lf(b) { return lab[b + 1] }                  # full, for notes
    function pdash(   b) {
      printf "%-6s %-13s", "------", "-------------"
      for (b = 0; b < nb; b++) printf " %12s", "------------"
      if (nb == 2) printf " %10s %-15s", "----------", "---------------"
      printf "\n"
    }
    BEGIN { split(ENVIRON["LABELS"], lab, "\n") }   # lab[1..nb]; trailing empty ignored
    {
      base = $1 SUBSEP $2
      b = $3 + 0
      s[base, b] += $7
      c[base, b]++
      if ($9 != "NA") tw[b] += $9   # total wall-clock elapsed, per binary
    }
    END {
      printf "%-6s %-13s", "solver", "file"
      for (b = 0; b < nb; b++) printf " %12s", L(b)
      if (nb == 2) printf " %10s %-15s", "ratio", "note"
      printf "\n"
      pdash()

      split("solve calc", solvers, " ")
      nfiles = split(files, farr, " ")
      for (si = 1; si <= 2; si++) {
        for (fi = 1; fi <= nfiles; fi++) {
          base = solvers[si] SUBSEP farr[fi]
          ok = 1
          for (b = 0; b < nb; b++) if (!((base, b) in c)) ok = 0
          if (!ok) continue
          # Every average should be positive; a zero is rounding and means
          # TestTimer.cpp should accumulate microseconds, not milliseconds.
          printf "%-6s %-13s", solvers[si], farr[fi]
          for (b = 0; b < nb; b++) { u[b] = s[base, b] / c[base, b]; printf " %12.2f", u[b] }
          if (nb == 2) {
            r = u[1] / u[0]
            if (within_epsilon(u[0], u[1])) note = "equal"
            else if (r >= 1) note = Lf(0) " faster"
            else note = Lf(1) " faster"
            printf " %10s %-15s", sprintf("%.2fx", r), note
          }
          printf "\n"
        }
      }

      pdash()
      # Total elapsed (wall-clock seconds) summed per binary across all runs.
      printf "%-6s %-13s", "TOTAL", "elapsed (s)"
      allpos = 1
      for (b = 0; b < nb; b++) { printf " %12.2f", tw[b] + 0; if (!(tw[b] > 0)) allpos = 0 }
      if (nb == 2) {
        if (allpos) {
          r = tw[1] / tw[0]
          if (within_epsilon(tw[0], tw[1])) tnote = "equal"
          else if (r >= 1) tnote = Lf(0) " faster"
          else tnote = Lf(1) " faster"
          printf " %10s %-15s", sprintf("%.2fx", r), tnote
        } else {
          printf " %10s %-15s", "", ""
        }
      }
      printf "\n"
    }
  ' "$RESULTS"
fi

echo
if [[ "$DRY_RUN" == "1" ]]; then
  echo "DRY_RUN: $total_runs dtest invocations (not run)."
else
  echo "Completed $run_no runs ($total_runs expected)."
fi
