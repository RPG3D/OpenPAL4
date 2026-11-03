#include "pal4/game_object_animation_switch.h"

#include <bit>

namespace pal4 {

namespace {

std::int32_t FindActionIndex(
    GameObjectEmbeddedAnimationState& state,
    std::string_view name,
    IGameObjectAnimationSwitchRuntime& runtime) {
    for (std::int32_t index = 0; index < runtime.AnimationCount(state);
         ++index) {
        if (runtime.CompareActionNames(runtime.ActionName(state, index),
                                       name) == 0)
            return index;
    }
    return -1;
}

}  // namespace

bool GameObjectAnimationSwitch::AttachHierarchyToClump(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t target_clump,
    IGameObjectAnimationSwitchRuntime& runtime) {
    if (state.words[1] == 0) return false;
    const auto hierarchy = runtime.FindHierarchy(state.words[1]);
    if (hierarchy == 0) return false;
    runtime.SetHierarchyFlags(hierarchy, 0x3000u);
    runtime.AttachHierarchy(hierarchy);
    runtime.AttachSkinAtomics(target_clump, hierarchy);
    return true;
}

bool GameObjectAnimationSwitch::SetCurrentAnimation(
    GameObjectEmbeddedAnimationState& state,
    const std::string_view name,
    IGameObjectAnimationSwitchRuntime& runtime) {
    auto& words = state.words;
    if (words[1] == 0 || words[2] == 0) return false;
    const auto hierarchy = runtime.FindHierarchy(words[1]);
    if (hierarchy == 0) return false;
    const auto index = FindActionIndex(state, name, runtime);
    if (index < 0) return false;

    runtime.CleanupCurrentRecord(words[3]);
    const auto record = words[2] +
                        static_cast<std::uint32_t>(index) * kRecordStride;
    words[3] = record;
    const auto interpolator = runtime.HierarchyInterpolator(hierarchy);
    runtime.SetCurrentAnimation(
        interpolator, runtime.RecordAnimation(record));
    runtime.SetCurrentTime(interpolator, 0.0f);
    runtime.UpdateMatrices(hierarchy);
    words[10] = words[15] = words[18] = words[19] = 0;
    return true;
}

bool GameObjectAnimationSwitch::SetCurrentAnimationBlended(
    GameObjectEmbeddedAnimationState& state,
    const std::string_view name, const float blend_time,
    IGameObjectAnimationSwitchRuntime& runtime) {
    auto& words = state.words;
    if (words[1] == 0 || words[2] == 0 ||
        words[4] == 0 || words[5] == 0)
        return false;
    const auto index = FindActionIndex(state, name, runtime);
    if (index < 0) return false;

    runtime.CleanupCurrentRecord(words[3]);
    runtime.SetCurrentAnimation(
        words[4], runtime.RecordAnimation(words[3]));
    const auto record = words[2] +
                        static_cast<std::uint32_t>(index) * kRecordStride;
    runtime.SetCurrentAnimation(
        words[5], runtime.RecordAnimation(record));
    words[3] = record;
    words[10] = words[18] = words[19] = 0;
    words[15] = 1;
    words[13] = words[14] = std::bit_cast<std::uint32_t>(blend_time);
    return true;
}

}  // namespace pal4
