/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef EXAMPLES_HANDS_H
#define EXAMPLES_HANDS_H

// General initialization of three hands to be used in examples.

extern int trump_suit_[3];
extern int first_hand_[3];
extern int dealer_hand_[3];
extern int vulnerability_[3];

extern char pbn_hands_[3][80];

extern unsigned int holdings_[3][4][4];

extern int play_count_[3];

extern char play_sequence_[3][106];
extern int play_suit_[3][52];
extern int play_rank_[3][52];


auto print_future_tricks(char title[], FutureTricks * fut) -> void;
auto equals_to_string(int equals, char * res) -> void;
auto compare_future_tricks(FutureTricks * fut, int handno, int solutions) -> bool;

auto set_table(DdTableResults * table, int handno) -> void;
auto compare_table(DdTableResults * table, int handno) -> bool;
auto print_table(DdTableResults * table) -> void;

auto compare_par(ParResults * par, int handno) -> bool;
auto compare_dealer_par(ParResultsDealer * par, int handno) -> bool;
auto print_par(ParResults * par) -> void;
auto print_dealer_par(ParResultsDealer * par) -> void;

auto compare_play(SolvedPlay * trace, int handno) -> bool;
auto print_bin_play(PlayTraceBin * play, SolvedPlay * solved) -> void;
auto print_pbn_play(PlayTracePBN * play, SolvedPlay * solved) -> void;


auto print_hand(char title[], 
  unsigned int rank_in_suit[DDS_HANDS][DDS_SUITS]) -> void;

auto print_pbn_hand(char title[], char remainCards[]) -> void;

auto convert_pbn(char * dealBuff,
  unsigned int remainCards[DDS_HANDS][DDS_SUITS]) -> int;

auto is_a_card(char cardChar) -> int;

#endif  // EXAMPLES_HANDS_H
