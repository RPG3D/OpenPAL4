#include "pal4/script_bgm_callbacks.h"

namespace pal4 {

std::uint8_t ScriptBgmCallbacks::SetMusic(
    std::uint32_t source, IScriptBgmRuntime& runtime) {
    return runtime.SetMusicBuffer(runtime.MusicPlayer(), source);
}

bool ScriptBgmCallbacks::IsInArea(
    std::uint32_t name_payload, IScriptBgmAreaRuntime& runtime) {
    const auto area = runtime.ReadWord(runtime.ArenaManager() + 0xe0u);
    auto name = runtime.ReadWord(name_payload + 4u);
    if (name == 0u) name = runtime.EmptyStringAddress();
    return runtime.CompareIgnoreCase(name, area) == 0;
}

std::uint8_t ScriptBgmCallbacks::Mute(
    std::uint8_t muted, IScriptBgmRuntime& runtime) {
    return runtime.SetProcessingState(runtime.MusicPlayer(), muted);
}

std::uint32_t ScriptBgmCallbacks::Play(
    std::uint32_t source, std::int32_t mode, float first, float second,
    IScriptBgmRuntime& runtime) {
    return runtime.PlayTrack(runtime.MusicPlayer(), source, mode,
                             first, second);
}

std::uint32_t ScriptBgmCallbacks::Stop(
    std::int32_t mode, float fade, IScriptBgmRuntime& runtime) {
    return runtime.StopTrack(runtime.MusicPlayer(), mode, fade);
}

std::uint32_t ScriptBgmCallbacks::StopArenaMusic(
    float duration, IScriptBgmRuntime& runtime) {
    return runtime.FadeOut(runtime.MusicPlayer(), duration);
}

}  // namespace pal4
