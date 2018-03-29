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


#define DDS_POS_LINES 5
// #define DDS_HAND_LINES 12
#define DDS_NODE_LINES 4
// #define DDS_FULL_LINE 80
#define DDS_HAND_OFFSET 16
#define DDS_HAND_OFFSET2 12
#define DDS_DIAG_WIDTH 34


string PrintSuit(const unsigned short suitCode);

void PrintDeal(
  ofstream& fout,
  const unsigned short ranks[][DDS_SUITS],
  const unsigned spacing);

void RankToDiagrams(
  unsigned short rankInSuit[DDS_HANDS][DDS_SUITS],
  nodeCardsType * np,
  char text[DDS_HAND_LINES][DDS_FULL_LINE]);

string WinnersToText(const unsigned short winRanks[]);

string NodeToText(nodeCardsType const * np);

string FullNodeToText(nodeCardsType const * np);

void PosToText(
  pos const * posPoint,
  const int target,
  const int depth,
  string& text);


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


void PrintDeal(
  ofstream& fout,
  const unsigned short ranks[][DDS_SUITS],
  const unsigned spacing)
{
  for (int s = 0; s < DDS_SUITS; s++)
  {
    fout << setw(spacing) << "" << 
      cardSuit[s] << " " <<
      PrintSuit(ranks[0][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    fout << cardSuit[s] << " " <<
      setw(2*spacing - 2) << left << PrintSuit(ranks[3][s]) <<
      cardSuit[s] << " " <<
      PrintSuit(ranks[1][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    fout << setw(spacing) << "" << 
      cardSuit[s] << " " <<
      PrintSuit(ranks[2][s]) << "\n";
  }

  fout << "\n";
  return;
}


void RankToDiagrams(
  const unsigned short rankInSuit[DDS_HANDS][DDS_SUITS],
  nodeCardsType const * np,
  char text[DDS_HAND_LINES][DDS_FULL_LINE])
{
  int c, h, s, r;

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE - 1] = '\0';
    text[l][DDS_DIAG_WIDTH ] = '|';
  }

  strncpy(text[0], "Sought", 6);
  strncpy(&text[0][DDS_DIAG_WIDTH + 5], "Found", 5);

  for (h = 0; h < DDS_HANDS; h++)
  {
    int offset, line;
    if (h == 0)
    {
      offset = DDS_HAND_OFFSET2;
      line = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET2;
      line = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET2;
      line = 8;
    }
    else
    {
      offset = 0;
      line = 4;
    }

    for (s = 0; s < DDS_SUITS; s++)
    {
      c = offset;
      for (r = 14; r >= 2; r--)
      {
        if (rankInSuit[h][s] & bitMapRank[r])
        {
          text[line + s][c] = static_cast<char>(cardRank[r]);
          text[line + s][c + DDS_DIAG_WIDTH + 5] =
            (r >= 15 - np->leastWin[s] ?
             static_cast<char>(cardRank[r]) : 'x');
          c++;
        }
      }

      if (c == offset)
      {
        text[line + s][c] = '-';
        text[line + s][c + DDS_DIAG_WIDTH + 5] = '-';
        c++;
      }

      if (h != 3)
        text[line + s][c + DDS_DIAG_WIDTH + 5] = '\0';
    }
  }
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


void PosToText(
  pos const * posPoint,
  const int target,
  const int depth,
  string& text)
{
  stringstream ss;
  ss << setw(16) << left << "Target" << target << "\n";
  ss << setw(16) << "Depth" << depth << "\n";
  ss << setw(16) << "tricksMAX" << posPoint->tricksMAX << "\n";
  ss << setw(16) << "First hand" << 
    cardHand[ posPoint->first[depth] ] << "\n";
  ss << setw(16) << "Next first" << 
    cardHand[ posPoint->first[depth - 1] ] << "\n";
  text = ss.str();
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
  PrintDeal(fout, ranks, 8);
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

  // Big enough for all uses.
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  fout << "Retrieved entry\n";
  fout << string(15, '-') << "\n";

  string stext;
  PosToText(posPoint, target, depth, stext);
  fout << stext << "\n";

  fout << FullNodeToText(np) << "\n";

  RankToDiagrams(posPoint->rankInSuit, np, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fout << string(text[i]) << "\n";
  fout << "\n";

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

  string stext;
  PosToText(posPoint, target, depth, stext);
  fout << stext << "\n";

  fout << NodeToText(np);

  moves->TrickToText((depth >> 2) + 1, text[0]);
  fout << string(text[0]) << "\n";

  PrintDeal(fout, posPoint->rankInSuit, 16);

  fout.close();
}


void DumpTopLevel(
  ThreadData const * thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode)
{
  ofstream fout;
  fout.open(thrp->fnTopLevel, ofstream::out | ofstream::app);

  char text[DDS_HAND_LINES][DDS_FULL_LINE];
  pos const * posPoint = &thrp->lookAheadPos;

  if (printMode == 0)
  {
    // Trying just one target.
    sprintf(text[0], "Single target %d, %s\n",
            tricks,
            "achieved");
  }
  else if (printMode == 1)
  {
    // Looking for best score.
    if (thrp->val)
    {
      sprintf(text[0],
              "Loop target %d, bounds %d .. %d, achieved with move %c%c\n",
              tricks,
              lower,
              upper,
              cardSuit[ thrp->bestMove[thrp->iniDepth].suit ],
              cardRank[ thrp->bestMove[thrp->iniDepth].rank ]);
    }
    else
    {
      sprintf(text[0],
              "Loop target %d, bounds %d .. %d, failed\n",
              tricks,
              lower,
              upper);
    }
  }
  else if (printMode == 2)
  {
    // Looking for other moves with best score.
    if (thrp->val)
    {
      sprintf(text[0],
              "Loop for cards with score %d, achieved with move %c%c\n",
              tricks,
              cardSuit[ thrp->bestMove[thrp->iniDepth].suit ],
              cardRank[ thrp->bestMove[thrp->iniDepth].rank ]);
    }
    else
    {
      sprintf(text[0],
              "Loop for cards with score %d, failed\n",
              tricks);
    }
  }

  size_t l = strlen(text[0]) - 1;

  memset(text[1], '-', l);
  text[1][l] = '\0';
  fout << string(text[0]) << string(text[1]) << "\n\n";

  PrintDeal(fout, posPoint->rankInSuit, 16);

  fout << WinnersToText(posPoint->winRanks[ thrp->iniDepth ]) << "\n";

  fout << thrp->nodes << " AB nodes, " <<
    thrp->trickNodes << " trick nodes\n\n";

  fout.close();
}

