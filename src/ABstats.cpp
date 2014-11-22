/*
   DDS 2.7.0   A bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund

   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.

   The code for calculation of par score / contracts is based upon the

   perl code written by Matthew Kidd for ACBLmerge.
   He has kindly given 
   me permission to include a C++ adaptation in DDS. 
*/



/*
   Licensed under the Apache License, Version 2.0 (the "License");

   you may not use this file except in compliance with the License.

   You may obtain a copy of the License at


   http://www.apache.org/licenses/LICENSE-2.0


   Unless required by applicable law or agreed to in writing, software

   distributed under the License is distributed on an "AS IS" BASIS,

   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 
  implied.  See the License for the specific language governing
 
   permissions and limitations under the License.
*/



/* 
   This object, ABstats, was was written by Søren Hein.
 
   Many thanks for allowing me to include it in DDS.
 

   It is a simple object for AB statistics and return values.
*/




#include "dds.h"

#include "ABstats.h"




ABstats::ABstats()

{
  sprintf(fname, "");

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




void ABstats::SetFile(char * fname)

{

  if (strlen(fname) > DDS_LINE_LEN)

    return;


  
if (fp != stdout)
 // Already set

    return;



  strncpy(this->fname, fname, strlen(fname));


  fp = fopen(fname, "w");

  if (! fp)

    fp = stdout;

}




void ABstats::SetName(int no, char * name)

{

  if (no < 0 || no >= DDS_AB_POS)

    return;

  
  sprintf(this->name[no], "%s\0", name);

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
	
 100. * sumScore1 / (double) s,
	
 psumScore1 / (double) s,
	
 scoreCum[1],
   
      100. * scoreCum[1] / (double) cs,

      	 pscoreCum[1] / (double) cs);



    fprintf(fp, "%2s %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n\n",

         "",

        "Side0",

        sumScore0,

        100. * sumScore0 / (double) s,

        psumScore0 / (double) s,

        scoreCum[0],
  
      100. * scoreCum[0] / (double) cs,
 
        pscoreCum[0] / (double) cs);



    for (int p = 0; p < DDS_AB_POS; p++)

    {

      if (sum[p])

      {

        fprintf(fp, "%2d %-20s %8d %5.1f %5.1f %8d %5.1f %5.1f\n",

	   p,
 
          name[p],
 
          sum[p],

	   100. * sum[p] / (double) s,

	   psum[p] / (double) sum[p],

	   counterCum[p],

	   100. * counterCum[p] / (double) cs,

	   pcounterCum[p] / (double) counterCum[p]);

      }

      else if (counterCum[p])

      {

        fprintf(fp, "%2d %-20s %8d %5.1f %5s %8d %5.1f %5.1f\n",

	   p,
 
          name[p],
 
          sum[p],

	   100. * sum[p] / (double) s,

	   "",

	   counterCum[p],

	   100. * counterCum[p] / (double) cs,

	   pcounterCum[p] / (double) counterCum[p]);

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
 
      100. * nodesCum[d] / (double) allnodesCum,

       100. * c / (double) allnodesCum);



    // "Branching factor" from end of one trick to end of

    // the previous trick.

    if ((d % 4 == 1) && 

        (d + 4 < DDS_MAXDEPTH) &&
 
	(nodesCum[d+4] > 0))

      fprintf(fp, "  %5.2f",

        nodesCum[d] / (double) nodesCum[d+4]);


    fprintf(fp, "\n");

  }


  fprintf(fp, "\n%-5s  %6d  %6d\n",
 
    "Total", allnodes, allnodesCum);



  if (allnodes)

  {

    fprintf(fp, "%-5s  %6.1f  %6.1f\n",
 
      "d_avg",
 
      np / (double) allnodes,
 
      ncp / (double) allnodesCum);

  }

  else if (allnodesCum)

  {

    fprintf(fp, "\n%-5s  %6s  %6.1f\n",
 
      "Avg",
 
      "-",

      ncp / (double) allnodesCum);

  }



  fprintf(fp, "%-5s  %6d\n\n\n", "Diff", allnodes - sumScore1 - sumScore0);

  

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
