#pragma once

#include "pal4/game_arena_ready_handler.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class GameRenderState;
class NativeActorControlSession;
class NativeGameArenaManagerOwner;
class NativeMinimapManagerOwner;
class NativePlayerManagerSession;
class NativePetManagerOwner;
class INativeArenaLoadHost;

class INativeArenaReadyHints {
public:
    virtual ~INativeArenaReadyHints() = default;
    [[nodiscard]] virtual std::uint32_t DatabaseAddress() const = 0;
    [[nodiscard]] virtual std::span<const ArenaReadyHintRecord> Records() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHint() = 0;
    virtual void Show(std::uint32_t window, std::string_view markup,
                      float x, float y, float scale,
                      std::uint32_t first_flag,
                      std::uint32_t second_flag) = 0;
    [[nodiscard]] virtual std::int32_t ReleaseMatches() = 0;
};

// Binds giArenaReady to live arena/input and persistent pet/minimap owners.
class NativeArenaReadyRuntime final : public IGameArenaReadyRuntime {
public:
    NativeArenaReadyRuntime(NativeGameArenaManagerOwner& arena,
                            NativeActorControlSession& control,
                            NativePlayerManagerSession& players,
                            GameRenderState& game_state,
                            INativeArenaLoadHost& host, NativePetManagerOwner& pet);
    ~NativeArenaReadyRuntime() override;

    void BindMinimap(NativeMinimapManagerOwner& minimap) noexcept;
    void BindHints(INativeArenaReadyHints& hints) noexcept { hints_ = &hints; }
    [[nodiscard]] std::uint32_t PetSource() const noexcept;

    std::uint32_t ActorControl() override;
    std::uint32_t ArenaManager() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::string_view ReadText(std::uint32_t address) override;
    std::uint32_t GameDatabase() override;
    std::span<const ArenaReadyHintRecord> HintRecords(
        std::uint32_t database_member) override;
    std::uint32_t CreateHintWindow() override;
    void ShowHint(std::uint32_t window, std::string_view markup,
                  float x, float y, float scale,
                  std::uint32_t first_flag,
                  std::uint32_t second_flag) override;
    std::int32_t ReleaseHintMatches() override;
    void ProcessActorInput(std::uint32_t actor) override;
    void ProcessActorInputAndClearVector(std::uint32_t actor) override;
    std::uint32_t InputManager() override;
    void InitializeInputState(std::uint32_t input) override;
    std::uint32_t PetManager() override;
    void InitializePetData(std::uint32_t pet) override;
    std::uint32_t MinimapManager() override;
    void UpdateGrid(std::uint32_t minimap, std::uint32_t position,
                    std::uint32_t direction) override;
    float LeaderY() override;
    float LeaderZ() override;
    void SetMinimapTexture(std::uint32_t level) override;
    std::uint32_t Milliseconds() override;
    std::uint32_t GameState() override;

private:
    NativeGameArenaManagerOwner& arena_;
    NativeActorControlSession& control_;
    NativePlayerManagerSession& players_;
    GameRenderState& game_state_;
    INativeArenaLoadHost& host_;
    NativeMinimapManagerOwner* minimap_ = nullptr;
    INativeArenaReadyHints* hints_ = nullptr;
    NativePetManagerOwner& pet_;
};

}  // namespace pal4
