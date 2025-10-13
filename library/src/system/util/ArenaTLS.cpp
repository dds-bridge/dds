#include "system/util/ArenaTLS.h"
#include "system/util/Arena.h"

namespace dds {
namespace tls {

thread_local Arena* arena = nullptr;

} // namespace tls
} // namespace dds
