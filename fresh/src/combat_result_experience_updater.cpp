#include "pal4/combat_result_experience_updater.h"
#include "pal4/combat_reward_record_layout.h"

#include <cmath>
#include <string>

namespace pal4 {

std::uint8_t CombatResultExperienceUpdater::Update(
    const std::uint32_t element, const float elapsed,
    ICombatResultExperienceRuntime& runtime) {
    const auto actor = runtime.ReadWord(element + 68u);
    const auto experience_base = runtime.ReadWord(element + 64u);
    const auto actor_slot = runtime.ReadInt(actor + 1192u);
    const auto record = experience_base + 40u *
        static_cast<std::uint32_t>(actor_slot) + 52u;
    const auto precise_delta = static_cast<double>(elapsed) *
        runtime.ReadFloat(element + 76u);
    const auto delta = static_cast<float>(precise_delta);
    const auto precise_current =
        static_cast<double>(runtime.ReadFloat(element + 72u)) + precise_delta;
    runtime.WriteFloat(element + 72u,
                       static_cast<float>(precise_current));
    const auto cap = runtime.ReadInt(record + CombatRewardRecordLayout::kUpdatedExperience);
    if (precise_current > static_cast<double>(cap))
        runtime.WriteFloat(element + 72u, static_cast<float>(cap));

    const auto base = runtime.ReadInt(record + CombatRewardRecordLayout::kPreviousExperience);
    const auto shown = runtime.FloatToInt(
        static_cast<double>(runtime.ReadFloat(element + 72u)) - base);
    const auto row = runtime.ReadWord(element + 60u);
    runtime.SetWindowText(runtime.ReadWord(row + 20u),
                          std::to_string(shown));

    auto remaining = runtime.ReadFloat(element + 84u) - delta;
    runtime.WriteFloat(element + 84u, remaining);
    const auto level = runtime.ReadInt(element + 80u);
    const auto current_actor = runtime.ReadWord(element + 68u);
    const auto threshold = runtime.ActorMetric(current_actor, 4u);
    bool skip_level_up = false;
    if (level >= threshold) {
        const auto minimum = static_cast<float>(
            runtime.ReadInt(element + 88u));
        if (remaining <= minimum) {
            remaining = minimum;
            runtime.WriteFloat(element + 84u, remaining);
            skip_level_up = true;
        }
    }
    if (!skip_level_up && remaining <= 0.0f) {
        const auto next_level = level + 1;
        runtime.WriteInt(element + 80u, next_level);
        if (next_level <= 99) {
            const auto level_actor = runtime.ReadWord(element + 68u);
            const auto actor_id = runtime.ActorId(level_actor);
            const auto next_experience = runtime.NextLevelExperience(
                actor_id, next_level);
            const auto updated_remaining =
                static_cast<float>(next_experience) -
                runtime.ReadFloat(element + 72u);
            runtime.WriteFloat(element + 84u, updated_remaining);
            const auto hint = runtime.ReadWord(element + 96u);
            const auto level_row = runtime.ReadWord(element + 60u);
            runtime.ApplyLevelUp(level_row, next_level, level_actor,
                                 hint, record);
        }
    }

    const auto remaining_text = runtime.FloatToInt(
        runtime.ReadFloat(element + 84u));
    const auto final_row = runtime.ReadWord(element + 60u);
    runtime.SetWindowText(runtime.ReadWord(final_row + 24u),
                          std::to_string(remaining_text));
    return 1u;
}

std::uint8_t CombatResultExperienceUpdater::Complete(
    const std::uint32_t element,
    ICombatResultExperienceRuntime& runtime) {
    const auto actor = runtime.ReadWord(element + 68u);
    const auto threshold = runtime.ActorMetric(actor, 4u);
    if (runtime.ReadInt(element + 80u) < threshold)
        return 0u;
    const auto minimum = runtime.ReadInt(element + 88u);
    const auto remaining = runtime.ReadFloat(element + 84u);
    if (static_cast<float>(minimum) < remaining ||
        std::isnan(remaining))
        return 0u;

    const auto current_actor = runtime.ReadWord(element + 68u);
    const auto slot = runtime.ReadInt(current_actor + 1192u);
    const auto base = runtime.ReadWord(element + 64u);
    const auto record = base + 40u * static_cast<std::uint32_t>(slot) + 52u;
    const auto current_metric = runtime.ActorMetric(current_actor, 5u);
    const auto current_text = current_metric - runtime.ReadInt(record + CombatRewardRecordLayout::kPreviousExperience);
    const auto row = runtime.ReadWord(element + 60u);
    runtime.SetWindowText(runtime.ReadWord(row + 20u),
                          std::to_string(current_text));
    const auto minimum_text = runtime.ReadInt(element + 88u);
    const auto final_row = runtime.ReadWord(element + 60u);
    runtime.SetWindowText(runtime.ReadWord(final_row + 24u),
                          std::to_string(minimum_text));
    return 1u;
}

}  // namespace pal4
