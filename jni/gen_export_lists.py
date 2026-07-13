#!/usr/bin/env python3
"""Generate linker export lists for the DDS shared library.

Parses the public C-ABI headers (dll.h, dds_c_api.h) for their exported
function symbols and emits:

  * a Linux version script (--version-script) with the symbols under `global:`
    and `local: *;` for everything else, and
  * a macOS exported-symbols list (-exported_symbols_list) with each symbol
    prefixed by a leading underscore.

Windows needs no file: exports there are driven by __declspec(dllexport).

The two lists are derived from the same parsed set so they cannot drift. Run
via Bazel genrule or manually; output is deterministic (symbols sorted).
"""

import argparse
import re
import sys

# A public export is a non-preprocessor, non-comment line that mentions the
# DLLEXPORT marker and declares a function: capture the identifier that
# immediately precedes the opening parenthesis. This handles both the
# trailing-return `auto STDCALL Name(` form in dll.h and the plain
# `Type dds_c_name(` form in dds_c_api.h.
_DECL_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def parse_symbols(paths):
    symbols = set()
    for path in paths:
        with open(path, encoding="utf-8") as handle:
            for raw in handle:
                line = raw.strip()
                if not line or line.startswith("#") or line.startswith("//"):
                    continue
                if "DLLEXPORT" not in line:
                    continue
                match = _DECL_RE.search(line)
                if not match:
                    continue
                name = match.group(1)
                # Skip the macro tokens themselves and the STDCALL marker.
                if name in ("DLLEXPORT", "STDCALL", "EXTERN_C"):
                    continue
                symbols.add(name)
    return sorted(symbols)


def write_version_script(symbols, path):
    lines = ["{", "  global:"]
    lines += [f"    {name};" for name in symbols]
    lines += ["  local:", "    *;", "};", ""]
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))


def write_exported_symbols(symbols, path):
    lines = [f"_{name}" for name in symbols]
    lines.append("")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--headers", nargs="+", required=True,
        help="Public C-ABI headers to scan for exported symbols.",
    )
    parser.add_argument("--linux", help="Output path for the Linux version script.")
    parser.add_argument("--macos", help="Output path for the macOS exported-symbols list.")
    args = parser.parse_args(argv)

    symbols = parse_symbols(args.headers)
    if not symbols:
        print("error: no exported symbols found", file=sys.stderr)
        return 1

    if args.linux:
        write_version_script(symbols, args.linux)
    if args.macos:
        write_exported_symbols(symbols, args.macos)
    if not args.linux and not args.macos:
        # No outputs requested: print both to stdout for inspection.
        print("# symbols:", *symbols, sep="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
