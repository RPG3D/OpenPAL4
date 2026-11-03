#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

// Current +E0 scene name, borrowed from the existing arena owner.
class IScriptBgmAreaName {
public:
    virtual ~IScriptBgmAreaName() = default;
    [[nodiscard]] virtual std::string_view SceneName() const = 0;
};

class IScriptBgmAreaRuntime {
public:
    virtual ~IScriptBgmAreaRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::int32_t CompareIgnoreCase(
        std::uint32_t lhs, std::uint32_t rhs) = 0;
};

class IScriptBgmRuntime : public IScriptBgmAreaRuntime {
public:
    virtual ~IScriptBgmRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MusicPlayer() = 0;
    [[nodiscard]] virtual std::uint8_t SetMusicBuffer(
        std::uint32_t player, std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint8_t SetProcessingState(
        std::uint32_t player, std::uint8_t muted) = 0;
    [[nodiscard]] virtual std::uint32_t PlayTrack(
        std::uint32_t player, std::uint32_t source, std::int32_t mode,
        float first, float second) = 0;
    [[nodiscard]] virtual std::uint32_t StopTrack(
        std::uint32_t player, std::int32_t mode, float fade) = 0;
    [[nodiscard]] virtual std::uint32_t FadeOut(
        std::uint32_t player, float duration) = 0;
};

class ScriptBgmCallbacks final {
public:
    // IDA 0x5d57a0, giBGMConfigSetMusic.
    [[nodiscard]] static std::uint8_t SetMusic(
        std::uint32_t source, IScriptBgmRuntime& runtime);
    // IDA 0x5d57c0, giBGMConfigIsInArea.
    [[nodiscard]] static bool IsInArea(
        std::uint32_t name_payload, IScriptBgmAreaRuntime& runtime);
    // IDA 0x5d57f0, giScriptMusicMute.
    [[nodiscard]] static std::uint8_t Mute(
        std::uint8_t muted, IScriptBgmRuntime& runtime);
    // IDA 0x5d5810, giScriptMusicPlay.
    [[nodiscard]] static std::uint32_t Play(
        std::uint32_t source, std::int32_t mode, float first, float second,
        IScriptBgmRuntime& runtime);
    // IDA 0x5d5840, giScriptMusicStop.
    [[nodiscard]] static std::uint32_t Stop(
        std::int32_t mode, float fade, IScriptBgmRuntime& runtime);
    // IDA 0x5d5860, giArenaMusicStop.
    [[nodiscard]] static std::uint32_t StopArenaMusic(
        float duration, IScriptBgmRuntime& runtime);
};

}  // namespace pal4
