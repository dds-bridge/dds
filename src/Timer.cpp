/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


/*
   See Timer.h for some description.
*/

#include <stdexcept>

#include "dds.h"
#include "Timer.h"


const char * TIMER_NAMES[TIMER_GROUPS] =
{
  "AB",
  "Make",
  "Undo",
  "Evaluate",
  "NextMove",
  "QuickTricks",
  "LaterTricks",
  "MoveGen",
  "Lookup",
  "Build"
};


Timer::Timer()
{
  Timer::Reset();
}


Timer::~Timer()
{
  if (fp != stdout && fp != nullptr)
    fclose(fp);
}


void Timer::Reset()
{
  strcpy(fname, "");

  for (int i = 0; i < DDS_TIMERS; i++)
  {
    sprintf(name[i], "Timer %4d", i);
    count[i] = 0;
    userCum[i] = 0;
    systCum[i] = 0.;
  }
}


void Timer::SetFile(char * ourFname)
{
  if (strlen(ourFname) > DDS_FNAME_LEN)
    return;

  strncpy(fname, ourFname, strlen(ourFname));

  fp = fopen(fname, "w");
  if (!fp)
    fp = stdout;
}


void Timer::SetName(int no, char * ourName)
{
  if (no < 0 || no >= DDS_TIMERS)
    return;

  sprintf(name[no], "%s", ourName);
}


void Timer::SetNames()
{
  char tag[LINE_LEN];

  for (int d = 0; d < TIMER_SPACING; d++)
  {
    int c = d % 4;
    sprintf(tag, "AB%d %d", c, d);
    Timer::SetName(TIMER_AB + d, tag);

    for (int n = 1; n < TIMER_GROUPS; n++)
    {
      sprintf(tag, "%s %d", TIMER_NAMES[n], d);
      Timer::SetName(n * TIMER_SPACING + d, tag);
    }
  }
}

void Timer::Start(int no)
{
  if (no < 0 || no >= DDS_TIMERS)
    return;

  systTimes0[no] = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&userTimes0[no]);
#else
  gettimeofday(&userTimes0[no], nullptr);
#endif
}


void Timer::End(int no)
{
  if (no < 0 || no >= DDS_TIMERS)
    return;

  systTimes1[no] = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&userTimes1[no]);
  int timeUser = static_cast<int>
                 (userTimes1[no].QuadPart - userTimes0[no].QuadPart);
#else
  gettimeofday(&userTimes1[no], nullptr);
  int timeUser = Timer::TimevalDiff(userTimes1[no],
                                    userTimes0[no]);
#endif

  count[no]++;

  // This is more or less in milli-seconds except on Windows,
  // where it is in "wall ticks". It is possible to convert
  // to milli-seconds, but the resolution is so poor for fast
  // functions that I leave it in integer form.

  userCum[no] += timeUser;
  systCum[no] += systTimes1[no] - systTimes0[no];
}


void Timer::OutputDetails()
{
  fprintf(fp, "%-14s %10s %10s %10s %10s %10s\n",
          "Name",
          "Number",
          "User ticks",
          "Avg",
          "System",
          "Avg ms");

  for (int no = 0; no < DDS_TIMERS; no++)
  {
    if (count[no] == 0)
      continue;

    fprintf(fp, "%-14s %10d %10lld %10.2f %10.0f %10.2f\n",
            name[no],
            count[no],
            userCum[no],
            static_cast<double>(userCum[no]) /
            static_cast<double>(count[no]),
            1000. * systCum[no],
            1000. * static_cast<double>(systCum[no]) /
            static_cast<double>(count[no]));
  }
  fprintf(fp, "\n");
}


void Timer::PrintStats()
{
  bool used = false;
  for (int no = 0; no < DDS_TIMERS; no++)
  {
    if (count[no])
    {
      used = true;
      break;
    }
  }

  if (! used)
    return;

  // Approximate the exclusive times of each function.
  // The ABsearch*() functions are recursively nested,
  // so subtract out the one below.
  // The other ones are subtracted out based on knowledge
  // of the functions.

  __int64 AB_userCum[TIMER_SPACING];
  double AB_systCum[TIMER_SPACING];

  AB_userCum[0] = userCum[0];
  AB_systCum[0] = systCum[0];

  __int64 AB_ct = count[0];
  __int64 AB_ucum = AB_userCum[0];
  double AB_scum = AB_systCum[0];

  for (int d = 1; d < TIMER_SPACING; d++)
  {
    AB_ct += count[d];

    if (userCum[d - 1] > userCum[d])
    {
      AB_userCum[d] = 0;
      AB_systCum[d] = 0;
      continue;
    }

    AB_userCum[d] = userCum[d] - userCum[d - 1];
    AB_systCum[d] = systCum[d] - systCum[d - 1];

    for (int no = 1; no < TIMER_GROUPS; no++)
    {
      int offset = no * TIMER_SPACING;
      AB_userCum[d] -= userCum[offset + d ];
      AB_systCum[d] -= systCum[offset + d ];
    }

    AB_ucum += AB_userCum[d];
    AB_scum += AB_systCum[d];
  }

  __int64 all_ucum = AB_ucum;
  double all_scum = AB_scum;
  for (int no = TIMER_SPACING; no < DDS_TIMERS; no++)
  {
    all_ucum += userCum[no];
    all_scum += systCum[no];
  }

  fprintf(fp, "%-14s %8s %10s %6s %4s %10s %6s %4s\n",
          "Name",
          "Count",
          "User",
          "Avg",
          "%",
          "Syst",
          "Avg",
          "%");

  if (AB_ct)
  {
    fprintf(fp, "%-14s %8lld %10lld %6.2f %4.1f %10.0f %6.2f %4.1f\n",
            TIMER_NAMES[0],
            AB_ct,
            AB_ucum,
            AB_ucum / static_cast<double>(AB_ct),
            100. * AB_ucum / all_ucum,
            1000. * AB_scum,
            1000. * AB_scum / static_cast<double>(AB_ct),
            100. * AB_scum / all_scum);
  }
  else
  {
    fprintf(fp, "%-14s %8lld %10lld %6s %4s %10.0f %6s %4s\n",
            TIMER_NAMES[0],
            AB_ct,
            AB_ucum,
            "-",
            "-",
            1000. * AB_scum,
            "-",
            "-");
  }

  __int64 ct[TIMER_GROUPS];
  for (int no = 1; no < TIMER_GROUPS; no++)
  {
    int offset = no * TIMER_SPACING;

    __int64 ucum = 0;
    double scum = 0;
    ct[no] = 0;

    for (int d = 0; d < TIMER_SPACING; d++)
    {
      ct[no] += count[offset + d];
      ucum += userCum[offset + d];
      scum += systCum[offset + d];
    }

    if (ct[no])
    {
      fprintf(fp, "%-14s %8lld %10lld %6.2f %4.1f %10.0f %6.2f %4.1f\n",
              TIMER_NAMES[no],
              ct[no],
              ucum,
              ucum / static_cast<double>(ct[no]),
              100. * ucum / all_ucum,
              1000. * scum,
              1000. * scum / static_cast<double>(ct[no]),
              100. * scum / all_scum);
    }
    else
    {
      fprintf(fp, "%-14s %8lld %10lld %6s %4s %10.0f %6s %4s\n",
              TIMER_NAMES[no],
              ct[no],
              ucum,
              "-",
              "-",
              1000. * scum,
              "-",
              "-");
    }
  }
  fprintf(fp, "----------------------------------");
  fprintf(fp, "-----------------------------------\n");
  fprintf(fp, "%-14s %8s %10lld %6s %4s %10.0f\n\n\n",
          "Sum",
          "",
          all_ucum,
          "",
          "",
          1000. * all_scum);

  // This doesn't work properly yet. The issue is that on some
  // loops there is no success, and in that case we have to try
  // all moves to see this. But no move ordering could have done
  // better. So we need to know the proportion for the successes
  // only. This probably becomes more natural when there is a Move
  // object. It doesn't really belong in Timer anyway.

  //double genMoves = ct[TIMER_NO_MOVEGEN] + ct[TIMER_NO_NEXTMOVE];
  //if (genMoves)
  //fprintf(fp, "Move generation quality %.1f%%\n\n\n",
  //100. * (1. - ct[TIMER_NO_MAKE] / genMoves));

  if (AB_ucum > 0. && AB_scum > 0.)
  {
    fprintf(fp, "%-14s %8s %10s %6s %4s %10s %6s %4s\n",
            "Name",
            "Count",
            "User",
            "Avg",
            "%",
            "Syst",
            "Avg",
            "%");

    for (int no = TIMER_SPACING - 1; no >= 0; no--)
    {
      if (count[no] == 0)
        continue;

      fprintf(fp, "%-14s %8d %10lld %6.2f %4.1f %10.0f %6.2f %4.1f\n",
              name[TIMER_AB + no],
              count[no],
              AB_userCum[no],
              AB_userCum[no] / static_cast<double>(count[no]),
              100. * AB_userCum[no] / AB_ucum,
              1000. * AB_systCum[no],
              1000. * AB_systCum[no] / static_cast<double>(count[no]),
              100. * AB_systCum[no] / static_cast<double>(AB_scum));

    }
    fprintf(fp, "----------------------------------");
    fprintf(fp, "-----------------------------------\n");
    fprintf(fp, "%-14s %8lld %10lld %6s %4s %10.0f\n\n\n",
            "Sum",
            AB_ct,
            AB_ucum,
            "",
            "",
            1000. * AB_scum);
  }

#ifdef DDS_TIMING_DETAILS
  Timer::OutputDetails();
#endif
}


int Timer::TimevalDiff(timeval x, timeval y)
{
  return 1000 * (x.tv_sec - y.tv_sec )
         + (x.tv_usec - y.tv_usec) / 1000;
}
