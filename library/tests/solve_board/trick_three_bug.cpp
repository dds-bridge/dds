/// @file trick_three_bug.cpp
/// @brief Regression test for the trick three bug in solve_board.
/// @details Verifies the solver reports nine tricks for a known board.

// C++ standard library headers
#include <algorithm>
#include <cstddef>

// Third-party headers
#include <gtest/gtest.h>

// Project headers
#include <api/dds.h>

class TrickThreeBugTests : public ::testing::Test
{
protected:
    TrickThreeBugTests() = default;
};

/// @brief Return the maximum trick score from a FutureTricks result.
/// @param fut FutureTricks produced by the solver.
/// @return Maximum score value across all results.
inline auto dds_max(FutureTricks const & fut) -> size_t
{
    int res = 0;
    for (int i = 0; i < 13 && fut.rank[i] > 0; ++i) {
        res = std::max(res, fut.score[i]);
    }

    return static_cast<size_t>(res);
}

/// Test case: declarer makes nine tricks on the regression board.
/// @details Reproduces the original bug scenario and validates the fix.
TEST_F(TrickThreeBugTests, test_declarer_makes_nine_tricks)
{
    SetMaxThreads(0);

    const int target = 0;
    const int solutions = 3;
    const int mode = 0;
    const int thread_index = 0;
    struct FutureTricks fut = {
        .nodes=0,
        .cards = 0,
        .suit = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .rank = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .equals = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .score = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    struct Deal dl = {
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
    ASSERT_EQ(RETURN_NO_FAULT, ret);
    const auto max = dds_max(fut);

    ASSERT_EQ(static_cast<size_t>(9), max);
}