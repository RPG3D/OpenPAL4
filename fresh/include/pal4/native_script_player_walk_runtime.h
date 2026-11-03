#pragma once

#include "pal4/native_script_frame_runner.h"
#include "pal4/script_player_walk.h"

#include <memory>
#include <string>
#include <vector>

namespace pal4 {

class NativeArenaCollisionTree;
class NativeBaseObjectMemory;
class NativeGameArenaManagerOwner;
class NativePlayerManagerSession;
class NativeScriptPlayerActionRuntime;
class NativeArenaNpcSceneLoader;

// Binds the script walk command to the active player's collision path.
class NativeScriptPlayerWalkRuntime final : public IScriptPlayerWalkRuntime,
                                            public INativeScriptFrameWaitRuntime {
public:
    NativeScriptPlayerWalkRuntime(
        NativePlayerManagerSession& players,
        NativeGameArenaManagerOwner& arena,
        NativeArenaCollisionTree& tree,
        NativeBaseObjectMemory& memory,
        NativeScriptPlayerActionRuntime& action,
        asIScriptContext& context);
    void BindNpcScene(NativeArenaNpcSceneLoader& npcs) noexcept {
        npcs_ = &npcs;
    }
    void UnbindNpcScene() noexcept;

    void ClearHandlers();
    [[nodiscard]] bool Tick(float elapsed) override;
    [[nodiscard]] bool BlocksScript() const override;
    [[nodiscard]] bool Waiting() const noexcept { return !handlers_.empty(); }

    [[nodiscard]] std::uint32_t PlayerManager() override;
    [[nodiscard]] std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) override;
    [[nodiscard]] std::uint32_t CurrentPlayer(std::uint32_t manager) override;
    [[nodiscard]] std::string_view ActorName(std::uint32_t actor) override;
    [[nodiscard]] std::uint32_t NpcManager() override;
    [[nodiscard]] std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] PlayerWalkPoint ReadPosition(std::uint32_t actor) override;
    [[nodiscard]] PlayerWalkPoint ConvertPoint(
        const PlayerWalkPoint& point) override;
    [[nodiscard]] std::uint32_t ArenaManager() override;
    [[nodiscard]] std::uint32_t PlanRoute(
        std::uint32_t route_source, PlayerWalkPoint& target) override;
    [[nodiscard]] bool StartMovement(
        std::uint32_t movement, const PlayerWalkPoint& from,
        const PlayerWalkPoint& to, std::uint32_t route,
        std::uint32_t mode) override;
    void SetMovementSpeed(std::uint32_t movement, float speed) override;
    void SetMovementStatus(std::uint32_t actor,
                           std::uint32_t value) override;
    [[nodiscard]] std::string_view ActionName(std::uint32_t actor) override;
    [[nodiscard]] std::int32_t SetAction(
        std::uint32_t actor, std::string_view action) override;
    [[nodiscard]] PlayerWalkWaitState* Allocate(std::uint32_t size) override;
    [[nodiscard]] std::uint32_t ScriptExecutor() override;
    void Construct(PlayerWalkWaitState& state, std::uint8_t wait_mode,
                   std::uint32_t queue) override;
    void AssignName(PlayerWalkWaitState& state,
                    std::string_view name) override;
    void CleanupWeaponEffects(std::uint32_t actor) override;
    void Flush(PlayerWalkWaitState& state) override;
    void AddHandler(std::uint32_t queue,
                    PlayerWalkWaitState* state) override;
    [[nodiscard]] std::uint32_t CleanupName() override;
    [[nodiscard]] std::uint32_t ShowPathError() override;
    [[nodiscard]] bool MovementActive(std::uint32_t movement) override;
    void StopMovement(std::uint32_t movement) override;
    void Close(PlayerWalkWaitState& state) override;

private:
    [[nodiscard]] bool IsPlayer(std::uint32_t actor) const noexcept;
    [[nodiscard]] std::uint32_t QueueAddress() const noexcept;
    void RequireMovement(std::uint32_t movement) const;

    NativePlayerManagerSession& players_;
    NativeGameArenaManagerOwner& arena_;
    NativeArenaCollisionTree& tree_;
    NativeBaseObjectMemory& memory_;
    NativeScriptPlayerActionRuntime& action_;
    NativeArenaNpcSceneLoader* npcs_{};
    asIScriptContext& context_;
    struct Handler {
        std::unique_ptr<PlayerWalkWaitState> state;
        std::uint8_t wait_mode{};
        bool closed{};
    };
    std::unique_ptr<PlayerWalkWaitState> pending_;
    std::vector<Handler> handlers_;
    std::string pending_name_;
    std::uint8_t pending_wait_mode_{};
};

}  // namespace pal4
