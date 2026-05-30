/*
   DDS, a bridge double dummy solver.

   Scaffolding for an instance-scoped API. This is a no-behavior-change
   adapter that allows driving the solver with an explicit context,
   while internally delegating to existing code paths.
*/

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <system/thread_data.hpp>
#include <system/util/utilities.hpp>
#include <trans_table/trans_table.hpp>

// Minimal configuration scaffold for future expansion.
// TT configuration without depending on Memory headers.
enum class TTKind { Small, Large };

/**
 * @brief Configuration options for SolverContext instances.
 *
 * Provides per-context configuration for transposition tables.
 * Values are applied when creating or reconfiguring
 * a SolverContext and persist across lazy TT creation.
 */
struct SolverConfig
{
  TTKind tt_kind_ = TTKind::Large;
  int tt_mem_default_mb_ = 0;
  int tt_mem_maximum_mb_ = 0;
};

/**
 * @brief Instance-scoped solver context for DDS.
 *
 * Owns or references ThreadData and exposes lightweight facades for search
 * state, move generation, and utilities. The context manages an instance-owned
 * transposition table (TT) and provides explicit reset and configuration hooks.
 *
 * @note Thread safety: not inherently thread-safe. Use one SolverContext per thread.
 */
class SolverContext
{
public:
  explicit SolverContext(std::shared_ptr<ThreadData> thread, SolverConfig cfg = {})
  : thr_(std::move(thread)), cfg_(cfg)
  {
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

  /**
   * @brief Access the underlying ThreadData shared pointer.
   *
   * @return Shared ownership of the ThreadData used by this context.
   */
  auto thread() const -> std::shared_ptr<ThreadData>
  {
    return thr_;
  }

  /**
   * @brief Access the current configuration snapshot.
   *
   * @return Const reference to the configuration stored in this context.
   */
  auto config() const -> const SolverConfig&
  {
    return cfg_;
  }

  // --- Utilities facade ---
  class UtilitiesContext
  {
  public:
    explicit UtilitiesContext(::dds::Utilities* util)
      : util_(util)
    {
    }

    auto util() -> ::dds::Utilities&
    {
      return *util_;
    }

    auto util() const -> const ::dds::Utilities&
    {
      return *util_;
    }

    auto log_append(const std::string& s) -> void
    {
      util_->log_append(s);
    }

    auto log_buffer() const -> const std::vector<std::string>&
    {
      return util_->log_buffer();
    }

    auto log_clear() -> void
    {
      util_->log_clear();
    }

  private:
    ::dds::Utilities* util_ = nullptr;
  };

  /**
   * @brief Access utilities facade for logging and stats.
   */
  /**
   * @brief Access utilities facade for mutable contexts.
   */
  auto utilities() -> UtilitiesContext
  {
    return UtilitiesContext(&utils_);
  }

  /**
   * @brief Access utilities facade for const contexts.
   * @note Returns a const-only wrapper to preserve const-correctness.
   */
  auto utilities() const -> UtilitiesContext
  {
    return UtilitiesContext(const_cast<dds::Utilities*>(&utils_));
  }

  // Developer note — TT lifecycle (instance-scoped)
  //
  // - Ownership: Each SolverContext::SearchContext owns its TransTable (TT)
  //   via a std::unique_ptr created lazily on first access. There is no
  //   global TT registry and no ThreadData-owned TT.
  // - Configuration: The effective TT kind and memory sizes are determined by
  //   the SolverContext's SolverConfig (tt_kind_, tt_mem_default_mb_, tt_mem_maximum_mb_),
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

  // Returns the owned transposition table instance (creates if null)
  /**
   * @brief Get or create the transposition table.
   *
   * @return Pointer to the owned TT instance.
   */
  auto trans_table() const -> TransTable*;
  // Returns the TT instance if it exists, or nullptr
  /**
   * @brief Get the transposition table if already created.
   *
   * @return Pointer to the TT instance or nullptr.
   */
  auto maybe_trans_table() const -> TransTable*;

  // Dispose and erase the TT instance associated with this thread, if any.
  /**
   * @brief Dispose the owned transposition table immediately.
   */
  auto dispose_trans_table() const -> void;

  // Lightweight facades used by tests and call sites; no-ops if no TT exists.
  /**
   * @brief Reset search state for a new solve.
   *
   * Calls TT reset with ResetReason::FreeMemory when applicable.
   */
  auto reset_for_solve() const -> void;   // Calls reset_memory(ResetReason::FreeMemory)
  // Lightweight per-iteration reset matching legacy ResetBestMoves semantics.
  // Only clears bestMove[*].rank and bestMoveTT[*].rank, updates memUsed and ABStats.
  /**
   * @brief Lightweight reset used inside search iterations.
   */
  auto reset_best_moves_lite() const -> void;
  /**
   * @brief Return all TT memory to the system without destroying the TT.
   */
  auto clear_tt() const -> void;         // Calls ReturnAllMemory()
  /**
   * @brief Resize TT memory defaults and limits in-place if TT exists.
   */
  auto resize_tt(int defMB, int maxMB) const -> void; // Updates sizes if TT exists
  // Explicit runtime configuration of TT kind and memory limits. Applies to
  // existing TT (resize or recreate) and persists for future creations.
  /**
   * @brief Configure TT kind and memory limits.
   */
  auto configure_tt(TTKind kind, int defMB, int maxMB) -> void;

  // --- Search state facade ---
  /**
   * @brief Facade for per-solve search state.
   */
  class SearchContext
  {
  public:
    SearchContext() = default;

    explicit SearchContext(std::shared_ptr<ThreadData> thr)
      : thr_(std::move(thr))
    {
    }

    // Returns the owned transposition table instance (creates if null)
    auto trans_table() -> TransTable*;
    // Returns the TT instance if it exists, or nullptr
    auto maybe_trans_table() const -> TransTable* { return tt_.get(); }
    // Dispose and erase the TT instance owned by this context, if any.
    auto dispose_trans_table() -> void { tt_.reset(); }
    // Trivial accessors defined in the header so call sites in hot inner
    // loops (notably ab_search.cpp) get inlined direct field accesses
    // instead of cross-TU function calls. The previous out-of-line
    // definitions in solver_context.cpp added ~20% to total ab_search
    // self-time on Linux/x86_64.
    auto analysis_flag() -> bool& { return thr_->analysisFlag; }
    auto analysis_flag() const -> bool { return thr_->analysisFlag; }
    auto lowest_win(int depth, int suit) -> unsigned short& { return thr_->lowestWin[depth][suit]; }
    auto lowest_win(int depth, int suit) const -> const unsigned short& { return thr_->lowestWin[depth][suit]; }
    auto best_move(int depth) -> MoveType& { return thr_->bestMove[depth]; }
    auto best_move(int depth) const -> const MoveType& { return thr_->bestMove[depth]; }
    auto best_move_tt(int depth) -> MoveType& { return thr_->bestMoveTT[depth]; }
    auto best_move_tt(int depth) const -> const MoveType& { return thr_->bestMoveTT[depth]; }
    auto winners(int trickIndex) -> WinnersType& { return thr_->winners[trickIndex]; }
    auto winners(int trickIndex) const -> const WinnersType& { return thr_->winners[trickIndex]; }
    // Node type store for each hand (MAXNODE/MINNODE)
    auto node_type_store(int hand) -> int& { return thr_->nodeTypeStore[hand]; }
    auto node_type_store(int hand) const -> const int& { return thr_->nodeTypeStore[hand]; }
    // Access to forbidden moves buffer used by Moves::Purge and solver loops
    auto forbidden_moves() -> MoveType* { return thr_->forbiddenMoves; }
    auto forbidden_moves() const -> const MoveType* { return thr_->forbiddenMoves; }
    auto forbidden_move(int index) -> MoveType& { return thr_->forbiddenMoves[index]; }
    auto forbidden_move(int index) const -> const MoveType& { return thr_->forbiddenMoves[index]; }
    auto clear_forbidden_moves() -> void {
      for (int k = 0; k <= 13; ++k) {
        thr_->forbiddenMoves[k].rank = 0;
        thr_->forbiddenMoves[k].suit = 0;
      }
    }
    auto nodes() -> int& { return thr_->nodes; }
    auto nodes() const -> const int& { return thr_->nodes; }
    auto trick_nodes() -> int& { return thr_->trickNodes; }
    auto trick_nodes() const -> const int& { return thr_->trickNodes; }
    auto ini_depth() -> int& { return thr_->iniDepth; }
    auto ini_depth() const -> int { return thr_->iniDepth; }

  public:
    // Allow SolverContext to bind or rebind the underlying ThreadData
    // after construction (useful when SolverContext owns the ThreadData
    // and sets it up after default construction).
    auto set_thread(const std::shared_ptr<ThreadData>& thr) -> void
    {
      thr_ = thr;
    }

    // Bind the owning SolverContext instance for access to config/utilities/arena
    auto set_owner(SolverContext* owner) -> void
    {
      owner_ = owner;
    }

  private:
    std::shared_ptr<ThreadData> thr_;
    // Instance-owned transposition table, created lazily on first access.
    std::unique_ptr<TransTable> tt_;
    // Back-reference to the owning SolverContext (for config and utilities).
    SolverContext* owner_ = nullptr;
  };

  // Expose a persistent SearchContext owned by the SolverContext.
  /**
   * @brief Access the persistent search-state facade.
   */
  auto search() -> SearchContext&
  {
    return search_;
  }

  /**
   * @brief Access the persistent search-state facade (const).
   */
  auto search() const -> const SearchContext&
  {
    return search_;
  }

  // --- Move generation facade ---
  /**
   * @brief Facade for move generation utilities bound to ThreadData.
   */
  class MoveGenContext
  {
  public:
    // Non-owning. `thr` must outlive this MoveGenContext; in practice the
    // ThreadData is owned by the enclosing SolverContext's `thr_`
    // shared_ptr, so a raw pointer here is safe and lets `SolverContext
    // ::move_gen()` return a value-typed facade without an atomic
    // shared_ptr refcount bump on every call (~22 calls per ab_search
    // invocation, hot path).
    explicit MoveGenContext(ThreadData* thr)
      : thr_(thr)
    {
    }

    auto move_gen_0(
      const int tricks,
      const Pos& tpos,
      const MoveType& bestMove,
      const MoveType& bestMoveTT,
      const RelRanksType thrp_rel[]) -> int;

    auto move_gen_123(
      const int tricks,
      const int relHand,
      const Pos& tpos) -> int;

    auto purge(
      const int tricks,
      const int relHand,
      const MoveType forbiddenMoves[]) -> void;

    auto make_next(
      const int trick,
      const int relHand,
      const unsigned short win_ranks[]) -> const MoveType*;

    // Simpler variant without win_ranks used in several SolverIF paths
    auto make_next_simple(
      const int trick,
      const int relHand) -> const MoveType*;

    auto get_length(
      const int trick,
      const int relHand) const -> int;

    auto rewind(
      const int tricks,
      const int relHand) -> void;

    auto register_hit(
      const int tricks,
      const int relHand) -> void;

    // Reinitialize move generation for a new lead hand at a given trick
    auto reinit(
      const int tricks,
      const int leadHand) -> void;

    // Initialize move generation state for a given trick and starting hand
    auto init(
      const int tricks,
      const int relStartHand,
      const int initialRanks[],
      const int initialSuits[],
      const unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS],
      const int trump,
      const int leadHand) -> void;

  // Diagnostics (no behavior change; passthrough to Moves)
  // Note: Emission is controlled by DDS_MOVES / DDS_MOVES_DETAILS.
    auto print_trick_stats(std::ofstream& fout) const -> void;
    auto print_trick_details(std::ofstream& fout) const -> void;
    auto print_function_stats(std::ofstream& fout) const -> void;

    // Read-only access to per-trick generated metadata
    auto get_trick_data(const int tricks) -> const TrickDataType&;

 // Read-only textual dump helper
    auto trick_to_text(const int trick) const -> std::string;

    // Specify a particular move at a trick/hand position
    auto make_specific(
      const MoveType& mply,
      const int trick,
      const int relHand) -> void;

  private:
    ThreadData* thr_ = nullptr;
  };

  /**
   * @brief Access move generation facade.
   */
  auto move_gen() const -> MoveGenContext
  {
    return MoveGenContext(thr_.get());
  }

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

auto ThreadMemoryUsed() -> double;
