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
  Close();
  fname_.clear();
}

void File::SetName(const std::string& fname_in)
{
  if (fname_in == fname_)
    return;

  Close();
  fname_ = fname_in;
}

std::ofstream& File::GetStream()
{
  if (!fout_.is_open() && !fname_.empty())
    fout_.open(fname_);

  return fout_;
}

void File::Close()
{
  if (fout_.is_open())
    fout_.close();
}
