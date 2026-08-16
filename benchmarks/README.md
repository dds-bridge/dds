# Benchmarks

Two of them, both measuring the same thing from opposite ends:

| target | what it measures | needs |
| --- | --- | --- |
| `//benchmarks:dds_replay` | a real client's whole DDS workload, replayed | a recording (one is committed) |
| `//benchmarks:warm_tt_benchmark` | transposition-table reuse in isolation | nothing; runs in the test cycle |

## Recorded-workload benchmark

`dds_replay` re-issues every DDS call a real client made, in the order it made
them, times them, and checks the answers still match what was recorded.

Most solver benchmarks pick a set of deals and solve them. That measures the
deals someone chose, which is not necessarily the work a client actually asks
for. A bridge engine's DDS traffic is dominated by things a hand-picked deal
list does not reproduce: batches of sampled hands rather than single deals, a
distribution of trick depths (most calls happen mid-play, not at trick 1), a mix
of `solutions=1` and `solutions=3`, and repeated calls on nearly identical
positions that hit a warm transposition table. Replaying a recording keeps all
of that.

Because the recording stores the result of every call, the replay also verifies
them. That makes this a regression test as well as a benchmark: any change that
alters a trick count shows up as a mismatch, on a workload of 154,370 solved
deals.

### Running the replay benchmark

```sh
bazel run -c opt //benchmarks:dds_replay
```

With no argument it replays the workload committed here. Pass a path to replay
your own recording instead:

```sh
bazel run -c opt //benchmarks:dds_replay -- /path/to/recording.jsonl
```

Useful options:

```sh
--list                      summarise the recording without running DDS
--threads N                 worker threads; repeat the flag to sweep several
--repeat N                  run N times and report the best
--purpose play              only one kind of call (bid/lead/play/claimcheck/par)
--min-trick 7 --max-trick 9 only part of the play
--solutions 3               only calls with this solutions value
--tricks                    also break the report down by trick number
--no-verify                 skip result checking (pure timing)
```

The report is broken down by purpose, with `ms/call`, `ms/board`, and the time
the same call took when it was recorded:

```
               calls    boards   seconds    ms/call   ms/board  rec ms/call
---------------------------------------------------------------------------
bid              962     50335    99.062     102.97      1.968       128.62
play            1226     73653    22.720      18.53      0.308        22.54
lead             128     17680    13.858     108.27      0.784       117.91
par               32        32     3.760     117.49    117.491       320.39
claimcheck       392     12702     1.135       2.90      0.089         4.20
---------------------------------------------------------------------------
TOTAL           2740    154402   140.534      51.29      0.910        65.09

VERIFY: all 2740 calls returned the recorded result
```

Comparing two builds is just two runs; `--repeat 3` and comparing the best run
of each keeps the noise down. Note that with `dds_mode=1` DDS reuses a
transposition table across consecutive solves on the same `SolverContext`, and
work is handed to threads first-come-first-served, so a single run has a few
percent of variation.

### The committed workload

The deals are real, not generated. `testdata/camrose-1-32.pbn` holds the 32
boards of **Camrose 2024** — the Camrose Trophy is the annual home
internationals, contested by the national bridge teams of England, Ireland,
Northern Ireland, Scotland and Wales — so every hand in the workload was dealt
for, and played in, a real international match.

Tournament boards are computer-dealt as well, so the point is not that these
deals are statistically unlike generated ones. It is that everything around them
is real: which calls got made, in what order, at what trick depth, over batches
of what size, is whatever a full session of bidding and play actually produced —
not a shape someone chose while writing a benchmark.

The play, however, is not the tournament's: `testdata/dds-camrose-1-32.jsonl`
(11.7 MB) is every DDS call made while **BEN** bid and played those 32 deals,
all four seats, from first call to last card. So the deals come from the event
and the DDS traffic is an engine's. It totals 2,708 solve calls over 154,370
sampled deals plus 32 par calculations — 178 s of DDS time as recorded.

DDS is a helper there, not the player. BEN consults it and then makes its own
choice, so the card DDS scored best is not necessarily the card that got played,
and a call's answer does not determine the next call. Each `solve` record is
self-contained — it carries the hands and the current trick it was asked about —
so replaying one depends on nothing else in the file. That is what makes the
benchmark's freedom legitimate: calls can be re-issued in any order, spread
across any number of threads, or filtered down to a subset with `--purpose` or
`--min-trick`, and every answer is still checkable against what was recorded.

It is committed rather than generated because it cannot be regenerated from this
repository: producing it needs [BEN](https://github.com/ThorvaldAagaard/ben), a
neural-network bridge engine, and its trained models. It was recorded with:

```sh
python game.py --boards "Camrose 1-32.pbn" --auto True \
       --config config/default.conf --ddsrecord dds-camrose-1-32.jsonl
```

`testdata/sample-recording.jsonl` is a 6-call excerpt of the same file. It backs
the fast `//benchmarks:replay_test` smoke test, so CI exercises the replay path
(JSON parsing, PBN decoding, batching, result canonicalisation, verification)
without spending two minutes on the full workload. Run it on its own with:

```sh
bazel test //benchmarks:replay_test
```

### Recording format

JSON Lines; one object per line, each tagged with `t`.

| `t` | meaning |
| --- | --- |
| `meta` | one per file: when/where it was recorded, DDS version, `dds_mode`, thread count |
| `board` | a marker for the deal being played |
| `solve` | one DDS solve call over a batch of sampled hands |
| `par` | one par calculation |

A `solve` record carries `strain_i`, `leader_i`, `current_trick` (card codes),
`solutions`, `hands_pbn` (one PBN deal per sampled board), the `result`, and the
`ms` it took. Cards are encoded as `suit * 13 + (14 - rank)`, and the trump
passed to DDS is `(strain_i - 1) % 5`.

`result` is a map of named integer lists, one entry per board in the batch:

* `solutions == 1` — `"max"` and `"min"`: the best and worst trick counts for
  the side to play.
* `solutions == 3` — one list per playable card, keyed by card code, including
  cards equivalent to the one DDS reported (its `equals` bitmap).

Any recorder that emits this format can be replayed; nothing here is specific to
the engine that produced this file.

## Warm-transposition-table benchmark

`warm_tt_benchmark` isolates one effect the replay only shows in aggregate: what
reusing a single `DDS_C_SOLVER_CTX` is worth, because the transposition table
survives between solves on nearly identical positions.

```sh
bazel test -c opt //benchmarks:warm_tt_benchmark --test_output=all
```

It values the opening leads of 100 fixed deals two ways — one `solutions=3`
solve (A), versus playing each of the top-K leads and solving the result with
`solutions=1` (B) — and runs B twice, with a fresh context per lead and with one
context reused:

```
[warm-TT benchmark] declarer South; opening leader West; 100 deals x {3N, 4S}
  3N+4S      K=4 over 200 deals:  A(sol=3)=  8324.4 ms   B_cold=  7735.8 ms (0.93x A)   B_warm=  4429.1 ms (0.53x A)   warm/cold=0.57
  3N+4S      K=6 over 200 deals:  A(sol=3)=  7644.8 ms   B_cold= 10190.7 ms (1.33x A)   B_warm=  5439.7 ms (0.71x A)   warm/cold=0.53
```

Reuse roughly halves the cost of the lead-by-lead approach, which is what makes
it cheaper than scoring every lead at once.

Unlike the replay, these deals are randomly generated rather than taken from
play, which is the point: the effect being measured is a property of the solver,
not of any particular workload, so a synthetic sample keeps it isolated and
lets the benchmark run with nothing committed alongside it.

The timings are printed, not asserted — CI load would make that flaky. What is
asserted is correctness: every solve succeeds and returns a card, and the warm
and cold runs agree on every lead's score. So a change that makes TT reuse
return a different answer fails this test, on any machine, on the same 100
deals (the seed is fixed; see the comment on it).
