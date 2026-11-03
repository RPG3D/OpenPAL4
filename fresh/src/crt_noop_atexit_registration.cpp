#include "pal4/crt_noop_atexit_registration.h"

#include <cstdint>

namespace pal4 {

int CrtNoopAtExitRegistration::Run(
    std::uint8_t& flag, ICrtNoopAtExitRuntime& runtime) {
    if ((flag & 1u) == 0u) flag = static_cast<std::uint8_t>(flag | 1u);
    return runtime.RegisterAtExit(kCallback);
}

}  // namespace pal4
