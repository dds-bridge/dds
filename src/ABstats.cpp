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
  for (int p = 0; p < DDS_AB_POS; p++)
  {
    for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
      counter[p][depth] = 0;
  }

  for (int side = 0; side < 2; side++)
  {
    for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
      score[side][depth] = 0;
  }

  for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
    nodes[depth] = 0;

  allnodes = 0;

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

  allnodesCum = 0;

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

  counter[no][depth]++;

  ABplaces[no].list[depth]++;
  ABplaces[no].sum++;
  ABplaces[no].sumWeighted += depth;
  ABplaces[no].sumCum++;
  ABplaces[no].sumCumWeighted += depth;

  const int iside = (side ? 1 : 0);

  score[iside][depth]++;

  ABsides[iside].list[depth]++;
  ABsides[iside].sum++;
  ABsides[iside].sumWeighted += depth;
  ABsides[iside].sumCum++;
  ABsides[iside].sumCumWeighted += depth;

}


void ABstats::IncrNode(int depth)
{
  nodes[depth]++;
  nodesCum[depth]++;
  allnodes++;
}


int ABstats::GetNodes() const
{
  return allnodes;
}


#include "../include/portab.h"
void ABstats::PrintStatsPosition(FILE * fpl) const
{
  UNUSED(fpl);
}


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

  allnodesCum += allnodes;

  int s = ABsides[1].sum + ABsides[0].sum;
  int cs = ABsides[1].sumCum + ABsides[0].sumCum;
  if (s)
  {
    fprintf(fp, "%2s %-20s %8s %5s %5s %8s %5s %5s\n",
            "No",
            "Return",
            "Count",
            "%",
            "d_avg",
            "Cumul",
            "%",
            "d_avg");

    fprintf(fp, "-----------------------------------");
    fprintf(fp, "------------------------------\n");

    fprintf(fp, "%2s %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n",
            "",
            "Side1",
            ABsides[1].sum,
            100. * ABsides[1].sum / static_cast<double>(s),
            ABsides[1].sumWeighted / static_cast<double>(s),
            ABsides[1].sumCum,
            100. * ABsides[1].sumCum / static_cast<double>(cs),
            ABsides[1].sumCumWeighted / static_cast<double>(cs));

    fprintf(fp, "%2s %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n\n",
            "",
            "Side0",
            ABsides[0].sum,
            100. * ABsides[0].sum / static_cast<double>(s),
            ABsides[0].sumWeighted / static_cast<double>(s),
            ABsides[0].sumCum,
            100. * ABsides[0].sumCum / static_cast<double>(cs),
            ABsides[0].sumCumWeighted / static_cast<double>(cs));

    for (int p = 0; p < DDS_AB_POS; p++)
    {
      if (ABplaces[p].sum)
      {
        fprintf(fp, "%2d %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n",
                p,
                name[p],
                ABplaces[p].sum,
                100. * ABplaces[p].sum / static_cast<double>(s),
                ABplaces[p].sumWeighted / static_cast<double>(ABplaces[p].sum),
                ABplaces[p].sumCum,
                100. * ABplaces[p].sumCum / static_cast<double>(cs),
                ABplaces[p].sumCumWeighted / static_cast<double>(ABplaces[p].sumCum));
      }
      else if (ABplaces[p].sumCum)
      {
        fprintf(fp, "%2d %-20s %8d %5.1f %5s %8d %5.1f %5.1f\n",
                p,
                name[p],
                ABplaces[p].sum,
                100. * ABplaces[p].sum / static_cast<double>(s),
                "",
                ABplaces[p].sumCum,
                100. * ABplaces[p].sumCum / static_cast<double>(cs),
                ABplaces[p].sumCumWeighted / static_cast<double>(ABplaces[p].sumCum));
      }
    }
  }

  fprintf(fp, "\n%5s %6s %6s %5s %5s %6s\n",
          "Depth",
          "Nodes",
          "Cumul",
          "Cum%",
          "Cumc%",
          "Branch");

  fprintf(fp, "------------------------------------------\n");

  int c = 0;
  double np = 0., ncp = 0.;
  for (int d = DDS_MAXDEPTH - 1; d >= 0; d--)
  {
    if (nodesCum[d] == 0)
      continue;

    c += nodesCum[d];
    np += d * nodes[d];
    ncp += d * nodesCum[d];

    fprintf(fp, "%5d %6d %6d %5.1f %5.1f",
            d,
            nodes[d],
            nodesCum[d],
            100. * nodesCum[d] / static_cast<double>(allnodesCum),
            100. * c / static_cast<double>(allnodesCum));

    // "Branching factor" from end of one trick to end of
    // the previous trick.
    if ((d % 4 == 1) &&
        (d < DDS_MAXDEPTH - 4) &&
        (nodesCum[d + 4] > 0))
      fprintf(fp, " %5.2f",
              nodesCum[d] / static_cast<double>(nodesCum[d + 4]));
    fprintf(fp, "\n");
  }

  fprintf(fp, "\n%-5s %6d %6d\n",
          "Total", allnodes, allnodesCum);

  if (allnodes)
  {
    fprintf(fp, "%-5s %6.1f %6.1f\n",
            "d_avg",
            np / static_cast<double>(allnodes),
            ncp / static_cast<double>(allnodesCum));
  }
  else if (allnodesCum)
  {
    fprintf(fp, "\n%-5s %6s %6.1f\n",
            "Avg",
            "-",
            ncp / static_cast<double>(allnodesCum));
  }

  fprintf(fp, "%-5s %6d\n\n\n", "Diff",
          allnodes - ABsides[1].sum - ABsides[0].sum);

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
    if (score[1][d] == 0 && score[0][d] == 0)
      continue;

    fprintf(fp, "%2d %6d %6d",
            d, score[1][d], score[0][d]);

    for (int p = 0; p < DDS_AB_POS; p++)
      fprintf(fp, " %5d", counter[p][d]);
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
