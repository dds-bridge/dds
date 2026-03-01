#include "converters.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace dds3_python
{

auto sequence_to_int_vector(
    const py::sequence& values,
    const std::size_t expected_size,
    const std::string& field_name) -> std::vector<int>
{
    if (values.size() != expected_size) {
        throw py::value_error(field_name + " must have size " + std::to_string(expected_size));
    }

    std::vector<int> result;
    result.reserve(expected_size);
    for (const py::handle value : values) {
        result.push_back(py::cast<int>(value));
    }

    return result;
}

auto sequence_to_bounded_int_vector(
    const py::sequence& values,
    const std::size_t expected_size,
    const int minimum_value,
    const int maximum_value,
    const std::string& field_name) -> std::vector<int>
{
    const auto result = sequence_to_int_vector(values, expected_size, field_name);
    for (const int value : result) {
        if (value < minimum_value || value > maximum_value) {
            throw py::value_error(
                field_name + " has invalid value " + std::to_string(value) +
                " (expected range " + std::to_string(minimum_value) + ".." +
                std::to_string(maximum_value) + ")");
        }
    }

    return result;
}

auto dict_to_deal(const py::dict& deal_input) -> Deal
{
    Deal deal{};
    deal.trump = py::cast<int>(deal_input["trump"]);
    deal.first = py::cast<int>(deal_input["first"]);

    const auto trick_suit = sequence_to_bounded_int_vector(
        py::cast<py::sequence>(deal_input["current_trick_suit"]),
        3,
        0,
        DDS_SUITS - 1,
        "current_trick_suit");
    const auto trick_rank = sequence_to_bounded_int_vector(
        py::cast<py::sequence>(deal_input["current_trick_rank"]),
        3,
        0,
        14,
        "current_trick_rank");

    for (int i = 0; i < 3; ++i) {
        deal.currentTrickSuit[i] = trick_suit[static_cast<std::size_t>(i)];
        deal.currentTrickRank[i] = trick_rank[static_cast<std::size_t>(i)];
    }

    const auto remain_cards_rows = py::cast<py::sequence>(deal_input["remain_cards"]);
    if (remain_cards_rows.size() != DDS_HANDS) {
        throw py::value_error("remain_cards must have DDS_HANDS rows");
    }

    for (int hand = 0; hand < DDS_HANDS; ++hand) {
        const auto row = py::cast<py::sequence>(remain_cards_rows[hand]);
        if (row.size() != DDS_SUITS) {
            throw py::value_error("each remain_cards row must have DDS_SUITS values");
        }
        for (int suit = 0; suit < DDS_SUITS; ++suit) {
            deal.remainCards[hand][suit] = py::cast<unsigned int>(row[suit]);
        }
    }

    return deal;
}

auto pbn_to_deal(
    const std::string& remain_cards,
    const int trump,
    const int first,
    const py::sequence& current_trick_suit,
    const py::sequence& current_trick_rank) -> DealPBN
{
    DealPBN deal{};
    deal.trump = trump;
    deal.first = first;

    const auto trick_suit = sequence_to_bounded_int_vector(
        current_trick_suit,
        3,
        0,
        DDS_SUITS - 1,
        "current_trick_suit");
    const auto trick_rank = sequence_to_bounded_int_vector(
        current_trick_rank,
        3,
        0,
        14,
        "current_trick_rank");
    for (int i = 0; i < 3; ++i) {
        deal.currentTrickSuit[i] = trick_suit[static_cast<std::size_t>(i)];
        deal.currentTrickRank[i] = trick_rank[static_cast<std::size_t>(i)];
    }

    std::memset(deal.remainCards, 0, sizeof(deal.remainCards));
    const std::size_t copy_size = std::min(remain_cards.size(), sizeof(deal.remainCards) - 1U);
    std::memcpy(deal.remainCards, remain_cards.c_str(), copy_size);
    deal.remainCards[copy_size] = '\0';

    return deal;
}

auto dict_to_dd_table_deal(const py::dict& table_input) -> DdTableDeal
{
    DdTableDeal table_deal{};
    const auto cards_rows = py::cast<py::sequence>(table_input["cards"]);
    if (cards_rows.size() != DDS_HANDS) {
        throw py::value_error("cards must have DDS_HANDS rows");
    }

    for (int hand = 0; hand < DDS_HANDS; ++hand) {
        const auto row = py::cast<py::sequence>(cards_rows[hand]);
        if (row.size() != DDS_SUITS) {
            throw py::value_error("each cards row must have DDS_SUITS values");
        }
        for (int suit = 0; suit < DDS_SUITS; ++suit) {
            table_deal.cards[hand][suit] = py::cast<unsigned int>(row[suit]);
        }
    }

    return table_deal;
}

auto dict_to_dd_table_results(const py::dict& table_input) -> DdTableResults
{
    DdTableResults table_results{};
    const auto table_rows = py::cast<py::sequence>(table_input["res_table"]);
    if (table_rows.size() != DDS_STRAINS) {
        throw py::value_error("res_table must have DDS_STRAINS rows");
    }

    for (int strain = 0; strain < DDS_STRAINS; ++strain) {
        const auto row = py::cast<py::sequence>(table_rows[strain]);
        if (row.size() != DDS_HANDS) {
            throw py::value_error("each res_table row must have DDS_HANDS values");
        }
        for (int hand = 0; hand < DDS_HANDS; ++hand) {
            table_results.res_table[strain][hand] = py::cast<int>(row[hand]);
        }
    }

    return table_results;
}

auto future_tricks_to_dict(const FutureTricks& future_tricks) -> py::dict
{
    py::dict result;
    result["nodes"] = future_tricks.nodes;
    result["cards"] = future_tricks.cards;
    result["suit"] = py::make_tuple(
        future_tricks.suit[0], future_tricks.suit[1], future_tricks.suit[2], future_tricks.suit[3],
        future_tricks.suit[4], future_tricks.suit[5], future_tricks.suit[6], future_tricks.suit[7],
        future_tricks.suit[8], future_tricks.suit[9], future_tricks.suit[10], future_tricks.suit[11],
        future_tricks.suit[12]);
    result["rank"] = py::make_tuple(
        future_tricks.rank[0], future_tricks.rank[1], future_tricks.rank[2], future_tricks.rank[3],
        future_tricks.rank[4], future_tricks.rank[5], future_tricks.rank[6], future_tricks.rank[7],
        future_tricks.rank[8], future_tricks.rank[9], future_tricks.rank[10], future_tricks.rank[11],
        future_tricks.rank[12]);
    result["equals"] = py::make_tuple(
        future_tricks.equals[0], future_tricks.equals[1], future_tricks.equals[2], future_tricks.equals[3],
        future_tricks.equals[4], future_tricks.equals[5], future_tricks.equals[6], future_tricks.equals[7],
        future_tricks.equals[8], future_tricks.equals[9], future_tricks.equals[10], future_tricks.equals[11],
        future_tricks.equals[12]);
    result["score"] = py::make_tuple(
        future_tricks.score[0], future_tricks.score[1], future_tricks.score[2], future_tricks.score[3],
        future_tricks.score[4], future_tricks.score[5], future_tricks.score[6], future_tricks.score[7],
        future_tricks.score[8], future_tricks.score[9], future_tricks.score[10], future_tricks.score[11],
        future_tricks.score[12]);

    return result;
}

auto dd_table_results_to_dict(const DdTableResults& table_results) -> py::dict
{
    py::list rows;
    for (int strain = 0; strain < DDS_STRAINS; ++strain) {
        py::list row;
        for (int hand = 0; hand < DDS_HANDS; ++hand) {
            row.append(table_results.res_table[strain][hand]);
        }
        rows.append(row);
    }

    py::dict result;
    result["res_table"] = rows;
    return result;
}

auto par_results_to_dict(const ParResults& par_results) -> py::dict
{
    py::list par_score;
    py::list par_contracts;

    par_score.append(std::string(par_results.par_score[0]));
    par_score.append(std::string(par_results.par_score[1]));

    par_contracts.append(std::string(par_results.par_contracts_string[0]));
    par_contracts.append(std::string(par_results.par_contracts_string[1]));

    py::dict result;
    result["par_score"] = par_score;
    result["par_contracts_string"] = par_contracts;
    return result;
}

auto list_to_dd_table_deals_pbn(
    const py::list& deals_pbn,
    const std::size_t max_tables) -> DdTableDealsPBN
{
    const auto table_count = static_cast<std::size_t>(deals_pbn.size());

    if (table_count > max_tables) {
        throw py::value_error(
            "Number of tables (" + std::to_string(table_count) +
            ") exceeds maximum (" + std::to_string(max_tables) + ")");
    }

    DdTableDealsPBN result{};
    result.no_of_tables = static_cast<int>(table_count);

    for (std::size_t i = 0; i < table_count; ++i) {
        const auto pbn_str = py::cast<std::string>(deals_pbn[i]);
        if (pbn_str.length() >= 80) {
            throw py::value_error(
                "PBN string at index " + std::to_string(i) +
                " is too long (max 79 characters)");
        }
        // Copy PBN string to the cards field
        std::strcpy(result.deals[i].cards, pbn_str.c_str());
    }

    return result;
}

auto dd_tables_res_to_list(const DdTablesRes& tables_res, const int num_tables) -> py::list
{
    const int count = std::max(0, std::min(num_tables, MAXNOOFTABLES));

    py::list result;
    for (int i = 0; i < count; ++i) {
        result.append(dd_table_results_to_dict(tables_res.results[i]));
    }
    return result;
}

auto all_par_results_to_list(const AllParResults& all_par_results, const int num_tables) -> py::list
{
    py::list result;
    for (int i = 0; i < num_tables; ++i) {
        result.append(par_results_to_dict(all_par_results.par_results[i]));
    }
    return result;
}

}  // namespace dds3_python
