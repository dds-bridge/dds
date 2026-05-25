import unittest

from dds3 import api_root
from dds3 import calc_all_tables_pbn
from dds3 import calc_dd_table
from dds3 import calc_par
from dds3 import calc_par_from_table
from dds3 import module_name
from dds3 import par
from dds3 import SolverContext
from dds3 import solve_all_boards_bin
from dds3 import solve_all_boards_pbn
from dds3 import solve_board
from dds3 import solve_board_pbn


class TestImport(unittest.TestCase):
    def test_import_and_api_root(self) -> None:
        self.assertEqual(api_root(), "dds.hpp")
        self.assertEqual(module_name(), "_dds3")
        self.assertTrue(callable(solve_board))
        self.assertTrue(callable(solve_board_pbn))
        self.assertTrue(callable(solve_all_boards_pbn))
        self.assertTrue(callable(solve_all_boards_bin))
        self.assertTrue(callable(calc_dd_table))
        self.assertTrue(callable(calc_all_tables_pbn))
        self.assertTrue(callable(par))
        self.assertTrue(callable(calc_par))
        self.assertTrue(callable(calc_par_from_table))
        self.assertIsNotNone(SolverContext)

        # Verify SolverContext can be instantiated.
        ctx = SolverContext()
        self.assertIsNotNone(ctx)


if __name__ == "__main__":
    unittest.main()
