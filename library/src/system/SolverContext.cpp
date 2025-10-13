#include "system/SolverContext.h"

// Keep dependencies local to this implementation to avoid include churn.
#include "system/Memory.h"       // for ThreadData definition
#include "dds/dds.h"             // moveType, WinnersType
#include "trans_table/TransTableS.h"
#include "trans_table/TransTableL.h"
#include "data_types/dds.h" // THREADMEM_* defaults
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include "system/util/Arena.h"
#include "utility/ScratchAllocTLS.h"

namespace {
// Central registry mapping ThreadData* to its TransTable instance.
// Use a leaky singleton to avoid static destruction order issues at exit.
static std::unordered_map<ThreadData*, TransTable*>& registry()
{
  static auto* map = new std::unordered_map<ThreadData*, TransTable*>();
  return *map;
}

// Per-thread Arena registry. Managed as a leaky singleton similar to TT.
namespace {
static std::unordered_map<ThreadData*, std::unique_ptr<dds::Arena>>& arena_registry()
{
  static auto* map = new std::unordered_map<ThreadData*, std::unique_ptr<dds::Arena>>();
  return *map;
}
}
}

TransTable* SolverContext::transTable() const
{
  if (!thr_)
    return nullptr;
  auto it = registry().find(thr_);
  if (it == registry().end() || it->second == nullptr)
  {
    TransTable* created = nullptr;
    // Prefer thread-stored configuration determined by Init/Memory
    TTKind kind = (thr_->ttType == DDS_TT_SMALL ? TTKind::Small : TTKind::Large);
    if (kind == TTKind::Small)
      created = new TransTableS();
    else
      created = new TransTableL();

    int defMB = (cfg_.ttMemDefaultMB > 0 ? cfg_.ttMemDefaultMB : thr_->ttMemDefault_MB);
    int maxMB = (cfg_.ttMemMaximumMB > 0 ? cfg_.ttMemMaximumMB : thr_->ttMemMaximum_MB);
    // Fallback to conservative defaults if absent
    if (defMB <= 0 || maxMB <= 0)
    {
      if (kind == TTKind::Small)
      {
        defMB = THREADMEM_SMALL_DEF_MB;
        maxMB = THREADMEM_SMALL_MAX_MB;
      }
      else
      {
        defMB = THREADMEM_LARGE_DEF_MB;
        maxMB = THREADMEM_LARGE_MAX_MB;
      }
    }

    // Optional environment overrides for tuning
    if (const char* s = std::getenv("DDS_TT_DEFAULT_MB"))
    {
      int v = std::atoi(s);
      if (v > 0)
        defMB = v;
    }
    if (const char* s = std::getenv("DDS_TT_LIMIT_MB"))
    {
      int v = std::atoi(s);
      if (v > 0)
        maxMB = std::min(maxMB, v);
    }
    if (maxMB < defMB)
      maxMB = defMB;

    // Optional one-time debug print per creation
    if (const char* dbg = std::getenv("DDS_DEBUG_TT_CREATE"))
    {
      if (*dbg)
      {
        std::cerr << "[DDS] TT create: kind="
                  << (kind == TTKind::Small ? 'S' : 'L')
                  << " defMB=" << defMB
                  << " maxMB=" << maxMB
                  << std::endl;
      }
    }

    created->SetMemoryDefault(defMB);
    created->SetMemoryMaximum(maxMB);
  created->MakeTT();

#ifdef DDS_UTILITIES_LOG
    // Append a tiny debug entry indicating TT creation and chosen kind/sizes.
    {
      const char kch = (kind == TTKind::Small ? 'S' : 'L');
      // Prefer arena-backed small buffer to avoid stack churn; fallback to stack.
      char* buf = nullptr;
      constexpr std::size_t kLen = 96;
      if (auto* a = const_cast<SolverContext*>(this)->arena()) {
        buf = static_cast<char*>(a->allocate({kLen, alignof(char)}));
      }
      char local[kLen];
      if (!buf) buf = local;
      std::snprintf(buf, kLen, "tt:create|%c|%d|%d", kch, defMB, maxMB);
      utilities().logAppend(std::string(buf));
    }
#endif

#ifdef DDS_UTILITIES_STATS
  utilities().util().stats().tt_creates++;
#endif

    // Attach to registry
    registry()[thr_] = created;
  }
  return registry()[thr_];
}

// --- Arena access (per-thread registry) ---
dds::Arena* SolverContext::arena()
{
  if (!thr_) return nullptr;
  auto& map = arena_registry();
  auto it = map.find(thr_);
  if (it == map.end()) {
    if (cfg_.arenaCapacityBytes == 0ULL) return nullptr;
    auto ins = map.emplace(thr_, std::make_unique<dds::Arena>(cfg_.arenaCapacityBytes));
    return ins.first->second.get();
  }
  return it->second.get();
}

const dds::Arena* SolverContext::arena() const
{
  // const overload delegates to non-const (registry is mutable globally)
  return const_cast<SolverContext*>(this)->arena();
}

// --- SearchContext out-of-line definitions ---
bool& SolverContext::SearchContext::analysisFlag() { return thr_->analysisFlag; }
bool SolverContext::SearchContext::analysisFlag() const { return thr_->analysisFlag; }
unsigned short& SolverContext::SearchContext::lowestWin(int depth, int suit) {
  return thr_->lowestWin[depth][suit];
}
const unsigned short& SolverContext::SearchContext::lowestWin(int depth, int suit) const {
  return thr_->lowestWin[depth][suit];
}
moveType& SolverContext::SearchContext::bestMove(int depth) {
  return thr_->bestMove[depth];
}
const moveType& SolverContext::SearchContext::bestMove(int depth) const {
  return thr_->bestMove[depth];
}
moveType& SolverContext::SearchContext::bestMoveTT(int depth) {
  return thr_->bestMoveTT[depth];
}
const moveType& SolverContext::SearchContext::bestMoveTT(int depth) const {
  return thr_->bestMoveTT[depth];
}
WinnersType& SolverContext::SearchContext::winners(int trickIndex) {
  return thr_->winners[trickIndex];
}
const WinnersType& SolverContext::SearchContext::winners(int trickIndex) const {
  return thr_->winners[trickIndex];
}
int& SolverContext::SearchContext::nodeTypeStore(int hand) { return thr_->nodeTypeStore[hand]; }
const int& SolverContext::SearchContext::nodeTypeStore(int hand) const { return thr_->nodeTypeStore[hand]; }
int& SolverContext::SearchContext::nodes() { return thr_->nodes; }
int& SolverContext::SearchContext::trickNodes() { return thr_->trickNodes; }
int& SolverContext::SearchContext::iniDepth() { return thr_->iniDepth; }
int SolverContext::SearchContext::iniDepth() const { return thr_->iniDepth; }
moveType* SolverContext::SearchContext::forbiddenMoves() { return thr_->forbiddenMoves; }
const moveType* SolverContext::SearchContext::forbiddenMoves() const { return thr_->forbiddenMoves; }
moveType& SolverContext::SearchContext::forbiddenMove(int index) { return thr_->forbiddenMoves[index]; }
const moveType& SolverContext::SearchContext::forbiddenMove(int index) const { return thr_->forbiddenMoves[index]; }
void SolverContext::SearchContext::clearForbiddenMoves() {
  for (int k = 0; k <= 13; ++k) {
    thr_->forbiddenMoves[k].rank = 0;
    thr_->forbiddenMoves[k].suit = 0;
  }
}

TransTable* SolverContext::maybeTransTable() const
{
  if (!thr_)
    return nullptr;
  auto it = registry().find(thr_);
  return (it == registry().end() ? nullptr : it->second);
}

void SolverContext::DisposeTransTable() const
{
  auto it = registry().find(thr_);
  if (it != registry().end())
  {
#ifdef DDS_UTILITIES_LOG
  // Append a tiny debug entry indicating TT disposal.
  utilities().logAppend("tt:dispose");
#endif
#ifdef DDS_UTILITIES_STATS
  utilities().util().stats().tt_disposes++;
#endif
    delete it->second;
    it->second = nullptr;
    registry().erase(it);
  }
}

SolverContext::~SolverContext() = default;

void SolverContext::ResetForSolve() const
{
#ifdef DDS_UTILITIES_LOG
  // Use arena-backed small buffer when available.
  {
    char* buf = nullptr;
    constexpr std::size_t kLen = 32;
    if (auto* a = const_cast<SolverContext*>(this)->arena()) {
      buf = static_cast<char*>(a->allocate({kLen, alignof(char)}));
    }
    char local[kLen];
    if (!buf) buf = local;
    std::snprintf(buf, kLen, "ctx:reset_for_solve");
    utilities().logAppend(std::string(buf));
  }
#endif
  if (auto* a = const_cast<SolverContext*>(this)->arena()) {
    a->reset();
  }
  if (auto* tt = maybeTransTable())
    tt->ResetMemory(TT_RESET_FREE_MEMORY);
  if (!thr_) return;
  // Reset a subset of search state to a clean slate.
  thr_->nodes = 0;
  thr_->trickNodes = 0;
  thr_->analysisFlag = false;
  for (int d = 0; d < 50; ++d) {
    thr_->bestMove[d].suit = 0;
    thr_->bestMove[d].rank = 0;
    thr_->bestMoveTT[d].suit = 0;
    thr_->bestMoveTT[d].rank = 0;
    for (int s = 0; s < DDS_SUITS; ++s) {
      thr_->lowestWin[d][s] = 0;
    }
  }
  for (int t = 0; t < 13; ++t) {
    thr_->winners[t].number = 0;
  }
  for (int k = 0; k <= 13; ++k) {
    thr_->forbiddenMoves[k].rank = 0;
    thr_->forbiddenMoves[k].suit = 0;
  }
}

void SolverContext::ClearTT() const
{
#ifdef DDS_UTILITIES_LOG
  utilities().logAppend("tt:clear");
#endif
  if (auto* tt = maybeTransTable())
    tt->ReturnAllMemory();
}

void SolverContext::ResizeTT(int defMB, int maxMB) const
{
#ifdef DDS_UTILITIES_LOG
  {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "tt:resize|%d|%d", defMB, maxMB);
    utilities().logAppend(std::string(buf));
  }
#endif
  if (auto* tt = maybeTransTable())
  {
    if (maxMB < defMB) maxMB = defMB;
    tt->SetMemoryDefault(defMB);
    tt->SetMemoryMaximum(maxMB);
  }
}

// Lightweight reset matching legacy ResetBestMoves semantics.
void SolverContext::ResetBestMovesLite() const
{
#ifdef DDS_UTILITIES_LOG
  utilities().logAppend("ctx:reset_best_moves_lite");
#endif
  if (!thr_) return;
  for (int d = 0; d <= 49; ++d)
  {
    thr_->bestMove[d].rank = 0;
    thr_->bestMoveTT[d].rank = 0;
  }
  // Keep memUsed in sync as the legacy code did
  if (auto* tt = maybeTransTable())
    thr_->memUsed = tt->MemoryInUse() + ThreadMemoryUsed();
  else
    thr_->memUsed = ThreadMemoryUsed();
#ifdef DDS_AB_STATS
  thr_->ABStats.Reset();
#endif
}

double ThreadMemoryUsed()
{
  // TODO:  Only needed because SolverIF wants to set it. Avoid?
  double memUsed =
    8192 * sizeof(relRanksType)
    / static_cast<double>(1024.);

  return memUsed;
}

// --- MoveGenContext out-of-line definitions ---
namespace {
struct ShimCtx { SolverContext* ctx; };
static void* ArenaAllocShim(std::size_t size, std::size_t align, void* c) {
  auto* sc = static_cast<ShimCtx*>(c);
  if (!sc || !sc->ctx) return nullptr;
  if (auto* a = sc->ctx->arena()) return a->allocate({size, align});
  return nullptr;
}
}

int SolverContext::MoveGenContext::MoveGen0(
  const int tricks,
  const pos& tpos,
  const moveType& bestMove,
  const moveType& bestMoveTT,
  const relRanksType thrp_rel[])
{
  // Expose an optional allocator to legacy code paths.
  SolverContext ctx(thr_);
  ShimCtx shim{&ctx};
  dds::tls::SetAlloc(&ArenaAllocShim, &shim);
  auto rc = thr_->moves.MoveGen0(tricks, tpos, bestMove, bestMoveTT, thrp_rel);
  dds::tls::ResetAlloc();
  return rc;
}

int SolverContext::MoveGenContext::MoveGen123(
  const int tricks,
  const int relHand,
  const pos& tpos)
{
  SolverContext ctx(thr_);
  ShimCtx shim{&ctx};
  dds::tls::SetAlloc(&ArenaAllocShim, &shim);
  auto rc = thr_->moves.MoveGen123(tricks, relHand, tpos);
  dds::tls::ResetAlloc();
  return rc;
}

void SolverContext::MoveGenContext::Purge(
  const int tricks,
  const int relHand,
  const moveType forbiddenMoves[])
{
  thr_->moves.Purge(tricks, relHand, forbiddenMoves);
}

const moveType* SolverContext::MoveGenContext::MakeNext(
  const int trick,
  const int relHand,
  const unsigned short winRanks[])
{
  return thr_->moves.MakeNext(trick, relHand, winRanks);
}

const moveType* SolverContext::MoveGenContext::MakeNextSimple(
  const int trick,
  const int relHand)
{
  return thr_->moves.MakeNextSimple(trick, relHand);
}

int SolverContext::MoveGenContext::GetLength(
  const int trick,
  const int relHand) const
{
  return thr_->moves.GetLength(trick, relHand);
}

void SolverContext::MoveGenContext::Rewind(
  const int tricks,
  const int relHand)
{
  thr_->moves.Rewind(tricks, relHand);
}

void SolverContext::MoveGenContext::RegisterHit(
  const int tricks,
  const int relHand)
{
  thr_->moves.RegisterHit(tricks, relHand);
}

const trickDataType& SolverContext::MoveGenContext::GetTrickData(const int tricks)
{
  return thr_->moves.GetTrickData(tricks);
}

void SolverContext::MoveGenContext::MakeSpecific(
  const moveType& mply,
  const int trick,
  const int relHand)
{
  thr_->moves.MakeSpecific(mply, trick, relHand);
}

std::string SolverContext::MoveGenContext::TrickToText(const int trick) const
{
  return thr_->moves.TrickToText(trick);
}

void SolverContext::MoveGenContext::Reinit(
  const int tricks,
  const int leadHand)
{
  thr_->moves.Reinit(tricks, leadHand);
}

void SolverContext::MoveGenContext::Init(
  const int tricks,
  const int relStartHand,
  const int initialRanks[],
  const int initialSuits[],
  const unsigned short rankInSuit[DDS_HANDS][DDS_SUITS],
  const int trump,
  const int leadHand)
{
  thr_->moves.Init(tricks, relStartHand, initialRanks, initialSuits,
                   rankInSuit, trump, leadHand);
}

void SolverContext::MoveGenContext::PrintTrickStats(std::ofstream& fout) const
{
  thr_->moves.PrintTrickStats(fout);
}

void SolverContext::MoveGenContext::PrintFunctionStats(std::ofstream& fout) const
{
  thr_->moves.PrintFunctionStats(fout);
}

void SolverContext::MoveGenContext::PrintTrickDetails(std::ofstream& fout) const
{
  thr_->moves.PrintTrickDetails(fout);
}