/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

// Keeps track of debug files without opening and closing them
// all the time, and without creating empty files.


#include "File.h"


File::File()
{
  File::Reset();
}


File::~File()
{
  File::Close();
}


void File::Reset()
{
  fname = "";
  fileOpen = false;
}


void File::SetName(const string& fnameIn)
{
  // No error checking!
  fname =  fnameIn;
}


ofstream& File::GetStream()
{
  if (! fileOpen)
  {
    fout.open(fname);
    fileOpen = true;
  }
  
  return fout;
}


void File::Close()
{
  if (fileOpen)
  {
    fout.close();
    fileOpen = false;
  }
}

