/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/*
   ABstats is a simple object for AB statistics and return values.
*/


#include "dds.h"
#include "ABstats.h"


ABstats::ABstats()
{
  fname = "";
  fileSet = false;
  fp = stdout;

  for (int p = 0; p < DDS_AB_POS; p++)
    sprintf(name[p], "Position %4d", p);

  ABstats::Reset();
}


ABstats::~ABstats()
{
  // Nothing to do
  if (fp != stdout && fp != nullptr)
    fclose(fp);
}


void ABstats::Reset()
{
  for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
    ABnodes.list[depth] = 0;

  ABnodes.sum = 0;
  ABnodes.sumWeighted = 0;

  for (int side = 0; side < 2; side++)
  {
    for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
      ABsides[side].list[depth] = 0;

    ABsides[side].sum = 0;
    ABsides[side].sumWeighted = 0;
  }

  for (int place = 0; place < DDS_AB_POS; place++)
  {
    for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
      ABplaces[place].list[depth] = 0;

    ABplaces[place].sum = 0;
    ABplaces[place].sumWeighted = 0;
  }
}


void ABstats::ResetCum()
{
  for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
    nodesCum[depth] = 0;

  ABnodesCum.sumCum = 0;
  ABnodesCum.sumCumWeighted = 0;

  for (int side = 0; side < 2; side++)
  {
    ABsides[side].sumCum = 0;
    ABsides[side].sumCumWeighted = 0;
  }

  for (int place = 0; place < DDS_AB_POS; place++)
  {
    ABplaces[place].sumCum = 0;
    ABplaces[place].sumCumWeighted = 0;
  }
}


void ABstats::SetFile(const string& fnameIn)
{
  fname = fnameIn;
}


void ABstats::SetName(int no, char * ourName)
{
  if (no < 0 || no >= DDS_AB_POS)
    return;

  sprintf(name[no], "%s", ourName);
}


void ABstats::IncrPos(
  int no, 
  bool side, 
  int depth)
{
  if (no < 0 || no >= DDS_AB_POS)
    return;

  ABplaces[no].list[depth]++;
  ABplaces[no].sum++;
  ABplaces[no].sumWeighted += depth;
  ABplaces[no].sumCum++;
  ABplaces[no].sumCumWeighted += depth;

  const int iside = (side ? 1 : 0);

  ABsides[iside].list[depth]++;
  ABsides[iside].sum++;
  ABsides[iside].sumWeighted += depth;
  ABsides[iside].sumCum++;
  ABsides[iside].sumCumWeighted += depth;

}


void ABstats::IncrNode(int depth)
{
  nodesCum[depth]++;

  ABnodes.list[depth]++;
  ABnodes.sum++;
  ABnodes.sumWeighted += depth;

  ABnodesCum.list[depth]++;
  ABnodesCum.sumCum++;
  ABnodesCum.sumCumWeighted += depth;
}


int ABstats::GetNodes() const
{
  return ABnodes.sum;
}


void ABstats::PrintHeaderPosition(FILE * fpl) const
{
  fprintf(fpl, "%2s %-20s %8s %5s %5s %8s %5s %5s\n",
    "No",
    "Return",
    "Count",
    "%",
    "d_avg",
    "Cumul",
    "%",
    "d_avg");

  fprintf(fpl, "-----------------------------------");
  fprintf(fpl, "------------------------------\n");
}

void ABstats::PrintStatsPosition(
  FILE * fpl,
  int no,
  char * text,
  const ABtracker& abt,
  const ABtracker& divisor) const
{
  if (abt.sum)
  {
    fprintf(fpl, "%2s %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n",
      (no == -1 ? "" : to_string(no).c_str()),
      text,
      abt.sum,
      100. * abt.sum / static_cast<double>(divisor.sum),
      abt.sumWeighted / static_cast<double>(abt.sum),
      abt.sumCum,
      100. * abt.sumCum / static_cast<double>(divisor.sumCum),
      abt.sumCumWeighted / static_cast<double>(abt.sumCum));
  }
  else if (abt.sumCum)
  {
    fprintf(fp, "%2s %-20s %8d %5.1f %5s %8d %5.1f %5.1f\n",
      (no == -1 ? "" : to_string(no).c_str()),
      text,
      abt.sum,
      100. * abt.sum / static_cast<double>(divisor.sum),
      "",
      abt.sumCum,
      100. * abt.sumCum / static_cast<double>(divisor.sumCum),
      abt.sumCumWeighted / static_cast<double>(abt.sumCum));
  }
}


void ABstats::PrintHeaderDepth(FILE * fpl) const
{
  fprintf(fpl, "\n%5s %6s %6s %5s %5s %6s\n",
          "Depth",
          "Nodes",
          "Cumul",
          "Cum%",
          "Cumc%",
          "Branch");

  fprintf(fp, "------------------------------------------\n");
}


void ABstats::PrintAverageDepth(FILE * fpl) const
{
  fprintf(fpl, "\n%-5s %6d %6d\n",
          "Total", ABnodes.sum, ABnodesCum.sumCum);

  if (ABnodes.sum)
  {
    fprintf(fpl, "%-5s %6.1f %6.1f\n",
      "d_avg",
      ABnodes.sumWeighted / static_cast<double>(ABnodes.sum),
      ABnodesCum.sumCumWeighted / static_cast<double>(ABnodesCum.sumCum));
  }
  else if (ABnodes.sumCum)
  {
    fprintf(fpl, "\n%-5s %6s %6.1f\n",
      "Avg",
      "-",
      ABnodes.sumCumWeighted / static_cast<double>(ABnodesCum.sumCum));
  }
}


#include "../include/portab.h"
void ABstats::PrintStats()
{
  if (! fileSet)
  {
    if (fname != "")
    {
      fp = fopen(fname.c_str(), "w");
      if (! fp)
        fp = stdout;
    }
    fileSet = true;
  }

  ABtracker ABsidesSum;
  ABsidesSum.sum = ABsides[1].sum + ABsides[0].sum;
  ABsidesSum.sumCum = ABsides[1].sumCum + ABsides[0].sumCum;

  if (ABsidesSum.sum)
  {
    ABstats::PrintHeaderPosition(fp);

    ABstats::PrintStatsPosition(fp, -1, "Side1", ABsides[1], ABsidesSum);
    ABstats::PrintStatsPosition(fp, -1, "Side0", ABsides[0], ABsidesSum);
    fprintf(fp, "\n");

    for (int p = 0; p < DDS_AB_POS; p++)
      ABstats::PrintStatsPosition(fp, p, name[p], ABplaces[p], ABsidesSum);
  }

  ABstats::PrintHeaderDepth(fp);

  int c = 0;
  for (int d = DDS_MAXDEPTH - 1; d >= 0; d--)
  {
    if (nodesCum[d] == 0)
      continue;

if (nodesCum[d] != ABnodesCum.list[d])
  fprintf(fp, "Err %d %d\n", nodesCum[d], ABnodesCum.list[d]);

    c += nodesCum[d];

    fprintf(fp, "%5d %6d %6d %5.1f %5.1f",
            d,
            ABnodes.list[d],
            nodesCum[d],
            100. * nodesCum[d] / static_cast<double>(ABnodesCum.sumCum),
            100. * c / static_cast<double>(ABnodesCum.sumCum));

    // "Branching factor" from end of one trick to end of
    // the previous trick.
    if ((d % 4 == 1) &&
        (d < DDS_MAXDEPTH - 4) &&
        (nodesCum[d + 4] > 0))
      fprintf(fp, " %5.2f",
              nodesCum[d] / static_cast<double>(nodesCum[d + 4]));
    fprintf(fp, "\n");
  }

  ABstats::PrintAverageDepth(fp);
  fprintf(fp, "\n");

  fprintf(fp, "%5s%7d%7d\n", "Nodes", ABnodes.sum, ABnodesCum.sumCum);
  fprintf(fp, "%5s%7d%7d\n", "Ends", ABsidesSum.sum, ABsidesSum.sumCum);
  if (ABsidesSum.sum)
    fprintf(fp, "%5s%6.0f%%%6.0f%%\n\n", "Ratio", 
      100. * ABsidesSum.sum / static_cast<double>(ABnodes.sum), 
      100. * ABsidesSum.sumCum / static_cast<double>(ABnodesCum.sumCum));

#ifdef DDS_AB_DETAILS
  fprintf(fp, "%2s %6s %6s",
          "d",
          "Side1",
          "Side0");

  for (int p = 0; p < DDS_AB_POS; p++)
    fprintf(fp, " %5d", p);
  fprintf(fp, "\n------------------------------");
  fprintf(fp, "-----------------------------\n");


  for (int d = DDS_MAXDEPTH - 1; d >= 0; d--)
  {
    if (ABsides[1].list[d] == 0 && ABsides[0].list[d] == 0)
      continue;

    fprintf(fp, "%2d %6d %6d",
            d, ABsides[1].list[d], ABsides[0].list[d]);

    for (int p = 0; p < DDS_AB_POS; p++)
      fprintf(fp, " %5d", ABplaces[p].list[d]);
    fprintf(fp, "\n");
  }

  fprintf(fp, "--------------------------------");
  fprintf(fp, "---------------------------\n");

  fprintf(fp, "%2s %6d %6d",
          "S", ABsides[1].sum, ABsides[0].sum);

  for (int p = 0; p < DDS_AB_POS; p++)
    fprintf(fp, " %5d", ABplaces[p].sum);
  fprintf(fp, "\n\n");
#endif

}
