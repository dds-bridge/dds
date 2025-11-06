/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TRANSTABLES_H
#define DDS_TRANSTABLES_H

/*
   This is an object for managing transposition tables and the
   associated memory.  Compared to TransTableL it uses a lot less
   memory and takes somewhat longer time.
*/


#include <vector>
#include <string>

#include "TransTable.hpp"


class TransTableS: public TransTable
{
  private:

    // Structures for the small memory option.

    struct WinCard
    {
      int order_set_;
      int win_mask_;
      NodeCards * first_;
      WinCard * prev_win_;
      WinCard * next_win_;
      WinCard * next_;
    };

    struct PosSearchSmall
    {
      WinCard * pos_search_point_;
      long long suit_lengths_;
      PosSearchSmall * left_;
      PosSearchSmall * right_;
    };

    struct TtAggr
    {
      int aggr_ranks_[DDS_SUITS];
      int win_mask_[DDS_SUITS];
    };

    struct StatsResets
    {
      int no_of_resets;
      int aggr_resets[kResetReasonCount];
    };


    long long aggr_len_sets_[14];
    StatsResets stats_resets_;

    WinCard temp_win_[5];
    int node_set_size_limit_;
    int win_set_size_limit_;
    unsigned long long maxmem_;
    unsigned long long allocmem_;
    unsigned long long summem_;
    int wmem_;
    int nmem_;
    int max_index_;
    int wcount_;
    int ncount_;
    bool clear_tt_flag_;
    int windex_;
    TtAggr * aggp_;

    PosSearchSmall * rootnp_[14][DDS_HANDS];
    WinCard ** pw_;
    NodeCards ** pn_;
    PosSearchSmall ** pl_[14][DDS_HANDS];
    NodeCards * node_cards_;
    WinCard * win_cards_;
    PosSearchSmall * pos_search_[14][DDS_HANDS];
    int node_set_size_; /* Index with range 0 to node_set_size_limit_ */
    int win_set_size_;  /* Index with range 0 to win_set_size_limit_ */
    int len_set_ind_[14][DDS_HANDS];
    int lcount_[14][DDS_HANDS];

    std::vector<std::string> reset_text_;

    long long suit_lengths_[14];

    int tt_in_use_;

    // Constants are provided via internal function-local static tables.

    auto wipe() -> void;

    auto init_tt() -> void;

    auto add_win_set() -> void;

    auto add_node_set() -> void;

    auto add_len_set(
      int trick, 
      int first_hand) -> void;

    auto build_sop(
      const unsigned short our_win_ranks[DDS_SUITS],
      const unsigned short aggr_arg[DDS_SUITS],
      const NodeCards& first,
      long long lengths,
      int tricks,
      int first_hand,
      bool flag
    ) -> void;

    auto build_path(
      const int win_mask[],
      const int win_order_set[],
      int u_bound,
      int l_bound,
      char best_move_suit,
      char best_move_rank,
      PosSearchSmall * node_ptr,
      bool& result
    ) -> NodeCards *;

    auto search_len_and_insert(
      PosSearchSmall * root_ptr,
      long long key,
      bool insert_node,
      int trick,
      int first_hand,
      bool& result
    ) -> PosSearchSmall *;

    auto update_sop(
      int u_bound,
      int l_bound,
      char best_move_suit,
      char best_move_rank,
      NodeCards * node
    ) -> NodeCards *;

    auto find_sop(
      const int order_set[],
      int limit,
      WinCard * node_p,
      bool& lower_flag
    ) -> NodeCards const *;

  public:

    TransTableS();

    ~TransTableS();

    void init(const int hand_lookup[][15]) override;
    void set_memory_default(int megabytes) override;
    void set_memory_maximum(int megabytes) override;
    void make_tt() override;
    void reset_memory(ResetReason reason) override;
    void return_all_memory() override;
    auto memory_in_use() const -> double override;

    auto lookup(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const int hand_dist[],
      int limit,
      bool& lower_flag
    ) -> NodeCards const * override;
    void add(
      int trick,
      int hand,
      const unsigned short aggr_target[],
      const unsigned short win_ranks_arg[],
      const NodeCards& first,
      bool flag
    ) override;

  // The small TT does not provide verbose dumping; implement no-op printers
  void print_suits(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
  void print_all_suits(std::ofstream& /*fout*/) const override {}
  void print_suit_stats(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
  void print_all_suit_stats(std::ofstream& /*fout*/) const override {}
  void print_summary_suit_stats(std::ofstream& /*fout*/) const override {}
  void print_entries_dist(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/, const int /*hand_dist*/[]) const override {}
  void print_entries_dist_and_cards(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/, const unsigned short /*aggr_target*/[], const int /*hand_dist*/[]) const override {}
  void print_entries(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
  void print_all_entries(std::ofstream& /*fout*/) const override {}
  void print_entry_stats(std::ofstream& /*fout*/, int /*trick*/, int /*hand*/) const override {}
  void print_all_entry_stats(std::ofstream& /*fout*/) const override {}
  void print_summary_entry_stats(std::ofstream& /*fout*/) const override {}

  // Bridge stats printers to existing small-TT implementations
  void print_node_stats(std::ofstream& fout) const override { PrintNodeStats(fout); }
  void print_reset_stats(std::ofstream& fout) const override { PrintResetStats(fout); }

  void PrintNodeStats(std::ofstream& fout) const;

  void PrintResetStats(std::ofstream& fout) const;
};

#endif
