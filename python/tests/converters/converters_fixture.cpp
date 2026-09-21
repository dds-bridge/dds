/// @file converters_fixture.cpp
/// @brief Test-only pybind module that calls the Deal converters directly.
///
/// dict_to_deal is reachable from Python through every solve API, but
/// deal_to_dict is not bound anywhere: it exists for extensions built
/// outside this repository, so nothing in dds3 calls it and nothing in the
/// existing test suite could reach it. This module gives the converters a
/// direct caller, which is what tests/test_converters.py drives.
///
/// It lives in its own package, and reaches converters.cpp through
/// //python:converters_srcs and converters.hpp through
/// //python:converters_hdrs, so it consumes them exactly as an extension in
/// another repository would: across a package boundary, through the public
/// targets, with no path into //python's own files. That makes it a test of
/// the reuse contract and not only of the conversion -- narrow either
/// target's visibility and this package stops building.

#include <pybind11/pybind11.h>

#include <dds3/converters.hpp>

namespace py = pybind11;

namespace
{

/// A Deal filled in directly, without going through dict_to_deal, so
/// deal_to_dict can be checked on its own rather than only as the inverse of
/// the one function it is most likely to share a mistake with. Every field
/// holds a distinct value, so a test can tell a transposed index from a
/// correct one.
auto reference_deal() -> Deal
{
    Deal deal{};
    deal.trump = 2;
    deal.first = 3;

    for (int i = 0; i < 3; ++i) {
        deal.currentTrickSuit[i] = i;       // 0, 1, 2
        deal.currentTrickRank[i] = 14 - i;  // 14, 13, 12
    }

    for (int hand = 0; hand < DDS_HANDS; ++hand) {
        for (int suit = 0; suit < DDS_SUITS; ++suit) {
            // Distinct per (hand, suit), and inside dict_to_deal's own
            // 0..0x7FFC bound so the resulting dict can be fed back through
            // it unchanged.
            deal.remainCards[hand][suit] =
                static_cast<unsigned int>((hand * DDS_SUITS + suit + 1) * 4);
        }
    }

    return deal;
}

}  // namespace

PYBIND11_MODULE(_converters_fixture, m)
{
    m.doc() = "Test-only direct access to the Deal converters.";

    m.def(
        "reference_deal_as_dict",
        []() { return dds3_python::deal_to_dict(reference_deal()); },
        "deal_to_dict() of a Deal built in C++, never through dict_to_deal.");

    m.def(
        "round_trip",
        [](const py::dict& deal_input) {
            return dds3_python::deal_to_dict(dds3_python::dict_to_deal(deal_input));
        },
        py::arg("deal"),
        "deal_to_dict(dict_to_deal(deal)).");
}
