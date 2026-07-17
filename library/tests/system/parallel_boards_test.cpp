#include <atomic>
#include <cstdlib>
#include <functional>
#include <new>
#include <vector>

#include <gtest/gtest.h>

#include <api/dll.h>
#include <system/parallel_boards.hpp>

namespace allocation_tracking
{

std::atomic<bool> enabled{false};
std::atomic<unsigned> allocations{0};

}  // namespace allocation_tracking

void* operator new(const std::size_t size)
{
  if (allocation_tracking::enabled.load(std::memory_order_relaxed))
    allocation_tracking::allocations.fetch_add(1, std::memory_order_relaxed);

  // C++ requires successful zero-size allocations to return non-null;
  // malloc(0) is allowed to return nullptr, so request at least one byte.
  if (void* const memory = std::malloc(size == 0 ? 1 : size))
    return memory;
  throw std::bad_alloc();
}

void operator delete(void* const memory) noexcept
{
  std::free(memory);
}

void operator delete(void* const memory, std::size_t) noexcept
{
  std::free(memory);
}

namespace
{

auto dispatched_boards(
  const int count,
  const std::vector<int>& order) -> std::vector<int>
{
  std::vector<int> boards;
  boards.reserve(static_cast<unsigned>(count));
  const std::function<int(int, int)> process_board =
    [&](const int, const int board) {
      boards.push_back(board);
      return RETURN_NO_FAULT;
    };

  const int result =
    parallel_all_boards_n(count, 1, process_board, &order);

  EXPECT_EQ(result, RETURN_NO_FAULT);
  return boards;
}

}  // namespace

TEST(ParallelAllBoards, ValidPermutationControlsDispatchOrder)
{
  // Arrange
  const std::vector<int> order{3, 1, 0, 2};

  // Act
  const std::vector<int> boards = dispatched_boards(4, order);

  // Assert
  EXPECT_EQ(boards, order);
}

TEST(ParallelAllBoards, DuplicateOrderFallsBackToIndexOrder)
{
  // Arrange
  const std::vector<int> order{0, 1, 1, 3};

  // Act
  const std::vector<int> boards = dispatched_boards(4, order);

  // Assert
  EXPECT_EQ(boards, (std::vector<int>{0, 1, 2, 3}));
}

TEST(ParallelAllBoards, OutOfRangeOrderFallsBackToIndexOrder)
{
  // Arrange
  const std::vector<int> order{-1, 1, 2, 4};

  // Act
  const std::vector<int> boards = dispatched_boards(4, order);

  // Assert
  EXPECT_EQ(boards, (std::vector<int>{0, 1, 2, 3}));
}

TEST(ParallelAllBoards, PermutationValidationUsesAtMostOneAllocation)
{
  // Arrange
  const std::vector<int> order{3, 1, 0, 2};
  std::vector<int> boards;
  boards.reserve(order.size());
  const std::function<int(int, int)> process_board =
    [&](const int, const int board) {
      boards.push_back(board);
      return RETURN_NO_FAULT;
    };
  allocation_tracking::allocations.store(0, std::memory_order_relaxed);

  // Act
  allocation_tracking::enabled.store(true, std::memory_order_relaxed);
  const int result =
    parallel_all_boards_n(4, 1, process_board, &order);
  allocation_tracking::enabled.store(false, std::memory_order_relaxed);

  // Assert
  EXPECT_EQ(result, RETURN_NO_FAULT);
  EXPECT_LE(
    allocation_tracking::allocations.load(std::memory_order_relaxed), 1u);
}

TEST(ParallelAllBoards, ZeroSizeNewReturnsNonNull)
{
  // C++ requires a successful zero-size allocation to return a distinct
  // non-null pointer; malloc(0) is allowed to return nullptr.
  void* const memory = ::operator new(0);
  EXPECT_NE(memory, nullptr);
  ::operator delete(memory);
}
