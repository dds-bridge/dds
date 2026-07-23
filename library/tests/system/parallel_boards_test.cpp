#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <new>
#include <thread>
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

TEST(ParallelAllBoards, WrongSizedOrderFallsBackToIndexOrder)
{
  // Arrange: valid values, but length does not match count.
  const std::vector<int> order{3, 1, 0};

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

TEST(ParallelAllBoards, MultiWorkerProcessesEachBoardOnce)
{
  // Arrange
  constexpr int count = 32;
  constexpr int workers = 4;
  std::vector<std::atomic<int>> hits(static_cast<unsigned>(count));
  for (auto& h : hits)
    h.store(0, std::memory_order_relaxed);

  // Act
  const int result = parallel_all_boards_n(
    count,
    workers,
    [&](const int worker_id, const int bno) -> int {
      // EXPECT does not abort; guard before indexing so a bad bno fails the
      // test cleanly instead of crashing with an out-of-range access.
      if (worker_id < 0 || worker_id >= workers || bno < 0 || bno >= count)
      {
        ADD_FAILURE() << "Invalid dispatch: worker_id=" << worker_id
                      << " bno=" << bno;
        return RETURN_UNKNOWN_FAULT;
      }
      hits[static_cast<unsigned>(bno)].fetch_add(1, std::memory_order_relaxed);
      return RETURN_NO_FAULT;
    });

  // Assert
  EXPECT_EQ(result, RETURN_NO_FAULT);
  for (int i = 0; i < count; ++i)
    EXPECT_EQ(hits[static_cast<unsigned>(i)].load(), 1) << "board " << i;
}

TEST(ParallelAllBoards, FailFastReturnsFirstError)
{
  // Arrange / Act
  const int result = parallel_all_boards_n(
    16,
    4,
    [](const int, const int bno) {
      return bno == 7 ? RETURN_TOO_MANY_BOARDS : RETURN_NO_FAULT;
    });

  // Assert
  EXPECT_EQ(result, RETURN_TOO_MANY_BOARDS);
}

TEST(ParallelAllBoards, ConcurrentCallersBothCompleteAndProcessAllBoards)
{
  // The pool is process-global; two threads dispatching batches at the same
  // time must not clobber each other's job (which would drop boards or hang
  // one caller forever waiting for workers that never picked its job up).
  constexpr int callers = 2;
  constexpr int count = 64;
  constexpr int workers = 2;
  constexpr auto deadline = std::chrono::seconds(20);

  // Arrange: per-caller hit counters and a start barrier so both callers
  // enter the dispatcher at the same moment.
  std::array<std::vector<std::atomic<int>>, callers> hits;
  for (auto& caller_hits : hits)
    caller_hits = std::vector<std::atomic<int>>(count);
  std::atomic<int> ready{0};
  std::array<std::promise<int>, callers> results;
  std::array<std::future<int>, callers> futures;
  for (int c = 0; c < callers; ++c)
    futures[static_cast<unsigned>(c)] =
      results[static_cast<unsigned>(c)].get_future();

  // Act
  std::vector<std::thread> threads;
  threads.reserve(callers);
  for (int c = 0; c < callers; ++c)
  {
    threads.emplace_back([&, c] {
      ready.fetch_add(1, std::memory_order_relaxed);
      while (ready.load(std::memory_order_relaxed) < callers)
        std::this_thread::yield();

      const int rc = parallel_all_boards_n(
        count,
        workers,
        [&, c](const int, const int bno) {
          hits[static_cast<unsigned>(c)][static_cast<unsigned>(bno)]
            .fetch_add(1, std::memory_order_relaxed);
          std::this_thread::sleep_for(std::chrono::microseconds(200));
          return RETURN_NO_FAULT;
        });
      results[static_cast<unsigned>(c)].set_value(rc);
    });
  }

  bool timed_out = false;
  for (auto& fut : futures)
  {
    if (fut.wait_for(deadline) != std::future_status::ready)
      timed_out = true;
  }

  // A hung caller can never be joined; detach so the failure is reportable.
  for (auto& th : threads)
  {
    if (timed_out)
      th.detach();
    else
      th.join();
  }
  ASSERT_FALSE(timed_out)
      << "a concurrent caller hung: its job was lost by the shared pool";

  // Assert: both callers succeeded and every board was processed exactly once
  // per caller.
  for (int c = 0; c < callers; ++c)
  {
    EXPECT_EQ(futures[static_cast<unsigned>(c)].get(), RETURN_NO_FAULT)
        << "caller " << c;
    for (int i = 0; i < count; ++i)
      EXPECT_EQ(
        hits[static_cast<unsigned>(c)][static_cast<unsigned>(i)].load(), 1)
          << "caller " << c << " board " << i;
  }
}

TEST(ParallelAllBoards, ReusesWorkerThreadsAcrossConsecutiveCalls)
{
  // Persistent pool should create workers once and reuse them. Spawn-per-call
  // creates a fresh set of threads on every multi-worker invocation.
  if (std::thread::hardware_concurrency() < 2)
    GTEST_SKIP() << "Need at least 2 hardware threads";

  constexpr int count = 64;
  constexpr int workers = 4;
  const auto noop = [](const int, const int) { return RETURN_NO_FAULT; };

  // Arrange: grow/warm the pool to the requested size.
  ASSERT_EQ(parallel_all_boards_n(count, workers, noop), RETURN_NO_FAULT);
  const auto created_after_warm = parallel_boards_worker_threads_created();
  ASSERT_GE(created_after_warm, static_cast<std::uint64_t>(workers));

  // Act: two more multi-worker runs at the same width.
  ASSERT_EQ(parallel_all_boards_n(count, workers, noop), RETURN_NO_FAULT);
  ASSERT_EQ(parallel_all_boards_n(count, workers, noop), RETURN_NO_FAULT);
  const auto created_after_reuse = parallel_boards_worker_threads_created();

  // Assert: reuse must not create additional OS threads.
  EXPECT_EQ(created_after_reuse, created_after_warm);
}
