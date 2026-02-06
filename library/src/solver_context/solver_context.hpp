/*
   DDS, a bridge double dummy solver.

   Scaffolding for an instance-scoped API. This is a no-behavior-change
   adapter that allows driving the solver with an explicit context,
   while internally delegating to existing code paths.
*/

#ifndef DDS_SYSTEM_SOLVERCONTEXT_H
#define DDS_SYSTEM_SOLVERCONTEXT_H

#include <system/thread_data.hpp>
#include <system/util/utilities.hpp>
#include <trans_table/trans_table.hpp>

#include <string>
#include <vector>
#include <random>
#include <cstddef>
#include <memory>

// Minimal configuration scaffold for future expansion.
// TT configuration without depending on Memory headers.
enum class TTKind { Small, Large };

struct SolverConfig
{
  TTKind tt_kind_ = TTKind::Large;
  int tt_mem_default_mb_ = 0;
  int tt_mem_maximum_mb_ = 0;
  // Optional deterministic RNG seed (0 means "no explicit seed").
  unsigned long long rng_seed_ = 0ULL;
  // Optional arena capacity (bytes). 0 disables arena.
  std::size_t arena_capacity_bytes_ = 0ULL;
};

class SolverContext
{
public:
  explicit SolverContext(std::shared_ptr<ThreadData> thread, SolverConfig cfg = {})
  : thr_(std::move(thread)), cfg_(cfg)
  {
#ifdef DDS_DEFAULT_ARENA_BYTES
  if (cfg_.arena_capacity_bytes_ == 0ULL) {
    cfg_.arena_capacity_bytes_ = static_cast<std::size_t>(DDS_DEFAULT_ARENA_BYTES);
  }
#endif
  if (cfg_.rng_seed_ != 0ULL) utils_.seed(cfg_.rng_seed_);
    // Bind the persistent facades to the underlying ThreadData.
    search_.set_thread(thr_);
    search_.set_owner(this);
  }

  // NOTE: constructors that accepted raw ThreadData* were removed as part
  // of the ownership migration. Callers should pass a
  // std::shared_ptr<ThreadData> (non-owning wrappers can be created with
  // std::shared_ptr<ThreadData>(ptr, [](ThreadData*){})).

  // Construct a context that owns its ThreadData instance. This is the
  // preferred mode for the new instance-scoped API: callers can create a
  // SolverContext at the top of the call-stack and pass it downwards.
  explicit SolverContext(SolverConfig cfg = {});

  ~SolverContext();

  std::shared_ptr<ThreadData> thread() const { return thr_; }
  const SolverConfig& config() const { return cfg_; }

  // --- Utilities facade ---
  class UtilitiesContext {
  public:
    explicit UtilitiesContext(::dds::Utilities* util) : util_(util) {}
    ::dds::Utilities& util() { return *util_; }
    const ::dds::Utilities& util() const { return *util_; }
  std::mt19937& rng() { return util_->rng(); }
  const std::mt19937& rng() const { return util_->rng(); }
    void seed_rng(unsigned long long seed) { util_->seed(seed); }
    void log_append(const std::string& s) { util_->log_append(s); }
    const std::vector<std::string>& log_buffer() const { return util_->log_buffer(); }
    void log_clear() { util_->log_clear(); }
  private:
    ::dds::Utilities* util_ = nullptr;
  };

  inline UtilitiesContext utilities() { return UtilitiesContext(&utils_); }
  inline UtilitiesContext utilities() const { return UtilitiesContext(&utils_); }

  // Developer note — TT lifecycle (instance-scoped)
  //
  // - Ownership: Each SolverContext::SearchContext owns its TransTable (TT)
  //   via a std::unique_ptr created lazily on first access. There is no
  //   global TT registry and no ThreadData-owned TT.
  // - Configuration: The effective TT kind and memory sizes are determined by
  //   the SolverContext's SolverConfig (ttKind, ttMemDefaultMB, ttMemMaximumMB),
  //   with optional environment overrides:
  //     DDS_TT_DEFAULT_MB  — overrides default MB if > 0
  //     DDS_TT_LIMIT_MB    — caps maximum MB if > 0
  //   Call configure_tt(...) at runtime to persist a new configuration and apply
  //   it to an existing TT (resize in place) or recreate if the kind changes.
  // - Reset semantics:
  //     reset_for_solve()      — clears a subset of search state and calls
  //                               tt->reset_memory(FreeMemory) when a TT exists;
  //                               preserves the TT allocation for reuse.
  //     reset_best_moves_lite() — clears only best-move ranks and updates memUsed.
  //     clear_tt()              — returns all TT memory to the system; preserves
  //                               future config and recreates lazily on demand.
  //     dispose_trans_table()  — destroys the owned TT immediately.
  // - Diagnostics: When built with DDS_UTILITIES_LOG / DDS_UTILITIES_STATS, TT
  //   lifecycle events append compact log entries and bump small counters.
  //
  // Arena support has been removed from the SolverContext; logging uses
  // stack-allocated buffers only.

  // Returns the owned transposition table instance (creates if null)
  TransTable* trans_table() const;
  // Returns the TT instance if it exists, or nullptr
  TransTable* maybe_trans_table() const;

  // Dispose and erase the TT instance associated with this thread, if any.
  void dispose_trans_table() const;

  // Lightweight facades used by tests and call sites; no-ops if no TT exists.
  void reset_for_solve() const;   // Calls reset_memory(ResetReason::FreeMemory)
  // Lightweight per-iteration reset matching legacy ResetBestMoves semantics.
  // Only clears bestMove[*].rank and bestMoveTT[*].rank, updates memUsed and ABStats.
  void reset_best_moves_lite() const;
  void clear_tt() const;         // Calls ReturnAllMemory()
  void resize_tt(int defMB, int maxMB) const; // Updates sizes if TT exists
  // Explicit runtime configuration of TT kind and memory limits. Applies to
  // existing TT (resize or recreate) and persists for future creations.
  void configure_tt(TTKind kind, int defMB, int maxMB);

  // --- Search state facade ---
  class SearchContext {
  public:
  SearchContext() = default;
  explicit SearchContext(std::shared_ptr<ThreadData> thr) : thr_(std::move(thr)) {}
    // Returns the owned transposition table instance (creates if null)
    TransTable* trans_table();
    // Returns the TT instance if it exists, or nullptr
    TransTable* maybe_trans_table() const;
    // Dispose and erase the TT instance owned by this context, if any.
    void dispose_trans_table();
    // analysis flag used to control incremental analysis behavior
    bool& analysis_flag();
    bool analysis_flag() const;
    unsigned short& lowest_win(int depth, int suit);
    const unsigned short& lowest_win(int depth, int suit) const;
    MoveType& best_move(int depth);
    const MoveType& best_move(int depth) const;
    MoveType& best_move_tt(int depth);
    const MoveType& best_move_tt(int depth) const;
    WinnersType& winners(int trickIndex);
    const WinnersType& winners(int trickIndex) const;
  // Node type store for each hand (MAXNODE/MINNODE)
  int& node_type_store(int hand);
  const int& node_type_store(int hand) const;
    // Access to forbidden moves buffer used by Moves::Purge and solver loops
    MoveType* forbidden_moves();
    const MoveType* forbidden_moves() const;
    MoveType& forbidden_move(int index);
    const MoveType& forbidden_move(int index) const;
  void clear_forbidden_moves();
    int& nodes();
    int& trick_nodes();
    int& ini_depth();
    int ini_depth() const;
  private:
    std::shared_ptr<ThreadData> thr_;
    // Instance-owned transposition table, created lazily on first access.
    std::unique_ptr<TransTable> tt_;
    // Back-reference to the owning SolverContext (for config and utilities).
    SolverContext* owner_;
  public:
    // Allow SolverContext to bind or rebind the underlying ThreadData
    // after construction (useful when SolverContext owns the ThreadData
    // and sets it up after default construction).
    void set_thread(const std::shared_ptr<ThreadData>& thr) { thr_ = thr; }
    // Bind the owning SolverContext instance for access to config/utilities/arena
    void set_owner(SolverContext* owner) { owner_ = owner; }
  };

  // Expose a persistent SearchContext owned by the SolverContext.
  SearchContext& search() { return search_; }
  const SearchContext& search() const { return search_; }


  // --- Move generation facade ---
  class MoveGenContext {
  public:
    explicit MoveGenContext(std::shared_ptr<ThreadData> thr)
      : thr_(std::move(thr)) {}

    int move_gen_0(
      const int tricks,
      const Pos& tpos,
      const MoveType& bestMove,
      const MoveType& bestMoveTT,
      const RelRanksType thrp_rel[]);

    int move_gen_123(
      const int tricks,
      const int relHand,
      const Pos& tpos);

    void purge(
      const int tricks,
      const int relHand,
      const MoveType forbiddenMoves[]);

    const MoveType* make_next(
      const int trick,
      const int relHand,
      const unsigned short win_ranks[]);

    // Simpler variant without win_ranks used in several SolverIF paths
    const MoveType* make_next_simple(
      const int trick,
      const int relHand);

    int get_length(
      const int trick,
      const int relHand) const;

    void rewind(
      const int tricks,
      const int relHand);

    void register_hit(
      const int tricks,
      const int relHand);

    // Reinitialize move generation for a new lead hand at a given trick
    void reinit(
      const int tricks,
      const int leadHand);

    // Initialize move generation state for a given trick and starting hand
    void init(
      const int tricks,
      const int relStartHand,
      const int initialRanks[],
      const int initialSuits[],
      const unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS],
      const int trump,
      const int leadHand);

  // Diagnostics (no behavior change; passthrough to Moves)
  // Note: Emission is controlled by DDS_MOVES / DDS_MOVES_DETAILS.
    void print_trick_stats(std::ofstream& fout) const;
    void print_trick_details(std::ofstream& fout) const;
    void print_function_stats(std::ofstream& fout) const;

    // Read-only access to per-trick generated metadata
    const TrickDataType& get_trick_data(const int tricks);

 // Read-only textual dump helper
    std::string trick_to_text(const int trick) const;

    // Specify a particular move at a trick/hand position
    void make_specific(
      const MoveType& mply,
      const int trick,
      const int relHand);

  private:
    std::shared_ptr<ThreadData> thr_;
  };

  inline MoveGenContext move_gen() const { return MoveGenContext(thr_); }

private:
  // Shared ownership of per-context ThreadData. Callers can construct
  // a context with an externally-owned std::shared_ptr<ThreadData> or
  // let the context create/own one via the default constructor.
  std::shared_ptr<ThreadData> thr_;
  // Persistent facade objects bound to this context. `search_` is
  // initialized after `thr_` is set in constructors.
  SearchContext search_;
  SolverConfig cfg_{};
  mutable ::dds::Utilities utils_{};
  // Arena removed.
  // NOTE: `owned_thr_` removed; `thr_` now represents the shared ownership
  // (if any) for this context.
  // Transposition table is now owned per SearchContext and created lazily.
  //
  // See the developer note above for details on TT lifecycle and resets.
};

double ThreadMemoryUsed();

#endif // DDS_SYSTEM_SOLVERCONTEXT_H
