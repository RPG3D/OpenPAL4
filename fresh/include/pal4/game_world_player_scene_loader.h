#pragma once

#include "pal4/game_arena_scene_record.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class IGameWorldPlayerSceneRuntime {
public:
    virtual ~IGameWorldPlayerSceneRuntime() = default;
    virtual void ResetPlayerData() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentPlayer() = 0;
    virtual void SetPlayerStateFlag(std::uint32_t player,
                                    std::uint32_t value) = 0;
    virtual void SelectPlayerAnimation(std::uint32_t member,
                                       std::uint32_t action_name_slot) = 0;
    virtual void AddPlayerToScene(std::uint32_t world,
                                  std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void SetActor(std::uint32_t control,
                          std::uint32_t player) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadText(
        std::uint32_t address) = 0;
    // Borrowed records must remain valid until Load returns.
    [[nodiscard]] virtual std::span<const GameArenaSceneRecord>
        SceneRecords() = 0;
    virtual void SetActorWord28(std::uint32_t control,
                                bool value) = 0;
};

class GameWorldPlayerSceneLoader final {
public:
    static constexpr std::uint32_t kInitialAnimationStringSlot = 0x8a76a4u;

    // IDA 0x519960.
    [[nodiscard]] static bool Load(std::uint32_t world,
                                   IGameWorldPlayerSceneRuntime& runtime);
};

}  // namespace pal4
