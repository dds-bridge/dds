/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SYSTEM_H
#define DDS_SYSTEM_H

/*
   This class encapsulates all(?) the system-dependent stuff.
 */

#include <string>
#include <vector>

#include "dds.h"

using namespace std;

#define DDS_SYSTEM_SOLVE 0
#define DDS_SYSTEM_CALC 1
#define DDS_SYSTEM_PLAY 2
#define DDS_SYSTEM_SIZE 3

typedef void (*fptrType)(const int thid);


class System
{
  private:

    unsigned runCat; // SOLVE / CALC / PLAY

    int numThreads;

    unsigned preferredSystem;

    vector<bool> availableSystem;

    vector<fptrType> CallbackSimpleList;
    vector<fptrType> CallbackComplexList;

    typedef int (System::*InitPtr)();
    vector<InitPtr> InitPtrList;

    typedef int (System::*RunPtr)();
    vector<RunPtr> RunPtrList;

    fptrType fptr;

#ifdef _MSC_VER
    HANDLE solveAllEvents[MAXNOOFTHREADS];
    LONG threadIndex;
#endif

    int InitThreadsBasic();
    int InitThreadsBoost();
    int InitThreadsOpenMP();
    int InitThreadsGCD();
    int InitThreadsWinAPI();
  
    int RunThreadsBasic();
    int RunThreadsBoost();
    int RunThreadsOpenMP();
    int RunThreadsGCD();
    int RunThreadsWinAPI();


  public:
    System();

    ~System();

    void Reset();

    int Register(
      const unsigned code,
      const int noOfThreads = 1);

    int PreferThreading(const unsigned code);

    int InitThreads();

    int RunThreads(const int chunkSize);

    string str(DDSInfo * info) const;
};

#endif
