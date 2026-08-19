/// @file replay_test.cpp
/// @brief Smoke test for the recorded-workload replay benchmark.
///
/// Replays a small committed recording and checks that DDS still returns
/// exactly what was recorded. That makes this a regression test as well as a
/// benchmark check: the recorded results were produced by a real session, so
/// any change that alters a trick count shows up here.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "recording.hpp"
#include "replay.hpp"

namespace {

using dds_replay::Call;
using dds_replay::Recording;
using dds_replay::ReplayEngine;

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
