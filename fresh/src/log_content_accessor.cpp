#include "pal4/log_content_accessor.h"

namespace pal4 {

std::uint32_t LogContentAccessor::Get(
    const std::uint32_t log, const std::uint32_t destination,
    ILogContentRuntime& runtime) {
    runtime.CopyString(log + 12u, destination);
    return destination;
}

}  // namespace pal4
