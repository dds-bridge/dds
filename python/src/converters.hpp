#pragma once

#include <string>
#include <vector>

#include <pybind11/pytypes.h>

#include <dds/dds.hpp>

namespace dds3_python
{

auto sequence_to_int_vector(
    const pybind11::sequence& values,
    std::size_t expected_size,
    const std::string& field_name) -> std::vector<int>;

auto dict_to_deal(const pybind11::dict& deal_input) -> Deal;
auto pbn_to_deal(
    const std::string& remain_cards,
    int trump,
    int first,
    const pybind11::sequence& current_trick_suit,
    const pybind11::sequence& current_trick_rank) -> DealPBN;
auto dict_to_dd_table_deal(const pybind11::dict& table_input) -> DdTableDeal;
auto dict_to_dd_table_results(const pybind11::dict& table_input) -> DdTableResults;

auto future_tricks_to_dict(const FutureTricks& future_tricks) -> pybind11::dict;
auto dd_table_results_to_dict(const DdTableResults& table_results) -> pybind11::dict;
auto par_results_to_dict(const ParResults& par_results) -> pybind11::dict;

}  // namespace dds3_python
