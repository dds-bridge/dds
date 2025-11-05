#include <gtest/gtest.h>
#include <api/dds.h>
class TrickThreeBugTests : public ::testing::Test {
     protected:
        TrickThreeBugTests() = default;
};

inline size_t dds_max(futureTricks const & fut)
{
    int res = 0;
    for(int i = 0; i < 13 && fut.rank[i] > 0; ++i)
    {
        res = std::max(res, fut.score[i]);
    }

    return static_cast<size_t>(res);
}

TEST_F(TrickThreeBugTests, test_declarer_makes_nine_tricks) {
    SetMaxThreads(0);

    int target = 0;
    int solutions = 3;
    int mode = 0;
    int thread_index = 0;
    struct futureTricks fut = {
        .nodes=0,
        .cards = 0,
        .suit = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .rank = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .equals = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .score = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    struct deal dl = {
        .trump = 4, // No Trump
        .first = 2, // South to play
        .currentTrickSuit = {0, 0, 0},
        .currentTrickRank = {5, 13, 0},
        .remainCards = {
            {512, 4096, 12320, 27184}, // North
            {256, 2576, 16792, 5120}, // East
            {3076, 17408, 580, 264}, // South
            {192, 324, 3072, 196}  // West
        }
    };

    auto ret = SolveBoard(dl, target, solutions, mode, &fut, thread_index);
    auto max = dds_max(fut);

    ASSERT_TRUE(max == 9);
}