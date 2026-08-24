# Fuzz harnesses

Coverage-guided fuzzing for the four DDS surfaces that consume caller- or
file-supplied data:

| Harness | Entry point | Why |
|---|---|---|
| `pbn` | `convert_from_pbn()` | The main text parser; the one path that routinely sees externally authored data (PBN files). |
| `calc_dd_table_pbn` | `CalcDDtablePBN()` | The full PBN-to-solver path: parse, then calculate a DD table. |
| `solve_board` | `SolveBoard()` | The main solver, including its input validation layer. |
| `par` | `Par()`, `SidesPar()`, `SidesParBin()`, `DealerPar()`, `DealerParBin()` | Derives contract text from a caller-supplied table into fixed-size buffers. |
| `calc_all_tables` | `CalcAllTablesN()`, `CalcAllTablesPBNN()`, `CalcAllTablesX()` | The batch entry points, and in particular their handling of a caller-supplied deal *count*. |

## Two ways to run each harness

**Corpus replay (runs in ordinary CI).** Every harness is also an ordinary
`cc_test` that replays the checked-in seed corpus. No libFuzzer needed, so it
works on every platform and under the sanitizer configs:

```
bazel test //library/tests/fuzz/...
bazel test --config=asan //library/tests/fuzz/...
```

This is what keeps a fixed bug fixed: a reproducer added to `corpus/` becomes a
permanent regression seed.

**Fuzzing campaign.** The `*_fuzz` targets are libFuzzer binaries, tagged
`manual` and built only under `--config=fuzz`:

```
bazel run --config=fuzz //library/tests/fuzz:pbn_fuzz -- \
  library/tests/fuzz/corpus/pbn -runs=1000000
```

`--config=fuzz` uses the registered hermetic LLVM toolchain, which ships
`libclang_rt.fuzzer`. Apple's toolchain does not, which is why the config does
not chain `--config=asan` — on macOS that switches to the Xcode toolchain and
the link fails. On Linux, add `--config=asan` for fuzzing with memory-error
detection:

```
bazel run --config=fuzz --config=asan //library/tests/fuzz:solve_board_fuzz -- \
  library/tests/fuzz/corpus/solve_board -runs=10000000
```

> **Use `bazel run`, not `bazel-bin/...` directly.** `--config=fuzz`, `asan`,
> `ubsan` and `tsan` all build with `--compilation_mode=dbg`, so they share the
> `darwin_arm64-dbg` / `k8-dbg` output directory name. A path taken from
> `bazel info --config=fuzz bazel-bin` can therefore point at a binary left
> behind by a *different* sanitizer config, which silently reproduces (or fails
> to reproduce) the wrong thing.

### Why the batch harness exists

The four single-deal harnesses drive one deal at a time, so none of them
exercised how the batch entry points handle `no_of_tables`. That is exactly
where `CalcAllTablesPBNN()` copied a caller-supplied count of records into a
fixed-size local before validating it — a stack-buffer-overflow *write* that
was found in code review rather than by fuzzing. `calc_all_tables_fuzz` covers
that surface, and removing either count guard reproduces a crash under it.

Two details in that harness are load-bearing:

- Slots the input does not perturb are pre-filled with a **valid** deal.
  `CalcAllTablesPBNN()` stops at the first slot `convert_from_pbn()` rejects,
  so with zeroed slots the loop returns `RETURN_PBN_FAULT` immediately and
  never reaches the boundary. The first version of this harness made that
  mistake and did not catch the bug it was written for.
- The fill deal holds **one card per hand**, not a full 52. This harness
  targets count and batch handling, not search depth; a full deal in every
  slot drops throughput from ~75000 executions in four minutes to ~1700.

## Harness contract

Each harness defines `LLVMFuzzerTestOneInput()` and `LLVMFuzzerInitialize()`.
libFuzzer treats the initializer as optional via a weak symbol, but weak
references are not portable between ELF and Mach-O, so the replay driver
requires it — harnesses with nothing to configure define a trivial one.

Harnesses must supply well-formed *containers* even for malformed content: the
PBN entry points take NUL-terminated strings, so the harness terminates the
buffer itself. Handing the library a non-terminated array would report a
harness bug as a library bug.

## Triaging a finding

1. libFuzzer writes the input to `./crash-<sha1>`.
2. Reproduce it under ASan with the replay driver, which gives a far better
   report than libFuzzer alone:
   ```
   mkdir -p /tmp/f && cp crash-<sha1> /tmp/f/
   bazel build --config=asan //library/tests/fuzz:par_fuzz_corpus_test
   ./bazel-bin/library/tests/fuzz/par_fuzz_corpus_test /tmp/f
   ```
3. If it is not yet fixed, put the reproducer in `findings/` and describe it in
   `findings/README.md` so the corpus tests stay green.
4. Once fixed, move it into the matching `corpus/` directory.

`findings/` records what the harnesses have found, fixed and open — see
`findings/README.md`. There are currently no open findings.
