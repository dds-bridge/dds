#!/usr/bin/env python3
"""Unit tests for benchmark.py (stdlib unittest)."""

from __future__ import annotations

import io
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import benchmark


class TestIsPowerOf10(unittest.TestCase):
    def test_powers(self) -> None:
        for n in (1, 10, 100, 1000, 10000):
            self.assertTrue(benchmark.is_power_of_10(n), n)

    def test_non_powers(self) -> None:
        for n in (0, -1, 2, 11, 99, 101, 300):
            self.assertFalse(benchmark.is_power_of_10(n), n)


class TestSelectHandFiles(unittest.TestCase):
    def test_selects_powers_of_10_descending(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            hands = Path(tmp)
            for name in (
                "list1.txt",
                "list10.txt",
                "list100.txt",
                "list1000.txt",
                "list2.txt",
                "list300.txt",
                "list1000_with_dups.txt",
            ):
                (hands / name).write_text("x\n")
            files = benchmark.select_hand_files(hands, max_deals=100)
            self.assertEqual(files, ["list100.txt", "list10.txt", "list1.txt"])

    def test_empty_raises(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(benchmark.BenchmarkError) as ctx:
                benchmark.select_hand_files(Path(tmp), max_deals=100)
            self.assertIn("no list10^n.txt", str(ctx.exception))


class TestParseDtestOutput(unittest.TestCase):
    def test_full_output(self) -> None:
        out = (
            "Number of hands           100\n"
            "User time (ms)            250\n"
            "Sys time (ms)             10\n"
            "Avg user time (ms)        2.5\n"
            "Ratio                     1.23\n"
        )
        parsed = benchmark.parse_dtest_output(out)
        self.assertEqual(parsed.user_ms, 250.0)
        self.assertEqual(parsed.sys_ms, 10.0)
        self.assertEqual(parsed.avg_user, 2.5)
        self.assertEqual(parsed.sys_user, 1.23)
        self.assertEqual(parsed.hands, 100)

    def test_zero_tokens(self) -> None:
        out = (
            "Number of hands           0\n"
            "User time (ms)            zero\n"
            "Sys time (ms)             zero\n"
            "Avg user time (ms)        zero\n"
        )
        parsed = benchmark.parse_dtest_output(out)
        self.assertEqual(parsed.user_ms, 0.0)
        self.assertEqual(parsed.sys_ms, 0.0)
        self.assertEqual(parsed.avg_user, 0.0)

    def test_avg_derived_from_user_and_hands(self) -> None:
        out = (
            "Number of hands           100\n"
            "User time (ms)            200\n"
            "Sys time (ms)             5\n"
        )
        parsed = benchmark.parse_dtest_output(out)
        self.assertEqual(parsed.avg_user, 2.0)

    def test_missing_fields_are_na(self) -> None:
        parsed = benchmark.parse_dtest_output("hello\n")
        self.assertIsNone(parsed.user_ms)
        self.assertIsNone(parsed.sys_ms)
        self.assertIsNone(parsed.avg_user)
        self.assertIsNone(parsed.sys_user)

    def test_sys_time_na_keeps_user_timing(self) -> None:
        # wasm32: clock() is unavailable; dtest prints Sys time (ms) n/a.
        out = (
            "Number of hands                 1\n"
            "User time (ms)                 21\n"
            "Avg user time (ms)          21.00\n"
            "Sys time (ms)                 n/a\n"
        )
        parsed = benchmark.parse_dtest_output(out)
        self.assertEqual(parsed.user_ms, 21.0)
        self.assertEqual(parsed.avg_user, 21.0)
        self.assertIsNone(parsed.sys_ms)
        self.assertTrue(benchmark.dtest_timing_usable(parsed))

    def test_missing_user_is_not_usable(self) -> None:
        parsed = benchmark.parse_dtest_output("Sys time (ms)             10\n")
        self.assertFalse(benchmark.dtest_timing_usable(parsed))

    def test_user_without_avg_is_not_usable(self) -> None:
        # User time present but no hands / avg line -> cannot form avg_user.
        out = "User time (ms)            250\nSys time (ms)             10\n"
        parsed = benchmark.parse_dtest_output(out)
        self.assertEqual(parsed.user_ms, 250.0)
        self.assertIsNone(parsed.avg_user)
        self.assertFalse(benchmark.dtest_timing_usable(parsed))


class TestRunTableHeader(unittest.TestCase):
    def test_sys_user_column_not_ratio(self) -> None:
        header, sep = benchmark.format_run_table_header("branch")
        self.assertRegex(header, r"\bsys/user\b")
        self.assertNotRegex(header, r"\bratio\b")
        self.assertIn("user_ms", header)
        self.assertIn("sys_ms", header)
        self.assertEqual(len(header.split()), len(sep.split()))


class TestWithinEpsilon(unittest.TestCase):
    def test_equal_within(self) -> None:
        self.assertTrue(benchmark.within_epsilon(100.0, 100.4, 0.5))
        self.assertFalse(benchmark.within_epsilon(100.0, 101.0, 0.5))

    def test_zero_hi(self) -> None:
        self.assertTrue(benchmark.within_epsilon(0.0, 0.0, 0.5))


class TestLabelForPath(unittest.TestCase):
    def test_basename(self) -> None:
        self.assertEqual(benchmark.label_for_path("/tmp/foo/dtest"), "dtest")


class TestDtestRel(unittest.TestCase):
    def test_posix_path_omits_exe(self) -> None:
        self.assertEqual(
            benchmark.dtest_rel(os_name="posix"),
            Path("bazel-bin/library/tests/dtest"),
        )

    def test_windows_path_uses_exe(self) -> None:
        self.assertEqual(
            benchmark.dtest_rel(os_name="nt"),
            Path("bazel-bin/library/tests/dtest.exe"),
        )

    def test_module_constant_matches_current_platform(self) -> None:
        self.assertEqual(benchmark.DTEST_REL, benchmark.dtest_rel())


class TestEnsureExecutable(unittest.TestCase):
    def test_skips_chmod_on_windows(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "tool"
            path.write_text("x")
            with mock.patch("benchmark.os.name", "nt"):
                with mock.patch("benchmark.os.chmod") as chmod_mock:
                    benchmark.ensure_executable(path)
            chmod_mock.assert_not_called()

    def test_sets_execute_bit_on_posix(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "tool"
            path.write_text("x")
            with mock.patch("benchmark.os.name", "posix"):
                with mock.patch("benchmark.os.stat") as stat_mock:
                    with mock.patch("benchmark.os.chmod") as chmod_mock:
                        stat_mock.return_value.st_mode = 0o100644
                        benchmark.ensure_executable(path)
            stat_mock.assert_called_once_with(path)
            chmod_mock.assert_called_once_with(path, 0o100755)


class TestRunnerCleanup(unittest.TestCase):
    def test_alt_leave_goes_to_injected_out(self) -> None:
        out = io.StringIO()
        err = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            runner = benchmark.BenchmarkRunner(
                Path(tmp),
                benchmark.Config(),
                out=out,
                err=err,
            )
            runner.alt_screen_active = True
            with mock.patch("sys.stdout", new_callable=io.StringIO) as fake_stdout:
                runner.cleanup()
            self.assertEqual(out.getvalue(), benchmark.ALT_LEAVE)
            self.assertEqual(fake_stdout.getvalue(), "")
            self.assertFalse(runner.alt_screen_active)

    def test_cleanup_noop_when_alt_screen_inactive(self) -> None:
        out = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            runner = benchmark.BenchmarkRunner(
                Path(tmp),
                benchmark.Config(),
                out=out,
            )
            runner.cleanup()
            self.assertEqual(out.getvalue(), "")


class TestRunBuild(unittest.TestCase):
    def test_failed_build_output_goes_to_injected_err(self) -> None:
        err = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            runner = benchmark.BenchmarkRunner(
                Path(tmp),
                benchmark.Config(details=False),
                err=err,
            )

            def fake_run(cmd, **kwargs):  # type: ignore[no-untyped-def]
                log = kwargs["stdout"]
                log.write("build blew up\n")
                log.flush()
                return mock.Mock(returncode=1)

            with mock.patch("subprocess.run", side_effect=fake_run):
                with mock.patch("sys.stderr", new_callable=io.StringIO) as fake_stderr:
                    with self.assertRaises(subprocess.CalledProcessError):
                        runner.run_build(["false"])
            self.assertIn("build blew up", err.getvalue())
            self.assertEqual(fake_stderr.getvalue(), "")


class TestResolveBazelCommand(unittest.TestCase):
    def test_prefers_bazelisk_when_available(self) -> None:
        def fake_which(name: str) -> str | None:
            return "/usr/bin/bazelisk" if name == "bazelisk" else None

        self.assertEqual(benchmark.resolve_bazel_command(which=fake_which), "bazelisk")

    def test_falls_back_to_bazel_when_bazelisk_missing(self) -> None:
        def fake_which(name: str) -> str | None:
            return "/usr/bin/bazel" if name == "bazel" else None

        self.assertEqual(benchmark.resolve_bazel_command(which=fake_which), "bazel")

    def test_falls_back_to_bazel_when_neither_on_path(self) -> None:
        self.assertEqual(benchmark.resolve_bazel_command(which=lambda _name: None), "bazel")


class TestBazelDtestCommand(unittest.TestCase):
    def test_bazel_dtest_invokes_resolved_launcher(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            runner = benchmark.BenchmarkRunner(Path(tmp), benchmark.Config())
            seen: list[list[str]] = []

            def capture(cmd, **_kwargs):  # type: ignore[no-untyped-def]
                seen.append(list(cmd))

            with mock.patch.object(runner, "run_build", side_effect=capture):
                with mock.patch.object(
                    benchmark, "resolve_bazel_command", return_value="bazelisk"
                ):
                    runner.bazel_dtest()
            self.assertEqual(seen, [["bazelisk", "build", "//library/tests:dtest"]])

    def test_dry_run_build_message_uses_resolved_launcher(self) -> None:
        err = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            runner = benchmark.BenchmarkRunner(
                Path(tmp),
                benchmark.Config(dry_run=True),
                err=err,
            )
            with mock.patch.object(
                benchmark, "resolve_bazel_command", return_value="bazelisk"
            ):
                runner.build_branch_binary("develop", Path(tmp) / "out")
            self.assertIn("bazelisk build //library/tests:dtest", err.getvalue())
            self.assertNotIn("bazel build //library/tests:dtest", err.getvalue())

    def test_bazel_dtest_wasm_invokes_resolved_launcher(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            runner = benchmark.BenchmarkRunner(Path(tmp), benchmark.Config())
            seen: list[list[str]] = []

            def capture(cmd, **_kwargs):  # type: ignore[no-untyped-def]
                seen.append(list(cmd))

            with mock.patch.object(runner, "run_build", side_effect=capture):
                with mock.patch.object(
                    benchmark, "resolve_bazel_command", return_value="bazelisk"
                ):
                    runner.bazel_dtest_wasm()
            self.assertEqual(seen, [["bazelisk", "build", "//wasm:dtest_wasm"]])

    def test_dry_run_wasm_branch_build_message(self) -> None:
        err = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp) / "wasm_out"
            dest.mkdir()
            runner = benchmark.BenchmarkRunner(
                Path(tmp),
                benchmark.Config(dry_run=True),
                err=err,
            )
            with mock.patch.object(
                benchmark, "resolve_bazel_command", return_value="bazelisk"
            ):
                js = runner.build_wasm_branch("develop", dest)
            text = err.getvalue()
            self.assertEqual(js, dest / "dtest.js")
            self.assertIn("bazelisk build //wasm:dtest_wasm", text)
            self.assertIn("dtest.js", text)
            self.assertIn("dtest.wasm", text)


class TestBuildBinariesWasm(unittest.TestCase):
    def test_wasm_branch_label_and_js_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            err = io.StringIO()
            runner = benchmark.BenchmarkRunner(
                root,
                benchmark.Config(
                    dry_run=True,
                    specs=[("wasm_branch", "develop")],
                ),
                err=err,
            )
            with mock.patch.object(
                benchmark,
                "git_prep_for_branches",
                return_value=([("wasm_branch", "develop")], "main"),
            ):
                with mock.patch.object(
                    benchmark, "resolve_bazel_command", return_value="bazelisk"
                ):
                    labels, paths = runner.build_binaries()
            self.assertEqual(labels, ["wasm:develop"])
            self.assertEqual(len(paths), 1)
            self.assertEqual(paths[0].name, "dtest.js")
            self.assertTrue(paths[0].parent.is_dir())
            self.assertIn("checkout main", err.getvalue())
            self.assertIn("//wasm:dtest_wasm", err.getvalue())


class TestRunOrder(unittest.TestCase):
    def test_default(self) -> None:
        self.assertEqual(benchmark.run_order(3, reverse=False), [0, 1, 2])

    def test_reverse(self) -> None:
        self.assertEqual(benchmark.run_order(3, reverse=True), [2, 1, 0])


class TestParseArgs(unittest.TestCase):
    def test_defaults(self) -> None:
        cfg = benchmark.parse_args([], env={})
        self.assertEqual(cfg.repeats, 1)
        self.assertEqual(cfg.max_deals, 100)
        self.assertEqual(cfg.epsilon, 0.5)
        self.assertFalse(cfg.build)
        self.assertFalse(cfg.details)
        self.assertFalse(cfg.sys_user)
        self.assertFalse(cfg.reverse)
        self.assertEqual(cfg.specs, [])
        self.assertEqual(cfg.dtest_extra, [])

    def test_env_overrides(self) -> None:
        cfg = benchmark.parse_args(
            [],
            env={
                "REPEATS": "3",
                "MAX_DEALS": "10",
                "EPSILON": "1.5",
                "DRY_RUN": "1",
                "DETAILS": "1",
                "SYS_USER": "1",
            },
        )
        self.assertEqual(cfg.repeats, 3)
        self.assertEqual(cfg.max_deals, 10)
        self.assertEqual(cfg.epsilon, 1.5)
        self.assertTrue(cfg.dry_run)
        self.assertTrue(cfg.details)
        self.assertTrue(cfg.sys_user)

    def test_binary_env_appended(self) -> None:
        cfg = benchmark.parse_args([], env={"BINARY": "/tmp/other"})
        self.assertEqual(cfg.specs, [("binary", "/tmp/other")])

    def test_cli_binary_suppresses_env(self) -> None:
        cfg = benchmark.parse_args(
            ["--binary", "/cli"],
            env={"BINARY": "/env"},
        )
        self.assertEqual(cfg.specs, [("binary", "/cli")])

    def test_repeatable_specs_and_dtest_extra(self) -> None:
        cfg = benchmark.parse_args(
            [
                "--branch",
                "develop",
                "--binary",
                "/tmp/dtest",
                "--repeats",
                "2",
                "--",
                "-n",
                "8",
                "-r",
            ],
            env={},
        )
        self.assertEqual(
            cfg.specs,
            [("branch", "develop"), ("binary", "/tmp/dtest")],
        )
        self.assertEqual(cfg.repeats, 2)
        self.assertEqual(cfg.dtest_extra, ["-n", "8", "-r"])

    def test_max_deals_aliases(self) -> None:
        for flag in ("--max-deals", "--max_deals", "-max-deals", "-max_deals"):
            cfg = benchmark.parse_args([flag, "10"], env={})
            self.assertEqual(cfg.max_deals, 10, flag)

    def test_repeats_only_once(self) -> None:
        with self.assertRaises(benchmark.BenchmarkError) as ctx:
            benchmark.parse_args(["--repeats", "2", "--repeats", "3"], env={})
        self.assertIn("--repeats may be given only once", str(ctx.exception))

    def test_branch_must_not_start_with_dash(self) -> None:
        with self.assertRaises(benchmark.BenchmarkError) as ctx:
            benchmark.parse_args(["--branch", "-bad"], env={})
        self.assertIn("must not start with '-'", str(ctx.exception))

    def test_reverse_requires_two_specs(self) -> None:
        with self.assertRaises(benchmark.BenchmarkError) as ctx:
            benchmark.parse_args(["--reverse", "--binary", "/x"], env={})
        self.assertIn("--reverse requires at least two", str(ctx.exception))

    def test_invalid_repeats(self) -> None:
        with self.assertRaises(benchmark.BenchmarkError):
            benchmark.parse_args(["--repeats", "0"], env={})

    def test_sys_user_flag(self) -> None:
        cfg = benchmark.parse_args(["--sys-user"], env={})
        self.assertTrue(cfg.sys_user)

    def test_invalid_epsilon(self) -> None:
        with self.assertRaises(benchmark.BenchmarkError):
            benchmark.parse_args(["--epsilon", "-1"], env={})

    def test_wasm_branch_spec(self) -> None:
        cfg = benchmark.parse_args(["--wasm_branch", "develop"], env={})
        self.assertEqual(cfg.specs, [("wasm_branch", "develop")])

    def test_wasm_branch_hyphen_alias(self) -> None:
        cfg = benchmark.parse_args(["--wasm-branch", "develop"], env={})
        self.assertEqual(cfg.specs, [("wasm_branch", "develop")])

    def test_wasm_branch_repeatable_with_branch(self) -> None:
        cfg = benchmark.parse_args(
            [
                "--branch",
                "develop",
                "--wasm_branch",
                "develop",
                "--wasm_branch",
                "feature",
            ],
            env={},
        )
        self.assertEqual(
            cfg.specs,
            [
                ("branch", "develop"),
                ("wasm_branch", "develop"),
                ("wasm_branch", "feature"),
            ],
        )

    def test_wasm_branch_must_not_start_with_dash(self) -> None:
        with self.assertRaises(benchmark.BenchmarkError) as ctx:
            benchmark.parse_args(["--wasm_branch", "-bad"], env={})
        self.assertIn("must not start with '-'", str(ctx.exception))

    def test_reverse_accepts_wasm_branch_pair(self) -> None:
        cfg = benchmark.parse_args(
            ["--reverse", "--branch", "develop", "--wasm_branch", "develop"],
            env={},
        )
        self.assertTrue(cfg.reverse)
        self.assertEqual(
            cfg.specs,
            [("branch", "develop"), ("wasm_branch", "develop")],
        )


class TestSummary(unittest.TestCase):
    def test_two_binary_ratio_and_note(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0),
            benchmark.ResultRow("solve", "list100.txt", 1, 1, 50.0, 1.0, 0.5, 1.0),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "fast"],
            files=["list100.txt"],
            epsilon=0.5,
        )
        header = text.splitlines()[0]
        self.assertRegex(header, r"\brel\b")
        self.assertNotRegex(header, r"\bratio\b")
        self.assertIn("base", text)
        self.assertIn("fast", text)
        self.assertIn("0.50x", text)
        self.assertIn("fast faster", text)
        self.assertIn("TOTAL  solve", text)
        self.assertNotIn("TOTAL  calc", text)

    def test_separate_total_lines_per_solver(self) -> None:
        # TOTAL is avg user ms: sum(user_ms) / deals (list100 => 100).
        rows = [
            benchmark.ResultRow("solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0),
            benchmark.ResultRow("solve", "list100.txt", 1, 1, 50.0, 1.0, 0.5, 1.0),
            benchmark.ResultRow("calc", "list100.txt", 0, 1, 200.0, 1.0, 2.0, 1.0),
            benchmark.ResultRow("calc", "list100.txt", 1, 1, 100.0, 1.0, 1.0, 1.0),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "fast"],
            files=["list100.txt"],
            epsilon=0.5,
        )
        lines = text.splitlines()
        solve_tot = next(line for line in lines if line.startswith("TOTAL  solve"))
        calc_tot = next(line for line in lines if line.startswith("TOTAL  calc"))
        self.assertRegex(solve_tot, r"\b1\.00\b")  # 100/100
        self.assertRegex(solve_tot, r"\b0\.50\b")  # 50/100
        self.assertRegex(solve_tot, r"0\.50x")
        self.assertIn("fast faster", solve_tot)
        self.assertRegex(calc_tot, r"\b2\.00\b")  # 200/100
        self.assertRegex(calc_tot, r"\b1\.00\b")  # 100/100
        self.assertRegex(calc_tot, r"0\.50x")
        self.assertIn("fast faster", calc_tot)
        self.assertNotRegex(solve_tot, r"\b100\.00\b")
        # No combined grand-total line.
        self.assertEqual(sum(1 for line in lines if line.startswith("TOTAL")), 2)

    def test_total_is_user_ms_per_deal_across_files_and_repeats(self) -> None:
        rows = [
            # solve base: user 100+30+20=150, deals 100+10+10=120 -> 1.25
            benchmark.ResultRow("solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0),
            benchmark.ResultRow("solve", "list10.txt", 0, 1, 30.0, 1.0, 3.0, 1.0),
            benchmark.ResultRow("solve", "list10.txt", 0, 2, 20.0, 1.0, 2.0, 1.0),
            # solve fast: 50+15+10=75 / 120 = 0.625 -> 0.50x
            benchmark.ResultRow("solve", "list100.txt", 1, 1, 50.0, 1.0, 0.5, 1.0),
            benchmark.ResultRow("solve", "list10.txt", 1, 1, 15.0, 1.0, 1.5, 1.0),
            benchmark.ResultRow("solve", "list10.txt", 1, 2, 10.0, 1.0, 1.0, 1.0),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "fast"],
            files=["list100.txt", "list10.txt"],
            epsilon=0.5,
        )
        solve_tot = next(
            line for line in text.splitlines() if line.startswith("TOTAL  solve")
        )
        self.assertRegex(solve_tot, r"\b1\.25\b")
        self.assertRegex(solve_tot, r"\b0\.62\b")  # 0.625 -> 0.62 with :.2f
        self.assertRegex(solve_tot, r"0\.50x")
        self.assertIn("fast faster", solve_tot)
        self.assertNotRegex(solve_tot, r"\b150\.00\b")
        self.assertNotRegex(solve_tot, r"\b75\.00\b")

    def test_total_prints_na_for_missing_binary_timing(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0, hands=100),
            # Binary 1: incomplete dtest output (no user_ms)
            benchmark.ResultRow("solve", "list100.txt", 1, 1, None, None, None, None),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "other"],
            files=["list100.txt"],
            epsilon=0.5,
        )
        solve_tot = next(
            line for line in text.splitlines() if line.startswith("TOTAL  solve")
        )
        self.assertRegex(solve_tot, r"\b1\.00\b")
        self.assertRegex(solve_tot, r"\bNA\b")
        self.assertNotRegex(solve_tot, r"\b0\.00\b")
        self.assertNotRegex(solve_tot, r"\d+\.\d+x")
        self.assertNotIn("faster", solve_tot)
        self.assertNotIn("equal", solve_tot)

    def test_total_excludes_rows_missing_avg_user(self) -> None:
        # user_ms alone is incomplete (per-file would print NA for avg_user).
        rows = [
            benchmark.ResultRow(
                "solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0, hands=100
            ),
            benchmark.ResultRow(
                "solve", "list100.txt", 1, 1, 50.0, 1.0, None, None, hands=100
            ),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "other"],
            files=["list100.txt"],
            epsilon=0.5,
        )
        solve_tot = next(
            line for line in text.splitlines() if line.startswith("TOTAL  solve")
        )
        self.assertRegex(solve_tot, r"\b1\.00\b")
        self.assertRegex(solve_tot, r"\bNA\b")
        self.assertNotRegex(solve_tot, r"\b0\.50\b")
        self.assertNotRegex(solve_tot, r"\d+\.\d+x")

    def test_total_excludes_zero_reported_hands(self) -> None:
        # hands=0 is real dtest output, not missing — do not fall back to list1.txt => 1.
        rows = [
            benchmark.ResultRow(
                "solve", "list1.txt", 0, 1, 100.0, 1.0, 1.0, 1.0, hands=1
            ),
            benchmark.ResultRow(
                "solve", "list1.txt", 1, 1, 0.0, 0.0, 0.0, None, hands=0
            ),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "other"],
            files=["list1.txt"],
            epsilon=0.5,
        )
        solve_tot = next(
            line for line in text.splitlines() if line.startswith("TOTAL  solve")
        )
        self.assertRegex(solve_tot, r"\b100\.00\b")
        self.assertRegex(solve_tot, r"\bNA\b")
        self.assertNotRegex(solve_tot, r"\b0\.00\b")
        self.assertNotRegex(solve_tot, r"\d+\.\d+x")

    def test_total_weights_by_reported_hands_not_filename(self) -> None:
        # Filename says 100, but dtest reported 50 hands processed.
        rows = [
            benchmark.ResultRow(
                "solve", "list100.txt", 0, 1, 100.0, 1.0, 2.0, 1.0, hands=50
            ),
            benchmark.ResultRow(
                "solve", "list100.txt", 1, 1, 50.0, 1.0, 1.0, 1.0, hands=50
            ),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "fast"],
            files=["list100.txt"],
            epsilon=0.5,
        )
        solve_tot = next(
            line for line in text.splitlines() if line.startswith("TOTAL  solve")
        )
        self.assertRegex(solve_tot, r"\b2\.00\b")  # 100/50
        self.assertRegex(solve_tot, r"\b1\.00\b")  # 50/50
        self.assertNotRegex(solve_tot, r"\b0\.50\b")  # would be 50/100 if filename used
        self.assertRegex(solve_tot, r"0\.50x")

    def test_default_summary_omits_sys_user_column(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", 0, 1, 100.0, 10.0, 1.0, 0.10),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base"],
            files=["list1.txt"],
            epsilon=0.5,
        )
        self.assertNotIn("sys/user", text)

    def test_sys_user_column_per_binary(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", 0, 1, 100.0, 10.0, 1.0, 0.10),
            benchmark.ResultRow("solve", "list1.txt", 1, 1, 50.0, 20.0, 0.5, 0.40),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "fast"],
            files=["list1.txt"],
            epsilon=0.5,
            sys_user=True,
        )
        header = text.splitlines()[0]
        # sys/user appears once per binary, immediately after each label column.
        self.assertEqual(header.count("sys/user"), 2)
        self.assertRegex(header, r"base\s+sys/user\s+fast\s+sys/user")
        solve_line = next(
            line for line in text.splitlines() if line.startswith("solve ")
        )
        self.assertRegex(solve_line, r"\b0\.10\b")
        self.assertRegex(solve_line, r"\b0\.40\b")
        self.assertIn("0.50x", text)

    def test_sys_user_averages_repeats_and_missing_is_na(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", 0, 1, 100.0, 10.0, 1.0, 0.10),
            benchmark.ResultRow("solve", "list1.txt", 0, 2, 100.0, 10.0, 1.0, 0.30),
            benchmark.ResultRow("solve", "list1.txt", 1, 1, 50.0, 1.0, 0.5, None),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "other"],
            files=["list1.txt"],
            epsilon=0.5,
            sys_user=True,
        )
        solve_line = next(
            line for line in text.splitlines() if line.startswith("solve ")
        )
        self.assertRegex(solve_line, r"\b0\.20\b")  # mean of 0.10 and 0.30
        self.assertRegex(solve_line, r"\bNA\b")

    def test_equal_within_epsilon(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0),
            benchmark.ResultRow("solve", "list100.txt", 1, 1, 100.2, 1.0, 1.002, 1.0),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["a", "b"],
            files=["list100.txt"],
            epsilon=0.5,
        )
        self.assertIn("equal", text)

    def test_three_binaries_no_note(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", b, 1, 10.0, 0.0, 1.0, None)
            for b in (0, 1, 2)
        ]
        text = benchmark.format_summary(
            rows,
            # Label substring "rel" must not be mistaken for the rel column.
            labels=["release", "b", "c"],
            files=["list1.txt"],
            epsilon=0.5,
        )
        header = text.splitlines()[0]
        self.assertNotRegex(header, r"\brel\b")
        self.assertNotRegex(header, r"\bnote\b")

    def test_zero_baseline_avg_skips_ratio(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", 0, 1, 0.0, 0.0, 0.0, None),
            benchmark.ResultRow("solve", "list1.txt", 1, 1, 1.0, 0.0, 1.0, None),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["a", "b"],
            files=["list1.txt"],
            epsilon=0.5,
        )
        solve_line = next(
            line for line in text.splitlines() if line.startswith("solve ")
        )
        self.assertRegex(solve_line, r"\b0\.00\b")
        self.assertRegex(solve_line, r"\b1\.00\b")
        self.assertNotRegex(solve_line, r"\d+\.\d+x")
        self.assertNotIn("faster", solve_line)
        self.assertNotIn("equal", solve_line)

    def test_missing_avg_prints_na_and_keeps_row(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", 0, 1, 100.0, 1.0, 1.0, 1.0),
            # Incomplete dtest output: avg_user missing for binary 1
            benchmark.ResultRow("solve", "list1.txt", 1, 1, None, None, None, None),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "other"],
            files=["list1.txt"],
            epsilon=0.5,
        )
        solve_line = next(
            line for line in text.splitlines() if line.startswith("solve ")
        )
        self.assertRegex(solve_line, r"\b1\.00\b")
        self.assertRegex(solve_line, r"\bNA\b")
        self.assertNotRegex(solve_line, r"\d+\.\d+x")
        self.assertNotIn("faster", solve_line)
        self.assertNotIn("equal", solve_line)

    def test_missing_avg_suppresses_ratio_either_side(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", 0, 1, None, None, None, None),
            benchmark.ResultRow("solve", "list1.txt", 1, 1, 50.0, 1.0, 0.5, 1.0),
        ]
        text = benchmark.format_summary(
            rows,
            labels=["base", "other"],
            files=["list1.txt"],
            epsilon=0.5,
        )
        solve_line = next(
            line for line in text.splitlines() if line.startswith("solve ")
        )
        self.assertRegex(solve_line, r"\bNA\b")
        self.assertRegex(solve_line, r"\b0\.50\b")
        self.assertNotRegex(solve_line, r"\d+\.\d+x")


class TestIsDdsRoot(unittest.TestCase):
    def test_valid(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "MODULE.bazel").write_text("module(name = \"dds\")\n")
            (root / "library" / "tests").mkdir(parents=True)
            (root / "library" / "tests" / "BUILD.bazel").write_text("#\n")
            self.assertTrue(benchmark.is_dds_root(root))

    def test_invalid(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            self.assertFalse(benchmark.is_dds_root(Path(tmp)))


def _git(repo: Path, *args: str) -> str:
    return subprocess.check_output(
        [
            "git",
            "-c",
            "core.fsmonitor=false",
            "-c",
            "advice.detachedHead=false",
            "-c",
            "core.autocrlf=false",
            "-c",
            "core.eol=lf",
            "-c",
            "safe.directory=*",
            "-C",
            str(repo),
            *args,
        ],
        text=True,
    ).strip()


def _setup_repo(repo: Path) -> None:
    (repo / "library" / "tests").mkdir(parents=True)
    (repo / "hands").mkdir()
    (repo / "MODULE.bazel").write_text('module(name = "dds")\n')
    (repo / "library" / "tests" / "BUILD.bazel").write_text("# test BUILD\n")
    (repo / "hands" / "list100.txt").write_text("hand\n")
    _git(repo, "init", "-q", "-b", "main")
    _git(repo, "config", "user.email", "test@example.com")
    _git(repo, "config", "user.name", "Test")
    _git(repo, "add", "MODULE.bazel", "library/tests/BUILD.bazel", "hands/list100.txt")
    _git(repo, "commit", "-q", "-m", "initial")
    _git(repo, "checkout", "-q", "-b", "other")
    (repo / "other.txt").write_text("other\n")
    _git(repo, "add", "other.txt")
    _git(repo, "commit", "-q", "-m", "other")
    _git(repo, "checkout", "-q", "main")
    _git(repo, "tag", "head-tag")


class TestRejectCheckoutBinaryWithBranch(unittest.TestCase):
    def test_rejects_relative_checkout_dtest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            old_cwd = Path.cwd()
            try:
                os.chdir(root)
                with self.assertRaises(benchmark.BenchmarkError) as ctx:
                    benchmark.reject_checkout_binary_with_branch(
                        root,
                        [
                            ("branch", "develop"),
                            ("binary", str(benchmark.DTEST_REL)),
                        ],
                    )
            finally:
                os.chdir(old_cwd)
            self.assertIn("checkout's", str(ctx.exception))
            self.assertIn(str(benchmark.DTEST_REL), str(ctx.exception))

    def test_rejects_absolute_checkout_dtest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            abs_path = root / benchmark.DTEST_REL
            with self.assertRaises(benchmark.BenchmarkError) as ctx:
                benchmark.reject_checkout_binary_with_branch(
                    root,
                    [("branch", "develop"), ("binary", str(abs_path))],
                )
            self.assertIn("checkout's", str(ctx.exception))

    def test_allows_external_binary_with_branch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            benchmark.reject_checkout_binary_with_branch(
                root,
                [("branch", "develop"), ("binary", "/tmp/other-dtest")],
            )

    def test_allows_checkout_binary_without_branch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            benchmark.reject_checkout_binary_with_branch(
                root,
                [("binary", str(root / benchmark.DTEST_REL))],
            )

    def test_allows_branch_only(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            benchmark.reject_checkout_binary_with_branch(
                root,
                [("branch", "develop")],
            )

    def test_rejects_checkout_wasm_js_with_wasm_branch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            abs_path = root / benchmark.DTEST_WASM_JS_REL
            with self.assertRaises(benchmark.BenchmarkError) as ctx:
                benchmark.reject_checkout_binary_with_branch(
                    root,
                    [("wasm_branch", "develop"), ("binary", str(abs_path))],
                )
            self.assertIn("checkout's", str(ctx.exception))
            self.assertIn(str(benchmark.DTEST_WASM_JS_REL), str(ctx.exception))

    def test_allows_external_binary_with_wasm_branch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            benchmark.reject_checkout_binary_with_branch(
                root,
                [("wasm_branch", "develop"), ("binary", "/tmp/other-dtest")],
            )


class TestGitPrepForBranches(unittest.TestCase):
    def test_setup_repo_leaves_clean_worktree(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            status = _git(
                repo, "status", "--porcelain", "--untracked-files=normal"
            )
            self.assertEqual(status, "")

    def test_dirty_same_commit_allowed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            (repo / "MODULE.bazel").write_text(
                (repo / "MODULE.bazel").read_text() + "dirty\n"
            )
            for ref in ("main", ".", "head-tag"):
                specs = [("branch", ref)]
                resolved, orig = benchmark.git_prep_for_branches(repo, specs)
                self.assertEqual(orig, "main")
                self.assertNotEqual(resolved[0][1], ".")

    def test_dirty_other_commit_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            (repo / "MODULE.bazel").write_text(
                (repo / "MODULE.bazel").read_text() + "dirty\n"
            )
            with self.assertRaises(benchmark.BenchmarkError) as ctx:
                benchmark.git_prep_for_branches(repo, [("branch", "other")])
            self.assertIn("working tree not clean", str(ctx.exception))

    def test_untracked_other_commit_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            (repo / "scratch.txt").write_text("untracked\n")
            with self.assertRaises(benchmark.BenchmarkError) as ctx:
                benchmark.git_prep_for_branches(repo, [("branch", "other")])
            self.assertIn("working tree not clean", str(ctx.exception))

    def test_untracked_same_commit_allowed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            (repo / "scratch.txt").write_text("untracked\n")
            benchmark.git_prep_for_branches(repo, [("branch", "main")])

    def test_non_commit_revspec_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            with self.assertRaises(benchmark.BenchmarkError) as ctx:
                benchmark.git_prep_for_branches(repo, [("branch", "HEAD^{tree}")])
            self.assertIn("unknown git ref", str(ctx.exception))

    def test_wasm_branch_dot_resolved_like_branch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            resolved, orig = benchmark.git_prep_for_branches(
                repo, [("wasm_branch", ".")]
            )
            self.assertEqual(orig, "main")
            self.assertEqual(resolved, [("wasm_branch", "main")])

    def test_wasm_branch_dirty_other_commit_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            repo.mkdir()
            _setup_repo(repo)
            (repo / "MODULE.bazel").write_text(
                (repo / "MODULE.bazel").read_text() + "dirty\n"
            )
            with self.assertRaises(benchmark.BenchmarkError) as ctx:
                benchmark.git_prep_for_branches(repo, [("wasm_branch", "other")])
            self.assertIn("working tree not clean", str(ctx.exception))


class TestRunDtestWasm(unittest.TestCase):
    def test_js_binary_invokes_node_with_cwd(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            js = root / "dtest.js"
            wasm = root / "dtest.wasm"
            js.write_text("// stub\n")
            wasm.write_bytes(b"\0")
            hands = root / "list1.txt"
            hands.write_text("hand\n")
            runner = benchmark.BenchmarkRunner(root, benchmark.Config())
            seen: dict[str, object] = {}

            def fake_run(cmd, **kwargs):  # type: ignore[no-untyped-def]
                seen["cmd"] = list(cmd)
                seen["cwd"] = kwargs.get("cwd")
                return subprocess.CompletedProcess(
                    cmd,
                    0,
                    stdout=(
                        "Number of hands          1\n"
                        "User time (ms)           10\n"
                        "Sys time (ms)            1\n"
                    ),
                    stderr="",
                )

            with mock.patch("benchmark.subprocess.run", side_effect=fake_run):
                parsed = runner.run_dtest(js, "solve", hands)
            self.assertEqual(
                seen["cmd"],
                ["node", str(js.resolve()), "-f", str(hands.resolve()), "-s", "solve"],
            )
            self.assertEqual(seen["cwd"], js.resolve().parent)
            self.assertEqual(parsed.user_ms, 10.0)

    def test_js_resolves_relative_hands_path(self) -> None:
        # cwd for node is the wasm artifact dir; relative -f must not resolve there.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            js = root / "wasm_out" / "dtest.js"
            js.parent.mkdir()
            js.write_text("// stub\n")
            (root / "hands").mkdir()
            (root / "hands" / "list1.txt").write_text("hand\n")
            runner = benchmark.BenchmarkRunner(root, benchmark.Config())
            old = Path.cwd()
            try:
                os.chdir(root)
                cmd = runner.dtest_command(js, "solve", Path("hands/list1.txt"))
            finally:
                os.chdir(old)
            self.assertEqual(cmd[0], "node")
            self.assertEqual(cmd[cmd.index("-f") + 1], str((root / "hands" / "list1.txt").resolve()))
            self.assertTrue(Path(cmd[cmd.index("-f") + 1]).is_absolute())

    def test_js_resolves_relative_script_path(self) -> None:
        # Relative --binary .js must not be re-resolved against cwd=js.parent.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            js = root / "bazel-bin" / "wasm" / "dtest.js"
            js.parent.mkdir(parents=True)
            js.write_text("// stub\n")
            (root / "hands" / "list1.txt").parent.mkdir(parents=True, exist_ok=True)
            (root / "hands" / "list1.txt").write_text("hand\n")
            runner = benchmark.BenchmarkRunner(root, benchmark.Config())
            old = Path.cwd()
            try:
                os.chdir(root)
                rel_js = Path("bazel-bin/wasm/dtest.js")
                cmd = runner.dtest_command(rel_js, "solve", Path("hands/list1.txt"))
            finally:
                os.chdir(old)
            self.assertEqual(cmd[0], "node")
            self.assertEqual(cmd[1], str(js.resolve()))
            self.assertTrue(Path(cmd[1]).is_absolute())

    def test_wasm_sys_na_does_not_warn(self) -> None:
        err = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            js = root / "dtest.js"
            js.write_text("// stub\n")
            hands = root / "list1.txt"
            hands.write_text("hand\n")
            runner = benchmark.BenchmarkRunner(root, benchmark.Config(), err=err)

            def fake_run(cmd, **_kwargs):  # type: ignore[no-untyped-def]
                return subprocess.CompletedProcess(
                    cmd,
                    0,
                    stdout=(
                        "Number of hands                 1\n"
                        "User time (ms)                 21\n"
                        "Avg user time (ms)          21.00\n"
                        "Sys time (ms)                 n/a\n"
                    ),
                    stderr="",
                )

            with mock.patch("benchmark.subprocess.run", side_effect=fake_run):
                parsed = runner.run_dtest(js, "solve", hands)
            self.assertEqual(parsed.user_ms, 21.0)
            self.assertIsNone(parsed.sys_ms)
            self.assertNotIn("incomplete dtest timing", err.getvalue())

    def test_dry_run_js_prints_node_command(self) -> None:
        err = io.StringIO()
        with tempfile.TemporaryDirectory() as tmp:
            js = Path(tmp) / "dtest.js"
            runner = benchmark.BenchmarkRunner(
                Path(tmp),
                benchmark.Config(dry_run=True),
                err=err,
            )
            runner.run_dtest(js, "calc", Path(tmp) / "list1.txt")
            self.assertIn("DRY_RUN: node ", err.getvalue())
            self.assertIn(str(js), err.getvalue())


class TestDryRunMain(unittest.TestCase):
    def test_dry_run_prints_commands(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "library" / "tests").mkdir(parents=True)
            (root / "hands").mkdir()
            (root / "MODULE.bazel").write_text('module(name = "dds")\n')
            (root / "library" / "tests" / "BUILD.bazel").write_text("#\n")
            (root / "hands" / "list1.txt").write_text("x\n")
            (root / "hands" / "list10.txt").write_text("x\n")
            fake_bin = root / "fake_dtest"
            fake_bin.write_text("#!/bin/sh\nexit 0\n")
            fake_bin.chmod(0o755)
            with mock.patch("benchmark.Path.cwd", return_value=root):
                with mock.patch("sys.stdout"), mock.patch("sys.stderr"):
                    rc = benchmark.main(
                        ["--binary", str(fake_bin), "--max-deals", "10"],
                        env={"DRY_RUN": "1", "HANDS_DIR": str(root / "hands")},
                    )
            self.assertEqual(rc, 0)


if __name__ == "__main__":
    unittest.main()
