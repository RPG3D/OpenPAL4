#include "pal4/combat_action_console_window_events.h"

#include <array>
#include <string_view>

namespace pal4 {
namespace {

constexpr std::array<CombatActionConsoleWindowEvents::Binding, CombatActionConsoleWindowEvents::kBindingCount>
    kBindings{{
        {"re_start_ui_control", 0x541570u},
        {"re_close_ui_control", 0x5415a0u},
        {"combat_start", 0x5415a0u},
    }};

}  // namespace

std::span<const CombatActionConsoleWindowEvents::Binding> CombatActionConsoleWindowEvents::Bindings(){return kBindings;}

std::uint32_t CombatActionConsoleWindowEvents::Initialize(
    const std::uint32_t window, IEventSubscriptionRuntime& runtime) {
    (void)Cleanup(window, runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto& binding = kBindings[index];
        const auto key = runtime.EventKey(binding.key);
        result = runtime.Register(runtime.EventRouter(), 2u, key,
                                  window, binding.callback);
        const auto slot = window + kFirstSlot + index * 8u;
        if (index == 2u) runtime.WriteWord(slot + 4u, key);
        runtime.WriteWord(slot, result);
        if (index != 2u) runtime.WriteWord(slot + 4u, key);
    }
    return result;
}

std::uint32_t CombatActionConsoleWindowEvents::Cleanup(
    const std::uint32_t window, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(window, kFirstSlot, kBindingCount, runtime);
    return 0u;
}

}  // namespace pal4
