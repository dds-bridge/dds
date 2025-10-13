// Lightweight TLS hook to make an Arena available in hot paths without
// threading a context parameter through legacy code.
#pragma once

namespace dds {
class Arena; // forward decl

namespace tls {
// Set by call sites that know the current thread's Arena (if any).
// Consumers can treat this as optional: nullptr means "no arena".
extern thread_local Arena* arena;
}

} // namespace dds
