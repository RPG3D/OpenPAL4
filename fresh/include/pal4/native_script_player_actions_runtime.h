#pragma once

#include "pal4/script_player_actions.h"
#include "pal4/script_current_player_visibility.h"

#include <cstdint>
#include <array>

namespace pal4 {

class NativePlayerManagerSession;
class NativeArenaActivePlayerSceneBinding;
class NativeSceneMovementAsset;
class NativeArenaCollisionTree;
class IBaseObjectActivityRuntime;
class INativeScriptPlayerLeaderServices;

class NativeScriptPlayerActionsRuntime final : public IScriptPlayerActionsRuntime,
    public IScriptCurrentPlayerVisibilityRuntime {
public:
    explicit NativeScriptPlayerActionsRuntime(NativePlayerManagerSession& players);
    void BindLeaderServices(INativeScriptPlayerLeaderServices& services) noexcept { leader_services_=&services; }

    void BindScene(std::uint32_t arena, NativeSceneMovementAsset& movement,
        NativeArenaCollisionTree& collision, IBaseObjectActivityRuntime& activity,
        NativeArenaActivePlayerSceneBinding* active_player = nullptr) noexcept;
    void UnbindScene() noexcept;
    std::uint32_t PlayerManager() override;
    std::uint32_t UpdatePlayerStates(std::uint32_t manager,
                                     std::int32_t index) override;
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override;
    std::uint32_t ArenaManager() override;
    std::uint32_t ScriptOnRemove(std::uint32_t arena,
                                 std::uint32_t player) override;
    std::uint32_t ScriptOnAdd(std::uint32_t arena,
                              std::uint32_t player) override;
    void SetStateFlag(std::uint32_t player, std::uint32_t flag) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t MovementAdapter() override;
    std::uint32_t SetClumpProperties(std::uint32_t adapter,
                                     std::uint32_t clump,
                                     std::uint32_t first,
                                     std::uint32_t second,
                                     std::uint32_t third,
                                     std::uint32_t fourth) override;
    std::uint32_t SetClumpRenderCallback(std::uint32_t adapter,
                                         std::uint32_t clump) override;

private:
    NativePlayerManagerSession& players_;
    INativeScriptPlayerLeaderServices* leader_services_{};
    std::uint32_t arena_{};
    NativeSceneMovementAsset* movement_{};
    NativeArenaCollisionTree* collision_{};
    IBaseObjectActivityRuntime* activity_{};
    NativeArenaActivePlayerSceneBinding* active_player_{};
    // Logical adapter slots for the shared script boundary; native owners remain
    // responsible for the real Clump registration and release.
    std::array<std::uint32_t, 2> movement_adapters_{};
};

}  // namespace pal4
