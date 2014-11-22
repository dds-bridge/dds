/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <stdexcept>

#include "dds.h"

struct par_suits_type {
  int suit;
  int tricks;
  int score;
};

struct best_par_type {
   int par_denom;
   int par_tricks;
}; 

struct parContr2Type {
  char contracts[10];
  int denom;
};

int stat_contr[5]={0,0,0,0,0};
const int max_low[3][8] = {{0,0,1,0,1,2,0,0},{0,0,1,2,0,1,0,0},{0,0,1,2,3,0,0,0}};  /* index 1: 0=NT, 1=Major, 2=Minor  index 2: contract level 1-7 */


int STDCALL CalcParPBN(ddTableDealPBN tableDealPBN, 
  ddTableResults * tablep, int vulnerable, parResults *presp) {
  int res;
  ddTableDeal tableDeal;
  int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]);
  int STDCALL CalcPar(ddTableDeal tableDeal, int vulnerable, 
    ddTableResults * tablep, parResults *presp);

  if (ConvertFromPBN(tableDealPBN.cards, tableDeal.cards)!=1)
    return RETURN_PBN_FAULT;

  res=CalcPar(tableDeal, vulnerable, tablep, presp);

  return res;
}

#ifdef DEALER_PAR_ENGINE_ONLY

int STDCALL Par(ddTableResults * tablep, parResults *presp, 
	int vulnerable) {
       /* vulnerable 0: None  1: Both  2: NS  3: EW */

  int res, i, k, m;
  parResultsDealer sidesRes[2];
  parContr2Type parContr2[10];
  
  int CalcMultiContracts(int max_lower, int tricks);

  res = SidesPar(tablep, sidesRes, vulnerable);
  if (res != 1)
    return res;
 
  for (k=0; k<16; k++) {
    presp->parScore[0][k]='\0';
    presp->parScore[1][k]='\0';
  }

  sprintf(presp->parScore[0], "NS %d", sidesRes[0].score);
  sprintf(presp->parScore[1], "EW %d", sidesRes[1].score);

  for (k=0; k<128; k++) {
    presp->parContractsString[0][k] = '\0';
    presp->parContractsString[1][k] = '\0';
  }

  strcat(presp->parContractsString[0], "NS:");
  strcat(presp->parContractsString[1], "EW:");

  char one_contr[4];
  for ( m=0; m<3; m++)
    one_contr[m]='\0';

  char strain_contr[2] = {'0','\0'};

  for (i=0; i<2; i++) { 

    if (sidesRes[i].score == 0) 
      continue;

    if (sidesRes[i].contracts[0][2] == '*') {
      /* Sacrifice */  

      for (k = 0;  k<sidesRes[i].number; k++) {

        for (int u=0; u<10; u++)
          parContr2[k].contracts[u]=sidesRes[i].contracts[k][u];          

        if (sidesRes[i].contracts[k][1]=='N')
	  parContr2[k].denom=0;
	else if (sidesRes[i].contracts[k][1]=='S')
	  parContr2[k].denom=1;
        else if (sidesRes[i].contracts[k][1]=='H')
	  parContr2[k].denom=2;
	else if (sidesRes[i].contracts[k][1]=='D')
	  parContr2[k].denom=3;
	else if (sidesRes[i].contracts[k][1]=='C')
	  parContr2[k].denom=4;
      }

      for (int s = 1; s < sidesRes[i].number; s++) {
        parContr2Type tmp = parContr2[s]; 
        int r = s; 
        for (; r && tmp.denom < parContr2[r - 1].denom ; --r) 
          parContr2[r] = parContr2[r - 1]; 
        parContr2[r] = tmp; 
      }


      for (int t=0; t<sidesRes[i].number; t++) {

	if (t != 0)
	  strcat(presp->parContractsString[i], ",");
  
	if (parContr2[t].contracts[5] == 'W')
	  strcat(presp->parContractsString[i], "EW ");
	else if (parContr2[t].contracts[5] == 'S')   
          strcat(presp->parContractsString[i], "NS ");
	else {
	  switch (parContr2[t].contracts[4]) {
	    case 'N': strcat(presp->parContractsString[i], "N "); break;
            case 'S': strcat(presp->parContractsString[i], "S "); break;
	    case 'E': strcat(presp->parContractsString[i], "E "); break;
	    case 'W': strcat(presp->parContractsString[i], "W "); break;
          }
        }
	for (m=0; m<2; m++)
	  one_contr[m] = parContr2[t].contracts[m];
	one_contr[2] = 'x';
	one_contr[3] = '\0';
	strcat(presp->parContractsString[i], one_contr); 
      }

    }
    else {
	/* Contract(s) make */

      char levels_coll[12];
      for (m=0; m<12; m++)
        levels_coll[m]='\0';

      for (k = 0; k<sidesRes[i].number; k++) {
	for (int u = 0; u<10; u++)
	  parContr2[k].contracts[u] = sidesRes[i].contracts[k][u];

	if (sidesRes[i].contracts[k][1] == 'N')
	  parContr2[k].denom = 0;
	else if (sidesRes[i].contracts[k][1] == 'S')
	  parContr2[k].denom = 1;
	else if (sidesRes[i].contracts[k][1] == 'H')
	  parContr2[k].denom = 2;
	else if (sidesRes[i].contracts[k][1] == 'D')
	  parContr2[k].denom = 3;
	else if (sidesRes[i].contracts[k][1] == 'C')
	  parContr2[k].denom = 4;
      }

      for (int s = 1; s < sidesRes[i].number; s++) {
	parContr2Type tmp = parContr2[s];
	int r = s;
	for (; r && tmp.denom < parContr2[r - 1].denom; --r)
	  parContr2[r] = parContr2[r - 1];
	parContr2[r] = tmp;
      }

      for (int t = 0; t<sidesRes[i].number; t++) {
	if (t != 0)
	  strcat(presp->parContractsString[i], ",");

        if (parContr2[t].contracts[4] == 'W')
	  strcat(presp->parContractsString[i], "EW ");
	else if (parContr2[t].contracts[4] == 'S')
          strcat(presp->parContractsString[i], "NS ");
	else {
	  switch (parContr2[t].contracts[3]) {
	    case 'N': strcat(presp->parContractsString[i], "N "); break;
            case 'S': strcat(presp->parContractsString[i], "S "); break;
	    case 'E': strcat(presp->parContractsString[i], "E "); break;
	    case 'W': strcat(presp->parContractsString[i], "W "); break;
          }
        }

	for (m=0; m<2; m++)
	  one_contr[m] = parContr2[t].contracts[m];
	one_contr[2]='\0';

	strain_contr[0]=one_contr[1];

	char * ptr_c = strchr(parContr2[t].contracts, '+');
	if (ptr_c != nullptr) {
	  ptr_c++;
	  int add_contr = (*ptr_c) - 48;


          sprintf(levels_coll, "%d", 
		CalcMultiContracts(add_contr, 
		(parContr2[t].contracts[0] - 48) + 6 + add_contr));

	  strcat(presp->parContractsString[i], levels_coll);
	  strcat(presp->parContractsString[i], strain_contr);
	  
        }
	else {
	  strcat(presp->parContractsString[i], one_contr);
	
	}
      }
    }
  }

  return res;
}

#else

int rawscore(
  int denom, 
  int tricks, 
  int isvul);

void IniSidesString(
  int dr, 
  int i, 
  int t1, 
  int t2, 
  char stri[]);

int CalcMultiContracts(
  int max_lower, 
  int tricks);

int VulnerDefSide(
  int side, 
  int vulnerable);

int STDCALL Par(ddTableResults * tablep, parResults *presp, 
	int vulnerable) {
       /* vulnerable 0: None  1: Both  2: NS  3: EW */

	/* The code for calculation of par score / contracts is based upon the
	perl code written by Matthew Kidd ACBLmerge. He has kindly given me permission
	to include a C++ adaptation in DDS. */

	/* The Par function computes the par result and contracts. */


  int denom_conv[5] = { 4, 0, 1, 2, 3 };
  /* Preallocate for efficiency. These hold result from last direction
     (N-S or E-W) examined. */
  int i, j, k, m, isvul;
  int current_side, both_sides_once_flag, denom_max = 0, max_lower;
  int new_score_flag, sc1, sc2, sc3;
  int prev_par_denom = 0, prev_par_tricks = 0;
  int denom_filter[5] = { 0, 0, 0, 0, 0 };
  int no_filtered[2] = { 0, 0 };
  int no_of_denom[2];
  int best_par_score[2];
  int best_par_sacut[2];
  best_par_type best_par[5][2];	/* 1st index order number. */

  int ut = 0, t1, t2, tt, score, dr, tu, tu_max, t3[5], t4[5], n;
  par_suits_type par_suits[5];
  char contr_sep[2] = { ',', '\0' };
  char temp[8], buff[4];

  int par_denom[2] = { -1, -1 };	 /* 0-4 = NT,S,H,D,C */
  int par_tricks[2] = { 6, 6 };	 /* Initial "contract" beats 0 NT */
  int par_score[2] = { 0, 0 };
  int par_sacut[2] = { 0, 0 };     /* Undertricks for sacrifice (0 if not sac) */


  /* Find best par result for N-S (i==0) or E-W (i==1). These will
     nearly always be the same, but when we have a "hot" situation
     they will not be. */

  for (i = 0; i <= 1; i++) {
  /* Start with the with the offensive side (current_side = 0) and alternate
  between sides seeking the to improve the result for the current side.*/

    no_filtered[i] = 0;
    for (m = 0; m <= 4; m++)
      denom_filter[m] = 0;

    current_side = 0;  both_sides_once_flag = 0;
    while (1) {

    /* Find best contract for current side that beats current contract.
    Choose highest contract if results are equal. */

      k = (i + current_side) % 2;

      isvul = ((vulnerable == 1) || (k ? (vulnerable == 3) : (vulnerable == 2)));

      new_score_flag = 0;
      prev_par_denom = par_denom[i];
      prev_par_tricks = par_tricks[i];

      /* Calculate tricks and score values and
      store them for each denomination in structure par_suits[5]. */

      n = 0;
      for (j = 0; j <= 4; j++) {
        if (denom_filter[j] == 0) {
	  /* Current denomination is not filtered out. */
	  t1 = k ? tablep->resTable[denom_conv[j]][1] : tablep->resTable[denom_conv[j]][0];
	  t2 = k ? tablep->resTable[denom_conv[j]][3] : tablep->resTable[denom_conv[j]][2];
	  tt = Max(t1, t2);
	  /* tt is the maximum number of tricks current side can take in
	  denomination.*/

	  par_suits[n].suit = j;
	  par_suits[n].tricks = tt;

	  if ((tt > par_tricks[i]) || ((tt == par_tricks[i]) &&
	     (j < par_denom[i])))
	    par_suits[n].score = rawscore(j, tt, isvul);
	  else
	    par_suits[n].score = rawscore(-1, prev_par_tricks - tt, isvul);
	  n++;
	}
      }

      /* Sort the items in the par_suits structure with decreasing order of the
	 values on the scores. */

      for (int s = 1; s < n; s++) {
	par_suits_type tmp = par_suits[s];
	int r = s;
	for (; r && tmp.score > par_suits[r - 1].score; --r)
	  par_suits[r] = par_suits[r - 1];
	par_suits[r] = tmp;
      }

      /* Do the iteration as before but now in the order of the sorted denominations. */

      for (m = 0; m<n; m++) {
	j = par_suits[m].suit;
	tt = par_suits[m].tricks;

	if ((tt > par_tricks[i]) || ((tt == par_tricks[i]) &&
	   (j < par_denom[i]))) {
	   /* Can bid higher and make contract.*/
	  score = rawscore(j, tt, isvul);
	}
	else {
	 /* Bidding higher in this denomination will not beat previous denomination
	    and may be a sacrifice. */
	  ut = prev_par_tricks - tt;
	  if (j >= prev_par_denom) {
	    /* Sacrifices higher than 7N are not permitted (but long ago
	       the official rules did not prohibit bidding higher than 7N!) */
	    if (prev_par_tricks == 13)
	      continue;
	    /* It will be necessary to bid one level higher, resulting in
             one more undertrick. */
	    ut++;
	  }
	  /* Not a sacrifice (due to par_tricks > prev_par_tricks) */
	  if (ut <= 0)
	    continue;
	  /* Compute sacrifice.*/
	  score = rawscore(-1, ut, isvul);
	}

	if (current_side == 1)
	  score = -score;

	if (((current_side == 0) && (score > par_score[i])) ||
	   ((current_side == 1) && (score < par_score[i]))) {
	  new_score_flag = 1;
	  par_score[i] = score;
	  par_denom[i] = j;

	  if (((current_side == 0) && (score > 0)) ||
		 ((current_side == 1) && (score < 0))) {
	    /* New par score from a making contract.
	    Can immediately update since score at same level in higher
	    ranking suit is always >= score in lower ranking suit and
	    better than any sacrifice. */

	    par_tricks[i] = tt;
	    par_sacut[i] = 0;
	  }
	  else {
	    par_tricks[i] = tt + ut;
	    par_sacut[i] = ut;
	  }
	}
      }


      if (!new_score_flag && both_sides_once_flag) {
	if (no_filtered[i] == 0) {
	  best_par_score[i] = par_score[i];
	  best_par_sacut[i] = par_sacut[i];
	  no_of_denom[i] = 0;
	}
	else if (best_par_score[i] != par_score[i])
	  break;
	if (no_filtered[i] >= 5)
	  break;
	denom_filter[par_denom[i]] = 1;
	no_filtered[i]++;
	best_par[no_of_denom[i]][i].par_denom = par_denom[i];
	best_par[no_of_denom[i]][i].par_tricks = par_tricks[i];
	no_of_denom[i]++;
	both_sides_once_flag = 0;
	current_side = 0;
	par_denom[i] = -1;
	par_tricks[i] = 6;
	par_score[i] = 0;
	par_sacut[i] = 0;     
      }
      else {
	both_sides_once_flag = 1;
	current_side = 1 - current_side;
      }
    }
  }

  presp->parScore[0][0] = 'N';
  presp->parScore[0][1] = 'S';
  presp->parScore[0][2] = ' ';
  presp->parScore[0][3] = '\0';
  presp->parScore[1][0] = 'E';
  presp->parScore[1][1] = 'W';
  presp->parScore[1][2] = ' ';
  presp->parScore[1][3] = '\0';

  sprintf(temp, "%d", best_par_score[0]);
  strcat(presp->parScore[0], temp);
  sprintf(temp, "%d", best_par_score[1]);
  strcat(presp->parScore[1], temp);

  presp->parContractsString[0][0] = 'N';
  presp->parContractsString[0][1] = 'S';
  presp->parContractsString[0][2] = ':';
  presp->parContractsString[0][3] = '\0';
  presp->parContractsString[1][0] = 'E';
  presp->parContractsString[1][1] = 'W';
  presp->parContractsString[1][2] = ':';
  presp->parContractsString[1][3] = '\0';      

  if (best_par_score[0] == 0) {
    /* Neither side can make anything.*/
    return RETURN_NO_FAULT;
  }

  for (i = 0; i <= 1; i++) {

    if (best_par_sacut[i] > 0) {
      /* Sacrifice */
      dr = (best_par_score[i] > 0) ? 0 : 1;
      /* Sort the items in the best_par structure with increasing order of the
      values on denom. */
				
      for (int s = 1; s < no_of_denom[i]; s++) {
	best_par_type tmp = best_par[s][i];
	int r = s;
	for (; r && tmp.par_denom < best_par[r - 1][i].par_denom; --r)
	   best_par[r][i] = best_par[r - 1][i];
	best_par[r][i] = tmp;
      }

      for (m = 0; m < no_of_denom[i]; m++) {

	j = best_par[m][i].par_denom;

	t1 = ((dr + i) % 2) ? tablep->resTable[denom_conv[j]][0] : tablep->resTable[denom_conv[j]][1];
	t2 = ((dr + i) % 2) ? tablep->resTable[denom_conv[j]][2] : tablep->resTable[denom_conv[j]][3];
	tt = (t1 > t2) ? t1 : t2;

	IniSidesString(dr, i, t1, t2, buff);

	if (presp->parContractsString[i][3] != '\0')
	   strcat(presp->parContractsString[i], contr_sep);

	strcat(presp->parContractsString[i], buff);
	sprintf(temp, "%d", best_par[m][i].par_tricks - 6);
	buff[0] = static_cast<char>(cardSuit[denom_conv[j]]);
	buff[1] = 'x';
	buff[2] = '\0';
	strcat(temp, buff);
	strcat(presp->parContractsString[i], temp);
      }
    }
    else {
      /* Par contract is a makeable contract.*/

      dr = (best_par_score[i] < 0) ? 0 : 1;

      tu_max = 0;
      for (m = 0; m <= 4; m++) {
	t3[m] = ((dr + i) % 2 == 0) ? tablep->resTable[denom_conv[m]][0] : tablep->resTable[denom_conv[m]][1];
	t4[m] = ((dr + i) % 2 == 0) ? tablep->resTable[denom_conv[m]][2] : tablep->resTable[denom_conv[m]][3];
	tu = (t3[m] > t4[m]) ? t3[m] : t4[m];
	if (tu > tu_max) {
	  tu_max = tu;
	  denom_max = m;  /* Lowest denomination if several denominations have max tricks. */
	}
      }

      for (m = 0; m < no_of_denom[i]; m++) {
	j = best_par[m][i].par_denom;

	t1 = ((dr + i) % 2) ? tablep->resTable[denom_conv[j]][0] : tablep->resTable[denom_conv[j]][1];
	t2 = ((dr + i) % 2) ? tablep->resTable[denom_conv[j]][2] : tablep->resTable[denom_conv[j]][3];
	tt = (t1 > t2) ? t1 : t2;

	IniSidesString(dr, i, t1, t2, buff);

	if (presp->parContractsString[i][3] != '\0')
          strcat(presp->parContractsString[i], contr_sep);

	strcat(presp->parContractsString[i], buff);

	if (denom_max < j)
	  max_lower = best_par[m][i].par_tricks - tu_max - 1;
	else
	  max_lower = best_par[m][i].par_tricks - tu_max;

	/* max_lower is the maximal contract lowering, otherwise opponent contract is
	higher. It is already known that par_score is high enough to make
	opponent sacrifices futile.
	To find the actual contract lowering allowed, it must be checked that the
	lowered contract still gets the score bonus points that is present in par score.*/

	sc2 = abs(best_par_score[i]);
	/* Score for making the tentative lower par contract. */
	while (max_lower > 0) {
	  if (denom_max < j)
	    sc1 = -rawscore(-1, best_par[m][i].par_tricks - max_lower - tu_max,
		VulnerDefSide(best_par_score[0]>0, vulnerable));
	  else
	    sc1 = -rawscore(-1, best_par[m][i].par_tricks - max_lower - tu_max + 1,
			VulnerDefSide(best_par_score[0] > 0, vulnerable));
	   /* Score for undertricks needed to beat the tentative lower par contract.*/

	  if (sc2 < sc1)
	    break;
	  else
	    max_lower--;
		
	  /* Tentative lower par contract must be 1 trick higher, since the cost
	  for the sacrifice is too small. */
	}

	int opp_tricks = Max(t3[j], t4[j]);

	while (max_lower > 0) {
	  sc3 = -rawscore(-1, best_par[m][i].par_tricks - max_lower - opp_tricks,
	     VulnerDefSide(best_par_score[0] > 0, vulnerable));

	  /* If opponents to side with par score start the bidding and has a sacrifice
	     in the par denom on the same trick level as implied by current max_lower,
	     then max_lower must be decremented. */

	  if ((sc2 > sc3) && (best_par_score[i] < 0))
	     /* Opposite side with best par score starts the bidding. */
	    max_lower--;
	  else
	    break;
	}

	switch (j) {
	  case 0:  k = 0; break;
	  case 1:  case 2: k = 1; break;
	  case 3:  case 4: k = 2; break;
	  default:
	    throw std::runtime_error("j not in [0..3] in Par");
	}

	max_lower = Min(max_low[k][best_par[m][i].par_tricks - 6], max_lower);

	n = CalcMultiContracts(max_lower, best_par[m][i].par_tricks);

	sprintf(temp, "%d", n);
	buff[0] = static_cast<char>(cardSuit[denom_conv[j]]);
	buff[1] = '\0';
	strcat(temp, buff);
	strcat(presp->parContractsString[i], temp);
      }
    }
  }

  return RETURN_NO_FAULT;
}


int rawscore(int denom, int tricks, int isvul) {
  int game_bonus, level, score;

  /* Computes score for undoubled contract or a doubled contract with
     for a given number of undertricks. These are the only possibilities
     for a par contract (aside from a passed out hand). 
  
     denom  - 0 = NT, 1 = Spades, 2 = Hearts, 3 = Diamonds, 4 = Clubs
             (same order as results from double dummy solver); -1 undertricks
     tricks - For making contracts (7-13); otherwise, number of undertricks.
     isvul  - True if vulnerable */

  if (denom==-1) {
    if (isvul)
      return -300 * tricks + 100;
    if (tricks<=3)
      return -200 * tricks + 100;
    return -300 * tricks + 400;
  }
  else {
    level=tricks-6;
    game_bonus=0;
    if (denom==0) {
      score=10 + 30 * level;
      if (level>=3)
	game_bonus=1;
    }
    else if ((denom==1)||(denom==2)) {
      score=30 * level;
      if (level>=4)
        game_bonus=1;
    }
    else {
      score=20 * level;
      if (level>=5)
	game_bonus=1;
    }
    if (game_bonus) {
      score+= (isvul ? 500 : 300);
    }
    else
      score+=50;

    if (level==6) {
      score+= (isvul ? 750 : 500);
    }
    else if (level==7) {
      score+= (isvul ? 1500 : 1000);
    }
  }

  return score;
}


void IniSidesString(int dr, int i, int t1, int t2, char stri[]) {

   if ((dr+i) % 2 ) {
     if (t1==t2) {
       stri[0]='N';
       stri[1]='S';
       stri[2]=' ';
       stri[3]='\0';
     }
     else if (t1 > t2) {
       stri[0]='N';
       stri[1]=' ';
       stri[2]='\0';
     }
     else {
       stri[0]='S';
       stri[1]=' ';
       stri[2]='\0';
     }
   }
   else {
     if (t1==t2) {
       stri[0]='E';
       stri[1]='W';
       stri[2]=' ';
       stri[3]='\0';
     }
     else if (t1 > t2) {
       stri[0]='E';
       stri[1]=' ';
       stri[2]='\0';
     }
     else {
       stri[0]='W';
       stri[1]=' ';
       stri[2]='\0';
     }
   }
   return;
}


int VulnerDefSide(int side, int vulnerable) {
  if (vulnerable == 0)
    return 0;
  else if (vulnerable == 1)
    return 1;
  else if (side) {
    /* N/S makes par contract. */
    if (vulnerable == 2)
      return 0;
    else
      return 1;
  }
  else {
    if (vulnerable == 3)
      return 0;
    else
      return 1;
  }
}

#endif


int CalcMultiContracts(int max_lower, int tricks) {
  int n;

  switch (tricks-6) {
    case 5: if (max_lower==3) {n = 2345;}
	    else if (max_lower==2) {n = 345;}
	    else if (max_lower==1) {n = 45;}
	    else {n = 5;}
	    break;
    case 4: if (max_lower==3) {n = 1234;}
	    else if (max_lower==2) {n = 234;}
	    else if (max_lower==1) {n = 34;}
	    else {n = 4;}
	    break;
    case 3: if (max_lower==2) {n = 123;}
	    else if (max_lower==1) {n = 23;}
	    else {n = 3;}
	    break;
    case 2: if (max_lower==1) {n = 12;}
	    else {n = 2;}
	    break;
    default: n = tricks-6;
  }
  return n;
}

int STDCALL CalcPar(ddTableDeal tableDeal, int vulnerable, 
    ddTableResults * tablep, parResults *presp) {

  int res;

  res=CalcDDtable(tableDeal, tablep);

  if (res!=1)
    return res;

  res=Par(tablep, presp, vulnerable);

  return res;
}

#if 0

/* Unsorted*/
int STDCALL DealerParBin(
  ddTableResults * tablep,
  parResultsMaster * presp,
  int dealer,
  int vulnerable)
{
  /* dealer     0: North 1: East  2: South  3: West */
  /* vulnerable 0: None  1: Both  2: NS     3: EW   */

  parResultsDealer parResDealer;
  int k, delta;

  int res = DealerPar(tablep, &parResDealer, dealer, vulnerable);

  if (res != RETURN_NO_FAULT)
  {
    return res;
  }

  presp->score = parResDealer.score;
  presp->number = parResDealer.number;

  for (k = 0; k < parResDealer.number; k++)
  {
    delta = 1;

    presp->contracts[k].level = int(parResDealer.contracts[k][0] - '0');

    switch (parResDealer.contracts[k][1])
    {
      case 'N':	presp->contracts[k].denom = 0; break;
      case 'S':	presp->contracts[k].denom = 1; break;
      case 'H':	presp->contracts[k].denom = 2; break;
      case 'D':	presp->contracts[k].denom = 3; break;
      case 'C':	presp->contracts[k].denom = 4; break;
    }

    if (strstr(parResDealer.contracts[k], "NS"))
      presp->contracts[k].seats = 4;
    else if (strstr(parResDealer.contracts[k], "EW"))
      presp->contracts[k].seats = 5;
    else if (strstr(parResDealer.contracts[k], "-N"))
    {
      presp->contracts[k].seats = 0;
      delta = 0;
    }
    else if (strstr(parResDealer.contracts[k], "-E"))
    {
      presp->contracts[k].seats = 1;
      delta = 0;
    }
    else if (strstr(parResDealer.contracts[k], "-S"))
    {
      presp->contracts[k].seats = 2;
      delta = 0;
    }
    else if (strstr(parResDealer.contracts[k], "-W"))
    {
      presp->contracts[k].seats = 3;
      delta = 0;
    }
	
    if (parResDealer.contracts[0][2] == '*')
    {
	/* Sacrifice */			
      presp->contracts[k].underTricks =(int) (parResDealer.contracts[k][6 + delta] -'0');
      presp->contracts[k].overTricks = 0;
    }
    else 
	/* Make */
    {
      if (strchr(parResDealer.contracts[k], '+'))
	presp->contracts[k].overTricks = (int)(parResDealer.contracts[k][5 + delta] - '0');
      else
	presp->contracts[k].overTricks = 0;
      presp->contracts[k].underTricks = 0;
		}
    }
  }
  return RETURN_NO_FAULT;
}
#endif

int STDCALL DealerParBin(
  ddTableResults * tablep,
  parResultsMaster * presp,
  int dealer,
  int vulnerable)
{
	/* dealer     0: North 1: East  2: South  3: West */
	/* vulnerable 0: None  1: Both  2: NS     3: EW   */

  parResultsDealer parResDealer;
  parContr2Type parContr2[10]; 
  int k, delta;

  int res = DealerPar(tablep, &parResDealer, dealer, vulnerable);

  if (res != RETURN_NO_FAULT)
  {
    return res;
  }

  for (k = 0; k<parResDealer.number; k++) {

    for (int u = 0; u<10; u++)
      parContr2[k].contracts[u] = parResDealer.contracts[k][u];

    if (parResDealer.contracts[k][1] == 'N')
      parContr2[k].denom = 0;
    else if (parResDealer.contracts[k][1] == 'S')
      parContr2[k].denom = 1;
    else if (parResDealer.contracts[k][1] == 'H')
      parContr2[k].denom = 2;
    else if (parResDealer.contracts[k][1] == 'D')
      parContr2[k].denom = 3;
    else if (parResDealer.contracts[k][1] == 'C')
      parContr2[k].denom = 4;
  }

  for (int s = 1; s < parResDealer.number; s++) {
    parContr2Type tmp = parContr2[s];
    int r = s;
    for (; r && tmp.denom < parContr2[r - 1].denom; --r)
      parContr2[r] = parContr2[r - 1];
    parContr2[r] = tmp;
  }

  presp->score = parResDealer.score;
  presp->number = parResDealer.number;

  for (k = 0; k < parResDealer.number; k++)
  {
    delta = 1;

    presp->contracts[k].level = int(parContr2[k].contracts[0] - '0');

    switch (parContr2[k].contracts[1])
    {
      case 'N': presp->contracts[k].denom = 0; break;
      case 'S': presp->contracts[k].denom = 1; break;
      case 'H': presp->contracts[k].denom = 2; break;
      case 'D': presp->contracts[k].denom = 3; break;
      case 'C': presp->contracts[k].denom = 4; break;
      default:
        throw std::runtime_error(
	  "contracts[1] not in (NSHDC) in DealerParBin");
    }

    if (strstr(parContr2[k].contracts, "NS"))
      presp->contracts[k].seats = 4;
    else if (strstr(parContr2[k].contracts, "EW"))
      presp->contracts[k].seats = 5;
    else if (strstr(parContr2[k].contracts, "-N"))
    {
      presp->contracts[k].seats = 0;
      delta = 0;
    }
    else if (strstr(parContr2[k].contracts, "-E"))
    {
      presp->contracts[k].seats = 1;
      delta = 0;
    }
    else if (strstr(parContr2[k].contracts, "-S"))
    {
      presp->contracts[k].seats = 2;
      delta = 0;
    }
    else if (strstr(parContr2[k].contracts, "-W"))
    {
      presp->contracts[k].seats = 3;
      delta = 0;
    }

    if (parResDealer.contracts[0][2] == '*')
    {
	/* Sacrifice */
      presp->contracts[k].underTricks = 
        static_cast<int>(parContr2[k].contracts[6 + delta] - '0');
      presp->contracts[k].overTricks = 0;
    }
    else
	/* Make */

    {
      if (strchr(parContr2[k].contracts, '+'))
	presp->contracts[k].overTricks = 
	  static_cast<int>(parContr2[k].contracts[5 + delta] - '0');
      else
	presp->contracts[k].overTricks = 0;
      presp->contracts[k].underTricks = 0;
    }
  }
  return RETURN_NO_FAULT;
}


int STDCALL SidesPar(ddTableResults * tablep, parResultsDealer sidesRes[2], int vulnerable) {

  int res, h, hbest[2], k;
  parResultsDealer parRes2[4];

  for (h = 0; h <= 3; h++) {

    res = DealerPar(tablep, &parRes2[h], h, vulnerable);

    char * p = strstr(parRes2[h].contracts[0], "pass");
    if (p != nullptr) {
      parRes2[h].number = 1;
      parRes2[h].score = 0;
    }
  }

  if (parRes2[2].score > parRes2[0].score)
    hbest[0] = 2;
  else
    hbest[0] = 0;

  if (parRes2[3].score > parRes2[1].score)
    hbest[1] = 3;
  else
    hbest[1] = 1;

  sidesRes[0].number = parRes2[hbest[0]].number;
  sidesRes[0].score = parRes2[hbest[0]].score;
  sidesRes[1].number = parRes2[hbest[1]].number;
  sidesRes[1].score = -parRes2[hbest[1]].score;

  for (k = 0; k < sidesRes[0].number; k++)
    strcpy(sidesRes[0].contracts[k], parRes2[hbest[0]].contracts[k]);

  for (k = 0; k < sidesRes[1].number; k++)
    strcpy(sidesRes[1].contracts[k], parRes2[hbest[1]].contracts[k]);

  return res;
}


int STDCALL SidesParBin(
  ddTableResults * tablep, 
  parResultsMaster sidesRes[2], 
  int vulnerable) 
  {

  int res, h, hbest[2], k;
  parResultsMaster parRes2[4];

  for (h = 0; h <= 3; h++) {

    res = DealerParBin(tablep, &parRes2[h], h, vulnerable);

    if (res != RETURN_NO_FAULT)
      return res;
 
    if (parRes2[h].score == 0) 
    {
      if ((h == 0) || (h == 2))
      {
	sidesRes[0].number = 1;
	sidesRes[0].score = 0;
      }
      else {
	sidesRes[1].number = 1;
	sidesRes[1].score = 0;
      }
    }
  }

  if (parRes2[2].score > parRes2[0].score)
    hbest[0] = 2;
  else
    hbest[0] = 0;

  if (parRes2[3].score > parRes2[1].score)
    hbest[1] = 3;
  else
    hbest[1] = 1;

  sidesRes[0].number = parRes2[hbest[0]].number;
  sidesRes[0].score = parRes2[hbest[0]].score;
  sidesRes[1].number = parRes2[hbest[1]].number;
  sidesRes[1].score = -parRes2[hbest[1]].score;

  for (k = 0; k < sidesRes[0].number; k++)
    sidesRes[0].contracts[k] = parRes2[hbest[0]].contracts[k];

  for (k = 0; k < sidesRes[1].number; k++)
    sidesRes[1].contracts[k] = parRes2[hbest[1]].contracts[k];

  return RETURN_NO_FAULT;
}


int STDCALL ConvertToDealerTextFormat(parResultsMaster *pres, char *resp) {
  int k, i;
  char buff[20];

  sprintf(resp, "Par %d: ", pres->score);

  for (k = 0; k < pres->number; k++) {

    if (k != 0)
      strcat(resp, "  ");

    switch (pres->contracts[k].seats) {
      case 0: strcat(resp, "N "); break;
      case 1: strcat(resp, "E "); break;
      case 2: strcat(resp, "S "); break;
      case 3: strcat(resp, "W "); break;
      case 4: strcat(resp, "NS "); break;
      case 5: strcat(resp, "EW "); break;
      default:
        throw std::runtime_error("Seats not in [N,W,S,W,NS,EW] in ConvertToDealerTextFormat");
    }

    for (i = 0; i < 10; i++)
      buff[i] = '\0';
    sprintf(buff, "%d", pres->contracts[k].level);
    strcat(resp, buff);

    switch (pres->contracts[k].denom) {
      case 0: strcat(resp, "N"); break;
      case 1: strcat(resp, "S"); break;
      case 2: strcat(resp, "H"); break;
      case 3: strcat(resp, "D"); break;
      case 4: strcat(resp, "C"); break;
      default:
        throw std::runtime_error("denom not in [N,S,H,D,C] in ConvertToDealerTextFormat");
    }

    if (pres->contracts[k].underTricks > 0) {
      strcat(resp, "x-");
      for (i = 0; i < 10; i++)
	buff[i] = '\0';
      sprintf(buff, "%d", pres->contracts[k].underTricks);
      strcat(resp, buff);
    }
    else if (pres->contracts[k].overTricks > 0) {
      strcat(resp, "+");
      for (i = 0; i < 10; i++)
	buff[i] = '\0';
      sprintf(buff, "%d", pres->contracts[k].overTricks);
      strcat(resp, buff);
    }
  }
  return RETURN_NO_FAULT;
}




int STDCALL ConvertToSidesTextFormat(parResultsMaster *pres, parTextResults *resp) {
  int k, i, j;
  char buff[20];

  for (i = 0; i < 2; i++)
    for (k = 0; k < 128; k++)
      resp->parText[i][k] = '\0';

  if (pres->score == 0) { 
    sprintf(resp->parText[0], "Par 0");
    return RETURN_NO_FAULT;
  }

  for (i = 0; i < 2; i++) { 

    sprintf(resp->parText[i], "Par %d: ", (pres + i)->score);

    for (k = 0; k < (pres + i)->number; k++) {

      if (k != 0)
	strcat(resp->parText[i], "  ");

      switch ((pres + i)->contracts[k].seats) {
	case 0: strcat(resp->parText[i], "N "); break;
	case 1: strcat(resp->parText[i], "E "); break;
	case 2: strcat(resp->parText[i], "S "); break;
	case 3: strcat(resp->parText[i], "W "); break;
	case 4: strcat(resp->parText[i], "NS "); break;
	case 5: strcat(resp->parText[i], "EW "); break;
        default:
          throw std::runtime_error("Seats not in [N,W,S,W,NS,EW] in ConvertToSidesTextFormat");
      }

      for (j = 0; j < 10; j++)
	buff[j] = '\0';
      sprintf(buff, "%d", (pres + i)->contracts[k].level);
      strcat(resp->parText[i], buff);

      switch ((pres + i)->contracts[k].denom) {
	case 0: strcat(resp->parText[i], "NT"); break;
	case 1: strcat(resp->parText[i], "S"); break;
	case 2: strcat(resp->parText[i], "H"); break;
	case 3: strcat(resp->parText[i], "D"); break;
	case 4: strcat(resp->parText[i], "C"); break;
        default:
          throw std::runtime_error("denom not in [N,S,H,D,C] in ConvertToSidesTextFormat");
      }

      if ((pres + i)->contracts[k].underTricks > 0) {
	strcat(resp->parText[i], "x-");
	for (j = 0; j < 10; j++)
	  buff[j] = '\0';
	sprintf(buff, "%d", (pres + i)->contracts[k].underTricks);
	strcat(resp->parText[i], buff);
      }
      else if ((pres + i)->contracts[k].overTricks > 0) {
	strcat(resp->parText[i], "+");
	for (j = 0; j < 10; j++)
	  buff[j] = '\0';
	sprintf(buff, "%d", (pres + i)->contracts[k].overTricks);
	strcat(resp->parText[i], buff);
      }
    }

    if (i == 0){
      if ((pres->score != -(pres + 1)->score) || (pres->number != (pres + 1)->number)) {
	resp->equal = false;
      }
      else {
	resp->equal = true;
	for (k = 0; k < pres->number; k++) {
	  if ((pres->contracts[k].denom != (pres + 1)->contracts[k].denom) ||
	     (pres->contracts[k].level != (pres + 1)->contracts[k].level) ||
	     (pres->contracts[k].overTricks != (pres + 1)->contracts[k].overTricks) ||
	     (pres->contracts[k].seats != (pres + 1)->contracts[k].seats) ||
	     (pres->contracts[k].underTricks != (pres + 1)->contracts[k].underTricks)) {
	     resp->equal = false;
	     break;
	  }
        }		
      }
    }
  }

  return RETURN_NO_FAULT;
}


