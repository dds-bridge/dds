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
#define DDS_HAND_LINES 12
#define DDS_NODE_LINES 4
#define DDS_FULL_LINE 80
#define DDS_HAND_OFFSET 16
#define DDS_HAND_OFFSET2 12
#define DDS_DIAG_WIDTH 34


string PrintSuit(const unsigned short suitCode);

void PrintDeal(
  ofstream& fout,
  const unsigned short ranks[][DDS_SUITS]);

void RankToDiagrams(
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
  nodeCardsType * np,
  char text[DDS_HAND_LINES][DDS_FULL_LINE]);

void WinnersToText(
  unsigned short int winRanks[DDS_SUITS],
  char text[DDS_SUITS][DDS_FULL_LINE]);

void NodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES][DDS_FULL_LINE]);

void FullNodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES][DDS_FULL_LINE]);

void PosToText(
  pos * posPoint,
  int target,
  int depth,
  char text[DDS_POS_LINES][DDS_FULL_LINE]);


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
  const unsigned short ranks[][DDS_SUITS])
{
  for (int s = 0; s < DDS_SUITS; s++)
  {
    fout << setw(8) << "" << 
      cardSuit[s] << " " <<
      PrintSuit(ranks[0][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    fout << cardSuit[s] << " " <<
      setw(14) << left << PrintSuit(ranks[3][s]) <<
      cardSuit[s] << " " <<
      PrintSuit(ranks[1][s]) << "\n";
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    fout << setw(8) << "" << 
      cardSuit[s] << " " <<
      PrintSuit(ranks[2][s]) << "\n";
  }

  fout << "\n";
  return;
}


void RankToText(
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
  char text[DDS_HAND_LINES][DDS_FULL_LINE])
{
  int c, h, s, r;

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE - 1] = '\0';
  }

  for (h = 0; h < DDS_HANDS; h++)
  {
    int offset, line;
    if (h == 0)
    {
      offset = DDS_HAND_OFFSET;
      line = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET;
      line = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET;
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
          text[line + s][c++] = static_cast<char>(cardRank[r]);
      }

      if (c == offset)
        text[line + s][c++] = '-';

      if (h != 3)
        text[line + s][c] = '\0';
    }
  }
}


void RankToDiagrams(
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
  nodeCardsType * np,
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


void WinnersToText(
  unsigned short int ourWinRanks[DDS_SUITS],
  char text[DDS_SUITS][DDS_FULL_LINE])
{
  int c, s, r;

  for (int l = 0; l < DDS_SUITS; l++)
    memset(text[l], ' ', DDS_FULL_LINE);

  for (s = 0; s < DDS_SUITS; s++)
  {
    text[s][0] = static_cast<char>(cardSuit[s]);

    c = 2;
    for (r = 14; r >= 2; r--)
    {
      if (ourWinRanks[s] & bitMapRank[r])
        text[s][c++] = static_cast<char>(cardRank[r]);
    }
    text[s][c] = '\0';
  }
}


void NodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES - 1][DDS_FULL_LINE])

{
  sprintf(text[0], "Address\t\t%p\n", static_cast<void *>(np));

  sprintf(text[1], "Bounds\t\t%d to %d tricks\n",
          static_cast<int>(np->lbound),
          static_cast<int>(np->ubound));

  sprintf(text[2], "Best move\t%c%c\n",
          cardSuit[ static_cast<int>(np->bestMoveSuit) ],
          cardRank[ static_cast<int>(np->bestMoveRank) ]);

}


void FullNodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES][DDS_FULL_LINE])

{
  sprintf(text[0], "Address\t\t%p\n", static_cast<void *>(np));

  sprintf(text[1], "Lowest used\t%c%c, %c%c, %c%c, %c%c\n",
          cardSuit[0], cardRank[ 15 - static_cast<int>(np->leastWin[0]) ],
          cardSuit[1], cardRank[ 15 - static_cast<int>(np->leastWin[1]) ],
          cardSuit[2], cardRank[ 15 - static_cast<int>(np->leastWin[2]) ],
          cardSuit[3], cardRank[ 15 - static_cast<int>(np->leastWin[3]) ]);

  sprintf(text[2], "Bounds\t\t%d to %d tricks\n",
          static_cast<int>(np->lbound),
          static_cast<int>(np->ubound));

  sprintf(text[3], "Best move\t%c%c\n",
          cardSuit[ static_cast<int>(np->bestMoveSuit) ],
          cardRank[ static_cast<int>(np->bestMoveRank) ]);

}


void PosToText(
  pos * posPoint,
  int target,
  int depth,
  char text[DDS_POS_LINES][DDS_FULL_LINE])
{
  sprintf(text[0], "Target\t\t%d\n" , target);
  sprintf(text[1], "Depth\t\t%d\n" , depth);
  sprintf(text[2], "tricksMAX\t%d\n" , posPoint->tricksMAX);
  sprintf(text[3], "First hand\t%c\n",
          cardHand[ posPoint->first[depth] ]);
  sprintf(text[4], "Next first\t%c\n",
          cardHand[ posPoint->first[depth - 1] ]);
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
  PrintDeal(fout, ranks);
  fout.close();
  return 0;
}


void DumpRetrieved(
  FILE * fp,
  pos * posPoint,
  nodeCardsType * np,
  int target,
  int depth)
{
  // Big enough for all uses.
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  fprintf(fp, "Retrieved entry\n");
  fprintf(fp, "---------------\n");

  PosToText(posPoint, target, depth, text);
  for (int i = 0; i < DDS_POS_LINES; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  FullNodeToText(np, text);
  for (int i = 0; i < DDS_NODE_LINES; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  RankToDiagrams(posPoint->rankInSuit, np, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");
}


void DumpStored(
  FILE * fp,
  pos * posPoint,
  Moves * moves,
  nodeCardsType * np,
  int target,
  int depth)
{
  // Big enough for all uses.
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  fprintf(fp, "Stored entry\n");
  fprintf(fp, "------------\n");

  PosToText(posPoint, target, depth, text);
  for (int i = 0; i < DDS_POS_LINES; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  NodeToText(np, text);
  for (int i = 0; i < DDS_NODE_LINES - 1; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  moves->TrickToText((depth >> 2) + 1, text[0]);
  fprintf(fp, "%s", text[0]);
  fprintf(fp, "\n");

  RankToText(posPoint->rankInSuit, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");
}


void DumpTopLevel(
  ThreadData * thrp,
  int tricks,
  int lower,
  int upper,
  int printMode)
{
#ifdef DDS_TOP_LEVEL
  char text[DDS_HAND_LINES][DDS_FULL_LINE];
  pos * posPoint = &thrp->lookAheadPos;
  FILE * fp = thrp->fpTopLevel;

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
  fprintf(fp, "%s%s\n\n", text[0], text[1]);

  RankToText(posPoint->rankInSuit, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");

  WinnersToText(posPoint->winRanks[ thrp->iniDepth ], text);
  for (int i = 0; i < DDS_SUITS; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");

  fprintf(fp, "%d AB nodes, %d trick nodes\n\n",
          thrp->nodes,
          thrp->trickNodes);
#else
  UNUSED(thrp);
  UNUSED(tricks);
  UNUSED(lower);
  UNUSED(upper);
  UNUSED(printMode);
#endif
}

