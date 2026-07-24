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
        self.assertEqual(parsed.ratio, 1.23)

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
        self.assertIsNone(parsed.ratio)


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
            },
        )
        self.assertEqual(cfg.repeats, 3)
        self.assertEqual(cfg.max_deals, 10)
        self.assertEqual(cfg.epsilon, 1.5)
        self.assertTrue(cfg.dry_run)
        self.assertTrue(cfg.details)

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

    def test_invalid_epsilon(self) -> None:
        with self.assertRaises(benchmark.BenchmarkError):
            benchmark.parse_args(["--epsilon", "-1"], env={})


class TestSummary(unittest.TestCase):
    def test_two_binary_ratio_and_note(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0, 1.0),
            benchmark.ResultRow("solve", "list100.txt", 1, 1, 50.0, 1.0, 0.5, 1.0, 0.5),
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
        self.assertIn("TOTAL", text)

    def test_equal_within_epsilon(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list100.txt", 0, 1, 100.0, 1.0, 1.0, 1.0, 1.0),
            benchmark.ResultRow("solve", "list100.txt", 1, 1, 100.2, 1.0, 1.002, 1.0, 1.0),
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
            benchmark.ResultRow("solve", "list1.txt", b, 1, 10.0, 0.0, 1.0, None, 0.1)
            for b in (0, 1, 2)
        ]
        text = benchmark.format_summary(
            rows,
            labels=["a", "b", "c"],
            files=["list1.txt"],
            epsilon=0.5,
        )
        self.assertNotIn("rel", text)
        self.assertNotIn("note", text)

    def test_zero_baseline_avg_skips_ratio(self) -> None:
        rows = [
            benchmark.ResultRow("solve", "list1.txt", 0, 1, 0.0, 0.0, 0.0, None, 0.0),
            benchmark.ResultRow("solve", "list1.txt", 1, 1, 1.0, 0.0, 1.0, None, 0.1),
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
            benchmark.ResultRow("solve", "list1.txt", 0, 1, 100.0, 1.0, 1.0, 1.0, 1.0),
            # Incomplete dtest output: avg_user missing for binary 1
            benchmark.ResultRow("solve", "list1.txt", 1, 1, None, None, None, None, 0.5),
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
            benchmark.ResultRow("solve", "list1.txt", 0, 1, None, None, None, None, 0.1),
            benchmark.ResultRow("solve", "list1.txt", 1, 1, 50.0, 1.0, 0.5, 1.0, 0.5),
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


class TestGitPrepForBranches(unittest.TestCase):
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
