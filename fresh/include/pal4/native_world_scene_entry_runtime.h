#pragma once

#include "pal4/world_state.h"

#include <cstdint>

namespace pal4 {

class NativeActorControlSession;
class NativeArenaCollisionTree;
class NativeBaseObjectMemory;
class NativeActorMessageOwner;
class NativeCameraManagerOwner;
class NativePalGameIVTextureStorage;
class NativeNpcManagerOwner;
class NativeMonsterManagerOwner;
class NativeSceneGobAssets;
class NativeTemperatureManagerOwner;
class GameRenderState;
class InputManager;
class IUiFrameEventRuntime;
struct NativeCombatSystemImage;
class NativeGameArenaManagerOwner;
class NativeArenaNpcSceneEntry;
class BackgroundMusicPlayer;
class IBackgroundMusicRuntime;
class NativeEffectSceneSession;

// Binds the PALIV_SCENE entry sequence to live native game owners.
class NativeWorldSceneEntryRuntime final : public IWorldStateLifecycleRuntime {
public:
    NativeWorldSceneEntryRuntime(NativePalGameIVTextureStorage& game,
                                 NativeCameraManagerOwner& cameras,
                                 NativeActorControlSession& actor,
                                 NativeActorMessageOwner& messages,
                                 pal4::InputManager& input,
                                 GameRenderState& game_state,
                                 NativeNpcManagerOwner& npcs,
                                 NativeMonsterManagerOwner& monsters,
                                 NativeSceneGobAssets& gobs,
                                 NativeTemperatureManagerOwner& temperature,
                                 std::uint32_t ui_manager,
                                 IUiFrameEventRuntime* ui_frames = nullptr);

    void BindMonsterPaths(NativeArenaCollisionTree& tree,
                          NativeBaseObjectMemory& actor_memory) noexcept;

    // Borrow the actual combat result and loaded scene services. This binds
    // victory/escape recovery and BGM restoration. Escape's effect branch
    // additionally requires its creating NativeEffectSceneSession.
    void BindCombatReturn(NativeCombatSystemImage&, NativeGameArenaManagerOwner&,
        NativeArenaNpcSceneEntry&, pal4::BackgroundMusicPlayer&, IBackgroundMusicRuntime&,
        NativeEffectSceneSession* effects = nullptr);

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t UIFrameManager() override;
    void SetUIFlag(std::uint32_t manager, std::uint32_t flag) override;
    void SendUIEvent(std::uint32_t manager, std::uint32_t event,
                     std::uint32_t first, std::uint32_t second) override;
    [[nodiscard]] std::uint32_t CombatSystem() override;
    [[nodiscard]] std::uint32_t MonsterManager() override;
    void SetMonsterFlags(std::uint32_t manager) override;
    void ClearMonsterFlags(std::uint32_t manager) override;
    [[nodiscard]] std::uint8_t CombatProperty519(std::uint32_t system) override;
    [[nodiscard]] std::uint32_t ActorControl() override;
    void EnableInvincibility(std::uint32_t control, float seconds) override;
    [[nodiscard]] std::uint32_t GameInstance() override;
    void ForEachStateMonster(std::uint32_t game) override;
    [[nodiscard]] std::uint32_t ArenaManager() override;
    [[nodiscard]] std::uint32_t BackgroundMusicPlayer() override;
    void SetBackgroundMusic(std::uint32_t player,
                            std::uint32_t arena_player,
                            std::uint32_t scene_name) override;
    void ClearStateCollection(std::uint32_t game) override;
    [[nodiscard]] std::uint32_t CameraManager() override;
    void ActivateCamera(std::uint32_t manager, std::uint32_t name) override;
    [[nodiscard]] std::uint32_t DefaultCameraName() override;
    [[nodiscard]] std::uint32_t ActorMessageManager() override;
    void ClearActorMessages(std::uint32_t manager) override;
    void UpdateActorComponents(std::uint32_t control) override;
    void ProcessActorInput(std::uint32_t control) override;
    void ClearActorInputVector(std::uint32_t control) override;
    [[nodiscard]] std::uint32_t InputManager() override;
    void InitializeInput(std::uint32_t manager) override;
    [[nodiscard]] std::uint32_t Milliseconds() override;
    [[nodiscard]] std::uint32_t GameState() override;
    [[nodiscard]] std::uint32_t NpcManager() override;
    void SetNpcGridUpdating(std::uint32_t grid, bool enabled) override;
    void ProcessMonsterCollisions(std::uint32_t manager,
                                   float delta) override;
    [[nodiscard]] std::uint32_t GameObjectManager() override;
    void SetGameObjectsActive(std::uint32_t manager, bool active) override;
    [[nodiscard]] std::uint32_t TemperatureManager() override;
    [[nodiscard]] std::uint32_t FindMonster(std::uint32_t manager,
                                             const char* name) override;
    [[nodiscard]] std::uint32_t LeaveCharacter(std::uint32_t arena,
                                                std::uint32_t monster) override;

private:
    NativePalGameIVTextureStorage& game_;
    NativeCameraManagerOwner& cameras_;
    NativeActorControlSession& actor_;
    NativeActorMessageOwner& messages_;
    pal4::InputManager& input_;
    GameRenderState& game_state_;
    NativeNpcManagerOwner& npcs_;
    NativeMonsterManagerOwner& monsters_;
    NativeSceneGobAssets& gobs_;
    NativeTemperatureManagerOwner& temperature_;
    std::uint32_t ui_manager_;
    IUiFrameEventRuntime* ui_frames_;
    NativeArenaCollisionTree* monster_tree_ = nullptr;
    NativeBaseObjectMemory* monster_memory_ = nullptr;
    NativeCombatSystemImage* combat_{};
    NativeGameArenaManagerOwner* arena_{};
    NativeArenaNpcSceneEntry* monster_scene_{};
    pal4::BackgroundMusicPlayer* music_{};
    IBackgroundMusicRuntime* music_runtime_{};
    NativeEffectSceneSession* effects_{};
};

}  // namespace pal4
