#!/usr/bin/env python3
"""Run //wasm:dtest_wasm under Node.js (for `bazel run //wasm:run_dtest_wasm`)."""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


def _runfiles_root() -> Path | None:
    for key in ("RUNFILES_DIR", "TEST_SRCDIR"):
        if key in os.environ:
            return Path(os.environ[key])
    return None


def rlocation(relpath: str) -> Path:
    """Resolve a runfiles path such as ``wasm/dtest.js``."""
    name = Path(relpath).name
    candidates: list[Path] = []

    # Same Bazel package: data deps land beside this script under bazel run.
    # Do not Path.resolve() — that follows the runfiles symlink into the
    # source tree and loses the sibling .js/.wasm data deps.
    candidates.append(Path(__file__).absolute().parent / name)

    root = _runfiles_root()
    if root is not None:
        candidates.append(root / relpath)
        candidates.append(root / "_main" / relpath)

    manifest = os.environ.get("RUNFILES_MANIFEST_FILE")
    if manifest:
        def manifest_lookup(key: str) -> Path | None:
            try:
                with open(manifest, encoding="utf-8") as f:
                    for line in f:
                        if line.startswith(key + " "):
                            _, path = line.rstrip("\n").split(" ", 1)
                            return Path(path)
            except OSError:
                return None
            return None

        for key in (relpath, f"_main/{relpath}"):
            resolved = manifest_lookup(key)
            if resolved is not None:
                candidates.append(resolved)
                break

    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise FileNotFoundError(relpath)


def build_node_command(*, js_path: str, argv: list[str]) -> list[str]:
    return ["node", js_path, *argv]


def resolve_cwd(env: dict[str, str]) -> str | None:
    # `bazel run` sets this to the directory where the user invoked Bazel, so
    # relative -f paths like hands/list1.txt resolve under NODERAWFS.
    return env.get("BUILD_WORKING_DIRECTORY") or None


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if not shutil.which("node"):
        print("error: node not found on PATH", file=sys.stderr)
        return 127

    js = rlocation("wasm/dtest.js")
    cmd = build_node_command(js_path=str(js), argv=args)
    cwd = resolve_cwd(dict(os.environ))
    return subprocess.call(cmd, cwd=cwd)


if __name__ == "__main__":
    raise SystemExit(main())
