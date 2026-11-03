#pragma once

#include "pal4/background_music_player.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeMilesDigitalDriver;
class NativeMilesMusicStream;

// Keeps each Miles sound alive until its own fade or playback completes.
class NativeBackgroundMusicRuntime final : public IBackgroundMusicRuntime {
public:
    NativeBackgroundMusicRuntime(std::filesystem::path game_root,
                                 NativeMilesDigitalDriver& driver,
                                 NativeMilesMusicStream& opening_stream);
    ~NativeBackgroundMusicRuntime() override;

    static constexpr std::uint32_t kOpeningHandle = 1u;
    void Update(float seconds);
    void SetVolume(float value);
    float Volume() const noexcept { return volume_; }
    std::vector<NativeMilesMusicStream*> Streams();
    void BindScriptInitializer(
        std::function<void(std::string_view, BackgroundMusicPlayer&)> callback);

    std::string_view DefaultTrack() override;
    void InitializeScript() override;
    std::uint32_t CreateInterpreter(std::string_view entry,
                                    std::string_view script_name,
                                    std::string_view script_path) override;
    bool ExecuteInterpreter(std::uint32_t interpreter,
                            BackgroundMusicPlayer& player) override;
    void HandleException(std::uint32_t interpreter) override;
    void ReleaseException(std::uint32_t interpreter) override;
    void CleanupScripts() override;
    std::string_view MusicDirectory() override;
    std::uint32_t PlaySound(std::string_view full_path, bool restore,
                            bool on_script_finish) override;
    std::uint32_t FadeInAudio(std::uint32_t handle, float seconds) override;
    std::uint32_t FadeOutAudio(std::uint32_t handle, float seconds) override;
    std::uint32_t StopAudioImmediately(std::uint32_t handle) override;
    bool IsAudioActive(std::uint32_t handle) override;
    std::uint8_t PauseAudio(std::uint32_t handle) override;
    std::uint32_t SetAudioProperty(std::uint32_t handle,
                                   std::uint32_t value_bits) override;
    std::uint8_t SavedTextAssignmentResult(std::string_view text,
                                           bool alias) override;
    std::uint32_t CleanupTemporaryText() override;
    std::uint32_t CleanupTemporaryPath() override;

private:
    NativeMilesMusicStream* Find(std::uint32_t handle);
    std::filesystem::path game_root_;
    NativeMilesDigitalDriver& driver_;
    NativeMilesMusicStream& opening_stream_;
    std::map<std::uint32_t, std::unique_ptr<NativeMilesMusicStream>> streams_;
    std::uint32_t next_handle_ = kOpeningHandle + 1u;
    float volume_ = 1.0f;
    std::function<void(std::string_view, BackgroundMusicPlayer&)>
        script_initializer_;
    std::string pending_script_name_;
};

}  // namespace pal4
