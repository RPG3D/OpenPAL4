#include "pal4/global_data_settings.h"

#include <bit>
#include <stdexcept>

namespace pal4 {
namespace {

void ApplyAudioLevel(const std::uint32_t manager,
                     const std::uint32_t level_index,
                     const GlobalDataAudioCollection collection,
                     const float value,
                     IGlobalDataSettingsRuntime& runtime) {
    runtime.SetAudioLevel(manager, level_index, value);
    for (std::size_t index = 0;
         index < runtime.AudioStreamCount(manager, collection); ++index) {
        const auto stream = runtime.AudioStreamAt(manager, collection, index);
        if (stream == 0u) continue;
        if (collection == GlobalDataAudioCollection::First) {
            runtime.SetStreamVolume(stream, value);
        } else {
            const auto kind = runtime.AudioStreamKind(stream);
            const auto expected = collection == GlobalDataAudioCollection::KindOne
                                      ? 1u : 2u;
            if (kind == expected) runtime.RefreshStream(stream);
        }
    }
}

}  // namespace

std::uint8_t GlobalDataSettings::Save(
    const GlobalDataSettingsState& state, const std::uint32_t archive,
    IGlobalDataSettingsSaveRuntime& runtime) {
    for (const auto& word : state.words)
        runtime.WriteBytes(archive, &word, sizeof(word));
    runtime.WriteBytes(archive, &state.byte_14, 1u);
    const auto first_manager = runtime.AudioManager();
    const auto first_level = runtime.AudioLevel(first_manager, 0u);
    runtime.WriteBytes(archive, &first_level, sizeof(first_level));
    const auto second_manager = runtime.AudioManager();
    const auto second_level = runtime.AudioLevel(second_manager, 1u);
    runtime.WriteBytes(archive, &second_level, sizeof(second_level));
    return 1u;
}

std::uint8_t GlobalDataSettings::Load(
    GlobalDataSettingsState& state, const std::uint32_t archive,
    IGlobalDataSettingsRuntime& runtime) {
    for (auto& word : state.words)
        runtime.ReadBytes(archive, &word, sizeof(word));
    runtime.ReadBytes(archive, &state.byte_14, 1u);

    float first_level = 0.0f;
    runtime.ReadBytes(archive, &first_level, sizeof(first_level));
    const auto first_manager = runtime.AudioManager();
    ApplyAudioLevel(first_manager, 0u, GlobalDataAudioCollection::First,
                    first_level, runtime);

    float second_level = 0.0f;
    runtime.ReadBytes(archive, &second_level, sizeof(second_level));
    const auto second_manager = runtime.AudioManager();
    ApplyAudioLevel(second_manager, 1u, GlobalDataAudioCollection::KindOne,
                    second_level, runtime);
    const auto third_manager = runtime.AudioManager();
    ApplyAudioLevel(third_manager, 2u, GlobalDataAudioCollection::KindTwo,
                    second_level, runtime);
    static_cast<void>(ApplyScaleOption(state, state.words[3], runtime));
    return 1u;
}

std::int32_t GlobalDataSettings::ResetDefaults(
    GlobalDataSettingsState& state,
    IGlobalDataSettingsRuntime& runtime) {
    state = DefaultState();
    const auto first = std::bit_cast<float>(0x3e99999au);
    const auto second = std::bit_cast<float>(0x3f4ccccdu);
    const auto first_manager = runtime.AudioManager();
    ApplyAudioLevel(first_manager, 0u, GlobalDataAudioCollection::First,
                    first, runtime);
    const auto second_manager = runtime.AudioManager();
    ApplyAudioLevel(second_manager, 1u, GlobalDataAudioCollection::KindOne,
                    second, runtime);
    const auto third_manager = runtime.AudioManager();
    ApplyAudioLevel(third_manager, 2u, GlobalDataAudioCollection::KindTwo,
                    second, runtime);
    return ApplyScaleOption(state, state.words[3], runtime);
}

std::int32_t GlobalDataSettings::ApplyScaleOption(
    GlobalDataSettingsState& state, const std::uint32_t option,
    IGlobalDataScaleRuntime& runtime) {
    state.words[3] = option;
    const float value = ScaleDistance(option);
    runtime.SetAtomicPluginFloatData(value);
    if (!runtime.HasEffectSystem())
        throw std::runtime_error("effect system has not been created");
    return runtime.SetEffectSystemSquaredValue(value);
}

}  // namespace pal4
