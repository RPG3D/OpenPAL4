#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <span>

namespace pal4 {

class ICombatEventHandlersExtendedRuntime : public IEventSubscriptionRuntime {
public:
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class CombatEventHandlersExtended final {
public:
    struct Binding {std::uint32_t descriptor,mode,callback;std::string_view name;};
    static std::span<const Binding> Bindings();
    static constexpr std::uint32_t kHandlerCount = 8u;

    // IDA 0x57c0b0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t owner, ICombatEventHandlersExtendedRuntime& runtime);
    // IDA 0x57c1d0.
    static void Remove(std::uint32_t owner,
                       ICombatEventHandlersExtendedRuntime& runtime);
    // IDA 0x57c260.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t owner, ICombatEventHandlersExtendedRuntime& runtime);
};

}  // namespace pal4
