/*
   DDS, a bridge double dummy solver.

   Scaffolding for an instance-scoped API. This is a no-behavior-change
   adapter that allows driving the solver with an explicit context,
   while internally delegating to existing code paths.
*/

#ifndef DDS_SYSTEM_SOLVERCONTEXT_H
#define DDS_SYSTEM_SOLVERCONTEXT_H

struct ThreadData;     // defined in system/Memory.h
struct deal;           // defined in dds/dll.h
struct futureTricks;   // defined in dds/dll.h
struct moveType;       // from dds/dds.h
struct WinnersType;    // from dds/dds.h
struct pos;            // from dds/dds.h
struct relRanksType;   // from dds/dds.h
struct trickDataType;  // from data_types/dds.h
#include "trans_table/TransTable.h" // ensure complete type and enums
#include "system/util/Utilities.h"   // instance-scoped RNG and logging
#include <string>
#include <vector>
#include <random>

// Minimal configuration scaffold for future expansion.
// TT configuration without depending on Memory headers.
enum class TTKind { Small, Large };

struct SolverConfig
{
  TTKind ttKind = TTKind::Small;
  int ttMemDefaultMB = 0;
  int ttMemMaximumMB = 0;
  // Optional deterministic RNG seed (0 means "no explicit seed").
  unsigned long long rngSeed = 0ULL;
};

class SolverContext
{
public:
  explicit SolverContext(ThreadData* thread, SolverConfig cfg = {})
  : thr_(thread), cfg_(cfg)
  {
    if (cfg_.rngSeed != 0ULL) utils_.seed(cfg_.rngSeed);
  }

  // Allow construction from const ThreadData* for read-only contexts
  explicit SolverContext(const ThreadData* thread, SolverConfig cfg = {})
  : thr_(const_cast<ThreadData*>(thread)), cfg_(cfg)
  {
    if (cfg_.rngSeed != 0ULL) utils_.seed(cfg_.rngSeed);
  }

  ~SolverContext();

  ThreadData* thread() const { return thr_; }
  const SolverConfig& config() const { return cfg_; }

  // --- Utilities facade ---
  class UtilitiesContext {
  public:
    explicit UtilitiesContext(::dds::Utilities* util) : util_(util) {}
    ::dds::Utilities& util() { return *util_; }
    const ::dds::Utilities& util() const { return *util_; }
    std::mt19937& rng() { return util_->rng(); }
    const std::mt19937& rng() const { return util_->rng(); }
    void seedRng(unsigned long long seed) { util_->seed(seed); }
    void logAppend(const std::string& s) { util_->log_append(s); }
    const std::vector<std::string>& logBuffer() const { return util_->log_buffer(); }
    void logClear() { util_->log_clear(); }
  private:
    ::dds::Utilities* util_ = nullptr;
  };

  inline UtilitiesContext utilities() { return UtilitiesContext(&utils_); }
  inline UtilitiesContext utilities() const { return UtilitiesContext(&utils_); }

  TransTable* transTable() const;
  TransTable* maybeTransTable() const;

  // Dispose and erase the TT instance associated with this thread, if any.
  void DisposeTransTable() const;

  // Lightweight facades used by tests and call sites; no-ops if no TT exists.
  void ResetForSolve() const;   // Calls ResetMemory(TT_RESET_FREE_MEMORY)
  // Lightweight per-iteration reset matching legacy ResetBestMoves semantics.
  // Only clears bestMove[*].rank and bestMoveTT[*].rank, updates memUsed and ABStats.
  void ResetBestMovesLite() const;
  void ClearTT() const;         // Calls ReturnAllMemory()
  void ResizeTT(int defMB, int maxMB) const; // Updates sizes if TT exists

  // --- Search state facade ---
  class SearchContext {
  public:
    explicit SearchContext(ThreadData* thr) : thr_(thr) {}
    // analysis flag used to control incremental analysis behavior
    bool& analysisFlag();
    bool analysisFlag() const;
    unsigned short& lowestWin(int depth, int suit);
    const unsigned short& lowestWin(int depth, int suit) const;
    moveType& bestMove(int depth);
    const moveType& bestMove(int depth) const;
    moveType& bestMoveTT(int depth);
    const moveType& bestMoveTT(int depth) const;
    WinnersType& winners(int trickIndex);
    const WinnersType& winners(int trickIndex) const;
  // Node type store for each hand (MAXNODE/MINNODE)
  int& nodeTypeStore(int hand);
  const int& nodeTypeStore(int hand) const;
    // Access to forbidden moves buffer used by Moves::Purge and solver loops
    moveType* forbiddenMoves();
    const moveType* forbiddenMoves() const;
    moveType& forbiddenMove(int index);
    const moveType& forbiddenMove(int index) const;
  void clearForbiddenMoves();
    int& nodes();
    int& trickNodes();
    int& iniDepth();
    int iniDepth() const;
  private:
    ThreadData* thr_ = nullptr;
  };

  inline SearchContext search() const { return SearchContext(thr_); }

  // --- Move generation facade ---
  class MoveGenContext {
  public:
    explicit MoveGenContext(ThreadData* thr) : thr_(thr) {}

    int MoveGen0(
      const int tricks,
      const pos& tpos,
      const moveType& bestMove,
      const moveType& bestMoveTT,
      const relRanksType thrp_rel[]);

    int MoveGen123(
      const int tricks,
      const int relHand,
      const pos& tpos);

    void Purge(
      const int tricks,
      const int relHand,
      const moveType forbiddenMoves[]);

    const moveType* MakeNext(
      const int trick,
      const int relHand,
      const unsigned short winRanks[]);

    // Simpler variant without winRanks used in several SolverIF paths
    const moveType* MakeNextSimple(
      const int trick,
      const int relHand);

    int GetLength(
      const int trick,
      const int relHand) const;

    void Rewind(
      const int tricks,
      const int relHand);

    void RegisterHit(
      const int tricks,
      const int relHand);

    // Reinitialize move generation for a new lead hand at a given trick
    void Reinit(
      const int tricks,
      const int leadHand);

    // Initialize move generation state for a given trick and starting hand
    void Init(
      const int tricks,
      const int relStartHand,
      const int initialRanks[],
      const int initialSuits[],
      const unsigned short rankInSuit[DDS_HANDS][DDS_SUITS],
      const int trump,
      const int leadHand);

  // Diagnostics (no behavior change; passthrough to Moves)
  // Note: Emission is controlled by DDS_MOVES / DDS_MOVES_DETAILS.
    void PrintTrickStats(std::ofstream& fout) const;
    void PrintTrickDetails(std::ofstream& fout) const;
    void PrintFunctionStats(std::ofstream& fout) const;

    // Read-only access to per-trick generated metadata
    const trickDataType& GetTrickData(const int tricks);

 // Read-only textual dump helper
    std::string TrickToText(const int trick) const;

    // Specify a particular move at a trick/hand position
    void MakeSpecific(
      const moveType& mply,
      const int trick,
      const int relHand);

  private:
    ThreadData* thr_ = nullptr;
  };

  inline MoveGenContext moveGen() const { return MoveGenContext(thr_); }

private:
  ThreadData* thr_ = nullptr;
  SolverConfig cfg_{};
  mutable ::dds::Utilities utils_{};
};

double ThreadMemoryUsed();

#endif // DDS_SYSTEM_SOLVERCONTEXT_H
