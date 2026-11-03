#pragma once

#include "pal4/script_completed_handler_queue.h"

#include "pal4/script_player_position.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayerRotation {
    float x{};
    float yaw{};
    float z{};
};
static_assert(sizeof(PlayerRotation) == 12u);

using PlayerRotationWaitState = PlayerPositionWaitState;
static_assert(sizeof(PlayerRotationWaitState) == 0x20u);

class IScriptPlayerRotationRuntime {
public:
    virtual ~IScriptPlayerRotationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentPlayer(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::string_view ActorName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ReadX(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ReadZ(std::uint32_t actor) = 0;
    virtual void SetRotation(std::uint32_t actor,
                             const PlayerRotation& rotation,
                             std::uint32_t mode) = 0;
    [[nodiscard]] virtual PlayerRotationWaitState* Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void Construct(PlayerRotationWaitState& state,
                           std::uint32_t mode,
                           std::uint32_t queue) = 0;
    [[nodiscard]] virtual bool AllocateText(PlayerRotationWaitState& state,
                                            std::uint32_t size) = 0;
    virtual void AssignName(PlayerRotationWaitState& state,
                            std::string_view name) = 0;
    virtual void Close(PlayerRotationWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            PlayerRotationWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
};

class ScriptPlayerRotation final {
public:
    static constexpr std::uint32_t kVtable = 0x849550u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptPlayerRotation(IScriptPlayerRotationRuntime& runtime,
        IScriptCompletedHandlerQueue* completed_handlers = nullptr)
        : runtime_(runtime), completed_handlers_(completed_handlers) {}

    // IDA 0x5e0d50, 0x5e1030, 0x5e11f0, 0x5e37f0 and 0x5e6980.
    [[nodiscard]] std::uint32_t SetDirection(
        std::int32_t index, std::string_view direction);
    [[nodiscard]] std::uint32_t SetAngle(std::int32_t index, float angle);
    [[nodiscard]] std::uint32_t SetCurrentAngle(float angle);
    [[nodiscard]] std::uint32_t SetNpcDirection(
        std::string_view npc_name, std::string_view direction);
    [[nodiscard]] std::uint32_t SetNpcAngle(
        std::string_view npc_name, float angle);

private:
    [[nodiscard]] std::uint32_t SetActor(
        std::uint32_t actor, float angle, std::int32_t index,
        bool current, bool npc, std::string_view npc_name);

    IScriptPlayerRotationRuntime& runtime_;
    IScriptCompletedHandlerQueue* completed_handlers_{};
};

}  // namespace pal4
