#include "utility/ArenaTLS.hpp"
#include "system/util/Arena.hpp"

namespace dds {
namespace tls {

thread_local Arena* arena = nullptr;

} // namespace tls
} // namespace dds
