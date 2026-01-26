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


auto print_future_tricks(char title[], futureTricks * fut) -> void;
auto equals_to_string(int equals, char * res) -> void;
auto compare_future_tricks(futureTricks * fut, int handno, int solutions) -> bool;

auto set_table(ddTableResults * table, int handno) -> void;
auto compare_table(ddTableResults * table, int handno) -> bool;
auto print_table(ddTableResults * table) -> void;

auto compare_par(parResults * par, int handno) -> bool;
auto compare_dealer_par(parResultsDealer * par, int handno) -> bool;
auto print_par(parResults * par) -> void;
auto print_dealer_par(parResultsDealer * par) -> void;

auto compare_play(solvedPlay * trace, int handno) -> bool;
auto print_bin_play(playTraceBin * play, solvedPlay * solved) -> void;
auto print_pbn_play(playTracePBN * play, solvedPlay * solved) -> void;


auto print_hand(char title[], 
  unsigned int rankInSuit[DDS_HANDS][DDS_SUITS]) -> void;

auto print_pbn_hand(char title[], char remainCards[]) -> void;

auto convert_pbn(char * dealBuff,
  unsigned int remainCards[DDS_HANDS][DDS_SUITS]) -> int;

auto is_a_card(char cardChar) -> int;

#endif  // EXAMPLES_HANDS_H
