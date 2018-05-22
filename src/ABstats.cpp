/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/*
   ABstats is a simple object for AB statistics and return values.
*/


#include <iomanip>

#include "ABstats.h"


ABstats::ABstats()
{
  ABstats::Reset();
  ABstats::SetNames();
}


ABstats::~ABstats()
{
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

  for (int place = 0; place < AB_SIZE; place++)
  {
    for (int depth = 0; depth < DDS_MAXDEPTH; depth++)
      ABplaces[place].list[depth] = 0;

    ABplaces[place].sum = 0;
    ABplaces[place].sumWeighted = 0;
  }
}


void ABstats::ResetCum()
{
  ABnodesCum.sumCum = 0;
  ABnodesCum.sumCumWeighted = 0;

  for (int side = 0; side < 2; side++)
  {
    ABsides[side].sumCum = 0;
    ABsides[side].sumCumWeighted = 0;
  }

  for (int place = 0; place < AB_SIZE; place++)
  {
    ABplaces[place].sumCum = 0;
    ABplaces[place].sumCumWeighted = 0;
  }
}


void ABstats::SetNames()
{
  name[AB_TARGET_REACHED] = "Target decided";
  name[AB_DEPTH_ZERO] = "depth == 0";
  name[AB_QUICKTRICKS] = "QuickTricks";
  name[AB_QUICKTRICKS_2ND] = "QuickTricks 2nd";
  name[AB_LATERTRICKS] = "LaterTricks";
  name[AB_MAIN_LOOKUP] = "Main lookup";
  name[AB_SIDE_LOOKUP] = "Other lookup";
  name[AB_MOVE_LOOP] = "Move trial";
}


void ABstats::IncrPos(
  const ABCountType no, 
  const bool side, 
  const int depth)
{
  if (no < 0 || no >= AB_SIZE)
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


void ABstats::IncrNode(const int depth)
{
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


void ABstats::PrintHeaderPosition(ofstream& fout) const
{
  fout << "No " <<
    setw(20) << left << "Return" <<
    setw(9) << right << "Count" <<
    setw(6) << "%" <<
    setw(6) << "d_avg" <<
    setw(9) << "Cumul" <<
    setw(6) << "%" <<
    setw(6) << "d_avg" << "\n";

  fout << string(65, '-') << "\n";
}


void ABstats::PrintStatsPosition(
  ofstream& fout,
  const int no,
  const string& text,
  const ABtracker& abt,
  const ABtracker& divisor) const
{
  if (! abt.sumCum)
    return;

  fout << setw(2) << (no == -1 ? "" : to_string(no)) << " " << 
    setw(20) << left << text <<
    setw(9) << right << abt.sum <<
    setw(6) << setprecision(1) << fixed << 
      100. * abt.sum / static_cast<double>(divisor.sum);

  if (abt.sum)
    fout << setw(6) << setprecision(1) << fixed << 
      abt.sumWeighted / static_cast<double>(abt.sum);
  else
    fout << setw(6) << "";

  fout << setw(9) << abt.sumCum <<
    setw(6) << setprecision(1) << fixed <<
      100. * abt.sumCum / static_cast<double>(divisor.sumCum) <<
    setw(6) << setprecision(1) << fixed <<
      abt.sumCumWeighted / static_cast<double>(abt.sumCum) << "\n";
}


void ABstats::PrintHeaderDepth(ofstream& fout) const
{
  fout << setw(5) << right << "Depth" <<
    setw(7) << "Nodes" <<
    setw(7) << "Cumul" <<
    setw(6) << "Cum%" <<
    setw(6) << "Cumc%" <<
    setw(7) << "Branch" << "\n";

  fout << string(38, '-') << "\n";
}


void ABstats::PrintStatsDepth(
  ofstream& fout,
  const int depth,
  const int cum) const
{
  fout << setw(5) << depth <<
    setw(7) << ABnodes.list[depth] <<
    setw(7) << ABnodesCum.list[depth] <<
    setw(6) << setprecision(1) << fixed <<
      100. * ABnodesCum.list[depth] / 
        static_cast<double>(ABnodesCum.sumCum) <<
    setw(6) << setprecision(1) << fixed <<
      100. * cum / static_cast<double>(ABnodesCum.sumCum);

  // "Branching factor" from end of one trick to end of
  // the previous trick.
  if ((depth % 4 == 1) &&
      (depth < DDS_MAXDEPTH - 4) &&
      (ABnodesCum.list[depth + 4] > 0))
    fout << setw(6) << setprecision(2) << fixed <<
      ABnodesCum.list[depth] / 
        static_cast<double>(ABnodesCum.list[depth + 4]);
  fout << "\n";
}


void ABstats::PrintAverageDepth(
  ofstream& fout,
  const ABtracker& ABsidesSum) const
{
  fout << "\nTotal" <<
    setw(7) << right << ABnodes.sum <<
    setw(7) << ABnodesCum.sumCum << "\n";

  if (! ABnodesCum.sumCum)
    return;

  fout << setw(5) << left << "Avg" << right;

  if (ABnodes.sum)
    fout << setw(7) << setprecision(1) << fixed << 
      ABnodes.sumWeighted / static_cast<double>(ABnodes.sum);
  else
    fout << setw(7) << "";

  fout << setw(7) << setprecision(1) << fixed << 
    ABnodesCum.sumCumWeighted / static_cast<double>(ABnodesCum.sumCum) <<
    "\n\n";

  fout << setw(5) << left << "Nodes" <<
    setw(7) << right << ABnodes.sum <<
    setw(7) << ABnodesCum.sumCum << "\n";

  fout << setw(5) << left << "Ends" <<
    setw(7) << right << ABsidesSum.sum <<
    setw(7) << ABsidesSum.sumCum << "\n";

  if (ABsidesSum.sum)
    fout << setw(5) << left << "Ratio" <<
      setw(6) << right << setprecision(0) << fixed << 
        100. * ABsidesSum.sum / static_cast<double>(ABnodes.sum) << "%" <<
      setw(6) << setprecision(0) << fixed << 
        100. * ABsidesSum.sumCum / static_cast<double>(ABnodesCum.sumCum) <<
        "%\n\n";
}


void ABstats::PrintHeaderDetail(ofstream& fout) const
{
  fout << " d" << setw(7) << "Side1" << setw(7) << "Side0";

  for (int p = 0; p < AB_SIZE; p++)
    fout << setw(6) << p;

  fout << "\n" << string(65, '-') << "\n";
}


void ABstats::PrintStatsDetail(
 ofstream& fout,
 const int depth) const
{
  if (ABsides[1].list[depth] == 0 && ABsides[0].list[depth] == 0)
    return;

  fout << setw(2) << depth <<
    setw(7) << ABsides[1].list[depth] <<
    setw(7) << ABsides[0].list[depth];

  for (int p = 0; p < AB_SIZE; p++)
    fout << setw(6) << ABplaces[p].list[depth];
  fout << "\n";
}


void ABstats::PrintSumDetail(ofstream& fout) const
{
  fout << string(65, '-') << "\n";

  fout << setw(2) << "S" <<
    setw(7) << ABsides[1].sum <<
    setw(7) << ABsides[0].sum;

  for (int p = 0; p < AB_SIZE; p++)
    fout << setw(6) << ABplaces[p].sum;
  fout << "\n\n";
}


void ABstats::PrintStats(ofstream& fout)
{
  ABtracker ABsidesSum;
  ABsidesSum.sum = ABsides[1].sum + ABsides[0].sum;
  ABsidesSum.sumCum = ABsides[1].sumCum + ABsides[0].sumCum;

  if (ABsidesSum.sum)
  {
    // First table: By side and position.

    ABstats::PrintHeaderPosition(fout);

    ABstats::PrintStatsPosition(fout, -1, "Side1", ABsides[1], ABsidesSum);
    ABstats::PrintStatsPosition(fout, -1, "Side0", ABsides[0], ABsidesSum);
    fout << "\n";

    for (int p = 0; p < AB_SIZE; p++)
      ABstats::PrintStatsPosition(fout, p, name[p], ABplaces[p], ABsidesSum);
    fout << "\n";
  }

  ABstats::PrintHeaderDepth(fout);


  // Second table: By depth.

  int c = 0;
  for (int d = DDS_MAXDEPTH - 1; d >= 0; d--)
  {
    if (ABnodesCum.list[d] == 0)
      continue;

    c += ABnodesCum.list[d];
    ABstats::PrintStatsDepth(fout, d, c);
  }

  ABstats::PrintAverageDepth(fout, ABsidesSum);


#ifdef DDS_AB_DETAILS
  // Third table: All the detail.

  ABstats::PrintHeaderDetail(fout);

  for (int d = DDS_MAXDEPTH - 1; d >= 0; d--)
    ABstats::PrintStatsDetail(fout, d);

  ABstats::PrintSumDetail(fout);
#endif
}

