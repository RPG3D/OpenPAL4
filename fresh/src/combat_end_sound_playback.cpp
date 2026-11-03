#include "pal4/combat_end_sound_playback.h"

namespace pal4 {

std::uint32_t CombatEndSoundPlayback::OnBattleEnd(
    const std::uint32_t event, ICombatEndSoundRuntime& runtime) {
    static_cast<void>(event);
    const auto system = runtime.CombatSystemInstance();
    if (!runtime.IsCombatSystemInactive(system)) {
        const auto active_system = runtime.CombatSystemInstance();
        if (runtime.ReadWord(active_system + 460u) != 0u) {
            runtime.FadeInSpecialMusic(runtime.BackgroundMusicPlayer(),
                                       active_system + 452u, 0.0f, 0u);
        } else {
            const auto music = runtime.ReadWord(
                runtime.CombatSystemInstance() + 128u);
            runtime.RestoreNormalMusic(runtime.BackgroundMusicPlayer(),
                                       music);
        }
    }
    const auto text = runtime.CreateText("WA14.wav");
    try {
        static_cast<void>(PlayEffect(
            runtime.TemporaryOutputSlot(), text, 1u, runtime));
    } catch (...) {
        static_cast<void>(runtime.ReleaseText(text));
        throw;
    }
    return runtime.ReleaseText(text);
}

std::uint8_t CombatEndSoundPlayback::OnMusicCue(
    const std::uint32_t event, ICombatEndSoundRuntime& runtime) {
    const auto cue = runtime.ReadWord(event);
    if (cue > 2u)
        return static_cast<std::uint8_t>(cue - 2u);
    const auto text = runtime.CreateText(
        cue == 0u ? "wa16.mp3" : "wa15.mp3");
    try {
        runtime.FadeInSpecialMusic(runtime.BackgroundMusicPlayer(),
                                   text, 0.0f, 1u);
    } catch (...) {
        static_cast<void>(runtime.ReleaseText(text));
        throw;
    }
    return static_cast<std::uint8_t>(runtime.ReleaseText(text));
}

std::uint32_t CombatEndSoundPlayback::PlayEffect(
    const std::uint32_t output, const std::uint32_t text_record,
    const std::uint32_t mode, ICombatEndSoundRuntime& runtime) {
    if (runtime.ReadWord(text_record + 8u) == 0u ||
        runtime.IsCombatSystemInactive(runtime.CombatSystemInstance())) {
        runtime.WriteWord(output, 0u);
        return output;
    }
    const auto path = runtime.CreateText("gamedata\\palSound\\");
    try {
        runtime.AppendText(path, text_record);
        runtime.PlayAudio(runtime.AudioManagerInstance(), output, path, mode);
    } catch (...) {
        static_cast<void>(runtime.ReleaseText(path));
        throw;
    }
    static_cast<void>(runtime.ReleaseText(path));
    return output;
}

}  // namespace pal4
