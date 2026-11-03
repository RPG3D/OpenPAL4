#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace pal4 {

class BackgroundMusicPlayer;

class IBackgroundMusicRuntime {
public:
    virtual ~IBackgroundMusicRuntime() = default;
    [[nodiscard]] virtual std::string_view DefaultTrack() = 0;
    virtual void InitializeScript() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        std::string_view entry, std::string_view script_name,
        std::string_view script_path) = 0;
    [[nodiscard]] virtual bool ExecuteInterpreter(
        std::uint32_t interpreter, BackgroundMusicPlayer& player) = 0;
    virtual void HandleException(std::uint32_t interpreter) = 0;
    virtual void ReleaseException(std::uint32_t interpreter) = 0;
    virtual void CleanupScripts() = 0;
    [[nodiscard]] virtual std::string_view MusicDirectory() = 0;
    [[nodiscard]] virtual std::uint32_t PlaySound(
        std::string_view full_path, bool restore, bool on_script_finish) = 0;
    [[nodiscard]] virtual std::uint32_t FadeInAudio(
        std::uint32_t handle, float seconds) = 0;
    [[nodiscard]] virtual std::uint32_t FadeOutAudio(
        std::uint32_t handle, float seconds) = 0;
    [[nodiscard]] virtual std::uint32_t StopAudioImmediately(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool IsAudioActive(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t PauseAudio(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t SetAudioProperty(
        std::uint32_t handle, std::uint32_t value_bits) = 0;
    [[nodiscard]] virtual std::uint8_t SavedTextAssignmentResult(
        std::string_view text, bool alias) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryText() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryPath() = 0;
};

class BackgroundMusicPlayer final {
public:
    static constexpr std::uint32_t kCurrentOffset = 0u;
    static constexpr std::uint32_t kSavedOffset = 16u;
    static constexpr std::uint32_t kPendingOffset = 32u;
    static constexpr std::uint32_t kAudioHandleOffset = 48u;
    static constexpr std::uint32_t kStateOffset = 52u;
    static constexpr std::uint32_t kModeOffset = 56u;
    static constexpr std::uint32_t kFadeOffset = 60u;

    [[nodiscard]] const std::string& CurrentTrack() const { return current_; }
    [[nodiscard]] const std::string& SavedTrack() const { return saved_; }
    [[nodiscard]] const std::string& PendingTrack() const { return pending_; }
    [[nodiscard]] std::uint32_t State() const { return state_; }
    [[nodiscard]] std::uint32_t Mode() const { return mode_; }
    [[nodiscard]] std::uint32_t FadeBits() const { return fade_bits_; }
    [[nodiscard]] std::uint32_t AudioHandle() const { return audio_handle_; }

    void SetCurrentTrack(std::string track) { current_ = std::move(track); }
    void SetSavedTrack(std::string track) { saved_ = std::move(track); }
    void SetPendingTrack(std::string track) { pending_ = std::move(track); }
    void SetState(std::uint32_t state) { state_ = state; }
    void SetMode(std::uint32_t mode) { mode_ = mode; }
    void SetFadeBits(std::uint32_t bits) { fade_bits_ = bits; }
    void SetAudioHandle(std::uint32_t handle) { audio_handle_ = handle; }

    // IDA 0x513ee0; the second stack argument is unused by the binary.
    [[nodiscard]] std::uint32_t SetBackgroundMusic(
        std::string_view scene, std::string_view unused_path,
        IBackgroundMusicRuntime& runtime);
    // IDA 0x5140a0.
    [[nodiscard]] std::uint32_t RestoreNormalBackgroundMusic(
        std::string_view scene, IBackgroundMusicRuntime& runtime);
    // IDA 0x514200.
    [[nodiscard]] std::uint32_t PlayScriptTrack(
        const std::string& track, std::uint32_t mode, float fade_seconds,
        std::uint32_t restore_fade_bits, IBackgroundMusicRuntime& runtime);
    // IDA 0x514370.
    [[nodiscard]] std::uint32_t ApplyScriptStopMode(
        std::uint32_t mode, float fade_seconds,
        IBackgroundMusicRuntime& runtime);
    // IDA 0x5143e0. Original return register is unspecified in script state 2.
    [[nodiscard]] std::uint32_t FadeOutUnlessScript(
        float seconds, IBackgroundMusicRuntime& runtime);
    // IDA 0x514400 and 0x514620.
    [[nodiscard]] std::uint32_t LoadTrack(
        std::string_view track, float fade_seconds, bool restore,
        IBackgroundMusicRuntime& runtime);
    [[nodiscard]] std::uint32_t StopCurrentWithFade(
        float seconds, IBackgroundMusicRuntime& runtime);
    // IDA 0x514700, 0x514880, 0x5148c0 and 0x5149f0.
    [[nodiscard]] std::uint8_t LoadAndPlayTrack(IBackgroundMusicRuntime& runtime);
    [[nodiscard]] std::uint32_t FadeInSpecialBackgroundMusic(
        std::string_view track, float fade_seconds, bool restore,
        IBackgroundMusicRuntime& runtime);
    [[nodiscard]] std::uint8_t AssignSavedTrack(
        const std::string& track, IBackgroundMusicRuntime& runtime);
    [[nodiscard]] std::uintptr_t OnScriptTrackFinished(
        IBackgroundMusicRuntime& runtime);

private:
    void RunInitializer(std::string_view scene, std::string_view suffix,
                        IBackgroundMusicRuntime& runtime);
    std::string current_;
    std::string saved_;
    std::string pending_;
    std::uint32_t state_ = 0u;
    std::uint32_t mode_ = 0u;
    std::uint32_t fade_bits_ = 0u;
    std::uint32_t audio_handle_ = 0u;
};

}  // namespace pal4
