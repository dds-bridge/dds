/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


int ConvertFromPBN(
  char			* dealBuff,
  unsigned int		remainCards[DDS_HANDS][DDS_SUITS]);

int ConvertPlayFromPBN(
  struct playTracePBN	* playPBN,
  struct playTraceBin	* playBin);

