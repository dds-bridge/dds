/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include "dds.h"
#include "TransTable.h"
#include "Moves.h"
#include "Memory.h"
#include "dump.h"


// #define DDS_HAND_LINES 12
// #define DDS_FULL_LINE 80


string PrintSuit(const unsigned short suitCode);
string PrintSuit(
  const unsigned short suitCode,
  const char leastWin);

string PrintDeal(
  const unsigned short ranks[][DDS_SUITS],
  const unsigned spacing);

void RankToDiagrams(
  unsigned short ranks[DDS_HANDS][DDS_SUITS],
  nodeCardsType const * np);

string WinnersToText(const unsigned short winRanks[]);

string NodeToText(nodeCardsType const * np);

string FullNodeToText(nodeCardsType const * np);

string PosToText(
  pos const * posPoint,
  const int target,
  const int depth);

string TopMove(
  const bool val,
  const moveType& bestMove);

string DumpTopHeader(
  ThreadData const * thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode);


string PrintSuit(const unsigned short suitCode)
{
  if (! suitCode)
    return "--";

  string st;
  for (int r = 14; r >= 2; r--)
    if ((suitCode & bitMapRank[r]))
      st += cardRank[r];
  return st;
}


string PrintSuit(
  const unsigned short suitCode,
  const char leastWin)
{
  if (! suitCode)
    return "--";

  string st;
  for (int r = 14; r >= 2; r--)
  {
    if ((suitCode & bitMapRank[r]))
    {
      if (r >= 15 - leastWin)
        st += cardRank[r];
      else
        st += "x";
    }
  }
  return st;
}


string PrintDeal(
  const unsigned short ranks[][DDS_SUITS],
  const unsigned spacing)
{
  stringstream ss;
  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << setw(spacing) << "" << 
      cardSuit[s] << " " <<
      PrintSuit(ranks[0][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << cardSuit[s] << " " <<
      setw(2*spacing - 2) << left << PrintSuit(ranks[3][s]) <<
      cardSuit[s] << " " <<
      PrintSuit(ranks[1][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << setw(spacing) << "" << 
      cardSuit[s] << " " <<
      PrintSuit(ranks[2][s]) << "\n";
  }

  return ss.str() + "\n";
}


string RankToDiagrams(
  const unsigned short ranks[DDS_HANDS][DDS_SUITS],
  nodeCardsType const * np)
{
  stringstream ss;
  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << setw(12) << left << 
      (s == 0 ? "Sought" : "") << 
      cardSuit[s] << " " << setw(20) << PrintSuit(ranks[0][s]) << "|    " <<
      setw(12) << (s == 0 ? "Found" : "") << 
      cardSuit[s] << " " << PrintSuit(ranks[0][s], np->leastWin[s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << 
      cardSuit[s] << " " << setw(22) << left << PrintSuit(ranks[3][s]) <<
      cardSuit[s] << " " << setw(8) << PrintSuit(ranks[1][s]) << "|    " << 
      cardSuit[s] << " " << 
        setw(22) << PrintSuit(ranks[3][s], np->leastWin[s]) << 
      cardSuit[s] << " " << PrintSuit(ranks[1][s], np->leastWin[s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    ss << setw(12) << left << "" << 
      cardSuit[s] << " " << setw(20) << PrintSuit(ranks[0][s]) << "|    " <<
      setw(12) << "" << cardSuit[s] << " " <<
      PrintSuit(ranks[0][s], np->leastWin[s]) << "\n";
  }
  return ss.str();
}


string WinnersToText(const unsigned short ourWinRanks[])
{
  stringstream ss;
  for (int s = 0; s < DDS_SUITS; s++)
    ss << cardSuit[s] << " " << PrintSuit(ourWinRanks[s]) << "\n";

  return ss.str();
}


string NodeToText(nodeCardsType const * np)
{
  stringstream ss;
  ss << setw(16) << left << "Address" << 
    static_cast<void const *>(np) << "\n";

  ss << setw(16) << left << "Bounds" << 
    static_cast<int>(np->lbound) << " to " <<
    static_cast<int>(np->ubound) << " tricks\n";

  ss << setw(16) << left << "Best move" << 
    cardSuit[ static_cast<int>(np->bestMoveSuit) ] <<
    cardRank[ static_cast<int>(np->bestMoveRank) ] << "\n";

  return ss.str();
}


string FullNodeToText(nodeCardsType const * np)

{
  stringstream ss;
  vector<int> v(DDS_SUITS);
  for (unsigned i = 0; i < DDS_SUITS; i++)
    v[i] = 15 - static_cast<int>(np->leastWin[i]);

  ss << setw(16) << left << "Lowest used" << 
    cardSuit[0] << cardRank[v[0]] << ", " <<
    cardSuit[1] << cardRank[v[1]] << ", " <<
    cardSuit[2] << cardRank[v[2]] << ", " <<
    cardSuit[3] << cardRank[v[3]] << "\n";

  return NodeToText(np) + ss.str();
}


string PosToText(
  pos const * posPoint,
  const int target,
  const int depth)
{
  stringstream ss;
  ss << setw(16) << left << "Target" << target << "\n";
  ss << setw(16) << "Depth" << depth << "\n";
  ss << setw(16) << "tricksMAX" << posPoint->tricksMAX << "\n";
  ss << setw(16) << "First hand" << 
    cardHand[ posPoint->first[depth] ] << "\n";
  ss << setw(16) << "Next first" << 
    cardHand[ posPoint->first[depth - 1] ] << "\n";
  return ss.str();
}


string DumpTopHeader(
  ThreadData const * thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode)
{
  string stext;
  if (printMode == 0)
  {
    // Trying just one target.
    stext = "Single target " + to_string(tricks) + ", " + "achieved";
  }
  else if (printMode == 1)
  {
    // Looking for best score.
    stext = "Loop target " + to_string(tricks) + ", " +
      "bounds " + to_string(lower) + " .. " + to_string(upper) + ", " +
      TopMove(thrp->val, thrp->bestMove[thrp->iniDepth]) + "";
  }
  else if (printMode == 2)
  {
    // Looking for other moves with best score.
    stext = "Loop for cards with score " + to_string(tricks) + ", " +
      TopMove(thrp->val, thrp->bestMove[thrp->iniDepth]);
  }
  return stext + "\n" + string(stext.size(), '-') + "\n";
}


string TopMove(
  const bool val,
  const moveType& bestMove)
{
  if (val)
  {
    stringstream ss;
    ss << "achieved with move " <<
      cardSuit[ bestMove.suit ] <<
      cardRank[ bestMove.rank ];
    return ss.str();
  }
  else
    return "failed";
}


int DumpInput(
  const int errCode, 
  const deal& dl, 
  const int target,
  const int solutions, 
  const int mode)
{
  ofstream fout;
  fout.open("dump.txt");

  fout << "Error code=" << errCode << "\n\n";
  fout << "Deal data:\n";
  fout << "trump=";

  if (dl.trump == DDS_NOTRUMP)
    fout << "N\n";
  else
    fout << cardSuit[dl.trump] << "\n";
  fout << "first=" << cardHand[dl.first] << "\n";

  unsigned short ranks[4][4];

  for (int k = 0; k <= 2; k++)
    if (dl.currentTrickRank[k] != 0)
    {
      fout << "index=" << k << 
        " currentTrickSuit=" << cardSuit[dl.currentTrickSuit[k]] <<
        " currentTrickRank= " << cardRank[dl.currentTrickRank[k]] << "\n";
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
  return 0;
}


void DumpRetrieved(
  const string& fname,
  pos const * posPoint,
  nodeCardsType const * np,
  const int target,
  const int depth)
{
  ofstream fout;
  fout.open(fname, ofstream::out | ofstream::app);

  fout << "Retrieved entry\n";
  fout << string(15, '-') << "\n";
  fout << PosToText(posPoint, target, depth) << "\n";
  fout << FullNodeToText(np) << "\n";
  fout << RankToDiagrams(posPoint->rankInSuit, np) << "\n";

  fout.close();
}


void DumpStored(
  const string& fname,
  pos const * posPoint,
  Moves const * moves,
  nodeCardsType const * np,
  const int target,
  const int depth)
{
  ofstream fout;
  fout.open(fname, ofstream::out | ofstream::app);

  // Big enough for all uses.
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  fout << "Stored entry\n";
  fout << string(12, '-') << "\n";
  fout << PosToText(posPoint, target, depth) << "\n";
  fout << NodeToText(np);

  moves->TrickToText((depth >> 2) + 1, text[0]);
  fout << string(text[0]) << "\n";
  fout << PrintDeal(posPoint->rankInSuit, 16);

  fout.close();
}


void DumpTopLevel(
  ThreadData const * thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode)
{
  pos const * posPoint = &thrp->lookAheadPos;

  ofstream fout;
  fout.open(thrp->fnTopLevel, ofstream::out | ofstream::app);

  fout << DumpTopHeader(thrp, tricks, lower, upper, printMode) << "\n";
  fout << PrintDeal(posPoint->rankInSuit, 16);
  fout << WinnersToText(posPoint->winRanks[ thrp->iniDepth ]) << "\n";
  fout << thrp->nodes << " AB nodes, " <<
    thrp->trickNodes << " trick nodes\n\n";

  fout.close();
}

