#!/usr/bin/env python3
"""Assert the shared library exports exactly the public C API and nothing else.

The expected symbol set is derived directly from the public headers (dll.h,
dds_c_api.h) via the same parser that generates the export lists — NOT from the
generated .lds. This is deliberate: driving "expected" from the .lds would be
tautological (the .lds also drives what the linker exports), so a public symbol
added to a header but never regenerated into the .lds would go undetected. By
parsing the headers instead, a stale .lds surfaces as a missing export.

This test:
  * confirms every symbol declared in the headers is actually exported, and
  * confirms no C++-mangled symbol (Itanium `_Z...`) leaks into the ABI.

Invoked as: export_set_test.py <shared-lib> <header>...
"""

import re
import subprocess
import sys

import gen_export_lists


def expected_symbols(header_paths):
    """Public symbols declared in the headers (source of truth for the ABI)."""
    return set(gen_export_lists.parse_symbols(header_paths))


def exported_symbols(lib_path):
    """Return the set of externally-defined text symbols in the library.

    Only `-g` (external symbols) is passed; defined-vs-undefined is decided from
    the symbol-type column (uppercase `T` = external defined text), so we do not
    depend on `-U`, whose meaning differs between BSD nm and llvm/GNU nm.
    """
    if sys.platform == "darwin":
        out = subprocess.check_output(["nm", "-g", lib_path], text=True)
        strip_underscore = True
    else:
        out = subprocess.check_output(["nm", "-D", "-g", lib_path], text=True)
        strip_underscore = False

    names = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        sym_type, name = parts[-2], parts[-1]
        if sym_type != "T":  # external defined text symbols only ('U'/'t' excluded)
            continue
        if strip_underscore and name.startswith("_"):
            name = name[1:]
        names.add(name)
    return names


def main(argv):
    lib_path, header_paths = argv[1], argv[2:]
    expected = expected_symbols(header_paths)
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
