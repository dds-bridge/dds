/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


void print_rank_count();
unsigned short int dbitMapRank[16];
unsigned char dcardRank[16];
unsigned char dcardSuit[5];

void main_identify();

void set_constants();

bool read_file(
  char * fname,
  int * number,
  int ** dealer_list,
  int ** vul_list,
  struct dealPBN ** deal_list,
  struct futureTricks ** fut_list,
  struct ddTableResults ** table_list,
  struct parResults ** par_list,
  struct parResultsDealer ** dealerpar_list,
  struct playTracePBN ** play_list,
  struct solvedPlay ** trace_list);

bool parse_NUMBER(
  char * line,
  int * number);

bool parse_PBN(
  char * line,
  int * dealer,
  int * vul,
  struct dealPBN * dl);

bool parse_FUT(
  char * line,
  struct futureTricks * fut);

bool parse_TABLE(
  char * line,
  struct ddTableResults * table);

bool parse_PAR(
  char * line,
  struct parResults * par);

bool parse_DEALERPAR(
  char * line,
  struct parResultsDealer * par);

bool parse_PLAY(
  char * line,
  struct playTracePBN * playp);

bool parse_TRACE(
  char * line,
  struct solvedPlay * solvedp);

bool parseable_GIB(
  char line[]);

bool parse_GIB(
  char line[],
  dealPBN * dl,
  ddTableResults * table);

bool compare_PBN(
  struct dealPBN * dl1,
  struct dealPBN * dl2);

bool compare_FUT(
  struct futureTricks * fut1,
  struct futureTricks * fut2);

bool compare_TABLE(
  struct ddTableResults * table1,
  struct ddTableResults * table2);

bool compare_PAR(
  struct parResults * par1,
  struct parResults * par2);

bool compare_DEALERPAR(
  struct parResultsDealer * par1,
  struct parResultsDealer * par2);

bool compare_TRACE(
  struct solvedPlay * trace1,
  struct solvedPlay * trace2);

bool print_PBN(
  struct dealPBN * dl);

bool print_FUT(
  struct futureTricks * fut);

void equals_to_string(
  int equals,
  char * res);

bool print_TABLE(
  struct ddTableResults * table);

bool print_PAR(
  struct parResults * par);

bool print_DEALERPAR(
  struct parResultsDealer * par);

bool print_PLAY(
  struct playTracePBN * play);

bool print_TRACE(
  struct solvedPlay * solvedp);

void loop_solve(
  struct boardsPBN * bop,
  struct solvedBoards * solvedbdp,
  struct dealPBN * deal_list,
  struct futureTricks * fut_list,
  int number);

bool loop_calc(
  struct ddTableDealsPBN * dealsp,
  struct ddTablesRes * resp,
  struct allParResults * parp,
  struct dealPBN * deal_list,
  struct ddTableResults * table_list,
  int number);

bool loop_par(
  int * vul_list,
  struct ddTableResults * table_list,
  struct parResults * par_list,
  int number);

bool loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  struct ddTableResults * table_list,
  struct parResultsDealer * dealerpar_list,
  int number);

bool loop_play(
  struct boardsPBN * bop,
  struct playTracesPBN * playsp,
  struct solvedPlays * solvedplp,
  struct dealPBN * deal_list,
  struct playTracePBN * play_list,
  struct solvedPlay * trace_list,
  int number);

void print_times(
  int number);

#ifndef _WIN32
int timeval_diff(
  timeval x,
  timeval y);
#endif

void timer_start();

int timer_end();

bool consume_int(
  char * line,
  int * pos,
  int * res);

bool consume_string(
  char * line,
  int * pos,
  char * res);

bool consume_tag(
  char * line,
  int * pos,
  const char * tag);

void dump_string(
  const char * line);

void set_complexity();

void set_weight();
