#!/usr/bin/env python3
"""Assert the shared library exports exactly the public C API and nothing else.

Given the built shared library and the checked-in Linux version script (used
here purely as the canonical list of expected symbol names), this test:

  * confirms every expected public symbol is exported, and
  * confirms no C++-mangled symbol (Itanium `_Z...`) leaks into the ABI.

Invoked as: export_set_test.py <shared-lib> <version_script.lds>
"""

import re
import subprocess
import sys


def expected_symbols(lds_path):
    """Parse the `global:` block of a version script into a symbol set."""
    symbols = set()
    in_global = False
    with open(lds_path, encoding="utf-8") as handle:
        for raw in handle:
            line = raw.strip()
            if line == "global:":
                in_global = True
                continue
            if line == "local:":
                in_global = False
                continue
            if in_global and line.endswith(";") and line != "*;":
                symbols.add(line[:-1].strip())
    return symbols


def exported_symbols(lib_path):
    """Return the set of externally-defined text symbols in the library."""
    if sys.platform == "darwin":
        out = subprocess.check_output(["nm", "-gU", lib_path], text=True)
        strip_underscore = True
    else:
        out = subprocess.check_output(
            ["nm", "-D", "--defined-only", lib_path], text=True
        )
        strip_underscore = False

    names = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        sym_type, name = parts[-2], parts[-1]
        if sym_type != "T":  # exported code symbols only
            continue
        if strip_underscore and name.startswith("_"):
            name = name[1:]
        names.add(name)
    return names


def main(argv):
    lib_path, lds_path = argv[1], argv[2]
    expected = expected_symbols(lds_path)
    exported = exported_symbols(lib_path)

    missing = sorted(expected - exported)
    if missing:
        print("FAIL: expected public symbols not exported:", *missing, sep="\n  ")
        return 1

    mangled = sorted(s for s in exported if re.match(r"^_?_Z", s))
    if mangled:
        print("FAIL: C++-mangled symbols leaked into the ABI:", *mangled, sep="\n  ")
        return 1

    extra = sorted(exported - expected)
    if extra:
        print("FAIL: unexpected symbols exported:", *extra, sep="\n  ")
        return 1

    print(f"OK: {len(expected)} public symbols exported, no leaks.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
