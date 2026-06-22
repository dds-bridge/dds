try:
    from ._dds3 import analyse_all_plays_pbn
    from ._dds3 import analyse_play_pbn
    from ._dds3 import api_root
    from ._dds3 import calc_all_tables_pbn
    from ._dds3 import calc_dd_table
    from ._dds3 import calc_par
    from ._dds3 import calc_par_from_table
    from ._dds3 import dealer_par
    from ._dds3 import initialise_static_memory
    from ._dds3 import module_name
    from ._dds3 import par
    from ._dds3 import set_max_threads
    from ._dds3 import SolverContext
    from ._dds3 import solve_all_boards_bin
    from ._dds3 import solve_all_boards_pbn
    from ._dds3 import solve_board
    from ._dds3 import solve_board_pbn
except ImportError:
    # Fallback for environments where _dds3 is available as a top-level module
    from _dds3 import analyse_all_plays_pbn
    from _dds3 import analyse_play_pbn
    from _dds3 import api_root
    from _dds3 import calc_all_tables_pbn
    from _dds3 import calc_dd_table
    from _dds3 import calc_par
    from _dds3 import calc_par_from_table
    from _dds3 import dealer_par
    from _dds3 import initialise_static_memory
    from _dds3 import module_name
    from _dds3 import par
    from _dds3 import set_max_threads
    from _dds3 import SolverContext
    from _dds3 import solve_all_boards_bin
    from _dds3 import solve_all_boards_pbn
    from _dds3 import solve_board
    from _dds3 import solve_board_pbn

__all__ = [
    "analyse_all_plays_pbn",
    "analyse_play_pbn",
    "api_root",
    "calc_all_tables_pbn",
    "calc_dd_table",
    "calc_par",
    "calc_par_from_table",
    "dealer_par",
    "initialise_static_memory",
    "module_name",
    "par",
    "set_max_threads",
    "SolverContext",
    "solve_all_boards_bin",
    "solve_all_boards_pbn",
    "solve_board",
    "solve_board_pbn",
]
