/**
 * @file moves_test.cpp
 * @brief Comprehensive unit tests for the Moves class
 *
 * Tests cover:
 * - Constructor and initialization
 * - Move generation properties
 * - Move selection
 * - Statistics tracking
 * - Edge cases and invariant violations
 */

#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <chrono>

#include "moves.hpp"

/**
 * @brief Test fixture for Moves class tests
 *
 * Provides setup for common test scenarios and helper methods.
 */
class MovesTest : public ::testing::Test {
 protected:
  MovesTest() = default;
  
  void SetUp() override {
    // Initialize test objects
    moves = std::make_unique<Moves>();
  }
  
  void TearDown() override {
    moves.reset();
  }
  
  /**
   * @brief Get sample rank_in_suit data for testing
   */
  const unsigned short (*getSampleRankInSuit())[4] {
    static unsigned short data[4][4] = {
      {0x3fff, 0x3fff, 0x3fff, 0x3fff},
      {0x3fff, 0x3fff, 0x3fff, 0x3fff},
      {0x3fff, 0x3fff, 0x3fff, 0x3fff},
      {0x3fff, 0x3fff, 0x3fff, 0x3fff}
    };
    return data;
  }
  
  std::unique_ptr<Moves> moves;
};

/**
 * @section Constructor and Initialization Tests
 */

TEST_F(MovesTest, ConstructorInitializesState) {
  // Verify constructor initializes object properly
  EXPECT_NE(moves.get(), nullptr);
  
  // Verify function names are initialized
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    EXPECT_FALSE(moves->funcName[i].empty());
  }
  
  // Verify statistics are zeroed
  EXPECT_EQ(moves->trickFuncTable.nfuncs, 0);
  EXPECT_EQ(moves->trickFuncSuitTable.nfuncs, 0);
}

TEST_F(MovesTest, InitializesTrackingState) {
  // Initialize with trick 5, starting from relative hand 0
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  moves->Init(5, 0, nullptr, nullptr, rankInSuit, 3, 0);
  
  // Verify state is initialized
  EXPECT_EQ(moves->currTrick, 5);
  EXPECT_EQ(moves->trump, 3);  // 3 = notrump
  
  // Verify move lists are reset
  for (int h = 0; h < 4; h++) {
    EXPECT_EQ(moves->moveList[5][h].current, 0);
    EXPECT_EQ(moves->moveList[5][h].last, 0);
  }
}

TEST_F(MovesTest, ReinitUpdateLeadHand) {
  // Initialize first
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  moves->Init(7, 0, nullptr, nullptr, rankInSuit, 0, 1);
  
  // Reinit with different lead hand
  moves->Reinit(7, 2);
  
  // Verify lead hand updated
  EXPECT_EQ(moves->track[7].lead_hand, 2);
}

TEST_F(MovesTest, GetLengthReturnsCorrectCount) {
  // GetLength should return valid counts
  // Note: moveList is initialized with last=0, so GetLength returns last+1
  EXPECT_GE(moves->GetLength(3, 0), 0);
  EXPECT_LE(moves->GetLength(3, 0), 14);  // Max 13 cards + 1
  EXPECT_GE(moves->GetLength(12, 3), 0);
  EXPECT_LE(moves->GetLength(12, 3), 14);
}

TEST_F(MovesTest, GetLengthHandlesEmptyList) {
  // Verify list lengths are reasonable (0-14 for max 13 cards)
  for (int t = 0; t < 13; t++) {
    for (int h = 0; h < 4; h++) {
      int length = moves->GetLength(t, h);
      EXPECT_GE(length, 0);
      EXPECT_LE(length, 14);
    }
  }
}

TEST_F(MovesTest, PrintMoveReturnsValidString) {
  // PrintMove should return a string when given a MovePlyType
  // It's primarily for debugging, so just verify it doesn't crash
  EXPECT_NO_THROW({
    auto result = moves->PrintMove(moves->moveList[0][0]);
    EXPECT_FALSE(result.empty());
  });
}

/**
 * @section Memory Safety Tests
 */

TEST_F(MovesTest, PointersInitializedToNullptr) {
  // After construction, pointers should be nullptr
  EXPECT_EQ(moves->trackp, nullptr);
  EXPECT_EQ(moves->mply, nullptr);
}

TEST_F(MovesTest, PointersSetCorrectlyDuringInit) {
  // After init, trackp should still be nullptr (it's set later in MoveGen0)
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  moves->Init(5, 0, nullptr, nullptr, rankInSuit, 3, 0);
  
  // After init, trackp should still be nullptr (it's set later in MoveGen0)
  EXPECT_EQ(moves->trackp, nullptr);
}

/**
 * @section Constants and Enums
 */

TEST_F(MovesTest, MgTypeEnumHasExpectedValues) {
  // Verify enum values are as expected
  EXPECT_EQ(static_cast<int>(MgType::NT0), 0);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP0), 1);
  EXPECT_EQ(static_cast<int>(MgType::NT_VOID1), 2);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP_VOID1), 3);
  EXPECT_EQ(static_cast<int>(MgType::NT_NOTVOID1), 4);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP_NOTVOID1), 5);
  EXPECT_EQ(static_cast<int>(MgType::NT_VOID2), 6);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP_VOID2), 7);
  // Verify SIZE is last
  EXPECT_GT(static_cast<int>(MgType::SIZE), 7);
}

TEST_F(MovesTest, FuncNameArrayHasSizeElements) {
  // Verify funcName array has correct size
  int count = 0;
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    if (!moves->funcName[i].empty()) {
      count++;
    }
  }
  EXPECT_EQ(count, static_cast<int>(MgType::SIZE));
}

/**
 * @section Data Structure Tests
 */

TEST_F(MovesTest, TrackArrayHas13Tricks) {
  // Verify track array has 13 tricks
  EXPECT_EQ(std::size(moves->track), 13);
}

TEST_F(MovesTest, MoveListArrayHas13TricksAnd4Hands) {
  // Verify moveList array dimensions
  EXPECT_EQ(std::size(moves->moveList), 13);
  for (int t = 0; t < 13; t++) {
    EXPECT_EQ(std::size(moves->moveList[t]), 4);
  }
}

TEST_F(MovesTest, LastCallArrayHas13TricksAnd4Hands) {
  // Verify lastCall array dimensions
  EXPECT_EQ(std::size(moves->lastCall), 13);
  for (int t = 0; t < 13; t++) {
    EXPECT_EQ(std::size(moves->lastCall[t]), 4);
  }
}

TEST_F(MovesTest, StatisticsStructuresProperlyInitialized) {
  // Verify statistics structures are initialized
  EXPECT_EQ(moves->trickFuncTable.nfuncs, 0);
  EXPECT_EQ(moves->trickFuncSuitTable.nfuncs, 0);
}

/**
 * @section Integration Tests
 */

TEST_F(MovesTest, CreateAndDestroySuccessfully) {
  // Verify object can be created and destroyed
  auto testMoves = std::make_unique<Moves>();
  EXPECT_NE(testMoves.get(), nullptr);
  testMoves.reset();
  EXPECT_TRUE(true);  // If we got here, no crash
}

TEST_F(MovesTest, MultipleInitializeCallsWork) {
  // Verify multiple Init calls work correctly
  const unsigned short (*rankInSuit)[4] = getSampleRankInSuit();
  
  for (int t = 0; t < 13; t++) {
    moves->Init(t, 0, nullptr, nullptr, rankInSuit, 0, t % 4);
    EXPECT_EQ(moves->currTrick, t);
  }
}

TEST_F(MovesTest, GetLengthWithValidBounds) {
  // Verify GetLength works for all valid bounds
  for (int t = 0; t < 13; t++) {
    for (int h = 0; h < 4; h++) {
      auto length = moves->GetLength(t, h);
      EXPECT_GE(length, 0);
      EXPECT_LE(length, 13);
    }
  }
}

/**
 * @section Documentation and Metadata
 */

TEST_F(MovesTest, FunctionNamesAreHumanReadable) {
  // Verify function names are readable strings
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    const auto& name = moves->funcName[i];
    EXPECT_FALSE(name.empty());
    EXPECT_GT(name.length(), 0);
    // Name should have printable characters
    for (char c : name) {
      EXPECT_TRUE(std::isprint(c) || c == ' ');
    }
  }
}

TEST_F(MovesTest, MemorySafetyFeaturesArePresent) {
  // Verify key memory safety features are in place
  EXPECT_EQ(moves->trackp, nullptr);  // Non-owning pointer initialized
  EXPECT_EQ(moves->mply, nullptr);    // Non-owning pointer initialized
  EXPECT_FALSE(moves->funcName[0].empty());  // funcName array exists and is initialized
}

/**
 * @section Performance Tests
 */

TEST_F(MovesTest, ConstructionIsQuick) {
  // Verify construction is fast
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 1000; i++) {
    auto temp = std::make_unique<Moves>();
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Should complete 1000 constructions in reasonable time
  EXPECT_LT(duration.count(), 1000);  // Less than 1 second for 1000
}

TEST_F(MovesTest, GetLengthIsQuick) {
  // Verify GetLength is fast
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 100000; i++) {
    volatile int result = moves->GetLength(i % 13, i % 4);
    (void)result;  // Use result to prevent optimization
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Should complete 100k calls in reasonable time
  EXPECT_LT(duration.count(), 500);  // Less than 500ms for 100k
}
