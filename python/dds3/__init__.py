try:
    from ._dds3 import api_root
    from ._dds3 import calc_all_tables_pbn
    from ._dds3 import calc_dd_table
    from ._dds3 import calc_par
    from ._dds3 import calc_par_from_table
    from ._dds3 import module_name
    from ._dds3 import par
    from ._dds3 import SolverContext
    from ._dds3 import solve_all_boards_bin
    from ._dds3 import solve_all_boards_pbn
    from ._dds3 import solve_board
    from ._dds3 import solve_board_pbn
except ImportError:
    # Fallback for environments where _dds3 is available as a top-level module
    from _dds3 import api_root
    from _dds3 import calc_all_tables_pbn
    from _dds3 import calc_dd_table
    from _dds3 import calc_par
    from _dds3 import calc_par_from_table
    from _dds3 import module_name
    from _dds3 import par
    from _dds3 import SolverContext
    from _dds3 import solve_all_boards_bin
    from _dds3 import solve_all_boards_pbn
    from _dds3 import solve_board
    from _dds3 import solve_board_pbn

__all__ = [
    "api_root",
    "calc_all_tables_pbn",
    "calc_dd_table",
    "calc_par",
    "calc_par_from_table",
    "module_name",
    "par",
    "SolverContext",
    "solve_all_boards_bin",
    "solve_all_boards_pbn",
    "solve_board",
    "solve_board_pbn",
]
