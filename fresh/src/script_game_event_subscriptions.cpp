#include "pal4/script_game_event_subscriptions.h"

#include <array>
#include <string_view>

namespace pal4 {
namespace {

using Binding=ScriptGameEventSubscriptions::Binding;

constexpr std::array<Binding, ScriptGameEventSubscriptions::kBindingCount>
    kBindings{{
        {"me_can_take_action", 2u, 0x54d670u},
        {"re_can_take_action", 2u, 0x54d670u},
        {"show_monster_tips", 2u, 0x54cfd0u},
        {"hide_monster_tips", 2u, 0x54da00u},
        {"re_moved_at_action_sequence", 2u, 0x54c9f0u},
        {"me_moved_at_action_sequence", 2u, 0x54ca30u},
        {"joint_attack_respond", 2u, 0x54cc60u},
        {"select_join_atk", 2u, 0x54ccb0u},
        {"combat_over", 1u, 0x54ca70u},
        {"combat_start", 1u, 0x54c9e0u},
        {"re_join_stage", 3u, 0x54c9e0u},
        {"me_join_stage", 3u, 0x54c9e0u},
        {"me_leave_stage", 3u, 0x54c9e0u},
        {"re_leave_stage", 3u, 0x54c9e0u},
        {"ae_become_dead", 3u, 0x54c9e0u},
        {"ae_become_alive", 3u, 0x54c9e0u},
        {"ae_start_action", 3u, 0x54da00u},
        {"ae_ui_select", 3u, 0x54d980u},
        {"ae_ui_unselect", 3u, 0x54d9d0u},
    }};

}  // namespace

std::span<const ScriptGameEventSubscriptions::Binding> ScriptGameEventSubscriptions::Bindings(){return kBindings;}

std::uint32_t ScriptGameEventSubscriptions::Register(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
    (void)Cleanup(owner, runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto& binding = kBindings[index];
        const auto key = runtime.EventKey(binding.name);
        result = runtime.Register(runtime.EventRouter(), binding.mode, key,
                                  owner, binding.callback);
        const auto slot = owner + kFirstSlot + index * 8u;
        if (index == kBindingCount - 1u)
            runtime.WriteWord(slot + 4u, key);
        runtime.WriteWord(slot, result);
        if (index != kBindingCount - 1u)
            runtime.WriteWord(slot + 4u, key);
    }
    return result;
}

std::uint32_t ScriptGameEventSubscriptions::Cleanup(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(owner, kFirstSlot, kBindingCount, runtime);
    return 0u;
}

}  // namespace pal4
