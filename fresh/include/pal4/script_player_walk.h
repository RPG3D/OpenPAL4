#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct PlayerWalkPoint {
    float x{};
    float y{};
    float z{};
};

struct PlayerWalkWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 8> base{};
    std::uint32_t phase{};
    std::array<std::byte, 16> name_buffer{};
    std::uint32_t actor{};
};
static_assert(sizeof(PlayerWalkWaitState) == 0x24u);

class IScriptPlayerWalkRuntime {
public:
    virtual ~IScriptPlayerWalkRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentPlayer(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::string_view ActorName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual PlayerWalkPoint ReadPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual PlayerWalkPoint ConvertPoint(
        const PlayerWalkPoint& point) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlanRoute(
        std::uint32_t route_source,
        PlayerWalkPoint& target) = 0;
    [[nodiscard]] virtual bool StartMovement(
        std::uint32_t movement, const PlayerWalkPoint& from,
        const PlayerWalkPoint& to, std::uint32_t route,
        std::uint32_t mode) = 0;
    virtual void SetMovementSpeed(std::uint32_t movement, float speed) = 0;
    virtual void SetMovementStatus(std::uint32_t actor,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ActionName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t SetAction(
        std::uint32_t actor, std::string_view action) = 0;
    [[nodiscard]] virtual PlayerWalkWaitState* Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void Construct(PlayerWalkWaitState& state,
                           std::uint8_t wait_mode,
                           std::uint32_t queue) = 0;
    virtual void AssignName(PlayerWalkWaitState& state,
                            std::string_view name) = 0;
    virtual void CleanupWeaponEffects(std::uint32_t actor) = 0;
    virtual void Flush(PlayerWalkWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            PlayerWalkWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
    [[nodiscard]] virtual std::uint32_t ShowPathError() = 0;
    [[nodiscard]] virtual bool MovementActive(std::uint32_t movement) = 0;
    virtual void StopMovement(std::uint32_t movement) = 0;
    virtual void Close(PlayerWalkWaitState& state) = 0;
};

class ScriptPlayerWalk final {
public:
    static constexpr std::uint32_t kVtable = 0x84957cu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::uint32_t kMovementOffset = 1380u;
    static constexpr std::uint32_t kRouteSourceOffset = 404u;

    explicit ScriptPlayerWalk(IScriptPlayerWalkRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5e1cc0, 0x5e1f60, 0x5e1fc0, 0x5e1fe0,
    // 0x5e2280, 0x5e2520, 0x5e27e0, 0x5e2a90, 0x5e2d40,
    // 0x5e61c0, 0x5e6350 and 0x5e64f0.
    [[nodiscard]] std::uint32_t WalkTo(std::int32_t index,
                                       const PlayerWalkPoint& target,
                                       std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t RunTo(std::int32_t index,
                                      const PlayerWalkPoint& target,
                                      std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t WalkCurrent(
        const PlayerWalkPoint& target, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t WalkBack(
        std::int32_t index, const PlayerWalkPoint& target,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t WalkNpc(
        std::string_view npc_name, const PlayerWalkPoint& target,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t RunNpc(
        std::string_view npc_name, const PlayerWalkPoint& target,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t BackNpc(
        std::string_view npc_name, const PlayerWalkPoint& target,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t EndMoveIndexed(std::int32_t index);
    [[nodiscard]] std::uint32_t EndMoveCurrent();
    [[nodiscard]] std::uint32_t EndMoveNpc(std::string_view npc_name);
    void Update(PlayerWalkWaitState& state);
    void Stop(PlayerWalkWaitState& state);

private:
    enum class NameSource { Index, CurrentActor, Npc };

    [[nodiscard]] std::uint32_t MoveNpc(
        std::string_view npc_name, const PlayerWalkPoint& target,
        std::uint8_t wait_mode, float speed,
        std::string_view action, std::uint32_t movement_status);
    [[nodiscard]] std::uint32_t MoveTo(
        std::int32_t index, const PlayerWalkPoint& target,
        std::uint8_t wait_mode, float speed,
        std::string_view action, std::uint32_t movement_status);
    [[nodiscard]] std::uint32_t MoveActor(
        std::uint32_t actor, std::int32_t index,
        NameSource name_source, std::string_view npc_name,
        const PlayerWalkPoint& target, std::uint8_t wait_mode,
        float speed, std::string_view action,
        std::uint32_t movement_status);
    [[nodiscard]] std::uint32_t EndActor(
        std::uint32_t actor, std::int32_t index,
        NameSource name_source, std::string_view npc_name);
    [[nodiscard]] std::string HandlerName(
        std::uint32_t actor, std::int32_t index,
        NameSource name_source, std::string_view npc_name);
    [[nodiscard]] std::uint32_t QueueHandler(
        std::uint32_t actor, std::string_view name,
        std::uint8_t wait_mode);
    IScriptPlayerWalkRuntime& runtime_;
};

}  // namespace pal4
