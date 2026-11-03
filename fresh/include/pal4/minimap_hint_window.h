#pragma once

#include <cstdint>
#include <string_view>
#include "pal4/hint_window.h"

namespace pal4 {

class IMinimapHintWindowRuntime : public IHintWindowRuntime {
public:
    virtual void Reallocate(std::uint32_t string_record,
                            std::uint32_t length) = 0;
};

class MinimapHintWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x844084u;
    static constexpr std::string_view kLayout = "minimaphintWindow";
    static constexpr std::string_view kClockName = "MinimapHintWindowClock";

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, IMinimapHintWindowRuntime& runtime);
};

}  // namespace pal4
