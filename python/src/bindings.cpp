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
        "    deal (dict): Deal dict with keys 'trump', 'first', 'cards', 'current_trick_suit', "
        "'current_trick_rank'.\n"
        "    target (int, optional): Target number of tricks for optimization (-1 = no target). Default: -1\n"
        "    solutions (int, optional): Depth of search (1-3, higher = more branches). Default: 3\n"
        "    mode (int, optional): 0 = auto, 1 = thread depth 6, 2 = node depth 12. Default: 0\n"
        "    thread_index (int, optional): Thread ID for transposition table access. Default: 0\n\n"
        "Returns:\n"
        "    dict: Result dict with keys 'return_code', 'solution_count', 'tricks', 'score'.\n\n"
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
        "    dict: Result dict with keys 'return_code', 'solution_count', 'tricks', 'score'.\n\n"
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
        "    table_deal (dict): DD table deal dict with key 'remain_cards' (52 integers).\n\n"
        "Returns:\n"
        "    dict: Double-dummy table with keys 'return_code' and 'res_table' (5x4 nested list).\n"
        "          res_table[strain][suit] = tricks available for that strain/suit.\n\n"
        "Raises:\n"
        "    ValueError: If input validation fails (invalid card distribution).\n"
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
        py::arg("vulnerable"),
        "Calculate par contracts and scores for a given double-dummy table.\n\n"
        "Args:\n"
        "    table_results (dict): DD table results dict with key 'res_table' (5x4 nested list).\n"
        "    vulnerable (int): Vulnerability (0=neither, 1=NS, 2=EW, 3=both).\n\n"
        "Returns:\n"
        "    dict: Par results with keys 'return_code', 'par_scores', and 'par_contracts'.\n"
        "          par_contracts[ns][contract_type] = contract string (e.g., '6NT+1', '7C=').\n\n"
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
