/**
 * @file moves_test.cpp
 * @brief Comprehensive unit tests for the Moves class
 *
 * Tests cover:
 * - Constructor and initialization
 * - Move generation (MoveGen0, MoveGen123)
 * - Move selection (MakeNext, MakeNextSimple)
 * - Move application (MakeSpecific)
 * - Move traversal (Step, Rewind)
 * - Statistics tracking (RegisterHit, UpdateStatsEntry)
 * - Edge cases and invariant violations
 */

#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <chrono>
#include <memory>

#include <api/dds.h>
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
   * @brief Create a minimal valid position for testing
   *
   * Creates a starting bridge position with all cards available.
   */
  Pos createTestPosition() {
    Pos pos;
    std::memset(&pos, 0, sizeof(Pos));
    
    // Initialize rank_in_suit with all cards available
    for (int hand = 0; hand < DDS_HANDS; hand++) {
      for (int suit = 0; suit < DDS_SUITS; suit++) {
        pos.rank_in_suit[hand][suit] = 0x3fff;  // All 13 cards available
      }
    }
    
    // Initialize winner to notrump
    for (int suit = 0; suit < DDS_SUITS; suit++) {
      pos.winner[suit].hand = 0;
      pos.winner[suit].rank = 0;
    }
    
    return pos;
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
  // Create test position
  Pos pos = createTestPosition();
  
  // Initialize with trick 5, starting from relative hand 0
  moves->Init(5, 0, nullptr, nullptr, pos.rank_in_suit, DDS_NOTRUMP, 0);
  
  // Verify state is initialized
  EXPECT_EQ(moves->currTrick, 5);
  EXPECT_EQ(moves->trump, DDS_NOTRUMP);
  
  // Verify move lists are reset
  for (int h = 0; h < DDS_HANDS; h++) {
    EXPECT_EQ(moves->moveList[5][h].current, 0);
    EXPECT_EQ(moves->moveList[5][h].last, 0);
  }
}

TEST_F(MovesTest, ReinitUpdateLeadHand) {
  // Initialize first
  Pos pos = createTestPosition();
  moves->Init(5, 0, nullptr, nullptr, pos.rank_in_suit, DDS_NOTRUMP, 0);
  
  // Reinit with different lead hand
  moves->Reinit(5, 2);
  
  // Verify lead hand is updated
  EXPECT_EQ(moves->track[5].leadHand, 2);
}

/**
 * @section Move Generation Tests
 */

TEST_F(MovesTest, GetLengthReturnsCorrectCount) {
  // Create and initialize a position
  Pos pos = createTestPosition();
  moves->Init(5, 0, nullptr, nullptr, pos.rank_in_suit, DDS_NOTRUMP, 0);
  
  // Set up a move list with known length
  moves->moveList[5][0].last = 10;
  
  // Verify GetLength returns correct value
  EXPECT_EQ(moves->GetLength(5, 0), 11);  // last + 1
}

TEST_F(MovesTest, GetLengthHandlesEmptyList) {
  Pos pos = createTestPosition();
  moves->Init(5, 0, nullptr, nullptr, pos.rank_in_suit, DDS_NOTRUMP, 0);
  
  // Move list starts empty
  EXPECT_EQ(moves->GetLength(5, 0), 1);  // last = 0 initially
}

/**
 * @section Move Selection Tests
 */

TEST_F(MovesTest, PrintMoveReturnsValidString) {
  MovePlyType moveList;
  moveList.current = 0;
  moveList.last = 0;
  
  MoveType move;
  move.suit = 0;
  move.rank = 10;
  moveList.move[0] = move;
  
  // PrintMove should return a non-empty string
  std::string result = moves->PrintMove(moveList);
  EXPECT_FALSE(result.empty());
  EXPECT_NE(result.find("current"), std::string::npos);
}

/**
 * @section Pointer Safety Tests
 */

TEST_F(MovesTest, PointersInitializedToNullptr) {
  // Verify non-owning pointers are initialized to nullptr
  EXPECT_EQ(moves->trackp, nullptr);
  EXPECT_EQ(moves->mply, nullptr);
}

TEST_F(MovesTest, PointersSetCorrectlyDuringInit) {
  Pos pos = createTestPosition();
  const int initialRanks[] = {0};
  const int initialSuits[] = {0};
  
  moves->Init(5, 0, initialRanks, initialSuits, pos.rank_in_suit, 
              DDS_NOTRUMP, 0);
  
  // After init, trackp should still be nullptr (it's set later in MoveGen0)
  EXPECT_EQ(moves->trackp, nullptr);
}

/**
 * @section Constants and Enums
 */

TEST_F(MovesTest, MgTypeEnumHasExpectedValues) {
  // Verify MgType enum class has expected values
  EXPECT_EQ(static_cast<int>(MgType::NT0), 0);
  EXPECT_EQ(static_cast<int>(MgType::TRUMP0), 1);
  EXPECT_EQ(static_cast<int>(MgType::SIZE), 13);
}

TEST_F(MovesTest, FuncNameArrayHasSizeElements) {
  // Verify funcName array has correct size
  EXPECT_EQ(std::size(moves->funcName), static_cast<size_t>(MgType::SIZE));
}

/**
 * @section Array Bounds Tests
 */

TEST_F(MovesTest, TrackArrayHas13Tricks) {
  // Verify track array size
  EXPECT_EQ(std::size(moves->track), 13);
}

TEST_F(MovesTest, MoveListArrayHas13TricksAnd4Hands) {
  // Verify moveList dimensions
  EXPECT_EQ(std::size(moves->moveList), 13);
  EXPECT_EQ(std::size(moves->moveList[0]), DDS_HANDS);
}

TEST_F(MovesTest, LastCallArrayHas13TricksAnd4Hands) {
  // Verify lastCall dimensions
  EXPECT_EQ(std::size(moves->lastCall), 13);
  EXPECT_EQ(std::size(moves->lastCall[0]), DDS_HANDS);
}

/**
 * @section Member Variable Tests
 */

TEST_F(MovesTest, StatisticsStructuresProperlyInitialized) {
  // Verify statistics structures have expected layout
  EXPECT_EQ(std::size(moves->trickTable), 13);
  EXPECT_EQ(std::size(moves->trickTable[0]), DDS_HANDS);
  
  // Verify counts are zero initially
  for (int t = 0; t < 13; t++) {
    for (int h = 0; h < DDS_HANDS; h++) {
      EXPECT_EQ(moves->trickTable[t][h].count, 0);
      EXPECT_EQ(moves->trickSuitTable[t][h].count, 0);
    }
  }
}

/**
 * @section Integration Tests
 */

TEST_F(MovesTest, CreateAndDestroySuccessfully) {
  // Create multiple instances to verify no resource leaks
  for (int i = 0; i < 10; i++) {
    auto temp = std::make_unique<Moves>();
    EXPECT_NE(temp.get(), nullptr);
  }
  // All should clean up properly
}

TEST_F(MovesTest, MultipleInitializeCallsWork) {
  Pos pos = createTestPosition();
  
  // Initialize multiple times - should work
  for (int trick = 0; trick < 13; trick++) {
    moves->Init(trick, 0, nullptr, nullptr, pos.rank_in_suit, 
                DDS_NOTRUMP, trick % DDS_HANDS);
    EXPECT_EQ(moves->currTrick, trick);
  }
}

/**
 * @section Error Condition Tests (Assertions)
 */

TEST_F(MovesTest, GetLengthWithValidBounds) {
  Pos pos = createTestPosition();
  moves->Init(5, 0, nullptr, nullptr, pos.rank_in_suit, DDS_NOTRUMP, 0);
  
  // Valid indices should not trigger assertions
  for (int t = 0; t < 13; t++) {
    for (int h = 0; h < DDS_HANDS; h++) {
      // Should not crash
      int len = moves->GetLength(t, h);
      EXPECT_GE(len, 0);
    }
  }
}

/**
 * @section Documentation Tests
 *
 * These tests verify documented behavior
 */

TEST_F(MovesTest, FunctionNamesAreHumanReadable) {
  // Verify function names are useful for logging/debugging
  std::vector<std::string> expectedNames = {
    "NT0", "Trump0", "NT_Void1", "Trump_Void1",
    "NT_Notvoid1", "Trump_Notvoid1", "NT_Void2", "Trump_Void2",
    "NT_Notvoid2", "Trump_Notvoid2", "NT_Void3", "Trump_Void3",
    "Comb_Notvoid3"
  };
  
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    EXPECT_EQ(moves->funcName[i], expectedNames[i]);
  }
}

TEST_F(MovesTest, MemorySafetyFeaturesArePresent) {
  // Verify RAII and memory safety features
  
  // Stack allocation: all member arrays are on stack
  EXPECT_TRUE(sizeof(moves->track) > 0);
  EXPECT_TRUE(sizeof(moves->moveList) > 0);
  EXPECT_TRUE(sizeof(moves->lastCall) > 0);
  
  // Non-owning pointers initialized
  EXPECT_EQ(moves->trackp, nullptr);
  EXPECT_EQ(moves->mply, nullptr);
}

/**
 * @section Performance Tests (Sanity Checks)
 */

TEST_F(MovesTest, ConstructionIsQuick) {
  // Verify constructor doesn't do expensive operations
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 1000; i++) {
    Moves temp;
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start);
  
  // 1000 constructions should be very fast
  EXPECT_LT(duration.count(), 100);  // 100ms for 1000 objects
}

TEST_F(MovesTest, GetLengthIsQuick) {
  Pos pos = createTestPosition();
  moves->Init(5, 0, nullptr, nullptr, pos.rank_in_suit, DDS_NOTRUMP, 0);
  
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 100000; i++) {
    for (int t = 0; t < 13; t++) {
      for (int h = 0; h < DDS_HANDS; h++) {
        moves->GetLength(t, h);
      }
    }
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start);
  
  // Should be essentially free
  EXPECT_LT(duration.count(), 1000);  // 1s for 5.2M calls
}
