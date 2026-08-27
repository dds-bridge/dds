/*
   DDS, a bridge double dummy solver.

   See LICENSE and README.
*/

/// @file par_fuzz.cpp
/// @brief Fuzz harness for the par calculation entry points.
///
/// The par API derives contract levels from a caller-supplied DdTableResults
/// and formats them into fixed-size character buffers. An unvalidated table
/// overflowed those buffers (see library/tests/par_validation_test.cpp); this
/// harness drives all five exported entry points with arbitrary tables and
/// vulnerability values so any further gap in that validation surfaces here.

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <api/dll.h>

namespace {

/// Consume `n` bytes from the input, or return false if too few remain.
class Reader
{
public:
  Reader(const uint8_t * data, size_t size) : data_(data), left_(size) {}

  auto take(void * out, size_t n) -> bool
  {
    if (left_ < n)
      return false;
    if (n == 0)
      return true;  // memcpy's source is declared nonnull; data_ may be null.
    std::memcpy(out, data_, n);
    data_ += n;
    left_ -= n;
    return true;
  }

private:
  const uint8_t * data_;
  size_t left_;
};

}  // namespace

extern "C" auto LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/) -> int
{
  // Nothing to configure; defined so the corpus-replay driver links.
  return 0;
}

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int
{
  Reader reader(data, size);

  DdTableResults table;
  if (!reader.take(&table, sizeof(table)))
    return 0;

  uint8_t selector = 0;
  if (!reader.take(&selector, sizeof(selector)))
    return 0;

  // Exercise the full legal vulnerability range plus out-of-range values,
  // since DealerPar() indexes a lookup table with this parameter.
  int const vulnerable = static_cast<int>(selector % 8) - 2;
  int const dealer = static_cast<int>((selector / 8) % 6) - 1;

  ParResults par_results;
  std::memset(&par_results, 0, sizeof(par_results));
  Par(&table, &par_results, vulnerable);

  ParResultsDealer sides[2];
  std::memset(sides, 0, sizeof(sides));
  SidesPar(&table, sides, vulnerable);

  ParResultsMaster sides_bin[2];
  std::memset(sides_bin, 0, sizeof(sides_bin));
  SidesParBin(&table, sides_bin, vulnerable);

  ParResultsDealer dealer_res;
  std::memset(&dealer_res, 0, sizeof(dealer_res));
  DealerPar(&table, &dealer_res, dealer, vulnerable);

  ParResultsMaster dealer_bin;
  std::memset(&dealer_bin, 0, sizeof(dealer_bin));
  DealerParBin(&table, &dealer_bin, dealer, vulnerable);

  return 0;
}
