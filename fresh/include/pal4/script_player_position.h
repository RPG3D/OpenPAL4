#pragma once

#include "pal4/script_completed_handler_queue.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace pal4 {

struct PlayerPosition {
    float x{};
    float y{};
    float z{};
};
static_assert(sizeof(PlayerPosition) == 12u);

struct PlayerPositionWaitState {
    std::uint32_t vtable_address{};
    std::uint8_t enabled{};
    std::uint8_t wait_mode{};
    std::uint16_t reserved_6{};
    std::uint32_t script_context{};
    std::uint32_t phase{};
    std::uint8_t mode_copy{};
    std::array<std::byte, 3> reserved_11{};
    std::uint32_t name_data{};
    std::uint32_t name_size{};
    std::uint32_t name_capacity{};
};
static_assert(sizeof(PlayerPositionWaitState) == 0x20u);
static_assert(offsetof(PlayerPositionWaitState, phase) == 0x0cu);
static_assert(offsetof(PlayerPositionWaitState, name_data) == 0x14u);

class IScriptPlayerPositionRuntime {
public:
    virtual ~IScriptPlayerPositionRuntime() = default;
    [[nodiscard]] virtual float CurrentY() {
        throw std::logic_error("PAL4 current player Y is unavailable");
    }
    [[nodiscard]] virtual float CurrentZ() {
        throw std::logic_error("PAL4 current player Z is unavailable");
    }
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    virtual void SetPosition(std::uint32_t actor,
                             const PlayerPosition& position) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentPlayer(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::string_view ActorName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void SetNpcPosition(std::uint32_t npc,
                                const PlayerPosition& position,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t SetNpcAction(
        std::uint32_t npc, std::string_view action) = 0;
    [[nodiscard]] virtual float MovementProperty(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    virtual void SelectAndUpdate(std::uint32_t adapter,
                                 std::uint32_t movement,
                                 float first, float second,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool CheckActive(std::uint32_t adapter,
                                           std::uint32_t movement,
                                           float first, float second,
                                           std::uint32_t mode) = 0;
    virtual void UpdateFloorColor() = 0;
    virtual void UpdateGrid(std::uint32_t position,
                            std::uint32_t context) = 0;
    virtual void SetAction(std::uint32_t actor,
                           std::string_view action) = 0;
    [[nodiscard]] virtual PlayerPositionWaitState* Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void Construct(PlayerPositionWaitState& state,
                           std::uint32_t mode,
                           std::uint32_t queue) = 0;
    virtual void AssignName(PlayerPositionWaitState& state,
                            std::string_view name) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            PlayerPositionWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
};

class ScriptPlayerPosition final {
public:
    static constexpr std::uint32_t kVtable = 0x849544u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::uint32_t kMovementOffset = 172u;
    static constexpr std::uint32_t kGridContextOffset = 160u;

    explicit ScriptPlayerPosition(IScriptPlayerPositionRuntime& runtime,
        IScriptCompletedHandlerQueue* completed_handlers = nullptr)
        : runtime_(runtime), completed_handlers_(completed_handlers) {}

    // IDA 0x5e08b0, 0x5e0b10 and 0x5e3600.
    [[nodiscard]] std::uint32_t Set(std::int32_t index,
                                    const PlayerPosition& position);
    [[nodiscard]] std::uint32_t SetCurrent(
        const PlayerPosition& position);
    [[nodiscard]] std::uint32_t SetNpc(
        std::string_view npc_name, const PlayerPosition& position);

private:
    [[nodiscard]] std::uint32_t SetActor(
        std::uint32_t actor, const PlayerPosition& position,
        std::int32_t index, bool current);
    [[nodiscard]] std::uint32_t QueuePosition(
        std::string_view name);
    IScriptPlayerPositionRuntime& runtime_;
    IScriptCompletedHandlerQueue* completed_handlers_{};
};

}  // namespace pal4
