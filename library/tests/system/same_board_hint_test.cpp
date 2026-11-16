/// @file same_board_hint_test.cpp
/// @brief Regression tests for the South hint in full-deal table solves.
///
/// Each board is solved with North leading first, then re-solved for East,
/// South and West starting from a hint. South's hint must be partner North's
/// score. It once came from the FutureTricks that East's solve had already
/// overwritten. Results were unchanged, only the work, so these tests observe
/// the hint itself in both table-solving paths: CalcDDtableN
/// (calc_single_common_internal) and CalcAllTablesX (calc_single_deal_scores).

#include <gtest/gtest.h>

#include <array>
#include <mutex>

#include <api/dll.h>
#include <calc_tables.hpp>
#include <dds/dds.hpp>

namespace
{

constexpr int kNorth = 0;
constexpr int kSouth = 2;
constexpr int kWest = 3;
constexpr int kTricks = 13;

std::mutex hint_mutex;
std::array<int, DDS_STRAINS> south_hints{};
std::array<int, DDS_STRAINS> south_hint_calls{};

void record_hint(const int trump, const int leader, const int hint)
{
    if (leader != kSouth)
        return;
    const std::lock_guard<std::mutex> lock(hint_mutex);
    south_hints[trump] = hint;
    south_hint_calls[trump]++;
}

DdTableDeal make_known_deal()
{
    DdTableDeal deal{};
    deal.cards[0][0] = 0x1800 | 0x0040;
    deal.cards[0][1] = 0x2000 | 0x0060 | 0x0004;
    deal.cards[0][2] = 0x0800 | 0x0100 | 0x0020;
    deal.cards[0][3] = 0x0400 | 0x0200 | 0x0100;
    deal.cards[1][0] = 0x0100 | 0x0080 | 0x0008;
    deal.cards[1][1] = 0x0800 | 0x0200 | 0x0080;
    deal.cards[1][2] = 0x4000 | 0x0400 | 0x0080 | 0x0040 | 0x0010;
    deal.cards[1][3] = 0x1000 | 0x0010;
    deal.cards[2][0] = 0x2000 | 0x0020;
    deal.cards[2][1] = 0x0400 | 0x0100 | 0x0008;
    deal.cards[2][2] = 0x2000 | 0x1000 | 0x0200;
    deal.cards[2][3] = 0x4000 | 0x0080 | 0x0040 | 0x0020 | 0x0004;
    deal.cards[3][0] = 0x4000 | 0x0400 | 0x0200 | 0x0010 | 0x0004;
    deal.cards[3][1] = 0x4000 | 0x1000 | 0x0010;
    deal.cards[3][2] = 0x0008 | 0x0004;
    deal.cards[3][3] = 0x2000 | 0x0800 | 0x0008;
    return deal;
}

class SameBoardHintTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        InitializeStaticMemory();
        south_hints.fill(-1);
        south_hint_calls.fill(0);
        set_same_board_hint_observer(record_hint);
    }

    void TearDown() override
    {
        set_same_board_hint_observer(nullptr);
    }

    // South's hint must be North-South's tricks with North leading (West
    // declares). The old code used East-West's tricks with East leading
    // (North declares), so the deal must make those differ in some strain.
    static void expect_south_hints_from_north(const DdTableResults& table)
    {
        bool old_hint_differs = false;
        for (int strain = 0; strain < DDS_STRAINS; strain++)
        {
            const int north_lead_ns = kTricks - table.res_table[strain][kWest];
            const int east_lead_ew = kTricks - table.res_table[strain][kNorth];
            old_hint_differs = old_hint_differs || (north_lead_ns != east_lead_ew);

            EXPECT_EQ(south_hint_calls[strain], 1) << "strain=" << strain;
            EXPECT_EQ(south_hints[strain], north_lead_ns) << "strain=" << strain;
        }
        ASSERT_TRUE(old_hint_differs)
            << "Test deal cannot tell North's score from East's";
    }
};

}  // namespace

TEST_F(SameBoardHintTest, CalcDDtableSouthUsesNorthScore)
{
    DdTableResults table{};
    ASSERT_EQ(CalcDDtableN(make_known_deal(), &table, 1), RETURN_NO_FAULT);
    expect_south_hints_from_north(table);
}

TEST_F(SameBoardHintTest, CalcAllTablesXSouthUsesNorthScore)
{
    DdTableDeal deal = make_known_deal();
    DdTableResults table{};
    int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};
    ASSERT_EQ(
        CalcAllTablesX(1, &deal, -1, filter, &table, nullptr, 1),
        RETURN_NO_FAULT);
    expect_south_hints_from_north(table);
}
