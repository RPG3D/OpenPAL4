#include "pal4/change_color_task.h"
#include "pal4/effect_color_math.h"
#include "pal4/effect_task_timing.h"

#include <bit>

namespace pal4 {
ChangeColorTaskState& ChangeColorTask::Construct(
    ChangeColorTaskState& state, IChangeColorTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    state.words[34u] &= 0xffffff00u;
    for (const auto index : {35u, 21u, 20u, 19u, 18u, 25u, 24u, 23u,
                             22u, 29u, 28u, 27u, 26u})
        state.words[index] = 0u;
    return state;
}

ChangeColorTaskState& ChangeColorTask::Destroy(
    ChangeColorTaskState& state, IChangeColorTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view ChangeColorTask::ClassName() {
    return "ChangeColorTask";
}

bool ChangeColorTask::LoadConfig(ChangeColorTaskState& state,
                                 const std::uint32_t config,
                                 IChangeColorConfigRuntime& runtime) {
    if (config == 0u)
        return false;

    const auto alpha = runtime.ReadInt(
        config, "Attr_Task_ChangeColor_Alpha", 0);
    state.words[34u] = (state.words[34u] & 0xffffff00u) |
                       static_cast<std::uint32_t>(alpha > 0);
    const auto red = static_cast<std::uint8_t>(runtime.ReadInt(
        config, "Attr_Task_ChangeColor_End_Red", 0));
    const auto green = static_cast<std::uint8_t>(runtime.ReadInt(
        config, "Attr_Task_ChangeColor_End_Green", 0));
    const auto blue = static_cast<std::uint8_t>(runtime.ReadInt(
        config, "Attr_Task_ChangeColor_End_Blue", 0));
    const auto end_alpha = static_cast<std::uint8_t>(runtime.ReadInt(
        config, "Attr_Task_ChangeColor_End_Alpha", 0));

    const auto inherit = runtime.ReadInt(config, "Attr_Inherit_Pre_Task", 0);
    state.words[15u] = (state.words[15u] & 0xffff00ffu) |
                       (static_cast<std::uint32_t>(inherit > 0) << 8u);
    const auto total = runtime.ReadInt(
        config, "Attr_Task_ChangeColor_Total", 0);
    state.words[34u] = (state.words[34u] & 0xffff00ffu) |
                       (static_cast<std::uint32_t>(total > 0) << 8u);

    const std::array channels{red, green, blue, end_alpha};
    for (std::uint32_t i = 0u; i < channels.size(); ++i) {
        const auto value = NormalizeEffectColorByte(channels[i]);
        state.words[22u + i] = std::bit_cast<std::uint32_t>(value);
    }

    const auto rollback = runtime.ReadInt(
        config, "Attr_Task_ChangeColor_Rollback", 0);
    state.words[36u] = (state.words[36u] & 0xffffff00u) |
                       static_cast<std::uint32_t>(rollback > 0);
    state.words[39u] = std::bit_cast<std::uint32_t>(runtime.ReadFloat(
        config, "Attr_Task_ChangeColor_Time_Bias", 0.0f));
    state.words[41u] = std::bit_cast<std::uint32_t>(runtime.ReadFloat(
        config, "Attr_Time_Interval", 0.0f));
    state.words[44u] = std::bit_cast<std::uint32_t>(runtime.ReadFloat(
        config, "Attr_Time_Interval2", 0.0f));
    return true;
}

bool ChangeColorTask::IsComplete(
    const ChangeColorTaskState& state,
    IChangeColorProgressRuntime& runtime) {
    return (state.words[36u] & 0xffu) == 0u &&
           runtime.DurationComplete(state);
}

std::uint8_t ChangeColorTask::InitializeProgress(
    ChangeColorTaskState& state, IChangeColorProgressRuntime& runtime) {
    const auto facet = runtime.ResolveColorFacet(
        state.words[16u], ClassName());
    state.words[35u] = facet;
    runtime.SetColorFacetEnabled(
        facet, static_cast<std::uint8_t>(state.words[34u] >> 8u));

    std::array<std::uint8_t, 4u> channels{};
    runtime.ReadColor(facet, channels);
    for (std::uint32_t i = 0u; i < channels.size(); ++i) {
        const auto normalized = NormalizeEffectColorByte(channels[i]);
        state.words[18u + i] = std::bit_cast<std::uint32_t>(normalized);
        if (i < 3u && (state.words[34u] & 0xffu) != 0u)
            state.words[22u + i] = state.words[18u + i];
    }

    float duration = std::bit_cast<float>(state.words[5u]);
    state.words[38u] = state.words[5u];
    if ((state.words[36u] & 0xffu) != 0u) {
        const auto bias = std::bit_cast<float>(state.words[39u]);
        duration = RandomizedEffectDuration(runtime.RandomInt(),duration,bias);
        state.words[38u] = std::bit_cast<std::uint32_t>(duration);
    }

    state.words[37u] = 0u;
    state.words[36u] &= 0xffff00ffu;
    state.words[40u] &= 0xffffff00u;
    state.words[42u] = 0u;
    state.words[43u] &= 0xffffff00u;
    state.words[45u] = 0u;
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        state.words[30u + i] = state.words[18u + i];
        const auto start = std::bit_cast<float>(state.words[18u + i]);
        const auto end = std::bit_cast<float>(state.words[22u + i]);
        state.words[26u + i] = std::bit_cast<std::uint32_t>(
            static_cast<float>((static_cast<double>(end) - start) /
                               duration));
    }
    return 1u;
}

}  // namespace pal4
