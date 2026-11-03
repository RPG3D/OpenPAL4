#include "pal4/gob_sound.h"
#include "pal4/gob_sound_config.h"
#include "pal4/gob_scene_package.h"

namespace pal4 {

std::uint32_t GobSound::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobSoundConfigRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto result = runtime.InitializeBaseModel(
        object, first, path, fourth, fifth);
    if (result == 0u) {
        runtime.CleanupScriptPackages();
        return 0u;
    }
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) {
        runtime.CleanupScriptPackages();
        return 0u;
    }
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters != 0u) {
        const auto old = runtime.FindXmlChild(
            parameters, "PAL4-GameObject-sound");
        if (old != 0u) runtime.RemoveXmlChild(parameters, old);
        static_cast<void>(runtime.CreateXmlChild(
            parameters, "PAL4-GameObject-sound"));
    }
    runtime.CleanupScriptPackages();
    runtime.WriteWord(object + 0x1dcu, 0u);
    return result;
}

std::uint32_t GobSound::SetClumpRenderCallback(
    const std::uint32_t object, IGobSoundConfigRuntime& runtime) {
    const auto clump = runtime.ReadWord(object + 0x14u);
    return runtime.InstallRenderCallback(runtime.MovementAdapter(), clump);
}

std::uint8_t GobSound::StopPlaybackAndSuppress(
    const std::uint32_t object, IGobSoundPlaybackRuntime& runtime) {
    const auto handle = object + kAudioHandleOffset;
    const auto playing = runtime.IsAudioPlaying(handle);
    const auto result = playing ? runtime.StopAudio(handle) :
                                  static_cast<std::uint8_t>(0u);
    runtime.WriteWord(object + 0x1dcu, 1u);
    return result;
}

std::uint8_t GobSound::ResumePlaybackAndUnsuppress(
    const std::uint32_t object, IGobSoundPlaybackRuntime& runtime) {
    const auto handle = object + kAudioHandleOffset;
    const auto playing = runtime.IsAudioPlaying(handle);
    const auto result = playing ? static_cast<std::uint8_t>(1u) :
                                  runtime.PauseAudio(handle);
    runtime.WriteWord(object + 0x1dcu, 0u);
    return result;
}

std::uint32_t GobSound::GetConfigNode(
    const std::uint32_t object, IGobSoundConfigRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters == 0u) return 0u;
    return runtime.FindXmlChild(parameters, "PAL4-GameObject-sound");
}

std::uint32_t GobSound::ReadConfigAndResetAudio(
    const std::uint32_t object, IGobSoundConfigRuntime& runtime) {
    runtime.ApplyBaseParameters(object);
    const auto node = GetConfigNode(object, runtime);
    return ApplyConfigValues(object, node, runtime);
}

std::uint32_t GobSound::ApplyConfigValues(const std::uint32_t object,
    const std::uint32_t node, IGobSoundConfigValueRuntime& runtime) {
    if (node == 0u) return 0u;
    runtime.AssignNameRecord(object + kNameRecordOffset,
        runtime.ReadConfigString(node, "PAL4-GameObject-sound-name", ""));
    runtime.WriteFloat(object + 0x1f4u, runtime.ReadConfigFloat(
        node, "PAL4-GameObject-sound-mintime", 0.0f));
    runtime.WriteFloat(object + 0x1f8u, runtime.ReadConfigFloat(
        node, "PAL4-GameObject-sound-maxtime", 1.0f));
    runtime.WriteWord(object + 0x200u, 0u);
    runtime.WriteWord(object + 0x204u, 0u);
    const auto handle = object + kAudioHandleOffset;
    if (runtime.IsAudioPlaying(handle))
        return runtime.PlayAudioHandle(handle);
    const auto active = runtime.CheckAudioActive(handle);
    if (active != 0u) return runtime.PlayAudioHandle(handle);
    return active;
}

}  // namespace pal4
