from dds3 import api_root
from dds3 import calc_all_tables_pbn
from dds3 import calc_dd_table
from dds3 import module_name
from dds3 import par
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
