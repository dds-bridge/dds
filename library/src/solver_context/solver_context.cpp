#include "solver_context.hpp"

#include <api/dds.h>
#include <trans_table/trans_table_s.hpp>
#include <trans_table/trans_table_l.hpp>
#include <memory>
#include <cstdlib>
#include <iostream>

// Owned-ThreadData constructor: allocate ThreadData as a member of the
// SolverContext so callers can create a context at the top of the stack
// and pass it down without a separate per-thread lookup.
SolverContext::SolverContext(SolverConfig cfg)
  : thr_(nullptr), cfg_(cfg)
{
#ifdef DDS_DEFAULT_ARENA_BYTES
  if (cfg_.arena_capacity_bytes_ == 0ULL) {
    cfg_.arena_capacity_bytes_ = static_cast<std::size_t>(DDS_DEFAULT_ARENA_BYTES);
  }
#endif
  // Create an owned ThreadData instance and keep it in thr_.
  thr_ = std::make_shared<ThreadData>();
  if (cfg_.rng_seed_ != 0ULL) utils_.seed(cfg_.rng_seed_);
  // Ensure persistent facades like SearchContext see the bound ThreadData.
  search_.set_thread(thr_);
  search_.set_owner(this);
}

TransTable* SolverContext::trans_table() const
{
  // Delegate to per-context SearchContext member (lazy creation inside).
  return const_cast<SolverContext*>(this)->search_.trans_table();
}

// --- SearchContext disposal helper ---
void SolverContext::SearchContext::dispose_trans_table()
{
  // Simply reset the unique_ptr; logging/stats are handled by caller.
  tt_.reset();
}

// --- SearchContext out-of-line definitions ---
bool& SolverContext::SearchContext::analysis_flag() { return thr_->analysisFlag; }
bool SolverContext::SearchContext::analysis_flag() const { return thr_->analysisFlag; }
unsigned short& SolverContext::SearchContext::lowest_win(int depth, int suit) {
  return thr_->lowestWin[depth][suit];
}
const unsigned short& SolverContext::SearchContext::lowest_win(int depth, int suit) const {
  return thr_->lowestWin[depth][suit];
}
MoveType& SolverContext::SearchContext::best_move(int depth) {
  return thr_->bestMove[depth];
}
const MoveType& SolverContext::SearchContext::best_move(int depth) const {
  return thr_->bestMove[depth];
}
MoveType& SolverContext::SearchContext::best_move_tt(int depth) {
  return thr_->bestMoveTT[depth];
}
const MoveType& SolverContext::SearchContext::best_move_tt(int depth) const {
  return thr_->bestMoveTT[depth];
}
WinnersType& SolverContext::SearchContext::winners(int trickIndex) {
  return thr_->winners[trickIndex];
}
const WinnersType& SolverContext::SearchContext::winners(int trickIndex) const {
  return thr_->winners[trickIndex];
}
int& SolverContext::SearchContext::node_type_store(int hand) { return thr_->nodeTypeStore[hand]; }
const int& SolverContext::SearchContext::node_type_store(int hand) const { return thr_->nodeTypeStore[hand]; }
int& SolverContext::SearchContext::nodes() { return thr_->nodes; }
int& SolverContext::SearchContext::trick_nodes() { return thr_->trickNodes; }
int& SolverContext::SearchContext::ini_depth() { return thr_->iniDepth; }
int SolverContext::SearchContext::ini_depth() const { return thr_->iniDepth; }
MoveType* SolverContext::SearchContext::forbidden_moves() { return thr_->forbiddenMoves; }
const MoveType* SolverContext::SearchContext::forbidden_moves() const { return thr_->forbiddenMoves; }
MoveType& SolverContext::SearchContext::forbidden_move(int index) { return thr_->forbiddenMoves[index]; }
const MoveType& SolverContext::SearchContext::forbidden_move(int index) const { return thr_->forbiddenMoves[index]; }
void SolverContext::SearchContext::clear_forbidden_moves() {
  for (int k = 0; k <= 13; ++k) {
    thr_->forbiddenMoves[k].rank = 0;
    thr_->forbiddenMoves[k].suit = 0;
  }
}

// New: per-context transposition table accessors
TransTable* SolverContext::SearchContext::maybe_trans_table() const {
  return tt_ ? tt_.get() : nullptr;
}

TransTable* SolverContext::SearchContext::trans_table() {
  if (tt_) return tt_.get();
  // Require owner (for config and utilities). If missing, fall back
  // to Large with built-in defaults.
  TTKind kind = (owner_ ? owner_->config().tt_kind_ : TTKind::Large);
  int defMB = (owner_ ? owner_->config().tt_mem_default_mb_ : 0);
  int maxMB = (owner_ ? owner_->config().tt_mem_maximum_mb_ : 0);
  // Final fallback to THREADMEM_* constants
  if (defMB <= 0 || maxMB <= 0) {
    if (kind == TTKind::Small) {
      defMB = THREADMEM_SMALL_DEF_MB;
      maxMB = THREADMEM_SMALL_MAX_MB;
    } else {
      defMB = THREADMEM_LARGE_DEF_MB;
      maxMB = THREADMEM_LARGE_MAX_MB;
    }
  }
  // Optional environment overrides
  if (const char* s = std::getenv("DDS_TT_DEFAULT_MB")) {
    int v = std::atoi(s);
    if (v > 0) defMB = v;
  }
  if (const char* s = std::getenv("DDS_TT_LIMIT_MB")) {
    int v = std::atoi(s);
    if (v > 0) maxMB = std::min(maxMB, v);
  }
  if (maxMB < defMB) maxMB = defMB;

  // Create appropriate concrete table
  if (kind == TTKind::Small)
    tt_ = std::unique_ptr<TransTable>(new TransTableS());
  else
    tt_ = std::unique_ptr<TransTable>(new TransTableL());

  tt_->set_memory_default(defMB);
  tt_->set_memory_maximum(maxMB);
  tt_->make_tt();

#ifdef DDS_UTILITIES_LOG
  {
    const char kch = (kind == TTKind::Small ? 'S' : 'L');
    char buf[96];
    std::snprintf(buf, sizeof(buf), "tt:create|%c|%d|%d", kch, defMB, maxMB);
    if (owner_) owner_->utilities().log_append(std::string(buf));
  }
#endif

#ifdef DDS_UTILITIES_STATS
  if (owner_) owner_->utilities().util().stats().tt_creates++;
#endif

  // Optional one-time debug print per creation
  if (const char* dbg = std::getenv("DDS_DEBUG_TT_CREATE")) {
    if (*dbg) {
      std::cerr << "[DDS] TT create: kind="
                << (kind == TTKind::Small ? 'S' : 'L')
                << " defMB=" << defMB
                << " maxMB=" << maxMB
                << std::endl;
    }
  }

  return tt_.get();
}

TransTable* SolverContext::maybe_trans_table() const
{
  return search_.maybe_trans_table();
}

void SolverContext::dispose_trans_table() const
{
#ifdef DDS_UTILITIES_LOG
    // Append a tiny debug entry indicating TT disposal.
    utilities().log_append("tt:dispose");
#endif
#ifdef DDS_UTILITIES_STATS
    utilities().util().stats().tt_disposes++;
#endif
  // Dispose the member-owned TT (if any)
  const_cast<SolverContext*>(this)->search_.dispose_trans_table();
}

// Defaulted destructor defined out-of-line so destruction of the
// owned std::shared_ptr<ThreadData> happens where ThreadData is a
// complete type.
SolverContext::~SolverContext() = default;

void SolverContext::reset_for_solve() const
{
#ifdef DDS_UTILITIES_LOG
  {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "ctx:reset_for_solve");
    utilities().log_append(std::string(buf));
  }
#endif
  if (auto* tt = search_.maybe_trans_table())
    tt->reset_memory(ResetReason::FreeMemory);
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

void SolverContext::clear_tt() const
{
#ifdef DDS_UTILITIES_LOG
  utilities().log_append("tt:clear");
#endif
  if (auto* tt = search_.maybe_trans_table())
    tt->return_all_memory();
}

void SolverContext::resize_tt(int defMB, int maxMB) const
{
#ifdef DDS_UTILITIES_LOG
  {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "tt:resize|%d|%d", defMB, maxMB);
    utilities().log_append(std::string(buf));
  }
#endif
  if (auto* tt = search_.maybe_trans_table())
  {
    if (maxMB < defMB) maxMB = defMB;
  tt->set_memory_default(defMB);
  tt->set_memory_maximum(maxMB);
  }
}

void SolverContext::configure_tt(TTKind kind, int defMB, int maxMB)
{
  // Apply environment limit if present to preserve existing behavior.
  if (const char* s = std::getenv("DDS_TT_LIMIT_MB")) {
    int v = std::atoi(s);
    if (v > 0) maxMB = std::min(maxMB, v);
  }
  if (maxMB < defMB) maxMB = defMB;

  // Persist configuration for future TT creations.
  cfg_.tt_kind_ = kind;
  cfg_.tt_mem_default_mb_ = defMB;
  cfg_.tt_mem_maximum_mb_ = maxMB;

  auto* tt = search_.maybe_trans_table();
  if (!tt) return; // Nothing to apply now; will take effect on lazy creation.

  // If kind changes, dispose and recreate now to ensure effect is applied.
  bool is_small = (dynamic_cast<TransTableS*>(tt) != nullptr);
  TTKind current_kind = is_small ? TTKind::Small : TTKind::Large;
  if (current_kind != kind) {
    dispose_trans_table();
    // Force immediate creation with new config to keep behavior explicit.
    (void)trans_table();
    return;
  }

  // Same kind: resize in-place.
  resize_tt(defMB, maxMB);
}

// Lightweight reset matching legacy ResetBestMoves semantics.
void SolverContext::reset_best_moves_lite() const
{
#ifdef DDS_UTILITIES_LOG
  utilities().log_append("ctx:reset_best_moves_lite");
#endif
  if (!thr_) return;
  for (int d = 0; d <= 49; ++d)
  {
    thr_->bestMove[d].rank = 0;
    thr_->bestMoveTT[d].rank = 0;
  }
  // Keep memUsed in sync as the legacy code did
  if (auto* tt = search_.maybe_trans_table())
    thr_->memUsed = tt->memory_in_use() + ThreadMemoryUsed();
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
    8192 * sizeof(RelRanksType)
    / static_cast<double>(1024.);

  return memUsed;
}

// --- MoveGenContext out-of-line definitions ---
// No TLS allocator shim required: move generation now runs without a global allocator hook.

int SolverContext::MoveGenContext::move_gen_0(
  const int tricks,
  const Pos& tpos,
  const MoveType& bestMove,
  const MoveType& bestMoveTT,
  const RelRanksType thrp_rel[])
{
  auto rc = thr_->moves.MoveGen0(tricks, tpos, bestMove, bestMoveTT, thrp_rel);
  return rc;
}

int SolverContext::MoveGenContext::move_gen_123(
  const int tricks,
  const int relHand,
  const Pos& tpos)
{
  auto rc = thr_->moves.MoveGen123(tricks, relHand, tpos);
  return rc;
}

void SolverContext::MoveGenContext::purge(
  const int tricks,
  const int relHand,
  const MoveType forbiddenMoves[])
{
  thr_->moves.Purge(tricks, relHand, forbiddenMoves);
}

const MoveType* SolverContext::MoveGenContext::make_next(
  const int trick,
  const int relHand,
  const unsigned short win_ranks[])
{
  return thr_->moves.MakeNext(trick, relHand, win_ranks);
}

const MoveType* SolverContext::MoveGenContext::make_next_simple(
  const int trick,
  const int relHand)
{
  return thr_->moves.MakeNextSimple(trick, relHand);
}

int SolverContext::MoveGenContext::get_length(
  const int trick,
  const int relHand) const
{
  return thr_->moves.GetLength(trick, relHand);
}

void SolverContext::MoveGenContext::rewind(
  const int tricks,
  const int relHand)
{
  thr_->moves.Rewind(tricks, relHand);
}

void SolverContext::MoveGenContext::register_hit(
  const int tricks,
  const int relHand)
{
  thr_->moves.RegisterHit(tricks, relHand);
}

const TrickDataType& SolverContext::MoveGenContext::get_trick_data(const int tricks)
{
  return thr_->moves.GetTrickData(tricks);
}

void SolverContext::MoveGenContext::make_specific(
  const MoveType& mply,
  const int trick,
  const int relHand)
{
  thr_->moves.MakeSpecific(mply, trick, relHand);
}

std::string SolverContext::MoveGenContext::trick_to_text(const int trick) const
{
  return thr_->moves.TrickToText(trick);
}

void SolverContext::MoveGenContext::reinit(
  const int tricks,
  const int leadHand)
{
  thr_->moves.Reinit(tricks, leadHand);
}

void SolverContext::MoveGenContext::init(
  const int tricks,
  const int relStartHand,
  const int initialRanks[],
  const int initialSuits[],
  const unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS],
  const int trump,
  const int leadHand)
{
  thr_->moves.Init(tricks, relStartHand, initialRanks, initialSuits,
                   rank_in_suit, trump, leadHand);
}

void SolverContext::MoveGenContext::print_trick_stats(std::ofstream& fout) const
{
  thr_->moves.PrintTrickStats(fout);
}

void SolverContext::MoveGenContext::print_function_stats(std::ofstream& fout) const
{
  thr_->moves.PrintFunctionStats(fout);
}

void SolverContext::MoveGenContext::print_trick_details(std::ofstream& fout) const
{
  thr_->moves.PrintTrickDetails(fout);
}
