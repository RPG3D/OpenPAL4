#include "pal4/combat_console_event_subscriptions.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<CombatConsoleEventBinding, CombatConsoleEventSubscriptions::kBindingCount>
    kBindings{{
        {"ae_physical_attack", 2u, 0x57b8d0u},
        {"ae_critical_attack", 2u, 0x57b8d0u},
        {"ae_fend_offed", 2u, 0x57b8d0u},
        {"ae_attack_missed", 2u, 0x540020u},
        {"ae_fleeing", 2u, 0x540170u},
        {"ae_revive", 2u, 0x57b8d0u},
        {"ae_flee_fail", 2u, 0x5401f0u},
        {"ae_protect_action", 2u, 0x540100u},
        {"ae_joint_attack", 2u, 0x57b8d0u},
        {"ae_react_attack", 2u, 0x5408b0u},
        {"ae_defende", 2u, 0x57b8d0u},
        {"re_attr_int_ext_changed", 2u, 0x5404b0u},
        {"me_attr_int_ext_changed", 2u, 0x5404b0u},
        {"ae_cast_magic", 2u, 0x540360u},
        {"ae_use_property", 2u, 0x5403d0u},
        {"ae_stunt", 2u, 0x540440u},
        {"ae_immunity_state_a", 3u, 0x540270u},
        {"ae_immunity_state_b", 3u, 0x540270u},
        {"ae_immunity_skill", 3u, 0x540270u},
        {"ae_immunity_normal", 3u, 0x540270u},
        {"add_picked_money", 2u, 0x540b40u},
        {"add_picked_property", 2u, 0x5409a0u},
        {"pick_up_fail", 2u, 0x540d20u},
    }};

}  // namespace

std::span<const CombatConsoleEventBinding> CombatConsoleEventSubscriptions::Bindings() {
    return kBindings;
}

std::uint32_t CombatConsoleEventSubscriptions::Subscribe(
    const std::uint32_t console, ICombatConsoleEventRuntime& runtime) {
    Unsubscribe(console, runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto& binding = kBindings[index];
        const auto key = runtime.EventKey(binding.name);
        result = runtime.Register(runtime.EventRouter(), binding.mode, key,
                                  console, binding.callback);
        runtime.WriteWord(console + 68u + 8u * index, result);
        runtime.WriteWord(console + 72u + 8u * index, key);
    }
    return result;
}

void CombatConsoleEventSubscriptions::Unsubscribe(
    const std::uint32_t console, ICombatConsoleEventRuntime& runtime) {
    EventSubscriptionSet::Clear(console, 68u, kBindingCount, runtime);
}

}  // namespace pal4
