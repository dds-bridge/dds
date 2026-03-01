#include <array>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>

#include <dds/dds.hpp>

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
    throw std::runtime_error("DDS error " + std::to_string(code) + ": " + std::string(message.data()));
}

auto register_solve_bindings(py::module_& module) -> void
{
    module.def(
        "solve_board",
        [](const py::dict& deal,
           const int target,
           const int solutions,
           const int mode,
           const int thread_index) {
            FutureTricks future_tricks{};
            const Deal native_deal = dds3_python::dict_to_deal(deal);
            const int code = SolveBoard(
                native_deal,
                target,
                solutions,
                mode,
                &future_tricks,
                thread_index);
            throw_on_dds_error(code);
            return dds3_python::future_tricks_to_dict(future_tricks);
        },
        py::arg("deal"),
        py::arg("target") = -1,
        py::arg("solutions") = 3,
        py::arg("mode") = 0,
        py::arg("thread_index") = 0,
        "Solve a single bridge deal from binary format.\n\n"
        "Args:\n"
        "    deal (dict): Deal dict with keys 'trump', 'first', 'remain_cards', 'current_trick_suit', "
        "'current_trick_rank'.\n"
        "    target (int, optional): Target number of tricks for optimization (-1 = no target). Default: -1\n"
        "    solutions (int, optional): Depth of search (1-3, higher = more branches). Default: 3\n"
        "    mode (int, optional): 0 = auto, 1 = thread depth 6, 2 = node depth 12. Default: 0\n"
        "    thread_index (int, optional): Thread ID for transposition table access. Default: 0\n\n"
        "Returns:\n"
        "    dict: Result dict with keys 'nodes', 'cards', 'suit', 'rank', 'equals', 'score'.\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails (invalid suit/rank range).\n"
        "    RuntimeError: If DDS solver returns error code.");

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
           const int thread_index) {
            FutureTricks future_tricks{};
            const DealPBN native_deal = dds3_python::pbn_to_deal(
                remain_cards,
                trump,
                first,
                current_trick_suit,
                current_trick_rank);
            const int code = SolveBoardPBN(
                native_deal,
                target,
                solutions,
                mode,
                &future_tricks,
                thread_index);
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
        "Solve a single bridge deal from PBN (Portable Bridge Notation) format.\n\n"
        "Args:\n"
        "    remain_cards (str): Remaining cards in PBN format (e.g., 'N:AK.234.456.789T...').\n"
        "    trump (int, optional): Trump suit (0=♠, 1=♥, 2=♦, 3=♣, 4=NT). Default: 4\n"
        "    first (int, optional): Seat that plays first (0=N, 1=E, 2=S, 3=W). Default: 0\n"
        "    current_trick_suit (tuple, optional): Suits in current trick (3-tuple of ints, 0-3). Default: (0, 0, 0)\n"
        "    current_trick_rank (tuple, optional): Ranks in current trick (3-tuple of ints, 0-14). Default: (0, 0, 0)\n"
        "    target (int, optional): Target number of tricks for optimization (-1 = no target). Default: -1\n"
        "    solutions (int, optional): Depth of search (1-3, higher = more branches). Default: 3\n"
        "    mode (int, optional): 0 = auto, 1 = thread depth 6, 2 = node depth 12. Default: 0\n"
        "    thread_index (int, optional): Thread ID for transposition table access. Default: 0\n\n"
        "Returns:\n"
        "    dict: Result dict with keys 'nodes', 'cards', 'suit', 'rank', 'equals', 'score'.\n\n"
        "Raises:\n"
        "    ValueError: If PBN format is invalid or input validation fails.\n"
        "    RuntimeError: If DDS solver returns error code.");
}

auto register_table_bindings(py::module_& module) -> void
{
    module.def(
        "calc_dd_table",
        [](const py::dict& table_deal) {
            DdTableResults table_results{};
            const DdTableDeal native_deal = dds3_python::dict_to_dd_table_deal(table_deal);
            const int code = CalcDDtable(native_deal, &table_results);
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
            // Validate and convert trump_filter
            const auto trump_filter_vec = dds3_python::sequence_to_bounded_int_vector(
                trump_filter,
                DDS_STRAINS,
                0,
                1,
                "trump_filter");

            // Convert list of PBN strings to DdTableDealsPBN
            const auto native_deals = dds3_python::list_to_dd_table_deals_pbn(
                deals_pbn,
                MAXNOOFTABLES);

            // Allocate result structures
            DdTablesRes tables_res{};
            AllParResults all_par_results{};

            // Call C++ API
            const int code = CalcAllTablesPBN(
                &native_deals,
                mode,
                trump_filter_vec.data(),
                &tables_res,
                &all_par_results);
            throw_on_dds_error(code);

            // Build result dict
            py::dict result;
            result["no_of_boards"] = tables_res.no_of_boards;
            result["tables"] = dds3_python::dd_tables_res_to_list(tables_res, native_deals.no_of_tables);
            if (mode != -1) {
                result["par_results"] = dds3_python::all_par_results_to_list(
                    all_par_results,
                    native_deals.no_of_tables);
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
        "          'par_results' (list): List of par result dicts (present only when mode != -1).\n\n"
        "Raises:\n"
        "    ValueError: If PBN format is invalid, trump_filter invalid, or too many tables.\n"
        "    RuntimeError: If DDS solver returns error code.");
}

auto register_par_bindings(py::module_& module) -> void
{
    module.def(
        "par",
        [](const py::dict& table_results, const int vulnerable) {
            const DdTableResults native_table = dds3_python::dict_to_dd_table_results(table_results);
            ParResults par_results{};
            const int code = Par(&native_table, &par_results, vulnerable);
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

}  // namespace

PYBIND11_MODULE(_dds3, module)
{
    module.doc() = "dds3 Python extension (MVP wrappers)";

    register_solve_bindings(module);
    register_table_bindings(module);
    register_par_bindings(module);

    module.def("api_root", []() {
        return "dds.hpp";
    });
    module.def("module_name", []() {
        return "_dds3";
    });
}
