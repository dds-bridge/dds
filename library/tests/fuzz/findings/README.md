# Findings

Reproducers for defects the harnesses have found that are **not yet fixed**
live here rather than in `corpus/`, so the corpus-replay tests stay green. A
fuzzing campaign will rediscover them immediately, which is expected.

When a defect is fixed, move its reproducer into the matching `corpus/`
directory so it becomes a permanent regression seed, and record it below.

## Open findings

**None.**

## Open hardening items

These are not memory-safety defects, but they are worth knowing about.

- `convert_from_pbn()` silently ignores any character that is not a card, `.`,
  ` ` or a compass letter (`pbn.cpp:113-116`), so a PBN string with an invalid
  rank parses "successfully" one card short rather than being refused. Since
  the `CalcDDtable*` paths now reject the resulting unbalanced deal, this is a
  diagnostics problem (`RETURN_CARD_COUNT` where `RETURN_PBN_FAULT` would be
  clearer) rather than a safety one. Tightening it needs care: PBN text from
  files often carries trailing newlines or `\r`, which the current loop
  tolerates, so a strict version needs an explicit whitespace allowance.

## Fixed

### 01 — unbalanced deal reached the search (CalcDDtable path)

`CalcDDtable()` and `CalcDDtablePBN()` did not check that the four hands held
equal numbers of cards, so a 51-card deal reached the search and read 14248
bytes past `rel_rank_storage` (`lookup_tables.cpp`, 122880 bytes) in
`weight_alloc_trump0()` / `QuickTricksPartnerHandNT()`. `SolveBoard()` rejected
the same deal via `board_value_checks()`; only the CalcDDtable path was
exposed. Reachable from an ordinary PBN file that is short of a card — the
reproducer contains **only legal PBN characters**.

Fixed by `table_deal_checks()` (`library/src/table_deal_validate.hpp`), applied
in `CalcDDtableN()`, `CalcAllTablesN()` and `CalcAllTablesX()`. It enforces the
same three rules `SolveBoard()` already did, so nothing is rejected that the
solver would have accepted.

Seeds: `corpus/calc_dd_table_pbn/unbalanced_51_cards.txt`,
`corpus/calc_dd_table_pbn/bad_rank.txt`.
Tests: `library/tests/deal_input_validation_test.cpp` (`CalcTableValidation`).

### 02 — `DealerPar()` did not validate `dealer`

A negative `dealer` propagated into `pno_list[]` and reached
`sacrifice_as_text()`, where `NUMBER_TO_PLAYER[static_cast<unsigned>(pno)]`
turned `-1` into 4294967295 and indexed a `std::string` array far out of
bounds. The crashing input used a **legal** `res_table`; only `dealer` was out
of range.

Same class as the `vulnerable` bug fixed by hand in `2abb260e`, which guarded
one parameter of the pair and missed the other. The fuzzer found it within
50000 runs.

Fixed by range-checking `dealer` in `DealerPar()`, and by replacing the
`static_cast<unsigned>` subscripts with the guarded `contract_text()` and
`player_text()` helpers — the cast is what turned a detectable bug into a wild
read.

Seed: `corpus/par/regression_negative_dealer.bin`.
Tests: `library/tests/par_validation_test.cpp` (`ParValidation`).

### 03 — `DumpInput()` read out of bounds while reporting invalid input

`board_range_checks()` correctly detected an out-of-range deal, then called
`DumpInput()` to log it — and `DumpInput()` indexed `card_suit[5]`,
`card_hand[4]` and `card_rank[16]` with the very values it was reporting as
invalid (`dump.cpp:288-298`). Present in release builds, since `DumpInput()` is
compiled in unless `DDS_NO_DUMP_ON_ERROR` is defined and the build does not
define it.

Fixed by the `suit_text()` / `hand_text()` / `rank_text()` helpers in
`dump.cpp`, which fall back to printing the raw integer when the value is out
of range — more useful in a diagnostic than a wrong character, and safe by
construction.

Still true, and unchanged here: `DumpInput()` writes `dump.txt` into the
process working directory whenever input is rejected, which is surprising for a
library. Define `DDS_NO_DUMP_ON_ERROR` to compile it out.

Seed: `corpus/solve_board/regression_out_of_range_deal.bin`.
Tests: `library/tests/deal_input_validation_test.cpp` (`DumpInputSafety`).

### 04 — `board_value_checks()` indexed with an unvalidated trick suit

Found while fuzzing the fixes for 01-03. `board_range_checks()` validates
`currentTrickSuit[k]` only when the matching `currentTrickRank[k]` is
non-zero, but `hand_rel_first` is derived from the total card count
(`hand_rel_first = (48 - ini_depth) % 4`, `solver_if.cpp:151`) rather than
from the trick entries. A deal with five cards and all trick ranks zero
therefore gives `hand_rel_first == 3`, and the loop in
`board_value_checks()` evaluates
`dl.remainCards[h][dl.currentTrickSuit[k]]` with a suit that was never
checked — a stack read far out of bounds. As with 03, the crash is inside the
validation logic itself.

Fixed by range-checking `currentTrickSuit[k]` inside that loop, where it is
actually used as a subscript, rather than in `board_range_checks()` — this
rejects only inputs that would genuinely have been read out of bounds, and
leaves callers that pass an uninitialised suit alongside a zero rank working
as before whenever the value is never used.

Seed: `corpus/solve_board/regression_unchecked_trick_suit.bin`.
Tests: `library/tests/deal_input_validation_test.cpp` (`DumpInputSafety`).

### 05 — `CalcAllTablesN()` solved an uninitialised board when given no deals

Found by the `calc_all_tables` harness on its first CI run, under
MemorySanitizer (`zero_tables.bin`).

`Boards bo;` is an uninitialised stack local. The board count was derived from
a `lastIndex` variable initialised to 0 and only assigned inside the
board-building loop, so `bo.no_of_boards = lastIndex + 1` claimed **one** board
even when `no_of_tables == 0` and the loop had written none.
`calc_all_boards_n()` then solved `bo.deals[0]`, `bo.target[0]`,
`bo.solutions[0]` and `bo.mode[0]`, none of which had ever been written.

ASan and UBSan do not see this; only MSan does, which is why it survived the
local sweep and surfaced in CI.

Fixed by returning `RETURN_NO_FAULT` early for zero deals, matching what
`CalcAllTablesX()` already did, and by taking the board count from `ind` --
the number of boards actually written -- rather than from a last-index
variable that starts at a valid-looking 0.

Seed: `corpus/calc_all_tables/zero_tables.bin`.
Tests: `library/tests/deal_input_validation_test.cpp`
(`CalcAllTablesWithZeroDealsSolvesNothing`).
