/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <algorithm>

#include "print.hpp"

using std::cout;
using std::endl;
using std::setw;
using std::string;
using std::right;
using std::left;
using std::min;


static unsigned short dbit_map_rank[16];
static unsigned char dcard_rank[16];
static unsigned char dcard_suit[5];

string equals_to_string(const int equals);


void set_constants()
{
  dbit_map_rank[15] = 0x2000;
  dbit_map_rank[14] = 0x1000;
  dbit_map_rank[13] = 0x0800;
  dbit_map_rank[12] = 0x0400;
  dbit_map_rank[11] = 0x0200;
  dbit_map_rank[10] = 0x0100;
  dbit_map_rank[ 9] = 0x0080;
  dbit_map_rank[ 8] = 0x0040;
  dbit_map_rank[ 7] = 0x0020;
  dbit_map_rank[ 6] = 0x0010;
  dbit_map_rank[ 5] = 0x0008;
  dbit_map_rank[ 4] = 0x0004;
  dbit_map_rank[ 3] = 0x0002;
  dbit_map_rank[ 2] = 0x0001;
  dbit_map_rank[ 1] = 0;
  dbit_map_rank[ 0] = 0;

  dcard_rank[ 2] = '2';
  dcard_rank[ 3] = '3';
  dcard_rank[ 4] = '4';
  dcard_rank[ 5] = '5';
  dcard_rank[ 6] = '6';
  dcard_rank[ 7] = '7';
  dcard_rank[ 8] = '8';
  dcard_rank[ 9] = '9';
  dcard_rank[10] = 'T';
  dcard_rank[11] = 'J';
  dcard_rank[12] = 'Q';
  dcard_rank[13] = 'K';
  dcard_rank[14] = 'A';
  dcard_rank[15] = '-';

  dcard_suit[0] = 'S';
  dcard_suit[1] = 'H';
  dcard_suit[2] = 'D';
  dcard_suit[3] = 'C';
  dcard_suit[4] = 'N';
}


void print_PBN(const DealPBN& dl)
{
  cout << setw(10) << left << "trump" << dl.trump << "\n";
  cout << setw(10) << "first" << dl.first << "\n";
  cout << setw(10) << "cards" << dl.remainCards << "\n";
}


void print_FUT(const FutureTricks& fut)
{
  cout << setw(6) << left << "cards" << fut.cards << "\n";
  cout << setw(6) << right <<  "No." << 
    setw(7) << "suit" <<
    setw(7) << "rank" <<
    setw(7) << "equals" <<
    setw(7) << "score" << "\n";

  for (int i = 0; i < fut.cards; i++)
  {
    cout << setw(6) << right << i <<
      setw(7) << dcard_suit[ fut.suit[i] ] <<
      setw(7) << dcard_rank[ fut.rank[i] ] <<
      setw(7) << equals_to_string(fut.equals[i]) <<
      setw(7) << fut.score[i] << "\n";
  }
}


string equals_to_string(const int equals)
{
  string st = "";
  for (unsigned i = 15; i >= 2; i--)
  {
    if ((equals >> 2) & dbit_map_rank[i])
      st += static_cast<char>(dcard_rank[i]);
  }
  return (st == "" ? "-" : st);
}


void print_TABLE(const DdTableResults& table)
{
  cout << setw(5) << right << "" <<
    setw(6) << "North" <<
    setw(6) << "South" <<
    setw(6) << "East" <<
    setw(6) << "West" << "\n";

  cout << setw(5) << right << "NT" <<
    setw(6) << table.res_table[4][0] <<
    setw(6) << table.res_table[4][2] <<
    setw(6) << table.res_table[4][1] <<
    setw(6) << table.res_table[4][3] << "\n";

  for (int suit = 0; suit <= 3; suit++)
  {
    cout << setw(5) << right << dcard_suit[suit] <<
      setw(6) << table.res_table[suit][0] <<
      setw(6) << table.res_table[suit][2] <<
      setw(6) << table.res_table[suit][1] <<
      setw(6) << table.res_table[suit][3] << "\n";
  }
}


void print_PAR(const ParResults& par)
{
  cout << setw(9) << left << "NS score" << par.par_score[0] << "\n";
  cout << setw(9) << "EW score" << par.par_score[1] << "\n";
  cout << setw(9) << "NS list" << par.par_contracts_string[0] << "\n";
  cout << setw(9) << "EW list" << par.par_contracts_string[1] << "\n";
}


void print_DEALERPAR(const ParResultsDealer& par)
{
  cout << setw(6) << left << "Score" << par.score << "\n";
  cout << setw(6) << left << "Pars" << par.number << "\n";

  for (int i = 0; i < par.number; i++)
    cout << left << "Par " << setw(2) << i << par.contracts[i] << "\n";
}


void print_PLAY(const PlayTracePBN& play)
{
  cout << setw(6) << right << "Number" << 
    setw(5) << play.number << "\n";

  for (int i = 0; i < play.number; i++)
     cout << setw(6) << i <<  "   " << 
       play.cards[2*i] << play.cards[2*i+1] << "\n";
}


void print_TRACE(const SolvedPlay& solved)
{
  cout << setw(6) << right << "Number" << 
    setw(5) << solved.number << "\n";

  for (int i = 0; i < solved.number; i++)
     cout << setw(6) << i << 
       setw(5) << solved.tricks[i] << "\n";
}


void print_double_TRACE(
  const SolvedPlay& solved, 
  const SolvedPlay& ref)
{
  cout << "Number solved vs ref: " << solved.number << " vs. " <<
    ref.number << "\n";

  const int m = min(solved.number, ref.number);
  for (int i = 0; i < m; i++)
  {
    cout << "Trick " << i << ": " << 
      solved.tricks[i] << " vs " <<
      ref.tricks[i] << 
      (solved.tricks[i] == ref.tricks[i] ? "" : " ERROR") << "\n";
  }

  if (solved.number > m)
  {
    for (int i = m; i < solved.number; i++)
      cout << "Solved " << i << ": " << solved.tricks[i] << "\n";
  }
  else if (ref.number > m)
  {
    for (int i = m; i < ref.number; i++)
      cout << "Ref    " << i << ": " << ref.tricks[i] << "\n";
  }
}

