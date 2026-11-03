#pragma once

#include "pal4/game_world_player_scene_loader.h"

namespace pal4 {

class NativeActorControlSceneRuntime;
class NativeActorControlSession;
class NativeArenaActivePlayerSceneBinding;
class NativeActorAnimationStateBinding;
class NativePlayerManagerSession;
class NativeSceneRecordCatalog;
class IPlayerManagerOperationsRuntime;

// Connects the original GameWorld player-scene sequence to owned x86 objects.
class NativeGameWorldPlayerSceneRuntime final
    : public IGameWorldPlayerSceneRuntime {
public:
    NativeGameWorldPlayerSceneRuntime(
        NativePlayerManagerSession& players,
        IPlayerManagerOperationsRuntime& reset_runtime,
        NativeActorAnimationStateBinding& animation,
        NativeArenaActivePlayerSceneBinding& scene_binding,
        NativeActorControlSession& control,
        NativeActorControlSceneRuntime& actor_scene,
        const NativeSceneRecordCatalog& scenes);

    [[nodiscard]] bool Load(std::uint32_t world);

private:
    void ResetPlayerData() override;
    std::uint32_t CurrentPlayer() override;
    void SetPlayerStateFlag(std::uint32_t player,
                            std::uint32_t value) override;
    void SelectPlayerAnimation(std::uint32_t member,
                               std::uint32_t action_name_slot) override;
    void AddPlayerToScene(std::uint32_t world,
                          std::uint32_t player) override;
    std::uint32_t ActorControl() override;
    void SetActor(std::uint32_t control, std::uint32_t player) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::string_view ReadText(std::uint32_t address) override;
    std::span<const GameArenaSceneRecord> SceneRecords() override;
    void SetActorWord28(std::uint32_t control, bool value) override;

    NativePlayerManagerSession& players_;
    IPlayerManagerOperationsRuntime& reset_runtime_;
    NativeActorAnimationStateBinding& animation_;
    NativeArenaActivePlayerSceneBinding& scene_binding_;
    NativeActorControlSession& control_;
    NativeActorControlSceneRuntime& actor_scene_;
    const NativeSceneRecordCatalog& scenes_;
    std::uint32_t prior_state_flag_ = 0u;
};

}  // namespace pal4
