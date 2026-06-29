#!/usr/bin/env bash
# Benchmark dtest performance on one or two binaries.
#
# Runs all combinations of solver (solve, calc) and hand file
# (list100/1000/…/1), largest files first. Always prints a summary. Per-run
# timing rows and build (git/bazel) output are shown only with --details;
# otherwise build output is captured to a log (surfaced only on failure) and
# per-run rows are suppressed. Does not pass dtest options unless given after
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
#   BRANCH     Path to branch dtest (default: bazel-bin under the current dir)
#   COMPARE    Optional second dtest binary for comparison
#              (or use --branch NAME to build the compare binary from a git branch)
#   HANDS_DIR  Directory containing list*.txt files (default: ./hands)
#   REPEATS    Runs per combination per binary (default: 1)
#   MAX_DEALS  Include list10^n.txt files with 10^n <= N (default: 100)
#   DRY_RUN    If 1, print commands only
#   DETAILS    If 1, keep per-run rows and build output (default: 0, summary only)
#   EPSILON    With --compare, max % diff to treat branch/compare as equal (default: 0.5)

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
BRANCH_NAMES=()
COMPARE_GIVEN=0
REPEATS_GIVEN=0
DTEST_EXTRA=()

# Cleanup state (set later). The EXIT trap restores the original git branch if
# --branch switched away, and removes temp files.
RESULTS=""
ORIG_BRANCH=""
COMPARE_TMP=""
BRANCH_TMP=""
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
  [[ -n "$COMPARE_TMP" ]] && rm -f "$COMPARE_TMP"
  [[ -n "$BRANCH_TMP" ]] && rm -f "$BRANCH_TMP"
  [[ -n "$BUILD_LOG" ]] && rm -f "$BUILD_LOG"
  [[ -n "$RESULTS" ]] && rm -f "$RESULTS"
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
  --branch NAME       Git branch to build and compare ("." means the current branch).
                      Once: compare the current branch against NAME. Twice (--branch A
                      --branch B): compare A vs B and ignore the current branch. With
                      --compare PATH: build NAME as the branch binary and compare it
                      against PATH, ignoring the current branch. Each branch is checked
                      out, dtest is built and its binary saved; the original branch is
                      then restored. Requires a clean tree.
  --compare PATH      Second dtest binary (summary; transient progress on tty). May be
                      combined with a single --branch NAME (NAME backs the branch binary).
  --details           Keep per-run timing rows and build (git/bazel) output
  --epsilon PCT       With --compare, treat timings within PCT% as equal (default: 0.5; env: EPSILON)
  --reverse           With --compare, run compare before branch each repeat (default: branch first)
  --                  End benchmark options; remaining args are passed to dtest
                      (e.g. -- -n 8 -r for 8 threads and slow-board report)

Environment:
  BRANCH, COMPARE, HANDS_DIR, REPEATS, MAX_DEALS, DRY_RUN, DETAILS, EPSILON

Examples:
  ./benchmark.sh
  ./benchmark.sh --build
  ./benchmark.sh -- -n 8
  ./benchmark.sh --repeats 3 -- -n 4 -r
  ./benchmark.sh --branch develop
  ./benchmark.sh --branch develop --branch opus-two-percent
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
      BRANCH_NAMES+=("${1:?missing value for --branch}")
      shift
      ;;
    --compare)
      shift
      COMPARE="${1:?missing value for --compare}"
      COMPARE_GIVEN=1
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

# A compare binary supplied via the COMPARE env var behaves like --compare PATH.
if [[ "$COMPARE_GIVEN" == "0" && -n "${COMPARE:-}" ]]; then
  COMPARE_GIVEN=1
fi

num_branches=${#BRANCH_NAMES[@]}

if [[ "$REVERSE" == "1" && -z "${COMPARE:-}" && "$num_branches" -eq 0 ]]; then
  echo "error: --reverse requires --compare or --branch" >&2
  exit 1
fi

if [[ "$num_branches" -gt 0 && "$COMPARE_GIVEN" == "1" && "$num_branches" -ne 1 ]]; then
  echo "error: --compare accepts exactly one --branch (got $num_branches)" >&2
  exit 1
fi

if [[ "$num_branches" -gt 2 ]]; then
  echo "error: --branch may be given at most twice (got $num_branches)" >&2
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

# Branch-mode binary selection:
#   --branch NAME                : branch = current checkout, compare = build(NAME)
#   --branch NAME --compare PATH : branch = build(NAME), compare = PATH
#                                  (the current branch's dtest is ignored)
#   --branch A --branch B        : branch = build(A), compare = build(B)
#                                  (the current branch's dtest is ignored)
setup_branches() {
  if ! git -C "$ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "error: --branch requires a git work tree at $ROOT" >&2
    exit 1
  fi

  ORIG_BRANCH="$(git -C "$ROOT" symbolic-ref --quiet --short HEAD 2>/dev/null \
    || git -C "$ROOT" rev-parse HEAD)"

  # "." is shorthand for the current branch.
  local i
  for i in "${!BRANCH_NAMES[@]}"; do
    if [[ "${BRANCH_NAMES[$i]}" == "." ]]; then
      BRANCH_NAMES[$i]="$ORIG_BRANCH"
    fi
  done

  local name
  for name in "${BRANCH_NAMES[@]}"; do
    if ! git -C "$ROOT" rev-parse --verify --quiet "$name" >/dev/null; then
      echo "error: --branch: unknown git ref '$name'" >&2
      exit 1
    fi
  done
  # Untracked files can also block a checkout ("would be overwritten"), so treat
  # any working tree change (tracked or untracked) as non-clean.
  if [[ -n "$(git -C "$ROOT" status --porcelain --untracked-files=normal)" ]]; then
    echo "error: working tree not clean; commit, stash, or remove changes (tracked or untracked) before using --branch" >&2
    exit 1
  fi

  if [[ "$COMPARE_GIVEN" == "1" ]]; then
    # --branch NAME --compare PATH: build NAME as the branch binary and keep the
    # user-supplied compare path. The current branch's dtest is not used.
    BRANCH_TMP="$(mktemp "${TMPDIR:-/tmp}/dds-dtest-branch.XXXXXX")"
    build_branch_binary "${BRANCH_NAMES[0]}" "$BRANCH_TMP"
    if [[ "$DRY_RUN" == "1" ]]; then
      echo "DRY_RUN: git -C $ROOT checkout $ORIG_BRANCH" >&2
    else
      echo "Restoring '$ORIG_BRANCH'..." >&2
      run_build git -C "$ROOT" checkout "$ORIG_BRANCH"
    fi
    BRANCH="$BRANCH_TMP"
  elif [[ "$num_branches" -eq 1 ]]; then
    COMPARE_TMP="$(mktemp "${TMPDIR:-/tmp}/dds-dtest-compare.XXXXXX")"
    build_branch_binary "${BRANCH_NAMES[0]}" "$COMPARE_TMP"
    # Restore the current branch and rebuild it as the branch binary.
    if [[ "$DRY_RUN" == "1" ]]; then
      echo "DRY_RUN: git -C $ROOT checkout $ORIG_BRANCH" >&2
      echo "DRY_RUN: (cd $ROOT && bazel build //library/tests:dtest)" >&2
    else
      echo "Restoring '$ORIG_BRANCH' and rebuilding..." >&2
      run_build checkout_and_build "$ORIG_BRANCH"
    fi
    COMPARE="$COMPARE_TMP"
  else
    # Two branches: build both, ignore the current branch's binary.
    BRANCH_TMP="$(mktemp "${TMPDIR:-/tmp}/dds-dtest-branch.XXXXXX")"
    COMPARE_TMP="$(mktemp "${TMPDIR:-/tmp}/dds-dtest-compare.XXXXXX")"
    build_branch_binary "${BRANCH_NAMES[0]}" "$BRANCH_TMP"
    build_branch_binary "${BRANCH_NAMES[1]}" "$COMPARE_TMP"
    if [[ "$DRY_RUN" == "1" ]]; then
      echo "DRY_RUN: git -C $ROOT checkout $ORIG_BRANCH" >&2
    else
      echo "Restoring '$ORIG_BRANCH'..." >&2
      run_build git -C "$ROOT" checkout "$ORIG_BRANCH"
    fi
    BRANCH="$BRANCH_TMP"
    COMPARE="$COMPARE_TMP"
  fi
}

if [[ "$num_branches" -gt 0 ]]; then
  setup_branches
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

  # Wrap with `time -p` so we can capture wall-clock elapsed for this run. Its
  # "real/user/sys" lines go to the merged output but do not collide with the
  # dtest lines parse_dtest_output looks for.
  #
  local out
  if ! out="$(command time -p "${cmd[@]}" 2>&1)"; then
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
  local wall
  wall="$(awk '/^real[[:space:]]/ { print $2; exit }' <<<"$out")"
  [[ -z "$wall" ]] && wall="NA"
  echo "$parsed $wall"
}

show_run_lines=1

print_run_table_header() {
  printf "%-6s %-13s %-12s %8s %8s %10s %6s %s\n" \
    "solver" "file" "ver" "user_ms" "sys_ms" "avg_user" "ratio" "run"
  printf "%-6s %-13s %-12s %8s %8s %10s %6s %s\n" \
    "------" "-------------" "------------" "--------" "--------" "----------" "------" "---"
}

print_run_row() {
  printf "%-6s %-13s %-12s %8s %8s %10s %6s %s\n" \
    "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8"
}

echo "DDS dtest benchmark"
echo "==================="
# Branch names backing each binary, when --branch was used. With one --branch
# the branch binary is the current checkout; with two it is the first name.
branch_branch_name=""
compare_branch_name=""
if [[ "$num_branches" -eq 1 && "$COMPARE_GIVEN" == "1" ]]; then
  # --branch NAME --compare PATH: NAME backs the branch binary; compare is a path.
  branch_branch_name="${BRANCH_NAMES[0]}"
elif [[ "$num_branches" -eq 1 ]]; then
  compare_branch_name="${BRANCH_NAMES[0]}"
elif [[ "$num_branches" -eq 2 ]]; then
  branch_branch_name="${BRANCH_NAMES[0]}"
  compare_branch_name="${BRANCH_NAMES[1]}"
fi

# Labels for the per-run details "ver" column: prefer the backing branch name
# when known (the current git branch for the branch binary, the --branch name
# for the compare binary), falling back to the generic "branch"/"compare".
branch_ver_label="branch"
if [[ -n "$branch_branch_name" ]]; then
  branch_ver_label="$branch_branch_name"
elif [[ -n "$git_branch" && "$git_branch" != "unknown" ]]; then
  branch_ver_label="$git_branch"
fi
compare_ver_label="compare"
if [[ -n "$compare_branch_name" ]]; then
  compare_ver_label="$compare_branch_name"
fi

if [[ -n "$branch_branch_name" ]]; then
  printf "%-12s %s\n" "branch:" "branch '$branch_branch_name' ($BRANCH)"
else
  printf "%-12s %s\n" "branch:" "$BRANCH"
fi
if [[ -n "${COMPARE:-}" ]]; then
  if [[ -n "$compare_branch_name" ]]; then
    printf "%-12s %s\n" "compare:" "branch '$compare_branch_name' ($COMPARE)"
  else
    printf "%-12s %s\n" "compare:" "$COMPARE"
  fi
  if [[ "$DETAILS" == "1" ]]; then
    printf "%-12s %s\n" "details:" "on (per-run rows + build output)"
  else
    printf "%-12s %s\n" "details:" "off (summary only)"
  fi
  if [[ "$REVERSE" == "1" ]]; then
    printf "%-12s %s\n" "run order:" "interleaved compare, branch"
  else
    printf "%-12s %s\n" "run order:" "interleaved branch, compare"
  fi
  printf "%-12s %s\n" "epsilon:" "${EPSILON}%"
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
  printf '\033[?1049h\033[H\033[2J' >/dev/tty   # enter alt screen, home, clear
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

      for pair in "${BIN_PAIRS[@]}"; do
        ver="${pair%%:*}"
        bin="${pair#*:}"
        run_no=$((run_no + 1))

        if [[ "$DRY_RUN" == "1" ]]; then
          run_dtest "$bin" "$solver" "$hands"
          continue
        fi

        read -r user sys avg ratio wall < <(run_dtest "$bin" "$solver" "$hands")

        if [[ "$show_run_lines" == "1" ]]; then
          if [[ "$ver" == "compare" ]]; then
            ver_disp="$compare_ver_label"
          else
            ver_disp="$branch_ver_label"
          fi
          print_run_row "$solver" "$file" "${ver_disp:0:12}" \
            "$user" "$sys" "$avg" "$ratio" "$run_label"
        fi

        printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
          "$solver" "$file" "$ver" "$rep" "$user" "$sys" "$avg" "$ratio" "$wall" \
          >>"$RESULTS"
      done
    done
  done
done

# Return to the normal screen, discarding the live dtest output, before the
# summary so the final output is just the header and the summary.
if [[ "$ALT_SCREEN" == "1" ]]; then
  printf '\033[?1049l' >/dev/tty
  ALT_SCREEN_ACTIVE=0
fi

if [[ -n "${COMPARE:-}" && "$DRY_RUN" != "1" ]]; then
  echo  # compare summary (two binaries)
  # Column headers default to the generic labels, but show the actual branch
  # names when known: the current git branch for the branch binary, and the
  # --branch name for the compare binary. Truncated to the 12-char column.
  cmp_label="compare_avg"
  if [[ -n "$compare_branch_name" ]]; then
    cmp_label="${compare_branch_name:0:12}"
  fi
  br_label="branch_avg"
  if [[ -n "$branch_branch_name" ]]; then
    br_label="${branch_branch_name:0:12}"
  elif [[ -n "$git_branch" && "$git_branch" != "unknown" ]]; then
    br_label="${git_branch:0:12}"
  fi

  # Names used in the "note" column ("<name> faster"); fall back to the generic
  # "branch"/"compare" when no branch name is known.
  note_branch="branch"
  if [[ -n "$branch_branch_name" ]]; then
    note_branch="$branch_branch_name"
  elif [[ -n "$compare_branch_name" && -n "$git_branch" && "$git_branch" != "unknown" ]]; then
    note_branch="$git_branch"
  fi
  note_compare="compare"
  if [[ -n "$compare_branch_name" ]]; then
    note_compare="$compare_branch_name"
  fi

  echo "Summary (avg user ms)"
  echo "=============================================================================="
  printf "%-6s %-13s %12s %12s %10s %-15s\n" \
    "solver" "file" "$cmp_label" "$br_label" "cmp/branch" "note"
  printf "%-6s %-13s %12s %12s %10s %-15s\n" \
    "------" "-------------" "------------" "------------" "----------" "---------------"

  awk -F'\t' -v files="${FILES[*]}" -v epsilon_pct="$EPSILON" \
      -v note_branch="$note_branch" -v note_compare="$note_compare" '
    function within_epsilon(a, b,    eps, hi, lo) {
      eps = epsilon_pct / 100
      if (a > b) { hi = a; lo = b } else { hi = b; lo = a }
      return (hi <= 0 || (hi - lo) / hi <= eps)
    }
    {
      base = $1 SUBSEP $2
      if ($3 == "compare") {
        s2[base] += $7
        c2[base]++
        if ($9 != "NA") tw2 += $9   # total wall-clock elapsed, compare
      } else if ($3 == "branch") {
        s1[base] += $7
        c1[base]++
        if ($9 != "NA") tw1 += $9   # total wall-clock elapsed, branch
      }
    }
    END {
      split("solve calc", solvers, " ")
      nfiles = split(files, filearr, " ")

      for (si = 1; si <= 2; si++) {
        for (fi = 1; fi <= nfiles; fi++) {
          base = solvers[si] SUBSEP filearr[fi]
          if (!(base in c2) || !(base in c1)) continue
          # Every member of s1, c1, s2, and c2 should be positive.
          # If not, it will be due to rounding to zero. To fix, update
          # TestTimer.cpp to accumulate microseconds rather than milliseconds. 
          u2 = s2[base] / c2[base]
          u1 = s1[base] / c1[base]
          cmp_branch = u2 / u1
          if (within_epsilon(u1, u2)) {
            note = "equal"
          } else if (cmp_branch >= 1) {
            note = note_branch " faster"
          } else {
            note = note_compare " faster"
          }
          sp = sprintf("%9.2fx", cmp_branch)
          printf "%-6s %-13s %12.2f %12.2f %10s %-15s\n",
            solvers[si], filearr[fi], u2, u1, sp, note
        }
      }

      # Total elapsed (wall-clock seconds) summed per binary across all runs.
      printf "%-6s %-13s %12s %12s %10s %-15s\n",
        "------", "-------------", "------------", "------------", "----------", "---------------"
      if (tw2 > 0 && tw1 > 0) {
        tnote = ""
        if (within_epsilon(tw1, tw2)) tnote = "equal"
        else if (tw2 / tw1 >= 1) tnote = note_branch " faster"
        else tnote = note_compare " faster"
        tsp = sprintf("%9.2fx", tw2 / tw1)
        printf "%-6s %-13s %12.2f %12.2f %10s %-15s\n",
          "TOTAL", "elapsed (s)", tw2, tw1, tsp, tnote
      } else {
        printf "%-6s %-13s %12.2f %12.2f %10s %-15s\n",
          "TOTAL", "elapsed (s)", tw2, tw1, "", ""
      }
    }
  ' "$RESULTS"
elif [[ "$DRY_RUN" != "1" ]]; then
  echo  # single-binary summary (no --compare)
  br_label="branch_avg"
  if [[ -n "$git_branch" && "$git_branch" != "unknown" ]]; then
    br_label="${git_branch:0:12}"
  fi

  echo "Summary (avg user ms)"
  echo "============================================================"
  printf "%-6s %-13s %12s %6s\n" "solver" "file" "$br_label" "runs"
  printf "%-6s %-13s %12s %6s\n" \
    "------" "-------------" "------------" "------"

  awk -F'\t' -v files="${FILES[*]}" '
    $3 == "branch" {
      base = $1 SUBSEP $2
      s[base] += $7
      c[base]++
      if ($9 != "NA") tw += $9   # total wall-clock elapsed
    }
    END {
      split("solve calc", solvers, " ")
      nfiles = split(files, filearr, " ")
      for (si = 1; si <= 2; si++) {
        for (fi = 1; fi <= nfiles; fi++) {
          base = solvers[si] SUBSEP filearr[fi]
          if (!(base in c)) continue
          printf "%-6s %-13s %12.2f %6d\n",
            solvers[si], filearr[fi], s[base] / c[base], c[base]
        }
      }
      printf "%-6s %-13s %12s %6s\n",
        "------", "-------------", "------------", "------"
      printf "%-6s %-13s %12.2f %6s\n", "TOTAL", "elapsed (s)", tw, ""
    }
  ' "$RESULTS"
fi

echo
if [[ "$DRY_RUN" == "1" ]]; then
  echo "DRY_RUN: $total_runs dtest invocations (not run)."
else
  echo "Completed $run_no runs ($total_runs expected)."
fi
