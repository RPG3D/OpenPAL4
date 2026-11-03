#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <span>

namespace pal4 {

using ICombatConsoleEventRuntime = IEventSubscriptionRuntime;

struct CombatConsoleEventBinding {
    std::string_view name;
    std::uint32_t mode;
    std::uint32_t callback;
};

class CombatConsoleEventSubscriptions final {
public:
    static constexpr std::uint32_t kBindingCount = 23u;
    [[nodiscard]] static std::span<const CombatConsoleEventBinding> Bindings();

    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t console, ICombatConsoleEventRuntime& runtime);
    static void Unsubscribe(std::uint32_t console,
                            ICombatConsoleEventRuntime& runtime);
};

}  // namespace pal4
