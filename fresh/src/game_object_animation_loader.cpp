#include "pal4/game_object_animation_loader.h"

#include <string>

namespace pal4 {

bool GameObjectAnimationLoader::LoadData(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t clump, const std::string_view base_path,
    const std::string_view table_suffix,
    IGameObjectAnimationLoadRuntime& runtime) {
    if (clump == 0) return false;
    const auto table_path = std::string(base_path) +
                            std::string(table_suffix) + ".tab";
    if (!runtime.LoadScript(state, table_path) ||
        runtime.AnimationCount(state) <= 0)
        return false;

    auto& words = state.words;
    if (words[2] != 0) {
        const auto old_array = words[2];
        runtime.DestructAnimationArray(
            old_array, kRecordStride,
            runtime.AnimationArrayCount(old_array - 4u));
        (void)runtime.FreeMemory(old_array - 4u);
    }

    const auto count = runtime.AnimationCount(state);
    const auto allocation = runtime.AllocateRecords(
        static_cast<std::uint32_t>(count) * kRecordStride + 4u);
    if (allocation == 0) {
        words[2] = 0;
        return false;
    }
    const auto records = allocation + 4u;
    runtime.ConstructRecords(records, static_cast<std::uint32_t>(count),
                             kRecordStride);
    words[2] = records;

    for (std::int32_t index = 0; index < runtime.AnimationCount(state);
         ++index) {
        const auto record = records +
                            static_cast<std::uint32_t>(index) * kRecordStride;
        const auto suffix = runtime.AnimationResourceSuffix(state, index);
        const auto path = std::string(base_path) + std::string(suffix);
        const auto name_length = suffix.size() >= 4 ? suffix.size() - 4 : 0;
        runtime.SetRecordName(record, suffix.substr(0, name_length));
        runtime.SetRecordBasePath(record, base_path);
        const auto animation = runtime.ReadAnimation(path);
        runtime.SetRecordAnimation(record, animation);
        if (animation == 0) return false;
        runtime.LoadAnimationEvents(state, record);
    }

    words[10] = 0;
    words[1] = clump;
    words[17] = 1;
    const auto hierarchy = runtime.FindHierarchy(clump);
    if (hierarchy == 0) return false;
    if (!runtime.HasHierarchy(clump)) {
        runtime.SetHierarchyFlags(hierarchy, 0x3000u);
        runtime.AttachHierarchy(hierarchy);
        runtime.AttachSkinAtomics(clump, hierarchy);
        runtime.InitializeHierarchyAnimationData(state, hierarchy);
    }

    words[3] = records;
    const auto interpolator = runtime.HierarchyInterpolator(hierarchy);
    runtime.SetCurrentAnimation(
        interpolator, runtime.RecordAnimation(records));
    runtime.SetCurrentTime(interpolator, 0.0f);
    runtime.UpdateMatrices(hierarchy);
    if (words[4] != 0) runtime.FreeWrappedMemory(words[4]);
    if (words[5] != 0) runtime.FreeWrappedMemory(words[5]);
    words[4] = runtime.CloneBlendSnapshot(
        runtime.HierarchyFrameCount(hierarchy),
        runtime.InterpolatorFrameData(interpolator));
    words[5] = runtime.CloneBlendSnapshot(
        runtime.HierarchyFrameCount(hierarchy),
        runtime.InterpolatorFrameData(interpolator));
    return true;
}

}  // namespace pal4
