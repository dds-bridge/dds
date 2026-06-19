/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "file.hpp"

dds::File::~File()
{
  Close();
}

void dds::File::Reset()
{
  Close();
  fname_.clear();
}

void dds::File::SetName(const std::string& fname_in)
{
  if (fname_in == fname_)
    return;

  Close();
  fname_ = fname_in;
}

std::ofstream& dds::File::GetStream()
{
  if (!fout_.is_open() && !fname_.empty())
    fout_.open(fname_);

  return fout_;
}

void dds::File::Close()
{
  if (fout_.is_open())
    fout_.close();
}
