#pragma once

#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_player_turn.h"

#include <memory>
#include <vector>
#include <string>

class asIScriptContext;

namespace pal4 {

class NativeActorAnimationStateBinding;
class NativePlayerManagerSession;
class NativeScriptPlayerActionRuntime;
class NativeScriptPlayerRotationRuntime;
class NativeSceneGobAssets;

// Runs original timed turns against owned player and scene NPC clumps.
class NativeScriptPlayerTurnRuntime final : public IScriptPlayerTurnRuntime,
                                            public INativeScriptFrameWaitRuntime {
public:
    NativeScriptPlayerTurnRuntime(
        NativePlayerManagerSession& players,
        NativeScriptPlayerRotationRuntime& rotation,
        NativeScriptPlayerActionRuntime& action,
        asIScriptContext& context);
    void BindAnimation(NativeActorAnimationStateBinding& binding) noexcept;
    void BindObjects(NativeSceneGobAssets& objects) noexcept { objects_ = &objects; }
    void ClearHandlers();
    [[nodiscard]] bool Tick(float elapsed) override;
    [[nodiscard]] bool Waiting() const noexcept { return !handlers_.empty(); }
    [[nodiscard]] bool BlocksScript() const override {
        for (const auto& handler : handlers_)
            if (handler.mode != 0u) return true;
        return false;
    }

    [[nodiscard]] std::uint32_t PlayerManager() override;
    [[nodiscard]] std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) override;
    [[nodiscard]] std::uint32_t CurrentPlayer(
        std::uint32_t manager) override;
    [[nodiscard]] std::int32_t CurrentPlayerIndex(
        std::uint32_t manager) override;
    [[nodiscard]] std::uint32_t ObjectManager() override;
    [[nodiscard]] std::uint32_t ObjectByName(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] std::uint32_t NpcManager() override;
    [[nodiscard]] std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] std::uint32_t NpcInfoByName(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] float BaselineYaw(std::uint32_t npc_info) override;
    [[nodiscard]] PlayerTurnVector ReadPosition(
        std::uint32_t actor) override;
    void ClampFacingVector(PlayerTurnVector& vector) override;
    [[nodiscard]] float FacingAngleDegrees(
        const PlayerTurnVector& vector) override;
    [[nodiscard]] PlayerTurnWaitState* Allocate(std::uint32_t size) override;
    [[nodiscard]] std::uint32_t ScriptExecutor() override;
    void Construct(PlayerTurnWaitState& state, std::uint8_t wait_mode,
                   std::uint32_t queue) override;
    void Flush(PlayerTurnWaitState& state) override;
    [[nodiscard]] bool AllocateText(PlayerTurnWaitState& state,
                                    std::uint32_t size) override;
    void AssignName(PlayerTurnWaitState& state,
                    std::string_view name) override;
    [[nodiscard]] float ReadYaw(std::uint32_t actor) override;
    [[nodiscard]] float ReadX(std::uint32_t actor) override;
    [[nodiscard]] float ReadZ(std::uint32_t actor) override;
    void SetRotation(std::uint32_t actor, float x, float yaw,
                     float z, std::uint32_t mode) override;
    [[nodiscard]] std::int32_t SetAction(
        std::uint32_t actor, std::string_view action) override;
    [[nodiscard]] std::int32_t SetTimedAction(
        std::uint32_t actor, std::string_view action,
        float blend) override;
    void CleanupWeaponEffects(std::uint32_t actor) override;
    void Close(PlayerTurnWaitState& state) override;
    void AddHandler(std::uint32_t queue,
                    PlayerTurnWaitState* state) override;
    [[nodiscard]] std::uint32_t CleanupName() override;

private:
    [[nodiscard]] std::uint32_t QueueAddress() const noexcept;

    NativePlayerManagerSession& players_;
    NativeScriptPlayerRotationRuntime& rotation_;
    NativeScriptPlayerActionRuntime& action_;
    asIScriptContext& context_;
    NativeActorAnimationStateBinding* binding_{};
    NativeSceneGobAssets* objects_{};
    struct BlendBinding {
        std::uint32_t actor;
        NativeActorAnimationStateBinding* binding;
    };
    std::vector<BlendBinding> blend_bindings_;
    std::unique_ptr<PlayerTurnWaitState> pending_;
    struct Handler {
        std::unique_ptr<PlayerTurnWaitState> state;
        std::string name;
        std::uint8_t mode{};
        bool closed{};
    };
    std::vector<Handler> handlers_;
    std::string pending_name_;
    std::uint8_t pending_mode_{};
    bool pending_closed_{};
};

}  // namespace pal4
