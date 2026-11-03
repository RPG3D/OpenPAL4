#pragma once

#include "pal4/script_player_position.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

class NativePlayerManagerSession;
class NativePlayerTransformRuntime;
class NativeActorAnimationStateBinding;
class NativePlayerAnimationSet;
class IScriptCameraCallbackRuntime;
class INativeNpcPositionServices;

class INativeScriptPlayerMovement {
public:
    virtual ~INativeScriptPlayerMovement() = default;
    [[nodiscard]] virtual std::uint32_t Address() const noexcept = 0;
    virtual void SelectAndUpdate(std::uint32_t movement, float above,
                                 float below, std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool CheckActive(std::uint32_t movement, float above,
                                           float below, std::uint32_t mode) = 0;
};

class INativeScriptFloorColor {
public:
    virtual ~INativeScriptFloorColor() = default;
    virtual void Update() = 0;
};

class INativeScriptGrid {
public:
    virtual ~INativeScriptGrid() = default;
    virtual void UpdateGrid(std::uint32_t position,
                            std::uint32_t context) = 0;
};

// Native current/indexed player position binding; borrows scene animation owners.
class NativeScriptPlayerPositionRuntime final
    : public IScriptPlayerPositionRuntime {
public:
    NativeScriptPlayerPositionRuntime(
        NativePlayerManagerSession& players,
        NativePlayerTransformRuntime& transforms);

    void BindAnimation(NativeActorAnimationStateBinding& animation) noexcept;
    void BindAnimations(NativePlayerAnimationSet& animations) noexcept;
    void BindMovement(INativeScriptPlayerMovement& movement) noexcept;
    void BindFloorColor(INativeScriptFloorColor& floor_color) noexcept;
    void BindGrid(INativeScriptGrid& grid) noexcept;
    void BindCamera(IScriptCameraCallbackRuntime& camera) noexcept;
    void BindNpcs(INativeNpcPositionServices* services) noexcept { npcs_=services; }

    [[nodiscard]] float CurrentY() override;
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override;
    // Player vtable+60 (43EDD0), shared by script and gameplay movement.
    void SetPosition(std::uint32_t actor,
                     const PlayerPosition& position) override;
    std::uint32_t CurrentPlayer(std::uint32_t manager) override;
    std::string_view ActorName(std::uint32_t actor) override;
    std::uint32_t NpcManager() override;
    std::uint32_t NpcByName(std::uint32_t manager,
                            std::string_view name) override;
    void SetNpcPosition(std::uint32_t npc,
                        const PlayerPosition& position,
                        std::uint32_t mode) override;
    std::int32_t SetNpcAction(std::uint32_t npc,
                              std::string_view action) override;
    float MovementProperty(std::uint32_t actor) override;
    std::uint32_t MovementAdapter() override;
    void SelectAndUpdate(std::uint32_t adapter,
                         std::uint32_t movement, float first,
                         float second, std::uint32_t mode) override;
    bool CheckActive(std::uint32_t adapter, std::uint32_t movement,
                     float first, float second,
                     std::uint32_t mode) override;
    void UpdateFloorColor() override;
    void UpdateGrid(std::uint32_t position,
                    std::uint32_t context) override;
    void SetAction(std::uint32_t actor,
                   std::string_view action) override;
    PlayerPositionWaitState* Allocate(std::uint32_t size) override;
    std::uint32_t ScriptExecutor() override;
    void Construct(PlayerPositionWaitState& state,
                   std::uint32_t mode,
                   std::uint32_t queue) override;
    void AssignName(PlayerPositionWaitState& state,
                    std::string_view name) override;
    void AddHandler(std::uint32_t queue,
                    PlayerPositionWaitState* state) override;
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
        PlayerPositionWaitState state{};
        std::string name;
    };

    NativePlayerManagerSession& players_;
    NativePlayerTransformRuntime& transforms_;
    NativeActorAnimationStateBinding* animation_ = nullptr;
    NativePlayerAnimationSet* animations_ = nullptr;
    [[nodiscard]] std::size_t ActorIndex(std::uint32_t actor) const;
    INativeScriptPlayerMovement* movement_ = nullptr;
    INativeScriptFloorColor* floor_color_ = nullptr;
    INativeScriptGrid* grid_ = nullptr;
    IScriptCameraCallbackRuntime* camera_ = nullptr;
    INativeNpcPositionServices* npcs_ = nullptr;
    std::unique_ptr<WaitRecord> pending_wait_;
    std::vector<std::unique_ptr<WaitRecord>> handlers_;
};

}  // namespace pal4
