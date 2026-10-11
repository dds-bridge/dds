/// @file replay_test.cpp
/// @brief Smoke test for the recorded-workload replay benchmark.
///
/// Replays a small committed recording and checks that DDS still returns
/// exactly what was recorded. That makes this a regression test as well as a
/// benchmark check: the recorded results were produced by a real session, so
/// any change that alters a trick count shows up here.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <api/dds_c_api.h>
#include <gtest/gtest.h>

#include "recording.hpp"
#include "replay.hpp"

namespace {

using dds_replay::Call;
using dds_replay::Recording;
using dds_replay::ReplayEngine;

auto create_returns_null() -> DDS_C_SOLVER_CTX { return nullptr; }

auto create_returns_sentinel() -> DDS_C_SOLVER_CTX
{
    return reinterpret_cast<DDS_C_SOLVER_CTX>(static_cast<uintptr_t>(0x1));
}

TEST(Replay, EnsureSolverContextRejectsNullCreate)
{
    // Arrange
    DDS_C_SOLVER_CTX ctx = nullptr;

    // Act
    const bool ok = dds_replay::ensure_solver_context(ctx, create_returns_null);

    // Assert: allocation failure must not leave a usable handle.
    EXPECT_FALSE(ok);
    EXPECT_EQ(nullptr, ctx);
}

TEST(Replay, EnsureSolverContextKeepsExistingHandle)
{
    // Arrange: already-warm context must not be replaced, even if create fails.
    DDS_C_SOLVER_CTX existing =
        reinterpret_cast<DDS_C_SOLVER_CTX>(static_cast<uintptr_t>(0x2));
    DDS_C_SOLVER_CTX ctx = existing;

    // Act
    const bool ok = dds_replay::ensure_solver_context(ctx, create_returns_null);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(existing, ctx);
}

TEST(Replay, EnsureSolverContextAdoptsCreatedHandle)
{
    // Arrange
    DDS_C_SOLVER_CTX ctx = nullptr;

    // Act
    const bool ok =
        dds_replay::ensure_solver_context(ctx, create_returns_sentinel);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(create_returns_sentinel(), ctx);
}

// --no-verify skips result comparison, but a solver/context failure must still
// surface so the CLI can exit non-zero instead of reporting a clean timing run.
TEST(Replay, SolverErrorsAreReportedEvenWhenVerifyIsOff)
{
    // Arrange: legal holdings, illegal leader -> SolveBoard rejects the deal.
    Call call;
    call.kind = Call::Kind::Solve;
    call.seq = 42;
    call.purpose = "bid";
    call.strain_i = 1;
    call.leader_i = 99;
    call.solutions = 1;
    call.hands_pbn = {"N:A.A.A.A 2.2.2.2 3.3.3.3 4.4.4.4"};

    ReplayEngine engine(/*threads=*/1, /*dds_mode=*/1);

    // Act
    const dds_replay::ReplayStats stats = engine.run({call}, /*verify=*/false);

    // Assert
    ASSERT_EQ(1u, stats.mismatches.size());
    EXPECT_EQ(42, stats.mismatches[0].seq);
    EXPECT_EQ("DDS error", stats.mismatches[0].why);
}

auto sample_path() -> std::string
{
    const std::string p = dds_replay::find_runfile(
        "_main/benchmarks/testdata/sample-recording.jsonl");
    return p.empty() ? "benchmarks/testdata/sample-recording.jsonl" : p;
}

TEST(Recording, ParsesTheSampleRecording)
{
    Recording rec;
    std::string error;
    ASSERT_TRUE(dds_replay::load_recording(sample_path(), rec, error)) << error;
    EXPECT_FALSE(rec.calls.empty());
    EXPECT_EQ("3.0.0", rec.dds_version);

    int solves = 0, pars = 0;
    for (const Call& c : rec.calls)
        (c.kind == Call::Kind::Solve ? solves : pars)++;
    EXPECT_GT(solves, 0);
    EXPECT_GT(pars, 0);

    // Every solve must carry at least one deal and a recorded result to check.
    for (const Call& c : rec.calls) {
        if (c.kind != Call::Kind::Solve)
            continue;
        EXPECT_FALSE(c.hands_pbn.empty());
        EXPECT_FALSE(c.result.empty()) << "seq " << c.seq;
    }
}

TEST(Recording, DecodesPbnHoldings)
{
    // North holds the four aces, one per suit; the rest is filler.
    unsigned int remain[4][4];
    ASSERT_TRUE(dds_replay::pbn_to_remain_cards(
        "N:A.A.A.A 2.2.2.2 3.3.3.3 4.4.4.4", remain));

    for (int suit = 0; suit < 4; ++suit) {
        EXPECT_EQ(1u << 14, remain[0][suit]) << "north suit " << suit;
        EXPECT_EQ(1u << 2,  remain[1][suit]) << "east suit "  << suit;
        EXPECT_EQ(1u << 3,  remain[2][suit]) << "south suit " << suit;
        EXPECT_EQ(1u << 4,  remain[3][suit]) << "west suit "  << suit;
    }

    // A non-North first seat must rotate the hands.
    unsigned int rot[4][4];
    ASSERT_TRUE(dds_replay::pbn_to_remain_cards(
        "W:A.A.A.A 2.2.2.2 3.3.3.3 4.4.4.4", rot));
    EXPECT_EQ(1u << 14, rot[3][0]);  // first listed hand is West
    EXPECT_EQ(1u << 2,  rot[0][0]);  // then North

    EXPECT_FALSE(dds_replay::pbn_to_remain_cards("garbage", remain));
}

// The point of the whole exercise: replaying the recording reproduces it.
TEST(Replay, ReproducesRecordedResults)
{
    Recording rec;
    std::string error;
    ASSERT_TRUE(dds_replay::load_recording(sample_path(), rec, error)) << error;
    ASSERT_FALSE(rec.calls.empty());

    ReplayEngine engine(/*threads=*/2, rec.dds_mode);
    const dds_replay::ReplayStats stats = engine.run(rec.calls, /*verify=*/true);

    for (const auto& m : stats.mismatches)
        ADD_FAILURE() << "seq " << m.seq << " (" << m.purpose << "): " << m.why;

    EXPECT_EQ(rec.calls.size(), static_cast<size_t>(stats.total.calls));
    EXPECT_GT(stats.total.boards, 0);
}

// Single-threaded and multi-threaded replay must agree: results are collected
// per board index, so thread count must not affect the answers.
TEST(Replay, ThreadCountDoesNotChangeResults)
{
    Recording rec;
    std::string error;
    ASSERT_TRUE(dds_replay::load_recording(sample_path(), rec, error)) << error;

    ReplayEngine solo(1, rec.dds_mode);
    ReplayEngine pool(4, rec.dds_mode);
    EXPECT_TRUE(solo.run(rec.calls, true).mismatches.empty());
    EXPECT_TRUE(pool.run(rec.calls, true).mismatches.empty());
}

}  // namespace
