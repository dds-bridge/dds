/// @file duplicate_boards_test.cpp
/// @brief Batch APIs must give repeated boards the same results as solving
///        each board on its own.
///
/// The batch paths once carried duplicate-detection helpers
/// (detect_solve_duplicates, detect_calc_duplicates, detect_play_duplicates)
/// that nothing called. These tests pin the behavior that matters without
/// them: a batch that repeats boards, interleaved with a different board,
/// solves every entry correctly, single-threaded and with several workers.

#include <gtest/gtest.h>

#include <thread>
#include <utility>
#include <vector>

#include <api/dll.h>
#include <dds/dds.hpp>

namespace
{

// PBN: N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3
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

// The known deal with the East and West hands swapped, so it solves
// differently from the known deal.
DdTableDeal make_swapped_deal()
{
    DdTableDeal deal = make_known_deal();
    for (int s = 0; s < DDS_SUITS; s++)
        std::swap(deal.cards[1][s], deal.cards[3][s]);
    return deal;
}

Deal to_deal(const DdTableDeal& table_deal, const int trump, const int first)
{
    Deal dl{};
    for (int h = 0; h < DDS_HANDS; h++)
        for (int s = 0; s < DDS_SUITS; s++)
            dl.remainCards[h][s] = table_deal.cards[h][s];
    dl.trump = trump;
    dl.first = first;
    return dl;
}

// Thread counts to try: one worker, and several when the machine has them.
std::vector<int> thread_counts()
{
    std::vector<int> counts = {1};
    if (std::thread::hardware_concurrency() > 2)
        counts.push_back(4);
    return counts;
}

void expect_future_tricks_equal(const FutureTricks& want,
                                const FutureTricks& got, const int board)
{
    ASSERT_EQ(want.cards, got.cards) << "board=" << board;
    for (int c = 0; c < want.cards; c++)
    {
        EXPECT_EQ(want.suit[c], got.suit[c]) << "board=" << board << " c=" << c;
        EXPECT_EQ(want.rank[c], got.rank[c]) << "board=" << board << " c=" << c;
        EXPECT_EQ(want.equals[c], got.equals[c]) << "board=" << board << " c=" << c;
        EXPECT_EQ(want.score[c], got.score[c]) << "board=" << board << " c=" << c;
    }
}

void expect_tables_equal(const DdTableResults& want, const DdTableResults& got,
                         const int table)
{
    for (int strain = 0; strain < DDS_STRAINS; strain++)
        for (int hand = 0; hand < DDS_HANDS; hand++)
            EXPECT_EQ(want.res_table[strain][hand], got.res_table[strain][hand])
                << "table=" << table << " strain=" << strain << " hand=" << hand;
}

}  // namespace

TEST(DuplicateBoards, SolveAllBoardsRepeatedBoardsMatchSingleSolves)
{
    InitializeStaticMemory();
    const Deal a = to_deal(make_known_deal(), /*trump=*/DDS_NOTRUMP, /*first=*/0);
    const Deal b = to_deal(make_known_deal(), /*trump=*/0, /*first=*/1);
    const std::vector<Deal> batch = {a, b, a, a, b};

    std::vector<FutureTricks> want(batch.size());
    for (unsigned i = 0; i < batch.size(); i++)
        ASSERT_EQ(SolveBoard(batch[i], -1, 3, 1, &want[i], 0), RETURN_NO_FAULT);
    // The interleaved board must solve differently, or repeats prove nothing.
    ASSERT_TRUE(want[0].cards != want[1].cards ||
                want[0].score[0] != want[1].score[0] ||
                want[0].suit[0] != want[1].suit[0] ||
                want[0].rank[0] != want[1].rank[0]);

    Boards bo{};
    bo.no_of_boards = static_cast<int>(batch.size());
    for (unsigned i = 0; i < batch.size(); i++)
    {
        bo.deals[i] = batch[i];
        bo.target[i] = -1;
        bo.solutions[i] = 3;
        bo.mode[i] = 1;
    }

    for (const int threads : thread_counts())
    {
        SolvedBoards solved{};
        ASSERT_EQ(SolveAllBoardsBinN(&bo, &solved, threads), RETURN_NO_FAULT)
            << "threads=" << threads;
        ASSERT_EQ(solved.no_of_boards, bo.no_of_boards);
        for (int i = 0; i < bo.no_of_boards; i++)
            expect_future_tricks_equal(want[static_cast<unsigned>(i)],
                                       solved.solved_board[i], i);
    }
}

TEST(DuplicateBoards, CalcAllTablesRepeatedDealsMatchSingleTables)
{
    InitializeStaticMemory();
    const DdTableDeal a = make_known_deal();
    const DdTableDeal b = make_swapped_deal();
    const std::vector<DdTableDeal> batch = {a, b, a, a};

    std::vector<DdTableResults> want(batch.size());
    for (unsigned i = 0; i < batch.size(); i++)
        ASSERT_EQ(CalcDDtable(batch[i], &want[i]), RETURN_NO_FAULT);
    bool tables_differ = false;
    for (int strain = 0; strain < DDS_STRAINS; strain++)
        for (int hand = 0; hand < DDS_HANDS; hand++)
            tables_differ = tables_differ ||
                want[0].res_table[strain][hand] != want[1].res_table[strain][hand];
    ASSERT_TRUE(tables_differ) << "swapped deal must give a different table";

    DdTableDeals deals{};
    deals.no_of_tables = static_cast<int>(batch.size());
    for (unsigned i = 0; i < batch.size(); i++)
        deals.deals[i] = batch[i];
    int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

    for (const int threads : thread_counts())
    {
        DdTablesRes res{};
        ASSERT_EQ(CalcAllTablesN(&deals, -1, filter, &res, nullptr, threads),
                  RETURN_NO_FAULT)
            << "threads=" << threads;
        for (unsigned i = 0; i < batch.size(); i++)
            expect_tables_equal(want[i], res.results[i], static_cast<int>(i));
    }
}

TEST(DuplicateBoards, AnalyseAllPlaysRepeatedTracesMatchSingleAnalyses)
{
    InitializeStaticMemory();
    const Deal dl = to_deal(make_known_deal(), /*trump=*/DDS_NOTRUMP, /*first=*/0);

    // North leads C T, East C 4, South C A, West C 3; South leads D K, West
    // D 2, North D 5, East D 4.
    PlayTraceBin full{};
    full.number = 8;
    const int suits[] = {3, 3, 3, 3, 2, 2, 2, 2};
    const int ranks[] = {10, 4, 14, 3, 13, 2, 5, 4};
    for (int i = 0; i < full.number; i++)
    {
        full.suit[i] = suits[i];
        full.rank[i] = ranks[i];
    }
    PlayTraceBin first_trick = full;
    first_trick.number = 4;

    const std::vector<PlayTraceBin> batch = {full, first_trick, full, full};

    std::vector<SolvedPlay> want(batch.size());
    for (unsigned i = 0; i < batch.size(); i++)
        ASSERT_EQ(AnalysePlayBin(dl, batch[i], &want[i], 0), RETURN_NO_FAULT);
    ASSERT_EQ(want[0].number, 9);  // a result before each card and after the last
    ASSERT_EQ(want[1].number, 5);

    Boards bo{};
    PlayTracesBin plays{};
    bo.no_of_boards = static_cast<int>(batch.size());
    plays.no_of_boards = bo.no_of_boards;
    for (unsigned i = 0; i < batch.size(); i++)
    {
        bo.deals[i] = dl;
        bo.target[i] = -1;
        bo.solutions[i] = 1;
        bo.mode[i] = 1;
        plays.plays[i] = batch[i];
    }

    for (const int chunk : {1, 2})
    {
        SolvedPlays solved{};
        ASSERT_EQ(AnalyseAllPlaysBin(&bo, &plays, &solved, chunk), RETURN_NO_FAULT)
            << "chunk=" << chunk;
        ASSERT_EQ(solved.no_of_boards, bo.no_of_boards);
        for (unsigned i = 0; i < batch.size(); i++)
        {
            const SolvedPlay& w = want[i];
            const SolvedPlay& g = solved.solved[i];
            ASSERT_EQ(w.number, g.number) << "board=" << i;
            for (int t = 0; t < w.number; t++)
                EXPECT_EQ(w.tricks[t], g.tricks[t])
                    << "board=" << i << " trick index=" << t;
        }
    }
}
