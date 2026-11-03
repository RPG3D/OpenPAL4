#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <span>

namespace pal4 {

class ScriptGameEventSubscriptions final {
public:
    struct Binding {std::string_view name;std::uint32_t mode,callback;};
    static constexpr std::uint32_t kFirstSlot = 60u;
    static constexpr std::uint32_t kBindingCount = 19u;
    static std::span<const Binding> Bindings();

    // IDA ScriptGameEventSubscriptions_Register at 0x54b000.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t owner, IEventSubscriptionRuntime& runtime);
    // IDA ScriptGameEventSubscriptions_Cleanup at 0x54b2f0.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t owner, IEventSubscriptionRuntime& runtime);
};

}  // namespace pal4
