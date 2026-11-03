#pragma once

#include "pal4/script_player_rotation.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

class NativePlayerManagerSession;
class NativePlayerTransformRuntime;
class INativeNpcPositionServices;

class NativeScriptPlayerRotationRuntime final
    : public IScriptPlayerRotationRuntime {
public:
    NativeScriptPlayerRotationRuntime(
        NativePlayerManagerSession& players,
        NativePlayerTransformRuntime& transforms);
    void BindNpcs(INativeNpcPositionServices* services) noexcept { npcs_=services; }

    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override;
    std::uint32_t CurrentPlayer(std::uint32_t manager) override;
    std::uint32_t NpcManager() override;
    std::uint32_t NpcByName(std::uint32_t manager,
                            std::string_view name) override;
    std::string_view ActorName(std::uint32_t actor) override;
    float ReadX(std::uint32_t actor) override;
    float ReadZ(std::uint32_t actor) override;
    void SetRotation(std::uint32_t actor, const PlayerRotation& rotation,
                     std::uint32_t mode) override;
    PlayerRotationWaitState* Allocate(std::uint32_t size) override;
    std::uint32_t ScriptExecutor() override;
    void Construct(PlayerRotationWaitState& state, std::uint32_t mode,
                   std::uint32_t queue) override;
    bool AllocateText(PlayerRotationWaitState& state,
                      std::uint32_t size) override;
    void AssignName(PlayerRotationWaitState& state,
                    std::string_view name) override;
    void Close(PlayerRotationWaitState& state) override;
    void AddHandler(std::uint32_t queue,
                    PlayerRotationWaitState* state) override;
    std::uint32_t CleanupName() override;

    void ProcessWaitHandlers();
    [[nodiscard]] std::size_t WaitHandlerCount() const noexcept {
        return handlers_.size();
    }
    [[nodiscard]] std::string_view LastWaitHandlerName() const noexcept {
        return handlers_.empty() ? std::string_view{} :
            std::string_view(handlers_.back()->name);
    }

private:
    struct WaitRecord {
        PlayerRotationWaitState state{};
        std::string name;
    };

    [[nodiscard]] std::size_t ActorIndex(std::uint32_t actor) const;
    NativePlayerManagerSession& players_;
    NativePlayerTransformRuntime& transforms_;
    INativeNpcPositionServices* npcs_{};
    std::unique_ptr<WaitRecord> pending_wait_;
    std::vector<std::unique_ptr<WaitRecord>> handlers_;
};

}  // namespace pal4
