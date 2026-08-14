#!/usr/bin/env bash
# End-to-end check for the .NET dd_table_for_deal CLI.
# Requires DDS_LIBRARY_PATH pointing at the Bazel-built shared library.
#
#   bazelisk build //jni:dds_shared
#   export DDS_LIBRARY_PATH="$(bazelisk info bazel-bin)/jni/libdds.dylib"  # .so / dds.dll
#   ./dotnet/DdTableForDeal/e2e.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

if [[ -z "${DDS_LIBRARY_PATH:-}" ]]; then
  echo "DDS_LIBRARY_PATH is not set (build //jni:dds_shared and export the lib path)" >&2
  exit 1
fi
if [[ ! -f "$DDS_LIBRARY_PATH" ]]; then
  echo "DDS_LIBRARY_PATH does not exist: $DDS_LIBRARY_PATH" >&2
  exit 1
fi

EXAMPLE_DEAL='N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93'
PROJECT=dotnet/DdTableForDeal/

assert_contains() {
  local haystack=$1
  local needle=$2
  if [[ "$haystack" != *"$needle"* ]]; then
    echo "Missing expected output: ${needle}" >&2
    echo "----- stdout -----" >&2
    printf '%s\n' "$haystack" >&2
    exit 1
  fi
}

assert_not_contains() {
  local haystack=$1
  local needle=$2
  if [[ "$haystack" == *"$needle"* ]]; then
    echo "Unexpected output: ${needle}" >&2
    echo "----- stdout -----" >&2
    printf '%s\n' "$haystack" >&2
    exit 1
  fi
}

run_and_check() {
  local label=$1
  shift
  echo "==> ${label}"

  local err_file out rc
  err_file="$(mktemp)"
  set +e
  out="$(dotnet run --project "$PROJECT" -- "$@" 2>"$err_file")"
  rc=$?
  set -e
  local err
  err="$(cat "$err_file")"
  rm -f "$err_file"

  if [[ $rc -ne 0 ]]; then
    echo "dotnet run failed (exit ${rc})" >&2
    echo "----- stderr -----" >&2
    printf '%s\n' "$err" >&2
    echo "----- stdout -----" >&2
    printf '%s\n' "$out" >&2
    exit 1
  fi
  if [[ "$err" == *"DDS error:"* || "$err" == *"Failed to load native DDS library"* ]]; then
    echo "Solver error on stderr:" >&2
    printf '%s\n' "$err" >&2
    exit 1
  fi

  assert_contains "$out" 'dd_table_for_deal:'
  assert_contains "$out" 'North'
  assert_contains "$out" '   NT     4     4     8     8'
  assert_contains "$out" '    S     3     3    10    10'
  assert_contains "$out" '    H     9     9     4     4'
  assert_contains "$out" '    D     8     8     4     4'
  assert_contains "$out" '    C     3     3     9     9'
  assert_contains "$out" 'Par: NS 5Hx -2 -300'
  assert_not_contains "$out" 'NS score:'

  local before_north before_par
  before_north=${out%%North*}
  before_par=${out%%Par:*}
  if [[ "$before_north" == "$out" || "$before_par" == "$out" \
        || ${#before_north} -ge ${#before_par} ]]; then
    echo "Expected 'North' before 'Par:' in output" >&2
    exit 1
  fi
}

run_and_check "inline PBN deal" "$EXAMPLE_DEAL"
run_and_check "hands/example.pbn" hands/example.pbn

echo "DdTableForDeal e2e OK"
