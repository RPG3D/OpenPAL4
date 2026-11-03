#include "pal4/combat_event_handlers_extended.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<CombatEventHandlersExtended::Binding, CombatEventHandlersExtended::kHandlerCount>
    kHandlers{{
        {0x8b5f9cu, 2u, 0x57c370u,"me_can_take_action"},
        {0x8b62f0u, 2u, 0x57c290u,"re_can_take_action"},
        {0x8b62d0u, 2u, 0x57c440u,"re_select_command"},
        {0x8b62ccu, 2u, 0x57c4b0u,"re_select_command_target"},
        {0x8b62dcu, 2u, 0x57c6c0u,"re_substitute"},
        {0x8b8b7cu, 2u, 0x57c840u,"repeat_action"},
        {0x8b8b8cu, 1u, 0x57c6b0u,"combat_over"},
        {0x8b8b74u, 1u, 0x57c920u,"start_select_target"},
    }};

}  // namespace

std::span<const CombatEventHandlersExtended::Binding> CombatEventHandlersExtended::Bindings(){return kHandlers;}

std::uint32_t CombatEventHandlersExtended::Initialize(
    std::uint32_t owner, ICombatEventHandlersExtendedRuntime& runtime) {
    Remove(owner, runtime);
    std::uint32_t handle = 0u;
    for (std::uint32_t index = 0u; index < kHandlers.size(); ++index) {
        const auto& spec = kHandlers[index];
        handle = runtime.Register(runtime.EventRouter(), spec.mode,
                                  spec.descriptor, owner, spec.callback);
        const auto slot = owner + index * 8u;
        if (index + 1u == kHandlers.size()) {
            runtime.WriteWord(slot + 4u, spec.descriptor);
            runtime.WriteWord(slot, handle);
        } else {
            runtime.WriteWord(slot, handle);
            runtime.WriteWord(slot + 4u, spec.descriptor);
        }
    }
    return handle;
}

void CombatEventHandlersExtended::Remove(
    std::uint32_t owner, ICombatEventHandlersExtendedRuntime& runtime) {
    EventSubscriptionSet::Clear(owner, 0u, kHandlerCount, runtime);
}

std::uint32_t CombatEventHandlersExtended::Construct(
    std::uint32_t owner, ICombatEventHandlersExtendedRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        const auto slot = owner + index * 8u;
        runtime.WriteWord(slot, 0u);
        runtime.WriteWord(slot + 4u, 0u);
    }
    (void)Initialize(owner, runtime);
    runtime.WriteByte(owner + 72u, 0u);
    return owner;
}

}  // namespace pal4
