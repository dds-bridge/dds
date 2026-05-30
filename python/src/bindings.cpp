#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>

#include <api/calc_par.hpp>
#include <dds/dds.hpp>
#include <pbn.hpp>
#include <solver_context/solver_context.hpp>

#include "converters.hpp"

namespace py = pybind11;

namespace
{

auto throw_on_dds_error(const int code) -> void
{
    if (code == RETURN_NO_FAULT) {
        return;
    }

    std::array<char, 80> message{};
    ErrorMessage(code, message.data());
    const std::string error_text =
        "DDS error " + std::to_string(code) + ": " + std::string(message.data());

    switch (code) {
    // Input validation errors from user-provided data: expose as ValueError in Python.
    case RETURN_TRUMP_WRONG:
    case RETURN_FIRST_WRONG:
    case RETURN_PBN_FAULT:
    case RETURN_TARGET_WRONG_LO:
    case RETURN_TARGET_WRONG_HI:
    case RETURN_SOLNS_WRONG_LO:
    case RETURN_SOLNS_WRONG_HI:
    case RETURN_THREAD_INDEX:
    case RETURN_MODE_WRONG_LO:
    case RETURN_MODE_WRONG_HI:
    case RETURN_NO_SUIT:
    case RETURN_TOO_MANY_TABLES:
        throw py::value_error(error_text);
    default:
        // All other errors are treated as solver/runtime failures.
        throw std::runtime_error(error_text);
    }
}

auto register_solve_bindings(py::module_& module) -> void
{
    // Overload 1: solve_board with optional context
    module.def(
        "solve_board",
        [](const py::dict& deal,
           const int target,
           const int solutions,
           const int mode,
           const int thread_index,
           py::object context_obj) {
            FutureTricks future_tricks{};
            const Deal native_deal = dds3_python::dict_to_deal(deal);
            SolverContext* context_ptr = nullptr;
            if (!context_obj.is_none()) {
                context_ptr = py::cast<SolverContext*>(context_obj);
            }
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;

                if (context_ptr == nullptr) {
                    // Create temporary context (old behavior)
                    code = SolveBoard(
                        native_deal,
                        target,
                        solutions,
                        mode,
                        &future_tricks,
                        thread_index);
                } else {
                    // Use provided context
                    code = solve_board(
                        *context_ptr,
                        native_deal,
                        target,
                        solutions,
                        mode,
                        &future_tricks);
                }
            }
            throw_on_dds_error(code);
            return dds3_python::future_tricks_to_dict(future_tricks);
        },
        py::arg("deal"),
        py::arg("target") = -1,
        py::arg("solutions") = 3,
        py::arg("mode") = 0,
        py::arg("thread_index") = 0,
        py::arg("context") = py::none(),
        "Solve a single bridge deal from binary format.\n\n"
        "Args:\n"
        "    deal (dict): Deal dict with keys 'trump', 'first', 'remain_cards', 'current_trick_suit', "
        "'current_trick_rank'.\n"
        "    target (int, optional): Target number of tricks for optimization (-1 = no target). Default: -1\n"
        "    solutions (int, optional): Depth of search (1-3, higher = more branches). Default: 3\n"
        "    mode (int, optional): 0 = auto, 1 = thread depth 6, 2 = node depth 12. Default: 0\n"
        "    thread_index (int, optional): Thread ID for transposition table access. Default: 0\n"
        "    context (SolverContext, optional): Reusable solver context for efficiency. Default: None\n\n"
        "Returns:\n"
        "    dict: Result dict with keys 'nodes', 'cards', 'suit', 'rank', 'equals', 'score'.\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails (invalid suit/rank range).\n"
        "    RuntimeError: If DDS solver returns error code.\n\n"
        "Example (with context reuse for multiple boards):\n"
        "    context = dds3.SolverContext()\n"
        "    result1 = dds3.solve_board(deal1, context=context)\n"
        "    result2 = dds3.solve_board(deal2, context=context)  # Reuses context");

    // Overload 2: solve_board_pbn with optional context
    module.def(
        "solve_board_pbn",
        [](const std::string& remain_cards,
           const int trump,
           const int first,
           const py::sequence& current_trick_suit,
           const py::sequence& current_trick_rank,
           const int target,
           const int solutions,
           const int mode,
           const int thread_index,
           py::object context_obj) {
            FutureTricks future_tricks{};
            const DealPBN native_deal = dds3_python::pbn_to_deal(
                remain_cards,
                trump,
                first,
                current_trick_suit,
                current_trick_rank);
            SolverContext* context_ptr = nullptr;
            if (!context_obj.is_none()) {
                context_ptr = py::cast<SolverContext*>(context_obj);
            }
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;

                if (context_ptr == nullptr) {
                    // Create temporary context (old behavior)
                    code = SolveBoardPBN(
                        native_deal,
                        target,
                        solutions,
                        mode,
                        &future_tricks,
                        thread_index);
                } else {
                    // Use provided context by converting PBN deal and calling
                    // the context-aware C++ SolveBoard overload.
                    Deal native_binary_deal{};
                    if (convert_from_pbn(native_deal.remainCards, native_binary_deal.remainCards) != RETURN_NO_FAULT) {
                        code = RETURN_PBN_FAULT;
                    } else {
                        for (int k = 0; k <= 2; ++k) {
                            native_binary_deal.currentTrickRank[k] = native_deal.currentTrickRank[k];
                            native_binary_deal.currentTrickSuit[k] = native_deal.currentTrickSuit[k];
                        }
                        native_binary_deal.first = native_deal.first;
                        native_binary_deal.trump = native_deal.trump;

                        code = solve_board(
                            *context_ptr,
                            native_binary_deal,
                            target,
                            solutions,
                            mode,
                            &future_tricks);
                    }
                }
            }
            throw_on_dds_error(code);
            return dds3_python::future_tricks_to_dict(future_tricks);
        },
        py::arg("remain_cards"),
        py::arg("trump") = 4,  // NT default
        py::arg("first") = 0,  // North default
        py::arg("current_trick_suit") = py::make_tuple(0, 0, 0),
        py::arg("current_trick_rank") = py::make_tuple(0, 0, 0),
        py::arg("target") = -1,
        py::arg("solutions") = 3,
        py::arg("mode") = 0,
        py::arg("thread_index") = 0,
        py::arg("context") = py::none(),
        "Solve a single bridge deal from PBN (Portable Bridge Notation) format.\n\n"
        "Args:\n"
        "    remain_cards (str): Remaining cards in PBN format (e.g., 'N:AK.234.456.789T...').\n"
        "    trump (int, optional): Trump suit (0=♠, 1=♥, 2=♦, 3=♣, 4=NT). Default: 4\n"
        "    first (int, optional): Seat that plays first (0=N, 1=E, 2=S, 3=W). Default: 0\n"
        "    current_trick_suit (tuple, optional): Suits in current trick (3-tuple of ints, 0-3). Default: (0, 0, 0)\n"
        "    current_trick_rank (tuple, optional): Ranks in current trick (3-tuple of ints, 0 or 2-14). Default: (0, 0, 0)\n"
        "    target (int, optional): Target number of tricks for optimization (-1 = no target). Default: -1\n"
        "    solutions (int, optional): Depth of search (1-3, higher = more branches). Default: 3\n"
        "    mode (int, optional): 0 = auto, 1 = thread depth 6, 2 = node depth 12. Default: 0\n"
        "    thread_index (int, optional): Thread ID for transposition table access. Default: 0\n"
        "    context (SolverContext, optional): Reusable solver context for efficiency. Default: None\n\n"
        "Returns:\n"
        "    dict: Result dict with keys 'nodes', 'cards', 'suit', 'rank', 'equals', 'score'.\n\n"
        "Raises:\n"
        "    ValueError: If PBN format is invalid or input validation fails.\n"
        "    RuntimeError: If DDS solver returns error code.");

    module.def(
        "solve_all_boards_pbn",
        [](const py::list& boards) {
            const auto board_count = boards.size();
            if (board_count > MAXNOOFBOARDS) {
                throw py::value_error(
                    "Number of boards (" + std::to_string(board_count) +
                    ") exceeds maximum (" + std::to_string(MAXNOOFBOARDS) + ")");
            }

            BoardsPBN native_boards{};
            native_boards.no_of_boards = static_cast<int>(board_count);

            for (std::size_t i = 0; i < board_count; ++i) {
                const py::dict board = py::cast<py::dict>(boards[i]);
                const std::string remain_cards = py::cast<std::string>(board["remain_cards"]);
                const int trump = board.contains("trump") ? py::cast<int>(board["trump"]) : 4;
                const int first = board.contains("first") ? py::cast<int>(board["first"]) : 0;
                const py::sequence trick_suit = board.contains("current_trick_suit")
                    ? py::cast<py::sequence>(board["current_trick_suit"])
                    : py::cast<py::sequence>(py::make_tuple(0, 0, 0));
                const py::sequence trick_rank = board.contains("current_trick_rank")
                    ? py::cast<py::sequence>(board["current_trick_rank"])
                    : py::cast<py::sequence>(py::make_tuple(0, 0, 0));
                native_boards.deals[i] =
                    dds3_python::pbn_to_deal(remain_cards, trump, first, trick_suit, trick_rank);
                native_boards.target[i] =
                    board.contains("target") ? py::cast<int>(board["target"]) : -1;
                native_boards.solutions[i] =
                    board.contains("solutions") ? py::cast<int>(board["solutions"]) : 3;
                native_boards.mode[i] =
                    board.contains("mode") ? py::cast<int>(board["mode"]) : 0;
            }

            SolvedBoards solved_boards{};
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;
                code = SolveAllBoards(&native_boards, &solved_boards);
            }
            throw_on_dds_error(code);

            py::list result;
            for (int i = 0; i < solved_boards.no_of_boards; ++i) {
                result.append(dds3_python::future_tricks_to_dict(solved_boards.solved_board[i]));
            }
            return result;
        },
        py::arg("boards"),
        "Solve multiple bridge deals in PBN format.\n\n"
        "Args:\n"
        "    boards (list): List of board dicts, each with:\n"
        "        remain_cards (str): Remaining cards in PBN format (e.g., 'N:AK.234.456.789T...').\n"
        "        trump (int, optional): Trump suit (0=♠, 1=♥, 2=♦, 3=♣, 4=NT). Default: 4\n"
        "        first (int, optional): Seat that plays first (0=N, 1=E, 2=S, 3=W). Default: 0\n"
        "        current_trick_suit (tuple, optional): Suits in current trick. Default: (0, 0, 0)\n"
        "        current_trick_rank (tuple, optional): Ranks in current trick. Default: (0, 0, 0)\n"
        "        target (int, optional): Target number of tricks (-1 = no target). Default: -1\n"
        "        solutions (int, optional): Depth of search (1-3). Default: 3\n"
        "        mode (int, optional): 0=auto, 1=thread depth 6, 2=node depth 12. Default: 0\n\n"
        "Returns:\n"
        "    list: List of result dicts with keys 'nodes', 'cards', 'suit', 'rank', 'equals', 'score'.\n\n"
        "Raises:\n"
        "    ValueError: If PBN format is invalid, input validation fails, or too many boards.\n"
        "    RuntimeError: If DDS solver returns error code.");

    module.def(
        "solve_all_boards_bin",
        [](const py::list& boards) {
            const auto board_count = boards.size();
            if (board_count > MAXNOOFBOARDS) {
                throw py::value_error(
                    "Number of boards (" + std::to_string(board_count) +
                    ") exceeds maximum (" + std::to_string(MAXNOOFBOARDS) + ")");
            }

            Boards native_boards{};
            native_boards.no_of_boards = static_cast<int>(board_count);

            for (std::size_t i = 0; i < board_count; ++i) {
                const py::dict board = py::cast<py::dict>(boards[i]);
                native_boards.deals[i] = dds3_python::dict_to_deal(board);
                native_boards.target[i] =
                    board.contains("target") ? py::cast<int>(board["target"]) : -1;
                native_boards.solutions[i] =
                    board.contains("solutions") ? py::cast<int>(board["solutions"]) : 3;
                native_boards.mode[i] =
                    board.contains("mode") ? py::cast<int>(board["mode"]) : 0;
            }

            SolvedBoards solved_boards{};
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;
                code = SolveAllBoardsBin(&native_boards, &solved_boards);
            }
            throw_on_dds_error(code);

            py::list result;
            for (int i = 0; i < solved_boards.no_of_boards; ++i) {
                result.append(dds3_python::future_tricks_to_dict(solved_boards.solved_board[i]));
            }
            return result;
        },
        py::arg("boards"),
        "Solve multiple bridge deals in binary format.\n\n"
        "Args:\n"
        "    boards (list): List of board dicts, each with:\n"
        "        trump (int): Trump suit (0=♠, 1=♥, 2=♦, 3=♣, 4=NT).\n"
        "        first (int): Seat that plays first (0=N, 1=E, 2=S, 3=W).\n"
        "        remain_cards (list): 4x4 nested list of card bitmasks (hand x suit, bits 2-14).\n"
        "        current_trick_suit (tuple): Suits in current trick (3-tuple of ints, 0-3).\n"
        "        current_trick_rank (tuple): Ranks in current trick (3-tuple of ints, 0 or 2-14).\n"
        "        target (int, optional): Target number of tricks (-1 = no target). Default: -1\n"
        "        solutions (int, optional): Depth of search (1-3). Default: 3\n"
        "        mode (int, optional): 0=auto, 1=thread depth 6, 2=node depth 12. Default: 0\n\n"
        "Returns:\n"
        "    list: List of result dicts with keys 'nodes', 'cards', 'suit', 'rank', 'equals', 'score'.\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails or too many boards.\n"
        "    RuntimeError: If DDS solver returns error code.");
}


auto register_table_bindings(py::module_& module) -> void
{
    module.def(
        "calc_dd_table",
        [](const py::dict& table_deal) {
            DdTableResults table_results{};
            const DdTableDeal native_deal = dds3_python::dict_to_dd_table_deal(table_deal);
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;
                code = CalcDDtable(native_deal, &table_results);
            }
            throw_on_dds_error(code);
            return dds3_python::dd_table_results_to_dict(table_results);
        },
        py::arg("table_deal"),
        "Calculate the double-dummy table for all contracts and strains.\n\n"
        "Args:\n"
        "    table_deal (dict): DD table deal dict with key 'cards' (4x4 nested list).\n\n"
        "Returns:\n"
        "    dict: Double-dummy table with key 'res_table' (5x4 nested list).\n"
        "          res_table[strain][hand] = tricks available for that strain/hand.\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails (invalid card distribution).\n"
        "    RuntimeError: If DDS solver returns error code.");

    module.def(
        "calc_all_tables_pbn",
        [](const py::list& deals_pbn, const int mode, const py::sequence& trump_filter) {
            // Validate mode parameter
            if (mode < -1 || mode > 3) {
                throw py::value_error(
                    "mode has invalid value " + std::to_string(mode) +
                    " (expected -1=disabled, 0=none, 1=both, 2=NS, 3=EW)");
            }

            // Validate and convert trump_filter
            const auto trump_filter_vec = dds3_python::sequence_to_bounded_int_vector(
                trump_filter,
                DDS_STRAINS,
                0,
                1,
                "trump_filter");

            const int included_strains = static_cast<int>(std::count(
                trump_filter_vec.begin(),
                trump_filter_vec.end(),
                0));

            // Par computation constraints:
            // - DDS only populates par results when ALL strains are included (see DDS CalcAllTables)
            // - AllParResults::par_results has fixed capacity MAXNOOFTABLES (not MAXNOOFTABLES*DDS_STRAINS)
            // - To ensure safe access, we either:
            //   (a) Reject par computation (mode != -1) unless all strains are included, OR
            //   (b) Cap max_tables to MAXNOOFTABLES when par is requested with all strains
            // This implements approach (a): reject invalid combinations and approach (b): cap appropriately.

            const bool wants_par = mode != -1;
            const bool can_compute_par = included_strains == DDS_STRAINS;

            if (wants_par && !can_compute_par) {
                throw py::value_error(
                    "Par computation (mode != -1) requires all strains to be included "
                    "(trump_filter must be all zeros)");
            }

            // Calculate max_tables based on par configuration:
            // - With par (all strains): limited to MAXNOOFTABLES (par buffer capacity)
            // - Without par (any filter): can use full MAXNOOFTABLES * DDS_STRAINS capacity
            const int max_tables =
                (wants_par && can_compute_par)
                    ? MAXNOOFTABLES
                    : ((included_strains > 0) ? ((MAXNOOFTABLES * DDS_STRAINS) / included_strains)
                                              : MAXNOOFTABLES);

            // Convert list of PBN strings to DdTableDealsPBN
            const auto native_deals = dds3_python::list_to_dd_table_deals_pbn(
                deals_pbn,
                static_cast<std::size_t>(max_tables));

            // Allocate result structures
            DdTablesRes tables_res{};
            AllParResults all_par_results{};

            // Call C++ API
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;
                code = CalcAllTablesPBN(
                    &native_deals,
                    mode,
                    trump_filter_vec.data(),
                    &tables_res,
                    &all_par_results);
            }
            throw_on_dds_error(code);

            // Build result dict
            py::dict result;
            result["no_of_boards"] = tables_res.no_of_boards;
            result["tables"] = dds3_python::dd_tables_res_to_list(tables_res, native_deals.no_of_tables);
            
            // Include par_results only if par was actually computed:
            // - Par computation requires mode != -1 AND all strains included
            // - This ensures AllParResults buffer (capacity MAXNOOFTABLES) won't be accessed out-of-bounds
            // - When conditions not met, return empty list for API consistency
            if (wants_par && can_compute_par) {
                result["par_results"] = dds3_python::all_par_results_to_list(
                    all_par_results,
                    native_deals.no_of_tables);
            } else {
                result["par_results"] = py::list();  // Empty when par disabled or strains filtered
            }
            return result;
        },
        py::arg("deals_pbn"),
        py::arg("mode") = -1,
        py::arg("trump_filter") = py::make_tuple(0, 0, 0, 0, 0),
        "Calculate double-dummy tables for multiple PBN deals with optional par scores.\n\n"
        "Args:\n"
        "    deals_pbn (list): List of PBN strings (e.g., ['N:AK.234.456.789T...', ...]).\n"
        "    mode (int, optional): Par vulnerability mode (-1=disabled, 0=none, 1=both, 2=NS, 3=EW). Default: -1\n"
        "    trump_filter (sequence, optional): Strains to skip (0=include, 1=skip). Default: (0,0,0,0,0)\n"
        "                                     Order: [♠, ♥, ♦, ♣, NT]\n\n"
        "Returns:\n"
        "    dict: Result dict with keys:\n"
        "          'no_of_boards' (int): Total number of calculated boards.\n"
        "          'tables' (list): List of DD table dicts, one per input deal.\n"
        "              Each table dict contains 'res_table' (5x4) in fixed strain order\n"
        "              [♠, ♥, ♦, ♣, NT]. Rows for filtered-out strains are returned as zeros.\n"
        "          'par_results' (list): List of par result dicts (empty when mode=-1).\n\n"
        "Raises:\n"
        "    ValueError: If PBN format is invalid, trump_filter invalid, or too many tables.\n"
        "    RuntimeError: If DDS solver returns error code.");
}

auto register_par_bindings(py::module_& module) -> void
{
    module.def(
        "par",
        [](const py::dict& table_results, const int vulnerable) {
            if (vulnerable < 0 || vulnerable > 3) {
                throw py::value_error(
                    "vulnerable has invalid value " + std::to_string(vulnerable) +
                    " (expected 0=none, 1=both, 2=NS, 3=EW)");
            }

            const DdTableResults native_table = dds3_python::dict_to_dd_table_results(table_results);
            ParResults par_results{};
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;
                code = Par(&native_table, &par_results, vulnerable);
            }
            throw_on_dds_error(code);
            return dds3_python::par_results_to_dict(par_results);
        },
        py::arg("table_results"),
        py::arg("vulnerable") = 0,
        "Calculate par contracts and scores for a given double-dummy table.\n\n"
        "Args:\n"
        "    table_results (dict): DD table results dict with key 'res_table' (5x4 nested list).\n"
        "    vulnerable (int): Vulnerability (0=none, 1=both, 2=NS, 3=EW).\n\n"
        "Returns:\n"
        "    dict: Par results with keys 'par_score' and 'par_contracts_string'.\n"
        "          par_contracts_string[ns] = contract string (e.g., '6NT+1', '7C=').\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails (invalid table or vulnerability).\n"
        "    RuntimeError: If DDS solver returns error code.");
}

auto register_calc_par_bindings(py::module_& module) -> void
{
    module.def(
        "calc_par",
        [](const py::dict& table_deal, const int vulnerable) {
            if (vulnerable < 0 || vulnerable > 3) {
                throw py::value_error(
                    "vulnerable has invalid value " + std::to_string(vulnerable) +
                    " (expected 0=none, 1=both, 2=NS, 3=EW)");
            }

            const DdTableDeal native_deal = dds3_python::dict_to_dd_table_deal(table_deal);
            DdTableResults table_results{};
            ParResults par_results{};
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;
                code = calc_par(
                    native_deal,
                    vulnerable,
                    &table_results,
                    &par_results);
            }
            throw_on_dds_error(code);
            
            // Return both DD table and par results
            py::dict result;
            result["dd_table"] = dds3_python::dd_table_results_to_dict(table_results);
            result["par_results"] = dds3_python::par_results_to_dict(par_results);
            return result;
        },
        py::arg("table_deal"),
        py::arg("vulnerable") = 0,
        "Calculate double-dummy table and par contracts for a deal.\n\n"
        "Combines CalcDDtable and Par operations in a single call. Creates a temporary\n"
        "solver context internally for each call. For repeated calculations, prefer\n"
        "calc_par_from_table if DD tables are already available, to avoid redundant\n"
        "table computation.\n\n"
        "Args:\n"
        "    table_deal (dict): Deal dict with key 'cards' (4x4 nested list of card bitmasks).\n"
        "                       cards[hand][suit] where hand=0-3 (N,E,S,W), suit=0-3 (♠,♥,♦,♣)\n"
        "                       Each card bitmask has bits 2-14 set for present ranks (2-A).\n"
        "    vulnerable (int): Vulnerability (0=none, 1=both, 2=NS, 3=EW). Default: 0\n\n"
        "Returns:\n"
        "    dict: Result dict with two keys:\n"
        "        'dd_table': DD table results (key 'res_table' = 5x4 nested list)\n"
        "        'par_results': Par results (keys 'par_score' and 'par_contracts_string')\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails (invalid cards or vulnerability).\n"
        "    RuntimeError: If DDS solver returns error code.");

    module.def(
        "calc_par_from_table",
        [](const py::dict& table_results, const int vulnerable) {
            if (vulnerable < 0 || vulnerable > 3) {
                throw py::value_error(
                    "vulnerable has invalid value " + std::to_string(vulnerable) +
                    " (expected 0=none, 1=both, 2=NS, 3=EW)");
            }

            const DdTableResults native_table = dds3_python::dict_to_dd_table_results(table_results);
            ParResults par_results{};
            int code = RETURN_NO_FAULT;
            {
                py::gil_scoped_release release;
                code = calc_par_from_table(
                    &native_table,
                    vulnerable,
                    &par_results);
            }
            throw_on_dds_error(code);
            return dds3_python::par_results_to_dict(par_results);
        },
        py::arg("table_results"),
        py::arg("vulnerable") = 0,
        "Calculate par contracts from a pre-computed double-dummy table.\n\n"
        "Lightweight alternative to calc_par when the DD table is already available.\n"
        "More efficient than calc_par when computing par for multiple deals with the same DD table,\n"
        "or when par needs to be recalculated with different vulnerability.\n\n"
        "Args:\n"
        "    table_results (dict): DD table results dict with key 'res_table' (5x4 nested list).\n"
        "    vulnerable (int): Vulnerability (0=none, 1=both, 2=NS, 3=EW). Default: 0\n\n"
        "Returns:\n"
        "    dict: Par results with keys 'par_score' and 'par_contracts_string'.\n"
        "          par_contracts_string[ns] = contract string (e.g., '6NT+1', '7C=').\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails (invalid table or vulnerability).\n"
        "    RuntimeError: If DDS solver returns error code.");
}
}  // namespace

PYBIND11_MODULE(_dds3, module)
{
    module.doc() = "dds3 Python extension (MVP wrappers)";

    // Register SolverContext class for context reuse
    py::class_<SolverContext>(
        module,
        "SolverContext",
        "A reusable solver context that maintains state across multiple solve operations.\n\n"
        "Creating a single context and reusing it for multiple solve_board calls is more\n"
        "efficient than creating a new context for each call.\n\n"
        "Example:\n"
        "    context = dds3.SolverContext()\n"
        "    result1 = dds3.solve_board(deal1, context=context)\n"
        "    result2 = dds3.solve_board(deal2, context=context)  # Reuses cached state\n")
        .def(py::init<>(), "Create a new solver context.");

    register_solve_bindings(module);
    register_table_bindings(module);
    register_par_bindings(module);
    register_calc_par_bindings(module);

    module.def("api_root", []() {
        return "dds.hpp";
    });
    module.def("module_name", []() {
        return "_dds3";
    });
}
