# Open findings

Reproducers for defects the harnesses have found that are **not yet fixed**.
They live here rather than in `corpus/` so the corpus-replay tests stay green;
a fuzzing campaign will rediscover them immediately, which is expected.

Move a file into the matching `corpus/` directory once its defect is fixed, so
it becomes a permanent regression seed.

## 01 — unbalanced deal reaches the search (CalcDDtable path)

**Reproduce**

```
bazel build --config=asan //library/tests/fuzz:calc_dd_table_pbn_fuzz_corpus_test
./bazel-bin/library/tests/fuzz/calc_dd_table_pbn_fuzz_corpus_test \
  library/tests/fuzz/findings/01_unbalanced_deal_51_cards.txt
```

```
AddressSanitizer: global-buffer-overflow
READ of size 2 ... in QuickTricksPartnerHandNT / weight_alloc_trump0
0x... is located 14248 bytes after global variable
  '(anonymous namespace)::rel_rank_storage'
  defined in 'library/src/lookup_tables/lookup_tables.cpp' of size 122880
```

**Cause.** `CalcDDtable()` / `CalcDDtablePBN()` do not validate that the four
hands hold equal numbers of cards. `SolveBoard()` does — `board_value_checks()`
in `solver_if.cpp` returns `RETURN_CARD_COUNT` for exactly this — so the same
malformed deal is rejected safely on that path and only the CalcDDtable path
reaches the search, where the relative-rank index runs off the end of
`rel_rank_storage`. It is an out-of-bounds *read*, not a write.

**Two ways in, both from an ordinary PBN file:**

- `01_unbalanced_deal_51_cards.txt` contains **only legal PBN characters** and
  is simply one card short (north's spades are `T8`, not `T98`) — the shape a
  hand-edited or truncated PBN file naturally takes.
- `01_unbalanced_deal_bad_rank.txt` is the same deal with a card replaced by
  `Z`. `convert_from_pbn()` silently ignores any character that is not a card,
  `.`, ` ` or a compass letter (`pbn.cpp:113-116`), so the invalid rank is
  skipped and the deal is short by one card while the parser still returns
  success.

**Fix sketch.** Two independent changes, either of which closes the crash:

1. Validate card counts in `CalcDDtable()`, mirroring `board_value_checks()`.
   This is the load-bearing fix, since case 1 uses only legal characters.
2. Make `convert_from_pbn()` reject unrecognised characters instead of
   skipping them. Do this carefully: PBN text from files often carries
   trailing newlines or `\r`, which the current loop tolerates, so tightening
   it needs an explicit whitespace allowance to avoid rejecting valid input.

## 02 — `DealerPar()` does not validate `dealer`

**Reproduce**

```
bazel build --config=asan //library/tests/fuzz:par_fuzz_corpus_test
mkdir -p /tmp/f && cp library/tests/fuzz/findings/02_dealer_par_negative_dealer.bin /tmp/f/
./bazel-bin/library/tests/fuzz/par_fuzz_corpus_test /tmp/f
```

```
AddressSanitizer: BUS on unknown address (READ)
    #5 sacrifice_as_text(int, int, int)
    #6 sacrifices_as_text(...)
    #7 DealerPar
```

**Cause.** `DealerPar()` validates its table and (since `2abb260e`) its
`vulnerable` argument, but not `dealer`. A negative `dealer` propagates into
`pno_list[]` and reaches `dealer_par.cpp:648`:

```cpp
return NUMBER_TO_CONTRACT[static_cast<unsigned>(no)] + "-" +
  NUMBER_TO_PLAYER[static_cast<unsigned>(pno)] + "-" + ...
```

The `static_cast<unsigned>` turns `pno == -1` into 4294967295, indexing far
outside the `std::string` array and reading a garbage string object. The
crashing input uses a **legal** `res_table`; only `dealer` is out of range.

**Fix sketch.** Range-check `dealer` to 0-3 in `DealerPar()` alongside the
existing `vulnerable` check — the header already documents 0 = North .. 3 =
West. The `static_cast<unsigned>` in `sacrifice_as_text()` is worth removing
too: it converts a bounds bug into a wild read rather than a negative index
that ASan or UBSan would flag more clearly.

**Note.** This is the same class as the `vulnerable` bug fixed by hand in
`2abb260e`; that fix guarded one parameter of the pair and missed the other.
The fuzzer found it within 50000 runs.

## 03 — `DumpInput()` reads out of bounds while reporting invalid input

**Reproduce**

```
bazel build --config=asan //library/tests/fuzz:solve_board_fuzz_corpus_test
mkdir -p /tmp/f && cp library/tests/fuzz/findings/03_dump_input_out_of_range_deal.bin /tmp/f/
./bazel-bin/library/tests/fuzz/solve_board_fuzz_corpus_test /tmp/f
```

```
AddressSanitizer: global-buffer-overflow
READ of size 1
    #0 DumpInput(int, Deal const&, int, int, int)
    #1 board_range_checks(Deal const&, int, int, int)
    #2 solve_board_internal(...)
    #4 SolveBoard
```

**Cause.** `board_range_checks()` correctly *detects* an out-of-range deal, then
calls `DumpInput()` to log it — and `DumpInput()` indexes the character tables
with the very values it is reporting as invalid (`dump.cpp:288-298`):

```cpp
fout << card_suit[dl.trump] << "\n";              // card_suit[DDS_STRAINS] == [5]
fout << "first=" << card_hand[dl.first] << "\n";  // card_hand[4]
  ... card_suit[dl.currentTrickSuit[k]]
  ... card_rank[dl.currentTrickRank[k]]           // card_rank[16]
```

The reproducer uses `currentTrickSuit = {7,7,7}` and `currentTrickRank =
{99,99,99}`, so `card_rank[99]` reads well past a 16-byte array. `trump` and
`first` are indexed the same way on the lines above.

**Reach.** `DumpInput()` is compiled in unless `DDS_NO_DUMP_ON_ERROR` is
defined, and the build does not define it — so this is present in release
builds, on the error path of the main solver entry point. Every `SolveBoard()`
rejection with an out-of-range `trump`, `first`, `currentTrickSuit` or
`currentTrickRank` goes through it. It is an out-of-bounds *read*.

Worth noting separately: `DumpInput()` also writes `dump.txt` into the process
working directory whenever any input is rejected, which is surprising behaviour
for a library and is a side effect a caller cannot disable at runtime.

**Fix sketch.** Bounds-check each index in `DumpInput()` before using it as a
table subscript, printing the raw integer when it is out of range — the value
is being reported *because* it is invalid, so it must never be trusted as an
index. Consider also making the `dump.txt` side effect opt-in.
