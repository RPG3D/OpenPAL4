#include "pal4/combat_script_event_handlers.h"

#include <array>

namespace pal4 {

namespace {

constexpr std::array<CombatScriptEventHandlers::Binding, CombatScriptEventHandlers::kHandlerCount>
    kHandlers{{
        {1u, 0x8b8b8cu, 0x574970u,"combat_over"},
        {3u, 0x8b596cu, 0x574600u,"ae_physical_attack"},
        {3u, 0x8b5980u, 0x574650u,"ae_stunt"},
        {3u, 0x8b597cu, 0x5746a0u,"ae_cast_magic"},
        {3u, 0x8b5984u, 0x5746f0u,"ae_use_property"},
        {3u, 0x8b5998u, 0x574740u,"ae_become_dead"},
        {3u, 0x8b5970u, 0x574790u,"ae_critical_attack"},
        {3u, 0x8b5974u, 0x5747e0u,"ae_fend_offed"},
        {3u, 0x8b594cu, 0x574b90u,"ae_revive"},
        {3u, 0x8b5968u, 0x574b60u,"ae_react_attack"},
        {3u, 0x8b5978u, 0x574b40u,"ae_attack_missed"},
        {3u, 0x8b5964u, 0x574800u,"ae_joint_attack"},
        {3u, 0x8b5960u, 0x574830u,"ae_protect_action"},
        {3u, 0x8b598cu, 0x574860u,"ae_flee_fail"},
        {3u, 0x8b5988u, 0x574890u,"ae_fleed"},
        {3u, 0x8b8b88u, 0x5748c0u,"combat_start"},
        {3u, 0x8b62f0u, 0x574b10u,"re_can_take_action"},
    }};

}  // namespace

std::span<const CombatScriptEventHandlers::Binding> CombatScriptEventHandlers::Bindings(){return kHandlers;}

std::uint32_t CombatScriptEventHandlers::Initialize(
    std::uint32_t owner, ICombatScriptEventHandlerRuntime& runtime) {
    runtime.RemoveAll(owner);
    std::uint32_t handle = 0u;
    for (std::uint32_t index = 0u; index < kHandlers.size(); ++index) {
        const auto& spec = kHandlers[index];
        handle = runtime.Register(runtime.GetRouter(), spec.mode,
                                  spec.descriptor, owner, spec.callback);
        const auto entry = owner + index * 8u;
        if (index + 1u == kHandlers.size()) {
            runtime.WriteWord(entry + 4u, spec.descriptor);
            runtime.WriteWord(entry, handle);
        } else {
            runtime.WriteWord(entry, handle);
            runtime.WriteWord(entry + 4u, spec.descriptor);
        }
    }
    return handle;
}

}  // namespace pal4
