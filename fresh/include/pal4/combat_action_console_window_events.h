#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class CombatActionConsoleWindowEvents final {
public:
    struct Binding {std::string_view key;std::uint32_t callback;};
    static std::span<const Binding> Bindings();
    static constexpr std::uint32_t kFirstSlot = 0x40u;
    static constexpr std::uint32_t kBindingCount = 3u;

    // IDA 0x5410b0/0x541130.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t window, IEventSubscriptionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t window, IEventSubscriptionRuntime& runtime);
};

}  // namespace pal4
