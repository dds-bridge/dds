/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <cassert>

#include "ab_search.hpp"
#include <ab_stats.hpp>
#include <dump.hpp>
#include <later_tricks.hpp>
#include <quick_tricks.hpp>
#include <lookup_tables/lookup_tables.hpp>
#include <solver_context/solver_context.hpp>
#include <system/timer_list.hpp>
#include <trans_table/trans_table.hpp>

// Internal ctx-enabled variants (forward declarations)
static bool ab_search_0_ctx(Pos * posPoint, int target, int depth, SolverContext& ctx);
static bool ab_search_1_ctx(Pos * posPoint, int target, int depth, SolverContext& ctx);
static bool ab_search_2_ctx(Pos * posPoint, int target, int depth, SolverContext& ctx);
static bool ab_search_3_ctx(Pos * posPoint, int target, int depth, SolverContext& ctx);
EvalType evaluate_with_context(Pos const * posPoint, int trump, SolverContext& ctx);

// ctx-enabled helpers to keep search-state access behind the facade
static void make_3_ctx(
  Pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  MoveType const * mply,
  SolverContext& ctx);

static void undo_0_ctx(
  Pos * posPoint,
  const int depth,
  const MoveType& mply,
  SolverContext& ctx);


void make_3_simple(
  Pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  MoveType const * mply,
  SolverContext& ctx);

void undo_0(
  Pos * posPoint,
  const int depth,
  const MoveType& mply,
  const std::shared_ptr<ThreadData>& thrp);

void undo_0_simple(
  Pos * posPoint,
  const int depth,
  const MoveType& mply);

void undo_1(
  Pos * posPoint,
  const int depth,
  const MoveType& mply);

void undo_2(
  Pos * posPoint,
  const int depth,
  const MoveType& mply);

void undo_3(
  Pos * posPoint,
  const int depth,
  const MoveType& mply);


const int handDelta[DDS_SUITS] = { 256, 16, 1, 0 };


bool ab_search(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  /* posPoint points to the current look-ahead position,
     target is number of tricks to take for the player,
     depth is the remaining search length, must be positive,
     the value of the subtree is returned.
     This is a specialized AB function for hand_rel_first == 0. */

  auto thrp = ctx.thread();
  int hand = posPoint->first[depth];
  int tricks = depth >> 2;
  bool success = (ctx.search().node_type_store(hand) == MAXNODE ? true : false);
  bool value = ! success;
#ifdef DDS_TOP_LEVEL
  ctx.search().nodes()++;
#endif

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    ctx.search().lowest_win(depth, ss) = 0;

  ctx.move_gen().move_gen_0(
    tricks,
    * posPoint,
    ctx.search().best_move(depth),
    ctx.search().best_move_tt(depth),
    thrp->rel);
  ctx.move_gen().purge(tricks, 0, ctx.search().forbidden_moves());

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->win_ranks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    MoveType const * mply = ctx.move_gen().make_next(tricks, 0,
      posPoint->win_ranks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

    make_0(posPoint, depth, mply);

    TIMER_START(TIMER_NO_AB, depth - 1);
  value = ab_search_1_ctx(posPoint, target, depth - 1, ctx);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    undo_1(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->win_ranks[depth][ss] =
          posPoint->win_ranks[depth - 1][ss];

  ctx.search().best_move(depth) = * mply;
#ifdef DDS_MOVES
  ctx.move_gen().register_hit(tricks, 0);
#endif
      goto ABexit;
    }
    // Accumulate win_ranks from the explored child to inform subsequent moves
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->win_ranks[depth][ss] |= posPoint->win_ranks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:

  AB_COUNT(AB_MOVE_LOOP, value, depth);
#ifdef DDS_AB_STATS
  thrp->ABStats.PrintStats(thrp->fileABstats.GetStream());
#endif

  return value;
}


bool ab_search_0(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  return ab_search_0_ctx(posPoint, target, depth, ctx);
}

// ctx-enabled implementation
static bool ab_search_0_ctx(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  /* posPoint points to the current look-ahead position,
     target is number of tricks to take for the player,
     depth is the remaining search length, must be positive,
     the value of the subtree is returned.
     This is a specialized AB function for hand_rel_first == 0. */

  auto thrp = ctx.thread();
  int trump = thrp->trump;
  int hand = posPoint->first[depth];
  int tricks = depth >> 2;

#ifdef DDS_TOP_LEVEL
  ctx.search().nodes()++;
#endif

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->win_ranks[depth][ss] = 0;

  if (depth >= 20)
  {
    /* Find node that fits the suit lengths */
    int limit;
    if (ctx.search().node_type_store(0) == MAXNODE)
      limit = target - posPoint->tricks_max - 1;
    else
      limit = tricks - (target - posPoint->tricks_max - 1);

    bool lowerFlag;
    TIMER_START(TIMER_NO_LOOKUP, depth);
  NodeCards const * cardsP =
      ctx.trans_table()->lookup(
        tricks, hand, posPoint->aggr, posPoint->hand_dist,
        limit, lowerFlag);
    TIMER_END(TIMER_NO_LOOKUP, depth);

    if (cardsP)
    {
#ifdef DDS_AB_HITS
      DumpRetrieved(thrp->fileRetrieved.GetStream(), 
        * posPoint, cardsP, target, depth);
#endif

      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->win_ranks[depth][ss] =
          win_ranks[ posPoint->aggr[ss] ]
          [ static_cast<unsigned char>(cardsP->least_win[ss]) ];

      if (cardsP->best_move_rank != 0)
      {
        ctx.search().best_move_tt(depth).suit = static_cast<unsigned char>(cardsP->best_move_suit);
        ctx.search().best_move_tt(depth).rank = static_cast<unsigned char>(cardsP->best_move_rank);
      }

      bool scoreFlag = (ctx.search().node_type_store(0) == MAXNODE ? lowerFlag : ! lowerFlag);

      AB_COUNT(AB_MAIN_LOOKUP, scoreFlag, depth);
      return scoreFlag;
    }
  }

  if (posPoint->tricks_max >= target)
  {
    AB_COUNT(AB_TARGET_REACHED, true, depth);
    return true;
  }
  else if (posPoint->tricks_max + tricks + 1 < target)
  {
    AB_COUNT(AB_TARGET_REACHED, false, depth);
    return false;
  }
  else if (depth == 0) /* Maximum depth? */
  {
    TIMER_START(TIMER_NO_EVALUATE, depth);
    EvalType evalData = evaluate_with_context(posPoint, trump, ctx);
    TIMER_END(TIMER_NO_EVALUATE, depth);

    bool value = (evalData.tricks >= target ? true : false);

    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->win_ranks[depth][ss] = evalData.win_ranks[ss];

    AB_COUNT(AB_DEPTH_ZERO, value, depth);
    return value;
  }

  bool res;
  TIMER_START(TIMER_NO_QT, depth);
  int qtricks = QuickTricks(* posPoint, hand, depth, target,
    trump, res, ctx);
  TIMER_END(TIMER_NO_QT, depth);

  if (ctx.search().node_type_store(hand) == MAXNODE)
  {
    if (res)
    {
      AB_COUNT(AB_QUICKTRICKS, 1, depth);
      return (qtricks == 0 ? false : true);
    }

  TIMER_START(TIMER_NO_LT, depth);
  res = LaterTricksMIN(* posPoint, hand, depth, target, trump, ctx);
  TIMER_END(TIMER_NO_LT, depth);

    if (! res)
    {
      AB_COUNT(AB_LATERTRICKS, true, depth);
      return false;
    }
  }
  else
  {
    if (res)
    {
      AB_COUNT(AB_QUICKTRICKS, false, depth);
      return (qtricks == 0 ? true : false);
    }

  TIMER_START(TIMER_NO_LT, depth);
  res = LaterTricksMAX(* posPoint, hand, depth, target, trump, ctx);
  TIMER_END(TIMER_NO_LT, depth);

    if (res)
    {
      AB_COUNT(AB_LATERTRICKS, false, depth);
      return true;
    }
  }

  if (depth < 20)
  {
    /* Find node that fits the suit lengths */
    int limit;
    if (ctx.search().node_type_store(0) == MAXNODE)
      limit = target - posPoint->tricks_max - 1;
    else
      limit = tricks - (target - posPoint->tricks_max - 1);

    bool lowerFlag;
    TIMER_START(TIMER_NO_LOOKUP, depth);
  NodeCards const * cardsP =
      ctx.trans_table()->lookup(
        tricks, hand, posPoint->aggr, posPoint->hand_dist,
        limit, lowerFlag);
    TIMER_END(TIMER_NO_LOOKUP, depth);

    if (cardsP)
    {
#ifdef DDS_AB_HITS
      DumpRetrieved(thrp->fileRetrieved.GetStream(), 
        * posPoint, * cardsP, target, depth);
#endif

      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->win_ranks[depth][ss] =
          win_ranks[ posPoint->aggr[ss] ]
          [ static_cast<unsigned char>(cardsP->least_win[ss]) ];

      if (cardsP->best_move_rank != 0)
      {
        ctx.search().best_move_tt(depth).suit = static_cast<unsigned char>(cardsP->best_move_suit);
        ctx.search().best_move_tt(depth).rank = static_cast<unsigned char>(cardsP->best_move_rank);
      }

      bool scoreFlag = (ctx.search().node_type_store(0) == MAXNODE ? lowerFlag : ! lowerFlag);

      AB_COUNT(AB_MAIN_LOOKUP, scoreFlag, depth);
      return scoreFlag;
    }
  }

  bool success = (ctx.search().node_type_store(hand) == MAXNODE ? true : false);
  bool value = ! success;

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    ctx.search().lowest_win(depth, ss) = 0;

  ctx.move_gen().move_gen_0(
    tricks,
    * posPoint,
    ctx.search().best_move(depth),
    ctx.search().best_move_tt(depth),
    thrp->rel);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->win_ranks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    MoveType const * mply = ctx.move_gen().make_next(tricks, 0,
      posPoint->win_ranks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

    make_0(posPoint, depth, mply);

    TIMER_START(TIMER_NO_AB, depth - 1);
  value = ab_search_1_ctx(posPoint, target, depth - 1, ctx);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    undo_1(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->win_ranks[depth][ss] =
          posPoint->win_ranks[depth - 1][ss];

      ctx.search().best_move(depth) = * mply;
#ifdef DDS_MOVES
  ctx.move_gen().register_hit(tricks, 0);
#endif
      goto ABexit;
    }
    // Accumulate win_ranks from the explored child to inform subsequent moves
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->win_ranks[depth][ss] |= posPoint->win_ranks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  NodeCards first;
  if (value)
  {
    if (ctx.search().node_type_store(0) == MAXNODE)
    {
      first.upper_bound = static_cast<char>(tricks + 1);
      first.lower_bound = static_cast<char>(target - posPoint->tricks_max);
    }
    else
    {
      first.upper_bound = static_cast<char>
                     (tricks + 1 - target + posPoint->tricks_max);
      first.lower_bound = 0;
    }
  }
  else
  {
    if (ctx.search().node_type_store(0) == MAXNODE)
    {
      first.upper_bound = static_cast<char>
                     (target - posPoint->tricks_max - 1);
      first.lower_bound = 0;
    }
    else
    {
      first.upper_bound = static_cast<char>(tricks + 1);
      first.lower_bound = static_cast<char>
                     (tricks + 1 - target + posPoint->tricks_max + 1);
    }
  }

  first.best_move_suit = static_cast<char>(ctx.search().best_move(depth).suit);
  first.best_move_rank = static_cast<char>(ctx.search().best_move(depth).rank);

  bool flag =
    ((ctx.search().node_type_store(hand) == MAXNODE && value) ||
     (ctx.search().node_type_store(hand) == MINNODE && !value))
    ? true : false;

  TIMER_START(TIMER_NO_BUILD, depth);
  ctx.trans_table()->add(
    tricks,
    hand,
    posPoint->aggr,
    posPoint->win_ranks[depth],
    first,
    flag);
  TIMER_END(TIMER_NO_BUILD, depth);

#ifdef DDS_AB_HITS
  DumpStored(thrp->fileStored.GetStream(), 
    * posPoint, ctx, first, target, depth);
#endif

  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ab_search_1(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  return ab_search_1_ctx(posPoint, target, depth, ctx);
}

static bool ab_search_1_ctx(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  auto thrp = ctx.thread();
  int trump = thrp->trump;
  int hand = HAND_ID(posPoint->first[depth], 1);
  bool success = (ctx.search().node_type_store(hand) == MAXNODE ? true : false);
  bool value = ! success;
  int tricks = (depth + 3) >> 2;

#ifdef DDS_TOP_LEVEL
  ctx.search().nodes()++;
#endif

  TIMER_START(TIMER_NO_QT, depth);
  int res = QuickTricksSecondHand(* posPoint, hand, depth, target, trump, ctx);
  TIMER_END(TIMER_NO_QT, depth);
  if (res) 
  {
    AB_COUNT(AB_QUICKTRICKS_2ND, true, depth);
    return success;
  }

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    ctx.search().lowest_win(depth, ss) = 0;

  ctx.move_gen().move_gen_123(tricks, 1, * posPoint);
  if (depth == ctx.search().ini_depth())
    ctx.move_gen().purge(tricks, 1, ctx.search().forbidden_moves());

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->win_ranks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
  MoveType const * mply = ctx.move_gen().make_next(tricks, 1, posPoint->win_ranks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

    make_1(posPoint, depth, mply);

    TIMER_START(TIMER_NO_AB, depth - 1);
  value = ab_search_2_ctx(posPoint, target, depth - 1, ctx);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    undo_2(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->win_ranks[depth][ss] = posPoint->win_ranks[depth - 1][ss];

      ctx.search().best_move(depth) = * mply;
#ifdef DDS_MOVES
  ctx.move_gen().register_hit(tricks, 1);
#endif
      goto ABexit;
    }

    // Accumulate win_ranks from the explored child to inform subsequent moves
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->win_ranks[depth][ss] |= posPoint->win_ranks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ab_search_2(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  return ab_search_2_ctx(posPoint, target, depth, ctx);
}

static bool ab_search_2_ctx(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  auto thrp = ctx.thread();
  int hand = HAND_ID(posPoint->first[depth], 2);
  bool success = (ctx.search().node_type_store(hand) == MAXNODE ? true : false);
  bool value = ! success;
  int tricks = (depth + 3) >> 2;

#ifdef DDS_TOP_LEVEL
  ctx.search().nodes()++;
#endif

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    ctx.search().lowest_win(depth, ss) = 0;

  ctx.move_gen().move_gen_123(tricks, 2, * posPoint);
  if (depth == ctx.search().ini_depth())
    ctx.move_gen().purge(tricks, 2, ctx.search().forbidden_moves());

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->win_ranks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
  MoveType const * mply = ctx.move_gen().make_next(tricks, 2, posPoint->win_ranks[depth]);

    if (mply == NULL)
      break;

    make_2(posPoint, depth, mply);

#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    TIMER_START(TIMER_NO_AB, depth - 1);
  value = ab_search_3_ctx(posPoint, target, depth - 1, ctx);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    undo_3(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->win_ranks[depth][ss] = posPoint->win_ranks[depth - 1][ss];

      ctx.search().best_move(depth) = * mply;
#ifdef DDS_MOVES
  ctx.move_gen().register_hit(tricks, 2);
#endif
      goto ABexit;
    }

    // Accumulate win_ranks from the explored child to inform subsequent moves
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->win_ranks[depth][ss] |= posPoint->win_ranks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ab_search_3(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  return ab_search_3_ctx(posPoint, target, depth, ctx);
}

static bool ab_search_3_ctx(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx)
{
  /* This is a specialized AB function for hand_rel_first == 3. */

  unsigned short int makeWinRank[DDS_SUITS];

  auto thrp = ctx.thread();
  int hand = HAND_ID(posPoint->first[depth], 3);
  bool success = (ctx.search().node_type_store(hand) == MAXNODE ? true : false);
  bool value = ! success;

#ifdef DDS_TOP_LEVEL
  ctx.search().nodes()++;
#endif

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    ctx.search().lowest_win(depth, ss) = 0;
  int tricks = (depth + 3) >> 2;

  ctx.move_gen().move_gen_123(tricks, 3, * posPoint);
  if (depth == ctx.search().ini_depth())
    ctx.move_gen().purge(tricks, 3, ctx.search().forbidden_moves());

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->win_ranks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
  MoveType const * mply = ctx.move_gen().make_next(tricks, 3, posPoint->win_ranks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

  make_3_ctx(posPoint, makeWinRank, depth, mply, ctx);

    ctx.search().trick_nodes()++; // As hand_rel_first == 0

    if (ctx.search().node_type_store(posPoint->first[depth - 1]) == MAXNODE)
      posPoint->tricks_max++;

  TIMER_START(TIMER_NO_AB, depth - 1);
  value = ab_search_0_ctx(posPoint, target, depth - 1, ctx);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
  undo_0_ctx(posPoint, depth, * mply, ctx);

    if (ctx.search().node_type_store(posPoint->first[depth - 1]) == MAXNODE)
      posPoint->tricks_max--;

    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->win_ranks[depth][ss] = static_cast<unsigned short>(
          posPoint->win_ranks[depth - 1][ss] | makeWinRank[ss]);

      ctx.search().best_move(depth) = * mply;
#ifdef DDS_MOVES
  ctx.move_gen().register_hit(tricks, 3);
#endif
      goto ABexit;
    }
    // Accumulate win_ranks from explored child to inform subsequent moves
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->win_ranks[depth][ss] |= posPoint->win_ranks[depth - 1][ss] | makeWinRank[ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}

void make_0(
  Pos * posPoint,
  const int depth,
  MoveType const * mply)
{
  /* First hand is not changed in next move */
  int h = posPoint->first[depth];
  int s = mply->suit;
  int r = mply->rank;

  posPoint->first[depth - 1] = h;
  posPoint->move[depth] = * mply;

  posPoint->rank_in_suit[h][s] &= (~bit_map_rank[r]);
  posPoint->aggr[s] ^= bit_map_rank[r];
  posPoint->hand_dist[h] -= handDelta[s];
  posPoint->length[h][s]--;
}


void make_1(
  Pos * posPoint,
  const int depth,
  MoveType const * mply)
{
  /* First hand is not changed in next move */
  int firstHand = posPoint->first[depth];
  posPoint->first[depth - 1] = firstHand;

  int h = HAND_ID(firstHand, 1);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rank_in_suit[h][s] &= (~bit_map_rank[r]);
  posPoint->aggr[s] ^= bit_map_rank[r];
  posPoint->hand_dist[h] -= handDelta[s];
  posPoint->length[h][s]--;
}


void make_2(
  Pos * posPoint,
  const int depth,
  MoveType const * mply)
{
  /* First hand is not changed in next move */
  int firstHand = posPoint->first[depth];
  posPoint->first[depth - 1] = firstHand;

  int h = HAND_ID(firstHand, 2);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rank_in_suit[h][s] &= (~bit_map_rank[r]);
  posPoint->aggr[s] ^= bit_map_rank[r];
  posPoint->hand_dist[h] -= handDelta[s];
  posPoint->length[h][s]--;
}


void make_3(
  Pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  MoveType const * mply,
  SolverContext& ctx)
{
  auto thrp = ctx.thread();
  int firstHand = posPoint->first[depth];

  const TrickDataType& data = ctx.move_gen().get_trick_data((depth + 3) >> 2);

  posPoint->first[depth - 1] = HAND_ID(firstHand, data.rel_winner);
  /* Defines who is first in the next move */

  int h = HAND_ID(firstHand, 3);
  /* Hand pointed to by posPoint->first will lead the next trick */

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int ss = data.best_suit;
  if (data.play_count[ss] >= 2)
  {
    // Win by rank when some else played that suit, too.
    int rr = data.best_rank;
    trickCards[ss] = static_cast<unsigned short>
      (bit_map_rank[rr] | data.best_sequence);
  }

  int r = mply->rank;
  int s = mply->suit;
  posPoint->rank_in_suit[h][s] &= (~bit_map_rank[r]);
  posPoint->aggr[s] ^= bit_map_rank[r];
  posPoint->hand_dist[h] -= handDelta[s];
  posPoint->length[h][s]--;

  // Changes that we may have to undo.
  WinnersType * wp = &thrp->winners[ (depth + 3) >> 2];
  wp->number = 0;

  for (int st = 0; st < 4; st++)
  {
    if (data.play_count[st])
    {
      int n = wp->number;
      wp->winner[n].suit = st;
      wp->winner[n].winnerRank = posPoint->winner[st].rank;
      wp->winner[n].winnerHand = posPoint->winner[st].hand;
      wp->winner[n].secondRank = posPoint->second_best[st].rank;
      wp->winner[n].secondHand = posPoint->second_best[st].hand;
      wp->number++;

      int aggr = posPoint->aggr[st];

  posPoint->winner[st].rank = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[1][st].rank);
  posPoint->winner[st].hand = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[1][st].hand);
  posPoint->second_best[st].rank = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[2][st].rank);
  posPoint->second_best[st].hand = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[2][st].hand);

    }
  }
}


// ctx-enabled version that records winners via the SearchContext facade
static void make_3_ctx(
  Pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  MoveType const * mply,
  SolverContext& ctx)
{
  auto thrp = ctx.thread();
  int firstHand = posPoint->first[depth];

  const TrickDataType& data = ctx.move_gen().get_trick_data((depth + 3) >> 2);

  posPoint->first[depth - 1] = HAND_ID(firstHand, data.rel_winner);
  /* Defines who is first in the next move */

  int h = HAND_ID(firstHand, 3);
  /* Hand pointed to by posPoint->first will lead the next trick */

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int ss = data.best_suit;
  if (data.play_count[ss] >= 2)
  {
    // Win by rank when some else played that suit, too.
    int rr = data.best_rank;
    trickCards[ss] = static_cast<unsigned short>
      (bit_map_rank[rr] | data.best_sequence);
  }

  int r = mply->rank;
  int s = mply->suit;
  posPoint->rank_in_suit[h][s] &= (~bit_map_rank[r]);
  posPoint->aggr[s] ^= bit_map_rank[r];
  posPoint->hand_dist[h] -= handDelta[s];
  posPoint->length[h][s]--;

  // Changes that we may have to undo.
  WinnersType * wp = &ctx.search().winners((depth + 3) >> 2);
  wp->number = 0;

  for (int st = 0; st < 4; st++)
  {
    if (data.play_count[st])
    {
      int n = wp->number;
      wp->winner[n].suit = st;
      wp->winner[n].winnerRank = posPoint->winner[st].rank;
      wp->winner[n].winnerHand = posPoint->winner[st].hand;
      wp->winner[n].secondRank = posPoint->second_best[st].rank;
      wp->winner[n].secondHand = posPoint->second_best[st].hand;
      wp->number++;

      int aggr = posPoint->aggr[st];

      posPoint->winner[st].rank = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[1][st].rank);
      posPoint->winner[st].hand = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[1][st].hand);
      posPoint->second_best[st].rank = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[2][st].rank);
      posPoint->second_best[st].hand = static_cast<unsigned char>(thrp->rel[aggr].abs_rank[2][st].hand);

    }
  }
}


void make_3_simple(
  Pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  MoveType const * mply,
  SolverContext& ctx)
{
  const TrickDataType& data = ctx.move_gen().get_trick_data((depth + 3) >> 2);

  int firstHand = posPoint->first[depth];

  // Leader of next trick
  posPoint->first[depth - 1] = HAND_ID(firstHand, data.rel_winner);

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int s = data.best_suit;
  if (data.play_count[s] >= 2)
  {
    // Win by rank when some else played that suit, too.
    int r = data.best_rank;
    trickCards[s] = static_cast<unsigned short>
      (bit_map_rank[r] | data.best_sequence);
  }

  int h = HAND_ID(firstHand, 3);
  int r = mply->rank;
  s = mply->suit;

  posPoint->aggr[s] ^= bit_map_rank[r];
  posPoint->hand_dist[h] -= handDelta[s];
}


void undo_0(
  Pos * posPoint,
  const int depth,
  const MoveType& mply,
  const std::shared_ptr<ThreadData>& thrp)
{
  int h = HAND_ID(posPoint->first[depth], 3);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rank_in_suit[h][s] |= bit_map_rank[r];
  posPoint->aggr[s] |= bit_map_rank[r];
  posPoint->hand_dist[h] += handDelta[s];
  posPoint->length[h][s]++;

  // Changes that we now undo.
  WinnersType const * wp = &thrp->winners[ (depth + 3) >> 2];

  for (int n = 0; n < wp->number; n++)
  {
    int st = wp->winner[n].suit;
    posPoint->winner[st].rank = wp->winner[n].winnerRank;
    posPoint->winner[st].hand = wp->winner[n].winnerHand;
    posPoint->second_best[st].rank = wp->winner[n].secondRank;
    posPoint->second_best[st].hand = wp->winner[n].secondHand;
  }
}

// ctx-enabled version that reads winners via the SearchContext facade
static void undo_0_ctx(
  Pos * posPoint,
  const int depth,
  const MoveType& mply,
  SolverContext& ctx)
{
  // No timers here; macros not used in this helper
  int h = HAND_ID(posPoint->first[depth], 3);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rank_in_suit[h][s] |= bit_map_rank[r];
  posPoint->aggr[s] |= bit_map_rank[r];
  posPoint->hand_dist[h] += handDelta[s];
  posPoint->length[h][s]++;

  // Changes that we now undo.
  WinnersType const * wp = &ctx.search().winners((depth + 3) >> 2);

  for (int n = 0; n < wp->number; n++)
  {
    int st = wp->winner[n].suit;
    posPoint->winner[st].rank = wp->winner[n].winnerRank;
    posPoint->winner[st].hand = wp->winner[n].winnerHand;
    posPoint->second_best[st].rank = wp->winner[n].secondRank;
    posPoint->second_best[st].hand = wp->winner[n].secondHand;
  }
}


void undo_0_simple(
  Pos * posPoint,
  const int depth,
  const MoveType& mply)
{
  int h = HAND_ID(posPoint->first[depth], 3);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->aggr[s] |= bit_map_rank[r];
  posPoint->hand_dist[h] += handDelta[s];
}


void undo_1(
  Pos * posPoint,
  const int depth,
  const MoveType& mply)
{
  int h = posPoint->first[depth];
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rank_in_suit[h][s] |= bit_map_rank[r];
  posPoint->aggr[s] |= bit_map_rank[r];
  posPoint->hand_dist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


void undo_2(
  Pos * posPoint,
  const int depth,
  const MoveType& mply)
{
  int h = HAND_ID(posPoint->first[depth], 1);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rank_in_suit[h][s] |= bit_map_rank[r];
  posPoint->aggr[s] |= bit_map_rank[r];
  posPoint->hand_dist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


void undo_3(
  Pos * posPoint,
  const int depth,
  const MoveType& mply)
{
  int h = HAND_ID(posPoint->first[depth], 2);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rank_in_suit[h][s] |= bit_map_rank[r];
  posPoint->aggr[s] |= bit_map_rank[r];
  posPoint->hand_dist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


EvalType evaluate_with_context(
  Pos const * posPoint,
  const int trump,
  SolverContext& ctx)
{
  auto thrp = ctx.thread();
  int s, h, hmax = 0, count = 0, k = 0;
  unsigned short rmax = 0;
  EvalType eval;

  int firstHand = posPoint->first[0];
  assert((firstHand >= 0) && (firstHand <= 3));

  for (s = 0; s < DDS_SUITS; s++)
    eval.win_ranks[s] = 0;

  /* Who wins the last trick? */
  if (trump != DDS_NOTRUMP) /* Highest trump card wins */
  {
    for (h = 0; h < DDS_HANDS; h++)
    {
      if (posPoint->rank_in_suit[h][trump] != 0)
        count++;
      if (posPoint->rank_in_suit[h][trump] > rmax)
      {
        hmax = h;
        rmax = posPoint->rank_in_suit[h][trump];
      }
    }

    if (rmax > 0) /* Trumpcard wins */
    {
      if (count >= 2)
        eval.win_ranks[trump] = rmax;

      if (ctx.search().node_type_store(hmax) == MAXNODE)
        goto maxexit;
      else
        goto minexit;
    }
  }

  /* Who has the highest card in the suit played by 1st hand? */

  k = 0;
  while (k <= 3) /* Find the card the 1st hand played */
  {
    if (posPoint->rank_in_suit[firstHand][k] != 0) /* Is this the card? */
      break;
    k++;
  }

  assert(k < 4);

  for (h = 0; h < DDS_HANDS; h++)
  {
    if (posPoint->rank_in_suit[h][k] != 0)
      count++;
    if (posPoint->rank_in_suit[h][k] > rmax)
    {
      hmax = h;
      rmax = posPoint->rank_in_suit[h][k];
    }
  }

  if (count >= 2)
    eval.win_ranks[k] = rmax;

  if (ctx.search().node_type_store(hmax) == MAXNODE)
    goto maxexit;
  else
    goto minexit;

maxexit:
  eval.tricks = posPoint->tricks_max + 1;
  return eval;

minexit:
  eval.tricks = posPoint->tricks_max;
  return eval;
}

