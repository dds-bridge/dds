/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_FILE_H
#define DDS_FILE_H

#include <iostream>
#include <fstream>

using namespace std;

namespace dds {
class File
{
  private:

    string fname;

    bool fileOpen;

    ofstream fout;

  public:

    File();

    ~File();

    void Reset();

    void SetName(const string& fnameIn);

    ofstream& GetStream();

    void Close();
};
}  // namespace dds
using dds::File;


#endif
