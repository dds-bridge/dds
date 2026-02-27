#include <pybind11/pybind11.h>

#include <dds/dds.hpp>

namespace py = pybind11;

PYBIND11_MODULE(dds3_ext, m)
{
    m.doc() = "dds3 Python extension (scaffold)";

    m.def("api_root", []() {
        return "dds.hpp";
    });
}
