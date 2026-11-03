#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace pal4 {

// Original7DEE60 appends every prepared context to one selected stack. Native
// owners retain separate typed lifetimes; a shared ordinal preserves that push
// order when selecting among their tops, without assigning category priority.
class NativeScriptPushOrder final {
public:
    std::uint64_t Next() {
        if (last_ == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("PAL4 script push order exhausted");
        return ++last_;
    }
private:
    std::uint64_t last_{};
};

} // namespace pal4
