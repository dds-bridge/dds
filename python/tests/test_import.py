from dds3 import api_root
from dds3 import calc_all_tables_pbn
from dds3 import calc_dd_table
from dds3 import calc_par
from dds3 import calc_par_from_table
from dds3 import module_name
from dds3 import par
from dds3 import SolverContext
from dds3 import solve_board
from dds3 import solve_board_pbn


def test_import_and_api_root() -> None:
    assert api_root() == "dds.hpp"
    assert module_name() == "_dds3"
    assert callable(solve_board)
    assert callable(solve_board_pbn)
    assert callable(calc_dd_table)
    assert callable(calc_all_tables_pbn)
    assert callable(par)
    assert callable(calc_par)
    assert callable(calc_par_from_table)
    assert SolverContext is not None
    # Verify SolverContext can be instantiated
    ctx = SolverContext()
    assert ctx is not None
