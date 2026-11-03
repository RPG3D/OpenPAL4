#include "pal4/combat_event_handlers_all.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<CombatEventHandlersAll::Binding, CombatEventHandlersAll::kHandlerCount>
    kHandlers{{
        {0x8b8b88u, 3u, 0x57a910u,"combat_start"},
        {0x8b8b84u, 3u, 0x57a770u,"combat_prepare"},
        {0x8b8b8cu, 1u, 0x57a920u,"combat_over"},
        {0x8b8b94u, 2u, 0x57b0a0u,"add_money"},
        {0x8b8b9cu, 2u, 0x57b0c0u,"add_property"},
        {0x8b8ba8u, 2u, 0x57b0f0u,"summon"},
        {0x8b8b90u, 2u, 0x57b8d0u,"close_system"},
        {0x8b5998u, 3u, 0x57b220u,"ae_become_dead"},
        {0x8b599cu, 3u, 0x57b520u,"ae_become_alive"},
        {0x8b631cu, 3u, 0x57b570u,"re_state_changed"},
        {0x8b5fc8u, 3u, 0x57b570u,"me_state_changed"},
        {0x8b5954u, 2u, 0x57b8d0u,"ae_start_action"},
        {0x8b5958u, 2u, 0x57b5e0u,"ae_end_action"},
        {0x8b5988u, 2u, 0x57b710u,"ae_fleed"},
        {0x8b62c8u, 2u, 0x57b120u,"re_new_added"},
        {0x8b5f88u, 2u, 0x57b1a0u,"me_new_added"},
        {0x8b8bb0u, 2u, 0x57b7b0u,"quick_end"},
    }};

}  // namespace

std::span<const CombatEventHandlersAll::Binding> CombatEventHandlersAll::Bindings(){return kHandlers;}

std::uint32_t CombatEventHandlersAll::Initialize(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
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

void CombatEventHandlersAll::Remove(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(owner, 0u, kHandlerCount, runtime);
}

std::uint32_t CombatEventHandlersAll::Construct(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
    for (std::uint32_t index = 0u; index < kHandlerCount; ++index) {
        const auto slot = owner + index * 8u;
        runtime.WriteWord(slot, 0u);
        runtime.WriteWord(slot + 4u, 0u);
    }
    static_cast<void>(Initialize(owner, runtime));
    return owner;
}

}  // namespace pal4
