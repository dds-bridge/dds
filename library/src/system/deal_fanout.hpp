#pragma once

#include <api/dll.h>

namespace dds
{
namespace internal
{

/**
 * @brief Cheap structural difficulty estimate (cards only, trump-independent).
 *
 * Per hand, sum the number of card groups per suit, with a bonus for voids.
 * Used to dispatch the most difficult deals first in parallel batch calculations.
 */
auto deal_fanout(const Deal& dl) -> int;

}  // namespace internal
}  // namespace dds
