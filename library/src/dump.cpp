/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "dump.hpp"
#include <solver_context/solver_context.hpp>
#include <trans_table/trans_table.hpp>


std::string PrintSuit(const unsigned short suitCode);
std::string PrintSuit(
  const unsigned short suitCode,
  const char leastWin);

std::string PrintDeal(
  const unsigned short ranks[][DDS_SUITS],
  const int spacing);

std::string RankToDiagrams(
  const unsigned short ranks[DDS_HANDS][DDS_SUITS],
  const NodeCards& node);

std::string WinnersToText(const unsigned short win_ranks[]);

std::string NodeToText(const NodeCards& node);

std::string FullNodeToText(const NodeCards& node);

std::string PosToText(
  const Pos& tpos,
  const int target,
  const int depth);

std::string TopMove(
  const bool val,
  const MoveType& bestMove);

std::string DumpTopHeader(
  const std::shared_ptr<ThreadData>& thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode);


std::string PrintSuit(const unsigned short suitCode)
{
  if (! suitCode)
    return "--";

  std::string st;
  for (int r = 14; r >= 2; r--)
    if ((suitCode & bit_map_rank[r]))
      st += static_cast<char>(card_rank[r]);
  return st;
}


std::string PrintSuit(
  const unsigned short suitCode,
  const char leastWin)
{
  if (! suitCode)
    return "--";

  std::string st;
  for (int r = 14; r >= 2; r--)
  {
    if ((suitCode & bit_map_rank[r]))
    {
      if (r >= 15 - leastWin)
        st += static_cast<char>(card_rank[r]);
      else
        st += "x";
    }
  }
  return st;
}


std::string PrintDeal(
  const unsigned short ranks[][DDS_SUITS],
  const int spacing)
{
  std::stringstream ss;
  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << std::setw(spacing) << "" << 
      card_suit[s] << " " <<
      PrintSuit(ranks[0][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << card_suit[s] << " " <<
      std::setw(2*spacing - 2) << std::left << PrintSuit(ranks[3][s]) <<
      card_suit[s] << " " <<
      PrintSuit(ranks[1][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << std::setw(spacing) << "" << 
      card_suit[s] << " " <<
      PrintSuit(ranks[2][s]) << "\n";
  }

  return ss.str() + "\n";
}


std::string RankToDiagrams(
  const unsigned short ranks[DDS_HANDS][DDS_SUITS],
  const NodeCards& node)
{
  std::stringstream ss;
  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << std::setw(12) << std::left << 
      (s == 0 ? "Sought" : "") << 
      card_suit[s] << " " << std::setw(20) << PrintSuit(ranks[0][s]) << "|    " <<
      std::setw(12) << (s == 0 ? "Found" : "") << 
      card_suit[s] << " " << 
        PrintSuit(ranks[0][s], node.least_win[s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << 
      card_suit[s] << " " << std::setw(22) << std::left << PrintSuit(ranks[3][s]) <<
      card_suit[s] << " " << std::setw(8) << PrintSuit(ranks[1][s]) << "|    " << 
      card_suit[s] << " " << 
        std::setw(22) << PrintSuit(ranks[3][s], node.least_win[s]) << 
      card_suit[s] << " " << 
        PrintSuit(ranks[1][s], node.least_win[s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << std::setw(12) << std::left << "" << 
      card_suit[s] << " " << std::setw(20) << PrintSuit(ranks[0][s]) << "|    " <<
      std::setw(12) << "" << card_suit[s] << " " <<
      PrintSuit(ranks[0][s], node.least_win[s]) << "\n";
  }
  return ss.str();
}


std::string WinnersToText(const unsigned short ourWinRanks[])
{
  std::stringstream ss;
  for (int s = 0; s < DDS_SUITS; s++)
    ss << card_suit[s] << " " << PrintSuit(ourWinRanks[s]) << "\n";

  return ss.str();
}


std::string NodeToText(const NodeCards& node)
{
  std::stringstream ss;
  ss << std::setw(16) << std::left << "Address" << 
    static_cast<void const *>(&node) << "\n";

  ss << std::setw(16) << std::left << "Bounds" << 
    static_cast<int>(node.lower_bound) << " to " <<
    static_cast<int>(node.upper_bound) << " tricks\n";

  ss << std::setw(16) << std::left << "Best move" << 
    card_suit[ static_cast<int>(node.best_move_suit) ] <<
    card_rank[ static_cast<int>(node.best_move_rank) ] << "\n";

  return ss.str();
}


std::string FullNodeToText(const NodeCards& node)

{
  std::stringstream ss;
  std::vector<int> v(DDS_SUITS);
  for (unsigned i = 0; i < DDS_SUITS; i++)
    v[i] = 15 - static_cast<int>(node.least_win[i]);

  ss << std::setw(16) << std::left << "Lowest used" << 
    card_suit[0] << card_rank[v[0]] << ", " <<
    card_suit[1] << card_rank[v[1]] << ", " <<
    card_suit[2] << card_rank[v[2]] << ", " <<
    card_suit[3] << card_rank[v[3]] << "\n";

  return NodeToText(node) + ss.str();
}


std::string PosToText(
  const Pos& tpos,
  const int target,
  const int depth)
{
  std::stringstream ss;
  ss << std::setw(16) << std::left << "Target" << target << "\n";
  ss << std::setw(16) << "Depth" << depth << "\n";
  ss << std::setw(16) << "tricks_max" << tpos.tricks_max << "\n";
  ss << std::setw(16) << "First hand" << card_hand[tpos.first[depth]] << "\n";
  ss << std::setw(16) << "Next first" << card_hand[tpos.first[depth - 1]] << "\n";
  return ss.str();
}


std::string DumpTopHeader(
  const std::shared_ptr<ThreadData>& thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode)
{
  // Use facade to read search-state safely (caller provides shared_ptr)
  SolverContext ctx{ thrp };
  std::string stext;
  if (printMode == 0)
  {
    // Trying just one target.
    stext = "Single target " + std::to_string(tricks) + ", " + "achieved";
  }
  else if (printMode == 1)
  {
    // Looking for best score.
    stext = "Loop target " + std::to_string(tricks) + ", " +
      "bounds " + std::to_string(lower) + " .. " + std::to_string(upper) + ", " +
    TopMove(thrp->val, ctx.search().best_move(ctx.search().ini_depth())) + "";
  }
  else if (printMode == 2)
  {
    // Looking for other moves with best score.
    stext = "Loop for cards with score " + std::to_string(tricks) + ", " +
      TopMove(thrp->val, ctx.search().best_move(ctx.search().ini_depth()));
  }
  return stext + "\n" + std::string(stext.size(), '-') + "\n";
}


std::string TopMove(
  const bool val,
  const MoveType& bestMove)
{
  if (val)
  {
    std::stringstream ss;
    ss << "achieved with move " <<
      card_suit[ bestMove.suit ] <<
      card_rank[ bestMove.rank ];
    return ss.str();
  }
  else
    return "failed";
}


int DumpInput(
  const int errCode, 
  const Deal& dl, 
  const int target,
  const int solutions, 
  const int mode)
{
#ifndef DDS_NO_DUMP_ON_ERROR
  std::ofstream fout;
  fout.open("dump.txt");

  fout << "Error code=" << errCode << "\n\n";
  fout << "Deal data:\n";
  fout << "trump=";

  if (dl.trump == DDS_NOTRUMP)
    fout << "N\n";
  else
    fout << card_suit[dl.trump] << "\n";
  fout << "first=" << card_hand[dl.first] << "\n";

  unsigned short ranks[4][4];

  for (int k = 0; k <= 2; k++)
    if (dl.currentTrickRank[k] != 0)
    {
      fout << "index=" << k << 
        " currentTrickSuit=" << card_suit[dl.currentTrickSuit[k]] <<
        " currentTrickRank= " << card_rank[dl.currentTrickRank[k]] << "\n";
    }

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
    {
      fout << "index1=" << h << " index2=" << s <<
        " remainCards=" << dl.remainCards[h][s] << "\n";
      ranks[h][s] = static_cast<unsigned short>
                    (dl.remainCards[h][s] >> 2);
    }

  fout << "\ntarget=" << target << "\n";
  fout << "solutions=" << solutions << "\n";
  fout << "mode=" << mode << "\n\n\n";
  fout << PrintDeal(ranks, 8);

  fout.close();
#endif
  return 0;
}


void DumpTopLevel(
  std::ofstream& fout,
  const std::shared_ptr<ThreadData>& thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode)
{
  const Pos& tpos = thrp->lookAheadPos;
  SolverContext ctx{ thrp };

  fout << DumpTopHeader(thrp, tricks, lower, upper, printMode) << "\n";
  fout << PrintDeal(tpos.rank_in_suit, 16);
  fout << WinnersToText(tpos.win_ranks[ctx.search().ini_depth()]) << "\n";
  fout << ctx.search().nodes() << " AB nodes, " <<
    ctx.search().trick_nodes() << " trick nodes\n\n";
}

