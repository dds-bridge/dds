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

#include "TransTableS.h"

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
    aggp_[0].aggrRanks[s] = 0;
    aggp_[0].winMask[s] = 0;
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
      aggp_[ind].aggrRanks[s] =
        (aggp_[ind].aggrRanks[s] >> 2) |
        (handLookup[s][topBitNo] << 24);

      aggp_[ind].winMask[s] =
        (aggp_[ind].winMask[s] >> 2) | (3 << 24);
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

  summem_ = (1ULL * (WINIT + 1) * sizeof(winCardType)) +
    (1ULL * (NINIT + 1) * sizeof(NodeCards)) +
       (1ULL * (LSIZE + 1) * 52 * sizeof(posSearchTypeSmall));
  wmem_ = static_cast<int>(1ULL * (WSIZE + 1) * sizeof(winCardType));
  nmem_ = static_cast<int>(1ULL * (NSIZE + 1) * sizeof(NodeCards));

    // Compute how many additional slabs we could potentially allocate.
    // Guard against negative values if maxmem_ < summem_ (which can happen
    // with very small configured limits).
    if (maxmem_ <= summem_)
      max_index_ = 0;
    else
    {
      const unsigned long long denom =
        static_cast<unsigned long long>(1ULL * (WSIZE + 1) * sizeof(winCardType));
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

    pw_ = static_cast<winCardType **>(calloc(static_cast<unsigned int>(max_index_ + 1), sizeof(winCardType *)));
    if (pw_ == NULL)
      exit(1);

  pn_ = static_cast<NodeCards **>(calloc(static_cast<unsigned int>(max_index_ + 1), sizeof(NodeCards *)));
    if (pn_ == NULL)
      exit(1);

    for (int k = 1; k <= 13; k++)
      for (int h = 0; h < DDS_HANDS; h++)
      {
        pl_[k][h] = static_cast
                   <posSearchTypeSmall **>(calloc(static_cast<unsigned int>(max_index_ + 1),
                                             sizeof(posSearchTypeSmall *)));
        if (pl_[k][h] == NULL)
          exit(1);
      }

    pw_[0] = static_cast<winCardType *>(calloc(WINIT + 1, sizeof(winCardType)));
    if (pw_[0] == NULL)
      exit(1);

  pn_[0] = static_cast<NodeCards *>(calloc(NINIT + 1, sizeof(NodeCards)));
    if (pn_[0] == NULL)
      exit(1);

    for (int k = 1; k <= 13; k++)
      for (int h = 0; h < DDS_HANDS; h++)
      {
        pl_[k][h][0] = static_cast<posSearchTypeSmall *>(calloc((LSIZE + 1),
                      sizeof(posSearchTypeSmall)));
        if (pl_[k][h][0] == NULL)
          exit(1);
      }

    aggp_ = static_cast<ttAggrType *>(calloc(8192, sizeof(ttAggrType)));
    if (aggp_ == NULL)
      exit(1);

    InitTT();

    for (int k = 1; k <= 13; k++)
      aggr_len_sets_[k] = 0;
    stats_resets_.noOfResets = 0;
    for (int k = 0; k <= 5; k++)
      stats_resets_.aggrResets[k] = 0;

  }

  return;
}


auto TransTableS::Wipe() -> void {
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



auto TransTableS::InitTT() -> void {
  win_set_size_limit_ = WINIT;
  node_set_size_limit_ = NINIT;
  allocmem_ = (WINIT + 1) * sizeof(winCardType);
  allocmem_ += 1ULL * (NINIT + 1) * sizeof(NodeCards);
  allocmem_ += 1ULL * (LSIZE + 1) * 52 * sizeof(posSearchTypeSmall);
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
      pos_search_[k][h][0].posSearchPoint = NULL;
      pos_search_[k][h][0].left = NULL;
      pos_search_[k][h][0].right = NULL;
      rootnp_[k][h] = &(pos_search_[k][h][0]);
    }
}


auto TransTableS::reset_memory([[maybe_unused]] const ResetReason reason) -> void {
  Wipe();

  InitTT();

  for (int k = 1; k <= 13; k++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      rootnp_[k][h] = &(pos_search_[k][h][0]);
      pos_search_[k][h][0].suit_lengths_ = 0;
      pos_search_[k][h][0].posSearchPoint = NULL;
      pos_search_[k][h][0].left = NULL;
      pos_search_[k][h][0].right = NULL;

      len_set_ind_[k][h] = 1;
    }
  }

#if defined(DDS_TT_STATS)
  stats_resets_.noOfResets++;
  stats_resets_.aggrResets[static_cast<int>(reason)]++;
#endif

  return;
}

auto TransTableS::return_all_memory() -> void {

  if (!tt_in_use_)
    return;
  tt_in_use_ = 0;

  Wipe();

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
  int aggrMem = 8192 * static_cast<int>(sizeof(ttAggrType));
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
  posSearchTypeSmall * pp;
  int orderSet[DDS_SUITS];
  NodeCards const * cardsP;

  suit_lengths_[trick] =
    (static_cast<long long>(handDist[0]) << 36) |
    (static_cast<long long>(handDist[1]) << 24) |
    (static_cast<long long>(handDist[2]) << 12) |
    (static_cast<long long>(handDist[3]));

  pp = SearchLenAndInsert(rootnp_[trick][hand],
    suit_lengths_[trick], false, trick, hand, res);

  /* Find node that fits the suit lengths */
  if ((pp != NULL) && res)
  {
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      orderSet[ss] =
        aggp_[aggrTarget[ss]].aggrRanks[ss];
    }

    if (pp->posSearchPoint == NULL)
      cardsP = NULL;
    else
    {
      cardsP = FindSOP(orderSet, limit, pp->posSearchPoint, lowerFlag);

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
  const unsigned short ourWinRanks[],
  const NodeCards& first,
  const bool flag) -> void {
  BuildSOP(ourWinRanks, aggrTarget, first, suit_lengths_[tricks],
           tricks, hand, flag);

  if (clear_tt_flag_)
    reset_memory(ResetReason::MemoryExhausted);

  return;
}


auto TransTableS::AddWinSet() -> void {
  if (clear_tt_flag_)
  {
    windex_++;
    win_set_size_ = windex_;
    win_cards_ = &(temp_win[windex_]);
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
      win_cards_ = &(temp_win[windex_]);
    }
    else
    {
      wcount_++;
      win_set_size_limit_ = WSIZE;
      pw_[wcount_] =
        static_cast<winCardType *>(malloc((WSIZE + 1) * sizeof(winCardType)));
      if (pw_[wcount_] == NULL)
      {
        clear_tt_flag_ = true;
        windex_++;
        win_set_size_ = windex_;
        win_cards_ = &(temp_win[windex_]);
      }
      else
      {
        allocmem_ += (WSIZE + 1) * sizeof(winCardType);
        win_set_size_ = 0;
        win_cards_ = pw_[wcount_];
      }
    }
  }
  else
    win_set_size_++;
  return;
}

auto TransTableS::AddNodeSet() -> void {
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

auto TransTableS::AddLenSet(
  const int trick, 
  const int firstHand) -> void {
  if (len_set_ind_[trick][firstHand] < LSIZE)
  {
    len_set_ind_[trick][firstHand]++;
#if defined(DDS_TT_STATS)
    aggr_len_sets_[trick]++;
#endif
    return;
  }

  // The memory chunk for the posSearchTypeSmall structure 
  // will be exceeded.

  const int incr = (LSIZE+1) * sizeof(posSearchTypeSmall);

  if ((allocmem_ + incr > maxmem_) || (lcount_[trick][firstHand] >= max_index_))
  {
    // Already allocated memory plus needed allocation overshot maxmem_.
    clear_tt_flag_ = true;
    return;
  }

  // Obtain another memory chunk LSIZE.

  lcount_[trick][firstHand]++;

  pl_[trick][firstHand][lcount_[trick][firstHand]] =
    static_cast<posSearchTypeSmall *>(malloc(incr));

  if (pl_[trick][firstHand][lcount_[trick][firstHand]] == NULL)
  {
    clear_tt_flag_ = true;
    return;
  }

  allocmem_ += incr;
  len_set_ind_[trick][firstHand] = 0;
  pos_search_[trick][firstHand] =
    pl_[trick][firstHand][lcount_[trick][firstHand]];
#if defined(DDS_TT_STATS)
  aggr_len_sets_[trick]++;
#endif
}


auto TransTableS::BuildSOP(
  const unsigned short ourWinRanks[DDS_SUITS],
  const unsigned short aggrArg[DDS_SUITS],
  const NodeCards& first,
  const long long lengths,
  const int tricks,
  const int firstHand,
  const bool flag) -> void {
  int winMask[DDS_SUITS];
  int winOrderSet[DDS_SUITS];
  char low[DDS_SUITS];

  for (int ss = 0; ss < DDS_SUITS; ss++)
  {
    int w = ourWinRanks[ss];
    if (w == 0)
    {
      winMask[ss] = 0;
      winOrderSet[ss] = 0;
      low[ss] = 15;
    }
    else
    {
      w = w & (-w);       /* Only lowest win */
      const unsigned short temp = 
        static_cast<unsigned short>(aggrArg[ss] & (-w));

      winMask[ss] = aggp_[temp].winMask[ss];
      winOrderSet[ss] = aggp_[temp].aggrRanks[ss];
    low[ss] = static_cast<char>(TTLowestRankTable()[static_cast<size_t>(temp)]);
    }
  }

  bool res;
  posSearchTypeSmall * np = SearchLenAndInsert(
    rootnp_[tricks][firstHand], lengths, true, tricks, firstHand, res);

  NodeCards * cardsP = BuildPath(
    winMask, 
    winOrderSet,
    static_cast<int>(first.ubound), 
    static_cast<int>(first.lbound),
    static_cast<char>(first.bestMoveSuit), 
    static_cast<char>(first.bestMoveRank),
    np, 
    res);

  if (res)
  {
    cardsP->ubound = static_cast<char>(first.ubound);
    cardsP->lbound = static_cast<char>(first.lbound);

    if (flag)
    {
      cardsP->bestMoveSuit = static_cast<char>(first.bestMoveSuit);
      cardsP->bestMoveRank = static_cast<char>(first.bestMoveRank);
    }
    else
    {
      cardsP->bestMoveSuit = 0;
      cardsP->bestMoveRank = 0;
    }

    for (int k = 0; k < DDS_SUITS; k++)
  cardsP->leastWin[k] = static_cast<char>(15 - low[k]);
  }
}


NodeCards * TransTableS::BuildPath(
  const int winMask[],
  const int winOrderSet[],
  const int ubound,
  const int lbound,
  const char bestMoveSuit,
  const char bestMoveRank,
  posSearchTypeSmall * nodep,
  bool& result)
{
  /* If result is TRUE, a new SOP has been created and BuildPath returns a
  pointer to it. If result is FALSE, an existing SOP is used and BuildPath
  returns a pointer to the SOP */

  bool found;
  winCardType * np, *p2, *nprev;
  NodeCards *p;

  np = nodep->posSearchPoint;
  nprev = NULL;
  int suit = 0;

  /* If winning node has a card that equals the next winning card deduced
  from the position, then there already exists a (partial) path */

  if (np == NULL)
  {
    /* There is no winning list created yet */
    /* Create winning nodes */
    p2 = &(win_cards_[win_set_size_]);
    AddWinSet();
    p2->next = NULL;
    p2->nextWin = NULL;
    p2->prevWin = NULL;
    nodep->posSearchPoint = p2;
    p2->winMask = winMask[suit];
    p2->orderSet = winOrderSet[suit];
    p2->first = NULL;
    np = p2;           /* Latest winning node */
    suit++;
    while (suit < DDS_SUITS)
    {
      p2 = &(win_cards_[win_set_size_]);
      AddWinSet();
      np->nextWin = p2;
      p2->prevWin = np;
      p2->next = NULL;
      p2->nextWin = NULL;
      p2->winMask = winMask[suit];
      p2->orderSet = winOrderSet[suit];
      p2->first = NULL;
      np = p2;         /* Latest winning node */
      suit++;
    }
    p = &(node_cards_[node_set_size_]);
    AddNodeSet();
    np->first = p;
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
        if ((np->winMask == winMask[suit]) &&
            (np->orderSet == winOrderSet[suit]))
        {
          /* Part of path found */
          found = true;
          nprev = np;
          break;
        }
        if (np->next != NULL)
          np = np->next;
        else
          break;
      }
      if (found)
      {
        suit++;
        if (suit >= DDS_SUITS)
        {
          result = false;
          return UpdateSOP(ubound, lbound, bestMoveSuit, bestMoveRank,
            np->first);
        }
        else
        {
          np = np->nextWin;       /* Find next winning node  */
          continue;
        }
      }
      else
        break;                    /* Node was not found */
    }               /* End outer while */

    /* Create additional node, coupled to existing node(s) */
    p2 = &(win_cards_[win_set_size_]);
    AddWinSet();
    p2->prevWin = nprev;
    if (nprev != NULL)
    {
      p2->next = nprev->nextWin;
      nprev->nextWin = p2;
    }
    else
    {
      p2->next = nodep->posSearchPoint;
      nodep->posSearchPoint = p2;
    }
    p2->nextWin = NULL;
    p2->winMask = winMask[suit];
    p2->orderSet = winOrderSet[suit];
    p2->first = NULL;
    np = p2;          /* Latest winning node */
    suit++;

    /* Rest of path must be created */
    while (suit < 4)
    {
      p2 = &(win_cards_[win_set_size_]);
      AddWinSet();
      np->nextWin = p2;
      p2->prevWin = np;
      p2->next = NULL;
      p2->winMask = winMask[suit];
      p2->orderSet = winOrderSet[suit];
      p2->first = NULL;
      p2->nextWin = NULL;
      np = p2;         /* Latest winning node */
      suit++;
    }

    /* All winning nodes in SOP have been traversed and new nodes created */
    p = &(node_cards_[node_set_size_]);
    AddNodeSet();
    np->first = p;
    result = true;
    return p;
  }
}

TransTableS::posSearchTypeSmall * TransTableS::SearchLenAndInsert(
  posSearchTypeSmall * rootp,
  const long long key,
  const bool insertNode,
  const int trick,
  const int firstHand,
  bool& result)
{
  /* Search for node which matches with the suit length combination
  given by parameter key. If no such node is found, NULL is
  returned if parameter insertNode is FALSE, otherwise a new
  node is inserted with suit_lengths_ set to key, the pointer to
  this node is returned.
  The algorithm used is defined in Knuth "The art of computer
  programming", vol.3 "Sorting and searching", 6.2.2 Algorithm T,
  page 424. */

  posSearchTypeSmall * np, *p, *sp;

  sp = NULL;
  if (insertNode)
    sp = &(pos_search_[trick][firstHand][len_set_ind_[trick][firstHand]]);

  np = rootp;
  while (1)
  {
    if (key == np->suit_lengths_)
    {
      result = true;
      return np;
    }
    else if (key < np->suit_lengths_)
    {
      if (np->left != NULL)
        np = np->left;
      else if (insertNode)
      {
        p = sp;
        AddLenSet(trick, firstHand);
        np->left = p;
        p->posSearchPoint = NULL;
        p->suit_lengths_ = key;
        p->left = NULL;
        p->right = NULL;
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
      if (np->right != NULL)
        np = np->right;
      else if (insertNode)
      {
        p = sp;
        AddLenSet(trick, firstHand);
        np->right = p;
        p->posSearchPoint = NULL;
        p->suit_lengths_ = key;
        p->left = NULL;
        p->right = NULL;
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


NodeCards * TransTableS::UpdateSOP(
  int ubound,
  int lbound,
  char bestMoveSuit,
  char bestMoveRank,
  NodeCards * nodep)
{
  /* Update SOP node with new values for upper and lower
  bounds. */
  if (lbound > nodep->lbound)
    nodep->lbound = static_cast<char>(lbound);
  if (ubound < nodep->ubound)
    nodep->ubound = static_cast<char>(ubound);

  nodep->bestMoveSuit = bestMoveSuit;
  nodep->bestMoveRank = bestMoveRank;

  return nodep;
}


NodeCards const * TransTableS::FindSOP(
  const int orderSet[],
  const int limit,
  winCardType * nodeP,
  bool& lowerFlag)
{
  winCardType * np;

  np = nodeP;
  int s = 0;

  while (np)
  {
    if ((np->winMask & orderSet[s]) == np->orderSet)
    {
      /* Winning rank set fits position */
      if (s != 3)
      {
        np = np->nextWin;
        s++;
        continue;
      }

      if (np->first->lbound > limit)
      {
        lowerFlag = true;
        return np->first;
      }
      else if (np->first->ubound <= limit)
      {
        lowerFlag = false;
        return np->first;
      }
    }

    while (np->next == NULL)
    {
      np = np->prevWin;
      s--;
      if (np == NULL) /* Previous node is header node? */
        return NULL;
    }
    np = np->next;
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
  fout << "Total no. of resets: " << stats_resets_.noOfResets << "\n" << endl;

  fout << setw(18) << left << "Reason" << 
    setw(6) << right << "Count" << "\n";

  for (unsigned k = 0; k < kResetReasonCount; k++)
    fout << setw(18) << left << reset_text_[k] <<
      setw(6) << right << stats_resets_.aggrResets[k] << "\n";
}

