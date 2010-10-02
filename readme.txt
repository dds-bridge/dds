DDS 2.1.1,  Bo Haglund 2010-08-02

For Win32, DDS compiles with Visual C++ 2005 Express edition 
and the Mingw port of gcc.

When using Visual C++, the statement
#include "stdafx.h" at the beginning of dds.cpp must be uncommented.

When not using Visual C++, the compilation of DDS includes function CalcDDtable
implemented using GCC/MingW OpenMP. When OpenMP is compiled with MingW and Windows,
then for the program using dds.dll, the file phreadGC2.dll must be placed in 
the same folder as dds.dll.
   

Linking with an application using DDS
-------------------------------------
The InitStart() function of DDS should be called from inside the
application. Then SolveBoard in DDS can be called multiple times
without the overhead of InitStart() at each call.
For this purpose, the application code must have an include
statement for the dll.h file in DDS.


Maximum number of threads
------------------------- 
The maximum number of simultaneous threads depends on the PC physical memory size:
1 GB or less, max 4 threads.
2, 3 or 4 GB, max 8 threads.
More than 4 GB, max 16 threads.
 
For Windows, allocating memory for the maximum number of simultaneous threads can 
be done by reading out the physical memory size from Windows. This is done in the DDS.DLL.
Another alternative is to provide the physical memory size as a parameter (gb_ram) in the
InitStart call. This alternative needs to be used when the operating system is not Windows.


Setting the number of simultaneous threads when calling CalcDDtable.
--------------------------------------------------------------------
For Windows, this can be done either by reading out the number of processor cores from
Windows and using this for setting the number of threads, or by supplying the number of
threads (ncores) in InitStart. This latter alternative needs to be used when the operating 
system is not Windows. 
 

Options at DDS compilation
--------------------------
There are 3 compiling options:

1) Compilation without definitions of STAT or TTDEBUG.
This is the default case given in the distributed source
where definitions of STAT and TTDEBUG are commented away.
This compilation alternative gives the best performance.

2) Compilation with definition STAT.
Uncomment the definition of STAT.
This gives deal search statistics information for each call
to SolveBoard, logged in stat.txt. Set target to a number of
tricks to be won by player's side and solutions=1
(avoid to set target to -1 and solutions different than 1,
because then the statistics output will be mixed for the
different repeated searches).

"score statistics" provides information on which depth the search
terminated and some termination information as described below. 
(Only a few searches terminate at the last trick.)

s1 = number of TRUE score values found
s0 = number of FALSE score values found
c1 = number of cutoffs because won tricks by player side >= target
c2 = number of cutoffs because won tricks by player side plus
     tricks left to play < target
c3 = as for c1 but with quick tricks evaluation
c4 = as for c2 but with quick tricks evaluation
c5 = number of transposition table cutoffs for player's (MAX) side 
c6 = number of transposition table cutoffs for opponent's (MIN) side
c7 = number of terminations at the last trick (i.e. when depth is 0)
c8 = number of cases when a new transposition table entry was added


3) Compilation with definition TTDEBUG.
Uncomment the definition of TTDEBUG.
Note that setting this option will make the double dummy solver very
slow!
This is indended for debugging of the transposition table logic.
Without this support, it can be extremely hard to find the reason for
a transposition table erroneous behaviour.
Set target to a number of tricks to be won by player's side and 
solutions=1 (not using target -1 and solutions different than 1).
Information of a number of transposition table hits will then be
recorded in the file rectt.txt. The number of recorded hits is defined
by SEARCHSIZE. Also, hit information will be stored in a structure
ttStore.
All transposition table entries generated during the time when transposition 
table hits are recorded, are logged in the file storett.txt.
After SolveBoard terminates, the following code snippet 
(or something similar) is called:


int k, m, val, h, s, iniDepth;
int lastTTstore, suppressTTlog;
struct moveType startMoves[3];
struct ttStoreType *ttStore;
struct nodeCardsType * cardsP;
struct pos thisPos;
FILE *fp;


fp=fopen("recpos.txt", "w");
suppressTTlog=TRUE;
for (k=0; k<=lastTTstore-1; k++) {
  for (m=0; m<=2; m++) {
    startMoves[m].suit=-1;
    startMoves[m].rank=-1;
  }
  for (h=0; h<=3; h++)
    for (s=0; s<=3; s++)
      thisPos.rankInSuit[h][s]=ttStore[k].suit[h][s];
  thisPos.handRelFirst=0;
  iniDepth=4 * ttStore[k].tricksLeft-4;
  thisPos.first[iniDepth-1]=ttStore[k].first;
  InitSearch(&thisPos, iniDepth, startMoves, ttStore[k].first, 0);
  val=ABsearch(&thisPos, ttStore[k].target, iniDepth);
  if (val==tt[k].scoreFlag)
    fprintf(fp, "No %d is OK\n", k);
  else {
    fprintf(fp, "No %d is not OK\n", k);
    fprintf(fp, "cardsP=%d\n", 
      ttStore[k].cardsP);
  }      
} 
fclose(fp);
suppressTTlog=FALSE; 


A "not OK" row number in file recpos.txt corresponds to an 
entry with the same number for lastTTstore in the file rectt.txt.
rectt.txt contain the cards and other information related to
the position getting the transposition table hit. It also contains
the pointer to the SOP (Subsets Of Positions) node, cardsP.
The SOP pointer cardsP value can be found in the storett.txt file, where
also the winning cards and other information of the transposition
table entry can be found.
By now comparing the information in rectt.txt with the information
in storett.txt for the same SOP node pointer, it is usually not too
difficult to deduce what caused the problem.   

    


