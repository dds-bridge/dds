#!/usr/bin/env python3
"""Unit tests for gen_export_lists.parse_symbols.

Exercises the parser directly on header snippets — the actual logic worth
testing — rather than the linked library. Covers the flat trailing-return
`auto STDCALL Name(` form, the shim `Type name(` form, and the tokens the
parser must ignore (preprocessor #define lines and comments).
"""

import os
import tempfile
import unittest

import gen_export_lists as gel


def parse(*snippets):
    paths = []
    try:
        for text in snippets:
            handle = tempfile.NamedTemporaryFile(
                "w", suffix=".h", delete=False, encoding="utf-8"
            )
            handle.write(text)
            handle.close()
            paths.append(handle.name)
        return gel.parse_symbols(paths)
    finally:
        for path in paths:
            os.unlink(path)


class ParseSymbolsTest(unittest.TestCase):

    def test_flat_trailing_return_form(self):
        header = (
            "EXTERN_C DLLEXPORT auto STDCALL SolveBoard(\n"
            "  struct Deal dl) -> int;\n"
            "EXTERN_C DLLEXPORT auto STDCALL GetDDSInfo(\n"
            "  struct DDSInfo * info) -> void;\n"
        )
        self.assertEqual(parse(header), ["GetDDSInfo", "SolveBoard"])

    def test_shim_pointer_form(self):
        header = (
            "DLLEXPORT int dds_c_solve_board(void* ctx, const struct Deal* dl);\n"
            "DLLEXPORT void dds_c_destroy_solvercontext(void* ctx);\n"
        )
        self.assertEqual(
            parse(header),
            ["dds_c_destroy_solvercontext", "dds_c_solve_board"],
        )

    def test_ignores_defines_and_comments(self):
        # The macro-defining lines and comments must not leak DLLEXPORT,
        # STDCALL, __declspec or commented-out declarations into the export set.
        header = (
            "#define DLLEXPORT __declspec(dllexport)\n"
            "#define STDCALL __stdcall\n"
            "// EXTERN_C DLLEXPORT auto STDCALL Commented(int) -> int;\n"
            "EXTERN_C DLLEXPORT auto STDCALL RealOne(int) -> int;\n"
        )
        self.assertEqual(parse(header), ["RealOne"])

    def test_dedup_and_sorted_across_headers(self):
        a = "DLLEXPORT int dds_c_calc_par(void* c);\n"
        b = (
            "DLLEXPORT int dds_c_calc_par(void* c);\n"  # duplicate across headers
            "EXTERN_C DLLEXPORT auto STDCALL CalcPar(int) -> int;\n"
        )
        self.assertEqual(parse(a, b), ["CalcPar", "dds_c_calc_par"])

    def test_empty_when_no_exports(self):
        self.assertEqual(parse("int internal_only(void);\n"), [])


if __name__ == "__main__":
    unittest.main()
