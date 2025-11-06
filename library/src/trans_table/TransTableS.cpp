/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <array>
#include <api/dds.h>

#include "TransTableS.hpp"

#define NSIZE 50000
#define WSIZE 50000
#define NINIT 60000
#define WINIT 170000
#define LSIZE 200 // Per trick and first hand

// Accessor for a lazily initialized, immutable TTlowestRank table.
static const std::array<int, 8192>& TTLowestRankTable()
{
  static const std::array<int, 8192> table = []{
    std::array<int, 8192> t{};
    unsigned int topBitRank = 1;
    t[0] = 15; // Void
    for (unsigned ind = 1; ind < 8192; ind++)
    {
      if (ind >= (topBitRank + topBitRank)) /* Next top bit */
        topBitRank <<= 1;
      t[ind] = t[ind ^ topBitRank] - 1;
    }
    return t;
  }();
  return table;
}

// Local using-declarations for readability in this implementation file only.
using std::ofstream;
using std::endl;
using std::setw;
using std::left;
using std::right;
using std::string;

/**
 * @brief Small transposition table for double dummy solver.
 *
 * TransTableS implements a small, memory-constrained transposition table
 * used to cache and retrieve results during double dummy bridge analysis.
 * It is designed for environments with limited memory resources and provides
 * similar lookup and caching functionality as TransTableL, but with a smaller footprint.
*/
TransTableS::TransTableS()
{
  // Ensure the table is built once.
  (void)TTLowestRankTable();
  tt_in_use_ = 0;
}


/**
 * @brief Destroy the TransTableS object and free all memory.
 *
 * Calls ReturnAllMemory to release all allocated resources.
 */
TransTableS::~TransTableS()
{
  TransTableS::return_all_memory();
}


// SetConstants removed; constants are produced by TTLowestRankTable().


auto TransTableS::init(const int handLookup[][15]) -> void {
  unsigned int topBitRank = 1;
  unsigned int topBitNo = 2;

  for (int s = 0; s < DDS_SUITS; s++)
  {
    aggp_[0].aggr_ranks_[s] = 0;
    aggp_[0].win_mask_[s] = 0;
  }

  for (unsigned int ind = 1; ind < 8192; ind++)
  {
    if (ind >= (topBitRank + topBitRank))
    {
      /* Next top bit */
      topBitRank <<= 1;
      topBitNo++;
    }
    aggp_[ind] = aggp_[ind ^ topBitRank];

    for (int s = 0; s < 4; s++)
    {
      aggp_[ind].aggr_ranks_[s] =
        (aggp_[ind].aggr_ranks_[s] >> 2) |
        (handLookup[s][topBitNo] << 24);

      aggp_[ind].win_mask_[s] =
        (aggp_[ind].win_mask_[s] >> 2) | (3 << 24);
    }
  }

  reset_text_.resize(kResetReasonCount);
  reset_text_[static_cast<int>(ResetReason::Unknown)] = "Unknown reason";
  reset_text_[static_cast<int>(ResetReason::TooManyNodes)] = "Too many nodes";
  reset_text_[static_cast<int>(ResetReason::NewDeal)] = "New deal";
  reset_text_[static_cast<int>(ResetReason::NewTrump)] = "New trump";
  reset_text_[static_cast<int>(ResetReason::MemoryExhausted)] = "Memory exhausted";
  reset_text_[static_cast<int>(ResetReason::FreeMemory)] = "Free thread memory";

  return;
}


auto TransTableS::set_memory_default([[maybe_unused]] const int megabytes) -> void { }


auto TransTableS::set_memory_maximum(const int megabytes) -> void {
  maxmem_ = 1000000ULL * static_cast<unsigned long long>(megabytes);
}


auto TransTableS::make_tt() -> void {
  // Note: keep local variables minimal; indices are declared in inner scopes.

  if (!tt_in_use_)
  {
    tt_in_use_ = 1;

  summem_ = (1ULL * (WINIT + 1) * sizeof(WinCard)) +
    (1ULL * (NINIT + 1) * sizeof(NodeCards)) +
       (1ULL * (LSIZE + 1) * 52 * sizeof(PosSearchSmall));
  wmem_ = static_cast<int>(1ULL * (WSIZE + 1) * sizeof(WinCard));
  nmem_ = static_cast<int>(1ULL * (NSIZE + 1) * sizeof(NodeCards));

    // Compute how many additional slabs we could potentially allocate.
    // Guard against negative values if maxmem_ < summem_ (which can happen
    // with very small configured limits).
    if (maxmem_ <= summem_)
      max_index_ = 0;
    else
    {
      const unsigned long long denom =
        static_cast<unsigned long long>(1ULL * (WSIZE + 1) * sizeof(WinCard));
      max_index_ = static_cast<int>((maxmem_ - summem_) / denom);
      if (max_index_ < 0)
        max_index_ = 0;
    }

    // Optional debug to aid troubleshooting when tuning memory limits.
    if (const char* dbg = std::getenv("DDS_DEBUG_TT_CREATE"))
    {
      if (*dbg)
      {
        std::cerr << "[DDS] TT(S) init: maxmem_=" << maxmem_
                  << " summem_=" << summem_
                  << " wmem_=" << wmem_
                  << " nmem_=" << nmem_
                  << " max_index_=" << max_index_
                  << std::endl;
      }
    }

    pw_ = static_cast<WinCard **>(calloc(static_cast<unsigned int>(max_index_ + 1), sizeof(WinCard *)));
    if (pw_ == NULL)
      exit(1);

  pn_ = static_cast<NodeCards **>(calloc(static_cast<unsigned int>(max_index_ + 1), sizeof(NodeCards *)));
    if (pn_ == NULL)
      exit(1);

    for (int k = 1; k <= 13; k++)
      for (int h = 0; h < DDS_HANDS; h++)
      {
        pl_[k][h] = static_cast
                   <PosSearchSmall **>(calloc(static_cast<unsigned int>(max_index_ + 1),
                                             sizeof(PosSearchSmall *)));
        if (pl_[k][h] == NULL)
          exit(1);
      }

    pw_[0] = static_cast<WinCard *>(calloc(WINIT + 1, sizeof(WinCard)));
    if (pw_[0] == NULL)
      exit(1);

  pn_[0] = static_cast<NodeCards *>(calloc(NINIT + 1, sizeof(NodeCards)));
    if (pn_[0] == NULL)
      exit(1);

    for (int k = 1; k <= 13; k++)
      for (int h = 0; h < DDS_HANDS; h++)
      {
        pl_[k][h][0] = static_cast<PosSearchSmall *>(calloc((LSIZE + 1),
                      sizeof(PosSearchSmall)));
        if (pl_[k][h][0] == NULL)
          exit(1);
      }

    aggp_ = static_cast<TtAggr *>(calloc(8192, sizeof(TtAggr)));
    if (aggp_ == NULL)
      exit(1);

    init_tt();

    for (int k = 1; k <= 13; k++)
      aggr_len_sets_[k] = 0;
    stats_resets_.no_of_resets = 0;
    for (int k = 0; k < kResetReasonCount; k++)
      stats_resets_.aggr_resets[k] = 0;

  }

  return;
}


auto TransTableS::wipe() -> void {
  int m;

  for (m = 1; m <= wcount_; m++)
  {
    if (pw_[m])
      free(pw_[m]);
    pw_[m] = NULL;
  }
  for (m = 1; m <= ncount_; m++)
  {
    if (pn_[m])
      free(pn_[m]);
    pn_[m] = NULL;
  }

  for (int k = 1; k <= 13; k++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      for (m = 1; m <= lcount_[k][h]; m++)
      {
        if (pl_[k][h][m])
          free(pl_[k][h][m]);
        pl_[k][h][m] = NULL;
      }
    }
  }

  allocmem_ = summem_;

  return;
}



auto TransTableS::init_tt() -> void {
  win_set_size_limit_ = WINIT;
  node_set_size_limit_ = NINIT;
  allocmem_ = (WINIT + 1) * sizeof(WinCard);
  allocmem_ += 1ULL * (NINIT + 1) * sizeof(NodeCards);
  allocmem_ += 1ULL * (LSIZE + 1) * 52 * sizeof(PosSearchSmall);
  win_cards_ = pw_[0];
  node_cards_ = pn_[0];
  wcount_ = 0;
  ncount_ = 0;

  node_set_size_ = 0;
  win_set_size_ = 0;

  clear_tt_flag_ = false;
  windex_ = -1;

  for (int k = 1; k <= 13; k++)
    for (int h = 0; h < DDS_HANDS; h++)
    {
      pos_search_[k][h] = pl_[k][h][0];
      // Set len_set_ind_ to 1 (not 0) because index 0 is reserved for the root node.
      // This ensures that the first Lookup/Add can safely use index 1,
      // and avoids overwriting the valid empty node at index 0.
      len_set_ind_[k][h] = 1;
      lcount_[k][h] = 0;
      // Initialize the root node to a valid empty node so that a
      // first Lookup/Add can function even before ResetMemory.
      pos_search_[k][h][0].suit_lengths_ = 0;
      pos_search_[k][h][0].pos_search_point_ = NULL;
      pos_search_[k][h][0].left_ = NULL;
      pos_search_[k][h][0].right_ = NULL;
      rootnp_[k][h] = &(pos_search_[k][h][0]);
    }
}


auto TransTableS::reset_memory([[maybe_unused]] const ResetReason reason) -> void {
  wipe();

  init_tt();

  for (int k = 1; k <= 13; k++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      rootnp_[k][h] = &(pos_search_[k][h][0]);
      pos_search_[k][h][0].suit_lengths_ = 0;
      pos_search_[k][h][0].pos_search_point_ = NULL;
      pos_search_[k][h][0].left_ = NULL;
      pos_search_[k][h][0].right_ = NULL;

      len_set_ind_[k][h] = 1;
    }
  }

#if defined(DDS_TT_STATS)
  stats_resets_.no_of_resets++;
  stats_resets_.aggr_resets[static_cast<int>(reason)]++;
#endif

  return;
}

auto TransTableS::return_all_memory() -> void {

  if (!tt_in_use_)
    return;
  tt_in_use_ = 0;

  wipe();

  if (pw_[0])
    free(static_cast<void*>(pw_[0]));
  pw_[0] = NULL;

  if (pn_[0])
    free(static_cast<void*>(pn_[0]));
  pn_[0] = NULL;

  for (int k = 1; k <= 13; k++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
        if (pl_[k][h][0])
          free(static_cast<void*>(pl_[k][h][0]));
      pl_[k][h][0] = NULL;
    }
  }

  if (pw_)
    free(static_cast<void*>(pw_));
  pw_ = NULL;

  if (pn_)
    free(static_cast<void*>(pn_));
  pn_ = NULL;

  if (aggp_)
    free(aggp_);
  aggp_ = NULL;

  return;
}


auto TransTableS::memory_in_use() const -> double {
  int ttMem = static_cast<int>(allocmem_);
  int aggrMem = 8192 * static_cast<int>(sizeof(TtAggr));
  return (ttMem + aggrMem) / static_cast<double>(1024.);
}


NodeCards const * TransTableS::lookup(
  const int trick,
  const int hand,
  const unsigned short aggrTarget[],
  const int handDist[],
  const int limit,
  bool& lowerFlag)
{
  bool res;
  PosSearchSmall * pp;
  int order_set_[DDS_SUITS];
  NodeCards const * cardsP;

  suit_lengths_[trick] =
    (static_cast<long long>(handDist[0]) << 36) |
    (static_cast<long long>(handDist[1]) << 24) |
    (static_cast<long long>(handDist[2]) << 12) |
    (static_cast<long long>(handDist[3]));

  pp = search_len_and_insert(rootnp_[trick][hand],
    suit_lengths_[trick], false, trick, hand, res);

  /* Find node that fits the suit lengths */
  if ((pp != NULL) && res)
  {
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      order_set_[ss] =
        aggp_[aggrTarget[ss]].aggr_ranks_[ss];
    }

    if (pp->pos_search_point_ == NULL)
      cardsP = NULL;
    else
    {
      cardsP = find_sop(order_set_, limit, pp->pos_search_point_, lowerFlag);

      if (cardsP == NULL)
        return cardsP;
    }
  }
  else
  {
    cardsP = NULL;
  }
  return cardsP;
}


auto TransTableS::add(
  const int tricks,
  const int hand,
  const unsigned short aggrTarget[],
  const unsigned short our_win_ranks[],
  const NodeCards& first,
  const bool flag) -> void {
  build_sop(our_win_ranks, aggrTarget, first, suit_lengths_[tricks],
           tricks, hand, flag);

  if (clear_tt_flag_)
    reset_memory(ResetReason::MemoryExhausted);

  return;
}


auto TransTableS::add_win_set() -> void {
  if (clear_tt_flag_)
  {
    windex_++;
    win_set_size_ = windex_;
    win_cards_ = &(temp_win_[windex_]);
  }
  else if (win_set_size_ >= win_set_size_limit_)
  {
    /* The memory chunk for the win_cards_ structure will be exceeded. */
    if (((allocmem_ + static_cast<unsigned long long>(wmem_)) > maxmem_) || (wcount_ >= max_index_) ||
        (win_set_size_ > SIMILARMAXWINNODES))
    {
      /* Already allocated memory plus needed allocation overshot maxmem_ */
      windex_++;
      win_set_size_ = windex_;
      clear_tt_flag_ = true;
      win_cards_ = &(temp_win_[windex_]);
    }
    else
    {
      wcount_++;
      win_set_size_limit_ = WSIZE;
      pw_[wcount_] =
        static_cast<WinCard *>(malloc((WSIZE + 1) * sizeof(WinCard)));
      if (pw_[wcount_] == NULL)
      {
        clear_tt_flag_ = true;
        windex_++;
        win_set_size_ = windex_;
        win_cards_ = &(temp_win_[windex_]);
      }
      else
      {
        allocmem_ += (WSIZE + 1) * sizeof(WinCard);
        win_set_size_ = 0;
        win_cards_ = pw_[wcount_];
      }
    }
  }
  else
    win_set_size_++;
  return;
}

auto TransTableS::add_node_set() -> void {
  if (node_set_size_ >= node_set_size_limit_)
  {
    /* The memory chunk for the node_cards_ structure will be exceeded. */
    if (((allocmem_ + static_cast<unsigned long long>(nmem_)) > maxmem_) || (ncount_ >= max_index_))
    {
      /* Already allocated memory plus needed allocation overshot maxmem_ */
      clear_tt_flag_ = true;
    }
    else
    {
      ncount_++;
      node_set_size_limit_ = NSIZE;
      pn_[ncount_] =
  static_cast<NodeCards *>(malloc((NSIZE + 1) * sizeof(NodeCards)));
      if (pn_[ncount_] == NULL)
      {
        clear_tt_flag_ = true;
      }
      else
      {
  allocmem_ += (NSIZE + 1) * sizeof(NodeCards);
        node_set_size_ = 0;
        node_cards_ = pn_[ncount_];
      }
    }
  }
  else
    node_set_size_++;
  return;
}

auto TransTableS::add_len_set(
  const int trick,
  const int first_hand) -> void {
  if (len_set_ind_[trick][first_hand] < LSIZE) {
    len_set_ind_[trick][first_hand]++;
#if defined(DDS_TT_STATS)
    aggr_len_sets_[trick]++;
#endif
    return;
  }

  // The memory chunk for the PosSearchSmall structure 
  // will be exceeded.

  const int incr = (LSIZE+1) * sizeof(PosSearchSmall);

  if ((allocmem_ + incr > maxmem_) || (lcount_[trick][first_hand] >= max_index_))
  {
    // Already allocated memory plus needed allocation overshot maxmem_.
    clear_tt_flag_ = true;
    return;
  }

  // Obtain another memory chunk LSIZE.

  lcount_[trick][first_hand]++;

  pl_[trick][first_hand][lcount_[trick][first_hand]] =
    static_cast<PosSearchSmall *>(malloc(incr));

  if (pl_[trick][first_hand][lcount_[trick][first_hand]] == NULL)
  {
    clear_tt_flag_ = true;
    return;
  }

  allocmem_ += incr;
  len_set_ind_[trick][first_hand] = 0;
  pos_search_[trick][first_hand] =
    pl_[trick][first_hand][lcount_[trick][first_hand]];
#if defined(DDS_TT_STATS)
  aggr_len_sets_[trick]++;
#endif
}


auto TransTableS::build_sop(
  const unsigned short our_win_ranks[DDS_SUITS],
  const unsigned short aggr_arg[DDS_SUITS],
  const NodeCards& first,
  const long long lengths,
  const int tricks,
  const int first_hand,
  const bool flag) -> void {
  int win_mask_[DDS_SUITS];
  int win_order_set[DDS_SUITS];
  char low[DDS_SUITS];

  for (int ss = 0; ss < DDS_SUITS; ss++)
  {
    int w = our_win_ranks[ss];
    if (w == 0)
    {
      win_mask_[ss] = 0;
      win_order_set[ss] = 0;
      low[ss] = 15;
    }
    else
    {
      w = w & (-w);       /* Only lowest win */
      const unsigned short temp = 
        static_cast<unsigned short>(aggr_arg[ss] & (-w));

      win_mask_[ss] = aggp_[temp].win_mask_[ss];
      win_order_set[ss] = aggp_[temp].aggr_ranks_[ss];
    low[ss] = static_cast<char>(TTLowestRankTable()[static_cast<size_t>(temp)]);
    }
  }

  bool res;
  PosSearchSmall * np = search_len_and_insert(
    rootnp_[tricks][first_hand], lengths, true, tricks, first_hand, res);

  NodeCards * cardsP = build_path(
    win_mask_, 
    win_order_set,
    static_cast<int>(first.upper_bound), 
    static_cast<int>(first.lower_bound),
    static_cast<char>(first.best_move_suit), 
    static_cast<char>(first.best_move_rank),
    np, 
    res);

  if (res)
  {
    cardsP->upper_bound = static_cast<char>(first.upper_bound);
    cardsP->lower_bound = static_cast<char>(first.lower_bound);

    if (flag)
    {
      cardsP->best_move_suit = static_cast<char>(first.best_move_suit);
      cardsP->best_move_rank = static_cast<char>(first.best_move_rank);
    }
    else
    {
      cardsP->best_move_suit = 0;
      cardsP->best_move_rank = 0;
    }

    for (int k = 0; k < DDS_SUITS; k++)
  cardsP->least_win[k] = static_cast<char>(15 - low[k]);
  }
}


auto TransTableS::build_path(
  const int win_mask_[],
  const int win_order_set[],
  const int u_bound,
  const int l_bound,
  const char best_move_suit,
  const char best_move_rank,
  PosSearchSmall * node_ptr,
  bool& result) -> NodeCards * {
  /* If result is TRUE, a new SOP has been created and build_path returns a
  pointer to it. If result is FALSE, an existing SOP is used and build_path
  returns a pointer to the SOP */

  bool found;
  WinCard * np, *p2, *nprev;
  NodeCards *p;

  np = node_ptr->pos_search_point_;
  nprev = NULL;
  int suit = 0;

  /* If winning node has a card that equals the next winning card deduced
  from the position, then there already exists a (partial) path */

  if (np == NULL)
  {
    /* There is no winning list created yet */
    /* Create winning nodes */
    p2 = &(win_cards_[win_set_size_]);
    add_win_set();
    p2->next_ = NULL;
    p2->next_win_ = NULL;
    p2->prev_win_ = NULL;
    node_ptr->pos_search_point_ = p2;
    p2->win_mask_ = win_mask_[suit];
    p2->order_set_ = win_order_set[suit];
    p2->first_ = NULL;
    np = p2;           /* Latest winning node */
    suit++;
    while (suit < DDS_SUITS)
    {
      p2 = &(win_cards_[win_set_size_]);
      add_win_set();
      np->next_win_ = p2;
      p2->prev_win_ = np;
      p2->next_ = NULL;
      p2->next_win_ = NULL;
      p2->win_mask_ = win_mask_[suit];
      p2->order_set_ = win_order_set[suit];
      p2->first_ = NULL;
      np = p2;         /* Latest winning node */
      suit++;
    }
    p = &(node_cards_[node_set_size_]);
    add_node_set();
    np->first_ = p;
    result = true;
    return p;
  }
  else
  {
    /* Winning list exists */
    while (1)
    {
      /* Find all winning nodes that correspond to current position */
      found = false;
      while (1)      /* Find node amongst alternatives */
      {
        if ((np->win_mask_ == win_mask_[suit]) &&
            (np->order_set_ == win_order_set[suit]))
        {
          /* Part of path found */
          found = true;
          nprev = np;
          break;
        }
        if (np->next_ != NULL)
          np = np->next_;
        else
          break;
      }
      if (found)
      {
        suit++;
        if (suit >= DDS_SUITS)
        {
          result = false;
          return update_sop(u_bound, l_bound, best_move_suit, best_move_rank,
            np->first_);
        }
        else
        {
          np = np->next_win_;       /* Find next winning node  */
          continue;
        }
      }
      else
        break;                    /* Node was not found */
    }               /* End outer while */

    /* Create additional node, coupled to existing node(s) */
    p2 = &(win_cards_[win_set_size_]);
    add_win_set();
    p2->prev_win_ = nprev;
    if (nprev != NULL)
    {
      p2->next_ = nprev->next_win_;
      nprev->next_win_ = p2;
    }
    else
    {
      p2->next_ = node_ptr->pos_search_point_;
      node_ptr->pos_search_point_ = p2;
    }
    p2->next_win_ = NULL;
    p2->win_mask_ = win_mask_[suit];
    p2->order_set_ = win_order_set[suit];
    p2->first_ = NULL;
    np = p2;          /* Latest winning node */
    suit++;

    /* Rest of path must be created */
    while (suit < 4)
    {
      p2 = &(win_cards_[win_set_size_]);
      add_win_set();
      np->next_win_ = p2;
      p2->prev_win_ = np;
      p2->next_ = NULL;
      p2->win_mask_ = win_mask_[suit];
      p2->order_set_ = win_order_set[suit];
      p2->first_ = NULL;
      p2->next_win_ = NULL;
      np = p2;         /* Latest winning node */
      suit++;
    }

    /* All winning nodes in SOP have been traversed and new nodes created */
    p = &(node_cards_[node_set_size_]);
    add_node_set();
    np->first_ = p;
    result = true;
    return p;
  }
}

auto TransTableS::search_len_and_insert(
  PosSearchSmall * root_ptr,
  const long long key,
  const bool insert_node,
  const int trick,
  const int first_hand,
  bool& result) -> TransTableS::PosSearchSmall * {
  /* Search for node which matches with the suit length combination
  given by parameter key. If no such node is found, NULL is
  returned if parameter insert_node is FALSE, otherwise a new
  node is inserted with suit_lengths_ set to key, the pointer to
  this node is returned.
  The algorithm used is defined in Knuth "The art of computer
  programming", vol.3 "Sorting and searching", 6.2.2 Algorithm T,
  page 424. */

  PosSearchSmall * np, *p, *sp;

  sp = NULL;
  if (insert_node)
    sp = &(pos_search_[trick][first_hand][len_set_ind_[trick][first_hand]]);

  np = root_ptr;
  while (1)
  {
    if (key == np->suit_lengths_)
    {
      result = true;
      return np;
    }
    else if (key < np->suit_lengths_)
    {
      if (np->left_ != NULL)
        np = np->left_;
      else if (insert_node)
      {
        p = sp;
        add_len_set(trick, first_hand);
        np->left_ = p;
        p->pos_search_point_ = NULL;
        p->suit_lengths_ = key;
        p->left_ = NULL;
        p->right_ = NULL;
        result = true;
        return p;
      }
      else
      {
        result = false;
        return NULL;
      }
    }
    else        /* key > suit_lengths_ */
    {
      if (np->right_ != NULL)
        np = np->right_;
      else if (insert_node)
      {
        p = sp;
        add_len_set(trick, first_hand);
        np->right_ = p;
        p->pos_search_point_ = NULL;
        p->suit_lengths_ = key;
        p->left_ = NULL;
        p->right_ = NULL;
        result = true;
        return p;
      }
      else
      {
        result = false;
        return NULL;
      }
    }
  }
}


auto TransTableS::update_sop(
  int u_bound,
  int l_bound,
  char best_move_suit,
  char best_move_rank,
  NodeCards * node_ptr) -> NodeCards * {
  /* Update SOP node with new values for upper and lower
  bounds. */
  if (l_bound > node_ptr->lower_bound)
    node_ptr->lower_bound = static_cast<char>(l_bound);
  if (u_bound < node_ptr->upper_bound)
    node_ptr->upper_bound = static_cast<char>(u_bound);

  node_ptr->best_move_suit = best_move_suit;
  node_ptr->best_move_rank = best_move_rank;

  return node_ptr;
}


auto TransTableS::find_sop(
  const int order_set_[],
  const int limit,
  WinCard * nodeP,
  bool& lowerFlag) -> NodeCards const * {
  WinCard * np;

  np = nodeP;
  int s = 0;

  while (np)
  {
    if ((np->win_mask_ & order_set_[s]) == np->order_set_)
    {
      /* Winning rank set fits position */
      if (s != 3)
      {
        np = np->next_win_;
        s++;
        continue;
      }

      if (np->first_->lower_bound > limit)
      {
        lowerFlag = true;
        return np->first_;
      }
      else if (np->first_->upper_bound <= limit)
      {
        lowerFlag = false;
        return np->first_;
      }
    }

    while (np->next_ == NULL)
    {
      np = np->prev_win_;
      s--;
      if (np == NULL) /* Previous node is header node? */
        return NULL;
    }
    np = np->next_;
  }
  return NULL;
}


auto TransTableS::PrintNodeStats(ofstream& fout) const -> void {
  fout << "Report of generated PosSearch nodes per trick level.\n";
  fout << "Trick level 13 is highest level with all 52 cards.\n";
  fout << string(51, '-') << "\n";

  fout << setw(5) << "Trick" << 
    setw(14) << right << "Created nodes" << "\n";

  for (int k = 13; k > 0; k--)
    fout << setw(5) << k << setw(14) << aggr_len_sets_[k-1] << "\n";

  fout << endl;
}


auto TransTableS::PrintResetStats(ofstream& fout) const -> void {
  fout << "Total no. of resets: " << stats_resets_.no_of_resets << "\n" << endl;

  fout << setw(18) << left << "Reason" << 
    setw(6) << right << "Count" << "\n";

  for (unsigned k = 0; k < kResetReasonCount; k++)
    fout << setw(18) << left << reset_text_[k] <<
      setw(6) << right << stats_resets_.aggr_resets[k] << "\n";
}

