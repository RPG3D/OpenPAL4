#pragma once

#include "pal4/game_arena_scene_record.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

enum class GameArenaRuntimeOperation : std::uint8_t {
    ProcessScriptStage,
    ResetPlayerData,
    InitializeNpc,
    LoadGameObjects,
    InitializeMonster,
    LoadPathway,
    CleanupScripts,
    DispatchUiEvent,
    UpdateArenaCache,
    ForEachPlayer,
    UpdatePlayerData,
    ForEachNpc,
    ForEachAnimation,
    ForEachMonster,
    ForEachEffect,
    SetActorWord28,
    PlayerZeroHasData,
    UpdatePlayerStates,
    SetCameraFarClip,
    ExecuteSceneScript,
    SetBackgroundMusic,
};

class IGameArenaSceneRuntime {
public:
    virtual ~IGameArenaSceneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadText(std::uint32_t address) = 0;
    virtual void SetSceneName(std::uint32_t address,
                              std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t NavClump() = 0;
    virtual void BuildCollisionTree(std::uint32_t world,
                                    std::uint32_t clump) = 0;
    virtual void ShowMissingNavmesh(std::string_view message,
                                    std::string_view title) = 0;
    [[nodiscard]] virtual std::string_view EffectRoot() = 0;
    virtual void LoadEffectRecords(std::string_view root,
                                   std::string_view world,
                                   std::string_view scene) = 0;
    // Borrowed records must remain valid until Load returns.
    [[nodiscard]] virtual std::span<const GameArenaSceneRecord>
        SceneRecords() = 0;
    virtual void UpdateProgress(float value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaRuntimeOperation operation,
        std::uint32_t first = 0u, std::uint32_t second = 0u,
        std::string_view first_text = {},
        std::string_view second_text = {}) = 0;
};

class GameArenaSceneRuntimeStub final : public IGameArenaSceneRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string_view ReadText(std::uint32_t) override { return {}; }
    void SetSceneName(std::uint32_t, std::string_view) override {}
    [[nodiscard]] std::uint32_t NavClump() override { return 0u; }
    void BuildCollisionTree(std::uint32_t, std::uint32_t) override {}
    void ShowMissingNavmesh(std::string_view, std::string_view) override {}
    [[nodiscard]] std::string_view EffectRoot() override { return {}; }
    void LoadEffectRecords(std::string_view, std::string_view,
                           std::string_view) override {}
    [[nodiscard]] std::span<const GameArenaSceneRecord> SceneRecords() override {
        return {};
    }
    void UpdateProgress(float) override {}
    [[nodiscard]] std::uint32_t Call(
        GameArenaRuntimeOperation, std::uint32_t, std::uint32_t,
        std::string_view, std::string_view) override { return 0u; }
};

class GameArenaSceneRuntimeLoader final {
public:
    static constexpr std::uint32_t kUiEvent = 0x8a80c4u;
    static constexpr std::uint32_t kPlayerCallback = 0x51b760u;
    static constexpr std::uint32_t kNpcCallback = 0x51bb00u;
    static constexpr std::uint32_t kAnimationCallback = 0x51b990u;
    static constexpr std::uint32_t kMonsterCallback = 0x51bc60u;
    static constexpr std::uint32_t kEffectCallback = 0x51b870u;

    // IDA 0x519310.
    [[nodiscard]] static bool Load(std::uint32_t world,
                                   std::string_view scene_name,
                                   IGameArenaSceneRuntime& runtime);
};

}  // namespace pal4
