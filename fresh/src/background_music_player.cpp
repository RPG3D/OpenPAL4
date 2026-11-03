#include "pal4/background_music_player.h"

#include <algorithm>
#include <bit>
#include <cctype>
#include <filesystem>
#include <utility>

namespace pal4 {

void BackgroundMusicPlayer::RunInitializer(
    const std::string_view scene, const std::string_view suffix,
    IBackgroundMusicRuntime& runtime) {
    std::string script_name(scene);
    std::transform(script_name.begin(), script_name.end(), script_name.begin(),
                   [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
    script_name.append(suffix);
    saved_ = runtime.DefaultTrack();
    runtime.InitializeScript();
    const auto interpreter = runtime.CreateInterpreter(
        "BGMIniter", script_name, "gamedata\\editData\\script\\music.csb");
    if (interpreter != 0u && runtime.ExecuteInterpreter(interpreter, *this)) {
        runtime.HandleException(interpreter);
        runtime.ReleaseException(interpreter);
    }
    runtime.CleanupScripts();
}

std::uint32_t BackgroundMusicPlayer::SetBackgroundMusic(
    const std::string_view scene, const std::string_view unused_path,
    IBackgroundMusicRuntime& runtime) {
    static_cast<void>(unused_path);
    RunInitializer(scene, "_MUSIC", runtime);
    const auto previous_state = state_;
    if (previous_state != 2u || mode_ != 0u) {
        state_ = 0u;
        if (previous_state == 2u)
            static_cast<void>(StopCurrentWithFade(std::bit_cast<float>(fade_bits_), runtime));
        if (saved_.empty()) {
            static_cast<void>(StopCurrentWithFade(1.0f, runtime));
        } else if (current_ != saved_) {
            static_cast<void>(StopCurrentWithFade(1.0f, runtime));
            static_cast<void>(LoadTrack(saved_, 3.0f, false, runtime));
        }
    }
    return runtime.CleanupTemporaryText();
}

std::uint32_t BackgroundMusicPlayer::RestoreNormalBackgroundMusic(
    const std::string_view scene, IBackgroundMusicRuntime& runtime) {
    state_ = 1u;
    RunInitializer(scene, "_COMBAT_MUSIC", runtime);
    static_cast<void>(StopCurrentWithFade(1.0f, runtime));
    if (!saved_.empty())
        static_cast<void>(LoadTrack(saved_, 0.0f, false, runtime));
    return runtime.CleanupTemporaryText();
}

std::uint32_t BackgroundMusicPlayer::PlayScriptTrack(
    const std::string& track, const std::uint32_t mode,
    const float fade_seconds, const std::uint32_t restore_fade_bits,
    IBackgroundMusicRuntime& runtime) {
    fade_bits_ = restore_fade_bits;
    // Materialize the source before assignment because the source may be pending_.
    pending_ = std::string(track);
    if (pending_.empty()) return 0u;
    state_ = 2u;
    mode_ = mode;
    static_cast<void>(StopCurrentWithFade(1.0f, runtime));
    return LoadTrack(pending_, fade_seconds, mode == 1u, runtime);
}

std::uint32_t BackgroundMusicPlayer::ApplyScriptStopMode(
    const std::uint32_t mode, const float fade_seconds,
    IBackgroundMusicRuntime& runtime) {
    if (mode == 0u) {
        if (state_ == 2u)
            static_cast<void>(StopCurrentWithFade(fade_seconds, runtime));
        if (saved_.empty()) return 0u;
        state_ = 0u;
        return LoadTrack(saved_, 3.0f, false, runtime);
    }
    if (mode == 1u || mode == 2u) {
        mode_ = mode;
        fade_bits_ = std::bit_cast<std::uint32_t>(fade_seconds);
        if (mode == 1u)
            return runtime.SetAudioProperty(audio_handle_, fade_bits_);
    }
    return mode;
}

std::uint32_t BackgroundMusicPlayer::FadeOutUnlessScript(
    const float seconds, IBackgroundMusicRuntime& runtime) {
    return state_ == 2u ? 0u : StopCurrentWithFade(seconds, runtime);
}

std::uint32_t BackgroundMusicPlayer::LoadTrack(
    const std::string_view track, const float fade_seconds,
    const bool restore, IBackgroundMusicRuntime& runtime) {
    if (current_ == track) return 0u;
    current_ = track;
    std::string path(runtime.MusicDirectory());
    path.append(track);
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension != ".mp3") path += ".mp3";
    audio_handle_ = runtime.PlaySound(path, restore, state_ == 2u);
    if (fade_seconds > 0.1f)
        static_cast<void>(runtime.FadeInAudio(audio_handle_, fade_seconds));
    return runtime.CleanupTemporaryPath();
}

std::uint32_t BackgroundMusicPlayer::StopCurrentWithFade(
    const float seconds, IBackgroundMusicRuntime& runtime) {
    current_ = runtime.DefaultTrack();
    if (seconds > 0.1f)
        return runtime.FadeOutAudio(audio_handle_, seconds);
    return runtime.StopAudioImmediately(audio_handle_);
}

std::uint8_t BackgroundMusicPlayer::LoadAndPlayTrack(
    IBackgroundMusicRuntime& runtime) {
    if (runtime.IsAudioActive(audio_handle_))
        return runtime.PauseAudio(audio_handle_);
    if (current_.empty()) return 0u;
    std::string path(runtime.MusicDirectory());
    path += current_;
    audio_handle_ = runtime.PlaySound(path, false, false);
    return static_cast<std::uint8_t>(runtime.CleanupTemporaryPath());
}

std::uint32_t BackgroundMusicPlayer::FadeInSpecialBackgroundMusic(
    const std::string_view track, const float fade_seconds,
    const bool restore, IBackgroundMusicRuntime& runtime) {
    const auto previous_state = state_;
    state_ = 3u;
    static_cast<void>(StopCurrentWithFade(
        previous_state == 2u ? std::bit_cast<float>(fade_bits_) : 1.0f,
        runtime));
    return LoadTrack(track, fade_seconds, restore, runtime);
}

std::uint8_t BackgroundMusicPlayer::AssignSavedTrack(
    const std::string& track, IBackgroundMusicRuntime& runtime) {
    const bool alias = &track == &saved_;
    const std::string source(track);
    saved_ = source;
    return runtime.SavedTextAssignmentResult(source, alias);
}

std::uintptr_t BackgroundMusicPlayer::OnScriptTrackFinished(
    IBackgroundMusicRuntime& runtime) {
    if (state_ != 2u || mode_ != 1u)
        return reinterpret_cast<std::uintptr_t>(this);
    state_ = 0u;
    static_cast<void>(StopCurrentWithFade(std::bit_cast<float>(fade_bits_), runtime));
    return LoadTrack(saved_, 3.0f, false, runtime);
}

}  // namespace pal4
