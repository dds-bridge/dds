#!/usr/bin/env bash
# Exercises benchmark.sh's --branch dirty-tree gate: a clean tree is required
# only when a --branch ref would check out a different commit than HEAD.
set -euo pipefail

fail() { echo "FAIL: $*" >&2; exit 1; }
pass() { echo "PASS: $*"; }

resolve_benchmark() {
  # Under bazel test, locate via runfiles. Otherwise, use the script directory.
  if [[ -n "${TEST_SRCDIR:-}${RUNFILES_DIR:-}${RUNFILES_MANIFEST_FILE:-}" ]]; then
    # --- begin runfiles.bash initialization v3 ---
    set -uo pipefail; set +e; f=bazel_tools/tools/bash/runfiles/runfiles.bash
    # shellcheck disable=SC1090
    source "${RUNFILES_DIR:-/dev/null}/$f" 2>/dev/null || \
      source "$(grep -sm1 "^$f " "${RUNFILES_MANIFEST_FILE:-/dev/null}" | cut -f2- -d' ')" 2>/dev/null || \
      source "$0.runfiles/$f" 2>/dev/null || \
      source "$(grep -sm1 "^$f " "$0.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
      source "$(grep -sm1 "^$f " "$0.exe.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
      { echo>&2 "ERROR: cannot find $f"; exit 1; }; f=; set -e
    # --- end runfiles.bash initialization v3 ---
    local path
    path="$(rlocation "_main/benchmark.sh" || true)"
    if [[ -n "$path" && -f "$path" ]]; then
      printf '%s' "$path"
      return 0
    fi
  fi
  local script_dir
  script_dir="$(cd "$(dirname "$0")" && pwd)"
  printf '%s' "$script_dir/benchmark.sh"
}

BENCHMARK="$(resolve_benchmark)"
[[ -f "$BENCHMARK" ]] || fail "benchmark.sh not found at $BENCHMARK"

command -v git >/dev/null 2>&1 || fail "git is required"

TMP="$(mktemp -d "${TEST_TMPDIR:-${TMPDIR:-/tmp}}/dds-benchmark-git-prep.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

git_q() {
  git -c core.fsmonitor=false -c advice.detachedHead=false "$@"
}

# Minimal two-branch repo at $1. Caller adds dirtiness (tracked and/or untracked).
setup_repo_base() {
  local repo="$1"
  mkdir -p "$repo/library/tests" "$repo/hands"
  # Minimal DDS root markers that benchmark.sh requires.
  printf 'module(name = "dds")\n' >"$repo/MODULE.bazel"
  printf '# test BUILD\n' >"$repo/library/tests/BUILD.bazel"
  printf 'hand\n' >"$repo/hands/list100.txt"

  git_q -C "$repo" init -q -b main
  git_q -C "$repo" config user.email "test@example.com"
  git_q -C "$repo" config user.name "Test"
  git_q -C "$repo" add MODULE.bazel library/tests/BUILD.bazel hands/list100.txt
  git_q -C "$repo" commit -q -m "initial"

  # Second commit on a side branch so --branch other differs from HEAD.
  git_q -C "$repo" checkout -q -b other
  echo "other" >"$repo/other.txt"
  git_q -C "$repo" add other.txt
  git_q -C "$repo" commit -q -m "other"
  git_q -C "$repo" checkout -q main

  # Tag at HEAD so --branch can name the same commit without switching.
  git_q -C "$repo" tag head-tag
}

run_bench() {
  local repo="$1"
  shift
  # Invoke via bash: Bazel runfiles may be read-only (no chmod +x).
  ( cd "$repo" && DRY_RUN=1 bash "$BENCHMARK" "$@" ) 2>&1
}

# --- Tracked dirty tree ---
setup_repo_base "$TMP/repo"
echo "dirty" >>"$TMP/repo/MODULE.bazel"

# Dirty tree + --branch for the current branch (by name) must succeed: no switch.
out="$(run_bench "$TMP/repo" --branch main)" || fail "dirty + --branch main exited $?"
[[ "$out" != *"working tree not clean"* ]] || fail "dirty + --branch main wrongly rejected: $out"
pass "dirty + --branch main allowed"

# Dirty tree + --branch . (current branch shorthand) must succeed.
out="$(run_bench "$TMP/repo" --branch .)" || fail "dirty + --branch . exited $?"
[[ "$out" != *"working tree not clean"* ]] || fail "dirty + --branch . wrongly rejected: $out"
pass "dirty + --branch . allowed"

# Dirty tree + tag at HEAD (same commit, peeled via ^{commit}) must succeed.
out="$(run_bench "$TMP/repo" --branch head-tag)" || fail "dirty + --branch head-tag exited $?"
[[ "$out" != *"working tree not clean"* ]] || fail "dirty + --branch head-tag wrongly rejected: $out"
pass "dirty + --branch head-tag allowed"

# Dirty tree + --branch other (different commit) must fail with the clean-tree error.
set +e
out="$(run_bench "$TMP/repo" --branch other 2>&1)"
rc=$?
set -e
[[ "$rc" -ne 0 ]] || fail "dirty + --branch other should fail"
[[ "$out" == *"working tree not clean"* ]] || fail "dirty + --branch other missing clean-tree error: $out"
pass "dirty + --branch other rejected"

# Non-commit revspec must fail at validation with a clear error (not a later ^{commit} crash).
set +e
out="$(run_bench "$TMP/repo" --branch 'HEAD^{tree}' 2>&1)"
rc=$?
set -e
[[ "$rc" -ne 0 ]] || fail "HEAD^{tree} should fail"
[[ "$out" == *"unknown git ref"* ]] || fail "HEAD^{tree} missing unknown-ref error: $out"
pass "non-commit revspec rejected"

# --- Untracked-only dirty tree (status --porcelain --untracked-files=normal) ---
setup_repo_base "$TMP/repo_untracked"
echo "untracked" >"$TMP/repo_untracked/scratch.txt"

# Untracked + --branch main (no switch) must succeed.
out="$(run_bench "$TMP/repo_untracked" --branch main)" \
  || fail "untracked + --branch main exited $?"
[[ "$out" != *"working tree not clean"* ]] \
  || fail "untracked + --branch main wrongly rejected: $out"
pass "untracked + --branch main allowed"

# Untracked + --branch other (switch) must fail: untracked can block checkout.
set +e
out="$(run_bench "$TMP/repo_untracked" --branch other 2>&1)"
rc=$?
set -e
[[ "$rc" -ne 0 ]] || fail "untracked + --branch other should fail"
[[ "$out" == *"working tree not clean"* ]] \
  || fail "untracked + --branch other missing clean-tree error: $out"
pass "untracked + --branch other rejected"

echo "All benchmark git-prep tests passed."
