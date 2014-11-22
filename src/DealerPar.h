
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

  
struct data_type {
  int	primacy;
  int	highest_making_no;
  int	dearest_making_no;
  int	dearest_score;
  int	vul_no;
};

struct list_type {
  int	score;
  int	dno;
  int	no;
  int	tricks;
  int	down;
};


#define BIGNUM 9999
#define DEBUG     1


void survey_scores(
  struct ddTableResults * tablep,
  int 			dealer,
  int 			vul_by_side[2],
  struct data_type	* data,
  int                   * num_candidates,
  struct list_type	list[2][5]);

void best_sacrifice(
  struct ddTableResults * tablep,
  int                   side,
  int                   no,
  int                   dno,
  int 			dealer,
  struct list_type	list[2][5],
  int			sacr[5][5],
  int			* best_down);

void sacrifices_as_text(
  struct ddTableResults * tablep,
  int                   side,
  int 			dealer,
  int			best_down,
  int                   no_decl,
  int                   dno,
  struct list_type	list[2][5],
  int			sacr[5][5],
  char			results[10][10],
  int			* res_no);

void reduce_contract(
  int			* no,
  int			sac_vul,
  int			down,
  int			* plus);

void contract_as_text(
  struct ddTableResults * tablep,
  int			side,
  int			no,
  int			dno,
  int			down,
  char			str[10]);

void sacrifice_as_text(
  int		no,
  int		pno,
  int		down,
  char		str[10]);


