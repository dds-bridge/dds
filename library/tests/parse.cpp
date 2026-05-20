/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

#include <api/dll.h>
#include "parse.hpp"

using std::cout;
using std::endl;
using std::ifstream;
using std::vector;
using std::string;
using std::getline;
using std::invalid_argument;
using std::out_of_range;


bool parse_PBN(
  const vector<string>& list,
  int& dealer,
  int& vul,
  DealPBN * dl);

bool parse_FUT(
  const vector<string>& list,
  FutureTricks * fut);

bool parse_TABLE(
  const vector<string>& list,
  DdTableResults * table);

bool parse_PAR(
  const vector<string>& list,
  ParResults * par);

bool parse_DEALERPAR(
  const vector<string>& list,
  ParResultsDealer * par);

bool parse_PLAY(
  const vector<string>& list,
  PlayTracePBN * play);

bool parse_TRACE(
  const vector<string>& list,
  SolvedPlay * solved);

bool parseable_GIB(const string& line);

bool parse_GIB(
  const string& line,
  DealPBN * dl,
  DdTableResults * table);

bool get_any_line(
  ifstream& fin,
  vector<string>& list,
  const string& tag,
  const int n);

bool get_head_element(
  const string& elem,
  const string& expected);

bool get_int_element(
  const string& elem,
  int& res,
  const string& errtext);

bool strip_quotes(
  const string& st,
  char * cstr,
  const string& errtag);

bool strip_quotes(
  const string& st,
  int& res,
  const string& errtag);

string trimTrailing(
  const string& st,
  const char c);

void splitIntoWords(
  const string& text,
  vector<string>& words);

bool str2int(
  const string& text,
  int& res);


bool read_file(
  const string& fname,
  int& number,
  bool& GIBmode,
  int ** dealer_list,
  int ** vul_list,
  DealPBN ** deal_list,
  FutureTricks ** fut_list,
  DdTableResults ** table_list,
  ParResults ** par_list,
  ParResultsDealer ** dealerpar_list,
  PlayTracePBN ** play_list,
  SolvedPlay ** trace_list)
{
  ifstream fin;
  fin.open(fname);

  string line;
  if (! getline(fin, line))
  {
    cout << "First line bad: '" << line << "'" << endl;
    return false;
  }

  vector<string> list;
  list.clear();
  splitIntoWords(line, list);

  if (list.size() == 2 && get_head_element(list[0], "NUMBER"))
  {
    // Hopefully a txt-style file.
    if (! str2int(list[1], number))
    {
      cout << "Not a number of hands: '" << list[1] << "'" << endl;
      return false;
    }
    else if (number <= 0 || number > 100000)
    {
      cout << "Suspect number of hands: " << number << endl;
      return false;
    }
  }
  else if (! parseable_GIB(line))
  {
    cout << "Not a GIB-style start: '" << line << "'" << endl;
    return false;
  }
  else
  {
    // Count the lines, then start over.
    GIBmode = 1;
    number = 1;
    while (1)
    {
      if (! getline(fin, line))
        break;
      number++;
    }
    fin.close();
    fin.open(fname);
  }


  // Make enough room for the hands.

  const size_t number_t = static_cast<size_t>(number);

  if ((*dealer_list = static_cast<int *>
      (calloc(number_t, sizeof(int)))) == NULL)
    return false;

  if ((*vul_list = static_cast<int *>
      (calloc(number_t, sizeof(int)))) == NULL)
    return false;

  if ((*deal_list = static_cast<DealPBN *>
      (calloc(number_t, sizeof(DealPBN)))) == NULL)
    return false;

  if ((*fut_list = static_cast<FutureTricks *>
      (calloc(number_t, sizeof(FutureTricks)))) == NULL)
    return false;

  if ((*table_list = static_cast<DdTableResults *>
      (calloc(number_t, sizeof(DdTableResults)))) == NULL)
    return false;

  if ((*par_list = static_cast<ParResults *>
      (calloc(number_t, sizeof(ParResults)))) == NULL)
    return false;

  if ((*dealerpar_list = static_cast<ParResultsDealer *>
      (calloc(number_t, sizeof(ParResultsDealer)))) == NULL)
    return false;

  if ((*play_list = static_cast<PlayTracePBN *>
      (calloc(number_t, sizeof(PlayTracePBN)))) == NULL)
    return false;

  if ((*trace_list = static_cast<SolvedPlay *>
      (calloc(number_t, sizeof(SolvedPlay)))) == NULL)
    return false;

  if (GIBmode)
  {
    for (int n = 0; n < number; n++)
    {
      if (! getline(fin, line))
      {
        cout << "Expected GIB line " << n << endl;
        return false;
      }
      if (! parse_GIB(line, &(*deal_list)[n], &(*table_list)[n]))
        return false;
    }
  }
  else
  {
    for (int n = 0; n < number; n++)
    {
      if (! get_any_line(fin, list, "PBN", n))
        return false;
      if (! parse_PBN(list, (*dealer_list)[n], 
          (*vul_list)[n], &(*deal_list)[n])) 
        return false;

      if (! get_any_line(fin, list, "FUT", n))
        return false;
      if (! parse_FUT(list, &(*fut_list)[n]))
        return false;

      if (! get_any_line(fin, list, "TABLE", n))
        return false;
      if (! parse_TABLE(list, &(*table_list)[n])) 
        return false;

      if (! get_any_line(fin, list, "PAR", n))
        return false;
      if (! parse_PAR(list, &(*par_list)[n])) 
        return false;

      if (! get_any_line(fin, list, "DEALERPAR", n))
        return false;
      if (! parse_DEALERPAR(list, &(*dealerpar_list)[n]))
        return false;

      if (! get_any_line(fin, list, "PLAY", n))
        return false;
      if (! parse_PLAY(list, &(*play_list)[n]))
        return false;

      if (! get_any_line(fin, list, "TRACE", n))
        return false;
      if (! parse_TRACE(list, &(*trace_list)[n]))
        return false;
    }
  }

  fin.close();
  return true;
}


bool parse_PBN(
  const vector<string>& list,
  int& dealer,
  int& vul,
  DealPBN * dl)
{
  if (list.size() != 9)
  {
    cout << "PBN list does not have 9 elements: " << list.size() << "\n";
    return false;
  }

  if (! get_head_element(list[0], "PBN"))
    return false;
  if (! get_int_element(list[1], dealer, "PBN dealer failed"))
    return false;
  if (! get_int_element(list[2], vul, "PBN vul failed"))
    return false;
  if (! get_int_element(list[3], dl->trump, "PBN trump failed"))
    return false;
  if (! get_int_element(list[4], dl->first, "PBN trump failed"))
    return false;

  for (int i = 0; i < 3; i++)
  {
    dl->currentTrickSuit[i] = 0;
    dl->currentTrickRank[i] = 0;
  }

  if (! strip_quotes(
      list[5] + " " + list[6] + " " + list[7] + " " + list[8],
      dl->remainCards, "PBN string"))
    return false;

  return true;
}


bool parse_FUT(
  const vector<string>& list,
  FutureTricks * fut)
{
  if (list.size() < 2)
  {
    cout << "PBN list does not have 2+ elements: " << list.size() << endl;
    return false;
  }

  if (! get_head_element(list[0], "FUT"))
    return false;
  if (! get_int_element(list[1], fut->cards, "FUT cards"))
    return false;

  if (static_cast<int>(list.size()) != 4 * fut->cards + 2)
  {
    cout << "PBN list does not have right length: " << list.size() << endl;
    return false;
  }

  const unsigned nu = static_cast<unsigned>(fut->cards);
  for (unsigned c = 0; c < nu; c++)
    if (! get_int_element(list[c+2], fut->suit[c], "FUT suit"))
      return false;

  for (unsigned c = 0; c < nu; c++)
    if (! get_int_element(list[c+nu+2], fut->rank[c], "FUT rank"))
      return false;

  for (unsigned c = 0; c < nu; c++)
    if (! get_int_element(list[c+2*nu+2], fut->equals[c], "FUT equals"))
      return false;

  for (unsigned c = 0; c < nu; c++)
    if (! get_int_element(list[c+3*nu+2], fut->score[c], "FUT score"))
      return false;

  return true;
}


bool parse_TABLE(
  const vector<string>& list,
  DdTableResults * table)
{
  if (list.size() != 21)
  {
    cout << "Table list does not have 21 elements: " << list.size() << endl;
    return false;
  }

  if (! get_head_element(list[0], "TABLE"))
    return false;

  for (unsigned suit = 0; suit < DDS_STRAINS; suit++)
  {
    for (unsigned pl = 0; pl < DDS_HANDS; pl++)
    {
      if (! get_int_element(list[DDS_HANDS * suit + pl + 1],
          table->res_table[suit][pl], "TABLE entry"))
        return false;
    }
  }

  return true;
}


bool parse_PAR(
  const vector<string>& list,
  ParResults * par)
{
  if (list.size() < 9)
  {
    cout << "PAR list does not have 9+ elements: " << list.size() << endl;
    return false;
  }

  if (! get_head_element(list[0], "PAR"))
    return false;

  if (! strip_quotes(list[1] + " " + list[2], par->par_score[0], 
      "PAR score 0"))
    return false;

  if (! strip_quotes(list[3] + " " + list[4], par->par_score[1], 
      "PAR score 1"))
    return false;

  unsigned i = 5;
  string st = "";
  while (i < list.size())
  {
    st += " " + list[i++];
    if (st.back() == '"')
      break;
  }

  if (! strip_quotes(st.substr(1), par->par_contracts_string[0], 
      "PAR contract 0"))
    return false;

  st = "";
  while (i < list.size())
  {
    st += " " + list[i++];
    if (st.back() == '"')
      break;
  }

  if (! strip_quotes(st.substr(1), par->par_contracts_string[1], 
      "PAR contract 1"))
    return false;

  return true;
}


bool parse_DEALERPAR(
  const vector<string>& list,
  ParResultsDealer * par)
{
  const size_t l = list.size();
  if (l < 3)
  {
    cout << "PAR2 list does not have 3+ elements: " << l << endl;
    return false;
  }

  if (! get_head_element(list[0], "PAR2"))
    return false;

  if (! strip_quotes(list[1], par->score, "PBN string"))
    return false;

  unsigned no = 0;
  while (no+2 < l)
  {
    if (! strip_quotes(list[no+2], par->contracts[no], "PAR2 contract"))
      break;
    no++;
  }

  par->number = static_cast<int>(no);
  return true;
}


bool parse_PLAY(
  const vector<string>& list,
  PlayTracePBN * playp)
{
  if (list.size() != 3)
  {
    cout << "PLAY list does not have 3 elements: " << list.size() << endl;
    return false;
  }

  if (! get_head_element(list[0], "PLAY"))
    return false;

  if (! get_int_element(list[1], playp->number, "PLAY number"))
    return false;

  if (! strip_quotes(list[2], playp->cards, "PLAY string"))
    return false;

  return true;
}


bool parse_TRACE(
  const vector<string>& list,
  SolvedPlay * solvedp)
{
  if (list.size() < 2)
  {
    cout << "TRACE list does not have 2+ elements: " << list.size() << endl;
    return false;
  }

  if (! get_head_element(list[0], "TRACE"))
    return false;

  if (! get_int_element(list[1], solvedp->number, "TRACE number"))
    return false;

  for (unsigned i = 0; i < static_cast<unsigned>(solvedp->number); i++)
    if (! get_int_element(list[2+i], solvedp->tricks[i], "TRACE element"))
      return false;

  return true;
}


bool parseable_GIB(const string& line)
{
  if (line.size() != 88)
    return false;

  if (line.substr(67, 1) != ":")
    return false;

  return true;
}

int GIB_TO_DDS[4] = {1, 0, 3, 2};

bool parse_GIB(
  const string& line,
  DealPBN * dl, 
  DdTableResults * table)
{
  string st = "W:" + line.substr(0, 67);
  strcpy(dl->remainCards, st.c_str());

  int dds_strain, dds_hand;
  for (int s = 0; s < DDS_STRAINS; s++)
  {
    dds_strain = (s == 0 ? 4 : s - 1);
    for (unsigned h = 0; h < DDS_HANDS; h++)
    {
      dds_hand = GIB_TO_DDS[h];
      char const c = (line.substr(
        68 + 4*static_cast<unsigned>(s) + h, 1).c_str())[0];
      int d;
      if (c >= 48 && c <= 57) // 0, 9
        d = c-48;
      else if (c >= 65 && c <= 70) // A, F
        d = c-55;
      else
        return false;

      if (dds_hand & 1)
        d = 13 - d;

      table->res_table[dds_strain][dds_hand] = d;
    }
  }
  return true;
}


bool get_any_line(
  ifstream& fin,
  vector<string>& list,
  const string& tag,
  const int n)
{
  string line;
  if (! getline(fin, line))
  {
    cout << "Expected txt " << tag << " line " << n << endl;
    return false;
  }

  list.clear();
  splitIntoWords(line, list);
  return true;
}


bool get_head_element(
  const string& elem,
  const string& expected)
{
  if (elem != expected)
  {
    cout << "PBN list does not start with " << expected << 
      ": '" << elem << "'" << endl;
    return false;
  }
  else
    return true;
}


bool get_int_element(
  const string& elem,
  int& res,
  const string& errtext)
{
  if (! str2int(elem, res))
  {
    cout << errtext << ": '" << elem << "'\n";
    return false;
  }
  else
    return true;
}


bool strip_quotes(
  const string& st,
  char * cstr,
  const string& errtag)
{
  // Could just be past the last one.
  if (st.size() == 0)
    return false;

  if (st.front() != '\"' || st.back() != '\"')
  {
    cout << errtag << " not in quotations: '" << st << "'\n";
    return false;
  }
  strcpy(cstr, st.substr(1, st.size()-2).c_str());
  return true;
}


bool strip_quotes(
  const string& st,
  int& res,
  const string& errtag)
{
  if (st.front() != '\"' || st.back() != '\"')
  {
    cout << errtag << " not in quotations: '" << st << "'" << endl;
    return false;
  }

  if (! str2int(st.substr(1, st.size()-2).c_str(), res))
  {
    cout << st << " not an int" << endl;
    return false;
  }

  return true;
}


string trimTrailing(
  const string& text,
  const char c)
{
  unsigned Pos = static_cast<unsigned>(text.length());
  while (Pos >= 1 && text.at(Pos-1) == c)
    Pos--;

  if (Pos == 0)
    return "";
  else
    return text.substr(0, Pos);
}


void splitIntoWords(
  const string& text,
  vector<string>& words)
{
  // Split into words (split on \s+, effectively).
  unsigned Pos = 0;
  unsigned startPos = 0;
  bool isSpace = true;

  // It seems compilers have different ideas about files.
  const size_t tl = text.length();
  string ttext;
  if (text.back() == ' ')
    ttext = text.substr(0, tl-1);
  else if (text.at(tl-2) == ' ')
    ttext = text.substr(0, tl-2);
  else
    ttext = text;

  const unsigned l = static_cast<unsigned>(ttext.length());

  while (Pos < l)
  {
    if (ttext.at(Pos) == ' ')
    {
      if (! isSpace)
      {
        words.push_back(ttext.substr(startPos, Pos-startPos));
        isSpace = true;
      }
    }
    else if (isSpace)
    {
      isSpace = false;
      startPos = Pos;
    }
    Pos++;
  }

  if (! isSpace)
    words.push_back(ttext.substr(startPos, Pos-startPos));
}


bool str2int(
  const string& text,
  int& res)
{
  int i;
  size_t Pos;
  try
  {
    i = stoi(text, &Pos);
    if (Pos != text.size())
      return false;

  }
  catch ([[maybe_unused]] const invalid_argument& ia)
  {
    return false;
  }
  catch ([[maybe_unused]] const out_of_range& ia)
  {
    return false;
  }

  res = i;
  return true;
}

