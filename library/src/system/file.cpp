/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "file.hpp"

File::~File()
{
  Close();
}

void File::Reset()
{
  fname_.clear();
  file_open_ = false;
}

void File::SetName(const std::string& fname_in)
{
  fname_ = fname_in;
}

std::ofstream& File::GetStream()
{
  if (!file_open_)
  {
    fout_.open(fname_);
    file_open_ = true;
  }

  return fout_;
}

void File::Close()
{
  if (file_open_)
  {
    fout_.close();
    file_open_ = false;
  }
}
