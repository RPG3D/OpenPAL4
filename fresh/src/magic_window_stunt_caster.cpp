#include "pal4/magic_window_stunt_caster.h"

#include <bit>

namespace pal4 {

bool MagicWindowStuntCaster::Cast(
    const MagicWindowState& state, std::int32_t target_character_index,
    const MagicDbStuntRecord& stunt, IMagicWindowCastRuntime& cast_runtime,
    IMagicWindowRequirementsRuntime& requirements_runtime,
    IMagicWindowCostRuntime& cost_runtime) {
    const auto target = cast_runtime.GetPlayer(target_character_index);
    if (target == 0)
        return true;
    const auto caster = cast_runtime.GetPlayer(state.selected_character_index);
    if (caster == 0)
        return true;

    auto show_failure = [&](std::uint32_t trigger_id) {
        cast_runtime.PlaySound("WA05.WAV");
        const auto text = cast_runtime.TriggerText(trigger_id);
        cast_runtime.ShowInsufficientMessage(text, 0.5f, 0.4f, 4u, 0u);
    };
    if (stunt.required_stat > cast_runtime.PlayerValue(caster, 8u)) {
        show_failure(0x6cu);
        return true;
    }
    if (!MagicWindowRequirements::Check(state, stunt, requirements_runtime)) {
        show_failure(0x3bu);
        return true;
    }

    cast_runtime.PlaySound("WD001.WAV");
    const auto delta = 0u - static_cast<std::uint32_t>(stunt.required_stat);
    cast_runtime.AdjustPlayerValue(caster, 8u,
                                   std::bit_cast<std::int32_t>(delta));
    MagicWindowCostProcessor::Apply(stunt, cost_runtime);
    const auto type = cast_runtime.StuntType(stunt);
    if (type == 3) {
        for (std::int32_t index = 0; index < 4; ++index) {
            const auto candidate = cast_runtime.GetPlayer(index);
            if (candidate != 0 && cast_runtime.IsActive(candidate))
                cast_runtime.CastEffect(caster, stunt.effect_id, candidate);
        }
    } else {
        cast_runtime.CastEffect(caster, stunt.effect_id, target);
    }
    return true;
}

}  // namespace pal4
