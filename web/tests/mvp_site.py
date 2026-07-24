"""Stage a self-contained DDS MVP site directory for tests."""
from __future__ import annotations

import http.server
import importlib.util
import os
import shutil
from pathlib import Path

STATIC_FILES = ("dds_mvp.html", "dds_mvp.css", "dds_mvp.js")

# Required for SharedArrayBuffer / WASM pthreads in Chromium.
CROSS_ORIGIN_ISOLATION_HEADERS = {
    "Cross-Origin-Opener-Policy": "same-origin",
    "Cross-Origin-Embedder-Policy": "require-corp",
}


def runfiles_root() -> Path:
    for key in ("RUNFILES_DIR", "TEST_SRCDIR"):
        if key in os.environ:
            return Path(os.environ[key])
    raise RuntimeError("not running under Bazel test")


def rlocation(relpath: str) -> Path:
    root = runfiles_root()
    for candidate in (root / relpath, root / "_main" / relpath):
        if candidate.exists():
            return candidate
    raise FileNotFoundError(relpath)


def _load_module(web_root: Path, name: str):
    path = web_root / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def stage_mvp_site(dest: Path) -> Path:
    """Copy HTML/JS/CSS and patched wasm artifacts into dest. Returns dest."""
    dest.mkdir(parents=True, exist_ok=True)
    web_root = rlocation("web")

    for name in STATIC_FILES:
        shutil.copyfile(web_root / name, dest / name)

    js_src = rlocation("web/dds_mvp_wasm.js")
    wasm_src = rlocation("web/dds_mvp_wasm.wasm")
    js_path = dest / "dds_mvp_wasm.js"
    wasm_path = dest / "dds_mvp_wasm.wasm"
    shutil.copyfile(js_src, js_path)
    shutil.copyfile(wasm_src, wasm_path)
    js_path.chmod(0o644)
    wasm_path.chmod(0o644)

    patch_mvp_wasm = _load_module(web_root, "patch_mvp_wasm")
    gen_wasm_bin_js = _load_module(web_root, "gen_wasm_bin_js")

    updated, code = patch_mvp_wasm.patch_text(js_path.read_text(encoding="utf-8"))
    if code != 0:
        raise RuntimeError("patch_mvp_wasm failed")
    js_path.write_text(updated, encoding="utf-8")

    (dest / "dds_mvp_wasm_bin.js").write_text(
        gen_wasm_bin_js.make_bin_js(wasm_path.read_bytes()),
        encoding="utf-8",
    )
    return dest


def make_isolated_http_handler(
    directory: Path,
    *,
    quiet: bool = True,
) -> type[http.server.SimpleHTTPRequestHandler]:
    """HTTP handler that serves *directory* with cross-origin isolation headers."""
    root = str(directory)

    class Handler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, request, client_address, server) -> None:
            super().__init__(request, client_address, server, directory=root)

        def end_headers(self) -> None:
            for key, value in CROSS_ORIGIN_ISOLATION_HEADERS.items():
                self.send_header(key, value)
            super().end_headers()

        def log_message(self, format: str, *args) -> None:  # noqa: A003
            if not quiet:
                super().log_message(format, *args)

    return Handler
