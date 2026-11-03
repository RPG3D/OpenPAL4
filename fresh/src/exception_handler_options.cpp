#include "pal4/exception_handler_options.h"

namespace pal4 {

std::uint32_t ExceptionHandlerOptions::Set(
    const std::uint32_t first, const std::uint32_t second,
    const std::uint32_t third, const std::uint32_t fourth,
    const std::uint32_t fifth) {
    values_ = {first, second, third, fourth, fifth};
    return fourth;
}

}  // namespace pal4
