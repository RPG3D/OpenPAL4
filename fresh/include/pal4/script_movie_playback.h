#pragma once

#include "pal4/script_movie_player.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct MovieWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
    float saved_volume{};
    std::uint8_t return_to_game{};
    std::array<std::byte, 3> reserved{};
};
static_assert(sizeof(MovieWaitState) == 0x28u);
static_assert(offsetof(MovieWaitState, saved_volume) == 0x20u);
static_assert(offsetof(MovieWaitState, return_to_game) == 0x24u);

struct MoviePlaybackSharedState {
    std::uint32_t finished{};
};

class IScriptMovieRuntime {
public:
    virtual ~IScriptMovieRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Player() = 0;
    [[nodiscard]] virtual bool PlayerReady(std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual bool AudioReady(std::uint32_t audio) = 0;
    virtual void InitializeAudio(std::uint32_t audio,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t AudioDevice(
        std::uint32_t audio) = 0;
    virtual void SetSoundSystem(std::uint32_t player,
                                std::uint32_t mode,
                                std::uint32_t device) = 0;
    virtual void CloseVideo(std::uint32_t player) = 0;
    virtual void OpenVideo(std::uint32_t player, std::string_view path,
                           std::uint32_t mode) = 0;
    virtual void InstallCompletionCallback(std::uint32_t player,
                                           std::uint32_t offset,
                                           std::uint32_t address) = 0;
    [[nodiscard]] virtual MovieWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(MovieWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(MovieWaitState& state) = 0;
    virtual void CloseWait(MovieWaitState& state) = 0;
    [[nodiscard]] virtual float ReadVolume(std::uint32_t audio) = 0;
    virtual void WriteVolume(std::uint32_t audio, float value) = 0;
    [[nodiscard]] virtual std::uint32_t ChannelCount(
        std::uint32_t audio) = 0;
    [[nodiscard]] virtual std::uint32_t ChannelAt(
        std::uint32_t audio, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ChannelType(
        std::uint32_t channel) = 0;
    virtual void InvokeChannelSlot44(std::uint32_t channel) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            MovieWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CreateStateDocument() = 0;
    [[nodiscard]] virtual std::uint8_t EnterState(
        std::uint32_t state, std::uint32_t document) = 0;
    virtual void DestroyStateDocument(std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPath() = 0;
    virtual void ResetScreenEffect() = 0;
};

class ScriptMoviePlayback final : public IScriptMoviePlayer {
public:
    static constexpr std::uint32_t kWaitVtable = 0x8495e8u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::uint32_t kCallbackOffset = 0x200394u;
    static constexpr std::uint32_t kCallbackAddress = 0x5e6970u;

    ScriptMoviePlayback(MoviePlaybackSharedState& shared,
                        IScriptMovieRuntime& runtime)
        : shared_(shared), runtime_(runtime) {}

    // IDA 0x5e66a0..0x5e6970.
    [[nodiscard]] std::uint32_t Play(std::string_view movie_name) override;
    // IDA 0x5e8dc0: final movie returns to game and enters state 12 twice.
    [[nodiscard]] std::uint32_t PlayFinal(
        std::string_view movie_name) override;
    [[nodiscard]] std::uint8_t Update(MovieWaitState& state);
    [[nodiscard]] std::uint8_t Stop(MovieWaitState& state);
    void MarkFinished();

private:
    [[nodiscard]] std::uint32_t PlayWithMode(std::string_view movie_name,
                                             bool final_movie);
    void RefreshTypeTwoChannels(std::uint32_t audio);
    [[nodiscard]] std::uint8_t ReturnToGame(const MovieWaitState& state);

    MoviePlaybackSharedState& shared_;
    IScriptMovieRuntime& runtime_;
};

}  // namespace pal4
