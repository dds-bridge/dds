/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_PBN_H
#define DDS_PBN_H

#include "../include/dll.h"


int ConvertFromPBN(
  char const * dealBuff,
  unsigned int remainCards[DDS_HANDS][DDS_SUITS]);

int ConvertPlayFromPBN(
  const playTracePBN& playPBN,
  playTraceBin& playBin);

#endif
