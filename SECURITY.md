# Security Policy

## What DDS is, for threat-modelling purposes

DDS is an **in-process library**, not a service. It opens no sockets, crosses no
privilege boundary, and keeps no persistent state between calls. In the normal
deployment the input is a bridge deal supplied by the calling application —
usually that application's own data, or a hand a user typed in themselves.

This matters when judging the severity of a memory-safety bug here. A defect
reachable only from data the caller already controls, in a library running in
the caller's own address space, is a robustness problem rather than a
privilege-escalation vector: there is no boundary being crossed.

Two deployments raise the stakes, and one lowers them:

- **PBN input from files** is the one classic untrusted-input path.
  `convert_from_pbn()` and the `*PBN` entry points parse externally authored
  text.
- **Server-side use** — any service that accepts user-submitted deals or PBN
  over a network — makes everything below remotely reachable. If you are
  building one, read "If you expose DDS to untrusted input" below.
- **The WebAssembly build** contains the blast radius. A memory error stays
  inside the module's linear memory and cannot corrupt the host page, so the
  browser deployment is substantially better protected than the native ones.

## Input trust model

**DDS assumes trusted, well-formed input.** It validates enough to catch honest
caller mistakes; it is not hardened against adversarial input, and the coverage
is uneven across entry points:

- `SolveBoard()` validates thoroughly — parameter ranges, card counts,
  duplicate cards, already-played cards — and returns a specific `RETURN_*`
  code. See `board_range_checks()` and `board_value_checks()` in
  `library/src/solver_if.cpp`.
- The par entry points validate the double dummy table
  (`par_table_checks()`, added after an out-of-range table was found to
  overflow a fixed character buffer) but not every scalar parameter.
- `CalcDDtable()` and `CalcDDtablePBN()` do **not** check that the four hands
  hold equal numbers of cards, so a malformed deal can reach the search.
- `convert_from_pbn()` silently ignores characters it does not recognise
  rather than rejecting the string.

Callers that cannot guarantee well-formed input should validate at their own
boundary rather than rely on the library to do it.

## Known unfixed issues

Open memory-safety defects found by the fuzz harnesses are tracked, with
reproducers and analysis, in
[`library/tests/fuzz/findings/README.md`](library/tests/fuzz/findings/README.md).

They are documented openly because DDS is a library whose consumers need the
information to judge their own exposure, and because all of them are
out-of-bounds *reads* reachable only through the input paths described above —
not remote code execution in any supported deployment. If that assessment is
wrong for your deployment, please tell us.

## If you expose DDS to untrusted input

The library was not designed for this. If you must:

1. **Validate at your boundary.** Reject deals that are not 13 cards per hand
   and tables whose entries fall outside 0-13, before calling DDS.
2. **Prefer `SolveBoard()`** over the `CalcDDtable*` entry points where you
   have the choice: its validation is the most complete.
3. **Sandbox it.** Run the solver in a separate process with memory and CPU
   limits. DDS allocates a large transposition table and its search is
   recursive, so resource exhaustion is a denial-of-service consideration
   independent of any memory-safety bug.
4. **Consider the WebAssembly build**, whose sandbox contains memory errors by
   construction.
5. Note that `DumpInput()` writes a `dump.txt` file into the process working
   directory whenever input is rejected. Define `DDS_NO_DUMP_ON_ERROR` to
   compile it out.

## Testing and tooling

The project runs AddressSanitizer, ThreadSanitizer and UndefinedBehaviorSanitizer
in CI on Linux and macOS, and carries libFuzzer harnesses for the four
input-handling surfaces:

```
bazel test --config=asan //library/...
bazel test //library/tests/fuzz/...
bazel run --config=fuzz //library/tests/fuzz:pbn_fuzz -- \
  library/tests/fuzz/corpus/pbn -runs=1000000
```

See [`library/tests/fuzz/README.md`](library/tests/fuzz/README.md) for how to
run a campaign and triage a finding. New reproducers are welcome as pull
requests against `library/tests/fuzz/corpus/` once the underlying defect is
fixed.

## Reporting a vulnerability

Please report suspected security issues through
**[GitHub's private vulnerability reporting](https://github.com/dds-bridge/dds/security/advisories/new)**
on this repository, which keeps the report private until a fix is available.

If you would rather not use GitHub, open a regular issue asking for a private
contact, without including details of the problem.

Please include the DDS version or commit, the platform and compiler, a
reproducer (a corpus file for one of the fuzz harnesses is ideal), and any
sanitizer output.

**What to expect.** DDS is maintained by a small group of volunteers, so please
allow time for a response. Issues in the categories described under "Input trust
model" above are likely to be treated as ordinary bugs and fixed in the open,
since the trust model is documented rather than implied. Issues that break the
documented model — anything reachable with well-formed, legal input, or any
out-of-bounds *write* — will be handled privately until fixed.
