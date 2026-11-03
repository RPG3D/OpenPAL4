#pragma once

#include "pal4/native_script_end_action_queue.h"

#include "pal4/native_script_frame_runner.h"
#include "pal4/script_player_action.h"
#include "pal4/script_action_hold_release.h"

#include <memory>
#include <string>
#include <vector>

namespace pal4 {

class NativeAnimatedActorAsset;
class NativePlayerActorAssets;
class NativePlayerAnimationSet;
class NativeActorAnimationStateBinding;
class NativePlayerManagerSession;
class NativeArenaNpcSceneLoader;
class NativePlayerEquipmentSession;

// Drives script actions with the owned player, RenderWare clip and AMF events.
class NativeScriptPlayerActionRuntime final : public IScriptPlayerActionRuntime,
                                             public IScriptActionHoldRuntime,
                                             public INativeScriptFrameWaitRuntime {
public:
    NativeScriptPlayerActionRuntime(
        NativePlayerManagerSession& players,
        const NativeAnimatedActorAsset& actor,
        asIScriptContext& context);
    void BindAnimation(NativeActorAnimationStateBinding& binding) noexcept;
    // Persistent owners outlive all handlers and scene borrowers.
    void BindPlayers(NativePlayerActorAssets& assets,
                     NativePlayerAnimationSet& animations) noexcept;
    // The application unbinds before clearing scene equipment resources.
    void BindEquipment(NativePlayerEquipmentSession* equipment) noexcept { equipment_=equipment; }
    void BindNpcScene(NativeArenaNpcSceneLoader& npcs) noexcept {
        npcs_ = &npcs;
    }
    void UnbindNpcScene() noexcept;
    // Scene frames own animation time; isolated wait fixtures can drive it here.
    void UsePlayerWorldClock(bool enabled) noexcept { player_world_clock_ = enabled; }
    void UseNpcWorldClock(bool enabled) noexcept { npc_world_clock_ = enabled; }
    [[nodiscard]] bool UsesPlayerWorldClock() const noexcept { return player_world_clock_; }
    [[nodiscard]] bool UsesNpcWorldClock() const noexcept { return npc_world_clock_; }
    [[nodiscard]] bool UsesAnimationWorldClock(std::uint32_t actor) const;
    void ClearHandlers();
    [[nodiscard]] IScriptCompletedHandlerQueue& CompletedHandlers() noexcept {
        return *end_actions_;
    }
    [[nodiscard]] std::shared_ptr<NativeScriptEndActionQueue> WaitQueue() const noexcept {
        return end_actions_;
    }
    [[nodiscard]] bool Tick(float elapsed) override;
    [[nodiscard]] bool Waiting() const noexcept {
        return !actions_.empty() || !end_actions_->Empty();
    }
    [[nodiscard]] bool BlocksScript() const override;
    [[nodiscard]] std::size_t ActiveActionCount() const noexcept { return actions_.size(); }
    [[nodiscard]] bool HoldEventSeen() const noexcept;
    // Borrows the validated per-role binding; persistent animation owner retains it.
    [[nodiscard]] NativeActorAnimationStateBinding& AnimationBinding(std::uint32_t actor) const;
    [[nodiscard]] std::string_view CurrentRecordName(std::uint32_t actor);

    [[nodiscard]] std::uint32_t PlayerManager() override;
    [[nodiscard]] std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t EmptyStringAddress() override;
    [[nodiscard]] std::uint32_t FindNpc(
        std::uint32_t manager, std::uint32_t name) override;
    void ClearAnimationFlag(
        std::uint32_t component, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t UpdatePlayerEffects(
        std::uint32_t player) override;
    [[nodiscard]] std::uint32_t UpdateNpcEffects(
        std::uint32_t npc) override;
    [[nodiscard]] std::uint32_t CurrentPlayer(std::uint32_t manager) override;
    [[nodiscard]] std::uint32_t NpcManager() override;
    [[nodiscard]] std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] std::uint32_t NpcInfoByName(std::uint32_t manager,std::string_view name);
    [[nodiscard]] std::int32_t SetAction(
        std::uint32_t actor, std::string_view action) override;
    void SetFlag(std::uint32_t actor, std::uint32_t flag) override;
    void ClearFlag(std::uint32_t actor, std::uint32_t flag) override;
    [[nodiscard]] bool GetFlag(std::uint32_t actor) override;
    [[nodiscard]] std::string_view ActionName(std::uint32_t actor) override;
    void ApplyEquipmentVisuals(std::uint32_t actor) override;
    void ClearWeaponEffects(std::uint32_t actor) override;
    void UpdateEffectList(std::uint32_t actor) override;
    void UpdateEffectList2(std::uint32_t actor) override;
    void UpdateEffectList3(std::uint32_t actor) override;
    void UpdateEffectList4(std::uint32_t actor) override;
    void CleanupWeaponEffects(std::uint32_t actor) override;
    [[nodiscard]] std::string_view FormatActorName(std::uint32_t actor) override;
    [[nodiscard]] PlayerActionWaitState* AllocateAction(
        std::uint32_t size) override;
    [[nodiscard]] PlayerEndActionWaitState* AllocateEnd(
        std::uint32_t size) override;
    [[nodiscard]] std::uint32_t ScriptExecutor() override;
    void ConstructAction(PlayerActionWaitState& state, std::uint8_t mode,
                         std::uint32_t queue) override;
    void ConstructEnd(PlayerEndActionWaitState& state, std::uint32_t mode,
                      std::uint32_t queue) override;
    void AssignName(PlayerActionWaitState& state, std::string_view name) override;
    void FlushAction(PlayerActionWaitState& state) override;
    void FlushEnd(PlayerEndActionWaitState& state) override;
    void AddAction(std::uint32_t queue, PlayerActionWaitState* state) override;
    [[nodiscard]] std::uint32_t AddEnd(
        std::uint32_t queue, PlayerEndActionWaitState* state) override;
    [[nodiscard]] std::uint32_t CleanupName() override;
    void CloseAction(PlayerActionWaitState& state) override;
    void CloseEnd(PlayerEndActionWaitState& state) override;

private:
    void RequireActor(std::uint32_t actor) const;
    [[nodiscard]] std::uint32_t QueueAddress() const noexcept;
    [[nodiscard]] std::size_t ActorIndex(std::uint32_t actor) const;
    [[nodiscard]] NativeActorAnimationStateBinding& PlayerBinding(std::uint32_t actor) const;
    void AdvanceAnimation(std::uint32_t actor, float elapsed);
    struct ActionHandler {
        std::unique_ptr<PlayerActionWaitState> state;
        std::string name;
        std::uint8_t wait_mode{};
        bool closed{};
    };

    NativePlayerManagerSession& players_;
    const NativeAnimatedActorAsset& actor_;
    asIScriptContext& context_;
    NativeActorAnimationStateBinding* binding_{};
    NativePlayerActorAssets* assets_{};
    NativePlayerAnimationSet* animations_{};
    NativeArenaNpcSceneLoader* npcs_{};
    NativePlayerEquipmentSession* equipment_{};
    bool player_world_clock_{};
    bool npc_world_clock_{};
    std::unique_ptr<PlayerActionWaitState> pending_;
    std::vector<ActionHandler> actions_;
    std::unique_ptr<PlayerEndActionWaitState> pending_end_;
    std::shared_ptr<NativeScriptEndActionQueue> end_actions_ =
        std::make_shared<NativeScriptEndActionQueue>();
    std::string handler_name_;
    std::string actor_name_;
    std::uint8_t wait_mode_{};
};

}  // namespace pal4
