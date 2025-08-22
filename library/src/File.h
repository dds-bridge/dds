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
/**
 * @brief File output manager for bridge double dummy solver.
 *
 * The File class provides a simple interface for managing output file streams
 * used for logging, statistics, and debugging during double dummy analysis.
 * It is used internally by various solver components and is not part of the public API.
 */
class File
{
  private:

    string fname;

    bool fileOpen;

    ofstream fout;

  public:

    /**
     * @brief Construct a new File object.
     *
     * Initializes the file output manager and prepares for output operations.
     */
    File();

    /**
     * @brief Destroy the File object and clean up resources.
     *
     * Closes the file stream and releases any associated resources.
     */
    ~File();

    void Reset();

    void SetName(const string& fnameIn);

    ofstream& GetStream();

    void Close();
};
}  // namespace dds
using dds::File;


#endif
