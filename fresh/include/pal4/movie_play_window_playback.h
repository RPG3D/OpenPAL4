#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/movie_play_window_lifecycle.h"

namespace pal4 {

struct MoviePlaybackEventWords {
    std::uint32_t first{};
    std::uint32_t second{};
};

class IMoviePlayWindowPlaybackRuntime {
public:
    virtual ~IMoviePlayWindowPlaybackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t BinkPlayer() = 0;
    [[nodiscard]] virtual bool PlayerSoundInitialized(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual bool AudioInitialized(
        std::uint32_t manager) = 0;
    virtual void InitializeAudio(std::uint32_t manager,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t AudioBackend(
        std::uint32_t manager) = 0;
    virtual void SetSoundSystem(std::uint32_t player,
                                std::uint32_t mode,
                                std::uint32_t backend) = 0;
    virtual void SetPlayerCallback(std::uint32_t player,
                                   std::uint32_t byte_offset,
                                   std::uint32_t handler) = 0;
    virtual void CloseVideo(std::uint32_t player) = 0;
    virtual void OpenVideo(std::uint32_t player,
                           std::uint32_t path_address,
                           std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t TogglePause(
        std::uint32_t player) = 0;
    virtual void HandleWindowEvent(std::string_view name,
                                   MoviePlaybackEventWords& words) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint8_t ProcessCommand(
        std::uint32_t tree, std::string_view command,
        MoviePlaybackEventWords& words) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view CString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual MoviePlayWindowState& FrameState(
        std::uint32_t frame) = 0;
    virtual void InitializeVideoPath(MoviePlayWindowState& state,
                                     std::string_view prefix) = 0;
    virtual void AppendVideoPath(MoviePlayWindowState& state,
                                 std::string_view suffix) = 0;
};

class MoviePlayWindowPlaybackRuntimeStub :
    public IMoviePlayWindowPlaybackRuntime {
public:
    [[nodiscard]] std::uint32_t BinkPlayer() override { return 0u; }
    [[nodiscard]] bool PlayerSoundInitialized(
        std::uint32_t) override { return true; }
    [[nodiscard]] std::uint32_t AudioManager() override { return 0u; }
    [[nodiscard]] bool AudioInitialized(
        std::uint32_t) override { return true; }
    void InitializeAudio(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AudioBackend(
        std::uint32_t) override { return 0u; }
    void SetSoundSystem(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    void SetPlayerCallback(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    void CloseVideo(std::uint32_t) override {}
    void OpenVideo(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t TogglePause(
        std::uint32_t) override { return 0u; }
    void HandleWindowEvent(std::string_view,
                           MoviePlaybackEventWords&) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint8_t ProcessCommand(
        std::uint32_t, std::string_view,
        MoviePlaybackEventWords&) override { return 0u; }
    [[nodiscard]] std::uint32_t FindFrame(
        std::uint32_t, std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::string_view CString(
        std::uint32_t) override { return {}; }
    [[nodiscard]] MoviePlayWindowState& FrameState(
        std::uint32_t) override { return frame_state_; }
    void InitializeVideoPath(MoviePlayWindowState&,
                             std::string_view) override {}
    void AppendVideoPath(MoviePlayWindowState&,
                         std::string_view) override {}

private:
    MoviePlayWindowState frame_state_{};
};

class MoviePlayWindowPlayback final {
public:
    static constexpr std::uint32_t kPlayerCallbackOffset = 0x200394u;
    static constexpr std::uint32_t kProcessMovieEvent = 0x449850u;

    [[nodiscard]] static std::uint32_t Play(
        const MoviePlayWindowState& state,
        IMoviePlayWindowPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint32_t TogglePause(
        IMoviePlayWindowPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t ProcessMovieEvent(
        IMoviePlayWindowPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandlePlaybackCommand(
        std::uint32_t event, IMoviePlayWindowPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopButton(
        IMoviePlayWindowPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t PauseButton(
        IMoviePlayWindowPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t PlayButton(
        const MoviePlayWindowState& state,
        IMoviePlayWindowPlaybackRuntime& runtime);
};

}  // namespace pal4
