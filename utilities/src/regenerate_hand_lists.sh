#!/usr/bin/env bash
# Build create_list_for_dtest and regenerate listN.txt for every list<number>.txt
# size into OUT_DIR (default: a fresh directory under /tmp). Does not overwrite
# hands/listN.txt in the repository unless OUT_DIR points there.
#
# Convention: listNNN.txt uses --seed NNN (see create_list_for_dtest docstring).
#
# Usage:
#   ./utilities/src/regenerate_hand_lists.sh
#   ./utilities/src/regenerate_hand_lists.sh --cards 5
#   OUT_DIR=/tmp/my-lists ./utilities/src/regenerate_hand_lists.sh
set -euo pipefail

CARDS=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --cards)
      if [[ $# -lt 2 ]]; then
        echo "Missing value for --cards (expected 1–13)" >&2
        exit 1
      fi
      CARDS="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
HANDS_DIR="$ROOT/hands"
cd "$ROOT"

OUT_DIR="${OUT_DIR:-/tmp/dds-hand-lists-$(date +%Y%m%d-%H%M%S)}"

echo "Output directory: $OUT_DIR"
mkdir -p "$OUT_DIR"

counts=()
while IFS= read -r n; do
  counts+=("$n")
done < <(
  for f in "$HANDS_DIR"/list*.txt; do
    [[ -f "$f" ]] || continue
    base="${f##*/}"
    base="${base%.txt}"
    if [[ "$base" =~ ^list([0-9]+)$ ]]; then
      echo "${BASH_REMATCH[1]}"
    fi
  done | sort -n | uniq
)

if ((${#counts[@]} == 0)); then
  echo "No list<number>.txt files found in $HANDS_DIR." >&2
  exit 1
fi

echo "Counts to generate: ${counts[*]}"

bazelisk build //python/utilities:create_list_for_dtest
GEN="$ROOT/bazel-bin/python/utilities/create_list_for_dtest"

for n in "${counts[@]}"; do
  out="$OUT_DIR/list${n}.txt"
  echo "Generating list${n}.txt (--seed ${n}) -> $out"
  gen_args=(-n "$n" --seed "$n")
  if [[ -n "$CARDS" ]]; then
    gen_args+=(--cards "$CARDS")
  fi
  gen_args+=(-o "$out")
  "$GEN" "${gen_args[@]}"
done

echo "Done. Wrote ${#counts[@]} files to $OUT_DIR"
ls -lh "$OUT_DIR"/list*.txt
