/*
   DDS, a bridge double dummy solver.

   See LICENSE and README.
*/

/// @file calc_all_tables_fuzz.cpp
/// @brief Fuzz harness for the batch table entry points.
///
/// The four single-deal harnesses drive one deal at a time and so never
/// exercised the batch entry points' *count* handling. That is where
/// CalcAllTablesPBNN() converted no_of_tables records into a fixed-size local
/// before validating anything -- a stack-buffer-overflow write found in
/// review rather than by fuzzing. This harness covers that surface.
///
/// Two different kinds of count are involved, and only one of them is the
/// library's business:
///
///   - DdTableDeals::no_of_tables and DdTableDealsPBN::no_of_tables are
///     fields inside a fixed-capacity struct, so any value at all is
///     legitimate fuzzer input and the library must bound it itself. The
///     harness passes them through verbatim.
///
///   - CalcAllTablesX() takes a count alongside a caller-allocated array, so
///     passing a count larger than the array would be a harness bug, not a
///     library one. The harness allocates exactly the number it declares, and
///     caps it so one input cannot allocate unboundedly.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

#include <api/dll.h>

namespace {

/// Upper bound on the array CalcAllTablesX() is asked to read, to keep any
/// single input's allocation and solve time bounded.
constexpr int kMaxDealsForX = 32;

/// Deal slots perturbed from the input. Every *other* slot is pre-filled with
/// a valid deal rather than left zeroed. That matters: CalcAllTablesPBNN()
/// stops at the first slot convert_from_pbn() rejects, so with zeroed slots
/// the conversion loop returns RETURN_PBN_FAULT immediately and never reaches
/// the count boundary that the whole harness exists to exercise.
constexpr int kMaxPopulated = 4;

/// A valid PBN deal used to fill the slots the input does not perturb. One
/// card per hand rather than a full 52: this harness targets count and batch
/// handling, not search depth -- the single-deal harnesses cover that -- and
/// a full deal in every slot drops throughput to a few executions a second.
/// Seeds can still place full deals in the perturbed slots.
constexpr char kFillPbn[] = "N:A... Q... K... J...";

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

  auto byte(uint8_t fallback) -> uint8_t
  {
    uint8_t v = fallback;
    take(&v, 1);
    return v;
  }

  auto remaining() const -> size_t { return left_; }

private:
  const uint8_t * data_;
  size_t left_;
};

/// The binary equivalent of kFillPbn: one spade each, so every slot is a
/// valid deal that solves immediately.
auto legal_deal() -> DdTableDeal
{
  DdTableDeal deal;
  std::memset(&deal, 0, sizeof(deal));
  deal.cards[0][0] = 0x4000;  // A
  deal.cards[1][0] = 0x1000;  // Q
  deal.cards[2][0] = 0x2000;  // K
  deal.cards[3][0] = 0x0800;  // J
  return deal;
}

}  // namespace

extern "C" auto LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/) -> int
{
  // One worker keeps runs deterministic and avoids per-input thread setup.
  SetMaxThreads(1);
  return 0;
}

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int
{
  Reader reader(data, size);

  // Passed to the library verbatim: bounding it is the library's job.
  int32_t raw_count = 0;
  if (!reader.take(&raw_count, sizeof(raw_count)))
    return 0;

  int trump_filter[DDS_STRAINS];
  for (int k = 0; k < DDS_STRAINS; k++)
    trump_filter[k] = (reader.byte(0) & 1);

  // -1 disables the par calculation; 0..3 select a vulnerability. Values
  // outside that are worth passing too.
  int const mode = static_cast<int>(reader.byte(0) % 8) - 2;

  uint8_t const selector = reader.byte(0);
  int const populate = static_cast<int>(reader.byte(0) % (kMaxPopulated + 1));

  auto results = std::make_unique<DdTablesRes>();
  std::memset(results.get(), 0, sizeof(DdTablesRes));
  auto par = std::make_unique<AllParResults>();
  std::memset(par.get(), 0, sizeof(AllParResults));

  switch (selector % 3)
  {
    case 0:
    {
      auto deals = std::make_unique<DdTableDeals>();
      std::memset(deals.get(), 0, sizeof(DdTableDeals));
      deals->no_of_tables = raw_count;

      DdTableDeal const fill = legal_deal();
      for (auto & slot : deals->deals)
        slot = fill;

      // Perturb the first few from the input so malformed deals are
      // reachable too, while the rest stay valid.
      for (int i = 0; i < populate; i++)
        reader.take(&deals->deals[i], sizeof(DdTableDeal));

      CalcAllTablesN(deals.get(), mode, trump_filter,
                     results.get(), par.get(), 1);
      break;
    }

    case 1:
    {
      auto deals = std::make_unique<DdTableDealsPBN>();
      std::memset(deals.get(), 0, sizeof(DdTableDealsPBN));
      deals->no_of_tables = raw_count;

      for (auto & slot : deals->deals)
        std::memcpy(slot.cards, kFillPbn, sizeof(kFillPbn));

      for (int i = 0; i < populate; i++)
      {
        // cards is a fixed char[80] the library reads as a C string, so the
        // harness terminates it; feeding a non-terminated array would report
        // a harness bug as a library one.
        auto & cards = deals->deals[i].cards;
        reader.take(cards, sizeof(cards) - 1);
        cards[sizeof(cards) - 1] = '\0';
      }

      CalcAllTablesPBNN(deals.get(), mode, trump_filter,
                        results.get(), par.get(), 1);
      break;
    }

    default:
    {
      // Count and array must agree here: see the file comment.
      int num_deals = raw_count < 0 ? 0 : raw_count % (kMaxDealsForX + 1);

      std::vector<DdTableDeal> deals(static_cast<size_t>(num_deals));
      for (int i = 0; i < num_deals; i++)
      {
        deals[static_cast<size_t>(i)] = legal_deal();
        if (i < populate)
          reader.take(&deals[static_cast<size_t>(i)], sizeof(DdTableDeal));
      }

      // CalcAllTablesX writes results[m] and par[m] for m < num_deals (see
      // the writes near the end of CalcAllTablesX), so one entry each per
      // deal. The +1 keeps .data() non-null when num_deals is 0.
      std::vector<DdTableResults> table_results(
        static_cast<size_t>(num_deals) + 1);
      std::vector<ParResults> par_results(
        static_cast<size_t>(num_deals) + 1);

      CalcAllTablesX(num_deals, deals.data(), mode, trump_filter,
                     table_results.data(), par_results.data(), 1);
      break;
    }
  }

  return 0;
}
