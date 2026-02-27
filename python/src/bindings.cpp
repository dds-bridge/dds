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
        py::arg("thread_index") = 0);

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
        py::arg("trump"),
        py::arg("first"),
        py::arg("current_trick_suit") = py::make_tuple(0, 0, 0),
        py::arg("current_trick_rank") = py::make_tuple(0, 0, 0),
        py::arg("target") = -1,
        py::arg("solutions") = 3,
        py::arg("mode") = 0,
        py::arg("thread_index") = 0);
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
        py::arg("table_deal"));
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
        py::arg("vulnerable"));
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
