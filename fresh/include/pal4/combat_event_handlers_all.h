#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <span>

namespace pal4 {

class CombatEventHandlersAll final {
public:
    struct Binding {std::uint32_t descriptor,mode,callback;std::string_view name;};
    static std::span<const Binding> Bindings();
    static constexpr std::uint32_t kHandlerCount = 17u;

    // IDA 0x57a460.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t owner, IEventSubscriptionRuntime& runtime);
    // IDA 0x57a6b0.
    static void Remove(std::uint32_t owner,
                       IEventSubscriptionRuntime& runtime);
    // IDA 0x57a740.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t owner, IEventSubscriptionRuntime& runtime);
};

}  // namespace pal4
