#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <array>

namespace pal4 {

class CombatEndSoundEvents final {
public:
    struct Binding { std::uint32_t descriptor, mode, callback; const char* name; };
    static constexpr std::array<Binding,2> Bindings() {
        return {{{0x8b8b84u,2u,0x579bd0u,"combat_prepare"},
                 {0x8b8b8cu,2u,0x579d00u,"combat_over"}}};
    }
    static constexpr std::uint32_t kFirstDescriptor = 0x8b8b84u;
    static constexpr std::uint32_t kSecondDescriptor = 0x8b8b8cu;

    // IDA 0x579ac0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t owner, IEventSubscriptionRuntime& runtime);
    // IDA 0x579b10.
    static void Remove(std::uint32_t owner,
                       IEventSubscriptionRuntime& runtime);
    // IDA 0x579ba0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t owner, IEventSubscriptionRuntime& runtime);
};

}  // namespace pal4
