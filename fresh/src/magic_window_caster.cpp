#include "pal4/magic_window_caster.h"

#include <bit>

namespace pal4 {

bool MagicWindowCaster::Cast(const MagicWindowState& state,
                             std::int32_t target_character_index,
                             const MagicDbSkillRecord& skill,
                             IMagicWindowCastRuntime& runtime) {
    const auto target = runtime.GetPlayer(target_character_index);
    if (target == 0)
        return true;
    const auto caster = runtime.GetPlayer(state.selected_character_index);
    if (caster == 0)
        return true;

    if (skill.cast_cost > runtime.PlayerValue(caster, 9u)) {
        runtime.PlaySound("WA05.WAV");
        const auto text = runtime.TriggerText(0x6bu);
        runtime.ShowInsufficientMessage(text, 0.35f, 0.4f, 4u, 0u);
        return true;
    }

    runtime.PlaySound("WD001.WAV");
    const auto delta = 0u - static_cast<std::uint32_t>(skill.cast_cost);
    runtime.AdjustPlayerValue(caster, 9u,
                              std::bit_cast<std::int32_t>(delta));
    const auto type = runtime.SkillType(skill);
    if (type == 3) {
        for (std::int32_t index = 0; index < 4; ++index) {
            const auto candidate = runtime.GetPlayer(index);
            if (candidate != 0 && runtime.IsActive(candidate))
                runtime.CastEffect(caster, skill.effect_id, candidate);
        }
    } else {
        runtime.CastEffect(caster, skill.effect_id, target);
    }
    return true;
}

}  // namespace pal4
