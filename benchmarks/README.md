# Recorded-workload benchmark

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

## Running it

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

## The committed workload

`testdata/dds-camrose-1-32.jsonl` (11.7 MB) is every DDS call made while playing
the 32 boards of **Camrose 2024**, whose deals are in
`testdata/camrose-1-32.pbn`. It totals 2,708 solve calls over 154,370 sampled
deals plus 32 par calculations — 178 s of DDS time as recorded.

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
without spending two minutes on the full workload.

## Recording format

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
