/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// General initialization of three hands to be used in examples.

extern int trump[3];
extern int first[3];
extern int dealer[3];
extern int vul[3];

extern char PBN[3][80];

extern unsigned int holdings[3][4][4];

extern int playNo[3];

extern char play[3][106];
extern int playSuit[3][52];
extern int playRank[3][52];


void PrintFut(char title[], futureTricks * fut);
void equals_to_string(int equals, char * res);
bool CompareFut(futureTricks * fut, int handno, int solutions);

void SetTable(ddTableResults * table, int handno);
bool CompareTable(ddTableResults * table, int handno);
void PrintTable(ddTableResults * table);

bool ComparePar(parResults * par, int handno);
bool CompareDealerPar(parResultsDealer * par, int handno);
void PrintPar(parResults * par);
void PrintDealerPar(parResultsDealer * par);

bool ComparePlay(solvedPlay * trace, int handno);
void PrintBinPlay(playTraceBin * play, solvedPlay * solved);
void PrintPBNPlay(playTracePBN * play, solvedPlay * solved);


void PrintHand(char title[], 
  unsigned int rankInSuit[DDS_HANDS][DDS_SUITS]);

void PrintPBNHand(char title[], char remainCards[]);

int ConvertPBN(char * dealBuff,
  unsigned int remainCards[DDS_HANDS][DDS_SUITS]);

int IsACard(char cardChar);

