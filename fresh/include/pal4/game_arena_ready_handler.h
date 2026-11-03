#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

struct ArenaReadyHintRecord {
    std::string_view arena;
    std::string_view area;
    std::string_view text;
};

class IGameArenaReadyRuntime {
public:
    virtual ~IGameArenaReadyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::span<const ArenaReadyHintRecord> HintRecords(
        std::uint32_t database_member) = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow() = 0;
    virtual void ShowHint(std::uint32_t window, std::string_view markup,
                          float x, float y, float scale,
                          std::uint32_t first_flag,
                          std::uint32_t second_flag) = 0;
    [[nodiscard]] virtual std::int32_t ReleaseHintMatches() = 0;
    virtual void ProcessActorInput(std::uint32_t actor) = 0;
    virtual void ProcessActorInputAndClearVector(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    virtual void InitializeInputState(std::uint32_t input) = 0;
    [[nodiscard]] virtual std::uint32_t PetManager() = 0;
    virtual void InitializePetData(std::uint32_t pet) = 0;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    virtual void UpdateGrid(std::uint32_t minimap,
                            std::uint32_t position,
                            std::uint32_t direction) = 0;
    [[nodiscard]] virtual float LeaderY() = 0;
    [[nodiscard]] virtual float LeaderZ() = 0;
    virtual void SetMinimapTexture(std::uint32_t level) = 0;
    [[nodiscard]] virtual std::uint32_t Milliseconds() = 0;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
};

class GameArenaReadyHandler final {
public:
    // IDA 0x5d3320, giArenaReady callback.
    [[nodiscard]] static std::int32_t Run(IGameArenaReadyRuntime& runtime);
    // IDA 0x5d38c0, giArenaReadyRestore callback.
    [[nodiscard]] static std::int32_t Restore(IGameArenaReadyRuntime& runtime);
};

}  // namespace pal4
