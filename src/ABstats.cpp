/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/* 
   This object, ABstats, is a simple object for AB statistics 
   and return values.
*/


#include "dds.h"
#include "ABstats.h"


ABstats::ABstats()
{
  strcpy(fname, "");
  fp = stdout;
  ABstats::Reset();
  ABstats::ResetCum();
}


ABstats::~ABstats()
{
  // Nothing to do
  if (fp != stdout)
    fclose(fp);
}


void ABstats::Reset()
{
  for (int p = 0; p < DDS_AB_POS; p++)
  {
    sprintf(name[p], "Position %4d", p);

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
}


void ABstats::ResetCum()
{
  for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
    nodesCum[depth] = 0;
  
  allnodesCum = 0;

  scoreCum[1]  = 0;
  scoreCum[0]  = 0;

  pscoreCum[1] = 0;
  pscoreCum[0] = 0;

  for (int p = 0; p < DDS_AB_POS; p++)
  {
    counterCum[p]  = 0;
    pcounterCum[p] = 0;
  }
}


void ABstats::SetFile(char * ourFname)
{
  if (strlen(ourFname) > DDS_LINE_LEN)
    return;
  
  if (fp != stdout) // Already set
    return;

  strncpy(fname, ourFname, strlen(fname));

  fp = fopen(fname, "w");
  if (! fp)
    fp = stdout;
}


void ABstats::SetName(int no, char * ourName)
{
  if (no < 0 || no >= DDS_AB_POS)
    return;
  
  sprintf(name[no], "%s", ourName);
}


void ABstats::IncrPos(int no, bool side, int depth)
{
  if (no < 0 || no >= DDS_AB_POS)
    return;
  
  counter[no][depth]++;
  if (side)
    score[1][depth]++;
  else
    score[0][depth]++;
}


void ABstats::IncrNode(int depth)
{
  nodes[depth]++;
  allnodes++;
}


int ABstats::GetNodes()
{
  return allnodes;
}


void ABstats::PrintStats()
{
  int sumScore1  = 0 , sumScore0  = 0;
  int psumScore1 = 0 , psumScore0 = 0;
  int sum[DDS_AB_POS], psum[DDS_AB_POS];

  for (int p = 0; p < DDS_AB_POS; p++)
  {
    sum[p]  = 0;
    psum[p] = 0;
  }

  for (int d = 0; d < DDS_MAXDEPTH; d++)
  {
    sumScore1  += score[1][d];
    sumScore0  += score[0][d];

    psumScore1 += d * score[1][d];
    psumScore0 += d * score[0][d];

    for (int p = 0; p < DDS_AB_POS; p++)
    {
      sum[p]  += counter[p][d];
      psum[p] += d * counter[p][d];
    }

    nodesCum[d] += nodes[d]; 
  }

  allnodesCum  += allnodes;

  scoreCum[1]  += sumScore1;
  scoreCum[0]  += sumScore0;

  pscoreCum[1] += psumScore1;
  pscoreCum[0] += psumScore0;

  for (int p = 0; p < DDS_AB_POS; p++)
  {
    counterCum[p]  += sum[p];
    pcounterCum[p] += psum[p];
  }

  int s  = sumScore1   + sumScore0;
  int cs = scoreCum[1] + scoreCum[0];
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
      sumScore1,
      100. * sumScore1 / static_cast<double>(s),
      psumScore1 / static_cast<double>(s),
      scoreCum[1], 
      100. * scoreCum[1] / static_cast<double>(cs),
      pscoreCum[1] / static_cast<double>(cs));

    fprintf(fp, "%2s %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n\n",
      "",
      "Side0",
      sumScore0,
      100. * sumScore0 / static_cast<double>(s),
      psumScore0 / static_cast<double>(s),
      scoreCum[0], 
      100. * scoreCum[0] / static_cast<double>(cs),
      pscoreCum[0] / static_cast<double>(cs));

    for (int p = 0; p < DDS_AB_POS; p++)
    {
      if (sum[p])
      {
        fprintf(fp, "%2d %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n",
	  p, 
          name[p], 
          sum[p],
	  100. * sum[p] / static_cast<double>(s),
	  psum[p] / static_cast<double>(sum[p]),
	  counterCum[p],
	  100. * counterCum[p] / static_cast<double>(cs),
	  pcounterCum[p] / static_cast<double>(counterCum[p]));
      }
      else if (counterCum[p])
      {
        fprintf(fp, "%2d %-20s %8d %5.1f %5s %8d %5.1f %5.1f\n",
	  p, 
          name[p], 
          sum[p],
	  100. * sum[p] / static_cast<double>(s),
	  "",
	  counterCum[p],
	  100. * counterCum[p] / static_cast<double>(cs),
	  pcounterCum[p] / static_cast<double>(counterCum[p]));
      }
    }
  }

  fprintf(fp, "\n%5s  %6s  %6s  %5s  %5s %6s\n",
    "Depth", 
    "Nodes", 
    "Cumul", 
    "Cum%", 
    "Cumc%", 
    "Branch");

  fprintf(fp, "------------------------------------------\n");

  int c = 0;
  double np = 0., ncp = 0.;
  for (int d = DDS_MAXDEPTH-1; d >= 0; d--)
  {
    if (nodesCum[d] == 0)
      continue;

    c   += nodesCum[d];
    np  += d * nodes[d];
    ncp += d * nodesCum[d];

    fprintf(fp, "%5d  %6d  %6d  %5.1f  %5.1f", 
      d, 
      nodes[d], 
      nodesCum[d], 
      100. * nodesCum[d] / static_cast<double>(allnodesCum),
      100. * c / static_cast<double>(allnodesCum));

    // "Branching factor" from end of one trick to end of
    // the previous trick.
    if ((d % 4 == 1) && 
        (d < DDS_MAXDEPTH - 4) && 
	(nodesCum[d+4] > 0))
      fprintf(fp, "  %5.2f",
        nodesCum[d] / static_cast<double>(nodesCum[d+4]));
    fprintf(fp, "\n");
  }

  fprintf(fp, "\n%-5s  %6d  %6d\n", 
    "Total", allnodes, allnodesCum);

  if (allnodes)
  {
    fprintf(fp, "%-5s  %6.1f  %6.1f\n", 
      "d_avg", 
      np / static_cast<double>(allnodes), 
      ncp / static_cast<double>(allnodesCum));
  }
  else if (allnodesCum)
  {
    fprintf(fp, "\n%-5s  %6s  %6.1f\n", 
      "Avg", 
      "-",
      ncp / static_cast<double>(allnodesCum));
  }

  fprintf(fp, "%-5s  %6d\n\n\n", "Diff", 
    allnodes - sumScore1 - sumScore0);

#ifdef DDS_AB_DETAILS
  fprintf(fp, "%2s  %6s %6s",
    "d",
    "Side1",
    "Side0");

  for (int p = 0; p < DDS_AB_POS; p++)
    fprintf(fp, " %5d", p);
  fprintf(fp, "\n------------------------------");
  fprintf(fp, "-----------------------------\n");


  for (int d = DDS_MAXDEPTH-1; d >= 0; d--)
  {
    if (score[1][d] == 0 && score[0][d] == 0)
      continue;

    fprintf(fp, "%2d  %6d %6d",
      d, score[1][d], score[0][d]);

    for (int p = 0; p < DDS_AB_POS; p++)
      fprintf(fp, " %5d", counter[p][d]);
    fprintf(fp, "\n");
  }

  fprintf(fp, "--------------------------------");
  fprintf(fp, "---------------------------\n");

  fprintf(fp, "%2s  %6d %6d",
    "S", sumScore1, sumScore0);

  for (int p = 0; p < DDS_AB_POS; p++)
    fprintf(fp, " %5d", sum[p]);
  fprintf(fp, "\n\n");
#endif

}
