#include "pal4/ai_action_task.h"

#include <algorithm>
#include <utility>

namespace pal4 {

double AiTargetWeightSelector::SkillProperty(
    const std::uint32_t skill, const std::int32_t index,
    IAiTargetWeightRuntime& runtime) {
    if (static_cast<std::uint32_t>(index) > 12u)
        return static_cast<double>(index);
    return static_cast<double>(runtime.ReadFloat(
        skill + 4u * (static_cast<std::uint32_t>(index) + 4u)));
}

double AiTargetWeightSelector::RandomRange(
    float first, float second, IAiTargetWeightRuntime& runtime) {
    if (first > second) std::swap(first, second);
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      static_cast<double>(0.001f);
    return static_cast<double>(first) +
           (static_cast<double>(second) - static_cast<double>(first)) * roll;
}

std::uint32_t AiTargetWeightSelector::Select(
    std::vector<std::uint32_t>& candidates,
    const std::uint32_t skill, const std::uint32_t mode,
    IAiTargetWeightRuntime& runtime) {
    if (candidates.empty()) return 0u;
    if (mode == 2u) {
        std::vector<std::uint32_t> wounded;
        for (const auto actor : candidates) {
            if (runtime.CurrentStat(actor, 7u) <
                runtime.MaxStat(actor, 0u))
                wounded.push_back(actor);
        }
        if (wounded.empty()) return 0u;
        candidates = std::move(wounded);
    }

    const auto roll_extended =
        static_cast<double>(runtime.RandomNumber() % 1000u) *
        static_cast<double>(0.001f);
    const auto roll = static_cast<float>(roll_extended);
    const auto first_threshold = runtime.ReadFloat(skill + 56u);
    if (roll_extended < first_threshold) {
        const auto selector = runtime.ConvertToInt(
            runtime.ReadFloat(skill + 20u));
        std::vector<std::uint32_t> matches;
        for (const auto actor : candidates) {
            if (runtime.ActorSelector(actor) == selector)
                matches.push_back(actor);
        }
        if (matches.empty()) return 0u;
        const auto index = runtime.ConvertToInt(RandomRange(
            0.0f, static_cast<float>(matches.size()), runtime));
        return matches.at(static_cast<std::size_t>(index));
    }

    const auto second_threshold = static_cast<float>(
        static_cast<double>(first_threshold) +
        SkillProperty(skill, 11, runtime));
    if (roll < second_threshold) {
        std::uint32_t lowest = 0u;
        for (const auto actor : candidates) {
            if (lowest == 0u ||
                runtime.CurrentStat(actor, 7u) <
                    runtime.CurrentStat(lowest, 7u))
                lowest = actor;
        }
        return lowest;
    }

    const auto third_threshold =
        static_cast<double>(second_threshold) +
        SkillProperty(skill, 12, runtime);
    if (!(third_threshold > roll)) return 0u;
    const auto index = runtime.ConvertToInt(RandomRange(
        0.0f, static_cast<float>(candidates.size()), runtime));
    return candidates.at(static_cast<std::size_t>(index));
}

}  // namespace pal4
