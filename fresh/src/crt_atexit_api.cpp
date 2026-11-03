#include "pal4/crt_atexit_stub.h"
#include "pal4/crt_noop_atexit_registration.h"

#include <cstdlib>

namespace pal4 {

int CrtAtexitApi::Register(CrtExitCallback callback) {
    // Never register an original EXE integer address as a host function.
    return callback == nullptr ? -1 : std::atexit(callback);
}

int CrtNoopAtExitRuntime::RegisterAtExit(std::uint32_t callback) {
    // Translate only the reviewed nullsub_82 identifier to a host callback.
    // The original executable's integer address is not callable in this host.
    if (callback != CrtNoopAtExitRegistration::kCallback) {
        return -1;
    }
    return std::atexit(+[] {});
}

}  // namespace pal4
