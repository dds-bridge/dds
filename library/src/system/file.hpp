/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <fstream>
#include <string>

namespace dds {

/**
 * @brief Lazy-opening output file for debug/statistics logging.
 */
class File
{
  private:

    std::string fname_;
    std::ofstream fout_;

  public:

    File() = default;

    ~File();

    void Reset();

    void SetName(const std::string& fname_in);

    std::ofstream& GetStream();

    void Close();
};

}  // namespace dds
