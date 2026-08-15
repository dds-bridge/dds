#!/usr/bin/env bash
# Run dtest on each hand-list file with every -s/--solver mode.
# Stops at the first dtest failure (non-zero exit or failure text on stdout).
#
# Usage:
#   utilities/src/verify_lists.sh hands/list1.txt hands/list2.txt
#   utilities/src/verify_lists.sh --dtest bazel-bin/library/tests/dtest hands/list1.txt
set -euo pipefail

usage() {
  cat >&2 <<EOF
Usage: $0 [--dtest PATH] <file>...

Run dtest on each file with every solver mode (-s solve|calc|play|par|dealerpar).
File paths are resolved relative to the current working directory.
Without --dtest, runs //library/tests:dtest from the repo root via bazelisk.
EOF
  exit 1
}

CWD="$(pwd)"

resolve_to_abs() {
  local path="$1"
  if [[ "$path" = /* ]]; then
    printf '%s\n' "$path"
  else
    printf '%s/%s\n' "$CWD" "$path"
  fi
}

display_path() {
  local abs="$1"
  local rel="${abs#"$CWD"/}"
  if [[ "$rel" != "$abs" ]]; then
    printf '%s\n' "$rel"
  else
    printf '%s\n' "$abs"
  fi
}

DTEST=""
FILES=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dtest)
      [[ $# -ge 2 ]] || usage
      DTEST="$2"
      shift 2
      ;;
    --dtest=*)
      DTEST="${1#*=}"
      shift
      ;;
    -h|--help)
      usage
      ;;
    *)
      FILES+=("$1")
      shift
      ;;
  esac
done

((${#FILES[@]} > 0)) || usage

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

USE_BAZELISK=false
if [[ -z "$DTEST" ]]; then
  USE_BAZELISK=true
else
  DTEST="$(resolve_to_abs "$DTEST")"
  if [[ ! -f "$DTEST" ]]; then
    echo "dtest binary not found: $(display_path "$DTEST")" >&2
    exit 1
  fi
fi

SOLVERS=(solve calc play par dealerpar)

# dtest exits 0 even when comparisons fail; match its failure messages.
FAIL_PAT='Difference|read_file failed|Error while parsing|GIB file only works with calc|loop_[a-z]+: .* return [0-9]+|Input file .* not found|Solver .* not found'

dtest_cmd() {
  local file="$1"
  local solver="$2"
  if $USE_BAZELISK; then
    printf 'cd %q && bazelisk run //library/tests:dtest -- -f %q -s %q\n' \
      "$ROOT" "$file" "$solver"
  else
    printf '%q ' "$DTEST" -f "$file" -s "$solver"
    echo
  fi
}

report_failure() {
  local file_display="$1"
  local file_abs="$2"
  local solver="$3"
  local out="$4"

  echo "$file_display NG"
  dtest_cmd "$file_abs" "$solver"
  cat "$out"
  rm -f "$out"
  exit 1
}

run_dtest() {
  local file_display="$1"
  local file_abs="$2"
  local solver="$3"
  local out
  out="$(mktemp "${TMPDIR:-/tmp}/verify_lists.XXXXXX")"

  if $USE_BAZELISK; then
    if ! (cd "$ROOT" && bazelisk run //library/tests:dtest -- -f "$file_abs" -s "$solver") \
      >"$out" 2>&1; then
      report_failure "$file_display" "$file_abs" "$solver" "$out"
    fi
  elif ! "$DTEST" -f "$file_abs" -s "$solver" >"$out" 2>&1; then
    report_failure "$file_display" "$file_abs" "$solver" "$out"
  fi

  if grep -Eq "$FAIL_PAT" "$out"; then
    report_failure "$file_display" "$file_abs" "$solver" "$out"
  fi
  rm -f "$out"
}

for file_arg in "${FILES[@]}"; do
  file_abs="$(resolve_to_abs "$file_arg")"
  file_display="$(display_path "$file_abs")"

  if [[ ! -f "$file_abs" ]]; then
    echo "Input file not found: $file_display" >&2
    exit 1
  fi

  for solver in "${SOLVERS[@]}"; do
    run_dtest "$file_display" "$file_abs" "$solver"
  done

  echo "$file_display OK"
done
