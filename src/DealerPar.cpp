/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/



#include "dds.h"


/* First index: 0 nonvul, 1 vul.  Second index: tricks down */
int DOUBLED_SCORES[2][14] = 
{
  {  0,  100,  300,  500,  800, 1100, 1400, 1700,
        2000, 2300, 2600, 2900, 3200, 3500 },
  {  0,  200,  500,  800, 1100, 1400, 1700, 2000, 
        2300, 2600, 2900, 3200, 3500, 3800 }
};

/* First index is contract number,
   0 is pass, 1 is 1C, ..., 35 is 7NT.
   Second index is 0 nonvul, 1 vul. */

int SCORES[36][2] =
{
  {   0,    0}, 
  {  70,   70}, {  70,   70}, {  80,   80}, {  80,   80}, {  90,   90},
  {  90,   90}, {  90,   90}, { 110,  110}, { 110,  110}, { 120,  120},
  { 110,  110}, { 110,  110}, { 140,  140}, { 140,  140}, { 400,  600},
  { 130,  130}, { 130,  130}, { 420,  620}, { 420,  620}, { 430,  630},
  { 400,  600}, { 400,  600}, { 450,  650}, { 450,  650}, { 460,  660},
  { 920, 1370}, { 920, 1370}, { 980, 1430}, { 980, 1430}, { 990, 1440},
  {1440, 2140}, {1440, 2140}, {1510, 2210}, {1510, 2210}, {1520, 2220}
};

/* Second index is contract number, 0 .. 35.
   First index is vul: none, only defender, only declarer, both. */

int DOWN_TARGET[36][4] = 
{
  {0,0,0,0},
  {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
  {0,0,0,0}, {0,0,0,0}, {1,0,1,0}, {1,0,1,0}, {1,0,1,0},
  {1,0,1,0}, {1,0,1,0}, {1,0,1,0}, {1,0,1,0}, {2,1,3,2},
  {1,0,1,0}, {1,0,1,0}, {2,1,3,2}, {2,1,3,2}, {2,1,3,2},
  {2,1,3,2}, {2,1,3,2}, {2,1,3,2}, {2,1,3,2}, {2,1,3,2},
  {4,3,5,4}, {4,3,5,4}, {4,3,6,5}, {4,3,6,5}, {4,3,6,5},
  {6,5,8,7}, {6,5,8,7}, {6,5,8,7}, {6,5,8,7}, {6,5,8,7}
};

int FLOOR_CONTRACT[36] =
{
   0,  1,  2,  3,  4,  5,  1,  2,  3,  4,  5,
       1,  2,  3,  4, 15,  1,  2, 18, 19, 15,
      21, 22, 18, 19, 15, 26, 27, 28, 29, 30,
      31, 32, 33, 34, 35
};

char NUMBER_TO_CONTRACT[36][3] =
{
  "0", 
  "1C", "1D", "1H", "1S", "1N",
  "2C", "2D", "2H", "2S", "2N",
  "3C", "3D", "3H", "3S", "3N",
  "4C", "4D", "4H", "4S", "4N",
  "5C", "5D", "5H", "5S", "5N",
  "6C", "6D", "6H", "6S", "6N",
  "7C", "7D", "7H", "7S", "7N"
};

char NUMBER_TO_PLAYER[4][2] = { "N", "E", "S", "W" };

/* First index is vul: none, both, NS, EW.
   Second index is vul (0, 1) for NS and then EW. */
int VUL_LOOKUP[4][2] = { {0, 0}, {1, 1}, {1, 0}, {0, 1} };

/* First vul is declarer (not necessarily NS), second is defender. */
int VUL_TO_NO[2][2] = { {0, 1}, {2, 3} };


/* Maps DDS order (S, H, D, C, NT) to par order (C, D, H, S, NT). */
int DENOM_ORDER[5] = { 3, 2, 1, 0, 4 };

  
struct data_type 
{
  int			primacy;
  int			highest_making_no;
  int			dearest_making_no;
  int			dearest_score;
  int			vul_no;
};

struct list_type 
{
  int			score;
  int			dno;
  int			no;
  int			tricks;
  int			down;
};


#define BIGNUM 9999


void survey_scores(
  ddTableResults 	* tablep,
  int 			dealer,
  int 			vul_by_side[2],
  data_type		* data,
  int                   * num_candidates,
  list_type		list[2][5]);

void best_sacrifice(
  ddTableResults 	* tablep,
  int                   side,
  int                   no,
  int                   dno,
  int 			dealer,
  list_type		list[2][5],
  int			sacr[5][5],
  int			* best_down);

void sacrifices_as_text(
  ddTableResults 	* tablep,
  int                   side,
  int 			dealer,
  int			best_down,
  int                   no_decl,
  int                   dno,
  list_type		list[2][5],
  int			sacr[5][5],
  char			results[10][10],
  int			* res_no);

void reduce_contract(
  int			* no,
  int			down,
  int			* plus);

void contract_as_text(
  ddTableResults 	* tablep,
  int			side,
  int			no,
  int			dno,
  int			down,
  char			str[10]);

void sacrifice_as_text(
  int			no,
  int			pno,
  int			down,
  char			str[10]);



int STDCALL DealerPar(
  ddTableResults 	* tablep,
  parResultsDealer 	* presp,
  int 			dealer,
  int 			vulnerable)
{
  /* dealer     0: North 1: East  2: South  3: West */
  /* vulnerable 0: None  1: Both  2: NS     3: EW   */

  int 			* vul_by_side = VUL_LOOKUP[vulnerable];
  data_type 		data;
  list_type 		list[2][5];

  /* First we find the side entitled to a plus score (primacy)
     and some statistics for each constructively bid (undoubled)
     contract that might be the par score.  */


  int num_cand;
  survey_scores(tablep, dealer, vul_by_side, &data, &num_cand, list);
  int side = data.primacy;

  if (side == -1)
  {
    presp->number = 1;
    sprintf(presp->contracts[0], "%s", "pass");
    return RETURN_NO_FAULT;
  }

  /* Go through the contracts, starting from the highest one. */
  list_type * lists = list[side];
  int vul_no    = data.vul_no;
  int best_plus = 0;
  int down      = 0;
  int sac_found = 0;

  int type[5], sac_gap[5];
  int best_down = 0;
  int sacr[5][5] = { {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0},
                     {0,0,0,0,0}, {0,0,0,0,0} };

  for (int n = 0; n < num_cand; n++)
  {
    int no  = lists[n].no;
    int dno = lists[n].dno;
    int target = DOWN_TARGET[no][vul_no];

    best_sacrifice(tablep, side, no, dno, dealer, list, sacr, &down);

    if (down <= target)
    {
      if (down > best_down) best_down = down;
      if (sac_found)
      {
        /* Declarer will never get a higher sacrifice by bidding
           less, so we can stop looking for sacrifices.  But it
	   can't be a worthwhile contract to bid, either. */
        type[n] = -1;
      }
      else
      {
        sac_found = 1;
	type[n]   = 0;
	lists[n].down = down;
      }
    }
    else
    {
      if (lists[n].score > best_plus)
        best_plus = lists[n].score;
      type[n] = 1;
      sac_gap[n] = target - down;
    }
  }

  int res_no  = 0;
  int vul_def = vul_by_side[1-side];
  int sac     = DOUBLED_SCORES[vul_def][best_down];

  if (! sac_found || best_plus > sac)
  {
    /* The primacy side bids. */
    presp->score = (side == 0 ? best_plus : -best_plus);

    for (int n = 0; n < num_cand; n++)
    {
      if (type[n] != 1 || lists[n].score != best_plus) continue;
      int no = lists[n].no, plus;
      reduce_contract(&no, sac_gap[n], &plus);

      contract_as_text(tablep, side, no, lists[n].dno, 
        plus, presp->contracts[res_no]);
      res_no++;
    }
  }
  else
  {
    /* The primacy side collects the penalty. */
    int sac_vul   = vul_by_side[1-side];
    int sac_score = DOUBLED_SCORES[sac_vul][best_down];
    presp->score = (side == 0 ? sac_score : -sac_score);

    for (int n = 0; n < num_cand; n++)
    {
      if (type[n] != 0 || lists[n].down != best_down) continue;
      sacrifices_as_text(tablep, side, dealer, best_down,
        lists[n].no, lists[n].dno, list, sacr, 
        presp->contracts, &res_no);
    }
  }
  presp->number = res_no;
  return RETURN_NO_FAULT;
}


void survey_scores(
  ddTableResults 	* tablep,
  int 			dealer,
  int 			vul_by_side[2],
  data_type		* data,
  int                   * num_candidates,
  list_type		list[2][5])
{
  /*
    When this is done, data has added the following entries:
    * primacy (0 or 1) is the side entitled to a plus score.
      If the deal should be passed out, it is -1, and nothing
      else is set.
    * highest_making_no is a contract number (for that side)
    * dearest_making_no is a contract number (for that side)
    * dearest_score is the best score if there is no sacrifice
    * vul_no is an index for a table, seen from the primacy

    list[side][dno] has added the following entries:
    * score
    * dno is the denomination number
    * no is a contract number
    * tricks is the number of tricks embedded in the contract
    For the primacy side, the list is sorted in descending
    order of the contract number (no).
  */

  data_type stats[2];

  for (int side = 0; side <= 1; side++)
  {
    int highest_making_no = 0;
    int dearest_making_no = 0;
    int dearest_score     = 0;

    for (int dno = 0; dno <= 4; dno++)
    {
      list_type * slist = &list[side][dno];
      int * t = tablep->resTable[ DENOM_ORDER[dno] ];
      int a    = t[side];
      int b    = t[side+2];
      int best = (a > b ? a : b);

      int no        = 5*(best-7) + dno + 1;
      slist->no     = no; /* May be negative! */

      if (best < 7)
      {
        slist->score = 0;
	continue;
      }

      int score     = SCORES[no][ vul_by_side[side] ];
      slist->score  = score;
      slist->dno    = dno;
      slist->tricks = best;

      if (score > dearest_score)
      {
        dearest_score     = score;
	dearest_making_no = no;
      }
      else if (score == dearest_score && no < dearest_making_no)
      {
        /* The lowest such, e.g. 3NT and 5C. */
	dearest_making_no = no;
      }

      if (no > highest_making_no)
      {
        highest_making_no = no;
      }
    }
    data_type * sside = &stats[side];
    sside->highest_making_no = highest_making_no;
    sside->dearest_making_no = dearest_making_no;
    sside->dearest_score     = dearest_score;
  }

  int primacy = 0;
  int s0 = stats[0].highest_making_no;
  int s1 = stats[1].highest_making_no;
  if (s0 > s1)
  {
    primacy = 0;
  }
  else if (s0 < s1)
  {
    primacy = 1;
  }
  else if (s0 == 0)
  {
    data->primacy = -1;
    return;
  }
  else
  {
    /* Special case, depends who can bid it first. */
    int dno   = (s0-1) % 5;
    int t_max = list[0][dno].tricks;
    int * t   = tablep->resTable[ DENOM_ORDER[dno] ];

    for (int pno = dealer; pno <= dealer+3; pno++)
    {
      if (t[pno % 4] != t_max) continue;
      primacy = pno % 2;
      break;
    }
  }

  data_type * sside = &stats[primacy];

  int dm_no               = sside->dearest_making_no;
  data->primacy           = primacy;
  data->highest_making_no = sside->highest_making_no;
  data->dearest_making_no = dm_no;
  data->dearest_score     = sside->dearest_score;

  int vul_primacy        = vul_by_side[primacy];
  int vul_other          = vul_by_side[1-primacy];
  data->vul_no           = VUL_TO_NO[vul_primacy][vul_other];

  /* Sort the scores in descending order of contract number,
     i.e. first by score and second by contract number in case
     the score is the same.  Primitive bubble sort... */
  int n = 5;
  do
  {
    int new_n = 0;
    for (int i = 1; i < n; i++)
    {
      if (list[primacy][i-1].no > list[primacy][i].no) continue;

      list_type temp = list[primacy][i-1];
      list[primacy][i-1] = list[primacy][i];
      list[primacy][i] = temp;

      new_n = i;
    }
    n = new_n;
  }
  while (n > 0);

  *num_candidates = 5;
  for (n = 0; n <= 4; n++)
  {
    if (list[primacy][n].no < dm_no) (*num_candidates)--;
  }
}


void best_sacrifice(
  ddTableResults 	* tablep,
  int                   side,
  int                   no,
  int                   dno,
  int 			dealer,
  list_type		list[2][5],
  int			sacr_table[5][5],
  int			* best_down)
{
  int other = 1 - side;
  list_type * sacr_list = list[other];
  *best_down = BIGNUM;

  for (int eno = 0; eno <= 4; eno++)
  {
    list_type sacr = sacr_list[eno];
    int down = BIGNUM;

    if (eno == dno)
    {
      int t_max     = static_cast<int>((no+34) / 5);
      int * t       = tablep->resTable[ DENOM_ORDER[dno] ];
      int incr_flag = 0;
      for (int pno = dealer; pno <= dealer+3; pno++)
      {
        int diff = t_max - t[pno % 4];
	int s    = pno % 2;
	if (s == side)
	{
	  if (diff == 0) incr_flag = 1;
	}
	else
	{
	  int local = diff + incr_flag;
	  if (local < down) down = local;
	}
      }
      if (sacr.no + 5*down > 35) down = BIGNUM;
    }
    else
    {
      down = static_cast<int>((no - sacr.no + 4) / 5);
      if (sacr.no + 5*down > 35) down = BIGNUM;
    }
    sacr_table[dno][eno] = down;
    if (down < *best_down) *best_down = down;
  }
}
 

void sacrifices_as_text(
  ddTableResults 	* tablep,
  int                   side,
  int 			dealer,
  int			best_down,
  int                   no_decl,
  int                   dno,
  list_type		list[2][5],
  int			sacr[5][5],
  char			results[10][10],
  int			* res_no)
{
  int other = 1 - side;
  list_type * sacr_list = list[other];

  for (int eno = 0; eno <= 4; eno++)
  {
    int down = sacr[dno][eno];
    if (down != best_down) continue;

    if (eno != dno)
    {
      int no_sac = sacr_list[eno].no + 5 * best_down;
      contract_as_text(tablep, other, no_sac, eno, -best_down, 
        results[*res_no]);
      (*res_no)++;
      continue;
    }

    int t_max     = static_cast<int>((no_decl + 34) / 5);
    int * t       = tablep->resTable[ DENOM_ORDER[dno] ];
    int incr_flag = 0;
    int p_hit     = 0;
    int  pno_list[2], sac_list[2];
    for (int pno = dealer; pno <= dealer+3; pno++)
    {
      int pno_mod = pno % 4;
      int diff    = t_max - t[pno_mod];
      int s       = pno % 2;
      if (s == side)
      {
	if (diff == 0) incr_flag = 1;
      }
      else
      {
	down = diff + incr_flag;
	if (down != best_down) continue;
	pno_list[p_hit] = pno_mod;
	sac_list[p_hit] = no_decl + 5*incr_flag;
	p_hit++;
      }
    }

    int ns0 = sac_list[0];
    if (p_hit == 1)
    {
      sacrifice_as_text(ns0, pno_list[0], best_down, 
        results[*res_no]);
      (*res_no)++;
      continue;
    }

    int ns1 = sac_list[1];
    if (ns0 == ns1)
    {
      /* Both players */
      contract_as_text(tablep, other, ns0, eno, -best_down, 
        results[*res_no]);
      (*res_no)++;
      continue;
    }

    int p = (ns0 < ns1 ? 0 : 1);
    sacrifice_as_text(sac_list[p], pno_list[p], best_down, 
      results[*res_no]);
    (*res_no)++;
  }
}


void reduce_contract(
  int			* no,
  int			sac_gap,
  int			* plus)
{
    /* Could be that we found 4C just making, but it would be
       enough to bid 2C +2.  But we don't want to bid so low that
       we lose a game or slam bonus. */

  if (sac_gap >= -1)
  {
    /* No scope to reduce. */
    *plus = 0;
    return;
  }

  /* This is the lowest contract that we could reduce to. */
  int flr = FLOOR_CONTRACT[*no];

  /* As such, declarer could reduce the contract by down+1 levels
     (where down is negative) and still the opponent's sacrifice
     would not turn profitable.  But for non-vulnerable partials, 
     this can go wrong:  1M+1 and 2M= both pay +90, but 3m*-2
     is a bad sacrifice against 2M=, while 2m*-1 would be a good
     sacrifice against 1M+1. */
  int no_sac_level = *no + 5*(sac_gap+1);
  int new_no       = (no_sac_level > flr ? no_sac_level : flr);
  *plus            = (*no - new_no) / 5;
  *no              = new_no;
}


void contract_as_text(
  ddTableResults 	* tablep,
  int			side,
  int			no,
  int			dno,
  int			delta,
  char			str[10])
{
  int * t   = tablep->resTable[ DENOM_ORDER[dno] ];
  int ta    = t[side];
  int tb    = t[side+2];
  int t_max = (ta > tb ? ta : tb);

  char d[4]  = "";
  if (delta != 0) 
    sprintf(d, "%+d", delta);

  sprintf(str, "%s%s%s%s%s",
    NUMBER_TO_CONTRACT[no],
    (delta < 0 ? "*-" : "-"),
    (ta == t_max ? NUMBER_TO_PLAYER[side]   : ""),
    (tb == t_max ? NUMBER_TO_PLAYER[side+2] : ""),
    d);
}


void sacrifice_as_text(
  int		no,
  int		pno,
  int		down,
  char		str[10])
{
  sprintf(str, "%s*-%s-%d",
    NUMBER_TO_CONTRACT[no],
    NUMBER_TO_PLAYER[pno],
    down);
}
