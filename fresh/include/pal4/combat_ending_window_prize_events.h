#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>

namespace pal4 {

class CombatEndingWindowPrizeEvents final {
public:
    static constexpr std::uint32_t kHandleOffset = 0x38u;
    static constexpr std::uint32_t kKeyOffset = 0x3cu;
    static constexpr std::uint32_t kUpdateCallback = 0x545ae0u;

    // IDA 0x542e40/0x542e80.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t window, IEventSubscriptionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t window, IEventSubscriptionRuntime& runtime);
};

}  // namespace pal4
