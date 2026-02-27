#include <pybind11/pybind11.h>

#include <dds/dds.hpp>

#include "converters.hpp"

namespace py = pybind11;

namespace
{

auto register_solve_bindings(py::module_& module) -> void
{
    module.def("solve_placeholder", []() {
        return "solve";
    });
}

auto register_table_bindings(py::module_& module) -> void
{
    module.def("table_placeholder", []() {
        return "table";
    });
}

auto register_par_bindings(py::module_& module) -> void
{
    module.def("par_placeholder", []() {
        return "par";
    });
}

}  // namespace

PYBIND11_MODULE(_dds3, module)
{
    module.doc() = "dds3 Python extension (scaffold)";

    register_solve_bindings(module);
    register_table_bindings(module);
    register_par_bindings(module);

    module.def("api_root", []() {
        return "dds.hpp";
    });
    module.def("module_name", []() {
        return "_dds3";
    });
    module.def("convert_string_placeholder", [](const std::string& text) {
        return dds3_python::convert_string_placeholder(text);
    });
}
